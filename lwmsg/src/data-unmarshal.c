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
 *        data-unmarshal.c
 *
 * Abstract:
 *
 *        Unmarshalling API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "config.h"
#include "data-private.h"
#include "util-private.h"
#include "type-private.h"
#include "convert-private.h"
#include "context-private.h"
#include "buffer-private.h"

#include <string.h>

static LWMsgStatus
lwmsg_data_unmarshal_internal(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object);

static LWMsgStatus
lwmsg_data_unmarshal_struct_pointee(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgTypeIter* inner,
    LWMsgBuffer* buffer,
    unsigned char** out
    );

static inline
LWMsgStatus
lwmsg_object_alloc(
    LWMsgDataContext* context,
    size_t size,
    unsigned char** out
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_context_alloc(context->context, size, (void**) (void*) out));

error:

    return status;
}

static inline
LWMsgStatus
lwmsg_object_realloc(
    LWMsgDataContext* context,
    unsigned char* object,
    size_t old_size,
    size_t new_size,
    unsigned char** out
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_context_realloc(
                      context->context,
                      (void*) object,
                      old_size,
                      new_size,
                      (void**) (void*) out));

error:

    return status;
}

static inline
void
lwmsg_object_free(
    LWMsgDataContext* context,
    unsigned char* object
    )
{
    lwmsg_context_free(context->context, (void*) object);
}

