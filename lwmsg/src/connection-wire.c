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
 *        connection-wire.c
 *
 * Abstract:
 *
 *        Connection API
 *        Wire protocol and network logic
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>
#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "connection-private.h"
#include "assoc-private.h"
#include "util-private.h"
#include "convert-private.h"
#include "xnet-private.h"
#include <lwmsg/data.h>

#ifndef CMSG_ALIGN
#    if defined(_CMSG_DATA_ALIGN)
#        define CMSG_ALIGN _CMSG_DATA_ALIGN
#    elif defined(_CMSG_ALIGN)
#        define CMSG_ALIGN _CMSG_ALIGN
#    elif defined(__DARWIN_ALIGN32)
#        define CMSG_ALIGN __DARWIN_ALIGN32
#    elif defined(ALIGN)
#        define CMSG_ALIGN ALIGN
#    endif
#endif

#ifndef CMSG_SPACE
#    define CMSG_SPACE(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + CMSG_ALIGN(len))
#endif

#ifndef CMSG_LEN
#    define CMSG_LEN(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#endif

static void
lwmsg_connection_packet_ntoh(ConnectionPacket* packet)
{
    packet->length = lwmsg_convert_uint32(packet->length, LWMSG_BIG_ENDIAN, LWMSG_NATIVE_ENDIAN);

    switch (packet->type)
    {
    case CONNECTION_PACKET_MESSAGE:
        packet->contents.msg.tag = lwmsg_convert_uint16(
            packet->contents.msg.tag,
            LWMSG_BIG_ENDIAN,
            LWMSG_NATIVE_ENDIAN);
        packet->contents.msg.cookie = lwmsg_convert_uint16(
            packet->contents.msg.cookie,
            LWMSG_BIG_ENDIAN,
            LWMSG_NATIVE_ENDIAN);
        packet->contents.msg.status = lwmsg_convert_uint32(
            packet->contents.msg.status,
            LWMSG_BIG_ENDIAN,
            LWMSG_NATIVE_ENDIAN);
        break;
    case CONNECTION_PACKET_CONNECT:
    case CONNECTION_PACKET_ACCEPT:
        packet->contents.greeting.packet_size = lwmsg_convert_uint32(
            packet->contents.greeting.packet_size,
            LWMSG_BIG_ENDIAN,
            LWMSG_NATIVE_ENDIAN);
        break;
    case CONNECTION_PACKET_SHUTDOWN:
        packet->contents.shutdown.status = lwmsg_convert_uint32(
            packet->contents.shutdown.status,
            LWMSG_BIG_ENDIAN,
            LWMSG_NATIVE_ENDIAN);
        break;
    }
}

static
uint32_t
lwmsg_connection_packet_length(
    ConnectionPacket* packet
    )
{
    return lwmsg_convert_uint32(packet->length, LWMSG_BIG_ENDIAN, LWMSG_NATIVE_ENDIAN);
}

static
LWMsgBool
lwmsg_connection_fragment_is_complete(
    ConnectionFragment* fragment
    )
{
    return !((fragment->cursor - fragment->data) < CONNECTION_PACKET_SIZE(ConnectionPacketBase) ||
             (fragment->cursor - fragment->data) < lwmsg_connection_packet_length((ConnectionPacket*)fragment->data));
}

static LWMsgStatus
lwmsg_connection_recvmsg(
    LWMsgAssoc* assoc,
    int fd,
    struct msghdr* msghdr,
    int flags,
    size_t* out_received
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    ssize_t received;

    LWMSG_ASSERT(fd >= 0);

    do
    {
        received = lwmsg_recvmsg_timeout(fd, msghdr, flags, priv->timeout.current);
    } while (received < 0 && errno == EINTR);

    if (received == 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_PEER_CLOSE);
    }
    else if (received < 0)
    {
        switch (errno)
        {
        case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
        case EWOULDBLOCK:
#endif
            if (priv->is_nonblock)
            {
                status = LWMSG_STATUS_PENDING;
            }
            else
            {
                status = LWMSG_STATUS_TIMEOUT;
            }
            break;
        case EPIPE:
        case ECONNRESET:
            status = LWMSG_STATUS_PEER_CLOSE;
            break;
        case EINVAL:
        case EBADF:
            /* bug */
            LWMSG_ASSERT_NOT_REACHED();
        default:
            status = lwmsg_status_map_errno(errno);
            LWMSG_LOG_ERROR(&assoc->context, "Unexpected system error from recvmsg(): %i", errno);
            break;
        }
        BAIL_ON_ERROR(status);
    }

    *out_received = (size_t) received;

error:

    return status;
}

