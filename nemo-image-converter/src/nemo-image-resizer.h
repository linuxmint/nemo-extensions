/*
 *  nemo-image-resize-dialog.h
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

#ifndef NEMO_IMAGE_RESIZER_H
#define NEMO_IMAGE_RESIZER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define NEMO_TYPE_IMAGE_RESIZER         (nemo_image_resizer_get_type ())
#define NEMO_IMAGE_RESIZER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), NEMO_TYPE_IMAGE_RESIZER, NemoImageResizer))
#define NEMO_IMAGE_RESIZER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), NEMO_TYPE_IMAGE_RESIZER, NemoImageResizerClass))
#define NEMO_IS_IMAGE_RESIZER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), NEMO_TYPE_IMAGE_RESIZER))
#define NEMO_IS_IMAGE_RESIZER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), NEMO_TYPE_IMAGE_RESIZER))
#define NEMO_IMAGE_RESIZER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), NEMO_TYPE_IMAGE_RESIZER, NemoImageResizerClass))

typedef struct _NemoImageResizer NemoImageResizer;
typedef struct _NemoImageResizerClass NemoImageResizerClass;

struct _NemoImageResizer {
	GObject parent;
};

struct _NemoImageResizerClass {
	GObjectClass parent_class;
	/* Add Signal Functions Here */
};

GType nemo_image_resizer_get_type (void);
NemoImageResizer *nemo_image_resizer_new (GList *files);
void nemo_image_resizer_show_dialog (NemoImageResizer *dialog);

G_END_DECLS

#endif /* NEMO_IMAGE_RESIZER_H */
