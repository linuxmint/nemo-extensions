/* #################################################
 Author:  Igor Feghali <ifeghali@interveritas.net>
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License as
 published by the Free Software Foundation; either version 2 of the
 License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more av.

 You should have received a copy of the GNU General Public
 License along with this program; if not, write to the
 Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.

 Based on evolution plugin by Roberto Majadas <roberto.majadas@hispalinux.es>

################################################# */

#include "config.h"
#include "nemo-sendto-plugin.h"
#include <glib/gi18n-lib.h>

static GHashTable *hash = NULL;

static 
gboolean init (NstPlugin *plugin)
{
	g_print ("Init thunderbird plugin\n");
	
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);
	
	hash = g_hash_table_new (g_str_hash, g_str_equal);
	return TRUE;
}

static
GtkWidget* get_contacts_widget (NstPlugin *plugin)
{
	GtkWidget *entry;
	GtkEntryCompletion *completion;
	GtkCellRenderer *renderer;
	
	entry = gtk_entry_new ();
	completion = gtk_entry_completion_new ();
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (completion), renderer, "pixbuf", 0, NULL);

	gtk_entry_set_completion (GTK_ENTRY (entry), completion);
	gtk_entry_completion_set_text_column (completion, 1);
	g_object_unref (completion);
	
	return entry;
}

static
gboolean send_files (NstPlugin *plugin, GtkWidget *contact_widget,
			GList *file_list)
{
	gchar *thunder_cmd, *cmd, *send_to;
	GList *l;
	GString *attach;

	send_to = (gchar *) gtk_entry_get_text (GTK_ENTRY(contact_widget));
	thunder_cmd = g_find_program_in_path ("thunderbird");
	attach = g_string_new ("\"\"");

	// #STORING SELECTED FILES IN A STRING##############
	g_string_append_printf (attach,"file://%s",file_list->data);

	for (l = file_list->next ; l; l=l->next)
		g_string_append_printf (attach,",file://%s",l->data);
	// #################################################

	// #CALLING THUNDERBIRD#############################
	cmd = g_strdup_printf ("%s -compose to=%s,\"attachment='%s'\"", thunder_cmd, send_to, attach->str);
	g_spawn_command_line_async (cmd, NULL);
	// #################################################

	// #CLEANING UP THE HOUSE###########################
	g_free (cmd);
	g_free (thunder_cmd);
	g_free (send_to);
	g_string_free (attach, TRUE);
	// #################################################

	return TRUE;
}

static 
gboolean destroy (NstPlugin *plugin)
{
	return TRUE;
}

static 
NstPluginInfo plugin_info = 
{
	"stock_mail",
	"thunderbird",
	N_("Email (Thunderbird)"),
	NULL,
	NEMO_CAPS_NONE,
	init,
	get_contacts_widget,
	NULL,
	send_files,
	destroy
}; 

NST_INIT_PLUGIN (plugin_info)
