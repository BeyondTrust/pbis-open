/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#include "includes.h"

DWORD
LwTaskGetTypes(
    LW_TASK_TYPE* pTaskTypes,
    PDWORD        pdwNumTypes
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LwTaskGetSchema(
    LW_TASK_TYPE       taskType,
    PLW_TASK_ARG_INFO* ppArgInfoArray,
    PDWORD             pdwNumArgInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LwTaskEnum(
    LW_TASK_TYPE   taskType,
    PLW_TASK_INFO* pTaskInfoArray,
    PDWORD         pdwNumTaskInfos,
    PDWORD         pdwResume
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LwTaskCreate(
    LW_TASK_TYPE taskType,
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LwTaskStart(
    PCSTR pszTaskId
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LwTaskGetStatus(
    PCSTR           pszTaskId,
    PLW_TASK_STATUS pStatus
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LwTaskStop(
    PCSTR pszTaskId
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LwTaskDelete(
    PCSTR pszTaskId
    )
{
    DWORD dwError = 0;

    return dwError;
}


