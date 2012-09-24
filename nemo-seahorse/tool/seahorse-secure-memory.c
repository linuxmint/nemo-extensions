/*
 * Seahorse
 *
 * Copyright (C) 2007 Stef Walter
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
#include <string.h>

#include <glib.h>

#include <gnome-keyring-memory.h>

#include "seahorse-secure-memory.h"

  /* extern declared in seahorse-secure-memory.h */
  gboolean seahorse_use_secure_mem = FALSE;

static gpointer
switch_malloc (gsize size)
{
    gpointer p;

    if (size == 0)
        return NULL;
    if (seahorse_use_secure_mem)
        p = gnome_keyring_memory_try_alloc (size);
    else
        p = malloc (size);
    return p;
}

static gpointer
switch_calloc (gsize num, gsize size)
{
    gpointer p;

    if (size == 0 || num == 0)
        return NULL;
    if (seahorse_use_secure_mem)
        p = gnome_keyring_memory_try_alloc (size * num);
    else
        p = calloc (num, size);
    return p;
}

static gpointer
switch_realloc (gpointer mem, gsize size)
{
    gpointer p;

    if (size == 0) {
        free (mem);
        return NULL;
    }

    if (!mem) {
        if (seahorse_use_secure_mem)
            p = gnome_keyring_memory_alloc (size);
        else
            p = malloc (size);
    } else if (gnome_keyring_memory_is_secure (mem))
        p = gnome_keyring_memory_try_realloc (mem, size);
    else
        p = realloc (mem, size);
    return p;
}

static void
switch_free (gpointer mem)
{
    if (mem) {
        if (gnome_keyring_memory_is_secure (mem))
            gnome_keyring_memory_free (mem);
        else
            free (mem);
    }
}

static gboolean
seahorse_try_gk_secure_memory ()
{
    gpointer p;

    p = gnome_keyring_memory_try_alloc (10);
    if (p != NULL) {
        gnome_keyring_memory_free (p);
        return TRUE;
    }

    return FALSE;
}

void
seahorse_secure_memory_init ()
{
    if (seahorse_try_gk_secure_memory() == TRUE) {
        GMemVTable vtable;

        memset (&vtable, 0, sizeof (vtable));
        vtable.malloc = switch_malloc;
        vtable.realloc = switch_realloc;
        vtable.free = switch_free;
        vtable.calloc = switch_calloc;
        g_mem_set_vtable (&vtable);
    } else {
        g_warning ("Unable to allocate secure memory from gnome-keyring.\n");
        g_warning ("Proceeding using insecure memory for password fields.\n");
    }
}

