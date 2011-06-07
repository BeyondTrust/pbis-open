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
**      cnnet.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  The NCA Connection Protocol Service's Network Service.
**
**
 */

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comprot.h>    /* Common protocol services */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnpkt.h>      /* NCA Connection packet encoding */
#include <cnid.h>       /* NCA Connection local ID service */
#include <cnassoc.h>    /* NCA Connection association service */
#include <cnassm.h>     /* NCA Connection association state machine */
#include <cnfbuf.h>     /* NCA Connection fragment buffer service */
#include <cncall.h>     /* NCA Connection call service */
#include <cnnet.h>
#include <lw/ntstatus.h>

/***********************************************************************/
/*
 * Global variables
 */
static unsigned32	rpc_g_cn_socket_read_buffer=0;
static unsigned32	rpc_g_cn_socket_write_buffer=0;


/******************************************************************************/
/*
 * Local defines
 */

/*
 * The server timeout for recvmsg()
 */
#define RPC_C_SOCKET_SERVER_RECV_TIMER	( rpc_g_cn_assoc_grp_tbl.grp_server_timer.frequency / RPC_C_CLOCK_HZ )

/*
 * The client timeout for recvmsg()
 */
#define RPC_C_SOCKET_CLIENT_RECV_TIMER	( rpc_g_cn_assoc_grp_tbl.grp_client_timer.frequency / RPC_C_CLOCK_HZ )


/***********************************************************************/
/*
 * Local routines
 */
INTERNAL void rpc__cn_network_desc_inq_ep (
    rpc_socket_t             /*desc*/,
    rpc_protseq_id_t         /*pseq_id*/,
    unsigned_char_p_t       * /*endpoint*/,
    unsigned32              *status);

INTERNAL pointer_t rpc__cn_network_init_desc (
    rpc_socket_t                * /*desc*/,
    boolean32                    /*spawned*/,
    rpc_protseq_id_t             /*pseq_id*/,
    unsigned32                   /*backlog*/,
    unsigned32                  *st);


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_network_desc_inq_ep
**
**  SCOPE:              INTERNAL - declared locally.
**
**  DESCRIPTION:
**
**  This routine will return an endpoint for the given socket and protocol
**  sequence id.
**
**  INPUTS:
**
**      desc            The network descriptor (socket) being queried.
**
**      pseq_id         The protocol sequence id of the network descriptor.
**
**  INPUTS/OUTPUTS:
**
**      endpoint        A pointer to the endpoint determined by this routine.
**
**  OUTPUTS:            none
**
**      status          Status returned.
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

INTERNAL void rpc__cn_network_desc_inq_ep 
(
  rpc_socket_t            desc,
  rpc_protseq_id_t        pseq_id,
  unsigned_char_p_t       *endpoint,
  unsigned32              *status
)
{
    rpc_addr_vector_p_t rpc_addr_vec;
    unsigned32          temp_status;
    
    RPC_CN_DBG_RTN_PRINTF (rpc__cn_network_desc_inq_ep);
    CODING_ERROR (status);

    /*
     * Create an RPC address from the socket descriptor.
     */
    rpc__naf_desc_inq_addr (pseq_id, desc, &rpc_addr_vec, status);
    if (*status != rpc_s_ok) return;
    
    if (rpc_addr_vec->len == 0)
    {
        rpc__naf_addr_vector_free (&rpc_addr_vec, &temp_status);
        *status = rpc_s_no_addrs;
        return;
    }

    /*
     * Inquire the endpoint from one of the RPC addresses returned.
     */
    rpc__naf_addr_inq_endpoint (rpc_addr_vec->addrs[0], endpoint, status);
    if (*status != rpc_s_ok)
    {
        rpc__naf_addr_vector_free (&rpc_addr_vec, &temp_status);
        return;
    }

    /*
     * Free the vector of RPC addresses.
     */
    rpc__naf_addr_vector_free (&rpc_addr_vec, status);
}

/***********************************************************************/


/*
**++
**
**  ROUTINE NAME:       rpc__cn_network_use_protseq
**
**  SCOPE:              PRIVATE - declared in cnnet.h
**
**  DESCRIPTION:
**
**  This routine will create the appropriate number of sockets based
**  on the max number of concurrent calls requested and set them up to
**  receive connect requests.
**
**  INPUTS:
**
**      pseq_id         The protocol sequence id over which to
**                      create the socket.
**      max_calls       The min number of concurrent call requests
**                      the server wishes to be able to handle.
**      rpc_addr        The rpc_addr containing the endpoint and use
**                      to bind to the socket. 
**      endpoint        The endpoint referred to above.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          status returned
**                              rpc_s_ok
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

PRIVATE void rpc__cn_network_use_protseq 
(
  rpc_protseq_id_t        pseq_id,
  unsigned32              max_calls,
  rpc_addr_p_t            rpc_addr,
  unsigned_char_p_t       endpoint,
  unsigned32              *status
)
{
    unsigned32          i;
    unsigned32          num_socks;
    unsigned32          created_sock_index;
    rpc_socket_t        sock;
    rpc_socket_error_t  serr;
    pointer_t           priv_info;
    rpc_socket_t        *sock_list;
    unsigned32          backlog;
    unsigned32          temp_status;
    static boolean      spawned              = false;

    /*
     * Local/Static var's for use in auto-start handling.
     */
#ifdef AUTOSTART
    static boolean              spawned_same_address = false;
    static boolean              spawned_checked      = false;
           rpc_naf_id_t         auto_naf_id;
           rpc_network_if_id_t  auto_network_if_id;
           rpc_protocol_id_t    auto_prot_id;
           rpc_addr_vector_p_t  auto_rpc_addr_vec    = NULL;
    static unsigned_char_p_t    auto_endpoint        = NULL;
           unsigned32           auto_status          = rpc_s_coding_error;
#endif

    CODING_ERROR (status);
    
    RPC_CN_DBG_RTN_PRINTF (rpc__cn_network_use_protseq);

    /*
     * If the caller specified the default number of max calls use
     * the maximum listen backlog for a single socket. This is because
     * for a well-known endpoint we can only create one socket.
     */
    if (max_calls == rpc_c_protseq_max_calls_default)
    {
        max_calls = RPC_C_LISTEN_BACKLOG;
    }
    else
    {
        /*
         * If the callers has specified something other than the max
         * calls default. If it exceeds the system max of queued
         * connections on a socket and is using a well-known
         * endpoint then return an error. Mutiple sockets using the
         * same endpoint cannot be created.
         */
        if ((endpoint != NULL) && (max_calls > RPC_C_LISTEN_BACKLOG))
        {
           *status = rpc_s_calls_too_large_for_wk_ep; 
           return;
       }
    }

