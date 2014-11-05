/*
 * Seahorse
 *
 * Copyright (C) 2006 Stefan Walter
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

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>

#include <glib/gi18n.h>
#include <gio/gio.h>

#include "seahorse-tool.h"
#include "seahorse-util.h"
#include "seahorse-widget.h"
#include "seahorse-gconf.h"
#include "seahorse-vfs-data.h"

#define ONE_GIGABYTE 1024 * 1024 * 1024
#define FILE_ATTRIBUTES "standard::*"

typedef struct _FileInfo {
    GFile *file;
    GFileInfo *info;
    gchar *uri;
} FileInfo;

typedef struct _FilesCtx {
    GSList *uris;
    GList *finfos;
    FileInfo *cur;
    gboolean remote;

    guint64 total;
    guint64 done;
} FilesCtx;

static void
free_file_info (FileInfo *finfo, gpointer unused)
{
    if (finfo) {
        g_object_unref (finfo->file);
        g_object_unref (finfo->info);
        g_free (finfo->uri);
    }
    g_free (finfo);
}

/* Included from file-roller/src/main.c for file types */

typedef enum {
	FR_FILE_TYPE_ACE,
	FR_FILE_TYPE_AR,
	FR_FILE_TYPE_ARJ,
	FR_FILE_TYPE_BZIP,
	FR_FILE_TYPE_BZIP2,
	FR_FILE_TYPE_COMPRESS,
	FR_FILE_TYPE_CPIO,
	FR_FILE_TYPE_DEB,
	FR_FILE_TYPE_ISO,
	FR_FILE_TYPE_EAR,
	FR_FILE_TYPE_EXE,
	FR_FILE_TYPE_GZIP,
	FR_FILE_TYPE_JAR,
	FR_FILE_TYPE_LHA,
	FR_FILE_TYPE_LZOP,
	FR_FILE_TYPE_RAR,
	FR_FILE_TYPE_RPM,
	FR_FILE_TYPE_TAR,
	FR_FILE_TYPE_TAR_BZ,
	FR_FILE_TYPE_TAR_BZ2,
	FR_FILE_TYPE_TAR_GZ,
	FR_FILE_TYPE_TAR_LZOP,
	FR_FILE_TYPE_TAR_COMPRESS,
	FR_FILE_TYPE_STUFFIT,
	FR_FILE_TYPE_WAR,
	FR_FILE_TYPE_ZIP,
	FR_FILE_TYPE_ZOO,
	FR_FILE_TYPE_7ZIP,
	FR_FILE_TYPE_NULL
} FRFileType;

typedef struct {
	char       *command;
	gboolean    can_open;
	gboolean    can_save;
	gboolean    support_many_files;
	FRFileType  file_type;
} FRCommandDescription;

typedef struct {
	FRFileType  id;
	char       *ext;
	char       *mime_type;
	char       *name;
} FRFileTypeDescription;

