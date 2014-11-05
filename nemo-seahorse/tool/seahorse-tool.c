/*
 * Seahorse
 *
 * Copyright (C) 2006 Stefan Walter
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

#include <errno.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>

#include <dbus/dbus-glib-bindings.h>

#include "cryptui.h"
#include "cryptui-key-store.h"

#include "seahorse-secure-memory.h"
#include "seahorse-tool.h"
#include "seahorse-util.h"
#include "seahorse-vfs-data.h"
#include "seahorse-libdialogs.h"
#include "seahorse-gconf.h"
#include "seahorse-util.h"

#define IMPORT_BUFFER_SIZE 50*1<<10 /* 50 kB */

/* -----------------------------------------------------------------------------
 * ARGUMENT PARSING
 */

static gchar **arg_uris = NULL;
static gboolean read_uris = FALSE;
gboolean mode_import = FALSE;
gboolean mode_encrypt = FALSE;
gboolean mode_sign = FALSE;
gboolean mode_encrypt_sign = FALSE;
gboolean mode_decrypt = FALSE;
gboolean mode_verify = FALSE;

static const GOptionEntry options[] = {
    { "import", 'i', 0, G_OPTION_ARG_NONE, &mode_import,
        N_("Import keys from the file"), NULL },
    { "encrypt", 'e', 0, G_OPTION_ARG_NONE, &mode_encrypt,
        N_("Encrypt file"), NULL },
    { "sign", 's', 0, G_OPTION_ARG_NONE, &mode_sign,
        N_("Sign file with default key"), NULL },
    { "encrypt-sign", 'n', 0, G_OPTION_ARG_NONE, &mode_encrypt_sign,
        N_("Encrypt and sign file with default key"), NULL },
    { "decrypt", 'd', 0, G_OPTION_ARG_NONE, &mode_decrypt,
        N_("Decrypt encrypted file"), NULL },
    { "verify", 'v', 0, G_OPTION_ARG_NONE, &mode_verify,
        N_("Verify signature file"), NULL },
    { "uri-list", 'T', 0, G_OPTION_ARG_NONE, &read_uris,
        N_("Read list of URIs on standard in"), NULL },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &arg_uris,
        NULL, N_("file...") },
    { NULL }
};

/* Returns a null terminated array of uris, use g_strfreev to free */
static gchar**
read_uri_arguments ()
{
    /* Read uris from stdin */
    if (read_uris) {

        GIOChannel* io;
        GArray* files;
        gchar* line;

        files = g_array_new (TRUE, TRUE, sizeof (gchar*));

        /* Opens a channel on stdin */
        io = g_io_channel_unix_new (0);

        while (g_io_channel_read_line (io, &line, NULL, NULL, NULL) == G_IO_STATUS_NORMAL) {
            if (line == NULL)
                continue;
            g_strstrip(line);
            if(line[0] == 0) {
                g_free(line);
                continue;
            }
            g_array_append_val(files, line);
        }

        g_io_channel_unref (io);
        return (gchar**)g_array_free (files, FALSE);

    /* Multiple arguments on command line */
    } else {

        gchar **args = arg_uris;
        arg_uris = NULL;
        return args;

    }
}

/* -----------------------------------------------------------------------------
 * Initialize Crypto
 */

 /* Setup in init_crypt */
DBusGConnection *dbus_connection = NULL;
DBusGProxy      *dbus_key_proxy = NULL;

static gboolean
init_crypt ()
{
    GError *error = NULL;

    if (!dbus_connection) {
        dbus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (!dbus_connection) {

            return FALSE;
        }

        dbus_key_proxy = dbus_g_proxy_new_for_name (dbus_connection, "org.gnome.seahorse",
                                               "/org/gnome/seahorse/keys",
                                               "org.gnome.seahorse.KeyService");
    }

    return TRUE;
}

/* -----------------------------------------------------------------------------
 * ENCRYPT SIGN
 */

