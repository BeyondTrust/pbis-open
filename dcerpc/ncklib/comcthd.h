/*
 * 
 * (c) Copyright 1989 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1989 DIGITAL EQUIPMENT CORPORATION
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 *                 permission to use, copy, modify, and distribute this
 * file for any purpose is hereby granted without fee, provided that
 * the above copyright notices and this notice appears in all source
 * code copies, and that none of the names of Open Software
 * Foundation, Inc., Hewlett-Packard Company, or Digital Equipment
 * Corporation be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Neither Open Software Foundation, Inc., Hewlett-
 * Packard Company, nor Digital Equipment Corporation makes any
 * representations about the suitability of this software for any
 * purpose.
 * 
 */
/*
 */
#ifndef _COMCTHD_H
#define _COMCTHD_H	1
/*
**
**  NAME
**
**      comcthd.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definitions of types/constants for the Call Thread Services
**  of the Common Communications Service component of the RPC runtime.
**
**
*/

#include <dce/dce.h>


#ifdef _cplusplus
extern "C" {
#endif

/***********************************************************************/
/*
 * R P C _ _ C T H R E A D _ I N I T
 *
 */

PRIVATE void rpc__cthread_init (
        unsigned32                  * /*status*/
    );


/***********************************************************************/
/*
 * R P C _ _ C T H R E A D _ S T A R T _ A L L
 *
 */

PRIVATE void rpc__cthread_start_all (
        unsigned32              /*default_pool_cthreads*/,
        unsigned32              * /*status*/
    );


/***********************************************************************/
/*
 * R P C _ _ C T H R E A D _ S T O P _ A L L
 *
 */

PRIVATE void rpc__cthread_stop_all (
        unsigned32              * /*status*/
    );


/***********************************************************************/
/*
 * R P C _ _ C T H R E A D _ I N V O K E _ N U L L
 *
 */

PRIVATE void rpc__cthread_invoke_null (
        rpc_call_rep_p_t        /*call_rep*/,
        dce_uuid_p_t                /*object*/,
        dce_uuid_p_t                /*if_uuid*/,
        unsigned32              /*if_ver*/,
        unsigned32              /*if_opnum*/,
        rpc_prot_cthread_executor_fn_t /*cthread_executor*/,
        pointer_t               /*call_args*/,
        unsigned32              * /*status*/
    );


/***********************************************************************/
/*
 * R P C _ _ C T H R E A D _ D E Q U E U E
 *
 */

PRIVATE boolean32 rpc__cthread_dequeue (
        rpc_call_rep_p_t        /*call*/
    );


/***********************************************************************/
/*
 * R P C _ _ C T H R E A D _ C A N C E L
 *
 */

PRIVATE void rpc__cthread_cancel (
        rpc_call_rep_p_t        /*call*/
    );

/***********************************************************************/
/*
 * R P C _ _ C T H R E A D _ C A N C E L _ C A F
 *
 */

PRIVATE boolean32 rpc__cthread_cancel_caf (
        rpc_call_rep_p_t        /*call*/
    );

/***********************************************************************/
/*
 * R P C _ _ C T H R E A D _ C A N C E L _ E N A B L E _ P O S T I N G
 *
 */
 PRIVATE void rpc__cthread_cancel_enable_post (
        rpc_call_rep_p_t        /*call*/
    );


#ifdef _cplusplus
}
#endif

#endif /* _COMCTHD_H */
