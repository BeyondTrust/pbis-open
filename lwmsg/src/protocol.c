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

#include "config.h"
#include "protocol-private.h"
#include "util-private.h"
#include "type-private.h"
#include "data-private.h"
#include "buffer-private.h"

#include <limits.h>

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
    lwmsg_memlist_init(&my_prot->specmem, context);

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
    lwmsg_memlist_destroy(&prot->specmem);
    
    free(prot->types);
    free(prot);
}

LWMsgStatus
lwmsg_protocol_get_message_type(
    LWMsgProtocol* prot,
    LWMsgTag tag,
    LWMsgTypeSpec** out_type)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (tag >= prot->num_types)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    *out_type = prot->types[tag]->type;

error:

    return status;
}

LWMsgStatus
lwmsg_protocol_get_message_name(
    LWMsgProtocol* prot,
    LWMsgTag tag,
    const char** name
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (tag >= prot->num_types)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    *name = prot->types[tag]->tag_name;

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

LWMsgStatus
lwmsg_protocol_get_protocol_rep(
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

    map.context = prot->context;

    BAIL_ON_ERROR(status = LWMSG_CONTEXT_ALLOC(prot->context, &my_rep));

    BAIL_ON_ERROR(status = LWMSG_CONTEXT_ALLOC_ARRAY(prot->context, prot->num_types, &my_rep->messages));

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

            BAIL_ON_ERROR(status = lwmsg_strdup(
                              prot->context,
                              prot->types[i]->tag_name,
                              &message->name));
          }
    }

    *rep = my_rep;

done:

    lwmsg_type_rep_map_destroy(&map);

    return status;

error:

    if (my_rep)
    {
        lwmsg_data_free_graph_cleanup(prot->context, protocol_rep_spec, my_rep);
    }

    goto done;
}

LWMsgStatus
lwmsg_protocol_add_protocol_rep(
    LWMsgProtocol* prot,
    LWMsgProtocolRep* rep
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpecBuffer* buffer = NULL;
    LWMsgTypeSpecMap specmap = {0};
    struct LWMsgProtocolSpec *spec = NULL;
    LWMsgTypeSpec* existing_type = NULL;
    LWMsgTypeRep* existing_rep = NULL;
    size_t i = 0;
    size_t next = 0;

    specmap.context = lwmsg_memlist_context(&prot->specmem);

    BAIL_ON_ERROR(status = LWMSG_CONTEXT_ALLOC_ARRAY(specmap.context, rep->message_count + 1, &spec));

    for (i = 0; i < rep->message_count; i++)
    {
        status = lwmsg_protocol_get_message_type(prot, rep->messages[i].tag, &existing_type);
        if (status == LWMSG_STATUS_NOT_FOUND)
        {
            /* Unknown tag -- convert type rep to a spec for addition to the protocol */
            BAIL_ON_ERROR(status = lwmsg_type_spec_from_rep_internal(
                              &specmap,
                              rep->messages[i].type,
                              &buffer));
            
            spec[next].tag = rep->messages[i].tag;
            spec[next].type = buffer->buffer;
            BAIL_ON_ERROR(status = lwmsg_strdup(
                              specmap.context,
                              rep->messages[i].name,
                              (char**) (void*) &spec[next].tag_name));
            next++;
        }
        else
        {
            BAIL_ON_ERROR(status);
            /* Known tag -- check for assignability with existing type */
            
            BAIL_ON_ERROR(status = lwmsg_type_rep_from_spec(
                              prot->context,
                              existing_type,
                              &existing_rep));

            BAIL_ON_ERROR(status = lwmsg_type_rep_is_assignable(existing_rep, rep->messages[i].type));

            lwmsg_type_free_rep(prot->context, existing_rep);
            existing_rep = NULL;
        }
    }

    spec[next].tag = -1;
    spec[next].type = NULL;
    spec[next].tag_name = NULL;

    BAIL_ON_ERROR(status = lwmsg_protocol_add_protocol_spec(
                          prot,
                          spec));
    
error:

    lwmsg_type_spec_map_destroy(&specmap);

    if (existing_rep)
    {
        lwmsg_type_free_rep(prot->context, existing_rep);
        existing_rep = NULL;
    }

    return status;
}

