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

#include <config.h>

#include "nemo-python-object.h"
#include "nemo-python.h"

#include <libnemo-extension/nemo-extension-types.h>

#define NO_IMPORT_PYGOBJECT
#include <pygobject.h>

/* Nemo extension headers */
#include <libnemo-extension/nemo-file-info.h>
#include <libnemo-extension/nemo-info-provider.h>
#include <libnemo-extension/nemo-column-provider.h>
#include <libnemo-extension/nemo-location-widget-provider.h>
#include <libnemo-extension/nemo-menu-item.h>
#include <libnemo-extension/nemo-menu-provider.h>
#include <libnemo-extension/nemo-property-page-provider.h>
#include <libnemo-extension/nemo-name-and-desc-provider.h>

#include <string.h>

#define METHOD_PREFIX ""

static GObjectClass *parent_class;

/* These macros assumes the following things:
 *   a METHOD_NAME is defined with is a string
 *   a goto label called beach
 *   the return value is called ret
 */

#define CHECK_METHOD_NAME(self)                                        \
	if (!PyObject_HasAttrString(self, METHOD_NAME))                    \
		goto beach;

#define CHECK_OBJECT(object)										   \
  	if (object->instance == NULL)									   \
  	{																   \
  		g_object_unref (object);									   \
  		goto beach;													   \
  	}																   \

#define CONVERT_LIST(py_files, files)                                  \
	{                                                                  \
		GList *l;                                                      \
        py_files = PyList_New(0);                                      \
		for (l = files; l; l = l->next)                                \
		{                                                              \
            PyObject *item = pygobject_new ((GObject *)l->data);       \
            PyList_Append(py_files, item);                             \
            Py_DECREF (item);                                          \
		}                                                              \
	}

#define HANDLE_RETVAL(py_ret)                                          \
    if (!py_ret)                                                       \
    {                                                                  \
        PyErr_Print();                                                 \
		goto beach;                                                    \
 	}                                                                  \
 	else if (py_ret == Py_None)                                        \
 	{                                                                  \
 		goto beach;                                                    \
	}

