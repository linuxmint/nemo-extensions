/*
 * Seahorse
 *
 * Copyright (C) 2003 Jacob Perkins
 * Copyright (C) 2004-2005 Stefan Walter
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
 * 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#include "config.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#ifdef WITH_SHARING
#include <avahi-glib/glib-watch.h>
#include <avahi-glib/glib-malloc.h>
#endif

#include <dbus/dbus-glib-bindings.h>

#include "seahorse-util.h"
#include "seahorse-gconf.h"
#include "seahorse-vfs-data.h"

#include <gpgme.h>

void
seahorse_util_show_error (GtkWindow *parent, const gchar *heading, const gchar *message)
{
	GtkWidget *error;

	g_return_if_fail (message);

	error = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
		GTK_BUTTONS_CLOSE, NULL);

	if (heading)
        g_object_set (G_OBJECT (error), "text", heading, "secondary-text", message, NULL);
    else
        g_object_set (G_OBJECT (error), "text", message, NULL);

	gtk_dialog_run (GTK_DIALOG (error));
	gtk_widget_destroy (error);
}

/**
 * seahorse_util_handle_gpgme:
 * @err: Error value
 *
 * Shows an error dialog for @err.
 **/
void
seahorse_util_handle_gpgme (gpgme_error_t err, const char* desc, ...)
{
    gchar *t = NULL;

    switch (gpgme_err_code(err)) {
    case GPG_ERR_CANCELED:
    case GPG_ERR_NO_ERROR:
    case GPG_ERR_ECANCELED:
        return;
    default:
        break;
    }

    va_list ap;
    va_start(ap, desc);

    if (desc)
        t = g_strdup_vprintf (desc, ap);

    va_end(ap);

    seahorse_util_show_error (NULL, t, gpgme_strerror (err));
    g_free(t);
}

void
seahorse_util_handle_error (GError* err, const char* desc, ...)
{
    gchar *t = NULL;

    va_list ap;

    if(!err)
        return;

    va_start(ap, desc);

    if (desc)
        t = g_strdup_vprintf (desc, ap);

    va_end(ap);

    /* Never show an error for 'cancelled' */
    if (err->code == DBUS_GERROR_REMOTE_EXCEPTION && err->domain == DBUS_GERROR &&
        strstr (dbus_g_error_get_name (err), "Cancelled"))
	    return;

    seahorse_util_show_error (NULL, t, err->message ? err->message : "");
    g_free(t);

    g_clear_error (&err);
}

/**
 * seahorse_util_gpgme_error_domain
 * Returns: The GError domain for gpgme errors
 **/
GQuark
seahorse_util_gpgme_error_domain ()
{
    static GQuark q = 0;
    if(q == 0)
        q = g_quark_from_static_string ("seahorse-gpgme-error");
    return q;
}

/**
 * seahorse_util_error_domain
 * Returns: The GError domain for generic seahorse errors
 **/
GQuark
seahorse_util_error_domain ()
{
    static GQuark q = 0;
    if(q == 0)
        q = g_quark_from_static_string ("seahorse-error");
    return q;
}

/**
 * seahorse_util_gpgme_to_error
 * @gerr GPGME error
 * @err  Resulting GError object
 **/
void
seahorse_util_gpgme_to_error (gpgme_error_t gerr, GError** err)
{
    gpgme_err_code_t code;

    /* Make sure this is actually an error */
    g_assert (gerr != 0);
    code = gpgme_err_code (gerr);

    /* Special case some error messages */
    if (code == GPG_ERR_DECRYPT_FAILED) {
        g_set_error (err, SEAHORSE_GPGME_ERROR, code, "%s",
                     _("Decryption failed. You probably do not have the decryption key."));
    } else {
        g_set_error (err, SEAHORSE_GPGME_ERROR, code, "%s",
                     gpgme_strerror (gerr));
    }
}

