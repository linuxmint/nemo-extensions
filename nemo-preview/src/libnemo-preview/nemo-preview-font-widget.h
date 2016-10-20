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

#ifndef __NEMO_PREVIEW_FONT_WIDGET_H__
#define __NEMO_PREVIEW_FONT_WIDGET_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include <cairo/cairo-ft.h>

G_BEGIN_DECLS

#define NEMO_PREVIEW_TYPE_FONT_WIDGET            (nemo_preview_font_widget_get_type ())
#define NEMO_PREVIEW_FONT_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NEMO_PREVIEW_TYPE_FONT_WIDGET, NemoPreviewFontWidget))
#define NEMO_PREVIEW_IS_FONT_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NEMO_PREVIEW_TYPE_FONT_WIDGET))
#define NEMO_PREVIEW_FONT_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  NEMO_PREVIEW_TYPE_FONT_WIDGET, NemoPreviewFontWidgetClass))
#define NEMO_PREVIEW_IS_FONT_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  NEMO_PREVIEW_TYPE_FONT_WIDGET))
#define NEMO_PREVIEW_FONT_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  NEMO_PREVIEW_TYPE_FONT_WIDGET, NemoPreviewFontWidgetClass))

typedef struct _NemoPreviewFontWidget          NemoPreviewFontWidget;
typedef struct _NemoPreviewFontWidgetPrivate   NemoPreviewFontWidgetPrivate;
typedef struct _NemoPreviewFontWidgetClass     NemoPreviewFontWidgetClass;

struct _NemoPreviewFontWidget
{
  GtkDrawingArea parent_instance;

  NemoPreviewFontWidgetPrivate *priv;
};

struct _NemoPreviewFontWidgetClass
{
  GtkDrawingAreaClass parent_class;
};

GType    nemo_preview_font_widget_get_type     (void) G_GNUC_CONST;

NemoPreviewFontWidget *nemo_preview_font_widget_new (const gchar *uri);

FT_Face nemo_preview_font_widget_get_ft_face (NemoPreviewFontWidget *self);

const gchar *nemo_preview_font_widget_get_uri (NemoPreviewFontWidget *self);

G_END_DECLS

#endif /* __NEMO_PREVIEW_FONT_WIDGET_H__ */
