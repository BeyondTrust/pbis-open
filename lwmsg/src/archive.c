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
 *        archive.c
 *
 * Abstract:
 *
 *        Archive API
 *        Primary entry points
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "config.h"
#include "archive-private.h"
#include "util-private.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

static LWMsgStatus
lwmsg_archive_construct(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgArchive* archive = ARCHIVE_PRIVATE(assoc);

    archive->fd = -1;
    archive->mode = 0;
    archive->disp = 0;
    archive->byte_order = LWMSG_BIG_ENDIAN;

    BAIL_ON_ERROR(status = lwmsg_data_context_new(&assoc->context, &archive->data_context));

error:

    return status;
}

static void
lwmsg_archive_destruct(
    LWMsgAssoc* assoc
    )
{
    LWMsgArchive* archive = ARCHIVE_PRIVATE(assoc);

    if (archive->fd != -1)
    {
        close(archive->fd);
    }

    if (archive->file)
    {
        free(archive->file);
    }

    if (archive->data_context)
    {
        lwmsg_data_context_delete(archive->data_context);
    }
}


static LWMsgStatus
lwmsg_archive_close_assoc(
    LWMsgAssoc* assoc
    )
{
    LWMsgArchive* archive = ARCHIVE_PRIVATE(assoc);

    return lwmsg_archive_close(archive);
}

static LWMsgStatus
lwmsg_archive_reset(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = LWMSG_STATUS_UNSUPPORTED);

error:
    
    return status;
}

static LWMsgStatus
lwmsg_archive_send_msg(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    )
{
    LWMsgArchive* archive = ARCHIVE_PRIVATE(assoc);

    return lwmsg_archive_write_message(archive, message);
}

static LWMsgStatus
lwmsg_archive_recv_msg(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    )
{
    LWMsgArchive* archive = ARCHIVE_PRIVATE(assoc);

    return lwmsg_archive_read_message(archive, message);
}

static LWMsgStatus
lwmsg_archive_finish(
    LWMsgAssoc* assoc,
    LWMsgMessage** message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    *message = NULL;

    return status;
}

static LWMsgStatus
lwmsg_archive_set_nonblock(
    LWMsgAssoc* assoc,
    LWMsgBool nonblock
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = LWMSG_STATUS_UNSUPPORTED);

error:

    return status;
}

static LWMsgStatus
lwmsg_archive_get_session(
    LWMsgAssoc* assoc,
    LWMsgSession** session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    
    BAIL_ON_ERROR(status = LWMSG_STATUS_UNSUPPORTED);
    
error:

    return status;
}
    
static LWMsgAssocState
lwmsg_archive_get_state(
    LWMsgAssoc* assoc
    )
{
    LWMsgArchive* archive = ARCHIVE_PRIVATE(assoc);

    if (archive->fd != -1)
    {
        return LWMSG_ASSOC_STATE_IDLE;
    }
    else
    {
        return LWMSG_ASSOC_STATE_NOT_ESTABLISHED;
    }
}

static
LWMsgStatus
lwmsg_archive_set_timeout(
    LWMsgAssoc* assoc,
    LWMsgTimeout type,
    LWMsgTime* value
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = LWMSG_STATUS_UNSUPPORTED);

error:

    return status;
}

