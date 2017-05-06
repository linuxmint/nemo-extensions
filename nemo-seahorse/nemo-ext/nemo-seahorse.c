/*
 *  Seahorse
 *
 *  Copyright (C) 2005 Tecsidel S.A.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Suite 500, MA 02110-1335, USA.
 *
 *  Authors: Fernando Herrera <fernando.herrera@tecsidel.es>
 *           Stef Walter <stef@memberwebs.com>
 */

#include <config.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <libnemo-extension/nemo-extension-types.h>
#include <libnemo-extension/nemo-file-info.h>
#include <libnemo-extension/nemo-menu-provider.h>
#include <libnemo-extension/nemo-name-and-desc-provider.h>
#include "nemo-seahorse.h"

static GObjectClass *parent_class;

static void
crypt_callback (NemoMenuItem *item, gpointer user_data)
{
    GList *files, *scan;
    char *uri, *t;
    GString *cmd;

    files = g_object_get_data (G_OBJECT (item), "files");
    g_assert (files != NULL);

    cmd = g_string_new ("seahorse-tool-nemo");
    g_string_append_printf (cmd, " --encrypt");

    for (scan = files; scan; scan = scan->next) {
        uri = nemo_file_info_get_uri ((NemoFileInfo*)scan->data);
        t = g_shell_quote (uri);
        g_free (uri);

        g_string_append_printf (cmd, " %s", t);
        g_free (t);
    }

    g_print ("EXEC: %s\n", cmd->str);
    g_spawn_command_line_async (cmd->str, NULL);

    g_string_free (cmd, TRUE);
}

static void
sign_callback (NemoMenuItem *item, gpointer user_data)
{
    GList *files, *scan;
    char *uri, *t;
    GString *cmd;

    cmd = g_string_new ("seahorse-tool-nemo");
    g_string_append_printf (cmd, " --sign");
    files = g_object_get_data (G_OBJECT (item), "files");
    g_assert (files != NULL);

    for (scan = files; scan; scan = scan->next) {
        uri = nemo_file_info_get_uri ((NemoFileInfo*)scan->data);
        t = g_shell_quote (uri);
        g_free (uri);

        g_string_append_printf (cmd, " %s", t);
        g_free (t);
    }

    g_print ("EXEC: %s\n", cmd->str);
    g_spawn_command_line_async (cmd->str, NULL);

    g_string_free (cmd, TRUE);
}

static char *pgp_encrypted_types[] = {
    "application/pgp",
    "application/pgp-encrypted",
    NULL
};

static char *no_display_types[] = {
    "application/x-desktop",
    NULL
};

static gboolean
is_mime_types (NemoFileInfo *file, char* types[])
{
    int i;

    for (i = 0; types[i] != NULL; i++)
        if (nemo_file_info_is_mime_type (file, types[i]))
            return TRUE;

    return FALSE;
}

static gboolean
is_all_mime_types (GList *files, char* types[])
{
    while (files) {
        if (!is_mime_types ((NemoFileInfo*)(files->data), types))
            return FALSE;
        files = g_list_next (files);
    }

    return TRUE;
}

static GList*
seahorse_nemo_get_file_items (NemoMenuProvider *provider,
                                  GtkWidget *window, GList *files)
{
    NemoMenuItem *item;
    GList *scan, *items = NULL;
    gboolean xnaut;
    guint num;
    gchar *uri;

    num = g_list_length (files);

    /* No files */
    if (num == 0)
        return NULL;

    /* Check if it's a desktop file. Copied from nemo-send extension */
    for (scan = files; scan; scan = scan->next) {
        uri = nemo_file_info_get_uri ((NemoFileInfo*)(scan->data));
        xnaut = (uri != NULL && g_ascii_strncasecmp (uri, "x-nemo-desktop", 18) == 0);
        g_free (uri);
        if (xnaut)
            return NULL;
    }

    /* A single encrypted file, no menu items */
    if (num == 1 &&
        is_mime_types ((NemoFileInfo*)files->data, pgp_encrypted_types))
        return NULL;

    /* All 'no display' types, no menu items */
    if (is_all_mime_types (files, no_display_types))
        return NULL;

    item = nemo_menu_item_new ("NemoSh::crypt", _("Encrypt..."),
        ngettext ("Encrypt (and optionally sign) the selected file", "Encrypt the selected files", num), NULL);
    g_object_set_data_full (G_OBJECT (item), "files", nemo_file_info_list_copy (files),
                                 (GDestroyNotify) nemo_file_info_list_free);
    g_signal_connect (item, "activate", G_CALLBACK (crypt_callback), provider);
    items = g_list_append (items, item);

    item = nemo_menu_item_new ("NemoSh::sign", _("Sign"),
        ngettext ("Sign the selected file", "Sign the selected files", num), NULL);
    g_object_set_data_full (G_OBJECT (item), "files", nemo_file_info_list_copy (files),
                                (GDestroyNotify) nemo_file_info_list_free);
    g_signal_connect (item, "activate", G_CALLBACK (sign_callback), provider);

    items = g_list_append (items, item);
    return items;
}

static GList *
seahorse_nemo_get_name_and_desc (NemoNameAndDescProvider *provider)
{
    GList *ret = NULL;

    ret = g_list_append (ret, ("Nemo Seahorse:::Allows encryption and decryption of OpenPGP files from the context menu"));

    return ret;
}

static void
seahorse_nemo_menu_provider_iface_init (NemoMenuProviderIface *iface)
{
    iface->get_file_items = seahorse_nemo_get_file_items;
}

static void
seahorse_nemo_nd_provider_iface_init (NemoNameAndDescProviderIface *iface)
{
    iface->get_name_and_desc = seahorse_nemo_get_name_and_desc;
}

static void
seahorse_nemo_instance_init (SeahorseNemo *sh)
{

}

static void
seahorse_nemo_class_init (SeahorseNemoClass *klass)
{
    parent_class = g_type_class_peek_parent (klass);
}

static GType seahorse_nemo_type = 0;

GType
seahorse_nemo_get_type (void)
{
    return seahorse_nemo_type;
}

void
seahorse_nemo_register_type (GTypeModule *module)
{
    static const GTypeInfo info = {
        sizeof (SeahorseNemoClass), (GBaseInitFunc)NULL,
        (GBaseFinalizeFunc)NULL, (GClassInitFunc)seahorse_nemo_class_init,
        NULL, NULL, sizeof(SeahorseNemo), 0,
        (GInstanceInitFunc)seahorse_nemo_instance_init,
    };

    static const GInterfaceInfo menu_provider_iface_info = {
        (GInterfaceInitFunc)seahorse_nemo_menu_provider_iface_init,
        NULL, NULL
    };

    static const GInterfaceInfo nd_provider_iface_info = {
        (GInterfaceInitFunc) seahorse_nemo_nd_provider_iface_init,
        NULL,NULL
    };


    seahorse_nemo_type = g_type_module_register_type (module,
                    G_TYPE_OBJECT, "SeahorseNemo", &info, 0);

    g_type_module_add_interface (module, seahorse_nemo_type,
                    NEMO_TYPE_MENU_PROVIDER, &menu_provider_iface_info);

    g_type_module_add_interface (module, seahorse_nemo_type,
                    NEMO_TYPE_NAME_AND_DESC_PROVIDER, &nd_provider_iface_info);
}