static gpgme_key_t*
prompt_recipients (gpgme_key_t *signkey)
{
    gpgme_error_t gerr = 0;
    CryptUIKeyset *keyset;
    gpgme_ctx_t ctx;
    gpgme_key_t key;
    GArray *keys = NULL;
    gchar **recips;
    gchar *signer;

    *signkey = NULL;

    keyset = cryptui_keyset_new ("openpgp", TRUE);

    if (cryptui_keyset_get_count (keyset) == 0) {
        cryptui_need_to_get_keys ();
    } else {
        recips = cryptui_prompt_recipients (keyset, _("Choose Recipients"), &signer);

        if (recips) {
            gpgme_check_version (NULL);
            gerr = gpgme_engine_check_version (GPGME_PROTOCOL_OpenPGP);
            g_return_val_if_fail (gerr == 0, NULL);

            gerr = gpgme_new (&ctx);
            g_return_val_if_fail (gerr == 0, NULL);

            if (signer) {
                /* Load up the GPGME secret key */
                gchar *id = cryptui_keyset_key_raw_keyid (keyset, signer);
                gerr = gpgme_get_key (ctx, id, signkey, 1);
                g_free (id);

                /* A more descriptive error message */
                if (GPG_ERR_EOF == gpgme_err_code (gerr))
                    gerr = gpgme_error (GPG_ERR_NOT_FOUND);
            }

            if (gerr == 0) {
                gchar **ids;
                guint num;

                /* Load up the GPGME keys */
                ids = cryptui_keyset_keys_raw_keyids (keyset, (const gchar**)recips);
                num = g_strv_length (ids);
                keys = g_array_new (TRUE, TRUE, sizeof (gpgme_key_t));
                gerr = gpgme_op_keylist_ext_start (ctx, (const gchar**)ids, 0, 0);
                g_free (ids);

                if (gerr == 0) {
                    while ((gerr = gpgme_op_keylist_next (ctx, &key)) == 0)
                        g_array_append_val (keys, key);
                    gpgme_op_keylist_end (ctx);
                }

                /* Ignore EOF error */
                if (GPG_ERR_EOF == gpgme_err_code (gerr))
                    gerr = 0;

                if (gerr == 0 && num != keys->len)
                    g_warning ("couldn't load all the keys (%d/%d) from GPGME", keys->len, num);
            }

            gpgme_release (ctx);
        }

        g_object_unref (keyset);

        if (!recips)
            return NULL;

        g_strfreev (recips);
        g_free (signer);

        if (gerr == 0 && keys->len)
            return (gpgme_key_t*)g_array_free (keys, FALSE);

        /* When failure, free all our return values */
        seahorse_util_free_keys ((gpgme_key_t*)g_array_free (keys, FALSE));
        if (*signkey)
            gpgme_key_unref (*signkey);

        seahorse_util_handle_gpgme (gerr, _("Couldn't load keys"));
    }

    return NULL;
}

static gboolean
encrypt_sign_start (SeahorseToolMode *mode, const gchar *uri, gpgme_data_t uridata,
                    SeahorsePGPOperation *pop, GError **err)
{
    gpgme_data_t cipher;
    gpgme_error_t gerr;
    gchar *touri;

    g_assert (mode->recipients && mode->recipients[0]);

    /* File to encrypt to */
    touri = seahorse_util_add_suffix (uri, SEAHORSE_CRYPT_SUFFIX,
                                      _("Choose Encrypted File Name for '%s'"));
    if (!touri)
        return FALSE;

    /* Open necessary files, release these with the operation */
    cipher = seahorse_vfs_data_create (touri, SEAHORSE_VFS_WRITE | SEAHORSE_VFS_DELAY, err);
    g_free (touri);
    if (!cipher)
        return FALSE;
    g_object_set_data_full (G_OBJECT (pop), "cipher-data", cipher,
                            (GDestroyNotify)gpgme_data_release);

    gpgme_set_armor (pop->gctx, seahorse_gconf_get_boolean (ARMOR_KEY));
    gpgme_set_textmode (pop->gctx, FALSE);

    /* Start actual encryption */
    gpgme_signers_clear (pop->gctx);
    if (mode->signer) {
        gpgme_signers_add (pop->gctx, mode->signer);
        gerr = gpgme_op_encrypt_sign_start (pop->gctx, mode->recipients,
                                            GPGME_ENCRYPT_ALWAYS_TRUST, uridata, cipher);

    } else {
        gerr = gpgme_op_encrypt_start (pop->gctx, mode->recipients,
                                       GPGME_ENCRYPT_ALWAYS_TRUST, uridata, cipher);
    }

    if (gerr != 0) {
        seahorse_util_gpgme_to_error (gerr, err);
        return FALSE;
    }

    return TRUE;
}

/* -----------------------------------------------------------------------------
 * SIGN
 */

static gboolean
signer_filter (CryptUIKeyset *ckset, const gchar *key, gpointer user_data)
{
    guint flags = cryptui_keyset_key_flags (ckset, key);
    return flags & CRYPTUI_FLAG_CAN_SIGN;
}

