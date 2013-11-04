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
#include <stdbool.h>
#include <stdint.h>
#include <glib.h>
#include <zlib.h>

#include "hash-lib-zlib.h"
#include "hash-lib.h"
#include "hash-func.h"

#define LIB_DATA ((struct hash_lib_zlib_s *)func->lib_data)

struct hash_lib_zlib_s {
	uLong csum;
};

static uLong gtkhash_hash_lib_zlib_checksum(const enum hash_func_e id,
	const uLong csum, const uint8_t *buffer, const size_t size)
{
	switch (id) {
		case HASH_FUNC_CRC32:
			return crc32(csum, buffer, size);
		case HASH_FUNC_ADLER32:
			return adler32(csum, buffer, size);
		default:
			g_assert_not_reached();
	}

	return 0;
}

bool gtkhash_hash_lib_zlib_is_supported(const enum hash_func_e id)
{
	switch (id) {
		case HASH_FUNC_CRC32:
		case HASH_FUNC_ADLER32:
			return true;
		default:
			return false;
	}
}

void gtkhash_hash_lib_zlib_start(struct hash_func_s *func)
{
	func->lib_data = g_new(struct hash_lib_zlib_s, 1);

	LIB_DATA->csum = gtkhash_hash_lib_zlib_checksum(func->id, 0L, Z_NULL, 0);
}

void gtkhash_hash_lib_zlib_update(struct hash_func_s *func,
	const uint8_t *buffer, const size_t size)
{
	LIB_DATA->csum = gtkhash_hash_lib_zlib_checksum(func->id, LIB_DATA->csum,
		buffer, size);
}

void gtkhash_hash_lib_zlib_stop(struct hash_func_s *func)
{
	g_free(LIB_DATA);
}

uint8_t *gtkhash_hash_lib_zlib_finish(struct hash_func_s *func, size_t *size)
{
	uint32_t csum = GUINT32_TO_BE(LIB_DATA->csum);
	g_free(LIB_DATA);

	*size = sizeof(csum);
	uint8_t *digest = g_memdup(&csum, *size);

	return digest;
}
