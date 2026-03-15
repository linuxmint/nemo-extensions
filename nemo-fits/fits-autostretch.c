/*  fits-autostretch.c
 *
 *  Auto-stretch for FITS image data — implementation.
 *  Ported from KStars/Ekos stretch.cpp; algorithm based on PixInsight XISF
 *  spec §8.5.6–8.5.7:
 *  https://pixinsight.com/doc/docs/XISF-1.0-spec/XISF-1.0-spec.html
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "fits-autostretch.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Preset table: {B, C} pairs matching KStars presets 1–7.
 * B  = target midtone brightness
 * C  = contrast / clipping aggressiveness
 * ------------------------------------------------------------------------- */

static const struct { double B; double C; } stretch_presets[] = {
    { 0.25,  2.8 },  /* 1 – default                */
    { 0.25,  1.5 },  /* 2 – softer                 */
    { 0.125, 1.0 },  /* 3 – dark / subtle          */
    { 0.125, 0.5 },  /* 4 – very subtle            */
    { 0.125, 2.0 },  /* 5 – medium dark            */
    { 0.125, 4.0 },  /* 6 – aggressive             */
    { 0.25,  5.5 },  /* 7 – very aggressive        */
};

/* -------------------------------------------------------------------------
 * Image type detection
 * ------------------------------------------------------------------------- */

FitsImageType
fits_detect_image_type (fitsfile *fptr, int naxis, const long *naxes)
{
    char bayer[FLEN_VALUE];
    int status = 0;

    if (naxis == 2) {
        if (!fits_read_key_str (fptr, "BAYERPAT", bayer, NULL, &status))
            return IMAGE_BAYER;
        return IMAGE_GREY;
    }

    if (naxis == 3) {
        if (naxes[2] == 3) return IMAGE_RGB;
        if (naxes[2] == 1) return IMAGE_GREY;   /* degenerate third axis */
    }

    return IMAGE_UNKNOWN;
}

/* -------------------------------------------------------------------------
 * Buffer helpers
 * ------------------------------------------------------------------------- */

void
fits_find_range (const double *buf, long n, double *min_out, double *max_out)
{
    double mn =  INFINITY;
    double mx = -INFINITY;

    for (long i = 0; i < n; i++) {
        if (isfinite (buf[i])) {
            if (buf[i] < mn) mn = buf[i];
            if (buf[i] > mx) mx = buf[i];
        }
    }
    *min_out = mn;
    *max_out = mx;
}

void
fits_normalise_plane (const double *raw, long n,
                      double min_val, double range_inv, double *out)
{
    for (long i = 0; i < n; i++)
        out[i] = isfinite (raw[i]) ? (raw[i] - min_val) * range_inv : 0.0;
}

unsigned char *
fits_grey_to_rgb_flipped (const unsigned char *grey, long width, long height)
{
    long npixels       = width * height;
    unsigned char *rgb = (unsigned char *)malloc (npixels * 3);
    if (!rgb) return NULL;

    for (long i = 0; i < npixels; i++) {
        long row = i / width;
        long col = i % width;
        long dst = (height - 1 - row) * width + col;
        rgb[dst * 3]     = grey[i];
        rgb[dst * 3 + 1] = grey[i];
        rgb[dst * 3 + 2] = grey[i];
    }
    return rgb;
}

unsigned char *
fits_rgb_flip_vertical (const unsigned char *rgb_in, long width, long height)
{
    long row_bytes     = width * 3;
    unsigned char *out = (unsigned char *)malloc (height * row_bytes);
    if (!out) return NULL;

    for (long row = 0; row < height; row++)
        memcpy (out + (height - 1 - row) * row_bytes,
                rgb_in + row * row_bytes,
                row_bytes);
    return out;
}

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

static int
stretch_cmp_double (const void *a, const void *b)
{
    double da = *(const double *)a;
    double db = *(const double *)b;
    return (da > db) - (da < db);
}

static double
stretch_median (const double *buf, long n, int sample_by)
{
    long n_samples = n / sample_by;
    if (n_samples < 1) n_samples = 1;

    double *tmp = (double *)malloc (n_samples * sizeof(double));
    if (!tmp) return 0.0;

    for (long i = 0, idx = 0; i < n_samples; i++, idx += sample_by)
        tmp[i] = buf[idx];

    qsort (tmp, n_samples, sizeof(double), stretch_cmp_double);
    double med = tmp[n_samples / 2];
    free (tmp);
    return med;
}

