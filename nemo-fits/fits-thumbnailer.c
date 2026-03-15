/*  fits-thumbnailer.c
 *
 *  Generates thumbnails for FITS image files.
 *  Supports greyscale and RGB (3-plane cube) FITS images.
 *  Auto-stretch via fits-autostretch.c (ported from KStars/Ekos).
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "fits-render.h"

#define MAX_SIZE 256

/* Read the persistent stretch setting written by the nemo-fits extension.
 * Path must stay in sync with get_settings_path() in nemo-fits.c. */
static gboolean
read_stretch_setting (void)
{
    char     *path = g_build_filename (g_get_user_config_dir (),
                                       "nemo-fits", "settings.ini", NULL);
    GKeyFile *kf   = g_key_file_new ();
    gboolean  val  = TRUE;   /* default: stretch enabled */

    if (g_key_file_load_from_file (kf, path, G_KEY_FILE_NONE, NULL))
        val = g_key_file_get_boolean (kf, "nemo-fits", "autostretch", NULL);

    g_key_file_free (kf);
    g_free (path);
    return val;
}

int
main (int argc, char **argv)
{
    int      size         = MAX_SIZE;
    gboolean show_help    = FALSE;
    gboolean show_version = FALSE;
    gboolean no_stretch   = FALSE;

    GOptionEntry entries[] = {
        { "size",       's', 0, G_OPTION_ARG_INT,  &size,         "Thumbnail size (default: 256)", "SIZE" },
        { "no-stretch", 'n', 0, G_OPTION_ARG_NONE, &no_stretch,   "Disable auto-stretch", NULL },
        { "help",       'h', 0, G_OPTION_ARG_NONE, &show_help,    "Show help", NULL },
        { "version",    'v', 0, G_OPTION_ARG_NONE, &show_version, "Show version", NULL },
        { NULL }
    };

    GOptionContext *context = g_option_context_new ("INPUT OUTPUT");
    g_option_context_add_main_entries (context, entries, NULL);

    GError *error = NULL;
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("Option parsing failed: %s\n", error->message);
        g_error_free (error);
        g_option_context_free (context);
        return 1;
    }
    g_option_context_free (context);

    if (show_version) {
        g_print ("fits-thumbnailer 6.7.0\n");
        return 0;
    }

    if (show_help || argc != 3) {
        g_print ("Usage: fits-thumbnailer [OPTIONS] INPUT OUTPUT\n"
                 "Generate thumbnails for FITS image files\n\n"
                 "Options:\n"
                 "  -s, --size SIZE    Thumbnail size in pixels (default: 256)\n"
                 "  -n, --no-stretch   Disable auto-stretch (linear scale only)\n"
                 "  -h, --help         Show this help\n"
                 "  -v, --version      Show version\n");
        return show_help ? 0 : 1;
    }

    if (size < 1 || size > 2048) {
        g_printerr ("Invalid size: %d (must be 1-2048)\n", size);
        return 1;
    }

    /* Command-line flag overrides the saved setting */
    gboolean do_stretch = no_stretch ? FALSE : read_stretch_setting ();

    GdkPixbuf *pixbuf = fits_render_pixbuf (argv[1], do_stretch, size);
    if (!pixbuf)
        return 1;

    GError *save_error = NULL;
    gboolean ok = gdk_pixbuf_save (pixbuf, argv[2], "png", &save_error, NULL);
    if (!ok && save_error) {
        g_printerr ("Save failed: %s\n", save_error->message);
        g_error_free (save_error);
    }
    g_object_unref (pixbuf);
    return ok ? 0 : 1;
}