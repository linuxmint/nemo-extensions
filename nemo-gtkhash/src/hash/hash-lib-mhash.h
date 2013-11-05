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

#ifndef GTKHASH_HASH_HASH_LIB_MHASH_H
#define GTKHASH_HASH_HASH_LIB_MHASH_H

#ifndef IN_HASH_LIB
	#error "don't use directly"
#endif

#include <stdlib.h>
#include <stdint.h>

#include "hash-func.h"

bool gtkhash_hash_lib_mhash_is_supported(const enum hash_func_e id);
void gtkhash_hash_lib_mhash_start(struct hash_func_s *func);
void gtkhash_hash_lib_mhash_update(struct hash_func_s *func,
	const uint8_t *buffer, const size_t size);
void gtkhash_hash_lib_mhash_stop(struct hash_func_s *func);
uint8_t *gtkhash_hash_lib_mhash_finish(struct hash_func_s *func, size_t *size);

#endif
