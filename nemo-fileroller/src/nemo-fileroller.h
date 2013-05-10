/*
 *  File-Roller
 * 
 *  Copyright (C) 2004 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Author: Paolo Bacchilega <paobac@cvs.gnome.org>
 * 
 */

#ifndef NEMO_FILEROLLER_H
#define NEMO_FILEROLLER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define NEMO_TYPE_FR  (nemo_fr_get_type ())
#define NEMO_FR(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), NEMO_TYPE_FR, NemoFr))
#define NEMO_IS_FR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), NEMO_TYPE_FR))

typedef struct _NemoFr      NemoFr;
typedef struct _NemoFrClass NemoFrClass;

struct _NemoFr {
	GObject __parent;
};

struct _NemoFrClass {
	GObjectClass __parent;
};

GType nemo_fr_get_type      (void);
void  nemo_fr_register_type (GTypeModule *module);

G_END_DECLS

#endif /* NEMO_FILEROLLER_H */
