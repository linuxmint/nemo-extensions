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

#ifndef NEMO_PYTHON_H
#define NEMO_PYTHON_H

#include <glib-object.h>
#include <glib/gprintf.h>
#include <Python.h>

typedef enum {
    NEMO_PYTHON_DEBUG_MISC = 1 << 0,
} NemoPythonDebug;

extern NemoPythonDebug nemo_python_debug;

#define debug(x) { if (nemo_python_debug & NEMO_PYTHON_DEBUG_MISC) \
                       g_printf( "nemo-python:" x "\n"); }
#define debug_enter()  { if (nemo_python_debug & NEMO_PYTHON_DEBUG_MISC) \
                             g_printf("%s: entered\n", __FUNCTION__); }
#define debug_enter_args(x, y) { if (nemo_python_debug & NEMO_PYTHON_DEBUG_MISC) \
                                     g_printf("%s: entered " x "\n", __FUNCTION__, y); }

extern PyTypeObject *_PyGtkWidget_Type;
#define PyGtkWidget_Type (*_PyGtkWidget_Type)

extern PyTypeObject *_PyNemoColumn_Type;
#define PyNemoColumn_Type (*_PyNemoColumn_Type)

extern PyTypeObject *_PyNemoColumnProvider_Type;
#define PyNemoColumnProvider_Type (*_PyNemoColumnProvider_Type)

extern PyTypeObject *_PyNemoInfoProvider_Type;
#define PyNemoInfoProvider_Type (*_PyNemoInfoProvider_Type)

extern PyTypeObject *_PyNemoLocationWidgetProvider_Type;
#define PyNemoLocationWidgetProvider_Type (*_PyNemoLocationWidgetProvider_Type)

extern PyTypeObject *_PyNemoNameAndDescProvider_Type;
#define PyNemoNameAndDescProvider_Type (*_PyNemoNameAndDescProvider_Type)

extern PyTypeObject *_PyNemoMenu_Type;
#define PyNemoMenu_Type (*_PyNemoMenu_Type)

extern PyTypeObject *_PyNemoMenuItem_Type;
#define PyNemoMenuItem_Type (*_PyNemoMenuItem_Type)

extern PyTypeObject *_PyNemoMenuProvider_Type;
#define PyNemoMenuProvider_Type (*_PyNemoMenuProvider_Type)

extern PyTypeObject *_PyNemoPropertyPage_Type;
#define PyNemoPropertyPage_Type (*_PyNemoPropertyPage_Type)

extern PyTypeObject *_PyNemoPropertyPageProvider_Type;
#define PyNemoPropertyPageProvider_Type (*_PyNemoPropertyPageProvider_Type)

extern PyTypeObject *_PyNemoOperationHandle_Type;
#define PyNemoOperationHandle_Type (*_PyNemoOperationHandle_Type)

#endif /* NEMO_PYTHON_H */
