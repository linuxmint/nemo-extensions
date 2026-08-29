/*  nemo-fits.c
 *
 *  Nemo extension: FITS file property page with auto-stretch toggle.
 *  Supports greyscale and RGB (3-plane cube) FITS images.
 *  Settings are persisted via GKeyFile to ~/.config/nemo-fits/settings.ini
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config.h>
#include "nemo-fits.h"

#include <libnemo-extension/nemo-property-page-provider.h>
#include <libnemo-extension/nemo-info-provider.h>
#include <libnemo-extension/nemo-name-and-desc-provider.h>
#include <libnemo-extension/nemo-menu-provider.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <string.h>
#include <fitsio.h>

#include "fits-autostretch.h"
#include "fits-render.h"

/* -------------------------------------------------------------------------
 * Settings helpers
 * ------------------------------------------------------------------------- */

#define SETTINGS_GROUP  "nemo-fits"
#define SETTINGS_KEY    "autostretch"

static char *
get_settings_path (void)
{
	return g_build_filename (g_get_user_config_dir (),
	                         "nemo-fits", "settings.ini", NULL);
}

static gboolean
settings_get_stretch (void)
{
	char     *path = get_settings_path ();
	GKeyFile *kf   = g_key_file_new ();
	gboolean  val  = TRUE;   /* default: stretch enabled */

	if (g_key_file_load_from_file (kf, path, G_KEY_FILE_NONE, NULL))
		val = g_key_file_get_boolean (kf, SETTINGS_GROUP, SETTINGS_KEY, NULL);

	g_key_file_free (kf);
	g_free (path);
	return val;
}

static void
settings_set_stretch (gboolean enabled)
{
	char     *path  = get_settings_path ();
	char     *dir   = g_path_get_dirname (path);
	GKeyFile *kf    = g_key_file_new ();
	GError   *error = NULL;

	g_key_file_load_from_file (kf, path, G_KEY_FILE_NONE, NULL);
	g_key_file_set_boolean (kf, SETTINGS_GROUP, SETTINGS_KEY, enabled);

	g_mkdir_with_parents (dir, 0755);

	if (!g_key_file_save_to_file (kf, path, &error)) {
		g_warning ("nemo-fits: could not save settings to %s: %s",
		           path, error->message);
		g_error_free (error);
	}

	g_key_file_free (kf);
	g_free (dir);
	g_free (path);
}

/* -------------------------------------------------------------------------
 * GObject boilerplate
 * ------------------------------------------------------------------------- */

static GType provider_types[1];

struct _NemoFitsExtension      { GObject parent_slot; };
struct _NemoFitsExtensionClass { GObjectClass parent_slot; };

static void property_page_provider_iface_init (NemoPropertyPageProviderIface *iface);
static void info_provider_iface_init          (NemoInfoProviderIface *iface);
static void name_and_desc_provider_iface_init (NemoNameAndDescProviderIface *iface);
static void menu_provider_iface_init          (NemoMenuProviderIface *iface);
static int  delete_thumbnail_for_uri          (const char *uri);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (NemoFitsExtension, nemo_fits_extension, G_TYPE_OBJECT, 0,
	G_IMPLEMENT_INTERFACE_DYNAMIC (NEMO_TYPE_PROPERTY_PAGE_PROVIDER,
	                               property_page_provider_iface_init)
	G_IMPLEMENT_INTERFACE_DYNAMIC (NEMO_TYPE_INFO_PROVIDER,
	                               info_provider_iface_init)
	G_IMPLEMENT_INTERFACE_DYNAMIC (NEMO_TYPE_NAME_AND_DESC_PROVIDER,
	                               name_and_desc_provider_iface_init)
	G_IMPLEMENT_INTERFACE_DYNAMIC (NEMO_TYPE_MENU_PROVIDER,
	                               menu_provider_iface_init))

static void nemo_fits_extension_init           (NemoFitsExtension *e)      { }
static void nemo_fits_extension_class_init     (NemoFitsExtensionClass *c) { }
static void nemo_fits_extension_class_finalize (NemoFitsExtensionClass *c) { }

/* -------------------------------------------------------------------------
 * FITS helpers
 * ------------------------------------------------------------------------- */

