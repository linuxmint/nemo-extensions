/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014 Victor A. Santos <victoraur.santos@gmail.com>
 * Copyright (C) 2014 David King <amigadave@amigadave.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n-lib.h>
#include <libnemo-extension/nemo-extension-types.h>
#include <libnemo-extension/nemo-file-info.h>
#include <libnemo-extension/nemo-menu-provider.h>

#define NEMO_TYPE_EASYTAG (nemo_easytag_get_type ())
#define NEMO_EASYTAG(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), NEMO_TYPE_EASYTAG, NemoEasytag))
#define NEMO_IS_EASYTAG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), NEMO_TYPE_EASYTAG))

typedef struct _NemoEasytag NemoEasytag;
typedef struct _NemoEasytagClass NemoEasytagClass;

struct _NemoEasytag
{
    GObject parent;
};

struct _NemoEasytagClass
{
    GObjectClass parent_class;
};

GType nemo_easytag_get_type (void);

typedef struct
{
    gboolean is_directory;
    gboolean is_file;
} FileMimeInfo;

static struct
{
    const gchar *mime_type;
    gboolean is_directory;
    gboolean is_file;
} easytag_mime_types[] =
{
    { "inode/directory", TRUE, FALSE },
    { "audio/x-mp3", FALSE, TRUE },
    { "audio/x-mpeg", FALSE, TRUE },
    { "audio/mpeg", FALSE, TRUE },
    { "application/ogg", FALSE, TRUE },
    { "audio/x-vorbis+ogg", FALSE, TRUE },
    { "audio/x-flac", FALSE, TRUE },
    { "audio/x-musepack", FALSE, TRUE },
    { "audio/x-ape", FALSE, TRUE },
    { "audio/x-speex+ogg", FALSE, TRUE },
    { "audio/x-opus+ogg", FALSE, TRUE },
    { NULL, FALSE }
};

static gboolean
unsupported_scheme (NemoFileInfo *file)
{
    gboolean result = FALSE;
    GFile *location;
    gchar *scheme;

    location = nemo_file_info_get_location (file);
    scheme = g_file_get_uri_scheme (location);

    if (scheme != NULL)
    {
        const gchar *unsupported[] = { "trash", "computer", NULL };
        gsize i;

        for (i = 0; unsupported[i] != NULL; i++)
        {
            if (strcmp (scheme, unsupported[i]) == 0)
            {
                result = TRUE;
            }
        }
    }

    g_free (scheme);
    g_object_unref (location);

    return result;
}

static FileMimeInfo
get_file_mime_info (NemoFileInfo *file)
{
    FileMimeInfo file_mime_info;
    gsize i;

    file_mime_info.is_directory = FALSE;
    file_mime_info.is_file = FALSE;

    for (i = 0; easytag_mime_types[i].mime_type != NULL; i++)
    {
        if (nemo_file_info_is_mime_type (file,
                                             easytag_mime_types[i].mime_type))
        {
            file_mime_info.is_directory = easytag_mime_types[i].is_directory;
            file_mime_info.is_file = easytag_mime_types[i].is_file;

            return file_mime_info;
        }
    }

    return file_mime_info;
}

static void
on_open_in_easytag (NemoMenuItem *item,
                    gpointer data)
{
    NemoFileInfo *dir;
    GDesktopAppInfo *appinfo;

    dir = g_object_get_data (G_OBJECT (item), "dir");

    appinfo = g_desktop_app_info_new ("easytag.desktop");

    if (appinfo)
    {
        GdkAppLaunchContext *context;
        GList *uris = NULL;

        uris = g_list_append (uris, nemo_file_info_get_uri (dir));
        context = gdk_display_get_app_launch_context (gdk_display_get_default ());

        g_app_info_launch_uris (G_APP_INFO (appinfo), uris,
                                G_APP_LAUNCH_CONTEXT (context), NULL);
    }
}

