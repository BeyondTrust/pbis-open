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
    pCopy->uShutdownTimeout = pInfo->uShutdownTimeout;
    
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