/* Core parameter computation for a single normalised (0..1) pixel plane. */
static void
stretch_params_one (const double *buf, long n, int preset, StretchParams *out)
{
    if (preset < 1) preset = 1;
    if (preset > STRETCH_NUM_PRESETS) preset = STRETCH_NUM_PRESETS;

    const double B = stretch_presets[preset - 1].B;
    const double C = stretch_presets[preset - 1].C;

    out->shadows    = 0.0;
    out->highlights = 1.0;
    out->midtones   = 0.5;

    if (n <= 0) return;

    const long max_samples = 500000L;
    int sample_by = (n < max_samples) ? 1 : (int)(n / max_samples);

    double med = stretch_median (buf, n, sample_by);

    long n_samples = n / sample_by;
    double *dev = (double *)malloc (n_samples * sizeof(double));
    if (!dev) return;

    for (long i = 0, idx = 0; i < n_samples; i++, idx += sample_by) {
        double d = buf[idx] - med;
        dev[i] = d < 0.0 ? -d : d;
    }
    qsort (dev, n_samples, sizeof(double), stretch_cmp_double);
    double MADN = 1.4826 * dev[n_samples / 2];
    free (dev);

    int upper_half = (med > 0.5);

    double shadows, highlights;
    if (upper_half || MADN == 0.0)
        shadows = 0.0;
    else
        shadows = fmin (1.0, fmax (0.0, med - C * MADN));

    if (!upper_half || MADN == 0.0)
        highlights = 1.0;
    else
        highlights = fmin (1.0, fmax (0.0, med + C * MADN));

    double X, M;
    if (!upper_half) { X = med - shadows;  M = B; }
    else             { X = B;              M = highlights - med; }

    double midtones;
    if      (X == 0.0) midtones = 0.0;
    else if (X == M)   midtones = 0.5;
    else if (X == 1.0) midtones = 1.0;
    else               midtones = ((M - 1.0) * X) / ((2.0 * M - 1.0) * X - M);

    out->shadows    = shadows;
    out->highlights = highlights;
    out->midtones   = midtones;
}

/* Apply one channel's stretch params to a normalised pixel plane.
 *
 * in         : normalised input plane (double, length n)
 * n          : number of pixels
 * p          : stretch parameters
 * out        : first byte of this channel in the output buffer
 * out_stride : byte distance between successive output pixels for this channel.
 *              Use 1 for a packed greyscale plane; use 3 for an interleaved
 *              RGB buffer, pointing out at the R, G, or B byte. */
static void
stretch_apply_one (const double *in, long n, const StretchParams *p,
                   unsigned char *out, int out_stride)
{
    const double shadows    = p->shadows;
    const double highlights = p->highlights;
    const double midtones   = p->midtones;

    const double hs_inv = (highlights == shadows) ? 1.0
                                                   : 1.0 / (highlights - shadows);
    const double k1 = (midtones - 1.0) * hs_inv * 255.0;
    const double k2 = (2.0 * midtones - 1.0) * hs_inv;

    for (long i = 0; i < n; i++) {
        double v = in[i];
        unsigned char val;

        if (v < shadows) {
            val = 0;
        } else if (v >= highlights) {
            val = 255;
        } else {
            double vf = v - shadows;
            double s  = (vf * k1) / (vf * k2 - midtones);
            if      (s < 0.0)   val = 0;
            else if (s > 255.0) val = 255;
            else                val = (unsigned char)s;
        }

        out[i * out_stride] = val;
    }
}

/* -------------------------------------------------------------------------
 * Public API — 1-channel (greyscale)
 * ------------------------------------------------------------------------- */

void
fits_autostretch_params (const double *buf, long n, int preset,
                         StretchParams *out)
{
    stretch_params_one (buf, n, preset, out);
}

void
fits_autostretch_apply (const double *buf, long n,
                        const StretchParams *p, unsigned char *out_grey)
{
    stretch_apply_one (buf, n, p, out_grey, 1);
}

void
fits_autostretch (const double *buf, long n, int preset, unsigned char *out_grey)
{
    StretchParams p;
    stretch_params_one (buf, n, preset, &p);
    stretch_apply_one  (buf, n, &p, out_grey, 1);
}

/* -------------------------------------------------------------------------
 * Public API — 3-channel (planar RGB)
 * ------------------------------------------------------------------------- */

void
fits_autostretch_params3 (const double *planes, long npixels, int preset,
                          StretchParams3 *out)
{
    stretch_params_one (planes,               npixels, preset, &out->r);
    stretch_params_one (planes + npixels,     npixels, preset, &out->g);
    stretch_params_one (planes + npixels * 2, npixels, preset, &out->b);
}

void
fits_autostretch_apply3 (const double *planes, long npixels,
                         const StretchParams3 *p, unsigned char *out_rgb)
{
    stretch_apply_one (planes,               npixels, &p->r, out_rgb + 0, 3);
    stretch_apply_one (planes + npixels,     npixels, &p->g, out_rgb + 1, 3);
    stretch_apply_one (planes + npixels * 2, npixels, &p->b, out_rgb + 2, 3);
}

void
fits_autostretch3 (const double *planes, long npixels, int preset,
                   unsigned char *out_rgb)
{
    StretchParams3 p;
    fits_autostretch_params3 (planes, npixels, preset, &p);
    fits_autostretch_apply3  (planes, npixels, &p, out_rgb);
}