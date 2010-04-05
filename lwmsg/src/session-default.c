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
 *        session-default.c
 *
 * Abstract:
 *
 *        Session management API
 *        Default session manager implementation
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "session-private.h"
#include "util-private.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct DefaultSession
{
    LWMsgSession base;
    /* Session identifier */
    LWMsgSessionID id;
    /* Security token of session creator */
    LWMsgSecurityToken* sec_token;
    /* Reference count */
    size_t refs;
    /* Pointer to linked list of handles */
    struct DefaultHandle* handles;
    /* Number of handles */
    size_t num_handles;
    /* Links to other sessions in the manager */
    struct DefaultSession* next, *prev;
    /* User data pointer */
    void* data;
} DefaultSession;

typedef struct DefaultHandle
{
    /* Handle type */
    const char* type;
    /* Handle refcount */
    size_t refs;
    /* Validity bit */
    LWMsgBool valid;
    /* Handle pointer */
    void* pointer;
    /* Handle locality */
    LWMsgHandleType locality;
    /* Handle id */
    unsigned long hid;
    /* Handle cleanup function */
    void (*cleanup)(void*);
    /* Links to other handles in the session */
    struct DefaultHandle* next, *prev;
} DefaultHandle;

typedef struct DefaultManager
{
    LWMsgSessionManager base;
    DefaultSession* sessions;
    unsigned long next_hid;
    LWMsgSessionConstructFunction construct;
    LWMsgSessionDestructFunction destruct;
    void* construct_data;
} DefaultManager;

#define DEFAULT_MANAGER(obj) ((DefaultManager*) (obj))
#define DEFAULT_SESSION(obj) ((DefaultSession*) (obj))

static void
default_free_handle(
    DefaultHandle* entry
    )
{
    if (entry->cleanup)
    {
        entry->cleanup(entry->pointer);
    }

    if (entry->prev)
    {
        entry->prev->next = entry->next;
    }
    
    if (entry->next)
    {
        entry->next->prev = entry->prev;
    }

    free(entry);
}

static
LWMsgStatus
default_add_handle(
    DefaultSession* session,
    const char* type,
    LWMsgHandleType locality,
    void* pointer,
    unsigned long hid,
    void (*cleanup)(void*),
    DefaultHandle** out_handle
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultHandle* handle = NULL;

    handle = calloc(1, sizeof(*handle));

    if (!handle)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    handle->type = type;
    handle->valid = LWMSG_TRUE;
    handle->refs = 1;
    handle->pointer = pointer ? pointer : handle;
    handle->cleanup = cleanup;
    handle->hid = hid;
    handle->locality = locality;

    handle->next = session->handles;

    if (session->handles)
    {
        session->handles->prev = handle;
    }

    session->handles = handle;
    session->num_handles++;

    *out_handle = handle;

error:

    return status;
}

static
DefaultSession*
default_find_session(
    DefaultManager* priv,
    const LWMsgSessionCookie* connect
    )
{
    DefaultSession* entry = NULL;

    for (entry = priv->sessions; entry; entry = entry->next)
    {
        if (!memcmp(connect->bytes, entry->id.connect.bytes, sizeof(connect->bytes)))
        {
            return entry;
        }
    }

    return NULL;
}

static
void
default_free_session(
    DefaultSession* session
    )
{
    DefaultHandle* handle, *next;

    for (handle = session->handles; handle; handle = next)
    {
        next = handle->next;
        default_free_handle(handle);
    }

    if (DEFAULT_MANAGER(session->base.manager)->destruct && session->data)
    {
        DEFAULT_MANAGER(session->base.manager)->destruct(session->sec_token, session->data);
    }

    if (session->sec_token)
    {
        lwmsg_security_token_delete(session->sec_token);
    }

    free(session);
}

static
void
default_delete(
    LWMsgSessionManager* manager
    )
{
    DefaultManager* priv = DEFAULT_MANAGER(manager);
    DefaultSession* entry, *next;

    for (entry = priv->sessions; entry; entry = next)
    {
        next = entry->next;

        default_free_session(entry);
    }

    free(manager);
}

static
LWMsgStatus
default_create(
    LWMsgSessionManager* manager,
    LWMsgSession** out_session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultSession* session = NULL;

    session = calloc(1, sizeof(*session));
    if (!session)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }
    
    session->base.manager = manager;
    session->refs = 1;

    lwmsg_session_generate_cookie(&session->id.connect);

    *out_session = LWMSG_SESSION(session);

error:

    return status;
}

