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
