/*
 *  nemo-wipe - a nemo extension to wipe file(s)
 * 
 *  Copyright (C) 2009-2011 Colomban Wendling <ban@herbesfolles.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef NW_PATH_LIST_H
#define NW_PATH_LIST_H

#include <glib.h>

#include <libnemo-extension/nemo-file-info.h>

G_BEGIN_DECLS


gchar  *nw_path_from_nfi                (NemoFileInfo *nfi);
GList  *nw_path_list_new_from_nfi_list  (GList *nfis);
void    nw_path_list_free               (GList *paths);
GList  *nw_path_list_copy               (GList *src);


G_END_DECLS

#endif /* guard */
