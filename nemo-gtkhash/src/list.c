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
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "list.h"
#include "main.h"
#include "hash.h"
#include "gui.h"

#define COL_N (COL_HASH + HASH_FUNCS_N)

enum {
	COL_FILE,
	COL_HASH
};

// Returns the row number with matching uri or -1 if not found
static int list_find_row(const char *uri)
{
	g_assert(uri);

	GtkTreeIter iter;

	GFile *file = g_file_new_for_uri(uri);
	char *pname = g_file_get_parse_name(file);
	g_object_unref(file);

	if (gtk_tree_model_get_iter_first(gui.treemodel, &iter)) {
		int row = 0;
		do {
			GValue value;
			value.g_type = 0;
			gtk_tree_model_get_value(gui.treemodel, &iter, COL_FILE, &value);
			const char *string = g_value_get_string(&value);

			if (strcmp(string, pname) == 0) {
				g_free(pname);
				g_value_unset(&value);
				return row;
			} else {
				g_value_unset(&value);
				row++;
			}
		} while (gtk_tree_model_iter_next(gui.treemodel, &iter));
	}

	g_free(pname);

	return -1;
}

void list_init(void)
{
	GType types[COL_N];

	for (int i = 0; i < HASH_FUNCS_N; i++) {
		gtk_tree_view_insert_column_with_attributes(gui.treeview, -1,
			hash.funcs[i].name, gtk_cell_renderer_text_new(),
			"text", COL_HASH + i, NULL);
	}

	for (int i = 0; i < COL_N; i++) {
		types[i] = G_TYPE_STRING;
		GtkTreeViewColumn *col = gtk_tree_view_get_column(gui.treeview, i);
		gtk_tree_view_column_set_resizable(col, true);
	}

	gui.liststore = gtk_list_store_newv(COL_N, types);
	gui.treemodel = GTK_TREE_MODEL(gui.liststore);

	gtk_tree_view_set_model(gui.treeview, gui.treemodel);

	gtk_tree_selection_set_mode(gui.treeselection, GTK_SELECTION_MULTIPLE);

	const GtkTargetEntry targets[] = {
		{ (char *)"text/uri-list", 0, 0 }
	};
	gtk_drag_dest_set(GTK_WIDGET(gui.treeview), GTK_DEST_DEFAULT_ALL, targets,
		G_N_ELEMENTS(targets), GDK_ACTION_COPY);
}

void list_update(void)
{
	for (int i = 0; i < HASH_FUNCS_N; i++) {
		GtkTreeViewColumn *col = gtk_tree_view_get_column(gui.treeview,
			COL_HASH + i);
		bool active = hash.funcs[i].enabled;
		gtk_tree_view_column_set_visible(col, active);
	}
}

void list_append_row(const char *uri)
{
	g_assert(uri);

	GFile *file = g_file_new_for_uri(uri);
	char *pname = g_file_get_parse_name(file);
	g_object_unref(file);

	if (list_find_row(pname) >= 0)
		return;

	gtk_list_store_insert_with_values(gui.liststore, NULL, G_MAXINT,
		COL_FILE, pname, -1);
	g_free(pname);

	gtk_widget_set_sensitive(GTK_WIDGET(gui.toolbutton_clear), true);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_treeview_clear), true);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.button_hash), true);
	gtk_widget_grab_focus(GTK_WIDGET(gui.button_hash));
}

void list_remove_selection(void)
{
	GList *rows = gtk_tree_selection_get_selected_rows(gui.treeselection,
		&gui.treemodel);

	for (GList *i = rows; i != NULL; i = i->next) {
		GtkTreePath *path = i->data;
		GtkTreeRowReference *ref = gtk_tree_row_reference_new(gui.treemodel,
			i->data);
		i->data = ref;
		gtk_tree_path_free(path);
	}

	for (GList *i = rows; i != NULL; i = i->next) {
		GtkTreeRowReference *ref = i->data;
		GtkTreePath *path = gtk_tree_row_reference_get_path(ref);
		GtkTreeIter iter;

		if (gtk_tree_model_get_iter(gui.treemodel, &iter, path))
			gtk_list_store_remove(gui.liststore, &iter);

		gtk_tree_path_free(path);
		gtk_tree_row_reference_free(ref);
	}

	g_list_free(rows);

	if (list_count_rows() == 0) {
		gtk_widget_set_sensitive(GTK_WIDGET(gui.toolbutton_clear), false);
		gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_treeview_clear), false);
		gtk_widget_set_sensitive(GTK_WIDGET(gui.button_hash), false);
	}
}

