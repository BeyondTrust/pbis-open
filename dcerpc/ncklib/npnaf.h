/*
 * Copyright (c) 2007, Novell, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Novell, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
#ifndef _NPNAF_H
#define _NPNAF_H	1
/*
**
**  NAME
**
**      npnaf.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definitions and Data Type declarations
**  used by Windows NT Named Pipes Network Address Family Extension
**  service.
**
**
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dce/dce.h>

#ifndef sec_id_base_v0_0_included
#include <dce/id_base.h>
#endif

/***********************************************************************
 *
 *  Include the Internet specific socket address 
 */


#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#else
#include <un.h>
#endif

#ifndef RPC_C_NP_DIR
#define RPC_C_NP_DIR	"/var/opt/novell/xad/rpc"
#endif

#ifndef RPC_C_NP_DIR_LEN
#define RPC_C_NP_DIR_LEN (sizeof(RPC_C_NP_DIR) - 1)
#endif

#define RPC_C_NP_SEC_CONTEXT_MIN_LEN    (4 /*Length*/ + 4 /*Version*/ + 4/*UserNameLength*/ + 4/*DomainNameLength*/ + 4/*SessionKeyLength*/)
#define RPC_C_NP_SEC_CONTEXT_MAX_LEN	(1024)

/* NetBIOS name length */
#define RPC_C_NETADDR_NP_MAX            18

/* Rest is for the path. */
#define RPC_C_ENDPOINT_NP_MAX           (RPC_C_PATH_NP_MAX - RPC_C_NETADDR_NP_MAX - 1)

/***********************************************************************
 *
 *  The representation of an RPC Address that holds an NP address.
 */

typedef struct rpc_addr_np_t
{
    rpc_protseq_id_t        rpc_protseq_id;
    socklen_t               len;
    struct sockaddr_un      sa;
    char                    remote_host[64];
} rpc_np_addr_t, *rpc_np_addr_p_t;

/*
 * Max Local DG Fragment Size:
 *
 *   The size in bytes of the largest DG fragment that can be sent to
 *   a "local" address. The data won't be transmitted over the "wire"
 *   by the transport service, i.e., the loopback is done on the local
 *   host. This is determined when the socket is created and won't
 *   change in the life of the socket.
 *
 * The constant defined here is based on experimentation.
 *
 * Caution: This must be less than RPC_C_DG_MAX_FRAG_SIZE::dg.h!
 */

#ifndef RPC_C_NP_MAX_LOCAL_FRAG_SIZE
#define RPC_C_NP_MAX_LOCAL_FRAG_SIZE (8 * 1024)
#endif


/***********************************************************************
 *
 *  Routine Prototypes for the Internet Extension service routines.
 */
    
#ifdef __cplusplus
extern "C" {
#endif


PRIVATE void rpc__np_init (  
        rpc_naf_epv_p_t             * /*naf_epv*/,
        unsigned32                  * /*status*/
    );

PRIVATE void rpc__np_desc_inq_addr (
        rpc_protseq_id_t             /*protseq_id*/,
        rpc_socket_t                 /*desc*/,
        rpc_addr_vector_p_t         * /*rpc_addr_vec*/,
        unsigned32                  * /*st*/
    );

PRIVATE void rpc__np_get_broadcast (
        rpc_naf_id_t                 /*naf_id*/,
        rpc_protseq_id_t             /*rpc_protseq_id*/,
        rpc_addr_vector_p_t         * /*rpc_addrs*/,
        unsigned32                  * /*status*/
    );

PRIVATE void rpc__np_init_local_addr_vec (
        unsigned32                  * /*status*/
    );

PRIVATE boolean32 rpc__np_is_local_network (
        rpc_addr_p_t                 /*rpc_addr*/,
        unsigned32                  * /*status*/
    );

PRIVATE boolean32 rpc__np_is_local_addr (
        rpc_addr_p_t                 /*rpc_addr*/,
        unsigned32                  * /*status*/
    );

#ifdef __cplusplus
}
#endif

              
#endif /* _NPNAF_H */
