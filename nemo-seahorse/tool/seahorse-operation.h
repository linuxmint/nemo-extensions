/*
 * Seahorse
 *
 * Copyright (C) 2004 Stefan Walter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SeahorseOperation: An operation taking place over time.
 *
 * - Generally this class is derived and a base class actually hooks in and
 *   performs the operation, keeps the properties updated etc...
 * - Used all over to represent things like key loading operations, search
 * - SeahorseMultiOperation allows you to combine multiple operations into
 *   a single one. Used when searching multiple key servers for example.
 * - Can be tied to a progress bar (see seahorse-progress.h)
 * - Holds a reference to itself while the operation is in progress.
 * - The seahorse_operation_mark_* are used by derived classes to update
 *   properties of the operation as things progress.
 *
 * Signals:
 *   done: The operation is complete.
 *   progress: The operation has progressed, or changed state somehow.
 *
 * Properties:
 *   result: The 'result' of the operation (if applicable).
 *           This depends on the derived operation class.
 *   progress: A fraction between 0.0 and 1.0 inclusive representing how far
 *           along this operation is. 0.0 = indeterminate, and 1.0 is done.
 *   message: A progress message to display to the user.
 */

#ifndef __SEAHORSE_OPERATION_H__
#define __SEAHORSE_OPERATION_H__

#include <gtk/gtk.h>

#define SEAHORSE_TYPE_OPERATION            (seahorse_operation_get_type ())
#define SEAHORSE_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SEAHORSE_TYPE_OPERATION, SeahorseOperation))
#define SEAHORSE_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SEAHORSE_TYPE_OPERATION, SeahorseOperationClass))
#define SEAHORSE_IS_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SEAHORSE_TYPE_OPERATION))
#define SEAHORSE_IS_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SEAHORSE_TYPE_OPERATION))
#define SEAHORSE_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SEAHORSE_TYPE_OPERATION, SeahorseOperationClass))

struct _SeahorseOperation;

typedef struct _SeahorseOperation {
    GObject parent;

    /*< public >*/
    gchar *message;                /* Progress status details ie: "foobar.jpg" */
    gdouble progress;              /* The current progress position, -1 for indeterminate */

    guint is_running : 1;          /* If the operation is running or not */
    guint is_done : 1;             /* Operation is done or not */
    guint is_cancelled : 1;        /* Operation is cancelled or not */;

    GError *error;

    /*< private> */
    gpointer result;
    GDestroyNotify result_destroy;

} SeahorseOperation;

typedef struct _SeahorseOperationClass {
    GObjectClass parent_class;

    /* signals --------------------------------------------------------- */

    /* Signal that occurs when the operation is complete */
    void (*done) (SeahorseOperation *operation);

    /* Signal that occurs progress or status changes */
    void (*progress) (SeahorseOperation *operation, const gchar *status, gdouble progress);

    /* virtual methods ------------------------------------------------- */

    /* Cancels a pending operation */
    void (*cancel) (SeahorseOperation *operation);

} SeahorseOperationClass;

GType       seahorse_operation_get_type      (void);

/* Methods ------------------------------------------------------------ */

void                seahorse_operation_cancel      (SeahorseOperation *operation);

#define             seahorse_operation_is_running(operation) \
                                                   ((operation)->is_running)

#define             seahorse_operation_is_successful(operation) \
                                                   ((operation)->error == NULL)

void                seahorse_operation_copy_error  (SeahorseOperation *operation,
                                                    GError **err);

const GError*       seahorse_operation_get_error   (SeahorseOperation *operation);

#define             seahorse_operation_get_progress(op) \
                                                   ((op)->progress)

#define             seahorse_operation_get_message(operation) \
                                                   ((const gchar*)((operation)->message))

/* ----------------------------------------------------------------------------
 *  SUBCLASS DECLARATION MACROS
 */

/*
 * To declare a subclass you need to put in code like this:
 *

#define SEAHORSE_TYPE_XX_OPERATION            (seahorse_xx_operation_get_type ())
#define SEAHORSE_XX_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SEAHORSE_TYPE_XX_OPERATION, SeahorseXxOperation))
#define SEAHORSE_XX_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SEAHORSE_TYPE_XX_OPERATION, SeahorseXxOperationClass))
#define SEAHORSE_IS_XX_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SEAHORSE_TYPE_XX_OPERATION))
#define SEAHORSE_IS_XX_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SEAHORSE_TYPE_XX_OPERATION))
#define SEAHORSE_XX_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SEAHORSE_TYPE_XX_OPERATION, SeahorseXxOperationClass))

DECLARE_OPERATION (Xx, xx)
   ... member vars here ...
END_DECLARE_OPERATION

 *
 * And then in your implementation something like this
 *

IMPLEMENT_OPERATION (Xx, xx)

static void
seahorse_xx_operation_init (SeahorseXxOperation *lop)
{

}

static void
seahorse_xx_operation_dispose (GObject *gobject)
{
    G_OBJECT_CLASS (operation_parent_class)->dispose (gobject);
}

static void
seahorse_xx_operation_finalize (GObject *gobject)
{
    G_OBJECT_CLASS (operation_parent_class)->finalize (gobject);
}

static void
seahorse_xx_operation_cancel (SeahorseOperation *operation)
{
    seahorse_operation_mark_done (operation, TRUE, NULL);
}

 *
 *
 */