static
LWMsgStatus
lwmsg_connection_recv_fragment(
    LWMsgAssoc* assoc,
    ConnectionFragment* fragment
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    ConnectionBuffer* buffer = &priv->recvbuffer;
    size_t length = 0;

    /* While we do not have an entire packet, repeat */
    while (!lwmsg_connection_fragment_is_complete(fragment))
    {
        struct msghdr msghdr = {0};
        struct iovec iovec = {0};
        union
        {
            struct cmsghdr cm;
            char buf[CMSG_SPACE(sizeof(int) * MAX_FD_PAYLOAD)];
        } buf_un;
        struct cmsghdr *cmsg = NULL;
        size_t received = 0;
        
        iovec.iov_base = fragment->cursor;
        
        if ((fragment->cursor - fragment->data) < CONNECTION_PACKET_SIZE(ConnectionPacketBase))
        {
            /* If we have not received a full packet header, ask for the remainder */
            iovec.iov_len = CONNECTION_PACKET_SIZE(ConnectionPacketBase) - (fragment->cursor - fragment->data);
        }
        else
        {
            /* Otherwise, we know the length of the packet, so ask for the rest of it */
            length = lwmsg_connection_packet_length((ConnectionPacket*)fragment->data);
            if (length > priv->packet_size)
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
            }

            iovec.iov_len = length - (fragment->cursor - fragment->data);
        }
        
        msghdr.msg_iov = &iovec;
        msghdr.msg_iovlen = 1;
        
        msghdr.msg_control = buf_un.buf;
        msghdr.msg_controllen = sizeof(buf_un.buf);
        
        BAIL_ON_ERROR(status = lwmsg_connection_recvmsg(assoc, priv->fd, &msghdr, 0, &received));
        
        fragment->cursor += received;
        
        if (msghdr.msg_controllen > 0 &&
            /* Work around bizarre behavior of X/Open recvmsg on HP-UX 11.11 */
            msghdr.msg_controllen <= sizeof(buf_un.buf))
        {
            for (cmsg = CMSG_FIRSTHDR(&msghdr); cmsg; cmsg = CMSG_NXTHDR(&msghdr, cmsg))
            {
                if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
                {
                    size_t numfds = (cmsg->cmsg_len - CMSG_ALIGN(sizeof(*cmsg))) / sizeof(int);
                    BAIL_ON_ERROR(status = lwmsg_connection_buffer_ensure_fd_capacity(buffer, numfds));
                    memcpy(buffer->fd + buffer->fd_length, CMSG_DATA(cmsg), numfds * sizeof(int));
                    buffer->fd_length += numfds;
                }
            }
        }
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_connection_sendmsg(
    LWMsgAssoc* assoc,
    int fd,
    struct msghdr* msghdr,
    int flags,
    size_t* out_sent
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    ssize_t sent;

    LWMSG_ASSERT(fd >= 0);

    do
    {
        sent = lwmsg_sendmsg_timeout(fd, msghdr, flags, priv->timeout.current);
    } while (sent < 0 && errno == EINTR);

    if (sent == 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_PEER_CLOSE);
    }
    else if (sent < 0)
    {
        switch (errno)
        {
        case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
        case EWOULDBLOCK:
#endif
            if (priv->is_nonblock)
            {
                status = LWMSG_STATUS_PENDING;
            }
            else
            {
                status = LWMSG_STATUS_TIMEOUT;
            }
            break;
        case EPIPE:
        case ECONNRESET:
            status = LWMSG_STATUS_PEER_CLOSE;
            break;
        case EINVAL:
        case EBADF:
            /* bug */
            LWMSG_ASSERT_NOT_REACHED();
        default:
            status = lwmsg_status_map_errno(errno);
            LWMSG_LOG_ERROR(&assoc->context, "Unexpected system error from sendmsg(): %i", errno);
            break;
        }
        BAIL_ON_ERROR(status);
    }

    *out_sent = (size_t) sent;

error:

    return status;
}

static
LWMsgStatus
lwmsg_connection_send_fragment(
    LWMsgAssoc* assoc,
    ConnectionFragment* fragment
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    ConnectionBuffer* buffer = &priv->sendbuffer;
    struct msghdr msghdr = {0};
    struct iovec iovec = {0};
    union
    {
        struct cmsghdr cm;
        char buf[CMSG_SPACE(sizeof(int) * MAX_FD_PAYLOAD)];
    } buf_un;
    struct cmsghdr *cmsg = NULL;
    size_t sent = 0;
    size_t fds_to_send = 0;
    size_t i;

    while (!lwmsg_connection_fragment_is_complete(fragment))
    {
        fds_to_send = buffer->fd_length;

        if (fds_to_send > MAX_FD_PAYLOAD)
        {
            fds_to_send = MAX_FD_PAYLOAD;
        }
        
        iovec.iov_base = fragment->cursor;
        iovec.iov_len = 
            lwmsg_connection_packet_length((ConnectionPacket*) fragment->data)
            - (fragment->cursor - fragment->data);

        msghdr.msg_iov = &iovec;
        msghdr.msg_iovlen = 1;
        
        if (fds_to_send)
        {
            msghdr.msg_control = buf_un.buf;
            msghdr.msg_controllen = CMSG_SPACE(fds_to_send * sizeof(int));
            memset(msghdr.msg_control, 0, msghdr.msg_controllen);
            
            cmsg = CMSG_FIRSTHDR(&msghdr);
            cmsg->cmsg_level = SOL_SOCKET;
            cmsg->cmsg_type = SCM_RIGHTS;
            cmsg->cmsg_len = CMSG_LEN(fds_to_send * sizeof(int));
            
            memcpy(CMSG_DATA(cmsg), buffer->fd, sizeof(int) * fds_to_send);
        }

        BAIL_ON_ERROR(status = lwmsg_connection_sendmsg(assoc, priv->fd, &msghdr, 0, &sent));
        
        fragment->cursor += sent;

        /* Close sent fds since they were copies */
        for (i = 0; i < fds_to_send; i++)
        {
            close(buffer->fd[i]);
        }
        
        /* Remove sent fds from queue */
        memmove(buffer->fd, buffer->fd + fds_to_send, (buffer->fd_length - fds_to_send) * sizeof(int));
        buffer->fd_length -= fds_to_send;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_connection_queue_fd(LWMsgAssoc* assoc, int fd)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    ConnectionBuffer* buffer = &priv->sendbuffer;

    BAIL_ON_ERROR(status = lwmsg_connection_buffer_ensure_fd_capacity(buffer, 1));
    buffer->fd[buffer->fd_length] = dup(fd);
    buffer->fd_length++;
    
error:

    return status;
}

LWMsgStatus
lwmsg_connection_dequeue_fd(LWMsgAssoc* assoc, int* out_fd)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    ConnectionBuffer* buffer = &priv->recvbuffer;

    if (buffer->fd_length == 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_PENDING);
    }

    *out_fd = buffer->fd[0];
    memmove(buffer->fd, buffer->fd + 1, (buffer->fd_length - 1) * sizeof(int));
    buffer->fd_length--;

error:

    return status;
}

