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
**      conv.c
**
**  FACILITY:
**
**      Conversation Manager (CONV)
**
**  ABSTRACT:
**
**  Implement conversation manager callback procedures for NCA RPC
**  datagram protocol.
**
**
*/

#include <dg.h>
#include <dgccall.h>
#include <dgccallt.h>

#include <dce/conv.h>
#include <dgglob.h>



/* ========================================================================= */

INTERNAL boolean conv_common  (
        dce_uuid_t * /*actuid*/,
        unsigned32 /*boot_time*/,
        rpc_dg_ccall_p_t * /*ccall*/,
        unsigned32 * /*st*/
    );

/* ========================================================================= */

/*
 * C O N V _ C O M M O N
 *
 * Common routine used by conv_who_are_you() and conv_are_you_there()
 * to find the client call handle associated with the incoming callback
 * and verify that we are communicating with the expected instance of
 * the server.
 */
   
INTERNAL boolean conv_common
(
    dce_uuid_t *actuid,
    unsigned32 boot_time,
    rpc_dg_ccall_p_t *ccall,
    unsigned32 *st
)
{
    /*
     * Find the call he's asking about.
     */

    *ccall = rpc__dg_ccallt_lookup(actuid, RPC_C_DG_NO_HINT);

    /*
     * No ccall entry will exist if an old duplicate WAY callback request
     * is received.  If there is no ccall entry corresponding to the
     * incoming activity id then we return an error status code.  The
     * server performing the WAY callback will detect the error and send
     * a reject packet that will be dropped by the client.
     */

    if (*ccall == NULL) 
    {
        *st = nca_s_bad_actid;
        return (false);
    }

    /*
     * If we don't know the boot time yet for this server, stash it away
     * now.
     */

    if ((*ccall)->c.call_server_boot == 0)
    {
        (*ccall)->c.call_server_boot = boot_time;
    }
    else 
    {
        /*
         * We DO know the boot time.  If what the server's supplied boot
         * time isn't what we think it should be, then we must be getting
         * called back by a new instance of the server (i.e., which
         * received a duplicate of an outstanding request of ours).  Since
         * we can't know whether the original instance of the server
         * executed our call or not, we return failure to the server
         * to prevent IT from executing call (i.e., for a possible second
         * time, violating the "at most once" rule).
         */
        if ((*ccall)->c.call_server_boot != boot_time)
        {
            *st = nca_s_you_crashed;
            RPC_DG_CCALL_RELEASE(ccall);
            return (false);
        }
    }

    *st = rpc_s_ok;
    return (true);
}


/*
 * C O N V _ W H O _ A R E _ Y O U
 *
 * This procedure is called remotely by a server that has no "connection"
 * information about a client from which it has just received a call.  This
 * call executes in the context of that client (i.e. it is a "call back").
 * We return the current sequence number of the client and his "identity".
 * We accept the boot time from the server for later use in calls to the
 * same server.
 */

PRIVATE void conv_who_are_you
(
    handle_t h ATTRIBUTE_UNUSED,       /* Not really */
    dce_uuid_t *actuid,
    unsigned32 boot_time,
    unsigned32 *seq,
    unsigned32 *st
)
{
    rpc_dg_ccall_p_t ccall;

    RPC_LOCK_ASSERT(0);

    if (! conv_common(actuid, boot_time, &ccall, st))
    {
        return;
    }

    *seq = ccall->c.call_seq;
    RPC_DG_CCALL_RELEASE(&ccall);
} 


/*
 * C O N V _ W H O _ A R E _ Y O U 2
 *
 * This procedure is called remotely by a server that needs to monitor the
 * the liveness of this client.  We return to it a UUID unique to this 
 * particular instance of this particular application.  We also return all
 * the information from a normal WAY callback, so that in the future servers
 * will only need to make this call to get all client information.
 */

PRIVATE void conv_who_are_you2
(
    handle_t h,       /* Not really */
    dce_uuid_t *actuid,
    unsigned32 boot_time,
    unsigned32 *seq,
    dce_uuid_t *cas_uuid,
    unsigned32 *st
)
{
    conv_who_are_you(h, actuid, boot_time, seq, st);
    *cas_uuid = rpc_g_dg_my_cas_uuid;
}   


/*
 * C O N V _ A R E _ Y O U _ T H E R E
 *
 * This procedure is called remotely by a server (while in the receive
 * state) that is trying to verify whether or not it's client is still
 * alive and sending input data.  This call executes in the context of
 * that client (i.e. it is a "call back").  We check that the client
 * call is still active and that the boot time from the server matches
 * that the active CCALL
 * 
 * Note that only the server side of this callback is currently
 * implemented.
 */
   
PRIVATE void conv_are_you_there
(
    handle_t h ATTRIBUTE_UNUSED,       /* Not really */
    dce_uuid_t *actuid,
    unsigned32 boot_time,
    unsigned32 *st
)
{
    rpc_dg_ccall_p_t ccall;

    RPC_LOCK_ASSERT(0);

    if (! conv_common(actuid, boot_time, &ccall, st))
    {
        return;
    }

    RPC_DG_CCALL_RELEASE(&ccall);
} 


