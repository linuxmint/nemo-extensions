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
#include <gtk/gtk.h>

#include "callbacks.h"
#include "main.h"
#include "gui.h"
#include "hash.h"
#include "prefs.h"
#include "list.h"
#include "hash/hash-string.h"

static bool on_window_delete_event(void)
{
	gtk_widget_hide(GTK_WIDGET(gui.window));
	gtk_main_quit();

	return true;
}

static void on_menuitem_save_as_activate(void)
{
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(
		gtk_file_chooser_dialog_new(_("Save Digests"), gui.window,
			GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
			NULL));
	gtk_file_chooser_set_do_overwrite_confirmation(chooser, true);


	if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(chooser);
		GString *string = g_string_sized_new(1024);

		for (int i = 0; i < HASH_FUNCS_N; i++) {
			if (!hash.funcs[i].enabled)
				continue;

			switch (gui_get_view()) {
				case GUI_VIEW_FILE: {
					const bool hmac_active = gtk_toggle_button_get_active(
						gui.togglebutton_hmac_file);
					const char *digest = gtk_entry_get_text(
						gui.hash_widgets[i].entry_file);
					if (digest && *digest) {
						g_string_append_printf(string,
							(hmac_active && (hash.funcs[i].block_size > 0)) ?
							"# HMAC-%s\n" : "# %s\n", hash.funcs[i].name);
					} else
						continue;
					char *path = gtk_file_chooser_get_filename(
						GTK_FILE_CHOOSER(gui.filechooserbutton));
					char *basename = g_path_get_basename(path);
					g_free(path);
					g_string_append_printf(string, "%s  %s\n",
					gtk_entry_get_text(gui.hash_widgets[i].entry_file),
						basename);
					g_free(basename);
					break;
				}
				case GUI_VIEW_TEXT: {
					const bool hmac_active = gtk_toggle_button_get_active(
						gui.togglebutton_hmac_text);
					g_string_append_printf(string,
						(hmac_active && (hash.funcs[i].block_size > 0)) ?
						"# HMAC-%s\n" : "# %s\n", hash.funcs[i].name);
					g_string_append_printf(string, "%s  \"%s\"\n",
						gtk_entry_get_text(gui.hash_widgets[i].entry_text),
						gtk_entry_get_text(gui.entry_text));
					break;
				}
				case GUI_VIEW_FILE_LIST: {
					int prev = -1;
					for (unsigned int row = 0; row < list_count_rows(); row++)
					{
						char *digest = list_get_digest(row, i);
						if (digest && *digest) {
							if (i != prev)
								g_string_append_printf(string, "# %s\n",
									hash.funcs[i].name);
							prev = i;
						} else {
							if (digest)
								g_free(digest);
							prev = i;
							continue;
						}
						char *uri = list_get_uri(row);
						char *basename = g_filename_display_basename(uri);
						g_string_append_printf(string, "%s  %s\n",
							digest, basename);
						g_free(basename);
						g_free(uri);
						g_free(digest);
					}
					break;
				}
				default:
					g_assert_not_reached();
			}
		}

		char *data = g_string_free(string, false);
		g_file_set_contents(filename, data, -1, NULL);

		g_free(data);
		g_free(filename);
	}

	gtk_widget_destroy(GTK_WIDGET(chooser));
}

static void on_menuitem_quit_activate(void)
{
	gtk_widget_hide(GTK_WIDGET(gui.window));
	gtk_main_quit();
}

static void on_menuitem_edit_activate(void)
{
	GtkWidget *widget = gtk_window_get_focus(gui.window);
	bool selectable = false;
	bool editable = false;
	bool selection_ready = false;
	bool clipboard_ready = false;

	if (GTK_IS_ENTRY(widget)) {
		selectable = gtk_entry_get_text_length(GTK_ENTRY(widget));
		editable = gtk_editable_get_editable(GTK_EDITABLE(widget));
		selection_ready = gtk_editable_get_selection_bounds(
			GTK_EDITABLE(widget), NULL, NULL);
		clipboard_ready = gtk_clipboard_wait_is_text_available(
			gtk_clipboard_get(GDK_NONE));
	}

	gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_cut),
		selection_ready && editable);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_copy),
		selection_ready);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_paste),
		editable && clipboard_ready);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_delete),
		selection_ready && editable);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_select_all),
		selectable);
}

