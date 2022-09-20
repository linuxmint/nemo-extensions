/*
 *  nemo-image-resizer.c
 * 
 *  Copyright (C) 2004-2008 Jürg Billeter
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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
 *  Author: Jürg Billeter <j@bitron.ch>
 * 
 */

#ifdef HAVE_CONFIG_H
 #include <config.h> /* for GETTEXT_PACKAGE */
#endif

#include "nemo-image-resizer.h"

#include <string.h>

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include <libnemo-extension/nemo-file-info.h>
 
typedef struct _NemoImageResizerPrivate NemoImageResizerPrivate;

struct _NemoImageResizerPrivate {
	GList *files;
	
	gchar *suffix;
	
	int images_resized;
	int images_total;
	gboolean cancelled;
	
	gchar *size;

	GtkDialog *resize_dialog;
	GtkRadioButton *default_size_radiobutton;
	GtkComboBoxText *size_combobox;
	GtkRadioButton *custom_pct_radiobutton;
	GtkSpinButton *pct_spinbutton;
	GtkRadioButton *custom_size_radiobutton;
	GtkSpinButton *width_spinbutton;
	GtkSpinButton *height_spinbutton;
	GtkRadioButton *append_radiobutton;
	GtkEntry *name_entry;
	GtkRadioButton *inplace_radiobutton;

	GtkWidget *progress_dialog;
	GtkWidget *progress_bar;
	GtkWidget *progress_label;
};

#define NEMO_IMAGE_RESIZER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), NEMO_TYPE_IMAGE_RESIZER, NemoImageResizerPrivate))

G_DEFINE_TYPE (NemoImageResizer, nemo_image_resizer, G_TYPE_OBJECT)

enum {
	PROP_FILES = 1,
};

typedef enum {
	/* Place Signal Types Here */
	SIGNAL_TYPE_EXAMPLE,
	LAST_SIGNAL
} NemoImageResizerSignalType;

static void
nemo_image_resizer_finalize(GObject *object)
{
	NemoImageResizer *dialog = NEMO_IMAGE_RESIZER (object);
	NemoImageResizerPrivate *priv = NEMO_IMAGE_RESIZER_GET_PRIVATE (dialog);
	
	g_free (priv->suffix);
		
	G_OBJECT_CLASS(nemo_image_resizer_parent_class)->finalize(object);
}

static void
nemo_image_resizer_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
	NemoImageResizer *dialog = NEMO_IMAGE_RESIZER (object);
	NemoImageResizerPrivate *priv = NEMO_IMAGE_RESIZER_GET_PRIVATE (dialog);

	switch (property_id) {
	case PROP_FILES:
		priv->files = g_value_get_pointer (value);
		priv->images_total = g_list_length (priv->files);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
		break;
	}
}

static void
nemo_image_resizer_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
	NemoImageResizer *self = NEMO_IMAGE_RESIZER (object);
	NemoImageResizerPrivate *priv = NEMO_IMAGE_RESIZER_GET_PRIVATE (self);

	switch (property_id) {
	case PROP_FILES:
		g_value_set_pointer (value, priv->files);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
		break;
	}
}

