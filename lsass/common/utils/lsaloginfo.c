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
 *        lsaloginfo.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Log Info
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
LsaBuildLogInfo(
    LsaLogLevel    maxAllowedLogLevel,
    LsaLogTarget   logTarget,
    PCSTR          pszPath,
    PLSA_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PLSA_LOG_INFO pLogInfo = NULL;
    
    BAIL_ON_INVALID_POINTER(ppLogInfo);
    
    dwError = LwAllocateMemory(
                    sizeof(LSA_LOG_INFO),
                    (PVOID*)&pLogInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pszPath))
    {
        dwError = LwAllocateString(
                        pszPath,
                        &pLogInfo->pszPath);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    pLogInfo->maxAllowedLogLevel  = maxAllowedLogLevel;
    pLogInfo->logTarget = logTarget;
    
    *ppLogInfo = pLogInfo;
    
cleanup:

    return dwError;
    
error:
    
    if (ppLogInfo)
    {
        *ppLogInfo = NULL;
    }
    
    if (pLogInfo)
    {
        LsaFreeLogInfo(pLogInfo);
    }
    
    goto cleanup;
}

VOID
LsaFreeLogInfo(
    PLSA_LOG_INFO pLogInfo
    )
{
    LW_SAFE_FREE_STRING(pLogInfo->pszPath);
    LwFreeMemory(pLogInfo);
}
