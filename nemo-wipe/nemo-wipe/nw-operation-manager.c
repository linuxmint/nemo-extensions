/*
 *  nemo-wipe - a nemo extension to wipe file(s)
 * 
 *  Copyright (C) 2009-2012 Colomban Wendling <ban@herbesfolles.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "nw-operation-manager.h"

#include <stdarg.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gsecuredelete/gsecuredelete.h>

#include "nw-progress-dialog.h"
#include "nw-compat.h"


static GtkResponseType  display_dialog     (GtkWindow       *parent,
                                            GtkMessageType   type,
                                            gboolean         wait_for_response,
                                            const gchar     *primary_text,
                                            const gchar     *secondary_text,
                                            const gchar     *first_button_text,
                                            ...) G_GNUC_NULL_TERMINATED;

/*
 * display_dialog:
 * @parent: Parent window, or %NULL
 * @type: The dialog type
 * @wait_for_response: Whether to wait for the dialog's response. Waiting for
 *                     the response force a modal-like dialog (using
 *                     gtk_dialog_run()), but allow to get the dialog's
 *                     response. If this options is %TRUE, this function will
 *                     always return %GTK_RESPONSE_NONE.
 *                     Use this if you want a modal dialog.
 * @primary_text: GtkMessageDialog's primary text
 * @secondary_text: GtkMessageDialog's secondary text, or %NULL
 * @first_button_text: Text of the first button, or %NULL
 * @...: (starting at @first_button_text) %NULL-terminated list of buttons text
 *       and response-id.
 * 
 * Returns: The dialog's response or %GTK_RESPONSE_NONE if @wait_for_response
 *          is %FALSE.
 */
static GtkResponseType
display_dialog (GtkWindow       *parent,
                GtkMessageType   type,
                gboolean         wait_for_response,
                const gchar     *primary_text,
                const gchar     *secondary_text,
                const gchar     *first_button_text,
                ...)
{
  GtkResponseType response = GTK_RESPONSE_NONE;
  GtkWidget      *dialog;
  va_list         ap;
  
  dialog = gtk_message_dialog_new (parent,
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   type, GTK_BUTTONS_NONE,
                                   "%s", primary_text);
  if (secondary_text) {
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              "%s", secondary_text);
  }
  va_start (ap, first_button_text);
  while (first_button_text) {
    GtkResponseType button_response = va_arg (ap, GtkResponseType);
    
    gtk_dialog_add_button (GTK_DIALOG (dialog), first_button_text, button_response);
    first_button_text = va_arg (ap, const gchar *);
  }
  va_end (ap);
  /* show the dialog */
  if (wait_for_response) {
    response = gtk_dialog_run (GTK_DIALOG (dialog));
    /* if not already destroyed by the parent */
    if (GTK_IS_WIDGET (dialog)) {
      gtk_widget_destroy (dialog);
    }
  } else {
    g_signal_connect (dialog, "response", G_CALLBACK (gtk_widget_destroy), NULL);
    gtk_widget_show (dialog);
  }
  
  return response;
}



/* Gets the last line (delimited by \n) in @str.
 * If the last character is \n, gets the chunk between the the previous \n and
 * the last one.
 * Free the returned string with g_free(). */
static gchar *
string_last_line (const gchar *str)
{
  gchar        *last_line;
  const gchar  *prev_eol  = str;
  const gchar  *eol       = str;
  gsize         i;
  
  for (i = 0; str[i] != 0; i++) {
    if (str[i] == '\n') {
      prev_eol = eol;
      eol = (&str[i]) + 1;
    }
  }
  if (*eol != 0 || eol == str) {
    last_line = g_strdup (eol);
  } else {
    last_line = g_strndup (prev_eol, (gsize)(eol - 1 - prev_eol));
  }
  
  return last_line;
}



