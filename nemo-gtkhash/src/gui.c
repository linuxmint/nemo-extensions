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
#include <string.h>
#include <errno.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "gui.h"
#include "main.h"
#include "callbacks.h"
#include "hash.h"
#include "list.h"
#include "prefs.h"
#include "hash/digest-format.h"

struct gui_s gui;

static struct {
	enum gui_state_e state;
} gui_priv = {
	.state = GUI_STATE_INVALID,
};

static GObject *gui_get_object(GtkBuilder *builder, const char *name)
{
	g_assert(name);

	GObject *obj = gtk_builder_get_object(builder, name);
	if (!obj)
		g_error("unknown object: \"%s\"", name);

	return obj;
}

static void gui_get_objects(GtkBuilder *builder)
{
	// Window
	gui.window = GTK_WINDOW(gui_get_object(builder,
		"window"));

	// Menus
	gui.menuitem_save_as = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_save_as"));
	gui.menuitem_quit = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_quit"));
	gui.menuitem_edit = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_edit"));
	gui.menuitem_cut = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_cut"));
	gui.menuitem_copy = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_copy"));
	gui.menuitem_paste = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_paste"));
	gui.menuitem_delete = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_delete"));
	gui.menuitem_select_all = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_select_all"));
	gui.menuitem_prefs = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_prefs"));
	gui.menuitem_about = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_about"));
	gui.radiomenuitem_file = GTK_RADIO_MENU_ITEM(gui_get_object(builder,
		"radiomenuitem_file"));
	gui.radiomenuitem_text = GTK_RADIO_MENU_ITEM(gui_get_object(builder,
		"radiomenuitem_text"));
	gui.radiomenuitem_file_list = GTK_RADIO_MENU_ITEM(gui_get_object(builder,
		"radiomenuitem_file_list"));

	// Toolbar
	gui.toolbar = GTK_TOOLBAR(gui_get_object(builder,
		"toolbar"));
	gui.toolbutton_add = GTK_TOOL_BUTTON(gui_get_object(builder,
		"toolbutton_add"));
	gui.toolbutton_remove = GTK_TOOL_BUTTON(gui_get_object(builder,
		"toolbutton_remove"));
	gui.toolbutton_clear = GTK_TOOL_BUTTON(gui_get_object(builder,
		"toolbutton_clear"));

	// Containers
	gui.vbox_single = GTK_VBOX(gui_get_object(builder,
		"vbox_single"));
	gui.vbox_list = GTK_VBOX(gui_get_object(builder,
		"vbox_list"));
	gui.hbox_input = GTK_HBOX(gui_get_object(builder,
		"hbox_input"));
	gui.hbox_output = GTK_HBOX(gui_get_object(builder,
		"hbox_output"));
	gui.vbox_outputlabels = GTK_VBOX(gui_get_object(builder,
		"vbox_outputlabels"));
	gui.vbox_digests_file = GTK_VBOX(gui_get_object(builder,
		"vbox_digests_file"));
	gui.vbox_digests_text = GTK_VBOX(gui_get_object(builder,
		"vbox_digests_text"));

	// Inputs
	gui.filechooserbutton = GTK_FILE_CHOOSER_BUTTON(gui_get_object(builder,
		"filechooserbutton"));
	gui.entry_text = GTK_ENTRY(gui_get_object(builder,
		"entry_text"));
	gui.entry_check_file = GTK_ENTRY(gui_get_object(builder,
		"entry_check_file"));
	gui.entry_check_text = GTK_ENTRY(gui_get_object(builder,
		"entry_check_text"));
	gui.togglebutton_hmac_file = GTK_TOGGLE_BUTTON(gui_get_object(builder,
		"togglebutton_hmac_file"));
	gui.togglebutton_hmac_text = GTK_TOGGLE_BUTTON(gui_get_object(builder,
		"togglebutton_hmac_text"));
	gui.entry_hmac_file = GTK_ENTRY(gui_get_object(builder,
		"entry_hmac_file"));
	gui.entry_hmac_text = GTK_ENTRY(gui_get_object(builder,
		"entry_hmac_text"));

	// Labels
	gui.label_file = GTK_LABEL(gui_get_object(builder,
		"label_file"));
	gui.label_text = GTK_LABEL(gui_get_object(builder,
		"label_text"));

	// Tree View
	gui.treeview = GTK_TREE_VIEW(gui_get_object(builder,
		"treeview"));
	gui.treeselection = GTK_TREE_SELECTION(gui_get_object(builder,
		"treeselection"));
	gui.menu_treeview = GTK_MENU(gui_get_object(builder,
		"menu_treeview"));
	gui.menuitem_treeview_add = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_treeview_add"));
	gui.menuitem_treeview_remove = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_treeview_remove"));
	gui.menuitem_treeview_clear = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_treeview_clear"));
	gui.menu_treeview_copy = GTK_MENU(gui_get_object(builder,
		"menu_treeview_copy"));
	gui.menuitem_treeview_copy = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_treeview_copy"));
	gui.menuitem_treeview_show_toolbar = GTK_MENU_ITEM(gui_get_object(builder,
		"menuitem_treeview_show_toolbar"));

	// Buttons
	gui.hseparator_buttons = GTK_HSEPARATOR(gui_get_object(builder,
		"hseparator_buttons"));
	gui.button_hash = GTK_BUTTON(gui_get_object(builder,
		"button_hash"));
	gui.button_stop = GTK_BUTTON(gui_get_object(builder,
		"button_stop"));

	// Progress Bar
	gui.progressbar = GTK_PROGRESS_BAR(gui_get_object(builder,
		"progressbar"));

	// Dialog
	gui.dialog = GTK_DIALOG(gui_get_object(builder,
		"dialog"));
	gui.dialog_table = GTK_TABLE(gui_get_object(builder,
		"dialog_table"));
	gui.dialog_combobox = GTK_COMBO_BOX(gui_get_object(builder,
		"dialog_combobox"));
	gui.dialog_button_close = GTK_BUTTON(gui_get_object(builder,
		"dialog_button_close"));
}

