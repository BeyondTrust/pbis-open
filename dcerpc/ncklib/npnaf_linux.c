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
**  Copyright (c) 1989 by
**      Hewlett-Packard Company, Palo Alto, Ca. & 
**      Digital Equipment Corporation, Maynard, Mass.
**
**
**  NAME
**
**      npnaf_bsd
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  This module contains routines specific to the Internet Protocol,
**  the Internet Network Address Family extension service, and the
**  Linux.
**
**  - taken from npnaf_bsd.c
**
**
**
*/

#include <commonp.h>
#include <com.h>
#include <comnaf.h>
#include <comsoc.h>
#include <npnaf.h>

#include <sys/ioctl.h>
#include <ctype.h>
#include <stddef.h>


/***********************************************************************
 *
 *  Internal prototypes and typedefs.
 */

#ifndef NO_SPRINTF
#  define RPC__NP_NETWORK_SPRINTF   sprintf
#else
#  define RPC__NP_NETWORK_SPRINTF   rpc__np_network_sprintf
#endif


/*
**++
**
**  ROUTINE NAME:       rpc__np_init_local_addr_vec
**
**  SCOPE:              PRIVATE - declared in ipnaf.h
**
**  DESCRIPTION:
**      
**  Initialize the local address vectors.
**
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:                        
**
**      status          A value indicating the status of the routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:
**
**      Update local_np_addr_vec
**
**--
**/