struct NwOperationData
{
  NwOperation        *operation;
  GtkWindow          *window;
  gulong              window_destroy_hid;
  NwProgressDialog   *progress_dialog;
  gchar              *failed_primary_text;
  gchar              *success_primary_text;
  gchar              *success_secondary_text;
};

/* Frees a NwOperationData structure */
static void
free_opdata (struct NwOperationData *opdata)
{
  if (opdata->window_destroy_hid) {
    g_signal_handler_disconnect (opdata->window, opdata->window_destroy_hid);
  }
  if (opdata->operation) {
    g_object_unref (opdata->operation);
  }
  g_free (opdata->failed_primary_text);
  g_free (opdata->success_primary_text);
  g_free (opdata->success_secondary_text);
  g_slice_free1 (sizeof *opdata, opdata);
}

/* if the parent window get destroyed, we honor gently the thing and leave it
 * to the death. doing this is useful not to have a bad window pointer later */
static void
opdata_window_destroy_handler (GtkWidget               *obj,
                               struct NwOperationData  *opdata)
{
  g_signal_handler_disconnect (opdata->window, opdata->window_destroy_hid);
  opdata->window_destroy_hid = 0;
  opdata->window = NULL;
}

/* Displays an operation's error */
static void
display_operation_error (struct NwOperationData  *opdata,
                         const gchar             *error)
{
  GtkWidget      *dialog;
  GtkWidget      *content_area;
  GtkWidget      *expander;
  GtkWidget      *scroll;
  GtkWidget      *view;
  GtkTextBuffer  *buffer;
  gchar          *short_error;
  
  dialog = gtk_message_dialog_new (opdata->window,
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE,
                                   "%s", opdata->failed_primary_text);
  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
  /* we hope that the last line in the error message is meaningful */
  short_error = string_last_line (error);
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            "%s", short_error);
  g_free (short_error);
  /* add the details expander */
  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  expander = gtk_expander_new_with_mnemonic (_("_Details"));
  gtk_container_add (GTK_CONTAINER (content_area), expander);
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (expander), scroll);
  buffer = gtk_text_buffer_new (NULL);
  gtk_text_buffer_set_text (buffer, error, -1);
  view = gtk_text_view_new_with_buffer (buffer);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scroll), view);
  gtk_widget_show_all (expander);
  /* show the dialog */
  g_signal_connect (dialog, "response", G_CALLBACK (gtk_widget_destroy), NULL);
  gtk_widget_show (dialog);
}

static void
operation_finished_handler (GsdDeleteOperation *operation,
                            gboolean            success,
                            const gchar        *error,
                            gpointer            data)
{
  struct NwOperationData *opdata = data;
  
  gtk_widget_destroy (GTK_WIDGET (opdata->progress_dialog));
  if (! success) {
    display_operation_error (opdata, error);
  } else {
    display_dialog (opdata->window, GTK_MESSAGE_INFO, FALSE,
                    opdata->success_primary_text,
                    opdata->success_secondary_text,
                    GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                    NULL);
  }
  free_opdata (opdata);
}

static void
operation_progress_handler (GsdDeleteOperation *operation,
                            gdouble             fraction,
                            gpointer            data)
{
  struct NwOperationData *opdata = data;
  
  nw_progress_dialog_set_fraction (opdata->progress_dialog, fraction);
}

/* sets @pref according to state of @toggle */
static void
pref_bool_toggle_changed_handler (GtkToggleButton *toggle,
                                  gboolean        *pref)
{
  *pref = gtk_toggle_button_get_active (toggle);
}

/* sets @pref to the value of the selected row, column 0 of combo's model */
static void
pref_enum_combo_changed_handler (GtkComboBox *combo,
                                 gint        *pref)
{
  GtkTreeIter   iter;
  
  if (gtk_combo_box_get_active_iter (combo, &iter)) {
    GtkTreeModel *model = gtk_combo_box_get_model (combo);
    
    gtk_tree_model_get (model, &iter, 0, pref, -1);
  }
}

