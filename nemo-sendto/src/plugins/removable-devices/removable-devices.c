/*
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
 * Authors:  Maxim Ermilov <ermilov.maxim@gmail.com>
 *           Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"
#include <string.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include "nst-common.h"
#include "nemo-sendto-plugin.h"

enum {
	NAME_COL,
	ICON_COL,
	MOUNT_COL,
	NUM_COLS,
};

GVolumeMonitor* vol_monitor = NULL;
GtkWidget *cb;

static void
cb_mount_removed (GVolumeMonitor *volume_monitor,
		  GMount         *mount,
		  NstPlugin      *plugin)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean b, found;

	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (cb)));
	b = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
	found = FALSE;

	while (b) {
		GMount *m;
		gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, MOUNT_COL, &m, -1);
		if (m == mount) {
			gtk_list_store_remove (store, &iter);
			g_object_unref (m);
			found = TRUE;
			break;
		}
		g_object_unref (m);
		b = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
	}

	/* If a mount was removed */
	if (found != FALSE) {
		/* And it was the selected one */
		if (gtk_combo_box_get_active (GTK_COMBO_BOX (cb)) == -1) {
			/* Select the first item in the list */
			gtk_combo_box_set_active (GTK_COMBO_BOX (cb), 0);
		}
	}
}

static void
cb_mount_changed (GVolumeMonitor *volume_monitor,
		  GMount         *mount,
		  NstPlugin      *plugin)
{
	GtkTreeIter iter;
	gboolean b;
	GtkListStore *store;

	if (g_mount_is_shadowed (mount) != FALSE) {
		cb_mount_removed (volume_monitor, mount, plugin);
		return;
	}

	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (cb)));
	b = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);

	while (b) {
		GMount *m;
		gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, MOUNT_COL, &m, -1);

		if (m == mount) {
			char *name;

			name = g_mount_get_name (mount);
			gtk_list_store_set (store, &iter,
					    NAME_COL, name,
					    ICON_COL, g_mount_get_icon (mount),
					    -1);
			g_free (name);
			g_object_unref (m);
			break;
		}
		g_object_unref (m);
		b = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
	}
}

static void
cb_mount_added (GVolumeMonitor *volume_monitor,
		GMount         *mount,
		NstPlugin      *plugin)
{
	char *name;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean select_added;

	if (g_mount_is_shadowed (mount) != FALSE)
		return;

	name = g_mount_get_name (mount);
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (cb));

	select_added = gtk_tree_model_iter_n_children (model, NULL) == 0;

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    NAME_COL, name,
			    ICON_COL, g_mount_get_icon (mount),
			    MOUNT_COL, mount,
			    -1);

	g_free (name);

	if (select_added != FALSE)
		gtk_combo_box_set_active (GTK_COMBO_BOX (cb), 0);

}

static gboolean
init (NstPlugin *plugin)
{
	g_print ("Init removable-devices plugin\n");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	vol_monitor = g_volume_monitor_get ();
	cb = gtk_combo_box_new ();

	return TRUE;
}

static GtkWidget*
get_contacts_widget (NstPlugin *plugin)
{
	GtkListStore *store;
	GList *l, *mounts;
	GtkTreeIter iter;
	GtkCellRenderer *text_renderer, *icon_renderer;

	mounts = g_volume_monitor_get_mounts (vol_monitor);

	store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_ICON, G_TYPE_OBJECT);

	for (l = mounts; l != NULL; l = l->next) {
		char *name;

		if (g_mount_is_shadowed (l->data) != FALSE) {
			g_object_unref (l->data);
			continue;
		}

		name = g_mount_get_name (l->data);

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    NAME_COL, name,
				    ICON_COL, g_mount_get_icon (l->data),
				    MOUNT_COL, l->data,
				    -1);
		g_free (name);

		g_object_unref (l->data);
	}
	g_list_free (mounts);

	gtk_cell_layout_clear (GTK_CELL_LAYOUT (cb));
	gtk_combo_box_set_model (GTK_COMBO_BOX (cb), GTK_TREE_MODEL (store));

	text_renderer = gtk_cell_renderer_text_new ();
	icon_renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb), icon_renderer, FALSE);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb), text_renderer, TRUE);

	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb), text_renderer, "text", 0,  NULL);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb), icon_renderer, "gicon", 1,  NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (cb), 0);

	g_signal_connect (G_OBJECT (vol_monitor), "mount-removed", G_CALLBACK (cb_mount_removed), plugin);
	g_signal_connect (G_OBJECT (vol_monitor), "mount-added", G_CALLBACK (cb_mount_added), plugin);
	g_signal_connect (G_OBJECT (vol_monitor), "mount-changed", G_CALLBACK (cb_mount_changed), plugin);

	return cb;
}

static gboolean
send_files (NstPlugin *plugin, GtkWidget *contact_widget,
	    GList *file_list)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GMount *dest_mount;
	GFile *mount_root;

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (contact_widget), &iter) == FALSE)
		return TRUE;

	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (cb)));
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, MOUNT_COL, &dest_mount, -1);
	mount_root = g_mount_get_root (dest_mount);

	copy_files_to (file_list, mount_root);

	g_object_unref (mount_root);

	return TRUE;
}

static gboolean
destroy (NstPlugin *plugin)
{
	gtk_widget_destroy (cb);

	g_object_unref (vol_monitor);
	return TRUE;
}

static
NstPluginInfo plugin_info = {
	"folder-remote",
	"folder-remote",
	N_("Removable disks and shares"),
	NULL,
	NEMO_CAPS_SEND_DIRECTORIES,
	init,
	get_contacts_widget,
	NULL,
	send_files,
	destroy
};

NST_INIT_PLUGIN (plugin_info)

