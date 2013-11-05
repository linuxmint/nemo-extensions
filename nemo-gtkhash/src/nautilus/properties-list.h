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

#ifndef GTKHASH_NAUTILUS_PROPERTIES_LIST_H
#define GTKHASH_NAUTILUS_PROPERTIES_LIST_H

#include "properties.h"

void gtkhash_properties_list_update_enabled(struct page_s *page, char *path_str);
void gtkhash_properties_list_update_digests(struct page_s *page);
void gtkhash_properties_list_check_digests(struct page_s *page);
char *gtkhash_properties_list_get_selected_digest(struct page_s *page);
void gtkhash_properties_list_refilter(struct page_s *page);
void gtkhash_properties_list_init(struct page_s *page);

#endif
