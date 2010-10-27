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
 *        prodcons.h
 *
 * Abstract:
 *
 *        Likewise I/O (LWIO) - nfs3
 *
 *        Producer/consumer queue interface.
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

#ifndef __PRODCONS_H__
#define __PRODCONS_H__

typedef VOID (*PFN_PRODCONS_QUEUE_FREE_ITEM)(PVOID pItem);

typedef struct __NFS3_PRODCONS_QUEUE NFS3_PRODCONS_QUEUE, *PNFS3_PRODCONS_QUEUE;

NTSTATUS
Nfs3ProdConsInit(
    ULONG                        ulNumMaxItems,
    PFN_PRODCONS_QUEUE_FREE_ITEM pfnFreeItem,
    PNFS3_PRODCONS_QUEUE*        ppQueue
    );

NTSTATUS
Nfs3ProdConsInitContents(
    PNFS3_PRODCONS_QUEUE         pQueue,
    ULONG                        ulNumMaxItems,
    PFN_PRODCONS_QUEUE_FREE_ITEM pfnFreeItem
    );

NTSTATUS
Nfs3ProdConsEnqueue(
    PNFS3_PRODCONS_QUEUE pQueue,
    PVOID                 pItem
    );

NTSTATUS
Nfs3ProdConsEnqueueFront(
    PNFS3_PRODCONS_QUEUE pQueue,
    PVOID                 pItem
    );

NTSTATUS
Nfs3ProdConsDequeue(
    PNFS3_PRODCONS_QUEUE pQueue,
    PVOID*                ppItem
    );

NTSTATUS
Nfs3ProdConsTimedDequeue(
    PNFS3_PRODCONS_QUEUE pQueue,
    struct timespec*      pTimespec,
    PVOID*                ppItem
    );

VOID
Nfs3ProdConsFree(
    PNFS3_PRODCONS_QUEUE* pQueue
    );
 
VOID
Nfs3ProdConsFreeContents(
    PNFS3_PRODCONS_QUEUE pQueue
    ); 
 
#endif  // __PRODCONS_H__

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

