/*
 *  File-Roller
 *
 *  Copyright (C) 2004 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Author: Paolo Bacchilega <paobac@cvs.gnome.org>
 *
 */

#include <config.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <libnemo-extension/nemo-extension-types.h>
#include <libnemo-extension/nemo-file-info.h>
#include <libnemo-extension/nemo-menu-provider.h>
#include <libnemo-extension/nemo-name-and-desc-provider.h>
#include "nemo-fileroller.h"


static GObjectClass *parent_class;
static gboolean always_show_extract_to = FALSE;

static void
extract_to_callback (NemoMenuItem *item,
		     gpointer          user_data)
{
	GList            *files;
	NemoFileInfo *file;
	char             *uri, *default_dir;
	char             *quoted_uri, *quoted_default_dir;
	GString          *cmd;

	files = g_object_get_data (G_OBJECT (item), "files");
	file = files->data;

	uri = nemo_file_info_get_uri (file);
	default_dir = nemo_file_info_get_parent_uri (file);

	quoted_uri = g_shell_quote (uri);
	quoted_default_dir = g_shell_quote (default_dir);

	cmd = g_string_new ("file-roller");
	g_string_append_printf (cmd,
				" --default-dir=%s --extract %s",
				quoted_default_dir,
				quoted_uri);

#ifdef DEBUG
	g_print ("EXEC: %s\n", cmd->str);
#endif

	g_spawn_command_line_async (cmd->str, NULL);

	g_string_free (cmd, TRUE);
	g_free (default_dir);
	g_free (uri);
	g_free (quoted_default_dir);
	g_free (quoted_uri);
}


static void
extract_here_callback (NemoMenuItem *item,
		       gpointer          user_data)
{
	GList            *files, *scan;
	GString          *cmd;

	files = g_object_get_data (G_OBJECT (item), "files");

	cmd = g_string_new ("file-roller --extract-here");

	for (scan = files; scan; scan = scan->next) {
		NemoFileInfo *file = scan->data;
		char             *uri;
		char             *quoted_uri;

		uri = nemo_file_info_get_uri (file);
		quoted_uri = g_shell_quote (uri);
		g_string_append_printf (cmd, " %s", quoted_uri);
		g_free (uri);
		g_free (quoted_uri);
	}

	g_spawn_command_line_async (cmd->str, NULL);

#ifdef DEBUG
	g_print ("EXEC: %s\n", cmd->str);
#endif

	g_string_free (cmd, TRUE);
}


static void
add_callback (NemoMenuItem *item,
	      gpointer          user_data)
{
	GList            *files, *scan;
	NemoFileInfo *file;
	char             *uri, *dir;
	char             *quoted_uri, *quoted_dir;
	GString          *cmd;

	files = g_object_get_data (G_OBJECT (item), "files");
	file = files->data;

	uri = nemo_file_info_get_uri (file);
	dir = g_path_get_dirname (uri);
	quoted_dir = g_shell_quote (dir);

	cmd = g_string_new ("file-roller");
	g_string_append_printf (cmd," --default-dir=%s --add", quoted_dir);

	g_free (uri);
	g_free (dir);
	g_free (quoted_dir);

	for (scan = files; scan; scan = scan->next) {
		NemoFileInfo *file = scan->data;

		uri = nemo_file_info_get_uri (file);
		quoted_uri = g_shell_quote (uri);
		g_string_append_printf (cmd, " %s", quoted_uri);
		g_free (uri);
		g_free (quoted_uri);
	}

	g_spawn_command_line_async (cmd->str, NULL);

	g_string_free (cmd, TRUE);
}