static void
on_open_with_easytag (NemoMenuItem *item,
                      gpointer data)
{
    GList *files;
    GDesktopAppInfo *appinfo;

    files = g_object_get_data (G_OBJECT (item), "files");

    appinfo = g_desktop_app_info_new ("easytag.desktop");

    if (appinfo)
    {
        GdkAppLaunchContext *context;
        GList *l;
        GList *uris = NULL;

        for (l = files; l != NULL; l = g_list_next (l))
        {
            uris = g_list_append (uris,
                                  nemo_file_info_get_uri (l->data));
        }

        context = gdk_display_get_app_launch_context (gdk_display_get_default ());

        g_app_info_launch_uris (G_APP_INFO (appinfo), uris,
                                G_APP_LAUNCH_CONTEXT (context), NULL);
    }
}

static GList *
nemo_easytag_get_file_items (NemoMenuProvider *provider,
                                 GtkWidget *window,
                                 GList *files)
{
    GList *items = NULL;
    GList *l;
    gboolean one_item;
    gboolean one_directory = TRUE;
    gboolean all_files = TRUE;

    if (files == NULL)
    {
        return NULL;
    }

    if (unsupported_scheme ((NemoFileInfo *)files->data))
    {
        return NULL;
    }

    for (l = files; l != NULL; l = g_list_next (l))
    {
        FileMimeInfo file_mime_info;
        NemoFileInfo *file = l->data;

        file_mime_info = get_file_mime_info (file);

        if (all_files && !file_mime_info.is_file)
        {
            all_files = FALSE;
        }

        if (one_directory && !file_mime_info.is_directory)
        {
            one_directory = FALSE;
        }
    }

    one_item = (files != NULL) && (files->next == NULL);

    if (one_directory && one_item)
    {
        NemoMenuItem *item;

        item = nemo_menu_item_new ("NemoEasytag::open_directory",
                                       _("Open in EasyTAG"),
                                       _("Open the selected directory in EasyTAG"),
                                       "easytag");
        g_signal_connect (item,
                          "activate",
                          G_CALLBACK (on_open_in_easytag),
                          provider);
        g_object_set_data (G_OBJECT (item),
                           "dir",
                           files->data);

        items = g_list_append (items, item);
    }
    else if (all_files)
    {
        NemoMenuItem *item;

        item = nemo_menu_item_new ("NemoEasytag::open_files",
                                       _("Open with EasyTAG"),
                                       _("Open the selected files in EasyTAG"),
                                       "easytag");
        g_signal_connect (item,
                          "activate",
                          G_CALLBACK (on_open_with_easytag),
                          provider);
        g_object_set_data_full (G_OBJECT (item),
                                "files",
                                nemo_file_info_list_copy (files),
                                (GDestroyNotify) nemo_file_info_list_free);

        items = g_list_append (items, item);
    }

    return items;
}

static void
nemo_easytag_menu_provider_iface_init (NemoMenuProviderIface *iface)
{
    iface->get_file_items = nemo_easytag_get_file_items;
}

G_DEFINE_DYNAMIC_TYPE_EXTENDED (NemoEasytag,
                                nemo_easytag,
                                G_TYPE_OBJECT,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (NEMO_TYPE_MENU_PROVIDER,
                                                               nemo_easytag_menu_provider_iface_init));

static void
nemo_easytag_init (NemoEasytag *self)
{
}

static void
nemo_easytag_class_init (NemoEasytagClass *class)
{
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
}

static void
nemo_easytag_class_finalize (NemoEasytagClass *class)
{
}

/* Nemo extension module. */
static GType type_list[1];

void
nemo_module_initialize (GTypeModule *module)
{
    nemo_easytag_register_type (module);

    type_list[0] = NEMO_TYPE_EASYTAG;
}

void
nemo_module_shutdown (void)
{
}

void
nemo_module_list_types (const GType **types,
                            int *num_types)
{
    *types = type_list;
    *num_types = G_N_ELEMENTS (type_list);
}
