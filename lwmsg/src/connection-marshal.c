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
 *        Connection-marshal.c
 *
 * Abstract:
 *
 *        Connection API
 *        Marshalling logic for connection-specific data types
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "config.h"
#include <lwmsg/type.h>
#include <lwmsg/data.h>
#include "convert-private.h"
#include "util-private.h"
#include "connection-private.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

static LWMsgStatus
lwmsg_connection_marshal_fd(
    LWMsgDataContext* context, 
    LWMsgTypeAttrs* attrs,
    void* object,
    void* transmit_object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int fd = *(int*) object;
    unsigned char indicator;
    LWMsgAssoc* assoc = NULL;

    BAIL_ON_ERROR(status = lwmsg_context_get_data(
                      lwmsg_data_context_get_context(context),
                      "assoc",
                      (void**) (void*) &assoc));

    if (fd > 0)
    {
        indicator = 0xFF;
        BAIL_ON_ERROR(status = lwmsg_connection_queue_fd(assoc, fd));
    }
    else
    {
        indicator = 0x00;
    }

    *(unsigned char*) transmit_object = indicator;

error:

    return status;
}

static LWMsgStatus
lwmsg_connection_unmarshal_fd(
    LWMsgDataContext* context,
    LWMsgTypeAttrs* attrs,
    void* transmit_object,
    void* natural_object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char flag = *(unsigned char*) transmit_object;
    int fd = -1;
    int fd2 = -1;
    LWMsgAssoc* assoc = NULL;

    BAIL_ON_ERROR(status = lwmsg_context_get_data(
                      lwmsg_data_context_get_context(context),
                      "assoc",
                      (void**) (void*) &assoc));

    if (flag)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_dequeue_fd(assoc, &fd));
    }

    if (fd == 0)
    {
        fd2 = dup(fd);
        if (fd2 < 0)
        {
            BAIL_ON_ERROR(status = lwmsg_status_map_errno(errno));
        }
        close(fd);
        fd = fd2;
    }

    *(int*) natural_object = fd;
    fd = -1;

error:

    if (fd >= 0)
    {
        close(fd);
    }

    return status;
}

static
void
lwmsg_connection_free_fd(
    LWMsgDataContext* context,
    LWMsgTypeAttrs* attrs,
    void* object,
    void* data
    )
{
    int fd = *(int*) object;

    if (fd > 0)
    {
        close(fd);
    }

    return;
}

LWMsgTypeSpec indicator_spec[] =
{
    LWMSG_UINT8(unsigned char),
    LWMSG_TYPE_END
};

LWMsgTypeClass lwmsg_fd_type_class =
{
    .is_pointer = LWMSG_FALSE,
    .transmit_type = indicator_spec,
    .marshal = lwmsg_connection_marshal_fd,
    .unmarshal = lwmsg_connection_unmarshal_fd,
    .destroy_presented = lwmsg_connection_free_fd,
    .destroy_transmitted = NULL /* Nothing to free in transmitted form */
};
