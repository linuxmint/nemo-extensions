/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  Copyright (C) 2004,2005 Johan Dahlin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef NEMO_PYTHON_OBJECT_H
#define NEMO_PYTHON_OBJECT_H

#include <Python.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _NemoPythonObject 		NemoPythonObject;
typedef struct _NemoPythonObjectClass 	NemoPythonObjectClass;

struct _NemoPythonObject {
	GObject parent_slot;
	PyObject *instance;
};

struct _NemoPythonObjectClass {
	GObjectClass parent_slot;
	PyObject *type;
};

GType nemo_python_object_get_type (GTypeModule *module, PyObject *type);

G_END_DECLS

#endif