static void
help_button_clicked_handler (GtkWidget *widget,
                             gpointer   data)
{
  GtkWindow  *parent = data;
  GError     *err = NULL;
  
  if (! gtk_show_uri (gtk_widget_get_screen (widget),
                      "ghelp:nemo-wipe?nemo-wipe-config",
                      gtk_get_current_event_time (),
                      &err)) {
    /* display the error.
     * here we cannot use non-blocking dialog since we are called from a
     * dialog ran by gtk_dialog_run(), then the dialog must be ran the same way
     * to get events */
    display_dialog (parent, GTK_MESSAGE_ERROR, TRUE,
                    _("Failed to open help"),
                    err->message,
                    GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                    NULL);
    g_error_free (err);
  }
}

/*
 * operation_confirm_dialog:
 * @parent: Parent window, or %NULL for none
 * @primary_text: Dialog's primary text
 * @secondary_text: Dialog's secondary text
 * @confirm_button_text: Text of the button to hit in order to confirm (can be a
 *                       stock item)
 * @confirm_button_icon: A #GtkWidget to use as the confirmation button's icon,
 *                       or %NULL for none or the default (e.g. if
 *                       @confirm_button_text is a stock item that have an icon)
 * @fast: return location for the Gsd.SecureDeleteOperation:fast setting, or
 *        %NULL
 * @delete_mode: return location for the Gsd.SecureDeleteOperation:mode setting,
 *               or %NULL
 * @zeroise: return location for the Gsd.ZeroableOperation:zeroise setting, or
 *           %NULL
 */