#ifdef AUTOSTART
    /*
     *************************************************************************
     * Autostart processing.                                                 *
     *                                                                       *
     * IF we have been spawned AND the socket associated with stdin has the  *
     * same address family, socket type, protocol family and endpoint as the *
     * protocol sequence being registered AND we have not already processed  *
     * the spawned case, THEN don't create a socket but use stdin instead.   *
     *************************************************************************
     */

    /*
     * See if we've already checked for auto-spawning.
     * If we have, don't do it again.
     */
    if (!spawned_checked)
    {
        /*
         * Determine the network address family id from the
         * RPC address given.
         */
        auto_naf_id = rpc_addr->sa.family;
        
        /*
         * If this call succeeds, we can safely assume stdin is a socket and
         * we will also have all the information about the socket that we need.
         */
        rpc__naf_desc_inq_network (0, 
                                   &auto_naf_id, 
                                   &auto_network_if_id, 
                                   &auto_prot_id, 
                                   &auto_status);
        spawned_checked = true;
        spawned = (auto_status == rpc_s_ok);

        /*
         * If we are spawned then determine the address associated with the
         * socket on stdin.  Then get the endpoint from the address for later
         * comparison with the endpoint associated with the protocol sequence
         * being registered (if it's well known).
         */
        if (spawned)
        {
            rpc__naf_desc_inq_addr (pseq_id, 0, &auto_rpc_addr_vec, status);
            if (*status != rpc_s_ok)
            {
                return;
            }
            if (auto_rpc_addr_vec->len == 0)
            {
                rpc__naf_addr_vector_free (&auto_rpc_addr_vec, &auto_status);
                *status = rpc_s_no_addrs;
                return;
            }
            rpc__naf_addr_inq_endpoint (auto_rpc_addr_vec->addrs[0],
                                        &auto_endpoint,
                                        status);
            if (*status != rpc_s_ok)
            {
                rpc__naf_addr_vector_free (&auto_rpc_addr_vec, &auto_status);
                return;
            }
            rpc__naf_addr_vector_free (&auto_rpc_addr_vec, &auto_status);
        }
    }

    /*
     * If we have been given an endpoint, then check the endpoint against
     * the endpoint of the socket on stdin.
     */
    if (endpoint != NULL)
    {
        if (spawned)
        {
            spawned_same_address = !strcmp ((char *) endpoint, 
                                            (char *) auto_endpoint);
        }
    }

#endif

    /*
     * Create as many sockets as are required to fufill the server's
     * request to be able to handle "max_calls" concurrent call
     * requests. For the CN protocol this means being able to
     * concurrently be able to queue and subsequently accept
     * "max_calls" connect requests. Since each socket can queue
     * "RPC_C_LISTEN_BACKLOG" connect requests the number of sockets
     * required are "max_calls"/"RPC_C_LISTEN_BACKLOG". Note that we
     * will not add the sockets to the listener thread's pool until all
     * of them have been created. This will avoid the problem of not
     * having a rpc__network_remove_desc function.
     */
    num_socks = (max_calls + RPC_C_LISTEN_BACKLOG - 1)/RPC_C_LISTEN_BACKLOG;
    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("(rpc__cn_network_use_protseq) Creating %d sockets\n", 
                     num_socks));
    RPC_MEM_ALLOC (sock_list, 
                   rpc_socket_t *, 
                   ((num_socks) * sizeof (rpc_socket_t)),
                   RPC_C_MEM_SOCKET_LIST,
                   RPC_C_MEM_WAITOK);
    for (i = 0, backlog = max_calls; 
         i < num_socks; 
         i++, backlog -= RPC_C_LISTEN_BACKLOG)
    {
        /*
         * First, create a socket for this RPC Protocol Sequence.
         */
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                        ("(rpc__cn_network_use_protseq) Created socket #%d\n", 
                         (i + 1)));
#ifdef AUTOSTART
        /*
         * See if we should use stdin as a socket or create a new one.
         */
        if ((i == 0) && (spawned_same_address))
        {
            /*
             * Just set the socket descriptor to stdin (0).
             */
            sock = 0;
        }
        else
#endif
        {
            serr = rpc__socket_open(pseq_id, NULL, &sock);
            if (RPC_SOCKET_IS_ERR (serr))
            {
                *status = rpc_s_cant_create_sock;
                break;
            }
        }

        /*
         * Set the socket to close itself if the process execs.
         */
        rpc__socket_set_close_on_exec(sock);

        /*
         * Next, bind the socket to the RPC address.
         */
        rpc__naf_addr_set_endpoint (endpoint, &rpc_addr, status);
        if (*status != rpc_s_ok)
        {
            RPC_SOCKET_CLOSE (sock);
            break;
        }
        
        serr = rpc__socket_bind (sock, rpc_addr);
        if (RPC_SOCKET_IS_ERR (serr))
        {
            *status = rpc_s_cant_bind_sock;
            RPC_SOCKET_CLOSE (sock);
            break;
        }

        /*
         * Initialize the socket.
         */
        priv_info = rpc__cn_network_init_desc (&sock,
                                               spawned,
                                               pseq_id,
                                               MIN (RPC_C_LISTEN_BACKLOG, backlog),
                                               status);
        if (*status != rpc_s_ok)
        {
            RPC_SOCKET_CLOSE (sock);
            break;
        }

        /*
         * Finally, add the socket to the listener thread's select
         * pool of sockets.
         */
        rpc__network_add_desc (sock, 
                               true, 
                               (endpoint == NULL), 
                               pseq_id, 
                               priv_info, 
                               status);
        
        if (*status != rpc_s_ok)
        {
            RPC_SOCKET_CLOSE (sock);
            break;
        }

        /*
         * Add the socket to our list we're keeping.
         */
        sock_list[i] = sock;
    }

    /*
     * Check whether we were able to create all the sockets we
     * needed to. If not, remove all sockets created in this call from
     * the listener thread's select pool and close the socket.
     * Ignore status codes being returned from remove_desc.
     */
    if (i != num_socks)
    {
        for (created_sock_index = 0; 
             created_sock_index < i; 
             created_sock_index++)
        {
            rpc__network_remove_desc (sock_list[created_sock_index], &temp_status);
            RPC_SOCKET_CLOSE (sock_list[created_sock_index]);
        }
    }
    else
    {
        *status = rpc_s_ok;
    }
    RPC_MEM_FREE (sock_list, RPC_C_MEM_SOCKET_LIST);
}


/***********************************************************************/


