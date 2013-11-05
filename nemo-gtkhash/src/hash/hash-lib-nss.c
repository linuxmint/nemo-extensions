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
#include <nss.h>
#include <pk11pub.h>

#include "hash-lib-nss.h"
#include "hash-lib.h"
#include "hash-func.h"

#define LIB_DATA ((struct hash_lib_nss_s *)func->lib_data)

struct hash_lib_nss_s {
	NSSInitContext *nss;
	PK11Context *pk11;
	SECOidTag alg;
};

static bool gtkhash_hash_lib_nss_set_alg(const enum hash_func_e id,
	SECOidTag *alg)
{
	switch (id) {
		case HASH_FUNC_MD2:
			*alg = SEC_OID_MD2;
			break;
		case HASH_FUNC_MD5:
			*alg = SEC_OID_MD5;
			break;
		case HASH_FUNC_SHA1:
			*alg = SEC_OID_SHA1;
			break;
		case HASH_FUNC_SHA256:
			*alg = SEC_OID_SHA256;
			break;
		case HASH_FUNC_SHA384:
			*alg = SEC_OID_SHA384;
			break;
		case HASH_FUNC_SHA512:
			*alg = SEC_OID_SHA512;
			break;
		default:
			return false;
	}

	return true;
}

static NSSInitContext *gtkhash_hash_lib_nss_init_context(void)
{
	PRUint32 flags = NSS_INIT_READONLY | NSS_INIT_NOCERTDB | NSS_INIT_NOMODDB;
	return NSS_InitContext(NULL, "", "", "", NULL, flags);
}

bool gtkhash_hash_lib_nss_is_supported(const enum hash_func_e id)
{
	struct hash_lib_nss_s data;

	if (!gtkhash_hash_lib_nss_set_alg(id, &data.alg))
		return false;

	if (G_UNLIKELY(!(data.nss = gtkhash_hash_lib_nss_init_context()))) {
		g_warning("NSS_InitContext failed (%d)", id);
		return false;
	}

	if (G_UNLIKELY(!(data.pk11 = PK11_CreateDigestContext(data.alg)))) {
		g_warning("PK11_CreateDigestContext failed (%d)", id);
		NSS_ShutdownContext(data.nss);
		return false;
	}

	PK11_DestroyContext(data.pk11, PR_TRUE);
	NSS_ShutdownContext(data.nss);

	return true;
}

void gtkhash_hash_lib_nss_start(struct hash_func_s *func)
{
	func->lib_data = g_new(struct hash_lib_nss_s, 1);

	if (G_UNLIKELY(!gtkhash_hash_lib_nss_set_alg(func->id, &LIB_DATA->alg)))
		g_assert_not_reached();

	LIB_DATA->nss = gtkhash_hash_lib_nss_init_context();
	g_assert(LIB_DATA->nss);

	LIB_DATA->pk11 = PK11_CreateDigestContext(LIB_DATA->alg);
	g_assert(LIB_DATA->pk11);

	SECStatus s = PK11_DigestBegin(LIB_DATA->pk11);
	(void)s;
	g_assert(s == SECSuccess);
}

void gtkhash_hash_lib_nss_update(struct hash_func_s *func,
	const uint8_t *buffer, const size_t size)
{
	SECStatus s = PK11_DigestOp(LIB_DATA->pk11, buffer, size);
	(void)s;
	g_assert(s == SECSuccess);
}

void gtkhash_hash_lib_nss_stop(struct hash_func_s *func)
{
	PK11_DestroyContext(LIB_DATA->pk11, PR_TRUE);
	NSS_ShutdownContext(LIB_DATA->nss);
	g_free(LIB_DATA);
}

uint8_t *gtkhash_hash_lib_nss_finish(struct hash_func_s *func, size_t *size)
{
	uint8_t buf[64 + 1];
	unsigned int len = 0;

	SECStatus s = PK11_DigestFinal(LIB_DATA->pk11, buf, &len, sizeof(buf));
	(void)s;
	g_assert(s == SECSuccess);
	g_assert(len < sizeof(buf));

	PK11_DestroyContext(LIB_DATA->pk11, PR_TRUE);
	NSS_ShutdownContext(LIB_DATA->nss);
	g_free(LIB_DATA);

	uint8_t *digest = g_memdup(buf, len);
	*size = len;

	return digest;
}
