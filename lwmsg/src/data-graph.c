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
 *        data-graph.c
 *
 * Abstract:
 *
 *        Data graph operations
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>

#include <inttypes.h>
#include <lwmsg/common.h>
#include "convert-private.h"
#include "context-private.h"
#include "type-private.h"
#include "data-private.h"
#include "util-private.h"


LWMsgStatus
lwmsg_data_extract_discrim_tag(
    LWMsgTypeIter* iter,
    unsigned char* dominating_struct,
    uint32_t* tag
    )
{
    return lwmsg_convert_integer(
        dominating_struct + iter->info.kind_compound.discrim.offset,
        iter->info.kind_compound.discrim.size,
        LWMSG_NATIVE_ENDIAN,
        tag,
        sizeof(*tag),
        LWMSG_NATIVE_ENDIAN,
        LWMSG_UNSIGNED);
}

LWMsgStatus
lwmsg_data_extract_length(
    LWMsgTypeIter* iter,
    unsigned char* dominating_struct,
    size_t *length
    )
{
    return lwmsg_convert_integer(
        dominating_struct + iter->info.kind_indirect.term_info.member.offset,
        iter->info.kind_indirect.term_info.member.size,
        LWMSG_NATIVE_ENDIAN,
        length,
        sizeof(*length),
        LWMSG_NATIVE_ENDIAN,
        LWMSG_SIGNED);
}

LWMsgStatus
lwmsg_data_extract_active_arm(
    LWMsgDataContext* context,
    LWMsgTypeIter* iter,
    unsigned char* dominating_struct,
    LWMsgTypeIter* active_iter
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    uint32_t tag;

    BAIL_ON_ERROR(status = lwmsg_data_extract_discrim_tag(
                      iter,
                      dominating_struct,
                      &tag));
    
    for (lwmsg_type_enter(iter, active_iter);
         lwmsg_type_valid(active_iter);
         lwmsg_type_next(active_iter))
    {
        if (tag == active_iter->tag)
        {
            goto done;
        }
    }

    if (context)
    {
        BAIL_ON_ERROR(status = DATA_RAISE(
            context,
            iter,
            LWMSG_STATUS_MALFORMED,
            "No arm with tag %" PRIu32 " found in union"));
    }
    else
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
    }
    
done:

    return status;

error:
        
    goto done;
}

static
LWMsgStatus
lwmsg_object_is_zero(
    LWMsgTypeIter* iter,
    unsigned char* object,
    int* is_zero
    )
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

LWMsgStatus
lwmsg_data_calculate_indirect_metrics(
    LWMsgTypeIter* iter,
    unsigned char* object,
    size_t* count,
    size_t* element_size
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char* element = NULL;
    int is_zero;
    LWMsgTypeIter inner;
    
    lwmsg_type_enter(iter, &inner);

    switch (iter->info.kind_indirect.term)
    {
    case LWMSG_TERM_STATIC:
        *count = iter->info.kind_indirect.term_info.static_length;
        break;
    case LWMSG_TERM_MEMBER:
        /* Extract the length out of the field of the actual structure */
        BAIL_ON_ERROR(status = lwmsg_data_extract_length(
                          iter,
                          iter->dom_object,
                          count));
        break;
    case LWMSG_TERM_ZERO:
        element = object;
        is_zero = 0;

        /* We have to calculate the count by searching for the zero element */
        for (*count = 0;;*count += 1)
        {
            BAIL_ON_ERROR(status = lwmsg_object_is_zero(
                              &inner,
                              element,
                              &is_zero));

            if (is_zero)
            {
                break;
            }

            element += inner.size;
        }
    }

    *element_size = inner.size;

error:

    return status;
}

LWMsgStatus
lwmsg_data_verify_range(
    LWMsgDataContext* context,
    LWMsgTypeIter* iter,
    void* object,
    size_t object_size
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    uint64_t value;

    BAIL_ON_ERROR(status = lwmsg_convert_integer(
                      object,
                      object_size,
                      LWMSG_NATIVE_ENDIAN,
                      &value,
                      sizeof(value),
                      LWMSG_NATIVE_ENDIAN,
                      iter->info.kind_integer.sign));

    if (value < iter->attrs.range_low || value > iter->attrs.range_high)
    {
        BAIL_ON_ERROR(status = DATA_RAISE(
                           context,
                           iter,
                           LWMSG_STATUS_MALFORMED,
                           "Integer value did not fall within specified range"));
    }

error:

    return status;
}

