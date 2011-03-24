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
 *        peer.h
 *
 * Abstract:
 *
 *        Multi-threaded peer API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_PEER_H__
#define __LWMSG_PEER_H__

#include <lwmsg/status.h>
#include <lwmsg/protocol.h>
#include <lwmsg/time.h>
#include <lwmsg/assoc.h>
#include <lwmsg/message.h>
#include <lwmsg/call.h>

/**
 * @file peer.h
 * @brief Peer API
 */

/**
 * @defgroup peer Peers
 * @ingroup public
 * @brief Nexus for incoming and outgoing calls
 *
 * An #LWMsgPeer struct acts as a nexus for both incoming and outgoing calls,
 * combining client and server functionality into a single abstraction.  A peer
 * speaks a single protocol (#LWMsgProtocol) and may do any or all of the following
 * simultaneously.
 *
 * - Listen for incoming calls on any number of endpoints: #lwmsg_peer_start_listen()
 * - Connect to a single endpoint on another peer to make outgoing calls: #lwmsg_peer_connect()
 *
 * To use an #LWMsgPeer as a call server:
 *
 * -# Create and set up an #LWMsgProtocol and optionally an #LWMsgContext
      to customize memory management and logging.
 * -# Create a peer with #lwmsg_peer_new().
 * -# Register one or more tables of dispatch functions to handle incoming calls
 *    with #lwmsg_peer_add_dispatch_spec().
 * -# Register one or more listening endpoints with #lwmsg_peer_add_listen_endpoint().
 * -# Start listening for incoming requests with #lwmsg_peer_start_listen().
 *
 * To use an #LWMsgPeer as a call client:
 *
 * -# Create and set up an #LWMsgProtocol and optionally an #LWMsgContext
 *    to customize memory management and logging.
 * -# Create a peer with #lwmsg_peer_new().
 * -# If the protocol makes use of callbacks, register one or more tables of dispatch
 *    functions to handle incoming callbacks with #lwmsg_peer_add_dispatch_spec().
 * -# Register one or more endpoints to connect to with #lwmsg_peer_add_connect_endpoint().
 *    The first one which can be succefully connected to is the one which will be used.
 * -# Connect with #lwmsg_peer_connect() to get an #LWMsgSession.
 * -# Acquire a call handle with #lwmsg_session_acquire_call() and call with #lwmsg_call_dispatch().
 */

/*@{*/

#ifndef DOXYGEN
typedef enum LWMsgDispatchType
{
    LWMSG_DISPATCH_TYPE_END = 0,
    LWMSG_DISPATCH_TYPE_BLOCK = 2,
    LWMSG_DISPATCH_TYPE_NONBLOCK = 3
} LWMsgDispatchType;
#endif

/**
 * @brief Dispatch specification
 *
 * This structure defines a table of dispatch functions
 * to handle incoming messages in a peer.  It should
 * be constructed as a statically-initialized array
 * using #LWMSG_DISPATCH() and #LWMSG_DISPATCH_END macros.
 */
typedef struct LWMsgDispatchSpec
#ifndef DOXYGEN
{
    LWMsgDispatchType type;
    LWMsgTag tag;
    void* data;
}
#endif
const LWMsgDispatchSpec;

/**
 * @brief Define blocking message handler
 *
 * Defines a message handler function for the given message tag
 * within a dispatch specification.  The provided callback function
 * may block indefinitely in the process of servicing the request.
 * It may also opt to complete the request asynchronously with
 * #lwmsg_call_pend() and #lwmsg_call_complete().
 *
 * @param tag the message tag
 * @param func an #LWMsgPeerCallFunction
 * @hideinitializer
 */
#define LWMSG_DISPATCH_BLOCK(tag, func) \
    {LWMSG_DISPATCH_TYPE_BLOCK, (tag), (void*) (LWMsgPeerCallFunction) (func)}

/**
 * @brief Define non-blocking message handler
 *
 * Defines a message handler function for the given message tag
 * within a dispatch specification.  The provided callback function
 * must not block indefinitely in the process of servicing the request.
 * If the request cannot be completed immediately, it must complete
 * it asynchronously.
 *
 * @param tag the message tag
 * @param func an #LWMsgPeerCallFunction
 * @hideinitializer
 */
#define LWMSG_DISPATCH_NONBLOCK(tag, func) \
    {LWMSG_DISPATCH_TYPE_NONBLOCK, (tag), (void*) (LWMsgPeerCallFunction) (func)}

