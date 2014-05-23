/*
 *  nemo-wipe - a nemo extension to wipe file(s)
 * 
 *  Copyright (C) 2009-2011 Colomban Wendling <ban@herbesfolles.org>
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

#ifndef NW_PROGRESS_DIALOG_H
#define NW_PROGRESS_DIALOG_H

#include <stdarg.h>
#include <glib.h>
#include <gtk/gtk.h>


G_BEGIN_DECLS


#define NW_TYPE_PROGRESS_DIALOG         (nw_progress_dialog_get_type ())
#define NW_PROGRESS_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), NW_TYPE_PROGRESS_DIALOG, NwProgressDialog))
#define NW_PROGRESS_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), NW_TYPE_PROGRESS_DIALOG, NwProgressDialogClass))
#define NW_IS_PROGRESS_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), NW_TYPE_PROGRESS_DIALOG))
#define NW_IS_PROGRESS_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), NW_TYPE_PROGRESS_DIALOG))
#define NW_PROGRESS_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), NW_TYPE_PROGRESS_DIALOG, NwProgressDialogClass))

typedef struct _NwProgressDialog        NwProgressDialog;
typedef struct _NwProgressDialogClass   NwProgressDialogClass;
typedef struct _NwProgressDialogPrivate NwProgressDialogPrivate;

struct _NwProgressDialog {
  GtkDialog parent_instance;
  NwProgressDialogPrivate *priv;
};

struct _NwProgressDialogClass {
  GtkDialogClass parent_class;
};

#define NW_PROGRESS_DIALOG_RESPONSE_COMPLETE 1


GType         nw_progress_dialog_get_type                   (void) G_GNUC_CONST;

GtkWidget    *nw_progress_dialog_new                        (GtkWindow       *parent,
                                                             GtkDialogFlags   flags,
                                                             const gchar     *format,
                                                             ...) G_GNUC_PRINTF (3, 4);
void          nw_progress_dialog_set_fraction               (NwProgressDialog  *dialog,
                                                             gdouble            fraction);
gdouble       nw_progress_dialog_get_fraction               (NwProgressDialog  *dialog);
void          nw_progress_dialog_pulse                      (NwProgressDialog  *dialog);
void          nw_progress_dialog_set_pulse_step             (NwProgressDialog  *dialog,
                                                             gdouble            fraction);
gdouble       nw_progress_dialog_get_pulse_step             (NwProgressDialog  *dialog);
void          nw_progress_dialog_set_progress_text          (NwProgressDialog  *dialog,
                                                             const gchar       *format,
                                                             ...) G_GNUC_PRINTF (2, 3);
const gchar  *nw_progress_dialog_get_progress_text          (NwProgressDialog  *dialog);
void          nw_progress_dialog_set_text                   (NwProgressDialog  *dialog,
                                                             const gchar       *format,
                                                             ...) G_GNUC_PRINTF (2, 3);
const gchar  *nw_progress_dialog_get_text                   (NwProgressDialog  *dialog);
void          nw_progress_dialog_cancel                     (NwProgressDialog  *dialog);
gboolean      nw_progress_dialog_is_canceled                (NwProgressDialog  *dialog);
void          nw_progress_dialog_finish                     (NwProgressDialog  *dialog,
                                                             gboolean           success);
gboolean      nw_progress_dialog_is_finished                (NwProgressDialog  *dialog);
void          nw_progress_dialog_set_has_close_button       (NwProgressDialog  *dialog,
                                                             gboolean           has_close_button);
gboolean      nw_progress_dialog_get_has_close_button       (NwProgressDialog  *dialog);
void          nw_progress_dialog_set_has_cancel_button      (NwProgressDialog  *dialog,
                                                             gboolean           has_close_button);
gboolean      nw_progress_dialog_get_has_cancel_button      (NwProgressDialog  *dialog);

void          nw_progress_dialog_set_auto_hide_action_area  (NwProgressDialog  *dialog,
                                                             gboolean           auto_hide);
gboolean      nw_progress_dialog_get_auto_hide_action_area  (NwProgressDialog  *dialog);


G_END_DECLS

#endif /* guard */
