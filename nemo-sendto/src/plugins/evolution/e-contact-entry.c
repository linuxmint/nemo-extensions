/* 
 * Copyright (C) 2004 Ross Burton <ross@burtonini.com
 *
 * e-contact-entry.c
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

/*
 * TODO:
 *
 *  Either a clear boolean property, or provide a _clear() method?
 *
 *  set_sources() should keep a ref to the sources and connect signal handlers
 *  to manage the list. Or just expose a GList* of ESource*
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>

#include <libedataserver/e-source.h>
#include <libebook/e-book.h>
#include <libebook/e-book-view.h>
#include <libebook/e-contact.h>

#include "e-contact-entry.h"
#include "econtactentry-marshal.h"

/* Signals */
enum {
  CONTACT_SELECTED, /* Signal argument is the contact. ref it if you want to keep it */
  ERROR,
  STATE_CHANGE,
  LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

/* Properties */
enum {
  PROP_0, /* TODO: why? */
  PROP_SOURCE_LIST,
  PROP_COMPLETE_LENGTH,
};

/*
 * TODO: store ESource* in EntryLookup, remove sources.
 */

struct EContactEntryPriv {
  GtkEntryCompletion *completion;
  GtkListStore *store;
  ESourceList *source_list;
  /* A list of EntryLookup structs we are searching */
  GList *lookup_entries;
  /* Number of characters to start searching at */
  int lookup_length;
  /* A null-terminated array of fields to complete on,
   * n_search_fields includes the terminating NULL */
  EContactField *search_fields;
  int n_search_fields;
  /* Display callback */
  EContactEntryDisplayFunc display_func;
  gpointer display_data;
  GDestroyNotify display_destroy;
};

/**
 * Struct containing details of the sources we are searching.
 */
typedef struct _EntryLookup {
  EContactEntry *entry;
  gboolean open;
  EBookStatus status;
  EBook *book;
  EBookView *bookview;
} EntryLookup;

/**
 * List store columns.
 */
enum {
  COL_NAME,
  COL_IDENTIFIER,
  COL_UID,
  COL_PHOTO,
  COL_LOOKUP,
  COL_TOTAL
};

G_DEFINE_TYPE(EContactEntry, e_contact_entry, GTK_TYPE_ENTRY);

static void lookup_entry_free (EntryLookup *lookup);
static EBookQuery* create_query (EContactEntry *entry, const char* s);
static guint entry_height (GtkWidget *widget);
static const char* stringify_ebook_error (const EBookStatus status);
static void e_contact_entry_item_free (EContactEntyItem *item);
static void entry_changed_cb (GtkEditable *editable, gpointer user_data);

/**
 * The entry was activated.  Take the first contact found and signal the user.
 */
static void
entry_activate_cb (EContactEntry *entry, gpointer user_data)
{
  GtkTreeIter iter;

  g_return_if_fail (E_IS_CONTACT_ENTRY (entry));
  if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (entry->priv->store), &iter)) {
    /*
     * Two possible situations:
     *
     * 1) the search is still in progress
     * 2) the search doesn't match any contacts
     *
     * For (2) we beep. For (1) the user suffers for now, and TODO: add a
     * searching boolean.
     */
    gdk_beep ();
  } else {
    char *uid, *identifier;
    EntryLookup *lookup;
    EContact *contact;
    GError *error = NULL;

    gtk_tree_model_get (GTK_TREE_MODEL (entry->priv->store), &iter, COL_UID, &uid, COL_LOOKUP, &lookup, COL_IDENTIFIER, &identifier, -1);
    g_return_if_fail (lookup != NULL);

    gtk_entry_set_text (GTK_ENTRY (entry), "");

    if (!e_book_get_contact (lookup->book, uid, &contact, &error)) {
      char* message;
      message = g_strdup_printf(_("Cannot get contact: %s"), error->message);
      g_signal_emit (entry, signals[ERROR], 0, message);
      g_free (message);
      g_error_free (error);
    } else {
      g_signal_emit (G_OBJECT (entry), signals[CONTACT_SELECTED], 0, contact, identifier);
      g_object_unref (contact);
    }
    g_free (uid);
    g_free (identifier);

    gtk_list_store_clear (entry->priv->store);
  }
}

/**
 * A contact was selected in the completion drop-down, so send a signal.
 */
