/*
 *  nemo-wipe - a nemo extension to wipe file(s)
 * 
 *  Copyright (C) 2009-2011 Colomban Wendling <ban@herbesfolles.org>
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

/* Contains compatibility things for old GLib, GTK and Nemo */

#ifndef NW_COMPAT_H
#define NW_COMPAT_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS


/* GLib stuff */

/* if GLib doesn't provide g_dngettext(), wrap it from dngettext() */
#if (! GLIB_CHECK_VERSION (2, 18, 0) && ! defined (g_dngettext))
# include <libintl.h>
# define g_dngettext dngettext
#endif


/* GTK stuff */

#if ! GTK_CHECK_VERSION(2, 14, 0)
# define gtk_dialog_get_action_area(dialog)   ((dialog)->action_area)
# define gtk_dialog_get_content_area(dialog)  ((dialog)->vbox)
#endif /* ! GTK_CHECK_VERSION(2, 14, 0) */

#if ! GTK_CHECK_VERSION (2, 18, 0)
# define gtk_widget_get_sensitive(w) (GTK_WIDGET_SENSITIVE (w))
#endif /* ! GTK_CHECK_VERSION (2, 18, 0) */

#if ! GTK_CHECK_VERSION (2, 13, 1)

static gboolean
gtk_show_uri (GdkScreen    *screen,
              const gchar  *uri,
              guint32       timestamp,
              GError      **error)
{
  gboolean  success;
  gchar    *quoted_uri;
  gchar    *cmd;
  
  quoted_uri = g_shell_quote (uri);
  cmd = g_strconcat ("xdg-open", " ", quoted_uri, NULL);
  g_free (quoted_uri);
  success = gdk_spawn_command_line_on_screen (screen, cmd, error);
  g_free (cmd);
  
  return success;
}

#endif


/* Nemo stuff */

#if ! (defined (HAVE_NEMO_FILE_INFO_GET_LOCATION) && \
       HAVE_NEMO_FILE_INFO_GET_LOCATION)
# undef HAVE_NEMO_FILE_INFO_GET_LOCATION
# define HAVE_NEMO_FILE_INFO_GET_LOCATION 1

#include <gio/gio.h>
#include <libnemo-extension/nemo-file-info.h>

static GFile *
nemo_file_info_get_location (NemoFileInfo *nfi)
{
  GFile *file;
  gchar *uri;
  
  uri = nemo_file_info_get_uri (nfi);
  file = g_file_new_for_uri (uri);
  g_free (uri);
  
  return file;
}

/* 
 * Workaround for the buggy behavior of g_file_get_path() on the GFile returned
 * by our nemo_file_info_get_location().
 * Should be harmless in general, and at least for us.
 * 
 * The buggy behavior made g_file_get_path() return the remote path for remote
 * locations, such as "/foo" for "ftp://name.domain.tld/foo", obviously leading
 * to really bad things such as unexpected data loss (by using a local file when
 * the user thinks we use the remote one).
 */
static gchar *
NEMO_WIPE_g_file_get_path (GFile *file)
{
  gchar *path = NULL;
  
  if (g_file_has_uri_scheme (file, "file")) {
    path = g_file_get_path (file);
  }
  
  return path;
}
/* overwrite the GIO implementation */
#define g_file_get_path NEMO_WIPE_g_file_get_path

#endif /* HAVE_NEMO_FILE_INFO_GET_LOCATION */


G_END_DECLS

#endif /* guard */
