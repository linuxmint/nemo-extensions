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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * The NemoPreview project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and NemoPreview. This
 * permission is above and beyond the permissions granted by the GPL license
 * NemoPreview is covered by.
 *
 * Authors: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#include "nemo-preview-font-loader.h"

#include <stdlib.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <gio/gio.h>

typedef struct {
  FT_Library library;
  FT_Long face_index;
  GFile *file;

  gchar *face_contents;
  gsize face_length;
} FontLoadJob;

static FontLoadJob *
font_load_job_new (FT_Library library,
                   const gchar *uri,
                   GAsyncReadyCallback callback,
                   gpointer user_data)
{
  FontLoadJob *job = g_slice_new0 (FontLoadJob);

  job->library = library;
  job->face_index = 0;
  job->file = g_file_new_for_uri (uri);

  return job;
}

static void
font_load_job_free (FontLoadJob *job)
{
  g_clear_object (&job->file);

  g_slice_free (FontLoadJob, job);
}

static FT_Face
create_face_from_contents (FontLoadJob *job,
                           gchar **contents,
                           GError **error)
{
  FT_Error ft_error;
  FT_Face retval;

  ft_error = FT_New_Memory_Face (job->library,
                                 (const FT_Byte *) job->face_contents,
                                 (FT_Long) job->face_length,
                                 job->face_index,
                                 &retval);

  if (ft_error != 0) {
    gchar *uri;
    uri = g_file_get_uri (job->file);
    g_set_error (error, G_IO_ERROR, 0,
                 "Unable to read the font face file '%s'", uri);
    retval = NULL;
    g_free (job->face_contents);
    g_free (uri);
  } else {
    *contents = job->face_contents;
  }

  return retval;
}

static void
font_load_job_do_load (FontLoadJob *job,
                       GError **error)
{
  gchar *contents;
  gsize length;

  g_file_load_contents (job->file, NULL,
                        &contents, &length, NULL, error);

  if ((error != NULL) && (*error == NULL)) {
    job->face_contents = contents;
    job->face_length = length;
  }
}

static void
font_load_job (GTask *task,
	       gpointer source_object,
	       gpointer user_data,
               GCancellable *cancellable)
{
  FontLoadJob *job = user_data;
  GError *error = NULL;

  font_load_job_do_load (job, &error);

  if (error != NULL)
    g_task_return_error (task, error);
  else
    g_task_return_boolean (task, TRUE);
}

/**
 * nemo_preview_new_ft_face_from_uri: (skip)
 *
 */
FT_Face
nemo_preview_new_ft_face_from_uri (FT_Library library,
                            const gchar *uri,
                            gchar **contents,
                            GError **error)
{
  FontLoadJob *job = NULL;
  FT_Face face;

  job = font_load_job_new (library, uri, NULL, NULL);
  font_load_job_do_load (job, error);

  if ((error != NULL) && (*error != NULL)) {
    font_load_job_free (job);
    return NULL;
  }

  face = create_face_from_contents (job, contents, error);
  font_load_job_free (job);

  return face;
}

/**
 * nemo_preview_new_ft_face_from_uri_async: (skip)
 *
 */
void
nemo_preview_new_ft_face_from_uri_async (FT_Library library,
                                  const gchar *uri,
                                  GAsyncReadyCallback callback,
                                  gpointer user_data)
{
  FontLoadJob *job = font_load_job_new (library, uri, callback, user_data);
  GTask *task;

  task = g_task_new (NULL, NULL, callback, user_data);
  g_task_set_task_data (task, job, (GDestroyNotify) font_load_job_free);
  g_task_run_in_thread (task, font_load_job);
  g_object_unref (task);
}

/**
 * nemo_preview_new_ft_face_from_uri_finish: (skip)
 *
 */
FT_Face
nemo_preview_new_ft_face_from_uri_finish (GAsyncResult *result,
                                   gchar **contents,
                                   GError **error)
{
  FontLoadJob *job;

  if (!g_task_propagate_boolean (G_TASK (result), error))
    return NULL;

  job = g_task_get_task_data (G_TASK (result));

  return create_face_from_contents (job, contents, error);
}
