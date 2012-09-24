/*
 * Secure memory allocation
 * Copyright (C) 1998,1999 Free Software Foundation, Inc.
 * Copyright (C) 1999,2000 Robert Bihlmeyer <robbe@orcus.priv.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _SEAHORSE_SECURE_MEMORY_H_
#define _SEAHORSE_SECURE_MEMORY_H_

#include <sys/types.h>

extern gboolean seahorse_use_secure_mem;

#define WITH_SECURE_MEM(EXP) \
    do { \
        gboolean _tmp = seahorse_use_secure_mem; \
        seahorse_use_secure_mem = TRUE; \
        EXP; \
        seahorse_use_secure_mem = _tmp; \
    } while (0)

/* This must be called before any glib/gtk/gnome functions */
void    seahorse_secure_memory_init         (void);

#endif /* _SEAHORSE_SECURE_MEMORY_H_ */
