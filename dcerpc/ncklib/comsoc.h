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
#ifndef _COMSOC_H
#define _COMSOC_H	1
/*
**
**  NAME:
**
**      comsoc.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  The internal network "socket" object interface.  A very thin veneer
**  over the BSD socket abstraction interfaces.  This makes life a little
**  easier when porting to different environments.
**  
**  All operations return a standard error value of type
**  rpc_socket_error_t, operate on socket handles of type rpc_socket_t
**  and socket addresses of type rpc_socket_addr_t.  These are the types
**  that one should use when coding.
**  
**  Note that there is a distinction between local runtime internal
**  representations of socket addresses and architected (on-the-wire)
**  representations used by location services.  This interface specifies
**  the local runtime internal representation.
**  
**  Operations that return an error value always set the value
**  appropriately.  A value other than rpc_c_socket_ok indicates failure;
**  the values of additional output parameters are undefined.  Other
**  error values are system dependent.
**
**
*/


/*
 * Include platform-specific socket definitions
 */

#include <dce/dce.h>

/*
 * The maximum number of iov elements which can be sent through
 * sendmsg is MSG_IOVLEN-1.
 */
#define RPC_C_MAX_IOVEC_LEN ( MSG_MAXIOVLEN - 1)

/*
 * The maximum number of connections which can be queued on a socket.
 */
#define RPC_C_LISTEN_BACKLOG SOMAXCONN

/*
 * Changing anything below will affect other portions of the runtime.
 */

typedef struct iovec rpc_socket_iovec_t;
typedef struct iovec *rpc_socket_iovec_p_t;
typedef int rpc_socket_error_t;

/*
 * bkoropoff // Likewise
 *
 * New types that allow for socket handles that are more abstract than just a file descriptor
 */
    
typedef boolean (*rpc_socket_enum_iface_fn_p_t) (
    rpc_socket_t sock,
    rpc_addr_p_t ip_addr,
    rpc_addr_p_t netmask_addr,
    rpc_addr_p_t broadcast_addr
    );

typedef struct rpc_cn_assoc_s_t rpc_cn_assoc_t, *rpc_cn_assoc_p_t;

