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

#include "properties.h"
#include "properties-prefs.h"
#include "properties-hash.h"
#include "../hash/hash-func.h"

#define PREFS_SCHEMA "org.nemo.extensions.gtkhash"
#define PREFS_KEY_HASH_FUNCS "hash-functions"
#define PREFS_KEY_SHOW_DISABLED_FUNCS "show-disabled-hash-functions"
#define PREFS_BIND_FLAGS \
	(G_SETTINGS_BIND_DEFAULT | \
	 G_SETTINGS_BIND_GET_NO_CHANGES)

static void gtkhash_properties_prefs_load_hash_funcs(struct page_s *page)
{
	char **strv = g_settings_get_strv(page->settings, PREFS_KEY_HASH_FUNCS);

	for (int i = 0; strv[i]; i++) {
		enum hash_func_e id = gtkhash_hash_func_get_id_from_name(strv[i]);
		if (id != HASH_FUNC_INVALID && page->funcs[id].supported)
			page->funcs[id].enabled = true;
	}

	g_strfreev(strv);
}

static void gtkhash_properties_prefs_load_show_disabled_funcs(struct page_s *page)
{
	g_settings_bind(page->settings, PREFS_KEY_SHOW_DISABLED_FUNCS,
		page->menuitem_show_funcs, "active", PREFS_BIND_FLAGS);
}

static void gtkhash_properties_prefs_load(struct page_s *page)
{
	g_assert(page->settings);

	gtkhash_properties_prefs_load_hash_funcs(page);
	gtkhash_properties_prefs_load_show_disabled_funcs(page);
}

static void gtkhash_properties_prefs_save_hash_funcs(struct page_s *page)
{
	const char **strv = NULL;
	int enabled = 0;

	for (int i = 0; i < HASH_FUNCS_N; i++) {
		if (page->funcs[i].enabled)
			enabled++;
	}

	if (enabled > 0) {
		strv = g_new0(const char *, enabled + 1);
		for (int i = 0, j = 0; (i < HASH_FUNCS_N) && (j < enabled); i++) {
			if (page->funcs[i].enabled) {
				strv[j] = page->funcs[i].name;
				j++;
			}
		}
	}

	g_settings_set_strv(page->settings, PREFS_KEY_HASH_FUNCS, strv);

	if (strv)
		g_free(strv);
}

static void gtkhash_properties_prefs_save(struct page_s *page)
{
	g_assert(page->settings);

	gtkhash_properties_prefs_save_hash_funcs(page);
}

void gtkhash_properties_prefs_init(struct page_s *page)
{
	page->settings = NULL;

	bool found = false;
	const char * const *schemas = g_settings_list_schemas();

	for (int i = 0; schemas[i]; i++) {
		if (g_strcmp0(schemas[i], PREFS_SCHEMA) == 0) {
			found = true;
			break;
		}
	}

	if (found) {
		page->settings = g_settings_new(PREFS_SCHEMA);
		gtkhash_properties_prefs_load(page);
	} else
		g_warning("GSettings schema \"" PREFS_SCHEMA "\" not found");
}

void gtkhash_properties_prefs_deinit(struct page_s *page)
{
	if (!page->settings)
		return;

	gtkhash_properties_prefs_save(page);

	g_object_unref(page->settings);
	page->settings = NULL;
}
