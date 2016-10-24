/*
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 *
 * The NemoPreview project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and NemoPreview. This
 * permission is above and beyond the permissions granted by the GPL license
 * NemoPreview is covered by.
 *
 * Authors: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#include "nemo-preview-file-loader.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

#define LOADER_ATTRS                          \
  G_FILE_ATTRIBUTE_STANDARD_ICON ","          \
  G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME ","  \
  G_FILE_ATTRIBUTE_STANDARD_SIZE ","          \
  G_FILE_ATTRIBUTE_STANDARD_TYPE ","          \
  G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","  \
  G_FILE_ATTRIBUTE_TIME_MODIFIED

#define DEEP_COUNT_ATTRS                      \
  G_FILE_ATTRIBUTE_STANDARD_SIZE ","          \
  G_FILE_ATTRIBUTE_STANDARD_TYPE ","          \
  G_FILE_ATTRIBUTE_STANDARD_NAME ","          \
  G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","  \
  G_FILE_ATTRIBUTE_UNIX_INODE

#define NOTIFICATION_TIMEOUT 300

G_DEFINE_TYPE (NemoPreviewFileLoader, nemo_preview_file_loader, G_TYPE_OBJECT);

enum {
  PROP_NAME = 1,
  PROP_SIZE,
  PROP_ICON,
  PROP_TIME,
  PROP_FILE,
  PROP_CONTENT_TYPE,
  PROP_FILE_TYPE,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

typedef struct {
  NemoPreviewFileLoader *self;

  GFile *file;
  GFileEnumerator *enumerator;
  GList *deep_count_subdirectories;
  GHashTable *seen_deep_count_inodes;

} DeepCountState;

struct _NemoPreviewFileLoaderPrivate {
  GFile *file;
  GFileInfo *info;

  GCancellable *cancellable;

  gint file_items;
  gint directory_items;
  gint unreadable_items;

  goffset total_size;

  gboolean loading;

  guint size_notify_timeout_id;
};

#define DIRECTORY_LOAD_ITEMS_PER_CALLBACK 100

static void deep_count_load (DeepCountState *state,
                             GFile *file);

static gboolean
size_notify_timeout_cb (gpointer user_data)
{
  NemoPreviewFileLoader *self = user_data;

  self->priv->size_notify_timeout_id = 0;

  g_object_notify (G_OBJECT (self), "size");

  return FALSE;
}

static void
queue_size_notify (NemoPreviewFileLoader *self)
{
  if (self->priv->size_notify_timeout_id != 0)
    return;

  self->priv->size_notify_timeout_id =
    g_timeout_add (NOTIFICATION_TIMEOUT,
                   size_notify_timeout_cb, self);
}

/* adapted from nautilus/libnautilus-private/nautilus-directory-async.c */

static inline gboolean
seen_inode (DeepCountState *state,
	    GFileInfo *info)
{
  guint64 inode;

  inode = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_UNIX_INODE);

  if (inode != 0) {
    return (g_hash_table_lookup (state->seen_deep_count_inodes, &inode) != NULL);
  }

  return FALSE;
}

static inline void
mark_inode_as_seen (DeepCountState *state,
		    GFileInfo *info)
{
  guint64 inode;

  inode = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_UNIX_INODE);

  if (inode != 0)
    g_hash_table_insert (state->seen_deep_count_inodes, &inode, GINT_TO_POINTER (1));
}

static void
deep_count_one (DeepCountState *state,
		GFileInfo *info)
{
  GFile *subdir;
  gboolean is_seen_inode;

  is_seen_inode = seen_inode (state, info);

  if (!is_seen_inode)
    mark_inode_as_seen (state, info);

  if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
    /* count the directory */
    state->self->priv->directory_items += 1;

    /* record the fact that we have to descend into this directory */
    subdir = g_file_get_child (state->file, g_file_info_get_name (info));
    state->deep_count_subdirectories =
      g_list_prepend (state->deep_count_subdirectories, subdir);
  } else {
    /* even non-regular files count as files */
    state->self->priv->file_items += 1;
  }

  /* count the size */
  if (!is_seen_inode &&
      g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_STANDARD_SIZE))
    state->self->priv->total_size += g_file_info_get_size (info);
}

static void
deep_count_state_free (DeepCountState *state)
{
  state->self->priv->loading = FALSE;
  
  if (state->enumerator) {
    if (!g_file_enumerator_is_closed (state->enumerator))
      g_file_enumerator_close_async (state->enumerator,
                                     0, NULL, NULL, NULL);

    g_object_unref (state->enumerator);
  }

  g_cancellable_reset (state->self->priv->cancellable);
  g_clear_object (&state->file);

  g_list_free_full (state->deep_count_subdirectories, g_object_unref);
  g_hash_table_destroy (state->seen_deep_count_inodes);

  g_free (state);
}

