/*  fits-autostretch.h
 *
 *  Auto-stretch for FITS image data — public interface.
 *  Ported from KStars/Ekos stretch.cpp; algorithm based on PixInsight XISF
 *  spec §8.5.6–8.5.7:
 *  https://pixinsight.com/doc/docs/XISF-1.0-spec/XISF-1.0-spec.html
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *
 *  Operates on double-precision input buffers, outputs 8-bit pixel data.
 *
 *  Supports greyscale (1-channel) and RGB (3-channel, planar) images.
 *  For 3-channel images, planes are stored sequentially:
 *    all R pixels, then all G pixels, then all B pixels (FITS cube convention).
 */

#pragma once

#include <fitsio.h>

/* -------------------------------------------------------------------------
 * Image type detection
 * ------------------------------------------------------------------------- */

typedef enum {
    IMAGE_GREY,    /* NAXIS=2, or NAXIS=3 with NAXIS3=1                   */
    IMAGE_RGB,     /* NAXIS=3 with NAXIS3=3 (sequential colour planes)    */
    IMAGE_BAYER,   /* NAXIS=2 with BAYERPAT keyword — rendered as grey    */
    IMAGE_UNKNOWN  /* anything else: attempt greyscale of first plane     */
} FitsImageType;

/* Inspect an open fitsfile and return how its image data should be rendered. */
FitsImageType fits_detect_image_type (fitsfile *fptr, int naxis,
                                      const long *naxes);

/* -------------------------------------------------------------------------
 * Buffer helpers
 * ------------------------------------------------------------------------- */

/* Walk buf of length n and return the min and max finite values. */
void fits_find_range (const double *buf, long n,
                      double *min_out, double *max_out);

/* Normalise n values from raw into out using pre-computed min and reciprocal
 * range.  Non-finite pixels are clamped to 0. */
void fits_normalise_plane (const double *raw, long n,
                           double min_val, double range_inv, double *out);

/* Convert a bottom-up greyscale byte plane into a top-down interleaved RGB
 * buffer (R=G=B).  Returns a malloc'd buffer the caller must free, or NULL
 * on allocation failure. */
unsigned char *fits_grey_to_rgb_flipped (const unsigned char *grey,
                                         long width, long height);

/* Flip a bottom-up interleaved RGB buffer to top-down order.
 * Returns a malloc'd buffer the caller must free, or NULL on failure. */
unsigned char *fits_rgb_flip_vertical (const unsigned char *rgb_in,
                                       long width, long height);

/* -------------------------------------------------------------------------
 * Stretch parameters
 * ------------------------------------------------------------------------- */

typedef struct {
    double shadows;
    double highlights;
    double midtones;
} StretchParams;

typedef struct {
    StretchParams r;
    StretchParams g;
    StretchParams b;
} StretchParams3;

/* Number of available presets (1-based index passed to API calls below).
 * Preset 1 (B=0.25, C=2.8) is the recommended default. */
#define STRETCH_NUM_PRESETS 7

/* -------------------------------------------------------------------------
 * Public API — 1-channel (greyscale)
 * ------------------------------------------------------------------------- */

/* Compute stretch parameters for a single normalised (0..1) pixel plane. */
void fits_autostretch_params (const double *buf, long n, int preset,
                              StretchParams *out);

/* Apply stretch parameters to buf, writing 8-bit results to out_grey. */
void fits_autostretch_apply (const double *buf, long n,
                             const StretchParams *p, unsigned char *out_grey);

/* Compute params and apply in one call. */
void fits_autostretch (const double *buf, long n, int preset,
                       unsigned char *out_grey);

/* -------------------------------------------------------------------------
 * Public API — 3-channel (planar RGB)
 *
 * Input convention (FITS cube layout):
 *   planes[0 .. npixels-1]            = R plane
 *   planes[npixels .. 2*npixels-1]    = G plane
 *   planes[2*npixels .. 3*npixels-1]  = B plane
 *
 * Output convention:
 *   out_rgb is interleaved: R0 G0 B0 R1 G1 B1 ...  (length 3*npixels bytes)
 * ------------------------------------------------------------------------- */

/* Compute independent stretch parameters for each of three planar channels. */
void fits_autostretch_params3 (const double *planes, long npixels, int preset,
                               StretchParams3 *out);

/* Apply per-channel stretch parameters to three planar inputs, writing
 * interleaved 8-bit RGB output. */
void fits_autostretch_apply3 (const double *planes, long npixels,
                              const StretchParams3 *p, unsigned char *out_rgb);

/* Compute params and apply in one call (3-channel planar). */
void fits_autostretch3 (const double *planes, long npixels, int preset,
                        unsigned char *out_rgb);