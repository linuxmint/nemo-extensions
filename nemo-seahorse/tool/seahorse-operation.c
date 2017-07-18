/*
 * Seahorse
 *
 * Copyright (C) 2004-2006 Stefan Walter
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "seahorse-util.h"
#include "seahorse-marshal.h"
#include "seahorse-operation.h"

/* Override the DEBUG_OPERATION_ENABLE switch here */
/* #define DEBUG_OPERATION_ENABLE 0 */

#ifndef DEBUG_OPERATION_ENABLE
#if _DEBUG
#define DEBUG_OPERATION_ENABLE 1
#else
#define DEBUG_OPERATION_ENABLE 0
#endif
#endif

#if DEBUG_OPERATION_ENABLE
#define DEBUG_OPERATION(x) g_printerr x
#else
#define DEBUG_OPERATION(x)
#endif

/* -----------------------------------------------------------------------------
 * SEAHORSE OPERATION
 */

enum {
    PROP_0,
    PROP_PROGRESS,
    PROP_MESSAGE,
    PROP_RESULT
};

enum {
    DONE,
    PROGRESS,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (SeahorseOperation, seahorse_operation, G_TYPE_OBJECT);

/* OBJECT ------------------------------------------------------------------- */

static void
seahorse_operation_init (SeahorseOperation *operation)
{
    /* This is the state of non-started operation. all other flags are zero */
    operation->progress = -1;
}

static void
seahorse_operation_set_property (GObject *object, guint prop_id,
                                 const GValue *value, GParamSpec *pspec)
{
    SeahorseOperation *op = SEAHORSE_OPERATION (object);

    switch (prop_id) {
    case PROP_MESSAGE:
        g_free (op->message);
        op->message = g_value_dup_string (value);
        break;
    }
}

static void
seahorse_operation_get_property (GObject *object, guint prop_id,
                                 GValue *value, GParamSpec *pspec)
{
    SeahorseOperation *op = SEAHORSE_OPERATION (object);

    switch (prop_id) {
    case PROP_PROGRESS:
        g_value_set_double (value, op->progress);
        break;
    case PROP_MESSAGE:
        g_value_set_string (value, op->message);
        break;
    case PROP_RESULT:
        g_value_set_pointer (value, op->result);
        break;
    }
}

/* dispose of all our internal references */
static void
seahorse_operation_dispose (GObject *gobject)
{
    SeahorseOperation *op = SEAHORSE_OPERATION (gobject);

    if (op->is_running)
        seahorse_operation_cancel (op);
    g_assert (!op->is_running);

    /* We do this in dispose in case it's a circular reference */
    if (op->result && op->result_destroy)
        (op->result_destroy) (op->result);
    op->result = NULL;
    op->result_destroy = NULL;

    G_OBJECT_CLASS (seahorse_operation_parent_class)->dispose (gobject);
}

/* free private vars */
static void
seahorse_operation_finalize (GObject *gobject)
{
    SeahorseOperation *op = SEAHORSE_OPERATION (gobject);
    g_assert (!op->is_running);

    if (op->error) {
        g_error_free (op->error);
        op->error = NULL;
    }

    g_free (op->message);
    op->message = NULL;

    G_OBJECT_CLASS (seahorse_operation_parent_class)->finalize (gobject);
}

static void
seahorse_operation_class_init (SeahorseOperationClass *klass)
{
    GObjectClass *gobject_class;

    seahorse_operation_parent_class = g_type_class_peek_parent (klass);
    gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = seahorse_operation_dispose;
    gobject_class->finalize = seahorse_operation_finalize;
    gobject_class->set_property = seahorse_operation_set_property;
    gobject_class->get_property = seahorse_operation_get_property;

    g_object_class_install_property (gobject_class, PROP_PROGRESS,
        g_param_spec_double ("progress", "Progress position", "Current progress position (fraction between 0 and 1)",
                             0.0, 1.0, 0.0, G_PARAM_READABLE));

    g_object_class_install_property (gobject_class, PROP_MESSAGE,
        g_param_spec_string ("message", "Status message", "Current progress message",
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_RESULT,
        g_param_spec_pointer ("result", "Operation Result", "Exact value depends on derived class.",
                              G_PARAM_READABLE));


    signals[DONE] = g_signal_new ("done", SEAHORSE_TYPE_OPERATION,
                G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (SeahorseOperationClass, done),
                NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    signals[PROGRESS] = g_signal_new ("progress", SEAHORSE_TYPE_OPERATION,
                G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (SeahorseOperationClass, progress),
                NULL, NULL, seahorse_marshal_VOID__STRING_DOUBLE, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_DOUBLE);
}

/* PUBLIC ------------------------------------------------------------------- */

void
seahorse_operation_cancel (SeahorseOperation *op)
{
    SeahorseOperationClass *klass;

    g_return_if_fail (SEAHORSE_IS_OPERATION (op));
    g_return_if_fail (op->is_running);

    g_object_ref (op);

    klass = SEAHORSE_OPERATION_GET_CLASS (op);

    /* A derived operation exists */
    if (klass->cancel)
        (*klass->cancel) (op);

    /* No derived operation exists */
    else
        seahorse_operation_mark_done (op, TRUE, NULL);

    g_object_unref (op);
}

void
seahorse_operation_copy_error  (SeahorseOperation *op, GError **err)
{
    g_return_if_fail (err == NULL || *err == NULL);
    if (err)
        *err = op->error ? g_error_copy (op->error) : NULL;
}

const GError*
seahorse_operation_get_error (SeahorseOperation *op)
{
    return op->error;
}

/* METHODS FOR DERIVED CLASSES ---------------------------------------------- */

void
seahorse_operation_mark_start (SeahorseOperation *op)
{
    g_return_if_fail (SEAHORSE_IS_OPERATION (op));

    /* A running operation always refs itself */
    g_object_ref (op);
    op->is_running = TRUE;
    op->is_done = FALSE;
    op->is_cancelled = FALSE;
    op->progress = -1;

    g_free (op->message);
    op->message = NULL;
}

void
seahorse_operation_mark_progress (SeahorseOperation *op, const gchar *message,
                                  gdouble progress)
{
    gboolean emit = FALSE;

    g_return_if_fail (SEAHORSE_IS_OPERATION (op));
    g_return_if_fail (op->is_running);

    if (progress != op->progress) {
        op->progress = progress;
        emit = TRUE;
    }

    if (!seahorse_util_string_equals (op->message, message)) {
        g_free (op->message);
        op->message = message ? g_strdup (message) : NULL;
        emit = TRUE;
    }

    if (emit)
        g_signal_emit (G_OBJECT (op), signals[PROGRESS], 0, op->message, progress);
}

void
seahorse_operation_mark_progress_full (SeahorseOperation *op, const gchar *message,
                                       gdouble current, gdouble total)
{
    if (current > total)
        current = total;
    seahorse_operation_mark_progress (op, message, total <= 0 ? -1 : current / total);
}


void
seahorse_operation_mark_done (SeahorseOperation *op, gboolean cancelled,
                              GError *error)
{
    g_return_if_fail (SEAHORSE_IS_OPERATION (op));
    g_return_if_fail (op->is_running);

    /* No message on completed operations */
    g_free (op->message);
    op->message = NULL;

    op->progress = cancelled ? -1 : 1.0;
    op->is_running = FALSE;
    op->is_done = TRUE;
    op->is_cancelled = cancelled;
    op->error = error;

    g_signal_emit (op, signals[PROGRESS], 0, NULL, op->progress);
    g_signal_emit (op, signals[DONE], 0);

    /* A running operation always refs itself */
    g_object_unref (op);
}

void
seahorse_operation_mark_result (SeahorseOperation *op, gpointer result,
                                GDestroyNotify destroy_func)
{
    if (op->result && op->result_destroy)
        (op->result_destroy) (op->result);
    op->result = result;
    op->result_destroy = destroy_func;
}
