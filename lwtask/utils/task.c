/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        syslog.c
 *
 * Abstract:
 *
 *        Likewise Task System (LWTASK)
 *
 *        Task API
 *
 *        Utilities
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

DWORD
LwTaskDuplicateArgList(
    PLW_TASK_ARG  pTaskArgArray,
    DWORD         dwNumArgs,
    PLW_TASK_ARG* ppTaskArgArray,
    PDWORD        pdwNumArgs
    )
{
    DWORD dwError = 0;
    PLW_TASK_ARG pTaskArgArrayCopy = NULL;
    DWORD        dwNumArgsCopy = 0;
    DWORD        iArg = 0;

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_ARG) * dwNumArgs,
                    (PVOID*)&pTaskArgArrayCopy);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwNumArgsCopy = dwNumArgs;

    for (; iArg < dwNumArgs; iArg++)
    {
        PLW_TASK_ARG pSrcArg = &pTaskArgArray[iArg];
        PLW_TASK_ARG pDstArg = &pTaskArgArrayCopy[iArg];

        dwError = LwAllocateString(
                        pSrcArg->pszArgName,
                        &pDstArg->pszArgName);
        BAIL_ON_LW_TASK_ERROR(dwError);

        pDstArg->dwArgType = pSrcArg->dwArgType;

        if (pSrcArg->pszArgValue)
        {
            dwError = LwAllocateString(
                        pSrcArg->pszArgValue,
                        &pDstArg->pszArgValue);
            BAIL_ON_LW_TASK_ERROR(dwError);
        }
    }

    *ppTaskArgArray = pTaskArgArrayCopy;
    *pdwNumArgs = dwNumArgsCopy;

cleanup:

    return dwError;

error:

    *ppTaskArgArray = NULL;
    *pdwNumArgs = 0;

    if (pTaskArgArrayCopy)
    {
        LwTaskFreeArgArray(pTaskArgArrayCopy, dwNumArgsCopy);
    }

    goto cleanup;
}

VOID
LwTaskFreeTaskInfoArray(
    PLW_TASK_INFO pTaskInfoArray,
    DWORD         dwNumTaskInfos
    )
{
    if (pTaskInfoArray)
    {
        DWORD iTaskInfos = 0;

        for (; iTaskInfos < dwNumTaskInfos; iTaskInfos++)
        {
            PLW_TASK_INFO pTaskInfo = &pTaskInfoArray[iTaskInfos];

            if (pTaskInfo->pArgArray)
            {
                LwTaskFreeArgArray(pTaskInfo->pArgArray, pTaskInfo->dwNumArgs);
            }

            if (pTaskInfo->pszTaskId)
            {
                LwFreeMemory(pTaskInfo->pszTaskId);
            }
        }

        LwFreeMemory(pTaskInfoArray);
    }
}

VOID
LwTaskFreeArgInfoArray(
    PLW_TASK_ARG_INFO pArgInfoArray,
    DWORD             dwNumArgInfos
    )
{
    if (pArgInfoArray)
    {
        DWORD iArgInfo = 0;

        for (; iArgInfo < dwNumArgInfos; iArgInfo++)
        {
            PLW_TASK_ARG_INFO pArgInfo = &pArgInfoArray[iArgInfo];

            if (pArgInfo->pszArgName)
            {
                LwFreeMemory(pArgInfo->pszArgName);
            }
        }

        LwFreeMemory(pArgInfoArray);
    }
}

VOID
LwTaskFreeArgArray(
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs
    )
{
    if (pArgArray)
    {
        DWORD iArg = 0;

        for (; iArg < dwNumArgs; iArg++)
        {
            PLW_TASK_ARG pArg = &pArgArray[iArg];

            if (pArg->pszArgName)
            {
                LwFreeMemory(pArg->pszArgName);
            }

            if (pArg->pszArgValue)
            {
                LwFreeMemory(pArg->pszArgValue);
            }
        }

        LwFreeMemory(pArgArray);
    }
}

VOID
LwTaskFreeMemory(
    PVOID pMemory
    )
{
	LW_SAFE_FREE_MEMORY(pMemory);
}
