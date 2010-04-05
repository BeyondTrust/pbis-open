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
 *        session-private.h
 *
 * Abstract:
 *
 *        Session management API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_SESSION_PRIVATE_H__
#define __LWMSG_SESSION_PRIVATE_H__

#include <lwmsg/session.h>

/**
 * @file session-private.h
 * @brief Session API (INTERNAL)
 * @internal
 */

/**
 * @internal
 * @defgroup session_private Sessions
 * @ingroup private
 * @brief Session and session manager implementation
 */

/*@{*/

typedef struct
{
    unsigned char bytes[8];
} LWMsgSessionCookie;

typedef struct LWMsgSessionID
{
    LWMsgSessionCookie connect;
    LWMsgSessionCookie accept;
} LWMsgSessionID;

typedef char LWMsgSessionString[66];
typedef uint32_t LWMsgHandleID;

/**
 * @internal
 * @brief Session manager function table structure
 *
 * This structure contains the implementation of a session manager.
 */
typedef struct LWMsgSessionManagerClass
{
    /**
     * @brief Delete session manager
     *
     * Deletes the session manager
     *
     * @param manager the session manager
     */
    void
    (*delete) (
        LWMsgSessionManager* manager
        );

    LWMsgStatus
    (*create) (
        LWMsgSessionManager* manager,
        LWMsgSession** session
        );

    LWMsgStatus
    (*connect) (
        LWMsgSession* session,
        const LWMsgSessionCookie* accept,
        LWMsgSecurityToken* token
        );

    LWMsgStatus
    (*accept) (
        LWMsgSessionManager* manager,
        const LWMsgSessionCookie* connect,
        LWMsgSecurityToken* token,
        LWMsgSession** session);

    void
    (*release) (
        LWMsgSession* session
        );

    /**
     * @brief Register local handle
     *
     * Registers a local handle on the given session.
     *
     * @param[in] manager the session manager
     * @param[in,out] handle the session handle
     * @param[in] type the name of the handle type
     * @param[in] ptr the handle pointer
     * @param[in] cleanup an optional cleanup function for the handle
     * @param[out] hid a unique integer which identifies the handle within the session
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_memory
     * @lwmsg_code{INVALID_HANDLE, the handle is already registered}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*register_handle_local) (
        LWMsgSession* session,
        const char* type,
        void* ptr,
        void (*cleanup)(void* ptr),
        LWMsgHandleID* hid
        );

    /**
     * @brief Register remote handle
     *
     * Registers a remote handle on the given session.
     *
     * @param[in] manager the session manager
     * @param[in,out] handle the session handle
     * @param[in] type the name of the handle type
     * @param[in] hid the internal ID of the handle
     * @param[in] cleanup an optional cleanup function for the handle
     * @param[out] ptr a pointer to a unique proxy object which represents the handle locally
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_memory
     * @lwmsg_code{INVALID_HANDLE, the handle is already registered}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*register_handle_remote) (
        LWMsgSession* session,
        const char* type,
        LWMsgHandleID hid,
        void (*cleanup)(void* ptr),
        void** ptr
        );
    
    /**
     * @brief Retain handle
     *
     * Retains a reference to the given handle, increasing its reference count by
     * one.  Even if a handle is unregistered, it will not be cleaned up until
     * all references are released.
     *
     * @param[in] manager the session manager
     * @param[in,out] session
     * @param[in] ptr the handle pointer
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{INVALID_HANDLE, the pointer is not registered as a handle}
     */
    LWMsgStatus
    (*retain_handle) (
        LWMsgSession* session,
        void* ptr
        );

    /**
     * @brief Release handle
     *
     * Releases a reference to the given handle, decreasing its reference count by
     * one.  When the handle reaches zero references, the cleanup function passed
     * the register function will be invoked.
     *
     * @param[in] manager the session manager
     * @param[in,out] session
     * @param[in] ptr the handle pointer
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{INVALID_HANDLE, the pointer is not registered as a handle}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*release_handle) (
        LWMsgSession* session,
        void* ptr
        );

    /**
     * @brief Unregister handle
     *
     * Releases a reference to the given handle, decreasing its reference count by
     * one, and marks the handle as stale.  Any further attempts to use the handle's
     * internal ID will fail, but release and retain operations will continue to
     * succeed until the reference count reaches 0.
     *
     * @param[in] manager the session manager
     * @param[in,out] session
     * @param[in] ptr the handle pointer
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{INVALID_HANDLE, the pointer is not registered as a handle}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*unregister_handle) (
        LWMsgSession* session,
        void* ptr
        );

    /**
     * @brief Map pointer to internal handle ID
     *
     * Maps a handle pointer to the internal ID within the session.
     *
     * @param[in] manager the session manager
     * @param[in,out] session
     * @param[in] ptr the handle pointer
     * @param[out] type the name of the handle type in the type specification
     * @param[out] htype the type of the handle -- local or remote
     * @param[out] hid the internal ID of the handle
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{INVALID_HANDLE, the pointer is not registered as a handle}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*handle_pointer_to_id) (
        LWMsgSession* session,
        void* ptr,
        const char** type,
        LWMsgHandleType* htype,
        LWMsgHandleID* hid
        );

    /**
     * @brief Map internal handle ID to pointer
     *
     * Maps an internal handle ID to a pointer
     *
     * @param[in] manager the session manager
     * @param[in,out] session the session handle
     * @param[in] type the name of the handle type in the type specification
     * @param[in] htype the type of the handle -- local or remote
     * @param[out] ptr the handle pointer
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{INVALID_HANDLE, the given ID\, type name\, and type is not registered as a handle}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*handle_id_to_pointer) (
        LWMsgSession* session,
        const char* type,
        LWMsgHandleType htype,
        LWMsgHandleID hid,
        void** ptr
        );

    /**
     * @brief Get security token for peer
     *
     * Gets a security token which represents the identity of the peer
     * for the given session.
     *
     * @param[in] manager the session manager
     * @param[in] session the session handle
     * @return a security token representing the peer identity, or NULL if unauthenticated
     */
    LWMsgSecurityToken*
    (*get_peer_security_token) (
        LWMsgSession* session
        );

    /**
     * @brief Get custom session data
     *
     * Gets a custom data pointer for the given session.  The data
     * pointer is set by the construtor function passed to the session enter
     * function.
     *
     * @param[in] manager the session manager
     * @param[in] session the session handle
     * @return the session data pointer
     */
    void*
    (*get_data) (
        LWMsgSession* session
        );

    const LWMsgSessionID*
    (*get_id) (
        LWMsgSession* session
        );

    size_t
    (*get_assoc_count) (
        LWMsgSession* session
        );

    size_t
    (*get_handle_count) (
        LWMsgSession* session
        );
} LWMsgSessionManagerClass;

