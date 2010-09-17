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
 *        type-print.c
 *
 * Abstract:
 *
 *        Type representation printing support
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "config.h"
#include "type-private.h"
#include "util-private.h"
#include "data-private.h"
#include "buffer-private.h"

typedef struct
{
    const LWMsgContext* context;
    unsigned int depth;
    LWMsgBool newline;
    LWMsgBuffer* buffer;
    void* print_data;
    LWMsgTypeRep* dominating_rep;
    LWMsgObjectMap map;
} PrintInfo;

static
LWMsgStatus
lwmsg_type_print_internal(
    LWMsgTypeRep* rep,
    PrintInfo* info
    );

static
LWMsgStatus
print_wrap(
    PrintInfo* info,
    const char* text,
    size_t length
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int i;
    unsigned char indent[1] = {' '};

    if (info->newline)
    {
        for (i = 0; i < info->depth; i++)
        {
            BAIL_ON_ERROR(status = lwmsg_buffer_write(info->buffer, indent, sizeof(indent)));
        }
        info->newline = LWMSG_FALSE;
    }

    BAIL_ON_ERROR(status = lwmsg_buffer_write(info->buffer, (unsigned char*) text, length));

error:

    return status;
}

static
LWMsgStatus
print(
    PrintInfo* info,
    const char* fmt,
    ...
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    va_list ap;
    char* text = NULL;

    va_start(ap, fmt);
    text = lwmsg_formatv(fmt, ap);
    va_end(ap);

    if (!text)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    print_wrap(info, text, strlen(text));

error:

    if (text)
    {
        free(text);
    }

    return status;
}

static
LWMsgStatus
newline(
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char nl[1] = {'\n'};

    BAIL_ON_ERROR(status = lwmsg_buffer_write(
                      info->buffer,
                      nl,
                      sizeof(nl)));
    
    info->newline = LWMSG_TRUE;

error:

    return status;
}

static
LWMsgStatus
newline_if(
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (!info->newline)
    {
        status = newline(info);
    }

    return status;
}

static
LWMsgStatus
print_type_name(
    PrintInfo* info,
    LWMsgTypeRep* rep
    )
{
    const char* kind = NULL;
    char* name = NULL;

    switch(rep->kind)
    {
    case LWMSG_KIND_ENUM:
        kind = "enum";
        name = rep->info.enum_rep.definition->name;
        break;
    case LWMSG_KIND_STRUCT:
        kind = "struct";
        name = rep->info.struct_rep.definition->name;
        break;
    case LWMSG_KIND_UNION:
        kind = "union";
        name = rep->info.union_rep.definition->name;
        break;
    default:
        return LWMSG_STATUS_INTERNAL;
        break;
    }

    if (name)
    {
        return print(info, name);
    }
    else
    {
        return print(info, "<anonymous>", kind);
    }
}

static
LWMsgStatus
print_field_name(
    PrintInfo* info,
    LWMsgTypeRep* rep,
    size_t index
    )
{
    LWMsgFieldRep* field = &rep->info.struct_rep.definition->fields[index];

    if (field->name)
    {
        return print(info, field->name);
    }
    else
    {
        return print(info, "field%lu", (unsigned long) index);
    }
}

static
LWMsgStatus
print_arm_name(
    PrintInfo* info,
    LWMsgTypeRep* rep,
    size_t index
    )
{
    LWMsgArmRep* arm = &rep->info.union_rep.definition->arms[index];

    if (arm->name)
    {
        return print(info, arm->name);
    }
    else
    {
        return print(info, "arm%lu", (unsigned long) index);
    }
}

static
LWMsgStatus
print_variant_name(
    PrintInfo* info,
    LWMsgTypeRep* rep,
    size_t index
    )
{
    LWMsgVariantRep* variant = &rep->info.enum_rep.definition->variants[index];

    if (variant->name)
    {
        return print(info, variant->name);
    }
    else
    {
        return print(info, "variant%lu", (unsigned long) index);
    }
}

