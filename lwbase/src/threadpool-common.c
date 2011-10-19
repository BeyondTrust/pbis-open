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
 *        threadpool-common.c
 *
 * Abstract:
 *
 *        Thread pool API (common)
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"
#include "threadpool-common.h"

static PLW_THREAD_POOL gpDelegatePool = NULL;
static ULONG gpDelegatePoolRefCount = 0;
static pthread_mutex_t gpDelegatePoolLock = PTHREAD_MUTEX_INITIALIZER;

NTSTATUS
AcquireDelegatePool(
    PLW_THREAD_POOL* ppPool
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LW_THREAD_POOL_ATTRIBUTES attrs =
        {
            .bDelegateTasks = FALSE,
            .lTaskThreads = -1,
            .lWorkThreads = 0,
            .ulTaskThreadStackSize = 0,
            .ulWorkThreadStackSize = 0
        };
        

    pthread_mutex_lock(&gpDelegatePoolLock);

    if (!gpDelegatePool)
    {
        status = LwRtlCreateThreadPool(&gpDelegatePool, &attrs);
        GOTO_ERROR_ON_STATUS(status);

        gpDelegatePoolRefCount = 1;
    }
    else
    {
        gpDelegatePoolRefCount++;
    }

    *ppPool = gpDelegatePool;

cleanup:

    pthread_mutex_unlock(&gpDelegatePoolLock);

    return status;

error:

    goto cleanup;
}

VOID
ReleaseDelegatePool(
    PLW_THREAD_POOL* ppPool
    )
{
    if (*ppPool)
    {
        pthread_mutex_lock(&gpDelegatePoolLock);
        
        assert(*ppPool == gpDelegatePool);
        
        if (--gpDelegatePoolRefCount == 0)
        {
            LwRtlFreeThreadPool(&gpDelegatePool);
        }
        
        pthread_mutex_unlock(&gpDelegatePoolLock);
        *ppPool = NULL;
    }
}

LW_NTSTATUS
LwRtlCreateThreadPoolAttributes(
    LW_OUT PLW_THREAD_POOL_ATTRIBUTES* ppAttrs
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_THREAD_POOL_ATTRIBUTES pAttrs = NULL;

    status = LW_RTL_ALLOCATE_AUTO(&pAttrs);
    GOTO_ERROR_ON_STATUS(status);
    
    pAttrs->bDelegateTasks = TRUE;
    pAttrs->lTaskThreads = -1;
    pAttrs->lWorkThreads = -4;
    pAttrs->ulTaskThreadStackSize = 0;
    pAttrs->ulWorkThreadStackSize = 0;

    *ppAttrs = pAttrs;

cleanup:

    return status;

error:

    *ppAttrs = NULL;

    goto cleanup;
}

LW_NTSTATUS
LwRtlSetThreadPoolAttribute(
    LW_IN LW_OUT PLW_THREAD_POOL_ATTRIBUTES pAttrs,
    LW_IN LW_THREAD_POOL_OPTION Option,
    ...
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    va_list ap;

    va_start(ap, Option);

    switch(Option)
    {
    case LW_THREAD_POOL_OPTION_DELEGATE_TASKS:
        pAttrs->bDelegateTasks = va_arg(ap, int);
        break;
    case LW_THREAD_POOL_OPTION_TASK_THREADS:
        pAttrs->lTaskThreads = va_arg(ap, LONG);
        break;
    case LW_THREAD_POOL_OPTION_WORK_THREADS:
        pAttrs->lWorkThreads = va_arg(ap, LONG);
        break;
    case LW_THREAD_POOL_OPTION_TASK_THREAD_STACK_SIZE:
        pAttrs->ulTaskThreadStackSize = va_arg(ap, ULONG);
        break;
    case LW_THREAD_POOL_OPTION_WORK_THREAD_STACK_SIZE:
        pAttrs->ulWorkThreadStackSize = va_arg(ap, ULONG);
        break;
    default:
        status = STATUS_NOT_SUPPORTED;
        GOTO_ERROR_ON_STATUS(status);
    }

cleanup:
    
    va_end(ap);

    return status;

error:

    goto cleanup;
}

VOID
LwRtlFreeThreadPoolAttributes(
    LW_IN LW_OUT PLW_THREAD_POOL_ATTRIBUTES* ppAttrs
    )
{
    RTL_FREE(ppAttrs);
}

VOID
SetCloseOnExec(
    int Fd
    )
{
    fcntl(Fd, F_SETFD, FD_CLOEXEC);
}
