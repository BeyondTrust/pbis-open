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
 *        connection-buffer.c
 *
 * Abstract:
 *
 *        Connection API
 *        Packet buffer logic
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "connection-private.h"
#include "util-private.h"

LWMsgStatus
lwmsg_connection_buffer_construct(ConnectionBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    memset(buffer, 0, sizeof(*buffer));

    lwmsg_ring_init(&buffer->fragments);

    buffer->fd_capacity = MAX_FD_PAYLOAD;
    buffer->fd_length = 0;
    buffer->fd = malloc(sizeof(int) * buffer->fd_capacity);

    if (!buffer->fd)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

done:

    return status;

error:

    if (buffer->fd)
    {
        free(buffer->fd);
    }

    goto done;
}

void
lwmsg_connection_buffer_empty(ConnectionBuffer* buffer)
{
    LWMsgRing* ring, *next;
    size_t i = 0;

    if (buffer->fd)
    {
        for (i = 0; i < buffer->fd_length; i++)
        {
            close(buffer->fd[i]);
        }
        memset(buffer->fd, 0xFF, sizeof(int) * buffer->fd_capacity);
    }

    for (ring = buffer->fragments.next; ring != &buffer->fragments; ring = next)
    {
        next = ring->next;
        lwmsg_ring_remove(ring);
        free(LWMSG_OBJECT_FROM_MEMBER(ring, ConnectionFragment, ring));
    }

    if (buffer->current)
    {
        free(buffer->current);
        buffer->current = NULL;
    }
}


void
lwmsg_connection_buffer_destruct(ConnectionBuffer* buffer)
{
    lwmsg_connection_buffer_empty(buffer);

    if (buffer->fd)
    {
        free(buffer->fd);
    }

    memset(buffer, 0, sizeof(*buffer));
}

LWMsgStatus
lwmsg_connection_buffer_create_fragment(
    ConnectionBuffer* buffer,
    size_t length,
    ConnectionFragment** fragment
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionFragment* frag = NULL;

    frag = calloc(1, offsetof(ConnectionFragment, data) + length);

    if (!frag)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    lwmsg_ring_init(&frag->ring);
    
    frag->cursor = frag->data;

    *fragment = frag;

error:

    return status;
}

void
lwmsg_connection_buffer_queue_fragment(
    ConnectionBuffer* buffer,
    ConnectionFragment* fragment
    )
{
    lwmsg_ring_insert_before(&buffer->fragments, &fragment->ring);
}

ConnectionFragment*
lwmsg_connection_buffer_dequeue_fragment(
    ConnectionBuffer* buffer
    )
{
    LWMsgRing* ring = NULL;

    if (buffer->fragments.next == &buffer->fragments)
    {
        return NULL;
    }
    else
    {
        ring = buffer->fragments.next;
        lwmsg_ring_remove(ring);
        return LWMSG_OBJECT_FROM_MEMBER(ring, ConnectionFragment, ring);
    }
}

ConnectionFragment*
lwmsg_connection_buffer_get_first_fragment(
    ConnectionBuffer* buffer
    )
{
    if (buffer->fragments.next != &buffer->fragments)
    {
        return LWMSG_OBJECT_FROM_MEMBER(buffer->fragments.next, ConnectionFragment, ring);
    }
    else
    {
        return NULL;
    }
}

ConnectionFragment*
lwmsg_connection_buffer_get_last_fragment(
    ConnectionBuffer* buffer
    )
{
    if (buffer->fragments.prev != &buffer->fragments)
    {
        return LWMSG_OBJECT_FROM_MEMBER(buffer->fragments.prev, ConnectionFragment, ring);
    }
    else
    {
        return NULL;
    }
}

void
lwmsg_connection_buffer_free_fragment(
    ConnectionBuffer* buffer,
    ConnectionFragment* fragment
    )
{
    free(fragment);
}


LWMsgStatus
lwmsg_connection_buffer_ensure_fd_capacity(ConnectionBuffer* buffer, size_t needed)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (buffer->fd_capacity - buffer->fd_length < needed)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
    }

error:
    
    return status;
}