FRFileTypeDescription file_type_desc[] = {
	{ FR_FILE_TYPE_ACE,          ".ace",     "application/x-ace", N_("Ace (.ace)") },
	{ FR_FILE_TYPE_AR,           ".ar",      "application/x-ar", N_("Ar (.ar)") },
	{ FR_FILE_TYPE_ARJ,          ".arj",     "application/x-arj", N_("Arj (.arj)") },
	{ FR_FILE_TYPE_BZIP,         ".bz",      "application/x-bzip", NULL },
	{ FR_FILE_TYPE_BZIP2,        ".bz2",     "application/x-bzip", NULL },
	{ FR_FILE_TYPE_COMPRESS,     ".Z",       "application/x-compress", NULL },
	{ FR_FILE_TYPE_CPIO,         ".cpio",    "application/x-cpio", NULL },
	{ FR_FILE_TYPE_DEB,          ".deb",     "application/x-deb", NULL },
	{ FR_FILE_TYPE_ISO,          ".iso",     "application/x-cd-image", NULL },
	{ FR_FILE_TYPE_EAR,          ".ear",     "application/x-ear", N_("Ear (.ear)") },
	{ FR_FILE_TYPE_EXE,          ".exe",     "application/x-ms-dos-executable", N_("Self-extracting zip (.exe)") },
	{ FR_FILE_TYPE_GZIP,         ".gz",      "application/x-gzip", NULL},
	{ FR_FILE_TYPE_JAR,          ".jar",     "application/x-jar", N_("Jar (.jar)")},
	{ FR_FILE_TYPE_LHA,          ".lzh",     "application/x-lha", N_("Lha (.lzh)") },
	{ FR_FILE_TYPE_LZOP,         ".lzo",     "application/x-lzop", NULL },
	{ FR_FILE_TYPE_RAR,          ".rar",     "application/x-rar", N_("Rar (.rar)") },
	{ FR_FILE_TYPE_RPM,          ".rpm",     "application/x-rpm", NULL },
	{ FR_FILE_TYPE_TAR,          ".tar",     "application/x-tar", N_("Tar uncompressed (.tar)") },
	{ FR_FILE_TYPE_TAR_BZ,       ".tar.bz",  "application/x-bzip-compressed-tar", N_("Tar compressed with bzip (.tar.bz)") },
	{ FR_FILE_TYPE_TAR_BZ2,      ".tar.bz2", "application/x-bzip-compressed-tar", N_("Tar compressed with bzip2 (.tar.bz2)") },
	{ FR_FILE_TYPE_TAR_GZ,       ".tar.gz",  "application/x-compressed-tar", N_("Tar compressed with gzip (.tar.gz)") },
	{ FR_FILE_TYPE_TAR_LZOP,     ".tar.lzo", "application/x-lzop-compressed-tar", N_("Tar compressed with lzop (.tar.lzo)") },
	{ FR_FILE_TYPE_TAR_COMPRESS, ".tar.Z",   "application/x-compressed-tar", N_("Tar compressed with compress (.tar.Z)") },
	{ FR_FILE_TYPE_STUFFIT,      ".sit",     "application/x-stuffit", NULL },
	{ FR_FILE_TYPE_WAR,          ".war",     "application/zip", N_("War (.war)") },
	{ FR_FILE_TYPE_ZIP,          ".zip",     "application/zip", N_("Zip (.zip)") },
	{ FR_FILE_TYPE_ZOO,          ".zoo",     "application/x-zoo", N_("Zoo (.zoo)") },
	{ FR_FILE_TYPE_7ZIP,         ".7z",      "application/x-7z-compressed", N_("7-Zip (.7z)") }
};

FRCommandDescription command_desc[] = {
	{ "tar",        TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_TAR },
	{ "zip",        TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_ZIP },
	{ "unzip",      TRUE,  FALSE, TRUE,  FR_FILE_TYPE_ZIP },
	{ "rar",        TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_RAR },
	{ "gzip",       TRUE,  TRUE,  FALSE, FR_FILE_TYPE_GZIP },
	{ "bzip2",      TRUE,  TRUE,  FALSE, FR_FILE_TYPE_BZIP2 },
	{ "unace",      TRUE,  FALSE, TRUE,  FR_FILE_TYPE_ACE },
	{ "ar",         TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_AR },
	{ "ar",         TRUE,  FALSE, TRUE,  FR_FILE_TYPE_DEB },
	{ "arj",        TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_ARJ },
	{ "bzip2",      TRUE,  FALSE, FALSE, FR_FILE_TYPE_BZIP },
	{ "compress",   TRUE,  TRUE,  FALSE, FR_FILE_TYPE_COMPRESS },
	{ "cpio",       TRUE,  FALSE, FALSE, FR_FILE_TYPE_CPIO },
	{ "isoinfo",    TRUE,  FALSE, TRUE,  FR_FILE_TYPE_ISO },
	{ "zip",        TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_EAR },
	{ "zip",        TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_JAR },
	{ "zip",        TRUE,  FALSE,  TRUE,  FR_FILE_TYPE_EXE },
	{ "lha",        TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_LHA },
	{ "lzop",       TRUE,  TRUE,  FALSE, FR_FILE_TYPE_LZOP },
	{ "rpm2cpio",   TRUE,  FALSE, TRUE,  FR_FILE_TYPE_RPM },
	{ "uncompress", TRUE,  FALSE, FALSE, FR_FILE_TYPE_COMPRESS },
	{ "unstuff",    TRUE,  FALSE, FALSE, FR_FILE_TYPE_STUFFIT },
	{ "zip",        TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_WAR },
	{ "zoo",        TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_ZOO },
	{ "7za",        TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_7ZIP },
	{ "7zr",        TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_7ZIP }
};

