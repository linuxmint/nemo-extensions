/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 * pidgin.c
 *       pidgin plugin for nemo-sendto
 *
 * Copyright (C) 2004 Roberto Majadas
 * Copyright (C) 2009 Pascal Terjan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more av.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 *
 * Author:  Roberto Majadas <roberto.majadas@openshine.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include "nemo-sendto-plugin.h"

#define OBJ_PATH "/im/pidgin/purple/PurpleObject"
#define INTERFACE "im.pidgin.purple.PurpleInterface"
#define SERVICE "im.pidgin.purple.PurpleService"

static DBusGProxy *proxy = NULL;
static GHashTable *contact_hash = NULL;

typedef struct _ContactData {
	int  account;
	int  id;
	char *name;
	char *alias;
} ContactData;

enum {
	COL_ICON,
	COL_ALIAS,
	NUM_COLS
};

/*
 * Print appropriate warnings when dbus raised error
 * on queries
 */
static void
handle_dbus_exception(GError *error)
{
	if (error == NULL) {
		g_warning("[Pidgin] unable to parse result");
		return;
	}
	else if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION) {
		g_warning ("[Pidgin] caught remote method exception %s: %s",
			   dbus_g_error_get_name (error),
			   error->message);
	}
	g_error_free (error);
}

static gboolean
init (NstPlugin *plugin)
{
	DBusGConnection *connection;
	GError *error;
	GArray *accounts;

	g_print ("Init pidgin plugin\n");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	error = NULL;
	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if(error != NULL) {
		g_warning("[Pidgin] unable to get session bus, error was:\n %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	proxy = dbus_g_proxy_new_for_name(connection,
					  SERVICE,
					  OBJ_PATH,
					  INTERFACE);
	dbus_g_connection_unref(connection);
	if (proxy == NULL)
		return FALSE;

	error = NULL;
	if (!dbus_g_proxy_call (proxy, "PurpleAccountsGetAllActive", &error, G_TYPE_INVALID,
				DBUS_TYPE_G_INT_ARRAY, &accounts, G_TYPE_INVALID)) {
		g_object_unref(proxy);
		g_error_free(error);
		return FALSE;		
	}
	g_array_free(accounts, TRUE);

	return TRUE;
}

static GdkPixbuf *
get_buddy_icon(int id)
{
	GError *error;
	GdkPixbuf *pixbuf = NULL;
	gchar *path = NULL;
	int icon;

	error=NULL;
	if (!dbus_g_proxy_call (proxy, "PurpleBuddyGetIcon", &error,
				G_TYPE_INT, id,
				G_TYPE_INVALID,
				G_TYPE_INT, &icon, G_TYPE_INVALID)) {
		handle_dbus_exception(error);
	}
	if (icon) {
		if (!dbus_g_proxy_call (proxy, "PurpleBuddyIconGetFullPath", &error,
					G_TYPE_INT, icon,
					G_TYPE_INVALID,
					G_TYPE_STRING, &path, G_TYPE_INVALID)) {
			handle_dbus_exception(error);
		}
		//FIXME Get the size from somewhere
		pixbuf = gdk_pixbuf_new_from_file_at_scale(path, 24, 24, TRUE, NULL);
	}

	return pixbuf;
}

static void
add_pidgin_contacts_to_model (GtkTreeStore *store,
			      GtkTreeIter *iter,
			      GtkTreeIter *parent)
{
	GError *error;
	GArray *contacts_list;
	GArray *accounts;
	int i, j;

	GdkPixbuf *icon;
	GHashTableIter hiter;
	GPtrArray *contacts_group;
	ContactData *dat;
	GValue val = {0,};

	if(proxy == NULL)
		return;

	error = NULL;
	if (!dbus_g_proxy_call (proxy, "PurpleAccountsGetAllActive", &error, G_TYPE_INVALID,
				DBUS_TYPE_G_INT_ARRAY,
				&accounts, G_TYPE_INVALID)) {
		handle_dbus_exception(error);
		return;
	}

	contact_hash = g_hash_table_new (g_str_hash, g_str_equal);

	for(i = 0; i < accounts->len; i++) {
		int account = g_array_index(accounts, int, i);
		error = NULL;
		if (!dbus_g_proxy_call (proxy, "PurpleFindBuddies", &error,
					G_TYPE_INT, account,
					G_TYPE_STRING, NULL,
					G_TYPE_INVALID,
					DBUS_TYPE_G_INT_ARRAY, &contacts_list, G_TYPE_INVALID))	{
			handle_dbus_exception(error);
			continue;
		}
		for(j = 0; j < contacts_list->len ; j++) {
			int id = g_array_index(contacts_list, int, j);
			int online;

			error = NULL;
			if (!dbus_g_proxy_call (proxy, "PurpleBuddyIsOnline", &error,
						G_TYPE_INT, id,
						G_TYPE_INVALID,
						G_TYPE_INT, &online, G_TYPE_INVALID)) {
				handle_dbus_exception(error);
				continue;
			}
			if (!online)
				continue;

			dat = g_new0 (ContactData, 1);

			dat->account = account;
			dat->id = id;

			error = NULL;
			if (!dbus_g_proxy_call (proxy, "PurpleBuddyGetName", &error,
						G_TYPE_INT, id,
						G_TYPE_INVALID,
						G_TYPE_STRING, &dat->name, G_TYPE_INVALID)) {
				handle_dbus_exception(error);
				g_free(dat);
				continue;
			}
			if (!dbus_g_proxy_call (proxy, "PurpleBuddyGetAlias", &error,
						G_TYPE_INT, id,
						G_TYPE_INVALID,
						G_TYPE_STRING, &dat->alias, G_TYPE_INVALID)) {
				handle_dbus_exception(error);
			}

			contacts_group = g_hash_table_lookup (contact_hash, dat->alias);
			if (contacts_group == NULL){
				GPtrArray *new_group = g_ptr_array_new ();
				g_ptr_array_add (new_group, dat);
				g_hash_table_insert (contact_hash, dat->alias, new_group);
			} else {
				g_ptr_array_add (contacts_group, dat);
			}
		}
		g_array_free(contacts_list, TRUE);
	}
	g_array_free (accounts, TRUE);

	g_hash_table_iter_init (&hiter, contact_hash);
	while (g_hash_table_iter_next (&hiter, NULL, (gpointer)&contacts_group)) {
		gint accounts;

		dat = g_ptr_array_index (contacts_group, 0);

		accounts = contacts_group->len;

		gtk_tree_store_append (store, parent, NULL);
		gtk_tree_store_set (store, parent, COL_ICON, NULL, COL_ALIAS, dat->alias, -1);

		gint i;
		for (i = 0; i < accounts; ++i) {
			dat = g_ptr_array_index (contacts_group, i);

			icon = get_buddy_icon(dat->id);

			if (accounts == 1) {
				g_value_init(&val, GDK_TYPE_PIXBUF);
				g_value_set_object (&val, (gpointer)icon);
				gtk_tree_store_set_value (store, parent, COL_ICON, &val);
				g_value_unset (&val);
				break;
			}
			gtk_tree_store_append (store, iter, parent);
			gtk_tree_store_set (store, iter,
					    COL_ICON, icon,
					    COL_ALIAS, dat->alias,
					    -1);
		}
	}
}

static void
customize (GtkCellLayout *cell_layout,
	   GtkCellRenderer *cell,
	   GtkTreeModel *tree_model,
	   GtkTreeIter *iter,
	   gpointer text)
{
	gboolean has_child;
	has_child = gtk_tree_model_iter_has_child (tree_model, iter);
	if (text) {
		if (has_child)
			g_object_set (G_OBJECT(cell), "xpad", 18, NULL);
		else
			g_object_set (G_OBJECT(cell), "xpad", 2, NULL);
	}
	g_object_set (G_OBJECT(cell), "sensitive", !has_child, NULL);
}

static GtkWidget *
get_contacts_widget (NstPlugin *plugin)
{
	GtkWidget *cb;
	GtkCellRenderer *renderer;
	GtkTreeStore *store;
	GtkTreeModel *model;
	GtkTreeIter *iter, *iter2;

	iter = g_malloc (sizeof(GtkTreeIter));
	iter2 = g_malloc (sizeof(GtkTreeIter));
	store = gtk_tree_store_new (NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	add_pidgin_contacts_to_model (store, iter, iter2);
	model = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model), COL_ALIAS,
					      GTK_SORT_ASCENDING);
	cb = gtk_combo_box_new_with_model (model);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb),
				    renderer,
				    FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb),
					renderer,
					"pixbuf", COL_ICON,
					NULL);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (cb), renderer,
					    customize,
					    (gboolean *)FALSE, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb),
					renderer,
					"text", COL_ALIAS,
					NULL);
	g_object_set(renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (cb), renderer,
					    customize,
					    (gboolean *)TRUE, NULL);

	gtk_combo_box_set_active (GTK_COMBO_BOX (cb), 0);
	gtk_combo_box_get_active_iter (GTK_COMBO_BOX(cb), iter);
	if (gtk_tree_model_iter_has_child (model, iter)) {
		GtkTreePath *path = gtk_tree_path_new_from_indices (0, 0, -1);
		gtk_tree_model_get_iter (model, iter2, path);
		gtk_tree_path_free (path);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (cb), iter2);
	}

	g_free (iter);
	g_free (iter2);
	return cb;
}

