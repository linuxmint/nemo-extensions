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
#include <polarssl/md.h>

#include "hash-lib-polarssl.h"
#include "hash-lib.h"
#include "hash-func.h"

#define LIB_DATA ((struct hash_lib_polarssl_s *)func->lib_data)

struct hash_lib_polarssl_s {
	md_context_t ctx;
};

static bool gtkhash_hash_lib_polarssl_set_type(const enum hash_func_e id, md_type_t *type)
{
	switch (id) {
		case HASH_FUNC_MD2:
			*type = POLARSSL_MD_MD2;
			break;
		case HASH_FUNC_MD4:
			*type = POLARSSL_MD_MD4;
			break;
		case HASH_FUNC_MD5:
			*type = POLARSSL_MD_MD5;
			break;
		case HASH_FUNC_SHA1:
			*type = POLARSSL_MD_SHA1;
			break;
		case HASH_FUNC_SHA224:
			*type = POLARSSL_MD_SHA224;
			break;
		case HASH_FUNC_SHA256:
			*type = POLARSSL_MD_SHA256;
			break;
		case HASH_FUNC_SHA384:
			*type = POLARSSL_MD_SHA384;
			break;
		case HASH_FUNC_SHA512:
			*type = POLARSSL_MD_SHA512;
			break;
		default:
			return false;
	}

	return true;
}

bool gtkhash_hash_lib_polarssl_is_supported(const enum hash_func_e id)
{
	struct hash_lib_polarssl_s data;
	md_type_t type;

	if (!gtkhash_hash_lib_polarssl_set_type(id, &type))
		return false;

	if (md_init_ctx(&data.ctx, md_info_from_type(type)) != 0)
		return false;

	if (md_free_ctx(&data.ctx) != 0) {
		g_assert_not_reached();
		return false;
	}

	return true;
}

void gtkhash_hash_lib_polarssl_start(struct hash_func_s *func)
{
	func->lib_data = g_new(struct hash_lib_polarssl_s, 1);
	md_type_t type;

	if (!gtkhash_hash_lib_polarssl_set_type(func->id, &type))
		g_assert_not_reached();

	if (md_init_ctx(&LIB_DATA->ctx, md_info_from_type(type)) != 0)
		g_assert_not_reached();

	if (md_starts(&LIB_DATA->ctx) != 0)
		g_assert_not_reached();
}

void gtkhash_hash_lib_polarssl_update(struct hash_func_s *func,
	const uint8_t *buffer, const size_t size)
{
	md_update(&LIB_DATA->ctx, buffer, size);
}

void gtkhash_hash_lib_polarssl_stop(struct hash_func_s *func)
{
	if (md_free_ctx(&LIB_DATA->ctx) != 0)
		g_assert_not_reached();
	g_free(LIB_DATA);
}

uint8_t *gtkhash_hash_lib_polarssl_finish(struct hash_func_s *func,
	size_t *size)
{
	*size = LIB_DATA->ctx.md_info->size;
	uint8_t *digest = g_malloc(*size);

	if (md_finish(&LIB_DATA->ctx, digest) != 0)
		g_assert_not_reached();

	if (md_free_ctx(&LIB_DATA->ctx) != 0)
		g_assert_not_reached();
	g_free(LIB_DATA);

	return digest;
}
