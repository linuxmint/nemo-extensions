/*
 *   Copyright (C) 2007-2013 Tristan Heaven <tristanheaven@gmail.com>
 *
 *   This file is part of GtkHash.
 *
 *   GtkHash is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   GtkHash is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GtkHash. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#if IN_NAUTILUS_EXTENSION
	#include <libnautilus-extension/nautilus-property-page.h>
	#include <libnautilus-extension/nautilus-property-page-provider.h>
#elif IN_NEMO_EXTENSION
	#include <libnemo-extension/nemo-property-page.h>
	#include <libnemo-extension/nemo-property-page-provider.h>
#elif IN_THUNAR_EXTENSION
	#include <thunarx/thunarx.h>
#endif

#include "properties.h"
#include "properties-list.h"
#include "properties-hash.h"
#include "properties-prefs.h"
#include "../hash/hash-func.h"
#include "../hash/hash-file.h"

#define BUILDER_XML DATADIR "/gtkhash-properties.xml.gz"

static GType page_type;

static GObject *gtkhash_properties_get_object(GtkBuilder *builder,
	const char *name)
{
	GObject *obj = gtk_builder_get_object(builder, name);
	if (!obj)
		g_warning("unknown object: \"%s\"", name);

	return obj;
}

static void gtkhash_properties_busy(struct page_s *page)
{
	page->busy = true;

	gtk_widget_set_sensitive(GTK_WIDGET(page->button_hash), false);
	gtk_widget_set_sensitive(GTK_WIDGET(page->button_stop), true);
	gtk_widget_set_sensitive(GTK_WIDGET(page->treeview), false);
	gtk_widget_set_sensitive(GTK_WIDGET(page->hbox_inputs), false);

	// Reset progress bar
	gtk_progress_bar_set_fraction(page->progressbar, 0.0);
	gtk_progress_bar_set_text(page->progressbar, " ");
	gtk_widget_show(GTK_WIDGET(page->progressbar));
}

static void gtkhash_properties_button_hash_set_sensitive(struct page_s *page)
{
	bool has_enabled = false;

	for (int i = 0; i < HASH_FUNCS_N; i++) {
		if (page->hash_file.funcs[i].enabled) {
			has_enabled = true;
			break;
		}
	}

	gtk_widget_set_sensitive(GTK_WIDGET(page->button_hash), has_enabled);
}

static void gtkhash_properties_entry_hmac_set_sensitive(struct page_s *page)
{
	bool active = gtk_toggle_button_get_active(page->togglebutton_hmac);

	gtk_widget_set_sensitive(GTK_WIDGET(page->entry_hmac), active);
}

void gtkhash_properties_idle(struct page_s *page)
{
	page->busy = false;

	gtk_widget_hide(GTK_WIDGET(page->progressbar));

	gtk_widget_set_sensitive(GTK_WIDGET(page->button_stop), false);
	gtk_widget_set_sensitive(GTK_WIDGET(page->treeview), true);
	gtk_widget_set_sensitive(GTK_WIDGET(page->hbox_inputs), true);
	gtkhash_properties_entry_hmac_set_sensitive(page);
	gtkhash_properties_button_hash_set_sensitive(page);

	gtkhash_properties_list_check_digests(page);
}

static void gtkhash_properties_on_cell_toggled(struct page_s *page,
	char *path_str)
{
	gtkhash_properties_list_update_enabled(page, path_str);
	gtkhash_properties_list_check_digests(page);
	gtkhash_properties_button_hash_set_sensitive(page);
}

static void gtkhash_properties_on_treeview_popup_menu(struct page_s *page)
{
	gtk_menu_popup(page->menu, NULL, NULL, NULL, NULL, 0,
		gtk_get_current_event_time());
}

static bool gtkhash_properties_on_treeview_button_press_event(
	struct page_s *page, GdkEventButton *event)
{
	// Right click
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		gtk_menu_popup(page->menu, NULL, NULL, NULL, NULL, 3,
			gdk_event_get_time((GdkEvent *)event));
	}

	return false;
}

static void gtkhash_properties_on_treeselection_changed(struct page_s *page)
{
	bool sensitive = false;
	char *digest = gtkhash_properties_list_get_selected_digest(page);
	if (digest) {
		sensitive = true;
		g_free(digest);
	}

	gtk_widget_set_sensitive(GTK_WIDGET(page->menuitem_copy), sensitive);
}

static void gtkhash_properties_on_menuitem_copy_activate(struct page_s *page)
{
	GtkClipboard *clipboard = gtk_clipboard_get(GDK_NONE);
	char *digest = gtkhash_properties_list_get_selected_digest(page);

	gtk_clipboard_set_text(clipboard, digest, -1);

	g_free(digest);
}

static void gtkhash_properties_on_menuitem_show_funcs_toggled(
	struct page_s *page)
{
	gtkhash_properties_list_refilter(page);
}

static void gtkhash_properties_on_entry_check_changed(struct page_s *page)
{
	gtkhash_properties_list_check_digests(page);
}

static void gtkhash_properties_on_entry_hmac_changed(struct page_s *page)
{
	gtkhash_hash_file_clear_digests(&page->hash_file);
	gtkhash_properties_list_update_digests(page);
	gtkhash_properties_list_check_digests(page);
}

static void gtkhash_properties_on_togglebutton_hmac_toggled(struct page_s *page)
{
	gtkhash_properties_entry_hmac_set_sensitive(page);
	gtkhash_properties_on_entry_hmac_changed(page);
}

static void gtkhash_properties_on_button_hash_clicked(struct page_s *page)
{
	gtkhash_properties_busy(page);
	gtkhash_hash_file_clear_digests(&page->hash_file);
	gtkhash_properties_list_update_digests(page);

	if (gtk_toggle_button_get_active(page->togglebutton_hmac)) {
		const uint8_t *hmac_key = (uint8_t *)gtk_entry_get_text(
			page->entry_hmac);
		const size_t key_size = gtk_entry_get_text_length(page->entry_hmac);
		gtkhash_properties_hash_start(page, hmac_key, key_size);
	} else
		gtkhash_properties_hash_start(page, NULL, 0);
}

static void gtkhash_properties_on_button_stop_clicked(struct page_s *page)
{
	gtk_widget_set_sensitive(GTK_WIDGET(page->button_stop), false);
	gtkhash_properties_hash_stop(page);
}

static void gtkhash_properties_free_page(struct page_s *page)
{
	gtkhash_properties_hash_stop(page);

	while (page->busy)
		gtk_main_iteration();

	gtkhash_properties_prefs_deinit(page);
	gtkhash_properties_hash_deinit(page);
	g_free(page->uri);
	g_object_unref(page->menu);
	g_object_unref(page->box);
	g_free(page);
}

static void gtkhash_properties_get_objects(struct page_s *page,
	GtkBuilder *builder)
{
	// Main container
	page->box = GTK_WIDGET(gtkhash_properties_get_object(builder,
		"vbox"));

	// Progress bar
	page->progressbar = GTK_PROGRESS_BAR(gtkhash_properties_get_object(builder,
		"progressbar"));

	// Treeview
	page->treeview = GTK_TREE_VIEW(gtkhash_properties_get_object(builder,
		"treeview"));
	page->treeselection = GTK_TREE_SELECTION(gtkhash_properties_get_object(builder,
		"treeselection"));
	page->cellrendtoggle = GTK_CELL_RENDERER_TOGGLE(gtkhash_properties_get_object(builder,
		"cellrenderertoggle"));

	// Popup menu
	page->menu = GTK_MENU(gtkhash_properties_get_object(builder,
		"menu"));
	page->menuitem_copy = GTK_IMAGE_MENU_ITEM(gtkhash_properties_get_object(builder,
		"imagemenuitem_copy"));
	page->menuitem_show_funcs = GTK_CHECK_MENU_ITEM(gtkhash_properties_get_object(builder,
		"checkmenuitem_show_funcs"));

	// Check/MAC inputs
	page->hbox_inputs = GTK_WIDGET(gtkhash_properties_get_object(builder,
		"hbox_inputs"));
	page->entry_check = GTK_ENTRY(gtkhash_properties_get_object(builder,
		"entry_check"));
	page->togglebutton_hmac = GTK_TOGGLE_BUTTON(gtkhash_properties_get_object(builder,
		"togglebutton_hmac"));
	page->entry_hmac = GTK_ENTRY(gtkhash_properties_get_object(builder,
		"entry_hmac"));

	// Buttons
	page->button_hash = GTK_BUTTON(gtkhash_properties_get_object(builder,
		"button_hash"));
	page->button_stop = GTK_BUTTON(gtkhash_properties_get_object(builder,
		"button_stop"));
}

static void gtkhash_properties_connect_signals(struct page_s *page)
{
	// Main container
	g_signal_connect_swapped(page->box, "destroy",
		G_CALLBACK(gtkhash_properties_free_page), page);

	// Treeview
	g_signal_connect_swapped(page->cellrendtoggle, "toggled",
		G_CALLBACK(gtkhash_properties_on_cell_toggled), page);
	g_signal_connect_swapped(page->treeview, "popup-menu",
		G_CALLBACK(gtkhash_properties_on_treeview_popup_menu), page);
	g_signal_connect_swapped(page->treeview, "button-press-event",
		G_CALLBACK(gtkhash_properties_on_treeview_button_press_event), page);
	g_signal_connect_swapped(page->treeselection, "changed",
		G_CALLBACK(gtkhash_properties_on_treeselection_changed), page);

	// Popup menu
	g_signal_connect_swapped(page->menuitem_copy, "activate",
		G_CALLBACK(gtkhash_properties_on_menuitem_copy_activate), page);
	g_signal_connect_swapped(page->menuitem_show_funcs, "toggled",
		G_CALLBACK(gtkhash_properties_on_menuitem_show_funcs_toggled), page);

	// Check
	g_signal_connect_swapped(page->entry_check, "changed",
		G_CALLBACK(gtkhash_properties_on_entry_check_changed), page);

	// HMAC
	g_signal_connect_swapped(page->togglebutton_hmac, "toggled",
		G_CALLBACK(gtkhash_properties_on_togglebutton_hmac_toggled), page);
	g_signal_connect_swapped(page->entry_hmac, "changed",
		G_CALLBACK(gtkhash_properties_on_entry_hmac_changed), page);

	// Buttons
	g_signal_connect_swapped(page->button_hash, "clicked",
		G_CALLBACK(gtkhash_properties_on_button_hash_clicked), page);
	g_signal_connect_swapped(page->button_stop, "clicked",
		G_CALLBACK(gtkhash_properties_on_button_stop_clicked), page);
}

static char *gtkhash_properties_get_xml(const char *filename)
{
	GMappedFile *map = g_mapped_file_new(filename, false, NULL);

	if (!map)
		return NULL;

	gsize map_len = g_mapped_file_get_length(map);
	if (map_len == 0) {
		g_mapped_file_unref(map);
		return NULL;
	}

	const char *map_data = g_mapped_file_get_contents(map);
	g_assert(map_data);

	GInputStream *input_mem = g_memory_input_stream_new_from_data(map_data,
		map_len, NULL);
	GConverter *converter = G_CONVERTER(g_zlib_decompressor_new(
		G_ZLIB_COMPRESSOR_FORMAT_GZIP));

	GInputStream *input_conv = g_converter_input_stream_new(input_mem,
		converter);

	g_object_unref(input_mem);
	g_object_unref(converter);

	GString *string = g_string_new(NULL);

	for (char buf[1024];;) {
		gssize len = g_input_stream_read(input_conv, buf, 1024, NULL, NULL);
		if (len <= 0)
			break;

		g_string_append_len(string, buf, len);
	}

	g_object_unref(input_conv);
	g_mapped_file_unref(map);

	return g_string_free(string, false);
}

static struct page_s *gtkhash_properties_new_page(char *uri)
{
	char *xml = gtkhash_properties_get_xml(BUILDER_XML);

	if (!xml || !*xml) {
		g_warning("failed to read \"%s\"", BUILDER_XML);
		g_free(xml);
		return NULL;
	}

	GtkBuilder *builder = gtk_builder_new();
	gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);

	GError *error = NULL;
	gtk_builder_add_from_string(builder, xml, -1, &error);

	g_free(xml);

	if (error) {
		g_warning("failed to read \"%s\":\n%s", BUILDER_XML, error->message);
		g_error_free(error);
		g_object_unref(builder);
		return NULL;
	}

	struct page_s *page = g_new(struct page_s, 1);
	page->uri = uri;

	gtkhash_properties_hash_init(page);

	if (gtkhash_properties_hash_funcs_supported(page) == 0) {
		g_warning("no hash functions available");
		gtkhash_properties_hash_deinit(page);
		g_free(page);
		return NULL;
	}

	gtkhash_properties_get_objects(page, builder);
	g_object_ref(page->box);
	g_object_ref(page->menu);
	g_object_unref(builder);

	gtkhash_properties_prefs_init(page);
	gtkhash_properties_list_init(page);
	gtkhash_properties_idle(page);

	gtkhash_properties_connect_signals(page);

	return page;
}

static GList *gtkhash_properties_get_pages(
#if IN_NAUTILUS_EXTENSION
	G_GNUC_UNUSED NautilusPropertyPageProvider *provider,
#elif IN_NEMO_EXTENSION
	G_GNUC_UNUSED NemoPropertyPageProvider *provider,
#elif IN_THUNAR_EXTENSION
	G_GNUC_UNUSED ThunarxPropertyPageProvider *provider,
#endif
	GList *files)
{
	// Only display page for a single file
	if (!files || files->next)
		return NULL;

#if IN_NAUTILUS_EXTENSION
	GFileType type = nautilus_file_info_get_file_type(files->data);

	char *uri = nautilus_file_info_get_uri(files->data);
#elif IN_NEMO_EXTENSION
	GFileType type = nemo_file_info_get_file_type(files->data);

	char *uri = nemo_file_info_get_uri(files->data);
#elif IN_THUNAR_EXTENSION
	GFileInfo *info = thunarx_file_info_get_file_info(files->data);
	GFileType type = g_file_info_get_file_type(info);
	g_object_unref(info);

	char *uri = thunarx_file_info_get_uri(files->data);
#endif

	// Only display page for regular files
	if (type != G_FILE_TYPE_REGULAR)
		return NULL;

	struct page_s *page = gtkhash_properties_new_page(uri);
	if (!page)
		return NULL;

#if IN_NAUTILUS_EXTENSION
	NautilusPropertyPage *ppage = nautilus_property_page_new(
		"GtkHash::properties", gtk_label_new(_("Digests")), page->box);
#elif IN_NEMO_EXTENSION
	NemoPropertyPage *ppage = nemo_property_page_new(
		"GtkHash::properties", gtk_label_new(_("Digests")), page->box);
#elif IN_THUNAR_EXTENSION
	GtkWidget *ppage = thunarx_property_page_new(_("Digests"));
	gtk_container_add(GTK_CONTAINER(ppage), page->box);
#endif

	GList *pages = g_list_append(NULL, ppage);

	return pages;
}

static void gtkhash_properties_iface_init(
#if IN_NAUTILUS_EXTENSION
	NautilusPropertyPageProviderIface *iface
#elif IN_NEMO_EXTENSION
	NemoPropertyPageProviderIface *iface
#elif IN_THUNAR_EXTENSION
	ThunarxPropertyPageProviderIface *iface
#endif
	)
{
	iface->get_pages = gtkhash_properties_get_pages;
}

static void gtkhash_properties_register_type(GTypeModule *module)
{
	const GTypeInfo info = {
		sizeof(GObjectClass),
		(GBaseInitFunc)NULL,
		(GBaseFinalizeFunc)NULL,
		(GClassInitFunc)NULL,
		(GClassFinalizeFunc)NULL,
		NULL,
		sizeof(GObject),
		0,
		(GInstanceInitFunc)NULL,
		(GTypeValueTable *)NULL
	};

	page_type = g_type_module_register_type(module, G_TYPE_OBJECT,
		"GtkHash", &info, 0);

	const GInterfaceInfo iface_info = {
		(GInterfaceInitFunc)gtkhash_properties_iface_init,
		(GInterfaceFinalizeFunc)NULL,
		NULL
	};

	g_type_module_add_interface(module, page_type,
#if IN_NAUTILUS_EXTENSION
		NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER,
#elif IN_NEMO_EXTENSION
		NEMO_TYPE_PROPERTY_PAGE_PROVIDER,
#elif IN_THUNAR_EXTENSION
		THUNARX_TYPE_PROPERTY_PAGE_PROVIDER,
#endif
		&iface_info);
}

#if __GNUC__
	#define PUBLIC __attribute__((visibility("default")))
#else
	#define PUBLIC G_MODULE_EXPORT
#endif

#if IN_NAUTILUS_EXTENSION
PUBLIC void nautilus_module_initialize(GTypeModule *module)
#elif IN_NEMO_EXTENSION
PUBLIC void nemo_module_initialize(GTypeModule *module)
#elif IN_THUNAR_EXTENSION
PUBLIC void thunar_extension_initialize(GTypeModule *module);
PUBLIC void thunar_extension_initialize(GTypeModule *module)
#endif
{
	gtkhash_properties_register_type(module);

#if ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif
}

#if IN_NAUTILUS_EXTENSION
PUBLIC void nautilus_module_shutdown(void)
#elif IN_NEMO_EXTENSION
PUBLIC void nemo_module_shutdown(void)
#elif IN_THUNAR_EXTENSION
PUBLIC void thunar_extension_shutdown(void);
PUBLIC void thunar_extension_shutdown(void)
#endif
{
}

#if IN_NAUTILUS_EXTENSION
PUBLIC void nautilus_module_list_types(const GType **types, int *num_types)
#elif IN_NEMO_EXTENSION
PUBLIC void nemo_module_list_types(const GType **types, int *num_types)
#elif IN_THUNAR_EXTENSION
PUBLIC void thunar_extension_list_types(const GType **types, int *num_types);
PUBLIC void thunar_extension_list_types(const GType **types, int *num_types)
#endif
{
	static GType type_list[1];

	type_list[0] = page_type;
	*types = type_list;
	*num_types = G_N_ELEMENTS(type_list);
}