/**
 * seahorse_util_get_display_date_string:
 * @time: Time value to parse
 *
 * Creates a string representation of @time for display in the UI.
 *
 * Returns: A string representing @time
 **/
gchar*
seahorse_util_get_display_date_string (const time_t time)
{
	GDate *created_date;
	gchar *created_string;

	if (time == 0)
		return "0";

	created_date = g_date_new ();
	g_date_set_time_t (created_date, time);
	created_string = g_new (gchar, 11);
	g_date_strftime (created_string, 11, _("%Y-%m-%d"), created_date);
	return created_string;
}

static gboolean
seahorse_util_print_fd (int fd, const char* s)
{
    /* Guarantee all data is written */
    int r, l = strlen (s);

    while (l > 0) {

        r = write (fd, s, l);

        if (r == -1) {
            if (errno == EPIPE)
                return FALSE;
            if (errno != EAGAIN && errno != EINTR) {
                g_critical ("couldn't write data to socket: %s", strerror (errno));
                return FALSE;
            }

        } else {
            s += r;
            l -= r;
        }
    }

    return TRUE;
}

/**
 * seahorse_util_printf_fd:
 * @fd: The file descriptor to write to
 * @fmt: The printf format of the data to write
 *
 * Returns: Whether the operation was successful or not.
 **/
gboolean
seahorse_util_printf_fd (int fd, const char* fmt, ...)
{
    gchar* t;
    va_list ap;
    gboolean ret;

    va_start (ap, fmt);
    t = g_strdup_vprintf (fmt, ap);
    va_end (ap);

    ret = seahorse_util_print_fd (fd, t);
    g_free (t);
    return ret;
}


/**
 * seahorse_util_uri_get_last:
 * @uri: The uri to parse
 *
 * Finds the last portion of @uri. Note that this does
 * not modify @uri. If the uri is invalid or has no
 * directories then the entire thing is returned.
 *
 * Returns: Last portion of @uri
 **/
const gchar*
seahorse_util_uri_get_last (const gchar* uri)
{
    const gchar* t;

    g_return_val_if_fail (uri, NULL);

    t = uri + strlen (uri);

    if (*(t - 1) == '/' && t != uri)
        t--;

    while (*(t - 1) != '/' && t != uri)
        t--;

    return t;
}

/**
 * seahorse_util_uri_split_last
 * @uri: The uri to split
 *
 * Splits the uri in two at it's last component. The result
 * is still part of the same string, so don't free it. This
 * modifies the @uri argument.
 *
 * Returns: The last component
 **/
const gchar*
seahorse_util_uri_split_last (gchar* uri)
{
    gchar* t;

    g_return_val_if_fail (uri, "");

    t = (gchar*)seahorse_util_uri_get_last (uri);
    if(t != uri)
        *(t - 1) = 0;

    return t;
}

/**
 * seahurse_util_uri_exists
 * @uri: The uri to check
 *
 * Verify whether a given uri exists or not.
 *
 * Returns: Whether it exists or not.
 **/
gboolean
seahorse_util_uri_exists (const gchar* uri)
{
    GFile *file;
    gboolean exists;

    file = g_file_new_for_uri (uri);
    g_return_val_if_fail (file, FALSE);

    exists = g_file_query_exists (file, NULL);
    g_object_unref (file);

    return exists;
}

/**
 * seahorse_util_uris_package
 * @package: Package uri
 * @uris: null-terminated array of uris to package
 *
 * Package uris into an archive. The uris must be local.
 *
 * Returns: Success or failure
 */
