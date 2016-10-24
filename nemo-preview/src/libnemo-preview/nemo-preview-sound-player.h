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

#ifndef __NEMO_PREVIEW_SOUND_PLAYER_H__
#define __NEMO_PREVIEW_SOUND_PLAYER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define NEMO_PREVIEW_TYPE_SOUND_PLAYER            (nemo_preview_sound_player_get_type ())
#define NEMO_PREVIEW_SOUND_PLAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NEMO_PREVIEW_TYPE_SOUND_PLAYER, NemoPreviewSoundPlayer))
#define NEMO_PREVIEW_IS_SOUND_PLAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NEMO_PREVIEW_TYPE_SOUND_PLAYER))
#define NEMO_PREVIEW_SOUND_PLAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  NEMO_PREVIEW_TYPE_SOUND_PLAYER, NemoPreviewSoundPlayerClass))
#define NEMO_PREVIEW_IS_SOUND_PLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  NEMO_PREVIEW_TYPE_SOUND_PLAYER))
#define NEMO_PREVIEW_SOUND_PLAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  NEMO_PREVIEW_TYPE_SOUND_PLAYER, NemoPreviewSoundPlayerClass))

typedef struct _NemoPreviewSoundPlayer          NemoPreviewSoundPlayer;
typedef struct _NemoPreviewSoundPlayerPrivate   NemoPreviewSoundPlayerPrivate;
typedef struct _NemoPreviewSoundPlayerClass     NemoPreviewSoundPlayerClass;

typedef enum
{
  NEMO_PREVIEW_SOUND_PLAYER_STATE_UNKNOWN = 0,
  NEMO_PREVIEW_SOUND_PLAYER_STATE_IDLE    = 1,
  NEMO_PREVIEW_SOUND_PLAYER_STATE_PLAYING = 2,
  NEMO_PREVIEW_SOUND_PLAYER_STATE_DONE    = 3,
  NEMO_PREVIEW_SOUND_PLAYER_STATE_ERROR   = 4
} NemoPreviewSoundPlayerState;

struct _NemoPreviewSoundPlayer
{
  GObject parent_instance;

  NemoPreviewSoundPlayerPrivate *priv;
};

struct _NemoPreviewSoundPlayerClass
{
  GObjectClass parent_class;
};

GType    nemo_preview_sound_player_get_type     (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __NEMO_PREVIEW_SOUND_PLAYER_H__ */