static gboolean
is_fits_uri (const char *uri)
{
	return g_str_has_suffix (uri, ".fits") ||
	       g_str_has_suffix (uri, ".fit")  ||
	       g_str_has_suffix (uri, ".fts")  ||
	       g_str_has_suffix (uri, ".FITS") ||
	       g_str_has_suffix (uri, ".FIT")  ||
	       g_str_has_suffix (uri, ".FTS");
}

static char *
read_keyword (fitsfile *fptr, const char *key)
{
	char value[FLEN_VALUE];
	int status = 0;
	return fits_read_key_str (fptr, key, value, NULL, &status)
	       ? NULL : g_strdup (value);
}

/* -------------------------------------------------------------------------
 * UI helpers
 * ------------------------------------------------------------------------- */

static void
add_row (GtkGrid *grid, int row, const char *label, const char *value)
{
	GtkWidget *lw, *vw;
	char *markup;

	lw = gtk_label_new (NULL);
	markup = g_markup_printf_escaped ("<b>%s</b>", label);
	gtk_label_set_markup (GTK_LABEL (lw), markup);
	g_free (markup);
	gtk_widget_set_halign (lw, GTK_ALIGN_END);
	gtk_grid_attach (grid, lw, 0, row, 1, 1);

	vw = gtk_label_new (value ? value : _("No Info"));
	gtk_widget_set_halign (vw, GTK_ALIGN_START);
	gtk_label_set_selectable (GTK_LABEL (vw), TRUE);
	gtk_grid_attach (grid, vw, 1, row, 1, 1);
}

static void
add_separator (GtkGrid *grid, int row)
{
	GtkWidget *sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_margin_top    (sep, 6);
	gtk_widget_set_margin_bottom (sep, 6);
	gtk_grid_attach (grid, sep, 0, row, 2, 1);
}

/* -------------------------------------------------------------------------
 * Preview state
 * ------------------------------------------------------------------------- */

#define PREVIEW_SIZE 200

typedef struct {
	GtkImage     *preview_img;
	GtkWidget    *hint_label;   /* shown after first toggle */
	char         *fits_path;    /* owned */
	NemoFileInfo *file_info;    /* ref-counted, owned */
} PreviewState;

static void
preview_state_free (PreviewState *s)
{
	g_free (s->fits_path);
	if (s->file_info)
		g_object_unref (s->file_info);
	g_free (s);
}

static void
on_stretch_toggled (GObject *sw, GParamSpec *pspec, gpointer user_data)
{
	PreviewState *ps    = (PreviewState *)user_data;
	gboolean      state = gtk_switch_get_active (GTK_SWITCH (sw));

	settings_set_stretch (state);

	/* Invalidate the cached thumbnail so it is regenerated with the new
	 * stretch setting next time Nemo displays this file. */
	char *uri = g_filename_to_uri (ps->fits_path, NULL, NULL);
	if (uri) {
		delete_thumbnail_for_uri (uri);
		g_free (uri);
	}

	if (ps->file_info)
		nemo_file_info_invalidate_extension_info (ps->file_info);

	/* Reveal the refresh hint the first time (and every time) the user toggles */
	if (ps->hint_label)
		gtk_widget_show (ps->hint_label);

	GdkPixbuf *pb = fits_render_pixbuf (ps->fits_path, state, PREVIEW_SIZE);
	if (pb) {
		gtk_image_set_from_pixbuf (ps->preview_img, pb);
		g_object_unref (pb);
	}
}

/* -------------------------------------------------------------------------
 * Property page
 * ------------------------------------------------------------------------- */

