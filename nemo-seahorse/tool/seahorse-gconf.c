/*
 * Seahorse
 *
 * Copyright (C) 2005 Stefan Walter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#include "config.h"

#include <stdlib.h>

#include "seahorse-gconf.h"

static GConfClient *global_gconf_client = NULL;

static void
global_client_free (void)
{
    if (global_gconf_client == NULL)
        return;

    gconf_client_remove_dir (global_gconf_client, SEAHORSE_DESKTOP_KEYS, NULL);
    gconf_client_remove_dir (global_gconf_client, SEAHORSE_SCHEMAS, NULL);

    g_object_unref (global_gconf_client);
    global_gconf_client = NULL;
}

static gboolean
handle_error (GError **error)
{
    g_return_val_if_fail (error != NULL, FALSE);

    if (*error != NULL) {
        g_warning ("GConf error:\n  %s", (*error)->message);
        g_error_free (*error);
        *error = NULL;

        return TRUE;
    }

    return FALSE;
}

static GConfClient*
get_global_client (void)
{
    GError *error = NULL;

    /* Initialize gconf if needed */
    if (!gconf_is_initialized ()) {
        char *argv[] = { "seahorse", NULL };

        if (!gconf_init (1, argv, &error)) {
            if (handle_error (&error))
                return NULL;
        }
    }

    if (global_gconf_client == NULL) {
        global_gconf_client = gconf_client_get_default ();

        if (global_gconf_client) {
            gconf_client_add_dir (global_gconf_client, SEAHORSE_DESKTOP_KEYS,
                                  GCONF_CLIENT_PRELOAD_NONE, &error);
            handle_error (&error);
            gconf_client_add_dir (global_gconf_client, SEAHORSE_SCHEMAS,
                                  GCONF_CLIENT_PRELOAD_NONE, &error);
            handle_error (&error);
        }

        atexit (global_client_free);
    }

    return global_gconf_client;
}

void
seahorse_gconf_set_boolean (const char *key, gboolean boolean_value)
{
	GConfClient *client;
	GError *error = NULL;

	g_return_if_fail (key != NULL);

	client = get_global_client ();
	g_return_if_fail (client != NULL);

	gconf_client_set_bool (client, key, boolean_value, &error);
	handle_error (&error);
}

gboolean
seahorse_gconf_get_boolean (const char *key)
{
	gboolean result;
	GConfClient *client;
	GError *error = NULL;

	g_return_val_if_fail (key != NULL, FALSE);

	client = get_global_client ();
	g_return_val_if_fail (client != NULL, FALSE);

	result = gconf_client_get_bool (client, key, &error);
	return handle_error (&error) ? FALSE : result;
}

void
seahorse_gconf_set_integer (const char *key, int int_value)
{
	GConfClient *client;
	GError *error = NULL;

	g_return_if_fail (key != NULL);

	client = get_global_client ();
	g_return_if_fail (client != NULL);

	gconf_client_set_int (client, key, int_value, &error);
	handle_error (&error);
}

int
seahorse_gconf_get_integer (const char *key)
{
	int result;
	GConfClient *client;
	GError *error = NULL;

	g_return_val_if_fail (key != NULL, 0);

	client = get_global_client ();
	g_return_val_if_fail (client != NULL, 0);

	result = gconf_client_get_int (client, key, &error);
    return handle_error (&error) ? 0 : result;
}

void
seahorse_gconf_set_string (const char *key, const char *string_value)
{
	GConfClient *client;
	GError *error = NULL;

	g_return_if_fail (key != NULL);

	client = get_global_client ();
	g_return_if_fail (client != NULL);

	gconf_client_set_string (client, key, string_value, &error);
	handle_error (&error);
}

char *
seahorse_gconf_get_string (const char *key)
{
	char *result;
	GConfClient *client;
	GError *error = NULL;

	g_return_val_if_fail (key != NULL, NULL);

	client = get_global_client ();
	g_return_val_if_fail (client != NULL, NULL);

	result = gconf_client_get_string (client, key, &error);
	return handle_error (&error) ? g_strdup ("") : result;
}