#define DECLARE_OPERATION(Opx, opx)                                            \
    typedef struct _Seahorse##Opx##Operation Seahorse##Opx##Operation;              \
    typedef struct _Seahorse##Opx##OperationClass Seahorse##Opx##OperationClass;    \
    GType          seahorse_##opx##_operation_get_type      (void);                 \
    struct _Seahorse##Opx##OperationClass {                                         \
        SeahorseOperationClass parent_class;                                        \
    };                                                                              \
    struct _Seahorse##Opx##Operation {                                              \
        SeahorseOperation parent;                                                   \

#define END_DECLARE_OPERATION                                                       \
    };                                                                              \

#define IMPLEMENT_OPERATION_EX(Opx, opx)                                                        \
    static void seahorse_##opx##_operation_class_init (Seahorse##Opx##OperationClass *klass);   \
    static void seahorse_##opx##_operation_init       (Seahorse##Opx##Operation *lop);          \
    static void seahorse_##opx##_operation_dispose    (GObject *gobject);                       \
    static void seahorse_##opx##_operation_finalize   (GObject *gobject);                       \
    static void seahorse_##opx##_operation_cancel     (SeahorseOperation *operation);           \
    GType seahorse_##opx##_operation_get_type (void) {                                          \
        static GType operation_type = 0;                                                        \
        if (!operation_type) {                                                                  \
            static const GTypeInfo operation_info = {                                           \
                sizeof (Seahorse##Opx##OperationClass), NULL, NULL,                             \
                (GClassInitFunc) seahorse_##opx##_operation_class_init, NULL, NULL,             \
                sizeof (Seahorse##Opx##Operation), 0,                                           \
                (GInstanceInitFunc)seahorse_##opx##_operation_init                              \
            };                                                                                  \
            operation_type = g_type_register_static (SEAHORSE_TYPE_OPERATION,                   \
                                "Seahorse" #Opx "Operation", &operation_info, 0);               \
        }                                                                                       \
        return operation_type;                                                                  \
    }                                                                                           \
    static GObjectClass *operation_parent_class = NULL;                                         \
    static void seahorse_##opx##_operation_class_init (Seahorse##Opx##OperationClass *klass) {  \
        GObjectClass *gobject_class  = G_OBJECT_CLASS (klass);                                  \
        SeahorseOperationClass *op_class = SEAHORSE_OPERATION_CLASS (klass);                    \
        operation_parent_class = g_type_class_peek_parent (klass);                              \
        op_class->cancel = seahorse_##opx##_operation_cancel;                                   \
        gobject_class->dispose = seahorse_##opx##_operation_dispose;                            \
        gobject_class->finalize = seahorse_##opx##_operation_finalize;                          \

#define END_IMPLEMENT_OPERATION_EX                                                              \
    }                                                                                           \

#define IMPLEMENT_OPERATION_PROPS(Opx, opx)                                                     \
    static void seahorse_##opx##_operation_set_property (GObject *gobject, guint prop_id,       \
                        const GValue *value, GParamSpec *pspec);                                \
    static void seahorse_##opx##_operation_get_property (GObject *gobject, guint prop_id,       \
                        GValue *value, GParamSpec *pspec);                                      \
    IMPLEMENT_OPERATION_EX(Opx, opx)                                                            \
        gobject_class->set_property = seahorse_##opx##_operation_set_property;                  \
        gobject_class->get_property = seahorse_##opx##_operation_get_property;                  \

#define END_IMPLEMENT_OPERATION_PROPS                                                           \
    END_IMPLEMENT_OPERATION_EX                                                                  \

#define IMPLEMENT_OPERATION(Opx, opx)                                                           \
    IMPLEMENT_OPERATION_EX(Opx, opx)                                                            \
    END_IMPLEMENT_OPERATION_EX                                                                  \


/* Helpers for Derived ----------------------------------------------- */

#define SEAHORSE_CALC_PROGRESS(cur, tot)

void              seahorse_operation_mark_start         (SeahorseOperation *operation);

/* Takes ownership of |error| */
void              seahorse_operation_mark_done          (SeahorseOperation *operation,
                                                         gboolean cancelled, GError *error);

void              seahorse_operation_mark_progress      (SeahorseOperation *operation,
                                                         const gchar *message,
                                                         gdouble progress);

void              seahorse_operation_mark_progress_full (SeahorseOperation *operation,
                                                         const gchar *message,
                                                         gdouble current, gdouble total);

void              seahorse_operation_mark_result        (SeahorseOperation *operation,
                                                         gpointer result, GDestroyNotify notify_func);

#endif /* __SEAHORSE_OPERATION_H__ */
