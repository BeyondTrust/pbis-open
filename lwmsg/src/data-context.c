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
 *        data-context.c
 *
 * Abstract:
 *
 *        Data context management
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>

#include "data-private.h"
#include "context-private.h"
#include "util-private.h"

#include <limits.h>

LWMsgStatus
lwmsg_data_context_new(
    const LWMsgContext* context,
    LWMsgDataContext** dcontext
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgDataContext* my_context = NULL;

    my_context = calloc(1, sizeof(*my_context));
    if (!my_context)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    my_context->context = context;
    my_context->byte_order = LWMSG_BIG_ENDIAN;

    *dcontext = my_context;

done:

    return status;

error:

    *dcontext = NULL;

    if (my_context)
    {
        free(my_context);
    }

    goto done;
}

void
lwmsg_data_context_delete(
    LWMsgDataContext* context
    )
{
    free(context);
}

void
lwmsg_data_context_set_byte_order(
    LWMsgDataContext* context,
    LWMsgByteOrder byte_order
    )
{
    context->byte_order = byte_order;
}

LWMsgByteOrder
lwmsg_data_context_get_byte_order(
    LWMsgDataContext* context
    )
{
    return context->byte_order;
}

const LWMsgContext*
lwmsg_data_context_get_context(
    LWMsgDataContext* context
    )
{
    return context->context;
}

LWMsgStatus
lwmsg_data_alloc_memory(
    LWMsgDataContext* context,
    size_t size,
    void** object
    )
{
    return lwmsg_context_alloc(context->context, size, object);
}

void
lwmsg_data_free_memory(
    LWMsgDataContext* context,
    void* object
    )
{
    lwmsg_context_free(context->context, object);
}

static
void*
lwmsg_data_object_map_get_key_id(
    const void* entry
    )
{
    return &((LWMsgObjectMapEntry*) entry)->id;
}

static
size_t
lwmsg_data_object_map_digest_id(
    const void* key
    )
{
    return *(LWMsgObjectID*) key;
}

static
LWMsgBool
lwmsg_data_object_map_equal_id(
    const void* key1,
    const void* key2
    )
{
    return *(LWMsgObjectID*) key1 == *(LWMsgObjectID*) key2;
}

static
void*
lwmsg_data_object_map_get_key_object(
    const void* entry
    )
{
    return ((LWMsgObjectMapEntry*) entry)->object;
}

static
size_t
lwmsg_data_object_map_digest_object(
    const void* key
    )
{
    return (size_t) key;
}

static
LWMsgBool
lwmsg_data_object_map_equal_object(
    const void* key1,
    const void* key2
    )
{
    return key1 == key2;
}

static
LWMsgStatus
lwmsg_data_object_map_init(
    LWMsgObjectMap* map
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (!map->hash_by_id.buckets)
    {
        BAIL_ON_ERROR(status = lwmsg_hash_init(
                          &map->hash_by_id,
                          11,
                          lwmsg_data_object_map_get_key_id,
                          lwmsg_data_object_map_digest_id,
                          lwmsg_data_object_map_equal_id,
                          offsetof(LWMsgObjectMapEntry, ring1)));

        BAIL_ON_ERROR(status = lwmsg_hash_init(
                          &map->hash_by_object,
                          11,
                          lwmsg_data_object_map_get_key_object,
                          lwmsg_data_object_map_digest_object,
                          lwmsg_data_object_map_equal_object,
                          offsetof(LWMsgObjectMapEntry, ring2)));
    }

error:

    return status;
}

LWMsgStatus
lwmsg_data_object_map_find_id(
    LWMsgObjectMap* map,
    LWMsgObjectID id,
    LWMsgTypeIter* iter,
    void** object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter inner;

    BAIL_ON_ERROR(status = lwmsg_data_object_map_init(map));

    LWMsgObjectMapEntry* entry = lwmsg_hash_find_key(&map->hash_by_id, &id);

    lwmsg_type_enter(iter, &inner);

    if (!entry)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }
    else if (entry->spec != inner.spec)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
    }

    *object = entry->object;

error:

    return status;
}

LWMsgStatus
lwmsg_data_object_map_find_object(
    LWMsgObjectMap* map,
    void* object,
    LWMsgObjectID* id
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_data_object_map_init(map));

    LWMsgObjectMapEntry* entry = lwmsg_hash_find_key(&map->hash_by_object, object);

    if (!entry)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    *id = entry->id;