LWMsgStatus
lwmsg_data_decode_enum_value(
    LWMsgDataContext* context,
    LWMsgTypeIter* iter,
    uint64_t value,
    uint64_t* mask,
    uint64_t* res
    )
{
    LWMsgTypeIter var;
    LWMsgBool found_value = LWMSG_FALSE;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    *mask = 0;

    for (lwmsg_type_enter(iter, &var);
         lwmsg_type_valid(&var);
         lwmsg_type_next(&var))
    {
        if (var.info.kind_variant.is_mask)
        {
            *mask |= (var.tag & value);
        }
    }

    *res = value & ~*mask;

    for (lwmsg_type_enter(iter, &var);
         lwmsg_type_valid(&var);
         lwmsg_type_next(&var))
    {
        if (!var.info.kind_variant.is_mask)
        {
            found_value = LWMSG_TRUE;

            if (*res == var.tag)
            {
                goto done;
            }
        }
    }

    if (*res == 0 && !found_value)
    {
        /* A residual of 0 is ok when the enum is a pure mask */
        goto done;
    }
    else
    {
        BAIL_ON_ERROR(status = DATA_RAISE(
            context,
            iter,
            LWMSG_STATUS_MALFORMED,
            "invalid enum value %lu",
            (unsigned long) *res));
    }

done:

    return status;

error:

    *res = 0;
    *mask = 0;

    goto done;
}

LWMsgStatus
lwmsg_data_visit_graph(
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgGraphVisitFunction func,
    void* data
    )
{
    return func(iter, object, data);
}

static inline
LWMsgBool
lwmsg_is_zero(
    unsigned char* object,
    size_t size
    )
{
    size_t i;

    for (i = 0; i < size; i++)
    {
        if (object[i] != 0)
        {
            return LWMSG_FALSE;
        }
    }

    return LWMSG_TRUE;
}

