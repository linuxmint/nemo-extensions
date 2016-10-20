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

#ifndef __NEMO_PREVIEW_FILE_LOADER_H__
#define __NEMO_PREVIEW_FILE_LOADER_H__

#include <glib-object.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define NEMO_PREVIEW_TYPE_FILE_LOADER            (nemo_preview_file_loader_get_type ())
#define NEMO_PREVIEW_FILE_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NEMO_PREVIEW_TYPE_FILE_LOADER, NemoPreviewFileLoader))
#define NEMO_PREVIEW_IS_FILE_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NEMO_PREVIEW_TYPE_FILE_LOADER))
#define NEMO_PREVIEW_FILE_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  NEMO_PREVIEW_TYPE_FILE_LOADER, NemoPreviewFileLoaderClass))
#define NEMO_PREVIEW_IS_FILE_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  NEMO_PREVIEW_TYPE_FILE_LOADER))
#define NEMO_PREVIEW_FILE_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  NEMO_PREVIEW_TYPE_FILE_LOADER, NemoPreviewFileLoaderClass))

typedef struct _NemoPreviewFileLoader          NemoPreviewFileLoader;
typedef struct _NemoPreviewFileLoaderPrivate   NemoPreviewFileLoaderPrivate;
typedef struct _NemoPreviewFileLoaderClass     NemoPreviewFileLoaderClass;

struct _NemoPreviewFileLoader
{
  GObject parent_instance;

  NemoPreviewFileLoaderPrivate *priv;
};

struct _NemoPreviewFileLoaderClass
{
  GObjectClass parent_class;
};

GType    nemo_preview_file_loader_get_type     (void) G_GNUC_CONST;

NemoPreviewFileLoader *nemo_preview_file_loader_new (GFile *file);

gchar *nemo_preview_file_loader_get_display_name (NemoPreviewFileLoader *self);
gchar *nemo_preview_file_loader_get_size_string  (NemoPreviewFileLoader *self);
gchar *nemo_preview_file_loader_get_date_string  (NemoPreviewFileLoader *self);
gchar *nemo_preview_file_loader_get_content_type_string (NemoPreviewFileLoader *self);
GdkPixbuf *nemo_preview_file_loader_get_icon     (NemoPreviewFileLoader *self);
GFileType nemo_preview_file_loader_get_file_type (NemoPreviewFileLoader *self);

gboolean nemo_preview_file_loader_get_loading (NemoPreviewFileLoader *self);

void nemo_preview_file_loader_stop (NemoPreviewFileLoader *self);

G_END_DECLS

#endif /* __NEMO_PREVIEW_FILE_LOADER_H__ */
