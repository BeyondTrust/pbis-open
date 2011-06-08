#include <commonp.h>
#include <com.h>
#include <comprot.h>
#include <comnaf.h>
#include <comp.h>
#include <comsoc.h>
#include <comsoc_bsd.h>
#include <errno.h>

rpc_socket_error_t
rpc__socket_open_basic (
    rpc_naf_id_t  naf,
    rpc_network_if_id_t net_if,
    rpc_network_protocol_id_t  net_prot,
    rpc_socket_basic_t *sock
    )
{
    return rpc__bsd_socket_open_basic(naf, net_if, net_prot, sock);
}

rpc_socket_error_t
rpc__socket_close_basic (
    rpc_socket_basic_t sock
    )
{
    return rpc__bsd_socket_close_basic(sock);
}


/* Open a socket */
rpc_socket_error_t
rpc__socket_open (
    rpc_protseq_id_t pseq_id,
    rpc_transport_info_handle_t info,
    rpc_socket_t* sock
    )
{
    int err = RPC_C_SOCKET_OK;

    *sock = calloc(1, sizeof(**sock));

    if (!*sock)
    {
        err = ENOMEM;
        goto error;
    }

    (*sock)->vtbl = rpc_g_protseq_id[pseq_id].socket_vtbl;
    (*sock)->pseq_id = pseq_id;

    err = (*sock)->vtbl->socket_construct(*sock, pseq_id, info);
    if (err)
    {
        goto error;
    }

done:

    return err;

error:

    if (*sock)
    {
        free(*sock);
        *sock = NULL;
    }

    goto done;
}

/* Close the socket */
rpc_socket_error_t
rpc__socket_close (
    rpc_socket_t sock
    )
{
    rpc_socket_error_t err = sock->vtbl->socket_destruct(sock);

    if (!err)
    {
        free(sock);
    }

    return err;
}

/* Bind the socket */
rpc_socket_error_t
rpc__socket_bind (
    rpc_socket_t sock,
    rpc_addr_p_t addr
    )
{
    return sock->vtbl->socket_bind(sock, addr);
}

/* Connect the socket */
void
rpc__socket_connect (
    rpc_socket_t sock,
    rpc_addr_p_t addr,
    rpc_cn_assoc_t* assoc,
    unsigned32 *st
    )
{
    sock->vtbl->socket_connect(sock, addr, assoc, st);
}

/* Accept a connection on a listen socket */
rpc_socket_error_t
rpc__socket_accept (
    rpc_socket_t sock,
    rpc_addr_p_t addr,
    rpc_socket_t* newsock)
{
    return sock->vtbl->socket_accept(sock, addr, newsock);
}

/* Begin listening for connections on a socket */
rpc_socket_error_t
rpc__socket_listen (
    rpc_socket_t sock,
    int backlog
    )
{
    return sock->vtbl->socket_listen(sock, backlog);
}

/* Send a message over the socket */
rpc_socket_error_t
rpc__socket_sendmsg (
    rpc_socket_t sock,
    rpc_socket_iovec_p_t iov,
    int iov_len,
    rpc_addr_p_t addr,
    int * cc
    )
{
    return sock->vtbl->socket_sendmsg(sock, iov, iov_len, addr, cc);
}

/* Receive data from the socket */
rpc_socket_error_t
rpc__socket_recvfrom (
    rpc_socket_t sock,
    byte_p_t buf,
    int len,
    rpc_addr_p_t from,
    int *cc
    )
{
    return sock->vtbl->socket_recvfrom(sock, buf, len, from, cc);
}

/* Receive a message from the socket */
rpc_socket_error_t
rpc__socket_recvmsg (
    rpc_socket_t sock,
    rpc_socket_iovec_p_t iov,
    int iov_len,
    rpc_addr_p_t addr,
    int * cc)
{
    return sock->vtbl->socket_recvmsg(sock, iov, iov_len, addr, cc);
}

/* Inquire local socket endpoint */
rpc_socket_error_t
rpc__socket_inq_endpoint (
    rpc_socket_t sock,
    rpc_addr_p_t addr
    )
{
    return sock->vtbl->socket_inq_endpoint(sock, addr);
}

/* Enabled broadcasting on datagram socket */
rpc_socket_error_t
rpc__socket_set_broadcast (
    rpc_socket_t sock
    )
{
    return sock->vtbl->socket_set_broadcast(sock);
}