static void gui_init_hash_funcs(void)
{
	int supported = 0;

	for (int i = 0; i < HASH_FUNCS_N; i++) {
		// File/Text view func labels
		char *label = g_strdup_printf("%s:", hash.funcs[i].name);
		gui.hash_widgets[i].label = GTK_LABEL(gtk_label_new(label));
		g_free(label);
		gtk_container_add(GTK_CONTAINER(gui.vbox_outputlabels),
			GTK_WIDGET(gui.hash_widgets[i].label));
		gtk_misc_set_alignment(GTK_MISC(gui.hash_widgets[i].label),
			// Left align
			0.0, 0.5);

		// File view digests
		gui.hash_widgets[i].entry_file = GTK_ENTRY(gtk_entry_new());
		gtk_container_add(GTK_CONTAINER(gui.vbox_digests_file),
			GTK_WIDGET(gui.hash_widgets[i].entry_file));
		gtk_editable_set_editable(GTK_EDITABLE(gui.hash_widgets[i].entry_file),
			false);
		gtk_entry_set_icon_activatable(gui.hash_widgets[i].entry_file,
			GTK_ENTRY_ICON_SECONDARY, false);

		// Text view digests
		gui.hash_widgets[i].entry_text = GTK_ENTRY(gtk_entry_new());
		gtk_container_add(GTK_CONTAINER(gui.vbox_digests_text),
			GTK_WIDGET(gui.hash_widgets[i].entry_text));
		gtk_editable_set_editable(GTK_EDITABLE(gui.hash_widgets[i].entry_text),
			false);
		gtk_entry_set_icon_activatable(gui.hash_widgets[i].entry_text,
			GTK_ENTRY_ICON_SECONDARY, false);

		// File list treeview popup menu
		gui.hash_widgets[i].menuitem_treeview_copy =
			GTK_MENU_ITEM(gtk_menu_item_new_with_label(hash.funcs[i].name));
		gtk_menu_shell_append(GTK_MENU_SHELL(gui.menu_treeview_copy),
			GTK_WIDGET(gui.hash_widgets[i].menuitem_treeview_copy));

		// Dialog checkbuttons
		gui.hash_widgets[i].button = GTK_TOGGLE_BUTTON(
			gtk_check_button_new_with_label(hash.funcs[i].name));
		if (hash.funcs[i].supported) {
			gtk_table_attach_defaults(gui.dialog_table,
				GTK_WIDGET(gui.hash_widgets[i].button),
				// Sort checkbuttons into 2 columns
				supported % 2 ? 1 : 0,
				supported % 2 ? 2 : 1,
				supported / 2,
				supported / 2 + 1);
			gtk_widget_show(GTK_WIDGET(gui.hash_widgets[i].button));

			supported++;
		}
	}
}

