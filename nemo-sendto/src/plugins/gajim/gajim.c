/*
 * gajim.c
 *       gajim plugin for nemo-sendto
 *
 * Copyright (C) 2006 Dimitur Kirov
 *               2006 Roberto Majadas <telemaco@openshine.com>
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
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include "nemo-sendto-plugin.h"

#define OBJ_PATH "/org/gajim/dbus/RemoteObject"
#define INTERFACE "org.gajim.dbus.RemoteInterface"
#define SERVICE "org.gajim.dbus"

const gchar *COMPLETION_PROPS[] = {"name", "jid"};
/* list of contacts, which are not offline */
static GHashTable *jid_table = NULL;
static gchar *iconset;


DBusGProxy *proxy = NULL;

/*
 * contact cb, gets property from contact dict
 * and put online contacts to jid_table
 */
static void
_foreach_contact(gpointer contact, gpointer user_data)
{
	const gchar *show;
	
	GValue *value;
	GHashTable *contact_table;
	
	/* holds contact props of already exisiting jid/nick */
	GHashTable *existing_contact;
	
	/* name of the contact in completion list
	   it may be jid, nick, jid (account), or nick(account) */
	GString *contact_str;
	
	gchar *jid;
	gchar *account;
	gint i;
	
	if (contact == NULL) {
		g_warning("Null contact in the list");
		return;
	}
	contact_table = (GHashTable *) contact;
	account = (gchar *) user_data;
	
	value = g_hash_table_lookup(contact_table, "show");
	if (value == NULL || !G_VALUE_HOLDS_STRING(value)) {
		g_warning("String expected (contact - show)");
		g_hash_table_destroy(contact_table);
		return;
	}
	show = g_value_get_string ((GValue *)value);
	if(g_str_equal(show, "offline") || g_str_equal(show, "error")) {
		g_hash_table_destroy(contact_table);
		return;
	}
	/* remove unneeded item with key resource and add account
	   to contact properties */
	g_hash_table_insert(contact_table, "account", account);
	g_hash_table_remove(contact_table, "resource");
	
	/* add nick the same way as jid */
	for(i=0;i<2;i++) {
		value = g_hash_table_lookup(contact_table, COMPLETION_PROPS[i]);	
		if(value == NULL || !G_VALUE_HOLDS_STRING(value)) {
			g_warning("String expected (contact - name)");
			return;
		}
		jid = g_value_dup_string((GValue *)value);
		existing_contact = g_hash_table_lookup(jid_table, jid);
		if(existing_contact) {
			/* add existing contact as nick (account) */
			contact_str = g_string_new(jid);
			g_string_append(contact_str, " (");
			g_string_append(contact_str,
				g_hash_table_lookup(existing_contact, "account"));
			g_string_append(contact_str, ")");
			g_hash_table_insert(jid_table, contact_str->str,
													existing_contact);
			g_string_free(contact_str, FALSE);
			
			/* add current contact as nick (account) */
			contact_str = g_string_new(jid);
			g_string_append(contact_str, " (");
			g_string_append(contact_str,
				g_hash_table_lookup(contact_table, "account"));
			g_string_append(contact_str, ")");
			g_hash_table_insert(jid_table, contact_str->str,
													contact_table);
			g_string_free(contact_str, FALSE);
		}
		else {
			g_hash_table_insert(jid_table, jid, contact_table);
		}
	}
	
}

/*
 * connect to session bus, onsuccess return TRUE
 */
static gboolean
init_dbus (void)
{
	DBusGConnection *connection;
	GError *error;
	gchar **accounts;

	error = NULL;
	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if(error != NULL) {
		g_warning("[Gajim] unable to get session bus, error was:\n %s", error->message);
		g_error_free(error);
		return FALSE;
	}
	proxy = dbus_g_proxy_new_for_name(connection,
									 SERVICE,
									 OBJ_PATH,
									 INTERFACE);
	dbus_g_connection_unref(connection);
	if (proxy == NULL){
		return FALSE;
	}

	error = NULL;
	if (!dbus_g_proxy_call (proxy, "list_accounts", &error, G_TYPE_INVALID,
				G_TYPE_STRV, &accounts, G_TYPE_INVALID))
	{
		g_object_unref(proxy);
		g_error_free(error);
		return FALSE;		
	}
	g_strfreev(accounts);
	return TRUE;
}

/*
 * Print appropriate warnings when dbus raised error
 * on queries
 */
static void
_handle_dbus_exception (GError *error, gboolean empty_list_messages)
{
	if (error == NULL) {
		g_warning("[Gajim] unable to parse result");
		return;
	}
	else if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION) {
		g_warning ("[Gajim] caught remote method exception %s: %s",
			dbus_g_error_get_name (error),
			error->message);
	}
	else if(empty_list_messages) {
		/* empty list and error goes here */
		g_warning ("[Gajim] empty result set: %d %d %s\n", error->domain,
			   error->code, error->message);
	}
	g_error_free (error);
}

