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

#ifndef NW_EXTENSION_H
#define NW_EXTENSION_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS


#define NW_TYPE_EXTENSION   (nw_extension_get_type ())
#define NW_EXTENSION(o)     (G_TYPE_CHECK_INSTANCE_CAST ((o), NW_TYPE_EXTENSION, NwExtension))
#define NW_IS_EXTENSION(o)  (G_TYPE_CHECK_INSTANCE_TYPE ((o), NW_TYPE_EXTENSION))
typedef struct _NwExtension       NwExtension;
typedef struct _NwExtensionClass  NwExtensionClass;

#define NW_EXTENSION_ERROR (nw_extension_error_quark ())

typedef enum {
  NW_EXTENSION_ERROR_SPAWN_FAILED,
  NW_EXTENSION_ERROR_CHILD_CRASHED,
  NW_EXTENSION_ERROR_CHILD_FAILED,
  NW_EXTENSION_ERROR_UNSUPPORTED_LOCATION,
  NW_EXTENSION_ERROR_NOT_IMPLEMENTED,
  NW_EXTENSION_ERROR_FAILED
} NwExtensionError;

struct _NwExtension {
  GObject parent_slot;
};

struct _NwExtensionClass {
  GObjectClass parent_slot;
};

GType   nw_extension_get_type         (void) G_GNUC_CONST;
GType   nw_extension_register_type    (GTypeModule *module);
GQuark  nw_extension_error_quark      (void) G_GNUC_CONST;


G_END_DECLS

#endif /* guard */
