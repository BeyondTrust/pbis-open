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
 *        Application Context API (public header)
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
 * @brief Application context API
 */

/**
 * @defgroup context Application contexts
 * @ingroup public
 * @brief Application contexts
 *
 * The application context API allows you to customize the way
 * <tt>lwmsg</tt> integrates with your application.  You can override
 * the default memory allocator or register a logging callback to
 * receive logging messages from the library.  Application contexts
 * can be chained together in a hierarchy, with contexts lower
 * in the hierarchy inheriting default settings from their parents.
 */

/*@{*/

/**
 * @brief Log level
 * 
 * Represents the severity of a log message
 */
typedef enum LWMsgLogLevel
{
    /** Message should always be logged */
    LWMSG_LOGLEVEL_ALWAYS,
    /** Error message */
    LWMSG_LOGLEVEL_ERROR,
    /** Warning message */
    LWMSG_LOGLEVEL_WARNING,
    /** Informational message */
    LWMSG_LOGLEVEL_INFO,
    /** Verbose message */
    LWMSG_LOGLEVEL_VERBOSE,
    /** Debugging message */
    LWMSG_LOGLEVEL_DEBUG,
    /** Trace message */
    LWMSG_LOGLEVEL_TRACE
} LWMsgLogLevel;

/**
 * @brief Application context
 *
 * An opaque type which stores all application context information.
 */
typedef struct LWMsgContext LWMsgContext;

/**
 * @brief Callback to allocate a memory object
 * 
 * A callback used whenever memory that needs to be freed by
 * the user is allocated -- for example, when unmarshalling
 * a data structure.  The allocated space must initialized to zero.
 * A request for zero bytes should not return <tt>NULL</tt>.
 * 
 * @param[in] size the number of bytes to allocate
 * @param[out] out the allocated object
 * @param[in] data the user data pointer registered by #lwmsg_context_set_memory_functions()
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
 * @brief Callback to free a memory object
 *
 * A callback used to free allocated memory.
 *
 * @param[in,out] object the memory object to free
 * @param[in] data the user data pointer registered by lwmsg_context_set_memory_functions()
 */ 
typedef void
(*LWMsgFreeFunction) (
    void* object,
    void* data
    );

/**
 * @brief Callback to reallocate a memory object
 *
 * A callback used to reallocate a block of memory. If the
 * reallocation grows the block, the additional space must be
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

#ifndef DOXYGEN
typedef LWMsgStatus
(*LWMsgContextDataFunction) (
    const char* key,
    void** out_value,
    void* data
    );
#endif

/**
 * @brief Logging callback
 *
 * A callback which is invoked by <tt>lwmsg</tt> when it has something to log.
 * The function should indicate with its return value whether the message
 * was actually logged (#LWMSG_TRUE) or filtered (#LWMSG_FALSE).  If the message
 * parameter is NULL, the function should not log anything but still return
 * a value indicating whether it would have logged at the given log level.  This
 * mechanism is used to avoid expensive calculations to produce log messages
 * that would be filtered out anyway.
 *
 * @param[in] level the level of message
 * @param[in] message the message
 * @param[in] name the name of the function that logged the message
 * @param[in] filename the name of the source file where the message was logged
 * @param[in] line the line number where the message was logged
 * @param[in] data a user data pointer registered with #lwmsg_context_set_log_function().
 * @return #LWMSG_TRUE if a message was or would be logged, #LWMSG_FALSE otherwise
 */
typedef LWMsgBool
(*LWMsgLogFunction) (
    LWMsgLogLevel level,
    const char* message,
    const char* function,
    const char* filename,
    unsigned int line,
    void* data
    );

/**
 * @brief Create a new context
 *
 * Creates a new context with an optional parent.
 * Options not explicitly set in the context will be
 * inherited from the parent.
 *
 * @param parent an optional parent context
 * @param context the created context
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_context_new(
    const LWMsgContext* parent,
    LWMsgContext** context
    );

/**
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
 * @brief Set context memory management functions
 *
 * Sets the memory management callbacks associated with the context.
 * If a function is set to null and a parent context is available,
 * the function in the parent context will be used.
 *
 * @param[in,out] context the context
 * @param[in] alloc a callback to allocate memory
 * @param[in] free a callback to free memory
 * @param[in] realloc a callback to reallocate a memory block
 * @param[in] data a user data pointer which will be passed to the callbacks when they are invoked
 */
void
lwmsg_context_set_memory_functions(
    LWMsgContext* context,
    LWMsgAllocFunction alloc,
    LWMsgFreeFunction free,
    LWMsgReallocFunction realloc,
    void* data
    );

/**
 * @brief Get context memory management functions
 *
 * Gets the memory management callbacks associated with the context.
 * If a given function was not explicitly set, the default function
 * provided by <tt>lwmsg</tt> will be returned.
 *
 * @param[in] context the context
 * @param[out] alloc a callback to allocate memory
 * @param[out] free a callback to free memory
 * @param[out] realloc a callback to reallocate a memory block
 * @param[out] data a user data pointer which should be passed to the callbacks when they are invoked
 */
void
lwmsg_context_get_memory_functions(
    const LWMsgContext* context,
    LWMsgAllocFunction* alloc,
    LWMsgFreeFunction* free,
    LWMsgReallocFunction* realloc,
    void** data
    );

#ifndef DOXYGEN
void
lwmsg_context_set_data_function(
    LWMsgContext* context,
    LWMsgContextDataFunction fn,
    void* data
    );
#endif

/**
 * @brief Set log call function
 *
 * Sets a callback function which will be called whenever <tt>lwmsg</tt>
 * wishes to log a message.
 *
 * @param[in,out] context the context
 * @param[in] logfn the logging function
 * @param[in] data a user data pointer that will be passed to the logging function
 */
void
lwmsg_context_set_log_function(
    LWMsgContext* context,
    LWMsgLogFunction logfn,
    void* data
    );

#ifndef DOXYGEN
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
#endif
    
/*@}*/

#endif
