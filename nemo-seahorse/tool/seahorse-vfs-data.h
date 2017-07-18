/*
 * Seahorse
 *
 * Copyright (C) 2004 Stefan Walter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <gio/gio.h>

#include <gpgme.h>

/**
 * A gpgme_data_t implementation which maps to a gnome-vfs handle.
 * Allows for accessing data on remote machines (ie: smb, sftp)
 */

#ifndef __SEAHORSE_VFS_DATA__
#define __SEAHORSE_VFS_DATA__

/*
 * HACKY: GPGME progress sucks, and doesn't update properly. So we
 * have a progress callback that can be hooked into the reading or
 * writing of a file.
 */

typedef void (*SeahorseVfsProgressCb) (gpgme_data_t data, goffset pos,
                                       gpointer userdata);

#define SEAHORSE_VFS_READ   0x00000000
#define SEAHORSE_VFS_WRITE  0x00000001
#define SEAHORSE_VFS_DELAY  0x00000010

gpgme_data_t        seahorse_vfs_data_create        (const gchar *uri, guint mode,
                                                     GError **err);

gpgme_data_t        seahorse_vfs_data_create_full   (GFile *file, guint mode,
                                                     SeahorseVfsProgressCb progcb,
                                                     gpointer userdata, GError **err);

#endif /* __SEAHORSE_VFS_DATA__ */