static void
nemo_image_resizer_class_init(NemoImageResizerClass *klass)
{
	g_type_class_add_private (klass, sizeof (NemoImageResizerPrivate));

	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *files_param_spec;

	object_class->finalize = nemo_image_resizer_finalize;
	object_class->set_property = nemo_image_resizer_set_property;
	object_class->get_property = nemo_image_resizer_get_property;

	files_param_spec = g_param_spec_pointer ("files",
	"Files",
	"Set selected files",
	G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

	g_object_class_install_property (object_class,
	PROP_FILES,
	files_param_spec);
}

static void run_op (NemoImageResizer *resizer);

static GFile *
nemo_image_resizer_transform_filename (NemoImageResizer *resizer, GFile *orig_file)
{
	NemoImageResizerPrivate *priv = NEMO_IMAGE_RESIZER_GET_PRIVATE (resizer);

	GFile *parent_file, *new_file;
	char *basename, *extension, *new_basename;
	
	g_return_val_if_fail (G_IS_FILE (orig_file), NULL);

	parent_file = g_file_get_parent (orig_file);

	basename = g_strdup (g_file_get_basename (orig_file));
	
	extension = g_strdup (strrchr (basename, '.'));
	if (extension != NULL)
		basename[strlen (basename) - strlen (extension)] = '\0';
		
	new_basename = g_strdup_printf ("%s%s%s", basename,
		priv->suffix == NULL ? ".tmp" : priv->suffix,
		extension == NULL ? "" : extension);
	g_free (basename);
	g_free (extension);

	new_file = g_file_get_child (parent_file, new_basename);

	g_object_unref (parent_file);
	g_free (new_basename);

	return new_file;
}

static void
op_finished (GPid pid, gint status, gpointer data)
{
	NemoImageResizer *resizer = NEMO_IMAGE_RESIZER (data);
	NemoImageResizerPrivate *priv = NEMO_IMAGE_RESIZER_GET_PRIVATE (resizer);
	
	gboolean retry = TRUE;
	
	NemoFileInfo *file = NEMO_FILE_INFO (priv->files->data);
	
	if (status != 0) {
		/* resizing failed */
		char *name = nemo_file_info_get_name (file);

		GtkWidget *msg_dialog = gtk_message_dialog_new (GTK_WINDOW (priv->progress_dialog),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
			GTK_BUTTONS_NONE,
			"'%s' cannot be resized. Check whether you have permission to write to this folder.",
			name);
		g_free (name);
		
		gtk_dialog_add_button (GTK_DIALOG (msg_dialog), _("_Skip"), 1);
		gtk_dialog_add_button (GTK_DIALOG (msg_dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
		gtk_dialog_add_button (GTK_DIALOG (msg_dialog), _("_Retry"), 0);
		gtk_dialog_set_default_response (GTK_DIALOG (msg_dialog), 0);
		
		int response_id = gtk_dialog_run (GTK_DIALOG (msg_dialog));
		gtk_widget_destroy (msg_dialog);
		if (response_id == 0) {
			retry = TRUE;
		} else if (response_id == GTK_RESPONSE_CANCEL) {
			priv->cancelled = TRUE;
		} else if (response_id == 1) {
			retry = FALSE;
		}
		
	} else if (priv->suffix == NULL) {
		/* resize image in place */
		GFile *orig_location = nemo_file_info_get_location (file);
		GFile *new_location = nemo_image_resizer_transform_filename (resizer, orig_location);
		g_file_move (new_location, orig_location, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, NULL);
		g_object_unref (orig_location);
		g_object_unref (new_location);
	}

	if (status == 0 || !retry) {
		/* image has been successfully resized (or skipped) */
		priv->images_resized++;
		priv->files = priv->files->next;
	}
	
	if (!priv->cancelled && priv->files != NULL) {
		/* process next image */
		run_op (resizer);
	} else {
		/* cancel/terminate operation */
		gtk_widget_destroy (priv->progress_dialog);
	}
}

static void
run_op (NemoImageResizer *resizer)
{
	NemoImageResizerPrivate *priv = NEMO_IMAGE_RESIZER_GET_PRIVATE (resizer);
	
	g_return_if_fail (priv->files != NULL);
	
	NemoFileInfo *file = NEMO_FILE_INFO (priv->files->data);

	GFile *orig_location = nemo_file_info_get_location (file);
	char *filename = g_file_get_path (orig_location);
	GFile *new_location = nemo_image_resizer_transform_filename (resizer, orig_location);
	char *new_filename = g_file_get_path (new_location);
	g_object_unref (orig_location);
	g_object_unref (new_location);

	/* FIXME: check whether new_uri already exists and provide "Replace _All", "_Skip", and "_Replace" options */
	
	gchar *argv[6];
	argv[0] = "/usr/bin/convert";
	argv[1] = filename;
	argv[2] = "-resize";
	argv[3] = priv->size;
	argv[4] = new_filename;
	argv[5] = NULL;
	
	pid_t pid;

	if (!g_spawn_async (NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL)) {
		// FIXME: error handling
		return;
	}
	
	g_free (filename);
	g_free (new_filename);
	
	g_child_watch_add (pid, op_finished, resizer);
	
	char *tmp;

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar), (double) (priv->images_resized + 1) / priv->images_total);
	tmp = g_strdup_printf (_("Resizing image: %d of %d"), priv->images_resized + 1, priv->images_total);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), tmp);
	g_free (tmp);
	
	char *name = nemo_file_info_get_name (file);
	tmp = g_strdup_printf (_("<i>Resizing \"%s\"</i>"), name);
	g_free (name);
	gtk_label_set_markup (GTK_LABEL (priv->progress_label), tmp);
	g_free (tmp);
	
}