#define HANDLE_LIST(py_ret, type, type_name)                           \
    {                                                                  \
        Py_ssize_t i = 0;                                              \
    	if (!PySequence_Check(py_ret) || PyUnicode_Check(py_ret))       \
    	{                                                              \
    		PyErr_SetString(PyExc_TypeError,                           \
    						METHOD_NAME " must return a sequence");    \
    		goto beach;                                                \
    	}                                                              \
    	for (i = 0; i < PySequence_Size (py_ret); i++)                 \
    	{                                                              \
    		PyGObject *py_item;                                        \
    		py_item = (PyGObject*)PySequence_GetItem (py_ret, i);      \
    		if (!pygobject_check(py_item, &Py##type##_Type))           \
    		{                                                          \
    			PyErr_SetString(PyExc_TypeError,                       \
    							METHOD_NAME                            \
    							" must return a sequence of "          \
    							type_name);                            \
    			goto beach;                                            \
    		}                                                          \
    		ret = g_list_append (ret, (type*) g_object_ref(py_item->obj));  \
            Py_DECREF(py_item);                                        \
    	}                                                              \
    }


static void
free_pygobject_data(gpointer data, gpointer user_data)
{
	/* Some NemoFile objects are cached and not freed until nemo
		itself is closed.  Since PyGObject stores data that must be freed by
		the Python interpreter, we must always free it before the interpreter
		is finalized. */
	g_object_set_data((GObject *)data, "PyGObject::instance-data", NULL);
}

static void
free_pygobject_data_list(GList *list)
{
	if (list == NULL)
		return;
		
	g_list_foreach(list, (GFunc)free_pygobject_data, NULL);
}

static PyObject *
nemo_python_boxed_new (PyTypeObject *type, gpointer boxed, gboolean free_on_dealloc)
{
	PyGBoxed *self = (PyGBoxed *) type->tp_alloc (type, 0);
	self->gtype = pyg_type_from_object ( (PyObject *) type);
	self->boxed = boxed;
	self->free_on_dealloc = free_on_dealloc;
	
	return (PyObject *) self;
}

#define METHOD_NAME "get_name_and_desc"
static GList *
nemo_python_object_get_name_and_desc (NemoNameAndDescProvider *provider)
{
    NemoPythonObject *object = (NemoPythonObject*)provider;
    PyObject *py_ret = NULL;
    GList *ret = NULL;
    PyGILState_STATE state = pyg_gil_state_ensure();
    
    debug_enter();

    CHECK_OBJECT(object);
    CHECK_METHOD_NAME(object->instance);

    py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX METHOD_NAME, NULL);
    HANDLE_RETVAL(py_ret);

    int i;
    for (i = 0; i < PySequence_Size (py_ret); i++)
        {
            PyObject *py_item;
            py_item = PySequence_GetItem (py_ret, i);
            if (!PyUnicode_Check(py_item))
            {
                PyErr_SetString(PyExc_TypeError,
                                METHOD_NAME
                                " must return a sequence of strings");
                goto beach;
            }
            ret = g_list_append (ret, (gchar *) PyUnicode_AsUTF8(py_item));
            Py_DECREF(py_item);
        }
 beach:
    Py_XDECREF(py_ret);
    pyg_gil_state_release(state);
    return ret;
}
#undef METHOD_NAME

static void
nemo_python_object_name_and_desc_provider_iface_init (NemoNameAndDescProviderIface *iface)
{
    iface->get_name_and_desc = nemo_python_object_get_name_and_desc;
}

#define METHOD_NAME "get_property_pages"
static GList *
nemo_python_object_get_property_pages (NemoPropertyPageProvider *provider,
										   GList 						*files)
{
	NemoPythonObject *object = (NemoPythonObject*)provider;
    PyObject *py_files, *py_ret = NULL;
    GList *ret = NULL;
	PyGILState_STATE state = pyg_gil_state_ensure();
	
  	debug_enter();

	CHECK_OBJECT(object);
	CHECK_METHOD_NAME(object->instance);

	CONVERT_LIST(py_files, files);
	
    py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX METHOD_NAME,
								 "(N)", py_files);
	HANDLE_RETVAL(py_ret);

	HANDLE_LIST(py_ret, NemoPropertyPage, "Nemo.PropertyPage");
	
 beach:
    free_pygobject_data_list (files);
	Py_XDECREF(py_ret);
	pyg_gil_state_release(state);
    return ret;
}
#undef METHOD_NAME


static void
nemo_python_object_property_page_provider_iface_init (NemoPropertyPageProviderIface *iface)
{
	iface->get_pages = nemo_python_object_get_property_pages;
}

#define METHOD_NAME "get_widget"
static GtkWidget *
nemo_python_object_get_widget (NemoLocationWidgetProvider *provider,
								   const char 				 	  *uri,
								   GtkWidget 					  *window)
{
	NemoPythonObject *object = (NemoPythonObject*)provider;
	GtkWidget *ret = NULL;
	PyObject *py_ret = NULL;
	PyGObject *py_ret_gobj;
	PyObject *py_uri = NULL;
	PyGILState_STATE state = pyg_gil_state_ensure();

	debug_enter();

	CHECK_OBJECT(object);
	CHECK_METHOD_NAME(object->instance);

	py_uri = PyUnicode_FromString(uri);

	py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX METHOD_NAME,
								 "(NN)", py_uri,
								 pygobject_new((GObject *)window));
	HANDLE_RETVAL(py_ret);

	py_ret_gobj = (PyGObject *)py_ret;
	if (!pygobject_check(py_ret_gobj, &PyGtkWidget_Type))
	{
		PyErr_SetString(PyExc_TypeError,
					    METHOD_NAME "should return a gtk.Widget");
		goto beach;
	}
	ret = (GtkWidget *)g_object_ref(py_ret_gobj->obj);

 beach:
	Py_XDECREF(py_ret);
	pyg_gil_state_release(state);
	return ret;
}
#undef METHOD_NAME