static
LWMsgStatus
default_accept(
    LWMsgSessionManager* manager,
    const LWMsgSessionCookie* connect,
    LWMsgSecurityToken* token,
    LWMsgSession** out_session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultManager* priv = DEFAULT_MANAGER(manager);
    DefaultSession* session = default_find_session(priv, connect);
    
    if (session)
    {
        if (!session->sec_token || !lwmsg_security_token_can_access(session->sec_token, token))
        {
            session = NULL;
            BAIL_ON_ERROR(status = LWMSG_STATUS_SECURITY);
        }

        session->refs++;
        lwmsg_security_token_delete(token);
    }
    else
    {
        session = calloc(1, sizeof(*session));
        if (!session)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
        }

        session->base.manager = manager;

        memcpy(session->id.connect.bytes, connect->bytes, sizeof(connect->bytes));
        lwmsg_session_generate_cookie(&session->id.accept);

        session->sec_token = token;
        session->refs = 1;

        if (DEFAULT_MANAGER(manager)->construct)
        {
            BAIL_ON_ERROR(status = DEFAULT_MANAGER(manager)->construct(
                              session->sec_token,
                              DEFAULT_MANAGER(manager)->construct_data,
                              &session->data));
        }

        if (priv->sessions)
        {
            priv->sessions->prev = session;
        }

        session->next = priv->sessions;
        priv->sessions = session;
    }

    *out_session = LWMSG_SESSION(session);

done:

    return status;

error:

    if (session)
    {
        default_free_session(session);
    }

    goto done;
}

static
LWMsgStatus
default_connect(
    LWMsgSession* session,
    const LWMsgSessionCookie* accept,
    LWMsgSecurityToken* token
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultSession* dsession = DEFAULT_SESSION(session);

    if (dsession->sec_token)
    {
        /* Session has been connected before */
        if (!lwmsg_security_token_can_access(dsession->sec_token, token))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_SECURITY);
        }
    
        /* Has session state been lost? */
        if (memcmp(&dsession->id.accept.bytes, accept->bytes, sizeof(accept->bytes)) &&
            dsession->num_handles > 0)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_SESSION_LOST);
        }

        lwmsg_security_token_delete(token);
        /* Update accept cookie if it changed */
        memcpy(dsession->id.accept.bytes, accept->bytes, sizeof(accept->bytes));
    }
    else
    {
        /* Session has not been connected bfore */
        dsession->sec_token = token;
        memcpy(dsession->id.accept.bytes, accept->bytes, sizeof(accept->bytes));
    }

    dsession->refs++;

done:

    return status;

error:

    goto done;
}

static
void
default_release(
    LWMsgSession* session
    )
{
    DefaultManager* priv = DEFAULT_MANAGER(session->manager);
    DefaultSession* my_session = DEFAULT_SESSION(session);

    my_session->refs--;
    
    if (my_session->refs == 0)
    {
        if (priv->sessions == my_session)
        {
            priv->sessions = priv->sessions->next;
        }

        if (my_session->next)
        {
            my_session->next->prev = my_session->prev;
        }
        
        if (my_session->prev)
        {
            my_session->prev->next = my_session->next;
        }

        default_free_session(my_session);
    }
}

static
LWMsgSecurityToken*
default_get_peer_security_token (
    LWMsgSession* session
    )
{
    DefaultSession* my_session = DEFAULT_SESSION(session);

    return my_session->sec_token;
}

static
LWMsgStatus
default_register_handle_remote(
    LWMsgSession* session,
    const char* type,
    LWMsgHandleID hid,
    void (*cleanup)(void* ptr),
    void** ptr)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultHandle* handle = NULL;

    BAIL_ON_ERROR(status = default_add_handle(
                      DEFAULT_SESSION(session),
                      type,
                      LWMSG_HANDLE_REMOTE,
                      NULL,
                      hid,
                      cleanup,
                      &handle));

    if (ptr)
    {
        *ptr = handle->pointer;
    }

error:

    return status;
}

static
LWMsgStatus
default_register_handle_local(
    LWMsgSession* session,
    const char* type,
    void* pointer,
    void (*cleanup)(void* ptr),
    LWMsgHandleID* hid
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultManager* priv = DEFAULT_MANAGER(session->manager);
    DefaultHandle* handle = NULL;

    BAIL_ON_ERROR(status = default_add_handle(
                      DEFAULT_SESSION(session),
                      type,
                      LWMSG_HANDLE_LOCAL,
                      pointer,
                      priv->next_hid++,
                      cleanup,
                      &handle));

    if (hid)
    {
        *hid = handle->hid;
    }

error:

    return status;
}

