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

#include "nemo-preview-pdf-loader.h"

#include "nemo-preview-utils.h"
#include <xreader-document.h>
#include <xreader-view.h>
#include <glib/gstdio.h>
#include <gdk/gdkx.h>

G_DEFINE_TYPE (NemoPreviewPdfLoader, nemo_preview_pdf_loader, G_TYPE_OBJECT);

enum {
  PROP_DOCUMENT = 1,
  PROP_URI
};

static void load_libreoffice (NemoPreviewPdfLoader *self);

struct _NemoPreviewPdfLoaderPrivate {
  EvDocument *document;
  gchar *uri;
  gchar *pdf_path;

  GPid libreoffice_pid;
};

static void
load_job_done (EvJob *job,
               gpointer user_data)
{
  NemoPreviewPdfLoader *self = user_data;

  if (ev_job_is_failed (job)) {
    g_print ("Failed to load document: %s", job->error->message);
    g_object_unref (job);

    return;
  }

  self->priv->document = g_object_ref (job->document);
  g_object_unref (job);

  g_object_notify (G_OBJECT (self), "document");
}

static void
load_pdf (NemoPreviewPdfLoader *self,
          const gchar *uri)
{
  EvJob *job;

  job = ev_job_load_new (uri);
  g_signal_connect (job, "finished",
                    G_CALLBACK (load_job_done), self);

  ev_job_scheduler_push_job (job, EV_JOB_PRIORITY_NONE);
}

static void
libreoffice_missing_ready_cb (GObject *source,
                              GAsyncResult *res,
                              gpointer user_data)
{
  NemoPreviewPdfLoader *self = user_data;
  GError *error = NULL;

  g_dbus_connection_call_finish (G_DBUS_CONNECTION (source), res, &error);
  if (error != NULL) {
    /* can't install libreoffice with packagekit - nothing else we can do */
    /* FIXME: error reporting! */
    g_warning ("libreoffice not found, and PackageKit failed to install it with error %s",
               error->message);
    return;
  }

  /* now that we have libreoffice installed, try again loading the document */
  load_libreoffice (self);
}

static void
libreoffice_missing (NemoPreviewPdfLoader *self)
{
  GApplication *app = g_application_get_default ();
  GtkWidget *widget = GTK_WIDGET (gtk_application_get_active_window (GTK_APPLICATION (app)));
  GDBusConnection *connection = g_application_get_dbus_connection (app);
  guint xid = 0;
  GdkWindow *gdk_window;
  const gchar *libreoffice_path[2];

  gdk_window = gtk_widget_get_window (widget);
  if (gdk_window != NULL)
    xid = GDK_WINDOW_XID (gdk_window);

  libreoffice_path[0] = "/usr/bin/libreoffice";
  libreoffice_path[1] = NULL;

  g_dbus_connection_call (connection,
                          "org.freedesktop.PackageKit",
                          "/org/freedesktop/PackageKit",
                          "org.freedesktop.PackageKit.Modify",
                          "InstallProvideFiles",
                          g_variant_new ("(u^ass)",
                                         xid,
                                         libreoffice_path,
                                         "hide-confirm-deps"),
                          NULL, G_DBUS_CALL_FLAGS_NONE,
                          G_MAXINT, NULL,
                          libreoffice_missing_ready_cb,
                          self);
}

static void
libreoffice_child_watch_cb (GPid pid,
                            gint status,
                            gpointer user_data)
{
  NemoPreviewPdfLoader *self = user_data;
  GFile *file;
  gchar *uri;

  g_spawn_close_pid (pid);
  self->priv->libreoffice_pid = -1;

  file = g_file_new_for_path (self->priv->pdf_path);
  uri = g_file_get_uri (file);
  load_pdf (self, uri);

  g_object_unref (file);
  g_free (uri);
}

static void
load_libreoffice (NemoPreviewPdfLoader *self)
{
  gchar *libreoffice_path;
  GFile *file;
  gchar *doc_path, *doc_name, *tmp_name, *pdf_dir;
  gboolean res;
  GPid pid;
  GError *error = NULL;
  const gchar *libreoffice_argv[] = {
    NULL /* to be replaced with binary */,
    "--convert-to", "pdf",
    "--outdir", NULL /* to be replaced with output dir */,
    NULL /* to be replaced with input file */,
    NULL
  };

  libreoffice_path = g_find_program_in_path ("libreoffice");
  if (libreoffice_path == NULL) {
    libreoffice_missing (self);
    return;
  }

  file = g_file_new_for_uri (self->priv->uri);
  doc_path = g_file_get_path (file);
  doc_name = g_file_get_basename (file);
  g_object_unref (file);

  /* libreoffice --convert-to replaces the extension with .pdf */
  tmp_name = g_strrstr (doc_name, ".");
  if (tmp_name)
    *tmp_name = '\0';
  tmp_name = g_strdup_printf ("%s.pdf", tmp_name);
  g_free (doc_name);

  pdf_dir = g_build_filename (g_get_user_cache_dir (), "sushi", NULL);
  self->priv->pdf_path = g_build_filename (pdf_dir, tmp_name, NULL);
  g_mkdir_with_parents (pdf_dir, 0700);

  g_free (tmp_name);
  libreoffice_argv[0] = libreoffice_path;
  libreoffice_argv[4] = pdf_dir;
  libreoffice_argv[5] = doc_path;

  tmp_name = g_strjoinv (" ", (gchar **) libreoffice_argv);
  g_debug ("Executing LibreOffice command: %s", tmp_name);
  g_free (tmp_name);

  res = g_spawn_async (NULL, (gchar **) libreoffice_argv, NULL,
                       G_SPAWN_DO_NOT_REAP_CHILD,
                       NULL, NULL,
                       &pid, &error);

  g_free (pdf_dir);
  g_free (doc_path);
  g_free (libreoffice_path);

  if (!res) {
    g_warning ("Error while spawning libreoffice: %s",
               error->message);
    g_error_free (error);

    return;
  }

  g_child_watch_add (pid, libreoffice_child_watch_cb, self);
  self->priv->libreoffice_pid = pid;
}