/*
**++
**
**  ROUTINE NAME:       rpc__cn_network_init_desc
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  This routine will do a socket listen on the network descriptor
**  given.
**
**  INPUTS:
**
**      spawned         A boolean indicating that the network descriptor
**                      being added is the product of an automatic
**                      spawning sequence and is to be replaced with
**                      another descriptor.
**
**      pseq_id         The protocol sequence id of the network
**                      descriptor being added.
**
**      backlog         The socket connection queue length.

**  INPUTS/OUTPUTS:
**
**      desc            The network descriptor being added to the
**                      listen thread's pool.
**
**  OUTPUTS:
**
**      status          status returned
**                              rpc_s_ok
**                              rpc_s_cant_create_sock
**                              rpc_s_cant_bind_sock
**                              rpc_s_cant_listen_sock
**                              rpc_s_no_addrs
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      return          The private information block to be
**                      registered with the network descriptor.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL pointer_t rpc__cn_network_init_desc 
(
  rpc_socket_t            *desc,
  boolean32               spawned,
  rpc_protseq_id_t        pseq_id,
  unsigned32              backlog,
  unsigned32              *status
)
{
    rpc_socket_t        connected_desc = RPC_SOCKET_INVALID;
    rpc_addr_vector_p_t rpc_addr_vec;
    rpc_addr_p_t        rpc_addr;
    unsigned_char_t     *endpoint;
    rpc_socket_error_t  serr;
    unsigned32          temp_status;
    unsigned32          ssize, rsize;
    
    CODING_ERROR (status);
    
    RPC_CN_DBG_RTN_PRINTF (rpc__cn_network_init_desc);

    /*
     * If we are spawned then we have to do a number of things differently.
     *
     * Get another socket and bind it to the same address as
     * the one for the socket descriptor passed to us except we need a unique
     * endpoint.  We get the unique endpoint by calling the set_endpoint
     * routine with a NULL endpoint.  It will dynamically assign us a unique
     * endpoint.
     */
    if (spawned)
    {
        /*
         * Determine the address(es) the descriptor passed in is
         * associated with.
         */
        connected_desc = *desc;
        rpc__naf_desc_inq_addr (pseq_id, connected_desc, &rpc_addr_vec, status);
        if (*status != rpc_s_ok)
        {
            return (NULL);
        }
        if (rpc_addr_vec->len == 0)
        {
            rpc__naf_addr_vector_free (&rpc_addr_vec, status);
            *status = rpc_s_no_addrs;
            return (NULL);
        }
        rpc_addr = rpc_addr_vec->addrs[0];

        /*
         * Get a new (and unique) endpoint address by providing a
         * NULL endpoint to the addr_set_endpoint routine.
         */
        rpc__naf_addr_set_endpoint (NULL, &rpc_addr, status);
        if (*status != rpc_s_ok)
        {
            rpc__naf_addr_vector_free (&rpc_addr_vec, &temp_status);
            return (NULL);
        }

        /*
         * Get another socket. Note that we need to invalidate the descriptor
         * passed to us if we return with an error since the calling routine,
         * register_protseq(), will close the descriptor and we don't want the
         * original descriptor passed to us to be closed since it's being
         * listened to due to the rpc__cn_assoc_listen() call below.
         */
        serr = rpc__socket_open (pseq_id, NULL, desc);
        if (RPC_SOCKET_IS_ERR(serr))
        {
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                            ("(rpc__cn_network_init_desc) rpc__socket_open failed, error = %d\n",
                             RPC_SOCKET_ETOI(serr)));
            rpc__naf_addr_vector_free (&rpc_addr_vec, status);
            *desc = RPC_SOCKET_INVALID;
            *status = rpc_s_cant_create_sock;
            return (NULL);
        }

        /*
         * Set the socket to close itself if the process execs.
         */
        rpc__socket_set_close_on_exec (*desc);

        /*
         * Bind the descriptor to the address created above.
         */
        serr = rpc__socket_bind (*desc, rpc_addr);
        if (RPC_SOCKET_IS_ERR(serr))
        {
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                            ("(rpc__cn_network_init_desc) rpc__socket_bind failed, error = %d\n",
                             RPC_SOCKET_ETOI(serr)));
            rpc__naf_addr_vector_free (&rpc_addr_vec, status);
            *status = rpc_s_cant_bind_sock;
            return (NULL);
        }
        rpc__naf_addr_vector_free (&rpc_addr_vec, &temp_status);
    }

    /*
     * Setup the socket's send and receive buffering
     */
    serr = rpc__socket_set_bufs (*desc, 
                                 rpc_g_cn_socket_read_buffer,
                                 rpc_g_cn_socket_write_buffer,
                                 &ssize,
                                 &rsize);
    if (RPC_SOCKET_IS_ERR (serr))
    {
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                        ("(rpc__cn_network_init_desc) desc->%x Can't set socket bufs, error=%d\n",
                         *desc,
                         RPC_SOCKET_ETOI (serr)));
    }

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_BUFFS,
                    ("(rpc__cn_network_init_desc) desc->%x desired_sndbuf %u, desired_rcvbuf %u\n",
                     *desc, rpc_g_cn_socket_read_buffer, rpc_g_cn_socket_write_buffer));
    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_BUFFS,
                    ("(rpc__cn_network_init_desc) desc->%x actual sndbuf %lu, actual rcvbuf %lu\n",
                     *desc, ssize, rsize));

    if (rsize < RPC_C_ASSOC_MUST_RECV_FRAG_SIZE)
    {
        /*
         * rpc_m_recvbuf_toosmall
         * "(%s) Socket's maximum receive buffering is less than
         * NCA Connection Protocol minimum requirement"
         */
        RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s"),
	    rpc_svc_cn_errors,
	    svc_c_sev_fatal | svc_c_action_abort,
	    rpc_m_recvbuf_toosmall,
	    "rpc__cn_network_init_desc" ));
    }

    /*
     * Determine the endpoint of the socket.
     */
    rpc__cn_network_desc_inq_ep (*desc, pseq_id, &endpoint, status);
    if (*status != rpc_s_ok)
    {
        return (NULL);
    }


    if (spawned)
    {
        /*
         * Allocate an association control block. The association control
         * block which comes back will have all mutexes and condition
         * variables created. Also the receiver thread will be created.
         */
        (void) rpc__cn_assoc_listen (connected_desc, endpoint, status);
        if (*status != rpc_s_ok)
        {
            rpc_string_free (&endpoint, &temp_status);
            return (NULL);
        }
    }

    /*
     * Now do the listen on the bound socket.
     */
    serr = rpc__socket_listen (*desc, backlog);
    if (RPC_SOCKET_IS_ERR(serr))
    {
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                        ("(rpc__cn_network_init_desc) desc->%x rpc__socket_listen failed, error = %d\n", 
                         *desc, RPC_SOCKET_ETOI(serr)));
        rpc_string_free (&endpoint, &temp_status);
        *status = rpc_s_cant_listen_sock;
        return (NULL);
    }

    *status = rpc_s_ok;
    return ((pointer_t) endpoint);
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_network_inq_prot_vers
**
**  SCOPE:              PRIVATE - declared in cnnet.h
**
**  DESCRIPTION:
**      
**  Return the version number of the NCA CN protocol currently in use.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      prot_id         The NCA protocol identifier.
**	version_major	The NCA CN major version number.
**	version_minor	The NCA CN minor version number.
**      status          The result of the operation. One of:
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

