/*
 *  nemo-wipe - a nemo extension to wipe file(s)
 * 
 *  Copyright (C) 2012 Colomban Wendling <ban@herbesfolles.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
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
 */

#ifndef NW_TYPE_UTILS_H
#define NW_TYPE_UTILS_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS


#define _NW_DEFINE_TYPE_MODULE_EXTENDED_BEGIN(TN, t_n, T_P, _f_)               \
                                                                               \
static void     t_n##_init              (TN        *self);                     \
static void     t_n##_class_init        (TN##Class *klass);                    \
                                                                               \
static gpointer t_n##_parent_class = NULL;                                     \
static GType    t_n##__GType = 0;                                              \
                                                                               \
static void                                                                    \
t_n##_class_intern_init (gpointer klass)                                       \
{                                                                              \
  t_n##_parent_class = g_type_class_peek_parent (klass);                       \
  t_n##_class_init ((TN##Class*) klass);                                       \
}                                                                              \
                                                                               \
GType                                                                          \
t_n##_get_type (void)                                                          \
{                                                                              \
  g_assert (t_n##__GType != 0);                                                \
                                                                               \
  return t_n##__GType;                                                         \
}                                                                              \
                                                                               \
GType                                                                          \
t_n##_register_type (GTypeModule *nw_define_type_module_module)                \
{                                                                              \
  static const GTypeInfo type_info = {                                         \
    sizeof (TN##Class),                       /* class size */                 \
    NULL,                                     /* base init */                  \
    NULL,                                     /* base finalize */              \
    (GClassInitFunc) t_n##_class_intern_init, /* class init */                 \
    NULL,                                     /* class finalize */             \
    NULL,                                     /* class data */                 \
    sizeof (TN),                              /* instance size */              \
    0,                                        /* n perallocs */                \
    (GInstanceInitFunc) t_n##_init,           /* instance init */              \
    NULL                                      /* value table */                \
  };                                                                           \
  GType nw_define_type_module_id = 0;                                          \
                                                                               \
  nw_define_type_module_id = g_type_module_register_type (nw_define_type_module_module, \
                                                          T_P, #TN,            \
                                                          &type_info, _f_);    \
  t_n##__GType = nw_define_type_module_id;                                     \
  { /* custom code follows */
#define _NW_DEFINE_TYPE_MODULE_EXTENDED_END()                                  \
    /* following custom code */                                                \
  }                                                                            \
  return nw_define_type_module_id;                                             \
}

#define NW_DEFINE_TYPE_MODULE_EXTENDED(TN, t_n, T_P, _f_, _C_)                 \
  _NW_DEFINE_TYPE_MODULE_EXTENDED_BEGIN (TN, t_n, T_P, _f_)                    \
  { _C_; }                                                                     \
  _NW_DEFINE_TYPE_MODULE_EXTENDED_END ()

#define NW_DEFINE_TYPE_MODULE_WITH_CODE(TN, t_n, T_P, _C_)                     \
  _NW_DEFINE_TYPE_MODULE_EXTENDED_BEGIN (TN, t_n, T_P, 0)                      \
  { _C_; }                                                                     \
  _NW_DEFINE_TYPE_MODULE_EXTENDED_END ()

#define NW_DEFINE_TYPE_MODULE(TN, t_n, T_P)                                    \
  NW_DEFINE_TYPE_MODULE_EXTENDED (TN, t_n, T_P, 0, {})

#define NW_TYPE_MODULE_IMPLEMENT_INTERFACE(TYPE_IFACE, iface_init)             \
{                                                                              \
  static const GInterfaceInfo nw_type_module_implement_interface_info = {      \
    (GInterfaceInitFunc) iface_init, NULL, NULL                                \
  };                                                                           \
                                                                               \
  g_type_module_add_interface (nw_define_type_module_module,                   \
                               nw_define_type_module_id,                       \
                               TYPE_IFACE,                                     \
                               &nw_type_module_implement_interface_info);      \
}


G_END_DECLS

#endif /* guard */
