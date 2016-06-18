/*
 *   Copyright (C) 2007-2013 Tristan Heaven <tristanheaven@gmail.com>
 *
 *   This file is part of GtkHash.
 *
 *   GtkHash is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   GtkHash is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GtkHash. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <glib.h>

#include "hash-file.h"
#include "hash-func.h"
#include "hash-lib.h"

// This lib can use GDK 2 or 3, but doesn't link either directly.
// Try to avoid potential ABI/API mismatch issues by only declaring
// necessary gdk.h functions...
guint gdk_threads_add_idle(GSourceFunc, gpointer);
guint gdk_threads_add_timeout(guint, GSourceFunc, gpointer);
// (this might cause other problems)

static gboolean gtkhash_hash_file_source_func(struct hash_file_s *data);
static void gtkhash_hash_file_hash_thread_func(void *func,
	struct hash_file_s *data);

static void gtkhash_hash_file_add_source(struct hash_file_s *data)
{
	g_mutex_lock(&data->priv.mutex);
	g_assert(!data->priv.source);
	data->priv.source = g_idle_add((GSourceFunc)gtkhash_hash_file_source_func,
		data);
	g_mutex_unlock(&data->priv.mutex);
}

static void gtkhash_hash_file_remove_source(struct hash_file_s *data)
{
	g_mutex_lock(&data->priv.mutex);
	if (G_UNLIKELY(!g_source_remove(data->priv.source)))
		g_assert_not_reached();
	data->priv.source = 0;
	g_mutex_unlock(&data->priv.mutex);
}

void gtkhash_hash_file_cancel(struct hash_file_s *data)
{
	g_cancellable_cancel(data->cancellable);
}

static gboolean gtkhash_hash_file_report_source_func(struct hash_file_s *data)
{
	if (data->report_source)
		gtkhash_hash_file_report_cb(data->cb_data, data->file_size,
			data->total_read, data->timer);

	return true;
}

static void gtkhash_hash_file_add_report_source(struct hash_file_s *data)
{
	g_assert(!data->report_source);
	data->report_source = gdk_threads_add_timeout(HASH_FILE_REPORT_INTERVAL,
		(GSourceFunc)gtkhash_hash_file_report_source_func, data);
}

static void gtkhash_hash_file_remove_report_source(struct hash_file_s *data)
{
	if (data->report_source) {
		if (G_UNLIKELY(!g_source_remove(data->report_source)))
			g_assert_not_reached();
		data->report_source = 0;
	}
}

static void gtkhash_hash_file_start(struct hash_file_s *data)
{
	g_assert(data->uri);

	int funcs_enabled = 0;

	for (int i = 0; i < HASH_FUNCS_N; i++) {
		if (data->funcs[i].enabled) {
			gtkhash_hash_lib_start(&data->funcs[i], data->hmac_key,
				data->key_size);
			funcs_enabled++;
		}
	}

	g_assert(funcs_enabled > 0);

	// Determine max number of threads to use
	const int cpus = g_get_num_processors();
	const int threads = CLAMP(MIN(funcs_enabled, cpus), 1, HASH_FUNCS_N);

	g_atomic_int_set(&data->pool_threads_n, 0);
	data->thread_pool = g_thread_pool_new(
		(GFunc)gtkhash_hash_file_hash_thread_func, data, threads, true, NULL);

	data->file = g_file_new_for_uri(data->uri);
	data->just_read = 0;
	data->buffer = g_malloc(HASH_FILE_BUFFER_SIZE);
	data->timer = g_timer_new();
	data->total_read = 0;

	data->state = HASH_FILE_STATE_OPEN;
}

static void gtkhash_hash_file_open_finish(G_GNUC_UNUSED GObject *source,
	GAsyncResult *res, struct hash_file_s *data)
{
	data->stream = g_file_read_finish(data->file, res, NULL);
	if (G_UNLIKELY(!data->stream &&
		!g_cancellable_is_cancelled(data->cancellable)))
	{
		g_warning("failed to open file (%s)", data->uri);
		g_cancellable_cancel(data->cancellable);
	}

	if (G_UNLIKELY(g_cancellable_is_cancelled(data->cancellable))) {
		if (data->stream)
			data->state = HASH_FILE_STATE_CLOSE;
		else
			data->state = HASH_FILE_STATE_FINISH;
	} else
		data->state = HASH_FILE_STATE_GET_SIZE;

	gtkhash_hash_file_add_source(data);
}

static void gtkhash_hash_file_open(struct hash_file_s *data)
{
	if (G_UNLIKELY(g_cancellable_is_cancelled(data->cancellable))) {
		data->state = HASH_FILE_STATE_FINISH;
		return;
	}

	gtkhash_hash_file_remove_source(data);
	g_file_read_async(data->file, G_PRIORITY_DEFAULT, data->cancellable,
		(GAsyncReadyCallback)gtkhash_hash_file_open_finish, data);
}

static void gtkhash_hash_file_get_size_finish(G_GNUC_UNUSED GObject *source,
	GAsyncResult *res, struct hash_file_s *data)
{
	GFileInfo *info = g_file_input_stream_query_info_finish(
		data->stream, res, NULL);
	data->file_size = g_file_info_get_size(info);
	g_object_unref(info);

	if (G_UNLIKELY(g_cancellable_is_cancelled(data->cancellable)))
		data->state = HASH_FILE_STATE_CLOSE;
	else if (data->file_size == 0)
		data->state = HASH_FILE_STATE_HASH;
	else {
		data->state = HASH_FILE_STATE_READ;
		gtkhash_hash_file_add_report_source(data);
	}

	gtkhash_hash_file_add_source(data);
}

static void gtkhash_hash_file_get_size(struct hash_file_s *data)
{
	if (G_UNLIKELY(g_cancellable_is_cancelled(data->cancellable))) {
		data->state = HASH_FILE_STATE_CLOSE;
		return;
	}

	gtkhash_hash_file_remove_source(data);
	g_file_input_stream_query_info_async(data->stream,
		G_FILE_ATTRIBUTE_STANDARD_SIZE, G_PRIORITY_DEFAULT, data->cancellable,
		(GAsyncReadyCallback)gtkhash_hash_file_get_size_finish, data);
}

static void gtkhash_hash_file_read_finish(G_GNUC_UNUSED GObject *source,
	GAsyncResult *res, struct hash_file_s *data)
{
	data->just_read = g_input_stream_read_finish(
		G_INPUT_STREAM(data->stream), res, NULL);

	if (G_UNLIKELY(data->just_read == -1) &&
		!g_cancellable_is_cancelled(data->cancellable))
	{
		g_warning("failed to read file (%s)", data->uri);
		g_cancellable_cancel(data->cancellable);
	} else if (G_UNLIKELY(data->just_read == 0)) {
		g_warning("unexpected EOF (%s)", data->uri);
		g_cancellable_cancel(data->cancellable);
	} else {
		data->total_read += data->just_read;
		if (G_UNLIKELY(data->total_read > data->file_size)) {
			g_warning("read %" G_GOFFSET_FORMAT
				" more bytes than expected (%s)", data->total_read -
				data->file_size, data->uri);
			g_cancellable_cancel(data->cancellable);
		} else
			data->state = HASH_FILE_STATE_HASH;
	}

	if (G_UNLIKELY(g_cancellable_is_cancelled(data->cancellable)))
		data->state = HASH_FILE_STATE_CLOSE;

	gtkhash_hash_file_add_source(data);
}

static void gtkhash_hash_file_read(struct hash_file_s *data)
{
	if (G_UNLIKELY(g_cancellable_is_cancelled(data->cancellable))) {
		data->state = HASH_FILE_STATE_CLOSE;
		return;
	}

	gtkhash_hash_file_remove_source(data);
	g_input_stream_read_async(G_INPUT_STREAM(data->stream),
		data->buffer, HASH_FILE_BUFFER_SIZE, G_PRIORITY_DEFAULT,
		data->cancellable, (GAsyncReadyCallback)gtkhash_hash_file_read_finish,
		data);
}

static void gtkhash_hash_file_hash_thread_func(void *func,
	struct hash_file_s *data)
{
	gtkhash_hash_lib_update(&data->funcs[GPOINTER_TO_UINT(func) - 1],
		data->buffer, data->just_read);

	if (g_atomic_int_dec_and_test(&data->pool_threads_n))
		gtkhash_hash_file_add_source(data);
}

static void gtkhash_hash_file_hash(struct hash_file_s *data)
{
	if (G_UNLIKELY(g_cancellable_is_cancelled(data->cancellable))) {
		data->state = HASH_FILE_STATE_CLOSE;
		return;
	}

	gtkhash_hash_file_remove_source(data);
	data->state =  HASH_FILE_STATE_HASH_FINISH;

	g_atomic_int_inc(&data->pool_threads_n);
	for (unsigned int i = 0; i < HASH_FUNCS_N; i++) {
		if (data->funcs[i].enabled) {
			g_atomic_int_inc(&data->pool_threads_n);
			g_thread_pool_push(data->thread_pool, GUINT_TO_POINTER(i + 1), NULL);
		}
	}

	if (g_atomic_int_dec_and_test(&data->pool_threads_n))
		gtkhash_hash_file_add_source(data);
}

static void gtkhash_hash_file_hash_finish(struct hash_file_s *data)
{
	g_assert(g_atomic_int_get(&data->pool_threads_n) == 0);

	if (G_UNLIKELY(g_cancellable_is_cancelled(data->cancellable))) {
		data->state = HASH_FILE_STATE_CLOSE;
		return;
	}

	if (data->total_read >= data->file_size)
		data->state = HASH_FILE_STATE_CLOSE;
	else
		data->state = HASH_FILE_STATE_READ;
}

static void gtkhash_hash_file_close_finish(G_GNUC_UNUSED GObject *source,
	GAsyncResult *res, struct hash_file_s *data)
{
	if (G_UNLIKELY(!g_input_stream_close_finish(G_INPUT_STREAM(data->stream), res, NULL) &&
		!g_cancellable_is_cancelled(data->cancellable)))
	{
		g_warning("failed to close file (%s)", data->uri);
	}

	g_object_unref(data->stream);

	gtkhash_hash_file_remove_report_source(data);
	data->state =  HASH_FILE_STATE_FINISH;
	gtkhash_hash_file_add_source(data);
}

static void gtkhash_hash_file_close(struct hash_file_s *data)
{
	gtkhash_hash_file_remove_source(data);
	g_input_stream_close_async(G_INPUT_STREAM(data->stream),
		G_PRIORITY_DEFAULT, data->cancellable,
		(GAsyncReadyCallback)gtkhash_hash_file_close_finish, data);
}

static void gtkhash_hash_file_finish(struct hash_file_s *data)
{
	if (G_UNLIKELY(g_cancellable_is_cancelled(data->cancellable))) {
		for (int i = 0; i < HASH_FUNCS_N; i++)
			if (data->funcs[i].enabled)
				gtkhash_hash_lib_stop(&data->funcs[i]);
	} else {
		for (int i = 0; i < HASH_FUNCS_N; i++)
			if (data->funcs[i].enabled)
				gtkhash_hash_lib_finish(&data->funcs[i]);
	}

	g_object_unref(data->file);
	g_free(data->buffer);
	g_timer_destroy(data->timer);
	g_thread_pool_free(data->thread_pool, true, false);

	data->state = HASH_FILE_STATE_CALLBACK;
}

static gboolean gtkhash_hash_file_callback_stop_func(void *cb_data)
{
	gtkhash_hash_file_stop_cb(cb_data);

	return false;
}

static gboolean gtkhash_hash_file_callback_finish_func(void *cb_data)
{
	gtkhash_hash_file_finish_cb(cb_data);

	return false;
}

static void gtkhash_hash_file_callback(struct hash_file_s *data)
{
	gtkhash_hash_file_remove_source(data);
	data->state = HASH_FILE_STATE_IDLE;

	if (G_UNLIKELY(g_cancellable_is_cancelled(data->cancellable))) {
		gdk_threads_add_idle(gtkhash_hash_file_callback_stop_func,
			data->cb_data);
	} else {
		gdk_threads_add_idle(gtkhash_hash_file_callback_finish_func,
			data->cb_data);
	}
}

static gboolean gtkhash_hash_file_source_func(struct hash_file_s *data)
{
	static void (* const state_funcs[])(struct hash_file_s *) = {
		[HASH_FILE_STATE_IDLE]        = NULL,
		[HASH_FILE_STATE_START]       = gtkhash_hash_file_start,
		[HASH_FILE_STATE_OPEN]        = gtkhash_hash_file_open,
		[HASH_FILE_STATE_GET_SIZE]    = gtkhash_hash_file_get_size,
		[HASH_FILE_STATE_READ]        = gtkhash_hash_file_read,
		[HASH_FILE_STATE_HASH]        = gtkhash_hash_file_hash,
		[HASH_FILE_STATE_HASH_FINISH] = gtkhash_hash_file_hash_finish,
		[HASH_FILE_STATE_CLOSE]       = gtkhash_hash_file_close,
		[HASH_FILE_STATE_FINISH]      = gtkhash_hash_file_finish,
		[HASH_FILE_STATE_CALLBACK]    = gtkhash_hash_file_callback,
	};

	state_funcs[data->state](data);

	return true;
}

void gtkhash_hash_file(struct hash_file_s *data, const char *uri,
	const uint8_t *hmac_key, const size_t key_size)
{
	g_assert(data);
	g_assert(uri && *uri);
	g_assert(data->state == HASH_FILE_STATE_IDLE);
	g_assert(data->report_source == 0);
	g_assert(data->priv.source == 0);

	data->uri = uri;
	data->hmac_key = hmac_key;
	data->key_size = key_size;
	g_cancellable_reset(data->cancellable);

	data->state = HASH_FILE_STATE_START;
	gtkhash_hash_file_add_source(data);
}

void gtkhash_hash_file_init(struct hash_file_s *data, struct hash_func_s *funcs,
	void *cb_data)
{
	data->file_size = 0;
	data->total_read = 0;
	data->cb_data = cb_data;
	data->uri = NULL;
	data->file = NULL;
	data->hmac_key = NULL;
	data->key_size = 0;
	data->cancellable = g_cancellable_new();
	data->stream = NULL;
	data->just_read = 0;
	data->buffer = NULL;
	data->timer = NULL;
	data->thread_pool = NULL;
	data->funcs = funcs;
	data->pool_threads_n = 0;
	data->report_source = 0;
	data->state = HASH_FILE_STATE_IDLE;

	g_mutex_init(&data->priv.mutex);
	data->priv.source = 0;
}

void gtkhash_hash_file_deinit(struct hash_file_s *data)
{
	// Shouldn't still be running
	g_assert(data->state == HASH_FILE_STATE_IDLE);
	g_assert(data->report_source == 0);
	g_assert(data->priv.source == 0);

	g_object_unref(data->cancellable);
	g_mutex_clear(&data->priv.mutex);
}

void gtkhash_hash_file_clear_digests(struct hash_file_s *data)
{
	for (int i = 0; i < HASH_FUNCS_N; i++)
		gtkhash_hash_func_clear_digest(&data->funcs[i]);
}
