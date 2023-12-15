/*
 * Nemo Filename Repairer Extension
 *
 * Copyright (C) 2008 Choe Hwanjin
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

#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include <libnemo-extension/nemo-menu-provider.h>
#include <libnemo-extension/nemo-name-and-desc-provider.h>

#include "nemo-filename-repairer.h"

static GType filename_repairer_type = 0;

// from http://www.microsoft.com/globaldev/reference/wincp.mspx
// Code Pages Supported by Windows
static const char* encoding_list[] = {
    "CP1252",  // Latin I  (default encoding)
    "CP1250",  // Central Europe
    "CP1251",  // Cyrillic
    "CP1253",  // Greek
    "CP1254",  // Turkish
    "CP1255",  // Hebrew
    "CP1256",  // Arabic
    "CP1257",  // Baltic
    "CP1258",  // Vietnam
    "CP874",   // Thai
    "CP932",   // Japanese Shift-JIS
    "CP936",   // Simplified Chinese GBK
    "CP949",   // Korean
    "CP950",   // Traditional Chinese Big5
    NULL
};

struct encoding_item {
    const char* locale;
    const char* encoding;
};

static const struct encoding_item default_encoding_list[] = {
    { "ar",    "CP1256"  },
    { "az",    "CP1251"  },
    { "az",    "CP1254"  },
    { "be",    "CP1251"  },
    { "bg",    "CP1251"  },
    { "cs",    "CP1250"  },
    { "cy",    "CP1253"  },
    { "el",    "CP1253"  },
    { "et",    "CP1257"  },
    { "fa",    "CP1256"  },
    { "he",    "CP1255"  },
    { "hr",    "CP1250"  },
    { "hu",    "CP1250"  },
    { "ja",    "CP932"   },
    { "kk",    "CP1251"  },
    { "ko",    "CP949"   },
    { "ky",    "CP1251"  },
    { "lt",    "CP1257"  },
    { "lv",    "CP1257"  },
    { "mk",    "CP1251"  },
    { "mn",    "CP1251"  },
    { "pl",    "CP1250"  },
    { "ro",    "CP1250"  },
    { "ru",    "CP1251"  },
    { "sk",    "CP1250"  },
    { "sl",    "CP1250"  },
    { "sq",    "CP1250"  },
    { "sr",    "CP1250"  },
    { "sr",    "CP1251"  },
    { "th",    "CP874"   },
    { "tr",    "CP1254"  },
    { "tt",    "CP1251"  },
    { "uk",    "CP1251"  },
    { "ur",    "CP1256"  },
    { "uz",    "CP1251"  },
    { "uz",    "CP1254"  },
    { "vi",    "CP1258"  },
    { "zh",    "CP936"   },
    { "zh",    "CP950"   },
    { NULL,    NULL      }
};

static void
show_error_message(GtkWidget* parent, const char* filename, GError* error)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(parent),
		GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL, 
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_CLOSE,
		_("<span size=\"larger\" weight=\"bold\">There was an error renaming the file to \"%s\"</span>"),
		filename);
    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog),
		"%s",
		error->message);
    gtk_dialog_run(GTK_DIALOG (dialog));
    gtk_widget_destroy(dialog);
}

static void
on_rename_menu_item_activated(NemoMenuItem *item, gpointer *data)
{
    gboolean res;
    gchar* new_name;
    GFile* file;
    GFile* new_file;
    GFile* parent;
    GtkWidget* window;
    GError* error = NULL;

    new_name = g_object_get_data(G_OBJECT(item), "Repairer::new_name");
    file = g_object_get_data(G_OBJECT(item), "Repairer::file");
    window = g_object_get_data(G_OBJECT(item), "Repairer::window");

    parent = g_file_get_parent(file);
    new_file = g_file_get_child(parent, new_name);

    res = g_file_move(file, new_file, G_FILE_COPY_NOFOLLOW_SYMLINKS,
		NULL, NULL, NULL, &error);
    if (!res) {
	if (error->code == G_IO_ERROR_EXISTS) {
	    GtkWidget *dialog;

	    dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(window),
			GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL, 
			GTK_MESSAGE_WARNING,
			GTK_BUTTONS_CLOSE,
			_("<span size=\"larger\" weight=\"bold\">A file named \"%s\" already exists.</span>"),
			new_name);
	    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog),
		    _("If you want to rename the selected file, "
		      "please move or rename \"%s\" first."),
		    new_name);
	    gtk_dialog_run(GTK_DIALOG (dialog));
	    gtk_widget_destroy(dialog);
	} else {
	    show_error_message(window, new_name, error);
	}
	g_error_free(error);
    }

    g_object_unref(parent);
    g_object_unref(new_file);
}

static void
on_repair_dialog_activated(NemoMenuItem* item, gpointer data)
{
    GList* files;
    guint i;
    guint n;
    gchar** argv;
    GError *error = NULL;

    files = (GList*)g_object_get_data(G_OBJECT(item), "Repairer::files");

    n = g_list_length(files);
    argv = g_new(gchar*, n + 2);

    argv[0] = g_strdup("nemo-filename-repairer");

    i = 1;
    while (files != NULL) {
	GFile* file;

	file = nemo_file_info_get_location(files->data);
	argv[i] = g_file_get_path(file);
	g_object_unref(file);

	files = g_list_next(files);
	i++;
    }
    argv[i] = NULL;

    g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);
    g_strfreev(argv);
}

static NemoMenuItem*
repair_dialog_menu_item_new(GList* files)
{
    const char* name;
    const char* label;
    const char* tooltip;
    NemoMenuItem* item;

    name    = "Repairer::manual_rename";
    label   = _("Repair filename ...");
    tooltip = _("Repair filename");

    files = nemo_file_info_list_copy(files);

    item = nemo_menu_item_new(name, label, tooltip, NULL);
    g_object_set_data_full(G_OBJECT(item), "Repairer::files",
	    files, (GDestroyNotify) nemo_file_info_list_free);

    g_signal_connect(G_OBJECT(item), "activate",
		     G_CALLBACK(on_repair_dialog_activated), NULL);

    return item;
}

static char*
get_filename_without_mnemonic(const char* filename)
{
    GString* str;

    str = g_string_new(NULL);
    while (*filename != '\0') {
	if (*filename == '_')
	    g_string_append(str, "__");
	else
	    g_string_append_c(str, *filename);
	filename++;
    }

    return g_string_free(str, FALSE);
}

static NemoMenuItem*
rename_menu_item_new(const char* name, GFile* file, int menu_index,
	GtkWidget* window, gboolean is_submenu)
{
    gchar id[128];
    gchar* filename;
    gchar* label;
    gchar* tooltip;
    NemoMenuItem* item;

    filename = get_filename_without_mnemonic(name);
    
    g_snprintf(id, sizeof(id), "Repairer::rename_as_%d", menu_index);
    if (is_submenu)
	label   = g_strdup(filename);
    else
	label   = g_strdup_printf(_("Re_name as \"%s\""), filename);
    tooltip = g_strdup_printf(_("Rename as \"%s\"."), name);

    g_object_ref(file);

    item = nemo_menu_item_new(id, label, tooltip, NULL);
    g_object_set_data(G_OBJECT(item), "Repairer::window", window);
    g_object_set_data_full(G_OBJECT(item), "Repairer::file",
			    file, g_object_unref);
    g_object_set_data_full(G_OBJECT(item), "Repairer::new_name",
			    g_strdup(name), g_free);
    g_signal_connect(G_OBJECT(item), "activate",
		     G_CALLBACK(on_rename_menu_item_activated), NULL);

    g_free(filename);
    g_free(label);
    g_free(tooltip);

    return item;
}

static GList*
append_uri_item(GList* menu, const char* name,  const char* new_name,
	GFile* file, GtkWidget* window)
{
    NemoMenuItem* item;
    int menu_index;

    if (strcmp(name, new_name) != 0) {
	menu_index = g_list_length(menu);
	item = rename_menu_item_new(new_name, file, menu_index, window, FALSE);
	menu = g_list_append(menu, item);
    }

    return menu;
}

static GList*
append_unicode_nfc_item(GList* menu, const char* name,
		GFile* file, GtkWidget* window)
{
    NemoMenuItem* item;
    char* nfc;
    int menu_index;

    /* test for MacOSX filename which is NFD */
    nfc = g_utf8_normalize(name, -1, G_NORMALIZE_NFC);
    if (nfc != NULL) {
	if (strcmp(name, nfc) != 0) {
	    menu_index = g_list_length(menu);
	    item = rename_menu_item_new(nfc, file, menu_index, window, FALSE);
	    menu = g_list_append(menu, item);
	}
	g_free(nfc);
    }

    return menu;
}

