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

#ifndef nemo_filename_repairer_h
#define nemo_filename_repairer_h

#include <glib.h>

#define NEMO_TYPE_RENAME_AS          (nemo_filename_repairer_get_type())
#define NEMO_RENAME_AS(obj)          (G_TYPE_CHECK_INSTANCE_CAST((obj), NEMO_TYPE_RENAME_AS, NemoFilenameRepairer))
#define NEMO_IS_RENAME_AS(obj)       (G_TYPE_CHECK_INSTANCE_TYPE((obj), NEMO_TYPE_RENAME_AS))

typedef struct _NemoFilenameRepairer	  NemoFilenameRepairer;
typedef struct _NemoFilenameRepairerClass	  NemoFilenameRepairerClass;

struct _NemoFilenameRepairer {
    GObject parent;
};

struct _NemoFilenameRepairerClass {
    GObjectClass parent;
};

G_BEGIN_DECLS

GType nemo_filename_repairer_get_type     (void);
void  nemo_filename_repairer_register_type(GTypeModule *module);

void  nemo_filename_repairer_on_module_init(void);
void  nemo_filename_repairer_on_module_shutdown(void);

G_END_DECLS

#endif // nemo_filename_repairer_h
