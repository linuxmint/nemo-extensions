/*  fits-render.h
 *
 *  Shared FITS image rendering pipeline — public interface.
 *  Opens a FITS file, reads pixel data, normalises, optionally auto-stretches,
 *  flips to top-down order, and returns a GdkPixbuf ready for display or saving.
 *
 *  Used by both the nemo-fits property page extension and fits-thumbnailer.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <gdk-pixbuf/gdk-pixbuf.h>

/* Render a FITS file to a GdkPixbuf.
 *
 * path        : local filesystem path to the FITS file.
 * do_stretch  : if TRUE, apply auto-stretch (preset 1: B=0.25, C=2.8);
 *               if FALSE, apply linear normalisation only.
 * max_size    : if either image dimension exceeds this value, the pixbuf is
 *               scaled down (preserving aspect ratio) so that the longer side
 *               equals max_size.  Pass 0 to return the full native resolution.
 *
 * Returns a new GdkPixbuf the caller must g_object_unref(), or NULL on error. */
GdkPixbuf *fits_render_pixbuf (const char *path, gboolean do_stretch,
                                int max_size);