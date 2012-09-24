/*
 * Seahorse
 *
 * Copyright (C) 2010 Stefan Walter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Suite 500, Boston, MA
 * 02110-1335, USA.
 */

#ifndef __SEAHORSE_SECURE_BUFFER_H__
#define __SEAHORSE_SECURE_BUFFER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SEAHORSE_TYPE_SECURE_BUFFER            (seahorse_secure_buffer_get_type ())
#define SEAHORSE_SECURE_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SEAHORSE_TYPE_SECURE_BUFFER, SeahorseSecureBuffer))
#define SEAHORSE_SECURE_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SEAHORSE_TYPE_SECURE_BUFFER, SeahorseSecureBufferClass))
#define SEAHORSE_IS_SECURE_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SEAHORSE_TYPE_SECURE_BUFFER))
#define SEAHORSE_IS_SECURE_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SEAHORSE_TYPE_SECURE_BUFFER))
#define SEAHORSE_SECURE_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SEAHORSE_TYPE_SECURE_BUFFER, SeahorseSecureBufferClass))

typedef struct _SeahorseSecureBuffer            SeahorseSecureBuffer;
typedef struct _SeahorseSecureBufferClass       SeahorseSecureBufferClass;
typedef struct _SeahorseSecureBufferPrivate     SeahorseSecureBufferPrivate;

struct _SeahorseSecureBuffer
{
	GtkEntryBuffer parent;
	SeahorseSecureBufferPrivate *priv;
};

struct _SeahorseSecureBufferClass
{
	GtkEntryBufferClass parent_class;
};

GType                     seahorse_secure_buffer_get_type               (void) G_GNUC_CONST;

GtkEntryBuffer*           seahorse_secure_buffer_new                    (void);

G_END_DECLS

#endif /* __SEAHORSE_SECURE_BUFFER_H__ */
