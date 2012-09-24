/* 
 * Copyright (C) 2004 Ross Burton <ross@burtonini.com>
 *
 * e-contact-entry.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * Authors: Ross Burton <ross@burtonini.com>
 */

#ifndef CONTACT_ENTRY_H
#define CONTACT_ENTRY_H

#include <libedataserver/e-source-group.h>
#include <libedataserver/e-source-list.h>
#include <libebook/e-contact.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define E_TYPE_CONTACT_ENTRY (e_contact_entry_get_type())
#define E_CONTACT_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
				E_TYPE_CONTACT_ENTRY, EContactEntry))
#define E_CONTACT_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
				E_TYPE_CONTACT_ENTRY, EContactEntryClass))
#define E_IS_CONTACT_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
						E_TYPE_CONTACT_ENTRY))
#define E_IS_CONTACT_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
						E_TYPE_CONTACT_ENTRY))
#define E_GET_CONTACT_ENTRY_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
				E_TYPE_CONTACT_ENTRY, EContactEntryClass))

typedef struct EContactEntryPriv EContactEntryPriv;

typedef struct {
  GtkEntry parent;
  EContactEntryPriv *priv;
} EContactEntry;

typedef struct {
  GtkEntryClass parent_class;
  /* Signal fired when a contact is selected. Use this over 'activate' */
  void (*contact_selected) (GtkWidget *entry, EContact *contact, const char *identifier);
  /* Signal fired when an async error occured */
  void (*error) (GtkWidget *entry, const char* error);
  /* Signal fired when the widget's state should change */
  void (*state_change) (GtkWidget *entry, gboolean state);
} EContactEntryClass;

typedef struct {
  char *display_string;
  char *identifier; /* a "unique" identifier */
} EContactEntyItem;

/* A GList of EContactEntyItems */
typedef GList* (*EContactEntryDisplayFunc) (EContact *contact, gpointer data);

GType e_contact_entry_get_type (void);

GtkWidget *e_contact_entry_new (void);

void e_contact_entry_set_source_list (EContactEntry *entry, ESourceList *list);
ESourceList *e_contact_entry_get_source_list (EContactEntry *entry);

void e_contact_entry_set_complete_length(EContactEntry *entry, int length);
int e_contact_entry_get_complete_length(EContactEntry *entry);

void e_contact_entry_set_display_func (EContactEntry *entry, EContactEntryDisplayFunc func, gpointer func_data, GDestroyNotify destroy);

void e_contact_entry_set_search_fields (EContactEntry *entry, const EContactField *fields);

G_END_DECLS

#endif /* CONTACT_ENTRY_H */