FRCommandDescription tar_command_desc[] = {
	{ "gzip",      TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_TAR_GZ },
	{ "bzip2",     TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_TAR_BZ2 },
	{ "bzip",      FALSE, TRUE,  TRUE,  FR_FILE_TYPE_TAR_BZ },
	{ "lzop",      TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_TAR_LZOP },
	{ "compress",  TRUE,  TRUE,  TRUE,  FR_FILE_TYPE_TAR_COMPRESS }
};

FRFileType save_type[32];

static gboolean
is_program_in_path (const char *filename)
{
	char *str;
	int   result = FALSE;

	str = g_find_program_in_path (filename);
	if (str != NULL) {
		g_free (str);
		result = TRUE;
	}

	return result;
}

static void
compute_supported_archive_types (void)
{
	int i, j;
	int s_i = 0;

	for (i = 0; i < G_N_ELEMENTS (command_desc); i++) {
		FRCommandDescription com = command_desc[i];

		if (!is_program_in_path (com.command))
			continue;

		if (strcmp (com.command, "tar") == 0)
			for (j = 0; j < G_N_ELEMENTS (tar_command_desc); j++) {
				FRCommandDescription com2 = tar_command_desc[j];

				if (!is_program_in_path (com2.command))
					continue;
				save_type[s_i++] = com2.file_type;
			}

		if (com.can_save && com.support_many_files)
			save_type[s_i++] = com.file_type;
	}

	save_type[s_i++] = FR_FILE_TYPE_NULL;
}


/* -----------------------------------------------------------------------------
 * CHECK STEP
 */

static gboolean
step_check_uris (FilesCtx *ctx,
                 const gchar **uris,
                 GError **err)
{
    GFile *file, *base;
    GFileInfo *info;
    gchar *t, *path;
    gboolean ret = TRUE;
    FileInfo *finfo;
    const gchar **k;
    GFileType type;

    g_assert (err && !*err);

    t = g_get_current_dir ();
    base = g_file_new_for_path (t);
    g_free (t);

    for (k = uris; *k; k++) {
        if (!seahorse_tool_progress_check ()) {
            ret = FALSE;
            break;
        }

        t = g_uri_parse_scheme (*k);
        if (t)
            file = g_file_new_for_uri (*k);
        else
	        file = g_file_resolve_relative_path (base, *k);
        g_free (t);

        g_return_val_if_fail (file != NULL, FALSE);

        /* Find out if file can be accessed locally? */
        path = g_file_get_path (file);
        if (!path)
            ctx->remote = TRUE;
        g_free (path);

        info = g_file_query_info (file, FILE_ATTRIBUTES,
                                  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, err);

        if (!info) {
            ret = FALSE;
            g_object_unref (file);
            break;
        }

        type = g_file_info_get_file_type (info);

        /* Only handle simple types */
        if (type == G_FILE_TYPE_REGULAR || type == G_FILE_TYPE_UNKNOWN ||
            type == G_FILE_TYPE_DIRECTORY) {

            finfo = g_new0 (FileInfo, 1);
            finfo->file = file;
            g_object_ref (file);
            finfo->info = info;
            g_object_ref (info);
            finfo->uri = g_file_get_uri (file);

            ctx->total += g_file_info_get_size (info);
            ctx->finfos = g_list_prepend (ctx->finfos, finfo);
        }

        g_object_unref (file);
        g_object_unref (info);
    }

    g_object_unref (base);

    ctx->finfos = g_list_reverse (ctx->finfos);
    return ret;
}

/* -----------------------------------------------------------------------------
 * PACKAGE STEP
 */

