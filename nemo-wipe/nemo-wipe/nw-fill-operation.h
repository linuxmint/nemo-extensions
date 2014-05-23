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

#ifndef NW_FILL_OPERATION_H
#define NW_FILL_OPERATION_H

#include <glib.h>
#include <glib-object.h>
#include <gsecuredelete/gsecuredelete.h>

#include "nw-operation.h"

G_BEGIN_DECLS


#define NW_TYPE_FILL_OPERATION          (nw_fill_operation_get_type ())
#define NW_FILL_OPERATION(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), NW_TYPE_FILL_OPERATION, NwFillOperation))
#define NW_FILL_OPERATION_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST ((k), NW_TYPE_FILL_OPERATION, NwFillOperationClass))
#define NW_IS_FILL_OPERATION(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), NW_TYPE_FILL_OPERATION))
#define NW_IS_FILL_OPERATION_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), NW_TYPE_FILL_OPERATION))
#define NW_FILL_OPERATION_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), NW_TYPE_FILL_OPERATION, NwFillOperationClass))

typedef struct _NwFillOperation         NwFillOperation;
typedef struct _NwFillOperationClass    NwFillOperationClass;
typedef struct _NwFillOperationPrivate  NwFillOperationPrivate;

struct _NwFillOperation {
  GsdFillOperation parent;
  NwFillOperationPrivate *priv;
};

struct _NwFillOperationClass {
  GsdFillOperationClass parent;
};

/**
 * NwFillOperationError:
 * @NW_FILL_OPERATION_ERROR_MISSING_MOUNT: A file have no mount
 * @NW_FILL_OPERATION_ERROR_REMOTE_MOUNT: A mount is not local
 * @NW_FILL_OPERATION_ERROR_FAILED: An error occurred
 * 
 * Possible errors from the %NW_FILL_OPERATION_ERROR domain.
 */
typedef enum
{
  NW_FILL_OPERATION_ERROR_MISSING_MOUNT,
  NW_FILL_OPERATION_ERROR_REMOTE_MOUNT,
  NW_FILL_OPERATION_ERROR_FAILED
} NwFillOperationError;

/**
 * NW_FILL_OPERATION_ERROR:
 * 
 * Domain for error coming from a NemoWipe's fill operation.
 */
#define NW_FILL_OPERATION_ERROR (nw_fill_operation_error_quark ())

GQuark        nw_fill_operation_error_quark   (void) G_GNUC_CONST;
GType         nw_fill_operation_get_type      (void) G_GNUC_CONST;

gboolean      nw_fill_operation_filter_files  (GList    *paths,
                                               GList   **work_paths_,
                                               GList   **work_mounts_,
                                               GError  **error);
NwOperation  *nw_fill_operation_new           (void);


G_END_DECLS

#endif /* guard */
