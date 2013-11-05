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
#include <gcrypt.h>

#include "hash-lib-gcrypt.h"
#include "hash-lib.h"
#include "hash-func.h"

#define LIB_DATA ((struct hash_lib_gcrypt_s *)func->lib_data)

struct hash_lib_gcrypt_s {
	gcry_md_hd_t h;
	int algo;
};

static bool gtkhash_hash_lib_gcrypt_set_algo(const enum hash_func_e id,
	int *algo)
{
	switch (id) {
		case HASH_FUNC_MD4:
			*algo = GCRY_MD_MD4;
			break;
		case HASH_FUNC_MD5:
			*algo = GCRY_MD_MD5;
			break;
		case HASH_FUNC_RIPEMD160:
			*algo = GCRY_MD_RMD160;
			break;
		case HASH_FUNC_SHA1:
			*algo = GCRY_MD_SHA1;
			break;
		case HASH_FUNC_SHA224:
			*algo = GCRY_MD_SHA224;
			break;
		case HASH_FUNC_SHA256:
			*algo = GCRY_MD_SHA256;
			break;
		case HASH_FUNC_SHA384:
			*algo = GCRY_MD_SHA384;
			break;
		case HASH_FUNC_SHA512:
			*algo = GCRY_MD_SHA512;
			break;
		case HASH_FUNC_TIGER192:
			*algo = GCRY_MD_TIGER;
			break;
		case HASH_FUNC_WHIRLPOOL:
			*algo = GCRY_MD_WHIRLPOOL;
			break;
		default:
			return false;
	}

	return true;
}

bool gtkhash_hash_lib_gcrypt_is_supported(const enum hash_func_e id)
{
	struct hash_lib_gcrypt_s data;

	if (!gtkhash_hash_lib_gcrypt_set_algo(id, &data.algo))
		return false;

	if (G_UNLIKELY(gcry_md_open(&data.h, data.algo, 0) != GPG_ERR_NO_ERROR)) {
		g_warning("gcry_md_open failed (%d)", id);
		return false;
	}

	gcry_md_close(data.h);

	return true;
}

void gtkhash_hash_lib_gcrypt_start(struct hash_func_s *func)
{
	func->lib_data = g_new(struct hash_lib_gcrypt_s, 1);

	if (!gtkhash_hash_lib_gcrypt_set_algo(func->id, &LIB_DATA->algo))
		g_assert_not_reached();

	if (gcry_md_open(&LIB_DATA->h, LIB_DATA->algo, 0) != GPG_ERR_NO_ERROR)
		g_assert_not_reached();
}

void gtkhash_hash_lib_gcrypt_update(struct hash_func_s *func,
	const uint8_t *buffer, const size_t size)
{
	gcry_md_write(LIB_DATA->h, buffer, size);
}

void gtkhash_hash_lib_gcrypt_stop(struct hash_func_s *func)
{
	gcry_md_close(LIB_DATA->h);
	g_free(LIB_DATA);
}

uint8_t *gtkhash_hash_lib_gcrypt_finish(struct hash_func_s *func, size_t *size)
{
	unsigned char *digest_tmp = gcry_md_read(LIB_DATA->h, LIB_DATA->algo);
	*size = gcry_md_get_algo_dlen(LIB_DATA->algo);
	uint8_t *digest = g_memdup(digest_tmp, *size);

	gcry_md_close(LIB_DATA->h);
	g_free(LIB_DATA);

	return digest;
}
