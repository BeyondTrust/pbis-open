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
 *        data-print.c
 *
 * Abstract:
 *
 *        Data graph printing
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "data-private.h"
#include "util-private.h"
#include "type-private.h"
#include "convert.h"

#include <stdio.h>

typedef struct
{
    LWMsgObjectMap map;
    const LWMsgContext* context;
    unsigned int depth;
    LWMsgBool newline;
    LWMsgDataPrintFunction print;
    void* print_data;
    LWMsgTypeRep* dominating_rep;
} PrintInfo;

static
LWMsgStatus
lwmsg_data_print_graph_visit(
    LWMsgTypeIter* iter,
    unsigned char* object,
    void* data
    );

static
LWMsgStatus
lwmsg_data_print_type_internal(
    LWMsgDataContext* context,
    LWMsgTypeRep* rep,
    PrintInfo* info
    );

static
LWMsgStatus
print_wrap(
    const char* text,
    size_t length,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PrintInfo* info = data;
    int i;

    if (info->newline)
    {
        for (i = 0; i < info->depth; i++)
        {
            BAIL_ON_ERROR(status = info->print("  ", 2, info->print_data));
        }
        info->newline = LWMSG_FALSE;
    }

    BAIL_ON_ERROR(status = info->print(text, length, info->print_data));

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

    print_wrap(text, strlen(text), info);

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
    info->print("\n", 1, info->print_data);
    info->newline = LWMSG_TRUE;

    return LWMSG_STATUS_SUCCESS;
}

