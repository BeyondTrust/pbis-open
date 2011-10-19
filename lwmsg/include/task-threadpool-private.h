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
 *        task.h
 *
 * Abstract:
 *
 *        Task manager API (private header -- select-based implementation)
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */
#ifndef __LWMSG_TASK_SELECT_PRIVATE_H__
#define __LWMSG_TASK_SELECT_PRIVATE_H__

#include <lwmsg/common.h>

#include "task-private.h"
#include "util-private.h"

#include <pthread.h>
#include <lw/base.h>

struct LWMsgTask
{
    void* task_data;
    PLW_TASK real_task;
    LWMsgTaskFunction real_function;
};

static inline
LWMsgStatus
lwmsg_error_map_ntstatus(
    NTSTATUS status
    )
{
    switch (status)
    {
    case STATUS_SUCCESS:
        return LWMSG_STATUS_SUCCESS;
    case STATUS_INSUFFICIENT_RESOURCES:
        return LWMSG_STATUS_MEMORY;
    default:
        return LWMSG_STATUS_ERROR;
    }
}

#define MAP_NTSTATUS(x) lwmsg_error_map_ntstatus((x))

#endif