static char *gui_get_xml(const char *filename)
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

void gui_init(const char *datadir)
{
	gtk_init(NULL, NULL);

	char *filename = g_build_filename(datadir, PACKAGE ".xml.gz", NULL);
	char *xml = gui_get_xml(filename);

	if (!xml || !*xml) {
		char *message = g_strdup_printf(_("Failed to read \"%s\""), filename);
		gui_error(message);
		g_free(message);
		g_free(xml);
		g_free(filename);
		exit(EXIT_FAILURE);
	}

	GtkBuilder *builder = gtk_builder_new();
	gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);

	GError *error = NULL;
	gtk_builder_add_from_string(builder, xml, -1, &error);

	g_free(xml);

	if (error) {
		char *message = g_strdup_printf(_("Failed to read \"%s\":\n%s"),
			filename, error->message);
		gui_error(message);
		g_free(message);
		g_free(filename);
		g_error_free(error);
		g_object_unref(builder);
		exit(EXIT_FAILURE);
	}

	g_free(filename);

	gui_get_objects(builder);
	g_object_ref(gui.menu_treeview);
	g_object_unref(builder);

#if (GTK_MAJOR_VERSION > 2)
	// Avoid GTK+ 2 "Unknown property" warning
	gtk_widget_set_valign(GTK_WIDGET(gui.vbox_single), GTK_ALIGN_START);
#endif

	gui_init_hash_funcs();

	gui_set_state(GUI_STATE_IDLE);

	callbacks_init();
}

static bool gui_can_add_uri(char *uri, char **error_str)
{
	g_assert(uri);

	bool can_add = false;
	GFile *file = g_file_new_for_uri(uri);
	GError *error = NULL;
	GFileInfo *info = g_file_query_info(file,
		G_FILE_ATTRIBUTE_STANDARD_TYPE "," G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
		G_FILE_QUERY_INFO_NONE, NULL, &error);

	if (info) {
		bool can_read = g_file_info_get_attribute_boolean(info,
			G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
		GFileType type = g_file_info_get_file_type(info);
		g_object_unref(info);
		if (!can_read)
			*error_str = g_strdup(g_strerror(EACCES));
		else if (type == G_FILE_TYPE_DIRECTORY)
			*error_str = g_strdup(g_strerror(EISDIR));
		else if (type != G_FILE_TYPE_REGULAR)
			*error_str = g_strdup(_("Not a regular file"));
		else
			can_add = true;
	} else {
		*error_str = g_strdup(error->message);
		g_error_free(error);
	}

	g_object_unref(file);

	return can_add;
}

unsigned int gui_add_uris(GSList *uris, enum gui_view_e view)
{
	g_assert(uris);

	GSList *readable = NULL;
	unsigned int readable_len = 0;
	{
		GSList *tmp = uris;
		do {
			char *error = g_strdup(_("Unknown error"));
			if (!gui_can_add_uri(tmp->data, &error)) {
				char *message = g_strdup_printf(_("Failed to add \"%s\":\n%s"),
					(char *)tmp->data, error);
				gui_error(message);
				g_free(error);
				g_free(message);
				continue;
			}
			if (!g_slist_find_custom(readable, tmp->data,
				(GCompareFunc)g_strcmp0))
			{
				readable = g_slist_prepend(readable, tmp->data);
				readable_len++;
			}
		} while ((tmp = tmp->next));
	}
	readable = g_slist_reverse(readable);

	if (view == GUI_VIEW_INVALID) {
		if (readable_len == 1)
			view = GUI_VIEW_FILE;
		else if (readable_len > 1)
			view = GUI_VIEW_FILE_LIST;
	}

	if (readable_len && (view == GUI_VIEW_FILE)) {
		gtk_file_chooser_set_uri(GTK_FILE_CHOOSER(gui.filechooserbutton),
			readable->data);
	} else if (readable_len && (view == GUI_VIEW_FILE_LIST)) {
		GSList *tmp = readable;
		do {
			list_append_row(tmp->data);
		} while ((tmp = tmp->next));
	}

	g_slist_free(readable);

	return readable_len;
}

void gui_add_check(const char *check)
{
	g_assert(check && *check);

	gtk_entry_set_text(gui.entry_check_file, check);
	gtk_entry_set_text(gui.entry_check_text, check);
}

void gui_add_text(const char *text)
{
	g_assert(text && *text);

	gtk_entry_set_text(gui.entry_text, text);
}

void gui_error(const char *message)
{
	GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
		GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), PACKAGE_NAME);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void gui_run(void)
{
	// Show window here so it isn't resized just after it's opened
	gtk_widget_show(GTK_WIDGET(gui.window));

	gtk_main();
}

