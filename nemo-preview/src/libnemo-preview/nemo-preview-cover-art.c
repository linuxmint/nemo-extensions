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

#include "nemo-preview-cover-art.h"

#include <musicbrainz5/mb5_c.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_DEFINE_TYPE (NemoPreviewCoverArtFetcher, nemo_preview_cover_art_fetcher, G_TYPE_OBJECT);

#define NEMO_PREVIEW_COVER_ART_FETCHER_GET_PRIVATE(obj)\
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NEMO_PREVIEW_TYPE_COVER_ART_FETCHER, NemoPreviewCoverArtFetcherPrivate))

enum {
  PROP_COVER = 1,
  PROP_TAGLIST,
};

struct _NemoPreviewCoverArtFetcherPrivate {
  GdkPixbuf *cover;
  GstTagList *taglist;

  gchar *asin;
  gboolean tried_cache;
  GInputStream *input_stream;
};

#define AMAZON_IMAGE_FORMAT "http://images.amazon.com/images/P/%s.01.LZZZZZZZ.jpg"

static void nemo_preview_cover_art_fetcher_set_taglist (NemoPreviewCoverArtFetcher *self,
                                                 GstTagList *taglist);
static void nemo_preview_cover_art_fetcher_get_uri_for_track_async (NemoPreviewCoverArtFetcher *self,
                                                             const gchar *artist,
                                                             const gchar *album,
                                                             GAsyncReadyCallback callback,
                                                             gpointer user_data);
static void try_read_from_file (NemoPreviewCoverArtFetcher *self,
                                GFile *file);

static void
nemo_preview_cover_art_fetcher_dispose (GObject *object)
{
  NemoPreviewCoverArtFetcherPrivate *priv = NEMO_PREVIEW_COVER_ART_FETCHER_GET_PRIVATE (object);

  g_clear_object (&priv->cover);
  g_clear_object (&priv->input_stream);

  if (priv->taglist != NULL) {
    gst_tag_list_free (priv->taglist);
    priv->taglist = NULL;
  }

  g_free (priv->asin);
  priv->asin = NULL;

  G_OBJECT_CLASS (nemo_preview_cover_art_fetcher_parent_class)->dispose (object);
}

static void
nemo_preview_cover_art_fetcher_get_property (GObject    *gobject,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  NemoPreviewCoverArtFetcher *self = NEMO_PREVIEW_COVER_ART_FETCHER (gobject);
  NemoPreviewCoverArtFetcherPrivate *priv = NEMO_PREVIEW_COVER_ART_FETCHER_GET_PRIVATE (self);

  switch (prop_id) {
  case PROP_COVER:
    g_value_set_object (value, priv->cover);
    break;
  case PROP_TAGLIST:
    g_value_set_boxed (value, priv->taglist);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    break;
  }
}