static gpgme_key_t
prompt_signer ()
{
    gpgme_error_t gerr = 0;
    CryptUIKeyset *keyset;
    CryptUIKeyStore *ckstore;
    gpgme_key_t key = NULL;
    gpgme_ctx_t ctx = NULL;
    gchar *signer;
    gchar *id;
    guint count;
    GList *keys;

    keyset = cryptui_keyset_new ("openpgp", TRUE);
    ckstore = cryptui_key_store_new (keyset, TRUE, NULL);
    cryptui_key_store_set_filter (ckstore, signer_filter, NULL);

    count = cryptui_key_store_get_count (ckstore);

    if (count == 0) {
        cryptui_need_to_get_keys ();
        return NULL;
    } else if (count == 1) {
        keys = cryptui_key_store_get_all_keys (ckstore);
        signer = (gchar*) keys->data;
        g_list_free (keys);
    } else {
        signer = cryptui_prompt_signer (keyset, _("Choose Signer"));
    }

    if (signer) {

        id = cryptui_keyset_key_raw_keyid (keyset, signer);
        g_free (signer);

        gpgme_check_version (NULL);
        gerr = gpgme_engine_check_version (GPGME_PROTOCOL_OpenPGP);
        g_return_val_if_fail (gerr == 0, NULL);

        gerr = gpgme_new (&ctx);
        g_return_val_if_fail (gerr == 0, NULL);

        /* Load up the GPGME secret key */
        gerr = gpgme_get_key (ctx, id, &key, 1);
        g_free (id);

        gpgme_release (ctx);

        if (gerr != 0)
            seahorse_util_handle_gpgme (gerr, _("Couldn't load keys"));
    }

    g_object_unref (ckstore);
    g_object_unref (keyset);
    return key;
}

static gboolean
sign_start (SeahorseToolMode *mode, const gchar *uri, gpgme_data_t uridata,
            SeahorsePGPOperation *pop, GError **err)
{
    gpgme_data_t cipher;
    gpgme_error_t gerr;
    gchar *touri;

    g_assert (mode->signer);

    /* File to encrypt to */
    touri = seahorse_util_add_suffix (uri, SEAHORSE_SIG_SUFFIX,
                                      _("Choose Signature File Name for '%s'"));
    if (!touri)
        return FALSE;

    /* Open necessary files, release these with the operation */
    cipher = seahorse_vfs_data_create (touri, SEAHORSE_VFS_WRITE | SEAHORSE_VFS_DELAY, err);
    g_free (touri);
    if (!cipher)
        return FALSE;
    g_object_set_data_full (G_OBJECT (pop), "cipher-data", cipher,
                            (GDestroyNotify)gpgme_data_release);

    gpgme_set_armor (pop->gctx, seahorse_gconf_get_boolean (ARMOR_KEY));
    gpgme_set_textmode (pop->gctx, FALSE);

    /* Start actual signage */
    gpgme_signers_clear (pop->gctx);
    gpgme_signers_add (pop->gctx, mode->signer);
    gerr = gpgme_op_sign_start (pop->gctx, uridata, cipher, GPGME_SIG_MODE_DETACH);
    if (gerr != 0) {
        seahorse_util_gpgme_to_error (gerr, err);
        return FALSE;
    }

    return TRUE;
}

/* -----------------------------------------------------------------------------
 * IMPORT
 */

static gchar *buffer;
static DBusGProxyCall* import_proxy_call;

static void
proxy_call_notification (DBusGProxy *proxy, DBusGProxyCall *call_id,
                         void *data)
{
    SeahorseOperation *op;
    op = SEAHORSE_OPERATION (data);
    seahorse_operation_mark_progress (op, _("Import is complete"), 1);
    seahorse_operation_mark_done (op, FALSE, NULL);

    g_free (buffer);
}

static gboolean
import_start (SeahorseToolMode *mode, const gchar *uri, gpgme_data_t uridata,
              SeahorsePGPOperation *pop, GError **err)
{
    size_t size;
    SeahorseOperation *op;
    gpgme_error_t gerr;

    /* Start actual import */

    op = SEAHORSE_OPERATION (pop);

    init_crypt();

    buffer = g_malloc0(IMPORT_BUFFER_SIZE); /* Allocate memory for key data */

    size = gpgme_data_read (uridata, (void*) buffer, IMPORT_BUFFER_SIZE);

    if (size > 0) {
        import_proxy_call = dbus_g_proxy_begin_call (dbus_key_proxy, "ImportKeys",
                                                     proxy_call_notification,
                                                     pop, NULL,
                                                     G_TYPE_STRING, "openpgp",
                                                     G_TYPE_STRING, buffer,
                                                     G_TYPE_INVALID);

        seahorse_operation_mark_start (op);
        seahorse_operation_mark_progress (op, _("Importing keys ..."), 0.5);
    } else {
        g_free (buffer);

        gerr = gpgme_err_code_from_errno (errno);
        seahorse_util_gpgme_to_error (gerr, err);
        return FALSE;
    }

    return TRUE;
}

