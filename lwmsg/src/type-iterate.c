
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
 *        type-iterate.c
 *
 * Abstract:
 *
 *        Type specification API
 *        Type iteration and object graph visiting
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "type-private.h"
#include "util-private.h"

static inline
void
lwmsg_type_find_end(
    LWMsgTypeSpec** in_out_spec
    )
{
    unsigned int depth = 1;
    LWMsgTypeSpec* spec = *in_out_spec;
    LWMsgTypeDirective cmd;
    LWMsgBool is_meta;
    LWMsgBool is_member;
    LWMsgBool is_debug;
    LWMsgArrayTermination term;

    for (;;)
    {
        cmd = *(spec++);

        is_meta = cmd & LWMSG_FLAG_META;
        is_debug = cmd & LWMSG_FLAG_DEBUG;
        is_member = cmd & LWMSG_FLAG_MEMBER;

        spec += is_meta ? 1 : 0;
        spec += is_debug ? 2 : 0;
        
        switch (cmd & LWMSG_CMD_MASK)
        {
        case LWMSG_CMD_END:
            if (--depth == 0)
            {
                goto done;
            }
            break;
        case LWMSG_CMD_VOID:
            break;
        case LWMSG_CMD_INTEGER:
            spec += is_member ? 4 : 3;
            break;
        case LWMSG_CMD_ENUM:
            spec += is_member ? 4 : 3;
            depth++;
            break;
        case LWMSG_CMD_STRUCT:
        case LWMSG_CMD_UNION:
            spec += is_member ? 2 : 1;
            depth++;
            break;
        case LWMSG_CMD_POINTER:
        case LWMSG_CMD_ARRAY:
            spec += is_member ? 1 : 0;
            depth++;
            break;
        case LWMSG_CMD_ENUM_VALUE:
        case LWMSG_CMD_ENUM_MASK:
            spec += 1;
            break;
        case LWMSG_CMD_TYPESPEC:
            spec += is_member ? 3 : 1;
            break;
        case LWMSG_CMD_TERMINATION:
            term = *(spec++);
            switch (term)
            {
            case LWMSG_TERM_STATIC:
                spec += 1;
                break;
            case LWMSG_TERM_MEMBER:
                spec += 2;
                break;
            case LWMSG_TERM_ZERO:
                break;
            }
            break;
        case LWMSG_CMD_TAG:
            spec += 1;
            break;
        case LWMSG_CMD_DISCRIM:
        case LWMSG_CMD_VERIFY:
        case LWMSG_CMD_RANGE:
            spec += 2;
            break;
        case LWMSG_CMD_CUSTOM:
            spec += is_member ? 4 : 3;
            break;
        case LWMSG_CMD_CUSTOM_ATTR:
            spec += 1;
            break;
        case LWMSG_CMD_ENCODING:
            spec += 1;
            break;
        case LWMSG_CMD_MAX_ALLOC:
            spec += 1;
            break;
        case LWMSG_CMD_SENSITIVE:
        case LWMSG_CMD_ALIASABLE:
        case LWMSG_CMD_NOT_NULL:
            break;
        default:
            abort();
        }
    }

done:

    *in_out_spec = spec;
}