static LWMsgStatus
lwmsg_data_unmarshal_integer(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char temp[MAX_INTEGER_SIZE];
    size_t in_size;
    size_t out_size;
    
    in_size = iter->info.kind_integer.width;
    out_size = iter->size;

    BAIL_ON_ERROR(status = lwmsg_buffer_read(buffer, temp, in_size));

    status = lwmsg_convert_integer(
        temp,
        in_size,
        context->byte_order,
        object,
        out_size,
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

    /* If a valid range is defined, check value against it */
    if (iter->attrs.flags & LWMSG_TYPE_FLAG_RANGE)
    {
        BAIL_ON_ERROR(status = lwmsg_data_verify_range(
                          context,
                          iter,
                          object,
                          out_size));
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_enum(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char temp[MAX_INTEGER_SIZE];
    size_t in_size;
    size_t out_size;
    uint64_t value = 0;
    uint64_t mask = 0;
    uint64_t res = 0;

    in_size = iter->info.kind_integer.width;
    out_size = iter->size;

    BAIL_ON_ERROR(status = lwmsg_buffer_read(buffer, temp, in_size));

    /* Make sure we can decode the value */
    status = lwmsg_convert_integer(
        temp,
        in_size,
        context->byte_order,
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

    /* Now convert it into its destination */
    BAIL_ON_ERROR(status = lwmsg_convert_integer(
                      temp,
                      in_size,
                      context->byte_order,
                      object,
                      out_size,
                      LWMSG_NATIVE_ENDIAN,
                      iter->info.kind_integer.sign));

    /* If a valid range is defined, check value against it */
    if (iter->attrs.flags & LWMSG_TYPE_FLAG_RANGE)
    {
        BAIL_ON_ERROR(status = lwmsg_data_verify_range(
                          context,
                          iter,
                          object,
                          out_size));
    }

error:

    return status;
}


static LWMsgStatus
lwmsg_data_unmarshal_custom(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeClass* typeclass = iter->info.kind_custom.typeclass;
    LWMsgTypeIter transmit_iter;
    void* transmit_object = NULL;
    LWMsgUnmarshalState my_state = {NULL, state->map};

    lwmsg_type_iterate(typeclass->transmit_type, &transmit_iter);

    if (typeclass->unmarshal)
    {
        /* Allocate memory for transmitted object */
        BAIL_ON_ERROR(status = lwmsg_data_alloc_memory(context, transmit_iter.size, &transmit_object));

        /* Unmarshal transmitted object */
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_internal(
            context,
            &my_state,
            &transmit_iter,
            buffer,
            transmit_object));

        /* Convert transmitted object into presented object */
        BAIL_ON_ERROR(status = iter->info.kind_custom.typeclass->unmarshal(
            context,
            &iter->attrs,
            transmit_object,
            object,
            iter->info.kind_custom.typedata));
    }
    else
    {
        /* Just unmarshal the transmitted type */
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_internal(
            context,
            state,
            &transmit_iter,
            buffer,
            object));
    }

error:

    if (transmit_object)
    {
        /* Since we constructed the transmitted object ourselves, we do not call
           the destroy_transmitted function of the type class */
        lwmsg_data_free_graph_internal(context, &transmit_iter, transmit_object);
        /* Free object itself */
        lwmsg_data_free_memory(context, transmit_object);
    }

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_struct_member(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* struct_iter,
    LWMsgTypeIter* member_iter,
    LWMsgBuffer* buffer,
    unsigned char* struct_object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgUnmarshalState my_state = {struct_object, state->map};
    unsigned char* member_object = struct_object + member_iter->offset;

    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_internal(
                      context,
                      &my_state,
                      member_iter,
                      buffer,
                      member_object));

error:

    return status;
}

/* Find or unmarshal the length of a pointer or array */
static LWMsgStatus
lwmsg_data_unmarshal_indirect_prologue(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgTypeIter* inner,
    LWMsgBuffer* buffer,
    size_t* out_count
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char temp[4];

    switch (iter->info.kind_indirect.term)
    {
    case LWMSG_TERM_STATIC:
        /* Static lengths are easy */
        *out_count = iter->info.kind_indirect.term_info.static_length;
        break;
    case LWMSG_TERM_MEMBER:
        /* The length is present in a member we have already unmarshalled */
        BAIL_ON_ERROR(status = lwmsg_data_extract_length(
                          iter,
                          state->dominating_object,
                          out_count));
        break;
    case LWMSG_TERM_ZERO:
        /* The length is present in the data stream as an unsigned 32-bit integer */
        BAIL_ON_ERROR(status = lwmsg_buffer_read(buffer, temp, sizeof(temp)));
        BAIL_ON_ERROR(status = lwmsg_convert_integer(
                          temp,
                          sizeof(temp),
                          context->byte_order,
                          out_count,
                          sizeof(*out_count),
                          LWMSG_NATIVE_ENDIAN,
                          LWMSG_UNSIGNED));
        break;
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_indirect(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgTypeIter* inner,
    LWMsgBuffer* buffer,
    unsigned char* object,
    size_t count
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i;
    unsigned char* element = NULL;

    if ((inner->kind == LWMSG_KIND_INTEGER ||
         inner->kind == LWMSG_KIND_ENUM) &&
        inner->info.kind_integer.width == 1 &&
        inner->size == 1)
    {
        /* If the element type is an integer and the packed and unpacked sizes are both 1,
           then we can copy directly from the marshalled data into the object.  This
           is an important optimization for unmarshalling character strings */
        BAIL_ON_ERROR(status = lwmsg_buffer_read(buffer, object, count * inner->size));
    }
    else
    {
        /* Otherwise, we need to unmarshal each element individually */
        element = object;
        
        for (i = 0; i < count; i++)
        {
            BAIL_ON_ERROR(status = lwmsg_data_unmarshal_internal(
                              context,
                              state,
                              inner,
                              buffer,
                              element));
            element += inner->size;
        }
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_pointees(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char** out)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t count = 0;
    size_t full_count = 0;
    size_t referent_size = 0;
    unsigned char* object = NULL;
    LWMsgTypeIter inner;
    LWMsgObjectID id = 0;

    lwmsg_type_enter(iter, &inner);

    /* Determine element count */
    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_indirect_prologue(
                      context,
                      state,
                      iter,
                      &inner,
                      buffer,
                      &count));

    /* Treat a single struct pointee as a special case as it could
       hold a flexible array member */
    if (inner.kind == LWMSG_KIND_STRUCT && count == 1)
    {
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_struct_pointee(
                          context,
                          state,
                          iter,
                          &inner,
                          buffer,
                          out));
    }
    else
    {
        /* Calculate total number of elements */
        if (iter->info.kind_indirect.term == LWMSG_TERM_ZERO)
        {
            /* If the referent is zero-terminated, we need to allocate an extra element */
            BAIL_ON_ERROR(status = DATA_RAISE(
                context,
                iter,
                lwmsg_add_unsigned(count, 1, &full_count),
                "Integer overflow in pointer referent length"));
        }
        else
        {
            full_count = count;
        }

        /* Calculate the referent size */
        BAIL_ON_ERROR(status = DATA_RAISE(
            context,
            iter,
            lwmsg_multiply_unsigned(full_count, inner.size, &referent_size),
            "Integer overflow in referent size"));

        if (iter->attrs.flags & LWMSG_TYPE_FLAG_ALIASABLE &&
            referent_size < sizeof(void*))
        {
            /* Make sure aliasable objects are always large enough to hold a pointer */
            referent_size = sizeof(void*);
        }

        /* Enforce MAX_ALLOC attribute */
        if (iter->attrs.max_alloc && referent_size > iter->attrs.max_alloc)
        {
            BAIL_ON_ERROR(status = DATA_RAISE(
                context,
                iter,
                LWMSG_STATUS_OVERFLOW,
                "Pointer referent exceeded max allocation size of %lu",
                (unsigned long) iter->attrs.max_alloc));
        }

        /* Allocate the referent */
        BAIL_ON_ERROR(status = lwmsg_object_alloc(context, referent_size, &object));

        if (iter->attrs.flags & LWMSG_TYPE_FLAG_ALIASABLE)
        {
            /* If this is the referent of an aliasable pointer, insert it into the object map now */
            BAIL_ON_ERROR(status = lwmsg_data_object_map_insert(
                              state->map,
                              object,
                              iter,
                              &id));
        }

        /* Unmarshal elements */
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_indirect(
                          context,
                          state,
                          iter,
                          &inner,
                          buffer,
                          object,
                          count));

        *out = object;
    }

done:

    return status;

error:

    *out = NULL;

    if (object)
    {
        lwmsg_data_free_graph_internal(context, iter, (unsigned char*) &object);
    }

    goto done;
}

static LWMsgStatus
lwmsg_data_unmarshal_aliasable_pointer(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char** out
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char id_rep[4];
    LWMsgObjectID id = 0;
    void* object = NULL;

    /* Read the object id from the stream */
    BAIL_ON_ERROR(status = lwmsg_buffer_read(buffer, id_rep, sizeof(id_rep)));

    BAIL_ON_ERROR(status = lwmsg_convert_integer(
                      id_rep,
                      sizeof(id_rep),
                      context->byte_order,
                      &id,
                      sizeof(id),
                      LWMSG_NATIVE_ENDIAN,
                      LWMSG_UNSIGNED));

    if (id != 0)
    {
        status = lwmsg_data_object_map_find_id(
            state->map,
            id,
            iter,
            &object);
    }
    else if (iter->attrs.flags & LWMSG_TYPE_FLAG_NOT_NULL)
    {
        BAIL_ON_ERROR(status = DATA_RAISE(
            context,
            iter,
            LWMSG_STATUS_MALFORMED,
            "NULL found in stream where non-nullable pointer expected"));
    }

    if (status == LWMSG_STATUS_NOT_FOUND)
    {
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_pointees(
                          context,
                          state,
                          iter,
                          buffer,
                          (unsigned char**) (void*) &object));
    }
    else
    {
        BAIL_ON_ERROR(status);
    }

    *(void**) out = object;

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_unaliasable_pointer(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char** out
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char ptr_flag = 0;

    if (iter->attrs.flags & LWMSG_TYPE_FLAG_NOT_NULL)
    {
        /* If pointer is never null, there is no flag in the stream */
        ptr_flag = 0xFF;
    }
    else
    {
        /* A flag in the stream indicates whether the pointer is NULL or not. */
        BAIL_ON_ERROR(status = lwmsg_buffer_read(buffer, &ptr_flag, sizeof(ptr_flag)));
    }
    
    if (ptr_flag)
    {
        /* If the pointer is non-null, unmarshal the pointees */
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_pointees(
                          context,
                          state,
                          iter,
                          buffer,
                          (unsigned char**) out));
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_pointer(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char** out
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (iter->attrs.flags & LWMSG_TYPE_FLAG_ALIASABLE)
    {
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_aliasable_pointer(
                          context,
                          state,
                          iter,
                          buffer,
                          out));
    }
    else
    {
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_unaliasable_pointer(
                          context,
                          state,
                          iter,
                          buffer,
                          out));
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_array(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t count = 0;
    LWMsgTypeIter inner;

    lwmsg_type_enter(iter, &inner);
    
    /* Determine element size and count */
    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_indirect_prologue(
                      context,
                      state,
                      iter,
                      &inner,
                      buffer,
                      &count));

    /* Unmarshal elements */
    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_indirect(
                      context,
                      state,
                      iter,
                      &inner,
                      buffer,
                      object,
                      count));

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_struct(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object,
    LWMsgTypeSpec** flexible_member
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter member;

    if (flexible_member)
    {
        *flexible_member = NULL;
    }

    iter->dom_object = object;

    for (lwmsg_type_enter(iter, &member);
         lwmsg_type_valid(&member);
         lwmsg_type_next(&member))
    {
        if (member.kind == LWMSG_KIND_ARRAY &&
            member.info.kind_indirect.term != LWMSG_TERM_STATIC)
        {
            if (flexible_member)
            {
                *flexible_member = member.spec;
            }
            continue;
        }

        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_struct_member(
                          context,
                          state,
                          iter,
                          &member,
                          buffer,
                          object));
    }

error:

    return status;
}

/* Free a structure that is missing its flexible array member */
static LWMsgStatus
lwmsg_data_unmarshal_free_partial_struct(
    LWMsgDataContext* context,
    LWMsgTypeIter* iter,
    unsigned char* object)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter member;

    for (lwmsg_type_enter(iter, &member);
         lwmsg_type_valid(&member);
         lwmsg_type_next(&member))
    {
        if (member.kind == LWMSG_KIND_ARRAY &&
            member.info.kind_indirect.term != LWMSG_TERM_STATIC)
        {
            break;
        }

        BAIL_ON_ERROR(status = lwmsg_data_free_graph_internal(
                          context,
                          &member,
                          object + member.offset));
    }

    lwmsg_object_free(context, object);

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_struct_pointee(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* pointer_iter,
    LWMsgTypeIter* struct_iter,
    LWMsgBuffer* buffer,
    unsigned char** out
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char* base_object = NULL;
    unsigned char* full_object = NULL;
    LWMsgTypeSpec* flexible_member = NULL;
    size_t base_size = 0;
    size_t full_size = 0;
    LWMsgObjectID id = 0;

    base_size = struct_iter->size;

    if (pointer_iter->attrs.flags & LWMSG_TYPE_FLAG_ALIASABLE &&
        base_size < sizeof(void*))
    {
        base_size = sizeof(void*);
    }

    /* Enforce MAX_ALLOC attribute */
    if (pointer_iter->attrs.max_alloc && base_size > pointer_iter->attrs.max_alloc)
    {
        BAIL_ON_ERROR(status = DATA_RAISE(
                     context,
                     pointer_iter,
                     LWMSG_STATUS_OVERFLOW,
                     "Pointer referent exceeded max allocation size of %lu",
                     (unsigned long) pointer_iter->attrs.max_alloc));
    }

    /* Allocate enough memory to hold the base of the object */
    BAIL_ON_ERROR(status = lwmsg_object_alloc(
                      context,
                      base_size,
                      &base_object));

    if (pointer_iter->attrs.flags & LWMSG_TYPE_FLAG_ALIASABLE)
    {
        /* If this is the referent of an aliasable pointer, insert it into the object map now */
        BAIL_ON_ERROR(status = lwmsg_data_object_map_insert(
                          state->map,
                          base_object,
                          pointer_iter,
                          &id));
    }

    /* Unmarshal all base members of the structure and find any flexible member */
    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_struct(
                      context,
                      state,
                      struct_iter,
                      buffer,
                      base_object,
                      &flexible_member));

    /* Now that the base of the object is unmarshalled, we can see if we need to
       reallocate space for a flexible member */
    if (flexible_member)
    {
        LWMsgTypeIter flex_iter;
        LWMsgTypeIter inner_iter;
        LWMsgUnmarshalState my_state = {base_object, state->map};
        size_t count = 0;
        size_t full_count = 0;
        size_t flexible_size = 0;

        lwmsg_type_iterate(flexible_member, &flex_iter);
        lwmsg_type_enter(&flex_iter, &inner_iter);

        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_indirect_prologue(
                          context,
                          &my_state,
                          &flex_iter,
                          &inner_iter,
                          buffer,
                          &count));

        /* Calculate total number of elements */
        if (flex_iter.info.kind_indirect.term == LWMSG_TERM_ZERO)
        {
            /* If the referent is zero-terminated, we need to allocate an extra element */
            BAIL_ON_ERROR(status = DATA_RAISE(
                            context,
                            pointer_iter,
                            lwmsg_add_unsigned(count, 1, &full_count),
                            "Integer overflow in pointer referent length"));
        }
        else
        {
            full_count = count;
        }

        /* Calculate the size of the flexible member */
        BAIL_ON_ERROR(status = DATA_RAISE(
                        context,
                        pointer_iter,
                        lwmsg_multiply_unsigned(full_count, inner_iter.size, &flexible_size),
                        "Integer overflow in pointer referent size"));

        /* Calculate the size of the full structure */
        BAIL_ON_ERROR(status = DATA_RAISE(
                        context,
                        pointer_iter,
                        lwmsg_add_unsigned(struct_iter->size, flexible_size, &full_size),
                        "Integer overflow in pointer referent size"));

        if (pointer_iter->attrs.flags & LWMSG_TYPE_FLAG_ALIASABLE &&
            full_size < sizeof(void*))
        {
            full_size = sizeof(void*);
        }

        /* Enforce MAX_ALLOC attribute */
        if (pointer_iter->attrs.max_alloc && full_size > pointer_iter->attrs.max_alloc)
        {
            BAIL_ON_ERROR(status = DATA_RAISE(
                         context,
                         pointer_iter,
                         LWMSG_STATUS_OVERFLOW,
                         "Pointer referent exceeded max allocation size of %lu",
                         (unsigned long) pointer_iter->attrs.max_alloc));
        }

        /* Allocate the full object */
        BAIL_ON_ERROR(status = lwmsg_object_realloc(
                          context,
                          base_object,
                          struct_iter->size,
                          full_size,
                          &full_object));

        base_object = NULL;

        /* Unmarshal the flexible array member */
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_indirect(
                          context,
                          &my_state,
                          &flex_iter,
                          &inner_iter,
                          buffer,
                          full_object + flex_iter.offset,
                          count));
    }
    else
    {
        full_object = base_object;
        base_object = NULL;
    }

    *out = full_object;
    