gboolean
seahorse_util_uris_package (const gchar* package, const char** uris)
{
    GError* err = NULL;
    gchar *out = NULL;
    gint status;
    gboolean r;
    GString *str;
    gchar *cmd;
    gchar *t;
    gchar *x;
    GFile *file, *fpackage;

    fpackage = g_file_new_for_uri (package);
    t = g_file_get_path (fpackage);
    x = g_shell_quote (t);
    g_free (t);

    /* create execution */
    str = g_string_new ("");
    g_string_printf (str, "file-roller --add-to=%s", x);
    g_free(x);

    while(*uris) {
        x = g_uri_parse_scheme (*uris);
        if (x)
            file = g_file_new_for_uri (*uris);
        else
            file = g_file_new_for_path (*uris);
        g_free (x);

        t = g_file_get_path (file);
        g_object_unref (file);
        g_return_val_if_fail (t != NULL, FALSE);

        x = g_shell_quote(t);
        g_free(t);

        g_string_append_printf (str, " %s", x);
        g_free(x);

        uris++;
    }

    /* Execute the command */
    cmd = g_string_free (str, FALSE);
    r = g_spawn_command_line_sync (cmd, &out, NULL, &status, &err);
    g_free (cmd);

    if (out) {
        g_print (out, NULL);
        g_free (out);
    }

    if (!r) {
        seahorse_util_handle_error (err, _("Couldn't run file-roller"));
        return FALSE;
    }

    if(!(WIFEXITED(status) && WEXITSTATUS(status) == 0)) {
        seahorse_util_show_error(NULL, _("Couldn't package files"),
                                 _("The file-roller process did not complete successfully"));
        return FALSE;
    }

    t = g_file_get_path (fpackage);
    if (t != NULL) {
        g_chmod (t, S_IRUSR | S_IWUSR);
        g_free (t);
    }

    return TRUE;
}

GtkWidget*
seahorse_util_chooser_save_new (const gchar *title, GtkWindow *parent)
{
    GtkWidget *dialog;

    dialog = gtk_file_chooser_dialog_new (title,
                parent, GTK_FILE_CHOOSER_ACTION_SAVE,
                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                NULL);

    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), FALSE);
    return dialog;
}

void
seahorse_util_chooser_show_key_files (GtkWidget *dialog)
{
    GtkFileFilter* filter = gtk_file_filter_new ();

    /* Filter for PGP keys. We also include *.asc, as in many
       cases that extension is associated with text/plain */
    gtk_file_filter_set_name (filter, _("All key files"));
    gtk_file_filter_add_mime_type (filter, "application/pgp-keys");
    gtk_file_filter_add_pattern (filter, "*.asc");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("All files"));
    gtk_file_filter_add_pattern (filter, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
}

void
seahorse_util_chooser_show_archive_files (GtkWidget *dialog)
{
    GtkFileFilter* filter;
    int i;

    static const char *archive_mime_type[] = {
        "application/x-ar",
        "application/x-arj",
        "application/x-bzip",
        "application/x-bzip-compressed-tar",
        "application/x-cd-image",
        "application/x-compress",
        "application/x-compressed-tar",
        "application/x-gzip",
        "application/x-java-archive",
        "application/x-jar",
        "application/x-lha",
        "application/x-lzop",
        "application/x-rar",
        "application/x-rar-compressed",
        "application/x-tar",
        "application/x-zoo",
        "application/zip",
        "application/x-7zip"
    };

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Archive files"));
    for (i = 0; i < G_N_ELEMENTS (archive_mime_type); i++)
        gtk_file_filter_add_mime_type (filter, archive_mime_type[i]);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("All files"));
    gtk_file_filter_add_pattern (filter, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
}