static GList*
append_default_encoding_items(GList* menu,
	    const char* name, GFile* file, GtkWidget* window)
{
    char* locale;
    const struct encoding_item* e;
    NemoMenuItem* item;
    int menu_index;

    locale = setlocale(LC_CTYPE, NULL);
    if (locale == NULL)
	return menu;

    menu_index = g_list_length(menu);
    for (e = default_encoding_list; e->locale != NULL; e++) {
	size_t len = strlen(e->locale);
	if (strncmp(e->locale, locale, len) == 0) {
	    gchar* new_name;
	    new_name = g_convert(name, -1,
			    "UTF-8", e->encoding, NULL, NULL, NULL);
	    if (new_name == NULL)
		continue;

	    if (strcmp(name, new_name) != 0) {
		item = rename_menu_item_new(new_name, file,
				menu_index, window, FALSE);
		menu = g_list_append(menu, item);
		menu_index++;
	    }

	    g_free(new_name);
	}
    }

    return menu;
}

static GList*
append_other_encoding_items(GList* menu,
	     const char* name, GFile* file, GtkWidget* window)
{
    NemoMenu* submenu;
    NemoMenuItem* item;
    GTree* new_name_table;
    gchar* new_name;
    int i;
    int menu_index;
    gpointer have_item;
    
    submenu = NULL;
    menu_index = g_list_length(menu);
    new_name_table = g_tree_new_full((GCompareDataFunc)strcmp,
			     NULL, g_free, NULL);
    for (i = 0; encoding_list[i] != NULL; i++) {
	new_name = g_convert(name, -1, "UTF-8", encoding_list[i],
			NULL, NULL, NULL);
	if (new_name == NULL)
	    continue;

	if (strcmp(name, new_name) == 0) {
	    g_free(new_name);
	    continue;
	}

	have_item = g_tree_lookup(new_name_table, new_name);
	if (have_item == NULL) {
	    if (submenu == NULL)
		submenu = nemo_menu_new();

	    item = rename_menu_item_new(new_name, file,
			    menu_index, window, TRUE);
	    nemo_menu_append_item(submenu, item);
	    g_tree_insert(new_name_table, new_name, new_name);
	    
	    menu_index++;
	} else {
	    g_free(new_name);
	}
    }

    g_tree_destroy(new_name_table);

    if (submenu != NULL) {
	const char* menu_id;
	menu_id = "Repairer::rename_as_submenu";
	item = nemo_menu_item_new(menu_id,
			      _("Select a filename"),
			      _("Select a filename from sub menu items."),
			      NULL);

	nemo_menu_item_set_submenu(item, submenu);
	menu = g_list_append(menu, item);
    }

    return menu;
}

