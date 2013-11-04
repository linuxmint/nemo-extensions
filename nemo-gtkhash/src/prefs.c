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

#include "prefs.h"
#include "main.h"
#include "hash.h"
#include "gui.h"
#include "hash/digest-format.h"
#include "hash/hash-func.h"

#define PREFS_SCHEMA "apps.gtkhash"
#define PREFS_KEY_DIGEST_FORMAT "digest-format"
#define PREFS_KEY_HASH_FUNCS "hash-functions"
#define PREFS_KEY_SHOW_TOOLBAR "show-toolbar"
#define PREFS_KEY_VIEW "view"
#define PREFS_KEY_WINDOW_HEIGHT "window-height"
#define PREFS_KEY_WINDOW_MAX "window-max"
#define PREFS_KEY_WINDOW_WIDTH "window-width"
#define PREFS_BIND_FLAGS \
	(G_SETTINGS_BIND_DEFAULT | \
	 G_SETTINGS_BIND_GET_NO_CHANGES)

static struct {
	GSettings *settings;
} prefs_priv = {
	.settings = NULL,
};

static void default_hash_funcs(void)
{
	bool has_enabled = false;

	// Try to enable default functions
	for (int i = 0; i < HASH_FUNCS_N; i++) {
		if (HASH_FUNC_IS_DEFAULT(i) && hash.funcs[i].supported) {
			gtk_toggle_button_set_active(gui.hash_widgets[i].button, true);
			has_enabled = true;
		}
	}

	if (has_enabled)
		return;

	// Try to enable any supported function
	for (int i = 0; i < HASH_FUNCS_N; i++) {
		if (hash.funcs[i].supported) {
			gtk_toggle_button_set_active(gui.hash_widgets[i].button, true);
			return;
		}
	}

	gui_error(_("Failed to enable any supported hash functions."));
	exit(EXIT_FAILURE);
}

static void default_show_toolbar(void)
{
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
		gui.menuitem_treeview_show_toolbar), true);
}

static void prefs_default(void)
{
	default_hash_funcs();
	default_show_toolbar();

	gui_update();
}

static void load_hash_funcs(void)
{
	char **strv = g_settings_get_strv(prefs_priv.settings, PREFS_KEY_HASH_FUNCS);
	bool has_enabled = false;

	for (int i = 0; strv[i]; i++) {
		enum hash_func_e id = gtkhash_hash_func_get_id_from_name(strv[i]);
		if (id != HASH_FUNC_INVALID && hash.funcs[id].supported) {
			hash.funcs[id].enabled = true;
			gtk_toggle_button_set_active(gui.hash_widgets[id].button, true);
			has_enabled = true;
		}
	}

	g_strfreev(strv);

	if (!has_enabled)
		default_hash_funcs();
}

static void load_digest_format(void)
{
	char *str = g_settings_get_string(prefs_priv.settings,
		PREFS_KEY_DIGEST_FORMAT);
	if (!str)
		return;

	enum digest_format_e format = DIGEST_FORMAT_INVALID;

	if (g_strcmp0(str, "hex-lower") == 0)
		format = DIGEST_FORMAT_HEX_LOWER;
	else if (g_strcmp0(str, "hex-upper") == 0)
		format = DIGEST_FORMAT_HEX_UPPER;
	else if (g_strcmp0(str, "base64") == 0)
		format = DIGEST_FORMAT_BASE64;

	g_free(str);

	if (DIGEST_FORMAT_IS_VALID(format))
		gui_set_digest_format(format);
}

static void load_view(void)
{
	char *str = g_settings_get_string(prefs_priv.settings, PREFS_KEY_VIEW);
	if (!str)
		return;

	enum gui_view_e view = GUI_VIEW_INVALID;

	if (g_strcmp0(str, "file") == 0)
		view = GUI_VIEW_FILE;
	else if (g_strcmp0(str, "text") == 0)
		view = GUI_VIEW_TEXT;
	else if (g_strcmp0(str, "file-list") == 0)
		view = GUI_VIEW_FILE_LIST;

	g_free(str);

	if (GUI_VIEW_IS_VALID(view))
		gui_set_view(view);
}