gchar*
seahorse_util_chooser_save_prompt (GtkWidget *dialog)
{
    GtkWidget* edlg;
    gchar *uri = NULL;

    while (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {

        uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER (dialog));

        if (uri == NULL)
            continue;

        if (seahorse_util_uri_exists (uri)) {

            edlg = gtk_message_dialog_new_with_markup (GTK_WINDOW (dialog),
                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
                        GTK_BUTTONS_NONE, _("<b>A file already exists with this name.</b>\n\nDo you want to replace it with a new file?"));
            gtk_dialog_add_buttons (GTK_DIALOG (edlg),
                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                        _("_Replace"), GTK_RESPONSE_ACCEPT, NULL);

            gtk_dialog_set_default_response (GTK_DIALOG (edlg), GTK_RESPONSE_CANCEL);

            if (gtk_dialog_run (GTK_DIALOG (edlg)) != GTK_RESPONSE_ACCEPT) {
                g_free (uri);
                uri = NULL;
            }

            gtk_widget_destroy (edlg);
        }

        if (uri != NULL)
            break;
    }

    gtk_widget_destroy (dialog);
    return uri;
}

/**
 * seahorse_util_add_suffix:
 * @ctx: Gpgme Context
 * @path: Path of an existing file
 * @suffix: Suffix type
 * @prompt: Overwrite prompt text
 *
 * Constructs a new path for a file based on @path plus a suffix determined by
 * @suffix. If ASCII Armor is enabled, the suffix will be '.asc'. Otherwise the
 * suffix will be '.pgp' if @suffix is %SEAHORSE_CRYPT_SUFFIX or '.sig' if
 * @suffix is %SEAHORSE_SIG_SUFFIX.
 *
 * Returns: A new path with the suffix appended to @path. NULL if prompt cancelled
 **/
gchar*
seahorse_util_add_suffix (const gchar *path, SeahorseSuffix suffix, const gchar *prompt)
{
    GtkWidget *dialog;
    const gchar *ext;
    gchar *uri;
    gchar *t;

    if (suffix == SEAHORSE_CRYPT_SUFFIX)
        ext = SEAHORSE_EXT_PGP;
    else
        ext = SEAHORSE_EXT_SIG;

    uri = g_strdup_printf ("%s%s", path, ext);

    if (prompt && uri && seahorse_util_uri_exists (uri)) {

        t = g_strdup_printf (prompt, seahorse_util_uri_get_last (uri));
        dialog = seahorse_util_chooser_save_new (t, NULL);
        g_free (t);

        seahorse_util_chooser_show_key_files (dialog);
        gtk_file_chooser_select_uri (GTK_FILE_CHOOSER (dialog), uri);

        g_free (uri);
        uri = NULL;

        uri = seahorse_util_chooser_save_prompt (dialog);
    }

    return uri;
}

/**
 * seahorse_util_remove_suffix:
 * @path: Path with a suffix
 * @prompt:Overwrite prompt text
 *
 * Removes a suffix from @path. Does not check if @path actually has a suffix.
 *
 * Returns: @path without a suffix. NULL if prompt cancelled
 **/
gchar*
seahorse_util_remove_suffix (const gchar *path, const gchar *prompt)
{
    GtkWidget *dialog;
    gchar *uri;
    gchar *t;

    g_return_val_if_fail (path != NULL, NULL);
    uri =  g_strndup (path, strlen (path) - 4);

    if (prompt && uri && seahorse_util_uri_exists (uri)) {

        t = g_strdup_printf (prompt, seahorse_util_uri_get_last (uri));
        dialog = seahorse_util_chooser_save_new (t, NULL);
        g_free (t);

        seahorse_util_chooser_show_key_files (dialog);
		gtk_file_chooser_select_uri (GTK_FILE_CHOOSER (dialog), uri);

        g_free (uri);
        uri = NULL;

        uri = seahorse_util_chooser_save_prompt (dialog);
    }

    return uri;
}

void
seahorse_util_free_keys (gpgme_key_t* keys)
{
    gpgme_key_t* k = keys;
    if (!keys)
        return;
    while (*k)
        gpgme_key_unref (*(k++));
    g_free (keys);
}

gboolean
seahorse_util_string_equals (const gchar *s1, const gchar *s2)
{
    if (!s1 && !s2)
        return TRUE;
    if (!s1 || !s2)
        return FALSE;
    return g_str_equal (s1, s2);
}