static void on_menuitem_cut_activate(void)
{
	GtkEditable *widget = GTK_EDITABLE(gtk_window_get_focus(gui.window));

	gtk_editable_cut_clipboard(widget);
}

static void on_menuitem_copy_activate(void)
{
	GtkEditable *widget = GTK_EDITABLE(gtk_window_get_focus(gui.window));

	gtk_editable_copy_clipboard(widget);
}

static void on_menuitem_paste_activate(void)
{
	GtkEditable *widget = GTK_EDITABLE(gtk_window_get_focus(gui.window));

	gtk_editable_paste_clipboard(widget);
}

static void on_menuitem_delete_activate(void)
{
	GtkEditable *widget = GTK_EDITABLE(gtk_window_get_focus(gui.window));

	gtk_editable_delete_selection(widget);
}

static void on_menuitem_select_all_activate(void)
{
	GtkEditable *widget = GTK_EDITABLE(gtk_window_get_focus(gui.window));

	gtk_editable_set_position(widget, -1);
	gtk_editable_select_region(widget, 0, -1);
}

static void on_menuitem_prefs_activate(void)
{
	gtk_widget_show(GTK_WIDGET(gui.dialog));
}

static void on_menuitem_about_activate(void)
{
	static const char * const license = {
		"This program is free software: you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation, either version 2 of the License, or\n"
		"(at your option) any later version.\n\n"
		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
		"GNU General Public License for more details.\n\n"
		"You should have received a copy of the GNU General Public License along\n"
		"with this program; if not, see <http://www.gnu.org/licenses/>.\n"
	};

	static const char * const authors[] = {
		"Tristan Heaven <tristanheaven@gmail.com>",
		NULL
	};

	gtk_show_about_dialog(
			gui.window,
			"authors", authors,
			"comments", _("A GTK+ utility for computing message digests or checksums."),
			"license", license,
			"program-name", PACKAGE_NAME,
#if ENABLE_NLS
			"translator-credits", _("translator-credits"),
#endif
			"version", VERSION,
			"website", "http://gtkhash.sourceforge.net/",
			NULL);
}

static void on_filechooserbutton_selection_changed(void)
{
	char *uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(gui.filechooserbutton));

	gtk_widget_set_sensitive(GTK_WIDGET(gui.button_hash), uri ? true : false);

	if (uri)
		gtk_widget_grab_focus(GTK_WIDGET(gui.button_hash));

	gui_clear_digests();
}

static void on_toolbutton_add_clicked(void)
{
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(
		gtk_file_chooser_dialog_new(_("Select Files"), gui.window,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL));
	gtk_file_chooser_set_select_multiple(chooser, true);
	gtk_file_chooser_set_local_only(chooser, false);

	if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
		GSList *uris = gtk_file_chooser_get_uris(chooser);
		gui_add_uris(uris, GUI_VIEW_FILE_LIST);
		g_slist_free_full(uris, g_free);
	}

	gtk_widget_destroy(GTK_WIDGET(chooser));
}

static void on_treeview_popup_menu(void)
{
	gtk_menu_popup(gui.menu_treeview, NULL, NULL, NULL, NULL, 0,
		gtk_get_current_event_time());
}

static bool on_treeview_button_press_event(G_GNUC_UNUSED GtkWidget *widget,
	GdkEventButton *event)
{
	// Right click
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		gtk_menu_popup(gui.menu_treeview, NULL, NULL, NULL, NULL, 3,
			gdk_event_get_time((GdkEvent *)event));
	}

	return false;
}

static void on_treeview_drag_data_received(G_GNUC_UNUSED GtkWidget *widget,
	GdkDragContext *context, G_GNUC_UNUSED gint x, G_GNUC_UNUSED gint y,
	GtkSelectionData *selection, G_GNUC_UNUSED guint info, guint t,
	G_GNUC_UNUSED gpointer data)
{
	char **uris = gtk_selection_data_get_uris(selection);
	GSList *list = NULL;

	if (!uris) {
		gtk_drag_finish(context, false, true, t);
		return;
	}

	for (int i = 0; uris[i]; i++)
		list = g_slist_prepend(list, uris[i]);

	list = g_slist_reverse(list);

	gui_add_uris(list, GUI_VIEW_FILE_LIST);

	g_slist_free(list);
	g_strfreev(uris);

	gtk_drag_finish(context, true, true, t);
}

