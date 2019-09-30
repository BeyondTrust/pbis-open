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
 *        memory.c
 *
 * Abstract:
 *
 *        BeyondTrust IO (LWIO)
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

