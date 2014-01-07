/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        connection.h
 *
 * Abstract:
 *
 *        Connection API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_CONNECTION_H__
#define __LWMSG_CONNECTION_H__

#include <lwmsg/assoc.h>
#include <lwmsg/security.h>
#include <unistd.h>

/**
 * @file connection.h
 * @brief Connection-oriented assocations
 */

/**
 * @defgroup connect Connections
 * @ingroup public
 * @brief Connection-oriented associations over UNIX sockets
 *
 * Connections provide a concrete implementation of the
 * association abstraction based on the BSD socket layer,
 * allowing messages to be exchanged over UNIX domain sockets.
 *
 * Connections over UNIX domain sockets support special features.
 * Access to the identity of the connected peer is available through
 * #lwmsg_session_get_peer_security_token(), which returns a security token
 * of type "local".  Use #lwmsg_local_token_get_eid() on the token
 * to query the effective uid and gid of the peer.
 *
 * Additionally, messages sent over a UNIX domain socket connection may
 * contain embedded file descriptors which will be mirrored
 * into the peer process through an underlying mechanism such
 * as SCM_RIGHTS.  This has applications ranging from implementing
 * priveledge separation to exchanging shared memory segments
 * or establishing side channels for efficient bulk data transfer.
 *
 * To use embedded file descriptors, use the #LWMSG_FD or
 * #LWMSG_MEMBER_FD() macros in your type specification.
 */

/* @{ */

/**
 * @brief Connection mode
 *
 * Describes the mode of a connection (local versus remote)
 */
typedef enum LWMsgConnectionMode
{
    /**
     * No connection mode set
     * @hideinitializer
     */
    LWMSG_CONNECTION_MODE_NONE = 0,
    /**
     * Local connection
     * @hideinitializer
     */
    LWMSG_CONNECTION_MODE_LOCAL = LWMSG_ENDPOINT_LOCAL,
    /**
     * Anonymous socket pair connection
     * @hideinitializer
     */
    LWMSG_CONNECTION_MODE_PAIR = LWMSG_ENDPOINT_PAIR
} LWMsgConnectionMode;

/**
 * @brief Create a new connection
 *
 * Creates a new connection which speaks the specified protocol.
 * A new connection begins in an unconnected state and must be
 * bound to a socket or endpoint before it can continue.
 *
 * @param[in] context an optional context
 * @param[in] prot the protocol supported by the new connection
 * @param[out] out_assoc the created connection
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_connection_new(
    const LWMsgContext* context,
    LWMsgProtocol* prot,
    LWMsgAssoc** out_assoc
    );

/**
 * @brief Set maximum packet size
 *
 * Sets the maximum packet size supported by the connection.
 * Larger packets sizes use more memory but make sending large
 * messsages more efficient.  The choice of packet size might also affect
 * fragmentation behavior or efficiency of the underlying transport mechanism.
 * 
 * The actual packet size used will be the smaller of the specified size
 * and the preferred packet size of the peer.  The packet size cannot be
 * changed after the connection is bound.
 *
 * @param[in,out] assoc the connection
 * @param[in] size the desired packet size
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the packet size cannot be changed in the connection's current state}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_connection_set_packet_size(
    LWMsgAssoc* assoc,
    size_t size
    );

/**
 * @brief Attach connection to existing socket
 *
 * Attaches the specified connection to an existing file descriptor,
 * which must be a valid socket which matches the specified mode.
 *
 * This function does not by itself cause connection activity
 * and thus is guaranteed not to block.
 *
 * @param[in,out] assoc the connection
 * @param[in] mode the connection mode
 * @param[in] fd the file descriptor
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_PARAMETER, the file descriptor is invalid}
 * @lwmsg_code{INVALID_STATE, a file descriptor or endpoint is already set}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_connection_set_fd(
    LWMsgAssoc* assoc,
    LWMsgConnectionMode mode,
    int fd
    );

/**
 * @brief Attach connection to named endpoint
 *
 * Attaches the specified connection to a named endpoint.
 * For a #LWMSG_CONNECTION_MODE_LOCAL connection, this is the
 * path of the domain socket file.  For a #LWMSG_CONNECTION_MODE_REMOTE
 * connection, this is the address/hostname and port of the remote host.
 *
 * This function does not by itself cause a connection to be establish
 * and thus is guaranteed not to block.
 *
 * @param[in,out] assoc the connection
 * @param[in] mode the connection mode
 * @param[in] endpoint the named endpoint
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_PARAMETER, the endpoint is invalid}
 * @lwmsg_code{INVALID_STATE, a file descriptor or endpoint is already set}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_connection_set_endpoint(
    LWMsgAssoc* assoc,
    LWMsgConnectionMode mode,
    const char* endpoint
    );

/**
 * @brief Retrieve information from a "local" security token
 *
 * Retrives the effective user ID and effective group ID from a
 * local access token (that is, a token for which #lwmsg_security_token_get_type()
 * returns "local")
 *
 * @param[in] token the local security token
 * @param[out] out_euid the effective user id
 * @param[out] out_egid the effective group id
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_PARAMETER, the provided token was not of the correct type}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_local_token_get_eid(
    LWMsgSecurityToken* token,
    uid_t *out_euid,
    gid_t *out_egid
    );

/**
 * @brief Retrieve client pid from a "local" security token
 *
 * Retrives the pid of the program connecting to the server based off a local
 * access token (that is, a token for which #lwmsg_security_token_get_type()
 * returns "local")
 *
 * @param[in] token the local security token
 * @param[out] out_pid the client pid. If the platform does not support querying the
 * pid of processes connecting over a UNIX domain socket, this will be -1.
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_PARAMETER, the provided token was not of the correct type}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_local_token_get_pid(
    LWMsgSecurityToken* token,
    pid_t *out_pid
    );

#ifndef DOXYGEN
extern LWMsgTypeClass lwmsg_fd_type_class;
#endif

/**
 * @brief Define a file descriptor
 *
 * Defines a file descriptor type within a type specification.
 * The corresponding C type should be <tt>int</tt>.
 * The value must either be a valid file descriptor
 * greater than 0, or a value less than or equal to 0
 * (indicating that no descriptor should be transmitted).
 *
 * The reason that 0 is not considered a valid file descriptor is
 * so that allocating zeroed memory is sufficient to initialize
 * any file descriptors contained in the block.
 *
 * If you must transmit file descriptor 0, first use dup() to acquire
 * a new descriptor with a non-zero value and transmit that.
 * @hideinitializer
 */
#define LWMSG_FD LWMSG_CUSTOM(int, &lwmsg_fd_type_class, NULL)

/**
 * @brief Define a file descriptor as a member
 *
 * Defines a file descriptor as a member of a containing type.
 * The corresponding C type should be <tt>int</tt>.
 *
 * @param type the containing type
 * @param field the field of the containing type
 * @hideinitializer
 */
#define LWMSG_MEMBER_FD(type, field) LWMSG_MEMBER_CUSTOM(type, field, &lwmsg_fd_type_class, NULL)

/* @} */

#endif