static void on_treeselection_changed(void)
{
	const int rows = gtk_tree_selection_count_selected_rows(gui.treeselection);

	gtk_widget_set_sensitive(GTK_WIDGET(gui.toolbutton_remove), (rows > 0));

	gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_treeview_remove),
		(rows > 0));
	gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_treeview_copy),
		(rows == 1));

	if (rows == 1) {
		for (int i = 0; i < HASH_FUNCS_N; i++) {
			if (!hash.funcs[i].enabled)
				continue;
			char *digest = list_get_selected_digest(i);
			gtk_widget_set_sensitive(GTK_WIDGET(
				gui.hash_widgets[i].menuitem_treeview_copy), (digest && *digest));
			g_free(digest);
		}
	}
}

static void on_menuitem_treeview_copy_activate(struct hash_func_s *func)
{
	char *digest = list_get_selected_digest(func->id);
	g_assert(digest);

	gtk_clipboard_set_text(gtk_clipboard_get(GDK_NONE), digest, -1);

	g_free(digest);
}

static void on_menuitem_treeview_show_toolbar_toggled(void)
{
	const bool show_toolbar = gtk_check_menu_item_get_active(
		GTK_CHECK_MENU_ITEM(gui.menuitem_treeview_show_toolbar));

	gtk_widget_set_visible(GTK_WIDGET(gui.toolbar), show_toolbar);
}

static void on_button_hash_clicked(void)
{
	if (gui_get_view() == GUI_VIEW_FILE) {
		// XXX: Workaround for when user clicks Cancel in FileChooserDialog and
		// XXX: uri is changed without emitting the "selection-changed" signal
		on_filechooserbutton_selection_changed();
		if (!gtk_widget_get_sensitive(GTK_WIDGET(gui.button_hash)))
			return;
	}

	switch (gui_get_view()) {
		case GUI_VIEW_FILE: {
			gui_clear_digests();
			gui_set_state(GUI_STATE_BUSY);
			char *uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(
				gui.filechooserbutton));
			hash_file_start(uri);
			break;
		}
		case GUI_VIEW_TEXT:
			hash_string();
			break;
		case GUI_VIEW_FILE_LIST:
			gui_clear_digests();
			gui_set_state(GUI_STATE_BUSY);
			hash_file_list_start();
			break;
		default:
			g_assert_not_reached();
	}
}

static void on_button_stop_clicked(void)
{
	gtk_widget_set_sensitive(GTK_WIDGET(gui.button_stop), false);
	hash_file_stop();
}

static void on_entry_hmac_changed(void)
{
	switch (gui_get_view()) {
		case GUI_VIEW_FILE:
			gui_clear_digests();
			break;
		case GUI_VIEW_TEXT:
			on_button_hash_clicked();
			break;
		default:
			g_assert_not_reached();
	}
}

static void on_togglebutton_hmac_toggled(void)
{
	switch (gui_get_view()) {
		case GUI_VIEW_FILE:
			gtk_widget_set_sensitive(GTK_WIDGET(gui.entry_hmac_file),
				gtk_toggle_button_get_active(gui.togglebutton_hmac_file));
			break;
		case GUI_VIEW_TEXT:
			gtk_widget_set_sensitive(GTK_WIDGET(gui.entry_hmac_text),
				gtk_toggle_button_get_active(gui.togglebutton_hmac_text));
			break;
		default:
			g_assert_not_reached();
	}

	on_entry_hmac_changed();
}

static bool on_dialog_delete_event(void)
{
	gtk_widget_hide(GTK_WIDGET(gui.dialog));
	return true;
}

static void on_dialog_combobox_changed(void)
{
	gui_clear_all_digests();

	if (gui_get_view() == GUI_VIEW_TEXT)
		on_button_hash_clicked();
}