static void
nemo_preview_cover_art_fetcher_set_property (GObject    *gobject,
                                      guint       prop_id,
                                      const GValue *value,
                                      GParamSpec *pspec)
{
  NemoPreviewCoverArtFetcher *self = NEMO_PREVIEW_COVER_ART_FETCHER (gobject);

  switch (prop_id) {
  case PROP_TAGLIST:
    nemo_preview_cover_art_fetcher_set_taglist (self, g_value_get_boxed (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    break;
  }
}

static void
nemo_preview_cover_art_fetcher_init (NemoPreviewCoverArtFetcher *self)
{
  self->priv = NEMO_PREVIEW_COVER_ART_FETCHER_GET_PRIVATE (self);
}

static void
nemo_preview_cover_art_fetcher_class_init (NemoPreviewCoverArtFetcherClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = nemo_preview_cover_art_fetcher_get_property;
  gobject_class->set_property = nemo_preview_cover_art_fetcher_set_property;
  gobject_class->dispose = nemo_preview_cover_art_fetcher_dispose;

  g_object_class_install_property
    (gobject_class,
     PROP_COVER,
     g_param_spec_object ("cover",
                          "Cover art",
                          "Cover art for the current attrs",
                          GDK_TYPE_PIXBUF,
                          G_PARAM_READABLE));

  g_object_class_install_property
    (gobject_class,
     PROP_TAGLIST,
     g_param_spec_boxed ("taglist",
                         "Taglist",
                         "Current file tags",
                          GST_TYPE_TAG_LIST,
                          G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (NemoPreviewCoverArtFetcherPrivate));
}

typedef struct {
  NemoPreviewCoverArtFetcher *self;
  GSimpleAsyncResult *result;
  gchar *artist;
  gchar *album;
} FetchUriJob;

static void
fetch_uri_job_free (gpointer user_data)
{
  FetchUriJob *data = user_data;

  g_clear_object (&data->self);
  g_clear_object (&data->result);
  g_free (data->artist);
  g_free (data->album);

  g_slice_free (FetchUriJob, data);
}

static FetchUriJob *
fetch_uri_job_new (NemoPreviewCoverArtFetcher *self,
                   const gchar *artist,
                   const gchar *album,
                   GAsyncReadyCallback callback,
                   gpointer user_data)
{
  FetchUriJob *retval;

  retval = g_slice_new0 (FetchUriJob);
  retval->artist = g_strdup (artist);
  retval->album = g_strdup (album);
  retval->self = g_object_ref (self);
  retval->result = g_simple_async_result_new (G_OBJECT (self), callback, user_data,
                                              nemo_preview_cover_art_fetcher_get_uri_for_track_async);

  return retval;
}

static gboolean
fetch_uri_job_callback (gpointer user_data)
{
  FetchUriJob *job = user_data;

  g_simple_async_result_complete (job->result);
  fetch_uri_job_free (job);

  return FALSE;
}

static gboolean
fetch_uri_job (GIOSchedulerJob *sched_job,
               GCancellable *cancellable,
               gpointer user_data)
{
  FetchUriJob *job = user_data;
  Mb5Metadata metadata;
  Mb5Query query;
  Mb5Release release;
  Mb5ReleaseList release_list;
  gchar *retval = NULL;
  gchar **param_names = NULL;
  gchar **param_values = NULL;

  query = mb5_query_new ("sushi", NULL, 0);

  param_names = g_new (gchar*, 3);
  param_values = g_new (gchar*, 3);

  param_names[0] = g_strdup ("query");
  param_values[0] = g_strdup_printf ("artist:\"%s\" AND release:\"%s\"", job->artist, job->album);

  param_names[1] = g_strdup ("limit");
  param_values[1] = g_strdup ("10");

  param_names[2] = NULL;
  param_values[2] = NULL;

  metadata = mb5_query_query (query, "release", "", "",
                              2, param_names, param_values);

  mb5_query_delete (query);

  if (metadata) {
    release_list = mb5_metadata_get_releaselist (metadata);
    int i;
    int release_list_length = mb5_release_list_size (release_list);
    for (i = 0; i < release_list_length; i++) {
      gchar asin[255];

      release = mb5_release_list_item (release_list, i);
      mb5_release_get_asin (release, asin, 255);

      if (asin != NULL &&
        asin[0] != '\0') {
        retval = g_strdup (asin);
        break;
      }
    }
  }
  mb5_metadata_delete (metadata);

  if (retval == NULL) {
    /* FIXME: do we need a better error? */
    g_simple_async_result_set_error (job->result,
                                     G_IO_ERROR,
                                     0, "%s",
                                     "Error getting the ASIN from MusicBrainz");
  } else {
    g_simple_async_result_set_op_res_gpointer (job->result,
                                               retval, NULL);
  }

  g_io_scheduler_job_send_to_mainloop_async (sched_job,
                                             fetch_uri_job_callback,
                                             job, NULL);
  g_strfreev (param_names);
  g_strfreev (param_values);
  return FALSE;
}

static gchar *
nemo_preview_cover_art_fetcher_get_uri_for_track_finish (NemoPreviewCoverArtFetcher *self,
                                                  GAsyncResult *result,
                                                  GError **error)
{
  gchar *retval;

  if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result),
                                             error))
    return NULL;

  retval = g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result));

  return retval;
}

