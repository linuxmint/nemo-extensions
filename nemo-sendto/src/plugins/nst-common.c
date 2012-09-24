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
 * Author:  Maxim Ermilov <ermilov.maxim@gmail.com>
 */

#include <gio/gio.h>

static gboolean
copy_fobject (GFile* source, GFile* dst)
{
	GFileEnumerator* en;
	GFileInfo* info;
	GError *err = NULL;
	char *file_name;
	GFile *dest;

	file_name = g_file_get_basename (source);
	dest = g_file_get_child (dst, file_name);
	g_free (file_name);

	if (g_file_query_file_type (source, G_FILE_QUERY_INFO_NONE, NULL) != G_FILE_TYPE_DIRECTORY) {
		gboolean ret;
		ret = g_file_copy (source, dest, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL);

		g_object_unref (dest);

		return ret;
	}

	en = g_file_enumerate_children (source, "*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
	if (!g_file_make_directory (dest, NULL, NULL)) {
		g_object_unref (en);
		g_object_unref (dest);
		return FALSE;
	}

	while ((info = g_file_enumerator_next_file (en, NULL, &err)) != NULL) {
		const char *name;

		name = g_file_info_get_name (G_FILE_INFO (info));

		if (name != NULL) {
			GFile *child;

			child = g_file_get_child (source, name);

			if (!copy_fobject (child, dest)) {
				g_object_unref (en);
				g_object_unref (dest);
				g_object_unref (child);

				return FALSE;
			}
			g_object_unref (child);
		}

		g_object_unref (info);
	}
	g_object_unref (en);
	g_object_unref (dest);

	if (err != NULL)
		return FALSE;
	return TRUE;
}

gboolean
copy_files_to (GList *file_list, GFile *dest)
{
	GList *l;
	gboolean retval = TRUE;

	for (l = file_list; l != NULL; l = l->next) {
		GFile *source;

		source = g_file_new_for_commandline_arg (l->data);
		if (copy_fobject (source, dest) == FALSE)
			retval = FALSE;
		g_object_unref (source);
	}

	return retval;
}
