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
 *        assoc-marshal.c
 *
 * Abstract:
 *
 *        Association API
 *        Marshalling logic for association-specific datatypes
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "config.h"
#include <lwmsg/type.h>
#include <lwmsg/assoc.h>
#include "convert-private.h"
#include "util-private.h"
#include "assoc-private.h"
#include "data-private.h"
#include "buffer-private.h"

#include <stdlib.h>
#include <string.h>

typedef union
{
    LWMsgHandleID local_id;
    LWMsgHandleID remote_id;
} LWMsgHandleDataRep;

typedef struct
{
    LWMsgHandleType type;
    LWMsgHandleDataRep data;
} LWMsgHandleRep;

static LWMsgStatus
lwmsg_assoc_marshal_handle(
    LWMsgDataContext* mcontext,
    LWMsgTypeAttrs* attrs,
    void* object,
    void* transmit_object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    void* handle = NULL;
    LWMsgHandleRep* transmit = transmit_object;
    LWMsgSession* session = NULL;
    const char* type = NULL;
    const LWMsgContext* context = lwmsg_data_context_get_context(mcontext);

    BAIL_ON_ERROR(status = lwmsg_context_get_data(context, "session", (void**) (void*) &session));

    handle = *(LWMsgHandle**) object;
    
    if (handle != NULL)
    {
        status = session->sclass->resolve_handle_to_id(
            session,
            handle,
            &type,
            &transmit->type,
            &transmit->data.local_id);

        switch (status)
        {
        case LWMSG_STATUS_INVALID_HANDLE:
            status = DATA_RAISE(
                mcontext,
                ITER_FROM_ATTRS(attrs),
                LWMSG_STATUS_INVALID_HANDLE,
                "Handle 0x%lx is invalid",
                (unsigned long) handle);
        default:
            break;
        }

        BAIL_ON_ERROR(status);

        /* Confirm that the handle is of the expected type */
        if (strcmp((const char*) data, type))
        {
            BAIL_ON_ERROR(status = DATA_RAISE(
                mcontext,
                ITER_FROM_ATTRS(attrs),
                LWMSG_STATUS_INVALID_HANDLE,
                "Handle 0x%lx has wrong data type: expected '%s', got '%s'",
                (unsigned long) handle,
                (const char*) data,
                type));
        }
        
        /* Confirm that handle origin is correct according to type attributes */
        if ((attrs->custom & LWMSG_ASSOC_HANDLE_LOCAL_FOR_RECEIVER && transmit->type != LWMSG_HANDLE_REMOTE) ||
            (attrs->custom & LWMSG_ASSOC_HANDLE_LOCAL_FOR_SENDER && transmit->type != LWMSG_HANDLE_LOCAL))
        {
            if (transmit->type == LWMSG_HANDLE_LOCAL)
            {
                BAIL_ON_ERROR(status = DATA_RAISE(
                    mcontext,
                    ITER_FROM_ATTRS(attrs),
                    LWMSG_STATUS_INVALID_HANDLE,
                    "Handle 0x%lx is not local for receiver as expected",
                    (unsigned long) handle));
            }
            else
            {
                BAIL_ON_ERROR(status = DATA_RAISE(
                    mcontext,
                    ITER_FROM_ATTRS(attrs),
                    LWMSG_STATUS_INVALID_HANDLE,
                    "Handle 0x%lx is not local for sender as expected",
                    (unsigned long) handle));
            }
        }
    }
    else
    {
        /* If the handle was not supposed to be NULL, raise an error */
        if (attrs->flags & LWMSG_TYPE_FLAG_NOT_NULL)
        {
            BAIL_ON_ERROR(status = DATA_RAISE(
                mcontext,
                ITER_FROM_ATTRS(attrs),
                LWMSG_STATUS_INVALID_HANDLE,
                "Handle is NULL where non-null expected",
                (unsigned long) handle));
        }

        transmit->type = LWMSG_HANDLE_NULL;
    }

cleanup:

    return status;

error:

    goto cleanup;
}