static gboolean
operation_confirm_dialog (GtkWindow                    *parent,
                          const gchar                  *primary_text,
                          const gchar                  *secondary_text,
                          const gchar                  *confirm_button_text,
                          GtkWidget                    *confirm_button_icon,
                          gboolean                     *fast,
                          GsdSecureDeleteOperationMode *delete_mode,
                          gboolean                     *zeroise)
{
  GtkResponseType response = GTK_RESPONSE_NONE;
  GtkWidget      *button;
  GtkWidget      *dialog;
  GtkWidget      *action_area;
  
  dialog = gtk_message_dialog_new (parent,
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                   "%s", primary_text);
  action_area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));
  if (secondary_text) {
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              "%s", secondary_text);
  }
  /* help button. don't use response not to close the dialog on click */
  button = gtk_button_new_from_stock (GTK_STOCK_HELP);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (help_button_clicked_handler), dialog);
  gtk_box_pack_start (GTK_BOX (action_area), button, FALSE, TRUE, 0);
  if (GTK_IS_BUTTON_BOX (action_area)) {
    gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (action_area), button, TRUE);
  }
  gtk_widget_show (button);
  /* cancel button */
  gtk_dialog_add_button (GTK_DIALOG (dialog),
                         GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT);
  /* launch button */
  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                  confirm_button_text, GTK_RESPONSE_ACCEPT);
  if (confirm_button_icon) {
    gtk_button_set_image (GTK_BUTTON (button), confirm_button_icon);
  }
  /* if we have settings to choose */
  if (fast || delete_mode || zeroise) {
    GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    GtkWidget *expander;
    GtkWidget *box;
    
    expander = gtk_expander_new_with_mnemonic (_("_Options"));
    gtk_container_add (GTK_CONTAINER (content_area), expander);
    box = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (expander), box);
    /* delete mode option */
    if (delete_mode) {
      GtkWidget        *hbox;
      GtkWidget        *label;
      GtkWidget        *combo;
      GtkListStore     *store;
      GtkCellRenderer  *renderer;
      
      hbox = gtk_hbox_new (FALSE, 5);
      gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, TRUE, 0);
      label = gtk_label_new_with_mnemonic (_("Number of _passes:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
      /* store columns: setting value     (enum)
       *                number of passes  (int)
       *                descriptive text  (string) */
      store = gtk_list_store_new (3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);
      combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
      /* number of passes column */
      renderer = gtk_cell_renderer_text_new ();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
                                      "text", 1, NULL);
      /* comment column */
      renderer = gtk_cell_renderer_text_new ();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
                                      "text", 2, NULL);
      /* Adds an item.
       * @value: the setting to return if selected
       * @n_pass: the number of pass this setting shows
       * @text: description text for this setting */
      #define ADD_ITEM(value, n_pass, text)                                    \
        G_STMT_START {                                                         \
          GtkTreeIter iter;                                                    \
                                                                               \
          gtk_list_store_append (store, &iter);                                \
          gtk_list_store_set (store, &iter, 0, value, 1, #n_pass, 2, text, -1);\
          if (value == *delete_mode) {                                         \
              gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);    \
          }                                                                    \
        } G_STMT_END
      /* add items */
      ADD_ITEM (GSD_SECURE_DELETE_OPERATION_MODE_NORMAL,
                38, _("(Gutmann method for old disks)"));
      ADD_ITEM (GSD_SECURE_DELETE_OPERATION_MODE_INSECURE,
                2, _("(advised for modern hard disks)"));
      ADD_ITEM (GSD_SECURE_DELETE_OPERATION_MODE_VERY_INSECURE,
                1, _("(only protects against software attacks)"));
      
      #undef ADD_ITEM
      /* connect change & pack */
      g_signal_connect (combo, "changed",
                        G_CALLBACK (pref_enum_combo_changed_handler), delete_mode);
      gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, TRUE, 0);
    }
    /* fast option */
    if (fast) {
      GtkWidget *check;
      
      check = gtk_check_button_new_with_mnemonic (
        _("_Fast and insecure mode (no /dev/urandom, no synchronize mode)")
      );
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), *fast);
      g_signal_connect (check, "toggled",
                        G_CALLBACK (pref_bool_toggle_changed_handler), fast);
      gtk_box_pack_start (GTK_BOX (box), check, FALSE, TRUE, 0);
    }
    /* "zeroise" option */
    if (zeroise) {
      GtkWidget *check;
      
      check = gtk_check_button_new_with_mnemonic (
        _("Last pass with _zeros instead of random data")
      );
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), *zeroise);
      g_signal_connect (check, "toggled",
                        G_CALLBACK (pref_bool_toggle_changed_handler), zeroise);
      gtk_box_pack_start (GTK_BOX (box), check, FALSE, TRUE, 0);
    }
    gtk_widget_show_all (expander);
  }
  /* run the dialog */
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  
  return response == GTK_RESPONSE_ACCEPT;
}

static void
progress_dialog_response_handler (GtkDialog *dialog,
                                  gint       response_id,
                                  gpointer   data)
{
  struct NwOperationData *opdata = data;
  
  switch (response_id) {
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_DELETE_EVENT:
      if (display_dialog (GTK_WINDOW (dialog), GTK_MESSAGE_QUESTION, TRUE,
                          _("Are you sure you want to cancel this operation?"),
                          _("Canceling this operation might leave some item(s) in "
                            "an intermediate state."),
                          _("Resume operation"), GTK_RESPONSE_REJECT,
                          _("Cancel operation"), GTK_RESPONSE_ACCEPT,
                          NULL) == GTK_RESPONSE_ACCEPT) {
        gsd_async_operation_cancel (GSD_ASYNC_OPERATION (opdata->operation));
      }
      break;
    
    default:
      break;
  }
}

