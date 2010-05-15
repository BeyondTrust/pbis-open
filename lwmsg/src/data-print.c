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
#include "protocol-private.h"
#include "buffer-private.h"
#include "convert-private.h"

#include <stdio.h>
#include <ctype.h>

typedef struct
{
    LWMsgObjectMap map;
    LWMsgDataContext* context;
    unsigned int depth;
    LWMsgBool newline;
    LWMsgBuffer* buffer;
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
lwmsg_data_print_integer(
    LWMsgTypeIter* iter,
    unsigned char* object,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    uintmax_t value;

    BAIL_ON_ERROR(status = lwmsg_convert_integer(
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
        BAIL_ON_ERROR(status = lwmsg_buffer_write(
                          info->buffer,
                          input_string,
                          input_length));
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

        BAIL_ON_ERROR(status = lwmsg_buffer_write(
                          info->buffer,
                          (const unsigned char*) output_string,
                          output_length));
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
lwmsg_data_print_hex_ascii(
    LWMsgTypeIter* iter,
    unsigned char* object,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t element_size = 0;
    size_t element_count = 0;
    unsigned char* input_string = NULL;
    size_t input_length = 0;
    size_t cluster = 0;
    size_t index = 0;

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

    BAIL_ON_ERROR(status = lwmsg_multiply_unsigned(element_count, element_size, &input_length));

    BAIL_ON_ERROR(status = print(info, "<hex>", (unsigned long) cluster));
    BAIL_ON_ERROR(status = newline(info));
    BAIL_ON_ERROR(status = print(info, "{"));
    info->depth += 4;
    BAIL_ON_ERROR(status = newline(info));

    for (cluster = 0; cluster < input_length; cluster += 16)
    {
        BAIL_ON_ERROR(status = print(info, "%.8lx  ", (unsigned long) cluster));

        for (index = cluster; index < cluster + 16; index++)
        {
            if (index % 8 == 0)
            {
                BAIL_ON_ERROR(status = print(info, " "));
            }
            
            if (index >= input_length)
            {
                BAIL_ON_ERROR(status = print(info, "  "));

                if (index < cluster + 15)
                {
                    BAIL_ON_ERROR(status = print(info, " "));
                }
            }
            else
            {
                BAIL_ON_ERROR(status = print(info, "%.2x", input_string[index]));

                if (index < cluster + 15)
                {
                    BAIL_ON_ERROR(status = print(info, " "));
                }
            }
        }

        BAIL_ON_ERROR(status = print(info, "  |"));

        for (index = cluster; index < cluster + 16; index++)
        {
            if (index >= input_length)
            {
                break;
            }
            else if (input_string[index] >= 32 && input_string[index] <= 126)
            {
                BAIL_ON_ERROR(status = print(info, "%c", input_string[index]));
            }
            else
            {
                BAIL_ON_ERROR(status = print(info, ".", input_string[index]));
            }
        }

        BAIL_ON_ERROR(status = print(info, "|"));

        BAIL_ON_ERROR(status = newline(info));
    }

    info->depth -= 4;
    BAIL_ON_ERROR(status = print(info, "}"));

error:

    return status;
}

static
LWMsgStatus
lwmsg_data_print_custom(
    LWMsgTypeIter* iter,
    unsigned char* object,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char* transmit_object = NULL;
    LWMsgTypeIter transmit_iter;
    LWMsgTypeClass* typeclass = iter->info.kind_custom.typeclass;

    if (typeclass->print)
    {
        BAIL_ON_ERROR(status = typeclass->print(
                          info->context,
                          &iter->attrs,
                          object,
                          iter->info.kind_custom.typedata,
                          info->buffer));
    }
    else
    {
        /* Convert the object to its transmitted form and print that */
        lwmsg_type_iterate(typeclass->transmit_type, &transmit_iter);

        /* Allocate space for the transmitted object */
        BAIL_ON_ERROR(status = LWMSG_ALLOC_ARRAY(
                          transmit_iter.size,
                          &transmit_object));

        /* Convert presented object into transmitted object */
        BAIL_ON_ERROR(status = iter->info.kind_custom.typeclass->marshal(
                          info->context,
                          &iter->attrs,
                          object,
                          transmit_object,
                          iter->info.kind_custom.typedata));

        /* Print the transmitted object */
        BAIL_ON_ERROR(status = lwmsg_data_visit_graph(
                          &transmit_iter,
                          transmit_object,
                          lwmsg_data_print_graph_visit,
                          info));
    }

error:

    if (transmit_object)
    {
        if (iter->info.kind_custom.typeclass->destroy_transmitted)
        {
            iter->info.kind_custom.typeclass->destroy_transmitted(
                info->context,
                &iter->attrs,
                transmit_object,
                iter->info.kind_custom.typedata);
        }
        free(transmit_object);
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
            info->depth += 4;
            BAIL_ON_ERROR(status = lwmsg_data_visit_graph_children(
                              iter,
                              object,
                              lwmsg_data_print_graph_visit_member,
                              data));
            info->depth -= 4;
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
                else
                {
                    BAIL_ON_ERROR(status = print(info, "<pointer> -> "));
                }

                /* Print pointee */

                /* String case */
                if (iter->info.kind_indirect.encoding != NULL)
                {
                    if (!strcmp(iter->info.kind_indirect.encoding, "hex+ascii"))
                    {
                        BAIL_ON_ERROR(status = lwmsg_data_print_hex_ascii(iter, object, info));
                    }
                    else
                    {
                        BAIL_ON_ERROR(status = print(info, "\""));
                        BAIL_ON_ERROR(status = lwmsg_data_print_string(iter, object, info));
                        BAIL_ON_ERROR(status = print(info, "\""));
                    }
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
                    info->depth += 4;
                    BAIL_ON_ERROR(status = lwmsg_data_visit_graph_children(
                                      iter,
                                      object,
                                      lwmsg_data_print_graph_visit_member,
                                      data));
                    info->depth -= 4;
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
            BAIL_ON_ERROR(status = lwmsg_data_print_custom(
                              iter,
                              object,
                              info));
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
    unsigned int indent,
    LWMsgBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter iter;
    PrintInfo info;

    memset(&info, 0, sizeof(info));

    info.newline = LWMSG_FALSE;
    info.depth = indent;
    info.context = context;
    info.buffer = buffer;

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
lwmsg_data_print_graph_alloc(
    LWMsgDataContext* context,
    LWMsgTypeSpec* type,
    void* object,
    char** result
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBuffer buffer = {0};
    unsigned char nul = 0;

    buffer.wrap = realloc_wrap;
    buffer.data = (void*) context->context;

    BAIL_ON_ERROR(status = lwmsg_data_print_graph(
                      context,
                      type,
                      object,
                      0,
                      &buffer));

    BAIL_ON_ERROR(status = lwmsg_buffer_write(&buffer, &nul, 1));
    
    *result = (char*) buffer.base;
   
cleanup:

    return status;

error:

    *result = NULL;

    if (buffer.base)
    {
        lwmsg_context_free(context->context, buffer.base);
    }

    goto cleanup;
}