LWMsgStatus
lwmsg_protocol_is_protocol_rep_compatible(
    LWMsgProtocol* prot,
    LWMsgProtocolRep* rep
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpec* existing_type = NULL;
    LWMsgTypeRep* existing_rep = NULL;
    size_t i = 0;

    for (i = 0; i < rep->message_count; i++)
    {
        status = lwmsg_protocol_get_message_type(prot, rep->messages[i].tag, &existing_type);
        if (status == LWMSG_STATUS_NOT_FOUND)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }
        else
        {
            BAIL_ON_ERROR(status);
            /* Known tag -- check for assignability with existing type */
            
            BAIL_ON_ERROR(status = lwmsg_type_rep_from_spec(
                              prot->context,
                              existing_type,
                              &existing_rep));

            BAIL_ON_ERROR(status = lwmsg_type_rep_is_assignable(existing_rep, rep->messages[i].type));

            lwmsg_type_free_rep(prot->context, existing_rep);
            existing_rep = NULL;
        }
    }
    
error:

    if (existing_rep)
    {
        lwmsg_type_free_rep(prot->context, existing_rep);
        existing_rep = NULL;
    }

    return status;
}


void
lwmsg_protocol_free_protocol_rep(
    LWMsgProtocol* prot,
    LWMsgProtocolRep* rep
    )
{
    lwmsg_data_free_graph_cleanup(prot->context, lwmsg_protocol_rep_spec, rep);
}

LWMsgStatus
lwmsg_protocol_print(
    LWMsgProtocol* prot,
    unsigned int indent,
    LWMsgBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgProtocolRep* rep = NULL;
    size_t i = 0;
    unsigned int j = 0;

    BAIL_ON_ERROR(status = lwmsg_protocol_get_protocol_rep(prot, &rep));

    for (i = 0; i < rep->message_count; i++)
    {
        for (j = 0; j < indent; j++)
        {
            BAIL_ON_ERROR(status = lwmsg_buffer_print(buffer, " "));
        }

        if (rep->messages[i].type)
        {
            if (rep->messages[i].name)
            {
                BAIL_ON_ERROR(status = lwmsg_buffer_print(
                                  buffer,
                                  "%s (%i): ",
                                  rep->messages[i].name,
                                  rep->messages[i].tag));
            }
            else
            {
                BAIL_ON_ERROR(status = lwmsg_buffer_print(
                                  buffer,
                                  "%i: ",
                                  rep->messages[i].tag));
            }
            
            BAIL_ON_ERROR(status = lwmsg_type_print_rep(
                              rep->messages[i].type,
                              indent + 4,
                              buffer));
        }
        else
        {
            if (rep->messages[i].name)
            {
                BAIL_ON_ERROR(status = lwmsg_buffer_print(
                                  buffer,
                                  "%s (%i)",
                                  rep->messages[i].name,
                                  rep->messages[i].tag));
            }
            else
            {
                BAIL_ON_ERROR(status = lwmsg_buffer_print(
                                  buffer,
                                  "%i",
                                  rep->messages[i].tag));
            }  
        }
        
        if (i < rep->message_count - 1)
        {
            BAIL_ON_ERROR(status = lwmsg_buffer_print(buffer, "\n"));
        }
    }

cleanup:

    if (rep)
    {
        lwmsg_data_free_graph_cleanup(prot->context, lwmsg_protocol_rep_spec, rep);
    }

    return status;

error:

    goto cleanup;
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
lwmsg_protocol_print_alloc(
    LWMsgProtocol* prot,
    unsigned int indent,
    char** text
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBuffer buffer = {0};
    unsigned char nul = 0;

    buffer.wrap = realloc_wrap;
    buffer.data = (void*) prot->context;

    BAIL_ON_ERROR(status = lwmsg_protocol_print(
                      prot,
                      indent,
                      &buffer));

    BAIL_ON_ERROR(status = lwmsg_buffer_write(&buffer, &nul, 1));

    *text = (char*) buffer.base;

cleanup:

    return status;

error:

    *text = NULL;

    if (buffer.base)
    {
        lwmsg_context_free(prot->context, buffer.base);
    }

    goto cleanup;
}

