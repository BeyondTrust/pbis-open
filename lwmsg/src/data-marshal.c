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
 *        data-marshal.c
 *
 * Abstract:
 *
 *        Marshalling API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "config.h"
#include "data-private.h"
#include "util-private.h"
#include "type-private.h"
#include "buffer-private.h"
#include "convert-private.h"

#include <string.h>

static LWMsgStatus
lwmsg_data_marshal_internal(
    LWMsgDataContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer
    );

static LWMsgStatus
lwmsg_object_is_zero(LWMsgMarshalState* state, LWMsgTypeIter* iter, unsigned char* object, int* is_zero)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i;

    *is_zero = 1;

    for (i = 0; i < iter->size; i++)
    {
        if (object[i] != 0)
        {
            *is_zero = 0;
            break;
        }
    }

    return status;
}

static LWMsgStatus
lwmsg_data_marshal_indirect_prologue(
    LWMsgDataContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer,
    LWMsgTypeIter* inner_iter,
    size_t* count)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char* element = NULL;
    unsigned char implicit_length[4];
    int is_zero;

    lwmsg_type_enter(iter, inner_iter);

    switch (iter->info.kind_indirect.term)
    {
    case LWMSG_TERM_STATIC:
        *count = iter->info.kind_indirect.term_info.static_length;
        break;
    case LWMSG_TERM_MEMBER:
        /* Extract the length out of the field of the actual structure */
        BAIL_ON_ERROR(status = lwmsg_data_extract_length(
                          iter,
                          state->dominating_object,
                          count));
        break;
    case LWMSG_TERM_ZERO:
        if (!object)
        {
            if (iter->attrs.flags & LWMSG_TYPE_FLAG_NOT_NULL)
            {
                BAIL_ON_ERROR(status = DATA_RAISE(
                    context,
                    iter,
                    LWMSG_STATUS_MALFORMED,
                    "NULL passed for non-nullable pointer"));
            }
            *count = 0;
        }
        else
        {
            element = object;
            is_zero = 0;
            
            /* We have to calculate the count by searching for the zero element */
            for (*count = 0;;*count += 1)
            {
                BAIL_ON_ERROR(status = lwmsg_object_is_zero(
                                  state,
                                  inner_iter,
                                  element,
                                  &is_zero));
                
                if (is_zero)
                {
                    break;
                }
                
                element += inner_iter->size;
            }
            
            /* The length is implicitly written into the output to
               facilitate easy unmarshalling */
            BAIL_ON_ERROR(status = lwmsg_convert_integer(
                              count,
                              sizeof(*count),
                              LWMSG_NATIVE_ENDIAN,
                              implicit_length,
                              4,
                              context->byte_order,
                              LWMSG_UNSIGNED));
            
            BAIL_ON_ERROR(status = lwmsg_buffer_write(buffer, implicit_length, 4));
        }
        break;
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_marshal_indirect(
    LWMsgDataContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i;
    size_t count = 0;
    unsigned char* element = NULL;
    LWMsgTypeIter inner_iter;

    BAIL_ON_ERROR(status = lwmsg_data_marshal_indirect_prologue(
                      context,
                      state,
                      iter,
                      object,
                      buffer,
                      &inner_iter,
                      &count));
    
    /* Enforce nullability.  Even if the pointer is marked
       as non-null, allow it to be null if there are no
       elements to marshal */
    if (iter->attrs.flags & LWMSG_TYPE_FLAG_NOT_NULL && !object && count != 0)
    {
        BAIL_ON_ERROR(status = DATA_RAISE(
            context,
            iter,
            LWMSG_STATUS_MALFORMED,
            "NULL passed for non-nullable pointer"));
    }

    if ((inner_iter.kind == LWMSG_KIND_INTEGER ||
         inner_iter.kind == LWMSG_KIND_ENUM) &&
        inner_iter.info.kind_integer.width == 1 &&
        inner_iter.size == 1)
    {
        /* As an optimization, if the element type is a 1-byte integer both
           packed and unpacked, we can write the entire array directly into
           the output.  This is important for character strings */
        BAIL_ON_ERROR(status = lwmsg_buffer_write(
                          buffer, 
                          object, 
                          count));
    }
    else if (count)
    {
        /* Otherwise, marshal each element individually */
        element = object;

        for (i = 0; i < count; i++)
        {
            BAIL_ON_ERROR(status = lwmsg_data_marshal_internal(
                              context,
                              state,
                              &inner_iter, 
                              element, 
                              buffer));
            element += inner_iter.size;
        }
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_marshal_integer(
    LWMsgDataContext* context, 
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char* in = object;
    unsigned char out[MAX_INTEGER_SIZE];
    size_t in_size;
    size_t out_size;

    out_size = iter->info.kind_integer.width;
    in_size = iter->size;

    /* If a valid range is defined, check value against it */
    if (iter->attrs.flags & LWMSG_TYPE_FLAG_RANGE)
    {
        BAIL_ON_ERROR(status = lwmsg_data_verify_range(
                          context,
                          iter,
                          object,
                          in_size));
    }

    status = lwmsg_convert_integer(
        in,
        in_size,
        LWMSG_NATIVE_ENDIAN,
        out,
        out_size,
        context->byte_order,
        iter->info.kind_integer.sign);
    BAIL_ON_ERROR(status = DATA_RAISE(
        context,
        iter,
        status,
        "Integer overflow converting from %s %lu-bit to %lu-bit",
        iter->info.kind_integer.sign == LWMSG_UNSIGNED ? "unsigned" : "signed",
        (unsigned long) in_size * 8,
        (unsigned long) out_size * 8));
    
    BAIL_ON_ERROR(status = lwmsg_buffer_write(buffer, out, out_size));


error:

    return status;
}

static LWMsgStatus
lwmsg_data_marshal_enum(
    LWMsgDataContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char* in = object;
    unsigned char out[MAX_INTEGER_SIZE];
    size_t in_size;
    size_t out_size;
    uint64_t value = 0;
    uint64_t mask = 0;
    uint64_t res = 0;

    out_size = iter->info.kind_integer.width;
    in_size = iter->size;

    /* Make sure we can decode the value */
    status = lwmsg_convert_integer(
                      in,
                      in_size,
                      LWMSG_NATIVE_ENDIAN,
                      &value,
                      sizeof(value),
                      LWMSG_NATIVE_ENDIAN,
                      iter->info.kind_integer.sign);
    BAIL_ON_ERROR(status = DATA_RAISE(
        context,
        iter,
        status,
        "Integer overflow converting from %s %lu-bit to %lu-bit",
        iter->info.kind_integer.sign == LWMSG_UNSIGNED ? "unsigned" : "signed",
        (unsigned long) in_size * 8,
        (unsigned long) out_size * 8));

    BAIL_ON_ERROR(status = lwmsg_data_decode_enum_value(
                      context,
                      iter,
                      value,
                      &mask,
                      &res));

    /* If a valid range is defined, check value against it */
    if (iter->attrs.flags & LWMSG_TYPE_FLAG_RANGE)
    {
        BAIL_ON_ERROR(status = lwmsg_data_verify_range(
                          context,
                          iter,
                          object,
                          in_size));
    }

    BAIL_ON_ERROR(status = lwmsg_convert_integer(in,
                                                 in_size,
                                                 LWMSG_NATIVE_ENDIAN,
                                                 out,
                                                 out_size,
                                                 context->byte_order,
                                                 iter->info.kind_integer.sign));

    BAIL_ON_ERROR(status = lwmsg_buffer_write(buffer, out, out_size));


error:

    return status;
}

static LWMsgStatus
lwmsg_data_marshal_custom(
    LWMsgDataContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    void* transmit_object = NULL;
    LWMsgTypeClass* typeclass = iter->info.kind_custom.typeclass;
    LWMsgTypeIter transmit_iter;
    LWMsgMarshalState my_state = {NULL, state->map};

    lwmsg_type_iterate(typeclass->transmit_type, &transmit_iter);

    if (typeclass->marshal)
    {
        /* Allocate space for the transmitted object */
        BAIL_ON_ERROR(status = lwmsg_data_alloc_memory(context, transmit_iter.size, &transmit_object));

        /* Convert presented object into transmitted object */
        BAIL_ON_ERROR(status = typeclass->marshal(
            context,
            &iter->attrs,
            object,
            transmit_object,
            iter->info.kind_custom.typedata));

        /* Marshal transmitted object */
        BAIL_ON_ERROR(status = lwmsg_data_marshal_internal(
            context,
            &my_state,
            &transmit_iter,
            transmit_object,
            buffer));
    }
    else
    {
        /* Just marshal the object as the transmitted type */
        BAIL_ON_ERROR(status = lwmsg_data_marshal_internal(
                  context,
                  state,
                  &transmit_iter,
                  object,
                  buffer));
    }
                      

error:

    if (transmit_object)
    {
        /* Although we have the type spec for the transmitted object,
           we do not call lwmsg_data_free_graph_internal() to clean it up
           because we did not build it ourselves.  Instead, we
           call the destroy_transmitted function of the type class. */
        if (iter->info.kind_custom.typeclass->destroy_transmitted)
        {
            iter->info.kind_custom.typeclass->destroy_transmitted(
                context,
                &transmit_iter.attrs,
                transmit_object,
                iter->info.kind_custom.typedata);
        }

        /* Free object itself */
        lwmsg_data_free_memory(context, transmit_object);
    }

    return status;
}

static LWMsgStatus
lwmsg_data_marshal_struct_member(
    LWMsgDataContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* struct_iter,
    LWMsgTypeIter* member_iter,
    unsigned char* object,
    LWMsgBuffer* buffer
    )
{
    LWMsgMarshalState my_state = {object, state->map};
    unsigned char* member_object = object + member_iter->offset;

    return lwmsg_data_marshal_internal(
        context,
        &my_state,
        member_iter,
        member_object,
        buffer);
}

static LWMsgStatus
lwmsg_data_marshal_struct(
    LWMsgDataContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter member;

    iter->dom_object = object;

    for (lwmsg_type_enter(iter, &member);
         lwmsg_type_valid(&member);
         lwmsg_type_next(&member))
    {
        BAIL_ON_ERROR(status = lwmsg_data_marshal_struct_member(
                          context,
                          state,
                          iter,
                          &member,
                          object,
                          buffer));
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_marshal_union(
    LWMsgDataContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter arm;

    /* Find the active arm */
    BAIL_ON_ERROR(status = lwmsg_data_extract_active_arm(
                      context,
                      iter,
                      state->dominating_object,
                      &arm));

    /* Simply marshal the active arm */
    BAIL_ON_ERROR(status = lwmsg_data_marshal_internal(
                      context,
                      state,
                      &arm,
                      object,
                      buffer));

error:

    return status;
}

static LWMsgStatus
lwmsg_data_marshal_aliasable_pointer(
    LWMsgDataContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgObjectID id = 0;
    unsigned char id_rep[4];
    LWMsgBool write_pointee = LWMSG_FALSE;

    if (*(void**) object)
    {
        status = lwmsg_data_object_map_find_object(
            state->map,
            *(void**) object,
            &id);

        if (status == LWMSG_STATUS_NOT_FOUND)
        {
            /* Not previous marshalled.  Add to object map and write pointee */
            write_pointee = LWMSG_TRUE;

            BAIL_ON_ERROR(status = lwmsg_data_object_map_insert(
                              state->map,
                              *(void**) object,
                              iter,
                              &id));
        }
        else
        {
            BAIL_ON_ERROR(status);
        }
    }
    else if (iter->attrs.flags & LWMSG_TYPE_FLAG_NOT_NULL)
    {
        BAIL_ON_ERROR(status = DATA_RAISE(
            context,
            iter,
            LWMSG_STATUS_MALFORMED,
            "NULL passed for non-nullable pointer"));
    }

    BAIL_ON_ERROR(status = lwmsg_convert_integer(
                      &id,
                      sizeof(id),
                      LWMSG_NATIVE_ENDIAN,
                      id_rep,
                      sizeof(id_rep),
                      context->byte_order,
                      LWMSG_UNSIGNED));

    BAIL_ON_ERROR(status = lwmsg_buffer_write(buffer, id_rep, sizeof(id_rep)));

    if (write_pointee)
    {
        BAIL_ON_ERROR(status = lwmsg_data_marshal_indirect(
                          context,
                          state,
                          iter,
                          *(unsigned char**) object,
                          buffer));
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_marshal_unaliasable_pointer(
    LWMsgDataContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char ptr_flag;

    /* Indicator byte showing whether the pointer is set */
    ptr_flag = *(void**) object ? 0xFF : 0x00;

    /* Only write pointer flag for nullable pointers */
    if (!(iter->attrs.flags & LWMSG_TYPE_FLAG_NOT_NULL))
    {
        BAIL_ON_ERROR(status = lwmsg_buffer_write(buffer, &ptr_flag, 1));
    }

    if (ptr_flag || (iter->attrs.flags & LWMSG_TYPE_FLAG_NOT_NULL))
    {
        /* Pointer is present, so also write pointee */
        BAIL_ON_ERROR(status = lwmsg_data_marshal_indirect(
                          context,
                          state,
                          iter,
                          *(unsigned char**) object,
                          buffer));
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_marshal_pointer(
    LWMsgDataContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (iter->attrs.flags & LWMSG_TYPE_FLAG_ALIASABLE)
    {
        BAIL_ON_ERROR(status = lwmsg_data_marshal_aliasable_pointer(
                          context,
                          state,
                          iter,
                          object,
                          buffer));
    }
    else
    {
        BAIL_ON_ERROR(status = lwmsg_data_marshal_unaliasable_pointer(
                          context,
                          state,
                          iter,
                          object,
                          buffer));
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_marshal_internal(
    LWMsgDataContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (iter->verify)
    {
        BAIL_ON_ERROR(status = iter->verify(context, LWMSG_FALSE, object, iter->verify_data));
    }

    switch (iter->kind)
    {
    case LWMSG_KIND_VOID:
        /* Nothing to marshal */
        break;
    case LWMSG_KIND_INTEGER:
        BAIL_ON_ERROR(status = lwmsg_data_marshal_integer(context, state, iter, object, buffer));
        break;
    case LWMSG_KIND_ENUM:
        BAIL_ON_ERROR(status = lwmsg_data_marshal_enum(context, state, iter, object, buffer));
        break;
    case LWMSG_KIND_CUSTOM:
        BAIL_ON_ERROR(status = lwmsg_data_marshal_custom(context, state, iter, object, buffer));
        break;
    case LWMSG_KIND_STRUCT:
        BAIL_ON_ERROR(status = lwmsg_data_marshal_struct(context, state, iter, object, buffer));
        break;
    case LWMSG_KIND_UNION:
        BAIL_ON_ERROR(status = lwmsg_data_marshal_union(context, state, iter, object, buffer));
        break;
    case LWMSG_KIND_POINTER:
        BAIL_ON_ERROR(status = lwmsg_data_marshal_pointer(context, state, iter, object, buffer));
        break;
    case LWMSG_KIND_ARRAY:
        BAIL_ON_ERROR(status = lwmsg_data_marshal_indirect(
                          context,
                          state,
                          iter,
                          object,
                          buffer));
        break;
    default:
        LWMSG_ASSERT_NOT_REACHED();
        break;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_data_marshal(LWMsgDataContext* context, LWMsgTypeSpec* type, void* object, LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgObjectMap map;
    LWMsgMarshalState state = {NULL, &map};
    LWMsgTypeIter iter;

    memset(&map, 0, sizeof(map));

    lwmsg_type_iterate_promoted(type, &iter);

    BAIL_ON_ERROR(status = lwmsg_data_marshal_internal(context, &state, &iter, (unsigned char*) &object, buffer));

    if (buffer->wrap)
    {
        BAIL_ON_ERROR(status = buffer->wrap(buffer, 0));
    }

error:

    lwmsg_data_object_map_destroy(&map);

    return status;
}

LWMsgStatus
lwmsg_data_marshal_flat(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    void* object,
    void* buffer,
    size_t length
    )
{
    LWMsgBuffer mbuf;

    mbuf.base = buffer;
    mbuf.end = buffer + length;
    mbuf.cursor = buffer;
    mbuf.wrap = NULL;

    return lwmsg_data_marshal(context, type, object, &mbuf);    
}

static
LWMsgStatus
lwmsg_data_marshal_alloc_wrap(
    LWMsgBuffer* buffer,
    size_t needed
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgDataContext* context = buffer->data;
    size_t oldlen = buffer->end - buffer->base;
    size_t newlen = oldlen == 0 ? 256 : oldlen * 2;
    void* newmem = NULL;
    size_t offset = 0;
    
    BAIL_ON_ERROR(status = lwmsg_context_realloc(context->context, buffer->base, oldlen, newlen, &newmem));

    offset = buffer->cursor - buffer->base;
    buffer->base = newmem;
    buffer->cursor = newmem + offset;
    buffer->end = newmem + newlen;

error:

    return status;
}

LWMsgStatus
lwmsg_data_marshal_flat_alloc(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    void* object,
    void** buffer,
    size_t* length
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBuffer mbuf = {0};
    
    mbuf.base = NULL;
    mbuf.cursor = NULL;
    mbuf.end = NULL;
    mbuf.wrap = lwmsg_data_marshal_alloc_wrap;
    mbuf.data = context;

    BAIL_ON_ERROR(status = lwmsg_data_marshal(context, type, object, &mbuf));

    *buffer = mbuf.base;
    *length = (mbuf.cursor - mbuf.base);

done:

    return status;

error:

    if (mbuf.base)
    {
        lwmsg_context_free(context->context, mbuf.base);
    }

    *buffer = NULL;
    *length = 0;

    goto done;
}