static
LWMsgStatus
lwmsg_data_visit_graph_indirect(
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgGraphVisitFunction func,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t count = 0;
    size_t i;
    void* element;
    LWMsgTypeIter inner;

    lwmsg_type_enter(iter, &inner);

    switch (iter->info.kind_indirect.term)
    {
    case LWMSG_TERM_STATIC:
        count = iter->info.kind_indirect.term_info.static_length;
        break;
    case LWMSG_TERM_MEMBER:
        BAIL_ON_ERROR(status = lwmsg_data_extract_length(
                          iter,
                          iter->dom_object,
                          &count));
        break;
    case LWMSG_TERM_ZERO:
        count = 1;
        for (element = object; !lwmsg_is_zero(element, inner.size); element += inner.size)
        {
            count++;
        }
        break;
    }

    element = object;
    for (i = 0; i < count; i++)
    {
        BAIL_ON_ERROR(status = func(&inner, element, data));
        element += inner.size;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_data_visit_graph_children(
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgGraphVisitFunction func,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter inner;

    switch (iter->kind)
    {
    case LWMSG_KIND_STRUCT:
        lwmsg_type_enter(iter, &inner);
        inner.dom_object = object;
        for (; lwmsg_type_valid(&inner); lwmsg_type_next(&inner))
        {
            BAIL_ON_ERROR(status = func(&inner, object + inner.offset, data));
        }
        break;
    case LWMSG_KIND_UNION:
        /* Find the active arm */
        BAIL_ON_ERROR(status = lwmsg_data_extract_active_arm(
                          NULL,
                          iter,
                          iter->dom_object,
                          &inner));
        BAIL_ON_ERROR(status = func(&inner, object, data));
        break;
    case LWMSG_KIND_POINTER:
        if (*(void**) object)
        {
            BAIL_ON_ERROR(status = lwmsg_data_visit_graph_indirect(
                              iter,
                              *(unsigned char**) object,
                              func,
                              data));
        }
        break;
    case LWMSG_KIND_ARRAY:
        BAIL_ON_ERROR(status = lwmsg_data_visit_graph_indirect(
                          iter,
                          object,
                          func,
                          data));
        break;
    default:
        break;
    }

error:

    return status;
}

typedef struct FreePath
{
    void* pointer;
    struct FreePath* up;
} FreePath;

typedef struct FreeInfo
{
    LWMsgDataContext* context;
    LWMsgFreeFunction free;
    void* data;
    FreePath* path;
    void* alias_list;
} FreeInfo;

static
LWMsgBool
detect_cycle(
    FreeInfo* info,
    void* pointer
    )
{
    FreePath* path = NULL;

    for (path = info->path; path != NULL; path = path->up)
    {
        if (path->pointer == pointer)
        {
            return LWMSG_TRUE;
        }
    }

    return LWMSG_FALSE;
}

static
LWMsgBool
detect_alias(
    FreeInfo* info,
    void* pointer
    )
{
    void* alias = NULL;

    for (alias = info->alias_list; alias; alias = *(void**) alias)
    {
        if (alias == pointer)
        {
            return LWMSG_TRUE;
        }
    }

    return LWMSG_FALSE;
}

static
LWMsgStatus
lwmsg_data_free_graph_visit(
    LWMsgTypeIter* iter,
    unsigned char* object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    FreeInfo* info = (FreeInfo*) data;
    FreePath path = {0};
    void* pointer = NULL;

    switch(iter->kind)
    {
    case LWMSG_KIND_CUSTOM:
        if (iter->info.kind_custom.typeclass->destroy_presented)
        {
            iter->info.kind_custom.typeclass->destroy_presented(
                info->context,
                &iter->attrs,
                object,
                iter->info.kind_custom.typedata);
        }
        break;
    case LWMSG_KIND_POINTER:
        pointer = *(void**) object;

        if (pointer == NULL)
        {
            /* Leave */
            goto error;
        }

        if (iter->attrs.flags & LWMSG_TYPE_FLAG_ALIASABLE)
        {
            /* If the pointer is aliasable, we need to avoid visiting its referent twice */
            if (detect_cycle(info, pointer) || detect_alias(info, pointer))
            {
                /* Leave */
                goto error;
            }

            /* Push pointer into path list so we avoid cycles */
            path.up = info->path;
            path.pointer = pointer;
            info->path = &path;
        }

        /* Visit referent */
        BAIL_ON_ERROR(status = lwmsg_data_visit_graph_children(
                          iter,
                          object,
                          lwmsg_data_free_graph_visit,
                          data));

        if (iter->attrs.flags & LWMSG_TYPE_FLAG_ALIASABLE)
        {
            /* Pop pointer from path list */
            info->path = info->path->up;

            /* Since we will never visit the pointer referent again, we can safely reuse
               its memory to link it into a list of aliasable memory objects */
            *(void**) pointer = info->alias_list;
            info->alias_list = pointer;
        }
        else
        {
            info->free(pointer, info->data);
        }
        break;
    default:
        BAIL_ON_ERROR(status = lwmsg_data_visit_graph_children(
                              iter,
                              object,
                              lwmsg_data_free_graph_visit,
                              data));
        break;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_data_free_graph_internal(
    LWMsgDataContext* context,
    LWMsgTypeIter* iter,
    unsigned char* object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    FreeInfo info = {0};
    void* alias = NULL;
    void* next = NULL;

    lwmsg_context_get_memory_functions(context->context, NULL, &info.free, NULL, &info.data);
    info.context = context;

    BAIL_ON_ERROR(status = lwmsg_data_visit_graph(
                      iter,
                      object,
                      lwmsg_data_free_graph_visit,
                      &info));

    /* Free all aliasable objects that were found */
    for (alias = info.alias_list; alias; alias = next)
    {
        next = *(void**) alias;

        info.free(alias, info.data);
    }

error:

    return status;
}

LWMsgStatus
lwmsg_data_free_graph(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    void* root
    )
{
    LWMsgTypeIter iter;

    lwmsg_type_iterate_promoted(type, &iter);

    return lwmsg_data_free_graph_internal(context, &iter, (unsigned char*) &root);
}

void
lwmsg_data_free_graph_cleanup(
    const LWMsgContext* context,
    LWMsgTypeSpec* type,
    void* root
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgDataContext dcontext;

    memset(&dcontext, 0, sizeof(dcontext));
    dcontext.context = context;

    status = lwmsg_data_free_graph(&dcontext, type, root);
    LWMSG_ASSERT(status == LWMSG_STATUS_SUCCESS);
}

LWMsgStatus
lwmsg_data_destroy_graph(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    void* root
    )
{
    LWMsgTypeIter iter;

    lwmsg_type_iterate(type, &iter);

    return lwmsg_data_free_graph_internal(context, &iter, root);
}

void
lwmsg_data_destroy_graph_cleanup(
    const LWMsgContext* context,
    LWMsgTypeSpec* type,
    void* root
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgDataContext dcontext;

    memset(&dcontext, 0, sizeof(dcontext));
    dcontext.context = context;

    status = lwmsg_data_destroy_graph(&dcontext, type, root);
    LWMSG_ASSERT(status == LWMSG_STATUS_SUCCESS);
}
