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
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <glib.h>

#include "digest.h"

static char *gtkhash_digest_get_lc_hex(struct digest_s *digest)
{
	char *ret = g_malloc0((digest->size * 2) + 1);

	for (size_t i = 0; i < digest->size; i++)
		snprintf(ret + (i * 2), 3, "%.2x", digest->bin[i]);

	return ret;
}

static char *gtkhash_digest_get_uc_hex(struct digest_s *digest)
{
	char *ret = gtkhash_digest_get_lc_hex(digest);

	for (char *c = ret; *c; c++)
		*c = g_ascii_toupper(*c);

	return ret;
}

static char *gtkhash_digest_get_base64(struct digest_s *digest)
{
	return g_base64_encode(digest->bin, digest->size);
}

struct digest_s *gtkhash_digest_new(void)
{
	struct digest_s *digest = g_new(struct digest_s, 1);
	digest->bin = NULL;
	digest->size = 0;

	for (int i = 0; i < DIGEST_FORMATS_N; i++)
		digest->data[i] = NULL;

	return digest;
}

void gtkhash_digest_set_data(struct digest_s *digest, uint8_t *bin,
	size_t size)
{
	g_assert(digest);
	g_assert(bin);
	g_assert(size);

	gtkhash_digest_free_data(digest);

	digest->bin = bin;
	digest->size = size;

	digest->data[DIGEST_FORMAT_HEX_LOWER] = gtkhash_digest_get_lc_hex(digest);
	digest->data[DIGEST_FORMAT_HEX_UPPER] = gtkhash_digest_get_uc_hex(digest);
	digest->data[DIGEST_FORMAT_BASE64] = gtkhash_digest_get_base64(digest);
}

const char *gtkhash_digest_get_data(struct digest_s *digest,
	const enum digest_format_e format)
{
	g_assert(digest);
	g_assert(DIGEST_FORMAT_IS_VALID(format));

	return digest->data[format];
}

void gtkhash_digest_free_data(struct digest_s *digest)
{
	g_assert(digest);

	if (digest->bin) {
		g_free(digest->bin);
		digest->bin = NULL;
	}

	digest->size = 0;

	for (int i = 0; i < DIGEST_FORMATS_N; i++) {
		if (digest->data[i]) {
			g_free(digest->data[i]);
			digest->data[i] = NULL;
		}
	}
}

void gtkhash_digest_free(struct digest_s *digest)
{
	g_assert(digest);

	gtkhash_digest_free_data(digest);

	g_free(digest);
}
