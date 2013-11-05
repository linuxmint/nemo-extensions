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

#ifndef GTKHASH_LIST_H
#define GTKHASH_LIST_H

#include "hash/hash-func.h"

void list_init(void);
void list_update(void);
void list_append_row(const char *uri);
void list_remove_selection(void);
char *list_get_uri(const unsigned int row);
GSList *list_get_all_uris(void);
unsigned int list_count_rows(void);
void list_set_digest(const char *uri, const enum hash_func_e id,
	const char *digest);
char *list_get_digest(const unsigned int row, const enum hash_func_e id);
char *list_get_selected_digest(const enum hash_func_e id);
void list_clear_digests(void);
void list_clear(void);

#endif
