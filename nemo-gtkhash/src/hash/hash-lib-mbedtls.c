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
#if HAVE_MBEDTLS_2_0_0
#include <mbedtls/md.h>
#else
#include <polarssl/md.h>
#endif

#include "hash-lib-mbedtls.h"
#include "hash-lib.h"
#include "hash-func.h"

#define LIB_DATA ((struct hash_lib_mbedtls_s *)func->lib_data)

struct hash_lib_mbedtls_s {
#if HAVE_MBEDTLS_2_0_0
	mbedtls_md_context_t ctx;
#else
	md_context_t ctx;
#endif
};

static bool gtkhash_hash_lib_mbedtls_set_type(const enum hash_func_e id,
#if HAVE_MBEDTLS_2_0_0
	mbedtls_md_type_t *type)
#else
	md_type_t *type)
#endif
{
	switch (id) {
		case HASH_FUNC_MD2:
#if HAVE_MBEDTLS_2_0_0
			*type = MBEDTLS_MD_MD2;
#else
			*type = POLARSSL_MD_MD2;
#endif
			break;
		case HASH_FUNC_MD4:
#if HAVE_MBEDTLS_2_0_0
			*type = MBEDTLS_MD_MD4;
#else
			*type = POLARSSL_MD_MD4;
#endif
			break;
		case HASH_FUNC_MD5:
#if HAVE_MBEDTLS_2_0_0
			*type = MBEDTLS_MD_MD5;
#else
			*type = POLARSSL_MD_MD5;
#endif
			break;
		case HASH_FUNC_RIPEMD160:
#if HAVE_MBEDTLS_2_0_0
			*type = MBEDTLS_MD_RIPEMD160;
#else
			*type = POLARSSL_MD_RIPEMD160;
#endif
			break;
		case HASH_FUNC_SHA1:
#if HAVE_MBEDTLS_2_0_0
			*type = MBEDTLS_MD_SHA1;
#else
			*type = POLARSSL_MD_SHA1;
#endif
			break;
		case HASH_FUNC_SHA224:
#if HAVE_MBEDTLS_2_0_0
			*type = MBEDTLS_MD_SHA224;
#else
			*type = POLARSSL_MD_SHA224;
#endif
			break;
		case HASH_FUNC_SHA256:
#if HAVE_MBEDTLS_2_0_0
			*type = MBEDTLS_MD_SHA256;
#else
			*type = POLARSSL_MD_SHA256;
#endif
			break;
		case HASH_FUNC_SHA384:
#if HAVE_MBEDTLS_2_0_0
			*type = MBEDTLS_MD_SHA384;
#else
			*type = POLARSSL_MD_SHA384;
#endif
			break;
		case HASH_FUNC_SHA512:
#if HAVE_MBEDTLS_2_0_0
			*type = MBEDTLS_MD_SHA512;
#else
			*type = POLARSSL_MD_SHA512;
#endif
			break;
		default:
			return false;
	}

	return true;
}


bool gtkhash_hash_lib_mbedtls_is_supported(const enum hash_func_e id)
{
#if HAVE_MBEDTLS_2_0_0
	mbedtls_md_type_t type;
#else
	md_type_t type;
#endif
	if (!gtkhash_hash_lib_mbedtls_set_type(id, &type))
		return false;

	struct hash_lib_mbedtls_s data;
#if HAVE_MBEDTLS_2_0_0
	mbedtls_md_init(&data.ctx);

	const mbedtls_md_info_t *info = mbedtls_md_info_from_type(type);
	if (mbedtls_md_setup(&data.ctx, info, 0) != 0) {
		mbedtls_md_free(&data.ctx);
		return false;
	}

	mbedtls_md_free(&data.ctx);
#else
	if (md_init_ctx(&data.ctx, md_info_from_type(type)) != 0)
		return false;

	if (md_free_ctx(&data.ctx) != 0) {
		g_assert_not_reached();
		return false;
	}
#endif

	return true;
}

void gtkhash_hash_lib_mbedtls_start(struct hash_func_s *func)
{
	func->lib_data = g_new(struct hash_lib_mbedtls_s, 1);
#if HAVE_MBEDTLS_2_0_0
	mbedtls_md_type_t type;
#else
	md_type_t type;
#endif

	if (!gtkhash_hash_lib_mbedtls_set_type(func->id, &type))
		g_assert_not_reached();

#if HAVE_MBEDTLS_2_0_0
	mbedtls_md_init(&LIB_DATA->ctx);

	const mbedtls_md_info_t *info = mbedtls_md_info_from_type(type);
	if (mbedtls_md_setup(&LIB_DATA->ctx, info, 0) != 0)
		g_assert_not_reached();

	if (mbedtls_md_starts(&LIB_DATA->ctx) != 0)
		g_assert_not_reached();

#else
	if (md_init_ctx(&LIB_DATA->ctx, md_info_from_type(type)) != 0)
		g_assert_not_reached();

	if (md_starts(&LIB_DATA->ctx) != 0)
		g_assert_not_reached();
#endif
}

void gtkhash_hash_lib_mbedtls_update(struct hash_func_s *func,
	const uint8_t *buffer, const size_t size)
{
#if HAVE_MBEDTLS_2_0_0
	mbedtls_md_update(&LIB_DATA->ctx, buffer, size);
#else
	md_update(&LIB_DATA->ctx, buffer, size);
#endif
}

void gtkhash_hash_lib_mbedtls_stop(struct hash_func_s *func)
{
#if HAVE_MBEDTLS_2_0_0
	mbedtls_md_free(&LIB_DATA->ctx);
#else
	if (md_free_ctx(&LIB_DATA->ctx) != 0)
		g_assert_not_reached();
#endif
	g_free(LIB_DATA);
}

uint8_t *gtkhash_hash_lib_mbedtls_finish(struct hash_func_s *func,
	size_t *size)
{
#if HAVE_MBEDTLS_2_0_0
	*size = mbedtls_md_get_size(LIB_DATA->ctx.md_info);
	uint8_t *digest = g_malloc(*size);

	if (mbedtls_md_finish(&LIB_DATA->ctx, digest) != 0)
		g_assert_not_reached();

	mbedtls_md_free(&LIB_DATA->ctx);
#else
	*size = LIB_DATA->ctx.md_info->size;
	uint8_t *digest = g_malloc(*size);

	if (md_finish(&LIB_DATA->ctx, digest) != 0)
		g_assert_not_reached();

	if (md_free_ctx(&LIB_DATA->ctx) != 0)
		g_assert_not_reached();
#endif
	g_free(LIB_DATA);

	return digest;
}
