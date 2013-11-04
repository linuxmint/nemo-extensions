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

#ifndef GTKHASH_NAUTILUS_RROPERTIES_HASH_H
#define GTKHASH_NAUTILUS_PROPERTIES_HASH_H

#include "properties.h"

void gtkhash_properties_hash_start(struct page_s *page, const uint8_t *hmac_key,
	const size_t key_size);
void gtkhash_properties_hash_stop(struct page_s *page);
int gtkhash_properties_hash_funcs_supported(struct page_s *page);
void gtkhash_properties_hash_init(struct page_s *page);
void gtkhash_properties_hash_deinit(struct page_s *page);

#endif
