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

#ifndef GTKHASH_GUI_H
#define GTKHASH_GUI_H

#include <stdbool.h>
#include <gtk/gtk.h>

#ifndef GTK_HBOX // Gtk+ 3
	#define GtkHBox GtkBox
	#define GTK_HBOX GTK_BOX
	#define GtkVBox GtkBox
	#define GTK_VBOX GTK_BOX
	#define GtkHSeparator GtkSeparator
	#define GTK_HSEPARATOR GTK_SEPARATOR
#endif

#include "hash.h"

#define GUI_VIEW_IS_VALID(X) (((X) >= 0) && ((X) <= GUI_VIEW_FILE_LIST))
#define GUI_STATE_IS_VALID(X) (((X) >= 0) && ((X) <= GUI_STATE_BUSY))

enum gui_view_e {
	GUI_VIEW_INVALID = -1,
	GUI_VIEW_FILE,
	GUI_VIEW_TEXT,
	GUI_VIEW_FILE_LIST,
};

enum gui_state_e {
	GUI_STATE_INVALID = -1,
	GUI_STATE_IDLE,
	GUI_STATE_BUSY,
};

extern struct gui_s {
	GtkWindow *window;
	GtkMenuItem *menuitem_save_as, *menuitem_quit;
	GtkMenuItem *menuitem_edit;
	GtkMenuItem *menuitem_cut, *menuitem_copy, *menuitem_paste;
	GtkMenuItem *menuitem_delete, *menuitem_select_all, *menuitem_prefs;
	GtkMenuItem *menuitem_about;
	GtkRadioMenuItem *radiomenuitem_file, *radiomenuitem_text, *radiomenuitem_file_list;
	GtkToolbar *toolbar;
	GtkToolButton *toolbutton_add, *toolbutton_remove, *toolbutton_clear;
	GtkVBox *vbox_single, *vbox_list;
	GtkHBox *hbox_input, *hbox_output;
	GtkVBox *vbox_outputlabels, *vbox_digests_file, *vbox_digests_text;
	GtkEntry *entry_text, *entry_check_file, *entry_check_text;
	GtkEntry *entry_hmac_file, *entry_hmac_text;
	GtkFileChooserButton *filechooserbutton;
	GtkToggleButton *togglebutton_hmac_file, *togglebutton_hmac_text;
	GtkLabel *label_file, *label_text;
	GtkTreeView *treeview;
	GtkTreeSelection *treeselection;
	GtkTreeModel *treemodel;
	GtkListStore *liststore;
	GtkMenu *menu_treeview;
	GtkMenuItem *menuitem_treeview_add, *menuitem_treeview_remove;
	GtkMenuItem *menuitem_treeview_clear;
	GtkMenu *menu_treeview_copy;
	GtkMenuItem *menuitem_treeview_copy;
	GtkMenuItem *menuitem_treeview_show_toolbar;
	GtkHSeparator *hseparator_buttons;
	GtkProgressBar *progressbar;
	GtkButton *button_hash, *button_stop;
	GtkDialog *dialog;
	GtkTable *dialog_table;
	GtkComboBox *dialog_combobox;
	GtkButton *dialog_button_close;
	struct {
		GtkToggleButton *button;
		GtkLabel *label;
		GtkEntry *entry_file, *entry_text;
		GtkMenuItem *menuitem_treeview_copy;
	} hash_widgets[HASH_FUNCS_N];
} gui;

void gui_init(const char *datadir);
unsigned int gui_add_uris(GSList *uris, enum gui_view_e view);
void gui_add_check(const char *check);
void gui_add_text(const char *text);
void gui_error(const char *message);
void gui_run(void);
void gui_deinit(void);
void gui_set_view(const enum gui_view_e view);
enum gui_view_e gui_get_view(void);
void gui_set_digest_format(const enum digest_format_e format);
enum digest_format_e gui_get_digest_format(void);
const uint8_t *gui_get_hmac_key(size_t *key_size);
void gui_update(void);
void gui_clear_digests(void);
void gui_clear_all_digests(void);
void gui_check_digests(void);
void gui_set_state(const enum gui_state_e state);
enum gui_state_e gui_get_state(void);
bool gui_is_maximised(void);

#endif
