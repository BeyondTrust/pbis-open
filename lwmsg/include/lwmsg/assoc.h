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
 *        assoc.h
 *
 * Abstract:
 *
 *        Association API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_ASSOC_H__
#define __LWMSG_ASSOC_H__

#include <lwmsg/status.h>
#include <lwmsg/message.h>
#include <lwmsg/protocol.h>
#include <lwmsg/context.h>
#include <lwmsg/time.h>
#include <lwmsg/security.h>
#include <lwmsg/session.h>
#include <lwmsg/call.h>

#include <stdlib.h>
#include <unistd.h>

/**
 * @file assoc.h
 * @brief Message-oriented APIs
 */

/**
 * @defgroup assoc Associations
 * @ingroup public
 * @brief Send and receive messages with a peer
 *
 * Associations are a message-oriented abstraction which provide a more
 * convenient mechanism for communication than raw access to the
 * marshalling facility:
 *
 * <ul>
 * <li>Straightforward message sending and receiving</li>
 * <li>Stateful communication using handles</li>
 * <li>Access to metadata such as peer credentials</li>
 * </ul>
 *
 * Messages consist of a message type and a payload which
 * is marshalled and unmarshalled automatically.  The marshaller type
 * of the associated payload and the set of available messages
 * are defined by a protocol.  
 * 
 * Associations are an abstract data type.  For a concrete implementation
 * useful in interprocess or network communication, see Connections.
 */

/**
 * @ingroup assoc
 * @brief An association
 *
 * An opaque, abstract structure for message-oriented communication.
 * Associations are not inherently thread-safe and must not be used
 * concurrently from multiple threads.
 */
typedef struct LWMsgAssoc LWMsgAssoc;

/**
 * @ingroup assoc
 * @brief Association state
 *
 * Represents the current state of an association as returned
 * by lwmsg_assoc_get_state().
 */
typedef enum LWMsgAssocState
{
    /** @brief Unspecified state */
    LWMSG_ASSOC_STATE_NONE,
    /** @brief Association is not established */
    LWMSG_ASSOC_STATE_NOT_ESTABLISHED,
    /** @brief Association is idle */
    LWMSG_ASSOC_STATE_IDLE,
    /** @brief Association is blocked waiting to send */
    LWMSG_ASSOC_STATE_BLOCKED_SEND,
    /** @brief Association is blocked waiting to receive */
    LWMSG_ASSOC_STATE_BLOCKED_RECV,
    /** @brief Association is blocked waiting to send and/or receive */
    LWMSG_ASSOC_STATE_BLOCKED_SEND_RECV,
    /** @brief Association is closed */
    LWMSG_ASSOC_STATE_CLOSED,
    /** @brief Association is busy */
    LWMSG_ASSOC_STATE_BUSY,
    /** @brief Association experienced an error */
    LWMSG_ASSOC_STATE_ERROR
} LWMsgAssocState;

/**
 * @ingroup assoc
 * @brief Timeout classification
 *
 * Represents a class of timeout which may be set
 * on an association with lwmsg_assoc_set_timeout()
 */
typedef enum LWMsgTimeout
{
    /** @brief Timeout for a message send or receive */
    LWMSG_TIMEOUT_MESSAGE,
    /** @brief Timeout for session establishment */
    LWMSG_TIMEOUT_ESTABLISH,
    /** @brief Idle timeout */
    LWMSG_TIMEOUT_IDLE
} LWMsgTimeout;

/**
 * @ingroup assoc
 * @brief Delete an association
 *
 * Deletes an association, releasing all allocated resources
 * 
 * @param[in] assoc the association to delete
 */
void
lwmsg_assoc_delete(
    LWMsgAssoc* assoc
    );

/**
 * @ingroup assoc
 * @brief Get association protocol
 *
 * Returns the protocol used by the association.
 *
 * @param[in] assoc the association
 * @return the protocol specified when the association was created
 */
LWMsgProtocol*
lwmsg_assoc_get_protocol(
    LWMsgAssoc* assoc
    );

/**
 * @ingroup assoc
 * @brief Send a message
 *
 * Sends a message on the specified association.  This function uses the
 * full LWMsgMessage data structure to specify the message
 *
 * @param[in] assoc the association
 * @param[in] message the message to send
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{TIMEOUT, operation timed out}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_send_message(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    );

/**
 * @ingroup assoc
 * @brief Receive a message
 *
 * Receives a message on the specified association.  This function uses the
 * full LWMsgMessage data structure to return the received message.
 *
 * @param[in] assoc the association
 * @param[out] message the received message
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{TIMEOUT, operation timed out}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_recv_message(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    );

/**
 * @ingroup assoc
 * @brief Send a message and receive a reply [deprecated]
 *
 * This function sends a message and receives a reply in a single
 * operation.
 *
 * @param[in] assoc the association
 * @param[in] send_message the message to send (request)
 * @param[out] recv_message the received message (reply)
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{TIMEOUT, operation timed out}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 * @deprecated use #lwmsg_assoc_send_message followed by #lwmsg_assoc_recv_message, or the call interface
 */