static void
deep_count_next_dir (DeepCountState *state)
{
  GFile *new_file;
  NemoPreviewFileLoader *self;

  self = state->self;
  g_clear_object (&state->file);

  if (state->deep_count_subdirectories != NULL) {
    /* Work on a new directory. */
    new_file = state->deep_count_subdirectories->data;
    state->deep_count_subdirectories =
      g_list_remove (state->deep_count_subdirectories, new_file);

    deep_count_load (state, new_file);
    g_object_unref (new_file);
  } else {
    deep_count_state_free (state);
  }

  /* queue notify */
  queue_size_notify (self);
}

static void
deep_count_more_files_callback (GObject *source_object,
				GAsyncResult *res,
				gpointer user_data)
{
  DeepCountState *state;
  GList *files, *l;
  GFileInfo *info;

  state = user_data;

  if (g_cancellable_is_cancelled (state->self->priv->cancellable)) {
    deep_count_state_free (state);
    return;
  }
	
  files = g_file_enumerator_next_files_finish (state->enumerator,
                                               res, NULL);
  
  for (l = files; l != NULL; l = l->next) {
    info = l->data;
    deep_count_one (state, info);
    g_object_unref (info);
  }

  if (files == NULL) {
    g_file_enumerator_close_async (state->enumerator, 0, NULL, NULL, NULL);
    g_object_unref (state->enumerator);
    state->enumerator = NULL;

    deep_count_next_dir (state);
  } else {
    g_file_enumerator_next_files_async (state->enumerator,
                                        DIRECTORY_LOAD_ITEMS_PER_CALLBACK,
                                        G_PRIORITY_DEFAULT,
                                        state->self->priv->cancellable,
                                        deep_count_more_files_callback,
                                        state);
  }

  g_list_free (files);
}

static void
deep_count_callback (GObject *source_object,
		     GAsyncResult *res,
		     gpointer user_data)
{
  DeepCountState *state;
  GFileEnumerator *enumerator;

  state = user_data;

  if (g_cancellable_is_cancelled (state->self->priv->cancellable)) {
    deep_count_state_free (state);
    return;
  }

  enumerator = g_file_enumerate_children_finish (G_FILE (source_object),
                                                 res, NULL);
	
  if (enumerator == NULL) {
    state->self->priv->unreadable_items += 1;	
    deep_count_next_dir (state);
  } else {
    state->enumerator = enumerator;
    g_file_enumerator_next_files_async (state->enumerator,
                                        DIRECTORY_LOAD_ITEMS_PER_CALLBACK,
                                        G_PRIORITY_LOW,
                                        state->self->priv->cancellable,
                                        deep_count_more_files_callback,
                                        state);
  }
}

static void
deep_count_load (DeepCountState *state,
                 GFile *file)
{
  state->file = g_object_ref (file);

  g_file_enumerate_children_async (state->file,
                                   DEEP_COUNT_ATTRS,
                                   G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, /* flags */
                                   G_PRIORITY_LOW, /* prio */
                                   state->self->priv->cancellable,
                                   deep_count_callback,
                                   state);
}

static void
deep_count_start (NemoPreviewFileLoader *self)
{
  DeepCountState *state;

  state = g_new0 (DeepCountState, 1);
  state->self = self;
  state->seen_deep_count_inodes = g_hash_table_new (g_int64_hash,
                                                    g_int64_equal);

  deep_count_load (state, self->priv->file);
}

static void
query_info_async_ready_cb (GObject *source,
                           GAsyncResult *res,
                           gpointer user_data)
{
  GFileInfo *info;
  GError *error = NULL;
  NemoPreviewFileLoader *self = user_data;

  info = g_file_query_info_finish (G_FILE (source),
                                   res, &error);

  if (error != NULL) {

    if (!g_cancellable_is_cancelled (self->priv->cancellable)) {
      gchar *uri;

      uri = g_file_get_uri (self->priv->file);
      g_warning ("Unable to query info for file %s: %s", uri, error->message);

      g_free (uri);
    }

    g_error_free (error);

    return;
  }

  self->priv->info = info;

  g_object_notify (G_OBJECT (self), "icon");
  g_object_notify (G_OBJECT (self), "name");
  g_object_notify (G_OBJECT (self), "time");
  g_object_notify (G_OBJECT (self), "content-type");
  g_object_notify (G_OBJECT (self), "file-type");

  if (g_file_info_get_file_type (info) != G_FILE_TYPE_DIRECTORY) {
    self->priv->loading = FALSE;
    g_object_notify (G_OBJECT (self), "size");
  } else {
    deep_count_start (self);
  }
}

