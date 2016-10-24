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

#include "nemo-preview-utils.h"

#include <gdk/gdkx.h>

static void
_cairo_round_rectangle (cairo_t *cr,
			gdouble	  x,
			gdouble	  y,
			gdouble	  w,
			gdouble	  h,
			gdouble	  radius)
{
	g_return_if_fail (cr != NULL);

	if (radius < 0.0001)
	{
		cairo_rectangle (cr, x, y, w, h);
		return;
	}

	cairo_move_to (cr, x+radius, y);
	cairo_arc (cr, x+w-radius, y+radius, radius, G_PI * 1.5, G_PI * 2);
	cairo_arc (cr, x+w-radius, y+h-radius, radius, 0, G_PI * 0.5);
	cairo_arc (cr, x+radius,   y+h-radius, radius, G_PI * 0.5, G_PI);
	cairo_arc (cr, x+radius,   y+radius,   radius, G_PI, G_PI * 1.5);
}

static void
rounded_background_draw_cb (ClutterCairoTexture *texture,
                            cairo_t *cr)
{
  ClutterActorBox allocation;

  clutter_actor_get_allocation_box (CLUTTER_ACTOR (texture), &allocation);
  clutter_cairo_texture_clear (CLUTTER_CAIRO_TEXTURE (texture));

  _cairo_round_rectangle (cr, allocation.x1, allocation.y1,
                          allocation.x2 - allocation.x1,
                          allocation.y2 - allocation.y1,
                          6.0);
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);

  cairo_fill (cr);
}

/**
 * nemo_preview_create_rounded_background:
 *
 * Returns: (transfer none): a #ClutterActor
 */
ClutterActor *
nemo_preview_create_rounded_background (void)
{
  ClutterActor *retval;

  retval = clutter_cairo_texture_new (1, 1);
  clutter_cairo_texture_set_auto_resize (CLUTTER_CAIRO_TEXTURE (retval), TRUE);

  g_signal_connect (retval, "draw",
                    G_CALLBACK (rounded_background_draw_cb), NULL);

  return retval;
}

/**
 * nemo_preview_create_foreign_window:
 * @xid:
 *
 * Returns: (transfer full): a #GdkWindow
 */
GdkWindow *
nemo_preview_create_foreign_window (guint xid)
{
  GdkWindow *retval;

  retval = gdk_x11_window_foreign_new_for_display (gdk_display_get_default (),
                                                   xid);

  return retval;
}

/**
 * nemo_preview_query_supported_document_types:
 *
 * Returns: (transfer full):
 */
gchar **
nemo_preview_query_supported_document_types (void)
{
  GList *infos, *l;
  gchar **retval = NULL;
  GPtrArray *array;
  EvTypeInfo *info;
  gint idx;

  infos = ev_backends_manager_get_all_types_info ();

  if (infos == NULL)
    return NULL;

  array = g_ptr_array_new ();

  for (l = infos; l != NULL; l = l->next) {
    info = l->data;

    for (idx = 0; info->mime_types[idx] != NULL; idx++)
      g_ptr_array_add (array, g_strdup (info->mime_types[idx]));
  }

  g_ptr_array_add (array, NULL);
  retval = (gchar **) g_ptr_array_free (array, FALSE);

  return retval;
}
