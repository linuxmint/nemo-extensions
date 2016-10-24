/*
 * Copyright (C) 2011 Lucas Rocha <lucasr@gnome.org>
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
 * Authors: Lucas Rocha <lucasr@gnome.org>
 *
 */

#include <glib.h>
#include <glib-object.h>

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#include "nemo-preview-enum-types.h"
#include "nemo-preview-sound-player.h"

G_DEFINE_TYPE (NemoPreviewSoundPlayer, nemo_preview_sound_player, G_TYPE_OBJECT);

#define NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE(obj)    \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), NEMO_PREVIEW_TYPE_SOUND_PLAYER, NemoPreviewSoundPlayerPrivate))

#define TICK_TIMEOUT 0.5

enum
{
  PROP_0,

  PROP_PLAYING,
  PROP_STATE,
  PROP_PROGRESS,
  PROP_DURATION,
  PROP_URI,
  PROP_TAGLIST
};

struct _NemoPreviewSoundPlayerPrivate
{
  GstElement            *pipeline;
  GstBus                *bus;
  NemoPreviewSoundPlayerState     state;
  char                  *uri;
  gboolean               playing;
  GstState               stacked_state;
  gdouble                stacked_progress;
  gdouble                target_progress;
  gdouble                duration;
  guint                  tick_timeout_id;

  GstDiscoverer         *discoverer;
  GstTagList            *taglist;

  guint                  in_seek : 1;
};

static void nemo_preview_sound_player_destroy_pipeline (NemoPreviewSoundPlayer *player);
static gboolean nemo_preview_sound_player_ensure_pipeline (NemoPreviewSoundPlayer *player);

static void
nemo_preview_sound_player_set_state (NemoPreviewSoundPlayer      *player,
                              NemoPreviewSoundPlayerState  state)
{
  NemoPreviewSoundPlayerPrivate *priv;

  g_return_if_fail (NEMO_PREVIEW_IS_SOUND_PLAYER (player));

  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (priv->state == state)
    return;

  priv->state = state;

  g_object_notify (G_OBJECT (player), "state");
}


static void
nemo_preview_sound_player_destroy_discoverer (NemoPreviewSoundPlayer *player)
{
  NemoPreviewSoundPlayerPrivate *priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (priv->discoverer == NULL)
    return;

  if (priv->taglist != NULL) {
    gst_tag_list_free (priv->taglist);
    priv->taglist = NULL;
  }

  gst_discoverer_stop (priv->discoverer);
  gst_object_unref (priv->discoverer);
  priv->discoverer = NULL;

  g_object_notify (G_OBJECT (player), "taglist");

  g_clear_object (&priv->taglist);
}

static void
discoverer_discovered_cb (GstDiscoverer *disco,
                          GstDiscovererInfo *info,
                          GError *error,
                          gpointer user_data)
{
  NemoPreviewSoundPlayer *player = user_data;
  NemoPreviewSoundPlayerPrivate *priv;
  const GstTagList *taglist;

  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (error != NULL)
    return;

  taglist = gst_discoverer_info_get_tags (info);

  if (taglist)
    {
      priv->taglist = gst_tag_list_copy (taglist);
      g_object_notify (G_OBJECT (player), "taglist");
    }
}

static gboolean
nemo_preview_sound_player_ensure_discoverer (NemoPreviewSoundPlayer *player)
{
  NemoPreviewSoundPlayerPrivate *priv;
  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (priv->discoverer)
    return TRUE;

  priv->discoverer = gst_discoverer_new (GST_SECOND * 60,
                                         NULL);

  if (priv->discoverer == NULL)
    return FALSE;

  g_signal_connect (priv->discoverer, "discovered",
                    G_CALLBACK (discoverer_discovered_cb), player);
  gst_discoverer_start (priv->discoverer);
  gst_discoverer_discover_uri_async (priv->discoverer, priv->uri);

  return TRUE;
}