static GList*
append_repair_menu_items(GList* menu, GtkWidget *window, GList *files)
{
    GFile* file;
    gboolean is_native;
    gchar* name;
    gchar* unescaped;
    gchar* reconverted;

    if (files == NULL)
	return menu;

    if (files->next != NULL)
	return menu;

    file = nemo_file_info_get_location(files->data);
    if (file == NULL)
	return menu;

    is_native = g_file_is_native(file);
    if (!is_native)
	return menu;

    name = g_file_get_basename(file);
    if (name == NULL)
	return menu;

    /* test for URI encoded filenames */
    unescaped = g_uri_unescape_string(name, NULL);
    if (unescaped != NULL) {
	if (g_utf8_validate(unescaped, -1, NULL)) {
	    menu = append_uri_item(menu, name, unescaped, file, window);
	    menu = append_unicode_nfc_item(menu, unescaped, file, window);
	}
	g_free(name);
	name = unescaped;
    }

    if (g_utf8_validate(name, -1, NULL)) {
	reconverted = g_convert(name, -1, "CP1252", "UTF-8", NULL, NULL, NULL);
	if (reconverted != NULL) {
	    menu = append_default_encoding_items(menu, reconverted, file, window);
	    menu = append_other_encoding_items(menu, reconverted, file, window);
	    g_free(reconverted);
	}
    } else {
	menu = append_default_encoding_items(menu, name, file, window);
	menu = append_other_encoding_items(menu, name, file, window);
    }

    g_free(name);
    g_object_unref(file);

    return menu;
}