static void
start_loading_file (NemoPreviewFileLoader *self)
{
  self->priv->loading = TRUE;

  g_file_query_info_async (self->priv->file,
                           LOADER_ATTRS,
                           G_FILE_QUERY_INFO_NONE,
                           G_PRIORITY_DEFAULT,
                           self->priv->cancellable,
                           query_info_async_ready_cb,
                           self);
}

static void
nemo_preview_file_loader_set_file (NemoPreviewFileLoader *self,
                            GFile *file)
{
  g_clear_object (&self->priv->file);
  g_clear_object (&self->priv->info);

  self->priv->file = g_object_ref (file);
  start_loading_file (self);
}

static void
nemo_preview_file_loader_dispose (GObject *object)
{
  NemoPreviewFileLoader *self = NEMO_PREVIEW_FILE_LOADER (object);

  g_clear_object (&self->priv->file);
  g_clear_object (&self->priv->info);

  if (self->priv->cancellable != NULL) {
    g_cancellable_cancel (self->priv->cancellable);
    g_clear_object (&self->priv->cancellable);
  }

  if (self->priv->size_notify_timeout_id != 0) {
    g_source_remove (self->priv->size_notify_timeout_id);
    self->priv->size_notify_timeout_id = 0;
  }

  G_OBJECT_CLASS (nemo_preview_file_loader_parent_class)->dispose (object);
}