/* Build a message for a given combination of files and folders */
static gchar*
make_message (guint folders, guint files)
{
    gchar *msg, *s1, *s2;

    g_assert(folders > 0 || files > 0);

    /* Necessary hoopla for translations */
    if (folders > 0 && files > 0) {

        /* TRANSLATOR: This string will become
         * "You have selected %d files and %d folders" */
        s1 = g_strdup_printf(ngettext("You have selected %d file ", "You have selected %d files ", files),
                             files);

        /* TRANSLATOR: This string will become
         * "You have selected %d files and %d folders" */
        s2 = g_strdup_printf(ngettext("and %d folder", "and %d folders", folders),
                             folders);

        /* TRANSLATOR: "%s%s" are "You have selected %d files and %d folders"
         * Swap order with "%2$s%1$s" if needed */
        msg = g_strdup_printf(_("<b>%s%s</b>"), s1, s2);

        g_free (s1);
        g_free (s2);
        return msg;

    } else if (files > 0) {

        return g_strdup_printf (ngettext ("You have selected %d file", "You have selected %d files", files), files);

    } else if (folders > 0) {

        return g_strdup_printf (
            ngettext ("You have selected %d folder", "You have selected %d folders", folders), folders);

    } else {
        g_assert_not_reached ();
        return NULL; /* to fix warnings */
    }
}

/* Callback for main option buttons */
static void
seperate_toggled (GtkWidget *widget, GtkWidget *package)
{
    gtk_widget_set_sensitive (package,
        !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}

/* Build the multiple file dialog */
static SeahorseWidget*
prepare_dialog (FilesCtx *ctx, guint nfolders, guint nfiles, GFileInfo *info, gchar* ext)
{
    SeahorseWidget *swidget;
    const gchar* pkg;
    GtkWidget *tog;
    GtkWidget *w;
    GtkWidget *combo;
    gchar *msg, *display;
    gboolean sep;
    gint i;
    GtkCellRenderer *cell;
    GtkTreeModel *store;
	FRFileType *save_type_list;

    g_assert (info);

    swidget = seahorse_widget_new ("multi-encrypt", NULL);
    g_return_val_if_fail (swidget != NULL, NULL);

    /* The main 'selected' message */
    msg = make_message (nfolders, nfiles);
    w = GTK_WIDGET (seahorse_widget_get_widget (swidget, "message"));
    gtk_label_set_markup (GTK_LABEL(w), msg);
    g_free (msg);

    /* Setup the remote or local messages */
    w = GTK_WIDGET (seahorse_widget_get_widget (swidget,
            ctx->remote ? "remote-options" : "local-options"));
    gtk_widget_show (w);

    tog = GTK_WIDGET (seahorse_widget_get_widget (swidget, "do-separate"));

    if (ctx->remote) {
        /* Always use the seperate option */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tog), TRUE);

    /* The local stuff */
    } else {

        sep = seahorse_gconf_get_boolean (MULTI_SEPERATE_KEY);

        /* Setup the package */
        w = GTK_WIDGET (seahorse_widget_get_widget (swidget, "package-name"));
        display = g_strdup (g_file_info_get_display_name (info));
        pkg = seahorse_util_uri_split_last (display);
        gtk_entry_set_text (GTK_ENTRY (w), pkg);
        g_free (display);

        /* Setup the URI combo box */
        combo = GTK_WIDGET (seahorse_widget_get_widget (swidget, "package-extension"));
        store = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
        gtk_combo_box_set_model (GTK_COMBO_BOX (combo), store);
        g_object_unref (store);

        gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));
        cell = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
                                        "text", 0,
                                        NULL);

        compute_supported_archive_types ();

        save_type_list = save_type;

        for (i = 0; save_type_list[i] != FR_FILE_TYPE_NULL; i++) {
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo),
		                                file_type_desc[save_type_list[i]].ext);
		    if (strcmp(ext, file_type_desc[save_type_list[i]].ext) == 0)
		        gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
    	}

        if(sep == FALSE) {
            gtk_widget_grab_focus (w);
            gtk_editable_select_region (GTK_EDITABLE (w), 0, strlen (pkg));
        }

        /* Setup the main radio buttons */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tog), sep);
        g_signal_connect (tog, "toggled", G_CALLBACK (seperate_toggled), w);
        seperate_toggled (tog, w);
    }

    return swidget;
}

