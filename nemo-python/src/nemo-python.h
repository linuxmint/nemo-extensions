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

#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

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

PyTypeObject *_PyGtkWidget_Type;
#define PyGtkWidget_Type (*_PyGtkWidget_Type)

PyTypeObject *_PyNemoColumn_Type;
#define PyNemoColumn_Type (*_PyNemoColumn_Type)

PyTypeObject *_PyNemoColumnProvider_Type;
#define PyNemoColumnProvider_Type (*_PyNemoColumnProvider_Type)

PyTypeObject *_PyNemoInfoProvider_Type;
#define PyNemoInfoProvider_Type (*_PyNemoInfoProvider_Type)

PyTypeObject *_PyNemoLocationWidgetProvider_Type;
#define PyNemoLocationWidgetProvider_Type (*_PyNemoLocationWidgetProvider_Type)

PyTypeObject *_PyNemoMenu_Type;
#define PyNemoMenu_Type (*_PyNemoMenu_Type)

PyTypeObject *_PyNemoMenuItem_Type;
#define PyNemoMenuItem_Type (*_PyNemoMenuItem_Type)

PyTypeObject *_PyNemoMenuProvider_Type;
#define PyNemoMenuProvider_Type (*_PyNemoMenuProvider_Type)

PyTypeObject *_PyNemoPropertyPage_Type;
#define PyNemoPropertyPage_Type (*_PyNemoPropertyPage_Type)

PyTypeObject *_PyNemoPropertyPageProvider_Type;
#define PyNemoPropertyPageProvider_Type (*_PyNemoPropertyPageProvider_Type)

PyTypeObject *_PyNemoOperationHandle_Type;
#define PyNemoOperationHandle_Type (*_PyNemoOperationHandle_Type)

#endif /* NEMO_PYTHON_H */
