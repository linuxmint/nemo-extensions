/*
 *  nemo-image-converter.h
 * 
 *  Copyright (C) 2004-2005 Jürg Billeter
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

#ifndef NEMO_IMAGE_CONVERTER_H
#define NEMO_IMAGE_CONVERTER_H

#include <glib-object.h>

G_BEGIN_DECLS

/* Declarations for the open terminal extension object.  This object will be
 * instantiated by nemo.  It implements the GInterfaces 
 * exported by libnemo. */


#define NEMO_TYPE_IMAGE_CONVERTER	  (nemo_image_converter_get_type ())
#define NEMO_IMAGE_CONVERTER(o)		  (G_TYPE_CHECK_INSTANCE_CAST ((o), NEMO_TYPE_IMAGE_CONVERTER, NemoImageConverter))
#define NEMO_IS_IMAGE_CONVERTER(o)	  (G_TYPE_CHECK_INSTANCE_TYPE ((o), NEMO_TYPE_IMAGE_CONVERTER))
typedef struct _NemoImageConverter	  NemoImageConverter;
typedef struct _NemoImageConverterClass	  NemoImageConverterClass;

struct _NemoImageConverter {
	GObject parent_slot;
};

struct _NemoImageConverterClass {
	GObjectClass parent_slot;
};

GType nemo_image_converter_get_type      (void);
void  nemo_image_converter_register_type (GTypeModule *module);

G_END_DECLS

#endif