/* Set socket buffer sizes */
rpc_socket_error_t
rpc__socket_set_bufs (
    rpc_socket_t sock,
    unsigned32 txsize,
    unsigned32 rxsize,
    unsigned32* ntxsize,
    unsigned32* nrxsize
    )
{
    return sock->vtbl->socket_set_bufs(sock, txsize, rxsize, ntxsize, nrxsize);
}

/* Set socket as non-blocking */
rpc_socket_error_t
rpc__socket_set_nbio (
    rpc_socket_t sock
    )
{
    return sock->vtbl->socket_set_nbio(sock);
}

/* Set socket to close automatically when the process loads a new executable */
rpc_socket_error_t
rpc__socket_set_close_on_exec (
    rpc_socket_t sock
    )
{
    return sock->vtbl->socket_set_close_on_exec(sock);
}

/* Get peer address */
rpc_socket_error_t
rpc__socket_getpeername (
    rpc_socket_t sock,
    rpc_addr_p_t addr
    )
{
    return sock->vtbl->socket_getpeername(sock, addr);
}

/* Get socket network interface */
rpc_socket_error_t
rpc__socket_get_if_id (
    rpc_socket_t sock,
    rpc_network_if_id_t* network_if_id
    )
{
    return sock->vtbl->socket_get_if_id(sock, network_if_id);
}

/* Set socket as keep-alive */
rpc_socket_error_t
rpc__socket_set_keepalive (
    rpc_socket_t sock
    )
{
    return sock->vtbl->socket_set_keepalive(sock);
}

/* Block for the specified period of time until the socket is ready for writing */
rpc_socket_error_t
rpc__socket_nowriteblock_wait (
    rpc_socket_t sock,
    struct timeval *tmo
    )
{
    return sock->vtbl->socket_nowriteblock_wait(sock, tmo);
}

/* Set socket receive timeout */
rpc_socket_error_t
rpc__socket_set_rcvtimeo (
    rpc_socket_t sock,
    struct timeval *tmo
    )
{
    return sock->vtbl->socket_set_rcvtimeo(sock, tmo);
}

/* Get peer credentials for local connection */
rpc_socket_error_t
rpc__socket_getpeereid (
    rpc_socket_t sock,
    uid_t* uid,
    gid_t* gid
    )
{
    return sock->vtbl->socket_getpeereid(sock, uid, gid);
}

int
rpc__socket_get_select_desc(
    rpc_socket_t sock
    )
{
    return sock->vtbl->socket_get_select_desc(sock);
}

rpc_socket_error_t
rpc__socket_enum_ifaces (
    rpc_socket_t sock,
    rpc_socket_enum_iface_fn_p_t efun,
    rpc_addr_vector_p_t *rpc_addr_vec,
    rpc_addr_vector_p_t *netmask_addr_vec,
    rpc_addr_vector_p_t *broadcast_addr_vec
    )
{
    return sock->vtbl->socket_enum_ifaces(sock, efun, rpc_addr_vec, netmask_addr_vec, broadcast_addr_vec);
}

rpc_socket_error_t
rpc__socket_inq_transport_info(
    rpc_socket_t sock,
    rpc_transport_info_p_t* info
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_transport_info_handle_t handle = NULL;

    serr = sock->vtbl->socket_inq_transport_info(sock, &handle);
    if (serr)
    {
        goto error;
    }

    if (handle != NULL)
    {
        serr = rpc__transport_info_create(sock->pseq_id, handle, info);
        if (serr)
        {
            goto error;
        }
    }
    else
    {
        *info = NULL;
    }

error:

    if (serr)
    {
        *info = NULL;
        if (handle != NULL)
        {
            sock->vtbl->transport_info_free(handle);
        }
    }

    return serr;
}

rpc_socket_error_t
rpc__transport_info_create(
    rpc_protseq_id_t protseq,
    rpc_transport_info_handle_t handle,
    rpc_transport_info_p_t* info
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_transport_info_p_t my_info = NULL;

    my_info = malloc(sizeof(*my_info));
    if (!my_info)
    {
        serr = ENOMEM;
        goto error;
    }

    my_info->refcount = 1;
    my_info->protseq = protseq;
    my_info->handle = handle;

    *info = my_info;

error:

    return serr;
}

