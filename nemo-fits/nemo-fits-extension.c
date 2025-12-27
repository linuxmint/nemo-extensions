#include <config.h>
#include "nemo-fits-extension.h"

#include <libnemo-extension/nemo-property-page-provider.h>
#include <libnemo-extension/nemo-info-provider.h>
#include <libnemo-extension/nemo-name-and-desc-provider.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <fitsio.h>

static GType provider_types[1];

struct _NemoFitsExtension {
	GObject parent_slot;
};

struct _NemoFitsExtensionClass {
	GObjectClass parent_slot;
};

static void property_page_provider_iface_init (NemoPropertyPageProviderIface *iface);
static void info_provider_iface_init (NemoInfoProviderIface *iface);
static void name_and_desc_provider_iface_init (NemoNameAndDescProviderIface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (NemoFitsExtension, nemo_fits_extension, G_TYPE_OBJECT, 0,
	G_IMPLEMENT_INTERFACE_DYNAMIC (NEMO_TYPE_PROPERTY_PAGE_PROVIDER,
				       property_page_provider_iface_init)
	G_IMPLEMENT_INTERFACE_DYNAMIC (NEMO_TYPE_INFO_PROVIDER,
				       info_provider_iface_init)
	G_IMPLEMENT_INTERFACE_DYNAMIC (NEMO_TYPE_NAME_AND_DESC_PROVIDER,
				       name_and_desc_provider_iface_init))

static void
nemo_fits_extension_init (NemoFitsExtension *fits)
{
}

static void
nemo_fits_extension_class_init (NemoFitsExtensionClass *class)
{
}

static void
nemo_fits_extension_class_finalize (NemoFitsExtensionClass *class)
{
}

static gboolean
is_fits (const char *uri)
{
	return g_str_has_suffix (uri, ".fits") ||
	       g_str_has_suffix (uri, ".fit") ||
	       g_str_has_suffix (uri, ".fts") ||
	       g_str_has_suffix (uri, ".FITS") ||
	       g_str_has_suffix (uri, ".FIT") ||
	       g_str_has_suffix (uri, ".FTS");
}

static char *
read_keyword (fitsfile *fptr, const char *key)
{
	char value[FLEN_VALUE];
	int status = 0;
	
	return fits_read_key_str (fptr, key, value, NULL, &status) ? NULL : g_strdup (value);
}

static void
add_row (GtkGrid *grid, int row, const char *label, const char *value)
{
	GtkWidget *label_widget, *value_widget;
	char *markup;
	
	label_widget = gtk_label_new (NULL);
	markup = g_markup_printf_escaped ("<b>%s</b>", label);
	gtk_label_set_markup (GTK_LABEL (label_widget), markup);
	g_free (markup);
	gtk_widget_set_halign (label_widget, GTK_ALIGN_END);
	gtk_grid_attach (grid, label_widget, 0, row, 1, 1);
	
	value_widget = gtk_label_new (value ? value : _("No Info"));
	gtk_widget_set_halign (value_widget, GTK_ALIGN_START);
	gtk_label_set_selectable (GTK_LABEL (value_widget), TRUE);
	gtk_grid_attach (grid, value_widget, 1, row, 1, 1);
}

static void
add_separator (GtkGrid *grid, int row)
{
	GtkWidget *sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_margin_top (sep, 6);
	gtk_widget_set_margin_bottom (sep, 6);
	gtk_grid_attach (grid, sep, 0, row, 2, 1);
}

static GList *
get_pages (NemoPropertyPageProvider *provider, GList *files)
{
	NemoFileInfo *file;
	GtkWidget *grid;
	NemoPropertyPage *page;
	char *uri, *path, *value;
	fitsfile *fptr = NULL;
	int status = 0, naxis, bitpix, row = 0;
	long naxes[10];
	
	if (!files || files->next)
		return NULL;
	
	file = files->data;
	uri = nemo_file_info_get_uri (file);
	
	if (!is_fits (uri)) {
		g_free (uri);
		return NULL;
	}
	
	path = g_filename_from_uri (uri, NULL, NULL);
	g_free (uri);
	
	if (!path)
		return NULL;
	
	grid = gtk_grid_new ();
	gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
	gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
	gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
	
	if (fits_open_file (&fptr, path, READONLY, &status)) {
		g_free (path);
		add_row (GTK_GRID (grid), row++, _("Error:"), _("Could not open FITS file"));
		goto done;
	}
	
	if (!fits_read_key (fptr, TINT, "NAXIS", &naxis, NULL, &status) && naxis > 0) {
		GString *dims = g_string_new ("");
		for (int i = 1; i <= naxis && i <= 10; i++) {
			char key[9];
			g_snprintf (key, sizeof(key), "NAXIS%d", i);
			status = 0;
			if (!fits_read_key (fptr, TLONG, key, &naxes[i-1], NULL, &status)) {
				if (i > 1) g_string_append (dims, " x ");
				g_string_append_printf (dims, "%ld", naxes[i-1]);
			}
		}
		if (naxis == 2)
			g_string_append (dims, _(" (width x height)"));
		else if (naxis == 3)
			g_string_append (dims, _(" (width x height x depth)"));
		
		add_row (GTK_GRID (grid), row++, _("Dimensions:"), dims->str);
		g_string_free (dims, TRUE);
	}
	
	status = 0;
	{
		int xbin = 0, ybin = 0;
		int has_x = !fits_read_key (fptr, TINT, "XBINNING", &xbin, NULL, &status);
		status = 0;
		int has_y = !fits_read_key (fptr, TINT, "YBINNING", &ybin, NULL, &status);
		
		if (has_x || has_y) {
			char *bin = g_strdup_printf ("%d x %d", has_x ? xbin : 0, has_y ? ybin : 0);
			add_row (GTK_GRID (grid), row++, _("Binning:"), bin);
			g_free (bin);
		}
	}
	
	status = 0;
	if (!fits_read_key (fptr, TINT, "BITPIX", &bitpix, NULL, &status)) {
		const char *type = NULL;
		switch (bitpix) {
			case 8:   type = "8-bit unsigned integer"; break;
			case 16:  type = "16-bit signed integer"; break;
			case 32:  type = "32-bit signed integer"; break;
			case 64:  type = "64-bit signed integer"; break;
			case -32: type = "32-bit floating point"; break;
			case -64: type = "64-bit floating point"; break;
		}
		if (type)
			add_row (GTK_GRID (grid), row++, _("Data Type:"), _(type));
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
		int offset;
		if (!fits_read_key (fptr, TINT, "OFFSET", &offset, NULL, &status)) {
			char *str = g_strdup_printf ("%d", offset);
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
	gtk_widget_show_all (grid);
	
	page = nemo_property_page_new ("NemoFitsExtension::property_page",
	                               gtk_label_new (_("Image")), grid);
	
	return g_list_append (NULL, page);
}

static NemoOperationResult
update_file_info (NemoInfoProvider *provider, NemoFileInfo *file,
                  GClosure *update_complete, NemoOperationHandle **handle)
{
	return NEMO_OPERATION_COMPLETE;
}

static void
property_page_provider_iface_init (NemoPropertyPageProviderIface *iface)
{
	iface->get_pages = get_pages;
}

static void
info_provider_iface_init (NemoInfoProviderIface *iface)
{
	iface->update_file_info = update_file_info;
}

static GList *
get_name_and_desc (NemoNameAndDescProvider *provider)
{
	return g_list_append (NULL, g_strdup_printf ("nemo-fits-extension:::%s",
	                      _("View FITS image information from the properties tab")));
}

static void
name_and_desc_provider_iface_init (NemoNameAndDescProviderIface *iface)
{
	iface->get_name_and_desc = get_name_and_desc;
}

void
nemo_module_initialize (GTypeModule *module)
{
	nemo_fits_extension_register_type (module);
	provider_types[0] = NEMO_TYPE_FITS_EXTENSION;
	
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
}

void
nemo_module_shutdown (void)
{
}

void
nemo_module_list_types (const GType **types, int *num_types)
{
	*types = provider_types;
	*num_types = G_N_ELEMENTS (provider_types);
}
