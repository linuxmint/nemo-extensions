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

#ifndef NW_OPERATION_MANAGER_H
#define NW_OPERATION_MANAGER_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "nw-operation.h"

G_BEGIN_DECLS


void    nw_operation_manager_run  (GtkWindow   *parent,
                                   GList       *files,
                                   const gchar *confirm_primary_text,
                                   const gchar *confirm_secondary_text,
                                   const gchar *confirm_button_text,
                                   GtkWidget   *confirm_button_icon,
                                   const gchar *progress_dialog_text,
                                   NwOperation *operation,
                                   const gchar *failed_primary_text,
                                   const gchar *success_primary_text,
                                   const gchar *success_secondary_text);


G_END_DECLS

#endif /* guard */
