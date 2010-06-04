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
 * HAVE QUESTIONS, OR WISHTO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        threadpool-common.h
 *
 * Abstract:
 *
 *        Thread pool API
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LWBASE_THREADPOOL_COMMON_H__
#define __LWBASE_THREADPOOL_COMMON_H__

struct _LW_THREAD_POOL_ATTRIBUTES
{
    BOOLEAN bDelegateTasks;
    LONG lTaskThreads;
    LONG lWorkThreads;
    ULONG ulTaskThreadStackSize;
    ULONG ulWorkThreadStackSize;
};

NTSTATUS
AcquireDelegatePool(
    PLW_THREAD_POOL* ppPool
    );

VOID
ReleaseDelegatePool(
    PLW_THREAD_POOL* ppPool
    );

static
inline
BOOLEAN
GetDelegateAttr(
    PLW_THREAD_POOL_ATTRIBUTES pAttrs
    )
{
    return pAttrs ? pAttrs->bDelegateTasks : TRUE;
}

static
inline
ULONG
GetTaskThreadsAttr(
    PLW_THREAD_POOL_ATTRIBUTES pAttrs,
    int numCpus
    )
{
    LONG lCount = pAttrs ? pAttrs->lTaskThreads : -1;

    return lCount < 0 ? -lCount * numCpus : lCount;
}

static
inline
ULONG
GetWorkThreadsAttr(
    PLW_THREAD_POOL_ATTRIBUTES pAttrs,
    int numCpus
    )
{
    LONG lCount = pAttrs ? pAttrs->lWorkThreads : -4;

    return lCount < 0 ? -lCount * numCpus : lCount;
}

VOID
SetCloseOnExec(
    int Fd
    );

#endif