static
LWMsgStatus
default_retain_handle(
    LWMsgSession* session,
    void* ptr
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultHandle* handle = NULL;
    DefaultSession* my_session = DEFAULT_SESSION(session);

    if (!my_session)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    for (handle = my_session->handles; handle; handle = handle->next)
    {
        if (handle->pointer == ptr)
        {
            handle->refs++;
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);

done:

    return status;

error:
    
    goto done;
}

static
LWMsgStatus
default_release_handle(
    LWMsgSession* session,
    void* ptr
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultHandle* handle = NULL;
    DefaultSession* my_session = DEFAULT_SESSION(session);

    if (!my_session)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    for (handle = my_session->handles; handle; handle = handle->next)
    {
        if (handle->pointer == ptr)
        {
            if (--handle->refs == 0)
            {
                if (handle == my_session->handles)
                {
                    my_session->handles = my_session->handles->next;
                }
                
                default_free_handle(handle);
                my_session->num_handles--;
            }
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);

done:

    return status;

error:
    
    goto done;
}

static
LWMsgStatus
default_unregister_handle(
    LWMsgSession* session,
    void* ptr
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultHandle* handle = NULL;
    DefaultSession* my_session = DEFAULT_SESSION(session);

    if (!my_session)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    for (handle = my_session->handles; handle; handle = handle->next)
    {
        if (handle->pointer == ptr)
        {
            if (!handle->valid)
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);
            }

            handle->valid = LWMSG_FALSE;

            if (--handle->refs == 0)
            {
                if (handle == my_session->handles)
                {
                    my_session->handles = my_session->handles->next;
                }
                
                default_free_handle(handle);
                my_session->num_handles--;
            }
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);

done:

    return status;

error:
    
    goto done;
}

static
LWMsgStatus
default_handle_pointer_to_id(
    LWMsgSession* session,
    void* pointer,
    const char** type,
    LWMsgHandleType* htype,
    LWMsgHandleID* hid
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultHandle* handle = NULL;
    DefaultSession* my_session = DEFAULT_SESSION(session);

    for (handle = my_session->handles; handle; handle = handle->next)
    {
        if (handle->pointer == pointer)
        {
            if (!handle->valid)
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);
            }

            if (type)
            {
                *type = handle->type;
            }
            if (htype)
            {
                *htype = handle->locality;
            }
            if (hid)
            {
                *hid = handle->hid;
            }
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
default_handle_id_to_pointer(
    LWMsgSession* session,
    const char* type,
    LWMsgHandleType htype,
    LWMsgHandleID hid,
    void** pointer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultHandle* handle = NULL;
    DefaultSession* my_session = DEFAULT_SESSION(session);

    for (handle = my_session->handles; handle; handle = handle->next)
    {
        if (handle->hid == hid && handle->locality == htype)
        {
            if (!handle->valid || (type && strcmp(type, handle->type)))
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);
            }
            
            *pointer = handle->pointer;
            handle->refs++;
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);

done:

    return status;

error:

    goto done;
}

static
void*
default_get_data (
    LWMsgSession* session
    )
{
    return DEFAULT_SESSION(session)->data;
}

static
const LWMsgSessionID*
default_get_id(
    LWMsgSession* session
    )
{
    return &DEFAULT_SESSION(session)->id;
}

static
size_t
default_get_assoc_count(
    LWMsgSession* session
    )
{
    return DEFAULT_SESSION(session)->refs;
}

static
size_t
default_get_handle_count(
    LWMsgSession* session
    )
{
    return DEFAULT_SESSION(session)->num_handles;
}

static LWMsgSessionManagerClass default_class = 
{
    .delete = default_delete,
    .create = default_create,
    .accept = default_accept,
    .connect = default_connect,
    .release = default_release,
    .register_handle_local = default_register_handle_local,
    .register_handle_remote = default_register_handle_remote,
    .retain_handle = default_retain_handle,
    .release_handle = default_release_handle,
    .unregister_handle = default_unregister_handle,
    .handle_pointer_to_id = default_handle_pointer_to_id,
    .handle_id_to_pointer = default_handle_id_to_pointer,
    .get_data = default_get_data,
    .get_id = default_get_id,
    .get_assoc_count = default_get_assoc_count,
    .get_handle_count = default_get_handle_count,
    .get_peer_security_token = default_get_peer_security_token
};

LWMsgStatus
lwmsg_default_session_manager_new(
    LWMsgSessionConstructFunction construct,
    LWMsgSessionDestructFunction destruct,
    void* construct_data,
    LWMsgSessionManager** manager
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultManager* my_manager = NULL;
    
    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_manager));

    my_manager->construct = construct;
    my_manager->destruct = destruct;
    my_manager->construct_data = construct_data;

    BAIL_ON_ERROR(status = lwmsg_session_manager_init(&my_manager->base, &default_class));

    *manager = LWMSG_SESSION_MANAGER(my_manager);

done:

    return status;

error:

    if (my_manager)
    {
        free(my_manager);
    }

    goto done;
}