struct LWMsgSession
{
    LWMsgSessionManager* manager;
};

struct LWMsgSessionManager
{
    LWMsgSessionManagerClass* mclass;
};

const LWMsgSessionID*
lwmsg_session_get_id(
    LWMsgSession* session
    );

void
lwmsg_session_generate_cookie(
    LWMsgSessionCookie* cookie
    );

#ifndef LWMSG_NO_THREADS
LWMsgStatus
lwmsg_shared_session_manager_new(
    LWMsgSessionConstructFunction construct,
    LWMsgSessionDestructFunction destruct,
    void* construct_data,
    LWMsgSessionManager** out_manager
    );
#endif

void
lwmsg_session_manager_delete(
    LWMsgSessionManager* manager
    );

void
lwmsg_session_id_to_string(
    const LWMsgSessionID* smid,
    LWMsgSessionString buffer
    );

LWMsgStatus
lwmsg_session_create (
    LWMsgSessionManager* manager,
    LWMsgSession** session
    );

LWMsgStatus
lwmsg_session_connect (
    LWMsgSession* session,
    const LWMsgSessionCookie* accept,
    LWMsgSecurityToken* token
    );

LWMsgStatus
lwmsg_session_accept(
    LWMsgSessionManager* manager,
    const LWMsgSessionCookie* connect,
    LWMsgSecurityToken* token,
    LWMsgSession** session
    );

void
lwmsg_session_release(
    LWMsgSession* session
    );

LWMsgStatus
lwmsg_session_register_handle_local (
    LWMsgSession* session,
    const char* type,
    void* ptr,
    void (*cleanup)(void* ptr),
    LWMsgHandleID* hid
    );

LWMsgStatus
lwmsg_session_register_handle_remote (
    LWMsgSession* session,
    const char* type,
    LWMsgHandleID hid,
    void (*cleanup)(void* ptr),
    void** ptr
    );

LWMsgStatus
lwmsg_session_handle_pointer_to_id (
    LWMsgSession* session,
    void* ptr,
    const char** type,
    LWMsgHandleType* htype,
    LWMsgHandleID* hid
    );

LWMsgStatus
lwmsg_session_handle_id_to_pointer (
    LWMsgSession* session,
    const char* type,
    LWMsgHandleType htype,
    LWMsgHandleID hid,
    void** ptr
    );

LWMsgStatus
lwmsg_default_session_manager_new(
    LWMsgSessionConstructFunction construct,
    LWMsgSessionDestructFunction destruct,
    void* construct_data,
    LWMsgSessionManager** out_manager
    );

LWMsgStatus
lwmsg_session_manager_init(
    LWMsgSessionManager* manager,
    LWMsgSessionManagerClass* mclass
    );

/**
 * @internal
 * @brief Get association count
 *
 * Returns a count of the number of associations that
 * have entered the given session.
 *
 * @param session the session
 * @return the number of associations currently inside the session
 */
size_t
lwmsg_session_get_assoc_count(
    LWMsgSession* session
    );

/**
 * @internal
 * @brief Get handle count
 *
 * Returns a count of the number of handles contained
 * in the given session.
 *
 * @param session the session
 * @return the number of handles current contained in the session
 */
size_t
lwmsg_session_get_handle_count(
    LWMsgSession* session
    );

#define LWMSG_SESSION_MANAGER(obj) ((LWMsgSessionManager*) (obj))
#define LWMSG_SESSION(obj) ((LWMsgSession*) (obj))

/*@}*/

#endif