static
void
lwmsg_type_iterate_inner(
    LWMsgTypeSpec* spec,
    LWMsgTypeIter* iter
    )
{
    LWMsgTypeDirective cmd;
    LWMsgArrayTermination term;
    size_t my_size;
    size_t my_offset;

    cmd = *(spec++);

    if (cmd & LWMSG_FLAG_META)
    {      
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            iter->meta.member_name = (const char*) *(spec++);
        }
        else
        {
            iter->meta.type_name = (const char*) *(spec++);
        }
    }

    if (cmd & LWMSG_FLAG_DEBUG)
    {
        iter->debug.file = (const char*) *(spec++);
        iter->debug.line = (unsigned int) *(spec++);
    }

    switch (cmd & LWMSG_CMD_MASK)
    {
    case LWMSG_CMD_INTEGER:
    case LWMSG_CMD_ENUM:
        iter->size = *(spec++);
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            iter->offset = *(spec++);
        }

        iter->info.kind_integer.width = *(spec++);
        iter->info.kind_integer.sign = *(spec++);

        if ((cmd & LWMSG_CMD_MASK) == LWMSG_CMD_ENUM)
        {
            iter->kind = LWMSG_KIND_ENUM;
            iter->inner = spec;
            lwmsg_type_find_end(&spec);
        }
        else
        {
            iter->kind = LWMSG_KIND_INTEGER;
        }
        break;
    case LWMSG_CMD_STRUCT:
        iter->size = *(spec++);
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            iter->offset = *(spec++);
        }
        iter->kind = LWMSG_KIND_STRUCT;
        iter->inner = spec;
        lwmsg_type_find_end(&spec);
        break;
    case LWMSG_CMD_UNION:
        iter->size = *(spec++);
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            iter->offset = *(spec++);
        }
        iter->kind = LWMSG_KIND_UNION;
        iter->inner = spec;
        lwmsg_type_find_end(&spec);
        break;
    case LWMSG_CMD_POINTER:
        iter->size = sizeof(void*);
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            iter->offset = *(spec++);
        }
        iter->kind = LWMSG_KIND_POINTER;
        iter->inner = spec;
        /* Default termination settings for a pointer */
        iter->info.kind_indirect.term = LWMSG_TERM_STATIC;
        iter->info.kind_indirect.term_info.static_length = 1;
        iter->info.kind_indirect.encoding = NULL;
        lwmsg_type_find_end(&spec);
        break;
    case LWMSG_CMD_ARRAY:
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            iter->offset = *(spec++);
        }
        iter->kind = LWMSG_KIND_ARRAY;
        iter->inner = spec;
        /* Default termination settings for an array */
        iter->info.kind_indirect.term = LWMSG_TERM_STATIC;
        iter->info.kind_indirect.term_info.static_length = 1;
        iter->info.kind_indirect.encoding = NULL;
        lwmsg_type_find_end(&spec);
        break;
    case LWMSG_CMD_TYPESPEC:
        my_size = 0;
        my_offset = 0;
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            my_size = *(spec++);
            my_offset = *(spec++);
        }
        lwmsg_type_iterate_inner((LWMsgTypeSpec*) *(spec++), iter);
        if (!iter->size)
        {
            iter->size = my_size;
        }
        iter->offset = my_offset;
        break;
    case LWMSG_CMD_CUSTOM:
        iter->size = *(spec++);
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            iter->offset = *(spec++);
        }
        iter->kind = LWMSG_KIND_CUSTOM;
        iter->info.kind_custom.typeclass = (LWMsgTypeClass*) *(spec++);
        iter->info.kind_custom.typedata = (void*) *(spec++);
        break;
    case LWMSG_CMD_VOID:
        iter->kind = LWMSG_KIND_VOID;
        break;
    case LWMSG_CMD_ENUM_VALUE:
    case LWMSG_CMD_ENUM_MASK:
        iter->kind = LWMSG_KIND_VOID;
        iter->info.kind_variant.is_mask = (cmd & LWMSG_CMD_MASK) == LWMSG_CMD_ENUM_MASK;
        iter->tag = (uint64_t) *(spec++);
        break;
    case LWMSG_CMD_TERMINATION:
    case LWMSG_CMD_TAG:
    case LWMSG_CMD_DISCRIM:
    case LWMSG_CMD_CUSTOM_ATTR:
        abort();
    case LWMSG_CMD_END:
        iter->kind = LWMSG_KIND_NONE;
        goto done;
    }

    /* Apply all attributes */
    for (;;)
    {
        iter->next = spec;

        cmd = *(spec++);

        if (cmd & LWMSG_FLAG_DEBUG)
        {
            spec += 2;
        }
        
        switch (cmd & LWMSG_CMD_MASK)
        {
        case LWMSG_CMD_TERMINATION:
            term = *(spec++);

            iter->info.kind_indirect.term = term;

            switch (term)
            {
            case LWMSG_TERM_STATIC:
                iter->info.kind_indirect.term_info.static_length = *(spec++);
                break;
            case LWMSG_TERM_MEMBER:
                iter->info.kind_indirect.term_info.member.offset = *(spec++);
                iter->info.kind_indirect.term_info.member.size = *(spec++);
                break;
            case LWMSG_TERM_ZERO:
                break;
            }
            break;
        case LWMSG_CMD_TAG:
            iter->tag = *(spec++);
            break;
        case LWMSG_CMD_DISCRIM:
            iter->info.kind_compound.discrim.offset = *(spec++);
            iter->info.kind_compound.discrim.size = *(spec++);
            break;
        case LWMSG_CMD_VERIFY:
            iter->verify = (LWMsgVerifyFunction) *(spec++);
            iter->verify_data = (void*) *(spec++);
            break;
        case LWMSG_CMD_RANGE:
            iter->attrs.flags |= LWMSG_TYPE_FLAG_RANGE;
            iter->attrs.range_low = (size_t) *(spec++);
            iter->attrs.range_high = (size_t) *(spec++);
            break;
        case LWMSG_CMD_NOT_NULL:
            iter->attrs.flags |= LWMSG_TYPE_FLAG_NOT_NULL;
            break;
        case LWMSG_CMD_ENCODING:
            iter->info.kind_indirect.encoding = (const char*) *(spec++);
            break;
        case LWMSG_CMD_SENSITIVE:
            iter->attrs.flags |= LWMSG_TYPE_FLAG_SENSITIVE;
            break;
        case LWMSG_CMD_ALIASABLE:
            iter->attrs.flags |= LWMSG_TYPE_FLAG_ALIASABLE;
            break;
        case LWMSG_CMD_CUSTOM_ATTR:
            iter->attrs.custom |= (size_t) *(spec++);
            break;
        case LWMSG_CMD_MAX_ALLOC:
            iter->attrs.max_alloc = (size_t) *(spec++);
            break;
        case LWMSG_CMD_END:
            /* No more members/types left to iterate */
            iter->next = NULL;
            goto done;
        default:
            goto done;
        }
    }