static void
nemo_image_resizer_response_cb (GtkDialog *dialog, gint response_id, gpointer user_data)
{
	NemoImageResizer *resizer = NEMO_IMAGE_RESIZER (user_data);
	NemoImageResizerPrivate *priv = NEMO_IMAGE_RESIZER_GET_PRIVATE (resizer);

	if (response_id == GTK_RESPONSE_OK) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->append_radiobutton))) {
			if (strlen (gtk_entry_get_text (priv->name_entry)) == 0) {
				GtkWidget *msg_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
					GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK, _("Please enter a valid filename suffix!"));
				gtk_dialog_run (GTK_DIALOG (msg_dialog));
				gtk_widget_destroy (msg_dialog);
				return;
			}
			priv->suffix = g_strdup (gtk_entry_get_text (priv->name_entry));
		}
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->default_size_radiobutton))) {
			priv->size = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (priv->size_combobox));
		} else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->custom_pct_radiobutton))) {
			priv->size = g_strdup_printf ("%d%%", (int) gtk_spin_button_get_value (priv->pct_spinbutton));
		} else {
			priv->size = g_strdup_printf ("%dx%d", (int) gtk_spin_button_get_value (priv->width_spinbutton), (int) gtk_spin_button_get_value (priv->height_spinbutton));
		}
		
		run_op (resizer);
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
nemo_image_resizer_init(NemoImageResizer *resizer)
{
	NemoImageResizerPrivate *priv = NEMO_IMAGE_RESIZER_GET_PRIVATE (resizer);

	GtkBuilder *ui;
	gchar      *path;
	guint       result;
	GError     *err = NULL;

	/* Let's create our gtkbuilder and load the xml file */
	ui = gtk_builder_new ();
	gtk_builder_set_translation_domain (ui, GETTEXT_PACKAGE);
	path = g_build_filename (DATADIR, PACKAGE, "nemo-image-resize.ui", NULL);
	result = gtk_builder_add_from_file (ui, path, &err);
	g_free (path);

	/* If we're unable to load the xml file */
	if (result == 0) {
		g_warning ("%s", err->message);
		g_error_free (err);
		return;
	}

	/* Grab some widgets */
	priv->resize_dialog = GTK_DIALOG (gtk_builder_get_object (ui, "resize_dialog"));
	priv->default_size_radiobutton =
		GTK_RADIO_BUTTON (gtk_builder_get_object (ui, "default_size_radiobutton"));
	priv->size_combobox = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (ui, "comboboxtext_size"));
	priv->custom_pct_radiobutton =
		GTK_RADIO_BUTTON (gtk_builder_get_object (ui, "custom_pct_radiobutton"));
	priv->pct_spinbutton = GTK_SPIN_BUTTON (gtk_builder_get_object (ui, "pct_spinbutton"));
	priv->custom_size_radiobutton =
		GTK_RADIO_BUTTON (gtk_builder_get_object (ui, "custom_size_radiobutton"));
	priv->width_spinbutton = GTK_SPIN_BUTTON (gtk_builder_get_object (ui, "width_spinbutton"));
	priv->height_spinbutton = GTK_SPIN_BUTTON (gtk_builder_get_object (ui, "height_spinbutton"));
	priv->append_radiobutton = GTK_RADIO_BUTTON (gtk_builder_get_object (ui, "append_radiobutton"));
	priv->name_entry = GTK_ENTRY (gtk_builder_get_object (ui, "name_entry"));
	priv->inplace_radiobutton = GTK_RADIO_BUTTON (gtk_builder_get_object (ui, "inplace_radiobutton"));

	/* Connect signal */
	g_signal_connect (G_OBJECT (priv->resize_dialog), "response",
			  (GCallback) nemo_image_resizer_response_cb,
			  resizer);
}

NemoImageResizer *
nemo_image_resizer_new (GList *files)
{
	return g_object_new (NEMO_TYPE_IMAGE_RESIZER, "files", files, NULL);
}

void
nemo_image_resizer_show_dialog (NemoImageResizer *resizer)
{
	NemoImageResizerPrivate *priv = NEMO_IMAGE_RESIZER_GET_PRIVATE (resizer);

	gtk_widget_show (GTK_WIDGET (priv->resize_dialog));
}