void gui_deinit(void)
{
	hash_file_stop();

	while (gui_get_state() != GUI_STATE_IDLE)
		gtk_main_iteration();

	gtk_widget_destroy(GTK_WIDGET(gui.window));
	g_object_unref(gui.menu_treeview);

	while (gtk_events_pending())
		gtk_main_iteration();
}

void gui_set_view(const enum gui_view_e view)
{
	switch (view) {
		case GUI_VIEW_FILE:
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
				gui.radiomenuitem_file), true);
			break;
		case GUI_VIEW_TEXT:
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
				gui.radiomenuitem_text), true);
			break;
		case GUI_VIEW_FILE_LIST:
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
				gui.radiomenuitem_file_list), true);
			break;
		default:
			g_assert_not_reached();
	}
}

enum gui_view_e gui_get_view(void)
{
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(
		gui.radiomenuitem_file)))
	{
		return GUI_VIEW_FILE;
	}

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(
		gui.radiomenuitem_text)))
	{
		return GUI_VIEW_TEXT;
	}

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(
		gui.radiomenuitem_file_list)))
	{
		return GUI_VIEW_FILE_LIST;
	}

	g_assert_not_reached();
	return GUI_VIEW_INVALID;
}

void gui_set_digest_format(const enum digest_format_e format)
{
	g_assert(DIGEST_FORMAT_IS_VALID(format));

	gtk_combo_box_set_active(gui.dialog_combobox, format);
}

enum digest_format_e gui_get_digest_format(void)
{
	enum digest_format_e format = gtk_combo_box_get_active(gui.dialog_combobox);
	g_assert(DIGEST_FORMAT_IS_VALID(format));

	return format;
}

const uint8_t *gui_get_hmac_key(size_t *key_size)
{
	g_assert(key_size);

	const uint8_t *hmac_key = NULL;
	*key_size = 0;

	switch (gui_get_view()) {
		case GUI_VIEW_FILE:
			if (gtk_toggle_button_get_active(gui.togglebutton_hmac_file)) {
				hmac_key = (uint8_t *)gtk_entry_get_text(gui.entry_hmac_file);
				*key_size = gtk_entry_get_text_length(gui.entry_hmac_file);
			}
			break;
		case GUI_VIEW_TEXT:
			if (gtk_toggle_button_get_active(gui.togglebutton_hmac_text)) {
				hmac_key = (uint8_t *)gtk_entry_get_text(gui.entry_hmac_text);
				*key_size = gtk_entry_get_text_length(gui.entry_hmac_text);
			}
			break;
		case GUI_VIEW_FILE_LIST:
			// TODO
			break;
		default:
			g_assert_not_reached();
	}

	return hmac_key;
}