static GList *
get_pages (NemoPropertyPageProvider *provider, GList *files)
{
	NemoFileInfo     *file;
	GtkWidget        *vbox, *grid, *frame, *preview_widget;
	NemoPropertyPage *page;
	char             *uri, *path, *value;
	fitsfile         *fptr   = NULL;
	int               status = 0, naxis, bitpix, row = 0;
	long              naxes[10];
	gboolean          stretch_on;

	if (!files || files->next)
		return NULL;

	file = files->data;
	uri  = nemo_file_info_get_uri (file);

	if (!is_fits_uri (uri)) {
		g_free (uri);
		return NULL;
	}

	path = g_filename_from_uri (uri, NULL, NULL);
	g_free (uri);
	if (!path) return NULL;

	stretch_on = settings_get_stretch ();

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

	/* --- Preview image --- */
	GdkPixbuf *pb = fits_render_pixbuf (path, stretch_on, PREVIEW_SIZE);
	preview_widget = gtk_image_new_from_pixbuf (pb);
	if (pb) g_object_unref (pb);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (frame), preview_widget);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	/* --- Stretch toggle row --- */
	{
		GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
		GtkWidget *lbl  = gtk_label_new (_("Auto-stretch:"));
		GtkWidget *sw   = gtk_switch_new ();

		gtk_widget_set_halign (lbl, GTK_ALIGN_START);
		gtk_widget_set_valign (sw,  GTK_ALIGN_CENTER);

		gtk_box_pack_start (GTK_BOX (hbox), lbl, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), sw,  FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

		/* Hint label — hidden until the user actually toggles the switch */
		GtkWidget *hint = gtk_label_new (_("Thumbnails update on next refresh (F5)"));
		gtk_widget_set_halign (hint, GTK_ALIGN_START);
		gtk_widget_set_sensitive (hint, FALSE);
		gtk_box_pack_start (GTK_BOX (vbox), hint, FALSE, FALSE, 0);
		gtk_widget_set_no_show_all (hint, TRUE);   /* excluded from show_all */

		PreviewState *ps = g_new0 (PreviewState, 1);
		ps->preview_img  = GTK_IMAGE (preview_widget);
		ps->hint_label   = hint;
		ps->fits_path    = g_strdup (path);
		ps->file_info    = g_object_ref (file);

		/* Set initial state before connecting the signal so construction
		 * does not trigger a spurious thumbnail deletion.
		 * notify::active fires only on genuine user-driven changes. */
		gtk_switch_set_active (GTK_SWITCH (sw), stretch_on);
		g_signal_connect_data (sw, "notify::active",
		                       G_CALLBACK (on_stretch_toggled), ps,
		                       (GClosureNotify)preview_state_free, 0);
	}

	/* --- Metadata grid --- */
	grid = gtk_grid_new ();
	gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
	gtk_grid_set_row_spacing    (GTK_GRID (grid), 6);
	gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);

	if (fits_open_file (&fptr, path, READONLY, &status)) {
		add_row (GTK_GRID (grid), row++, _("Error:"), _("Could not open FITS file"));
		goto done;
	}

	status = 0;
	if (!fits_read_key (fptr, TINT, "NAXIS", &naxis, NULL, &status) && naxis > 0) {
		GString *dims = g_string_new ("");
		for (int i = 1; i <= naxis && i <= 10; i++) {
			char key[9];
			g_snprintf (key, sizeof (key), "NAXIS%d", i);
			status = 0;
			if (!fits_read_key (fptr, TLONG, key, &naxes[i-1], NULL, &status)) {
				if (i > 1) g_string_append (dims, " x ");
				g_string_append_printf (dims, "%ld", naxes[i-1]);
			}
		}
		if      (naxis == 2) g_string_append (dims, _(" (width x height)"));
		else if (naxis == 3) g_string_append (dims, _(" (width x height x depth)"));
		add_row (GTK_GRID (grid), row++, _("Dimensions:"), dims->str);
		g_string_free (dims, TRUE);

		FitsImageType itype = fits_detect_image_type (fptr, naxis, naxes);
		const char *mode = NULL;
		switch (itype) {
			case IMAGE_GREY:    mode = _("Greyscale");                        break;
			case IMAGE_RGB:     mode = _("RGB (colour cube)");                break;
			case IMAGE_BAYER:   mode = _("Bayer mosaic (greyscale preview)"); break;
			case IMAGE_UNKNOWN: mode = _("Unknown (greyscale preview)");      break;
		}
		if (mode) add_row (GTK_GRID (grid), row++, _("Colour Mode:"), mode);
	}

	status = 0;
	{
		int xbin = 0, ybin = 0;
		int has_x = !fits_read_key (fptr, TINT, "XBINNING", &xbin, NULL, &status);
		status = 0;
		int has_y = !fits_read_key (fptr, TINT, "YBINNING", &ybin, NULL, &status);
		if (has_x || has_y) {
			char *bin = g_strdup_printf ("%d x %d",
			                             has_x ? xbin : 0,
			                             has_y ? ybin : 0);
			add_row (GTK_GRID (grid), row++, _("Binning:"), bin);
			g_free (bin);
		}
	}

	status = 0;
	if (!fits_read_key (fptr, TINT, "BITPIX", &bitpix, NULL, &status)) {
		const char *type = NULL;
		switch (bitpix) {
			case   8: type = "8-bit unsigned integer";  break;
			case  16: type = "16-bit signed integer";   break;
			case  32: type = "32-bit signed integer";   break;
			case  64: type = "64-bit signed integer";   break;
			case -32: type = "32-bit floating point";   break;
			case -64: type = "64-bit floating point";   break;
		}
		if (type) add_row (GTK_GRID (grid), row++, _("Data Type:"), _(type));
	}

	add_separator (GTK_GRID (grid), row++);

	value = read_keyword (fptr, "OBJECT");
	add_row (GTK_GRID (grid), row++, _("Object:"), value);
	g_free (value);

	value = read_keyword (fptr, "TELESCOP");
	add_row (GTK_GRID (grid), row++, _("Telescope:"), value);
	g_free (value);

	value = read_keyword (fptr, "INSTRUME");
	add_row (GTK_GRID (grid), row++, _("Instrument:"), value);
	g_free (value);

	value = read_keyword (fptr, "OBSERVER");
	add_row (GTK_GRID (grid), row++, _("Observer:"), value);
	g_free (value);

	add_separator (GTK_GRID (grid), row++);

	value = read_keyword (fptr, "DATE-OBS");
	add_row (GTK_GRID (grid), row++, _("Date Observed:"), value);
	g_free (value);

	status = 0;
	{
		double exp;
		if (!fits_read_key (fptr, TDOUBLE, "EXPTIME", &exp, NULL, &status)) {
			char *str = g_strdup_printf ("%.1f s", exp);
			add_row (GTK_GRID (grid), row++, _("Exposure Time:"), str);
			g_free (str);
		} else {
			add_row (GTK_GRID (grid), row++, _("Exposure Time:"), NULL);
		}
	}

	value = read_keyword (fptr, "FILTER");
	add_row (GTK_GRID (grid), row++, _("Filter:"), value);
	g_free (value);

	value = read_keyword (fptr, "BAYERPAT");
	add_row (GTK_GRID (grid), row++, _("Bayer Pattern:"), value);
	g_free (value);

	status = 0;
	{
		double gain;
		if (!fits_read_key (fptr, TDOUBLE, "GAIN", &gain, NULL, &status)) {
			char *str = g_strdup_printf ("%.2f", gain);
			add_row (GTK_GRID (grid), row++, _("Gain:"), str);
			g_free (str);
		} else {
			add_row (GTK_GRID (grid), row++, _("Gain:"), NULL);
		}
	}

	status = 0;
	{
		/* OFFSET is not a reserved FITS keyword; capture software commonly
		 * writes it as a floating-point value, so read as TDOUBLE. */
		double offset;
		if (!fits_read_key (fptr, TDOUBLE, "OFFSET", &offset, NULL, &status)) {
			char *str = g_strdup_printf ("%.2f", offset);
			add_row (GTK_GRID (grid), row++, _("Offset:"), str);
			g_free (str);
		} else {
			add_row (GTK_GRID (grid), row++, _("Offset:"), NULL);
		}
	}

	status = 0;
	{
		double temp;
		if (!fits_read_key (fptr, TDOUBLE, "CCD-TEMP", &temp, NULL, &status)) {
			char *str = g_strdup_printf ("%.2f °C", temp);
			add_row (GTK_GRID (grid), row++, _("CCD Temperature:"), str);
			g_free (str);
		} else {
			add_row (GTK_GRID (grid), row++, _("CCD Temperature:"), NULL);
		}
	}

	add_separator (GTK_GRID (grid), row++);
	fits_close_file (fptr, &status);

