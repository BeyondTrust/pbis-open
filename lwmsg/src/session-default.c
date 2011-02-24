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
    struct LWMsgHandle* handles;
    /* Number of handles */
    size_t num_handles;
    /* Links to other sessions in the manager */
    struct DefaultSession* next, *prev;
    /* User data pointer */
    void* data;
} DefaultSession;

struct LWMsgHandle
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
    struct LWMsgHandle *next, *prev;
};

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
    LWMsgHandle* entry
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
    LWMsgHandle** out_handle
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgHandle* handle = NULL;

    handle = calloc(1, sizeof(*handle));

    if (!handle)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    handle->type = type;
    handle->valid = LWMSG_TRUE;
    handle->refs = 1;
    handle->pointer = pointer;
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
    LWMsgHandle* handle, *next;

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

        session->refs = 1;

        if (DEFAULT_MANAGER(manager)->construct)
        {
            BAIL_ON_ERROR(status = DEFAULT_MANAGER(manager)->construct(
                              token,
                              DEFAULT_MANAGER(manager)->construct_data,
                              &session->data));
        }

        session->sec_token = token;

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
    LWMsgHandle** handle)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgHandle* my_handle = NULL;

    BAIL_ON_ERROR(status = default_add_handle(
                      DEFAULT_SESSION(session),
                      type,
                      LWMSG_HANDLE_REMOTE,
                      NULL,
                      hid,
                      cleanup,
                      &my_handle));

    if (handle)
    {
        *handle = my_handle;
    }

error:

    return status;
}

static
LWMsgStatus
default_register_handle_local(
    LWMsgSession* session,
    const char* type,
    void* data,
    void (*cleanup)(void* ptr),
    LWMsgHandle** handle
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultManager* priv = DEFAULT_MANAGER(session->manager);
    LWMsgHandle* my_handle = NULL;

    BAIL_ON_ERROR(status = default_add_handle(
                      DEFAULT_SESSION(session),
                      type,
                      LWMSG_HANDLE_LOCAL,
                      data,
                      priv->next_hid++,
                      cleanup,
                      &my_handle));

    if (handle)
    {
        *handle = my_handle;
    }

error:

    return status;
}

static
void
default_retain_handle(
    LWMsgSession* session,
    LWMsgHandle* handle
    )
{
    handle->refs++;
}

static
void
default_release_handle(
    LWMsgSession* session,
    LWMsgHandle* handle
    )
{
    DefaultSession* my_session = DEFAULT_SESSION(session);

    if (--handle->refs == 0)
    {
        if (handle == my_session->handles)
        {
            my_session->handles = my_session->handles->next;
        }
                
        default_free_handle(handle);
        my_session->num_handles--;
    }
}

static
LWMsgStatus
default_unregister_handle(
    LWMsgSession* session,
    LWMsgHandle* handle
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultSession* my_session = DEFAULT_SESSION(session);

    if (!my_session)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

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

done:

    return status;

error:
    
    goto done;
}

static
LWMsgStatus
default_resolve_handle_to_id(
    LWMsgSession* session,
    LWMsgHandle* handle,
    const char** type,
    LWMsgHandleType* htype,
    LWMsgHandleID* hid
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

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

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
default_resolve_id_to_handle(
    LWMsgSession* session,
    const char* type,
    LWMsgHandleType htype,
    LWMsgHandleID hid,
    LWMsgHandle** handle
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgHandle* my_handle = NULL;
    DefaultSession* my_session = DEFAULT_SESSION(session);

    for (my_handle = my_session->handles; my_handle; my_handle = my_handle->next)
    {
        if (my_handle->hid == hid && my_handle->locality == htype)
        {
            if (!my_handle->valid || (type && strcmp(type, my_handle->type)))
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);
            }
            
            *handle = my_handle;
            my_handle->refs++;
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
LWMsgStatus
default_get_handle_data(
    LWMsgSession* session,
    LWMsgHandle* handle,
    void** data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (!handle->valid)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);
    }

    if (data)
    {
        *data = handle->pointer;
    }

error:

    return status;
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
    .resolve_handle_to_id = default_resolve_handle_to_id,
    .resolve_id_to_handle = default_resolve_id_to_handle,
    .get_handle_data = default_get_handle_data,
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
