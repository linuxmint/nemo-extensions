/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* 
 * Copyright (C) 2004 Roberto Majadas
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
#include <string.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include "nemo-sendto-plugin.h"

#define NEMO_SENDTO_LAST_MEDIUM	"last-medium"
#define NEMO_SENDTO_LAST_COMPRESS	"last-compress"
#define NEMO_SENDTO_STATUS_LABEL_TIMEOUT_SECONDS 10

#define UNINSTALLED_PLUGINDIR "plugins/removable-devices"
#define UNINSTALLED_SOURCE "nemo-sendto-command.c"

#define SOEXT           ("." G_MODULE_SUFFIX)
#define SOEXT_LEN       (strlen (SOEXT))

enum {
	COLUMN_ICON,
	COLUMN_DESCRIPTION,
	NUM_COLUMNS,
};

/* Options */
static char **filenames = NULL;

GList *file_list = NULL;
gboolean has_dirs = FALSE;
GList *plugin_list = NULL;
GHashTable *hash ;
guint option = 0;

static GSettings *settings = NULL;

typedef struct _NS_ui NS_ui;

struct _NS_ui {
	GtkWidget *dialog;
	GtkWidget *options_combobox;
	GtkWidget *send_to_label;
	GtkWidget *hbox_contacts_ws;
	GtkWidget *cancel_button;
	GtkWidget *send_button;
	GtkWidget *pack_combobox;
	GtkWidget *pack_checkbutton;
	GtkWidget *pack_entry;
	GList *contact_widgets;

	GtkWidget *status_box;
	GtkWidget *status_image;
	GtkWidget *status_label;
	guint status_timeoutid;
};

static const GOptionEntry entries[] = {
	{ G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, "Files to send", "[FILES...]" },
	{ NULL }
};

static void 
destroy_dialog (GtkWidget *widget, gpointer data )
{
        gtk_main_quit ();
}

static char *
get_filename_from_list (void)
{
	GList *l;
	GString *common_part = NULL;
	gboolean matches = TRUE;
	guint offset = 0;
	const char *encoding;
	gboolean use_utf8 = TRUE;

	encoding = g_getenv ("G_FILENAME_ENCODING");

	if (encoding != NULL && strcasecmp(encoding, "UTF-8") != 0)
		use_utf8 = FALSE;

	if (file_list == NULL)
		return NULL;

	common_part = g_string_new("");

	while (TRUE) {
		gunichar cur_char = '\0';
		for (l = file_list; l ; l = l->next) {
			char *path = NULL, *name = NULL;
			char *offset_name = NULL;

			path = g_filename_from_uri ((char *) l->data,
					NULL, NULL);
			if (!path)
				break;

			name = g_path_get_basename (path);

			if (!use_utf8) {
				char *tmp;

				tmp = g_filename_to_utf8 (name, -1,
						NULL, NULL, NULL);
				g_free (name);
				name = tmp;
			}

			if (!name) {
				g_free (path);
				break;
			}

			if (offset >= g_utf8_strlen (name, -1)) {
				g_free(name);
				g_free(path);
				matches = FALSE;
				break;
			}

			offset_name = g_utf8_offset_to_pointer (name, offset);

			if (offset_name == g_utf8_strrchr (name, -1, '.')) {
				g_free (name);
				g_free (path);
				matches = FALSE;
				break;
			}
			if (cur_char == '\0') {
				cur_char = g_utf8_get_char (offset_name);
			} else if (cur_char != g_utf8_get_char (offset_name)) {
				g_free (name);
				g_free (path);
				matches = FALSE;
				break;
			}
			g_free (name);
			g_free (path);
		}
		if (matches == TRUE && cur_char != '\0') {
			offset++;
			common_part = g_string_append_unichar (common_part,
					cur_char);
		} else {
			break;
		}
	}

	if (g_utf8_strlen (common_part->str, -1) < 4) {
		g_string_free (common_part, TRUE);
		return NULL;
	}

	return g_string_free (common_part, FALSE);
}

