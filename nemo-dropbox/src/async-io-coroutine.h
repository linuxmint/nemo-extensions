/*
 * Copyright 2008 Evenflow, Inc.
 *
 * async-io-coroutine.h
 * Macros to simplify writing coroutines for the glib main loop.
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

#ifndef ASYNC_IO_COROUTINE_H
#define ASYNC_IO_COROUTINE_H

#include <glib.h>

G_BEGIN_DECLS

#define CRBEGIN(pos) switch (pos) { case 0:
#define CREND } return FALSE
#define CRYIELD(pos) do { pos = __LINE__; return TRUE; case __LINE__:;} while (0)
#define CRHALT return FALSE  

#define CRREADLINE(pos, chan, where)                             \
  while (1) {							\
    gchar *__line;						\
    gsize __line_length, __newline_pos;				\
    GIOStatus __iostat;							\
    									\
    __iostat = g_io_channel_read_line(chan, &__line,			\
				      &__line_length,			\
				      &__newline_pos, NULL);		\
    if (__iostat == G_IO_STATUS_AGAIN) {				\
      CRYIELD(pos);                                                \
    }									\
    else if (__iostat == G_IO_STATUS_NORMAL) {				\
      *(__line + __newline_pos) = '\0';				\
      where = __line;						\
      break;							\
    }								\
    else if (__iostat == G_IO_STATUS_EOF ||			\
	     __iostat == G_IO_STATUS_ERROR) {			\
      CRHALT;							\
    }								\
    else {							\
      g_assert_not_reached();					\
      CRHALT;							\
    }								\
  }

G_END_DECLS

#endif
