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
#include <mhash.h>

#include "hash-lib-mhash.h"
#include "hash-lib.h"
#include "hash-func.h"

#define LIB_DATA ((struct hash_lib_mhash_s *)func->lib_data)

struct hash_lib_mhash_s {
	MHASH thread;
	hashid type;
};

static bool gtkhash_hash_lib_mhash_set_type(const enum hash_func_e id,
	hashid *type)
{
	switch (id) {
		case HASH_FUNC_GOST:
			*type = MHASH_GOST;
			break;
		case HASH_FUNC_HAVAL128_3:
			*type = MHASH_HAVAL128;
			break;
		case HASH_FUNC_HAVAL160_3:
			*type = MHASH_HAVAL160;
			break;
		case HASH_FUNC_HAVAL192_3:
			*type = MHASH_HAVAL192;
			break;
		case HASH_FUNC_HAVAL224_3:
			*type = MHASH_HAVAL224;
			break;
		case HASH_FUNC_HAVAL256_3:
			*type = MHASH_HAVAL256;
			break;
		case HASH_FUNC_MD2:
			*type = MHASH_MD2;
			break;
		case HASH_FUNC_MD4:
			*type = MHASH_MD4;
			break;
		case HASH_FUNC_MD5:
			*type = MHASH_MD5;
			break;
		case HASH_FUNC_RIPEMD128:
			*type = MHASH_RIPEMD128;
			break;
		case HASH_FUNC_RIPEMD160:
			*type = MHASH_RIPEMD160;
			break;
		case HASH_FUNC_RIPEMD256:
			*type = MHASH_RIPEMD256;
			break;
		case HASH_FUNC_RIPEMD320:
			*type = MHASH_RIPEMD320;
			break;
		case HASH_FUNC_SHA1:
			*type = MHASH_SHA1;
			break;
		case HASH_FUNC_SHA224:
			*type = MHASH_SHA224;
			break;
		case HASH_FUNC_SHA256:
			*type = MHASH_SHA256;
			break;
		case HASH_FUNC_SHA384:
			*type = MHASH_SHA384;
			break;
		case HASH_FUNC_SHA512:
			*type = MHASH_SHA512;
			break;
		case HASH_FUNC_SNEFRU128:
			*type = MHASH_SNEFRU128;
			break;
		case HASH_FUNC_SNEFRU256:
			*type = MHASH_SNEFRU256;
			break;
		case HASH_FUNC_TIGER128:
			*type = MHASH_TIGER128;
			break;
		case HASH_FUNC_TIGER160:
			*type = MHASH_TIGER160;
			break;
		case HASH_FUNC_TIGER192:
			*type = MHASH_TIGER192;
			break;
		case HASH_FUNC_WHIRLPOOL:
			*type = MHASH_WHIRLPOOL;
			break;
		default:
			return false;
	}

	return true;
}

bool gtkhash_hash_lib_mhash_is_supported(const enum hash_func_e id)
{
	struct hash_lib_mhash_s data;

	if (!gtkhash_hash_lib_mhash_set_type(id, &data.type))
		return false;

	if (G_UNLIKELY((data.thread = mhash_init(data.type)) == MHASH_FAILED)) {
		g_warning("mhash_init failed (%d)", id);
		return false;
	}

	mhash_deinit(data.thread, NULL);

	return true;
}

void gtkhash_hash_lib_mhash_start(struct hash_func_s *func)
{
	func->lib_data = g_new(struct hash_lib_mhash_s, 1);

	if (!gtkhash_hash_lib_mhash_set_type(func->id, &LIB_DATA->type))
		g_assert_not_reached();

	LIB_DATA->thread = mhash_init(LIB_DATA->type);
	g_assert(LIB_DATA->thread != MHASH_FAILED);
}

void gtkhash_hash_lib_mhash_update(struct hash_func_s *func,
	const uint8_t *buffer, const size_t size)
{
	mhash(LIB_DATA->thread, buffer, size);
}

void gtkhash_hash_lib_mhash_stop(struct hash_func_s *func)
{
	mhash_deinit(LIB_DATA->thread, NULL);
	g_free(LIB_DATA);
}

uint8_t *gtkhash_hash_lib_mhash_finish(struct hash_func_s *func, size_t *size)
{
	uint8_t *digest = mhash_end_m(LIB_DATA->thread, g_malloc);
	*size = mhash_get_block_size(LIB_DATA->type);

	g_free(LIB_DATA);

	return digest;
}