void callbacks_init(void)
{
#define CON(OBJ, SIG, CB) g_signal_connect(G_OBJECT(OBJ), SIG, CB, NULL)
	CON(gui.window,                         "delete-event",        G_CALLBACK(on_window_delete_event));
	CON(gui.menuitem_save_as,               "activate",            on_menuitem_save_as_activate);
	CON(gui.menuitem_quit,                  "activate",            on_menuitem_quit_activate);
	CON(gui.menuitem_edit,                  "activate",            on_menuitem_edit_activate);
	CON(gui.menuitem_cut,                   "activate",            on_menuitem_cut_activate);
	CON(gui.menuitem_copy,                  "activate",            on_menuitem_copy_activate);
	CON(gui.menuitem_paste,                 "activate",            on_menuitem_paste_activate);
	CON(gui.menuitem_delete,                "activate",            on_menuitem_delete_activate);
	CON(gui.menuitem_select_all,            "activate",            on_menuitem_select_all_activate);
	CON(gui.menuitem_prefs,                 "activate",            on_menuitem_prefs_activate);
	CON(gui.radiomenuitem_file,             "toggled",             gui_update);
	CON(gui.radiomenuitem_text,             "toggled",             gui_update);
	CON(gui.radiomenuitem_file_list,        "toggled",             gui_update);
	CON(gui.menuitem_about,                 "activate",            on_menuitem_about_activate);
//	file-set isn't emitted when file is deleted
//	CON(gui.filechooserbutton,              "file-set",            on_filechooserbutton_file_set);
	CON(gui.filechooserbutton,              "selection-changed",   on_filechooserbutton_selection_changed);
	CON(gui.entry_text,                     "changed",             on_button_hash_clicked);
	CON(gui.togglebutton_hmac_file,         "toggled",             on_togglebutton_hmac_toggled);
	CON(gui.togglebutton_hmac_text,         "toggled",             on_togglebutton_hmac_toggled);
	CON(gui.entry_hmac_file,                "changed",             on_entry_hmac_changed);
	CON(gui.entry_hmac_text,                "changed",             on_entry_hmac_changed);
	CON(gui.entry_check_file,               "changed",             gui_check_digests);
	CON(gui.entry_check_text,               "changed",             gui_check_digests);
	CON(gui.toolbutton_add,                 "clicked",             on_toolbutton_add_clicked);
	CON(gui.toolbutton_remove,              "clicked",             list_remove_selection);
	CON(gui.toolbutton_clear,               "clicked",             list_clear);
	CON(gui.treeview,                       "popup-menu",          on_treeview_popup_menu);
	CON(gui.treeview,                       "button-press-event",  G_CALLBACK(on_treeview_button_press_event));
	CON(gui.treeview,                       "drag-data-received",  G_CALLBACK(on_treeview_drag_data_received));
	CON(gui.treeselection,                  "changed",             on_treeselection_changed);
	CON(gui.menuitem_treeview_add,          "activate",            on_toolbutton_add_clicked);
	CON(gui.menuitem_treeview_remove,       "activate",            list_remove_selection);
	CON(gui.menuitem_treeview_clear,        "activate",            list_clear);
	CON(gui.menuitem_treeview_show_toolbar, "toggled",             on_menuitem_treeview_show_toolbar_toggled);
	CON(gui.button_hash,                    "clicked",             on_button_hash_clicked);
	CON(gui.button_stop,                    "clicked",             on_button_stop_clicked);
	CON(gui.dialog,                         "delete-event",        G_CALLBACK(on_dialog_delete_event));
	CON(gui.dialog_button_close,            "clicked",             G_CALLBACK(on_dialog_delete_event));
	CON(gui.dialog_combobox,                "changed",             on_dialog_combobox_changed);

	for (int i = 0; i < HASH_FUNCS_N; i++) {
		CON(gui.hash_widgets[i].button, "toggled", gui_update);
		g_signal_connect_swapped(gui.hash_widgets[i].menuitem_treeview_copy,
			"activate", G_CALLBACK(on_menuitem_treeview_copy_activate),
			&hash.funcs[i]);
	}

#undef CON
}