PRIVATE void rpc__cn_network_inq_prot_vers 
(
  unsigned8               *prot_id,
  unsigned32		*version_major,
  unsigned32		*version_minor,
  unsigned32              *status
)
{
    CODING_ERROR (status);

    *prot_id = RPC_C_CN_PROTO_ID;
    *version_major = RPC_C_CN_PROTO_VERS;
    *version_minor = RPC_C_CN_PROTO_VERS_MINOR;
    *status = rpc_s_ok;
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_network_select_dispatch
**
**  SCOPE:              PRIVATE - declared in cnnet.h
**
**  DESCRIPTION:
**
**  This routine will accept the connection request which has
**  arrived on the network descriptor provided as input. An
**  association control block will then be allocated (and as a result a
**  receiver thread to go along with it). Once the association
**  control block has been set up with the new connection information
**  the receiver thread will be started.
**
**  INPUTS:
**
**      desc            The network descriptor on which the network
**                      event arrived.
**      priv_info       The private information block given by the
**                      protocol service when the network descriptor
**                      was intialized.
**      is_active       true indicates the application is waiting
**                      for network events on this socket.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**                      rpc_s_ok
**                      rpc_s_cannot_accept
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

PRIVATE void rpc__cn_network_select_dispatch 
(
  rpc_socket_t            desc,
  pointer_t               priv_info,
  boolean32               is_active,
  unsigned32              *st
)
{
    rpc_socket_t        newdesc;
    rpc_socket_error_t  serr;
    
    RPC_CN_DBG_RTN_PRINTF (rpc__cn_network_select_dispatch);
    CODING_ERROR(st);
    
    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("CN: desc->%x connection request received\n", desc));
    
    /*
     * We have been called by the network listener thread because a
     * network event occured on the descriptor passed in. Since the
     * only descriptors the network listener thread is listening on for us
     * are listening sockets we should just be able to accept the
     * connection. 
     */
    serr = rpc__socket_accept (desc, NULL, &newdesc);
    if (RPC_SOCKET_IS_ERR(serr))
    {
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                        ("(rpc__cn_network_select_dispatch) desc->%x rpc__socket_accept failed, error = %d\n",
                         desc, RPC_SOCKET_ETOI(serr)));
        
        *st = rpc_s_cannot_accept;
        dcethread_yield ();
    }
    else
    {
        if (RPC_DBG2 (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL))
        {
            rpc_protseq_id_t    protseq_id;
            rpc_addr_p_t        rpc_addr = NULL;
            unsigned_char_t     *netaddr = NULL, *endpoint = NULL;
            unsigned32          dbg_status;

            rpc__naf_desc_inq_protseq_id (newdesc,
                                          RPC_C_PROTOCOL_ID_NCACN,
                                          &protseq_id,
                                          &dbg_status); 

            if (dbg_status == rpc_s_ok)
            {
                rpc__naf_desc_inq_peer_addr (newdesc, 
                                             protseq_id,
                                             &rpc_addr,
                                             &dbg_status);
            }

            if (dbg_status == rpc_s_ok && rpc_addr != NULL)
            {
                rpc__naf_addr_inq_netaddr (rpc_addr,
                                           &netaddr,
                                           &dbg_status);

                rpc__naf_addr_inq_endpoint (rpc_addr,
                                            &endpoint,
                                            &dbg_status);
            }

            if (rpc_addr != NULL)
                rpc__naf_addr_free (&rpc_addr, &dbg_status);
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                ("CN: desc->%x connection request accepted/new connection on desc->%x from %s[%s]\n",
                 desc,
                 newdesc,
                 (netaddr != NULL) ? (char *)netaddr : "(null)",
                 (endpoint != NULL) ? (char *)endpoint : "(null)"));
            if (netaddr != NULL)
                rpc_string_free (&netaddr, &dbg_status);
            if (endpoint != NULL)
                rpc_string_free (&endpoint, &dbg_status);
        }

        if (!is_active)
        {
            /*
             * If the application is not currently listening for calls
             * then promptly close this socket and therefore the
             * connection.
             */
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                            ("CN: desc->%x socket not active ... being closed\n", newdesc));
            serr = RPC_SOCKET_CLOSE (newdesc);
        }
        else
        {
            /*
             * Set the socket to close itself if the process execs.
             */
            rpc__socket_set_close_on_exec (newdesc);
            
#ifndef NODELAY_BROKEN
            /*
             * The application is listening for new calls.
             * Set the packet no delay option on the connection.
             */
            rpc__naf_set_pkt_nodelay (newdesc, NULL, st);
            if (*st != rpc_s_ok)
            {
                /* 
                 * The set option failed. We'll continue anyway.
                 */
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                                ("(rpc__cn_network_select_dispatch) desc->%x rpc__naf_set_pkt_nodelay failed, error = %d\n",
                                 newdesc, *st));
            }
#endif
            
            /*
             * Set the keepalive socket option for this connection.
             */
            serr = rpc__socket_set_keepalive (newdesc);
            if (RPC_SOCKET_IS_ERR(serr))
            {
                /*
                 * The socket set keepalive option failed.
                 */
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                                ("(RPC_C_CN_DBG_ERRORS) desc->%x rpc__socket_set_keepalive failed failed, error = %d\n",
                                 newdesc, serr));
            }

            {
                struct timeval tmo;
                
                tmo.tv_sec = RPC_C_SOCKET_SERVER_RECV_TIMER;
                tmo.tv_usec = 0;

                serr = rpc__socket_set_rcvtimeo (newdesc, &tmo);
                if (RPC_SOCKET_IS_ERR(serr))
                {
                    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                                    ("(RPC_C_CN_DBG_ERRORS) desc->%x rpc__socket_set_rcvtimeo failed failed, error = %d\n",
                                     newdesc, serr));
                }
            }

            /*
             * Acquire the CN global mutex to prevent other threads from
             * running.
             */
            RPC_CN_LOCK ();

            /*
             * Allocate an association control block. The association
             * control block which comes back will have all mutexes and
             * condition variables created. Also the receiver thread
             * will be created.
             */
            (void) rpc__cn_assoc_listen (newdesc,
                                         (unsigned_char_t *) priv_info,
                                         st);
            if (*st != rpc_s_ok)
            {
                /* 
                 * The association listen failed. Close the connection.
                 */
                serr = RPC_SOCKET_CLOSE (newdesc);
            }

	    /*
	     * Release the CN global mutex to allow other threads to
	     * run.
	     */
	    RPC_CN_UNLOCK ();
	}
    }
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_network_req_connect
**
**  SCOPE:              PRIVATE - declared in cnnet.h
**
**  DESCRIPTION:
**
**  This routine will create a connection to the address given as an
**  argument.
**
**  INPUTS:
**
**      assoc           The association control block allocated for
**                      this connection.
**      rpc_addr        The RPC address to which the connection is
**                      to be made.
**      call_r          The call rep data structure to be linked to
**                      the association control block. This may be
**                      NULL if this association is being allocated
**                      on the server side. 
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**                      rpc_s_ok
**                      rpc_s_cant_create_sock
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

