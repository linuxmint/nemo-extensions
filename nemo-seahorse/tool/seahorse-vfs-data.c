/*
 * Seahorse
 *
 * Copyright (C) 2004 Stefan Walter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <gio/gio.h>
#include <gtk/gtk.h>

#include "seahorse-vfs-data.h"
#include "seahorse-util.h"

#include <gpgme.h>

#define PROGRESS_BLOCK  16 * 1024

/* The current/last VFS operation */
typedef enum _VfsAsyncOp
{
    VFS_OP_NONE,
    VFS_OP_OPENING,
    VFS_OP_READING,
    VFS_OP_WRITING,
    VFS_OP_FLUSHING,
    VFS_OP_SEEKING
}
VfsAsyncOp;

/* The state of the VFS system */
typedef enum _VfsAsyncState
{
    VFS_ASYNC_PROCESSING,
    VFS_ASYNC_CANCELLED,
    VFS_ASYNC_READY
}
VfsAsyncState;

/* A handle for VFS work */
typedef struct _VfsAsyncHandle {

    gpgme_data_t gdata;             /* A pointer to the outside gpgme_data_t handle */

    GFile *file;                    /* The file we're operating on */
    GCancellable *cancellable;      /* For cancelling any concurrent operation */
    gboolean writer;                /* Whether a writer or a reader */
    GInputStream *istream;          /* Stream for reading from */
    GOutputStream *ostream;         /* Stream for writing to */

    VfsAsyncOp    operation;        /* The last/current operation */
    VfsAsyncState state;            /* State of the last/current operation */

    GError *error;                  /* Result of the current operation */
    gpointer buffer;                /* Current operation's buffer */
    goffset processed;              /* Number of bytes processed in current op */

    goffset last;                   /* Last update sent about number of bytes */
    goffset total;                  /* Total number of bytes read or written */

    SeahorseVfsProgressCb progcb;   /* Progress callback */
    gpointer userdata;              /* User data for progress callback */
} VfsAsyncHandle;

static void vfs_data_cancel(VfsAsyncHandle *ah);

/* Waits for a given VFS operation to complete without halting the UI */
static gboolean
vfs_data_wait_results(VfsAsyncHandle* ah, gboolean errors)
{
    VfsAsyncOp op;
    gint code;

    seahorse_util_wait_until (ah->state != VFS_ASYNC_PROCESSING);

    op = ah->operation;
    ah->operation = VFS_OP_NONE;

    /* Cancelling looks like an error to our caller */
    if (ah->state == VFS_ASYNC_CANCELLED) {
        errno = 0;
        return FALSE;
    }

    g_assert (ah->state == VFS_ASYNC_READY);

    /* There was no operation going on, result codes are not relevant */
    if (op == VFS_OP_NONE)
        return TRUE;

    /* No error result */
    if (!ah->error) {
	    return TRUE;
    }

    code = -1;
    if (ah->error->domain == G_IO_ERROR)
        code = ah->error->code;

    if (code == G_IO_ERROR_CANCELLED) {
        vfs_data_cancel (ah);
        errno = 0;
        return FALSE;
    }

    /* Check for error codes */
    if(errors) {
        switch(code) {
        #define GIO_TO_SYS_ERR(v, s)    \
            case v: errno = s; break;
        GIO_TO_SYS_ERR (G_IO_ERROR_FAILED, EIO);
        GIO_TO_SYS_ERR (G_IO_ERROR_NOT_FOUND, ENOENT);
        GIO_TO_SYS_ERR (G_IO_ERROR_EXISTS, EEXIST);
        GIO_TO_SYS_ERR (G_IO_ERROR_IS_DIRECTORY, EISDIR);
        GIO_TO_SYS_ERR (G_IO_ERROR_NOT_DIRECTORY, ENOTDIR);
        GIO_TO_SYS_ERR (G_IO_ERROR_NOT_EMPTY, ENOTEMPTY);
        GIO_TO_SYS_ERR (G_IO_ERROR_NOT_REGULAR_FILE, EINVAL);
        GIO_TO_SYS_ERR (G_IO_ERROR_NOT_SYMBOLIC_LINK, EINVAL);
        GIO_TO_SYS_ERR (G_IO_ERROR_NOT_MOUNTABLE_FILE, EINVAL);
        GIO_TO_SYS_ERR (G_IO_ERROR_FILENAME_TOO_LONG, ENAMETOOLONG);
        GIO_TO_SYS_ERR (G_IO_ERROR_INVALID_FILENAME, EINVAL);
        GIO_TO_SYS_ERR (G_IO_ERROR_TOO_MANY_LINKS, EMLINK);
        GIO_TO_SYS_ERR (G_IO_ERROR_NO_SPACE, ENOSPC);
        GIO_TO_SYS_ERR (G_IO_ERROR_INVALID_ARGUMENT, EINVAL);
        GIO_TO_SYS_ERR (G_IO_ERROR_PERMISSION_DENIED, EACCES);
        GIO_TO_SYS_ERR (G_IO_ERROR_NOT_SUPPORTED, EOPNOTSUPP);
        GIO_TO_SYS_ERR (G_IO_ERROR_NOT_MOUNTED, EIO);
        GIO_TO_SYS_ERR (G_IO_ERROR_ALREADY_MOUNTED, EIO);
        GIO_TO_SYS_ERR (G_IO_ERROR_CLOSED, EINVAL);
        GIO_TO_SYS_ERR (G_IO_ERROR_CANCELLED, EIO);
        GIO_TO_SYS_ERR (G_IO_ERROR_PENDING, EAGAIN);
        GIO_TO_SYS_ERR (G_IO_ERROR_READ_ONLY, EPERM);
        GIO_TO_SYS_ERR (G_IO_ERROR_CANT_CREATE_BACKUP, EIO);
        GIO_TO_SYS_ERR (G_IO_ERROR_WRONG_ETAG, EIO);
        GIO_TO_SYS_ERR (G_IO_ERROR_TIMED_OUT, EIO);
        GIO_TO_SYS_ERR (G_IO_ERROR_WOULD_RECURSE, EIO);
        GIO_TO_SYS_ERR (G_IO_ERROR_BUSY, EBUSY);
        GIO_TO_SYS_ERR (G_IO_ERROR_WOULD_BLOCK, EWOULDBLOCK);
        GIO_TO_SYS_ERR (G_IO_ERROR_HOST_NOT_FOUND, ENOENT);
        GIO_TO_SYS_ERR (G_IO_ERROR_WOULD_MERGE, EIO);
        GIO_TO_SYS_ERR (G_IO_ERROR_FAILED_HANDLED, EIO);
        default:
            errno = EIO;
            break;
        };
    }

    /* When errors on open we look cancelled */
    if(op == VFS_OP_OPENING)
        ah->state = VFS_ASYNC_CANCELLED;

    return FALSE;
}

