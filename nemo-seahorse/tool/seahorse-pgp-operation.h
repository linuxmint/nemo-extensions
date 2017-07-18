/*
 * Seahorse
 *
 * Copyright (C) 2006 Stefan Walter
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

/**
 * SeahorsePGPOperation: A way to track operations done via GPGME.
 *
 * - Derived from SeahorseOperation
 * - Uses the arcane gpgme_io_cbs API.
 * - Creates a new context so this doesn't block other operations.
 *
 * Signals:
 *   results: The GPGME results are ready. This is called before
 *            the 'done' signal of SeahorseOperation.
 *
 * Properties:
 *  gctx:  (pointer) GPGME context.
 *  message: (string) message to display instead of from GPGME.
 *  default-total: (guint) When GPGME reports 0 as progress total, use this instead.
 *
 * HOW TO USE:
 * 1. Create a SeahorsePGPOperation with seahorse_pgp_operation_new.
 * 2. You'll be using the gpgme_ctx_t out of the gctx data member.
 * 3. Use one of the gpgme_op_*_start functions to start an operation with gctx.
 * 4. Hand off the operation to seahorse_progress_* functions (which claim
 *    ownership of the operation) to see a progress dialog or status bar.
 *
 * NOTES:
 * - Never use with gpgme_op_keylist_start and gpgme_op_trustlist_start.
 */

#ifndef __SEAHORSE_PGP_OPERATION_H__
#define __SEAHORSE_PGP_OPERATION_H__

/*
 * TODO: Eventually most of the stuff from seahorse-pgp-key-op and
 * seahorse-op should get merged into here.
 */

#include "seahorse-operation.h"
#include <gpgme.h>

#define SEAHORSE_TYPE_PGP_OPERATION            (seahorse_pgp_operation_get_type ())
#define SEAHORSE_PGP_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SEAHORSE_TYPE_PGP_OPERATION, SeahorsePGPOperation))
#define SEAHORSE_PGP_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SEAHORSE_TYPE_PGP_OPERATION, SeahorsePGPOperationClass))
#define SEAHORSE_IS_PGP_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SEAHORSE_TYPE_PGP_OPERATION))
#define SEAHORSE_IS_PGP_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SEAHORSE_TYPE_PGP_OPERATION))
#define SEAHORSE_PGP_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SEAHORSE_TYPE_PGP_OPERATION, SeahorsePGPOperationClass))

typedef struct _SeahorsePGPOperation SeahorsePGPOperation;
typedef struct _SeahorsePGPOperationClass SeahorsePGPOperationClass;

struct _SeahorsePGPOperation {
    SeahorseOperation parent;

    /*< public >*/
    gpgme_ctx_t gctx;
};

struct _SeahorsePGPOperationClass {
    SeahorseOperationClass parent_class;

    /* Signal that occurs when the results of the GPGME operation are ready */
    void (*results) (SeahorsePGPOperation *pop);
};


GType                   seahorse_pgp_operation_get_type     (void);

SeahorsePGPOperation*   seahorse_pgp_operation_new          (const gchar *message);

/* Call when calling of gpgme_op_*_start failed */
void                    seahorse_pgp_operation_mark_failed  (SeahorsePGPOperation *pop,
                                                             gpgme_error_t gerr);

#endif /* __SEAHORSE_PGP_OPERATION_H__ */
