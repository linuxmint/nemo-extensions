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
#include <gtk/gtk.h>

#include "properties.h"
#include "properties-list.h"
#include "properties-hash.h"
#include "../hash/hash-func.h"

enum {
	COL_ID,
	COL_ENABLED,
	COL_HASH_FUNC,
	COL_DIGEST
};

static GtkTreeModelFilter *gtkhash_properties_list_get_filter(
	struct page_s *page)
{
	return GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(page->treeview));
}

static GtkTreeModel *gtkhash_properties_list_get_model(struct page_s *page)
{
	GtkTreeModelFilter *filter = gtkhash_properties_list_get_filter(page);
	return gtk_tree_model_filter_get_model(filter);
}

static GtkListStore *gtkhash_properties_list_get_store(struct page_s *page)
{
	return GTK_LIST_STORE(gtkhash_properties_list_get_model(page));
}

void gtkhash_properties_list_update_enabled(struct page_s *page, char *path_str)
{
	GtkTreeModel *model = gtkhash_properties_list_get_model(page);
	GtkListStore *store = gtkhash_properties_list_get_store(page);
	GtkTreeIter iter;

	gtk_tree_model_get_iter_from_string(model, &iter, path_str);

	int id;
	gboolean enabled;
	gtk_tree_model_get(model, &iter, COL_ID, &id, COL_ENABLED, &enabled, -1);
	enabled = !enabled;
	gtk_list_store_set(store, &iter, COL_ENABLED, enabled, -1);

	if (!enabled) {
		// Clear digest for disabled func
		gtkhash_hash_func_clear_digest(&page->hash_file.funcs[id]);
		const char *digest = gtkhash_hash_func_get_digest(
			&page->hash_file.funcs[id], DIGEST_FORMAT_HEX_LOWER);
		gtk_list_store_set(store, &iter, COL_DIGEST, digest, -1);
	}

	page->hash_file.funcs[id].enabled = enabled;
}

void gtkhash_properties_list_update_digests(struct page_s *page)
{
	GtkTreeModel *model = gtkhash_properties_list_get_model(page);
	GtkListStore *store = gtkhash_properties_list_get_store(page);
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter_first(model, &iter))
		return;

	do {
		int id;
		gtk_tree_model_get(model, &iter, COL_ID, &id, -1);

		const char *digest = gtkhash_hash_func_get_digest(
			&page->hash_file.funcs[id], DIGEST_FORMAT_HEX_LOWER);
		gtk_list_store_set(store, &iter, COL_DIGEST, digest, -1);
	} while (gtk_tree_model_iter_next(model, &iter));

	gtk_tree_view_columns_autosize(page->treeview);
}

void gtkhash_properties_list_check_digests(struct page_s *page)
{
	const char *str_in = gtk_entry_get_text(page->entry_check);
	const char *icon = NULL;

	if (*str_in) {
		for (int i = 0; i < HASH_FUNCS_N; i++) {
			if (!page->hash_file.funcs[i].enabled)
				continue;

			const char *str_out = gtkhash_hash_func_get_digest(
				&page->hash_file.funcs[i], DIGEST_FORMAT_HEX_LOWER);
			if (strcasecmp(str_in, str_out) == 0) {
				icon = GTK_STOCK_YES;
				break;
			}
		}
	}

	gtk_entry_set_icon_from_stock(page->entry_check, GTK_ENTRY_ICON_SECONDARY,
		icon);
}

char *gtkhash_properties_list_get_selected_digest(struct page_s *page)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	if (!gtk_tree_selection_get_selected(page->treeselection, &model, &iter))
		return NULL;

	char *digest = NULL;;
	gtk_tree_model_get(model, &iter, COL_DIGEST, &digest, -1);

	if (digest && *digest)
		return digest;
	else {
		g_free(digest);
		return NULL;
	}
}

void gtkhash_properties_list_refilter(struct page_s *page)
{
	GtkTreeModelFilter *filter = gtkhash_properties_list_get_filter(page);
	gtk_tree_model_filter_refilter(filter);

	bool active = gtk_check_menu_item_get_active(page->menuitem_show_funcs);
	GtkTreeViewColumn *col = gtk_tree_view_get_column(page->treeview, 0);
	gtk_tree_view_column_set_visible(col, active);

	gtk_tree_view_columns_autosize(page->treeview);
}

static bool gtkhash_properties_list_filter(GtkTreeModel *model,
	GtkTreeIter *iter, struct page_s *page)
{
	gboolean enabled;
	gtk_tree_model_get(model, iter, COL_ENABLED, &enabled, -1);

	if (!enabled && !gtk_check_menu_item_get_active(page->menuitem_show_funcs))
		return false;

	return true;
}

void gtkhash_properties_list_init(struct page_s *page)
{
	GtkListStore *store = gtkhash_properties_list_get_store(page);

	for (int i = 0; i < HASH_FUNCS_N; i++) {
		if (!page->hash_file.funcs[i].supported)
			continue;
		const char *digest = gtkhash_hash_func_get_digest(
			&page->hash_file.funcs[i], DIGEST_FORMAT_HEX_LOWER);
		gtk_list_store_insert_with_values(store, NULL, i,
			COL_ID, i,
			COL_ENABLED, page->hash_file.funcs[i].enabled,
			COL_HASH_FUNC, page->hash_file.funcs[i].name,
			COL_DIGEST, digest,
			-1);
	}

	GtkTreeModelFilter *filter = gtkhash_properties_list_get_filter(page);
	gtk_tree_model_filter_set_visible_func(filter,
		(GtkTreeModelFilterVisibleFunc)gtkhash_properties_list_filter, page,
		NULL);

	gtkhash_properties_list_refilter(page);
}