/* Called when a file open completes */
static void
vfs_data_open_done (GObject *source, GAsyncResult *res, gpointer callback_data)
{
    VfsAsyncHandle* ah = (VfsAsyncHandle*)callback_data;

    if(ah->state == VFS_ASYNC_PROCESSING) {
        g_assert (ah->operation == VFS_OP_OPENING);
	g_clear_error (&ah->error);

        if (ah->writer)
            ah->ostream = G_OUTPUT_STREAM (g_file_replace_finish (ah->file, res, &ah->error));
        else
            ah->istream = G_INPUT_STREAM (g_file_read_finish (ah->file, res, &ah->error));

        ah->state = VFS_ASYNC_READY;
        ah->total = 0;
        ah->last = 0;
    }
}

/* Open the given handle */
static void
vfs_data_open_helper (VfsAsyncHandle *ah)
{
    g_assert (ah->file != NULL);
    g_assert (ah->state == VFS_ASYNC_READY);

    ah->state = VFS_ASYNC_PROCESSING;
    ah->operation = VFS_OP_OPENING;

    /* Note that we always overwrite the file */
    if (ah->writer)
        g_file_replace_async (ah->file, NULL, FALSE, G_FILE_CREATE_NONE, G_PRIORITY_DEFAULT, ah->cancellable, vfs_data_open_done, ah);
    else
        g_file_read_async (ah->file, G_PRIORITY_DEFAULT, ah->cancellable, vfs_data_open_done, ah);
}

/* Open the given URI */
static VfsAsyncHandle*
vfs_data_open (GFile *file, gboolean write, gboolean delayed)
{
    VfsAsyncHandle* ah;

    /* We only support delayed opening of write files */
    g_assert (write || !delayed);

    ah = g_new0 (VfsAsyncHandle, 1);
    ah->cancellable = g_cancellable_new ();
    ah->state = VFS_ASYNC_READY;
    ah->operation = VFS_OP_NONE;
    ah->file = file;
    ah->writer = write;
    g_object_ref (file);

    /* Open the file right here and now if requested */
    if (!delayed)
        vfs_data_open_helper (ah);

    return ah;
}

/* Called when a read completes */
static void
vfs_data_read_done (GObject *source, GAsyncResult *res, gpointer callback_data)
{
    VfsAsyncHandle* ah = (VfsAsyncHandle*)callback_data;

    if (ah->state == VFS_ASYNC_PROCESSING) {
        g_assert (ah->operation == VFS_OP_READING);
	g_clear_error (&ah->error);

        ah->processed = g_input_stream_read_finish (ah->istream, res, &ah->error);
        ah->state = VFS_ASYNC_READY;
        ah->total += ah->processed;

        /* Call progress callback if setup */
        if (ah->progcb && ah->total >= ah->last + PROGRESS_BLOCK)
            (ah->progcb) (ah->gdata, ah->total, ah->userdata);
    }
}

