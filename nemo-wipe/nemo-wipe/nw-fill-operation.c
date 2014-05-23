/*
 *  nemo-wipe - a nemo extension to wipe file(s)
 * 
 *  Copyright (C) 2009-2012 Colomban Wendling <ban@herbesfolles.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "nw-fill-operation.h"

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>
#include <gio/gio.h>
#if HAVE_GIO_UNIX
# include <gio/gunixmounts.h>
#endif
#include <gsecuredelete/gsecuredelete.h>

#include "nw-operation.h"
#include "nw-path-list.h"


GQuark
nw_fill_operation_error_quark (void)
{
  static volatile gsize quark = 0;
  
  if (g_once_init_enter (&quark)) {
    GQuark q = g_quark_from_static_string ("NwFillOperationError");
    
    g_once_init_leave (&quark, q);
  }
  
  return (GQuark) quark;
}


static void     nw_fill_operation_operation_iface_init  (NwOperationInterface *iface);
static void     nw_fill_operation_real_add_file         (NwOperation *op,
                                                         const gchar *path);
static void     nw_fill_operation_finalize              (GObject *object);
static void     nw_fill_operation_finished_handler      (GsdFillOperation *operation,
                                                         gboolean          success,
                                                         const gchar      *message,
                                                         NwFillOperation  *self);
static void     nw_fill_operation_progress_handler      (GsdFillOperation *operation,
                                                         gdouble           fraction,
                                                         NwFillOperation  *self);


struct _NwFillOperationPrivate {
  GList  *directories;
  
  guint   n_op;
  guint   n_op_done;
  
  gulong  progress_hid;
  gulong  finished_hid;
};

G_DEFINE_TYPE_WITH_CODE (NwFillOperation,
                         nw_fill_operation,
                         GSD_TYPE_FILL_OPERATION,
                         G_IMPLEMENT_INTERFACE (NW_TYPE_OPERATION,
                                                nw_fill_operation_operation_iface_init))


static void
nw_fill_operation_operation_iface_init (NwOperationInterface *iface)
{
  iface->add_file = nw_fill_operation_real_add_file;
}

static void
nw_fill_operation_class_init (NwFillOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  object_class->finalize = nw_fill_operation_finalize;
  
  g_type_class_add_private (klass, sizeof (NwFillOperationPrivate));
}

static void
nw_fill_operation_init (NwFillOperation *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            NW_TYPE_FILL_OPERATION,
                                            NwFillOperationPrivate);
  
  self->priv->directories = NULL;
  self->priv->n_op = 0;
  self->priv->n_op_done = 0;
  
  self->priv->finished_hid = g_signal_connect (self, "finished",
                                               G_CALLBACK (nw_fill_operation_finished_handler),
                                               self);
  self->priv->progress_hid = g_signal_connect (self, "progress",
                                               G_CALLBACK (nw_fill_operation_progress_handler),
                                               self);
}

static void
nw_fill_operation_finalize (GObject *object)
{
  NwFillOperation *self = NW_FILL_OPERATION (object);
  
  g_list_foreach (self->priv->directories, (GFunc) g_free, NULL);
  g_list_free (self->priv->directories);
  self->priv->directories = NULL;
  
  G_OBJECT_CLASS (nw_fill_operation_parent_class)->finalize (object);
}

static void
nw_fill_operation_real_add_file (NwOperation *op,
                                 const gchar *path)
{
  NwFillOperation *self = NW_FILL_OPERATION (op);
  
  /* FIXME: filter file? */
  self->priv->directories = g_list_prepend (self->priv->directories,
                                            g_strdup (path));
  self->priv->n_op ++;
  
  gsd_fill_operation_set_directory (GSD_FILL_OPERATION (self),
                                    self->priv->directories->data);
}

/* wrapper for the progress handler returning the current progression over all
 * operations  */