static char *
pack_files (NS_ui *ui)
{
	char *file_roller_cmd;
	const char *filename;
	GList *l;
	GString *cmd, *tmp;
	char *pack_type, *tmp_dir, *tmp_work_dir, *packed_file;

	file_roller_cmd = g_find_program_in_path ("file-roller");
	filename = gtk_entry_get_text(GTK_ENTRY(ui->pack_entry));

	g_assert (filename != NULL && *filename != '\0');
	
	tmp_dir = g_strdup_printf ("%s/nemo-sendto-%s", 
				   g_get_tmp_dir(), g_get_user_name());	
	g_mkdir (tmp_dir, 0700);
	tmp_work_dir = g_strdup_printf ("%s/nemo-sendto-%s/%li",
					g_get_tmp_dir(), g_get_user_name(),
					time(NULL));
	g_mkdir (tmp_work_dir, 0700);
	g_free (tmp_dir);

	switch (gtk_combo_box_get_active (GTK_COMBO_BOX(ui->pack_combobox)))
	{
	case 0:
		pack_type = g_strdup (".zip");
		break;
	case 1:
		pack_type = g_strdup (".tar.gz");
		break;
	case 2: 
		pack_type = g_strdup (".tar.bz2");
		break;
	default:
		pack_type = NULL;
		g_assert_not_reached ();
	}

	g_settings_set_int (settings,
			    NEMO_SENDTO_LAST_COMPRESS,
			    gtk_combo_box_get_active(GTK_COMBO_BOX(ui->pack_combobox)));

	cmd = g_string_new ("");
	g_string_printf (cmd, "%s --add-to=\"%s/%s%s\"",
			 file_roller_cmd, tmp_work_dir,
			 filename,
			 pack_type);

	/* file-roller doesn't understand URIs */
	for (l = file_list ; l; l=l->next){
		char *file;

		file = g_filename_from_uri (l->data, NULL, NULL);
		g_string_append_printf (cmd," \"%s\"", file);
		g_free (file);
	}

	g_spawn_command_line_sync (cmd->str, NULL, NULL, NULL, NULL);
	g_string_free (cmd, TRUE);
	tmp = g_string_new("");
	g_string_printf (tmp,"%s/%s%s", tmp_work_dir,
			 filename,
			 pack_type);
	g_free (tmp_work_dir);
	packed_file = g_filename_to_uri (tmp->str, NULL, NULL);
	g_string_free(tmp, TRUE);
	return packed_file;
}

static gboolean
status_label_clear (gpointer data)
{
	NS_ui *ui = (NS_ui *) data;
	gtk_label_set_label (GTK_LABEL (ui->status_label), "");
	gtk_widget_hide (ui->status_image);

	ui->status_timeoutid = 0;

	return FALSE;
}

static void
send_button_cb (GtkWidget *widget, NS_ui *ui)
{
	char *f, *error;
	NstPlugin *p;
	GtkWidget *w;

	gtk_widget_set_sensitive (ui->dialog, FALSE);

	p = (NstPlugin *) g_list_nth_data (plugin_list, option);
	w = (GtkWidget *) g_list_nth_data (ui->contact_widgets, option);

	if (ui->status_timeoutid != 0) {
		g_source_remove (ui->status_timeoutid);
		status_label_clear (ui);
	}

	if (p == NULL)
		return;

	if (p->info->validate_destination != NULL) {
		error = NULL;
		if (p->info->validate_destination (p, w, &error) == FALSE) {
			char *message;

			message = g_strdup_printf ("<b>%s</b>", error);
			g_free (error);
			gtk_label_set_markup (GTK_LABEL (ui->status_label), message);
			g_free (message);
			ui->status_timeoutid = g_timeout_add_seconds (NEMO_SENDTO_STATUS_LABEL_TIMEOUT_SECONDS,
								      status_label_clear,
								      ui);
			gtk_widget_show (ui->status_image);
			gtk_widget_show (ui->status_box);
			gtk_widget_set_sensitive (ui->dialog, TRUE);
			return;
		}
	}

	g_settings_set_string (settings,
			       NEMO_SENDTO_LAST_MEDIUM,
			       p->info->id);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->pack_checkbutton))){
		f = pack_files (ui);
		if (f != NULL) {
			GList *packed_file = NULL;
			packed_file = g_list_append (packed_file, f);
			if (!p->info->send_files (p, w, packed_file)) {
				g_list_free (packed_file);
				return;
			}
			g_list_free (packed_file);
		} else {
			gtk_widget_set_sensitive (ui->dialog, TRUE);
			return;
		}
	} else {
		if (!p->info->send_files (p, w, file_list)) {
			g_list_foreach (file_list, (GFunc) g_free, NULL);
			g_list_free (file_list);
			file_list = NULL;
			return;
		}
		g_list_free (file_list);
		file_list = NULL;
	}
	destroy_dialog (NULL,NULL);
}