/*
 * C O N V _ W H O _ A R E _ Y O U _ A U T H
 *
 * This procedure is called remotely by a server that has no "connection"
 * information about a client from which it has just received a call,
 * when the incoming call is authenticated.  In addition to the
 * information returned by conv_who_are_you, a challenge-response
 * takes place to inform the server of the identity of the client.
 */

PRIVATE void conv_who_are_you_auth 
(
    handle_t h ATTRIBUTE_UNUSED, /* not really */
    dce_uuid_t *actuid,
    unsigned32 boot_time,
    ndr_byte *in_data,
    signed32 in_len,
    signed32 out_max_len,
    unsigned32 *seq,
    dce_uuid_t *cas_uuid,
    ndr_byte *out_data,
    signed32 *out_len,
    unsigned32 *st
)
{
    rpc_dg_ccall_p_t ccall;
    rpc_dg_auth_epv_p_t epv;
    ndr_byte *save_out_data = out_data;
    
    RPC_LOCK_ASSERT(0);
    
    if (! conv_common(actuid, boot_time, &ccall, st))
    {
        return;
    }

    *cas_uuid = rpc_g_dg_my_cas_uuid;
    *seq = ccall->c.call_seq;

    /*
     * If there's already a credentials buffer associated with this
     * call handle, free it.  We rely on the underlying security code
     * to do cacheing if appropriate.
     */
    if (ccall->auth_way_info != NULL)
    {
        RPC_MEM_FREE(ccall->auth_way_info, RPC_C_MEM_DG_EPAC);
        ccall->auth_way_info     = NULL;
        ccall->auth_way_info_len = 0;
    }

    /* 
     * Make sure that we really have an authenticated call here, 
     * lest we dereference null and blow up the process.
     */
    epv = ccall->c.auth_epv;
    if (epv == NULL) 
    {
        *st = rpc_s_binding_has_no_auth;
    } 
    else 
    {
	RPC_DG_CALL_UNLOCK(&(ccall->c));
	RPC_UNLOCK(0);
	
	(*epv->way_handler) (ccall->c.key_info, in_data, in_len,
            out_max_len, &out_data, out_len, st);

	RPC_LOCK(0);
	RPC_DG_CALL_LOCK(&(ccall->c));

        if (*out_len > out_max_len)
        {
            /*
             * If the credentials did not fit in the buffer provided,
             * the WAY handler will have alloced up a buffer big enough
             * to hold them, and returned a pointer to that storage in
             * out_data.  
             *
             * Stash a pointer to this buffer in the call handle, copy 
             * as much of the credentials as will fit in the real response 
             * packet, and return a status that indicates that the caller 
             * needs to fetch the rest of the credentials.
             */
            ccall->auth_way_info = out_data;
            ccall->auth_way_info_len = *out_len;

            memcpy(save_out_data, out_data, out_max_len);
            *out_len = out_max_len;

            *st = rpc_s_partial_credentials;
        }
    }
    RPC_DG_CCALL_RELEASE(&ccall);
}

/*
 * C O N V _ W H O _ A R E _ Y O U _ A U T H _ M O R E
 *
 * This routine is used, in conjunction with the conv_who_are_you_auth()
 * operation, for retrieving oversized PACs.  In the event that a client's
 * credentials are too large to fit within a single DG packet, the server
 * can retrieve the PAC, packet by packet, by repeated calls on this
 * operation.
 *
 * Note that because the "conv" interface is serviced directly by the
 * the DG protocol (as opposed to being handled by a call executor thread),
 * there is no additional client overhead with retrieving the PAC by
 * multiple single-packet calls, rather than a single multiple-packet call.
 * The small amount of overhead induced on the server side is more than
 * compensated for by being able to avoid adding flow-control/windowing
 * to the DG protocol's internal handling of the conv interface.
 *
 * Logically, this call returns
 *
 *        client_credentials[index ... (index+out_max_len-1)]
 */

PRIVATE void conv_who_are_you_auth_more (h, actuid, boot_time, index,
                                         out_max_len, out_data, out_len, st)
handle_t h ATTRIBUTE_UNUSED; /* not really */
dce_uuid_t *actuid;
unsigned32 boot_time;
signed32 index;
signed32 out_max_len;
ndr_byte *out_data;
signed32 *out_len;
unsigned32 *st;
{
    rpc_dg_ccall_p_t ccall;
    rpc_dg_auth_epv_p_t epv ATTRIBUTE_UNUSED;

    RPC_LOCK_ASSERT(0);

    if (! conv_common(actuid, boot_time, &ccall, st))
    {
        return;
    }

    if ((unsigned32)(index + out_max_len) >= ccall->auth_way_info_len)
    {
        *out_len = ccall->auth_way_info_len - index;
        *st = rpc_s_ok;
    }
    else
    {
        *out_len = out_max_len;
        *st = rpc_s_partial_credentials;
    }

    memcpy(out_data, ccall->auth_way_info + index, *out_len);
    RPC_DG_CCALL_RELEASE(&ccall);
}