static
LWMsgStatus
lwmsg_connection_check_full_fragment(
    LWMsgAssoc* assoc,
    ConnectionFragment* fragment
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPacket* packet = (ConnectionPacket*) fragment->data;

    switch (packet->type)
    {
    case CONNECTION_PACKET_SHUTDOWN:
        break;
    case CONNECTION_PACKET_MESSAGE:
        if (lwmsg_connection_packet_length(packet) < CONNECTION_PACKET_SIZE(ConnectionPacketMsg))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }
        break;
    case CONNECTION_PACKET_CONNECT:
    case CONNECTION_PACKET_ACCEPT:
        if (lwmsg_connection_packet_length(packet) != CONNECTION_PACKET_SIZE(ConnectionPacketGreeting))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }
        break;
    }

error:

    return status;
}

static unsigned char*
lwmsg_connection_packet_payload(ConnectionPacket* packet)
{
    switch (packet->type)
    {
    case CONNECTION_PACKET_MESSAGE:
        return (unsigned char*) packet + CONNECTION_PACKET_SIZE(ConnectionPacketMsg);
    }

    return NULL;
}

static
LWMsgStatus
lwmsg_connection_dequeue_fragment(
    LWMsgAssoc* assoc,
    ConnectionFragment** fragment
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    ConnectionFragment* my_fragment = NULL;
    ConnectionPacket* packet = NULL;

    if (lwmsg_ring_is_empty(&priv->recvbuffer.fragments))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_PEER_CLOSE);
    }

    my_fragment = lwmsg_connection_buffer_dequeue_fragment(&priv->recvbuffer);
    packet = (ConnectionPacket*) my_fragment->data;

    if (packet->type == CONNECTION_PACKET_SHUTDOWN)
    {
        status = packet->contents.shutdown.status;

        switch (status)
        {
        case LWMSG_STATUS_PEER_CLOSE:
        case LWMSG_STATUS_PEER_ABORT:
        case LWMSG_STATUS_PEER_RESET:
            BAIL_ON_ERROR(status);
            break;
        default:
            BAIL_ON_ERROR(status = LWMSG_STATUS_PEER_ABORT);
            break;
        }
    }

    *fragment = my_fragment;

done:

    return status;

error:

    *fragment = NULL;

    if (my_fragment)
    {
        free(my_fragment);
    }

    goto done;
}

static void
lwmsg_connection_load_fragment(
    LWMsgBuffer* buffer,
    ConnectionFragment* fragment,
    size_t space
    )
{
    ConnectionPacket* packet = (ConnectionPacket*) fragment->data;

    buffer->base = (unsigned char*) fragment;
    buffer->cursor = lwmsg_connection_packet_payload(packet);
    buffer->end = fragment->data + space;
}

static LWMsgStatus
lwmsg_connection_recv_wrap(LWMsgBuffer* buffer, size_t needed)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = (LWMsgAssoc*) buffer->data;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    ConnectionFragment* fragment = (ConnectionFragment*) buffer->base;
    ConnectionPacket* packet = NULL;

    buffer->base = NULL;

    /* Discard packet */
    lwmsg_connection_buffer_free_fragment(&priv->recvbuffer, fragment);
    fragment = NULL;

    if (needed)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_dequeue_fragment(assoc, &fragment));
        packet = (ConnectionPacket*) fragment->data;

        if (packet->type != CONNECTION_PACKET_MESSAGE)
        {
            ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_MALFORMED, "Did not receive message packet as expected");
        }

        lwmsg_connection_load_fragment(buffer, fragment, packet->length);
        fragment = NULL;
    }
    
error:

    if (fragment)
    {
        lwmsg_connection_buffer_free_fragment(&priv->recvbuffer, fragment);
    }

    return status;
}

static LWMsgStatus
lwmsg_connection_send_wrap(LWMsgBuffer* buffer, size_t needed)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = (LWMsgAssoc*) buffer->data;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    ConnectionFragment* fragment = (ConnectionFragment*) buffer->base;
    ConnectionPacket* packet = (ConnectionPacket*) fragment->data;

    buffer->base = NULL;
   
    /* Update size of packet based on amount of buffer that was used */
    packet->length = buffer->cursor - (unsigned char*) packet;

    /* If the marshaller is done, this is the last fragment */
    if (needed == 0)
    {
        packet->flags |= CONNECTION_PACKET_FLAG_LAST_FRAGMENT;
    }

    /* Now that the packet is complete, send it */
    lwmsg_connection_buffer_queue_fragment(
                      &priv->sendbuffer,
                      fragment);
    fragment = NULL;

    /* Flush the send buffer */
    status = lwmsg_connection_send_all_fragments(assoc);

    if (needed && status == LWMSG_STATUS_PENDING)
    {
        /* If we have more to marshal but we can't send,
           ignore it for now and continue to queue up packets */
        status = LWMSG_STATUS_SUCCESS;
    }

    BAIL_ON_ERROR(status);

    /* If we have more fragments to send, allocate a new packet */
    if (needed)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_buffer_create_fragment(
                          &priv->sendbuffer,
                          priv->packet_size,
                          &fragment));

        packet = (ConnectionPacket*) fragment->data;
        packet->type = CONNECTION_PACKET_MESSAGE;
        packet->flags = 0;
        packet->contents.msg.flags = (uint8_t) priv->outgoing->flags;
        packet->contents.msg.status = (uint32_t) priv->outgoing->status;
        packet->contents.msg.cookie = (uint16_t) priv->outgoing->cookie;
        packet->contents.msg.tag = (int16_t) priv->outgoing->tag;
        
        lwmsg_connection_load_fragment(buffer, fragment, priv->packet_size);
        fragment = NULL;
    }

error:

    if (fragment)
    {
        lwmsg_connection_buffer_free_fragment(&priv->sendbuffer, fragment);
    }

    return status;
}