static gboolean
completion_match_selected_cb (GtkEntryCompletion *completion, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
  EContactEntry *entry;
  EntryLookup *lookup;
  char *uid, *identifier;
  EContact *contact = NULL;
  GError *error = NULL;

  g_return_val_if_fail (user_data != NULL, TRUE);
  entry = (EContactEntry*)user_data;

  gtk_tree_model_get (model, iter, COL_UID, &uid, COL_LOOKUP, &lookup, COL_IDENTIFIER, &identifier, -1);
  if (!e_book_get_contact (lookup->book, uid, &contact, &error)) {
    char *message;
    message = g_strdup_printf (_("Could not find contact: %s"), error->message);
    g_signal_emit (entry, signals[ERROR], 0, message);
    g_free (message);
    return FALSE;
  }
  g_signal_handlers_block_by_func (G_OBJECT (entry), entry_changed_cb, NULL);
  gtk_entry_set_text (GTK_ENTRY (entry), "");
  g_signal_emit (G_OBJECT (entry), signals[CONTACT_SELECTED], 0, contact, identifier);
  g_signal_handlers_unblock_by_func (G_OBJECT (entry), entry_changed_cb, NULL);
  g_object_unref (contact);
  g_free (uid);
  g_free (identifier);

  gtk_list_store_clear (entry->priv->store);
  return TRUE;
}

static gboolean
completion_match_cb (GtkEntryCompletion *completion, const gchar *key, GtkTreeIter *iter, gpointer user_data)
{
  /* Assumption: all entries in the model are valid candiates */
  /* TODO: do we really need to do all this? */
  char *cell;
  gtk_tree_model_get (gtk_entry_completion_get_model (completion), iter, 0, &cell, -1);
  if (cell == NULL) {
    /* We get NULL cells if we've appended to the store without setting. */
    return FALSE;
  } else {
    /* Is there anything sane to do here? */
    g_free (cell);
    return TRUE;
  }
}

static GList *
e_contact_entry_display_func (EContact *contact)
{
  GList *items, *emails, *l;
  EContactEntyItem *item;

  items = NULL;
  emails = e_contact_get (contact, E_CONTACT_EMAIL);
  for (l = emails; l != NULL; l = l->next) {
    item = g_new0 (EContactEntyItem, 1);
    item->identifier = item->identifier = g_strdup (l->data);
    item->display_string = g_strdup_printf ("%s <%s>", (char*)e_contact_get_const (contact, E_CONTACT_NAME_OR_ORG), item->identifier);

    items = g_list_prepend (items, item);
  }

  return g_list_reverse (items);
}

/* This is the maximum number of entries that GTK+ will show */
#define MAX_ENTRIES 15
/**
 * Callback from the EBookView that more contacts matching the query have been found. Add these to
 * the model if we still want more contacts, or stop the view.
 */