/* Get the package name and selection */
static gchar*
get_results (SeahorseWidget *swidget)
{
    const gchar* name;
    const gchar* t;
    gchar *full_name, *ext;
    GtkWidget *w;
    gboolean sep;

    w = GTK_WIDGET (seahorse_widget_get_widget (swidget, "do-separate"));
    sep = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
    seahorse_gconf_set_boolean (MULTI_SEPERATE_KEY, sep);

    /* no packaging */
    if(!sep) {

        w = GTK_WIDGET (seahorse_widget_get_widget (swidget, "package-name"));
        name = gtk_entry_get_text (GTK_ENTRY (w));

        w = GTK_WIDGET (seahorse_widget_get_widget (swidget, "package-extension"));
        ext = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (w));

        /* No paths */
        t = strrchr(name, '/');
        name = t ? ++t : name;

        /* If someone goes out of their way to delete the file name,
         * we're simply unimpressed and put back a default. */
        if(name[0] == 0)
            name = "encrypted-package";

        full_name = g_strdup_printf("%s%s", name, ext);

        /* Save the extension */
        seahorse_gconf_set_string (MULTI_EXTENSION_KEY, ext);

        return full_name;
    }

    return NULL;
}

static gboolean
step_process_multiple (FilesCtx *ctx,
                       const gchar **orig_uris,
                       GError **err)
{
    SeahorseWidget *swidget;
    gboolean done = FALSE;
    FileInfo *pkg_info = NULL;
    gchar *package = NULL;
    gchar *ext;
    GFile *file, *parent;
    gboolean ok = FALSE;
    GtkWidget *dlg;
    guint nfolders, nfiles;
    gchar *uris[2];
    gchar *uri;
    GList *l;

    g_assert (err && !*err);

    for (l = ctx->finfos, nfolders = nfiles = 0; l; l = g_list_next (l)) {
        FileInfo *finfo = (FileInfo*)l->data;
        if (g_file_info_get_file_type (finfo->info) == G_FILE_TYPE_DIRECTORY)
            ++nfolders;
        else
            ++nfiles;
    }

    /* In the case of one or less files, no dialog */
    if(nfolders == 0 && nfiles <= 1)
        return TRUE;

    /* The package extension */
    if ((ext = seahorse_gconf_get_string (MULTI_EXTENSION_KEY)) == NULL)
        ext = g_strdup (".zip"); /* Yes this happens when the schema isn't installed */

    /* Figure out a good URI for our package */
    for (l = ctx->finfos; l; l = g_list_next (l)) {
        if (l->data) {
            pkg_info = (FileInfo*)(l->data);
            break;
        }
    }

    /* This sets up but doesn't run the dialog */
    swidget = prepare_dialog (ctx, nfolders, nfiles, pkg_info->info, ext);

    g_free (ext);

    dlg = seahorse_widget_get_toplevel (swidget);

    /* Inhibit popping up of progress dialog */
    seahorse_tool_progress_block (TRUE);

    while (!done) {
        switch (gtk_dialog_run (GTK_DIALOG (dlg)))
        {
        case GTK_RESPONSE_HELP:
            /* TODO: Implement help */
            break;

        case GTK_RESPONSE_OK:
            package = get_results (swidget);
            ok = TRUE;
            /* Fall through */

        default:
            done = TRUE;
            break;
        }
    }

    /* Let progress dialog pop up */
    seahorse_tool_progress_block (FALSE);

    seahorse_widget_destroy (swidget);

    /* Cancelled */
    if (!ok)
        return FALSE;

    /* No package was selected? */
    if (!package)
        return TRUE;

    /* A package was selected */

    /* Make a new path based on the first uri */
    parent = g_file_get_parent (pkg_info->file);
    if (!parent)
    	parent = pkg_info->file;
    file = g_file_get_child_for_display_name (parent, package, err);
    if (!file)
	    return FALSE;

    uri = g_file_get_uri (file);
    g_return_val_if_fail (uri, FALSE);
    g_object_unref (file);

    if (!seahorse_util_uris_package (uri, orig_uris)) {
        g_free (uri);
        return FALSE;
    }

    /* Free all file info */
    g_list_foreach (ctx->finfos, (GFunc)free_file_info, NULL);
    g_list_free (ctx->finfos);
    ctx->finfos = NULL;

    /* Reload up the new file, as what to encrypt */
    uris[0] = uri;
    uris[1] = NULL;
    ok = step_check_uris (ctx, (const gchar**)uris, err);
    g_free (uri);

    return ok;
}

