/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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
#include <lwmsg/call.h>

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
    LWMsgSessionCookie connect_id;
    LWMsgSessionCookie accept_id;
} LWMsgSessionID;

typedef char LWMsgSessionString[66];
typedef uint32_t LWMsgHandleID;

typedef struct LWMsgSessionClass
{
    LWMsgStatus
    (*connect_peer) (
        LWMsgSession* session,
        const LWMsgSessionCookie* accept_peer,
        LWMsgSecurityToken* token
        );

    LWMsgStatus
    (*accept_peer) (
        LWMsgSession* session,
        const LWMsgSessionCookie* connect_peer,
        LWMsgSecurityToken* token);

    void
    (*release) (
        LWMsgSession* session
        );

    LWMsgStatus
    (*register_handle_local) (
        LWMsgSession* session,
        const char* type,
        void* data,
        void (*cleanup)(void* ptr),
        LWMsgHandle** handle
        );

    LWMsgStatus
    (*register_handle_remote) (
        LWMsgSession* session,
        const char* type,
        LWMsgHandleID hid,
        void (*cleanup)(void* ptr),
        LWMsgHandle** handle
        );
    
    void
    (*retain_handle) (
        LWMsgSession* session,
        LWMsgHandle* handle
        );

    void
    (*release_handle) (
        LWMsgSession* session,
        LWMsgHandle* handle
        );

    LWMsgStatus
    (*unregister_handle) (
        LWMsgSession* session,
        LWMsgHandle* handle
        );

    LWMsgStatus
    (*get_handle_data) (
        LWMsgSession* session,
        LWMsgHandle* handle,
        void** data
        );

    LWMsgStatus
    (*resolve_handle_to_id) (
        LWMsgSession* session,
        LWMsgHandle* handle,
        const char** type,
        LWMsgHandleType* htype,
        LWMsgHandleID* hid
        );

    LWMsgStatus
    (*resolve_id_to_handle) (
        LWMsgSession* session,
        const char* type,
        LWMsgHandleType htype,
        LWMsgHandleID hid,
        LWMsgHandle** handle
        );

    LWMsgSecurityToken*
    (*get_peer_security_token) (
        LWMsgSession* session
        );

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

    LWMsgStatus
    (*acquire_call) (
        LWMsgSession* session,
        LWMsgCall** call
        );
} LWMsgSessionClass;

struct LWMsgSession
{
    LWMsgSessionClass* sclass;
};

const LWMsgSessionID*
lwmsg_session_get_id(
    LWMsgSession* session
    );

void
lwmsg_session_generate_cookie(
    LWMsgSessionCookie* cookie
    );

void
lwmsg_session_id_to_string(
    const LWMsgSessionID* smid,
    LWMsgSessionString buffer
    );

void
lwmsg_session_release(
    LWMsgSession* session
    );

size_t
lwmsg_session_get_assoc_count(
    LWMsgSession* session
    );

size_t
lwmsg_session_get_handle_count(
    LWMsgSession* session
    );

#define LWMSG_SESSION(obj) ((LWMsgSession*) (obj))

/*@}*/

#endif