done:

    return status;

error:

    if (base_object)
    {
        /* We must avoid visiting flexible array members when
           freeing the base object because it does not have
           space for them in the allocated block */
        lwmsg_data_unmarshal_free_partial_struct(
            context,
            struct_iter,
            base_object);
    }

    if (full_object)
    {
        lwmsg_data_free_graph_internal(context, pointer_iter, (unsigned char*) &base_object);
    }

    goto done;
}

static LWMsgStatus
lwmsg_data_unmarshal_union(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter arm;

    /* Find the active arm */
    BAIL_ON_ERROR(status = lwmsg_data_extract_active_arm(
                      context,
                      iter,
                      state->dominating_object,
                      &arm));

    /* Simply unmarshal the object as the particular arm */
    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_internal(
                      context,
                      state,
                      &arm,
                      buffer,
                      object));

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_internal(
    LWMsgDataContext* context,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    switch (iter->kind)
    {
    case LWMSG_KIND_VOID:
        /* Nothing to unmarshal */
        break;
    case LWMSG_KIND_INTEGER:
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_integer(
                          context,
                          state,
                          iter,
                          buffer,
                          object));
        break;
    case LWMSG_KIND_ENUM:
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_enum(
                          context,
                          state,
                          iter,
                          buffer,
                          object));
        break;
   case LWMSG_KIND_CUSTOM:
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_custom(
                          context,
                          state,
                          iter,
                          buffer,
                          object));
        break;
    case LWMSG_KIND_STRUCT:
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_struct(
                          context,
                          state,
                          iter,
                          buffer,
                          object,
                          NULL));
        break;
    case LWMSG_KIND_UNION:
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_union(
                          context,
                          state,
                          iter,
                          buffer,
                          object));
        break;
    case LWMSG_KIND_POINTER:
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_pointer(
                          context,
                          state,
                          iter,
                          buffer,
                          (unsigned char **) object));
        break;
    case LWMSG_KIND_ARRAY:
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_array(
                          context,
                          state,
                          iter,
                          buffer,
                          object));
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_UNIMPLEMENTED);
    }
    
    if (iter->verify)
    {
        BAIL_ON_ERROR(status = iter->verify(context, LWMSG_TRUE, object, iter->verify_data));
    }

