/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        queue.h
 *
 * Abstract:
 *
 *        NFS3
 *
 *        Queue interface.
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

typedef DWORD (*PFNNFS3_FOREACH_QUEUE_ITEM) (PVOID pItem, PVOID pUserData);

typedef struct __NFS3_QUEUE_ITEM NFS3_QUEUE_ITEM, *PNFS3_QUEUE_ITEM;

typedef struct __NFS3_QUEUE NFS3_QUEUE, *PNFS3_QUEUE;

NTSTATUS
Nfs3QueueCreate(
    PNFS3_QUEUE* ppQueue
    );

NTSTATUS
Nfs3QueueEnqueue(
    PNFS3_QUEUE pQueue,
    PVOID       pItem
    );

NTSTATUS
Nfs3QueueEnqueueFront(
    PNFS3_QUEUE pQueue,
    PVOID      pItem
    );

PVOID
Nfs3QueueDequeue(
    PNFS3_QUEUE pQueue
    );

BOOLEAN
Nfs3QueueIsEmpty(
    PNFS3_QUEUE pQueue
    );

NTSTATUS
Nfs3QueueForeach(
    PNFS3_QUEUE pQueue,
    PFNNFS3_FOREACH_QUEUE_ITEM pfnAction,
    PVOID pUserData
    );

VOID
Nfs3QueueFree(
    PNFS3_QUEUE pQueue
    );

#endif  // __QUEUE_H__

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