LWMsgStatus
lwmsg_connection_begin_timeout(
    LWMsgAssoc* assoc,
    LWMsgTime* value
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);

    if (!priv->is_nonblock && value && lwmsg_time_is_positive(value))
    {
        priv->timeout.current = value;
    }
    else
    {
        priv->timeout.current = NULL;
    }

    return status;
}

LWMsgStatus
lwmsg_connection_send_all_fragments(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    
    while (priv->sendbuffer.current ||
           !lwmsg_ring_is_empty(&priv->sendbuffer.fragments))
    {
        if (!priv->sendbuffer.current)
        {
            priv->sendbuffer.current = lwmsg_connection_buffer_dequeue_fragment(&priv->sendbuffer);
            lwmsg_connection_packet_ntoh((ConnectionPacket*) priv->sendbuffer.current->data);
        }

        BAIL_ON_ERROR(status = lwmsg_connection_send_fragment(assoc, priv->sendbuffer.current));
        free(priv->sendbuffer.current);
        priv->sendbuffer.current = NULL;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_connection_recv_next_fragment(
    LWMsgAssoc* assoc,
    ConnectionFragment** fragment
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    
    if (!priv->recvbuffer.current)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_buffer_create_fragment(
                          &priv->recvbuffer,
                          priv->packet_size,
                          &priv->recvbuffer.current));
    }

    BAIL_ON_ERROR(status = lwmsg_connection_recv_fragment(assoc, priv->recvbuffer.current));
    BAIL_ON_ERROR(status = lwmsg_connection_check_full_fragment(assoc, priv->recvbuffer.current));
    lwmsg_connection_packet_ntoh((ConnectionPacket*) priv->recvbuffer.current->data);
    lwmsg_connection_buffer_queue_fragment(&priv->recvbuffer, priv->recvbuffer.current);
    *fragment = priv->recvbuffer.current;
    priv->recvbuffer.current = NULL;

done:

    return status;

error:

    *fragment = NULL;

    goto done;
}

static
LWMsgStatus
lwmsg_connection_begin_connect_local(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    struct sockaddr_un sockaddr;
    long opts = 0;

    priv->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    
    if (priv->fd == -1)
    {
        BAIL_ON_ERROR(status = RAISE_ERRNO(&assoc->context));
    }

    BAIL_ON_ERROR(status = lwmsg_set_close_on_exec(priv->fd));
    BAIL_ON_ERROR(status = lwmsg_set_block_sigpipe(priv->fd));

    /* Get socket flags */
    if ((opts = fcntl(priv->fd, F_GETFL, 0)) < 0)
    {
        BAIL_ON_ERROR(status = RAISE_ERRNO(&assoc->context));
    }

    if (priv->is_nonblock)
    {
        /* Set non-blocking flag */
        opts |= O_NONBLOCK;
    }
    
    /* Set socket flags */
    if (fcntl(priv->fd, F_SETFL, opts) < 0)
    {
        BAIL_ON_ERROR(status = RAISE_ERRNO(&assoc->context));
    }

    sockaddr.sun_family = AF_UNIX;

    if (strlen(priv->endpoint) + 1 > sizeof(sockaddr.sun_path))
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER, "Endpoint is too long for underlying protocol");
    }

    strcpy(sockaddr.sun_path, priv->endpoint);

    BAIL_ON_ERROR(status = lwmsg_connection_begin_timeout(
                      assoc,
                      &priv->timeout.establish));

    if (connect(priv->fd, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) < 0)
    {
        switch (errno)
        {
        case 0:
            status = LWMSG_STATUS_SUCCESS;
            break;
        case EINPROGRESS:
            status = LWMSG_STATUS_PENDING;
            break;
        case ENOENT:
            status = LWMSG_STATUS_FILE_NOT_FOUND;
            break;
        case ECONNREFUSED:
            status = LWMSG_STATUS_CONNECTION_REFUSED;
            break;
        default:
            status = lwmsg_status_map_errno(errno);
            break;
        }
        BAIL_ON_ERROR(status);
    }

done:
    
    return status;

error:

    if (priv->fd >= 0)
    {
        close(priv->fd);
        priv->fd = -1;
    }

    goto done;
}