static void
nemo_preview_cover_art_fetcher_get_uri_for_track_async (NemoPreviewCoverArtFetcher *self,
                                                 const gchar *artist,
                                                 const gchar *album,
                                                 GAsyncReadyCallback callback,
                                                 gpointer user_data)
{
  FetchUriJob *job;

  job = fetch_uri_job_new (self, artist, album, callback, user_data);
  g_io_scheduler_push_job (fetch_uri_job,
                           job, NULL,
                           G_PRIORITY_DEFAULT, NULL);
}

static GFile *
get_gfile_for_amazon (NemoPreviewCoverArtFetcher *self)
{
  GFile *retval;
  gchar *uri;

  uri = g_strdup_printf (AMAZON_IMAGE_FORMAT, self->priv->asin);
  retval = g_file_new_for_uri (uri);
  g_free (uri);

  return retval;
}

static GFile *
get_gfile_for_cache (NemoPreviewCoverArtFetcher *self)
{
  GFile *retval;
  gchar *cache_path;
  gchar *filename, *path;

  cache_path = g_build_filename (g_get_user_cache_dir (),
                                 "sushi", NULL);
  g_mkdir_with_parents (cache_path, 0700);

  filename = g_strdup_printf ("%s.jpg", self->priv->asin);
  path = g_build_filename (cache_path, filename, NULL);
  retval = g_file_new_for_path (path);

  g_free (filename);
  g_free (path);
  g_free (cache_path);

  return retval;
}

static void
cache_splice_ready_cb (GObject *source,
                       GAsyncResult *res,
                       gpointer user_data)
{
  GError *error = NULL;

  g_output_stream_splice_finish (G_OUTPUT_STREAM (source),
                                 res, &error);

  if (error != NULL) {
    g_warning ("Can't save the cover art image in the cache: %s\n", error->message);
    g_error_free (error);

    return;
  }
}

static void
cache_replace_ready_cb (GObject *source,
                        GAsyncResult *res,
                        gpointer user_data)
{
  GFileOutputStream *cache_stream;
  GError *error = NULL;
  NemoPreviewCoverArtFetcher *self = user_data;

  cache_stream = g_file_replace_finish (G_FILE (source),
                                        res, &error);

  if (error != NULL) {
    g_warning ("Can't save the cover art image in the cache: %s\n", error->message);
    g_error_free (error);

    return;
  }

  g_seekable_seek (G_SEEKABLE (self->priv->input_stream), 0, G_SEEK_SET,
                   NULL, NULL);

  g_output_stream_splice_async (G_OUTPUT_STREAM (cache_stream), 
                                self->priv->input_stream,
                                G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE |
                                G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                G_PRIORITY_DEFAULT,
                                NULL,
                                cache_splice_ready_cb, self);

  g_object_unref (cache_stream);
}

static void
pixbuf_from_stream_async_cb (GObject *source,
                             GAsyncResult *res,
                             gpointer user_data)
{
  NemoPreviewCoverArtFetcher *self = user_data;
  NemoPreviewCoverArtFetcherPrivate *priv = NEMO_PREVIEW_COVER_ART_FETCHER_GET_PRIVATE (self);
  GError *error = NULL;
  GdkPixbuf *pix;
  GFile *file, *cache_file;

  pix = gdk_pixbuf_new_from_stream_finish (res, &error);

  if (error != NULL) {
    if (!self->priv->tried_cache) {
      self->priv->tried_cache = TRUE;

      file = get_gfile_for_amazon (self);
      try_read_from_file (self, file);

      g_object_unref (file);
    } else {
      g_print ("Unable to fetch Amazon cover art: %s\n", error->message);
    }

    g_error_free (error);
    return;
  }

  priv->cover = pix;
  g_object_notify (G_OBJECT (self), "cover");

  if (self->priv->tried_cache) {
    /* the pixbuf has been loaded. if we didn't hit the cache,
     * save it now.
     */
    cache_file = get_gfile_for_cache (self);
    g_file_replace_async (cache_file, 
                          NULL, FALSE,
                          G_FILE_CREATE_PRIVATE,
                          G_PRIORITY_DEFAULT,
                          NULL,
                          cache_replace_ready_cb,
                          self);

    g_object_unref (cache_file);
  }
}

