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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * The NemoPreview project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and NemoPreview. This
 * permission is above and beyond the permissions granted by the GPL license
 * NemoPreview is covered by.
 *
 * Authors: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#ifndef __NEMO_PREVIEW_COVER_ART_H__
#define __NEMO_PREVIEW_COVER_ART_H__

#include <glib-object.h>
#include <gst/tag/tag.h>

G_BEGIN_DECLS

#define NEMO_PREVIEW_TYPE_COVER_ART_FETCHER            (nemo_preview_cover_art_fetcher_get_type ())
#define NEMO_PREVIEW_COVER_ART_FETCHER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NEMO_PREVIEW_TYPE_COVER_ART_FETCHER, NemoPreviewCoverArtFetcher))
#define NEMO_PREVIEW_IS_COVER_ART_FETCHER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NEMO_PREVIEW_TYPE_COVER_ART_FETCHER))
#define NEMO_PREVIEW_COVER_ART_FETCHER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  NEMO_PREVIEW_TYPE_COVER_ART_FETCHER, NemoPreviewCoverArtFetcherClass))
#define NEMO_PREVIEW_IS_COVER_ART_FETCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  NEMO_PREVIEW_TYPE_COVER_ART_FETCHER))
#define NEMO_PREVIEW_COVER_ART_FETCHER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  NEMO_PREVIEW_TYPE_COVER_ART_FETCHER, NemoPreviewCoverArtFetcherClass))

typedef struct _NemoPreviewCoverArtFetcher          NemoPreviewCoverArtFetcher;
typedef struct _NemoPreviewCoverArtFetcherPrivate   NemoPreviewCoverArtFetcherPrivate;
typedef struct _NemoPreviewCoverArtFetcherClass     NemoPreviewCoverArtFetcherClass;

struct _NemoPreviewCoverArtFetcher
{
  GObject parent_instance;

  NemoPreviewCoverArtFetcherPrivate *priv;
};

struct _NemoPreviewCoverArtFetcherClass
{
  GObjectClass parent_class;
};

GType    nemo_preview_cover_art_fetcher_get_type     (void) G_GNUC_CONST;
NemoPreviewCoverArtFetcher* nemo_preview_cover_art_fetcher_new (GstTagList *taglist);

G_END_DECLS

#endif /* __NEMO_PREVIEW_COVER_ART_H__ */