LWMsgStatus
lwmsg_connection_begin_connect_socket(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
 
    switch (priv->mode)
    {
    case LWMSG_CONNECTION_MODE_LOCAL:
        BAIL_ON_ERROR(status = lwmsg_connection_begin_connect_local(assoc));
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_INTERNAL);
        break;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_connection_finish_connect_socket(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    long err = 0;
    socklen_t len = 0;
    fd_set readfds;
    fd_set writefds;
    struct timeval timeout = {0, 0};
    int nfds = 0;

    if (!priv->is_nonblock)
    {
        do
        {
            if (priv->timeout.current)
            {
                timeout.tv_sec = priv->timeout.current->seconds;
                timeout.tv_usec = priv->timeout.current->microseconds;
            }

            FD_ZERO(&writefds);
            FD_ZERO(&readfds);
            FD_SET(priv->fd, &writefds);
            
            nfds = priv->fd + 1;
            
            err = select(nfds, &readfds, &writefds, NULL, priv->timeout.current ? &timeout : NULL);
        } while (err == 0);
        
        if (err < 0)
        {
            /* Select failed for some reason */
            BAIL_ON_ERROR(status = RAISE_ERRNO(&assoc->context));
        }
    }
    
    /* If we make it here, our connect operation should be done */           
    len = sizeof(err);
    /* Use getsockopt to extract the result of the connect call */
    if (getsockopt(priv->fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
    {
        /* Getsockopt failed for some reason */
        BAIL_ON_ERROR(status = RAISE_ERRNO(&assoc->context));
    }

    switch (err)
    {
    case 0:
        status = LWMSG_STATUS_SUCCESS;
        break;
    case EINPROGRESS:
        status = LWMSG_STATUS_PENDING;
        break;
    case ENOENT:
        status = LWMSG_STATUS_FILE_NOT_FOUND;
        break;
    case ECONNREFUSED:
        status = LWMSG_STATUS_CONNECTION_REFUSED;
        break;
    default:
        status = lwmsg_status_map_errno(err);
        break;
    }

    BAIL_ON_ERROR(status);

done:

    return status;

error:

    if (status != LWMSG_STATUS_PENDING)
    {
        if (priv->fd != -1)
        {
            close(priv->fd);
            priv->fd = -1;
        }
    }

    goto done;
}

LWMsgStatus
lwmsg_connection_connect_existing(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    long opts = 0;

    BAIL_ON_ERROR(status = lwmsg_connection_begin_timeout(
                      assoc,
                      &priv->timeout.establish));
    
    /* Get socket flags */
    if ((opts = fcntl(priv->fd, F_GETFL, 0)) < 0)
    {
        BAIL_ON_ERROR(status = RAISE_ERRNO(&assoc->context));
    }

    /* Set non-blocking flag */
    if (priv->is_nonblock)
    {
        opts |= O_NONBLOCK;
    }
    
    /* Set socket flags */
    if (fcntl(priv->fd, F_SETFL, opts) < 0)
    {
        BAIL_ON_ERROR(status = RAISE_ERRNO(&assoc->context));
    }

error:

    return status;
}


LWMsgStatus
lwmsg_connection_begin_send_connect(
    LWMsgAssoc* assoc,
    LWMsgSession* session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    ConnectionFragment* fragment = NULL;
    ConnectionPacket* packet = NULL;
    int fds[2] = { -1, -1 };
    const LWMsgSessionID* id = lwmsg_session_get_id(session);

    BAIL_ON_ERROR(status = lwmsg_connection_buffer_create_fragment(
                      &priv->sendbuffer,
                      priv->packet_size,
                      &fragment));

    packet = (ConnectionPacket*) fragment->data;

    packet->type = CONNECTION_PACKET_CONNECT;
    packet->length = CONNECTION_PACKET_SIZE(ConnectionPacketGreeting);
    packet->flags = CONNECTION_PACKET_FLAG_SINGLE;
    packet->contents.greeting.packet_size = (uint32_t) priv->packet_size;
    packet->contents.greeting.flags = 0;
    
    memcpy(packet->contents.greeting.cookie, id->connect_id.bytes, sizeof(LWMsgSessionCookie));

#ifndef HAVE_PEERID_METHOD
    /* If this system does not have a simple method for getting the identity
     * of a socket peer, improvise
     */
    if (priv->mode == LWMSG_CONNECTION_MODE_LOCAL)
    {
        /* If we are connecting to a local endpoint */
        if (priv->endpoint)
        {
            uid_t uid;
            gid_t gid;

            BAIL_ON_ERROR(status = lwmsg_connection_get_endpoint_owner(
                              assoc,
                              priv->endpoint,
                              &uid,
                              &gid));

            /* Only send a token to root or ourselves since it could
               be used by the peer to impersonate us */
            if (uid == 0 || uid == getuid())
            {
                /* Send an auth fd */
                if (pipe(fds) != 0)
                {
                    BAIL_ON_ERROR(status = RAISE_ERRNO(&assoc->context));
                }
                
                BAIL_ON_ERROR(status = lwmsg_connection_queue_fd(assoc, fds[0]));
                
                packet->contents.greeting.flags |= CONNECTION_GREETING_AUTH_LOCAL;
            }
        }
    }
#endif

    lwmsg_connection_buffer_queue_fragment(&priv->sendbuffer, fragment);
    fragment = NULL;

error:

    if (fds[0] != -1)
    {
        close(fds[0]);
    }

    if (fds[1] != -1)
    {
        close(fds[1]);
    }

    if (fragment)
    {
        lwmsg_connection_buffer_free_fragment(&priv->sendbuffer, fragment);
    }

    return status;
}

LWMsgStatus
lwmsg_connection_finish_send_connect(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_connection_send_all_fragments(assoc));

error:

    return status;
}

LWMsgStatus
lwmsg_connection_begin_recv_connect(
    LWMsgAssoc* assoc
    )
{
    return LWMSG_STATUS_SUCCESS;
}

LWMsgStatus
lwmsg_connection_finish_recv_connect(
    LWMsgAssoc* assoc,
    LWMsgSession* session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    ConnectionFragment* fragment = NULL;
    ConnectionPacket* packet = NULL;
    int fd = -1;
    LWMsgSecurityToken* token = NULL;
    
    BAIL_ON_ERROR(status = lwmsg_connection_recv_next_fragment(assoc, &fragment));
    BAIL_ON_ERROR(status = lwmsg_connection_dequeue_fragment(assoc, &fragment));

    packet = (ConnectionPacket*) fragment->data;

    if (packet->type != CONNECTION_PACKET_CONNECT)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_MALFORMED, "Received malformed connect packet");
    }

    if (packet->contents.greeting.packet_size < sizeof(ConnectionPacket) + 1)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_MALFORMED, "Invalid packet size request from peer");
    }

    if (packet->contents.greeting.packet_size < (uint32_t) priv->packet_size)
    {
        priv->packet_size = packet->contents.greeting.packet_size;
    }

    if (priv->mode == LWMSG_CONNECTION_MODE_LOCAL)
    {
#ifdef HAVE_PEERID_METHOD
        /* If we have a simple way of getting the peer id, just use it */
        BAIL_ON_ERROR(status = lwmsg_local_token_from_socket_peer(priv->fd, &token));
#else
        /* Otherwise, use the explicit auth method */
        if (packet->contents.greeting.flags & CONNECTION_GREETING_AUTH_LOCAL)
        {
            struct stat statbuf;

            BAIL_ON_ERROR(status = lwmsg_connection_dequeue_fd(assoc, &fd));
            
            if (fstat(fd, &statbuf))
            {
                BAIL_ON_ERROR(status = RAISE_ERRNO(&assoc->context));
            }
            
            if (!S_ISFIFO(statbuf.st_mode) ||
                (statbuf.st_mode & (S_IRWXO | S_IRWXG)) != 0)
            {
                ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_SECURITY, "Received invalid security token");
            }
            
            BAIL_ON_ERROR(status = lwmsg_local_token_new(statbuf.st_uid, statbuf.st_gid, (pid_t)-1, &token));
        }
        else if (priv->endpoint)
        {
            uid_t uid;
            gid_t gid;

            /* Attempt to stat endpoint for owner information */
            BAIL_ON_ERROR(status = lwmsg_connection_get_endpoint_owner(
                              assoc,
                              priv->endpoint,
                              &uid,
                              &gid));
         
            BAIL_ON_ERROR(status = lwmsg_local_token_new(uid, gid, (pid_t)-1, &token));
        }