/* Called by gpgme to read data */
static ssize_t
vfs_data_read (void *handle, void *buffer, size_t size)
{
    VfsAsyncHandle* ah = (VfsAsyncHandle*)handle;
    ssize_t sz = 0;

    /* Just in case we have an operation, like open */
    if (!vfs_data_wait_results(ah, TRUE))
        return -1;

    g_assert (ah->state == VFS_ASYNC_READY);

    /* Start async operation */
    ah->buffer = buffer;
    ah->state = VFS_ASYNC_PROCESSING;
    ah->operation = VFS_OP_READING;
    g_input_stream_read_async (ah->istream, buffer, size, G_PRIORITY_DEFAULT, ah->cancellable, vfs_data_read_done, ah);

    /* Wait for it */
    if (!vfs_data_wait_results (ah, TRUE))
        return -1;

    /* Return results */
    sz = (ssize_t)ah->processed;
    ah->state = VFS_ASYNC_READY;

    ah->buffer = NULL;
    ah->processed = 0;

    return sz;
}

/* Called when a write completes */
static void
vfs_data_write_done (GObject *source, GAsyncResult *res, gpointer callback_data)
{
    VfsAsyncHandle* ah = (VfsAsyncHandle*)callback_data;

    if(ah->state == VFS_ASYNC_PROCESSING) {
        g_assert (ah->operation == VFS_OP_WRITING);
	g_clear_error (&ah->error);

        ah->processed = g_output_stream_write_finish (ah->ostream, res, &ah->error);
        ah->state = VFS_ASYNC_READY;
        ah->total += ah->processed;

        /* Call progress callback if setup */
        if (ah->progcb && ah->total >= ah->last + PROGRESS_BLOCK)
            (ah->progcb) (ah->gdata, ah->total, ah->userdata);
    }
}

/* Called when a flush completes */
static void
vfs_data_flush_done (GObject *source, GAsyncResult *res, gpointer callback_data)
{
    VfsAsyncHandle* ah = (VfsAsyncHandle*)callback_data;

    if(ah->state == VFS_ASYNC_PROCESSING) {
        g_assert (ah->operation == VFS_OP_FLUSHING);
	g_clear_error (&ah->error);

        g_output_stream_flush_finish (ah->ostream, res, &ah->error);
        ah->state = VFS_ASYNC_READY;
    }
}

/* Called by gpgme to write data */
static ssize_t
vfs_data_write (void *handle, const void *buffer, size_t size)
{
    VfsAsyncHandle* ah = (VfsAsyncHandle*)handle;
    ssize_t sz = 0;

    /* If the file isn't open yet, then do that now */
    if (!ah->ostream && ah->state == VFS_ASYNC_READY)
        vfs_data_open_helper (ah);

    /* Just in case we have an operation, like open */
    if (!vfs_data_wait_results(ah, TRUE))
        return -1;

    g_assert (ah->state == VFS_ASYNC_READY);

    /* Start async operation */
    ah->buffer = (gpointer)buffer;
    ah->state = VFS_ASYNC_PROCESSING;
    ah->operation = VFS_OP_WRITING;
    g_output_stream_write_async (ah->ostream, buffer, size, G_PRIORITY_DEFAULT, ah->cancellable, vfs_data_write_done, ah);

    /* Wait for it */
    if (!vfs_data_wait_results(ah, TRUE))
        return -1;

    /* Return results */
    sz = (ssize_t)ah->processed;

    ah->buffer = NULL;
    ah->processed = 0;

    /* Sadly GPGME doesn't support errors on close, so we have to flush after each write */
    ah->state = VFS_ASYNC_PROCESSING;
    ah->operation = VFS_OP_FLUSHING;
    g_output_stream_flush_async (ah->ostream, G_PRIORITY_DEFAULT, ah->cancellable, vfs_data_flush_done, ah);

    /* Wait for it */
    if (!vfs_data_wait_results(ah, TRUE))
        return -1;

    return sz;
}