static
LWMsgStatus
print_variant_value(
    PrintInfo* info,
    LWMsgTypeRep* rep,
    uint64_t value
    )
{
    size_t index = 0;

    if (rep->kind == LWMSG_KIND_ENUM)
    {
        for (index = 0; index < rep->info.enum_rep.definition->variant_count; index++)
        {
            LWMsgVariantRep* variant = &rep->info.enum_rep.definition->variants[index];

            if (variant->value == value && variant->name)
            {
                return print(info, variant->name);
            }
        }
    }

    if (rep->info.enum_rep.definition->sign == LWMSG_UNSIGNED)
    {
        return print(info, "%llu", (unsigned long long) value);
    }
    else
    {
        return print(info, "%lli", (signed long long) value);
    }
}


static
LWMsgStatus
lwmsg_type_print_enum(
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i = 0;
    LWMsgVariantRep* var = NULL;

    if (!rep->info.enum_rep.definition->seen)
    {
        BAIL_ON_ERROR(status = newline_if(info));
    }
    BAIL_ON_ERROR(status = print(
                      info, "%sint%u enum ",
                      rep->info.enum_rep.definition->sign == LWMSG_UNSIGNED ? "u" : "",
                      rep->info.enum_rep.definition->width * 8));

    BAIL_ON_ERROR(status = print_type_name(info, rep));

    if (!rep->info.enum_rep.definition->seen)
    {
        rep->info.enum_rep.definition->seen = LWMSG_TRUE;

        BAIL_ON_ERROR(status = newline(info));
        BAIL_ON_ERROR(status = print(info, "{"));
        info->depth += 4;
        BAIL_ON_ERROR(status = newline(info));

        for (i = 0; i < rep->info.enum_rep.definition->variant_count; i++)
        {
            var = &rep->info.enum_rep.definition->variants[i];
            BAIL_ON_ERROR(status = print_variant_name(info, rep, i));
            BAIL_ON_ERROR(status = print(info, " = "));
            if (var->is_mask)
            {
                BAIL_ON_ERROR(status = print(
                                  info, "0x%.*llx",
                                  rep->info.enum_rep.definition->width * 2,
                                  var->value));
            }
            else
            {
                BAIL_ON_ERROR(status = print(info, "%llu", (unsigned long long) var->value));
            }

            if (i < rep->info.enum_rep.definition->variant_count - 1)
            {
                BAIL_ON_ERROR(status = print(info, ","));
            }
            BAIL_ON_ERROR(status = newline(info));
        }

        info->depth -= 4;
        BAIL_ON_ERROR(status = print(info, "}"));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_print_integer(
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = print(
                      info, "%sint%u",
                      rep->info.integer_rep.definition->sign == LWMSG_UNSIGNED ? "u" : "",
                      rep->info.integer_rep.definition->width * 8));

    if (rep->flags & LWMSG_TYPE_FLAG_RANGE)
    {
            BAIL_ON_ERROR(status = print(
                              info,
                              rep->info.integer_rep.definition->sign == LWMSG_UNSIGNED ?
                              " <range=%llu,%llu>" : " <range=%lli,%lli>",
                              (unsigned long long) rep->info.integer_rep.lower_bound,
                              (unsigned long long) rep->info.integer_rep.upper_bound));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_print_struct(
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i = 0;
    LWMsgTypeRep* old_rep = NULL;


    if (!rep->info.struct_rep.definition->seen)
    {
        BAIL_ON_ERROR(status = newline_if(info));
    }
    BAIL_ON_ERROR(status = print(info, "struct "));
    BAIL_ON_ERROR(status = print_type_name(info, rep));

    if (!rep->info.struct_rep.definition->seen)
    {
        rep->info.struct_rep.definition->seen = LWMSG_TRUE;

        old_rep = info->dominating_rep;
        info->dominating_rep = rep;

        BAIL_ON_ERROR(status = newline(info));
        BAIL_ON_ERROR(status = print(info, "{"));
        info->depth += 4;
        BAIL_ON_ERROR(status = newline(info));

        for (i = 0; i < rep->info.struct_rep.definition->field_count; i++)
        {
            BAIL_ON_ERROR(status = lwmsg_type_print_internal(
                              rep->info.struct_rep.definition->fields[i].type,
                              info));
            BAIL_ON_ERROR(status = print(info, " "));
            BAIL_ON_ERROR(status = print_field_name(info, rep, i));
            BAIL_ON_ERROR(status = print(info, ";"));
            BAIL_ON_ERROR(status = newline(info));
        }

        info->dominating_rep = old_rep;

        info->depth -= 4;
        BAIL_ON_ERROR(status = print(info, "}"));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_print_union(
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i = 0;
    LWMsgTypeRep* discrim = NULL;

    if (!rep->info.union_rep.definition->seen)
    {
        BAIL_ON_ERROR(status = newline_if(info));
    }
    BAIL_ON_ERROR(status = print(info, "union "));
    BAIL_ON_ERROR(status = print_type_name(info, rep));

    if (!rep->info.union_rep.definition->seen)
    {
        rep->info.union_rep.definition->seen = LWMSG_TRUE;

        discrim = info->dominating_rep->info.struct_rep.definition->fields[rep->info.union_rep.discrim_member_index].type;

        BAIL_ON_ERROR(status = newline(info));
        BAIL_ON_ERROR(status = print(info, "{"));
        info->depth += 4;
        BAIL_ON_ERROR(status = newline(info));

        for (i = 0; i < rep->info.union_rep.definition->arm_count; i++)
        {
            BAIL_ON_ERROR(status = lwmsg_type_print_internal(
                              rep->info.union_rep.definition->arms[i].type,
                              info));

            BAIL_ON_ERROR(status = print(info, " "));
            BAIL_ON_ERROR(status = print_arm_name(info, rep, i));
            BAIL_ON_ERROR(status = print(info, " <tag="));
            BAIL_ON_ERROR(status = print_variant_value(info, discrim, rep->info.union_rep.definition->arms[i].tag));
            BAIL_ON_ERROR(status = print(info, ">;"));
            BAIL_ON_ERROR(status = newline(info));
        }

        info->depth -= 4;
        BAIL_ON_ERROR(status = print(info, "} <discrim="));
        BAIL_ON_ERROR(status = print_field_name(
                          info,
                          info->dominating_rep,
                          rep->info.union_rep.discrim_member_index));
        BAIL_ON_ERROR(status = print(info, ">"));
    }


error:

    return status;
}

static
LWMsgStatus
lwmsg_type_print_pointer(
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    const char* comma = "";

    BAIL_ON_ERROR(status = lwmsg_type_print_internal(
                          rep->info.pointer_rep.pointee_type,
                          info));

    if (!(rep->flags & LWMSG_TYPE_FLAG_PROMOTED))
    {
        if (rep->flags & LWMSG_TYPE_FLAG_NOT_NULL)
        {
            BAIL_ON_ERROR(status = print(info, "&"));
        }
        else
        {
            BAIL_ON_ERROR(status = print(info, "*"));
        }

        if (rep->flags & LWMSG_TYPE_FLAG_ALIASABLE)
        {
            BAIL_ON_ERROR(status = print(info, "!"));
        }
    }

    if ((rep->info.pointer_rep.static_length && rep->info.pointer_rep.static_length != 1) ||
        rep->info.pointer_rep.zero_terminated ||
        (rep->info.pointer_rep.encoding && *rep->info.pointer_rep.encoding) ||
        rep->info.pointer_rep.length_member_index >= 0 ||
        (rep->flags & LWMSG_TYPE_FLAG_SENSITIVE))
    {
        BAIL_ON_ERROR(status = print(info," <"));

        if (rep->info.pointer_rep.static_length && rep->info.pointer_rep.static_length != 1)
        {
            BAIL_ON_ERROR(status = print(info, "%slength=%lu", comma, rep->info.pointer_rep.static_length));
            comma = ",";
        }

        if (rep->info.pointer_rep.zero_terminated && rep->info.pointer_rep.encoding)
        {
            BAIL_ON_ERROR(status = print(info, "%sstring", comma));
            comma = ",";
        }
        else if (rep->info.pointer_rep.zero_terminated)
        {
            BAIL_ON_ERROR(status = print(info, "%szero", comma));
            comma = ",";
        }

        if (rep->info.pointer_rep.encoding && *rep->info.pointer_rep.encoding)
        {
            BAIL_ON_ERROR(status = print(info, "%sencoding=%s", comma, rep->info.pointer_rep.encoding));
            comma = ",";
        }

        if (rep->info.pointer_rep.length_member_index >= 0)
        {
            BAIL_ON_ERROR(status = print(info, "%slength=", comma));
            BAIL_ON_ERROR(status = print_field_name(
                              info,
                              info->dominating_rep,
                              rep->info.pointer_rep.length_member_index));
            comma = ",";
        }

        if (rep->flags & LWMSG_TYPE_FLAG_SENSITIVE)
        {
            BAIL_ON_ERROR(status = print(info, "%ssensitive", comma));
            comma = ",";
        }

        BAIL_ON_ERROR(status = print(info, ">"));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_print_array(
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_type_print_internal(
                          rep->info.array_rep.element_type,
                          info));

    if (rep->info.array_rep.static_length)
    {
        BAIL_ON_ERROR(status = print(info, "[%lu]", rep->info.array_rep.static_length));
    }
    else if (rep->info.array_rep.length_member_index >= 0)
    {
        BAIL_ON_ERROR(status = print(info, "["));
        BAIL_ON_ERROR(status = print_field_name(
                          info,
                          info->dominating_rep,
                          rep->info.array_rep.length_member_index));
        BAIL_ON_ERROR(status = print(info, "]"));
    }
    else
    {
        BAIL_ON_ERROR(status = print(info, "[]", rep->info.array_rep.static_length));
    }

    if (rep->info.array_rep.zero_terminated)
    {
        BAIL_ON_ERROR(status = print(info, " <zero>", rep->info.array_rep.static_length));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_print_internal(
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    switch (rep->kind)
    {
    case LWMSG_KIND_INTEGER:
        BAIL_ON_ERROR(status = lwmsg_type_print_integer(
                          rep,
                          info));
        break;
    case LWMSG_KIND_ENUM:
        BAIL_ON_ERROR(status = lwmsg_type_print_enum(
                          rep,
                          info));
        break;
    case LWMSG_KIND_STRUCT:
        BAIL_ON_ERROR(status = lwmsg_type_print_struct(
                          rep,
                          info));
        break;
    case LWMSG_KIND_UNION:
        BAIL_ON_ERROR(status = lwmsg_type_print_union(
                          rep,
                          info));
        break;
    case LWMSG_KIND_POINTER:
        BAIL_ON_ERROR(status = lwmsg_type_print_pointer(
                          rep,
                          info));
        break;
    case LWMSG_KIND_ARRAY:
        BAIL_ON_ERROR(status = lwmsg_type_print_array(
                          rep,
                          info));
        break;
    case LWMSG_KIND_VOID:
        BAIL_ON_ERROR(status = print(info, "void"));
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        break;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_type_print_rep(
    LWMsgTypeRep* rep,
    unsigned int indent,
    LWMsgBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PrintInfo info;

    memset(&info, 0, sizeof(info));

    info.newline = LWMSG_FALSE;
    info.depth = indent;
    info.buffer = buffer;

    BAIL_ON_ERROR(status = lwmsg_type_print_internal(
                      rep,
                      &info));
    
error:
    
    lwmsg_data_object_map_destroy(&info.map);
    
    return status;
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
lwmsg_type_print_spec_alloc(
    const LWMsgContext* context,
    LWMsgTypeSpec* type,
    char** result
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBuffer buffer = {0};
    LWMsgTypeRep* rep = NULL;
    unsigned char nul = 0;

    buffer.wrap = realloc_wrap;
    buffer.data = (void*) context;
    
    BAIL_ON_ERROR(status = lwmsg_type_rep_from_spec(NULL, type, &rep));

    BAIL_ON_ERROR(status = lwmsg_type_print_rep(rep, 0, &buffer));
    
    BAIL_ON_ERROR(status = lwmsg_buffer_write(&buffer, &nul, 1));

    *result = (char*) buffer.base;
    
cleanup:

    lwmsg_type_free_rep(NULL, rep);
    
    return status;
    
error:

    *result = NULL;

    if (buffer.base)
    {
        lwmsg_context_free(context, buffer.base);
    }

    goto cleanup;
}
