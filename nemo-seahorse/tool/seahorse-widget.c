/*
 * Seahorse
 *
 * Copyright (C) 2002 Jacob Perkins
 * Copyright (C) 2005 Stefan Walter
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

#include <config.h>

#include <string.h>

#include <glib/gi18n.h>

#include "seahorse-widget.h"

#define STATUS "status"

enum {
	PROP_0,
	PROP_NAME
};

enum {
	DESTROY,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void     class_init          (SeahorseWidgetClass    *klass);
static void     object_init         (SeahorseWidget         *swidget);

static void     object_dispose      (GObject                *object);
static void     object_finalize     (GObject                *gobject);

static void     object_set_property (GObject                *object,
                                     guint                  prop_id,
                                     const GValue           *value,
                                     GParamSpec             *pspec);

static void     object_get_property (GObject                *object,
                                     guint                  prop_id,
                                     GValue                 *value,
                                     GParamSpec             *pspec);

/* signal functions */
G_MODULE_EXPORT void on_widget_closed   (GtkWidget             *widget,
                                              SeahorseWidget        *swidget);

G_MODULE_EXPORT void on_widget_help     (GtkWidget             *widget,
                                              SeahorseWidget        *swidget);

G_MODULE_EXPORT gboolean on_widget_delete_event  (GtkWidget             *widget,
                                                       GdkEvent              *event,
                                                       SeahorseWidget        *swidget);

static GObjectClass *parent_class = NULL;

/* Hash of widgets with name as key */
static GHashTable *widgets = NULL;

GType
seahorse_widget_get_type (void)
{
	static GType widget_type = 0;

	if (!widget_type) {
		static const GTypeInfo widget_info = {
			sizeof (SeahorseWidgetClass), NULL, NULL,
			(GClassInitFunc) class_init,
			NULL, NULL, sizeof (SeahorseWidget), 0, (GInstanceInitFunc) object_init
		};

		widget_type = g_type_register_static (G_TYPE_OBJECT, "SeahorseWidget",
		                                      &widget_info, 0);
	}

	return widget_type;
}

static void
seahorse_widget_constructed (GObject *object)
{
	SeahorseWidget *self = SEAHORSE_WIDGET (object);
	GtkWindow *window;
	gint width, height;
	gchar *path;

	G_OBJECT_CLASS (parent_class)->constructed (object);

	/* Load window size for windows that aren't dialogs */
	window = GTK_WINDOW (seahorse_widget_get_toplevel (self));
	if (!GTK_IS_DIALOG (window)) {
		path = g_strdup_printf ("/org/nemo/plugins/seahorse/windows/%s/", self->name);
		self->settings = g_settings_new_with_path ("org.nemo.plugins.seahorse.window", path);
		g_free (path);

		width = g_settings_get_int (self->settings, "width");
		height = g_settings_get_int (self->settings, "height");

		if (width > 0 && height > 0)
			gtk_window_resize (window, width, height);
	}
}