static void
send_if_no_pack_cb (GtkWidget *widget, NS_ui *ui)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ui->pack_checkbutton))) {
		if (gtk_widget_is_sensitive (ui->pack_entry)) {
			gtk_widget_grab_focus (ui->pack_entry);
		} else {
			gtk_widget_grab_focus (ui->pack_checkbutton);
		}
	} else {
		send_button_cb (widget, ui);
	}
}

static void
toggle_pack_check (GtkWidget *widget, NS_ui *ui)
{
	GtkToggleButton *t = GTK_TOGGLE_BUTTON (widget);
	gboolean enabled, send_enabled;

	enabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (t));
	gtk_widget_set_sensitive (ui->pack_combobox, enabled);
	gtk_widget_set_sensitive (ui->pack_entry, enabled);

	send_enabled = TRUE;

	if (enabled) {
		const char *filename;

		filename = gtk_entry_get_text(GTK_ENTRY(ui->pack_entry));
		if (filename == NULL || *filename == '\0')
			send_enabled = FALSE;
	}

	gtk_widget_set_sensitive (ui->send_button, send_enabled);
}

static void
option_changed (GtkComboBox *cb, NS_ui *ui)
{
	GList *aux;
	NstPlugin *p;
	gboolean supports_dirs = FALSE;

	aux = g_list_nth (ui->contact_widgets, option);
	option = gtk_combo_box_get_active (GTK_COMBO_BOX(cb));
	gtk_widget_hide ((GtkWidget *) aux->data);
	aux = g_list_nth (ui->contact_widgets, option);
	gtk_widget_show ((GtkWidget *) aux->data);

	gtk_label_set_mnemonic_widget (GTK_LABEL (ui->send_to_label), aux->data);

	p = (NstPlugin *) g_list_nth_data (plugin_list, option);
	supports_dirs = (p->info->capabilities & NEMO_CAPS_SEND_DIRECTORIES);

	if (has_dirs == FALSE || supports_dirs != FALSE) {
		gboolean toggle;

		toggle = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ui->pack_checkbutton));
		gtk_widget_set_sensitive (ui->pack_combobox, toggle);
		gtk_widget_set_sensitive (ui->pack_entry, toggle);
		gtk_widget_set_sensitive (ui->pack_checkbutton, TRUE);
	} else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ui->pack_checkbutton), TRUE);
		gtk_widget_set_sensitive (ui->pack_checkbutton, FALSE);
	}
}

static void
set_contact_widgets (NS_ui *ui)
{
	GList *aux ;
	GtkWidget *w;
	NstPlugin *p;

	ui->contact_widgets = NULL;

	for (aux = plugin_list; aux; aux = aux->next){
		p = (NstPlugin *) aux->data;
		w = p->info->get_contacts_widget(p);
		gtk_box_pack_end (GTK_BOX(ui->hbox_contacts_ws),w, TRUE, TRUE, 0);
		gtk_widget_hide (GTK_WIDGET(w));
		ui->contact_widgets = g_list_append (ui->contact_widgets, w);
		if (GTK_IS_ENTRY (w)) {
			g_signal_connect_after (G_OBJECT (w), "activate",
						G_CALLBACK (send_if_no_pack_cb), ui);
		}
	}
}