typedef struct rpc_socket_vtbl_s
{
    /* Used when opening a socket */
    rpc_socket_error_t
    (*socket_construct) (
        rpc_socket_t sock,
        rpc_protseq_id_t pseq_id,
        rpc_transport_info_handle_t info
        );
    /* Used when closing a socket */
    rpc_socket_error_t
    (*socket_destruct) (
        rpc_socket_t sock
        );
    /* Bind the socket */
    rpc_socket_error_t
    (*socket_bind) (
        rpc_socket_t sock,
        rpc_addr_p_t addr
        );
    /* Connect the socket */
    void
    (*socket_connect) (
        rpc_socket_t sock,
        rpc_addr_p_t addr,
        rpc_cn_assoc_t* assoc,
        unsigned32 *st
        );
    /* Accept a connection on a listen socket */
    rpc_socket_error_t
    (*socket_accept) (
        rpc_socket_t sock,
        rpc_addr_p_t addr,
        rpc_socket_t* newsock
        );
    /* Begin listening for connections on a socket */
    rpc_socket_error_t
    (*socket_listen) (
        rpc_socket_t sock,
        int backlog
        );
    /* Send a message over the socket */
    rpc_socket_error_t
    (*socket_sendmsg) (
        rpc_socket_t sock,
        rpc_socket_iovec_p_t iov,
        int iov_len,
        rpc_addr_p_t addr,
        int * cc
        );
    /* Receive data from the socket */
    rpc_socket_error_t
    (*socket_recvfrom) (
        rpc_socket_t sock,
        byte_p_t buf,
        int len,
        rpc_addr_p_t from,
        int *cc
        );
    /* Receive a message from the socket */
    rpc_socket_error_t
    (*socket_recvmsg) (
        rpc_socket_t sock,
        rpc_socket_iovec_p_t iov,
        int iov_len,
        rpc_addr_p_t addr,
        int * cc
        );
    /* Inquire local socket endpoint */
    rpc_socket_error_t
    (*socket_inq_endpoint) (
        rpc_socket_t sock,
        rpc_addr_p_t addr
        );
    /* Enabled broadcasting on datagram socket */
    rpc_socket_error_t
    (*socket_set_broadcast) (
        rpc_socket_t sock
        );
    /* Set socket buffer sizes */
    rpc_socket_error_t
    (*socket_set_bufs) (
        rpc_socket_t sock,
        unsigned32 txsize,
        unsigned32 rxsize,
        unsigned32* ntxsize,
        unsigned32* nrxsize
        );
    /* Set socket as non-blocking */
    rpc_socket_error_t
    (*socket_set_nbio) (
        rpc_socket_t sock
        );
    /* Set socket to close automatically when the process loads a new executable */
    rpc_socket_error_t
    (*socket_set_close_on_exec) (
        rpc_socket_t sock
        );
    /* Get peer address */
    rpc_socket_error_t
    (*socket_getpeername) (
        rpc_socket_t sock,
        rpc_addr_p_t addr
        );
    /* Get socket network interface */
    rpc_socket_error_t
    (*socket_get_if_id) (
        rpc_socket_t sock,
        rpc_network_if_id_t* network_if_id
        );
    /* Set socket as keep-alive */
    rpc_socket_error_t
    (*socket_set_keepalive) (
        rpc_socket_t sock
        );
    /* Block for the specified period of time until the socket is ready for writing */
    rpc_socket_error_t
    (*socket_nowriteblock_wait) (
        rpc_socket_t sock,
        struct timeval *tmo
        );
    /* Set socket receive timeout */
    rpc_socket_error_t
    (*socket_set_rcvtimeo) (
        rpc_socket_t sock,
        struct timeval *tmo
        );
    /* Get peer credentials for local connection */
    rpc_socket_error_t
    (*socket_getpeereid) (
        rpc_socket_t sock,
        uid_t* uid,
        gid_t* gid
        );
    /* Get a file descriptor suitable for using select() to decide when data is readable */
    int
    (*socket_get_select_desc) (
        rpc_socket_t sock
        );
    /* Enumerate available network interfaces for the socket */
    rpc_socket_error_t
    (*socket_enum_ifaces) (
        rpc_socket_t sock,
        rpc_socket_enum_iface_fn_p_t efun,
        rpc_addr_vector_p_t *rpc_addr_vec,
        rpc_addr_vector_p_t *netmask_addr_vec,
        rpc_addr_vector_p_t *broadcast_addr_vec
        );
    /* Inquire transport-level info */
    rpc_socket_error_t
    (*socket_inq_transport_info) (
        rpc_socket_t sock,
        rpc_transport_info_handle_t* info
        );
    /* Free transport-level info */
    void
    (*transport_info_free) (
        rpc_transport_info_handle_t info
        );

    boolean
    (*transport_info_equal) (
        rpc_transport_info_handle_t info1,
        rpc_transport_info_handle_t info2
        );

    /* Inquire for access token */
    rpc_socket_error_t
    (*transport_inq_access_token) (
        rpc_transport_info_handle_t info,
        rpc_access_token_p_t* token
        );
} rpc_socket_vtbl_t, *rpc_socket_vtbl_p_t;
    
typedef struct rpc_socket_handle_s
{
    rpc_socket_vtbl_t* vtbl;
    rpc_protseq_id_t pseq_id;
    union
    {
        ssize_t word;
        void* pointer;
    } data;
} rpc_socket_handle_t, *rpc_socket_handle_p_t;
    
typedef int rpc_socket_basic_t;

