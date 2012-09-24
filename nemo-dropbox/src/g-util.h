/*
 * Copyright 2008 Evenflow, Inc.
 *
 * g-util.h
 * Helper macros.
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

#ifndef G_UTIL_H
#define G_UTIL_H

#include <glib.h>
#include <glib/gprintf.h>

G_BEGIN_DECLS

#ifdef ND_DEBUG

#define debug_enter() {g_print("Entering "); g_print(__FUNCTION__); g_printf("\n");}
#define debug(format, ...) {g_print(__FUNCTION__); g_print(": "); \
    g_printf(format, ## __VA_ARGS__); g_print("\n");}
#define debug_return(v) {g_print("Exiting "); g_print(__FUNCTION__); g_printf("\n"); return v;}

#else

#define debug_enter() do {} while(0)
#define debug(format, ...) do {} while(0)
#define debug_return(v) do {} while(0)

#endif

G_END_DECLS

#endif
