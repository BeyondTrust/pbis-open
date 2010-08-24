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
            DWORD         iArg = 0;

            if (pTaskInfo->pArgArray)
            {
                for (; iArg < pTaskInfo->dwNumArgs; iArg++)
                {
                    PLW_TASK_ARG pTaskArg = &pTaskInfo->pArgArray[iArg];

                    if (pTaskArg->pszArgName)
                    {
                        LwFreeMemory(pTaskArg->pszArgName);
                    }
                    if (pTaskArg->pszArgValue)
                    {
                        LwFreeMemory(pTaskArg->pszArgValue);
                    }
                }

                LwFreeMemory(pTaskInfo->pArgArray);
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