/* 
 * nw_operation_manager_run:
 * @parent: Parent window for dialogs
 * @files: List of paths to pass to @operation_launcher_func
 * @confirm_primary_text: Primary text for the confirmation dialog
 * @confirm_secondary_text: Secondary text for the confirmation dialog
 * @confirm_button_text: Text for the confirm button of the confirmation dialog.
 *                       It may be a GTK stock item.
 * @confirm_button_icon: A #GtkWidget to use as the confirmation button's icon,
 *                       or %NULL for none or the default (e.g. if
 *                       @confirm_button_text is a stock item that have an icon)
 * @progress_dialog_text: Text for the progress dialog
 * @operation: (transfer:full): the operation object
 * @failed_primary_text: Primary text of the dialog displayed if operation failed.
 *                       (secondary is the error message)
 * @success_primary_text: Primary text for the the success dialog
 * @success_secondary_text: Secondary text for the the success dialog
 * 
 * 
 */
void
nw_operation_manager_run (GtkWindow    *parent,
                          GList        *files,
                          const gchar  *confirm_primary_text,
                          const gchar  *confirm_secondary_text,
                          const gchar  *confirm_button_text,
                          GtkWidget    *confirm_button_icon,
                          const gchar  *progress_dialog_text,
                          NwOperation  *operation,
                          const gchar  *failed_primary_text,
                          const gchar  *success_primary_text,
                          const gchar  *success_secondary_text)
{
  /* if the user confirms, try to launch the operation */
  gboolean                      fast        = FALSE;
  GsdSecureDeleteOperationMode  delete_mode = GSD_SECURE_DELETE_OPERATION_MODE_INSECURE;
  gboolean                      zeroise     = FALSE;
  
  if (! operation_confirm_dialog (parent,
                                  confirm_primary_text, confirm_secondary_text,
                                  confirm_button_text, confirm_button_icon,
                                  &fast, &delete_mode, &zeroise)) {
    g_object_unref (operation);
  } else {
    GError                 *err = NULL;
    struct NwOperationData *opdata;
    
    opdata = g_slice_alloc (sizeof *opdata);
    opdata->window = parent;
    opdata->window_destroy_hid = g_signal_connect (opdata->window, "destroy",
                                                   G_CALLBACK (opdata_window_destroy_handler), opdata);
    opdata->progress_dialog = NW_PROGRESS_DIALOG (nw_progress_dialog_new (opdata->window, 0,
                                                                          "%s", progress_dialog_text));
    nw_progress_dialog_set_has_cancel_button (opdata->progress_dialog, TRUE);
    g_signal_connect (opdata->progress_dialog, "response",
                      G_CALLBACK (progress_dialog_response_handler), opdata);
    opdata->failed_primary_text = g_strdup (failed_primary_text);
    opdata->success_primary_text = g_strdup (success_primary_text);
    opdata->success_secondary_text = g_strdup (success_secondary_text);
    opdata->operation = operation;
    g_object_set (operation,
                  "fast", fast,
                  "mode", delete_mode,
                  "zeroise", zeroise,
                  NULL);
    g_signal_connect (opdata->operation, "finished",
                      G_CALLBACK (operation_finished_handler), opdata);
    g_signal_connect (opdata->operation, "progress",
                      G_CALLBACK (operation_progress_handler), opdata);
    
    nw_operation_add_files (opdata->operation, files);
    if (! gsd_secure_delete_operation_run (GSD_SECURE_DELETE_OPERATION (opdata->operation),
                                           &err)) {
      if (err->code == G_SPAWN_ERROR_NOENT) {
        gchar *message;
        
        /* Merge the error message with our. Pretty much a hack, but should be
         * correct and more precise. */
        message = g_strdup_printf (_("%s. "
                                     "Please make sure you have the secure-delete "
                                     "package properly installed on your system."),
                                   err->message);
        display_operation_error (opdata, message);
        g_free (message);
      } else {
        display_operation_error (opdata, err->message);
      }
      g_error_free (err);
      gtk_widget_destroy (GTK_WIDGET (opdata->progress_dialog));
      free_opdata (opdata);
    } else {
      gtk_widget_show (GTK_WIDGET (opdata->progress_dialog));
    }
  }
}