static struct {
	char     *mime_type;
	gboolean  is_compressed;
} archive_mime_types[] = {
		{ "application/x-7z-compressed", TRUE },
		{ "application/x-7z-compressed-tar", TRUE },
		{ "application/x-ace", TRUE },
		{ "application/x-alz", TRUE },
		{ "application/x-ar", TRUE },
		{ "application/x-arj", TRUE },
		{ "application/x-bzip", TRUE },
		{ "application/x-bzip-compressed-tar", TRUE },
		{ "application/x-bzip1", TRUE },
		{ "application/x-bzip1-compressed-tar", TRUE },
		{ "application/vnd.ms-cab-compressed", TRUE },
		{ "application/x-cbr", TRUE },
		{ "application/x-cbz", TRUE },
		{ "application/x-cd-image", FALSE },
		{ "application/x-compress", TRUE },
		{ "application/x-compressed-tar", TRUE },
		{ "application/x-cpio", TRUE },
		{ "application/x-deb", TRUE },
		{ "application/x-ear", TRUE },
		{ "application/x-ms-dos-executable", FALSE },
		{ "application/x-gtar", FALSE },
		{ "application/x-gzip", TRUE },
		{ "application/x-gzpostscript", TRUE },
		{ "application/x-java-archive", TRUE },
		{ "application/x-lha", TRUE },
		{ "application/x-lhz", TRUE },
		{ "application/x-lzip", TRUE },
		{ "application/x-lzip-compressed-tar", TRUE },
		{ "application/x-lzma", TRUE },
		{ "application/x-lzma-compressed-tar", TRUE },
		{ "application/x-lzop", TRUE },
		{ "application/x-lzop-compressed-tar", TRUE },
		{ "application/x-ms-wim", TRUE },
		{ "application/x-rar", TRUE },
		{ "application/x-rar-compressed", TRUE },
		{ "application/x-rpm", TRUE },
		{ "application/x-rzip", TRUE },
		{ "application/x-tar", FALSE },
		{ "application/x-tarz", TRUE },
		{ "application/x-stuffit", TRUE },
		{ "application/x-war", TRUE },
		{ "application/x-xz", TRUE },
		{ "application/x-xz-compressed-tar", TRUE },
		{ "application/x-zip", TRUE },
		{ "application/x-zip-compressed", TRUE },
		{ "application/x-zoo", TRUE },
		{ "application/x-zstd-compressed-tar", TRUE },
		{ "application/zip", TRUE },
		{ "application/zstd", TRUE },
		{ "multipart/x-zip", TRUE },
		{ NULL, FALSE }
};


typedef struct {
      gboolean is_archive;
      gboolean is_derived_archive;
      gboolean is_compressed_archive;
} FileMimeInfo;


static FileMimeInfo
get_file_mime_info (NemoFileInfo *file)
{
	FileMimeInfo file_mime_info;
	int          i;

	file_mime_info.is_archive = FALSE;
	file_mime_info.is_derived_archive = FALSE;
	file_mime_info.is_compressed_archive = FALSE;

	for (i = 0; archive_mime_types[i].mime_type != NULL; i++)
		if (nemo_file_info_is_mime_type (file, archive_mime_types[i].mime_type)) {
			char *mime_type;
			char *content_type_mime_file;
			char *content_type_mime_compare;

			mime_type = nemo_file_info_get_mime_type (file);

			content_type_mime_file = g_content_type_from_mime_type (mime_type);
			content_type_mime_compare = g_content_type_from_mime_type (archive_mime_types[i].mime_type);

			file_mime_info.is_archive = TRUE;
			file_mime_info.is_compressed_archive = archive_mime_types[i].is_compressed;
			if ((content_type_mime_file != NULL) && (content_type_mime_compare != NULL))
				file_mime_info.is_derived_archive = ! g_content_type_equals (content_type_mime_file, content_type_mime_compare);

			g_free (mime_type);
			g_free (content_type_mime_file);
			g_free (content_type_mime_compare);

			return file_mime_info;
		}

	return file_mime_info;
}


static gboolean
unsupported_scheme (NemoFileInfo *file)
{
	gboolean  result = FALSE;
	GFile    *location;
	char     *scheme;

	location = nemo_file_info_get_location (file);
	scheme = g_file_get_uri_scheme (location);

	if (scheme != NULL) {
		const char *unsupported[] = { "trash", "computer", NULL };
		int         i;

		for (i = 0; unsupported[i] != NULL; i++)
			if (strcmp (scheme, unsupported[i]) == 0)
				result = TRUE;
	}

	g_free (scheme);
	g_object_unref (location);

	return result;
}


