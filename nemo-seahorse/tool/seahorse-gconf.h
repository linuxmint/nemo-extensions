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
 * 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * A collection of functions for accessing gconf. Adapted from libeel.
 */

#include <glib.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include <gtk/gtk.h>

#define SEAHORSE_DESKTOP_KEYS           "/desktop/pgp"

#define ARMOR_KEY SEAHORSE_DESKTOP_KEYS "/ascii_armor"
#define MULTI_EXTENSION_KEY SEAHORSE_DESKTOP_KEYS "/package_extension"
#define MULTI_SEPERATE_KEY SEAHORSE_DESKTOP_KEYS "/multi_seperate"

#define SEAHORSE_SCHEMAS            "/apps/seahorse"

#define WINDOW_SIZE                SEAHORSE_SCHEMAS "/windows/"

void            seahorse_gconf_set_boolean       (const char         *key,
                                                  gboolean           boolean_value);

gboolean        seahorse_gconf_get_boolean       (const char         *key);

void            seahorse_gconf_set_integer       (const char         *key,
                                                  int                int_value);

int             seahorse_gconf_get_integer       (const char         *key);

void            seahorse_gconf_set_string        (const char         *key,
                                                  const char         *string_value);

char*           seahorse_gconf_get_string        (const char         *key);