static gboolean
import_done (SeahorseToolMode *mode, const gchar *uri, gpgme_data_t uridata,
             SeahorsePGPOperation *pop, GError **err)
{
    gchar **keys, **k;
    gint nkeys = 0;
    gboolean ret;

    ret = dbus_g_proxy_end_call (dbus_key_proxy, import_proxy_call, err,
                                 G_TYPE_STRV, &keys,
                                 G_TYPE_INVALID);

    if (ret) {
        for (k = keys, nkeys = 0; *k; k++)
            nkeys++;
        g_strfreev (keys);

        if (!nkeys)
            seahorse_notification_display(_("Import Failed"),
                                      _("Keys were found but not imported."),
                                      FALSE,
                                      NULL,
                                      NULL);
    }

    g_object_unref (dbus_key_proxy);

    return FALSE;
}

static void
import_show (SeahorseToolMode *mode)
{
    GtkWidget *dlg;
    gchar *t;

    /* TODO: This should eventually use libnotify */

    if (mode->imports <= 0)
        return;

    if (mode->imports == 1)
	t = g_strdup_printf (_("Imported key"));
    else
	t = g_strdup_printf (ngettext ("Imported %d key", "Imported %d keys", mode->imports), mode->imports);

    dlg = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
                                  GTK_MESSAGE_INFO, GTK_BUTTONS_OK, t, NULL);
    g_free (t);
    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
}

/* -----------------------------------------------------------------------------
 * DECRYPT
 */

static gboolean
decrypt_start (SeahorseToolMode *mode, const gchar *uri, gpgme_data_t uridata,
               SeahorsePGPOperation *pop, GError **err)
{
    gpgme_data_t plain;
    gpgme_error_t gerr;
    gchar *touri;

    /* File to decrypt to */
    touri = seahorse_util_remove_suffix (uri, _("Choose Decrypted File Name for '%s'"));
    if (!touri)
        return FALSE;

    /* Open necessary files, release these with the operation */
    plain = seahorse_vfs_data_create (touri, SEAHORSE_VFS_WRITE | SEAHORSE_VFS_DELAY, err);
    g_free (touri);
    if (!plain)
        return FALSE;
    g_object_set_data_full (G_OBJECT (pop), "plain-data", plain,
                            (GDestroyNotify)gpgme_data_release);

    /* Start actual decryption */
    gerr = gpgme_op_decrypt_verify_start (pop->gctx, uridata, plain);
    if (gerr != 0) {
        seahorse_util_gpgme_to_error (gerr, err);
        return FALSE;
    }

    return TRUE;
}

static gboolean
decrypt_done (SeahorseToolMode *mode, const gchar *uri, gpgme_data_t uridata,
              SeahorsePGPOperation *pop, GError **err)
{
    gpgme_verify_result_t status;

    status = gpgme_op_verify_result (pop->gctx);
    if (status && status->signatures)
        seahorse_notify_signatures (uri, status);

    return TRUE;
}

/* -----------------------------------------------------------------------------
 * VERIFY
 */

static gboolean
verify_start (SeahorseToolMode *mode, const gchar *uri, gpgme_data_t uridata,
              SeahorsePGPOperation *pop, GError **err)
{
    gpgme_data_t plain;
    gpgme_error_t gerr;
    gchar *original, *unesc_uri;

    /* File to decrypt to */
    original = seahorse_util_remove_suffix (uri, NULL);

    /* The original file doesn't exist, prompt for it */
    if (!seahorse_util_uri_exists (original)) {

        GtkWidget *dialog;
        gchar *t;

        unesc_uri = g_uri_unescape_string (seahorse_util_uri_get_last (uri), NULL);
        t = g_strdup_printf (_("Choose Original File for '%s'"),
                             unesc_uri);

        dialog = gtk_file_chooser_dialog_new (t,
                                NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                NULL);

        g_free (unesc_uri);
        g_free (t);

        gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (dialog), original);
        gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), FALSE);

        g_free (original);
        original = NULL;

        seahorse_tool_progress_block (TRUE);

        if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
            original = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));

        seahorse_tool_progress_block (FALSE);

        gtk_widget_destroy (dialog);
    }

    if (!original)
        return FALSE;

    g_object_set_data_full (G_OBJECT (pop), "original-file", original, g_free);

    /* Open necessary files, release with operation */
    plain = seahorse_vfs_data_create (original, SEAHORSE_VFS_READ, err);
    if (!plain)
        return FALSE;
    g_object_set_data_full (G_OBJECT (pop), "plain-data", plain,
                            (GDestroyNotify)gpgme_data_release);

    /* Start actual verify */
    gerr = gpgme_op_verify_start (pop->gctx, uridata, plain, NULL);
    if (gerr != 0) {
        seahorse_util_gpgme_to_error (gerr, err);
        return FALSE;
    }

    return TRUE;
}

