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
**  NAME
**
**      comnaf.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definition of the Network Address Family Services for the Common
**  Communication Services component. These routines are called by the
**  other services internal to the RPC runtime (such as protocol services)
**  to dispatch to the appropriate Network Address Family service.
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comprot.h>    /* Common protocol services */
#include <comnaf.h>     /* Common network address family services */
#include <comp.h>       /* Private communications services */
#include <comtwrref.h>  /* Private tower defs for other RPC components */

#include <assert.h>


/*
**++
**
**  ROUTINE NAME:       rpc__naf_addr_alloc
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to create an RPC Address.
**
**  INPUTS:
**
**      protseq_id      The RPC Protocol Sequence ID which represents a
**                      NAF, NAF Transport Protocol, and NAF Interface Type
**
**      naf_id          The NAF ID of the RPC Address
**
**      endpoint        The string containing the endpoint to be placed 
**                      in the RPC Address
**
**      netaddr         The string containing the network address to be
**                      placed in the RPC Address
**
**      network_options A vector of network options tag and value strings 
**                      to be placed in the RPC Address
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      rpc_addr        The RPC Address to be created in the format of a
**                      particular NAF
**
**      status          A value indicating the return status of the routine
**
**          Any of the values returned by the NAF addr_alloc routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_addr_alloc 
(
    rpc_protseq_id_t        protseq_id,
    rpc_naf_id_t            naf_id,
    unsigned_char_p_t       endpoint,
    unsigned_char_p_t       netaddr,
    unsigned_char_p_t       network_options,
    rpc_addr_p_t            *rpc_addr,
    unsigned32              *status
)
{
    RPC_LOG_NAF_ADDR_ALLOC_NTR;

    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[naf_id].epv->naf_addr_alloc)
        (protseq_id, naf_id, endpoint, netaddr, network_options,
        rpc_addr, status);

    RPC_LOG_NAF_ADDR_ALLOC_XIT;
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_addr_free
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to free an RPC Address.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:
**
**      rpc_addr        The RPC Address to be freed in the format of a
**                      particular NAF
**
**  OUTPUTS:
**
**      status          A value indicating the return status of the routine
**
**          Any of the values returned by the NAF addr_free routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_addr_free 
(
    rpc_addr_p_t            *rpc_addr,
    unsigned32              *status
)
{
    RPC_LOG_NAF_ADDR_FREE_NTR;

    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[(*rpc_addr)->sa.family].epv->naf_addr_free)
        (rpc_addr, status);

    RPC_LOG_NAF_ADDR_FREE_XIT;
}


/*
**++
**
**  ROUTINE NAME:       rpc__naf_addr_vector_free
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to free a vector of
**  RPC Addresses.  Each address and the vector itself are freed.
**  
**  Note that we ignore NULL entries in the vector.  This is to make
**  it easy for recipients of a vector to "steal" the pointers to RPC
**  Addresses and put them in some other data structure.  After the
**  stealing, they null out the vector entry.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:
**
**      rpc_addr_vec    The vector of RPC Addresses to be freed
**
**  OUTPUTS:
**
**      status          A value indicating the return status of the routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_addr_vector_free 
(
    rpc_addr_vector_p_t     *rpc_addr_vec,
    unsigned32              *status
)
{
    unsigned16 i;

    /*
     * In case the vector is empty.
     */
    *status = rpc_s_ok;

    for (i = 0; i < (*rpc_addr_vec)->len; i++)
    {
        if ((*rpc_addr_vec)->addrs[i] != NULL)
        {
            (*rpc_g_naf_id[(*rpc_addr_vec)->addrs[i]->sa.family].epv->naf_addr_free)
                (&(*rpc_addr_vec)->addrs[i], status);
        }
    }

    RPC_MEM_FREE (*rpc_addr_vec, RPC_C_MEM_RPC_ADDR_VEC);
}


