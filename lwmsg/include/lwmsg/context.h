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
 *        context.h
 *
 * Abstract:
 *
 *        Marshalling context API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_CONTEXT_H__
#define __LWMSG_CONTEXT_H__

#include <lwmsg/status.h>
#include <lwmsg/type.h>

#include <stdlib.h>

/**
 * @file context.h
 * @brief Low-level marshalling context API
 */

typedef enum LWMsgLogLevel
{
    LWMSG_LOGLEVEL_ERROR,
    LWMSG_LOGLEVEL_WARNING,
    LWMSG_LOGLEVEL_INFO,
    LWMSG_LOGLEVEL_VERBOSE,
    LWMSG_LOGLEVEL_DEBUG,
    LWMSG_LOGLEVEL_TRACE
} LWMsgLogLevel;

/**
 * @ingroup marshal
 * @brief A marshaller context
 *
 * An opaque type which stores all marshalling context information.
 * Contexts are not inherently thread-safe and should not be
 * concurrently modified.
 */
typedef struct LWMsgContext LWMsgContext;

/**
 * @ingroup marshal
 * @brief Callback to allocate a memory object
 * 
 * A callback used by the marshaller to allocate memory.
 * The allocated space must initialized to zero.
 * 
 * @param size the number of bytes to allocate
 * @param out the allocated object
 * @param data the user data pointer registered by lwmsg_context_set_memory_functions()
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
typedef LWMsgStatus
(*LWMsgAllocFunction) (
    size_t size,
    void** out,
    void* data
    );

/**
 * @ingroup marshal
 * @brief Callback to free a memory object
 *
 * A callback used by the marshaller to free allocated memory.
 *
 * @param object the memory object to free
 * @param data the user data pointer registered by lwmsg_context_set_memory_functions()
 */ 
typedef void
(*LWMsgFreeFunction) (
    void* object,
    void* data
    );

/**
 * @ingroup marshal
 * @brief Callback to reallocate a memory object
 *
 * A callback used by the marshaller to reallocate a block of memory.
 * If the reallocation grows the block, the additional space must be
 * initialized to zero.  If object is NULL, it should behave
 * as a simple allocation with the same semantics as #LWMsgAllocFunction.
 * 
 * @param[in,out] object the original memory object
 * @param[in] old_size the size of the original memory object
 * @param[in] new_size the desired size
 * @param[out] new_object the reallocated object
 * @param[in] data the user data pointer registered by lwmsg_context_set_memory_functions()
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
typedef LWMsgStatus
(*LWMsgReallocFunction) (
    void* object,
    size_t old_size,
    size_t new_size,
    void** new_object,
    void* data
    );

typedef LWMsgStatus
(*LWMsgContextDataFunction) (
    const char* key,
    void** out_value,
    void* data
    );

typedef LWMsgBool
(*LWMsgLogFunction) (
    LWMsgLogLevel level,
    const char* message,
    const char* filename,
    unsigned int line,
    void* data
    );

/**
 * @ingroup marshal
 * @brief Create a new context
 *
 * Creates a new context with an optional parent.
 * Options not explicitly set in the context will be
 * inherited from the parent.
 *
 * @param parent an optional parent context
 * @param context the created context
 * @return LWMSG_STATUS_SUCCESS on success or LWMSG_STATUS_MEMORY if out of memory
 */
LWMsgStatus
lwmsg_context_new(
    const LWMsgContext* parent,
    LWMsgContext** context
    );

/**
 * @ingroup marshal
 * @brief Delete a context
 *
 * Deletes a context.  It is the caller's responsibility to ensure that
 * no other context, protocols, associations, etc. still reference the
 * context.
 * @param context the context to delete
 */
void
lwmsg_context_delete(
    LWMsgContext* context
    );

/**
 * @ingroup marshal
 * @brief Set context memory management functions
 *
 * Sets the memory management callbacks associated with the context.
 * If a function is set to null and a parent context is available,
 * the function in the parent context will be used.
 *
 * @param context the context
 * @param alloc a callback to allocate memory
 * @param free a callback to free memory
 * @param realloc a callback to reallocate a memory block
 * @param data a user data pointer which will be passed to the callbacks when they are invoked
 */
void
lwmsg_context_set_memory_functions(
    LWMsgContext* context,
    LWMsgAllocFunction alloc,
    LWMsgFreeFunction free,
    LWMsgReallocFunction realloc,
    void* data
    );

void
lwmsg_context_get_memory_functions(
    const LWMsgContext* context,
    LWMsgAllocFunction* alloc,
    LWMsgFreeFunction* free,
    LWMsgReallocFunction* realloc,
    void** data
    );

void
lwmsg_context_set_data_function(
    LWMsgContext* context,
    LWMsgContextDataFunction fn,
    void* data
    );

void
lwmsg_context_set_log_function(
    LWMsgContext* context,
    LWMsgLogFunction logfn,
    void* data
    );

LWMsgStatus
lwmsg_context_alloc(
    const LWMsgContext* context,
    size_t size,
    void** object
    );

void
lwmsg_context_free(
    const LWMsgContext* context,
    void* object
    );

LWMsgStatus
lwmsg_context_realloc(
    const LWMsgContext* context,
    void* old_object,
    size_t old_size,
    size_t new_size,
    void** new_object
    );
    
/**
 * @ingroup marshal
 * @brief Fetch detailed error message
 *
 * Retrieves a human-readable description of the last marshalling error which occured.
 * The returned string becomes undefined when another function is called which uses this
 * context or the context is deleted.
 *
 * @param context the context
 * @param status the status code of the most recent error
 */
const char*
lwmsg_context_get_error_message(
    LWMsgContext* context,
    LWMsgStatus status
    );

#endif