static void
nw_fill_operation_progress_handler (GsdFillOperation *operation,
                                    gdouble           fraction,
                                    NwFillOperation  *self)
{
  /* abort emission and replace by our overridden one.  Not to do that
   * recursively, we block our handler during the re-emission */
  g_signal_stop_emission_by_name (operation, "progress");
  g_signal_handler_block (operation, self->priv->progress_hid);
  g_signal_emit_by_name (operation, "progress", 0,
                         (self->priv->n_op_done + fraction) / self->priv->n_op);
  g_signal_handler_unblock (operation, self->priv->progress_hid);
}

/* timeout function to launch next operation after finish of the previous
 * operation.
 * we need this kind of hack since operation are locked while running. */
static gboolean
launch_next_operation (NwFillOperation *self)
{
  gboolean busy;
  
  busy = gsd_async_operation_get_busy (GSD_ASYNC_OPERATION (self));
  if (! busy) {
    GError   *err = NULL;
    gboolean  success;
    
    success = gsd_fill_operation_run (GSD_FILL_OPERATION (self),
                                      self->priv->directories->data,
                                      &err);
    if (! success) {
      g_signal_handler_block (self, self->priv->finished_hid);
      g_signal_emit_by_name (self, "finished", 0, success, err->message);
      g_signal_handler_unblock (self, self->priv->finished_hid);
      g_error_free (err);
    }
  }
  
  return busy; /* keeps our timeout function until lock is released */
}

/* Removes the current directory to proceed */
static void
nw_fill_operation_pop_dir (NwFillOperation *self)
{
  GList *tmp;
  
  tmp = self->priv->directories;
  if (tmp) {
    self->priv->directories = tmp->next;
    g_free (tmp->data);
    g_list_free_1 (tmp);
  }
}

/* Wrapper for the finished handler.  It launches the next operation if there
 * is one left. */
static void
nw_fill_operation_finished_handler (GsdFillOperation *operation,
                                    gboolean          success,
                                    const gchar      *message,
                                    NwFillOperation  *self)
{
  if (success) {
    self->priv->n_op_done++;
    /* remove the directory just proceeded */
    nw_fill_operation_pop_dir (self);
    
    /* if we have work left to be done... */
    if (self->priv->directories) {
      /* block signal emission, it's not the last one */
      g_signal_stop_emission_by_name (operation, "finished");
      
      /* we can't launch the next operation right here since the previous must
       * release its lock before, which is done just after return of the current
       * function.
       * To work around this, we add a timeout function that will try to launch
       * the next operation if the current one is not busy, which fixes the
       * problem. */
      g_timeout_add (10, (GSourceFunc) launch_next_operation, self);
    }
  }
}


#if HAVE_GIO_UNIX
/*
 * find_mountpoint_unix:
 * @path: An absolute path for which find the mountpoint.
 * 
 * Gets the UNIX mountpoint path of a given path.
 * <note>
 *  This function would actually never return %NULL since the mount point of
 *  every files is at least /.
 * </note>
 * 
 * Returns: The path of the mountpoint of @path or %NULL if not found.
 *          Free with g_free().
 */
static gchar *
find_mountpoint_unix (const gchar *path)
{
  gchar    *mountpoint = g_strdup (path);
  gboolean  found = FALSE;
  
  while (! found && mountpoint) {
    GUnixMountEntry *unix_mount;
    
    unix_mount = g_unix_mount_at (mountpoint, NULL);
    if (unix_mount) {
      found = TRUE;
      g_unix_mount_free (unix_mount);
    } else {
      gchar *tmp = mountpoint;
      
      mountpoint = g_path_get_dirname (tmp);
      /* check if dirname() changed the path to avoid infinite loop (e.g. when
       * / was reached) */
      if (strcmp (mountpoint, tmp) == 0) {
        g_free (mountpoint);
        mountpoint = NULL;
      }
      g_free (tmp);
    }
  }
  
  return mountpoint;
}
#endif

