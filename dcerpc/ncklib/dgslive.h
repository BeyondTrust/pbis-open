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
/*
**
**  NAME:
**
**      dgslive.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  DG maintain/monitor liveness service routines.
**
**
*/

#ifndef _DGSLIVE_H
#define _DGSLIVE_H


/*
 * R P C _ D G _ C L I E N T _ R E L E A S E
 *
 * Release a reference to a client handle.  This macro is called by the SCT
 * monitor when it is time to free an SCT entry.  It the entry has a reference
 * to a client handle, that reference is decremented.  If the count falls to 1,
 * meaning the only other reference is the client_rep table, the client_free
 * routine is called to free the handle.
 */

#define RPC_DG_CLIENT_RELEASE(scte) { \
    if ((scte)->client != NULL) \
    { \
        rpc_dg_client_rep_p_t client = (scte)->client; \
        assert(client->refcnt > 1); \
        if (--client->refcnt == 1) \
            rpc__dg_client_free((rpc_client_handle_t) (scte)->client);\
        (scte)->client = NULL; \
    } \
}

PRIVATE void rpc__dg_binding_inq_client (   
        rpc_binding_rep_p_t  /*binding_r*/,
        rpc_client_handle_t * /*client_h*/,
        unsigned32 * /*st*/
    );

PRIVATE void rpc__dg_client_free (   
        rpc_client_handle_t  /*client_h*/
    );

#endif /* _DGSLIVE_H */