/* -----------------------------------------------------------------------------
 * EXPAND STEP
 */

static gboolean
visit_enumerator (FilesCtx *ctx, GFile *parent, GFileEnumerator *enumerator, GError **err)
{
    GFileEnumerator *children;
    gboolean ret = TRUE;
    GFileInfo *info;
    FileInfo *finfo;
    GFile *file;

    for (;;) {
	if (!seahorse_tool_progress_check ()) {
            ret = FALSE;
            break;
	}

	info = g_file_enumerator_next_file (enumerator, NULL, err);
        if (!info) {
            if (err && *err)
                ret = FALSE;
            break;
        }

        file = g_file_resolve_relative_path (parent, g_file_info_get_name (info));
        g_return_val_if_fail (file, FALSE);

        /* Enumerate child directories */
        if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
                children = g_file_enumerate_children (file, FILE_ATTRIBUTES,
                                                      G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                                      NULL, err);
                if (!enumerator) {
                    ret = FALSE;
                    break;
                }

                ret = visit_enumerator (ctx, file, children, err);
                if (!ret)
                    break;

        /* A file, add it */
        } else {

            finfo = g_new0 (FileInfo, 1);
            finfo->info = info;
            finfo->file = file;
            finfo->uri = g_file_get_uri (file);
            g_object_ref (info);
            g_object_ref (file);

            ctx->total += g_file_info_get_size (info);
            ctx->finfos = g_list_append (ctx->finfos, finfo);
        }

        g_object_unref (file);
        file = NULL;
    }

    if (file != NULL)
        g_object_unref (file);

    g_object_unref (enumerator);
    return ret;
}

static gboolean
step_expand_uris (FilesCtx *ctx,
                  GError **err)
{
    GFileEnumerator *enumerator;
    gboolean ret = TRUE;
    FileInfo *finfo;
    GList *l;

    g_assert (err && !*err);

    for (l = ctx->finfos; l; l = g_list_next (l)) {

        if (!seahorse_tool_progress_check ()) {
            ret = FALSE;
            break;
        }

        finfo = (FileInfo*)(l->data);
        if (!finfo || !finfo->uri)
            continue;

        if (g_file_info_get_file_type (finfo->info) == G_FILE_TYPE_DIRECTORY) {

            enumerator = g_file_enumerate_children (finfo->file, FILE_ATTRIBUTES,
                                                    G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                                    NULL, err);
            if (!enumerator)
                return FALSE;

            if (!visit_enumerator (ctx, finfo->file, enumerator, err))
                return FALSE;

            /* We don't actually do operations on the dirs */
            free_file_info (finfo, NULL);
            l->data = NULL;
        }
    }

    return ret;
}

/* -----------------------------------------------------------------------------
 * ACTUAL OPERATION STEP
 */

static void
progress_cb (gpgme_data_t data, goffset pos, FilesCtx *ctx)
{
    gdouble total, done, size, portion;
    goffset fsize;

    g_assert (ctx && ctx->cur);

    total = ctx->total > ONE_GIGABYTE ? ctx->total / 1000 : ctx->total;
    done = ctx->total > ONE_GIGABYTE ? ctx->done / 1000 : ctx->done;
    fsize = g_file_info_get_size (ctx->cur->info);
    size = ctx->total > ONE_GIGABYTE ? fsize / 1000 : fsize;
    portion = ctx->total > ONE_GIGABYTE ? pos / 1000 : pos;

    total = total <= 0 ? 1 : total;
    size = size <= 0 ? 1 : size;

    /* The cancel check is done elsewhere */
    seahorse_tool_progress_update ((done / total) + ((size / total) * (portion / size)),
                                   seahorse_util_uri_get_last (g_file_info_get_display_name (ctx->cur->info)));
}

