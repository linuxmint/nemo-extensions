/*
 * Seahorse
 *
 * Copyright (C) 2003 Jacob Perkins
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
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#ifndef __SEAHORSE_WIDGET_H__
#define __SEAHORSE_WIDGET_H__

#include <glib.h>
#include <gtk/gtk.h>

#define SEAHORSE_TYPE_WIDGET            (seahorse_widget_get_type ())
#define SEAHORSE_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SEAHORSE_TYPE_WIDGET, SeahorseWidget))
#define SEAHORSE_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SEAHORSE_TYPE_WIDGET, SeahorseWidgetClass))
#define SEAHORSE_IS_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SEAHORSE_TYPE_WIDGET))
#define SEAHORSE_IS_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SEAHORSE_TYPE_WIDGET))
#define SEAHORSE_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SEAHORSE_TYPE_WIDGET, SeahorseWidgetClass))

typedef struct _SeahorseWidget SeahorseWidget;
typedef struct _SeahorseWidgetClass SeahorseWidgetClass;

/**
 * SeahorseWidget:
 * @parent: The parent #GtkObject
 * @gtkbuilder: The #GtkBuilder object for the #SeahorseWidget
 * @name: The name of the gtkbuilder file
 *
 * A window created from a gtkbuilder file.
 *
 * - All SeahorseWidget objects are destroyed when the SeahorseContext
 *   goes bye-bye.
 * - Implements fun GtkUIManager stuff.
 *
 * Signals:
 *   destroy: The window was destroyed.
 *
 * Properties:
 *   name: (gchar*) The name of the gtkbuilder file to load.
 */

struct _SeahorseWidget {
	GObject parent;

	/*< public >*/
	GtkBuilder *gtkbuilder;
	gchar *name;

	/*< private >*/
	gboolean destroying;
	gboolean in_destruction;
};

struct _SeahorseWidgetClass {
	GObjectClass parent_class;

	/*< signals >*/
	void (*destroy) (SeahorseWidget *swidget);
};

GType            seahorse_widget_get_type (void) G_GNUC_CONST;

SeahorseWidget*  seahorse_widget_new                (const gchar      *name,
                                                     GtkWindow        *parent);

SeahorseWidget*  seahorse_widget_new_allow_multiple (const gchar      *name,
                                                     GtkWindow        *parent);

SeahorseWidget*  seahorse_widget_find               (const gchar      *name);

GtkWidget*       seahorse_widget_get_toplevel       (SeahorseWidget   *swidget);

GtkWidget*       seahorse_widget_get_widget         (SeahorseWidget   *swidget,
                                                     const char       *identifier);

void             seahorse_widget_destroy            (SeahorseWidget   *swidget);

#endif /* __SEAHORSE_WIDGET_H__ */