/**
 * @brief Terminate dispatch table
 *
 * This macro is used in dispatch table construction to
 * mark the end of the table
 * @hideinitializer
 */
#define LWMSG_DISPATCH_END {LWMSG_DISPATCH_TYPE_END, -1, NULL}

/**
 * @brief Peer structure
 *
 * An opaque structure from which calls can be answered
 * or dispatched.
 */
typedef struct LWMsgPeer LWMsgPeer;

/**
 * @brief Call handler function
 *
 * A callback function which handles an incoming call request.  The function
 * may complete the call immediately by filling in the out params structure
 * and returning #LWMSG_STATUS_SUCCESS, or asynchronously by invoking
 * #lwmsg_call_pend() on the call handle, returning #LWMSG_STATUS_PENDING,
 * and completing the call later with #lwmsg_call_complete().  Returning
 * any other status code will cause the client call to fail with the same
 * status.
 *
 * The contents of the in params structure is defined only for the duration
 * of the function call and must not be referenced after the function returns
 * or modified during the course of the call.  The call handle is also only
 * valid while the call is in progress and should not be referenced after
 * completion.
 *
 * Data inserted into the out params structure must be allocated with the
 * same memory manager as the #LWMsgPeer which dispatched the call.
 * By default, this is plain malloc().  The caller assumes ownership of all
 * such memory and the responsibility of freeing it.
 * 
 * @param[in,out] call the call handle
 * @param[in] in the input parameters
 * @param[out] out the output parameters
 * @param[in] data the data pointer set by #lwmsg_peer_set_dispatch_data()
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{PENDING, the request will be completed asynchronously}
 * @lwmsg_etc{call-specific failure}
 * @lwmsg_endstatus
 * @see #lwmsg_context_set_memory_functions() and #lwmsg_peer_new() for
 * customizing the peer's memory manager.
 */
typedef
LWMsgStatus
(*LWMsgPeerCallFunction) (
    LWMsgCall* call,
    const LWMsgParams* in,
    LWMsgParams* out,
    void* data
    );

/**
 * @brief Exception handler function
 *
 * A callback function which is invoked when an unexpected error occurs
 * in the process of handling incoming calls.
 *
 * @param[in] peer the peer handle
 * @param[in] status the status code of the error
 * @param[in] data a user data pointer
 */
typedef
void
(*LWMsgPeerExceptionFunction) (
    LWMsgPeer* peer,
    LWMsgStatus status,
    void* data
    );

/**
 * @brief Call trace function
 *
 * A callback function which allows tracing when a call begins or
 * ends.
 *
 * @param[in] call the call handle
 * @param[in] params the input or output parameters of the call
 * @param[in] data a user data pointer
 */
typedef
void
(*LWMsgPeerTraceFunction) (
    LWMsgCall* call,
    const LWMsgParams* params,
    LWMsgStatus status,
    void* data
    );

/**
 * @brief Create a new peer object
 *
 * Creates a new peer object
 *
 * @param[in] context an optional context
 * @param[in] protocol a protocol object which describes the protocol spoken by the peer
 * @param[out] peer the created peer object
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{INVALID_PARAMETER, protocol was <tt>NULL</tt>}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_new(
    const LWMsgContext* context,
    LWMsgProtocol* protocol,
    LWMsgPeer** peer
    );

/**
 * @brief Delete a peer object
 *
 * Deletes a peer object.
 *
 * @warning Attempting to delete a peer which has outstanding
 * outgoing calls has undefined behavior.  Attempting to delete
 * a peer which is listening for incoming calls will block
 * until all outstanding incoming calls can be cancelled.
 *
 * @param[in,out] peer the peer object to delete
 */
void
lwmsg_peer_delete(
    LWMsgPeer* peer
    );