static void
nemo_preview_file_loader_get_property (GObject *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  NemoPreviewFileLoader *self = NEMO_PREVIEW_FILE_LOADER (object);

  switch (prop_id) {
  case PROP_NAME:
    g_value_take_string (value, nemo_preview_file_loader_get_display_name (self));
    break;
  case PROP_SIZE:
    g_value_take_string (value, nemo_preview_file_loader_get_size_string (self));
    break;
  case PROP_TIME:
    g_value_take_string (value, nemo_preview_file_loader_get_date_string (self));
    break;
  case PROP_ICON:
    g_value_take_object (value, nemo_preview_file_loader_get_icon (self));
    break;
  case PROP_FILE:
    g_value_set_object (value, self->priv->file);
    break;
  case PROP_CONTENT_TYPE:
    g_value_take_string (value, nemo_preview_file_loader_get_content_type_string (self));
    break;
  case PROP_FILE_TYPE:
    g_value_set_enum (value, nemo_preview_file_loader_get_file_type (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
nemo_preview_file_loader_set_property (GObject *object,
                                guint       prop_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
  NemoPreviewFileLoader *self = NEMO_PREVIEW_FILE_LOADER (object);

  switch (prop_id) {
  case PROP_FILE:
    nemo_preview_file_loader_set_file (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
nemo_preview_file_loader_class_init (NemoPreviewFileLoaderClass *klass)
{
  GObjectClass *oclass;

  oclass = G_OBJECT_CLASS (klass);
  oclass->dispose = nemo_preview_file_loader_dispose;
  oclass->get_property = nemo_preview_file_loader_get_property;
  oclass->set_property = nemo_preview_file_loader_set_property;

  properties[PROP_FILE] =
    g_param_spec_object ("file",
                         "File",
                         "The loaded file",
                         G_TYPE_FILE,
                         G_PARAM_READWRITE);
  properties[PROP_NAME] =
    g_param_spec_string ("name",
                         "Name",
                         "The display name",
                         NULL,
                         G_PARAM_READABLE);
  properties[PROP_SIZE] =
    g_param_spec_string ("size",
                         "Size",
                         "The size string",
                         NULL,
                         G_PARAM_READABLE);
  properties[PROP_TIME] =
    g_param_spec_string ("time",
                         "Time",
                         "The time string",
                         NULL,
                         G_PARAM_READABLE);
  properties[PROP_CONTENT_TYPE] =
    g_param_spec_string ("content-type",
                         "Content Type",
                         "The content type",
                         NULL,
                         G_PARAM_READABLE);
  properties[PROP_FILE_TYPE] =
    g_param_spec_enum ("file-type",
                       "File Type",
                       "The file type",
                       G_TYPE_FILE_TYPE,
                       G_FILE_TYPE_UNKNOWN,
                       G_PARAM_READABLE);
  properties[PROP_ICON] =
    g_param_spec_object ("icon",
                         "Icon",
                         "The icon of the file",
                         GDK_TYPE_PIXBUF,
                         G_PARAM_READABLE);

  g_type_class_add_private (klass, sizeof (NemoPreviewFileLoaderPrivate));
  g_object_class_install_properties (oclass, NUM_PROPERTIES, properties);
}

static void
nemo_preview_file_loader_init (NemoPreviewFileLoader *self)
{
  self->priv =
    G_TYPE_INSTANCE_GET_PRIVATE (self,
                                 NEMO_PREVIEW_TYPE_FILE_LOADER,
                                 NemoPreviewFileLoaderPrivate);

  self->priv->cancellable = g_cancellable_new ();
  self->priv->total_size = -1;
}

NemoPreviewFileLoader *
nemo_preview_file_loader_new (GFile *file)
{
  return g_object_new (NEMO_PREVIEW_TYPE_FILE_LOADER,
                       "file", file,
                       NULL);
}

/**
 * nemo_preview_file_loader_get_display_name:
 * @self:
 *
 * Returns: (transfer full):
 */
gchar *
nemo_preview_file_loader_get_display_name (NemoPreviewFileLoader *self)
{
  if (self->priv->info == NULL)
    return NULL;

  return g_strdup (g_file_info_get_display_name (self->priv->info));
}

/**
 * nemo_preview_file_loader_get_icon:
 * @self:
 *
 * Returns: (transfer full):
 */
GdkPixbuf *
nemo_preview_file_loader_get_icon (NemoPreviewFileLoader *self)
{
  GdkPixbuf *retval;
  GtkIconInfo *info;
  GError *error = NULL;

  if (self->priv->info == NULL)
    return NULL;

  info = gtk_icon_theme_lookup_by_gicon (gtk_icon_theme_get_default (),
                                         g_file_info_get_icon (self->priv->info),
                                         256,
                                         GTK_ICON_LOOKUP_GENERIC_FALLBACK);

  if (info == NULL)
    return NULL;

  retval = gtk_icon_info_load_icon (info, &error);
  gtk_icon_info_free (info);

  if (error != NULL) {
    gchar *uri;

    uri = g_file_get_uri (self->priv->file);
    g_warning ("Unable to load icon for %s: %s", uri, error->message);

    g_free (uri);
    g_error_free (error);

    return NULL;
  }

  return retval;
}

/**
 * nemo_preview_file_loader_get_size_string:
 * @self:
 *
 * Returns: (transfer full):
 */
gchar *
nemo_preview_file_loader_get_size_string (NemoPreviewFileLoader *self)
{
  goffset size;

  if (self->priv->info == NULL)
    return NULL;

  if (g_file_info_get_file_type (self->priv->info) != G_FILE_TYPE_DIRECTORY) {
    size = g_file_info_get_size (self->priv->info);
    return g_format_size (size);
  }

  if (self->priv->total_size != -1) {
    gchar *str, *size_str, *retval;
    const gchar *items_str;

    size = self->priv->total_size;

    /* FIXME: we prolly could also use directory_items and unreadable_items
     * somehow.
     */
    items_str = g_dngettext (NULL,
                             "%d item",
                             "%d items",
                             self->priv->file_items + self->priv->directory_items);
    str = g_strdup_printf (items_str, self->priv->file_items + self->priv->directory_items);
    size_str = g_format_size (size);
    
    retval = g_strconcat (size_str, ", ", str, NULL);
    g_free (str);
    g_free (size_str);

    return retval;
  } else if (!self->priv->loading) {
    return g_strdup (_("Empty Folder"));
  }

  return NULL;
}

gboolean
nemo_preview_file_loader_get_loading (NemoPreviewFileLoader *self)
{
  return self->priv->loading;
}

/**
 * nemo_preview_file_loader_get_date_string:
 * @self:
 *
 * Returns: (transfer full):
 */
gchar *
nemo_preview_file_loader_get_date_string (NemoPreviewFileLoader *self)
{
  GTimeVal timeval;
  GDateTime *date;
  gchar *retval;

  if (self->priv->info == NULL)
    return NULL;

  g_file_info_get_modification_time (self->priv->info,
                                     &timeval);
  date = g_date_time_new_from_timeval_local (&timeval);

  /* FIXME: is this right? */
  retval = g_date_time_format (date, "%x %X");
  g_date_time_unref (date);

  return retval;
}

/**
 * nemo_preview_file_loader_get_content_type_string:
 * @self:
 *
 * Returns: (transfer full):
 */
gchar *
nemo_preview_file_loader_get_content_type_string (NemoPreviewFileLoader *self)
{
  if (self->priv->info == NULL)
    return NULL;

  return g_content_type_get_description (g_file_info_get_content_type (self->priv->info));
}

/**
 * nemo_preview_file_loader_get_file_type:
 * @self:
 *
 * Returns:
 */
GFileType
nemo_preview_file_loader_get_file_type (NemoPreviewFileLoader *self)
{
  if (self->priv->info == NULL)
    return G_FILE_TYPE_UNKNOWN;

  return g_file_info_get_file_type (self->priv->info);
}

void
nemo_preview_file_loader_stop (NemoPreviewFileLoader *self)
{
  if (self->priv->cancellable == NULL)
    return;

  g_cancellable_cancel (self->priv->cancellable);
}
