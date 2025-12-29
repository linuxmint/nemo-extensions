#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <fitsio.h>
#include <math.h>

#define MAX_SIZE 256

static gboolean
create_thumbnail (const char *input, const char *output, int size)
{
	fitsfile *fptr = NULL;
	int status = 0, naxis;
	long naxes[10], width, height, npixels;
	double *img_data = NULL;
	guchar *rgb_data = NULL;
	GdkPixbuf *pixbuf = NULL, *scaled = NULL;
	GError *error = NULL;
	gboolean success = FALSE;
	
	if (fits_open_file (&fptr, input, READONLY, &status))
		return FALSE;
	
	if (fits_read_key (fptr, TINT, "NAXIS", &naxis, NULL, &status) || naxis < 2)
		goto cleanup;
	
	for (int i = 1; i <= naxis && i <= 10; i++) {
		char key[9];
		g_snprintf (key, sizeof(key), "NAXIS%d", i);
		status = 0;
		if (fits_read_key (fptr, TLONG, key, &naxes[i-1], NULL, &status))
			goto cleanup;
	}
	
	width = naxes[0];
	height = naxes[1];
	npixels = width * height;
	
	if (width == 0 || height == 0)
		goto cleanup;
	
	img_data = g_malloc (npixels * sizeof(double));
	if (!img_data)
		goto cleanup;
	
	long fpixel[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
	if (fits_read_pix (fptr, TDOUBLE, fpixel, npixels, NULL, img_data, NULL, &status))
		goto cleanup_data;
	
	double min_val = INFINITY, max_val = -INFINITY;
	for (long i = 0; i < npixels; i++) {
		if (isfinite(img_data[i])) {
			if (img_data[i] < min_val) min_val = img_data[i];
			if (img_data[i] > max_val) max_val = img_data[i];
		}
	}
	
	rgb_data = g_malloc (npixels * 3);
	if (!rgb_data)
		goto cleanup_data;
	
	double range = max_val - min_val;
	if (range > 0) {
		for (long i = 0; i < npixels; i++) {
			guchar val = isfinite(img_data[i]) ?
			             (guchar)((img_data[i] - min_val) / range * 255.0) : 0;
			
			long row = i / width;
			long col = i % width;
			long idx = (height - 1 - row) * width + col;
			
			rgb_data[idx * 3] = rgb_data[idx * 3 + 1] = rgb_data[idx * 3 + 2] = val;
		}
	} else {
		memset (rgb_data, 128, npixels * 3);
	}
	
	pixbuf = gdk_pixbuf_new_from_data (rgb_data, GDK_COLORSPACE_RGB, FALSE,
	                                   8, width, height, width * 3,
	                                   (GdkPixbufDestroyNotify)g_free, NULL);
	if (!pixbuf) {
		g_free (rgb_data);
		goto cleanup_data;
	}
	
	if (width > size || height > size) {
		double scale = MIN((double)size / width, (double)size / height);
		int new_w = MAX(1, (int)(width * scale));
		int new_h = MAX(1, (int)(height * scale));
		
		scaled = gdk_pixbuf_scale_simple (pixbuf, new_w, new_h, GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		pixbuf = scaled;
	}
	
	success = gdk_pixbuf_save (pixbuf, output, "png", &error, NULL);
	if (!success && error) {
		g_warning ("Save failed: %s", error->message);
		g_error_free (error);
	}
	
	g_object_unref (pixbuf);

cleanup_data:
	g_free (img_data);
	
cleanup:
	if (fptr)
		fits_close_file (fptr, &status);
	
	return success;
}

int
main (int argc, char **argv)
{
	int size = MAX_SIZE;
	gboolean show_help = FALSE, show_version = FALSE;
	
	GOptionEntry entries[] = {
		{ "size", 's', 0, G_OPTION_ARG_INT, &size, "Thumbnail size (default: 256)", "SIZE" },
		{ "help", 'h', 0, G_OPTION_ARG_NONE, &show_help, "Show help", NULL },
		{ "version", 'v', 0, G_OPTION_ARG_NONE, &show_version, "Show version", NULL },
		{ NULL }
	};
	
	GOptionContext *context = g_option_context_new ("INPUT OUTPUT");
	g_option_context_add_main_entries (context, entries, NULL);
	
	GError *error = NULL;
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_printerr ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		g_option_context_free (context);
		return 1;
	}
	
	g_option_context_free (context);
	
	if (show_version) {
		g_print ("fits-thumbnailer 6.4.0\n");
		return 0;
	}
	
	if (show_help || argc != 3) {
		g_print ("Usage: fits-thumbnailer [OPTIONS] INPUT OUTPUT\n"
		         "Generate thumbnails for FITS image files\n\n"
		         "Options:\n"
		         "  -s, --size SIZE    Thumbnail size in pixels (default: 256)\n"
		         "  -h, --help         Show this help\n"
		         "  -v, --version      Show version\n");
		return show_help ? 0 : 1;
	}
	
	if (size < 1 || size > 2048) {
		g_printerr ("Invalid size: %d (must be 1-2048)\n", size);
		return 1;
	}
	
	return create_thumbnail (argv[1], argv[2], size) ? 0 : 1;
}