static void
nemo_preview_sound_player_set_uri (NemoPreviewSoundPlayer *player,
                            const char    *uri)
{
  NemoPreviewSoundPlayerPrivate *priv;

  g_return_if_fail (NEMO_PREVIEW_IS_SOUND_PLAYER (player));

  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (!g_strcmp0 (priv->uri, uri))
    return;

  g_free (priv->uri);
  priv->uri = g_strdup (uri);

  if (priv->pipeline)
    nemo_preview_sound_player_destroy_pipeline (player);

  if (priv->discoverer)
    nemo_preview_sound_player_destroy_discoverer (player);

  nemo_preview_sound_player_ensure_pipeline (player);
  nemo_preview_sound_player_ensure_discoverer (player);

  g_object_notify (G_OBJECT (player), "uri");
}

static void
nemo_preview_sound_player_set_progress (NemoPreviewSoundPlayer *player,
                                 gdouble        progress)
{
  NemoPreviewSoundPlayerPrivate *priv;
  GstState pending;
  GstQuery *duration_q;
  gint64 position;

  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (!priv->pipeline)
    return;

  priv->target_progress = progress;

  if (priv->in_seek)
    {
      priv->stacked_progress = progress;
      return;
    }

  gst_element_get_state (priv->pipeline, &priv->stacked_state, &pending, 0);

  if (pending)
    priv->stacked_state = pending;

  gst_element_set_state (priv->pipeline, GST_STATE_PAUSED);

  duration_q = gst_query_new_duration (GST_FORMAT_TIME);

  if (gst_element_query (priv->pipeline, duration_q))
    {
      gint64 duration = 0;

      gst_query_parse_duration (duration_q, NULL, &duration);

      position = progress * duration;
    }
  else
    position = 0;

  gst_query_unref (duration_q);

  gst_element_seek (priv->pipeline,
		    1.0,
		    GST_FORMAT_TIME,
		    GST_SEEK_FLAG_FLUSH,
		    GST_SEEK_TYPE_SET,
		    position,
		    GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

  priv->in_seek = TRUE;
  priv->stacked_progress = 0.0;
}

static gdouble
nemo_preview_sound_player_get_progress (NemoPreviewSoundPlayer *player)
{
  NemoPreviewSoundPlayerPrivate *priv;
  GstQuery *position_q, *duration_q;
  gdouble progress;

  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (!priv->pipeline)
    return 0.0;

  if (priv->in_seek)
    {
      return priv->target_progress;
    }

  position_q = gst_query_new_position (GST_FORMAT_TIME);
  duration_q = gst_query_new_duration (GST_FORMAT_TIME);

  if (gst_element_query (priv->pipeline, position_q) &&
      gst_element_query (priv->pipeline, duration_q))
    {
      gint64 position, duration;

      position = duration = 0;

      gst_query_parse_position (position_q, NULL, &position);
      gst_query_parse_duration (duration_q, NULL, &duration);

      progress = CLAMP ((gdouble) position / (gdouble) duration, 0.0, 1.0);
    }
  else
    progress = 0.0;

  gst_query_unref (position_q);
  gst_query_unref (duration_q);

  return progress;
}

static void
nemo_preview_sound_player_query_duration (NemoPreviewSoundPlayer *player)
{
  NemoPreviewSoundPlayerPrivate *priv;
  gdouble new_duration, difference;
  gint64 duration;

  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (!gst_element_query_duration (priv->pipeline, GST_FORMAT_TIME, &duration))
    return;

  new_duration = (gdouble) duration / GST_SECOND;

  difference = ABS (priv->duration - new_duration);

  if (difference > 1e-3)
    {
      priv->duration = new_duration;

      if (difference > 1.0)
        g_object_notify (G_OBJECT (player), "duration");
    }
}

static void
nemo_preview_sound_player_reset_pipeline (NemoPreviewSoundPlayer *player)
{
  NemoPreviewSoundPlayerPrivate *priv;
  GstState state, pending;
  GstMessage *msg;

  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (!priv->pipeline)
    return;

  gst_element_get_state (priv->pipeline, &state, &pending, 0);

  if (state == GST_STATE_NULL && pending == GST_STATE_VOID_PENDING)
    {
      return;
    }
  else if (state == GST_STATE_NULL && pending != GST_STATE_VOID_PENDING)
    {
      gst_element_set_state (priv->pipeline, GST_STATE_NULL);
      return;
    }

  gst_element_set_state (priv->pipeline, GST_STATE_READY);
  gst_element_get_state (priv->pipeline, NULL, NULL, -1);

  while ((msg = gst_bus_pop (priv->bus)))
    gst_bus_async_signal_func (priv->bus, msg, NULL);

  gst_element_set_state (priv->pipeline, GST_STATE_NULL);

  g_object_notify (G_OBJECT (player), "duration");
  g_object_notify (G_OBJECT (player), "progress");
}

static void
nemo_preview_sound_player_destroy_pipeline (NemoPreviewSoundPlayer *player)
{
  NemoPreviewSoundPlayerPrivate *priv;

  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (priv->bus)
    {
      gst_bus_set_flushing (priv->bus, TRUE);
      gst_bus_remove_signal_watch (priv->bus);

      gst_object_unref (priv->bus);
      priv->bus = NULL;
    }

  if (priv->pipeline)
    {
      gst_element_set_state (priv->pipeline, GST_STATE_NULL);

      gst_object_unref (priv->pipeline);
      priv->pipeline = NULL;
    }

  if (priv->tick_timeout_id != 0)
    {
      g_source_remove (priv->tick_timeout_id);
      priv->tick_timeout_id = 0;
    }

  g_object_notify (G_OBJECT (player), "duration");
  g_object_notify (G_OBJECT (player), "progress");
}

static gboolean
nemo_preview_sound_player_tick_timeout (gpointer user_data)
{
  GObject *player = user_data;

  g_object_notify (player, "progress");

  return TRUE;
}

static void
nemo_preview_sound_player_on_state_changed (GstBus        *bus,
                                  GstMessage    *msg,
                                  NemoPreviewSoundPlayer *player)
{
  NemoPreviewSoundPlayerPrivate *priv;
  GstState state, old_state;

  g_return_if_fail (NEMO_PREVIEW_IS_SOUND_PLAYER (player));

  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (msg->src != GST_OBJECT (priv->pipeline))
    return;

  gst_message_parse_state_changed (msg, &old_state, &state, NULL);

  if (state == GST_STATE_PAUSED && old_state == GST_STATE_READY)
    nemo_preview_sound_player_query_duration (player);

  switch (state)
    {
    case GST_STATE_PLAYING:
      nemo_preview_sound_player_set_state (player, NEMO_PREVIEW_SOUND_PLAYER_STATE_PLAYING);

      if (priv->tick_timeout_id == 0)
        {
          priv->tick_timeout_id =
            g_timeout_add (TICK_TIMEOUT * 1000,
                           nemo_preview_sound_player_tick_timeout,
                           player);
        }
      break;

    case GST_STATE_READY:
    case GST_STATE_PAUSED:
      nemo_preview_sound_player_set_state (player, NEMO_PREVIEW_SOUND_PLAYER_STATE_IDLE);

      if (priv->tick_timeout_id != 0)
        {
          g_source_remove (priv->tick_timeout_id);
          priv->tick_timeout_id = 0;
        }
      break;

    default:
      /* Do nothing */
      break;
    }
}

static void
nemo_preview_sound_player_on_error (GstBus        *bus,
                          GstMessage    *msg,
                          NemoPreviewSoundPlayer *player)
{
  nemo_preview_sound_player_reset_pipeline (player);
  nemo_preview_sound_player_set_state (player, NEMO_PREVIEW_SOUND_PLAYER_STATE_ERROR);
}

static void
nemo_preview_sound_player_on_eos (GstBus        *bus,
                        GstMessage    *msg,
                        NemoPreviewSoundPlayer *player)
{
  g_object_notify (G_OBJECT (player), "progress");

  nemo_preview_sound_player_set_state (player, NEMO_PREVIEW_SOUND_PLAYER_STATE_DONE);
  nemo_preview_sound_player_reset_pipeline (player);
}

static void
nemo_preview_sound_player_on_async_done (GstBus        *bus,
                               GstMessage    *msg,
                               NemoPreviewSoundPlayer *player)
{
  NemoPreviewSoundPlayerPrivate *priv;

  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (priv->in_seek)
    {
      g_object_notify (G_OBJECT (player), "progress");

      priv->in_seek = FALSE;
      gst_element_set_state (priv->pipeline, priv->stacked_state);

      if (priv->stacked_progress)
        {
          nemo_preview_sound_player_set_progress (player, priv->stacked_progress);
        }
    }
}

static void
nemo_preview_sound_player_on_duration (GstBus        *bus,
                                GstMessage    *msg,
                                NemoPreviewSoundPlayer *player)
{
  gint64 duration;

  gst_message_parse_duration (msg, NULL, &duration);

  if (G_UNLIKELY (duration != GST_CLOCK_TIME_NONE))
    return;

  nemo_preview_sound_player_query_duration (player);
}

static gboolean
nemo_preview_sound_player_ensure_pipeline (NemoPreviewSoundPlayer *player)
{
  NemoPreviewSoundPlayerPrivate *priv;
  GError *error;
  gchar *pipeline_desc;

  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (priv->pipeline)
    return TRUE;

  if (priv->uri == NULL)
    {
      nemo_preview_sound_player_set_state (player, NEMO_PREVIEW_SOUND_PLAYER_STATE_ERROR);
      return FALSE;
    }

  error = NULL;

  pipeline_desc = g_strdup_printf("playbin uri=\"%s\"",
                                  priv->uri);

  priv->pipeline = gst_parse_launch (pipeline_desc, &error);

  g_free (pipeline_desc);

  if (error)
    {
      g_error_free (error);
      priv->pipeline = NULL;

      nemo_preview_sound_player_set_state (player, NEMO_PREVIEW_SOUND_PLAYER_STATE_ERROR);
      return FALSE;
    }

  if (!gst_element_set_state (priv->pipeline, GST_STATE_READY))
    {
      g_object_unref (priv->pipeline);
      priv->pipeline = NULL;

      nemo_preview_sound_player_set_state (player, NEMO_PREVIEW_SOUND_PLAYER_STATE_ERROR);
      return FALSE;
    }

  priv->bus = gst_element_get_bus (priv->pipeline);

  gst_bus_add_signal_watch (priv->bus);

  g_signal_connect (priv->bus,
                    "message::state-changed",
                    G_CALLBACK (nemo_preview_sound_player_on_state_changed),
                    player);

  g_signal_connect (priv->bus,
                    "message::error",
                    G_CALLBACK (nemo_preview_sound_player_on_error),
                    player);

  g_signal_connect (priv->bus,
                    "message::eos",
                    G_CALLBACK (nemo_preview_sound_player_on_eos),
                    player);

  g_signal_connect (priv->bus,
                    "message::async-done",
                    G_CALLBACK (nemo_preview_sound_player_on_async_done),
                    player);

  g_signal_connect (priv->bus,
                    "message::duration",
                    G_CALLBACK (nemo_preview_sound_player_on_duration),
                    player);

  /* Pause pipeline so that the file duration becomes
   * available as soon as possible */
  gst_element_set_state (priv->pipeline, GST_STATE_PAUSED);

  return TRUE;
}

void
nemo_preview_sound_player_set_playing (NemoPreviewSoundPlayer *player,
                             gboolean       playing)
{
  NemoPreviewSoundPlayerPrivate *priv;
  GstState state;

  g_return_if_fail (NEMO_PREVIEW_IS_SOUND_PLAYER (player));

  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (playing)
    state = GST_STATE_PLAYING;
  else
    state = GST_STATE_PAUSED;

  if (nemo_preview_sound_player_ensure_pipeline (player))
    gst_element_set_state (priv->pipeline, state);

  g_object_notify (G_OBJECT (player), "playing");
  g_object_notify (G_OBJECT (player), "progress");
}

static gboolean
nemo_preview_sound_player_get_playing (NemoPreviewSoundPlayer *player)
{
  NemoPreviewSoundPlayerPrivate *priv;
  GstState state, pending;
  gboolean playing;

  g_return_val_if_fail (NEMO_PREVIEW_IS_SOUND_PLAYER (player), FALSE);

  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  if (!priv->pipeline)
    return FALSE;

  gst_element_get_state (priv->pipeline, &state, &pending, 0);

  if (pending)
    playing = (pending == GST_STATE_PLAYING);
  else
    playing = (state == GST_STATE_PLAYING);

  return playing;
}

static void
nemo_preview_sound_player_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (nemo_preview_sound_player_parent_class)->finalize (gobject);
}