static void gui_menuitem_save_as_set_sensitive(void)
{
	if (gui_get_state() == GUI_STATE_BUSY) {
		gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_save_as), false);
		return;
	}

	bool sensitive = false;

	switch (gui_get_view()) {
		case GUI_VIEW_FILE:
			for (int i = 0; i < HASH_FUNCS_N; i++) {
				if (hash.funcs[i].enabled &&
					*gtk_entry_get_text(gui.hash_widgets[i].entry_file))
				{
					sensitive = true;
					break;
				}
			}
			break;
		case GUI_VIEW_TEXT:
			sensitive = true;
			break;
		case GUI_VIEW_FILE_LIST:
			for (int i = 0; i < HASH_FUNCS_N; i++) {
				if (hash.funcs[i].enabled) {
					char *digest = list_get_digest(0, i);
					if (digest != NULL && *digest) {
						g_free(digest);
						sensitive = true;
						break;
					}
				}
			}
			break;
		default:
			g_assert_not_reached();
	}

	gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_save_as), sensitive);
}

void gui_update(void)
{
	bool has_enabled = false;

	for (int i = 0; i < HASH_FUNCS_N; i++) {
		if (hash.funcs[i].supported) {
			hash.funcs[i].enabled = gtk_toggle_button_get_active(
				gui.hash_widgets[i].button);
			if (hash.funcs[i].enabled)
				has_enabled = true;
		} else
			hash.funcs[i].enabled = false;

		gtk_widget_set_visible(GTK_WIDGET(gui.hash_widgets[i].label),
			hash.funcs[i].enabled);
		gtk_widget_set_visible(GTK_WIDGET(gui.hash_widgets[i].entry_file),
			hash.funcs[i].enabled);
		gtk_widget_set_visible(GTK_WIDGET(gui.hash_widgets[i].entry_text),
			hash.funcs[i].enabled);
		gtk_widget_set_visible(GTK_WIDGET(gui.hash_widgets[i].menuitem_treeview_copy),
			hash.funcs[i].enabled);
	}

	list_update();

	enum gui_view_e view = gui_get_view();

	if ((view == GUI_VIEW_FILE) || (view == GUI_VIEW_TEXT)) {
		gtk_widget_hide(GTK_WIDGET(gui.toolbar));
		gtk_widget_hide(GTK_WIDGET(gui.vbox_list));
		gtk_widget_show(GTK_WIDGET(gui.vbox_single));
	}

	switch (view) {
		case GUI_VIEW_FILE: {
			gtk_widget_hide(GTK_WIDGET(gui.label_text));
			gtk_widget_hide(GTK_WIDGET(gui.entry_text));
			gtk_widget_hide(GTK_WIDGET(gui.entry_check_text));
			gtk_widget_hide(GTK_WIDGET(gui.entry_hmac_text));
			gtk_widget_hide(GTK_WIDGET(gui.togglebutton_hmac_text));
			gtk_widget_hide(GTK_WIDGET(gui.vbox_digests_text));
			gtk_widget_show(GTK_WIDGET(gui.label_file));
			gtk_widget_show(GTK_WIDGET(gui.filechooserbutton));
			gtk_widget_show(GTK_WIDGET(gui.entry_check_file));
			gtk_widget_show(GTK_WIDGET(gui.entry_hmac_file));
			gtk_widget_show(GTK_WIDGET(gui.togglebutton_hmac_file));
			gtk_widget_show(GTK_WIDGET(gui.vbox_digests_file));
			gtk_widget_show(GTK_WIDGET(gui.hseparator_buttons));
			gtk_widget_show(GTK_WIDGET(gui.button_hash));

			gtk_widget_set_sensitive(GTK_WIDGET(gui.entry_hmac_file),
				gtk_toggle_button_get_active(gui.togglebutton_hmac_file));

			char *uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(
				gui.filechooserbutton));
			if (uri) {
				gtk_widget_set_sensitive(GTK_WIDGET(gui.button_hash),
					has_enabled);
				g_free(uri);
			} else
				gtk_widget_set_sensitive(GTK_WIDGET(gui.button_hash), false);
			break;
		}
		case GUI_VIEW_TEXT:
			gtk_widget_hide(GTK_WIDGET(gui.label_file));
			gtk_widget_hide(GTK_WIDGET(gui.filechooserbutton));
			gtk_widget_hide(GTK_WIDGET(gui.entry_check_file));
			gtk_widget_hide(GTK_WIDGET(gui.entry_hmac_file));
			gtk_widget_hide(GTK_WIDGET(gui.togglebutton_hmac_file));
			gtk_widget_hide(GTK_WIDGET(gui.vbox_digests_file));
			gtk_widget_hide(GTK_WIDGET(gui.hseparator_buttons));
			gtk_widget_hide(GTK_WIDGET(gui.button_hash));
			gtk_widget_show(GTK_WIDGET(gui.label_text));
			gtk_widget_show(GTK_WIDGET(gui.entry_text));
			gtk_widget_show(GTK_WIDGET(gui.entry_check_text));
			gtk_widget_show(GTK_WIDGET(gui.entry_hmac_text));
			gtk_widget_show(GTK_WIDGET(gui.togglebutton_hmac_text));
			gtk_widget_show(GTK_WIDGET(gui.vbox_digests_text));

			gtk_widget_set_sensitive(GTK_WIDGET(gui.entry_hmac_text),
				gtk_toggle_button_get_active(gui.togglebutton_hmac_text));

			gtk_widget_grab_focus(GTK_WIDGET(gui.entry_text));

			gtk_button_clicked(gui.button_hash);
			break;
		case GUI_VIEW_FILE_LIST:
			gtk_widget_hide(GTK_WIDGET(gui.vbox_single));
			gtk_widget_hide(GTK_WIDGET(gui.hseparator_buttons));
			gtk_widget_show(GTK_WIDGET(gui.vbox_list));
			gtk_widget_show(GTK_WIDGET(gui.button_hash));

			gtk_widget_set_visible(GTK_WIDGET(gui.toolbar),
				gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(
					gui.menuitem_treeview_show_toolbar)));
			gtk_widget_set_sensitive(GTK_WIDGET(gui.button_hash),
				has_enabled && (list_count_rows() > 0));
			break;
		default:
			g_assert_not_reached();
	}

	gui_check_digests();
	gui_menuitem_save_as_set_sensitive();
}