static void
nemo_python_object_location_widget_provider_iface_init (NemoLocationWidgetProviderIface *iface)
{
	iface->get_widget = nemo_python_object_get_widget;
}

#define METHOD_NAME "get_file_items"
static GList *
nemo_python_object_get_file_items (NemoMenuProvider *provider,
									   GtkWidget 			*window,
									   GList 				*files)
{
	NemoPythonObject *object = (NemoPythonObject*)provider;
    GList *ret = NULL;
    PyObject *py_ret = NULL, *py_files;
	PyGILState_STATE state = pyg_gil_state_ensure();
	
  	debug_enter();

	CHECK_OBJECT(object);	

	if (PyObject_HasAttrString(object->instance, "get_file_items_full"))
	{
		CONVERT_LIST(py_files, files);
		py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX "get_file_items_full",
									 "(NNN)",
									 pygobject_new((GObject *)provider), 
									 pygobject_new((GObject *)window), 
									 py_files);
	}
	else if (PyObject_HasAttrString(object->instance, "get_file_items"))
	{
		CONVERT_LIST(py_files, files);
		py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX METHOD_NAME,
									 "(NN)", 
									 pygobject_new((GObject *)window), 
									 py_files);
	}
	else
	{
		goto beach;
	}

	HANDLE_RETVAL(py_ret);

	HANDLE_LIST(py_ret, NemoMenuItem, "Nemo.MenuItem");

 beach:
 	free_pygobject_data_list(files);
	Py_XDECREF(py_ret);
	pyg_gil_state_release(state);
    return ret;
}
#undef METHOD_NAME

#define METHOD_NAME "get_background_items"
static GList *
nemo_python_object_get_background_items (NemoMenuProvider *provider,
											 GtkWidget 			  *window,
											 NemoFileInfo 	  *file)
{
	NemoPythonObject *object = (NemoPythonObject*)provider;
    GList *ret = NULL;
    PyObject *py_ret = NULL;
	PyGILState_STATE state = pyg_gil_state_ensure();
	
  	debug_enter();

	CHECK_OBJECT(object);

	if (PyObject_HasAttrString(object->instance, "get_background_items_full"))
	{
		py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX "get_background_items_full",
									 "(NNN)",
									 pygobject_new((GObject *)provider),
									 pygobject_new((GObject *)window),
									 pygobject_new((GObject *)file));
	}
	else if (PyObject_HasAttrString(object->instance, "get_background_items"))
	{
		py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX METHOD_NAME,
									 "(NN)",
									 pygobject_new((GObject *)window),
									 pygobject_new((GObject *)file));
	}
	else
	{
		goto beach;
	}

	HANDLE_RETVAL(py_ret);

	HANDLE_LIST(py_ret, NemoMenuItem, "Nemo.MenuItem");
	
 beach:
	free_pygobject_data(file, NULL);
	Py_XDECREF(py_ret);
	pyg_gil_state_release(state);
    return ret;
}
#undef METHOD_NAME

static void
nemo_python_object_menu_provider_iface_init (NemoMenuProviderIface *iface)
{
	iface->get_background_items = nemo_python_object_get_background_items;
	iface->get_file_items = nemo_python_object_get_file_items;
}

#define METHOD_NAME "get_columns"
static GList *
nemo_python_object_get_columns (NemoColumnProvider *provider)
{
	NemoPythonObject *object = (NemoPythonObject*)provider;
    GList *ret = NULL;
    PyObject *py_ret = NULL;
	PyGILState_STATE state = pyg_gil_state_ensure();                                    \

	debug_enter();
		
	CHECK_OBJECT(object);
	CHECK_METHOD_NAME(object->instance);

    py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX METHOD_NAME,
								 NULL);

	HANDLE_RETVAL(py_ret);

	HANDLE_LIST(py_ret, NemoColumn, "Nemo.Column");
	
 beach:
	if (py_ret != NULL)
		Py_XDECREF(py_ret);
	pyg_gil_state_release(state);
    return ret;
}
#undef METHOD_NAME

static void
nemo_python_object_column_provider_iface_init (NemoColumnProviderIface *iface)
{
	iface->get_columns = nemo_python_object_get_columns;
}


