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

/* Nemo extension */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "nw-extension.h"

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>


static GType provider_types[1];

/* initialization */
void
nemo_module_initialize (GTypeModule *module)
{
  g_message ("Initializing");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  provider_types[0] = nw_extension_register_type (module);
}

/* extension points types registration */
void
nemo_module_list_types (const GType **types,
                            int          *num_types)
{
  *types = provider_types;
  *num_types = G_N_ELEMENTS (provider_types);
}

/* cleanup */
void
nemo_module_shutdown (void)
{
}