void gui_clear_digests(void)
{
	switch (gui_get_view()) {
		case GUI_VIEW_FILE:
			for (int i = 0; i < HASH_FUNCS_N; i++)
				gtk_entry_set_text(gui.hash_widgets[i].entry_file, "");
			gui_check_digests();
			break;
		case GUI_VIEW_TEXT:
			for (int i = 0; i < HASH_FUNCS_N; i++)
				gtk_entry_set_text(gui.hash_widgets[i].entry_text, "");
			gui_check_digests();
			break;
		case GUI_VIEW_FILE_LIST:
			list_clear_digests();
			break;
		default:
			g_assert_not_reached();
	}

	gui_menuitem_save_as_set_sensitive();
}

void gui_clear_all_digests(void)
{
	for (int i = 0; i < HASH_FUNCS_N; i++)
		gtk_entry_set_text(gui.hash_widgets[i].entry_file, "");

	for (int i = 0; i < HASH_FUNCS_N; i++)
		gtk_entry_set_text(gui.hash_widgets[i].entry_text, "");

	list_clear_digests();

	gui_check_digests();
}

void gui_check_digests(void)
{
	const char *str_in_file = gtk_entry_get_text(gui.entry_check_file);
	const char *str_in_text = gtk_entry_get_text(gui.entry_check_text);

	const char *icon_in_file = NULL;
	const char *icon_in_text = NULL;

	for (int i = 0; i < HASH_FUNCS_N; i++) {
		if (!hash.funcs[i].enabled)
			continue;

		const char *str_out_file = gtk_entry_get_text(
			gui.hash_widgets[i].entry_file);
		const char *str_out_text = gtk_entry_get_text(
			gui.hash_widgets[i].entry_text);

		const char *icon_out_file = NULL;
		const char *icon_out_text = NULL;

		switch (gui_get_digest_format()) {
			case DIGEST_FORMAT_HEX_LOWER:
			case DIGEST_FORMAT_HEX_UPPER:
				if (*str_in_file && (strcasecmp(str_in_file, str_out_file) == 0))
					icon_out_file = GTK_STOCK_YES;
				if (*str_in_text && (strcasecmp(str_in_text, str_out_text) == 0))
					icon_out_text = GTK_STOCK_YES;
				break;
			case DIGEST_FORMAT_BASE64:
				if (*str_in_file && (strcmp(str_in_file, str_out_file) == 0))
					icon_out_file = GTK_STOCK_YES;
				if (*str_in_text && (strcmp(str_in_text, str_out_text) == 0))
					icon_out_text = GTK_STOCK_YES;
				break;
			default:
				g_assert_not_reached();
		}

		gtk_entry_set_icon_from_stock(gui.hash_widgets[i].entry_file,
			GTK_ENTRY_ICON_SECONDARY, icon_out_file);
		gtk_entry_set_icon_from_stock(gui.hash_widgets[i].entry_text,
			GTK_ENTRY_ICON_SECONDARY, icon_out_text);

		if (icon_out_file)
			icon_in_file = GTK_STOCK_YES;
		if (icon_out_text)
			icon_in_text = GTK_STOCK_YES;
	}

	gtk_entry_set_icon_from_stock(gui.entry_check_file,
		GTK_ENTRY_ICON_SECONDARY, icon_in_file);
	gtk_entry_set_icon_from_stock(gui.entry_check_text,
		GTK_ENTRY_ICON_SECONDARY, icon_in_text);
}