PRIVATE void rpc__cn_network_req_connect 
(
  rpc_addr_p_t            rpc_addr,
  rpc_cn_assoc_p_t        assoc,
  unsigned32              *st
)
{
    volatile rpc_socket_error_t  serr;
    volatile boolean32  retry_op;
    volatile boolean32           connect_completed;
    rpc_naf_id_t        naf_id;
    rpc_addr_p_t        temp_rpc_addr;
    unsigned32          temp_status;
    unsigned32          ssize, rsize;
    rpc_transport_info_p_t transport_info = NULL;

    //DO_NOT_CLOBBER(serr);
    //DO_NOT_CLOBBER(connect_completed);
	 
    RPC_CN_DBG_RTN_PRINTF (rpc__cn_network_req_connect);
    CODING_ERROR(st);
    
    /*
     * Now we need to create a connection to the server address
     * contained in the RPC address given. First create a socket to
     * do the connect on.
     */
    serr = rpc__socket_open (rpc_addr->rpc_protseq_id,
                             assoc->transport_info ? assoc->transport_info->handle : NULL,
                             (rpc_socket_t*) &assoc->cn_ctlblk.cn_sock);
    if (RPC_SOCKET_IS_ERR(serr))
    {
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                        ("(rpc__cn_network_req_connect) call_rep->%x assoc->%x desc->%x rpc__socket_open failed, error = %d\n",
                         assoc->call_rep,
                         assoc,
                         assoc->cn_ctlblk.cn_sock,
                         RPC_SOCKET_ETOI(serr)));
        
        *st = rpc_s_cant_create_sock;
    }
    else
    {
        /*
         * Setup the socket's send and receive buffering
         */
        serr = rpc__socket_set_bufs (assoc->cn_ctlblk.cn_sock,
                                     rpc_g_cn_socket_read_buffer,
                                     rpc_g_cn_socket_write_buffer,
                                     &ssize,
                                     &rsize);
        if (RPC_SOCKET_IS_ERR (serr))
        {
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                            ("(rpc__cn_network_req_connect) call_rep->%x assoc->%x desc->%x Can't set socket bufs, error=%d\n",
                             assoc->call_rep,
                             assoc,
                             assoc->cn_ctlblk.cn_sock, 
                             RPC_SOCKET_ETOI (serr)));
        }
        
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_BUFFS,
                        ("(rpc__cn_network_req_connect) desc->%x desired_sndbuf %u, desired_rcvbuf %u\n",
                         assoc->cn_ctlblk.cn_sock, rpc_g_cn_socket_read_buffer, rpc_g_cn_socket_write_buffer));
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_BUFFS,
                       ("(rpc__cn_network_req_connect) desc->%x actual sndbuf %lu, actual rcvbuf %lu\n",
                        assoc->cn_ctlblk.cn_sock, ssize, rsize));
        
        if (rsize < RPC_C_ASSOC_MUST_RECV_FRAG_SIZE)
        {
	    /*
	     * rpc_m_recvbuf_toosmall
	     * "(%s) Socket's maximum receive buffering is less than
	     * NCA Connection Protocol minimum requirement"
	     */
	    RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s"),
	        rpc_svc_cn_errors,
	        svc_c_sev_fatal | svc_c_action_abort,
	        rpc_m_recvbuf_toosmall,
	        "rpc__cn_network_req_connect" ));
        }

        /*
         * Set the socket to close itself if the process execs.
         */
        rpc__socket_set_close_on_exec(assoc->cn_ctlblk.cn_sock);

        /*
         * Allocate the temporarry rpc_addr to be passed to
         * rpc__socket_bind().
         */
        naf_id = rpc_g_protseq_id[rpc_addr->rpc_protseq_id].naf_id;
        rpc__naf_addr_alloc (rpc_addr->rpc_protseq_id,
                             naf_id,
                             (unsigned_char_p_t) NULL,  /* endpoint */
                             (unsigned_char_p_t) NULL,  /* network address */
                             (unsigned_char_p_t) NULL,  /* network option */
                             &temp_rpc_addr,
                             st);
        if (*st != rpc_s_ok)
        {
            serr = RPC_SOCKET_CLOSE (assoc->cn_ctlblk.cn_sock);
            if (RPC_SOCKET_IS_ERR(serr))
            {
                /*
                 * The socket close failed.
                 */
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                                ("(rpc__cn_network_req_connect) desc->%x RPC_SOCKET_CLOSE failed, error = %d\n", 
                                 assoc->cn_ctlblk.cn_sock, 
                                 RPC_SOCKET_ETOI(serr)));
            }
            return;
        }

        /*
         * Next, bind the socket to the RPC address.
         */
        serr = rpc__socket_bind (assoc->cn_ctlblk.cn_sock, temp_rpc_addr);
        rpc__naf_addr_free(&temp_rpc_addr, &temp_status);
        if (RPC_SOCKET_IS_ERR (serr))
        {
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                            ("(rpc__cn_network_req_connect) desc->%x rpc__socket_bind failed, error = %d\n",
                             assoc->cn_ctlblk.cn_sock, 
                             RPC_SOCKET_ETOI(serr)));
            *st = rpc_s_cant_bind_sock;
            
            /*
             * The bind request failed. Close the socket just created
             * and free the association control block.
             */
            serr = RPC_SOCKET_CLOSE (assoc->cn_ctlblk.cn_sock);
            if (RPC_SOCKET_IS_ERR(serr))
            {
                /*
                 * The socket close failed.
                 */
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                                ("(rpc__cn_network_req_connect) desc->%x RPC_SOCKET_CLOSE failed, error = %d\n", 
                                 assoc->cn_ctlblk.cn_sock, 
                                 RPC_SOCKET_ETOI(serr)));
            }
            return;
        }

        /*
         * Indicate that the connection is being attempted.
         */
        assoc->cn_ctlblk.cn_state = RPC_C_CN_CONNECTING;
        
        /*
         * Since the connect call will block we will release the CN
         * global mutex before the call and reqaquire it after the call.
         */
        RPC_CN_UNLOCK ();
        
        /*
         * Now actually do the connect to the server.
         */
        retry_op = true;

        /*
         * Poll for cancels while the connect is in progress.
         * Only exit the while loop on success, failure, or
         * when you've received a cancel and the cancel timer
         * has expired.
         * If it is just a cancel, set cancel_pending and 
         * start the cancel timer.
         */
        connect_completed = false;
        while (! connect_completed)
        {
            DCETHREAD_TRY
            {
#ifdef NON_CANCELLABLE_IO
                /*
                 * By POSIX definition dcethread_enableasync_throw is not a "cancel
                 * point" because it must return an error status and an errno.
                 * dcethread_enableasync_throw(1) will not deliver
                 * a pending cancel nor will the cancel be delivered asynchronously,
                 * thus the need for dcethread_checkinterrupt.
                 */
                dcethread_enableasync_throw(1);
	        dcethread_checkinterrupt();
#endif /* NON_CANCELLABLE_IO */
                rpc__socket_connect (assoc->cn_ctlblk.cn_sock, rpc_addr,
					    assoc, st);

#ifdef NON_CANCELLABLE_IO
                dcethread_enableasync_throw(0);
#endif /* NON_CANCELLABLE_IO */
                /*
                 * If we got here, then the connect was not cancelled;
                 * it has therefore completed either successfully or
                 * with serr set.
                 */
                connect_completed = true;
            }
            DCETHREAD_CATCH (dcethread_interrupt_e)
            {
#ifdef NON_CANCELLABLE_IO
                dcethread_enableasync_throw(0);
#endif /* NON_CANCELLABLE_IO */

                RPC_CN_LOCK ();
                /*
                 * Record the fact that a cancel has been
                 * detected. Also, start a timer if this is
                 * the first cancel detected.
                 */
                rpc__cn_call_local_cancel (assoc->call_rep,
                                           &retry_op,
                                           st);
                RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                                ("(rpc__cn_network_req_connect) call_rep->%x assoc->%x desc->%x cancel caught before association setup\n", 
                                 assoc->call_rep,
                                 assoc,
                                 assoc->cn_ctlblk.cn_sock));
                RPC_CN_UNLOCK ();
            }
            DCETHREAD_ENDTRY
            if (!retry_op)
            {
                RPC_CN_LOCK ();
                RPC_SOCKET_CLOSE (assoc->cn_ctlblk.cn_sock);
                return;
            }

        }

        /* 
         * The connect completed; see if it completed successfully.
         */
        RPC_CN_LOCK ();

        if (*st != rpc_s_ok)
        {
            RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                            ("(rpc__cn_network_req_connect) desc->%x rpc__socket_connect failed, error = %d\n",
                             assoc->cn_ctlblk.cn_sock, 
                             *st));
            
            /*
             * The connect request failed. Close the socket just created
             * and free the association control block.
             */
            serr = RPC_SOCKET_CLOSE (assoc->cn_ctlblk.cn_sock);
            if (RPC_SOCKET_IS_ERR(serr))
            {
                /*
                 * The socket close failed.
                 */
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                                ("(rpc__cn_network_req_connect) desc->%x RPC_SOCKET_CLOSE failed, error = %d\n", 
                                 assoc->cn_ctlblk.cn_sock, 
                                 RPC_SOCKET_ETOI(serr)));
            }
        }
        else
        {
#ifndef NODELAY_BROKEN
            /*
             * Set the packet no delay option on the connection.
             */
            rpc__naf_set_pkt_nodelay (assoc->cn_ctlblk.cn_sock,
                                      rpc_addr, 
                                      st);
            if (*st != rpc_s_ok)
            {
                /* 
                 * The set option failed. We'll continue anyway.
                 */
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                                ("(rpc__cn_network_req_connect) desc->%x rpc__naf_set_pkt_nodelay failed, error = %d\n",
                                 assoc->cn_ctlblk.cn_sock, 
                                 *st));
            }