static
LWMsgStatus
lwmsg_archive_open_fd(
    LWMsgArchive* archive
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (archive->fd == -1)
    {
        if (archive->file)
        {
            archive->fd = open(
                archive->file,
                ((archive->disp & LWMSG_ARCHIVE_READ) ? O_RDONLY : (O_WRONLY | O_CREAT)),
                archive->mode);
            if (archive->fd < 0)
            {
                BAIL_ON_ERROR(status = RAISE_ERRNO(&archive->base.context));
            }
        }
        else
        {
            ASSOC_RAISE_ERROR(&archive->base, status = LWMSG_STATUS_INVALID_STATE, "No fd or filename specified");
        }
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_archive_connect(
    LWMsgAssoc* assoc,
    LWMsgSession* session
    )
{
    LWMsgArchive* archive = ARCHIVE_PRIVATE(assoc);
 
    return lwmsg_archive_open(archive, LWMSG_ARCHIVE_WRITE);
}

static LWMsgAssocClass archive_class =
{
    .construct = lwmsg_archive_construct,
    .destruct = lwmsg_archive_destruct,
    .send_msg = lwmsg_archive_send_msg,
    .recv_msg = lwmsg_archive_recv_msg,
    .close = lwmsg_archive_close_assoc,
    .reset = lwmsg_archive_reset,
    .get_session = lwmsg_archive_get_session,
    .get_state = lwmsg_archive_get_state,
    .set_timeout = lwmsg_archive_set_timeout,
    .connect_peer = lwmsg_archive_connect,
    .set_nonblock = lwmsg_archive_set_nonblock,
    .finish = lwmsg_archive_finish
};

LWMsgStatus
lwmsg_archive_new(
    const LWMsgContext* context,
    LWMsgProtocol* prot,
    LWMsgArchive** archive
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;

    BAIL_ON_ERROR(status = lwmsg_assoc_new(context, prot, &archive_class, sizeof(LWMsgArchive), &assoc));

    *archive = ARCHIVE_PRIVATE(assoc);

done:

    return status;

error:

    goto done;
}

LWMsgStatus
lwmsg_archive_set_fd(
    LWMsgArchive* archive,
    int fd
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (fd < 0)
    {
        ASSOC_RAISE_ERROR(&archive->base, status = LWMSG_STATUS_INVALID_PARAMETER, "Invalid file descriptor");
    }

    if (archive->file)
    {
        free(archive->file);
        archive->file = NULL;
    }

    if (archive->fd >= 0)
    {
        close(archive->fd);
        archive->fd = -1;
    }

    archive->fd = fd;

error:

    return status;
}

LWMsgStatus
lwmsg_archive_set_file(
    LWMsgArchive* archive,
    const char* file,
    mode_t mode
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (archive->file)
    {
        free(archive->file);
        archive->file = NULL;
    }

    if (archive->fd >= 0)
    {
        close(archive->fd);
        archive->fd = -1;
    }

    archive->file = strdup(file);
    
    if (!archive->file)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    archive->mode = mode;

error:

    return status;
}

LWMsgStatus
lwmsg_archive_set_byte_order(
    LWMsgArchive* archive,
    LWMsgByteOrder order
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    archive->byte_order = order;

    lwmsg_data_context_set_byte_order(archive->data_context, order);

    return status;
}

void
lwmsg_archive_set_protocol_update(
    LWMsgArchive* archive,
    LWMsgBool update
    )
{
    archive->update_protocol = update;
}

LWMsgAssoc*
lwmsg_archive_as_assoc(
    LWMsgArchive* archive
    )
{
    return &archive->base;
}

void
lwmsg_archive_delete(
    LWMsgArchive* archive
    )
{
    lwmsg_assoc_delete(&archive->base);
}

LWMsgStatus
lwmsg_archive_open(
    LWMsgArchive* archive,
    LWMsgArchiveDisposition disp
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (archive->fd >= 0)
    {
        ARCHIVE_RAISE_ERROR(archive, status = LWMSG_STATUS_INVALID_STATE, "Archive is already open");
    }

    archive->disp = disp;
 
    switch (archive->disp & (LWMSG_ARCHIVE_READ | LWMSG_ARCHIVE_WRITE))
    {
    case LWMSG_ARCHIVE_READ:
        BAIL_ON_ERROR(status = lwmsg_archive_open_fd(archive));
        BAIL_ON_ERROR(status = lwmsg_archive_read_header_fd(archive));
        break;
    case LWMSG_ARCHIVE_WRITE:
        BAIL_ON_ERROR(status = lwmsg_archive_open_fd(archive));
        BAIL_ON_ERROR(status = lwmsg_archive_write_header_fd(archive));
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_PARAMETER);
    }

error:

    return status;
}

LWMsgStatus
lwmsg_archive_close(
    LWMsgArchive* archive
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (archive->fd != -1)
    {
        close(archive->fd);
        archive->fd = -1;
    }

    return status;
}

LWMsgStatus
lwmsg_archive_write_message(
    LWMsgArchive* archive,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (!(archive->disp & LWMSG_ARCHIVE_WRITE))
    {
        ARCHIVE_RAISE_ERROR(archive, status = LWMSG_STATUS_INVALID_STATE,
                            "Archive not open for writing");
    }

    BAIL_ON_ERROR(status = lwmsg_archive_write_message_fd(archive, message));

error:

    return status;
}

LWMsgStatus
lwmsg_archive_read_message(
    LWMsgArchive* archive,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (!archive->disp & LWMSG_ARCHIVE_READ)
    {
        ARCHIVE_RAISE_ERROR(archive, status = LWMSG_STATUS_INVALID_STATE,
                            "Archive not open for reading");
    }

    BAIL_ON_ERROR(status = lwmsg_archive_read_message_fd(archive, message));

error:

    return status;
}

LWMsgStatus
lwmsg_archive_destroy_message(
    LWMsgArchive* archive,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpec* type = NULL;

    if (message->tag >= 0)
    {
        BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(archive->base.prot, message->tag, &type));

        if (type != NULL)
        {
            BAIL_ON_ERROR(status = lwmsg_data_free_graph(archive->data_context, type, message->data));
        }

        message->tag = -1;
        message->data = NULL;
    }

error:

    return status;
}
