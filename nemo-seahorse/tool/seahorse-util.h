/*
 * Seahorse
 *
 * Copyright (C) 2003 Jacob Perkins
 * Copyright (C) 2004-2005 Stefan Walter
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
 * A bunch of miscellaneous utility functions.
 */

#ifndef __SEAHORSE_UTIL_H__
#define __SEAHORSE_UTIL_H__

#include <gtk/gtk.h>
#include <time.h>
#include <gpgme.h>

#include "config.h"

typedef enum {
    SEAHORSE_CRYPT_SUFFIX,
    SEAHORSE_SIG_SUFFIX,
} SeahorseSuffix;

#define SEAHORSE_EXT_SIG ".sig"
#define SEAHORSE_EXT_PGP ".pgp"

gchar*		seahorse_util_get_display_date_string   (const time_t		time);


#define SEAHORSE_GPGME_ERROR  (seahorse_util_gpgme_error_domain ())

GQuark      seahorse_util_gpgme_error_domain (void) G_GNUC_CONST;

#define SEAHORSE_ERROR  (seahorse_util_error_domain ())

GQuark      seahorse_util_error_domain (void) G_GNUC_CONST;

void        seahorse_util_gpgme_to_error        (gpgme_error_t gerr,
                                                 GError** err);

void        seahorse_util_show_error            (GtkWindow          *parent,
                                                 const gchar        *heading,
                                                 const gchar        *message);

void        seahorse_util_handle_gpgme          (gpgme_error_t      err,
                                                 const gchar*       desc, ...);

void        seahorse_util_handle_error          (GError*            err,
                                                 const char*        desc, ...);

gboolean    seahorse_util_printf_fd         (int fd,
                                             const char* data, ...);

gboolean    seahorse_util_uri_exists        (const gchar* uri);

const gchar* seahorse_util_uri_get_last     (const gchar* uri);

const gchar* seahorse_util_uri_split_last   (gchar* uri);

gboolean    seahorse_util_uris_package      (const gchar* package,
                                             const gchar** uris);

GtkWidget*  seahorse_util_chooser_save_new              (const gchar *title,
                                                         GtkWindow *parent);

void        seahorse_util_chooser_show_key_files        (GtkWidget *dialog);

void        seahorse_util_chooser_show_archive_files    (GtkWidget *dialog);

gchar*      seahorse_util_chooser_save_prompt           (GtkWidget *dialog);

gchar*		seahorse_util_add_suffix		(const gchar        *path,
                                             SeahorseSuffix     suffix,
                                             const gchar        *prompt);

gchar*      seahorse_util_remove_suffix     (const gchar        *path,
                                             const gchar        *prompt);

void         seahorse_util_free_keys          (gpgme_key_t* keys);

gboolean    seahorse_util_string_equals       (const gchar *s1, const gchar *s2);

#define     seahorse_util_wait_until(expr)                \
    while (!(expr)) {                                     \
        while (g_main_context_pending(NULL) && !(expr))   \
            g_main_context_iteration (NULL, FALSE);       \
        g_thread_yield ();                                \
    }

#endif /* __SEAHORSE_UTIL_H__ */