static void
class_init (SeahorseWidgetClass *klass)
{
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->constructed = seahorse_widget_constructed;
	gobject_class->dispose = object_dispose;
	gobject_class->finalize = object_finalize;
	gobject_class->set_property = object_set_property;
	gobject_class->get_property = object_get_property;

	g_object_class_install_property (gobject_class, PROP_NAME,
	         g_param_spec_string ("name", "Widget name", "Name of gtkbuilder file and main widget",
	                              NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	signals[DESTROY] = g_signal_new ("destroy", SEAHORSE_TYPE_WIDGET,
	                                 G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (SeahorseWidgetClass, destroy),
	                                 NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
object_init (SeahorseWidget *swidget)
{

}

static void
object_dispose (GObject *object)
{
	SeahorseWidget *swidget = SEAHORSE_WIDGET (object);

	if (!swidget->in_destruction) {
		swidget->in_destruction = TRUE;
		g_signal_emit (swidget, signals[DESTROY], 0);
		swidget->in_destruction = FALSE;
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/* Disconnects callbacks, destroys main window widget,
 * and frees the xml definition and any other data */
static void
object_finalize (GObject *gobject)
{
	SeahorseWidget *swidget;

	swidget = SEAHORSE_WIDGET (gobject);

	/* Remove widget from hash and destroy hash if empty */
    if (widgets) {
    	g_hash_table_remove (widgets, swidget->name);
    	if (g_hash_table_size (widgets) == 0) {
    		g_hash_table_destroy (widgets);
    		widgets = NULL;
    	}
    }

    if (seahorse_widget_get_widget (swidget, swidget->name))
        gtk_widget_destroy (GTK_WIDGET (seahorse_widget_get_widget (swidget, swidget->name)));

	g_object_unref (swidget->gtkbuilder);
	swidget->gtkbuilder = NULL;

	g_clear_object (&swidget->settings);
	g_free (swidget->name);

	G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
object_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SeahorseWidget *swidget;
    GError *error = NULL;
    GtkWidget *w;
    char *path;

    swidget = SEAHORSE_WIDGET (object);

    switch (prop_id) {
    /* Loads gtkbuilder xml definition from name, connects common callbacks */
    case PROP_NAME:
        g_return_if_fail (swidget->name == NULL);
        swidget->name = g_value_dup_string (value);
        path = g_strdup_printf ("%sseahorse-%s.xml",
                                SEAHORSE_UIDIR, swidget->name);
        swidget->gtkbuilder = gtk_builder_new ();
        gtk_builder_add_from_file (swidget->gtkbuilder, path, &error);
        if (error != NULL) {
            g_warning ("couldn't load ui file: %s", error->message);
            g_error_free (error);
        }
        g_free (path);
        g_return_if_fail (swidget->gtkbuilder != NULL);

        gtk_builder_connect_signals (swidget->gtkbuilder, swidget);

        w = GTK_WIDGET (seahorse_widget_get_widget (swidget, swidget->name));
        /*TODO: glade_xml_ensure_accel (swidget->gtkbuilder);*/

        gtk_window_set_icon_name (GTK_WINDOW (w), "seahorse");
        break;
    }
}

static void
object_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	SeahorseWidget *swidget;
	swidget = SEAHORSE_WIDGET (object);

	switch (prop_id) {
		case PROP_NAME:
			g_value_set_string (value, swidget->name);
			break;
		default:
			break;
	}
}

/* Destroys widget */
G_MODULE_EXPORT void
on_widget_closed (GtkWidget *widget, SeahorseWidget *swidget)
{
	seahorse_widget_destroy (swidget);
}

/* Closed widget */
G_MODULE_EXPORT gboolean
on_widget_delete_event (GtkWidget *widget, GdkEvent *event, SeahorseWidget *swidget)
{
	on_widget_closed (widget, swidget);
    return FALSE; /* propogate event */
}

/**
 * seahorse_widget_new:
 * @name: Name of widget, filename part of gtkbuilder file, and name of main window
 * @parent: GtkWindow to make the parent of the new swidget
 *
 * Creates a new #SeahorseWidget.
 *
 * Returns: The new #SeahorseWidget, or NULL if the widget already exists
 **/
SeahorseWidget*
seahorse_widget_new (const gchar *name, GtkWindow *parent)
{
        /* Check if have widget hash */
    SeahorseWidget *swidget = seahorse_widget_find (name);
    GtkWindow *window;

    /* If widget already exists, present */
    if (swidget != NULL) {
        gtk_window_present (GTK_WINDOW (seahorse_widget_get_widget (swidget, swidget->name)));
        return NULL;
    }

    /* If widget doesn't already exist, create & insert into hash */
    swidget = g_object_new (SEAHORSE_TYPE_WIDGET, "name", name, NULL);
    if(!widgets)
        widgets = g_hash_table_new ((GHashFunc)g_str_hash, (GCompareFunc)g_str_equal);
    g_hash_table_insert (widgets, g_strdup (name), swidget);

    if (parent != NULL) {
        window = GTK_WINDOW (seahorse_widget_get_widget (swidget, swidget->name));
        gtk_window_set_transient_for (window, parent);
    }

    return swidget;
}

/**
 * seahorse_widget_new_allow_multiple:
 * @name: Name of widget, filename part of gtkbuilder file, and name of main window
 * @parent: GtkWindow to make the parent of the new swidget
 *
 * Creates a new #SeahorseWidget without checking if it already exists.
 *
 * Returns: The new #SeahorseWidget
 **/
SeahorseWidget*
seahorse_widget_new_allow_multiple (const gchar *name, GtkWindow *parent)
{
    GtkWindow *window;
    SeahorseWidget *swidget = g_object_new (SEAHORSE_TYPE_WIDGET, "name", name,  NULL);

    if (parent != NULL) {
        window = GTK_WINDOW (seahorse_widget_get_widget (swidget, swidget->name));
        gtk_window_set_transient_for (window, parent);
    }

	gtk_builder_connect_signals (swidget->gtkbuilder, NULL);

	return swidget;
}

SeahorseWidget*
seahorse_widget_find (const gchar *name)
{
    /* Check if have widget hash */
    if (widgets != NULL)
        return SEAHORSE_WIDGET (g_hash_table_lookup (widgets, name));
    return NULL;
}

/**
 * seahorse_widget_get_toplevel
 * @swidget: The seahorse widget
 *
 * Return the top level widget in this seahorse widget
 *
 * Returns: The top level widget
 **/
GtkWidget*
seahorse_widget_get_toplevel (SeahorseWidget     *swidget)
{
    GtkWidget *widget = GTK_WIDGET (seahorse_widget_get_widget (swidget, swidget->name));
    g_return_val_if_fail (widget != NULL, NULL);
    return widget;
}

GtkWidget*
seahorse_widget_get_widget (SeahorseWidget *swidget, const char *identifier)
{
    GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (swidget->gtkbuilder, identifier));
    if (widget == NULL)
	    g_warning ("could not find widget %s for seahorse-%s.xml", identifier, swidget->name);
    return widget;
}

/**
 * seahorse_widget_destroy:
 * @swidget: #SeahorseWidget to destroy
 *
 * Unrefs @swidget.
 **/
void
seahorse_widget_destroy (SeahorseWidget *self)
{
	GtkWidget *widget;
	gint width, height;

	g_return_if_fail (self != NULL && SEAHORSE_IS_WIDGET (self));
	widget = seahorse_widget_get_toplevel (self);

	/* Save window size */
	if (self->settings) {
		gtk_window_get_size (GTK_WINDOW (widget), &width, &height);
		g_settings_set_int (self->settings, "width", width);
		g_settings_set_int (self->settings, "height", height);
	}

	/* Destroy Widget */
	if (!self->destroying) {
		self->destroying = TRUE;
		gtk_widget_destroy (seahorse_widget_get_toplevel (self));
		g_object_unref (self);
	}

}