/*
 * query object, about the contact list for each account
 * and fill all available contacts in the contacts table
 */
static gboolean
_get_contacts (void)
{
	GError *error;
	GSList *contacts_list;
	GHashTable *prefs_map;
	gchar **accounts;
	gchar **account_iter;
	gchar *account;
	
	error = NULL;

	if (proxy == NULL) {
		g_warning("[Gajim] unable to connect to session bus");
		return FALSE;
	}
	/* get gajim prefs and lookup for iconset */
	if (!dbus_g_proxy_call(proxy, "prefs_list", &error, G_TYPE_INVALID,
			dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_STRING),
			&prefs_map, G_TYPE_INVALID))
	{
		_handle_dbus_exception(error, TRUE);
		return FALSE;
	}
	gpointer iconset_ptr = g_hash_table_lookup(prefs_map, "iconset");
	if (iconset_ptr != NULL) {
		iconset = g_strdup((gchar *)iconset_ptr);
	} else {
		g_warning("[Gajim] unable to get prefs value for iconset");
		return FALSE;
	}
	g_hash_table_destroy(prefs_map);
	/* END get gajim prefs */
	error= NULL;
	if (!dbus_g_proxy_call (proxy, "list_accounts", &error, G_TYPE_INVALID,
			G_TYPE_STRV,
			&accounts, G_TYPE_INVALID))
	{
		_handle_dbus_exception(error, TRUE);
		return FALSE;
	}
	for(account_iter = accounts; *account_iter ; account_iter++) {
		account = g_strdup(*account_iter);
		error = NULL;	
		/* query gajim remote object and put results in 'contacts_list' */
		if (!dbus_g_proxy_call (proxy, "list_contacts", &error,
				G_TYPE_STRING, account, /* call arguments */
				G_TYPE_INVALID, /* delimiter */
				/* return value is collection of maps */
				dbus_g_type_get_collection ("GSList",
					dbus_g_type_get_map ("GHashTable",
						G_TYPE_STRING, G_TYPE_VALUE)),
				&contacts_list, G_TYPE_INVALID))
		{
			_handle_dbus_exception(error, FALSE);
			error = NULL;
			continue;
		}
		g_slist_foreach (contacts_list, _foreach_contact, account);
		g_slist_free(contacts_list);
	}
	g_strfreev (accounts);
	return TRUE;
}

static gboolean
init (NstPlugin *plugin)
{
	g_print ("Init gajim plugin\n");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	
	/* connect to gajim dbus service */
	jid_table = g_hash_table_new (g_str_hash, g_str_equal);
	if (!init_dbus()) {
		return FALSE;
	}
	return TRUE;
}


static void
_set_pixbuf_from_status (const gchar *show, GdkPixbuf **pixbuf)
{
	GString *pixbuf_path;
	GError *error;
	
	pixbuf_path = g_string_new(GAJIM_SHARE_DIR);
	g_string_append_c(pixbuf_path, '/');
	g_string_append(pixbuf_path, "data");
	g_string_append_c(pixbuf_path, '/');
	g_string_append(pixbuf_path, "iconsets");
	g_string_append_c(pixbuf_path, '/');
	g_string_append(pixbuf_path, iconset);
	g_string_append_c(pixbuf_path, '/');
	g_string_append(pixbuf_path, "16x16");
	g_string_append_c(pixbuf_path, '/');
	g_string_append(pixbuf_path, show);
	g_string_append(pixbuf_path, ".png");
	if(g_file_test(pixbuf_path->str, G_FILE_TEST_EXISTS) &&
		g_file_test(pixbuf_path->str, G_FILE_TEST_IS_REGULAR)) {
		error = NULL;
		*pixbuf = gdk_pixbuf_new_from_file(pixbuf_path->str, &error);
		if(error != NULL) {
			g_error_free(error);
		}
	}
	g_string_free(pixbuf_path, FALSE);
}

static void
_add_contact_to_model(gpointer key, gpointer value, gpointer user_data)
{
	GtkTreeIter *iter;
	GtkListStore *store;
	GdkPixbuf *pixbuf;
	GValue *val;
	GHashTable *contact_props;
	const gchar *show;
	
	contact_props = (GHashTable *) value;
	pixbuf = NULL;
	val = g_hash_table_lookup(contact_props, "show");
	if (value == NULL || !G_VALUE_HOLDS_STRING(val)) {
		g_warning("String expected (contact - show)");
		pixbuf = NULL;
	} else {
		show = g_value_get_string ((GValue *)val);
		_set_pixbuf_from_status(show, &pixbuf);
	}
	
	store = (GtkListStore *) user_data;
	iter = g_malloc (sizeof(GtkTreeIter));
	gtk_list_store_append (store, iter);
	gtk_list_store_set (store, iter, 0, pixbuf, 1, key, -1);
	g_free (iter);
}

/*
 * put gajim contacts to jid_list
 * filtering only these which are connected
 */