#include <comsoc_sys.h>
#include <comnaf.h>
#include <ipnaf.h>
#include <cnp.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * R P C _ _ S O C K E T _ O P E N
 *
 * Create a new socket for the specified Protocol Sequence.
 * The new socket has blocking IO semantics.
 *
 * (see BSD UNIX socket(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_open (
        rpc_protseq_id_t pseq_id,
        rpc_transport_info_handle_t info,
        rpc_socket_t * sock
    );


/*
 * R P C _ _ S O C K E T _ O P E N _ B A S I C
 *
 * A special version of socket_open that is used *only* by 
 * the low level initialization code when it is trying to 
 * determine what network services are supported by the host OS.
 */

PRIVATE rpc_socket_error_t rpc__socket_open_basic (
        rpc_naf_id_t  /*naf*/,
        rpc_network_if_id_t  /*net_if*/,
        rpc_network_protocol_id_t  /*net_prot*/,
        rpc_socket_basic_t * /*sock*/
    );


/*
 * R P C _ _ S O C K E T _ C L O S E
 *
 * Close (destroy) a socket.
 *
 * (see BSD UNIX close(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_close (
        rpc_socket_t /*sock*/
    );


/*
 * R P C _ _ S O C K E T _ C L O S E _ B A S I C
 *
 * Close (destroy) a basic socket.
 *
 * (see BSD UNIX close(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_close_basic (
        rpc_socket_basic_t /*sock*/
    );

/*
 * R P C _ _ S O C K E T _ B I N D
 *
 * Bind a socket to a specified local address.
 *
 * (see BSD UNIX bind(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_bind (
        rpc_socket_t  /*sock*/,
        rpc_addr_p_t /*addr*/
    );

/*
 * R P C _ _ S O C K E T _ C O N N E C T
 *
 * Connect a socket to a specified peer's address.
 * This is used only by Connection oriented Protocol Services.
 *
 * (see BSD UNIX connect(2)).
 */

PRIVATE void rpc__socket_connect (
        rpc_socket_t  /*sock*/,
        rpc_addr_p_t /*addr*/,
	rpc_cn_assoc_t* /*assoc*/,
        unsigned32* /*st*/
    );


/*
 * R P C _ _ S O C K E T _ A C C E P T
 *
 * Accept a connection on a socket, creating a new socket for the new
 * connection.  A rpc_addr_t appropriate for the NAF corresponding to
 * this socket must be provided.  addr.len must set to the actual size
 * of addr.sa.  This operation fills in addr.sa and sets addr.len to
 * the new size of the field.  This is used only by Connection oriented
 * Protocol Services.
 * 
 * (see BSD UNIX accept(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_accept (
        rpc_socket_t  /*sock*/,
        rpc_addr_p_t  /*addr*/,
        rpc_socket_t * /*newsock*/
    );


/*
 * R P C _ _ S O C K E T _ L I S T E N
 *
 * Listen for a connection on a socket.
 * This is used only by Connection oriented Protocol Services.
 *
 * (see BSD UNIX listen(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_listen (
        rpc_socket_t /*sock*/,
        int /*backlog*/
    );


/*
 * R P C _ _ S O C K E T _ S E N D M S G
 *
 * Send a message over a given socket.  An error code as well as the
 * actual number of bytes sent are returned.
 *
 * (see BSD UNIX sendmsg(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_sendmsg (
        rpc_socket_t  /*sock*/,
        rpc_socket_iovec_p_t  /*iov*/,   /* array of bufs of data to send */
        int  /*iov_len*/,        /* number of bufs */
        rpc_addr_p_t  /*addr*/,  /* addr of receiver */
        int * /*cc*/             /* returned number of bytes actually sent */
    );


/*
 * R P C _ _ S O C K E T _ R E C V F R O M
 *
 * Recieve the next buffer worth of information from a socket.  A
 * rpc_addr_t appropriate for the NAF corresponding to this socket must
 * be provided.  addr.len must set to the actual size of addr.sa.  This
 * operation fills in addr.sa and sets addr.len to the new size of the
 * field.  An error status as well as the actual number of bytes received
 * are also returned.
 * 
 * (see BSD UNIX recvfrom(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_recvfrom (
        rpc_socket_t  /*sock*/,
        byte_p_t  /*buf*/,       /* buf for rcvd data */
        int  /*len*/,            /* len of above buf */
        rpc_addr_p_t  /*from*/,  /* addr of sender */
        int * /*cc*/             /* returned number of bytes actually rcvd */
    );


