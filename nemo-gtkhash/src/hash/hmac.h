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

#ifndef GTKHASH_HASH_HMAC_H
#define GTKHASH_HASH_HMAC_H

#ifndef IN_HASH_LIB
	#error "don't use directly"
#endif

#include <stdlib.h>
#include <stdint.h>

#include "hash-func.h"

void gtkhash_hmac_start(struct hash_func_s *func, const uint8_t *key,
	const size_t key_size);
void gtkhash_hmac_stop(struct hash_func_s *func);
void gtkhash_hmac_finish(struct hash_func_s *func);

#endif
