/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        context.c
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Execution context
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
VOID
LwTaskFreeContext(
    PLW_TASK_CONTEXT pContext
    );

DWORD
LwTaskCreateContext(
    PLW_SRV_TASK      pTask,
    PLW_TASK_ARG*     ppArgArray,
    PDWORD            pdwNumArgs,
    PLW_TASK_CONTEXT* ppContext
    )
{
    DWORD dwError = 0;
    PLW_TASK_CONTEXT pContext = NULL;

    dwError = LwAllocateMemory(sizeof(LW_TASK_CONTEXT), (PVOID*)&pContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pContext->pTask = pTask;
    InterlockedIncrement(&pTask->refCount);

    pContext->pArgArray = *ppArgArray;
    *ppArgArray = NULL;

    pContext->dwNumArgs = *pdwNumArgs;
    *pdwNumArgs = 0;

    *ppContext = pContext;

cleanup:

    return dwError;

error:

    *ppContext = NULL;

    if (pContext)
    {
        LwTaskReleaseContext(pContext);
    }

    goto cleanup;
}

BOOLEAN
LwTaskIsValidContext(
    PLW_TASK_CONTEXT pContext
    )
{
    return (pContext && pContext->pTask) ? TRUE : FALSE;
}

VOID
LwTaskReleaseContextHandle(
    HANDLE hContext
    )
{
    LwTaskReleaseContext((PLW_TASK_CONTEXT)hContext);
}

VOID
LwTaskReleaseContext(
    PLW_TASK_CONTEXT pContext
    )
{
    if (InterlockedDecrement(&pContext->refCount) == 0)
    {
        LwTaskFreeContext(pContext);
    }
}

static
VOID
LwTaskFreeContext(
    PLW_TASK_CONTEXT pContext
    )
{
    if (pContext->pTask)
    {
        LwTaskSrvRelease(pContext->pTask);
    }

    if (pContext->pArgArray)
    {
        LwTaskFreeArgArray(pContext->pArgArray, pContext->dwNumArgs);
    }
}
