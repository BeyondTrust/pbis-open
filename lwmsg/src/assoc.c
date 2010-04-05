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

#ifdef LWMSG_DISABLE_DEPRECATED

LWMsgStatus
lwmsg_assoc_send(
    LWMsgAssoc* assoc,
    LWMsgTag type,
    void* object
    );

LWMsgStatus
lwmsg_assoc_recv(
    LWMsgAssoc* assoc,
    LWMsgTag* type,
    void** object
    );

LWMsgStatus
lwmsg_assoc_send_transact(
    LWMsgAssoc* assoc,
    LWMsgTag in_type,
    void* in_object,
    LWMsgTag* out_type,
    void** out_object
    );

#endif

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
lwmsg_assoc_recv_message_transact(
    LWMsgAssoc* assoc,
    LWMsgAssocDispatchFunction dispatch,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMessage recv_message = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage send_message = LWMSG_MESSAGE_INITIALIZER;

    BAIL_ON_ERROR(status = assoc->aclass->recv_msg(assoc, &recv_message));
    BAIL_ON_ERROR(status = dispatch(assoc, &recv_message, &send_message, data));
    BAIL_ON_ERROR(status = assoc->aclass->send_msg(assoc, &send_message));

error:
    
    if (recv_message.tag != -1 && recv_message.data)
    {
        lwmsg_assoc_destroy_message(assoc, &recv_message);
    }

    if (send_message.tag != -1 && send_message.data)
    {
        lwmsg_assoc_destroy_message(assoc, &send_message);
    }

    return status;
}

LWMsgStatus
lwmsg_assoc_send(
    LWMsgAssoc* assoc,
    LWMsgTag type,
    void* object
    )
{
    LWMsgMessage message = LWMSG_MESSAGE_INITIALIZER;

    message.tag = type;
    message.data = object;

    return lwmsg_assoc_send_message(assoc, &message);
}

LWMsgStatus
lwmsg_assoc_recv(
    LWMsgAssoc* assoc,
    LWMsgTag* out_type,
    void** out_object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMessage message = LWMSG_MESSAGE_INITIALIZER;

    BAIL_ON_ERROR(status = lwmsg_assoc_recv_message(assoc, &message));

    *out_type = message.tag;
    *out_object = message.data;

error:

    return status;
}


LWMsgStatus
lwmsg_assoc_send_transact(
    LWMsgAssoc* assoc,
    LWMsgTag in_type,
    void* in_object,
    LWMsgTag* out_type,
    void** out_object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMessage in_message = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage out_message = LWMSG_MESSAGE_INITIALIZER;

    in_message.tag = in_type;
    in_message.data = in_object;

    BAIL_ON_ERROR(status = lwmsg_assoc_send_message_transact(assoc, &in_message, &out_message));

    *out_type = out_message.tag;
    *out_object = out_message.data;

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

const char*
lwmsg_assoc_get_error_message(
    LWMsgAssoc* assoc,
    LWMsgStatus status
    )
{
    return lwmsg_context_get_error_message(&assoc->context, status);
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
    return assoc->aclass->connect(assoc, session);
}

LWMsgStatus
lwmsg_assoc_accept(
    LWMsgAssoc* assoc,
    LWMsgSessionManager* manager,
    LWMsgSession** session
    )
{
    return assoc->aclass->accept(assoc, manager, session);
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
    char* payload_result = NULL;
    char* my_result = NULL;
    const char* tag_name = NULL;
    
    BAIL_ON_ERROR(status = lwmsg_data_context_new(&assoc->context, &context));
    BAIL_ON_ERROR(status = lwmsg_protocol_get_message_name(assoc->prot, message->tag, &tag_name));
    BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(assoc->prot, message->tag, &type));

    if (type)
    {
        BAIL_ON_ERROR(status = lwmsg_data_print_graph_alloc(context, type, message->data, &payload_result));
        
        my_result = lwmsg_format("%s: %s", tag_name, payload_result);
    }
    else
    {
        my_result = lwmsg_format("%s", tag_name);
    }

    if (!my_result)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    *result = my_result;

cleanup:

    if (context)
    {
        lwmsg_data_context_delete(context);
    }

    if (payload_result)
    {
        lwmsg_context_free(&assoc->context, payload_result);
    }

    return status;

error:

    *result = NULL;

    if (my_result)
    {
        lwmsg_context_free(&assoc->context, my_result);
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