static gboolean
set_model_for_options_combobox (NS_ui *ui)
{
	GdkPixbuf *pixbuf;
        GtkTreeIter iter;
        GtkListStore *model;
	GtkIconTheme *it;
	GtkCellRenderer *renderer;
	GtkWidget *widget;
	GList *aux;
	NstPlugin *p;
	char *last_used = NULL;
	int i = 0;
	gboolean last_used_support_dirs = FALSE;

	it = gtk_icon_theme_get_default ();

	model = gtk_list_store_new (NUM_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING);

	last_used = g_settings_get_string (settings,
					   NEMO_SENDTO_LAST_MEDIUM);

	for (aux = plugin_list; aux; aux = aux->next) {
		p = (NstPlugin *) aux->data;
		pixbuf = gtk_icon_theme_load_icon (it, p->info->icon, 16, 
						   GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
					COLUMN_ICON, pixbuf,
					COLUMN_DESCRIPTION, dgettext(p->info->gettext_package, p->info->description),
					-1);
		if (last_used != NULL && !strcmp(last_used, p->info->id)) {
			option = i;
			last_used_support_dirs = (p->info->capabilities & NEMO_CAPS_SEND_DIRECTORIES);
		}
		i++;
	}
	g_free(last_used);

	gtk_combo_box_set_model (GTK_COMBO_BOX(ui->options_combobox),
				GTK_TREE_MODEL (model));
	renderer = gtk_cell_renderer_pixbuf_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (ui->options_combobox),
                                    renderer,
                                    FALSE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (ui->options_combobox), 
					renderer,
                                        "pixbuf", COLUMN_ICON,
                                        NULL);		
        renderer = gtk_cell_renderer_text_new ();
        g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (ui->options_combobox),
                                    renderer,
                                    TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (ui->options_combobox), 
					renderer,
                                        "text", COLUMN_DESCRIPTION,
                                        NULL);

	g_signal_connect (G_OBJECT (ui->options_combobox), "changed",
			  G_CALLBACK (option_changed), ui);

	gtk_combo_box_set_active (GTK_COMBO_BOX (ui->options_combobox), option);

	/* Grab the focus for the most recently used widget */
	widget = g_list_nth_data (ui->contact_widgets, option);
	gtk_widget_grab_focus (widget);

	return last_used_support_dirs;
}

static void
pack_entry_changed_cb (GObject *object, GParamSpec *spec, NS_ui *ui)
{
	gboolean send_enabled;

	send_enabled = TRUE;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ui->pack_checkbutton))) {
		const char *filename;

		filename = gtk_entry_get_text(GTK_ENTRY(ui->pack_entry));
		if (filename == NULL || *filename == '\0')
			send_enabled = FALSE;
	}

	gtk_widget_set_sensitive (ui->send_button, send_enabled);
}

static void
update_button_image (GtkSettings *settings,
		     GParamSpec *spec,
		     GtkWidget *widget)
{
	gboolean show_images;

	g_object_get (settings, "gtk-button-images", &show_images, NULL);
	if (show_images == FALSE)
		gtk_widget_hide (widget);
	else
		gtk_widget_show (widget);
}