LWMsgStatus
lwmsg_assoc_send_message_transact(
    LWMsgAssoc* assoc,
    LWMsgMessage* send_message,
    LWMsgMessage* recv_message
    );

/**
 * @ingroup assoc
 * @brief Close an association
 *
 * Closes the specified assocation, which may include notifying the
 * peer and shutting down the underlying communication channel.
 * Unlike lwmsg_assoc_delete(), which aggressively closes the
 * association and releases all resources in constant time, this
 * function may block indefinitely, time out, or fail, but allows
 * for a controlled, orderly shutdown.  After an association is closed,
 * the result of performing further sends or receives is unspecified.
 *
 * @param[in] assoc the association
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{TIMEOUT, operation timed out}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_close(
    LWMsgAssoc* assoc
    );

/**
 * @ingroup assoc
 * @brief Reset an association
 *
 * Resets an association to its baseline state, which is similar
 * in immediate effect to closing it (e.g. shutting down the
 * underlying communication channel). However, while closing
 * an association with lwmsg_assoc_close() generally represents
 * an intent to cease further communication, a reset implies that
 * communication may resume once the problem that necessitated the
 * reset has been remedied.  For example, a server which times out an
 * idle client connection will typically reset it rather than close it,
 * indicating to the client that it should reset the association locally
 * and resume communication if it is still alive.
 *
 * The difference between close and reset is not purely symbolic.
 * The association implementation may release additional resources
 * and state it considers obsolete when closed but keep such state
 * intact when it is reset.
 *
 * A reset, like a close, removes an association from its session.
 * If this was the last association in that session, the session
 * and all its handles are no longer valid.
 *
 * @param[in] assoc the association
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{TIMEOUT, operation timed out}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_reset(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_assoc_finish(
    LWMsgAssoc* assoc,
    LWMsgMessage** completed
    );

LWMsgStatus
lwmsg_assoc_set_nonblock(
    LWMsgAssoc* assoc,
    LWMsgBool nonblock
    );

/**
 * @ingroup assoc
 * @brief Destroy a message
 *
 * Destroys a message structure, freeing any data payload it may contain.
 *
 * @param[in] assoc the assocation
 * @param[in] message the message to destroy
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{NOT_FOUND, the message tag is not known by the association's protocol}
 * @lwmsg_etc{an error returned by the memory manager}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_destroy_message(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    );

#ifndef LWMSG_DISABLE_DEPRECATED
/**
 * @brief Destroy a message <b>[DEPRECATED]</b>
 * @deprecated Use #lwmsg_assoc_destroy_message()
 * @hideinitializer
 */
#define lwmsg_assoc_free_message(assoc, message) \
    lwmsg_assoc_destroy_message(assoc, message)

/**
 * @ingroup assoc
 * @brief Free a message (simple) <b>[DEPRECATED]</b>
 *
 * Frees the object graph of a message using the memory manager and
 * protocol of the specified association.  This function does not
 * require a complete LWMsgMessage structure.
 *
 * @param[in] assoc the assocation
 * @param[in] tag the tag of the message to free
 * @param[in] root the root of the object graph
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 * @deprecated Use #lwmsg_assoc_destroy_message()
 */
LWMsgStatus
lwmsg_assoc_free_graph(
    LWMsgAssoc* assoc,
    LWMsgTag tag,
    void* root
    );
#endif

/**
 * @ingroup assoc
 * @brief Get current session
 * 
 * Gets the current session for the specified association.  The session can be used
 * for handle management and authentication of the peer.  A session will only be
 * available after calling #lwmsg_assoc_connect() or #lwmsg_assoc_accept() and before
 * calling #lwmsg_assoc_close().  Some types of associations may not support sessions,
 * in which case this function will return #LWMSG_STATUS_SUCCESS and set session
 * to NULL.
 *
 * @param[in] assoc the association
 * @param[out] session the session
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the association is closed or not yet established}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_get_session(
    LWMsgAssoc* assoc,
    LWMsgSession** session
    );

/**
 * @ingroup assoc
 * @brief Get association state
 *
 * Gets the current state of the specified association.  This may
 * reveal details such as:
 *
 * - Whether the association has been closed
 * - Whether the association is part of an established session
 * - If the association is ready to send a message or receive a message
 *
 * @param[in] assoc the association
 * @return the current state of the association
 */
LWMsgAssocState
lwmsg_assoc_get_state(
    LWMsgAssoc* assoc
    );