/**
 * @brief Set timeout
 *
 * Sets the specified timeout to the specified value.
 * See #lwmsg_assoc_set_timeout() for more information.
 *
 * @param[in,out] peer the peer object
 * @param[in] type the type of timeout to set
 * @param[in] value the value, or NULL for no timeout
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{UNSUPPORTED, the specified timeout type is not supported}
 * @lwmsg_code{INVALID_PARAMETER, the timeout was invalid}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_set_timeout(
    LWMsgPeer* peer,
    LWMsgTimeout type,
    LWMsgTime* value
    );

/**
 * @brief Set maximum number of simultaneous incoming associations
 *
 * Sets the maximum numbers of incoming associations which the peer will track
 * simultaneously.  Associations beyond this will wait until a slot becomes
 * available.
 *
 * @param[in,out] peer the peer object
 * @param[in] max_clients the maximum number of simultaneous associations to support
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the peer is already listening}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_set_max_listen_clients(
    LWMsgPeer* peer,
    unsigned int max_clients
    );

/**
 * @brief Set maximum number of backlogged associations
 *
 * Sets the maximum numbers of pending associations which the peer will keep
 * waiting until a client slot becomes available.  Pending associations beyond
 * this value will be rejected outright.
 *
 * @param[in,out] peer the peer object
 * @param[in] max_backlog the maximum number of clients to queue
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the peer is already active}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_set_max_listen_backlog(
    LWMsgPeer* peer,
    unsigned int max_backlog
    );

/**
 * @brief Add a message dispatch specification
 *
 * Adds a set of message dispatch functions to the specified
 * peer object.
 *
 * @param[in,out] peer the peer object
 * @param[in] spec the dispatch specification
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_add_dispatch_spec(
    LWMsgPeer* peer,
    LWMsgDispatchSpec* spec
    );

/**
 * @brief Add listen socket
 *
 * Adds a socket on which the peer will accept incoming associations.
 * This function must be passed a valid socket descriptor
 * that matches the specified mode and is already listening (has had
 * listen() called on it).  The peer will assume ownership of this
 * fd.
 *
 * @param[in,out] peer the peer object
 * @param[in] type the endpoint type
 * @param[in] fd the socket on which to listen
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the peer is already active}
 * @lwmsg_code{INVALID_PARAMETER, the file descriptor was invalid}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_add_listen_fd(
    LWMsgPeer* peer,
    LWMsgEndpointType type,
    int fd
    );
    
/**
 * @brief Add listening endpoint
 *
 * Adds an endpoint on which the peer will listen for connections.
 *
 * @param[in,out] peer the peer object
 * @param[in] type the type of endpoint
 * @param[in] endpoint the endpoint path on which to listen
 * @param[in] permissions permissions for the endpoint (only applicable to local endpoints)
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the peer is already active}
 * @lwmsg_code{INVALID_PARAMETER, the endpoint was invalid}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_add_listen_endpoint(
    LWMsgPeer* peer,
    LWMsgEndpointType type,
    const char* endpoint,
    mode_t permissions
    );

/**
 * @brief Set session construct and destruct functions for incoming clients
 *
 * Sets functions which will be called when a client is accepted into a session
 * for the first time.  The constructor function may set up a session context
 * which the destructor function should clean up when the session is terminated.
 *
 * @param[in,out] peer the peer handle
 * @param[in] construct a session constructor function
 * @param[in] destruct a session destructor function
 * @param[in] data a user data pointer to be passed to both functions
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the peer is already active}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_set_listen_session_functions(
    LWMsgPeer* peer,
    LWMsgSessionConstructFunction construct,
    LWMsgSessionDestructFunction destruct,
    void* data
    );

/**
 * @brief Set dispatch data pointer
 *
 * Sets the user data pointer which is passed to dispatch functions.
 * This function may only be used while the peer is inactive.
 *
 * @param[in,out] peer the peer object
 * @param[in] data the data pointer
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the peer is already active}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_set_dispatch_data(
    LWMsgPeer* peer,
    void* data
    );

/**
 * @brief Get dispatch data pointer
 *
 * Gets the user data pointer which is passed to dispatch functions.
 * If no pointer was explicitly set, the value defaults to NULL.
 *
 * @param[in] peer the peer object
 * @return the data pointer
 */
void*
lwmsg_peer_get_dispatch_data(
    LWMsgPeer* peer
    );