static
gboolean send_file(int account, const char *who, const char *filename)
{
	GError *error;
	int connection;

	error = NULL;
	if (!dbus_g_proxy_call(proxy, "PurpleAccountGetConnection", &error,
			       G_TYPE_INT, account,
			       G_TYPE_INVALID,
			       G_TYPE_INT, &connection, G_TYPE_INVALID)) {
		handle_dbus_exception(error);
		return FALSE;
	}

	if (!connection) {
		g_warning("[Pidgin] account is not connected");
		return FALSE;
	}

	error = NULL;
	if (!dbus_g_proxy_call(proxy, "ServSendFile", &error,
			       G_TYPE_INT, connection,
			       G_TYPE_STRING, who,
			       G_TYPE_STRING, filename,
			       G_TYPE_INVALID, G_TYPE_INVALID)) {
		handle_dbus_exception(error);
		return FALSE;
	}
	return TRUE;
}

static
gboolean send_files (NstPlugin *plugin, GtkWidget *contact_widget,
		     GList *file_list)
{
	GError *error;
	GList *file_iter;

	GFile *file;
	gchar *file_path;

	gint depth;
	GtkTreeIter iter;
	GtkTreePath *path;
	gint *indices;
	const gchar *alias;
	GPtrArray *contacts_group;
	ContactData *dat;
	GValue val = {0,};


	if(proxy == NULL)
		return FALSE;

	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (contact_widget), &iter);
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (
		gtk_combo_box_get_model (GTK_COMBO_BOX(
			contact_widget))), &iter);
	depth = gtk_tree_path_get_depth(path);
	indices = gtk_tree_path_get_indices(path);
	gtk_tree_path_free (path);
	gtk_tree_model_get_value (GTK_TREE_MODEL (gtk_combo_box_get_model (
		    GTK_COMBO_BOX(contact_widget))),
	    &iter, COL_ALIAS, &val);
	alias = g_value_get_string (&val);
	contacts_group = g_hash_table_lookup (contact_hash, alias);
	g_value_unset (&val);
	dat = g_ptr_array_index (contacts_group, (depth == 2)?indices[1]:0);

	for(file_iter = file_list; file_iter != NULL;
	    file_iter = g_list_next(file_iter)) {
		error= NULL;

		file = g_file_new_for_uri ((gchar *)file_iter->data);
		file_path = g_file_get_path (file);
		g_object_unref (file);

		if(file_path == NULL) {
			g_warning("[Pidgin] %d Unable to convert URI `%s' to absolute file path",
				  error->code, (gchar *)file_iter->data);
			g_error_free(error);
			continue;
		}

		if(!send_file(dat->account, dat->name, file_path))
			g_warning("[Pidgin] Failed to send %s file to %s", file_path, dat->name);
	}
	return TRUE;
}

static void
free_contact (ContactData *dat)
{
	g_free(dat->name);
	g_free(dat->alias);
	g_free(dat);
}

static gboolean
destroy (NstPlugin *plugin)
{
	GHashTableIter iter;
	GPtrArray *contacts_group;
	ContactData *dat;

	g_hash_table_iter_init (&iter, contact_hash);
	while (g_hash_table_iter_next (&iter, NULL, (gpointer)&contacts_group)) {
		gint accounts;
		accounts = contacts_group->len;

		gint i;
		for (i = 0; i < accounts; ++i) {
			dat = g_ptr_array_index (contacts_group, i);
			free_contact (dat);
		}
		g_ptr_array_free (contacts_group, TRUE);
	}
	g_hash_table_destroy (contact_hash);
	return TRUE;
}

static
NstPluginInfo plugin_info = {
	"im",
	"pidgin",
	N_("Instant Message (Pidgin)"),
	NULL,
	NEMO_CAPS_NONE,
	init,
	get_contacts_widget,
	NULL,
	send_files,
	destroy
};

NST_INIT_PLUGIN (plugin_info)

