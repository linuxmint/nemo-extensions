/*
 * Copyright 2008 Evenflow, Inc.
 *
 * nemo-dropbox.h
 * Header file for nemo-dropbox.c
 *
 * This file is part of nemo-dropbox.
 *
 * nemo-dropbox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * nemo-dropbox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nemo-dropbox.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef NEMO_DROPBOX_H
#define NEMO_DROPBOX_H

#include <glib.h>
#include <glib-object.h>

#include <libnemo-extension/nemo-info-provider.h>

#include "dropbox-command-client.h"
#include "nemo-dropbox-hooks.h"
#include "dropbox-client.h"

G_BEGIN_DECLS

/* Declarations for the dropbox extension object.  This object will be
 * instantiated by nemo.  It implements the GInterfaces 
 * exported by libnemo. */

#define NEMO_TYPE_DROPBOX	  (nemo_dropbox_get_type ())
#define NEMO_DROPBOX(o)	  (G_TYPE_CHECK_INSTANCE_CAST ((o), NEMO_TYPE_DROPBOX, NemoDropbox))
#define NEMO_IS_DROPBOX(o)	  (G_TYPE_CHECK_INSTANCE_TYPE ((o), NEMO_TYPE_DROPBOX))
typedef struct _NemoDropbox      NemoDropbox;
typedef struct _NemoDropboxClass NemoDropboxClass;

struct _NemoDropbox {
  GObject parent_slot;
  GHashTable *filename2obj;
  GHashTable *obj2filename;
  GMutex *emblem_paths_mutex;
  GHashTable *emblem_paths;
  DropboxClient dc;
};

struct _NemoDropboxClass {
	GObjectClass parent_slot;
};

GType nemo_dropbox_get_type(void);
void  nemo_dropbox_register_type(GTypeModule *module);

extern gboolean dropbox_use_nemo_submenu_workaround;
extern gboolean dropbox_use_operation_in_progress_workaround;

G_END_DECLS

#endif