static LWMsgStatus
lwmsg_assoc_unmarshal_handle(
    LWMsgDataContext* mcontext,
    LWMsgTypeAttrs* attrs,
    void* transmit_object,
    void* object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgHandle* handle = NULL;
    LWMsgHandleRep* transmit = transmit_object;
    LWMsgSession* session = NULL;
    const LWMsgContext* context = lwmsg_data_context_get_context(mcontext);
    LWMsgHandleType location = transmit->type;

    BAIL_ON_ERROR(status = lwmsg_context_get_data(context, "session", (void**) (void*) &session));
       
    if (location != LWMSG_HANDLE_NULL)
    {
        /* Invert sense of handle location */
        if (location == LWMSG_HANDLE_LOCAL)
        {
            location = LWMSG_HANDLE_REMOTE;
        }
        else
        {
            location = LWMSG_HANDLE_LOCAL;
        }

        if ((attrs->custom & LWMSG_ASSOC_HANDLE_LOCAL_FOR_RECEIVER && location != LWMSG_HANDLE_LOCAL) ||
            (attrs->custom & LWMSG_ASSOC_HANDLE_LOCAL_FOR_SENDER && location != LWMSG_HANDLE_REMOTE))
        {
            if (location == LWMSG_HANDLE_LOCAL)
            {
                BAIL_ON_ERROR(status = DATA_RAISE(
                    mcontext,
                    ITER_FROM_ATTRS(attrs),
                    LWMSG_STATUS_INVALID_HANDLE,
                    "Handle is not local for receiver as expected",
                    (unsigned long) handle));
            }
            else
            {
                BAIL_ON_ERROR(status = DATA_RAISE(
                     mcontext,
                     ITER_FROM_ATTRS(attrs),
                     LWMSG_STATUS_INVALID_HANDLE,
                     "Handle is not local for sender as expected",
                     (unsigned long) handle));
            }
        }

        /* Look up the handle */
        status = session->sclass->resolve_id_to_handle(
            session,
            (const char*) data,
            location,
            transmit->data.local_id,
            &handle);

        switch (status)
        {
        case LWMSG_STATUS_NOT_FOUND:
            if (location == LWMSG_HANDLE_REMOTE)
            {
                /* Implicitly register handle seen from the peer for the first time */
                BAIL_ON_ERROR(status = session->sclass->register_handle_remote(
                                  session,
                                  (const char*) data,
                                  transmit->data.local_id,
                                  NULL,
                                  &handle));
            }
            else
            {
                BAIL_ON_ERROR(status = DATA_RAISE(
                     mcontext,
                     ITER_FROM_ATTRS(attrs),
                     LWMSG_STATUS_INVALID_HANDLE,
                     "Invalid handle ID (%lu)",
                     (unsigned long) transmit->data.local_id));
            }
            break;
        default:
            BAIL_ON_ERROR(status);    
        }
        
        /* Set pointer on unmarshalled object */
        *(LWMsgHandle**) object = handle;
    }
    else
    {
        if (attrs->flags & LWMSG_TYPE_FLAG_NOT_NULL)
        {
            MARSHAL_RAISE_ERROR(mcontext, status = LWMSG_STATUS_INVALID_HANDLE,
                        "Invalid handle: expected non-null handle");
        }
        *(void**) object = NULL;
    }

error:

    return status;
}

static
void
lwmsg_assoc_free_handle(
    LWMsgDataContext* context,
    LWMsgTypeAttrs* attr,
    void* object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgHandle* handle = NULL;
    LWMsgSession* session = NULL;
    
    BAIL_ON_ERROR(status = lwmsg_context_get_data(
        lwmsg_data_context_get_context(context),
        "session",
        (void**) (void*) &session));

    handle = *(LWMsgHandle**) object;

    if (handle)
    {
        lwmsg_session_release_handle(session, handle);
    }

error:

    return;
}

