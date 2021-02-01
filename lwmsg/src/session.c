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
 *        session.c
 *
 * Abstract:
 *
 *        Session management API
 *        Primary entry points
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "util-private.h"
#include "session-private.h"
#include "mt19937ar.h"
#include <lwmsg/time.h>

#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

void
lwmsg_session_id_to_string(
    const LWMsgSessionID* id,
    LWMsgSessionString buffer
    )
{
    sprintf(buffer, "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
            (unsigned int) id->connect_id.bytes[0],
            (unsigned int) id->connect_id.bytes[1],
            (unsigned int) id->connect_id.bytes[2],
            (unsigned int) id->connect_id.bytes[3],
            (unsigned int) id->connect_id.bytes[4],
            (unsigned int) id->connect_id.bytes[5],
            (unsigned int) id->connect_id.bytes[6],
            (unsigned int) id->connect_id.bytes[7],
            (unsigned int) id->accept_id.bytes[0],
            (unsigned int) id->accept_id.bytes[1],
            (unsigned int) id->accept_id.bytes[2],
            (unsigned int) id->accept_id.bytes[3],
            (unsigned int) id->accept_id.bytes[4],
            (unsigned int) id->accept_id.bytes[5],
            (unsigned int) id->accept_id.bytes[6],
            (unsigned int) id->accept_id.bytes[7]);
}

void
lwmsg_session_generate_cookie(
    LWMsgSessionCookie* cookie
    )
{
    mt m;
    uint32_t seed[3];
    uint32_t s;
    int i;
    LWMsgTime now;

    lwmsg_time_now(&now);

    /* Add in 32 bits of data from the address of the cookie */
    seed[0] = (uint32_t) (size_t) cookie;
    /* Add in 32 bits of data from the current pid */
    seed[1] = (uint32_t) getpid();
    /* Add in 32 bits of data from the current time */
    seed[2] = (uint32_t) now.microseconds;
        
    mt_init_by_array(&m, seed, sizeof(seed) / sizeof(*seed));
        
    for (i = 0; i < sizeof(cookie->bytes); i += sizeof(s))
    {
        s = mt_genrand_int32(&m);
        
        memcpy(cookie->bytes + i, &s, sizeof(s));
    }
}

void
lwmsg_session_release(
    LWMsgSession* session
    )
{
    if (session && session->sclass->release)
    {
        session->sclass->release(session);
    }
}

size_t
lwmsg_session_get_assoc_count(
    LWMsgSession* session
    )
{
    return session->sclass->get_assoc_count(session);
}

size_t
lwmsg_session_get_handle_count(
    LWMsgSession* session
    )
{
    return session->sclass->get_handle_count(session);
}

LWMsgStatus
lwmsg_session_register_handle(
    LWMsgSession* session,
    const char* typename,
    void* data,
    LWMsgHandleCleanupFunction cleanup,
    LWMsgHandle** handle
    )
{
    return session->sclass->register_handle_local(
        session,
        typename,
        data,
        cleanup,
        handle);
}

void
lwmsg_session_retain_handle(
    LWMsgSession* session,
    LWMsgHandle* handle
    )
{
    session->sclass->retain_handle(session, handle);
}

void
lwmsg_session_release_handle(
    LWMsgSession* session,
    LWMsgHandle* handle
    )
{
    session->sclass->release_handle(session, handle);
}

LWMsgStatus
lwmsg_session_unregister_handle(
    LWMsgSession* session,
    LWMsgHandle* handle
    )
{
    return session->sclass->unregister_handle(session, handle);
}

LWMsgStatus
lwmsg_session_get_handle_data(
    LWMsgSession* session,
    LWMsgHandle* handle,
    void** data
    )
{
    return session->sclass->get_handle_data(session, handle, data);
}

LWMsgStatus
lwmsg_session_get_handle_location(
    LWMsgSession* session,
    LWMsgHandle* handle,
    LWMsgHandleType* location
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    status = session->sclass->resolve_handle_to_id(
        session,
        handle,
        NULL,
        location,
        NULL);

    return status;
}

void*
lwmsg_session_get_data(
    LWMsgSession* session
    )
{
    return session->sclass->get_data(session);
}

LWMsgSecurityToken*
lwmsg_session_get_peer_security_token(
    LWMsgSession* session
    )
{
    return session->sclass->get_peer_security_token(session);
}

LWMsgStatus
lwmsg_session_acquire_call(
    LWMsgSession* session,
    LWMsgCall** call
    )
{
    return session->sclass->acquire_call(session, call);
}

const LWMsgSessionID*
lwmsg_session_get_id(
    LWMsgSession* session
    )
{
    return session->sclass->get_id(session);
}
