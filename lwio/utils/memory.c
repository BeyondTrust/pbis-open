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
 *        memory.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Memory Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

LW_NTSTATUS
LwIoAllocateMemory(
    size_t Size,
    LW_PVOID * ppMemory
    )
{
    return RTL_ALLOCATE(ppMemory, VOID, Size);
}

LW_NTSTATUS
LwIoAllocateBuffer(
    size_t     Size,
    LW_PVOID * ppMemory
    )
{
    return LW_RTL_ALLOCATE_NOCLEAR(ppMemory, VOID, Size);
}

VOID
LwIoFreeBuffer(
    LW_PVOID pMemory
    )
{
    LwRtlMemoryFree(pMemory);
}

LW_NTSTATUS
LwIoReallocMemory(
    LW_PVOID pMemory,
    size_t Size,
    LW_PVOID * ppNewMemory
    )
{
    PVOID pNewMemory = LwRtlMemoryRealloc(pMemory, Size);

    if (!pNewMemory)
    {
        *ppNewMemory = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        *ppNewMemory = pNewMemory;
        return STATUS_SUCCESS;
    }
}

VOID
LwIoFreeMemory(
    PVOID pMemory
    )
{
    LwRtlMemoryFree(pMemory);
}

DWORD
SMBAllocateString(
    PCSTR  pszInputString,
    PSTR* ppszOutputString
    )
{
    DWORD dwError = 0;
    DWORD dwLen = 0;
    PSTR  pszOutputString = NULL;

    if (!pszInputString) {
        dwError = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    dwLen = strlen(pszInputString);

    dwError = LwIoAllocateMemory(dwLen+1, (PVOID *)&pszOutputString);
    BAIL_ON_LWIO_ERROR(dwError);

    if (dwLen) {
       memcpy(pszOutputString, pszInputString, dwLen);
    }

    *ppszOutputString = pszOutputString;

cleanup:

    return dwError;

error:

    LWIO_SAFE_FREE_STRING(pszOutputString);

    *ppszOutputString = NULL;

    goto cleanup;
}

VOID
SMBFreeString(
    PSTR pszString
    )
{
    LwIoFreeMemory(pszString);
}

VOID
SMBFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    )
{
    DWORD i;

    if ( ppStringArray ) {
        for(i = 0; i < dwCount; i++)
        {
            if (ppStringArray[i]) {
                SMBFreeString(ppStringArray[i]);
            }
        }

        LwIoFreeMemory(ppStringArray);
    }

    return;
}

VOID
SMBFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    )
{
    PSTR* ppTmp = ppStringArray;

    while (ppTmp && *ppTmp) {

          SMBFreeString(*ppTmp);

          ppTmp++;
    }

    LwIoFreeMemory(ppStringArray);
}