void gui_set_state(const enum gui_state_e state)
{
	g_assert(GUI_STATE_IS_VALID(state));

	gui_priv.state = state;

	if (gui_get_view() == GUI_VIEW_TEXT)
		return;

	const bool busy = (state == GUI_STATE_BUSY);

	gtk_widget_set_visible(GTK_WIDGET(gui.button_hash), !busy);
	gtk_widget_set_visible(GTK_WIDGET(gui.button_stop), busy);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.button_stop), busy);

	gtk_progress_bar_set_fraction(gui.progressbar, 0.0);
	gtk_progress_bar_set_text(gui.progressbar, " ");
	gtk_widget_set_visible(GTK_WIDGET(gui.progressbar), busy);

	gtk_widget_set_sensitive(GTK_WIDGET(gui.hbox_input), !busy);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.hbox_output), !busy);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.toolbar), !busy);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.treeview), !busy);

	gtk_widget_set_sensitive(GTK_WIDGET(gui.radiomenuitem_text), !busy);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.radiomenuitem_file), !busy);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.radiomenuitem_file_list), !busy);

	gtk_widget_set_sensitive(GTK_WIDGET(gui.dialog_table), !busy);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.dialog_combobox), !busy);

	if (busy) {
		gtk_window_set_default(gui.window, GTK_WIDGET(gui.button_stop));
		gui_menuitem_save_as_set_sensitive();
	} else {
		gtk_window_set_default(gui.window, GTK_WIDGET(gui.button_hash));
		gui_menuitem_save_as_set_sensitive();
	}
}

enum gui_state_e gui_get_state(void)
{
	g_assert(GUI_STATE_IS_VALID(gui_priv.state));

	return gui_priv.state;
}

bool gui_is_maximised(void)
{
	GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(gui.window));

	if (!window)
		return false;

	GdkWindowState state = gdk_window_get_state(window);

	return (state & GDK_WINDOW_STATE_MAXIMIZED);
}
