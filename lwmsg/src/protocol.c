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
 *        protocol.c
 *
 * Abstract:
 *
 *        Protocol specification and construction API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "protocol-private.h"
#include "util-private.h"
#include "type-private.h"
#include "data-private.h"

static LWMsgTypeSpec protocol_message_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgProtocolMessageRep),
    LWMSG_MEMBER_INT16(LWMsgProtocolMessageRep, tag),
    LWMSG_ATTR_RANGE(0, INT16_MAX),
    LWMSG_MEMBER_TYPESPEC(LWMsgProtocolMessageRep, type, lwmsg_type_rep_spec),
    LWMSG_MEMBER_PSTR(LWMsgProtocolMessageRep, name),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


static LWMsgTypeSpec protocol_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgProtocolRep),
    LWMSG_MEMBER_UINT16(LWMsgProtocolRep, message_count),
    LWMSG_MEMBER_POINTER(LWMsgProtocolRep, messages, LWMSG_TYPESPEC(protocol_message_rep_spec)),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_LENGTH_MEMBER(LWMsgProtocolRep, message_count),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec* lwmsg_protocol_rep_spec = protocol_rep_spec;

LWMsgStatus
lwmsg_protocol_new(
    LWMsgContext* context,
    LWMsgProtocol** prot
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    LWMsgProtocol* my_prot = calloc(1, sizeof(*my_prot));

    if (!my_prot)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    my_prot->context = context;

    *prot = my_prot;

done:

    return status;

error:

    if (my_prot)
    {
        free(my_prot);
    }

    goto done;
}

void
lwmsg_protocol_delete(LWMsgProtocol* prot)
{
    free(prot->types);
    free(prot);
}

LWMsgStatus
lwmsg_protocol_get_message_type(LWMsgProtocol* prot, unsigned int tag, LWMsgTypeSpec** out_type)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (tag >= prot->num_types)
    {
        RAISE_ERROR(prot, status = LWMSG_STATUS_NOT_FOUND,
                    "Unknown message type");
    }
    else
    {
        *out_type = prot->types[tag]->type;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_protocol_get_message_name(
    LWMsgProtocol* prot,
    unsigned int tag,
    const char** name
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (tag >= prot->num_types)
    {
        RAISE_ERROR(prot, status = LWMSG_STATUS_NOT_FOUND,
                    "Unknown message type");
    }
    else
    {
        *name = prot->types[tag]->tag_name;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_protocol_add_protocol_spec(LWMsgProtocol* prot, LWMsgProtocolSpec* spec)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgProtocolSpec** new_types = NULL;
    size_t num_types = 0;
    size_t i;


    for (i = 0; spec[i].tag != -1; i++)
    {
        if (spec[i].tag >= num_types)
        {
            num_types = spec[i].tag + 1;
        }
    }

    if (num_types > prot->num_types)
    {
        new_types = realloc(prot->types, sizeof(*new_types) * num_types);
        if (!new_types)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
        }

        memset(new_types + prot->num_types, 0, (num_types - prot->num_types) * sizeof(*new_types));

        prot->types = new_types;
        prot->num_types = num_types;
    }
    
    for (i = 0; spec[i].tag != -1; i++)
    {
        /* A NULL typespec indicates a message with an empty payload */
        prot->types[spec[i].tag] = &spec[i];
    }

error:

    return status;
}


const char*
lwmsg_protocol_get_error_message(
    LWMsgProtocol* prot,
    LWMsgStatus status
    )
{
    return lwmsg_error_message(status, &prot->error);
}

LWMsgStatus
lwmsg_protocol_create_representation(
    LWMsgDataContext* context,
    LWMsgProtocol* prot,
    LWMsgProtocolRep** rep
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeRepMap map = {0};
    size_t i = 0;
    LWMsgProtocolRep* my_rep = NULL;
    LWMsgProtocolMessageRep* message = NULL;
    LWMsgTypeIter iter;

    map.context = context->context;

    BAIL_ON_ERROR(status = lwmsg_data_alloc_memory(
                      context,
                      sizeof(*my_rep),
                      (void**) (void*) &my_rep));

    BAIL_ON_ERROR(status = lwmsg_data_alloc_memory(
                      context,
                      sizeof(*my_rep->messages) * prot->num_types,
                      (void**) (void*) &my_rep->messages));

    for (i = 0; i < prot->num_types; i++)
    {
        if (prot->types[i])
        {
            message = &my_rep->messages[my_rep->message_count++];

            message->tag = prot->types[i]->tag;

            if (prot->types[i]->type)
            {
                lwmsg_type_iterate(prot->types[i]->type, &iter);

                BAIL_ON_ERROR(status = lwmsg_type_rep_from_spec_internal(
                                  &map,
                                  &iter,
                                  &message->type));
            }

            lwmsg_strdup(
                context->context,
                prot->types[i]->tag_name,
                &message->name);
        }
    }

    *rep = my_rep;

done:

    return status;

error:

    if (my_rep)
    {
        lwmsg_data_free_graph(context, protocol_rep_spec, my_rep);
    }

    goto done;
}
