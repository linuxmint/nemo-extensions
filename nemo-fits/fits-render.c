/*  fits-render.c
 *
 *  Shared FITS image rendering pipeline — implementation.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "fits-render.h"
#include "fits-autostretch.h"

#include <glib.h>
#include <fitsio.h>
#include <math.h>
#include <string.h>

/* Stretch preset used for all rendered thumbnails and previews.
 * Preset 1: B=0.25, C=2.8 — the KStars default. */
#define RENDER_STRETCH_PRESET 1

GdkPixbuf *
fits_render_pixbuf (const char *path, gboolean do_stretch, int max_size)
{
    fitsfile  *fptr   = NULL;
    int        status = 0, naxis;
    long       naxes[10];
    double    *raw    = NULL;
    double    *norm   = NULL;
    guchar    *ch_out = NULL;
    guchar    *rgb    = NULL;
    GdkPixbuf *pixbuf = NULL;
    GdkPixbuf *scaled = NULL;

    if (fits_open_file (&fptr, path, READONLY, &status))
        return NULL;

    /* Read NAXIS */
    if (fits_read_key (fptr, TINT, "NAXIS", &naxis, NULL, &status) || naxis < 2)
        goto done;

    /* Read NAXISn — treat any single key failure as fatal since we cannot
     * safely compute image dimensions with a partially initialised array. */
    for (int i = 1; i <= naxis && i <= 10; i++) {
        char key[9];
        g_snprintf (key, sizeof (key), "NAXIS%d", i);
        status = 0;
        if (fits_read_key (fptr, TLONG, key, &naxes[i - 1], NULL, &status))
            goto done;
    }

    {
        const long width   = naxes[0];
        const long height  = naxes[1];
        const long npixels = width * height;

        if (width == 0 || height == 0)
            goto done;

        FitsImageType itype   = fits_detect_image_type (fptr, naxis, naxes);
        const int     nplanes = (itype == IMAGE_RGB) ? 3 : 1;
        const long    total   = npixels * nplanes;

        /* Read all pixel data in one call */
        raw = g_malloc (total * sizeof (double));
        if (!raw) goto done;

        long fpixel[10] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        if (fits_read_pix (fptr, TDOUBLE, fpixel, total, NULL, raw, NULL, &status))
            goto done;

        /* Find data range */
        double min_val, max_val;
        fits_find_range (raw, total, &min_val, &max_val);

        if (!isfinite (min_val) || !isfinite (max_val) || max_val <= min_val) {
            /* Flat or empty image: return a mid-grey pixbuf */
            guchar *flat = g_malloc (npixels * 3);
            if (!flat) goto done;
            memset (flat, 128, npixels * 3);
            pixbuf = gdk_pixbuf_new_from_data (flat, GDK_COLORSPACE_RGB, FALSE,
                                               8, width, height, width * 3,
                                               (GdkPixbufDestroyNotify)g_free,
                                               NULL);
            if (!pixbuf) g_free (flat);
            goto scale_and_return;
        }

        /* Normalise to 0..1 */
        norm = g_malloc (total * sizeof (double));
        if (!norm) goto done;
        fits_normalise_plane (raw, total, min_val, 1.0 / (max_val - min_val), norm);

        /* Stretch and assemble */
        if (itype == IMAGE_RGB) {
            ch_out = g_malloc (npixels * 3);
            if (!ch_out) goto done;

            if (do_stretch) {
                fits_autostretch3 (norm, npixels, RENDER_STRETCH_PRESET, ch_out);
            } else {
                for (long i = 0; i < npixels; i++) {
                    ch_out[i * 3]     = (guchar)(norm[i]             * 255.0 + 0.5);
                    ch_out[i * 3 + 1] = (guchar)(norm[i + npixels]   * 255.0 + 0.5);
                    ch_out[i * 3 + 2] = (guchar)(norm[i + npixels*2] * 255.0 + 0.5);
                }
            }

            rgb = (guchar *)fits_rgb_flip_vertical (ch_out, width, height);
            g_free (ch_out);
            ch_out = NULL;

        } else {
            /* Greyscale path — also covers BAYER and UNKNOWN (first plane only) */
            ch_out = g_malloc (npixels);
            if (!ch_out) goto done;

            if (do_stretch)
                fits_autostretch (norm, npixels, RENDER_STRETCH_PRESET, ch_out);
            else {
                for (long i = 0; i < npixels; i++)
                    ch_out[i] = (guchar)(norm[i] * 255.0 + 0.5);
            }

            rgb = (guchar *)fits_grey_to_rgb_flipped (ch_out, width, height);
            g_free (ch_out);
            ch_out = NULL;
        }

        if (!rgb) goto done;

        pixbuf = gdk_pixbuf_new_from_data (rgb, GDK_COLORSPACE_RGB, FALSE,
                                           8, width, height, width * 3,
                                           (GdkPixbufDestroyNotify)free, NULL);
        if (!pixbuf) {
            free (rgb);
            goto done;
        }

scale_and_return:
        if (pixbuf && max_size > 0 &&
            (width > max_size || height > max_size)) {
            double sc  = MIN ((double)max_size / width,
                              (double)max_size / height);
            int    new_w = MAX (1, (int)(width  * sc));
            int    new_h = MAX (1, (int)(height * sc));

            scaled = gdk_pixbuf_scale_simple (pixbuf, new_w, new_h,
                                              GDK_INTERP_BILINEAR);
            g_object_unref (pixbuf);
            pixbuf = scaled;
        }
    }

done:
    g_free (ch_out);
    g_free (norm);
    g_free (raw);
    if (fptr) fits_close_file (fptr, &status);
    return pixbuf;
}