static gboolean
step_operation (FilesCtx *ctx,
                SeahorseToolMode *mode,
                GError **err)
{
    SeahorsePGPOperation *pop = NULL;
    gpgme_data_t data = NULL;
    gboolean ret = FALSE;

    SeahorseOperation *op;
    FileInfo *finfo;
    GList *l;
    gchar *filename;

    /* Reset our done counter */
    ctx->done = 0;

    for (l = ctx->finfos; l; l = g_list_next (l)) {

        finfo = (FileInfo*)l->data;
        if (!finfo || !finfo->file)
            continue;

        ctx->cur = finfo;

        /* A new operation for each context */
        pop = seahorse_pgp_operation_new (NULL);
        op = SEAHORSE_OPERATION (pop);

        data = seahorse_vfs_data_create_full (finfo->file, SEAHORSE_VFS_READ,
                                              (SeahorseVfsProgressCb)progress_cb,
                                              ctx, err);
        if (!data)
            goto finally;

        /* Inhibit popping up of progress dialog */
        seahorse_tool_progress_block (TRUE);

        /* Embed filename during encryption */
        if (mode_encrypt)
        {
            filename = g_file_get_basename (finfo->file);
            gpgme_data_set_file_name (data, filename);
            g_free (filename);
        }

        /* The start callback */
        if (mode->startcb) {
            if (!(mode->startcb) (mode, finfo->uri, data, pop, err))
                goto finally;
        }

        /* Let progress dialog pop up */
        seahorse_tool_progress_block (FALSE);

        /* Run until the operation completes */
        seahorse_util_wait_until ((!seahorse_operation_is_running (op) ||
                                   !seahorse_tool_progress_check ()));

        /* If cancel then reflect that */
        if (seahorse_operation_is_running (op)) {
            seahorse_operation_cancel (op);
            goto finally;
        }

        if (!seahorse_operation_is_successful (op)) {
            seahorse_operation_copy_error (op, err);
            goto finally;
        }

        /* The done callback */
        if (mode->donecb) {
            if (!(mode->donecb) (mode, finfo->uri, data, pop, err))
                goto finally;
        }

        ctx->done += g_file_info_get_size (finfo->info);
        ctx->cur = NULL;

        g_object_unref (pop);
        pop = NULL;

        gpgme_data_release (data);
        data = NULL;
    }

    seahorse_tool_progress_update (1.0, "");
    ret = TRUE;

finally:
    if (pop)
        g_object_unref (pop);
    if (data)
        gpgme_data_release (data);

    return ret;
}

/* -----------------------------------------------------------------------------
 * MAIN COORDINATION
 */

int
seahorse_tool_files_process (SeahorseToolMode *mode, const gchar **uris)
{
    const gchar *errdesc = NULL;
    GError *err = NULL;
    FilesCtx ctx;
    int ret = 1;

    memset (&ctx, 0, sizeof (ctx));

    /* Start progress bar */
    seahorse_tool_progress_start (mode->title);
    if (!seahorse_tool_progress_update (-1, _("Preparing...")))
        goto finally;

    /*
     * 1. Check all the uris, format them properly, and figure
     * out what they are (ie: files, directories, size etc...)
     */

    if (!step_check_uris (&ctx, uris, &err)) {
        errdesc = _("Couldn't list files");
        goto finally;
    }

    /*
     * 2. Prompt user and see what they want to do
     * with multiple files.
     */
    if (mode->package) {
        if (!step_process_multiple (&ctx, uris, &err)) {
            errdesc = _("Couldn't package files");
            goto finally;
        }
    }

    if (!seahorse_tool_progress_check ())
        goto finally;

    /*
     * 2. Expand remaining URIs, so we know what we're
     * dealing with. This also gets file size info.
     */
    if (!step_expand_uris (&ctx, &err)) {
        errdesc = _("Couldn't list files");
        goto finally;
    }

    if (!seahorse_tool_progress_update (0.0, NULL))
        goto finally;

    /*
     * 3. Now execute enc operation on every file
     */
    if (!step_operation (&ctx, mode, &err)) {
        errdesc = mode->errmsg;
        goto finally;
    }

    seahorse_tool_progress_update (1.0, NULL);
    ret = 0;

finally:

    seahorse_tool_progress_stop ();

    if (err) {
        seahorse_util_handle_error (err, errdesc, ctx.cur ?
                                    g_file_info_get_display_name (ctx.cur->info) : "");
    }

    if (ctx.finfos) {
        g_list_foreach (ctx.finfos, (GFunc)free_file_info, NULL);
        g_list_free (ctx.finfos);
    }

    return ret;
}
