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
 *        buffer.h
 *
 * Abstract:
 *
 *        Marshalling buffer API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */
#ifndef __LWMSG_BUFFER_PRIVATE_H__
#define __LWMSG_BUFFER_PRIVATE_H__

#include "config.h"
#ifdef HAVE_STRING_H
#  include <string.h>
#endif
#include <stdarg.h>
#include <lwmsg/buffer.h>

#include "util-private.h"

static inline
LWMsgStatus
lwmsg_buffer_read(
    LWMsgBuffer* buffer,
    void* out,
    size_t count
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char* out_bytes = (unsigned char*) out;

    while (count)
    {
        if (buffer->cursor + count > buffer->end)
        {
            /* Not enough bytes remain in buffer, so copy what we have and ask for more */
            size_t remaining = buffer->end - buffer->cursor;
            memcpy(out_bytes, buffer->cursor, remaining);
            count -= remaining;
            buffer->cursor += remaining;
            out_bytes += remaining;

            if (buffer->wrap)
            {
                BAIL_ON_ERROR(status = buffer->wrap(buffer, count));
            }
            else
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_EOF);
            }
        }
        else
        {
            memcpy(out_bytes, buffer->cursor, count);
            buffer->cursor += count;
            break;
        }
    }

error:

    return status;
}

static inline
LWMsgStatus
lwmsg_buffer_write(
    LWMsgBuffer* buffer,
    const unsigned char* in_bytes,
    size_t count)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t remaining;
    size_t writable;

    while (count)
    {
        remaining = buffer->end - buffer->cursor;

        if (count > remaining)
            writable = remaining;
        else
            writable = count;

        memcpy(buffer->cursor, in_bytes, writable);

        in_bytes += writable;
        count -= writable;
        buffer->cursor += writable;

        if (count)
        {
            if (buffer->wrap)
            {
                BAIL_ON_ERROR(status = buffer->wrap(buffer, count));
            }
            else
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_EOF);
            }
        }
    }

error:

    return status;
}

static inline
LWMsgStatus
lwmsg_buffer_finish(
    LWMsgBuffer* buffer
    )
{
    if (buffer->wrap)
    {
        return buffer->wrap(buffer, 0);
    }
    else
    {
        return LWMSG_STATUS_SUCCESS;
    }
}

LWMsgStatus
lwmsg_buffer_realloc_wrap(
    LWMsgBuffer* buffer,
    size_t count
    );

#endif
