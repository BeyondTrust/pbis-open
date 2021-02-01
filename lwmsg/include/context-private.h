/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Module Name:
 *
 *        context-private.h
 *
 * Abstract:
 *
 *        Marshalling context API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_CONTEXT_PRIVATE_H__
#define __LWMSG_CONTEXT_PRIVATE_H__

#include <lwmsg/context.h>

#include "type-private.h"
#include "status-private.h"

struct LWMsgContext
{
    LWMsgAllocFunction alloc;
    LWMsgFreeFunction free;
    LWMsgReallocFunction realloc;
    void* memdata;
    LWMsgContextDataFunction datafn;
    void* datafndata;
    LWMsgLogFunction logfn;
    void* logfndata;
    
    const struct LWMsgContext* parent;
};

typedef struct LWMsgMemoryList
{
    LWMsgContext context;
    LWMsgContext* parent_context;
    LWMsgRing blocks;
} LWMsgMemoryList;

void
lwmsg_context_setup(
    LWMsgContext* context,
    const LWMsgContext* parent
    );

void
lwmsg_context_cleanup(
    LWMsgContext* context
    );

LWMsgStatus
lwmsg_context_get_data(
    const LWMsgContext* context,
    const char* key,
    void** out_data
    );

void
lwmsg_context_log(
    const LWMsgContext* context,
    LWMsgLogLevel level,
    const char* message,
    const char* function,
    const char* filename,
    unsigned int line
    );

void
lwmsg_context_log_printf(
    const LWMsgContext* context,
    LWMsgLogLevel level,
    const char* function,
    const char* filename,
    unsigned int line,
    const char* format,
    ...
    );

LWMsgStatus
lwmsg_context_raise(
    const LWMsgContext* context,
    LWMsgStatus status,
    const char* function,
    const char* filename,
    unsigned int line,
    const char* format,
    ...
    );

LWMsgBool
lwmsg_context_would_log(
    const LWMsgContext* context,
    LWMsgLogLevel level
    );

void
lwmsg_memlist_init(
    LWMsgMemoryList* list,
    LWMsgContext* context
    );

void
lwmsg_memlist_destroy(
    LWMsgMemoryList* list
    );

static inline
LWMsgContext*
lwmsg_memlist_context(
    LWMsgMemoryList* list
    )
{
    return &list->context;
}

#define LWMSG_LOG(context, level, ...) \
    (lwmsg_context_log_printf((context), (level), __func__, __FILE__, __LINE__, __VA_ARGS__))

#define LWMSG_LOG_ALWAYS(context, ...) LWMSG_LOG(context, LWMSG_LOGLEVEL_ALWAYS, __VA_ARGS__)
#define LWMSG_LOG_ERROR(context, ...) LWMSG_LOG(context, LWMSG_LOGLEVEL_ERROR, __VA_ARGS__)
#define LWMSG_LOG_WARNING(context, ...) LWMSG_LOG(context, LWMSG_LOGLEVEL_WARNING, __VA_ARGS__)
#define LWMSG_LOG_INFO(context, ...) LWMSG_LOG(context, LWMSG_LOGLEVEL_INFO, __VA_ARGS__)
#define LWMSG_LOG_VERBOSE(context, ...) LWMSG_LOG(context, LWMSG_LOGLEVEL_VERBOSE, __VA_ARGS__)
#define LWMSG_LOG_DEBUG(context, ...) LWMSG_LOG(context, LWMSG_LOGLEVEL_DEBUG, __VA_ARGS__)
#define LWMSG_LOG_TRACE(context, ...) LWMSG_LOG(context, LWMSG_LOGLEVEL_TRACE, __VA_ARGS__)

#define RAISE(context, status, ...) \
    (lwmsg_context_raise((context), (status), __func__, __FILE__, __LINE__, __VA_ARGS__))

#define RAISE_ERRNO(context) RAISE((context), lwmsg_status_map_errno(errno), NULL)

#endif