error:

    return status;
}

LWMsgStatus
lwmsg_data_object_map_insert(
    LWMsgObjectMap* map,
    void* object,
    LWMsgTypeIter* iter,
    LWMsgObjectID* id
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgObjectMapEntry* entry = NULL;
    LWMsgTypeIter inner;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&entry));

    lwmsg_type_enter(iter, &inner);

    lwmsg_ring_init(&entry->ring1);
    lwmsg_ring_init(&entry->ring2);
    entry->object = object;
    entry->spec = inner.spec;

    if (*id != 0)
    {
        entry->id = *id;
    }
    else
    {
        if (map->next_id == 0)
        {
            map->next_id++;
        }
        else if (map->next_id == UINT32_MAX)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_OVERFLOW);
        }

        entry->id = map->next_id++;
    }

    lwmsg_hash_insert_entry(&map->hash_by_id, entry);
    lwmsg_hash_insert_entry(&map->hash_by_object, entry);

    *id = entry->id;

done:

    return status;

error:

    if (entry)
    {
        free(entry);
    }

    goto done;
}

void
lwmsg_data_object_map_destroy(
    LWMsgObjectMap* map
    )
{
    LWMsgHashIter iter = {0};
    LWMsgObjectMapEntry* entry = NULL;

    if (map->hash_by_id.buckets)
    {
        lwmsg_hash_iter_begin(&map->hash_by_id, &iter);
        while ((entry = lwmsg_hash_iter_next(&map->hash_by_id, &iter)))
        {
            lwmsg_hash_remove_entry(&map->hash_by_id, entry);
            free(entry);
        }
        lwmsg_hash_iter_end(&map->hash_by_id, &iter);

        lwmsg_hash_destroy(&map->hash_by_id);
    }

    if (map->hash_by_object.buckets)
    {
        lwmsg_hash_destroy(&map->hash_by_object);
    }
}

LWMsgStatus
lwmsg_data_raise(
    LWMsgDataContext* context,
    LWMsgTypeIter* iter,
    LWMsgStatus status,
    const char* function,
    const char* filename,
    unsigned int line,
    const char* format,
    ...
    )
{
    char* message = NULL;
    va_list ap;

    if (status)
    {
        if (format)
        {
            va_start(ap, format);
            message = lwmsg_formatv(format, ap);
            va_end(ap);
        }

        if (message)
        {
            if (iter->meta.member_name)
            {
                if (iter->meta.container_name)
                {
                    lwmsg_context_raise(
                        context->context,
                        status,
                        function,
                        filename,
                        line,
                        "%s.%s: %s",
                        iter->meta.container_name,
                        iter->meta.member_name,
                        message);
                }
                else
                {
                    lwmsg_context_raise(
                        context->context,
                        status,
                        function,
                        filename,
                        line,
                        ".%s: %s",
                        iter->meta.member_name,
                        message);
                }
            }
            else if (iter->meta.type_name)
            {
                lwmsg_context_raise(
                    context->context,
                    status,
                    function,
                    filename,
                    line,
                    "%s: %s",
                    iter->meta.type_name,
                    message);
            }
            else
            {
                lwmsg_context_raise(
                    context->context,
                    status,
                    function,
                    filename,
                    line,
                    "%s",
                    message);
            }
        }
        else
        {
            if (iter->meta.member_name)
            {
                if (iter->meta.container_name)
                {
                    lwmsg_context_raise(
                        context->context,
                        status,
                        function,
                        filename,
                        line,
                        "%s.%s",
                        iter->meta.container_name,
                        iter->meta.type_name,
                        iter->meta.member_name);
                }
                else
                {
                    lwmsg_context_raise(
                        context->context,
                        status,
                        function,
                        filename,
                        line,
                        ".%s",
                        iter->meta.type_name,
                        iter->meta.member_name);
                }
            }
            else if (iter->meta.type_name)
            {
                lwmsg_context_raise(
                    context->context,
                    status,
                    function,
                    filename,
                    line,
                    "%s",
                    iter->meta.type_name);
            }
            else
            {
                lwmsg_context_raise(
                    context->context,
                    status,
                    function,
                    filename,
                    line,
                    NULL);
            }
        }
    }

    return status;
}