/**
 * @ingroup assoc
 * @brief Set timeout
 * 
 * Sets a timeout that should be used for subsequent operations.
 *
 * @param[in] assoc the association
 * @param[in] type the type of timeout
 * @param[in] value the value of the timeout, or NULL for no timeout
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{UNSUPPORTED, the association does not support the specified timeout type}
 * @lwmsg_etc{implementation-specific error}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_set_timeout(
    LWMsgAssoc* assoc,
    LWMsgTimeout type,
    LWMsgTime* value
    );

/**
 * @ingroup assoc
 * @brief Connect association to peer
 *
 * Connects an association to a peer.  The peer should call #lwmsg_assoc_accept()
 * to accept the connection.  An existing session can be passed in the <tt>session</tt>
 * parameter -- otherwise, a new session will be created automatically and can be
 * accessed later with #lwmsg_assoc_get_session().
 *
 * @param[in,out] assoc the association
 * @param[in,out] session an optional existing session to use for the association
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{CONNECTION_REFUSED, connection refused}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_connect(
    LWMsgAssoc* assoc,
    LWMsgSession* session
    );

/**
 * @ingroup assoc
 * @brief Accept connection from peer on assocation
 *
 * Accepts a connection from a peer on the given association.  The peer should call
 * #lwmsg_assoc_connect() to initiate the connection.  An existing session can be
 * passed as the <tt>session</tt> parameter.  Otherwise, the association will create
 * one automatically.
 *
 * @param[in,out] assoc the association
 * @param[in,out] session (optional) an existing session to use
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{CONNECTION_REFUSED, connection refused}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_accept(
    LWMsgAssoc* assoc,
    LWMsgSession* session
    );
 
/**
 * @ingroup assoc
 * @brief Print message in human-readable form
 *
 * Prints a message in a human-readable using the protocol information
 * for the given association.  The result is allocated using the same
 * memory manager as the association.
 *
 * @param[in] assoc the association
 * @param[in] message the message to print
 * @param[out] result the printed form of the message
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_print_message_alloc(
    LWMsgAssoc* assoc,
    LWMsgMessage* message,
    char** result
    );

/**
 * @ingroup assoc
 * @brief Acquire call handle [DEPRECATED]
 *
 * Acquires a call handle that can be used to make a call across an
 * association as a send followed by a receive.  Only one such call
 * handle may be acquired until it is released with #lwmsg_call_release().
 *
 * @param[in,out] assoc the association
 * @param[out] call the call handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{BUSY, a call handle has already been acquired}
 * @lwmsg_code{UNSUPPORTED, the association does not support calls}
 * @lwmsg_endstatus
 * @deprecated Use #lwmsg_session_acquire_call() with the session
 * passed to #lwmsg_assoc_connect() or #lwmsg_assoc_accept().  If
 * you did not pass a session yourself, you can use #lwmsg_assoc_get_session()
 * to get the active session.
 */
LWMsgStatus
lwmsg_assoc_acquire_call(
    LWMsgAssoc* assoc,
    LWMsgCall** call
    );

#ifndef DOXYGEN
extern LWMsgTypeClass lwmsg_handle_type_class;

#define LWMSG_ASSOC_HANDLE_LOCAL_FOR_RECEIVER 0x1
#define LWMSG_ASSOC_HANDLE_LOCAL_FOR_SENDER 0x2

#endif

/**
 * @ingroup types
 * @brief Define a handle
 *
 * Defines a handle type within a type specification.  Handles
 * are opaque pointer types which are only usable with associations.
 *
 * @param htype the name of the handle type
 * @hideinitializer
 */
#define LWMSG_HANDLE(htype) LWMSG_CUSTOM(void*, &lwmsg_handle_type_class, (void*) #htype)

/**
 * @ingroup types
 * @brief Define a handle as a member
 *
 * Defines a handle type as a member of a struct or union.  Handles
 * are opaque pointer types which are only usable with associations.
 *
 * @param type the type of the containing struct or union
 * @param field the field within the containing type
 * @param htype the name of the handle type
 * @hideinitializer
 */
#define LWMSG_MEMBER_HANDLE(type, field, htype) LWMSG_MEMBER_CUSTOM(type, field, &lwmsg_handle_type_class, (void*) #htype)

/**
 * @ingroup types
 * @brief Ensure that handle is local to receiving peer
 *
 * Specifies that the previous type or member, which must be a handle,
 * must be a local handle from the perspective of the receiver.
 * @hideinitializer
 */
#define LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER LWMSG_ATTR_CUSTOM(LWMSG_ASSOC_HANDLE_LOCAL_FOR_RECEIVER)

/**
 * @ingroup types
 * @brief Ensure that handle is local to sending peer
 *
 * Specifies that the previous type or member, which must be a handle,
 * must be a local handle from the perspective of the sender.
 * @hideinitializer
 */
#define LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER LWMSG_ATTR_CUSTOM(LWMSG_ASSOC_HANDLE_LOCAL_FOR_SENDER)

#endif