/*
 * R P C _ _ S O C K E T _ R E C V M S G
 *
 * Receive a message over a given socket.  A rpc_addr_t appropriate for
 * the NAF corresponding to this socket must be provided.  addr.len must
 * set to the actual size of addr.sa.  This operation fills in addr.sa
 * and sets addr.len to the new size of the field.  An error code as
 * well as the actual number of bytes received are also returned.
 * 
 * (see BSD UNIX recvmsg(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_recvmsg (
        rpc_socket_t  /*sock*/,
        rpc_socket_iovec_p_t  /*iov*/,   /* array of bufs for rcvd data */
        int  /*iov_len*/,        /* number of bufs */
        rpc_addr_p_t  /*addr*/,  /* addr of sender */
        int * /*cc*/             /* returned number of bytes actually rcvd */
    );


/*
 * R P C _ _ S O C K E T _ I N Q _ A D D R
 *
 * Return the local address associated with a socket.  A rpc_addr_t
 * appropriate for the NAF corresponding to this socket must be provided.
 * addr.len must set to the actual size of addr.sa.  This operation fills
 * in addr.sa and sets addr.len to the new size of the field.
 *
 * !!! NOTE: You should use rpc__naf_desc_inq_addr() !!!
 *
 * This routine is indended for use only by the internal routine:
 * rpc__naf_desc_inq_addr().  rpc__socket_inq_endpoint() only has the
 * functionality of BSD UNIX getsockname() which doesn't (at least not
 * on all systems) return the local network portion of a socket's address.
 * rpc__naf_desc_inq_addr() returns the complete address for a socket.
 *
 * (see BSD UNIX getsockname(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_inq_endpoint (
        rpc_socket_t  /*sock*/,
        rpc_addr_p_t /*addr*/
    );

/*
 * R P C _ _ S O C K E T _ I N Q _ P E E R _ E N D P O I N T
 *
 * Return the local address associated with a socket.  A rpc_addr_t
 * appropriate for the NAF corresponding to this socket must be provided.
 * addr.len must set to the actual size of addr.sa.  This operation fills
 * in addr.sa and sets addr.len to the new size of the field.
 *
 * !!! NOTE: You should use rpc__naf_desc_inq_addr() !!!
 *
 * This routine is indended for use only by the internal routine:
 * rpc__naf_desc_inq_addr().  rpc__socket_inq_endpoint() only has the
 * functionality of BSD UNIX getsockname() which doesn't (at least not
 * on all systems) return the local network portion of a socket's address.
 * rpc__naf_desc_inq_addr() returns the complete address for a socket.
 *
 * (see BSD UNIX getsockname(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_inq_peer_endpoint (
        rpc_socket_t  /*sock*/,
        rpc_addr_p_t /*addr*/
    );


/*
 * R P C _ _ S O C K E T _ S E T _ B R O A D C A S T
 *
 * Enable broadcasting for the socket (as best it can).
 * Used only by Datagram based Protocol Services.
 */

PRIVATE rpc_socket_error_t rpc__socket_set_broadcast (
        rpc_socket_t /*sock*/
    );


/*
 * R P C _ _ S O C K E T _ S E T _ B U F S
 *
 * Set the socket's send and receive buffer sizes and return the new
 * values.
 * 
 * (similar to BSD UNIX setsockopt()).
 */

PRIVATE rpc_socket_error_t rpc__socket_set_bufs (
        rpc_socket_t  /*sock*/, 
        unsigned32  /*txsize*/, 
        unsigned32  /*rxsize*/, 
        unsigned32 * /*ntxsize*/, 
        unsigned32 * /*nrxsize*/
    );


/*
 * R P C _ _ S O C K E T _ S E T _ N B I O
 *
 * Set a socket to non-blocking mode.
 *
 * (see BSD UNIX fcntl(sock, F_SETFL, O_NDELAY))
 */

PRIVATE rpc_socket_error_t rpc__socket_set_nbio (
        rpc_socket_t /*sock*/
    );


/*
 * R P C _ _ S O C K E T _ S E T _ C L O S E _ O N _ E X E C
 *
 * Set a socket to a mode whereby it is not inherited by a spawned process
 * executing some new image. This is possibly a no-op on some systems.
 *
 * (see BSD UNIX fcntl(sock, F_SETFD, 1))
 */