#define METHOD_NAME "cancel_update"
static void
nemo_python_object_cancel_update (NemoInfoProvider 		*provider,
									  NemoOperationHandle 	*handle)
{
	NemoPythonObject *object = (NemoPythonObject*)provider;
    PyObject *py_ret = NULL;

	PyGILState_STATE state = pyg_gil_state_ensure();
	PyObject *py_handle = nemo_python_boxed_new (_PyNemoOperationHandle_Type, handle, FALSE);

  	debug_enter();

	CHECK_OBJECT(object);
	CHECK_METHOD_NAME(object->instance);

    py_ret = PyObject_CallMethod(object->instance,
								 METHOD_PREFIX METHOD_NAME, "(NN)",
								 pygobject_new((GObject*)provider),
								 py_handle);

    HANDLE_RETVAL(py_ret);

 beach:
    Py_XDECREF(py_ret);
	pyg_gil_state_release(state);
}
#undef METHOD_NAME

typedef struct {
  gpointer ptr;
} DummyStruct;

#define METHOD_NAME "update_file_info"
static NemoOperationResult
nemo_python_object_update_file_info (NemoInfoProvider 		*provider,
										 NemoFile 				*file,
										 GClosure 					*update_complete,
										 NemoOperationHandle   **handle)
{
	NemoPythonObject *object = (NemoPythonObject*)provider;
    NemoOperationResult ret = NEMO_OPERATION_COMPLETE;
    PyObject *py_ret = NULL;
	PyGILState_STATE state = pyg_gil_state_ensure();

    /* For python extensions, we can't do assignment on the handle within python itself,
     * so we make a dummy struct to fill it.  Nemo relies on the handle pointer for
     * async flow control on info provider extensions, so it's cricial this does not remain
     * NULL.  This also allows a nice hash table key for python extensions to track idle
     * ids, and handle cancel_upate calls correctly.
     */

    *handle = (NemoOperationHandle *) g_new0(DummyStruct, 1);

    PyObject *py_handle = nemo_python_boxed_new (_PyNemoOperationHandle_Type, *handle, TRUE);

    debug_enter();

	CHECK_OBJECT(object);

	if (PyObject_HasAttrString(object->instance, "update_file_info_full"))
	{
		py_ret = PyObject_CallMethod(object->instance,
									 METHOD_PREFIX "update_file_info_full", "(NNNN)",
									 pygobject_new((GObject*)provider),
									 py_handle,
									 pyg_boxed_new(G_TYPE_CLOSURE, update_complete, TRUE, TRUE),
									 pygobject_new((GObject*)file));
	}
	else if (PyObject_HasAttrString(object->instance, "update_file_info"))
	{
		py_ret = PyObject_CallMethod(object->instance,
									 METHOD_PREFIX METHOD_NAME, "(N)",
									 pygobject_new((GObject*)file));
	}
	else
	{
		goto beach;
	}
	
	HANDLE_RETVAL(py_ret);

	if (!PyLong_Check(py_ret))
	{
		PyErr_SetString(PyExc_TypeError,
						METHOD_NAME " must return None or a int");
		goto beach;
	}

	ret = PyLong_AsLong(py_ret);
	
 beach:
 	free_pygobject_data(file, NULL);
	Py_XDECREF(py_ret);
	pyg_gil_state_release(state);
    return ret;
}
#undef METHOD_NAME

static void
nemo_python_object_info_provider_iface_init (NemoInfoProviderIface *iface)
{
	iface->cancel_update = nemo_python_object_cancel_update;
	iface->update_file_info = nemo_python_object_update_file_info;
}

static void 
nemo_python_object_instance_init (NemoPythonObject *object)
{
	NemoPythonObjectClass *class;
  	debug_enter();

	class = (NemoPythonObjectClass*)(((GTypeInstance*)object)->g_class);

	object->instance = PyObject_CallObject(class->type, NULL);
	if (object->instance == NULL)
		PyErr_Print();
}