static gchar *
find_mountpoint (const gchar *path,
                 GError     **error)
{
  gchar  *mountpoint_path = NULL;
  GFile  *file;
  GMount *mount;
  GError *err = NULL;
  
  /* Try with GIO first */
  file = g_file_new_for_path (path);
  mount = g_file_find_enclosing_mount (file, NULL, &err);
  if (mount) {
    GFile *mountpoint_file;
    
    mountpoint_file = g_mount_get_root (mount);
    mountpoint_path = g_file_get_path (mountpoint_file);
    if (! mountpoint_path) {
      gchar *uri = g_file_get_uri (mountpoint_file);
      
      g_set_error (&err,
                   NW_FILL_OPERATION_ERROR,
                   NW_FILL_OPERATION_ERROR_REMOTE_MOUNT,
                   _("Mount \"%s\" is not local"), uri);
      g_free (uri);
    }
    g_object_unref (mountpoint_file);
    g_object_unref (mount);
  }
  g_object_unref (file);
  #if HAVE_GIO_UNIX
  /* fallback to find_unix_mount() */
  if (! mountpoint_path) {
    g_clear_error (&err);
    mountpoint_path = find_mountpoint_unix (path);
    if (! mountpoint_path) {
      g_set_error (&err,
                   NW_FILL_OPERATION_ERROR,
                   NW_FILL_OPERATION_ERROR_MISSING_MOUNT,
                   _("No mount point found for path \"%s\""), path);
    }
  }
  #endif
  if (! mountpoint_path) {
    g_propagate_error (error, err);
  }
  
  return mountpoint_path;
}

/*
 * nw_fill_operation_filter_files:
 * @paths: A list of paths to filter
 * @work_paths_: return location for filtered paths
 * @work_mounts_: return location for filtered paths' mounts
 * @error: return location for errors, or %NULL to ignore them
 * 
 * Tries to get usable paths (local directories) and keep only one per
 * mountpoint.
 * 
 * The returned lists (@work_paths_ and @work_mounts_) have the same length, and
 * an index in a list correspond to the same in the other:
 * g_list_index(work_paths_, 0) is the path of g_list_index(work_mounts_, 0).
 * Free returned lists with nw_path_list_free().
 * 
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
nw_fill_operation_filter_files (GList    *paths,
                                GList   **work_paths_,
                                GList   **work_mounts_,
                                GError  **error)
{
  GList  *work_paths  = NULL;
  GError *err         = NULL;
  GList  *work_mounts = NULL;
  
  g_return_val_if_fail (paths != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  
  for (; ! err && paths; paths = g_list_next (paths)) {
    const gchar  *file_path = paths->data;
    gchar        *mountpoint;
    
    mountpoint = find_mountpoint (file_path, &err);
    if (G_LIKELY (mountpoint)) {
      if (g_list_find_custom (work_mounts, mountpoint, (GCompareFunc)strcmp)) {
        /* the mountpoint is already added, skip it */
        g_free (mountpoint);
      } else {
        gchar *path;
        
        work_mounts = g_list_prepend (work_mounts, mountpoint);
        /* if it is not a directory, gets its container directory.
         * no harm since files cannot be mountpoint themselves, then it gets
         * at most the mountpoint itself */
        if (! g_file_test (file_path, G_FILE_TEST_IS_DIR)) {
          path = g_path_get_dirname (file_path);
        } else {
          path = g_strdup (file_path);
        }
        work_paths = g_list_prepend (work_paths, path);
      }
    }
  }
  if (err || ! work_paths_) {
    nw_path_list_free (work_paths);
  } else {
    *work_paths_ = g_list_reverse (work_paths);
  }
  if (err || ! work_mounts_) {
    nw_path_list_free (work_mounts);
  } else {
    *work_mounts_ = g_list_reverse (work_mounts);
  }
  if (err) {
    g_propagate_error (error, err);
  }
  
  return ! err;
}

NwOperation *
nw_fill_operation_new (void)
{
  return g_object_new (NW_TYPE_FILL_OPERATION, NULL);
}
