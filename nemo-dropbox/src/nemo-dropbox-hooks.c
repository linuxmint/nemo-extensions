/*
 * Copyright 2008 Evenflow, Inc.
 *
 * nemo-dropbox-hooks.c
 * Implements connection handling and C interface for the Dropbox hook socket.
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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <string.h>

#include <glib.h>

#include "g-util.h"
#include "async-io-coroutine.h"
#include "dropbox-client-util.h"
#include "nemo-dropbox-hooks.h"

typedef struct {
  DropboxUpdateHook hook;
  gpointer ud;
} HookData;

static gboolean
try_to_connect(NemoDropboxHookserv *hookserv);

static gboolean
handle_hook_server_input(GIOChannel *chan,
			 GIOCondition cond,
			 NemoDropboxHookserv *hookserv) {
  /*debug_enter(); */

  /* we have some sweet macros defined that allow us to write this
     async event handler like a microthread yeahh, watch out for context */
  CRBEGIN(hookserv->hhsi.line);
  while (1) {
    hookserv->hhsi.command_args =
      g_hash_table_new_full((GHashFunc) g_str_hash,
			    (GEqualFunc) g_str_equal,
			    (GDestroyNotify) g_free,
			    (GDestroyNotify) g_strfreev);
    hookserv->hhsi.numargs = 0;
    
    /* read the command name */
    {
      gchar *line;
      CRREADLINE(hookserv->hhsi.line, chan, line);
      hookserv->hhsi.command_name = dropbox_client_util_desanitize(line);
      g_free(line);
    }

    /*debug("got a hook name: %s", hookserv->hhsi.command_name); */

    /* now read each arg line (until a certain limit) until we receive "done" */
    while (1) {
      gchar *line;

      /* if too many arguments, this connection seems malicious */
      if (hookserv->hhsi.numargs >= 20) {
	CRHALT;
      }

      CRREADLINE(hookserv->hhsi.line, chan, line);

      if (strcmp("done", line) == 0) {
	g_free(line);
	break;
      }
      else {
	gboolean parse_result;
	
	parse_result =
	  dropbox_client_util_command_parse_arg(line,
						hookserv->hhsi.command_args);
	g_free(line);

	if (FALSE == parse_result) {
	  debug("bad parse");
	  CRHALT;
	}
      }

      hookserv->hhsi.numargs += 1;
    }

    {
      HookData *hd;
      hd = (HookData *)
	g_hash_table_lookup(hookserv->dispatch_table,
			    hookserv->hhsi.command_name);
      if (hd != NULL) {
	(hd->hook)(hookserv->hhsi.command_args, hd->ud);
      }
    }
    
    g_free(hookserv->hhsi.command_name);
    g_hash_table_unref(hookserv->hhsi.command_args);
    hookserv->hhsi.command_name = NULL;
    hookserv->hhsi.command_args = NULL;
  }
  CREND;
}

static void
watch_killer(NemoDropboxHookserv *hookserv) {
  debug("hook client disconnected");

  hookserv->connected = FALSE;

  g_hook_list_invoke(&(hookserv->ondisconnect_hooklist), FALSE);
  
  /* we basically just have to free the memory allocated in the
     handle_hook_server_init ctx */
  if (hookserv->hhsi.command_name != NULL) {
    g_free(hookserv->hhsi.command_name);
    hookserv->hhsi.command_name = NULL;
  }

  if (hookserv->hhsi.command_args != NULL) {
    g_hash_table_unref(hookserv->hhsi.command_args);
    hookserv->hhsi.command_args = NULL;
  }

  g_io_channel_unref(hookserv->chan);
  hookserv->chan = NULL;
  hookserv->event_source = 0;
  hookserv->socket = 0;

  /* lol we also have to start a new connection */
  try_to_connect(hookserv);
}