/*
**++
**
**  ROUTINE NAME:       rpc__naf_addr_copy
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to copy an RPC Address.
**
**  INPUTS:
**
**      src_rpc_addr    The RPC Address to be copied in the format of a
**                      particular NAF
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      dst_rpc_addr    The RPC Address to be created in the format of a
**                      particular NAF
**
**      status          A value indicating the return status of the routine
**
**          Any of the values returned by the NAF addr_copy routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_addr_copy 
(
    rpc_addr_p_t            src_rpc_addr,
    rpc_addr_p_t            *dst_rpc_addr,
    unsigned32              *status
)
{
    RPC_LOG_NAF_ADDR_COPY_NTR;

    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[src_rpc_addr->sa.family].epv->naf_addr_copy)
        (src_rpc_addr, dst_rpc_addr, status);

    RPC_LOG_NAF_ADDR_COPY_XIT;
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_addr_overcopy
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Like rpc__naf_addr_copy, except uses storage from an existing target 
**  address, if there is one (thus saving the malloc).
**
**  INPUTS:
**
**      src_rpc_addr    The RPC Address to be copied in the format of a
**                      particular NAF
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      dst_rpc_addr    The RPC Address to be created in the format of a
**                      particular NAF
**
**      status          A value indicating the return status of the routine
**
**          Any of the values returned by the NAF addr_copy routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_addr_overcopy 
(
    rpc_addr_p_t            src_rpc_addr,
    rpc_addr_p_t            *dst_rpc_addr,
    unsigned32              *status
)
{
    /*
     * If there is no existing destinition address yet, or if it's not big
     * enough to receive the source address, call the NAF copy.  (Free the
     * existing one first in the latter case.)  Otherwise, just overwrite
     * the existing destination with the source address.
     */
    if (*dst_rpc_addr == NULL || (*dst_rpc_addr)->len < src_rpc_addr->len)
    {
        if (*dst_rpc_addr != NULL)
            (*rpc_g_naf_id[(*dst_rpc_addr)->sa.family].epv->naf_addr_free)
                (dst_rpc_addr, status);
        (*rpc_g_naf_id[src_rpc_addr->sa.family].epv->naf_addr_copy)
            (src_rpc_addr, dst_rpc_addr, status);
    }
    else
    {
        assert((*dst_rpc_addr)->len >= sizeof((*dst_rpc_addr)->sa));
        **dst_rpc_addr = *src_rpc_addr;
        memmove( &(*dst_rpc_addr)->sa, &src_rpc_addr->sa, src_rpc_addr->len);
        *status = rpc_s_ok;
    }
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_addr_set_endpoint
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to set the endpoint in
**  an RPC Address.
**
**  INPUTS:
**
**      endpoint        The string containing the endpoint to be placed 
**                      in the RPC Address
**
**  INPUTS/OUTPUTS:
**
**      rpc_addr        The RPC Address in which to set the endpoint in the
**                      format of a particular NAF
**
**  OUTPUTS:
**
**      status          A value indicating the return status of the routine
**
**          Any of the values returned by the NAF addr_set_endpoint routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_addr_set_endpoint 
(
    unsigned_char_p_t       endpoint,
    rpc_addr_p_t            *rpc_addr,
    unsigned32              *status
)
{
    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[(*rpc_addr)->sa.family].epv->naf_addr_set_endpoint)
        (endpoint, rpc_addr, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_addr_inq_endpoint
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to inquire the endpoint
**  in an RPC Address.
**
**  INPUTS:
**
**      rpc_addr        The RPC Address from which to obtain the endpoint
**                      in the format of a particular NAF
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      endpoint        The string containing the endpoint to be obtained
**                      from the RPC Address
**
**      status          A value indicating the return status of the routine
**
**          Any of the values returned by the NAF addr_inq_endpoint routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_addr_inq_endpoint 
(
    rpc_addr_p_t            rpc_addr,
    unsigned_char_t         **endpoint,
    unsigned32              *status
)
{
    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[rpc_addr->sa.family].epv->naf_addr_inq_endpoint)
        (rpc_addr, endpoint, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_addr_set_netaddr
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to set the network address
**  in an RPC Address.
**
**  INPUTS:
**
**      netaddr         The string containing the network address to be
**                      placed in the RPC Address
**
**  INPUTS/OUTPUTS:
**
**      rpc_addr        The RPC Address in which to set the network address
**                      in the format of a particular NAF
**
**  OUTPUTS:
**
**      status          A value indicating the return status of the routine
**
**          Any of the values returned by the NAF addr_set_netaddr routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_addr_set_netaddr 
(
    unsigned_char_p_t       netaddr,
    rpc_addr_p_t            *rpc_addr,
    unsigned32              *status
)
{
    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[(*rpc_addr)->sa.family].epv->naf_addr_set_netaddr)
        (netaddr, rpc_addr, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_addr_inq_netaddr
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to obtain the network
**  address from an RPC Address.
**
**  INPUTS:
**
**      rpc_addr        The RPC Address from which to obtain the network
**                      address in the format of a particular NAF
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      netaddr         The string containing the network address
**                      obtained from the RPC Address
**
**      status          A value indicating the return status of the routine
**
**          Any of the values returned by the NAF addr_inq_netaddr routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_addr_inq_netaddr (rpc_addr, netaddr, status)
    
rpc_addr_p_t            rpc_addr;
unsigned_char_t         **netaddr;
unsigned32              *status;

{
    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[rpc_addr->sa.family].epv->naf_addr_inq_netaddr)
        (rpc_addr, netaddr, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_addr_set_options
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to set the network options
**  in an RPC Address.
**
**  INPUTS:
**
**      network_options A network options string 
**
**  INPUTS/OUTPUTS:
**
**      rpc_addr        The RPC Address in which to set the network options
**                      in the format of a particular NAF
**
**  OUTPUTS:
**
**      status          A value indicating the return status of the routine
**
**          Any of the values returned by the NAF addr_set_options routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_addr_set_options (network_options, rpc_addr, status)
    
unsigned_char_p_t       network_options;
rpc_addr_p_t            *rpc_addr;
unsigned32              *status;

{
    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[(*rpc_addr)->sa.family].epv->naf_addr_set_options)
        (network_options, rpc_addr, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_addr_inq_options
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to obtain the network
**  options from an RPC Address.
**
**  INPUTS:
**
**      rpc_addr        The RPC Address from which to obtain the network
**                      options in the format of a particular NAF
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      network_options A network options string
**
**      status          A value indicating the return status of the routine
**
**          Any of the values returned by the NAF addr_inq_options routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_addr_inq_options (rpc_addr, network_options, status)
    
rpc_addr_p_t            rpc_addr;
unsigned_char_t         **network_options;
unsigned32              *status;

{
    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[rpc_addr->sa.family].epv->naf_addr_inq_options)
        (rpc_addr, network_options, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_desc_inq_addr
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to return a vector
**  of RPC Addresses that reflect all the local network addresses
**  the supplied socket descriptor will listen on.  The endpoints
**  in each of the returned RPC Addresses will be the same.  If
**  the local host has multiple network addresses, each will be
**  returned in a separate RPC Addresses.
**
**  INPUTS:
**
**      protseq_id      The RPC Protocol Sequence ID which represents a
**                      NAF, NAF Transport Protocol, and NAF Interface Type
**
**      desc            The network descriptor which was returned from the
**                      NAF Service
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      rpc_addr_vec    The returned vector of RPC Addresses
**
**      status          A value indicating the return status of the routine
**
**          Any of the values returned by the NAF desc_inq_addr routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_desc_inq_addr (protseq_id, desc, rpc_addr_vec, status)
    
rpc_protseq_id_t        protseq_id;
rpc_socket_t            desc;
rpc_addr_vector_p_t     *rpc_addr_vec;
unsigned32              *status;

{
    rpc_naf_id_t naf_id;                                     

    naf_id = RPC_PROTSEQ_INQ_NAF_ID(protseq_id);

    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[naf_id].epv->naf_desc_inq_addr)
        (protseq_id, desc, rpc_addr_vec, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_desc_inq_network
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**
**  Dispatch to a Network Address Family Service to return the network
**  interface type and network protocol family for a descriptor (socket).
**
**  INPUTS:
**
**      desc            socket descriptor to query
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      naf_id          network address family id
**
**      socket_type     network interface type id
**
**      protocol_id     network protocol family id
**
**      status          status returned by called routines
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_desc_inq_network
        (desc, naf_id, socket_type, protocol_id, status)

rpc_socket_t              desc;
rpc_naf_id_t              *naf_id;
rpc_network_if_id_t       *socket_type;
rpc_network_protocol_id_t *protocol_id;
unsigned32                *status;

{
    CODING_ERROR (status);

    /*
     * Determine the network address family.
     */
     
    rpc__naf_desc_inq_naf_id (desc, naf_id, status);
    if (*status != rpc_s_ok) return;
    
    /*
     * Vector to the appropriate NAF routine to ascertain the
     * rest of the network information for this descriptor.
     * If the naf_id isn't supported, set status to rpc_s_cant_get_if_id.
     */
    if (RPC_NAF_INQ_SUPPORTED(*naf_id))
    {
        (*rpc_g_naf_id[*naf_id].epv->naf_desc_inq_network)
            (desc, socket_type, protocol_id, status);
    }
    else
    {
        *status = rpc_s_cant_get_if_id;
    }
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_desc_inq_naf_id
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**
**  Return the network address family of a descriptor (socket).
**
**  INPUTS:
**
**      desc            socket descriptor to query
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      naf_id          network address family id
**
**      status          status returned
**                              rpc_s_ok
**                              rpc_s_no_memory
**                              rpc_s_cant_inq_socket
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_desc_inq_naf_id 
(
    rpc_socket_t              desc,
    rpc_naf_id_t              *naf_id,
    unsigned32                *status
)
{
    rpc_addr_p_t        addr;
    unsigned8           buff[sizeof (*addr)];
    rpc_socket_error_t  serr;

    CODING_ERROR (status);

    /*
     * Get the endpoint info, ie. fill in the rpc_addr_t. We're
     * really only interested in the family returned in the socket
     * address and therefore only provide a socket address buffer 2
     * bytes long.
     */
    addr = (rpc_addr_p_t) buff;
#ifdef _SOCKADDR_LEN
    addr->len = sizeof(addr->sa.family) + sizeof(addr->sa.length);
#elif ! defined(AIX32)
    addr->len = sizeof (addr->sa.family);
#else
    addr->len = (long) (&(addr->sa.data) - &(addr->sa));
#endif /* AIX32 */
    
    addr->sa.family = 0;
    serr = rpc__socket_inq_endpoint (desc, addr);

    if (RPC_SOCKET_IS_ERR (serr))
    {
        *status = rpc_s_cant_inq_socket;
        goto error;
    }
    
    /* On some systems, getsockname fails silently on UNIX
       domain sockets.  This has been seen on HP-UX, Darwin, and AIX.
       Detect this case and try getpeername instead */
    if (addr->sa.family == 0)   
    {
        serr = rpc__socket_getpeername (desc, addr);

        if (RPC_SOCKET_IS_ERR (serr))
        {
            *status = rpc_s_cant_inq_socket;
            goto error;
        }
    }

    *naf_id = addr->sa.family;

    /* If we *still* got 0 for some reason, force it to AF_UNIX */
    if (*naf_id == 0)
    {
        *naf_id = AF_UNIX;
    }

    *status = rpc_s_ok;

error:

    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_desc_inq_protseq_id
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**
**  Return the protocol sequence id for a descriptor (socket).
**  (We are passed the network protocol family id.)
**
**  INPUTS:
**
**      desc            socket descriptor to query
**
**      protocol_id     network protocol family id
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      protseq_id      network protocol sequence id
**
**      status          status returned
**                              rpc_s_ok
**                              rpc_s_invalid_rpc_protseq
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_desc_inq_protseq_id 
(
    rpc_socket_t              desc,
    rpc_network_protocol_id_t protocol_id,
    rpc_protseq_id_t         *protseq_id,
    unsigned32               *status
)
{
    rpc_protseq_id_t         i;           /* index into protseq table */
    rpc_naf_id_t             naf_id;      /* naf id returned to us */
    rpc_network_if_id_t      socket_type; /* network protocol type */
    
    CODING_ERROR (status);

    /*
     * Determine the correct network address family.
     */
     
    rpc__naf_desc_inq_naf_id (desc, &naf_id, status);
    if (*status != rpc_s_ok) return;

    /*
     * Determine the rest of the network stuff.
     */
     
    rpc__naf_desc_inq_network
        (desc, &naf_id, &socket_type, &protocol_id, status);
    if (*status != rpc_s_ok) return;

    /*
     * Now that we've looked everything up, determine the protocol
     * sequence id by scanning the protocol sequence table for a match
     * on the network address family id, socket type (rpc protocol id)
     * and the network protocol id.
     */

    for (i=0; i<RPC_C_PROTSEQ_ID_MAX; i++)
    {
        if ((naf_id == rpc_g_protseq_id[i].naf_id) &&
            (protocol_id == rpc_g_protseq_id[i].network_protocol_id) &&
            (socket_type == rpc_g_protseq_id[i].network_if_id))
        {
            break;
        }
    }

    /*
     * Check the results of the scan, set status and return protseq_id.
     */

     if (i<RPC_C_PROTSEQ_ID_MAX)
     {
        *protseq_id = rpc_g_protseq_id[i].rpc_protseq_id;
        *status = rpc_s_ok;
     }
     else
     {
        *protseq_id = RPC_C_INVALID_PROTSEQ_ID;
        *status = rpc_s_invalid_rpc_protseq;
     }
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_desc_inq_peer_addr
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**
**  Return the network address (name) of a connected peer.
**
**  INPUTS:
**
**      desc            socket descriptor to query
**
**      protseq_id      network protocol sequence id
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      addr            peer address
**
**      status          status returned
**                              rpc_s_ok
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_desc_inq_peer_addr 
(
    rpc_socket_t             desc,
    rpc_protseq_id_t         protseq_id,
    rpc_addr_p_t            *addr,
    unsigned32              *status
)
{
    rpc_naf_id_t naf_id;                                     

    naf_id = RPC_PROTSEQ_INQ_NAF_ID(protseq_id);

    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[naf_id].epv->naf_desc_inq_peer_addr)
        (protseq_id, desc, addr, status);
}


/*
**++
**
**  ROUTINE NAME:       rpc__naf_inq_max_tsdu
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to find out the size
**  of the largest possible transport service data unit (TSDU); i.e.,
**  the size of the largest transport protocol data unit (TPDU) that
**  can be passed through the transport service interface.  ("data units"
**  excludes all header bytes.)
**
**  INPUTS:
**
**      pseq_id         The protocol sequence of interest
**
**  INPUTS/OUTPUTS:
**
**      none
**
**  OUTPUTS:
**
**      max_tsdu        A pointer to where the maximum TSDU should be stored.
**
**      status          A value indicating the return status of the routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_inq_max_tsdu 
(
    rpc_protseq_id_t pseq_id,
    unsigned32 *max_tsdu,
    unsigned32     *status
)
{
    rpc_naf_id_t naf_id;                                     
    rpc_network_if_id_t iftype;
    rpc_network_protocol_id_t protocol;

    naf_id   = RPC_PROTSEQ_INQ_NAF_ID(pseq_id);
    iftype   = RPC_PROTSEQ_INQ_NET_IF_ID(pseq_id);
    protocol = RPC_PROTSEQ_INQ_NET_PROT_ID(pseq_id);

    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[naf_id].epv->naf_inq_max_tsdu)
        (naf_id, iftype, protocol, max_tsdu, status);
}


/*
**++
**
**  ROUTINE NAME:       rpc__naf_inq_max_pth_unfrg_tpdu
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to find out the size
**  of the largest transport protocol data unit (TPDU) that can be sent
**  to a particular address without being fragmented along the way.
**
**  INPUTS:
**   
**      rpc_addr        The address that forms the path of interest
**
**  INPUTS/OUTPUTS:
**
**      none
**
**  OUTPUTS:
**
**      max_tpdu        A pointer to where the maximum TPDU should be stored.
**
**      status          A value indicating the return status of the routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_inq_max_pth_unfrg_tpdu 
(
    rpc_addr_p_t rpc_addr,
    unsigned32 *max_tpdu,
    unsigned32     *status
)
{
    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[rpc_addr->sa.family].epv->naf_inq_max_pth_unfrg_tpdu)
        (rpc_addr, RPC_PROTSEQ_INQ_NET_IF_ID(rpc_addr->rpc_protseq_id), 
        RPC_PROTSEQ_INQ_NET_PROT_ID(rpc_addr->rpc_protseq_id), max_tpdu, status);
}


/*
**++
**
**  ROUTINE NAME:       rpc__naf_inq_max_loc_unfrg_tpdu
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to find out the
**  size of the largest transport protocol data unit (TPDU) that can
**  fit (without fragmentation) through the "widest" network interface
**  on the local host.
**
**  INPUTS:
**
**      pseq_id         The protocol sequence of interest
**
**  INPUTS/OUTPUTS:
**
**      none
**
**  OUTPUTS:
**
**      max_tpdu        A pointer to where the maximum TPDU should be stored.
**
**      status          A value indicating the return status of the routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_inq_max_loc_unfrg_tpdu 
(
    rpc_protseq_id_t pseq_id,
    unsigned32 *max_tpdu,
    unsigned32     *status
)
{
    rpc_naf_id_t naf_id;                                     
    rpc_network_if_id_t iftype;
    rpc_network_protocol_id_t protocol;

    naf_id   = RPC_PROTSEQ_INQ_NAF_ID(pseq_id);
    iftype   = RPC_PROTSEQ_INQ_NET_IF_ID(pseq_id);
    protocol = RPC_PROTSEQ_INQ_NET_PROT_ID(pseq_id);

    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[naf_id].epv->naf_inq_max_loc_unfrg_tpdu)
        (naf_id, iftype, protocol, max_tpdu, status);
}


/*
**++
**
**  ROUTINE NAME:       rpc__naf_get_broadcast
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to find out what the maximum
**  transport protocol data unit size is for that address family.
**
**  INPUTS:
**
**      naf_id          The Netork Address Family we're interested in.
**      prot_seq        The protocol sequence 
**
**  INPUTS/OUTPUTS:
**
**      none
**
**  OUTPUTS:
**
**      rpc_addrs       A pointer to a pointer to a structure in which to put the
**                      list of broadcast addresses.  This structure will be allocated
**                      by the address family specific routine
**
**      status          A value indicating the return status of the routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_get_broadcast 
(
    rpc_naf_id_t            naf_id,
    rpc_protseq_id_t        protseq_id,
    rpc_addr_vector_p_t     *rpc_addrs,
     unsigned32 	    *status 
)
{
    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[naf_id].epv->naf_get_broadcast)
        (naf_id, protseq_id, rpc_addrs, status);
}


/*
**++
**
**  ROUTINE NAME:       rpc__naf_addr_compare
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to determine if the two
**  input addresses are equal.
**
**  INPUTS:
**
**      addr1           The first RPC Address to be compared.
**
**      addr2           The second RPC Address to be compared.
**
**  INPUTS/OUTPUTS:
**
**      none
**
**  OUTPUTS:
**
**      status          A value indicating the return status of the routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_c_true if the addresses are equal
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE boolean rpc__naf_addr_compare 
(
    rpc_addr_p_t            addr1,
    rpc_addr_p_t            addr2,
    unsigned32              *status
)
{   
    /*
     * Handle NULL pointers here.
     */
    if ((addr1 == NULL) || (addr2 == NULL))
    {
        *status = rpc_s_ok;
        if (addr1 == addr2) return true;
        else return false;
    }

    /*
     * dispatch to the appropriate NAF service
     */
    if ((*rpc_g_naf_id[addr1->sa.family].epv->naf_addr_compare)
        (addr1, addr2, status))
        return true;
    else
        return false;
}


/*
**++
**
**  ROUTINE NAME:       rpc__naf_set_pkt_nodelay
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to set the option
**  for the transport which will result in the transport "pushing" onto
**  the network everything it is given as it is given.
**  input addresses are equal.
**
**  INPUTS:
**
**      desc            The network descriptor representing the
**                      transport on which to set the option.
**      rpc_addr        The rpc address of the remote side of the
**                      connection. If NULL it will be determined
**                      from the network descriptor.
**
**  INPUTS/OUTPUTS:
**
**      none
**
**  OUTPUTS:
**
**      status          A value indicating the return status of the routine
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

PRIVATE void rpc__naf_set_pkt_nodelay 
(
    rpc_socket_t            desc,
    rpc_addr_p_t            rpc_addr,
    unsigned32              *status
)
{   
    rpc_naf_id_t            naf_id;

    /*
     * dispatch to the appropriate NAF service
     */
    if (rpc_addr == NULL)
    {
        rpc__naf_desc_inq_naf_id (desc, &naf_id, status);
        if (*status != rpc_s_ok) return;
    }
    else
    {
        naf_id = rpc_addr->sa.family;
    }
    (*rpc_g_naf_id[naf_id].epv->naf_set_pkt_nodelay)
        (desc, status);
}


/*
**++
**
**  ROUTINE NAME:       rpc__naf_is_connect_closed
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to determine
**  whether the connection represented by the network descriptor given
**  is closed.
**
**  INPUTS:
**
**      desc            The network descriptor representing the
**                      connection.
**
**  INPUTS/OUTPUTS:
**
**      none
**
**  OUTPUTS:           
**
**      status          A value indicating the return status of the routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      boolean         true if the connection is closed, false otherwise.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE boolean rpc__naf_is_connect_closed 
(
    rpc_socket_t            desc,
    unsigned32              *status
)
{   
    rpc_naf_id_t            naf_id;

    /*
     * dispatch to the appropriate NAF service
     */
    rpc__naf_desc_inq_naf_id (desc, &naf_id, status);
    if (*status != rpc_s_ok) return TRUE; /* ??? */
    return ((*rpc_g_naf_id[naf_id].epv->naf_is_connect_closed)
            (desc, status));
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_addr_from_sa
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Create an RPC addr from a aockaddr.
**
**  INPUTS:
**
**      sockaddr        The sockaddr to use in the created RPC addr
**
**      sockaddr_len    Length of "sockaddr"
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      rpc_addr        Returned RPC addr
**
**      status          A value indicating the return status of the routine:
**                          rpc_s_ok
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_addr_from_sa 
(
    sockaddr_p_t     sockaddr,
    unsigned32       sockaddr_len,
    rpc_addr_p_t     *rpc_addr,
    unsigned32     *status
)
{
    /*
     * Allocate memory for an RPC address.
     */
    RPC_MEM_ALLOC (
        *rpc_addr,
        rpc_addr_p_t,
        (sockaddr_len + sizeof(rpc_protseq_id_t) + sizeof(unsigned32)),
        RPC_C_MEM_RPC_ADDR,
        RPC_C_MEM_WAITOK);

    /*
     * Copy the sockaddr into the RPC addr.
     */
    memcpy (&((*rpc_addr)->sa), sockaddr, sockaddr_len);

    /*
     * Set the sockaddr length into the RPC addr.
     */
    (*rpc_addr)->len = sockaddr_len;

    *status = rpc_s_ok;
    return;
}
/*
**++
**
**  ROUTINE NAME:       rpc__naf_tower_flrs_from_addr
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to create lower tower 
**  floors from an RPC addr.
**
**  INPUTS:
**
**      rpc_addr	RPC addr to convert to lower tower floors.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      lower_flrs      The returned lower tower floors.
**
**      status          A value indicating the return status of the routine:
**                          rpc_s_ok
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_tower_flrs_from_addr 
(
    rpc_addr_p_t       rpc_addr,
    twr_p_t            *lower_flrs,
    unsigned32         *status
)
{
    /*
     * Dispatch to the appropriate NAF service tower_flrs_from_addr routine
     * passing through the args (rpc_addr, lower_flrs, status):
     */

    (*rpc_g_naf_id[rpc_addr->sa.family].epv->naf_tower_flrs_from_addr)
       (rpc_addr, lower_flrs, status);
}

/*
**++
**
**  ROUTINE NAME:       rpc__naf_tower_flrs_to_addr
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to create an RPC addr
**  from a protocol tower.
**
**  INPUTS:
**
**      tower_octet_string
**                      Protocol tower octet string whose floors are used to
**                      construct an RPC addr
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      rpc_addr	RPC addr constructed from a protocol tower.
**
**      status          A value indicating the return status of the routine:
**                          rpc_s_ok
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_tower_flrs_to_addr 
(
  byte_p_t           tower_octet_string,
  rpc_addr_p_t       *rpc_addr,
  unsigned32         *status
)
{
    rpc_protseq_id_t  protseq_id;
    rpc_naf_id_t      naf_id;
    unsigned16        flr_count;
    rpc_tower_ref_p_t tower_ref;
    unsigned32        temp_status;

    CODING_ERROR (status);

#ifdef RPC_NO_TOWER_SUPPORT

    *status = rpc_s_coding_error;

#else

    /*
     * Need a tower_ref_t in order to determine the protocol sequence for
     * this tower. 
     */

    /*
     * Get the floor count from the octet string and convert it to local
     * endian rep.
     */
    memcpy (&flr_count, tower_octet_string, RPC_C_TOWER_FLR_COUNT_SIZE);

    if (NDR_LOCAL_INT_REP != ndr_c_int_little_endian)
    {
        SWAB_INPLACE_16 ((flr_count));
    }

    /*
     * Allocate a tower_ref_t, with flr_count floors and 
     * initialize the tower ref with the tower_octet_string. We are in
     * the import path here, we must have a complete tower. So start at
     * floor 1.
     * beginning at floor 1.
     */
    rpc__tower_ref_alloc (tower_octet_string, (unsigned32) flr_count, 1, 
        &tower_ref, status);

    if (*status != rpc_s_ok)
    {
        return;
    }

    /*
     * Get the protocol sequence id for this protocol tower.
     */
    rpc__tower_ref_inq_protseq_id (tower_ref, &protseq_id, status);

    if (*status != rpc_s_ok)
    {
        rpc__tower_ref_free (&tower_ref, &temp_status);
        return;
    }

    /*
     * Done with the tower ref
     */
    rpc__tower_ref_free (&tower_ref, status);

    if (*status != rpc_s_ok)
    {
        return;
    }

    if (! RPC_PROTSEQ_INQ_SUPPORTED(protseq_id))
    {
        *status = rpc_s_protseq_not_supported;
        return;
    }

    /*
     * Get the NAF ID from the protocol sequence.
     */
    naf_id = RPC_PROTSEQ_INQ_NAF_ID (protseq_id);

    /* Dispatch to the appropriate NAF service tower_flrs_to_addr routine
     * passing through the args (tower, rpc_addr, status):
     */
    (*rpc_g_naf_id[naf_id].epv->naf_tower_flrs_to_addr)
       (tower_octet_string, rpc_addr, status);

    if (*status != rpc_s_ok)
    {
        return;
    }

    /* 
     * Set the protocol sequence id into the RPC addr.
     */
    (*rpc_addr)->rpc_protseq_id = protseq_id;

#endif

    return;
}


/*
**++
**
**  ROUTINE NAME:       rpc__naf_set_port_restriction
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to set a protocol
**  sequence's port restriction structure.
**  
**
**  INPUTS:
**
**      protseq_id
**                      The protocol sequence id to set port restriction
**                      on. 
**      n_elements
**                      The number of port ranges passed in.
**
**      first_port_name_list
**                      An array of pointers to strings containing the
**                      lower bound port names. 
**
**      last_port_name_list
**                      An array of pointers to strings containing the
**                      upper bound port names.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       
**
**--
**/

PRIVATE void rpc__naf_set_port_restriction 
(
     rpc_protseq_id_t            protseq_id,
     unsigned32                  n_elements,
     unsigned_char_p_t           *first_port_name_list,
     unsigned_char_p_t           *last_port_name_list,
     unsigned32                  *status
)               
{

    rpc_naf_id_t        naf_id;

    /* 
     * Dispatch to appropriate NAF service.
     */

    naf_id = RPC_PROTSEQ_INQ_NAF_ID (protseq_id);

    (*rpc_g_naf_id[naf_id].epv->naf_set_port_restriction) 
        (protseq_id,
         n_elements,            
         first_port_name_list, 
         last_port_name_list,  
         status);
}                                       /* rpc__naf_set_port_restriction */


/*
**++
**
**  ROUTINE NAME:       rpc__naf_get_next_restricted_port
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Provides the next restricted port to try to bind to.  
**
**  INPUTS:
**
**      protseq_id
**                      The protocol sequence id of that the caller is
**                      trying to bind to.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      port_name
**                      A pointer to an address-family-specific port name.  
**
**      status
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_get_next_restricted_port
(
     rpc_protseq_id_t            protseq_id,
     unsigned_char_p_t           *port_name,
     unsigned32                  *status
)
{

    rpc_naf_id_t        naf_id;

    /* 
     * Dispatch to appropriate NAF service.
     */

    naf_id = RPC_PROTSEQ_INQ_NAF_ID (protseq_id);

    (*rpc_g_naf_id[naf_id].epv->naf_get_next_restricted_port) 
        (protseq_id, port_name, status);
    
}                                       /* rpc__naf_get_next_restricted_port */


/*
**++
**
**  ROUTINE NAME:       rpc__naf_inq_max_frag_size
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:
**      
**  Dispatch to a Network Address Family Service to find out the size
**  of the largest fragment size that can be sent to a particular
**  address
**
**  INPUTS:
**
**      rpc_addr        The address that forms the path of interest
**
**  INPUTS/OUTPUTS:
**
**      none
**
**  OUTPUTS:
**
**      max_frag_size   A pointer to where the maximum fragment size
**                      should be stored.
**
**      status          A value indicating the return status of the routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__naf_inq_max_frag_size
(
 rpc_addr_p_t rpc_addr,
 unsigned32   *max_frag_size,
 unsigned32   *status
)
{
    /*
     * dispatch to the appropriate NAF service
     */
    (*rpc_g_naf_id[rpc_addr->sa.family].epv->naf_inq_max_frag_size)
        (rpc_addr, max_frag_size, status);
}