#endif
            
            /*
             * Update the transport information by querying the socket
             */
            serr = rpc__socket_inq_transport_info(assoc->cn_ctlblk.cn_sock, &transport_info);
            if (RPC_SOCKET_IS_ERR(serr))
            {
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                                ("(rpc__cn_network_req_connect) desc->%x rpc__socket_inq_transport_info failed, error = %d\n",
                                 assoc->cn_ctlblk.cn_sock, 
                                 serr));
            }
            else
            {
                rpc__transport_info_release(assoc->transport_info);
                assoc->transport_info = transport_info;
            }

            /*
             * Indicate that there is a valid connection.
             */
            assoc->cn_ctlblk.cn_state = RPC_C_CN_OPEN;
            
            /*
             * A connection is now set up. Tell the receiver thread to begin
             * receiving on the connection.
             */
            if (assoc->cn_ctlblk.cn_rcvr_waiters)
            {
                RPC_COND_SIGNAL (assoc->cn_ctlblk.cn_rcvr_cond, 
                                 rpc_g_global_mutex);
            }
            
            /*
             * Set the keepalive socket option for this connection.
             */
            serr = rpc__socket_set_keepalive (assoc->cn_ctlblk.cn_sock);
            if (RPC_SOCKET_IS_ERR(serr))
            {
                /*
                 * The socket set keepalive option failed.
                 * The connection on the other side may have gone away.
                 * This is not a big deal because the client will
                 * find this out in the normal manner.
                 */
                RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                                ("(rpc__cn_network_req_connect) desc->%x rpc__socket_set_keepalive failed failed, error = %d\n",
                                 assoc->cn_ctlblk.cn_sock, serr));
            }

            {
                struct timeval tmo;
                
                tmo.tv_sec = RPC_C_SOCKET_CLIENT_RECV_TIMER;
                tmo.tv_usec = 0;

                serr = rpc__socket_set_rcvtimeo (assoc->cn_ctlblk.cn_sock, &tmo);
                if (RPC_SOCKET_IS_ERR(serr))
                {
                    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_ERRORS,
                                    ("(rpc__cn_network_req_connect) desc->%x rpc__socket_set_rcvtimeo failed failed, error = %d\n",
                                     assoc->cn_ctlblk.cn_sock, serr));
                }
            }

            /*
             * Everything went OK. Return a successful status code. 
             */
            *st = rpc_s_ok;
        }
    }
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_network_close_connect
**
**  SCOPE:              PRIVATE - declared in cnnet.h
**
**  DESCRIPTION:
**
**  This routine will close an open connection.
**
**  INPUTS:
**
**      assoc           The association control block to which this connection
**                      is attached. 
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The return status of this routine.
**                      rpc_s_ok
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

PRIVATE void rpc__cn_network_close_connect 
(
  rpc_cn_assoc_p_t        assoc,
  unsigned32              *st
)
{

    RPC_CN_DBG_RTN_PRINTF (rpc__cn_network_close_connect);
    CODING_ERROR (st);
    
    /*
     * Closing the connection will cause the receiver thread to stop
     * trying to receive on and and return to waiting on a condition
     * variable for a new connection. We don't want to close the
     * socket here since the a thread other than the receiver thread
     * for this connection may open a new socket, get the same
     * descriptor, and establish a connection. The receiver thread
     * would then never detect that the connection went away and two
     * receiver threads would be receiving off the same connection.
     */
    if (assoc->cn_ctlblk.cn_state == RPC_C_CN_OPEN)
    {
        dcethread_interrupt_throw (assoc->cn_ctlblk.cn_rcvr_thread_id);
    }
    else
    {
        RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                        ("(rpc__cn_network_close_connect) no cancel cn_state->%d\n",
                         assoc->cn_ctlblk.cn_state));
    }

    *st = rpc_s_ok;
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_network_mon
**
**  SCOPE:              PRIVATE - declared in cnnet.h
**
**  DESCRIPTION:
**
**  This routine notifies the connection protocol that at least one
**  connection to the address space identified in the binding rep
**  provided as input should be kept open.
**
**  INPUTS:
**
**      binding_r       The binding rep identifying the client
**                      process to be monitored.
**      client_h        The unique identifier of the client process
**                      which is really the id of an association group.
**      rundown         The routine to call if the communications
**                      link to the client process fails.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**                      rpc_s_ok
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

