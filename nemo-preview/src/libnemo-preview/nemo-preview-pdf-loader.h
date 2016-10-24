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

#ifndef __NEMO_PREVIEW_PDF_LOADER_H__
#define __NEMO_PREVIEW_PDF_LOADER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define NEMO_PREVIEW_TYPE_PDF_LOADER            (nemo_preview_pdf_loader_get_type ())
#define NEMO_PREVIEW_PDF_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NEMO_PREVIEW_TYPE_PDF_LOADER, NemoPreviewPdfLoader))
#define NEMO_PREVIEW_IS_PDF_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NEMO_PREVIEW_TYPE_PDF_LOADER))
#define NEMO_PREVIEW_PDF_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  NEMO_PREVIEW_TYPE_PDF_LOADER, NemoPreviewPdfLoaderClass))
#define NEMO_PREVIEW_IS_PDF_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  NEMO_PREVIEW_TYPE_PDF_LOADER))
#define NEMO_PREVIEW_PDF_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  NEMO_PREVIEW_TYPE_PDF_LOADER, NemoPreviewPdfLoaderClass))

typedef struct _NemoPreviewPdfLoader          NemoPreviewPdfLoader;
typedef struct _NemoPreviewPdfLoaderPrivate   NemoPreviewPdfLoaderPrivate;
typedef struct _NemoPreviewPdfLoaderClass     NemoPreviewPdfLoaderClass;

struct _NemoPreviewPdfLoader
{
  GObject parent_instance;

  NemoPreviewPdfLoaderPrivate *priv;
};

struct _NemoPreviewPdfLoaderClass
{
  GObjectClass parent_class;
};

GType    nemo_preview_pdf_loader_get_type     (void) G_GNUC_CONST;

NemoPreviewPdfLoader *nemo_preview_pdf_loader_new (const gchar *uri);
void nemo_preview_pdf_loader_cleanup_document (NemoPreviewPdfLoader *self);
void nemo_preview_pdf_loader_get_max_page_size (NemoPreviewPdfLoader *self,
                                         gdouble *width,
                                         gdouble *height);


G_END_DECLS

#endif /* __NEMO_PREVIEW_PDF_LOADER_H__ */