static void
nemo_python_object_finalize (GObject *object)
{
  	debug_enter();

	if (((NemoPythonObject *)object)->instance != NULL)
		Py_DECREF(((NemoPythonObject *)object)->instance);
}

static void
nemo_python_object_class_init (NemoPythonObjectClass *class,
								   gpointer 				  class_data)
{
	debug_enter();

	parent_class = g_type_class_peek_parent (class);
	
	class->type = (PyObject*)class_data;
	
	G_OBJECT_CLASS (class)->finalize = nemo_python_object_finalize;
}

GType 
nemo_python_object_get_type (GTypeModule *module, 
								 PyObject 	*type)
{
    PyObject *name_str = NULL;
	GTypeInfo *info;
	gchar *type_name;
	GType gtype;
	  
	static const GInterfaceInfo property_page_provider_iface_info = {
		(GInterfaceInitFunc) nemo_python_object_property_page_provider_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo location_widget_provider_iface_info = {
		(GInterfaceInitFunc) nemo_python_object_location_widget_provider_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo menu_provider_iface_info = {
		(GInterfaceInitFunc) nemo_python_object_menu_provider_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo column_provider_iface_info = {
		(GInterfaceInitFunc) nemo_python_object_column_provider_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo info_provider_iface_info = {
		(GInterfaceInitFunc) nemo_python_object_info_provider_iface_init,
		NULL,
		NULL
	};

    static const GInterfaceInfo nd_provider_iface_info = {
        (GInterfaceInitFunc) nemo_python_object_name_and_desc_provider_iface_init,
        NULL,
        NULL
    };

    name_str = PyObject_GetAttrString(type, "__name__");

	debug_enter_args("type=%s", PyUnicode_AsUTF8(name_str));
	info = g_new0 (GTypeInfo, 1);
	
	info->class_size = sizeof (NemoPythonObjectClass);
	info->class_init = (GClassInitFunc)nemo_python_object_class_init;
	info->instance_size = sizeof (NemoPythonObject);
	info->instance_init = (GInstanceInitFunc)nemo_python_object_instance_init;

	info->class_data = type;
	Py_INCREF(type);

	type_name = g_strdup_printf("%s+NemoPython",
								PyUnicode_AsUTF8(name_str));

    Py_XDECREF(name_str);
		
	gtype = g_type_module_register_type (module, 
										 G_TYPE_OBJECT,
										 type_name,
										 info, 0);

    g_free (info);
    g_free (type_name);

	if (PyObject_IsSubclass(type, (PyObject*)&PyNemoPropertyPageProvider_Type))
	{
		g_type_module_add_interface (module, gtype, 
									 NEMO_TYPE_PROPERTY_PAGE_PROVIDER,
									 &property_page_provider_iface_info);
	}

	if (PyObject_IsSubclass(type, (PyObject*)&PyNemoLocationWidgetProvider_Type))
	{
		g_type_module_add_interface (module, gtype,
									 NEMO_TYPE_LOCATION_WIDGET_PROVIDER,
									 &location_widget_provider_iface_info);
	}
	
	if (PyObject_IsSubclass(type, (PyObject*)&PyNemoMenuProvider_Type))
	{
		g_type_module_add_interface (module, gtype, 
									 NEMO_TYPE_MENU_PROVIDER,
									 &menu_provider_iface_info);
	}

	if (PyObject_IsSubclass(type, (PyObject*)&PyNemoColumnProvider_Type))
	{
		g_type_module_add_interface (module, gtype, 
									 NEMO_TYPE_COLUMN_PROVIDER,
									 &column_provider_iface_info);
	}

	if (PyObject_IsSubclass(type, (PyObject*)&PyNemoInfoProvider_Type))
	{
		g_type_module_add_interface (module, gtype, 
									 NEMO_TYPE_INFO_PROVIDER,
									 &info_provider_iface_info);
	}

    if (PyObject_IsSubclass(type, (PyObject*)&PyNemoNameAndDescProvider_Type))
    {
        g_type_module_add_interface (module, gtype, 
                                     NEMO_TYPE_NAME_AND_DESC_PROVIDER,
                                     &nd_provider_iface_info);
    }

    return gtype;
}