/* Called from gpgme to seek a file */
static off_t
vfs_data_seek (void *handle, off_t offset, int whence)
{
    VfsAsyncHandle* ah = (VfsAsyncHandle*)handle;
    GSeekable *seekable = NULL;
    GSeekType wh;

    /* If the file isn't open yet, then do that now */
    if (!ah->ostream && !ah->istream && ah->state == VFS_ASYNC_READY)
        vfs_data_open_helper (ah);

    /* Just in case we have an operation, like open */
    if (!vfs_data_wait_results(ah, TRUE))
        return (off_t)-1;

    g_assert (ah->state == VFS_ASYNC_READY);

    if (ah->writer && G_IS_SEEKABLE (ah->ostream))
        seekable = G_SEEKABLE (ah->ostream);
    else
        seekable = G_SEEKABLE (ah->istream);

    if (!seekable || !g_seekable_can_seek (seekable)) {
        errno = ENOTSUP;
        return -1;
    }

    switch(whence)
    {
    case SEEK_SET:
        wh = G_SEEK_SET;
        break;
    case SEEK_CUR:
        wh = G_SEEK_CUR;
        break;
    case SEEK_END:
        wh = G_SEEK_END;
        break;
    default:
        g_assert_not_reached();
        break;
    }

    /* All seek operations are not async */
    g_clear_error (&ah->error);
    g_seekable_seek (seekable, (goffset)offset, wh, ah->cancellable, &ah->error);

    /* Start async operation */
    ah->state = VFS_ASYNC_READY;
    if (!vfs_data_wait_results (ah, TRUE))
        return -1;

    /* Return results */
    ah->state = VFS_ASYNC_READY;
    return offset;
}

/* Dummy callback for closing a file */
static void
vfs_data_close_done (GObject *source, GAsyncResult *res, gpointer callback_data)
{
    /*
     * GPGME doesn't have a way to return errors from close. So we mitigate this
     * by always flushing data above. This isn't foolproof, but seems like the
     * best we can do.
     */
}

/* Called to cancel the current operation.
 * Puts the async handle into a cancelled state. */
static void
vfs_data_cancel (VfsAsyncHandle *ah)
{
    switch (ah->state) {
    case VFS_ASYNC_CANCELLED:
        break;

    case VFS_ASYNC_PROCESSING:
        g_cancellable_cancel (ah->cancellable);
        break;

    case VFS_ASYNC_READY:
        break;
    }

    if (ah->ostream)
        g_output_stream_close_async (ah->ostream, G_PRIORITY_DEFAULT, NULL, vfs_data_close_done, ah);
    else if (ah->istream)
        g_input_stream_close_async (ah->istream, G_PRIORITY_DEFAULT, NULL, vfs_data_close_done, ah);
    else
	return;

    vfs_data_wait_results (ah, FALSE);

    ah->state = VFS_ASYNC_CANCELLED;
}

/* Called by gpgme to close a file */
static void
vfs_data_release (void *handle)
{
    VfsAsyncHandle* ah = (VfsAsyncHandle*)handle;

    vfs_data_cancel (ah);

    if (ah->file)
        g_object_unref (ah->file);
    ah->file = NULL;

    if (ah->istream)
        g_object_unref (ah->istream);
    ah->istream = NULL;

    if (ah->ostream)
        g_object_unref (ah->ostream);
    ah->ostream = NULL;

    if (ah->cancellable)
        g_object_unref (ah->cancellable);
    ah->cancellable = NULL;

    g_clear_error (&ah->error);

    g_free (ah);
}

/* GPGME vfs file operations */
static struct gpgme_data_cbs vfs_data_cbs = {
    vfs_data_read,
    vfs_data_write,
    vfs_data_seek,
    vfs_data_release
};

/* -----------------------------------------------------------------------------
 */

/* Create a data on the given uri, remote uris get gnome-vfs backends,
 * local uris get normal file access. */
static gpgme_data_t
create_vfs_data (GFile *file, guint mode, SeahorseVfsProgressCb progcb,
                 gpointer userdata, gpg_error_t* err)
{
    gpgme_error_t gerr;
    gpgme_data_t ret = NULL;
    VfsAsyncHandle* handle = NULL;

    if (!err)
        err = &gerr;

    handle = vfs_data_open (file, mode & SEAHORSE_VFS_WRITE,
                                  mode & SEAHORSE_VFS_DELAY);
    if (handle) {

        *err = gpgme_data_new_from_cbs (&ret, &vfs_data_cbs, handle);
        if (*err != 0) {
            vfs_data_cbs.release (handle);
            ret = NULL;
        }

        handle->progcb = progcb;
        handle->userdata = userdata;
        handle->gdata = ret;
    }

    return ret;
}


gpgme_data_t
seahorse_vfs_data_create (const gchar *uri, guint mode, GError **err)
{
    GFile *file = g_file_new_for_uri (uri);
    gpgme_data_t data;

    data = seahorse_vfs_data_create_full (file, mode, NULL, NULL, err);
    g_object_unref (file);

    return data;
}

gpgme_data_t
seahorse_vfs_data_create_full (GFile *file, guint mode, SeahorseVfsProgressCb progcb,
                               gpointer userdata, GError **err)
{
    gpgme_data_t data;
    gpgme_error_t gerr;

    g_return_val_if_fail (!err || !*err, NULL);
    data = create_vfs_data (file, mode, progcb, userdata, &gerr);
    if (!data)
        seahorse_util_gpgme_to_error (gerr, err);
    return data;
}
