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
#include "encoding-dialog.h"

#define ENCODINGS_DIALOG_UI PKGDATADIR "/encoding-dialog.ui"

#if GTK_CHECK_VERSION(3,0,0)
#define HAVE_GTK_COMBO_BOX_ENTRY 0
#else
#define HAVE_GTK_COMBO_BOX_ENTRY 1
#endif

enum {
    ENCODING_COLUMN_ENCODING,
    ENCODING_NUM_COLUMNS
};

static const char* encoding_list[] = {
    "ISO-8859-1",          /* Western */
    "ISO-8859-2",          /* Central European */
    "ISO-8859-3",          /* South European */
    "ISO-8859-4",          /* Baltic */
    "ISO-8859-5",          /* Cyrillic */
    "ISO-8859-6",          /* Arabic */
    "ISO-8859-7",          /* Greek */
    "ISO-8859-8",          /* Hebrew */
    "ISO-8859-9",          /* Turkish */
    "ISO-8859-10",         /* Nordic */
    "ISO-8859-11",         /* Thai */
    "ISO-8859-13",         /* Baltic */
    "ISO-8859-14",         /* Celtic */
    "ISO-8859-15",         /* Western */
    "ISO-8859-16",         /* Eastern European */

    "EUC-CN",              /* Chinese Simplified */
    "EUC-JP",              /* Japanese */
    "EUC-KR",              /* Korean */
    "EUC-TW",              /* Chinese Traditional */
};

static GtkListStore*
get_encoding_list_model()
{
    GtkListStore* store;
    GtkTreeIter iter;
    int i;
    int n;

    store = gtk_list_store_new(ENCODING_NUM_COLUMNS, G_TYPE_STRING); 

    i = 0;
    n = G_N_ELEMENTS(encoding_list);
    for (i = 0; i < n; i++) {
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
		ENCODING_COLUMN_ENCODING, encoding_list[i],
		-1);
    }

    return store;
}

static char*
encoding_dialog_get_encoding(GtkDialog* dialog)
{
    GtkComboBox* combobox;

    combobox = g_object_get_data(G_OBJECT(dialog), "encoding_entry");

#if HAVE_COMBO_BOX_ENTRY
    return gtk_combo_box_get_active_text(combobox);
#else
    {
	GtkWidget* entry;
	const char* text;

	entry = gtk_bin_get_child(GTK_BIN(combobox));
	text = gtk_entry_get_text(GTK_ENTRY(entry));
	return g_strdup(text);
    }
#endif
}

static GtkWidget*
combobox_entry_new(GtkTreeModel* model)
{
    GtkWidget* combobox;

#if HAVE_COMBO_BOX_ENTRY
    combobox = gtk_combo_box_entry_new_with_model(model,
	    ENCODING_COLUMN_ENCODING);
#else
    combobox = gtk_combo_box_new_with_model_and_entry(model);
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(combobox),
	    ENCODING_COLUMN_ENCODING);
#endif
    return combobox;
}

GtkDialog*
encoding_dialog_new(GtkWindow* parent)
{
    GtkBuilder* builder;
    GObject* object;
    GtkDialog* dialog;
    GtkWidget* combobox;
    GtkTreeModel* model;

    dialog = NULL;

    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, ENCODINGS_DIALOG_UI, NULL);
    gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);

    object = gtk_builder_get_object(builder, "encoding_dialog");
    if (object == NULL)
	goto done;

    dialog = GTK_DIALOG(object);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);

    model = GTK_TREE_MODEL(get_encoding_list_model());
    combobox = combobox_entry_new(model);
    g_object_set_data(G_OBJECT(dialog), "encoding_entry", combobox);

    object = gtk_builder_get_object(builder, "hbox1");
    if (object == NULL)
	goto done;

    gtk_box_pack_start(GTK_BOX(object), combobox, FALSE, TRUE, 0);
    gtk_widget_show(combobox);

    object = gtk_builder_get_object(builder, "accellabel1");
    if (object == NULL)
	goto done;

    gtk_accel_label_set_accel_widget(GTK_ACCEL_LABEL(object), combobox);

done:
    g_object_unref(builder);

    return dialog;
}

static gboolean
is_valid_encoding(const char* encoding)
{
    GIConv cd;

    if (encoding == NULL || encoding[0] == '\0')
	return FALSE;

    cd = g_iconv_open("UTF-8", encoding);
    if (cd == (GIConv)-1) {
        return FALSE;
    }
    g_iconv_close(cd);

    return TRUE;
}

void
encoding_dialog_show_error_message(GtkDialog* dialog, const char* encoding)
{
    GtkWidget* message;

    if (encoding == NULL || encoding[0] == '\0')
	return;

    message = gtk_message_dialog_new_with_markup(GTK_WINDOW(dialog),
	    GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
	    GTK_MESSAGE_ERROR,
	    GTK_BUTTONS_CLOSE,
	    _("<span size=\"larger\">The selected \"%s\" is invalid encoding.</span>\n"
	      "<span size=\"larger\">Please choose another one.</span>"),
	    encoding);
    gtk_dialog_run(GTK_DIALOG(message));
    gtk_widget_destroy(message);
}

char*
encoding_dialog_run(GtkDialog* dialog)
{
    char* encoding;
    gint response;

run:
    response = gtk_dialog_run(dialog);

    if (response == GTK_RESPONSE_OK) {
	encoding = encoding_dialog_get_encoding(dialog);
	if (is_valid_encoding(encoding)) {
	    return encoding;
	} else {
	    encoding_dialog_show_error_message(dialog, encoding);
	    g_free(encoding);
	    goto run;
	}
    }

    return NULL;
}
