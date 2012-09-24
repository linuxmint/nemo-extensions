/*
 * Copyright 2008 Evenflow, Inc.
 *
 * dropbox-client.h
 * Header file for dropbox-client.c
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

#ifndef DROPBOX_CLIENT_H
#define DROPBOX_CLIENT_H

#include <glib.h>
#include "dropbox-command-client.h"
#include "nemo-dropbox-hooks.h"

G_BEGIN_DECLS

typedef struct {
  DropboxCommandClient dcc;
  NemoDropboxHookserv hookserv;
  GHookList onconnect_hooklist;
  GHookList ondisconnect_hooklist;
  gboolean hook_connect_called;
  gboolean command_connect_called;
  gboolean hook_disconnect_called;
  gboolean command_disconnect_called;
} DropboxClient;

typedef void (*DropboxClientConnectionAttemptHook)(guint, gpointer);
typedef GHookFunc DropboxClientConnectHook;

void
dropbox_client_setup(DropboxClient *dc);

void
dropbox_client_start(DropboxClient *dc);

gboolean
dropbox_client_is_connected(DropboxClient *dc);

void
dropbox_client_force_reconnect(DropboxClient *dc);

void
dropbox_client_add_on_connect_hook(DropboxClient *dc,
				   DropboxClientConnectHook dhcch,
				   gpointer ud);

void
dropbox_client_add_on_disconnect_hook(DropboxClient *dc,
				      DropboxClientConnectHook dhcch,
				      gpointer ud);

void
dropbox_client_add_connection_attempt_hook(DropboxClient *dc,
					   DropboxClientConnectionAttemptHook dhcch,
					   gpointer ud);

G_END_DECLS

#endif