/**
 * @brief Start listening for incoming associations
 *
 * Starts listening for incoming associations from other peers on all
 * endpoints registered with #lwmsg_peer_add_listen_endpoint().  This
 * function returns once all endpoints are ready to accept incoming
 * associations.
 *
 * @param[in,out] peer the peer object
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_start_listen(
    LWMsgPeer* peer
    );

/**
 * @brief Stop listening for incoming assocations
 *
 * Stops the specified peer accepting new associations and aggressively
 * terminates any existing associations, including cancelling all outstanding
 * incoming calls.  This function returns once all incoming associations have
 * been terminated.
 *
 * @param[in,out] peer the peer object
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_stop_listen(
    LWMsgPeer* peer
    );

/**
 * @brief Set exception handler
 *
 * Sets a callback function which will be invoked when an error occurs
 * during the course of servicing incoming or outgoing calls.  The function may
 * take appropriate action depending on the error, such as logging a warning or
 * instructing the main application thread to shut down.
 *
 * @warning Do not call #lwmsg_peer_stop_listen() from an exception handler
 *
 * @param[in,out] peer the peer handle
 * @param[in] except the handler function
 * @param[in] except_data a user data pointer to pass to the handler function
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_set_exception_function(
    LWMsgPeer* peer,
    LWMsgPeerExceptionFunction except,
    void* except_data
    );

/**
 * @brief Set trace functions
 *
 * Sets functions which will be invoked whenever a call begins or ends.
 * To determine the direction of a call, use #lwmsg_call_get_direction().
 * To store extra data on the call handle, use #lwmsg_call_set_user_data().
 * This mechanism can be use for logging, statistics gathering, etc.
 *
 * @param[in,out] peer the peer handle
 * @param[in] begin trace begin function
 * @param[in] end trace end function
 * @param[in] data user data pointer to pass to trace functions
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the peer is already active}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_set_trace_functions(
    LWMsgPeer* peer,
    LWMsgPeerTraceFunction begin,
    LWMsgPeerTraceFunction end,
    void* data
    );

/**
 * @brief Add connection endpoint
 *
 * Adds an endpoint which will be used when establishing an outgoing association
 * with #lwmsg_peer_connect().
 *
 * @param[in,out] peer the peer handle
 * @param[in] type the type of endpoint
 * @param[in] endpoint the endpoint path
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_add_connect_endpoint(
    LWMsgPeer* peer,
    LWMsgEndpointType type,
    const char* endpoint
    );

/**
 * @brief Create outgoing session
 *
 * Creates a session suitable for making calls to one of the endpoints
 * registered with #lwmsg_peer_add_connect_endpoint().  Establishing
 * a connection to an endpoint will not occur until a call is made,
 * at which point the endpoints will be tried in order until one succeeds.
 *
 * @param[in,out] peer the peer handle
 * @param[out] session the created session
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_connect(
    LWMsgPeer* peer,
    LWMsgSession** session
    );

/**
 * @brief Close outgoing session
 *
 * Closes the session established by #lwmsg_peer_connect().  All outstanding
 * outgoing calls will be canceled and all open handles will be rendered
 * invalid.  The session handle may no longer be used after calling
 * this function.
 *
 * @param[in,out] peer the peer handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_disconnect(
    LWMsgPeer* peer
    );

/**
 * @brief Establish session on existing fd
 *
 * Establishes a session with a peer like #lwmsg_peer_connect(),
 * but uses the provided pre-connected file descriptor.
 *
 * @param[in,out] peer the peer handle
 * @param[in] type the endpoint type
 * @param[in] fd the fd
 * @param[out] session set to the created session
 * @retval #LWMSG_STATUS_SUCCESS success
 */
LWMsgStatus
lwmsg_peer_connect_fd(
    LWMsgPeer* peer,
    LWMsgEndpointType type,
    int fd,
    LWMsgSession** session
    );

/**
 * @brief Accept session on existing fd
 *
 * Accepts a session on an fd that is already connected.
 *
 * @param[in,out] peer the peer handle
 * @param[in] type the endpoint type
 * @param[in] fd the fd
 * @retval #LWMSG_STATUS_SUCCESS success
 */
LWMsgStatus
lwmsg_peer_accept_fd(
    LWMsgPeer* peer,
    LWMsgEndpointType type,
    int fd
    );

/**
 * @brief Acquire outgoing call handle [DEPRECATED]
 *
 * Acquire a call handle which can be used to make an outgoing call.  Peer
 * call handles fully support asynchronous calls.  The handle may be reused
 * after each call completes.  If the peer has not been connected with 
 * #lwmsg_peer_connect(), this function will do so implicitly.  The acquired
 * call handle should be released with #lwmsg_call_release() when no longer
 * needed.
 *
 * @param[in,out] peer the peer handle
 * @param[out] call the acquired call handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 * @deprecated Use #lwmsg_session_acquire_call() on the #LWMsgSession returned
 * by #lwmsg_peer_connect() instead.
 */
LWMsgStatus
lwmsg_peer_acquire_call(
    LWMsgPeer* peer,
    LWMsgCall** call
    );

/*@}*/

#endif
