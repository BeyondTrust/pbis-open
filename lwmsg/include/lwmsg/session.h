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
 *        session.h
 *
 * Abstract:
 *
 *        Sesssion management API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_SESSION_H__
#define __LWMSG_SESSION_H__

#include <lwmsg/status.h>
#include <lwmsg/security.h>
#include <lwmsg/common.h>
#include <lwmsg/context.h>

/**
 * @file session.h
 * @brief Session API
 */

/**
 * @defgroup session Sessions
 * @ingroup public
 * @brief Session abstraction
 *
 * A session is an abstract connection with a peer that allows
 * calls to be made and state to be shared.
 */

/*@{*/

#ifndef DOXYGEN
struct LWMsgCall;
#endif

/**
 * @brief A session
 *
 * An opaque session structure
 */
typedef struct LWMsgSession LWMsgSession;

/**
 * @brief A handle
 *
 * An opaque handle structure
 */
typedef struct LWMsgHandle LWMsgHandle;

/**
 * @brief Handle cleanup callback
 *
 * A callback used to clean up a handle after it is no longer in use.
 * A cleanup callback can be registered as part of lwmsg_session_register_handle().
 * The cleanup callback will be invoked when the last reference to the
 * handle is dropped, or when the session containing the handle is
 * torn down.
 *
 * @param[in] handle the handle to clean up
 */
typedef void (*LWMsgHandleCleanupFunction) (void* handle);

/**
 * @brief Session constructor callback
 *
 * A callback function which is invoked when a session is first establish.
 * It may set a session data pointer which will be attached to the session
 * to track custom user information.  It may inspect the provided security token
 * and choose to reject the session entirely.
 *
 * @param[in] token a security token representing the identity of the peer
 * @param[in] data a user data pointer
 * @param[out] session_data a session data pointer which contains custom per-session information
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{SECURITY, the peer is denied access}
 * @lwmsg_endstatus
 */
typedef LWMsgStatus
(*LWMsgSessionConstructFunction) (
    LWMsgSecurityToken* token,
    void* data,
    void** session_data
    );

/**
 * @brief Session destructor callback
 *
 * A callback function which is invoked by the session manager
 * when a session is no longer in use.  It should clean up the
 * session data created by the constructor function.
 *
 * @param[in] token a security token representing the identity of the peer
 * @param[in,out] session_data the custom session data created by the constructor
 */
typedef void
(*LWMsgSessionDestructFunction) (
    LWMsgSecurityToken* token,
    void* session_data
    );

/**
 * @brief Handle type
 *
 * Specifies the type of an opaque handle within a session.  Only
 * handles which are local may be safely dereferenced.  Handles
 * marked as remote are proxies for objects created by the peer
 * and have undefined contents.
 */
typedef enum LWMsgHandleType
{
    /** The handle is NULL
     * @hideinitializer
     */
    LWMSG_HANDLE_NULL = 0,
    /** 
     * The handle is local
     * @hideinitializer
     */
    LWMSG_HANDLE_LOCAL = 1,
    /**
     * The handle is remote
     * @hideinitializer
     */
    LWMSG_HANDLE_REMOTE = 2
} LWMsgHandleType;

/**
 * @brief Register local handle
 *
 * Registers a local handle so that it may be passed to the peer in
 * subsequent calls.
 *
 * @param[in,out] session the session
 * @param[in] typename a string constant describing the type of the handle.
 * This should be the same as the type name given to the #LWMSG_HANDLE()
 * or #LWMSG_MEMBER_HANDLE() macro in the type specification.
 * @param[in] data the local data to associate with the handle
 * @param[in] cleanup a cleanup function which should be invoked when the
 * handle is no longer referenced
 * @param[out] handle set to the created handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_session_register_handle(
    LWMsgSession* session,
    const char* typename,
    void* data,
    LWMsgHandleCleanupFunction cleanup,
    LWMsgHandle** handle
    );

/**
 * @brief Increase handle reference count
 *
 * Takes an extra reference to the given handle.
 * When the last reference to a handle is released, the handle
 * will be cleaned up using the function passed to #lwmsg_session_register_handle().
 *
 * @param[in,out] session the session
 * @param[in,out] handle the handle
 */
void
lwmsg_session_retain_handle(
    LWMsgSession* session,
    LWMsgHandle* handle
    );

/**
 * @brief Decrease handle reference count
 *
 * Releases a reference to the given handle.
 * When the last reference to a handle is released, the handle
 * will be cleaned up using the function passed to #lwmsg_session_register_handle().
 *
 * @param[in,out] session the session
 * @param[in,out] handle the handle
 */
void
lwmsg_session_release_handle(
    LWMsgSession* session,
    LWMsgHandle* handle
    );

/**
 * @brief Unregister handle
 *
 * Unregisters a handle.  In addition to releasing a reference as by
 * #lwmsg_session_release_handle(), this also marks the handle as invalid
 * and will not allow any further use of it in the session.  After a handle
 * is unregistered, it may only be retained or released -- all other operations
 * treat it as invalid.
 *
 * A handle should only be unregistered by the creator that originally
 * registered it.  All other users of the handle should release their
 * reference with #lwmsg_session_release_handle().
 *
 * @param[in,out] session the session
 * @param[in] handle the handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_HANDLE, the handle was not previously registered}
 * with the session, or was already unregistered
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_session_unregister_handle(
    LWMsgSession* session,
    LWMsgHandle* handle
    );

/**
 * @brief Get handle data
 *
 * Gets the data associated with a handle when it was originally registered.
 *
 * This function may only safely be called by the creator of the handle
 * that originally registered it.  In particular, attempting to use this
 * function on a remote handle has undefined behavior.
 *
 * @param[in] session the session
 * @param[in] handle the handle
 * @param[out] data set to the data associated with the handle
 * @retval LWMSG_STATUS_SUCCESS success
 * @retval LWMSG_STATUS_INVALID_HANDLE the handle is invalid or has been
 * invalidated
 */
LWMsgStatus
lwmsg_session_get_handle_data(
    LWMsgSession* session,
    LWMsgHandle* handle,
    void** data
    );

/**
 * @brief Query handle type
 *
 * Queries the type of a given handle: local or remote.
 *
 * @param[in] session the session
 * @param[in] handle the handle
 * @param[out] location the location of the handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_HANDLE, the handle is not registered}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_session_get_handle_location(
    LWMsgSession* session,
    LWMsgHandle* handle,
    LWMsgHandleType* location
    );

/**
 * @brief Get custom session data
 *
 * Retrieves the custom session data pointer created by the session's construtor
 * function.
 *
 * @param[in] session the session
 * @return the session data pointer
 */
void*
lwmsg_session_get_data(
    LWMsgSession* session
    );

/**
 * @brief Get peer security token
 *
 * Retrives the security token for the remote peer.  The token remains
 * valid for the duration of the session.
 *
 * @param[in] session the session
 * @return the security token, or NULL if the session is unauthenticated
 */
LWMsgSecurityToken*
lwmsg_session_get_peer_security_token(
    LWMsgSession* session
    );

/**
 * @brief Acquire a call handle
 *
 * Acquires a handle suitable for making a call to the session peer.
 *
 * @param[in] session the session
 * @param[out] call set to the acquired call handle
 * @retval #LWMSG_STATUS_SUCCESS success
 */
LWMsgStatus
lwmsg_session_acquire_call(
    LWMsgSession* session,
    struct LWMsgCall** call
    );

/*@}*/

#endif
