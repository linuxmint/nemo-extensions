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

#include "md6/md6.h"

#include "hash-lib-md6.h"
#include "hash-lib.h"
#include "hash-func.h"

#define LIB_DATA ((struct hash_lib_md6_s *)func->lib_data)

struct hash_lib_md6_s {
	md6_state state;
};

bool gtkhash_hash_lib_md6_is_supported(const enum hash_func_e id)
{
	switch (id) {
		case HASH_FUNC_MD6_224:
		case HASH_FUNC_MD6_256:
		case HASH_FUNC_MD6_384:
		case HASH_FUNC_MD6_512:
			return true;
		default:
			return false;
	}
}

void gtkhash_hash_lib_md6_start(struct hash_func_s *func)
{
	func->lib_data = g_new(struct hash_lib_md6_s, 1);

	md6_init(&LIB_DATA->state, func->digest_size * 8);
	LIB_DATA->state.hashbitlen = func->digest_size * 8;
}

void gtkhash_hash_lib_md6_update(struct hash_func_s *func,
	const uint8_t *buffer, const size_t size)
{
	md6_update(&LIB_DATA->state, (unsigned char *)buffer, size * 8);
}

void gtkhash_hash_lib_md6_stop(struct hash_func_s *func)
{
	g_free(LIB_DATA);
}

uint8_t *gtkhash_hash_lib_md6_finish(struct hash_func_s *func, size_t *size)
{
	md6_final(&LIB_DATA->state, NULL);

	*size = LIB_DATA->state.hashbitlen / 8;
	uint8_t *digest = g_memdup(&LIB_DATA->state.hashval, *size);

	g_free(LIB_DATA);

	return digest;
}