static
LWMsgStatus
lwmsg_data_print_integer(
    LWMsgTypeIter* iter,
    unsigned char* object,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    uintmax_t value;

    BAIL_ON_ERROR(status = 
                  lwmsg_convert_integer(
                      object,
                      iter->size,
                      LWMSG_NATIVE_ENDIAN,
                      &value,
                      sizeof(value),
                      LWMSG_NATIVE_ENDIAN,
                      iter->info.kind_integer.sign));

    if (iter->info.kind_integer.sign == LWMSG_SIGNED)
    {
        BAIL_ON_ERROR(status = print(info, "%lld", (long long int) value));
    }
    else
    {
        BAIL_ON_ERROR(status = print(info, "%llu", (unsigned long long int) value));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_data_print_enum(
    LWMsgTypeIter* iter,
    unsigned char* object,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    uint64_t value = 0;
    uint64_t mask = 0;
    uint64_t res = 0;
    LWMsgTypeIter var;

    BAIL_ON_ERROR(status = lwmsg_convert_integer(
                      object,
                      iter->size,
                      LWMSG_NATIVE_ENDIAN,
                      &value,
                      sizeof(value),
                      LWMSG_NATIVE_ENDIAN,
                      iter->info.kind_integer.sign));

    BAIL_ON_ERROR(status = lwmsg_data_decode_enum_value(
                      iter,
                      value,
                      &mask,
                      &res));

    for (lwmsg_type_enter(iter, &var);
         lwmsg_type_valid(&var);
         lwmsg_type_next(&var))
    {
        if (!var.info.kind_variant.is_mask && var.tag == res)
        {
            if (var.meta.type_name)
            {
                print(info, "%s", var.meta.type_name);
            }
            else
            {
                print(info,
                      iter->info.kind_integer.sign == LWMSG_UNSIGNED ? "%llu" : "%lli",
                      (unsigned long long) res);
            }

            if (mask)
            {
                print(info, " | ");
            }
        }
    }

    if (mask)
    {
        for (lwmsg_type_enter(iter, &var);
             lwmsg_type_valid(&var);
             lwmsg_type_next(&var))
        {
            if (var.info.kind_variant.is_mask && (mask & var.tag))
            {
                mask &= ~var.tag;

                if (var.meta.type_name)
                {
                    print(info, "%s", var.meta.type_name);
                }
                else
                {
                    print(info,
                          iter->info.kind_integer.sign == LWMSG_UNSIGNED ? "%llu" : " %lli",
                          (unsigned long long) res);
                }

                if (mask)
                {
                    print(info, " | ");
                }
            }
        }
    }
    else if (!res)
    {
        print(info, "0");
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_data_print_graph_visit_member(
    LWMsgTypeIter* iter,
    unsigned char* object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PrintInfo* info = (PrintInfo*) data;
    
    if (iter->meta.member_name)
    {
        BAIL_ON_ERROR(status = print(info, ".%s = ", iter->meta.member_name));
    }

    BAIL_ON_ERROR(status = 
                  lwmsg_data_print_graph_visit(
                      iter,
                      object,
                      data));

    BAIL_ON_ERROR(status = newline(info));

error:

    return status;
}

static
LWMsgStatus
lwmsg_data_print_string(
    LWMsgTypeIter* iter,
    unsigned char* object,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    void* input_string = NULL;
    void* output_string = NULL;
    size_t input_length = 0;
    ssize_t output_length = 0;
    size_t element_size = 0;
    size_t element_count = 0;

    if (iter->kind == LWMSG_KIND_POINTER)
    {
        input_string = *(void**) object;
    }
    else
    {
        input_string = object;
    }

    BAIL_ON_ERROR(status = lwmsg_data_calculate_indirect_metrics(
                      iter,
                      input_string,
                      &element_count,
                      &element_size));

    input_length = element_count * element_size;

    if (!strcmp(iter->info.kind_indirect.encoding, ""))
    {
        BAIL_ON_ERROR(status = info->print(input_string, input_length, info->print_data));
    }
    else
    {
        output_length = lwmsg_convert_string_alloc(
            input_string,
            input_length,
            (void**) (void*) &output_string,
            iter->info.kind_indirect.encoding,
            "");
        
        if (output_length == -1)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
        }

        BAIL_ON_ERROR(status = info->print(output_string, (size_t) output_length, info->print_data));
    }

error:

    if (output_string)
    {
        free(output_string);
    }

    return status;
}

static
LWMsgStatus
lwmsg_data_print_graph_visit(
    LWMsgTypeIter* iter,
    unsigned char* object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PrintInfo* info = (PrintInfo*) data;
    LWMsgObjectID id = 0;

    if (iter->attrs.flags & LWMSG_TYPE_FLAG_SENSITIVE)
    {
        BAIL_ON_ERROR(status = print(info, "<sensitive>"));
    }
    else
    {
        switch (iter->kind)
        {
        case LWMSG_KIND_UNION:
        case LWMSG_KIND_STRUCT:
            if (iter->meta.type_name)
            {
                BAIL_ON_ERROR(status = print(info, "<%s>", iter->meta.type_name));
            }
            else
            {
                BAIL_ON_ERROR(status = print(info, iter->kind == LWMSG_KIND_STRUCT ? "<struct>" : "<union>"));
            }
            BAIL_ON_ERROR(status = newline(info));
            BAIL_ON_ERROR(status = print(info, "{"));
            BAIL_ON_ERROR(status = newline(info));
            info->depth++;
            BAIL_ON_ERROR(status = lwmsg_data_visit_graph_children(
                              iter,
                              object,
                              lwmsg_data_print_graph_visit_member,
                              data));
            info->depth--;
            BAIL_ON_ERROR(status = print(info, "}"));
            break;
        case LWMSG_KIND_ARRAY:
        case LWMSG_KIND_POINTER:
            /* NULL case */
            if (iter->kind == LWMSG_KIND_POINTER && *(void**) object == NULL)
            {
                BAIL_ON_ERROR(status = print(info, "<null>"));
            }
            else
            {
                /* Print prefix */
                if (iter->attrs.flags & LWMSG_TYPE_FLAG_PROMOTED)
                {
                    /* Print nothing */
                }
                else if (iter->kind == LWMSG_KIND_ARRAY)
                {
                    BAIL_ON_ERROR(status = print(info, "<array> "));
                }
                else if (iter->attrs.flags & LWMSG_TYPE_FLAG_ALIASABLE)
                {
                    status = lwmsg_data_object_map_find_object(
                        &info->map,
                        *(void**) object,
                        &id);

                    if (status == LWMSG_STATUS_NOT_FOUND)
                    {
                        BAIL_ON_ERROR(status = lwmsg_data_object_map_insert(
                                          &info->map,
                                          *(void**) object,
                                          iter,
                                          &id));

                        BAIL_ON_ERROR(status = print(info, "<pointer:%lu> -> ", (unsigned long) id));
                    }
                    else
                    {
                        BAIL_ON_ERROR(status);

                        BAIL_ON_ERROR(status = print(info, "<pointer:%lu>", (unsigned long) id));
                        /* Skip printing pointee */
                        goto error;
                    }
                }

                /* Print pointee */

                /* String case */
                if (iter->info.kind_indirect.encoding != NULL)
                {
                    BAIL_ON_ERROR(status = print(info, "\""));
                    BAIL_ON_ERROR(status = lwmsg_data_print_string(iter, object, info));
                    BAIL_ON_ERROR(status = print(info, "\""));
                }
                /* Singleton case */
                else if (iter->info.kind_indirect.term == LWMSG_TERM_STATIC &&
                         iter->info.kind_indirect.term_info.static_length == 1)
                {
                    BAIL_ON_ERROR(status = lwmsg_data_visit_graph_children(
                                      iter,
                                      object,
                                      lwmsg_data_print_graph_visit,
                                      data));
                }
                /* General case */
                else
                {
                    BAIL_ON_ERROR(status = newline(info));
                    BAIL_ON_ERROR(status = print(info, "{"));
                    BAIL_ON_ERROR(status = newline(info));
                    info->depth++;
                    BAIL_ON_ERROR(status = lwmsg_data_visit_graph_children(
                                      iter,
                                      object,
                                      lwmsg_data_print_graph_visit_member,
                                      data));
                    info->depth--;
                    BAIL_ON_ERROR(status = print(info, "}"));
                }
            }
            break;
        case LWMSG_KIND_INTEGER:
            BAIL_ON_ERROR(status = lwmsg_data_print_integer(
                              iter,
                              object,
                              info));
            break;
        case LWMSG_KIND_ENUM:
            BAIL_ON_ERROR(status = lwmsg_data_print_enum(
                              iter,
                              object,
                              info));
            break;
        case LWMSG_KIND_CUSTOM:
            if (iter->info.kind_custom.typeclass->print)
            {
                BAIL_ON_ERROR(status = iter->info.kind_custom.typeclass->print(
                                  info->context,
                                  &iter->attrs,
                                  object,
                                  print_wrap,
                                  info,
                                  iter->info.kind_custom.typedata));
            }
            else
            {
                BAIL_ON_ERROR(status = print(info, "<custom>"));
            }
        case LWMSG_KIND_NONE:
        case LWMSG_KIND_VOID:
            break;
        }
    }
        
error:
    
    return status;
}

LWMsgStatus
lwmsg_data_print_graph(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    void* object,
    LWMsgDataPrintFunction print,
    void* print_data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter iter;
    PrintInfo info;

    memset(&info, 0, sizeof(info));

    info.newline = LWMSG_TRUE;
    info.depth = 0;
    info.context = context->context;
    info.print = print;
    info.print_data = print_data;

    lwmsg_type_iterate_promoted(type, &iter);
    
    BAIL_ON_ERROR(status = lwmsg_data_visit_graph(
                      &iter,
                      (unsigned char*) &object,
                      lwmsg_data_print_graph_visit,
                      &info));

error:

    lwmsg_data_object_map_destroy(&info.map);

    return status;
}

typedef struct print_alloc_info
{
    LWMsgDataContext* context;
    char* buffer;
    size_t buffer_size;
    size_t buffer_capacity;
} print_alloc_info;

static
LWMsgStatus
lwmsg_data_print_graph_alloc_print(
    const char* text,
    size_t length,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    print_alloc_info* info = (print_alloc_info*) data;
    size_t new_capacity = 0;
    char* new_buffer = NULL;

    while (info->buffer_size + (length + 1) > info->buffer_capacity)
    {
        if (info->buffer_capacity == 0)
        {
            new_capacity = 512;
        }
        else
        {
            new_capacity = info->buffer_capacity * 2;
        }

        BAIL_ON_ERROR(status = lwmsg_context_realloc(
                          info->context->context,
                          info->buffer,
                          info->buffer_capacity,
                          new_capacity,
                          (void**) (void*) &new_buffer));
        info->buffer = new_buffer;
        info->buffer_capacity = new_capacity;
    }

    memcpy(info->buffer + info->buffer_size, text, length);
    info->buffer[info->buffer_size + length] = '\0';
    info->buffer_size += length;

error:

    return status;
}

LWMsgStatus
lwmsg_data_print_graph_alloc(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    void* object,
    char** result
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    print_alloc_info info = {0};

    memset(&info, 0, sizeof(info));

    info.context = context;

    BAIL_ON_ERROR(status = lwmsg_data_print_graph(
                      context,
                      type,
                      object,
                      lwmsg_data_print_graph_alloc_print,
                      &info));

    *result = info.buffer;
   
cleanup:

    return status;

error:

    *result = NULL;

    if (info.buffer)
    {
        lwmsg_context_free(context->context, info.buffer);
    }

    goto cleanup;
}

static
LWMsgStatus
print_type_name(
    PrintInfo* info,
    LWMsgTypeRep* rep
    )
{
    if (rep->name)
    {
        return print(info, rep->name);
    }
    else
    {
        return print(info, "__type_%lx", (unsigned long) (size_t) rep);
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
    LWMsgFieldRep* field = &rep->info.struct_rep.fields[index];

    if (field->name)
    {
        return print(info, field->name);
    }
    else
    {
        return print(info, "__field_%lu", (unsigned long) index);
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
    LWMsgArmRep* arm = &rep->info.union_rep.arms[index];

    if (arm->name)
    {
        return print(info, arm->name);
    }
    else
    {
        return print(info, "__arm%lu", (unsigned long) index);
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
    LWMsgVariantRep* variant = &rep->info.enum_rep.variants[index];

    if (variant->name)
    {
        return print(info, variant->name);
    }
    else
    {
        return print(info, "__variant%lu", (unsigned long) index);
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
        for (index = 0; index < rep->info.enum_rep.variant_count; index++)
        {
            LWMsgVariantRep* variant = &rep->info.enum_rep.variants[index];

            if (variant->value == value)
            {
                return print(info, variant->name);
            }
        }
    }

    if (rep->info.enum_rep.sign == LWMSG_UNSIGNED)
    {
        return print(info, "%llu", (unsigned long long) index);
    }
    else
    {
        return print(info, "%lli", (signed long long) index);
    }
}


static
LWMsgStatus
lwmsg_data_print_type_enum(
    LWMsgDataContext* context,
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i = 0;
    LWMsgVariantRep* var = NULL;

    BAIL_ON_ERROR(status = print(
                      info, "%sint%u enum ",
                      rep->info.enum_rep.sign == LWMSG_UNSIGNED ? "u" : "",
                      rep->info.enum_rep.width * 8));

    BAIL_ON_ERROR(status = print_type_name(info, rep));

    if (!(rep->flags & 0x80000000))
    {
        rep->flags |= 0x80000000;

        BAIL_ON_ERROR(status = newline(info));
        BAIL_ON_ERROR(status = print(info, "{"));
        info->depth += 2;
        BAIL_ON_ERROR(status = newline(info));

        for (i = 0; i < rep->info.enum_rep.variant_count; i++)
        {
            var = &rep->info.enum_rep.variants[i];
            BAIL_ON_ERROR(status = print_variant_name(info, rep, i));
            BAIL_ON_ERROR(status = print(info, " = "));
            if (var->is_mask)
            {
                BAIL_ON_ERROR(status = print(
                                  info, "0x%.*llx",
                                  rep->info.enum_rep.width * 2,
                                  var->value));
            }
            else
            {
                BAIL_ON_ERROR(status = print(info, "%llu", (unsigned long long) var->value));
            }

            if (i < rep->info.enum_rep.variant_count - 1)
            {
                BAIL_ON_ERROR(status = print(info, ","));
            }
            BAIL_ON_ERROR(status = newline(info));
        }

        info->depth -= 2;
        BAIL_ON_ERROR(status = print(info, "}"));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_data_print_type_integer(
    LWMsgDataContext* context,
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = print(
                      info, "%sint%u",
                      rep->info.integer_rep.sign == LWMSG_UNSIGNED ? "u" : "",
                      rep->info.integer_rep.width * 8));

    if (rep->flags & LWMSG_TYPE_FLAG_RANGE)
    {
            BAIL_ON_ERROR(status = print(
                              info,
                              rep->info.integer_rep.sign == LWMSG_UNSIGNED ?
                              " <range=%llu,%llu>" : " <range=%lli,%lli>",
                              (unsigned long long) rep->info.integer_rep.lower_bound,
                              (unsigned long long) rep->info.integer_rep.upper_bound));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_data_print_type_struct(
    LWMsgDataContext* context,
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i = 0;
    LWMsgTypeRep* old_rep = NULL;


    BAIL_ON_ERROR(status = print(info, "struct "));
    BAIL_ON_ERROR(status = print_type_name(info, rep));

    if (!(rep->flags & 0x80000000))
    {
        rep->flags |= 0x80000000;

        old_rep = info->dominating_rep;
        info->dominating_rep = rep;

        BAIL_ON_ERROR(status = newline(info));
        BAIL_ON_ERROR(status = print(info, "{"));
        info->depth += 2;
        BAIL_ON_ERROR(status = newline(info));

        for (i = 0; i < rep->info.struct_rep.field_count; i++)
        {
            BAIL_ON_ERROR(status = lwmsg_data_print_type_internal(
                              context,
                              rep->info.struct_rep.fields[i].type,
                              info));
            BAIL_ON_ERROR(status = print(info, " "));
            BAIL_ON_ERROR(status = print_field_name(info, rep, i));
            BAIL_ON_ERROR(status = print(info, ";"));
            BAIL_ON_ERROR(status = newline(info));
        }

        info->dominating_rep = old_rep;

        info->depth -= 2;
        BAIL_ON_ERROR(status = print(info, "}"));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_data_print_type_union(
    LWMsgDataContext* context,
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i = 0;
    LWMsgTypeRep* discrim = NULL;

    BAIL_ON_ERROR(status = print(info, "union "));
    BAIL_ON_ERROR(status = print_type_name(info, rep));

    if (!(rep->flags & 0x80000000))
    {
        rep->flags |= 0x80000000;

        discrim = info->dominating_rep->info.struct_rep.fields[rep->info.union_rep.discrim_member_index].type;

        BAIL_ON_ERROR(status = newline(info));
        BAIL_ON_ERROR(status = print(info, "{"));
        info->depth += 2;
        BAIL_ON_ERROR(status = newline(info));

        for (i = 0; i < rep->info.union_rep.arm_count; i++)
        {
            BAIL_ON_ERROR(status = lwmsg_data_print_type_internal(
                              context,
                              rep->info.union_rep.arms[i].type,
                              info));

            BAIL_ON_ERROR(status = print(info, " "));
            BAIL_ON_ERROR(status = print_arm_name(info, rep, i));
            BAIL_ON_ERROR(status = print(info, " <tag="));
            BAIL_ON_ERROR(status = print_variant_value(info, discrim, rep->info.union_rep.arms[i].tag));
            BAIL_ON_ERROR(status = print(info, ">;"));
            BAIL_ON_ERROR(status = newline(info));
        }

        info->depth -= 2;
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
lwmsg_data_print_type_pointer(
    LWMsgDataContext* context,
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    const char* comma = "";

    BAIL_ON_ERROR(status = lwmsg_data_print_type_internal(
                          context,
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
lwmsg_data_print_type_array(
    LWMsgDataContext* context,
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_data_print_type_internal(
                          context,
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
lwmsg_data_print_type_custom(
    LWMsgDataContext* context,
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_data_print_type_internal(
                          context,
                          rep->info.custom_rep.transmitted_type,
                          info));
    BAIL_ON_ERROR(status = print(info, " <custom="));
    BAIL_ON_ERROR(status = print_type_name(info, rep));
    BAIL_ON_ERROR(status = print(info, ">"));

error:

    return status;
}

static
LWMsgStatus
lwmsg_data_print_type_internal(
    LWMsgDataContext* context,
    LWMsgTypeRep* rep,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    switch (rep->kind)
    {
    case LWMSG_KIND_INTEGER:
        BAIL_ON_ERROR(status = lwmsg_data_print_type_integer(
                          context,
                          rep,
                          info));
        break;
    case LWMSG_KIND_ENUM:
        BAIL_ON_ERROR(status = lwmsg_data_print_type_enum(
                          context,
                          rep,
                          info));
        break;
    case LWMSG_KIND_STRUCT:
        BAIL_ON_ERROR(status = lwmsg_data_print_type_struct(
                          context,
                          rep,
                          info));
        break;
    case LWMSG_KIND_UNION:
        BAIL_ON_ERROR(status = lwmsg_data_print_type_union(
                          context,
                          rep,
                          info));
        break;
    case LWMSG_KIND_POINTER:
        BAIL_ON_ERROR(status = lwmsg_data_print_type_pointer(
                          context,
                          rep,
                          info));
        break;
    case LWMSG_KIND_ARRAY:
        BAIL_ON_ERROR(status = lwmsg_data_print_type_array(
                          context,
                          rep,
                          info));
        break;
    case LWMSG_KIND_CUSTOM:
        BAIL_ON_ERROR(status = lwmsg_data_print_type_custom(
                          context,
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
lwmsg_data_print_type(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    LWMsgDataPrintFunction print,
    void* print_data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PrintInfo info;
    LWMsgTypeRep* rep = NULL;

    memset(&info, 0, sizeof(info));

    info.newline = LWMSG_TRUE;
    info.depth = 0;
    info.context = context->context;
    info.print = print;
    info.print_data = print_data;

    BAIL_ON_ERROR(status = lwmsg_type_rep_from_spec(context->context, type, &rep));

    BAIL_ON_ERROR(status = lwmsg_data_print_type_internal(
                      context,
                      rep,
                      &info));

error:

    if (rep)
    {
        status = lwmsg_data_free_graph(context, lwmsg_type_rep_spec, rep);
    }

    lwmsg_data_object_map_destroy(&info.map);

    return status;
}

LWMsgStatus
lwmsg_data_print_type_alloc(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    char** result
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    print_alloc_info info = {0};

    memset(&info, 0, sizeof(info));

    info.context = context;

    BAIL_ON_ERROR(status = lwmsg_data_print_type(
                      context,
                      type,
                      lwmsg_data_print_graph_alloc_print,
                      &info));

    *result = info.buffer;

cleanup:

    return status;

error:

    *result = NULL;

    if (info.buffer)
    {
        lwmsg_context_free(context->context, info.buffer);
    }

    goto cleanup;
}