#endif
    }
    else if (priv->mode == LWMSG_CONNECTION_MODE_PAIR)
    {
        BAIL_ON_ERROR(status = lwmsg_local_token_new(getuid(), getgid(), getpid(), &token));
    }

    /* Accept client into a session */
    BAIL_ON_ERROR(status = session->sclass->accept_peer(
                      session,
                      (const LWMsgSessionCookie*) packet->contents.greeting.cookie,
                      token));
    token = NULL;

    /* Reconstruct buffers in case our packet size changed */
    lwmsg_connection_buffer_destruct(&priv->recvbuffer);
    BAIL_ON_ERROR(status = lwmsg_connection_buffer_construct(&priv->recvbuffer));
    lwmsg_connection_buffer_destruct(&priv->sendbuffer);
    BAIL_ON_ERROR(status = lwmsg_connection_buffer_construct(&priv->sendbuffer));

    /* Set up marshal context for message exchange (this is where byte order
       and other negotiated format settings should be set) */
    if (priv->marshal_context)
    {
        lwmsg_data_context_delete(priv->marshal_context);
        priv->marshal_context = NULL;
    }

    BAIL_ON_ERROR(status = lwmsg_data_context_new(&assoc->context, &priv->marshal_context));

error:

    if (fragment)
    {
        lwmsg_connection_buffer_free_fragment(&priv->recvbuffer, fragment);
    }

    if (fd != -1)
    {
        close(fd);
    }

    if (token)
    {
        lwmsg_security_token_delete(token);
    }

    return status;
}

LWMsgStatus
lwmsg_connection_begin_send_accept(
    LWMsgAssoc* assoc,
    LWMsgSession* session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    ConnectionFragment* fragment = NULL;
    ConnectionPacket* packet = NULL;
    int fds[2] = { -1, -1 };
    const LWMsgSessionID* id = lwmsg_session_get_id(session);

    BAIL_ON_ERROR(status = lwmsg_connection_buffer_create_fragment(
                      &priv->sendbuffer,
                      priv->packet_size,
                      &fragment));

    packet = (ConnectionPacket*) fragment->data;

    packet->type = CONNECTION_PACKET_ACCEPT;
    packet->length = CONNECTION_PACKET_SIZE(ConnectionPacketGreeting);
    packet->flags = CONNECTION_PACKET_FLAG_SINGLE;
    packet->contents.greeting.packet_size = (uint32_t) priv->packet_size;
    packet->contents.greeting.flags = 0;
    
    memcpy(packet->contents.greeting.cookie, id->accept_id.bytes, sizeof(LWMsgSessionCookie));

#ifndef HAVE_PEERID_METHOD
    /* If this system does not have a simple method for getting the identity
     * of a socket peer, improvise
     */
    if (priv->mode == LWMSG_CONNECTION_MODE_LOCAL)
    {
        /* If we are connecting to a local endpoint */
        if (priv->endpoint)
        {
            uid_t uid;
            gid_t gid;

            BAIL_ON_ERROR(status = lwmsg_connection_get_endpoint_owner(
                              assoc,
                              priv->endpoint,
                              &uid,
                              &gid));

            /* Only send a token to root or ourselves since it could
               be used by the peer to impersonate us */
            if (uid == 0 || uid == getuid())
            {
                /* Send an auth fd */
                if (pipe(fds) != 0)
                {
                    BAIL_ON_ERROR(status = RAISE_ERRNO(&assoc->context));
                }
                
                BAIL_ON_ERROR(status = lwmsg_connection_queue_fd(assoc, fds[0]));
                
                packet->contents.greeting.flags |= CONNECTION_GREETING_AUTH_LOCAL;
            }
        }
    }
#endif

    lwmsg_connection_buffer_queue_fragment(&priv->sendbuffer, fragment);
    fragment = NULL;

error:

    if (fds[0] != -1)
    {
        close(fds[0]);
    }

    if (fds[1] != -1)
    {
        close(fds[1]);
    }

    if (fragment)
    {
        lwmsg_connection_buffer_free_fragment(&priv->sendbuffer, fragment);
    }

    return status;
}

LWMsgStatus
lwmsg_connection_finish_send_accept(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_connection_send_all_fragments(assoc));

error:

    return status;
}

LWMsgStatus
lwmsg_connection_begin_recv_accept(
    LWMsgAssoc* assoc
    )
{
    return LWMSG_STATUS_SUCCESS;
}

