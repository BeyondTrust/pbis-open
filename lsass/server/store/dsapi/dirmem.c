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
 *        dirmem.c
 *
 * Abstract:
 *
 *        Likewise Directory API - Memory Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

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

