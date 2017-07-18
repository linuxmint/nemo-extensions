/*
 * Seahorse
 *
 * Copyright (C) 2003 Jacob Perkins
 * Copyright (C) 2004 - 2006 Stefan Walter
 * Copyright (C) 2012 Red Hat
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "seahorse-passphrase.h"
#include "seahorse-util.h"

#include <glib/gi18n.h>

#include <gcr/gcr.h>
#include <gpgme.h>

#define HIG_SMALL      6        /* gnome hig small space in pixels */
#define HIG_LARGE     12        /* gnome hig large space in pixels */

/* Convert passed text to utf-8 if not valid */
static gchar *
utf8_validate (const gchar *text)
{
    gchar *result;

    if (!text)
        return NULL;

    if (g_utf8_validate (text, -1, NULL))
        return g_strdup (text);

    result = g_locale_to_utf8 (text, -1, NULL, NULL, NULL);
    if (!result) {
        gchar *p;

        result = p = g_strdup (text);
        while (!g_utf8_validate (p, -1, (const gchar **) &p))
            *p = '?';
    }
    return result;
}

typedef struct {
	GAsyncResult *result;
	GMainLoop *loop;
} SyncClosure;

static void
on_sync_complete (GObject *source,
                  GAsyncResult *result,
                  gpointer user_data)
{
	SyncClosure *sync = user_data;
	sync->result = g_object_ref (result);
	g_main_loop_quit (sync->loop);
}

gpgme_error_t
seahorse_passphrase_get (gconstpointer dummy, const gchar *passphrase_hint,
                         const char* passphrase_info, int flags, int fd)
{
	gchar **split_uid = NULL;
	gchar *label = NULL;
	gchar *errmsg = NULL;
	const gchar *pass;
	GcrPrompt *prompt;
	SyncClosure sync;
	GError *error = NULL;
	gchar *msg;

	sync.result = NULL;
	sync.loop = g_main_loop_new (NULL, FALSE);

	gcr_system_prompt_open_async (-1, NULL, on_sync_complete, &sync);

	g_main_loop_run (sync.loop);
	g_assert (sync.result != NULL);

	prompt = gcr_system_prompt_open_finish (sync.result, &error);

	g_main_loop_unref (sync.loop);
	g_object_unref (sync.result);

	if (error != NULL) {
		g_message ("Couldn't open system prompt: %s", error->message);
		g_error_free (error);
		return gpgme_error (GPG_ERR_CANCELED);
	}

	if (passphrase_info && strlen(passphrase_info) < 16)
		flags |= SEAHORSE_PASS_NEW;

	if (passphrase_hint)
		split_uid = g_strsplit (passphrase_hint, " ", 2);

	if (flags & SEAHORSE_PASS_BAD)
		errmsg = g_strdup_printf (_("Wrong passphrase."));

	if (split_uid && split_uid[0] && split_uid[1]) {
		if (flags & SEAHORSE_PASS_NEW)
			label = g_strdup_printf (_("Enter new passphrase for '%s'"), split_uid[1]);
		else
			label = g_strdup_printf (_("Enter passphrase for '%s'"), split_uid[1]);
	} else {
		if (flags & SEAHORSE_PASS_NEW)
			label = g_strdup (_("Enter new passphrase"));
		else
			label = g_strdup (_("Enter passphrase"));
	}

	g_strfreev (split_uid);

	gcr_prompt_set_message (prompt, _("Passphrase"));

	msg = utf8_validate (errmsg ? errmsg : label);
	gcr_prompt_set_description (prompt, msg);
	g_free (msg);

	gcr_prompt_set_password_new (prompt, flags & SEAHORSE_PASS_NEW);

	gcr_prompt_set_continue_label (prompt, _("Ok"));
	gcr_prompt_set_cancel_label (prompt, _("Cancel"));

	g_free (label);
	g_free (errmsg);

	pass = gcr_prompt_password_run (prompt, NULL, &error);
	if (pass != NULL)
		seahorse_util_printf_fd (fd, "%s\n", pass);

	gcr_system_prompt_close_async (GCR_SYSTEM_PROMPT (prompt), NULL, NULL, NULL);
	g_object_unref (prompt);

	if (error != NULL) {
		g_message ("Couldn't prompt for password: %s", error->message);
		g_error_free (error);
		return gpgme_error (GPG_ERR_CANCELED);
	}

	return 0;
}
