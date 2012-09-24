/* nemo-share -- Nemo File Sharing Extension
 *
 * Sebastien Estienne <sebastien.estienne@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * (C) Copyright 2005 Ethium, Inc.
 */

#ifndef NEMO_SHARE_H
#define NEMO_SHARE_H

#include <glib-object.h>

G_BEGIN_DECLS

/* Declarations for the Share extension object.  This object will be
 * instantiated by nemo.  It implements the GInterfaces
 * exported by libnemo. */


typedef struct _NemoShare      NemoShare;
typedef struct _NemoShareClass NemoShareClass;

struct _NemoShare {
	GObject parent_slot;
};

struct _NemoShareClass {
	GObjectClass parent_slot;

	/* No extra class members */
};


typedef struct _NemoShareData      NemoShareData;

struct _NemoShareData {
  gchar		*fullpath;
  gchar		*section;
  NemoFileInfo *fileinfo;
};

G_END_DECLS

typedef enum {
  NEMO_SHARE_NOT_SHARED,
  NEMO_SHARE_SHARED_RO,
  NEMO_SHARE_SHARED_RW
} NemoShareStatus;

#endif
 