static void load_show_toolbar(void)
{
	g_settings_bind(prefs_priv.settings, PREFS_KEY_SHOW_TOOLBAR,
		gui.menuitem_treeview_show_toolbar, "active", PREFS_BIND_FLAGS);
}

static void load_window_size(void)
{
	if (g_settings_get_boolean(prefs_priv.settings, PREFS_KEY_WINDOW_MAX)) {
		gtk_window_maximize(gui.window);
		return;
	}

	int width = g_settings_get_int(prefs_priv.settings, PREFS_KEY_WINDOW_WIDTH);
	int height = g_settings_get_int(prefs_priv.settings, PREFS_KEY_WINDOW_HEIGHT);

	if ((width > 0) && (height > 0))
		gtk_window_resize(gui.window, width, height);
}

static void prefs_load(void)
{
	g_assert(prefs_priv.settings);

	load_hash_funcs();
	load_digest_format();
	load_view();
	load_show_toolbar();
	load_window_size();

	gui_update();
}

static void save_hash_funcs(void)
{
	const char **strv = NULL;
	int enabled = 0;

	for (int i = 0; i < HASH_FUNCS_N; i++) {
		if (hash.funcs[i].enabled) {
			enabled++;
		}
	}

	if (enabled > 0) {
		strv = g_new0(const char *, enabled + 1);
		for (int i = 0, j = 0; (i < HASH_FUNCS_N) && (j < enabled); i++) {
			if (hash.funcs[i].enabled) {
				strv[j] = hash.funcs[i].name;
				j++;
			}
		}
	}

	g_settings_set_strv(prefs_priv.settings, PREFS_KEY_HASH_FUNCS, strv);

	if (strv)
		g_free(strv);
}

static void save_digest_format(void)
{
	const char *str = NULL;

	switch (gui_get_digest_format()) {
		case DIGEST_FORMAT_HEX_LOWER:
			str = "hex-lower";
			break;
		case DIGEST_FORMAT_HEX_UPPER:
			str = "hex-upper";
			break;
		case DIGEST_FORMAT_BASE64:
			str = "base64";
			break;
		default:
			g_assert_not_reached();
	}

	g_settings_set_string(prefs_priv.settings, PREFS_KEY_DIGEST_FORMAT, str);
}

static void save_view(void)
{
	const char *str = NULL;

	switch (gui_get_view()) {
		case GUI_VIEW_FILE:
			str = "file";
			break;
		case GUI_VIEW_TEXT:
			str = "text";
			break;
		case GUI_VIEW_FILE_LIST:
			str = "file-list";
			break;
		default:
			g_assert_not_reached();
	}

	g_settings_set_string(prefs_priv.settings, PREFS_KEY_VIEW, str);
}

static void save_window_size(void)
{
	bool max = gui_is_maximised();
	g_settings_set_boolean(prefs_priv.settings, PREFS_KEY_WINDOW_MAX, max);
	if (max)
		return;

	int width, height;
	gtk_window_get_size(gui.window, &width, &height);

	g_settings_set_int(prefs_priv.settings, PREFS_KEY_WINDOW_HEIGHT, height);
	g_settings_set_int(prefs_priv.settings, PREFS_KEY_WINDOW_WIDTH, width);
}

static void prefs_save(void)
{
	g_assert(prefs_priv.settings);

	save_hash_funcs();
	save_digest_format();
	save_view();
	save_window_size();
}

void prefs_init(void)
{
	g_assert(!prefs_priv.settings);

	bool found = false;
	const char * const *schemas = g_settings_list_schemas();

	for (int i = 0; schemas[i]; i++) {
		if (g_strcmp0(schemas[i], PREFS_SCHEMA) == 0) {
			found = true;
			break;
		}
	}

	if (found) {
		prefs_priv.settings = g_settings_new(PREFS_SCHEMA);
		prefs_load();
	} else {
		g_warning("GSettings schema \"" PREFS_SCHEMA
			"\" not found - using default preferences");
		prefs_default();
	}
}

void prefs_deinit(void)
{
	if (!prefs_priv.settings)
		return;

	prefs_save();

	g_object_unref(prefs_priv.settings);
	prefs_priv.settings = NULL;
}
