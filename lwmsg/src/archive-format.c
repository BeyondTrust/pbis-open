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
 *        archive-format.c
 *
 * Abstract:
 *
 *        Archive API
 *        Data file format functions
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "config.h"
#include "archive-private.h"
#include "util-private.h"
#include "protocol-private.h"
#include "data-private.h"
#include "convert-private.h"

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

static
LWMsgStatus
lwmsg_archive_write_fd(
    LWMsgArchive* archive,
    const void* data,
    size_t size
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    const char* cursor = (const char*) data;
    size_t remaining = size;
    ssize_t written = 0;

    while (remaining)
    {
        do
        {
            written = write(archive->fd, cursor, remaining);
        } while (written < 0 && (errno == EINTR || errno == EAGAIN));
        
        if (written < 0)
        {
            BAIL_ON_ERROR(status = RAISE_ERRNO(&archive->base.context));
        }

        remaining -= written;
        cursor += written;
        archive->offset += written;
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_archive_read_fd(
    LWMsgArchive* archive,
    void* buffer,
    size_t size,
    size_t* bytes_read
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    char* cursor = (char*) buffer;
    size_t remaining = size;
    ssize_t count = 0;
    size_t total = 0;

    while (remaining)
    {
        do
        {
            count = read(archive->fd, cursor, remaining);
        } while (count < 0 && (errno == EINTR || errno == EAGAIN));
        
        if (count < 0)
        {
            BAIL_ON_ERROR(status = RAISE_ERRNO(&archive->base.context));
        }
        else if (count == 0)
        {
            if (remaining == size)
            {
                /* We didn't read anything */
                BAIL_ON_ERROR(status = LWMSG_STATUS_EOF);
            }
            else
            {
                /* Short but successful read */
                break;
            }
        }
        
        remaining -= count;
        cursor += count;
        archive->offset += count;
        total += count;
    }

    *bytes_read = total;

error:
    
    return status;
}

static
LWMsgStatus
lwmsg_archive_seek_fd(
    LWMsgArchive* archive,
    off_t position
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    off_t new_position = 0;

    if ((new_position = lseek(archive->fd, position, SEEK_SET)) == (off_t) -1)
    {
        BAIL_ON_ERROR(status = RAISE_ERRNO(&archive->base.context));
    }

    archive->offset = new_position;

error:

    return status;
}

static
void
lwmsg_archive_populate_header(
    LWMsgArchive* archive,
    ArchiveHeader* header
    )
{
    memcpy(header->magic, "LWMA", sizeof(header->magic));
    header->version_flags = 0;

    if (archive->byte_order == LWMSG_BIG_ENDIAN)
    {
        header->version_flags |= ARCHIVE_VERSION_FLAG_BIG_ENDIAN;
    }

    header->version_major = ARCHIVE_VERSION_MAJOR;
    header->version_minor = ARCHIVE_VERSION_MINOR;
    header->version_micro = ARCHIVE_VERSION_MICRO;
    header->format_flags = 0;

    if (archive->disp & LWMSG_ARCHIVE_SCHEMA)
    {
        header->format_flags |= ARCHIVE_FORMAT_FLAG_SCHEMA;
    }

    header->format_flags = LWMSG_SWAP32(
        header->format_flags,
        LWMSG_NATIVE_ENDIAN,
        archive->byte_order);

    memset(header->protocol_id, 0, sizeof(header->protocol_id));
}

static
LWMsgStatus
lwmsg_archive_write_message_wrap_fd(
    LWMsgBuffer* buffer,
    size_t needed
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgArchive* archive = buffer->data;

    /* Flush data in the buffer to file */
    BAIL_ON_ERROR(status = lwmsg_archive_write_fd(archive, buffer->base, (size_t) (buffer->cursor - buffer->base)));
    buffer->cursor = buffer->base;

error:

    return status;
}

static
LWMsgStatus
lwmsg_archive_write_schema_fd(
    LWMsgArchive* archive
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpec* type = lwmsg_protocol_rep_spec;
    LWMsgProtocolRep* rep = NULL;
    off_t length_offset = archive->offset;
    off_t end_offset = 0;
    LWMsgBuffer buffer = {0};
    unsigned char data[2048];
    uint32_t length = 0;
    
    BAIL_ON_ERROR(status = lwmsg_protocol_get_protocol_rep(archive->base.prot, &rep));

    buffer.base = data;
    buffer.end = data + sizeof(data);
    buffer.cursor = buffer.base;
    buffer.wrap = lwmsg_archive_write_message_wrap_fd;
    buffer.data = archive;

    /* Leave room to write the schema length in later */
    BAIL_ON_ERROR(status = lwmsg_archive_seek_fd(archive, length_offset + sizeof(length)));
    
    /* Write the marshaled protocol schema */
    BAIL_ON_ERROR(status = lwmsg_data_marshal(archive->data_context, type, rep, &buffer));
    
    end_offset = archive->offset;

    /* Go back and write the schema length */
    BAIL_ON_ERROR(status = lwmsg_archive_seek_fd(archive, length_offset));

    length = LWMSG_SWAP32(
        (uint32_t) (end_offset - length_offset) - sizeof(length),
        LWMSG_NATIVE_ENDIAN,
        archive->byte_order);

    BAIL_ON_ERROR(status = lwmsg_archive_write_fd(archive, &length, sizeof(length)));

    /* Seek back to end of stream */
    BAIL_ON_ERROR(status = lwmsg_archive_seek_fd(archive, end_offset));

error:

    if (rep)
    {
        lwmsg_protocol_free_protocol_rep(archive->base.prot, rep);
    }

    return status;
}

LWMsgStatus
lwmsg_archive_write_header_fd(
    LWMsgArchive* archive
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ArchiveHeader header;

    lwmsg_archive_populate_header(archive, &header);
    BAIL_ON_ERROR(status = lwmsg_archive_write_fd(archive, &header, sizeof(header)));

    if (archive->disp & LWMSG_ARCHIVE_SCHEMA)
    {
        BAIL_ON_ERROR(status = lwmsg_archive_write_schema_fd(archive));
    }

error:

    return status;
}

static
void
lwmsg_archive_populate_message_header(
    LWMsgArchive* archive,
    LWMsgMessage* message,
    size_t data_size,
    ArchiveMessage* header
    )
{
    header->flags = LWMSG_SWAP16((uint16_t) message->flags, LWMSG_NATIVE_ENDIAN, archive->byte_order);
    header->status = LWMSG_SWAP32((uint32_t) message->status, LWMSG_NATIVE_ENDIAN, archive->byte_order);
    header->cookie = LWMSG_SWAP16((uint16_t) message->cookie, LWMSG_NATIVE_ENDIAN, archive->byte_order);
    header->tag = LWMSG_SWAP16((int16_t) message->tag, LWMSG_NATIVE_ENDIAN, archive->byte_order);
    header->size = LWMSG_SWAP32((uint32_t) data_size, LWMSG_NATIVE_ENDIAN, archive->byte_order);
}

LWMsgStatus
lwmsg_archive_write_message_fd(
    LWMsgArchive* archive,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char data[2048];
    ArchiveMessage header = {0};
    LWMsgBuffer buffer = {0};
    LWMsgTypeSpec* type = NULL;
    off_t header_offset = archive->offset;
    off_t end_offset = 0;

    BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(archive->base.prot, message->tag, &type));

    buffer.base = data;
    buffer.end = data + sizeof(data);
    buffer.cursor = buffer.base;
    buffer.wrap = lwmsg_archive_write_message_wrap_fd;
    buffer.data = archive;

    /* Leave room to write the header in later */
    BAIL_ON_ERROR(status = lwmsg_archive_seek_fd(archive, header_offset + sizeof(header)));

    /* Write the marshaled data payload */
    BAIL_ON_ERROR(status = lwmsg_data_marshal(archive->data_context, type, message->data, &buffer));

    end_offset = archive->offset;
    
    /* Go back and write the header */
    BAIL_ON_ERROR(status = lwmsg_archive_seek_fd(archive, header_offset));
    lwmsg_archive_populate_message_header(archive, message, end_offset - header_offset - sizeof(header), &header);
    BAIL_ON_ERROR(status = lwmsg_archive_write_fd(archive, &header, sizeof(header)));

    /* Seek back to end of message stream */
    BAIL_ON_ERROR(status = lwmsg_archive_seek_fd(archive, end_offset));

error:

    return status;
}


typedef struct readinfo
{
    LWMsgArchive* archive;
    unsigned char data[2048];
    size_t remaining;
} readinfo;

static
LWMsgStatus
lwmsg_archive_read_message_wrap_fd (
    LWMsgBuffer* buffer,
    size_t needed
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    readinfo* info = buffer->data;
    LWMsgArchive* archive = info->archive;
    size_t count = 0;

    if (needed)
    {
        /* Read next block of data from file */
        if (info->remaining == 0)
        {
            /* The message was longer than the length specified in the header */
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }
        else
        {
            BAIL_ON_ERROR(status = lwmsg_archive_read_fd(
                              archive,
                              info->data,
                              info->remaining > sizeof(info->data) ? sizeof(info->data) : info->remaining,
                              &count));
            info->remaining -= count;
            buffer->end = buffer->base + count;
            buffer->cursor = buffer->base;
        }
    }
    else
    {
        if (info->remaining != 0)
        {
            /* The message was shorter than the length specified in the header */
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_archive_read_schema_fd(
    LWMsgArchive* archive,
    ArchiveHeader* header
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    uint32_t format_flags = LWMSG_SWAP32(header->format_flags, LWMSG_NATIVE_ENDIAN, archive->byte_order);
    uint32_t length = 0;
    size_t count = 0;
    LWMsgProtocolRep* rep = NULL;      

    if (format_flags & ARCHIVE_FORMAT_FLAG_SCHEMA)
    {
        BAIL_ON_ERROR(status = lwmsg_archive_read_fd(archive, &length, sizeof(length), &count));
        
        if (count < sizeof(length))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }

        length = LWMSG_SWAP32(length, LWMSG_NATIVE_ENDIAN, archive->byte_order);

        if (archive->disp & LWMSG_ARCHIVE_SCHEMA)
        {
            readinfo info;
            LWMsgBuffer buffer = {0};
            LWMsgTypeSpec* type = lwmsg_protocol_rep_spec;
            
            info.archive = archive;
            info.remaining = length;
            buffer.base = info.data;
            buffer.end = buffer.base;
            buffer.cursor = buffer.base;
            buffer.wrap = lwmsg_archive_read_message_wrap_fd;
            buffer.data = &info;
            
            /* Unmarshal the schema */
            BAIL_ON_ERROR(status = lwmsg_data_unmarshal(
                              archive->data_context,
                              type,
                              &buffer,
                              (void**) (void*) &rep));

            if (archive->update_protocol)
            {
                /* Insert the schema into the protocol structure */
                BAIL_ON_ERROR(status = lwmsg_protocol_add_protocol_rep(
                                  archive->base.prot,
                                  rep));
            }
            else
            {
                /* Check the schema against the protocol */
                BAIL_ON_ERROR(status = lwmsg_protocol_is_protocol_rep_compatible(
                                  archive->base.prot,
                                  rep));
            }
        }
        else
        {
            BAIL_ON_ERROR(status = lwmsg_archive_seek_fd(archive, archive->offset + length));
        }
    }

error:

    if (rep)
    {
        lwmsg_data_free_graph_cleanup(archive->data_context->context, lwmsg_protocol_rep_spec, rep);
    }

    return status;
}

LWMsgStatus
lwmsg_archive_read_header_fd(
    LWMsgArchive* archive
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ArchiveHeader header;
    size_t count = 0;

    BAIL_ON_ERROR(status = lwmsg_archive_read_fd(archive, &header, sizeof(header), &count));

    if (count < sizeof(header) || memcmp(header.magic, "LWMA", 4) != 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
    }
    
    if (header.version_major > ARCHIVE_VERSION_MAJOR ||
        header.version_minor > ARCHIVE_VERSION_MINOR)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
    }

    archive->version_major = header.version_major;
    archive->version_minor = header.version_minor;

    if (header.version_flags & ARCHIVE_VERSION_FLAG_BIG_ENDIAN)
    {
        archive->byte_order = LWMSG_BIG_ENDIAN;
    }
    else
    {
        archive->byte_order = LWMSG_LITTLE_ENDIAN;
    }

    lwmsg_data_context_set_byte_order(archive->data_context, archive->byte_order);

    BAIL_ON_ERROR(status = lwmsg_archive_read_schema_fd(archive, &header));

error:

    return status;
}

static
LWMsgStatus
lwmsg_archive_read_message_header(
    LWMsgArchive* archive,
    LWMsgMessage* message,
    size_t* size
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ArchiveMessage_v0 header_v0 = {0};
    ArchiveMessage header = {0};
    size_t count = 0;

    switch(archive->version_major)
    {
    case 0:
        BAIL_ON_ERROR(status = lwmsg_archive_read_fd(archive, &header_v0, sizeof(header_v0), &count));

        if (count < sizeof(header))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }

        message->flags = 0;
        message->status = LWMSG_SWAP32(header_v0.status, archive->byte_order, LWMSG_NATIVE_ENDIAN);
        message->cookie = (uint16_t) LWMSG_SWAP32(header_v0.cookie, archive->byte_order, LWMSG_NATIVE_ENDIAN);
        message->tag = (int16_t) LWMSG_SWAP32(header_v0.tag, archive->byte_order, LWMSG_NATIVE_ENDIAN);
        *size = LWMSG_SWAP32(header_v0.size, archive->byte_order, LWMSG_NATIVE_ENDIAN);
        break;
    case ARCHIVE_VERSION_MAJOR:
        BAIL_ON_ERROR(status = lwmsg_archive_read_fd(archive, &header, sizeof(header), &count));

        if (count < sizeof(header))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }

        message->flags = LWMSG_SWAP16(header.flags, archive->byte_order, LWMSG_NATIVE_ENDIAN);
        message->status = LWMSG_SWAP32(header.status, archive->byte_order, LWMSG_NATIVE_ENDIAN);
        message->cookie = LWMSG_SWAP16(header.cookie, archive->byte_order, LWMSG_NATIVE_ENDIAN);
        message->tag = LWMSG_SWAP16(header.tag, archive->byte_order, LWMSG_NATIVE_ENDIAN);
        *size = LWMSG_SWAP32(header.size, archive->byte_order, LWMSG_NATIVE_ENDIAN);
        break;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_archive_read_message_fd(
    LWMsgArchive* archive,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    readinfo info;
    LWMsgBuffer buffer = {0};
    LWMsgTypeSpec* type = NULL;
    size_t message_size = 0;

    BAIL_ON_ERROR(status = lwmsg_archive_read_message_header(archive, message, &message_size));
    BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(archive->base.prot, message->tag, &type));
    
    info.archive = archive;
    info.remaining = message_size;
    buffer.base = info.data;
    buffer.end = buffer.base;
    buffer.cursor = buffer.base;
    buffer.wrap = lwmsg_archive_read_message_wrap_fd;
    buffer.data = &info;

    /* Unmarshal the message payload */
    BAIL_ON_ERROR(status = lwmsg_data_unmarshal(archive->data_context, type, &buffer, &message->data));

error:

    return status;
}