static gboolean
try_to_connect(NemoDropboxHookserv *hookserv) {
  /* create socket */
  hookserv->socket = socket(PF_UNIX, SOCK_STREAM, 0);
  
  /* set native non-blocking, for connect timeout */
  {
    int flags;

    if ((flags = fcntl(hookserv->socket, F_GETFL, 0)) < 0) {
      goto FAIL_CLEANUP;
    }

    if (fcntl(hookserv->socket, F_SETFL, flags | O_NONBLOCK) < 0) {
      goto FAIL_CLEANUP;
    }
  }

  /* connect to server, might fail of course */
  {
    struct sockaddr_un addr;
    socklen_t addr_len;
    
    /* intialize address structure */
    addr.sun_family = AF_UNIX;
    g_snprintf(addr.sun_path,
	       sizeof(addr.sun_path),
	       "%s/.dropbox/iface_socket",
	       g_get_home_dir());
    addr_len = sizeof(addr) - sizeof(addr.sun_path) + strlen(addr.sun_path);

    /* if there was an error we have to try again later */
    if (connect(hookserv->socket, (struct sockaddr *) &addr, addr_len) < 0) {
      if (errno == EINPROGRESS) {
	fd_set writers;
	struct timeval tv = {1, 0};
        FD_ZERO(&writers);
	FD_SET(hookserv->socket, &writers);
	
	/* if nothing was ready after 3 seconds, fail out homie */
	if (select(hookserv->socket+1, NULL, &writers, NULL, &tv) == 0) {
	  goto FAIL_CLEANUP;
	}

	if (connect(hookserv->socket, (struct sockaddr *) &addr, addr_len) < 0) {
	  debug("couldn't connect to hook server after 1 second");
	  goto FAIL_CLEANUP;
	}
      }
      else {
	goto FAIL_CLEANUP;
      }
    }
  }

  /* lol sometimes i write funny codez */
  if (FALSE) {
  FAIL_CLEANUP:
    close(hookserv->socket);
    g_timeout_add_seconds(1, (GSourceFunc) try_to_connect, hookserv);
    return FALSE;
  }

  /* great we connected!, let's create the channel and wait on it */
  hookserv->chan = g_io_channel_unix_new(hookserv->socket);
  g_io_channel_set_line_term(hookserv->chan, "\n", -1);
  g_io_channel_set_close_on_unref(hookserv->chan, TRUE);

  /*debug("create channel"); */

  /* Set non-blocking ;) (again just in case) */
  {
    GIOFlags flags;
    GIOStatus iostat;
    
    flags = g_io_channel_get_flags(hookserv->chan);
    iostat = g_io_channel_set_flags(hookserv->chan, flags | G_IO_FLAG_NONBLOCK,
				    NULL);
    if (iostat == G_IO_STATUS_ERROR) {
      g_io_channel_unref(hookserv->chan);
      g_timeout_add_seconds(1, (GSourceFunc) try_to_connect, hookserv);
      return FALSE;
    }
  }

  /*debug("set non blocking"); */

  /* this is fun, async io watcher */
  hookserv->hhsi.line = 0;
  hookserv->hhsi.command_args = NULL;
  hookserv->hhsi.command_name = NULL;
  hookserv->event_source = 
    g_io_add_watch_full(hookserv->chan, G_PRIORITY_DEFAULT,
			G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
			(GIOFunc) handle_hook_server_input, hookserv,
			(GDestroyNotify) watch_killer);

  debug("hook client connected");
  hookserv->connected = TRUE;
  g_hook_list_invoke(&(hookserv->onconnect_hooklist), FALSE);

  /*debug("added watch");*/
  return FALSE;
}

/* should only be called in glib main loop */
/* returns a gboolean because it is a GSourceFunc */
gboolean nemo_dropbox_hooks_force_reconnect(NemoDropboxHookserv *hookserv) {
  debug_enter();

  if (hookserv->connected == FALSE) {
    return FALSE;
  }

  debug("forcing hook to reconnect");

  g_assert(hookserv->event_source >= 0);
  
  if (hookserv->event_source > 0) {
    g_source_remove(hookserv->event_source);
  }
  else if (hookserv->event_source == 0) {
    debug("event source was zero!!!!!");
  }
	 
  return FALSE;
}

gboolean
nemo_dropbox_hooks_is_connected(NemoDropboxHookserv *hookserv) {
  return hookserv->connected;
}

void
nemo_dropbox_hooks_setup(NemoDropboxHookserv *hookserv) {
  hookserv->dispatch_table = g_hash_table_new_full((GHashFunc) g_str_hash,
						   (GEqualFunc) g_str_equal,
						   g_free, g_free);
  hookserv->connected = FALSE;

  g_hook_list_init(&(hookserv->ondisconnect_hooklist), sizeof(GHook));
  g_hook_list_init(&(hookserv->onconnect_hooklist), sizeof(GHook));
}

void
nemo_dropbox_hooks_add_on_disconnect_hook(NemoDropboxHookserv *hookserv,
					      DropboxHookClientConnectHook dhcch,
					      gpointer ud) {
  GHook *newhook;
  
  newhook = g_hook_alloc(&(hookserv->ondisconnect_hooklist));
  newhook->func = dhcch;
  newhook->data = ud;
  
  g_hook_append(&(hookserv->ondisconnect_hooklist), newhook);
}

void
nemo_dropbox_hooks_add_on_connect_hook(NemoDropboxHookserv *hookserv,
					   DropboxHookClientConnectHook dhcch,
					   gpointer ud) {
  GHook *newhook;
  
  newhook = g_hook_alloc(&(hookserv->onconnect_hooklist));
  newhook->func = dhcch;
  newhook->data = ud;
  
  g_hook_append(&(hookserv->onconnect_hooklist), newhook);
}

void nemo_dropbox_hooks_add(NemoDropboxHookserv *ndhs,
				const gchar *hook_name,
				DropboxUpdateHook hook, gpointer ud) {
  HookData *hd;
  hd = g_new(HookData, 1);
  hd->hook = hook;
  hd->ud = ud;
  g_hash_table_insert(ndhs->dispatch_table, g_strdup(hook_name), hd);
}

void
nemo_dropbox_hooks_start(NemoDropboxHookserv *hookserv) {
  try_to_connect(hookserv);
}
