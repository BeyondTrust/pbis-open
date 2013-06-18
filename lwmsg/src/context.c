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
 *        context.c
 *
 * Abstract:
 *
 *        Marshalling context API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "context-private.h"
#include "status-private.h"
#include "util-private.h"
#include "type-private.h"
#include <lwmsg/type.h>

#include <stdlib.h>

static LWMsgStatus
lwmsg_context_default_alloc (
    size_t size,
    void** out,
    void* data)
{
    void* object = malloc(size == 0 ? 1 : size);

    if (!object)
    {
        return LWMSG_STATUS_MEMORY;
    }
    else
    {
        memset(object, 0, size);
        *out = object;
        return LWMSG_STATUS_SUCCESS;
    }
}

static
void
lwmsg_context_default_free (
    void* object,
    void* data
    )
{
    free(object);
}

static LWMsgStatus
lwmsg_context_default_realloc (
    void* object,
    size_t old_size,
    size_t new_size,
    void** new_object,
    void* data)
{
    void* nobj = realloc(object, new_size);

    if (!nobj)
    {
        return LWMSG_STATUS_MEMORY;
    }
    else
    {
        if (new_size > old_size)
        {
            memset(nobj + old_size, 0, new_size - old_size);
        }
        *new_object = nobj;
        return LWMSG_STATUS_SUCCESS;
    }
}

void
lwmsg_context_setup(
    LWMsgContext* context,
    const LWMsgContext* parent
    )
{
    memset(context, 0, sizeof(*context));
    context->parent = parent;
}

LWMsgStatus
lwmsg_context_new(
    const LWMsgContext* parent,
    LWMsgContext** context
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgContext* my_context = NULL;

    my_context = calloc(1, sizeof(*my_context));

    if (my_context == NULL)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    lwmsg_context_setup(my_context, parent);

    *context = my_context;

error:

    return status;
}

void
lwmsg_context_cleanup(LWMsgContext* context)
{
    return;
}

void
lwmsg_context_delete(LWMsgContext* context)
{
    lwmsg_context_cleanup(context);
    free(context);
}

void
lwmsg_context_set_memory_functions(
    LWMsgContext* context,
    LWMsgAllocFunction alloc,
    LWMsgFreeFunction free,
    LWMsgReallocFunction realloc,
    void* data
    )
{
    context->alloc = alloc;
    context->free = free;
    context->realloc = realloc;
    context->memdata = data;
}

void
lwmsg_context_get_memory_functions(
    const LWMsgContext* context,
    LWMsgAllocFunction* alloc,
    LWMsgFreeFunction* free,
    LWMsgReallocFunction* realloc,
    void** data
    )
{
    if (!context)
    {
        if (alloc)
        {
            *alloc = lwmsg_context_default_alloc;
        }
        if (free)
        {
            *free = lwmsg_context_default_free;
        }
        if (realloc)
        {
            *realloc = lwmsg_context_default_realloc;
        }
        if (data)
        {
            *data = NULL;
        }
    }
    else if (context->alloc)
    {
        if (alloc)
        {
            *alloc = context->alloc;
        }
        if (free)
        {
            *free = context->free;
        }
        if (realloc)
        {
            *realloc = context->realloc;
        }
        if (data)
        {
            *data = context->memdata;
        }
    }
    else
    {
        lwmsg_context_get_memory_functions(context->parent, alloc, free, realloc, data);
    }
}

void
lwmsg_context_set_data_function(
    LWMsgContext* context,
    LWMsgContextDataFunction fn,
    void* data
    )
{
    context->datafn = fn;
    context->datafndata = data;
}

void
lwmsg_context_set_log_function(
    LWMsgContext* context,
    LWMsgLogFunction logfn,
    void* data
    )
{
    context->logfn = logfn;
    context->logfndata = data;
}

