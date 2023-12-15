/*
 * Nemo Filename Repairer Extension
 *
 * Copyright (C) 2008 Choe Hwanjin
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Author: Choe Hwajin <choe.hwanjin@gmail.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include "nemo-filename-repairer.h"

static GType type_list[1];

void
nemo_module_initialize(GTypeModule *module)
{
    nemo_filename_repairer_register_type(module);
    type_list[0] = NEMO_TYPE_RENAME_AS;

    bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

    nemo_filename_repairer_on_module_init();
}

void
nemo_module_shutdown(void)
{
    nemo_filename_repairer_on_module_shutdown();
}

void 
nemo_module_list_types(const GType **types,
			   int          *num_types)
{
    *types = type_list;
    *num_types = G_N_ELEMENTS (type_list);
}
