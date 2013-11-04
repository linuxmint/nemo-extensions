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

#ifndef GTKHASH_HASH_DIGEST_H
#define GTKHASH_HASH_DIGEST_H

#ifndef IN_HASH_LIB
	#error "don't use directly"
#endif

#include <stdlib.h>
#include <stdint.h>

#include "digest-format.h"

struct digest_s {
	uint8_t *bin;
	size_t size;
	char *data[DIGEST_FORMATS_N];
};

struct digest_s *gtkhash_digest_new(void);
void gtkhash_digest_set_data(struct digest_s *digest, uint8_t *bin,
	size_t size);
const char *gtkhash_digest_get_data(struct digest_s *digest,
	const enum digest_format_e format);
void gtkhash_digest_free_data(struct digest_s *digest);
void gtkhash_digest_free(struct digest_s *digest);

#endif
