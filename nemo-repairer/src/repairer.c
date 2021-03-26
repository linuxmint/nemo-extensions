/*
 * Nemo Filename Repairer Extension
 *
 * Copyright (C) 2011 Choe Hwanjin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Author: Choe Hwajin <choe.hwanjin@gmail.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "nemo-filename-repairer-i18n.h"
#include "repair-dialog.h"

int main(int argc, char** argv)
{
    int i;
    GtkDialog* dialog;
    GSList* files;
    gint res;

#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
#endif

    gtk_init(&argc, &argv);

    files = NULL;
    if (argc == 1) {
	dialog = GTK_DIALOG(gtk_file_chooser_dialog_new(
			_("Nemo Filename Repairer: Select Files To Rename"),
			NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			_("_Cancel"), GTK_RESPONSE_CANCEL,
			_("_Open"), GTK_RESPONSE_ACCEPT,
			NULL));
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	res = gtk_dialog_run(dialog);
	if (res == GTK_RESPONSE_ACCEPT) {
	    files = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
    } else {
	for (i = 1; i < argc; i++) {
	    GFile* file = g_file_new_for_path(argv[i]);
	    files = g_slist_prepend(files, file);
	}
	files = g_slist_reverse(files);
    }

    if (files == NULL)
	return 0;

    dialog = repair_dialog_new(files);
    res = gtk_dialog_run(dialog);
    gtk_widget_hide(GTK_WIDGET(dialog));

    if (res == GTK_RESPONSE_APPLY) {
	repair_dialog_do_repair(dialog);
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));

    g_slist_foreach(files, (GFunc)g_object_unref, NULL);
    g_slist_free(files);

    return 0;
}