done:
	g_free (path);
	gtk_widget_show_all (vbox);

	page = nemo_property_page_new ("NemoFitsExtension::property_page",
	                               gtk_label_new (_("Image")), vbox);
	return g_list_append (NULL, page);
}

/* -------------------------------------------------------------------------
 * Thumbnail cache helpers
 * -------------------------------------------------------------------------
 *
 * Freedesktop thumbnail spec: thumbnails are stored as PNGs named after the
 * MD5 hash of the source file's canonical file:// URI, in:
 *   ~/.cache/thumbnails/normal/   (up to 128x128)
 *   ~/.cache/thumbnails/large/    (up to 256x256)
 * Deleting both forces Nemo to regenerate on next directory view.
 * ------------------------------------------------------------------------- */

static int
delete_thumbnail_for_uri (const char *uri)
{
	const char *sizes[] = { "normal", "large", NULL };
	char *hash  = g_compute_checksum_for_string (G_CHECKSUM_MD5, uri, -1);
	char *fname = g_strdup_printf ("%s.png", hash);
	int   count = 0;

	for (int i = 0; sizes[i]; i++) {
		char *thumb_path = g_build_filename (g_get_user_cache_dir (),
		                                     "thumbnails", sizes[i], fname,
		                                     NULL);
		if (g_unlink (thumb_path) == 0)
			count++;
		g_free (thumb_path);
	}

	g_free (fname);
	g_free (hash);
	return count;
}

