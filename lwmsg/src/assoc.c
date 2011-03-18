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
 *        assoc.c
 *
 * Abstract:
 *
 *        Association API
 *        Primary entry points
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include <config.h>
#include <string.h>
#include <lwmsg/data.h>
#include "assoc-private.h"
#include "util-private.h"
#include "protocol-private.h"
#include "session-private.h"
#include "data-private.h"
#include "buffer-private.h"

static LWMsgStatus
lwmsg_assoc_context_get_data(
    const char* key,
    void** out_data,
    void* data
    )
{
    if (!strcmp(key, "assoc"))
    {
        *out_data = data;
        return LWMSG_STATUS_SUCCESS;
    }
    else if (!strcmp(key, "session"))
    {
        return lwmsg_assoc_get_session(
            (LWMsgAssoc*) data,
            (LWMsgSession**)(void*) out_data);
    }
    else
    {
        return LWMSG_STATUS_NOT_FOUND;
    }
}

LWMsgStatus
lwmsg_assoc_new(
    const LWMsgContext* context,
    LWMsgProtocol* prot,
    LWMsgAssocClass* aclass,
    size_t size,
    LWMsgAssoc** out_assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    LWMsgAssoc* assoc = calloc(1, size);

    if (!assoc)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    assoc->prot = prot;
    assoc->aclass = aclass;

    lwmsg_context_setup(&assoc->context, context);
    lwmsg_context_set_data_function(&assoc->context, lwmsg_assoc_context_get_data, assoc);

    BAIL_ON_ERROR(status = lwmsg_assoc_call_init(&assoc->call));

    if (aclass->construct)
    {
        aclass->construct(assoc);
    }

    *out_assoc = assoc;

error:

    return status;
}

void
lwmsg_assoc_delete(
    LWMsgAssoc* assoc
    )
{
    lwmsg_context_cleanup(&assoc->context);
    if (assoc->aclass->destruct)
    {
        assoc->aclass->destruct(assoc);
    }

    free(assoc);
}

LWMsgProtocol*
lwmsg_assoc_get_protocol(
    LWMsgAssoc* assoc
    )
{
    return assoc->prot;
}

LWMsgStatus
lwmsg_assoc_send_message(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = assoc->aclass->send_msg(assoc, message));
    
error:
    
    return status;
}

LWMsgStatus
lwmsg_assoc_recv_message(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = assoc->aclass->recv_msg(assoc, message));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_send_message_transact(
    LWMsgAssoc* assoc,
    LWMsgMessage* send_message,
    LWMsgMessage* recv_message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = assoc->aclass->send_msg(assoc, send_message));
    BAIL_ON_ERROR(status = assoc->aclass->recv_msg(assoc, recv_message));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_get_session(
    LWMsgAssoc* assoc,
    LWMsgSession** session
    )
{
    return assoc->aclass->get_session(assoc, session);
}

LWMsgStatus
lwmsg_assoc_close(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = assoc->aclass->close(assoc));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_reset(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = assoc->aclass->reset(assoc));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_destroy_message(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpec* type = NULL;
    LWMsgDataContext* context = NULL;

    if (message->tag != -1)
    {
        BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(assoc->prot, message->tag, &type));

        if (type != NULL)
        {
            BAIL_ON_ERROR(status = lwmsg_data_context_new(&assoc->context, &context));
            BAIL_ON_ERROR(status = lwmsg_data_free_graph(context, type, message->data));
        }

        message->tag = -1;
        message->data = NULL;
    }

error:

    if (context)
    {
        lwmsg_data_context_delete(context);
    }

    return status;
}

LWMsgStatus
lwmsg_assoc_free_graph(
    LWMsgAssoc* assoc,
    LWMsgTag mtype,
    void* object
    );


LWMsgStatus
lwmsg_assoc_free_graph(
    LWMsgAssoc* assoc,
    LWMsgTag mtype,
    void* object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpec* type = NULL;
    LWMsgDataContext* context = NULL;

    BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(assoc->prot, mtype, &type));
    BAIL_ON_ERROR(status = lwmsg_data_context_new(&assoc->context, &context));
    BAIL_ON_ERROR(status = lwmsg_data_free_graph(context, type, object));

error:

    if (context)
    {
        lwmsg_data_context_delete(context);
    }

    return status;
}