PRIVATE void rpc__cn_network_mon 
(
  rpc_binding_rep_p_t     binding_r ATTRIBUTE_UNUSED,
  rpc_client_handle_t     client_h,
  rpc_network_rundown_fn_t rundown,
  unsigned32              *st
)
{
    rpc_cn_assoc_grp_t  *assoc_grp;
    rpc_cn_local_id_t   grp_id;

    RPC_CN_DBG_RTN_PRINTF (rpc__cn_network_mon);
    CODING_ERROR(st);
    
    /*
     * Get the association group using the group id provided as a
     * client handle. 
     */
    grp_id.all = (unsigned long) client_h;
    grp_id = rpc__cn_assoc_grp_lkup_by_id (grp_id,
                                           RPC_C_CN_ASSOC_GRP_SERVER,
                                           binding_r->transport_info,
                                           st);

    /*
     * If the association group control block can't be found
     * return an error.
     */
    if (RPC_CN_LOCAL_ID_VALID (grp_id))
    {
        /*
         * Now we have the association group. Place the rundown function
         * in it and bump the reference count.
         */
        assoc_grp = RPC_CN_ASSOC_GRP (grp_id);
        assoc_grp->grp_liveness_mntr = rundown;
        assoc_grp->grp_refcnt++;
        *st = rpc_s_ok;
    }
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_network_stop_mon
**
**  SCOPE:              PRIVATE - declared in cnnet.h
**
**  DESCRIPTION:
**
**  This routine is called when it is no longer necessary for the
**  runtime to keep a connection open for the caller to the address
**  space identified by the provided client handle.
**
**  INPUTS:
**
**      binding_r       The binding rep identifying the client
**                      process to be monitored.
**      client_h        The unique identifier of the client process
**                      which is really an association group control block.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**                      rpc_s_ok
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

PRIVATE void rpc__cn_network_stop_mon 
(
  rpc_binding_rep_p_t     binding_r ATTRIBUTE_UNUSED,
  rpc_client_handle_t     client_h,
  unsigned32              *st
)
{
    rpc_cn_assoc_grp_t  *assoc_grp;
    rpc_cn_local_id_t   grp_id;
    
    CODING_ERROR(st);
    RPC_CN_DBG_RTN_PRINTF (rpc__cn_network_stop_mon);
    
    /*
     * Get the association group using the group id provided as a
     * client handle. 
     */
    grp_id.all = (unsigned long) client_h;
    grp_id = rpc__cn_assoc_grp_lkup_by_id (grp_id,
                                           RPC_C_CN_ASSOC_GRP_SERVER,
                                           binding_r->transport_info,
                                           st);

    /*
     * If the association group control block can't be found
     * return an error.
     */
    if (RPC_CN_LOCAL_ID_VALID (grp_id))
    {
        /*
         * Now we have the association group. Decrement the reference count.
         */
        assoc_grp = RPC_CN_ASSOC_GRP (grp_id);
        assoc_grp->grp_refcnt--;
        *st = rpc_s_ok;
    }
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_network_maint
**
**  SCOPE:              PRIVATE - declared in cnnet.h
**
**  DESCRIPTION:
**
**  This routine is called when a connection to the address space
**  represented by the binding rep should be kept alive. Since we are
**  assuming all our connections have the KEEPALIVE feature there is
**  nothing for us to do here except make sure we keep at least one
**  connection open.
**
**  INPUTS:
**
**      binding_r       The binding rep identifying the server
**                      process to which a communications link
**                      should be maintained.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**                      rpc_s_ok
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

PRIVATE void rpc__cn_network_maint 
(
  rpc_binding_rep_p_t     binding_r,
  unsigned32              *st
)
{
    rpc_cn_assoc_grp_t          *assoc_grp;
    rpc_cn_local_id_t           grp_id;

    CODING_ERROR(st);
    RPC_CN_DBG_RTN_PRINTF (rpc__cn_network_maint);
    
    /*
     * Get the association group using the group id contained in the
     * binding handle.
     */
    grp_id = rpc__cn_assoc_grp_lkup_by_id (((rpc_cn_binding_rep_t *)
                                            (binding_r))->grp_id,
                                           RPC_C_CN_ASSOC_GRP_CLIENT,
                                           binding_r->transport_info,
                                           st);  
    
    /*
     * If the association group control block can't be found
     * return an error.
     */
    if (RPC_CN_LOCAL_ID_VALID (grp_id))
    {
        /*
         * We now have the association group control block we've been
         * looking for. Increment the reference count.
         */
        assoc_grp = RPC_CN_ASSOC_GRP (grp_id);
        assoc_grp->grp_refcnt++;
        *st = rpc_s_ok;
    }
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_network_stop_maint
**
**  SCOPE:              PRIVATE - declared in cnnet.h
**
**  DESCRIPTION:
**
**  This routine is called when a connection to the address space
**  represented by the binding rep need no longer be kept alive.
**
**  INPUTS:
**
**      binding_r       The binding rep identifying the server
**                      process to which a communications link
**                      was being maintained.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**                      rpc_s_ok
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

PRIVATE void rpc__cn_network_stop_maint 
(
  rpc_binding_rep_p_t     binding_r,
  unsigned32              *st
)
{
    rpc_cn_assoc_grp_t  *assoc_grp;
    rpc_cn_local_id_t   grp_id;

    CODING_ERROR(st);
    RPC_CN_DBG_RTN_PRINTF (rpc__cn_network_stop_maint);
    
    /*
     * Get the association group using the group id contained in the
     * binding handle.
     */
    grp_id = rpc__cn_assoc_grp_lkup_by_id (((rpc_cn_binding_rep_t *)
                                            (binding_r))->grp_id,
                                           RPC_C_CN_ASSOC_GRP_CLIENT,
                                           binding_r->transport_info,
                                           st); 
    
    /*
     * If the association group control block can't be found
     * return an error.
     */
    if (RPC_CN_LOCAL_ID_VALID (grp_id))
    {
        /*
         * We now have the association group control block we've been
         * looking for. Decrement the reference count.
         */
        assoc_grp = RPC_CN_ASSOC_GRP (grp_id);
        assoc_grp->grp_refcnt--;
        *st = rpc_s_ok;
    }
}

/***********************************************************************/


/*
**++
**
**  ROUTINE NAME:       rpc__cn_network_connect_fail
**
**  SCOPE:              PRIVATE - declared in cnnet.h
**
**  DESCRIPTION:
**
**  This routine determine whether a given status code indicated a connect
**  request failed.
**
**  INPUTS:
**
**      st              The status code.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     boolean32
**
**      true            The status code is from a failed connection
**                      request false otherwise
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE boolean32 rpc__cn_network_connect_fail 
(
unsigned32              st
)
{
    switch ((int)st)
    {
        case rpc_s_cancel_timeout:
        case rpc_s_connect_timed_out:
        case rpc_s_connect_rejected:
        case rpc_s_network_unreachable:
        case rpc_s_connect_no_resources:
        case rpc_s_rem_network_shutdown:
        case rpc_s_too_many_rem_connects:
        case rpc_s_no_rem_endpoint:
        case rpc_s_rem_host_down:
        case rpc_s_host_unreachable:
        case rpc_s_access_control_info_inv:
        case rpc_s_loc_connect_aborted:
        case rpc_s_connect_closed_by_rem:
        case rpc_s_rem_host_crashed:
        case rpc_s_invalid_endpoint_format:
        case rpc_s_cannot_connect:
        {
            return (true);
        }
        
        default:
        {
            return (false);
        }
    }
}

/***********************************************************************/


/*
**++
**
**  ROUTINE NAME:       rpc__cn_network_serr_to_status
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  This routine converts a socket interface error into an RPC
**  status code.
**
**  INPUTS:
**
**      serr            The socket error.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The status code.
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

PRIVATE void rpc__cn_network_serr_to_status 
(
  rpc_socket_error_t      serr,
  unsigned32              *st
)
{
    switch (serr)
    {
        case RPC_C_SOCKET_ETIMEDOUT:
        *st = rpc_s_connect_timed_out;
        break;
        
        case RPC_C_SOCKET_ECONNREFUSED:
        *st = rpc_s_connect_rejected;
        break;
        
        case RPC_C_SOCKET_ENETUNREACH:
        *st = rpc_s_network_unreachable;
        break;
        
        case RPC_C_SOCKET_ENOSPC:
        *st = rpc_s_connect_no_resources;
        break;
        
        case RPC_C_SOCKET_ENETDOWN:
        *st = rpc_s_rem_network_shutdown;
        break;
        
#ifdef ETOOMANYREFS
        case RPC_C_SOCKET_ETOOMANYREFS:
        *st = rpc_s_too_many_rem_connects;
        break;
#endif        

        case RPC_C_SOCKET_ESRCH:
        *st = rpc_s_no_rem_endpoint;
        break;
      
#ifdef EHOSTDOWN  
        case RPC_C_SOCKET_EHOSTDOWN:
        *st = rpc_s_rem_host_down;
        break;
#endif        

        case RPC_C_SOCKET_EHOSTUNREACH:
        *st = rpc_s_host_unreachable;
        break;
        
        case RPC_C_SOCKET_EACCESS:
        *st = rpc_s_invalid_credentials;
        break;
        
        case RPC_C_SOCKET_ECONNABORTED:
        *st = rpc_s_loc_connect_aborted;
        break;
        
        case RPC_C_SOCKET_ECONNRESET:
        *st = rpc_s_connect_closed_by_rem;
        break;
        
        case RPC_C_SOCKET_ENETRESET:
        *st = rpc_s_rem_host_crashed;
        break;
        
        case RPC_C_SOCKET_ENOEXEC:
        *st = rpc_s_invalid_endpoint_format;
        break;

        case RPC_C_SOCKET_ETIME:
        *st = rpc_s_auth_skew;
        break;

        // Using this to pass through an RODC error that
        // requires reauthentication.
        case LW_STATUS_KDC_CERT_REVOKED:
        *st = rpc_s_auth_tkt_expired;
        break;
        
        case -1:
        *st = rpc_s_unknown_status_code;
        break;

	case ENOENT:
        *st = rpc_s_no_name_mapping;
	break;

        default:
        *st = rpc_s_cannot_connect;
        break;
    }
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_inq_sock_buffsize
**
**  SCOPE:              PRIVATE - declared in cnnet.h
**
**  DESCRIPTION:
**
**  This routine returns the CN global socket buffer sizes.
**  A zero value means the operating system default.
**
**  INPUTS:		none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      rsize           The receive buffer size (rpc_g_cn_socket_read_buffer)
**
**      ssize           The send buffer size (rpc_g_cn_socket_read_buffer)
**
**      st              The status code.
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

PRIVATE void
rpc__cn_inq_sock_buffsize(
	unsigned32	*rsize,
	unsigned32	*ssize,
	error_status_t	*st)
{
    *rsize = rpc_g_cn_socket_read_buffer;
    *ssize = rpc_g_cn_socket_write_buffer;
    *st = rpc_s_ok;
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_set_sock_buffsize
**
**  SCOPE:              PRIVATE - declared in cnnet.h
**
**  DESCRIPTION:
**
**  This routine sets the CN global socket buffer sizes.
**  A zero value for either buffer will use the OS default buffering.
**
**  INPUTS:
**
**       rsize          The socket receive buffer size
**
**       ssize          The socket send buffer size
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            
**
**      st              The status code.
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

PRIVATE void
rpc__cn_set_sock_buffsize(
	unsigned32	rsize,
	unsigned32	ssize,
	error_status_t	*st)
{
    rpc_g_cn_socket_read_buffer = rsize;
    rpc_g_cn_socket_write_buffer = ssize;
    *st = rpc_s_ok;
}


/***********************************************************************/
/*
**++
**
**  ROUTINE NAME:       rpc__cn_network_close
**
**  SCOPE:              PRIVATE - declared in cnnet.h
**
**  DESCRIPTION:
**
**  This routine is called when a connection to the address space
**  represented by the binding rep should be closed.
**
**  INPUTS:
**
**      binding_r       The binding rep identifying the server
**                      process to which a communications link
**                      was being maintained.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      st              The return status of this routine.
**                      rpc_s_ok
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

PRIVATE void rpc__cn_network_close 
(
  rpc_binding_rep_p_t     binding_r,
  unsigned32              *st
)
{
    rpc_cn_assoc_grp_t  *assoc_grp;
    rpc_cn_local_id_t   grp_id;

    CODING_ERROR(st);
    RPC_CN_DBG_RTN_PRINTF (rpc__cn_network_close);
    
    /*
     * Get the association group using the group id contained in the
     * binding handle.
     */
    grp_id = rpc__cn_assoc_grp_lkup_by_id (((rpc_cn_binding_rep_t *)
                                            (binding_r))->grp_id,
                                           RPC_C_CN_ASSOC_GRP_CLIENT,
                                           binding_r->transport_info,
                                           st); 
    
    /*
     * If the association group control block can't be found
     * return an error.
     */
    if (RPC_CN_LOCAL_ID_VALID (grp_id))
    {
        rpc_cn_assoc_t *assoc;

        /*
         * We now have the association group control block we've been
         * looking for. Free it.
         */

        assoc_grp = RPC_CN_ASSOC_GRP (grp_id);
        assert (assoc_grp != NULL);
        assoc = (rpc_cn_assoc_t *)assoc_grp->grp_assoc_list.next;
        rpc__cn_assoc_abort (assoc, st);
        *st = rpc_s_ok;
    }
}

/***********************************************************************/

