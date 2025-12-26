/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  nemo-fits-extension.h
 * 
 *  Copyright (C) 2024 Linux Mint
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
 *  Author: Linux Mint <root@linuxmint.com>
 * 
 */

#ifndef NEMO_FITS_EXTENSION_H
#define NEMO_FITS_EXTENSION_H

#include <glib-object.h>

G_BEGIN_DECLS

#define NEMO_TYPE_FITS_EXTENSION  (nemo_fits_extension_get_type ())
#define NEMO_FITS_EXTENSION(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), NEMO_TYPE_FITS_EXTENSION, NemoFitsExtension))
#define NEMO_IS_FITS_EXTENSION(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), NEMO_TYPE_FITS_EXTENSION))

typedef struct _NemoFitsExtension      NemoFitsExtension;
typedef struct _NemoFitsExtensionClass NemoFitsExtensionClass;

GType nemo_fits_extension_get_type (void);

G_END_DECLS

#endif /* NEMO_FITS_EXTENSION_H */