/* -------------------------------------------------------------------------
 * Menu provider — single-file context menu item
 * ------------------------------------------------------------------------- */

static void
on_regenerate_file (NemoMenuItem *item, gpointer user_data)
{
	NemoFileInfo *file = NEMO_FILE_INFO (user_data);
	char *uri = nemo_file_info_get_uri (file);

	delete_thumbnail_for_uri (uri);
	nemo_file_info_invalidate_extension_info (file);

	g_free (uri);
}

static GList *
get_file_items (NemoMenuProvider *provider, GtkWidget *window, GList *files)
{
	/* Show "Regenerate FITS Thumbnail" when exactly one FITS file is selected */
	if (!files || files->next)
		return NULL;

	NemoFileInfo *file = files->data;
	char *uri = nemo_file_info_get_uri (file);

	if (!is_fits_uri (uri)) {
		g_free (uri);
		return NULL;
	}
	g_free (uri);

	NemoMenuItem *item = nemo_menu_item_new (
		"NemoFitsExtension::regenerate_thumbnail",
		_("Regenerate FITS Thumbnail"),
		_("Delete the cached thumbnail and force regeneration"),
		NULL);

	g_signal_connect_data (item, "activate",
	                       G_CALLBACK (on_regenerate_file),
	                       g_object_ref (file),
	                       (GClosureNotify)g_object_unref, 0);

	return g_list_append (NULL, item);
}

/* -------------------------------------------------------------------------
 * Interface wiring
 * ------------------------------------------------------------------------- */

static NemoOperationResult
update_file_info (NemoInfoProvider *provider, NemoFileInfo *file,
                  GClosure *update_complete, NemoOperationHandle **handle)
{
	return NEMO_OPERATION_COMPLETE;
}

static void property_page_provider_iface_init (NemoPropertyPageProviderIface *iface)
	{ iface->get_pages = get_pages; }

static void info_provider_iface_init (NemoInfoProviderIface *iface)
	{ iface->update_file_info = update_file_info; }

static GList *
get_name_and_desc (NemoNameAndDescProvider *provider)
{
	return g_list_append (NULL, g_strdup_printf ("nemo-fits:::%s",
	                      _("View FITS image information from the properties tab")));
}

static void name_and_desc_provider_iface_init (NemoNameAndDescProviderIface *iface)
	{ iface->get_name_and_desc = get_name_and_desc; }

static void
menu_provider_iface_init (NemoMenuProviderIface *iface)
{
	iface->get_file_items       = get_file_items;
	iface->get_background_items = NULL;
}

/* -------------------------------------------------------------------------
 * Module entry points
 * ------------------------------------------------------------------------- */

void
nemo_module_initialize (GTypeModule *module)
{
	nemo_fits_extension_register_type (module);
	provider_types[0] = NEMO_TYPE_FITS_EXTENSION;
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
}

void nemo_module_shutdown (void) { }

void
nemo_module_list_types (const GType **types, int *num_types)
{
	*types     = provider_types;
	*num_types = G_N_ELEMENTS (provider_types);
}