PRIVATE rpc_socket_error_t rpc__socket_set_close_on_exec (
        rpc_socket_t /*sock*/
    );

/*
 * R P C _ _ S O C K E T _ G E T P E E R N A M E
 *
 * Get name of connected peer.
 * This is used only by Connection oriented Protocol Services.
 *
 * (see BSD UNIX getpeername(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_getpeername (
        rpc_socket_t  /*sock*/,
        rpc_addr_p_t /*addr*/
    );

/*
 * R P C _ _ S O C K E T _ G E T _ I F _ I D
 *
 * Get socket network interface id (socket type).
 *
 * (see BSD UNIX getsockopt(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_get_if_id (
        rpc_socket_t         /*sock*/,
        rpc_network_if_id_t * /*network_if_id*/
    );

/*
 * R P C _ _ S O C K E T _ S E T _ K E E P A L I V E.
 *
 * Set keepalive option for connection.
 * Used only by Connection based Protocol Services.
 *
 * (see BSD UNIX setsockopt(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_set_keepalive (
        rpc_socket_t        /*sock*/
    );

/*
 * R P C _ _ S O C K E T _ N O W R I T E B L O C K _ W A I T
 *
 * Wait until the a write on the socket should succede without
 * blocking.  If tmo is NULL, the wait is unbounded, otherwise
 * tmo specifies the max time to wait. rpc_c_socket_etimedout
 * if a timeout occurs.  This operation in not cancellable.
 */

PRIVATE rpc_socket_error_t rpc__socket_nowriteblock_wait (
        rpc_socket_t  /*sock*/,
        struct timeval * /*tmo*/
    );

/*
 * R P C _ _ S O C K E T _ S E T _ R C V T I M E O
 *
 * Set receive timeout option for connection.
 * Used only by Connection based Protocol Services.
 *
 * (see BSD UNIX setsockopt(2)).
 */

PRIVATE rpc_socket_error_t rpc__socket_set_rcvtimeo (
        rpc_socket_t,       /*sock*/
        struct timeval *    /*tmo*/
    );

/*
 * R P C _ _ S O C K E T _ G E T P E E R E I D
 *
 * Get UNIX domain socket peer credentials
 */

PRIVATE rpc_socket_error_t rpc__socket_getpeereid (
	rpc_socket_t,	    /*sock*/
	uid_t *,
	gid_t *
    );

PRIVATE int
rpc__socket_get_select_desc(
    rpc_socket_t sock
    );

PRIVATE rpc_socket_error_t
rpc__socket_enum_ifaces (
    rpc_socket_t sock,
    rpc_socket_enum_iface_fn_p_t efun,
    rpc_addr_vector_p_t *rpc_addr_vec,
    rpc_addr_vector_p_t *netmask_addr_vec,
    rpc_addr_vector_p_t *broadcast_addr_vec
    );

PRIVATE rpc_socket_error_t
rpc__socket_inq_transport_info(
    rpc_socket_t sock,
    rpc_transport_info_p_t* info
    );

PRIVATE rpc_socket_error_t
rpc__transport_info_create(
    rpc_protseq_id_t protseq,
    rpc_transport_info_handle_t handle,
    rpc_transport_info_p_t* info
    );

PRIVATE void
rpc__transport_info_retain(
    rpc_transport_info_p_t info
    );

PRIVATE void
rpc__transport_info_release(
    rpc_transport_info_p_t info
    );

PRIVATE boolean
rpc__transport_info_equal(
    rpc_transport_info_p_t info1,
    rpc_transport_info_p_t info2
    );

/*
 * Public error constants and functions for comparing errors.
 * The _ETOI_ (error-to-int) function converts a socket error to a simple
 * integer value that can be used in error mesages.
 */