static gboolean
need_repair_dialog(GList* files)
{
    char* name;
    gboolean res;

    while (files != NULL) {
	if (nemo_file_info_is_directory(files->data))
	    return TRUE;

	name = nemo_file_info_get_name(files->data);
	res = g_utf8_validate(name, -1, NULL);
	g_free(name);
	if (!res)
	    return TRUE;

	files = g_list_next(files);
    }
    
    return FALSE;
}

static GList *
nemo_filename_repairer_get_file_items(NemoMenuProvider *provider,
					  GtkWidget            *window,
					  GList                *files)
{
    NemoMenuItem* item;
    GList* menu;

    menu = NULL;
    menu = append_repair_menu_items(menu, window, files);

    if (need_repair_dialog(files)) {
	item = repair_dialog_menu_item_new(files);
	menu = g_list_append(menu, item);
    }

    return menu;
}

static GList *
nemo_filename_repairer_get_name_and_desc (NemoNameAndDescProvider *provider)
{
    GList *ret = NULL;
    gchar *string = g_strdup_printf ("nemo-filename-repairer:::%s",
      _("Allows filename encoding repair from the context menu"));
    ret = g_list_append (ret, (string));

    return ret;
}

static void
nemo_filename_repairer_menu_provider_iface_init(NemoMenuProviderIface *iface)
{
    iface->get_file_items = nemo_filename_repairer_get_file_items;
}

static void
nemo_filename_repairer_nd_provider_iface_init (NemoNameAndDescProviderIface *iface)
{
    iface->get_name_and_desc = nemo_filename_repairer_get_name_and_desc;
}

static void 
nemo_filename_repairer_instance_init(NemoFilenameRepairer *instance)
{
}

static void
nemo_filename_repairer_class_init(NemoFilenameRepairerClass *klass)
{
}

GType
nemo_filename_repairer_get_type(void) 
{
    return filename_repairer_type;
}

void
nemo_filename_repairer_register_type(GTypeModule *module)
{
    static const GTypeInfo info = {
	sizeof(NemoFilenameRepairerClass),
	(GBaseInitFunc)NULL,
	(GBaseFinalizeFunc)NULL,
	(GClassInitFunc)nemo_filename_repairer_class_init,
	NULL, 
	NULL,
	sizeof(NemoFilenameRepairer),
	0,
	(GInstanceInitFunc)nemo_filename_repairer_instance_init,
    };

    static const GInterfaceInfo menu_provider_iface_info = {
	(GInterfaceInitFunc)nemo_filename_repairer_menu_provider_iface_init,
	NULL,
	NULL
    };

    static const GInterfaceInfo nd_provider_iface_info = {
        (GInterfaceInitFunc) nemo_filename_repairer_nd_provider_iface_init,
        NULL,
        NULL
    };


    filename_repairer_type = g_type_module_register_type(module,
						 G_TYPE_OBJECT,
						 "NemoFilenameRepairer",
						 &info, 0);

    g_type_module_add_interface(module,
				filename_repairer_type,
				NEMO_TYPE_MENU_PROVIDER,
				&menu_provider_iface_info);


    g_type_module_add_interface(module,
				filename_repairer_type,
				NEMO_TYPE_NAME_AND_DESC_PROVIDER,
				&nd_provider_iface_info);
}

void  nemo_filename_repairer_on_module_init(void)
{
}

void  nemo_filename_repairer_on_module_shutdown(void)
{
}
