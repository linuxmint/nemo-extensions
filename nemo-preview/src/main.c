/*
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 *
 * The NemoPreview project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and NemoPreview. This
 * permission is above and beyond the permissions granted by the GPL license
 * NemoPreview is covered by.
 *
 * Authors: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#include "config.h"

#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>

#include <girepository.h>

#include <cjs/gjs.h>

#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <X11/Xlib.h>
#endif

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <clutter-gst/clutter-gst.h>
#include <clutter-gtk/clutter-gtk.h>

static void
parse_options (int *argc, char ***argv)
{
  GOptionContext *ctx;
  GError *error = NULL;

  ctx = g_option_context_new (NULL);

  g_option_context_add_group (ctx, g_irepository_get_option_group ());

  if (!g_option_context_parse (ctx, argc, argv, &error))
    {
      g_print ("sushi: %s\n", error->message);
      exit(1);
    }

  g_option_context_free (ctx);
}

static void
register_all_viewers (GjsContext *ctx)
{
  GDir *dir;
  const gchar *name;
  gchar *path;
  GError *error = NULL;

  dir = g_dir_open (NEMO_PREVIEW_PKGDATADIR "/js/viewers", 0, &error);

  if (dir == NULL) {
    g_warning ("Can't open module directory: %s\n", error->message);
    g_error_free (error);
    return;
  }
 
  name = g_dir_read_name (dir);

  while (name != NULL) {
    path = g_build_filename (NEMO_PREVIEW_PKGDATADIR "/js/viewers",
                             name, NULL);
    if (!gjs_context_eval_file (ctx,
                                path,
                                NULL,
                                &error)) {
      g_warning ("Unable to parse viewer %s: %s", name, error->message);
      g_clear_error (&error);
    }

    g_free (path);
    name = g_dir_read_name (dir);
  }

  g_dir_close (dir);
}

int
main (int argc, char **argv)
{
  GjsContext *js_context;
  GError *error;

#ifdef GDK_WINDOWING_X11
  XInitThreads ();
#endif

  clutter_x11_set_use_argb_visual (TRUE);

  if (gtk_clutter_init (&argc, &argv) < 0)
    return EXIT_FAILURE;

  clutter_gst_init (0, NULL);

  parse_options (&argc, &argv);

  js_context = gjs_context_new_with_search_path (NULL);
  error = NULL;

  register_all_viewers (js_context);

  if (!gjs_context_eval (js_context,
                         "const Main = imports.ui.main;\n"
                         "Main.run();\n",
                         -1,
                         __FILE__,
                         NULL,
                         &error))
    g_error("Failed to load main javascript: %s", error->message);

  return EXIT_SUCCESS;
}
