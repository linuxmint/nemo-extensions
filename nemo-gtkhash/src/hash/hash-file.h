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

#ifndef GTKHASH_HASH_HASH_FILE_H
#define GTKHASH_HASH_HASH_FILE_H

#include <stdbool.h>
#include <stdint.h>
#include <glib.h>
#include <gio/gio.h>

#include "hash-func.h"

enum hash_file_state_e {
	HASH_FILE_STATE_IDLE,
	HASH_FILE_STATE_START,
	HASH_FILE_STATE_OPEN,
	HASH_FILE_STATE_GET_SIZE,
	HASH_FILE_STATE_READ,
	HASH_FILE_STATE_HASH,
	HASH_FILE_STATE_HASH_FINISH,
	HASH_FILE_STATE_CLOSE,
	HASH_FILE_STATE_FINISH,
	HASH_FILE_STATE_CALLBACK,
};

struct hash_file_s {
	goffset file_size, total_read;
	void *cb_data;
	const char *uri;
	GFile *file;
	const uint8_t *hmac_key;
	size_t key_size;
	GCancellable *cancellable;
	GFileInputStream *stream;
	gssize just_read;
	uint8_t *buffer;
	GTimer *timer;
	GThreadPool *thread_pool;
	struct hash_func_s *funcs;
	int pool_threads_n;
	unsigned int report_source;
	enum hash_file_state_e state;
	struct {
		GMutex mutex;
		unsigned int source;
	} priv;
};

void gtkhash_hash_file_cancel(struct hash_file_s *data);
void gtkhash_hash_file_init(struct hash_file_s *data, struct hash_func_s *funcs,
	void *cb_data);
void gtkhash_hash_file_deinit(struct hash_file_s *data);
void gtkhash_hash_file_clear_digests(struct hash_file_s *data);
void gtkhash_hash_file(struct hash_file_s *data, const char *uri,
	const uint8_t *hmac_key, const size_t key_size);

void gtkhash_hash_file_report_cb(void *data, goffset file_size,
	goffset total_read, GTimer *timer);
void gtkhash_hash_file_finish_cb(void *data);
void gtkhash_hash_file_stop_cb(void *data);

#endif
