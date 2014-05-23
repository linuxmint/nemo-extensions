/*
 *  nemo-wipe - a nemo extension to wipe file(s)
 * 
 *  Copyright (C) 2012 Colomban Wendling <ban@herbesfolles.org>
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

#ifndef NW_OPERATION_H
#define NW_OPERATION_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS


#define NW_TYPE_OPERATION             (nw_operation_get_type ())
#define NW_OPERATION(o)               (G_TYPE_CHECK_INSTANCE_CAST ((o), NW_TYPE_OPERATION, NwOperation))
#define NW_IS_OPERATION(o)            (G_TYPE_CHECK_INSTANCE_TYPE ((o), NW_TYPE_OPERATION))
#define NW_OPERATION_GET_INTERFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), NW_TYPE_OPERATION, NwOperationInterface))

typedef struct _NwOperation           NwOperation;
typedef struct _NwOperationInterface  NwOperationInterface;

struct _NwOperationInterface {
  GTypeInterface parent;
  
  void   (*add_file)        (NwOperation *self,
                             const gchar *path);
  void   (*add_files)       (NwOperation *self,
                             GList       *files);
};


GType   nw_operation_get_type   (void) G_GNUC_CONST;

void    nw_operation_add_file   (NwOperation *self,
                                 const gchar *path);
void    nw_operation_add_files  (NwOperation *self,
                                 GList       *files);


G_END_DECLS

#endif /* guard */