static void
read_async_ready_cb (GObject *source,
                     GAsyncResult *res,
                     gpointer user_data)
{
  NemoPreviewCoverArtFetcher *self = user_data;
  NemoPreviewCoverArtFetcherPrivate *priv = NEMO_PREVIEW_COVER_ART_FETCHER_GET_PRIVATE (self);
  GFileInputStream *stream;
  GError *error = NULL;
  GFile *file;

  stream = g_file_read_finish (G_FILE (source),
                               res, &error);

  if (error != NULL) {
    if (!self->priv->tried_cache) {
      self->priv->tried_cache = TRUE;

      file = get_gfile_for_amazon (self);
      try_read_from_file (self, file);

      g_object_unref (file);
    } else {
      g_print ("Unable to fetch Amazon cover art: %s\n", error->message);
    }

    g_error_free (error);
    return;
  }

  priv->input_stream = G_INPUT_STREAM (stream);
  gdk_pixbuf_new_from_stream_async (priv->input_stream, NULL,
                                    pixbuf_from_stream_async_cb, self);
}

static void
try_read_from_file (NemoPreviewCoverArtFetcher *self,
                    GFile *file)
{
  g_file_read_async (file,
                     G_PRIORITY_DEFAULT, NULL,
                     read_async_ready_cb, self);
}

static void
cache_file_query_info_cb (GObject *source,
                          GAsyncResult *res,
                          gpointer user_data)
{
  GFileInfo *cache_info = NULL;
  NemoPreviewCoverArtFetcher *self = user_data;
  GError *error = NULL;
  GFile *file;

  cache_info = g_file_query_info_finish (G_FILE (source),
                                         res, &error);

  if (error != NULL) {
    self->priv->tried_cache = TRUE;
    file = get_gfile_for_amazon (self);
    g_error_free (error);
  } else {
    file = G_FILE (g_object_ref (source));
  }

  try_read_from_file (self, file);

  g_clear_object (&cache_info);
  g_object_unref (file);
}

static void
amazon_cover_uri_async_ready_cb (GObject *source,
                                 GAsyncResult *res,
                                 gpointer user_data)
{
  NemoPreviewCoverArtFetcher *self = NEMO_PREVIEW_COVER_ART_FETCHER (source);
  GError *error = NULL;
  GFile *file;

  self->priv->asin = nemo_preview_cover_art_fetcher_get_uri_for_track_finish
    (self, res, &error);

  if (error != NULL) {
    g_print ("Unable to fetch the Amazon cover art uri from MusicBrainz: %s\n",
             error->message);
    g_error_free (error);

    return;
  }

  file = get_gfile_for_cache (self);
  g_file_query_info_async (file, G_FILE_ATTRIBUTE_STANDARD_TYPE,
                           G_FILE_QUERY_INFO_NONE,
                           G_PRIORITY_DEFAULT, NULL, 
                           cache_file_query_info_cb,
                           self);

  g_object_unref (file);
}

static void
try_fetch_from_amazon (NemoPreviewCoverArtFetcher *self)
{
  NemoPreviewCoverArtFetcherPrivate *priv = NEMO_PREVIEW_COVER_ART_FETCHER_GET_PRIVATE (self);
  gchar *artist = NULL;
  gchar *album = NULL;

  gst_tag_list_get_string (priv->taglist,
                           GST_TAG_ARTIST, &artist);
  gst_tag_list_get_string (priv->taglist,
                           GST_TAG_ALBUM, &album);

  if (artist == NULL &&
      album == NULL) {
    /* don't even try */
    return;
  }

  nemo_preview_cover_art_fetcher_get_uri_for_track_async
    (self, artist, album,
     amazon_cover_uri_async_ready_cb, NULL);

  g_free (artist);
  g_free (album);
}

/* code taken from Totem; totem-gst-helpers.c
 *
 * Copyright (C) 2003-2007 the GStreamer project
 *      Julien Moutte <julien@moutte.net>
 *      Ronald Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2005-2008 Tim-Philipp Müller <tim centricular net>
 * Copyright (C) 2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>
 * Copyright © 2009 Christian Persch
 *
 * License: GPLv2+ with exception clause, see COPYING
 *
 */

