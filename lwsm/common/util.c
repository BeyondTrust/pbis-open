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
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        util.c
 *
 * Abstract:
 *
 *        Utility functions
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

PWSTR
LwSmWc16sLastChar(
    PWSTR pwszHaystack,
    WCHAR needle
    )
{
    PWSTR pwszFound = NULL;
    PWSTR pwszCursor = NULL;

    for (pwszCursor = pwszHaystack; *pwszCursor; pwszCursor++)
    {
        if (*pwszCursor == needle)
        {
            pwszFound = pwszCursor;
        }
    }
    
    return pwszFound;
}

VOID
LwSmFreeStringList(
    PWSTR* ppwszStrings
    )
{
    PWSTR* ppwszCursor = NULL;

    if (ppwszStrings)
    {
        for (ppwszCursor = ppwszStrings; *ppwszCursor; ppwszCursor++)
        {
            LwFreeMemory(*ppwszCursor);
        }
        LwFreeMemory(ppwszStrings);
    }

    return;
}

size_t
LwSmStringListLength(
    PWSTR* ppwszStrings
    )
{
    PWSTR* ppwszCursor = NULL;
    size_t len = 0;

    for (ppwszCursor = ppwszStrings; *ppwszCursor; ppwszCursor++)
    {
        len++;
    }

    return len;
}

size_t
LwSmStringListLengthA(
    PSTR* ppszStrings
    )
{
    PSTR* ppszCursor = NULL;
    size_t len = 0;

    for (ppszCursor = ppszStrings; *ppszCursor; ppszCursor++)
    {
        len++;
    }

    return len;
}

DWORD
LwSmCopyStringList(
    PWSTR* ppwszStrings,
    PWSTR** pppwszCopy
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszCopy = NULL;
    size_t len = 0;
    size_t i = 0;

    if (ppwszStrings)
    {
        len = LwSmStringListLength(ppwszStrings);
        
        dwError = LwAllocateMemory(sizeof(*ppwszCopy) * (len + 1), OUT_PPVOID(&ppwszCopy));
        BAIL_ON_ERROR(dwError);
        
        for (i = 0; i < len; i++)
        {
            dwError = LwAllocateWc16String(&ppwszCopy[i], ppwszStrings[i]);
            BAIL_ON_ERROR(dwError);
        }
        
        *pppwszCopy = ppwszCopy;
    }
    else
    {
        *pppwszCopy = NULL;
    }

cleanup:

    return dwError;

error:

    *pppwszCopy = NULL;

    if (ppwszCopy)
    {
        LwSmFreeStringList(ppwszCopy);
    }

    goto cleanup;
}

BOOLEAN
LwSmStringListContains(
    PWSTR* ppwszStrings,
    PWSTR pwszNeedle
    )
{
    size_t i = 0;

    for (i = 0; ppwszStrings[i]; i++)
    {
        if (LwRtlWC16StringIsEqual(ppwszStrings[i], pwszNeedle, FALSE))
        {
            return TRUE;
        }
    }

    return FALSE;
}

DWORD
LwSmStringListAppend(
    PWSTR** pppwszStrings,
    PWSTR pwszElement
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszNewStrings = NULL;
    size_t len = 0;

    len = LwSmStringListLength(*pppwszStrings);

    dwError = LwReallocMemory(
        *pppwszStrings,
        OUT_PPVOID(&ppwszNewStrings),
        sizeof(*ppwszNewStrings) * (len + 2));
    BAIL_ON_ERROR(dwError);

    ppwszNewStrings[len] = pwszElement;
    ppwszNewStrings[len+1] = NULL;

    *pppwszStrings = ppwszNewStrings;

error:

    return dwError;
}

DWORD
LwSmCopyString(
    PCWSTR pwszString,
    PWSTR* ppwszCopy
    )
{
    if (pwszString)
    {
        return LwAllocateWc16String(ppwszCopy, pwszString);
    }
    else
    {
        *ppwszCopy = NULL;
        return LW_ERROR_SUCCESS;
    }
}

DWORD
LwSmCopyServiceInfo(
    PLW_SERVICE_INFO pInfo,
    PLW_SERVICE_INFO* ppCopy
    )
{
    DWORD dwError = 0;
    PLW_SERVICE_INFO pCopy = NULL;

    dwError = LwAllocateMemory(sizeof(*pCopy), OUT_PPVOID(&pCopy));
    BAIL_ON_ERROR(dwError);

    pCopy->type = pInfo->type;
    pCopy->bAutostart = pInfo->bAutostart;
    pCopy->dwFdLimit = pInfo->dwFdLimit;
    pCopy->dwCoreSize = pInfo->dwCoreSize;
    pCopy->DefaultLogType = pInfo->DefaultLogType;
    pCopy->DefaultLogLevel = pInfo->DefaultLogLevel;
    
    dwError = LwSmCopyString(pInfo->pwszPath, &pCopy->pwszPath);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyString(pInfo->pwszName, &pCopy->pwszName);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyString(pInfo->pwszDescription, &pCopy->pwszDescription);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyString(pInfo->pwszGroup, &pCopy->pwszGroup);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyStringList(pInfo->ppwszArgs, &pCopy->ppwszArgs);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyStringList(pInfo->ppwszEnv, &pCopy->ppwszEnv);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyStringList(pInfo->ppwszDependencies, &pCopy->ppwszDependencies);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyString(pInfo->pDefaultLogTarget, &pCopy->pDefaultLogTarget);
    BAIL_ON_ERROR(dwError);

    *ppCopy = pCopy;

cleanup:

    return dwError;

error:

    if (pCopy)
    {
        LwSmCommonFreeServiceInfo(pCopy);
    }

    goto cleanup;
}

VOID
LwSmCommonFreeServiceInfo(
    PLW_SERVICE_INFO pInfo
    )
{
    if (pInfo)
    {
        if (pInfo->pwszPath)
        {
            LwFreeMemory(pInfo->pwszPath);
        }

        if (pInfo->pwszName)
        {
            LwFreeMemory(pInfo->pwszName);
        }

        if (pInfo->pwszDescription)
        {
            LwFreeMemory(pInfo->pwszDescription);
        }
        
        if (pInfo->pwszGroup)
        {
            LwFreeMemory(pInfo->pwszGroup);
        }

        if (pInfo->pDefaultLogTarget)
        {
            LwFreeMemory(pInfo->pDefaultLogTarget);
        }

        LwSmFreeStringList(pInfo->ppwszArgs);
        LwSmFreeStringList(pInfo->ppwszEnv);
        LwSmFreeStringList(pInfo->ppwszDependencies);
        LwFreeMemory(pInfo);
    }

    return;
};
