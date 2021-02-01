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
 *        dirmem.c
 *
 * Abstract:
 *
 *        BeyondTrust Directory API - Memory Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"
#include <sqlite3.h>


DWORD
DirectoryAllocateMemory(
    size_t sSize,
    PVOID* ppMemory
    )
{
    DWORD dwError = 0;
    PVOID pMemory = NULL;

    pMemory = malloc(sSize);
    if (!pMemory)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        memset(pMemory,0, sSize);
    }

    *ppMemory = pMemory;

    return dwError;
}

DWORD
DirectoryReallocMemory(
    PVOID  pMemory,
    PVOID* ppNewMemory,
    size_t sSize
    )
{
    DWORD dwError = 0;
    PVOID pNewMemory = NULL;

    if (pMemory == NULL)
    {
       pNewMemory = malloc(sSize);
       memset(pNewMemory, 0, sSize);
    }
    else
    {
       pNewMemory = realloc(pMemory, sSize);
    }

    if (!pNewMemory)
    {
       dwError = LW_ERROR_OUT_OF_MEMORY;
       *ppNewMemory = NULL;
    }
    else
    {
       *ppNewMemory = pNewMemory;
    }

    return dwError;
}

VOID
DirectoryFreeMemory(
    PVOID pMemory
    )
{
    free(pMemory);
}

DWORD
DirectoryAllocateString(
    PCSTR pszInputString,
    PSTR* ppszOutputString
    )
{
    DWORD  dwError = 0;
    size_t sByteLen = 0;
    PSTR   pszOutputString = NULL;

    if (!pszInputString)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    sByteLen = strlen(pszInputString);

    dwError = DirectoryAllocateMemory(
                sByteLen + 1,
                (PVOID *)&pszOutputString);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    if (sByteLen)
    {
       memcpy(pszOutputString, pszInputString, sByteLen);
    }

    *ppszOutputString = pszOutputString;

cleanup:

    return dwError;

error:

    if (pszOutputString)
    {
        DirectoryFreeString(pszOutputString);
    }

    *ppszOutputString = NULL;

    goto cleanup;
}

DWORD
DirectoryAllocateStringW(
    PWSTR  pwszInputString,
    PWSTR* ppwszOutputString
    )
{
    DWORD  dwError = 0;
    size_t sByteLen = 0;
    PWSTR  pwszOutputString = NULL;

    if (!pwszInputString)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    sByteLen = wc16slen(pwszInputString) * sizeof(wchar16_t);

    dwError = DirectoryAllocateMemory(
                sByteLen + sizeof(wchar16_t),
                (PVOID *)&pwszOutputString);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    if (sByteLen)
    {
       memcpy((PBYTE)pwszOutputString, (PBYTE)pwszInputString, sByteLen);
    }

    *ppwszOutputString = pwszOutputString;

cleanup:

    return dwError;

error:

    if (pwszOutputString)
    {
        DirectoryFreeStringW(pwszOutputString);
    }

    *ppwszOutputString = NULL;

    goto cleanup;
}

VOID
DirectoryFreeStringW(
    PWSTR pwszString
    )
{
    DirectoryFreeMemory(pwszString);
}

VOID
DirectoryFreeString(
    PSTR pszString
    )
{
    DirectoryFreeMemory(pszString);
}

VOID
DirectoryFreeStringArray(
    PWSTR* ppStringArray,
    DWORD  dwCount
    )
{
    if ( ppStringArray )
    {
        DWORD i;

        for(i = 0; i < dwCount; i++)
        {
            if (ppStringArray[i])
            {
                DirectoryFreeStringW(ppStringArray[i]);
            }
        }

        DirectoryFreeMemory(ppStringArray);
    }
}


DWORD
DirectoryAllocateWC16StringFilterPrintf(
    OUT PWSTR* pOutput,
    IN PCSTR Format,
    ...
    )
{
    DWORD err = ERROR_SUCCESS;
    va_list args;
    PSTR formattedString = NULL;
    PWSTR output = NULL;

    va_start(args, Format);
    formattedString = sqlite3_vmprintf(Format, args);
    va_end(args);

    if (formattedString == NULL)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        BAIL_ON_LSA_ERROR(err);
    }

    err = LwMbsToWc16s(formattedString, &output);
    BAIL_ON_LSA_ERROR(err);

error:
    if (err)
    {
        LW_SAFE_FREE_MEMORY(output);
    }

    if (formattedString)
    {
        sqlite3_free(formattedString);
    }

    *pOutput = output;

    return err;
}