static void
nemo_sendto_create_ui (void)
{
	GtkBuilder *app;
	GError* error = NULL;
	NS_ui *ui;
	gboolean one_file = FALSE;
	gboolean supports_dirs;
	GtkSettings *gtk_settings;
	GtkWidget *button_image;

	app = gtk_builder_new ();
	if (!gtk_builder_add_from_file (app, UIDIR "/" "nemo-sendto.ui", &error))	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	ui = g_new0 (NS_ui, 1);

	ui->hbox_contacts_ws = GTK_WIDGET (gtk_builder_get_object (app, "hbox_contacts_widgets"));
	ui->send_to_label = GTK_WIDGET (gtk_builder_get_object (app, "send_to_label"));
	ui->options_combobox = GTK_WIDGET (gtk_builder_get_object (app, "options_combobox"));
	ui->dialog = GTK_WIDGET (gtk_builder_get_object (app, "nemo_sendto_dialog"));
	ui->cancel_button = GTK_WIDGET (gtk_builder_get_object (app, "cancel_button"));
	ui->send_button = GTK_WIDGET (gtk_builder_get_object (app, "send_button"));
	ui->pack_combobox = GTK_WIDGET (gtk_builder_get_object (app, "pack_combobox"));	
	ui->pack_entry = GTK_WIDGET (gtk_builder_get_object (app, "pack_entry"));
	ui->pack_checkbutton = GTK_WIDGET (gtk_builder_get_object (app, "pack_checkbutton"));
	ui->status_box = GTK_WIDGET (gtk_builder_get_object (app, "status_box"));
	ui->status_label = GTK_WIDGET (gtk_builder_get_object (app, "status_label"));
	ui->status_image = GTK_WIDGET (gtk_builder_get_object (app, "status_image"));

	gtk_settings = gtk_settings_get_default ();
	button_image = GTK_WIDGET (gtk_builder_get_object (app, "image1"));
	g_signal_connect (G_OBJECT (gtk_settings), "notify::gtk-button-images",
			  G_CALLBACK (update_button_image), button_image);
	update_button_image (gtk_settings, NULL, button_image);

	gtk_combo_box_set_active (GTK_COMBO_BOX(ui->pack_combobox),
				  g_settings_get_int (settings,
						      NEMO_SENDTO_LAST_COMPRESS));

	if (file_list != NULL && file_list->next != NULL)
		one_file = FALSE;
	else if (file_list != NULL)
		one_file = TRUE;
	
	gtk_entry_set_text (GTK_ENTRY (ui->pack_entry), _("Files"));

	if (one_file) {
		char *filepath = NULL, *filename = NULL;

		filepath = g_filename_from_uri ((char *)file_list->data,
				NULL, NULL);

		if (filepath != NULL)
			filename = g_path_get_basename (filepath);
		if (filename != NULL && filename[0] != '\0')
			gtk_entry_set_text (GTK_ENTRY (ui->pack_entry), filename);

		g_free (filename);
		g_free (filepath);
	} else {
		char *filename = get_filename_from_list ();
		if (filename != NULL && filename[0] != '\0') {
			gtk_entry_set_text (GTK_ENTRY (ui->pack_entry),
					filename);
		}
		g_free (filename);
	}

	set_contact_widgets (ui);
	supports_dirs = set_model_for_options_combobox (ui);
	g_signal_connect (G_OBJECT (ui->dialog), "destroy",
                          G_CALLBACK (destroy_dialog), NULL);
	g_signal_connect (G_OBJECT (ui->cancel_button), "clicked",
			  G_CALLBACK (destroy_dialog), NULL);
	g_signal_connect (G_OBJECT (ui->send_button), "clicked",
			  G_CALLBACK (send_button_cb), ui);
	g_signal_connect (G_OBJECT (ui->pack_entry), "activate",
			  G_CALLBACK (send_button_cb), ui);
	g_signal_connect (G_OBJECT (ui->pack_entry), "notify::text",
			  G_CALLBACK (pack_entry_changed_cb), ui);
	g_signal_connect (G_OBJECT (ui->pack_checkbutton), "toggled",
			  G_CALLBACK (toggle_pack_check), ui);

	if (has_dirs == FALSE || supports_dirs != FALSE) {
		gboolean toggle;

		toggle = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ui->pack_checkbutton));
		gtk_widget_set_sensitive (ui->pack_combobox, toggle);
		gtk_widget_set_sensitive (ui->pack_entry, toggle);
	} else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ui->pack_checkbutton), TRUE);
		gtk_widget_set_sensitive (ui->pack_checkbutton, FALSE);
	}

	gtk_widget_show (ui->dialog);

}

static void
nemo_sendto_plugin_dir_process (const char *plugindir)
{
	GDir *dir;
	const char *item;
	NstPlugin *p = NULL;
	gboolean (*nst_init_plugin)(NstPlugin *p);
	GError *err = NULL;

	dir = g_dir_open (plugindir, 0, &err);

	if (dir == NULL) {
		g_warning ("Can't open the plugins dir: %s", err ? err->message : "No reason");
		if (err)
			g_error_free (err);
	} else {
		while ((item = g_dir_read_name(dir))) {
			if (g_str_has_suffix (item, SOEXT)) {
				char *module_path;

				p = g_new0(NstPlugin, 1);
				module_path = g_module_build_path (plugindir, item);
				p->module = g_module_open (module_path, 0);
			        if (!p->module) {
                			g_warning ("error opening %s: %s", module_path, g_module_error ());
					g_free (module_path);
					continue;
				}
				g_free (module_path);

				if (!g_module_symbol (p->module, "nst_init_plugin", (gpointer *) &nst_init_plugin)) {
			                g_warning ("error: %s", g_module_error ());
					g_module_close (p->module);
					continue;
				}

				nst_init_plugin (p);
				if (p->info->init(p)) {
					plugin_list = g_list_append (plugin_list, p);
				} else {
					g_free (p);
				}
			}
		}
		g_dir_close (dir);
	}
}

