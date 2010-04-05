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
#include "util-private.h"

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
    lwmsg_error_clear(&context->error);

    free(context);
}

const char*
lwmsg_data_context_get_error_message(
    LWMsgDataContext* context,
    LWMsgStatus status
    )
{
    return lwmsg_error_message(status, &context->error);
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
lwmsg_data_context_raise_error(
    LWMsgDataContext* context,
    LWMsgStatus status,
    const char* format,
    ...
    )
{
    va_list ap;

    va_start(ap, format);

    status = lwmsg_error_raise_v(&context->error, status, format, ap);

    va_end(ap);

    return status;
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