LWMsgStatus
lwmsg_connection_finish_recv_accept(
    LWMsgAssoc* assoc,
    LWMsgSession* session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    ConnectionFragment* fragment = NULL;
    ConnectionPacket* packet = NULL;
    int fd = -1;
    LWMsgSecurityToken* token = NULL;
    
    BAIL_ON_ERROR(status = lwmsg_connection_recv_next_fragment(assoc, &fragment));
    BAIL_ON_ERROR(status = lwmsg_connection_dequeue_fragment(assoc, &fragment));

    packet = (ConnectionPacket*) fragment->data;

    if (packet->type != CONNECTION_PACKET_ACCEPT)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_MALFORMED, "Received malformed accept packet");
    }

    if (packet->contents.greeting.packet_size < sizeof(ConnectionPacket) + 1)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_MALFORMED, "Invalid packet size request from peer");
    }

    if (packet->contents.greeting.packet_size < (uint32_t) priv->packet_size)
    {
        priv->packet_size = packet->contents.greeting.packet_size;
    }

    if (priv->mode == LWMSG_CONNECTION_MODE_LOCAL)
    {
#ifdef HAVE_PEERID_METHOD
        /* If we have a simple way of getting the peer id, just use it */
        BAIL_ON_ERROR(status = lwmsg_local_token_from_socket_peer(priv->fd, &token));
#else
        /* Otherwise, use the explicit auth method */
        if (packet->contents.greeting.flags & CONNECTION_GREETING_AUTH_LOCAL)
        {
            struct stat statbuf;

            BAIL_ON_ERROR(status = lwmsg_connection_dequeue_fd(assoc, &fd));
            
            if (fstat(fd, &statbuf))
            {
                BAIL_ON_ERROR(status = RAISE_ERRNO(&assoc->context));
            }
            
            if (!S_ISFIFO(statbuf.st_mode) ||
                (statbuf.st_mode & (S_IRWXO | S_IRWXG)) != 0)
            {
                ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_SECURITY, "Received invalid security token");
            }
            
            BAIL_ON_ERROR(status = lwmsg_local_token_new(statbuf.st_uid, statbuf.st_gid, (pid_t)-1, &token));
        }
        else if (priv->endpoint)
        {
            uid_t uid;
            gid_t gid;

            /* Attempt to stat endpoint for owner information */
            BAIL_ON_ERROR(status = lwmsg_connection_get_endpoint_owner(
                              assoc,
                              priv->endpoint,
                              &uid,
                              &gid));
         
            BAIL_ON_ERROR(status = lwmsg_local_token_new(uid, gid, (pid_t)-1, &token));
        }
#endif
    }
    else if (priv->mode == LWMSG_CONNECTION_MODE_PAIR)
    {
        BAIL_ON_ERROR(status = lwmsg_local_token_new(getuid(), getgid(), getpid(), &token));
    }

    /* Connect the session */
    BAIL_ON_ERROR(status = session->sclass->connect_peer(
                      session,
                      (const LWMsgSessionCookie*) packet->contents.greeting.cookie,
                      token));
    token = NULL;    

    /* Reconstruct buffers in case our packet size changed */
    lwmsg_connection_buffer_destruct(&priv->recvbuffer);
    BAIL_ON_ERROR(status = lwmsg_connection_buffer_construct(&priv->recvbuffer));
    lwmsg_connection_buffer_destruct(&priv->sendbuffer);
    BAIL_ON_ERROR(status = lwmsg_connection_buffer_construct(&priv->sendbuffer));

    /* Set up marshal context for message exchange (this is where byte order
       and other negotiated format settings should be set) */
    if (priv->marshal_context)
    {
        lwmsg_data_context_delete(priv->marshal_context);
        priv->marshal_context = NULL;
    }

    BAIL_ON_ERROR(status = lwmsg_data_context_new(&assoc->context, &priv->marshal_context));

error:

    if (fragment)
    {
        lwmsg_connection_buffer_free_fragment(&priv->recvbuffer, fragment);
    }

    if (fd != -1)
    {
        close(fd);
    }
    
    if (token)
    {
        lwmsg_security_token_delete(token);
    }

    return status;
}

LWMsgStatus
lwmsg_connection_begin_send_message(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    LWMsgProtocol* prot = lwmsg_assoc_get_protocol(assoc);
    LWMsgTypeSpec* type = NULL;
    ConnectionFragment* fragment = NULL;
    ConnectionPacket* packet = NULL;
    LWMsgMessage* message = priv->outgoing;
    LWMsgBuffer buffer;
    
    memset(&buffer, 0, sizeof(buffer));

    if (message->tag != LWMSG_TAG_INVALID)
    {
        status = lwmsg_protocol_get_message_type(prot, message->tag, &type);
        
        switch (status)
        {
        case LWMSG_STATUS_NOT_FOUND:
            status = LWMSG_STATUS_MALFORMED;
            break;
        default:
            break;
        }
        BAIL_ON_ERROR(status);
    }

    BAIL_ON_ERROR(status = lwmsg_connection_buffer_create_fragment(
                      &priv->sendbuffer,
                      priv->packet_size,
                      &fragment));

    packet = (ConnectionPacket*) fragment->data;

    packet->type = CONNECTION_PACKET_MESSAGE;
    packet->flags = CONNECTION_PACKET_FLAG_FIRST_FRAGMENT;
    packet->contents.msg.flags = message->flags;
    packet->contents.msg.status = message->status;
    packet->contents.msg.cookie = message->cookie;
    packet->contents.msg.tag = message->tag;

    /* Set up timeout */
    BAIL_ON_ERROR(status = lwmsg_connection_begin_timeout(
                      assoc,
                      &priv->timeout.message));

    /* If the message has no payload, send a zero-length message */
    if (type == NULL)
    {
        packet->length = CONNECTION_PACKET_SIZE(ConnectionPacketMsg);
        packet->flags |= CONNECTION_PACKET_FLAG_LAST_FRAGMENT;

        lwmsg_connection_buffer_queue_fragment(&priv->sendbuffer, fragment);
        fragment = NULL;
    }
    else
    {
        lwmsg_connection_load_fragment(&buffer, fragment, priv->packet_size);
        fragment = NULL;
        
        buffer.wrap = lwmsg_connection_send_wrap;
        buffer.data = assoc;
        
        BAIL_ON_ERROR(status = lwmsg_data_marshal(
                          priv->marshal_context,
                          type,
                          message->data,
                          &buffer));
    }
    
error:

    if (fragment)
    {
        lwmsg_connection_buffer_free_fragment(&priv->sendbuffer, fragment);
    }

    if (buffer.base)
    {
        lwmsg_connection_buffer_free_fragment(&priv->sendbuffer, (ConnectionFragment*) buffer.base);
    }

    /* On error, empty out remaining fragments in send buffer */
    if (status && status != LWMSG_STATUS_PENDING)
    {
        lwmsg_connection_buffer_empty(&priv->sendbuffer);
    }

    return status;
}