LWMsgAssocState
lwmsg_assoc_get_state(
    LWMsgAssoc* assoc
    )
{
    return assoc->aclass->get_state(assoc);
}


LWMsgStatus
lwmsg_assoc_set_timeout(
    LWMsgAssoc* assoc,
    LWMsgTimeout type,
    LWMsgTime* value
    )
{
    return assoc->aclass->set_timeout(assoc, type, value);
}

LWMsgStatus
lwmsg_assoc_connect(
    LWMsgAssoc* assoc,
    LWMsgSession* session
    )
{
    return assoc->aclass->connect_peer(assoc, session);
}

LWMsgStatus
lwmsg_assoc_accept(
    LWMsgAssoc* assoc,
    LWMsgSession* session
    )
{
    return assoc->aclass->accept_peer(assoc, session);
}

LWMsgStatus
lwmsg_assoc_finish(
    LWMsgAssoc* assoc,
    LWMsgMessage** message
    )
{
    return assoc->aclass->finish(assoc, message);
}

LWMsgStatus
lwmsg_assoc_set_nonblock(
    LWMsgAssoc* assoc,
    LWMsgBool nonblock
    )
{
    return assoc->aclass->set_nonblock(assoc, nonblock);
}

static
LWMsgStatus
realloc_wrap(
    LWMsgBuffer* buffer,
    size_t count
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    const LWMsgContext* context = buffer->data;
    size_t offset = buffer->cursor - buffer->base;
    size_t length = buffer->end - buffer->base;
    size_t new_length = 0;
    unsigned char* new_buffer = NULL;

    if (count)
    {
        if (length == 0)
        {
            new_length = 256;
        }
        else
        {
            new_length = length * 2;
        }
        
        BAIL_ON_ERROR(status = lwmsg_context_realloc(
                          context,
                          buffer->base,
                          length,
                          new_length,
                          (void**) (void*) &new_buffer));
        
        buffer->base = new_buffer;
        buffer->end = new_buffer + new_length;
        buffer->cursor = new_buffer + offset;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_print_message_alloc(
    LWMsgAssoc* assoc,
    LWMsgMessage* message,
    char** result
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgDataContext* context = NULL;
    LWMsgTypeSpec* type = NULL;
    LWMsgBuffer buffer = {0};
    const char* tag_name = NULL;
    unsigned char nul = 0;

    buffer.wrap = realloc_wrap;
    buffer.data = (void*) &assoc->context;
    
    BAIL_ON_ERROR(status = lwmsg_data_context_new(&assoc->context, &context));
    BAIL_ON_ERROR(status = lwmsg_protocol_get_message_name(assoc->prot, message->tag, &tag_name));
    BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(assoc->prot, message->tag, &type));
    
    if (type)
    {
        BAIL_ON_ERROR(status = lwmsg_buffer_print(&buffer, "%s: ", tag_name));
        BAIL_ON_ERROR(status = lwmsg_data_print_graph(
                          context,
                          type,
                          message->data,
                          4,
                          &buffer));

        BAIL_ON_ERROR(status = lwmsg_buffer_write(&buffer, &nul, 1));
    }
    else
    {
        BAIL_ON_ERROR(status = lwmsg_buffer_print(&buffer, "%s", tag_name));
    }
    
    *result = (char*) buffer.base;

cleanup:

    if (context)
    {
        lwmsg_data_context_delete(context);
    }

    return status;

error:

    *result = NULL;

    if (buffer.base)
    {
        lwmsg_context_free(&assoc->context, buffer.base);
    }

    goto cleanup;
}

LWMsgStatus
lwmsg_assoc_acquire_call(
    LWMsgAssoc* assoc,
    LWMsgCall** call
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSession* session = NULL;

    if (assoc->call.in_use)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_BUSY);
    }
    else
    {
        switch (lwmsg_assoc_get_state(assoc))
        {
        case LWMSG_ASSOC_STATE_IDLE:
            break;
        case LWMSG_ASSOC_STATE_NOT_ESTABLISHED:
            BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, &session));
            BAIL_ON_ERROR(status = lwmsg_assoc_connect(assoc, session));
            break;
        default:
            BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
        }

        assoc->call.in_use = LWMSG_TRUE;
        *call = LWMSG_CALL(&assoc->call);
    }

error:

    return status;
}