void
rpc__transport_info_retain(
    rpc_transport_info_p_t info
    )
{
    if (info)
    {
        info->refcount++;
    }
}

void
rpc__transport_info_release(
    rpc_transport_info_p_t info
    )
{
    if (info && --info->refcount == 0)
    {
        rpc_g_protseq_id[info->protseq].socket_vtbl->transport_info_free(info->handle);
        free(info);
    }
}

PRIVATE boolean
rpc__transport_info_equal(
    rpc_transport_info_p_t info1,
    rpc_transport_info_p_t info2
    )
{
    return
        (info1 == NULL && info2 == NULL) ||
        (info1 != NULL && info2 == NULL &&
         rpc_g_protseq_id[info1->protseq].socket_vtbl->transport_info_equal(info1->handle, NULL)) ||
        (info1 != NULL && info2 != NULL && info1->protseq == info2->protseq &&
         rpc_g_protseq_id[info1->protseq].socket_vtbl->transport_info_equal(info1->handle, info2->handle));
}

void
rpc_binding_set_transport_info(
    rpc_binding_handle_t         binding_handle,
    rpc_transport_info_handle_t  info_handle,
    unsigned32                   *st
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_binding_rep_p_t binding_r = (rpc_binding_rep_p_t) binding_handle;

    CODING_ERROR (st);
    RPC_VERIFY_INIT ();

    serr = rpc__transport_info_create(
        binding_r->rpc_addr->rpc_protseq_id,
        info_handle,
        &binding_r->transport_info);

    if (serr)
    {
        *st = rpc_s_no_memory;
        goto error;
    }

    (*rpc_g_protocol_id[binding_r->protocol_id].binding_epv->binding_changed) (binding_r, st);

    if (*st)
    {
        goto error;
    }

    *st = rpc_s_ok;

error:

    return;
}

void
rpc_binding_inq_transport_info(
    rpc_binding_handle_t         binding_handle,
    rpc_transport_info_handle_t  *info,
    unsigned32                   *st
    )
{
    rpc_binding_rep_p_t binding_r = (rpc_binding_rep_p_t) binding_handle;

    CODING_ERROR (st);
    RPC_VERIFY_INIT ();

    if (binding_r && binding_r->transport_info)
    {
        *info = binding_r->transport_info->handle;
    }
    else
    {
        *info = NULL;
    }

    *st = rpc_s_ok;
}

void
rpc_binding_inq_prot_seq(
    rpc_binding_handle_t  binding_handle,
    unsigned32            *prot_seq,
    unsigned32            *st
    )
{
    rpc_binding_rep_p_t binding_r = (rpc_binding_rep_p_t) binding_handle;

    CODING_ERROR (st);
    RPC_VERIFY_INIT ();

    if (binding_r && binding_r->transport_info)
    {
        *prot_seq = (unsigned32)binding_r->transport_info->protseq;
    }
    else if (binding_r)
    {
        *prot_seq = (unsigned32)binding_r->rpc_addr->rpc_protseq_id;
    }
    else
    {
        *prot_seq = (unsigned32)RPC_C_INVALID_PROTSEQ_ID;
    }

    *st = rpc_s_ok;
}

void
rpc_binding_inq_access_token_caller(
    rpc_binding_handle_t binding_handle,
    rpc_access_token_p_t* token,
    unsigned32* st
    )
{
    rpc_binding_rep_p_t binding_r = (rpc_binding_rep_p_t) binding_handle;
    int err = 0;

    CODING_ERROR (st);
    RPC_VERIFY_INIT ();

    *token = NULL;

    if (binding_r)
    {
        if (binding_r->auth_info)
        {
            rpc_g_authn_protocol_id[binding_r->auth_info->authn_protocol].epv->
                inq_access_token(binding_r->auth_info, token, st);
            if (*st != rpc_s_ok)
            {
                goto error;
            }
        }

        if (!*token && binding_r->transport_info)
        {
            err = rpc_g_protseq_id[binding_r->transport_info->protseq].socket_vtbl->
                transport_inq_access_token(
                    binding_r->transport_info->handle,
                    token);
            if (err)
            {
                *st = rpc_s_binding_has_no_auth;
                goto error;
            }
            
        }
    }

    if (!*token)
    {
        *st = rpc_s_binding_has_no_auth;
        goto error;
    }
    else
    {
        *st = rpc_s_ok;
    }

error:

    return;
}
