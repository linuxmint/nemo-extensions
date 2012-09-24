/*
 * Copyright 2008 Evenflow, Inc.
 *
 * nemo-dropbox-hooks.h
 * Header file for nemo-dropbox-hooks.c
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

#ifndef NEMO_DROPBOX_HOOKS_H
#define NEMO_DROPBOX_HOOKS_H

#include <glib.h>

G_BEGIN_DECLS

typedef void (*DropboxUpdateHook)(GHashTable *, gpointer);
typedef void (*DropboxHookClientConnectHook)(gpointer);

typedef struct {
  GIOChannel *chan;
  int socket;
  struct {
    int line;
    gchar *command_name;
    GHashTable *command_args;
    int numargs;
  } hhsi;
  gboolean connected;
  guint event_source;
  GHashTable *dispatch_table;
  GHookList ondisconnect_hooklist;
  GHookList onconnect_hooklist;
} NemoDropboxHookserv;

void
nemo_dropbox_hooks_setup(NemoDropboxHookserv *);

void
nemo_dropbox_hooks_start(NemoDropboxHookserv *);

gboolean
nemo_dropbox_hooks_is_connected(NemoDropboxHookserv *);

gboolean
nemo_dropbox_hooks_force_reconnect(NemoDropboxHookserv *);

void
nemo_dropbox_hooks_add(NemoDropboxHookserv *ndhs,
			   const gchar *hook_name,
			   DropboxUpdateHook hook, gpointer ud);
void
nemo_dropbox_hooks_add_on_disconnect_hook(NemoDropboxHookserv *hookserv,
					      DropboxHookClientConnectHook dhcch,
					      gpointer ud);

void
nemo_dropbox_hooks_add_on_connect_hook(NemoDropboxHookserv *hookserv,
					   DropboxHookClientConnectHook dhcch,
					   gpointer ud);


G_END_DECLS

#endif