static GdkPixbuf *
totem_gst_buffer_to_pixbuf (GstBuffer *buffer)
{
  GdkPixbufLoader *loader;
  GdkPixbuf *pixbuf = NULL;
  GError *err = NULL;
  GstMapInfo info;

  if (!gst_buffer_map (buffer, &info, GST_MAP_READ)) {
    GST_WARNING("could not map memory buffer");
    return NULL;
  }

  loader = gdk_pixbuf_loader_new ();

  if (gdk_pixbuf_loader_write (loader, info.data, info.size, &err) &&
      gdk_pixbuf_loader_close (loader, &err)) {
    pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    if (pixbuf)
      g_object_ref (pixbuf);
  } else {
    GST_WARNING("could not convert tag image to pixbuf: %s", err->message);
    g_error_free (err);
  }

  g_object_unref (loader);

  gst_buffer_unmap (buffer, &info);

  return pixbuf;
}

static GstSample *
totem_gst_tag_list_get_cover_real (GstTagList *tag_list)
{
  GstSample *cover_sample = NULL;
  guint i;

  for (i = 0; ; i++) {
    GstSample *sample;
    GstCaps *caps;
    const GstStructure *caps_struct;
    int type;

    if (!gst_tag_list_get_sample_index (tag_list, GST_TAG_IMAGE, i, &sample))
      break;

    caps = gst_sample_get_caps (sample);
    caps_struct = gst_caps_get_structure (caps, 0);
    gst_structure_get_enum (caps_struct,
			    "image-type",
			    GST_TYPE_TAG_IMAGE_TYPE,
			    &type);
    if (type == GST_TAG_IMAGE_TYPE_UNDEFINED) {
      if (cover_sample == NULL) {
	/* take a ref here since we will continue and unref below */
	cover_sample = gst_sample_ref (sample);
      }
    } else if (type == GST_TAG_IMAGE_TYPE_FRONT_COVER) {
      cover_sample = sample;
      break;
    }
    gst_sample_unref (sample);
  }

  return cover_sample;
}

GdkPixbuf *
totem_gst_tag_list_get_cover (GstTagList *tag_list)
{
  GstSample *cover_sample;

  g_return_val_if_fail (tag_list != NULL, FALSE);

  cover_sample = totem_gst_tag_list_get_cover_real (tag_list);
  /* Fallback to preview */
    if (!cover_sample) {
      gst_tag_list_get_sample_index (tag_list, GST_TAG_PREVIEW_IMAGE, 0,
				     &cover_sample);
    }

  if (cover_sample) {
    GstBuffer *buffer;
    GdkPixbuf *pixbuf;

    buffer = gst_sample_get_buffer (cover_sample);
    pixbuf = totem_gst_buffer_to_pixbuf (buffer);
    gst_sample_unref (cover_sample);
    return pixbuf;
  }

  return NULL;
}
/* */
static void
try_fetch_from_tags (NemoPreviewCoverArtFetcher *self)
{
  NemoPreviewCoverArtFetcherPrivate *priv = NEMO_PREVIEW_COVER_ART_FETCHER_GET_PRIVATE (self);

  if (priv->taglist == NULL)
    return;

  if (priv->cover != NULL)
    g_clear_object (&priv->cover);

  priv->cover = totem_gst_tag_list_get_cover (priv->taglist);

  if (priv->cover != NULL)
    g_object_notify (G_OBJECT (self), "cover");
  else
    try_fetch_from_amazon (self);
}

static void
nemo_preview_cover_art_fetcher_set_taglist (NemoPreviewCoverArtFetcher *self,
                                     GstTagList *taglist)
{
  NemoPreviewCoverArtFetcherPrivate *priv = NEMO_PREVIEW_COVER_ART_FETCHER_GET_PRIVATE (self);

  g_clear_object (&priv->cover);

  if (priv->taglist != NULL) {
    gst_tag_list_free (priv->taglist);
    priv->taglist = NULL;
  }

  priv->taglist = gst_tag_list_copy (taglist);
  try_fetch_from_tags (self);
}

NemoPreviewCoverArtFetcher *
nemo_preview_cover_art_fetcher_new (GstTagList *taglist)
{
  return g_object_new (NEMO_PREVIEW_TYPE_COVER_ART_FETCHER,
                       "taglist", taglist,
                       NULL);
}
