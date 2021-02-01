/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        syslog.c
 *
 * Abstract:
 *
 *        BeyondTrust Task System (LWTASK)
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