done:
    
    return;
}

void
lwmsg_type_iterate(
    LWMsgTypeSpec* spec,
    LWMsgTypeIter* iter
    )
{
    iter->spec = spec;
    iter->verify = NULL;
    iter->size = 0;
    iter->offset = 0;
    
    memset(&iter->attrs, 0, sizeof(iter->attrs));
    memset(&iter->meta, 0, sizeof(iter->meta));
    memset(&iter->debug, 0, sizeof(iter->debug));
    
    lwmsg_type_iterate_inner(spec, iter);
}

static void
lwmsg_type_promote(
    LWMsgTypeSpec* spec,
    LWMsgTypeIter* iter
    )
{
     memset(iter, 0, sizeof(*iter));
     iter->kind = LWMSG_KIND_POINTER;
     iter->info.kind_indirect.term = LWMSG_TERM_STATIC;
     iter->info.kind_indirect.term_info.static_length = 1;
     iter->inner = spec;
     iter->size = sizeof(void*);
     iter->attrs.flags = LWMSG_TYPE_FLAG_NOT_NULL | LWMSG_TYPE_FLAG_PROMOTED;
}

void
lwmsg_type_iterate_promoted(
    LWMsgTypeSpec* spec,
    LWMsgTypeIter* iter
    )
{
    switch (*spec & LWMSG_CMD_MASK)
    {
    case LWMSG_CMD_POINTER:
        lwmsg_type_iterate(spec, iter);
        break;
    case LWMSG_CMD_CUSTOM:
        lwmsg_type_iterate(spec, iter);
        if (!iter->info.kind_custom.typeclass->is_pointer)
        {
            lwmsg_type_promote(spec, iter);
        }
        break;
    default:
        lwmsg_type_promote(spec, iter);
        break;
    }
}