error:

    return status;

}

LWMsgStatus
lwmsg_data_unmarshal(LWMsgDataContext* context, LWMsgTypeSpec* type, LWMsgBuffer* buffer, void** out)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgObjectMap map;
    LWMsgUnmarshalState my_state = {NULL, &map};
    LWMsgTypeIter iter;

    memset(&map, 0, sizeof(map));

    lwmsg_type_iterate_promoted(type, &iter);

    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_internal(context, &my_state, &iter, buffer, (unsigned char*) out));

    if (buffer->wrap)
    {
        BAIL_ON_ERROR(status = buffer->wrap(buffer, 0));
    }

error:

    lwmsg_data_object_map_destroy(&map);

    return status;
}

LWMsgStatus
lwmsg_data_unmarshal_into(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    LWMsgBuffer* buffer,
    void* object,
    size_t size
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgObjectMap map;
    LWMsgUnmarshalState my_state = {NULL, &map};
    LWMsgTypeIter iter;

    memset(&map, 0, sizeof(map));

    lwmsg_type_iterate(type, &iter);

    if (size < iter.size)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_BUFFER_TOO_SMALL);
    }

    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_internal(context, &my_state, &iter, buffer, object));

    if (buffer->wrap)
    {
        BAIL_ON_ERROR(status = buffer->wrap(buffer, 0));
    }

error:

    lwmsg_data_object_map_destroy(&map);

    return status;
}

LWMsgStatus
lwmsg_data_unmarshal_flat(LWMsgDataContext* context, LWMsgTypeSpec* type, const void* buffer, size_t length, void** out)
{
    LWMsgBuffer mbuf;

    mbuf.base = (unsigned char*) buffer;
    mbuf.cursor = mbuf.base;
    mbuf.end = mbuf.base + length;
    mbuf.wrap = NULL;

    return lwmsg_data_unmarshal(context, type, &mbuf, out);
}