static LWMsgStatus
lwmsg_assoc_print_handle(
    LWMsgDataContext* context, 
    LWMsgTypeAttrs* attrs,
    void* object,
    void* data,
    LWMsgBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgHandle* handle = NULL;
    LWMsgHandleType location;
    LWMsgHandleID id;
    LWMsgAssoc* assoc = NULL;
    LWMsgSession* session = NULL;
    const char* type;
    char* str = NULL;

    BAIL_ON_ERROR(status = lwmsg_context_get_data(context->context, "assoc", (void**) (void*) &assoc));

    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, &session));

    handle = *(LWMsgHandle**) object;

    if (handle != NULL)
    {
        status = session->sclass->resolve_handle_to_id(
            session,
            handle,
            &type,
            &location,
            &id);
        
        switch (status)
        {
        case LWMSG_STATUS_NOT_FOUND:
            status = LWMSG_STATUS_INVALID_HANDLE;
        default:
            break;
        }

        BAIL_ON_ERROR(status);

        /* Confirm that the handle is of the expected type */
        if (strcmp((const char*) data, type))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);
        }

        str = lwmsg_format("<%s:%s[%lu]>",
                           type,
                           location == LWMSG_HANDLE_LOCAL ? "local" : "remote",
                           id);

        BAIL_ON_ERROR(status = lwmsg_buffer_write(buffer, (unsigned char*) str, strlen(str)));
    }
    else
    {
        static const char* nullstr = "<null>";
        if (attrs->flags & LWMSG_TYPE_FLAG_NOT_NULL)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);
        }

        BAIL_ON_ERROR(status = lwmsg_buffer_write(buffer, (unsigned char*) nullstr, strlen(nullstr)));
    }

error:

    if (str)
    {
        free(str);
    }

    return status;
}

static LWMsgTypeSpec handle_enum_spec[] =
{
    LWMSG_ENUM_BEGIN(LWMsgHandleType, 1, LWMSG_UNSIGNED),
    LWMSG_ENUM_VALUE(LWMSG_HANDLE_NULL),
    LWMSG_ENUM_VALUE(LWMSG_HANDLE_LOCAL),
    LWMSG_ENUM_VALUE(LWMSG_HANDLE_REMOTE),
    LWMSG_ENUM_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec handle_data_spec[] =
{
    LWMSG_UNION_BEGIN(LWMsgHandleDataRep),
    LWMSG_MEMBER_UINT32(LWMsgHandleDataRep, local_id),
    LWMSG_ATTR_TAG(LWMSG_HANDLE_LOCAL),
    LWMSG_MEMBER_UINT32(LWMsgHandleDataRep, remote_id),
    LWMSG_ATTR_TAG(LWMSG_HANDLE_REMOTE),
    LWMSG_MEMBER_VOID(LWMsgHandleDataRep, null_id),
    LWMSG_ATTR_TAG(LWMSG_HANDLE_NULL),
    LWMSG_UNION_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec handle_id_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgHandleRep),
    LWMSG_MEMBER_TYPESPEC(LWMsgHandleRep, type, handle_enum_spec),
    LWMSG_MEMBER_TYPESPEC(LWMsgHandleRep, data, handle_data_spec),
    LWMSG_ATTR_DISCRIM(LWMsgHandleRep, type),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeClass lwmsg_handle_type_class =
{
    .is_pointer = LWMSG_TRUE,
    .transmit_type = handle_id_spec,
    .marshal = lwmsg_assoc_marshal_handle,
    .unmarshal = lwmsg_assoc_unmarshal_handle,
    .destroy_presented = lwmsg_assoc_free_handle,
    .destroy_transmitted = NULL, /* Nothing to free in transmitted form */
    .print = lwmsg_assoc_print_handle
};
