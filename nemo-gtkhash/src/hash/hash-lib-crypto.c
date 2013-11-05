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
#include <openssl/evp.h>

#include "hash-lib-crypto.h"
#include "hash-lib.h"
#include "hash-func.h"

#define LIB_DATA ((struct hash_lib_crypto_s *)func->lib_data)

struct hash_lib_crypto_s {
	EVP_MD_CTX ctx;
};

static const EVP_MD *gtkhash_hash_lib_crypto_get_md(const enum hash_func_e id)
{
	switch (id) {
		case HASH_FUNC_MD4:
			return EVP_md4();
		case HASH_FUNC_MD5:
			return EVP_md5();
		case HASH_FUNC_SHA0:
			return EVP_sha();
		case HASH_FUNC_SHA1:
			return EVP_sha1();
		case HASH_FUNC_RIPEMD160:
			return EVP_ripemd160();
		case HASH_FUNC_MDC2:
			return EVP_mdc2();
		default:
			return NULL;
	}

	g_assert_not_reached();
}

bool gtkhash_hash_lib_crypto_is_supported(const enum hash_func_e id)
{
	struct hash_lib_crypto_s data;
	const EVP_MD *md;

	OpenSSL_add_all_digests();

	if (!(md = gtkhash_hash_lib_crypto_get_md(id)))
		return false;

	EVP_MD_CTX_init(&data.ctx);
	bool ret = (EVP_DigestInit_ex(&data.ctx, md, NULL) == 1);
	EVP_MD_CTX_cleanup(&data.ctx);

	return ret;
}

void gtkhash_hash_lib_crypto_start(struct hash_func_s *func)
{
	func->lib_data = g_new(struct hash_lib_crypto_s, 1);
	const EVP_MD *md;

	if (!(md = gtkhash_hash_lib_crypto_get_md(func->id)))
		g_assert_not_reached();

	EVP_MD_CTX_init(&LIB_DATA->ctx);

	if (EVP_DigestInit_ex(&LIB_DATA->ctx, md, NULL) != 1)
		g_assert_not_reached();
}

void gtkhash_hash_lib_crypto_update(struct hash_func_s *func,
	const uint8_t *buffer, const size_t size)
{
	EVP_DigestUpdate(&LIB_DATA->ctx, buffer, size);
}

void gtkhash_hash_lib_crypto_stop(struct hash_func_s *func)
{
	EVP_MD_CTX_cleanup(&LIB_DATA->ctx);
	g_free(LIB_DATA);
}

uint8_t *gtkhash_hash_lib_crypto_finish(struct hash_func_s *func, size_t *size)
{
	*size = EVP_MD_CTX_size(&LIB_DATA->ctx);
	g_assert(*size > 0);

	uint8_t *digest = g_malloc0(*size);

	unsigned int len;
	if (EVP_DigestFinal_ex(&LIB_DATA->ctx, digest, &len) != 1)
		g_assert_not_reached();
	g_assert(*size == len);

	EVP_MD_CTX_cleanup(&LIB_DATA->ctx);
	g_free(LIB_DATA);

	return digest;
}
