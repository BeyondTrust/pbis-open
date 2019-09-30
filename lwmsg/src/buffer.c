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