static gboolean
add_gajim_contacts_to_model (GtkListStore *store)
{
	if(!_get_contacts()) {
		return FALSE;	
	}
	if(g_hash_table_size(jid_table) == 0) {
		return FALSE;
	}
	g_hash_table_foreach(jid_table, _add_contact_to_model, store);
	return TRUE;
}

/*
 * fill completion model for the entry, using list of
 * available gajim contacts
 */
static GtkWidget *
get_contacts_widget (NstPlugin *plugin)
{
	GtkWidget *entry;
	GtkEntryCompletion *completion;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeModel *completion_model;
	
	entry = gtk_entry_new ();
	completion = gtk_entry_completion_new ();
	
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion),
					renderer,
					FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (completion), renderer,
					"pixbuf", 0, NULL);
	
	
	store = gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	if(!add_gajim_contacts_to_model (store)) {
		gtk_widget_set_sensitive(entry, FALSE);
	}
	completion_model = GTK_TREE_MODEL (store);
	gtk_entry_completion_set_model (completion, completion_model);
	gtk_entry_set_completion (GTK_ENTRY (entry), completion);
	gtk_entry_completion_set_text_column (completion, 1);
	g_object_unref (completion_model);
	g_object_unref (completion);
	return entry;
}

static void
show_error (const gchar *title, const gchar *message)
{
	GtkWidget *dialog;
	
	dialog = gtk_message_dialog_new_with_markup(NULL,
								GTK_DIALOG_DESTROY_WITH_PARENT,
								GTK_MESSAGE_ERROR,
								GTK_BUTTONS_CLOSE, NULL);
	gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog),
                                g_markup_printf_escaped("<b>%s</b>\n\n%s", title, message));
	gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
	
}

static gboolean
send_files (NstPlugin *plugin,
	    GtkWidget *contact_widget,
	    GList *file_list)
{
	GError *error;
	GValue *value;
	GList *file_iter;
	GHashTable *contact_props;
	
	gchar *send_to;
	gchar *jid;
	gchar *account;
	gchar *file_path;

	if(proxy == NULL) {
		show_error(_("Unable to send file"),
			   _("There is no connection to gajim remote service."));
		return FALSE;
	}
	send_to = (gchar *) gtk_entry_get_text (GTK_ENTRY(contact_widget));
	g_debug("[Gajim] sending to: %s", send_to);
	if (strlen (send_to) != 0){
		contact_props = g_hash_table_lookup (jid_table, send_to);
		if(contact_props == NULL) {
			jid = send_to;
			account = NULL;
		}
		else {
			value = g_hash_table_lookup(contact_props, "jid");	
			if(value == NULL || !G_VALUE_HOLDS_STRING(value)) {
				g_warning("[Gajim] string expected (contact - jid)");
				return FALSE;
			}
			
			jid = g_value_dup_string((GValue *)value);
			account = g_hash_table_lookup(contact_props, "account");	
		}
	}
	else {
		g_warning("[Gajim] missing recipient");
		show_error(_("Sending file failed"),
						_("Recipient is missing."));
		return FALSE;
	}
	
	error= NULL;
	for(file_iter = file_list; file_iter != NULL; file_iter = file_iter->next) {
		char *uri = file_iter->data;

		g_debug("[Gajim] file: %s", uri);
		error= NULL;
		file_path = g_filename_from_uri(uri, NULL, &error);
		if(error != NULL) {
			g_warning("%d Unable to convert URI `%s' to absolute file path",
				error->code, uri);
			g_error_free(error);
			continue;
		}

		g_debug("[Gajim] file: %s", file_path);
		if(account) {
			dbus_g_proxy_call (proxy, "send_file", &error,
					   G_TYPE_STRING, file_path,
					   G_TYPE_STRING, jid,
					   G_TYPE_STRING, account,
					   G_TYPE_INVALID,
					   G_TYPE_INVALID);
		} else {
			dbus_g_proxy_call (proxy, "send_file", &error,
					   G_TYPE_STRING, file_path,
					   G_TYPE_STRING, jid,
					   G_TYPE_INVALID,
					   G_TYPE_INVALID);
		}
		g_free(file_path);
		if(error != NULL)
		{
			if(error->domain != DBUS_GERROR || error->code != DBUS_GERROR_INVALID_ARGS) {
				g_warning("[Gajim] sending file %s to %s failed:", uri, send_to);
				g_error_free(error);
				show_error(_("Sending file failed"), _("Unknown recipient."));
				return FALSE;
			}
			g_error_free(error);
		}
	}
	return TRUE;
}

static gboolean
destroy (NstPlugin *plugin)
{
	if (proxy != NULL) {
		g_object_unref(proxy);
	}
	g_hash_table_destroy(jid_table);
	return TRUE;
}

static
NstPluginInfo plugin_info = {
	"im-jabber",
	"gajim",
	N_("Instant Message (Gajim)"),
	NULL,
	NEMO_CAPS_NONE,
	init,
	get_contacts_widget,
	NULL,
	send_files,
	destroy
};

NST_INIT_PLUGIN (plugin_info)