static
void
lwmsg_context_get_log_function(
    const LWMsgContext* context,
    LWMsgLogFunction* logfn,
    void** logfndata
    )
{
    if (!context)
    {
        *logfn = NULL;
        *logfndata = NULL;
    }
    else if (context->logfn)
    {
        *logfn = context->logfn;
        *logfndata = context->logfndata;
    }
    else
    {
        lwmsg_context_get_log_function(context->parent, logfn, logfndata);
    }
}

LWMsgStatus
lwmsg_context_get_data(
    const LWMsgContext* context,
    const char* key,
    void** out_data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    for (; context; context = context->parent)
    {
        if (context->datafn)
        {
            status = context->datafn(key, out_data, context->datafndata);
            if (status == LWMSG_STATUS_NOT_FOUND)
            {
                status = LWMSG_STATUS_SUCCESS;
                continue;
            }
            BAIL_ON_ERROR(status);
            break;
        }
    }

error:

    return status;
}

void
lwmsg_context_log(
    const LWMsgContext* context,
    LWMsgLogLevel level,
    const char* function,
    const char* message,
    const char* filename,
    unsigned int line
    )
{
    LWMsgLogFunction logfn = NULL;
    void* logfndata = NULL;

    lwmsg_context_get_log_function(context, &logfn, &logfndata);

    if (logfn)
    {
        logfn(level, message, function, filename, line, logfndata);
    }
}

void
lwmsg_context_log_printf(
    const LWMsgContext* context,
    LWMsgLogLevel level,
    const char* function,
    const char* filename,
    unsigned int line,
    const char* format,
    ...
    )
{
    LWMsgLogFunction logfn = NULL;
    void* logfndata = NULL;
    char* message = NULL;
    va_list ap;

    lwmsg_context_get_log_function(context, &logfn, &logfndata);

    if (logfn)
    {
        va_start(ap, format);
        message = lwmsg_formatv(format, ap);
        va_end(ap);

        if (message)
        {
            logfn(level, message, function, filename, line, logfndata);
            free(message);
        }
    }
}

LWMsgStatus
lwmsg_context_raise(
    const LWMsgContext* context,
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

    if (format)
    {
        va_start(ap, format);
        message = lwmsg_formatv(format, ap);
        va_end(ap);
    }

    if (message)
    {
        lwmsg_context_log_printf(
            context,
            LWMSG_LOGLEVEL_ERROR,
            function,
            filename,
            line,
            "%s: %s",
            lwmsg_status_name(status),
            message);
    }
    else
    {
        lwmsg_context_log_printf(
            context,
            LWMSG_LOGLEVEL_ERROR,
            function,
            filename,
            line,
            "%s",
            lwmsg_status_name(status));
    }

    return status;
}

LWMsgStatus
lwmsg_context_alloc(
    const LWMsgContext* context,
    size_t size,
    void** object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAllocFunction fn_alloc = NULL;
    LWMsgReallocFunction fn_realloc = NULL;
    void* data = NULL;

    lwmsg_context_get_memory_functions(context, &fn_alloc, NULL, &fn_realloc, &data);
    
    if (fn_alloc)
    {
        BAIL_ON_ERROR(status = fn_alloc(size, object, data));
    }
    else if (fn_realloc)
    {
        BAIL_ON_ERROR(status = fn_realloc(NULL, 0, size, object, data));
    }
    else
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_UNSUPPORTED);
    }

cleanup:

    return status;

error:

    *object = NULL;

    goto cleanup;
}

void
lwmsg_context_free(
    const LWMsgContext* context,
    void* object
    )
{
    LWMsgFreeFunction fn_free = NULL;
    void* data = NULL;

    lwmsg_context_get_memory_functions(context, NULL, &fn_free, NULL, &data);
    
    fn_free(object, data);
}