char *list_get_uri(const unsigned int row)
{
	g_assert(row <= list_count_rows());

	GtkTreeIter iter;
	GValue value;
	value.g_type = 0;

	if (!gtk_tree_model_get_iter_first(gui.treemodel, &iter))
		return NULL;

	for (unsigned int i = 0; i < row; i++)
		gtk_tree_model_iter_next(gui.treemodel, &iter);

	gtk_tree_model_get_value(gui.treemodel, &iter, COL_FILE, &value);
	GFile *file = g_file_parse_name(g_value_get_string(&value));
	g_value_unset(&value);

	char *uri = g_file_get_uri(file);
	g_object_unref(file);

	return uri;
}

GSList *list_get_all_uris(void)
{
	GSList *uris = NULL;

	for (unsigned int i = 0; i < list_count_rows(); i++) {
		char *uri = list_get_uri(i);
		uris = g_slist_prepend(uris, uri);
	}

	return g_slist_reverse(uris);;
}

unsigned int list_count_rows(void)
{
	GtkTreeIter iter;
	unsigned int count = 0;

	if (gtk_tree_model_get_iter_first(gui.treemodel, &iter)) {
		count++;
		while(gtk_tree_model_iter_next(gui.treemodel, &iter))
			count++;
	}

	return count;
}

void list_set_digest(const char *uri, const enum hash_func_e id,
	const char *digest)
{
	g_assert(uri);
	g_assert(HASH_FUNC_IS_VALID(id));

	GtkTreeIter iter;
	int row = list_find_row(uri);
	g_assert (row >= 0);

	if (!gtk_tree_model_get_iter_first(gui.treemodel, &iter))
		return;

	for (int i = 0; i < row; i++)
		gtk_tree_model_iter_next(gui.treemodel, &iter);

	gtk_list_store_set(gui.liststore, &iter, COL_HASH + id, digest, -1);
}

char *list_get_digest(const unsigned int row, const enum hash_func_e id)
{
	g_assert(row <= list_count_rows());
	g_assert(HASH_FUNC_IS_VALID(id));

	GtkTreeIter iter;
	char *digest;
	GValue value;
	value.g_type = 0;

	if (!gtk_tree_model_get_iter_first(gui.treemodel, &iter))
		return NULL;

	for (unsigned int i = 0; i < row; i++)
		gtk_tree_model_iter_next(gui.treemodel, &iter);

	gtk_tree_model_get_value(gui.treemodel, &iter, COL_HASH + id, &value);
	digest = g_strdup(g_value_get_string(&value));
	g_value_unset(&value);

	return digest;
}

char *list_get_selected_digest(const enum hash_func_e id)
{
	GList *rows = gtk_tree_selection_get_selected_rows(gui.treeselection,
		&gui.treemodel);

	// Should only have one row selected
	g_assert(!rows->next);

	GtkTreePath *path = rows->data;
	unsigned int row = *gtk_tree_path_get_indices(path);

	gtk_tree_path_free(path);
	g_list_free(rows);

	return list_get_digest(row, id);
}

void list_clear_digests(void)
{
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter_first(gui.treemodel, &iter))
		return;

	do {
		for (int i = 0; i < HASH_FUNCS_N; i++)
			gtk_list_store_set(gui.liststore, &iter, COL_HASH + i, "", -1);
	} while (gtk_tree_model_iter_next(gui.treemodel, &iter));
}

void list_clear(void)
{
	gtk_list_store_clear(gui.liststore);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.toolbutton_clear), false);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.menuitem_treeview_clear), false);
	gtk_widget_set_sensitive(GTK_WIDGET(gui.button_hash), false);
}