#define RPC_C_SOCKET_OK           0             /* a successful error value */
#define RPC_C_SOCKET_EWOULDBLOCK  EWOULDBLOCK   /* operation would block */
#define RPC_C_SOCKET_EINTR        EINTR         /* operation was interrupted */
#define RPC_C_SOCKET_EIO          EIO           /* I/O error */
#define RPC_C_SOCKET_EADDRINUSE   EADDRINUSE    /* address was in use (see bind) */
#define RPC_C_SOCKET_ECONNRESET   ECONNRESET    /* connection reset by peer */
#define RPC_C_SOCKET_ETIMEDOUT    ETIMEDOUT     /* connection request timed out*/
#define RPC_C_SOCKET_ECONNREFUSED ECONNREFUSED  /* connection request refused */
#define RPC_C_SOCKET_ENOTSOCK     ENOTSOCK      /* descriptor was not a socket */
#define RPC_C_SOCKET_ENETUNREACH  ENETUNREACH   /* network is unreachable*/
#define RPC_C_SOCKET_ENOSPC       ENOSPC        /* no local or remote resources */
#define RPC_C_SOCKET_ENETDOWN     ENETDOWN      /* network is down */
#define RPC_C_SOCKET_ETOOMANYREFS ETOOMANYREFS  /* too many remote connections */
#define RPC_C_SOCKET_ESRCH        ESRCH         /* remote endpoint not found */
#define RPC_C_SOCKET_EHOSTDOWN    EHOSTDOWN     /* remote host is down */
#define RPC_C_SOCKET_EHOSTUNREACH EHOSTUNREACH  /* remote host is unreachable */
#define RPC_C_SOCKET_ECONNABORTED ECONNABORTED  /* local host aborted connect */
#define RPC_C_SOCKET_ECONNRESET   ECONNRESET    /* remote host reset connection */
#define RPC_C_SOCKET_ENETRESET    ENETRESET     /* remote host crashed */
#define RPC_C_SOCKET_ENOEXEC      ENOEXEC       /* invalid endpoint format for remote */
#define RPC_C_SOCKET_EACCESS      EACCES        /* access control information */
                                                /* invalid at remote node */
#define RPC_C_SOCKET_EPIPE        EPIPE         /* a write on a pipe */
                                                /* or socket for which there */
                                                /* is no process to */
                                                /* read the data. */
#define RPC_C_SOCKET_EAGAIN       EAGAIN        /* no more processes */
#define RPC_C_SOCKET_EALREADY     EALREADY      /* operation already */
                                                /* in progress */
#define RPC_C_SOCKET_EDEADLK      EDEADLK       /* resource deadlock */
                                                /* would occur */
#define RPC_C_SOCKET_EINPROGRESS  EINPROGRESS   /* operation now in */
                                                /* progress */
#define RPC_C_SOCKET_EISCONN      EISCONN       /* socket is already */
                                                /* connected */
#define RPC_C_SOCKET_ENOTSUP      ENOTSUP       /* operation not supported */
#ifdef ETIME
#define RPC_C_SOCKET_ETIME        ETIME         /* A time skew occurred */
#else
#define RPC_C_SOCKET_ETIME        (ELAST + 1)   /* A time skew occurred */
#endif

/*
 * A macro to determine if an socket error can be recovered from by
 * retrying.
 */
#define RPC_SOCKET_ERR_IS_BLOCKING(s) \
    ((s == RPC_C_SOCKET_EAGAIN) || (s == RPC_C_SOCKET_EWOULDBLOCK) || (s == RPC_C_SOCKET_EINPROGRESS) || \
     (s == RPC_C_SOCKET_EALREADY) || (s == RPC_C_SOCKET_EDEADLK))

#define RPC_SOCKET_ERR_EQ(serr, e)  ((serr) == e)

#define RPC_SOCKET_IS_ERR(serr)     (! RPC_SOCKET_ERR_EQ(serr, RPC_C_SOCKET_OK))

#define RPC_SOCKET_ETOI(serr)       (serr)

static inline int
__rpc_socket_close(rpc_socket_t* sock)
{
    int __err = rpc__socket_close(*sock);
    *sock = NULL;
    return __err;
}

#define RPC_SOCKET_CLOSE(s) __rpc_socket_close((rpc_socket_t*)(void*)&(s))

#endif /* _COMSOC_H */