LWMsgStatus
lwmsg_context_realloc(
    const LWMsgContext* context,
    void* old_object,
    size_t old_size,
    size_t new_size,
    void** new_object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAllocFunction fn_alloc = NULL;
    LWMsgFreeFunction fn_free = NULL;
    LWMsgReallocFunction fn_realloc = NULL;
    void* data = NULL;
    
    lwmsg_context_get_memory_functions(context, &fn_alloc, &fn_free, &fn_realloc, &data);
    
    if (fn_realloc)
    {
        BAIL_ON_ERROR(status = fn_realloc(old_object, old_size, new_size, new_object, data));
    }
    else if (fn_alloc && fn_free)
    {
        BAIL_ON_ERROR(status = fn_alloc(new_size, new_object, data));
        memcpy(*new_object, old_object, new_size < old_size ? new_size : old_size);
        fn_free(old_object, data);
    }
    else
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_UNSUPPORTED);
    }

cleanup:

    return status;

error:

    *new_object = NULL;

    goto cleanup;
}

LWMsgBool
lwmsg_context_would_log(
    const LWMsgContext* context,
    LWMsgLogLevel level
    )
{
    LWMsgLogFunction logfn = NULL;
    void* logfndata = NULL;

    lwmsg_context_get_log_function(context, &logfn, &logfndata);

    if (logfn)
    {
        return logfn(level, NULL, NULL, NULL, 0, logfndata);
    }
    else
    {
        return LWMSG_FALSE;
    }
}

#define MEMLIST_ALIGN(x,a) (((x)+(a)-1) & ~((a)-1))
#define MEMLIST_HEADER_SIZE (MEMLIST_ALIGN(sizeof(LWMsgRing),sizeof(void*)))

static
LWMsgStatus
lwmsg_memlist_alloc(
    size_t size,
    void** out,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMemoryList* list = data;
    LWMsgRing* ring = NULL;

    size += MEMLIST_HEADER_SIZE;

    BAIL_ON_ERROR(status = lwmsg_context_alloc(
                      list->parent_context,
                      size,
                      (void**) (void*) &ring));
    
    lwmsg_ring_init(ring);
    lwmsg_ring_enqueue(&list->blocks, ring);

    *out = ((unsigned char*) ring) + MEMLIST_HEADER_SIZE;

error:

    return status;
}

static
void
lwmsg_memlist_free(
    void* object,
    void* data
    )
{
    LWMsgMemoryList* list = data;
    LWMsgRing* ring = NULL;

    ring = (LWMsgRing*) (((unsigned char*) object) - MEMLIST_HEADER_SIZE);

    lwmsg_ring_remove(ring);

    lwmsg_context_free(list->parent_context, object);
}

static LWMsgStatus
lwmsg_memlist_realloc (
    void* object,
    size_t old_size,
    size_t new_size,
    void** new_object,
    void* data)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMemoryList* list = data;
    LWMsgRing* ring = NULL;
    LWMsgRing* new_ring = NULL;

    ring = (LWMsgRing*) (((unsigned char*) object) - MEMLIST_HEADER_SIZE);
    lwmsg_ring_remove(ring);

    BAIL_ON_ERROR(status = lwmsg_context_realloc(
                      list->parent_context,
                      ring,
                      old_size + MEMLIST_HEADER_SIZE,
                      new_size + MEMLIST_HEADER_SIZE,
                      (void**) (void*) &new_ring));

    lwmsg_ring_init(new_ring);
    lwmsg_ring_enqueue(&list->blocks, new_ring);

    *new_object = ((unsigned char*) new_ring) + MEMLIST_HEADER_SIZE;
    
error:

    return status;
}

void
lwmsg_memlist_init(
    LWMsgMemoryList* list,
    LWMsgContext* context
    )
{
    list->parent_context = context;

    lwmsg_context_setup(&list->context, context);
    lwmsg_context_set_memory_functions(
        &list->context,
        lwmsg_memlist_alloc,
        lwmsg_memlist_free,
        lwmsg_memlist_realloc,
        list);
    lwmsg_ring_init(&list->blocks);
}

void
lwmsg_memlist_destroy(
    LWMsgMemoryList* list
    )
{
    LWMsgRing* ring = NULL;
    LWMsgRing* next = NULL;

    for (ring = list->blocks.next; ring != &list->blocks; ring = next)
    {
        next = ring->next;

        lwmsg_context_free(list->parent_context, ring);
    }

    lwmsg_context_cleanup(&list->context);
}