static gboolean
verify_done (SeahorseToolMode *mode, const gchar *uri, gpgme_data_t uridata,
             SeahorsePGPOperation *pop, GError **err)
{
    const gchar *orig;
    gpgme_verify_result_t status;

    status = gpgme_op_verify_result (pop->gctx);
    if (status && status->signatures) {

        orig = g_object_get_data (G_OBJECT (pop), "original-file");
        if (!orig)
            orig = uri;
        seahorse_notify_signatures (orig, status);

    } else {

        /*
         * TODO: What should happen with multiple files at this point.
         * The last thing we want to do is cascade a big pile of error
         * dialogs at the user.
         */

        g_set_error (err, SEAHORSE_ERROR, -1, _("No valid signatures found"));
        return FALSE;
    }

    return TRUE;
}

/* -----------------------------------------------------------------------------
 * MAIN
 */

/* TODO: Temp. Checks to see if any dialogs are open when we're called
   as a command line app. Once all are gone, closes app */
static gboolean
check_dialogs (gpointer dummy)
{
    if(seahorse_notification_have ())
        return TRUE;

    gtk_main_quit ();
    return FALSE;
}

int
main (int argc, char **argv)
{
    GOptionContext *octx = NULL;
    SeahorseToolMode mode;
    gchar **uris = NULL;
    int ret = 0;

    seahorse_secure_memory_init ();

#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    /*
     * In order to keep the progress window responsive we use a model
     * where two processes communicate with each other. One does the
     * operations, the other shows the progress window, handles cancel.
     */
    seahorse_tool_progress_init (argc, argv);

    octx = g_option_context_new ("");
    g_option_context_add_main_entries (octx, options, GETTEXT_PACKAGE);

    gtk_init_with_args (&argc, &argv, _("File Encryption Tool"), (GOptionEntry *) options, GETTEXT_PACKAGE, NULL);

    /* Load up all our arguments */
    uris = read_uri_arguments ();

    if(!uris || !uris[0]) {
        fprintf (stderr, "seahorse-tool: must specify files\n");
        g_option_context_free (octx);
        return 2;
    }

    /* The basic settings for the operation */
    memset (&mode, 0, sizeof (mode));

    if (mode_encrypt_sign || mode_encrypt) {
        mode.recipients = prompt_recipients (&mode.signer);
        if (mode.recipients) {
            mode.title = _("Encrypting");
            mode.errmsg = _("Couldn't encrypt file: %s");
            mode.startcb = encrypt_sign_start;
            mode.package = TRUE;
        }

    } else if (mode_sign) {
        mode.signer = prompt_signer ();
        if (mode.signer) {
            mode.title = _("Signing");
            mode.errmsg = _("Couldn't sign file: %s");
            mode.startcb = sign_start;
        }

    } else if (mode_import) {
        mode.title = _("Importing");
        mode.errmsg = _("Couldn't import keys from file: %s");
        mode.startcb = import_start;
        mode.donecb = import_done;
        mode.imports = 0;

    } else if (mode_decrypt) {
        mode.title = _("Decrypting");
        mode.errmsg = _("Couldn't decrypt file: %s");
        mode.startcb = decrypt_start;
        mode.donecb = decrypt_done;

    } else if (mode_verify) {
        mode.title = _("Verifying");
        mode.errmsg = _("Couldn't verify file: %s");
        mode.startcb = verify_start;
        mode.donecb = verify_done;

    } else {
        fprintf (stderr, "seahorse-tool: must specify an operation");
        g_option_context_free (octx);
        return 2;
    }

    /* Must at least have a start cb to do something */
    if (mode.startcb) {

        ret = seahorse_tool_files_process (&mode, (const gchar**)uris);

        /* Any results necessary */
        if (ret == 0) {
            if (mode_import)
                import_show (&mode);
        }
    }

    /* TODO: This is temporary code. The actual display of these things should
       be via the daemon. */
    g_idle_add_full (G_PRIORITY_LOW, (GSourceFunc)check_dialogs, NULL, NULL);
    gtk_main ();

    if (mode.recipients)
        seahorse_util_free_keys (mode.recipients);
    if (mode.signer)
        gpgme_key_unref (mode.signer);

    g_strfreev (uris);
    g_option_context_free (octx);

    return ret;
}