LWMsgStatus
lwmsg_connection_finish_send_message(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);

    BAIL_ON_ERROR(status = lwmsg_connection_send_all_fragments(assoc));

error:

    /* On error, empty out remaining fragments in send buffer */
    if (status && status != LWMSG_STATUS_PENDING)
    {
        lwmsg_connection_buffer_empty(&priv->sendbuffer);
    }

    return status;
}

LWMsgStatus
lwmsg_connection_begin_recv_message(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);

    /* Set up timeout */
    BAIL_ON_ERROR(status = lwmsg_connection_begin_timeout(
                      assoc,
                      &priv->timeout.message));

error:

    return status;
}

LWMsgStatus
lwmsg_connection_finish_recv_message(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);
    LWMsgProtocol* prot = lwmsg_assoc_get_protocol(assoc);
    LWMsgContext* context = &assoc->context;
    LWMsgTypeSpec* type = NULL;
    ConnectionFragment* fragment = NULL;
    ConnectionPacket* packet = NULL;
    LWMsgBuffer buffer;

    memset(&buffer, 0, sizeof(buffer));

    do
    {
        BAIL_ON_ERROR(status = lwmsg_connection_recv_next_fragment(assoc, &fragment));
        packet = (ConnectionPacket*) fragment->data;
    } while (!(packet->flags & CONNECTION_PACKET_FLAG_LAST_FRAGMENT));
    
    /* The recv buffer should now contain all fragments in the message */
    BAIL_ON_ERROR(status = lwmsg_connection_dequeue_fragment(assoc, &fragment));
    packet = (ConnectionPacket*) fragment->data;

    if (packet->type != CONNECTION_PACKET_MESSAGE)
    {
        BAIL_ON_ERROR(status = RAISE(
            context,
            LWMSG_STATUS_MALFORMED,
            "Did not receive message packet as expected"));
    }

    if (packet->contents.msg.tag != LWMSG_TAG_INVALID)
    {
        status = lwmsg_protocol_get_message_type(prot, packet->contents.msg.tag, &type);
        
        switch (status)
        {
        case LWMSG_STATUS_NOT_FOUND:
            status = LWMSG_STATUS_MALFORMED;
            break;
        default:
            break;
        }
        BAIL_ON_ERROR(status);
    }

    priv->incoming->flags = packet->contents.msg.flags;
    priv->incoming->status = packet->contents.msg.status;
    priv->incoming->cookie = packet->contents.msg.cookie;
    priv->incoming->tag = packet->contents.msg.tag;

    if (type == NULL)
    {
        /* If message has no payload, just set the payload to NULL */
        priv->incoming->data = NULL;
    }
    else
    {
        lwmsg_connection_load_fragment(&buffer, fragment, packet->length);
        fragment = NULL;
        
        buffer.wrap = lwmsg_connection_recv_wrap;
        buffer.data = assoc;
        
        status = lwmsg_data_unmarshal(
            priv->marshal_context,
            type,
            &buffer,
            &priv->incoming->data);

        /* Deal with malformed/invalid messages by returning a synthetic
           message rather than bailing out entirely */
        switch (status)
        {
        case LWMSG_STATUS_MALFORMED:
        case LWMSG_STATUS_INVALID_HANDLE:
        case LWMSG_STATUS_OVERFLOW:
        case LWMSG_STATUS_UNDERFLOW:
            priv->incoming->flags |= LWMSG_MESSAGE_FLAG_SYNTHETIC;
            priv->incoming->status = status;
            priv->incoming->tag = LWMSG_TAG_INVALID;
            priv->incoming->data = NULL;
            status = LWMSG_STATUS_SUCCESS;
            break;
        default:
            BAIL_ON_ERROR(status);
        }
    }

error:

    if (fragment)
    {
        lwmsg_connection_buffer_free_fragment(&priv->recvbuffer, fragment);
    }

    if (buffer.base)
    {
        lwmsg_connection_buffer_free_fragment(&priv->recvbuffer, (ConnectionFragment*) buffer.base);
    }

    return status;
}

LWMsgStatus
lwmsg_connection_begin_send_shutdown(
    LWMsgAssoc* assoc,
    LWMsgStatus reason
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPacket* packet = NULL;
    ConnectionFragment* fragment = NULL;
    ConnectionPrivate* priv = CONNECTION_PRIVATE(assoc);

    /* Set up timeout */
    BAIL_ON_ERROR(status = lwmsg_connection_begin_timeout(
                      assoc,
                      &priv->timeout.establish));

    BAIL_ON_ERROR(status = lwmsg_connection_buffer_create_fragment(
                      &priv->sendbuffer,
                      priv->packet_size,
                      &fragment));
    
    packet = (ConnectionPacket*) fragment->data;
    
    packet->type = CONNECTION_PACKET_SHUTDOWN;
    packet->length = CONNECTION_PACKET_SIZE(ConnectionPacketShutdown);
    packet->flags = CONNECTION_PACKET_FLAG_SINGLE;
    packet->contents.shutdown.status = reason;
    
    lwmsg_connection_buffer_queue_fragment(&priv->sendbuffer, fragment);

done:

    return status;
    
error:

    if (fragment)
    {
        free(fragment);
    }

    goto done;
}

LWMsgStatus
lwmsg_connection_finish_send_shutdown(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    /* Flush the send buffer */
    BAIL_ON_ERROR(status = lwmsg_connection_send_all_fragments(assoc));

error:

    return status;
}