static gboolean
nemo_sendto_plugin_init (void)
{
	if (g_file_test (UNINSTALLED_PLUGINDIR, G_FILE_TEST_IS_DIR) != FALSE) {
		/* Try to load the local plugins */
		GError *err = NULL;
		GDir *dir;
		const char *item;

		dir = g_dir_open ("plugins/", 0, &err);
		if (dir == NULL) {
			g_warning ("Can't open the plugins dir: %s", err ? err->message : "No reason");
			if (err)
				g_error_free (err);
			return FALSE;
		}
		while ((item = g_dir_read_name(dir))) {
			char *plugindir;

			plugindir = g_strdup_printf ("plugins/%s/.libs/", item);
			if (g_file_test (plugindir, G_FILE_TEST_IS_DIR) != FALSE)
				nemo_sendto_plugin_dir_process (plugindir);
			g_free (plugindir);
		}
		g_dir_close (dir);
	}

	if (g_list_length (plugin_list) == 0)
		nemo_sendto_plugin_dir_process (PLUGINDIR);

	return g_list_length (plugin_list) != 0;
}

static char *
escape_ampersands_and_commas (const char *url)
{
	int i;
	char *str, *ptr;

	/* Count the number of ampersands & commas */
	i = 0;
	ptr = (char *) url;
	while ((ptr = strchr (ptr, '&')) != NULL) {
		i++;
		ptr++;
	}
	ptr = (char *) url;
	while ((ptr = strchr (ptr, ',')) != NULL) {
		i++;
		ptr++;
	}

	/* No ampersands or commas ? */
	if (i == 0)
		return NULL;

	/* Replace the '&' */
	str = g_malloc0 (strlen (url) - i + 3 * i + 1);
	ptr = str;
	for (i = 0; url[i] != '\0'; i++) {
		if (url[i] == '&') {
			*ptr++ = '%';
			*ptr++ = '2';
			*ptr++ = '6';
		} else if (url[i] == ',') {
			*ptr++ = '%';
			*ptr++ = '2';
			*ptr++ = 'C';
		} else {
			*ptr++ = url[i];
		}
	}

	return str;
}

static void
nemo_sendto_init (void)
{
	int i;

	if (g_module_supported() == FALSE)
		g_error ("Could not initialize gmodule support");

	for (i = 0; filenames != NULL && filenames[i] != NULL; i++) {
		GFile *file;
		char *filename, *escaped, *uri;

		file = g_file_new_for_commandline_arg (filenames[i]);
		filename = g_file_get_path (file);
		if (filename == NULL)
			continue;

		if (g_file_test (filename, G_FILE_TEST_IS_DIR) != FALSE)
			has_dirs = TRUE;

		uri = g_filename_to_uri (filename, NULL, NULL);
		g_free (filename);
		escaped = escape_ampersands_and_commas (uri);

		if (escaped == NULL) {
			file_list = g_list_prepend (file_list, uri);
		} else {
			file_list = g_list_prepend (file_list, escaped);
			g_free (uri);
		}
	}

	if (file_list == NULL) {
		g_print (_("Expects URIs or filenames to be passed as options\n"));
		exit (1);
	}

	file_list = g_list_reverse (file_list);
}

int main (int argc, char **argv)
{
	GOptionContext *context;
	GError *error = NULL;

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	g_thread_init (NULL);
	context = g_option_context_new ("nemo-sendto");
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		g_print (_("Could not parse command-line options: %s\n"), error->message);
		g_error_free (error);
		return 1;
	}

	settings = g_settings_new ("org.nemo.Sendto");
	nemo_sendto_init ();
	if (nemo_sendto_plugin_init () == FALSE) {
		GtkWidget *error_dialog;

		error_dialog =
			gtk_message_dialog_new (NULL,
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_OK,
						_("Could not load any plugins."));
		gtk_message_dialog_format_secondary_text
			(GTK_MESSAGE_DIALOG (error_dialog),
			 _("Please verify your installation"));

		gtk_window_set_title (GTK_WINDOW (error_dialog), ""); /* as per HIG */
		gtk_container_set_border_width (GTK_CONTAINER (error_dialog), 5);
		gtk_dialog_set_default_response (GTK_DIALOG (error_dialog),
						 GTK_RESPONSE_OK);
		gtk_dialog_run (GTK_DIALOG (error_dialog));
		return 1;
	}
	nemo_sendto_create_ui ();

	gtk_main ();
	g_object_unref(settings);

	return 0;
}