PRIVATE void rpc__np_init_local_addr_vec
(
    unsigned32 *status
)
{
    CODING_ERROR (status);

    *status = rpc_s_ok;
    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc__np_desc_inq_addr
**
**  SCOPE:              PRIVATE - declared in npnaf.h
**
**  DESCRIPTION:
**      
**  Receive a socket descriptor which is queried to obtain family, endpoint
**  and network address.  If this information appears valid for an IP
**  address,  space is allocated for an RPC address which is initialized
**  with the information obtained from the socket.  The address indicating
**  the created RPC address is returned in rpc_addr.
**
**  INPUTS:
**
**      protseq_id      Protocol Sequence ID representing a particular
**                      Network Address Family, its Transport Protocol,
**                      and type.
**
**      desc            Descriptor, indicating a socket that has been
**                      created on the local operating platform.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      rpc_addr_vec
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok               The call was successful.
**
**          rpc_s_no_memory         Call to malloc failed to allocate memory.
**
**          rpc_s_cant_inq_socket  Attempt to get info about socket failed.
**
**          Any of the RPC Protocol Service status codes.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__np_desc_inq_addr 
(
    rpc_protseq_id_t        protseq_id,
    rpc_socket_t            sock,
    rpc_addr_vector_p_t     *rpc_addr_vec,
    unsigned32              *status
)
{
    rpc_np_addr_p_t         np_addr;
    rpc_np_addr_t           loc_np_addr;
#ifdef NP_NETADDR
    char                    *p;
#endif
    int                     err = 0;

    CODING_ERROR (status);

    memset (&loc_np_addr, 0, sizeof(rpc_np_addr_t));

    loc_np_addr.len = sizeof(loc_np_addr) - offsetof(rpc_np_addr_t, sa);

    /*
     * Do a "getsockname" into a local IP RPC address.  If the network
     * address part of the result is non-zero, then the socket must be
     * bound to a particular IP address and we can just return a RPC
     * address vector with that one address (and endpoint) in it.
     * Otherwise, we have to enumerate over all the local network
     * interfaces the local host has and construct an RPC address for
     * each one of them.
     */

    err = rpc__socket_inq_endpoint(sock, (rpc_addr_p_t) &loc_np_addr);
    
    if (err)
    {
        *status = -1;
        return;
    }

    RPC_MEM_ALLOC (
        np_addr,
        rpc_np_addr_p_t,
        sizeof (rpc_np_addr_t),
        RPC_C_MEM_RPC_ADDR,
        RPC_C_MEM_WAITOK);
    
    if (np_addr == NULL)
    {
        *status = rpc_s_no_memory;
        return;
    }
    
    RPC_MEM_ALLOC (
        *rpc_addr_vec,
        rpc_addr_vector_p_t,
        sizeof **rpc_addr_vec,
        RPC_C_MEM_RPC_ADDR_VEC,
        RPC_C_MEM_WAITOK);
    
    if (*rpc_addr_vec == NULL)
    {
        RPC_MEM_FREE (np_addr, RPC_C_MEM_RPC_ADDR);
        *status = rpc_s_no_memory;
        return;
    }
    
    memset(np_addr, 0, sizeof(rpc_np_addr_t));
    np_addr->rpc_protseq_id = protseq_id;
    np_addr->len = sizeof (struct sockaddr_un);
    np_addr->sa = loc_np_addr.sa;
    
    /*
     * Force the address family to AF_UNIX as getsockname()
     * does not work for UNIX domain sockets on all platforms
     */
    np_addr->sa.sun_family = AF_UNIX;
    
    if (np_addr->rpc_protseq_id == RPC_C_PROTSEQ_ID_NCACN_NP)
    {
#ifdef NP_NETADDR
        np_addr->sa.sun_path[RPC_C_ENDPOINT_NP_MAX + 1] = '\\';
        np_addr->sa.sun_path[RPC_C_ENDPOINT_NP_MAX + 2] = '\\';
        gethostname(&np_addr->sa.sun_path[RPC_C_ENDPOINT_NP_MAX + 3], RPC_C_NETADDR_NP_MAX - 3);
        np_addr->sa.sun_path[RPC_C_NETADDR_NP_MAX - 1] = '\0';
        for (p = &np_addr->sa.sun_path[RPC_C_ENDPOINT_NP_MAX + 3]; *p != '.' && *p != '\0'; p++)
            *p = toupper(*p);
        *p = '\0';
#endif /* NP_NETADDR */
    }
    
    (*rpc_addr_vec)->len = 1;
    (*rpc_addr_vec)->addrs[0] = (rpc_addr_p_t) np_addr;
    
    *status = rpc_s_ok;
    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc__np_get_broadcast
**
**  SCOPE:              PRIVATE - EPV declared in npnaf.h
**
**  DESCRIPTION:
**      
**  Return a vector of RPC addresses that represent all the address
**  required so that sending on all of them results in broadcasting on
**  all the local network interfaces.
**
**
**  INPUTS:
**
**      naf_id          Network Address Family ID serves
**                      as index into EPV for IP routines.
**
**      rpc_protseq_id
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:                        
**
**      rpc_addr_vec
**
**      status          A value indicating the status of the routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__np_get_broadcast 
(
    rpc_naf_id_t            naf_id ATTRIBUTE_UNUSED,
    rpc_protseq_id_t        protseq_id,
    rpc_addr_vector_p_t     *rpc_addr_vec,
    unsigned32              *status 
)
{
	naf_id = 0;
	protseq_id = 0;
	rpc_addr_vec = NULL;

    CODING_ERROR (status);

    *status = rpc_s_cant_inq_socket;   
    return;
}
/*
**++
**
**  ROUTINE NAME:       rpc__np_is_local_network
**
**  SCOPE:              PRIVATE - declared in npnaf.h
**
**  DESCRIPTION:
**      
**  Return a boolean value to indicate if the given RPC address is on
**  the same IP subnet.
**
**
**  INPUTS:
**
**      rpc_addr        The address that forms the path of interest
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:                        
**
**      status          A value indicating the status of the routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      result          true => the address is on the same subnet.
**                      false => not.
**
**  SIDE EFFECTS:       none
**
**--
**/
PRIVATE boolean32 rpc__np_is_local_network
(
    rpc_addr_p_t rpc_addr,
    unsigned32   *status
)
{
    CODING_ERROR (status);

    if (rpc_addr == NULL)
    {
        *status = rpc_s_invalid_arg;
        return false;
    }

    *status = rpc_s_ok;

    return true;
}

/*
**++
**
**  ROUTINE NAME:       rpc__np_is_local_addr
**
**  SCOPE:              PRIVATE - declared in npnaf.h
**
**  DESCRIPTION:
**      
**  Return a boolean value to indicate if the given RPC address is the
**  the local IP address.
**
**
**  INPUTS:
**
**      rpc_addr        The address that forms the path of interest
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:                        
**
**      status          A value indicating the status of the routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      result          true => the address is local.
**                      false => not.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE boolean32 rpc__np_is_local_addr
(
    rpc_addr_p_t rpc_addr,
    unsigned32   *status
)
{
    CODING_ERROR (status);

    if (rpc_addr == NULL)
    {
        *status = rpc_s_invalid_arg;
        return false;
    }

    *status = rpc_s_ok;

    return true;
}