static void
view_contacts_added_cb (EBook *book, GList *contacts, gpointer user_data)
{
  EntryLookup *lookup;
  guint max_height;
  int i;

  g_return_if_fail (user_data != NULL);
  g_return_if_fail (contacts != NULL);
  lookup = (EntryLookup*)user_data;

  max_height = entry_height (GTK_WIDGET (lookup->entry));

  for (i = 0; contacts != NULL && i < MAX_ENTRIES; contacts = g_list_next (contacts)) {
    GtkTreeIter iter;
    EContact *contact;
    EContactPhoto *photo;
    GdkPixbuf *pixbuf = NULL;
    GList *entries, *e;

    entries = NULL;
    contact = E_CONTACT (contacts->data);

    if (lookup->entry->priv->display_func) {
      entries = lookup->entry->priv->display_func (contact, lookup->entry->priv->display_data);
    } else {
      entries = e_contact_entry_display_func (contact);
    }

    /* Don't add the contact to the list if we don't have a string */
    if (entries == NULL)
      return;

    photo = e_contact_get (contact, E_CONTACT_PHOTO);
#ifndef HAVE_ECONTACTPHOTOTYPE
    if (photo) {
#else
    if (photo && photo->type == E_CONTACT_PHOTO_TYPE_INLINED) {
#endif
      GdkPixbufLoader *loader;

      loader = gdk_pixbuf_loader_new ();

#ifndef HAVE_ECONTACTPHOTOTYPE
      if (gdk_pixbuf_loader_write (loader, (guchar *)photo->data,
			      photo->length, NULL))
#else
      if (gdk_pixbuf_loader_write (loader, (guchar *)photo->data.inlined.data,
			      photo->data.inlined.length, NULL))
#endif
        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

      if (pixbuf) {
          GdkPixbuf *tmp;
          gint width = gdk_pixbuf_get_width (pixbuf);
          gint height = gdk_pixbuf_get_height (pixbuf);
          double scale = 1.0;

          if (height > width) {
            scale = max_height / (double) height;
          } else {
            scale = max_height / (double) width;
          }

          if (scale < 1.0) {
            tmp = gdk_pixbuf_scale_simple (pixbuf, width * scale, height * scale, GDK_INTERP_BILINEAR);
            g_object_unref (pixbuf);
            pixbuf = tmp;
          }
      }
    }
    if (photo)
      e_contact_photo_free (photo);

    for (e = entries; e; e = e->next) {
      EContactEntyItem *item = e->data;

      gtk_list_store_append (lookup->entry->priv->store, &iter);
      /* At this point the matcher callback gets called */
      gtk_list_store_set (lookup->entry->priv->store, &iter,
			  COL_NAME, item->display_string,
			  COL_IDENTIFIER, item->identifier,
			  COL_UID, e_contact_get_const (contact, E_CONTACT_UID),
			  COL_PHOTO, pixbuf,
			  COL_LOOKUP, lookup,
			  -1);
      e_contact_entry_item_free (item);
    }
    g_list_free (entries);
    if (pixbuf) g_object_unref (pixbuf);
  }
}

/**
 * The query on the EBookView has completed.
 */
static void
view_completed_cb (EBookView *book_view, EBookViewStatus status, gpointer user_data)
{
  EntryLookup *lookup;
  g_return_if_fail (user_data != NULL);
  /* TODO: handle status != OK */
  g_return_if_fail (status == E_BOOK_ERROR_OK);
  g_return_if_fail (book_view != NULL);

  lookup = (EntryLookup*)user_data;
  g_object_unref (lookup->bookview);
}

/**
 * The EBookView to lookup the completions with has been created.
 */
static void
bookview_cb (EBook *book, EBookStatus status, EBookView *book_view, gpointer closure)
{
  EntryLookup *lookup;
  /* TODO: handle status != OK */
  g_return_if_fail (status == E_BOOK_ERROR_OK);
  g_return_if_fail (closure != NULL);
  
  lookup = (EntryLookup*)closure;
  
  g_object_ref (book_view);
  /* This shouldn't happen of course */
  if (lookup->bookview) {
    e_book_view_stop (lookup->bookview);
    g_object_unref (lookup->bookview);
  }
  lookup->bookview = book_view;
  g_object_add_weak_pointer ((GObject*)book_view, (gpointer*)&lookup->bookview);
  
  g_signal_connect (book_view, "contacts_added", (GCallback)view_contacts_added_cb, lookup);
  g_signal_connect (book_view, "sequence_complete", (GCallback)view_completed_cb, lookup);
  
  e_book_view_start (book_view);
}

static void
entry_changed_cb (GtkEditable *editable, gpointer user_data)
{
  EContactEntry *entry;
  entry = E_CONTACT_ENTRY (editable);

  if (gtk_entry_get_text_length (GTK_ENTRY (editable)) >= entry->priv->lookup_length) {
    GList *l;
    EBookQuery *query;

    /* TODO: I appear to do this to stop duplicates, but its a bit of a hack */
    for (l = entry->priv->lookup_entries; l != NULL; l = l->next) {
      EntryLookup *lookup;
      lookup = (EntryLookup*)l->data;
      if (lookup->bookview) {
        e_book_view_stop (lookup->bookview);
        g_object_unref (lookup->bookview);
      }
    }
    
    gtk_list_store_clear (entry->priv->store);

    query = create_query (entry, gtk_editable_get_chars (editable, 0, -1));
    for (l = entry->priv->lookup_entries; l != NULL; l = l->next) {
      EntryLookup *lookup;
      lookup = (EntryLookup*)l->data;

      /* If the book isn't open yet, skip this source */
      if (!lookup->open)
        continue;
      
      if (e_book_async_get_book_view (lookup->book, query, NULL, 11, (EBookBookViewCallback)bookview_cb, lookup) != 0) {
        g_signal_emit (entry, signals[ERROR], 0, _("Cannot create searchable view."));
      }
    }
    e_book_query_unref (query);
  }
}

static void
book_opened_cb (EBook *book, EBookStatus status, gpointer data)
{
  EntryLookup *lookup;

  g_return_if_fail (book != NULL);
  g_return_if_fail (data != NULL);

  lookup = (EntryLookup*)data;

  /* Don't error out if we're not the last one to open */
  lookup->status = status;
  if (status != E_BOOK_ERROR_OK) {
    GList *l;

    for (l = lookup->entry->priv->lookup_entries; l != NULL; l = l->next) {
      EntryLookup *e = l->data;
      /* Not opened yet is ->open false && ->status not an error */
      if (e->open != FALSE || e->status == E_BOOK_ERROR_OK) {
        /* Don't error yet */
        return;
      }
    }
    
    g_signal_emit (lookup->entry, signals[STATE_CHANGE], 0, FALSE);
    g_signal_emit (lookup->entry, signals[ERROR], 0, stringify_ebook_error (status));
    return;
  }
  lookup->open = TRUE;
  g_signal_emit (lookup->entry, signals[STATE_CHANGE], 0, TRUE);
}


/*
 *
 * Accessors to the fields
 *
 */

void
e_contact_entry_set_source_list (EContactEntry *entry,
    				  ESourceList *source_list)
{
  GError *error = NULL;
  GSList *list, *l;

  g_return_if_fail (E_IS_CONTACT_ENTRY (entry));

  /* Release the old sources */
  if (entry->priv->lookup_entries) {
    g_list_foreach (entry->priv->lookup_entries, (GFunc)lookup_entry_free, NULL);
    g_list_free (entry->priv->lookup_entries);
  }
  if (entry->priv->source_list) {
    g_object_unref (entry->priv->source_list);
  }

  /* If we have no new sources, disable and return here */
  if (source_list == NULL) {
    g_signal_emit (entry, signals[STATE_CHANGE], 0, FALSE);
    entry->priv->source_list = NULL;
    entry->priv->lookup_entries = NULL;
    return;
  }

  entry->priv->source_list = source_list;
  /* So that the list isn't going away underneath us */
  g_object_ref (entry->priv->source_list);

  /* That gets us a list of ESourceGroup */
  list = e_source_list_peek_groups (source_list);
  entry->priv->lookup_entries = NULL;

  for (l = list; l != NULL; l = l->next) {
    ESourceGroup *group = l->data;
    GSList *sources = NULL, *m;
    /* That should give us a list of ESource */
    sources = e_source_group_peek_sources (group);
    for (m = sources; m != NULL; m = m->next) {
      ESource *source = m->data;
      ESource *s = e_source_copy (source);
      EntryLookup *lookup;
      char *uri;

      uri = g_strdup_printf("%s/%s", e_source_group_peek_base_uri (group), e_source_peek_relative_uri (source));
      e_source_set_absolute_uri (s, uri);
      g_free (uri);

      /* Now add those to the lookup entries list */
      lookup = g_new0 (EntryLookup, 1);
      lookup->entry = entry;
      lookup->status = E_BOOK_ERROR_OK;
      lookup->open = FALSE;

      if ((lookup->book = e_book_new (s, &error)) == NULL) {
        /* TODO handle this better, fire the error signal I guess */
        g_warning ("%s", error->message);
	g_error_free (error);
	g_free (lookup);
      } else {
        entry->priv->lookup_entries = g_list_append (entry->priv->lookup_entries, lookup);
	e_book_async_open(lookup->book, TRUE, (EBookCallback)book_opened_cb, lookup);
      }

      g_object_unref (s);
    }
  }

  if (entry->priv->lookup_entries == NULL)
    g_signal_emit (entry, signals[STATE_CHANGE], 0, FALSE);
}

ESourceList *
e_contact_entry_get_source_list (EContactEntry *entry)
{
  g_return_val_if_fail (E_IS_CONTACT_ENTRY (entry), NULL);

  return entry->priv->source_list;
}

void
e_contact_entry_set_complete_length (EContactEntry *entry, int length)
{
  g_return_if_fail (E_IS_CONTACT_ENTRY (entry));
  g_return_if_fail (length >= 1);

  entry->priv->lookup_length = length;
  gtk_entry_completion_set_minimum_key_length (entry->priv->completion, entry->priv->lookup_length);
}

int
e_contact_entry_get_complete_length (EContactEntry *entry)
{
  g_return_val_if_fail (E_IS_CONTACT_ENTRY (entry), 3); /* TODO: from paramspec? */
  
  return entry->priv->lookup_length;
}

void
e_contact_entry_set_display_func (EContactEntry *entry, EContactEntryDisplayFunc func, gpointer func_data, GDestroyNotify destroy)
{
  g_return_if_fail (E_IS_CONTACT_ENTRY (entry));

  if (entry->priv->display_destroy) {
    entry->priv->display_destroy (entry->priv->display_func);
  }
  
  entry->priv->display_func = func;
  entry->priv->display_data = func_data;
  entry->priv->display_destroy = destroy;
}

void
e_contact_entry_set_search_fields (EContactEntry *entry, const EContactField *fields)
{
  int i;

  g_free (entry->priv->search_fields);
  i = 0;
  while (fields[i] != 0) {
    i++;
  }

  entry->priv->search_fields = g_new0 (EContactField, i + 1);
  memcpy (entry->priv->search_fields, fields, sizeof (EContactField) * (i + 1));
  entry->priv->n_search_fields = i + 1;
}

/*
 *
 * GObject functions
 *
 */

static void
e_contact_entry_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
  EContactEntry *entry;
  
  g_return_if_fail (E_IS_CONTACT_ENTRY (object));
  entry = E_CONTACT_ENTRY (object);
  
  switch (property_id) {
  case PROP_SOURCE_LIST:
    e_contact_entry_set_source_list (entry, g_value_get_object (value));
    break;
  case PROP_COMPLETE_LENGTH:
    e_contact_entry_set_complete_length (entry, g_value_get_int (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
e_contact_entry_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
  EContactEntry *entry;
  g_return_if_fail (E_IS_CONTACT_ENTRY (object));
  entry = E_CONTACT_ENTRY (object);
  
  switch (property_id) {
  case PROP_SOURCE_LIST:
    g_value_set_object (value, e_contact_entry_get_source_list (entry));
    break;
  case PROP_COMPLETE_LENGTH:
    g_value_set_int (value, e_contact_entry_get_complete_length (entry));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
e_contact_entry_finalize (GObject *object)
{
  GList *l;
  EContactEntry *entry = (EContactEntry *)object;
  if (entry->priv) {
    for (l = entry->priv->lookup_entries; l != NULL; l = g_list_next (l)) {
      lookup_entry_free (l->data);
    }
    g_free (entry->priv->search_fields);
    g_list_free (entry->priv->lookup_entries);
    g_object_unref (entry->priv->completion);
    g_object_unref (entry->priv->store);
    g_object_unref (entry->priv->source_list);

    if (entry->priv->display_destroy) {
      entry->priv->display_destroy (entry->priv->display_func);
    }
    g_free (entry->priv);
  }
  G_OBJECT_CLASS (e_contact_entry_parent_class)->finalize (object);
}

static void
reset_search_fields (EContactEntry *entry)
{
  EContactField fields[] = { E_CONTACT_FULL_NAME, E_CONTACT_EMAIL, E_CONTACT_NICKNAME, E_CONTACT_ORG, 0 };

  g_free (entry->priv->search_fields);
  entry->priv->search_fields = g_new0 (EContactField, G_N_ELEMENTS (fields));
  memcpy (entry->priv->search_fields, fields, sizeof (fields));
  entry->priv->n_search_fields = G_N_ELEMENTS (fields);
}

static void
e_contact_entry_init (EContactEntry *entry)
{
  GtkCellRenderer *renderer;

  entry->priv = g_new0 (EContactEntryPriv, 1);

  g_signal_connect (entry, "activate", G_CALLBACK (entry_activate_cb), NULL);
  g_signal_connect (entry, "changed", G_CALLBACK (entry_changed_cb), NULL);

  entry->priv->store = gtk_list_store_new (COL_TOTAL, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_POINTER);

  entry->priv->search_fields = NULL;
  reset_search_fields (entry);

  entry->priv->completion = gtk_entry_completion_new ();
  gtk_entry_completion_set_popup_set_width (entry->priv->completion, FALSE);
  gtk_entry_completion_set_model (entry->priv->completion, GTK_TREE_MODEL (entry->priv->store));
  gtk_entry_completion_set_match_func (entry->priv->completion, (GtkEntryCompletionMatchFunc)completion_match_cb, NULL, NULL);
  g_signal_connect (entry->priv->completion, "match-selected", G_CALLBACK (completion_match_selected_cb), entry);
  e_contact_entry_set_complete_length (entry, G_PARAM_SPEC_INT (g_object_class_find_property (G_OBJECT_GET_CLASS(entry), "complete-length"))->default_value);
  gtk_entry_set_completion (GTK_ENTRY (entry), entry->priv->completion);

  /* Photo */
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (entry->priv->completion), renderer, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (entry->priv->completion), renderer, "pixbuf", COL_PHOTO);

  /* Name */
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (entry->priv->completion), renderer, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (entry->priv->completion), renderer, "text", COL_NAME);

  entry->priv->lookup_entries = NULL;

  entry->priv->display_func = entry->priv->display_data = entry->priv->display_destroy = NULL;
}

static void
e_contact_entry_class_init (EContactEntryClass *klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  
  object_class = (GObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;
  
  /* GObject */
  object_class->set_property = e_contact_entry_set_property;
  object_class->get_property = e_contact_entry_get_property;
  object_class->finalize = e_contact_entry_finalize;

  /* Properties */
  g_object_class_install_property (object_class, PROP_SOURCE_LIST,
                                   g_param_spec_object ("source-list", "Source List", "The source list to search for contacts.",
                                                        E_TYPE_SOURCE_LIST, G_PARAM_READWRITE));
  
  g_object_class_install_property (object_class, PROP_COMPLETE_LENGTH,
                                   g_param_spec_int ("complete-length", "Complete length", "Number of characters to start a search on.",
                                                     2, 99, 3, G_PARAM_READWRITE));
  
  /* Signals */
  signals[CONTACT_SELECTED] = g_signal_new ("contact-selected",
                                            G_TYPE_FROM_CLASS (object_class),
                                            G_SIGNAL_RUN_LAST,
                                            G_STRUCT_OFFSET (EContactEntryClass, contact_selected),
                                            NULL, NULL,
                                            econtactentry_marshal_VOID__OBJECT_STRING,
                                            G_TYPE_NONE, 2, E_TYPE_CONTACT, G_TYPE_STRING);
  
  signals[ERROR] = g_signal_new ("error",
                                 G_TYPE_FROM_CLASS (object_class),
                                 G_SIGNAL_RUN_LAST,
                                 G_STRUCT_OFFSET (EContactEntryClass, error),
                                 NULL, NULL,
                                 g_cclosure_marshal_VOID__STRING,
                                 G_TYPE_NONE, 1, G_TYPE_STRING);
  signals[STATE_CHANGE] = g_signal_new ("state-change",
		  			G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_LAST,
					G_STRUCT_OFFSET (EContactEntryClass, state_change),
					NULL, NULL,
					g_cclosure_marshal_VOID__BOOLEAN,
					G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

GtkWidget *
e_contact_entry_new (void)
{
  return g_object_new (e_contact_entry_get_type (), NULL);
}


/*
 *
 * Miscellaneous utility functions.
 *
 */

static void
lookup_entry_free (EntryLookup *lookup)
{
  /* We didn't take a reference for -> entry so don't unref it here. Yet. */
  g_return_if_fail (lookup != NULL);
  /* TODO: unref entry if we ref it anywhere */
  if (lookup->bookview) {
    g_warning("EBookView still around");
    g_object_unref (lookup->bookview);
  }
  if (lookup->book) {
    g_object_unref (lookup->book);
  } else {
    g_warning ("EntryLookup object with no book member");
  }
  g_free (lookup);
}

/**
 * Split a string of tokens separated by whitespace into an array of tokens.
 */
static GArray *
split_query_string (const gchar *str)
{
  GArray *parts = g_array_sized_new (FALSE, FALSE, sizeof (char *), 2);
  PangoLogAttr *attrs;
  guint str_len = strlen (str), word_start = 0, i;
  
  attrs = g_new0 (PangoLogAttr, str_len + 1);  
  /* TODO: do we need to specify a particular language or is NULL ok? */
  pango_get_log_attrs (str, -1, -1, NULL, attrs, str_len + 1);
  
  for (i = 0; i < str_len + 1; i++) {
    char *start_word, *end_word, *word;
    if (attrs[i].is_word_end) {
      start_word = g_utf8_offset_to_pointer (str, word_start);
      end_word = g_utf8_offset_to_pointer (str, i);
      word  = g_strndup (start_word, end_word - start_word);
      g_array_append_val (parts, word);
    }
    if (attrs[i].is_word_start) {
      word_start = i;
    }
  }
  g_free (attrs);
  return parts;
}

/**
 * Create a query which looks for the specified string in a contact's full name, email addresses and
 * nick name.
 */
static EBookQuery*
create_query (EContactEntry *entry, const char* s)
{
  EBookQuery *query;
  GArray *parts = split_query_string (s);
  /* TODO: ORG doesn't appear to work */
  EBookQuery ***field_queries;
  EBookQuery **q;
  guint j;
  int i;

  q = g_new0 (EBookQuery *, entry->priv->n_search_fields - 1);
  field_queries = g_new0 (EBookQuery **, entry->priv->n_search_fields - 1);

  for (i = 0; i < entry->priv->n_search_fields - 1; i++) {
    field_queries[i] = g_new0 (EBookQuery *, parts->len);
    for (j = 0; j < parts->len; j++) {
        field_queries[i][j] = e_book_query_field_test (entry->priv->search_fields[i], E_BOOK_QUERY_CONTAINS, g_array_index (parts, gchar *, j));
    }
    q[i] = e_book_query_and (parts->len, field_queries[i], TRUE);
  }
  g_array_free (parts, TRUE);

  query = e_book_query_or (entry->priv->n_search_fields - 1, q, TRUE);

  for (i = 0; i < entry->priv->n_search_fields - 1; i++) {
    g_free (field_queries[i]);
  }
  g_free (field_queries);
  g_free (q);

  return query;
}

/**
 * Given a widget, determines the height that text will normally be drawn.
 */
static guint
entry_height (GtkWidget *widget)
{
  PangoLayout *layout;
  int bound;
  g_return_val_if_fail (widget != NULL, 0);
  layout = gtk_widget_create_pango_layout (widget, NULL);
  pango_layout_get_pixel_size (layout, NULL, &bound);
  return bound;
}

/**
 * Free a EContactEntyItem struct.
 */
static void
e_contact_entry_item_free (EContactEntyItem *item)
{
  g_free (item->display_string);
  g_free (item->identifier);
  g_free (item);
}

/**
 * Return a string representing a given EBook status code.
 */
static const char*
stringify_ebook_error(const EBookStatus status)
{
  switch (status) {
  case E_BOOK_ERROR_OK:
    return _("Success");
  case E_BOOK_ERROR_INVALID_ARG:
    return _("An argument was invalid.");
  case E_BOOK_ERROR_BUSY:
    return _("The address book is busy.");
  case E_BOOK_ERROR_REPOSITORY_OFFLINE:
    return _("The address book is offline.");
  case E_BOOK_ERROR_NO_SUCH_BOOK:
    return _("The address book does not exist.");
  case E_BOOK_ERROR_NO_SELF_CONTACT:
    return _("The \"Me\" contact does not exist.");
  case E_BOOK_ERROR_SOURCE_NOT_LOADED:
    return _("The address book is not loaded.");
  case E_BOOK_ERROR_SOURCE_ALREADY_LOADED:
    return _("The address book is already loaded.");
  case E_BOOK_ERROR_PERMISSION_DENIED:
    return _("Permission was denied when accessing the address book.");
  case E_BOOK_ERROR_CONTACT_NOT_FOUND:
    return _("The contact was not found.");
  case E_BOOK_ERROR_CONTACT_ID_ALREADY_EXISTS:
    return _("This contact ID already exists.");
  case E_BOOK_ERROR_PROTOCOL_NOT_SUPPORTED:
    return _("The protocol is not supported.");
  case E_BOOK_ERROR_CANCELLED:
    return _("The operation was cancelled.");
  case E_BOOK_ERROR_COULD_NOT_CANCEL:
    return _("The operation could not be cancelled.");
  case E_BOOK_ERROR_AUTHENTICATION_FAILED:
    return _("The address book authentication failed.");
  case E_BOOK_ERROR_AUTHENTICATION_REQUIRED:
    return _("Authentication is required to access the address book and was not given.");
  case E_BOOK_ERROR_TLS_NOT_AVAILABLE:
    return _("A secure connection is not available.");
  case E_BOOK_ERROR_CORBA_EXCEPTION:
    return _("A CORBA error occurred whilst accessing the address book.");
  case E_BOOK_ERROR_NO_SUCH_SOURCE:
    return _("The address book source does not exist.");
  case E_BOOK_ERROR_OTHER_ERROR:
    return _("An unknown error occurred.");
  default:
    g_warning ("Unknown status %d", status);
    return _("An unknown error occurred.");
  }
}