static void
nemo_preview_sound_player_dispose (GObject *gobject)
{
  nemo_preview_sound_player_destroy_pipeline (NEMO_PREVIEW_SOUND_PLAYER (gobject));
  nemo_preview_sound_player_destroy_discoverer (NEMO_PREVIEW_SOUND_PLAYER (gobject));

  G_OBJECT_CLASS (nemo_preview_sound_player_parent_class)->dispose (gobject);
}

static void
nemo_preview_sound_player_get_property (GObject    *gobject,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  NemoPreviewSoundPlayer *player;
  NemoPreviewSoundPlayerPrivate *priv;
  
  player = NEMO_PREVIEW_SOUND_PLAYER (gobject);
  priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  switch (prop_id)
    {
    case PROP_PLAYING:
      g_value_set_boolean (value,
                           nemo_preview_sound_player_get_playing (player));
      break;

    case PROP_STATE:
      g_value_set_enum (value, priv->state);
      break;

    case PROP_PROGRESS:
      g_value_set_double (value,
                          nemo_preview_sound_player_get_progress (player));
      break;

    case PROP_DURATION:
      g_value_set_double (value, priv->duration);
      break;

    case PROP_URI:
      g_value_set_string (value, priv->uri);
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
nemo_preview_sound_player_set_property (GObject      *gobject,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  NemoPreviewSoundPlayer *player = NEMO_PREVIEW_SOUND_PLAYER (gobject);

  switch (prop_id)
    {
    case PROP_PLAYING:
      nemo_preview_sound_player_set_playing (player,
                                      g_value_get_boolean (value));
      break;

    case PROP_PROGRESS:
      nemo_preview_sound_player_set_progress (player,
                                       g_value_get_double (value));
      break;

    case PROP_URI:
      nemo_preview_sound_player_set_uri (player,
                                  g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
nemo_preview_sound_player_class_init (NemoPreviewSoundPlayerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (NemoPreviewSoundPlayerPrivate));

  gobject_class->get_property = nemo_preview_sound_player_get_property;
  gobject_class->set_property = nemo_preview_sound_player_set_property;
  gobject_class->dispose = nemo_preview_sound_player_dispose;
  gobject_class->finalize = nemo_preview_sound_player_finalize;

  g_object_class_install_property
                 (gobject_class,
                  PROP_PLAYING,
                  g_param_spec_boolean ("playing",
                                        "Playing",
                                        "Whether player is playing or not",
                                        FALSE,
                                        G_PARAM_READWRITE));

  g_object_class_install_property
                 (gobject_class,
                  PROP_PROGRESS,
                  g_param_spec_double ("progress",
                                       "Progress",
                                       "Player's playback progress",
                                       0.0,
                                       1.0,
                                       0.0,
                                       G_PARAM_READWRITE));

  g_object_class_install_property
                 (gobject_class,
                  PROP_DURATION,
                  g_param_spec_double ("duration",
                                       "Duration",
                                       "Sound duration",
                                       0.0,
                                       G_MAXDOUBLE,
                                       0.0,
                                       G_PARAM_READABLE));

  g_object_class_install_property
                 (gobject_class,
                  PROP_STATE,
                  g_param_spec_enum ("state",
                                     "State",
                                     "State of the sound player",
                                     NEMO_PREVIEW_TYPE_PREVIEW_SOUND_PLAYER_STATE,
                                     NEMO_PREVIEW_SOUND_PLAYER_STATE_UNKNOWN,
                                     G_PARAM_READABLE));

  g_object_class_install_property
                 (gobject_class,
                  PROP_URI,
                  g_param_spec_string ("uri",
                                       "uri",
                                       "Uri to load sound from",
                                       NULL,
                                       G_PARAM_READWRITE |
                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property
    (gobject_class,
     PROP_TAGLIST,
     g_param_spec_boxed ("taglist",
                         "Taglist",
                         "Taglist for the current URI",
                         GST_TYPE_TAG_LIST,
                         G_PARAM_READABLE));
}

static void
nemo_preview_sound_player_init (NemoPreviewSoundPlayer *player)
{
  player->priv = NEMO_PREVIEW_SOUND_PLAYER_GET_PRIVATE (player);

  player->priv->state = NEMO_PREVIEW_SOUND_PLAYER_STATE_UNKNOWN;
  player->priv->playing = FALSE;
  player->priv->uri = NULL;
  player->priv->pipeline = NULL;
  player->priv->bus = NULL;
  player->priv->stacked_progress = 0.0;
  player->priv->duration = 0.0;
  player->priv->tick_timeout_id = 0;
}