static GList *
nemo_fr_get_file_items (NemoMenuProvider *provider,
			    GtkWidget            *window,
			    GList                *files)
{
	GList    *items = NULL;
	GList    *scan;
	gboolean  can_write = TRUE;
	gboolean  one_item;
	gboolean  one_archive = FALSE;
	gboolean  one_derived_archive = FALSE;
	gboolean  one_compressed_archive = FALSE;
	gboolean  all_archives = TRUE;
	gboolean  all_archives_derived = TRUE;
	gboolean  all_archives_compressed = TRUE;

	if (files == NULL)
		return NULL;

	if (unsupported_scheme ((NemoFileInfo *) files->data))
		return NULL;

	for (scan = files; scan; scan = scan->next) {
		NemoFileInfo *file = scan->data;
		FileMimeInfo      file_mime_info;

		file_mime_info = get_file_mime_info (file);

		if (all_archives && ! file_mime_info.is_archive)
			all_archives = FALSE;

		if (all_archives_compressed && file_mime_info.is_archive && ! file_mime_info.is_compressed_archive)
			all_archives_compressed = FALSE;

		if (all_archives_derived && file_mime_info.is_archive && ! file_mime_info.is_derived_archive)
			all_archives_derived = FALSE;

		if (can_write) {
			NemoFileInfo *parent;

			parent = nemo_file_info_get_parent_info (file);
			can_write = nemo_file_info_can_write (parent);
			g_object_unref (parent);
		}
	}

	/**/

	one_item = (files != NULL) && (files->next == NULL);
	one_archive = one_item && all_archives;
	one_derived_archive = one_archive && all_archives_derived;
	one_compressed_archive = one_archive && all_archives_compressed;

	if (all_archives && can_write) {
		NemoMenuItem *item;

		item = nemo_menu_item_new ("NemoFr::extract_here",
					       _("Extract Here"),
					       /* Translators: the current position is the current folder */
					       _("Extract the selected archive to the current position"),
					       "extract-archive-symbolic");
		g_signal_connect (item,
				  "activate",
				  G_CALLBACK (extract_here_callback),
				  provider);
		g_object_set_data_full (G_OBJECT (item),
					"files",
					nemo_file_info_list_copy (files),
					(GDestroyNotify) nemo_file_info_list_free);

		items = g_list_append (items, item);
	}
	if (all_archives &&
        (!can_write || always_show_extract_to)) {
		NemoMenuItem *item;

		item = nemo_menu_item_new ("NemoFr::extract_to",
					       _("Extract To..."),
					       _("Extract the selected archive"),
					       "extract-archive-symbolic");
		g_signal_connect (item,
				  "activate",
				  G_CALLBACK (extract_to_callback),
				  provider);
		g_object_set_data_full (G_OBJECT (item),
					"files",
					nemo_file_info_list_copy (files),
					(GDestroyNotify) nemo_file_info_list_free);

		items = g_list_append (items, item);

	}

	if (! one_compressed_archive || one_derived_archive) {
		NemoMenuItem *item;

		item = nemo_menu_item_new ("NemoFr::add",
					       _("Compress..."),
					       _("Create a compressed archive with the selected objects"),
					       "add-files-to-archive-symbolic");
		g_signal_connect (item,
				  "activate",
				  G_CALLBACK (add_callback),
				  provider);
		g_object_set_data_full (G_OBJECT (item),
					"files",
					nemo_file_info_list_copy (files),
					(GDestroyNotify) nemo_file_info_list_free);

		items = g_list_append (items, item);
	}

	return items;
}

static GList *
nemo_fr_get_name_and_desc (NemoNameAndDescProvider *provider)
{
    GList *ret = NULL;

    ret = g_list_append (ret, ("Nemo Fileroller:::Allows managing of archives from the context menu"));

    return ret;
}

static void
nemo_fr_menu_provider_iface_init (NemoMenuProviderIface *iface)
{
	iface->get_file_items = nemo_fr_get_file_items;
}

static void
nemo_fr_nd_provider_iface_init (NemoNameAndDescProviderIface *iface)
{
    iface->get_name_and_desc = nemo_fr_get_name_and_desc;
}

static void
nemo_fr_instance_init (NemoFr *fr)
{
}

static void
nemo_fr_class_init (NemoFrClass *class)
{
	parent_class = g_type_class_peek_parent (class);
}


static GType fr_type = 0;


GType
nemo_fr_get_type (void)
{
	return fr_type;
}


void
nemo_fr_register_type (GTypeModule *module)
{
	static const GTypeInfo info = {
		sizeof (NemoFrClass),
		(GBaseInitFunc) NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc) nemo_fr_class_init,
		NULL,
		NULL,
		sizeof (NemoFr),
		0,
		(GInstanceInitFunc) nemo_fr_instance_init,
	};

	static const GInterfaceInfo menu_provider_iface_info = {
		(GInterfaceInitFunc) nemo_fr_menu_provider_iface_init,
		NULL,
		NULL
	};

    static const GInterfaceInfo nd_provider_iface_info = {
        (GInterfaceInitFunc) nemo_fr_nd_provider_iface_init,
        NULL,
        NULL
    };

	fr_type = g_type_module_register_type (module,
					       G_TYPE_OBJECT,
					       "NemoFileRoller",
					       &info, 0);

	g_type_module_add_interface (module,
				     fr_type,
				     NEMO_TYPE_MENU_PROVIDER,
				     &menu_provider_iface_info);

    g_type_module_add_interface (module,
                     fr_type,
                     NEMO_TYPE_NAME_AND_DESC_PROVIDER,
                     &nd_provider_iface_info);
}
