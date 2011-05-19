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
 *        buffer.c
 *
 * Abstract:
 *
 *        Buffer API
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "config.h"
#include "buffer-private.h"

LWMsgStatus
lwmsg_buffer_realloc_wrap(
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
lwmsg_buffer_print(
    LWMsgBuffer* buffer,
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

    BAIL_ON_ERROR(status = lwmsg_buffer_write(buffer, (unsigned char*) text, strlen(text)));

error:

    if (text)
    {
        free(text);
    }

    return status;
}
