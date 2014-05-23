/*
 *  nemo-wipe - a nemo extension to wipe file(s)
 * 
 *  Copyright (C) 2009-2012 Colomban Wendling <ban@herbesfolles.org>
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

#ifndef NW_DELETE_OPERATION_H
#define NW_DELETE_OPERATION_H

#include <glib.h>
#include <glib-object.h>
#include <gsecuredelete/gsecuredelete.h>

#include "nw-operation.h"

G_BEGIN_DECLS


#define NW_TYPE_DELETE_OPERATION         (nw_delete_operation_get_type ())
#define NW_DELETE_OPERATION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), NW_TYPE_DELETE_OPERATION, NwDeleteOperation))
#define NW_DELETE_OPERATION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), NW_TYPE_DELETE_OPERATION, NwDeleteOperationClass))
#define NW_IS_DELETE_OPERATION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), NW_TYPE_DELETE_OPERATION))
#define NW_IS_DELETE_OPERATION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), NW_TYPE_DELETE_OPERATION))
#define NW_DELETE_OPERATION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), NW_TYPE_DELETE_OPERATION, NwDeleteOperationClass))

typedef struct _NwDeleteOperation         NwDeleteOperation;
typedef struct _NwDeleteOperationClass    NwDeleteOperationClass;

struct _NwDeleteOperation {
  GsdDeleteOperation parent;
};

struct _NwDeleteOperationClass {
  GsdDeleteOperationClass parent;
};


GType         nw_delete_operation_get_type    (void) G_GNUC_CONST;
NwOperation  *nw_delete_operation_new         (void);


G_END_DECLS

#endif /* guard */