static gboolean
content_type_is_native (const gchar *content_type)
{
  gchar **native_types;
  gint idx;
  gboolean found = FALSE;

  native_types = nemo_preview_query_supported_document_types ();

  for (idx = 0; native_types[idx] != NULL; idx++) {
    found = g_content_type_is_a (content_type, native_types[idx]);
    if (found)
      break;
  }

  g_strfreev (native_types);

  return found;
}

static void
query_info_ready_cb (GObject *obj,
                     GAsyncResult *res,
                     gpointer user_data)
{
  NemoPreviewPdfLoader *self = user_data;
  GError *error = NULL;
  GFileInfo *info;
  const gchar *content_type;

  info = g_file_query_info_finish (G_FILE (obj),
                                   res, &error);

  if (error != NULL) {
    g_warning ("Unable to query the mimetype of %s: %s",
               self->priv->uri, error->message);
    g_error_free (error);

    return;
  }

  content_type = g_file_info_get_content_type (info);

  if (content_type_is_native (content_type))
    load_pdf (self, self->priv->uri);
  else
    load_libreoffice (self);

  g_object_unref (info);
}

static void
start_loading_document (NemoPreviewPdfLoader *self)
{
  GFile *file;

  file = g_file_new_for_uri (self->priv->uri);
  g_file_query_info_async (file,
                           G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                           G_FILE_QUERY_INFO_NONE,
                           G_PRIORITY_DEFAULT,
                           NULL,
                           query_info_ready_cb,
                           self);

  g_object_unref (file);
}

static void
nemo_preview_pdf_loader_set_uri (NemoPreviewPdfLoader *self,
                          const gchar *uri)
{
  g_clear_object (&self->priv->document);
  g_free (self->priv->uri);

  self->priv->uri = g_strdup (uri);
  start_loading_document (self);
}

void
nemo_preview_pdf_loader_cleanup_document (NemoPreviewPdfLoader *self)
{
  if (self->priv->pdf_path) {
    g_unlink (self->priv->pdf_path);
    g_free (self->priv->pdf_path);
  }

  if (self->priv->libreoffice_pid != -1) {
    kill (self->priv->libreoffice_pid, SIGKILL);
    self->priv->libreoffice_pid = -1;
  }
}

static void
nemo_preview_pdf_loader_dispose (GObject *object)
{
  NemoPreviewPdfLoader *self = NEMO_PREVIEW_PDF_LOADER (object);

  nemo_preview_pdf_loader_cleanup_document (self);

  g_clear_object (&self->priv->document);
  g_free (self->priv->uri);

  G_OBJECT_CLASS (nemo_preview_pdf_loader_parent_class)->dispose (object);
}

static void
nemo_preview_pdf_loader_get_property (GObject *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  NemoPreviewPdfLoader *self = NEMO_PREVIEW_PDF_LOADER (object);

  switch (prop_id) {
  case PROP_DOCUMENT:
    g_value_set_object (value, self->priv->document);
    break;
  case PROP_URI:
    g_value_set_string (value, self->priv->uri);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
nemo_preview_pdf_loader_set_property (GObject *object,
                               guint       prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
  NemoPreviewPdfLoader *self = NEMO_PREVIEW_PDF_LOADER (object);

  switch (prop_id) {
  case PROP_URI:
    nemo_preview_pdf_loader_set_uri (self, g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
nemo_preview_pdf_loader_class_init (NemoPreviewPdfLoaderClass *klass)
{
  GObjectClass *oclass;

  oclass = G_OBJECT_CLASS (klass);
  oclass->dispose = nemo_preview_pdf_loader_dispose;
  oclass->get_property = nemo_preview_pdf_loader_get_property;
  oclass->set_property = nemo_preview_pdf_loader_set_property;

    g_object_class_install_property
      (oclass,
       PROP_DOCUMENT,
       g_param_spec_object ("document",
                            "Document",
                            "The loaded document",
                            EV_TYPE_DOCUMENT,
                            G_PARAM_READABLE));

    g_object_class_install_property
      (oclass,
       PROP_URI,
       g_param_spec_string ("uri",
                            "URI",
                            "The URI to load",
                            NULL,
                            G_PARAM_READWRITE));

    g_type_class_add_private (klass, sizeof (NemoPreviewPdfLoaderPrivate));
}

static void
nemo_preview_pdf_loader_init (NemoPreviewPdfLoader *self)
{
  self->priv =
    G_TYPE_INSTANCE_GET_PRIVATE (self,
                                 NEMO_PREVIEW_TYPE_PDF_LOADER,
                                 NemoPreviewPdfLoaderPrivate);
  self->priv->libreoffice_pid = -1;
}

NemoPreviewPdfLoader *
nemo_preview_pdf_loader_new (const gchar *uri)
{
  return g_object_new (NEMO_PREVIEW_TYPE_PDF_LOADER,
                       "uri", uri,
                       NULL);
}

/**
 * nemo_preview_pdf_loader_get_max_page_size:
 * @self:
 * @width: (out):
 * @height: (out):
 *
 */
void
nemo_preview_pdf_loader_get_max_page_size (NemoPreviewPdfLoader *self,
                                    gdouble *width,
                                    gdouble *height)
{
  if (self->priv->document == NULL)
    return;

  ev_document_get_max_page_size (self->priv->document, width, height);
}
