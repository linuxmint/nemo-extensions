/*
 * Copyright 2008 Evenflow, Inc.
 *
 * dropbox-client-util.c
 * Utility functions for implementing dropbox clients.
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

static gchar chars_not_to_escape[] = {
  1, 2, 3, 4, 5, 6, 7, 8, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
  23, 24, 25, 26, 27, 28, 29, 30, 31, 34, 127,
  -128, -127, -126, -125, -124, -123, -122, -121, -120, -119,
  -118, -117, -116, -115, -114, -113, -112, -111, -110, -109,
  -108, -107, -106, -105, -104, -103, -102, -101, -100, -99,
  -98, -97, -96, -95, -94, -93, -92, -91, -90, -89,
  -88, -87, -86, -85, -84, -83, -82, -81, -80, -79,
  -78, -77, -76, -75, -74, -73, -72, -71, -70, -69,
  -68, -67, -66, -65, -64, -63, -62, -61, -60, -59,
  -58, -57, -56, -55, -54, -53, -52, -51, -50, -49,
  -48, -47, -46, -45, -44, -43, -42, -41, -40, -39,
  -38, -37, -36, -35, -34, -33, -32, -31, -30, -29,
  -28, -27, -26, -25, -24, -23, -22, -21, -20, -19,
  -18, -17, -16, -15, -14, -13, -12, -11, -10, -9,
  -8, -7, -6, -5, -4, -3, -2, -1, 0
};

gchar *dropbox_client_util_sanitize(const gchar *a) {
  /* this function escapes teh following utf-8 characters:
   * '\\', '\n', '\t'
   */
  return g_strescape(a, chars_not_to_escape);
}

gchar *dropbox_client_util_desanitize(const gchar *a) {
  return g_strcompress(a);
}

gboolean
dropbox_client_util_command_parse_arg(const gchar *line, GHashTable *return_table) {
  gchar **argval;
  guint len;
  gboolean retval;
  
  argval = g_strsplit(line, "\t", 0);
  len = g_strv_length(argval);

  /*  debug("parsed: (%d) %s", len, line); */
  
  if (len > 1) {
    int i;
    gchar **vals;
    
    vals = g_new(gchar *, len);
    vals[len - 1] = NULL;
    
    for (i = 1; argval[i] != NULL; i++) {
      vals[i-1] = dropbox_client_util_desanitize(argval[i]);
      
    }
    
    g_hash_table_insert(return_table,
			dropbox_client_util_desanitize(argval[0]),
			vals);
    retval = TRUE;
  }
  else {
    retval = FALSE;
  }

  g_strfreev(argval);
  return retval;
}

