/*
 * Copyright 2008 Evenflow, Inc.
 *
 * dropbox-client.c
 * Implements connection handling and C interface for interfacing with the Dropbox daemon.
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

#include <glib.h>

#include "g-util.h"
#include "dropbox-command-client.h"
#include "nemo-dropbox-hooks.h"
#include "dropbox-client.h"

static void
hook_on_connect(DropboxClient *dc) {
  dc->hook_connect_called = TRUE;
  
  if (dc->command_connect_called) {
    debug("client connection");
    g_hook_list_invoke(&(dc->onconnect_hooklist), FALSE);
    /* reset flags */
    dc->hook_connect_called = dc->command_connect_called = FALSE;
  }
}

static void
command_on_connect(DropboxClient *dc) {
  dc->command_connect_called = TRUE;
  
  if (dc->hook_connect_called) {
    debug("client connection");
    g_hook_list_invoke(&(dc->onconnect_hooklist), FALSE);
    /* reset flags */
    dc->hook_connect_called = dc->command_connect_called = FALSE;
  }
}

static void
command_on_disconnect(DropboxClient *dc) {
  dc->command_disconnect_called = TRUE;
  
  if (dc->hook_disconnect_called) {
    debug("client disconnect");
    g_hook_list_invoke(&(dc->ondisconnect_hooklist), FALSE);
    /* reset flags */
    dc->hook_disconnect_called = dc->command_disconnect_called = FALSE;
  }
  else {
    nemo_dropbox_hooks_force_reconnect(&(dc->hookserv));
  }
}

static void
hook_on_disconnect(DropboxClient *dc) {
  dc->hook_disconnect_called = TRUE;
  
  if (dc->command_disconnect_called) {
    debug("client disconnect");
    g_hook_list_invoke(&(dc->ondisconnect_hooklist), FALSE);
    /* reset flags */
    dc->hook_disconnect_called = dc->command_disconnect_called = FALSE;
  }
  else {
    dropbox_command_client_force_reconnect(&(dc->dcc));
  }
}

gboolean
dropbox_client_is_connected(DropboxClient *dc) {
  return (dropbox_command_client_is_connected(&(dc->dcc)) &&
	  nemo_dropbox_hooks_is_connected(&(dc->hookserv)));
}

void
dropbox_client_force_reconnect(DropboxClient *dc) {
  if (dropbox_client_is_connected(dc) == TRUE) {
    debug("forcing client to reconnect");
    dropbox_command_client_force_reconnect(&(dc->dcc));
    nemo_dropbox_hooks_force_reconnect(&(dc->hookserv));
  }
}

/* should only be called once on initialization */
void
dropbox_client_setup(DropboxClient *dc) {
  nemo_dropbox_hooks_setup(&(dc->hookserv));
  dropbox_command_client_setup(&(dc->dcc));

  g_hook_list_init(&(dc->ondisconnect_hooklist), sizeof(GHook));
  g_hook_list_init(&(dc->onconnect_hooklist), sizeof(GHook));

  dc->hook_disconnect_called = dc->command_disconnect_called = FALSE;
  dc->hook_connect_called = dc->command_connect_called = FALSE;

  nemo_dropbox_hooks_add_on_connect_hook(&(dc->hookserv), 
					     (DropboxHookClientConnectHook)
					     hook_on_connect, dc);
  
  dropbox_command_client_add_on_connect_hook(&(dc->dcc),
					     (DropboxCommandClientConnectHook)
					     command_on_connect, dc);
  
  nemo_dropbox_hooks_add_on_disconnect_hook(&(dc->hookserv), 
						(DropboxHookClientConnectHook)
						hook_on_disconnect, dc);
  
  dropbox_command_client_add_on_disconnect_hook(&(dc->dcc),
						(DropboxCommandClientConnectHook)
						command_on_disconnect, dc);
}

/* not thread safe */
void
dropbox_client_add_on_disconnect_hook(DropboxClient *dc,
				      DropboxClientConnectHook dhcch,
				      gpointer ud) {
  GHook *newhook;
  
  newhook = g_hook_alloc(&(dc->ondisconnect_hooklist));
  newhook->func = dhcch;
  newhook->data = ud;
  
  g_hook_append(&(dc->ondisconnect_hooklist), newhook);
}

/* not thread safe */
void
dropbox_client_add_on_connect_hook(DropboxClient *dc,
				   DropboxClientConnectHook dhcch,
				   gpointer ud) {
  GHook *newhook;
  
  newhook = g_hook_alloc(&(dc->onconnect_hooklist));
  newhook->func = dhcch;
  newhook->data = ud;
  
  g_hook_append(&(dc->onconnect_hooklist), newhook);
}

/* not thread safe */
void
dropbox_client_add_connection_attempt_hook(DropboxClient *dc,
					   DropboxClientConnectionAttemptHook dhcch,
					   gpointer ud) {
  debug("shouldn't be here...");

  dropbox_command_client_add_connection_attempt_hook(&(dc->dcc),
						     dhcch, ud);
}

/* should only be called once on initialization */
void
dropbox_client_start(DropboxClient *dc) {
  debug("starting connections");
  nemo_dropbox_hooks_start(&(dc->hookserv));
  dropbox_command_client_start(&(dc->dcc));
}
