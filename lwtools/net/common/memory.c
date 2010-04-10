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
 *        Likewise System NET Utilities
 *
 *        Memory Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#include "includes.h"

DWORD
LwNetAllocateMemory(
    size_t Size,
    LW_PVOID * ppMemory
    )
{
    return LwAllocateMemory(Size, ppMemory);
}

DWORD
LwNetReallocMemory(
    LW_PVOID pMemory,
    LW_PVOID * ppNewMemory,
    size_t Size
    )
{
    return LwReallocMemory(pMemory,
	                       ppNewMemory,
	                       Size);
}

VOID
LwNetFreeMemory(
    PVOID pMemory
    )
{
	LwFreeMemory(pMemory);
}

DWORD
LwNetAllocateString(
    PCSTR  pszInputString,
    PSTR* ppszOutputString
    )
{
    DWORD dwError = 0;
    DWORD dwLen = 0;
    PSTR  pszOutputString = NULL;

    if (!pszInputString) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LTNET_ERROR(dwError);
    }

    dwLen = strlen(pszInputString);

    dwError = LwNetAllocateMemory(dwLen+1, (PVOID *)&pszOutputString);
    BAIL_ON_LTNET_ERROR(dwError);

    if (dwLen) {
       memcpy(pszOutputString, pszInputString, dwLen);
    }

    *ppszOutputString = pszOutputString;

cleanup:

    return dwError;

error:

    LTNET_SAFE_FREE_STRING(pszOutputString);

    *ppszOutputString = NULL;

    goto cleanup;
}

VOID
LwNetFreeString(
    PSTR pszString
    )
{
    LwNetFreeMemory(pszString);
}

VOID
LwNetFreeStringArray(
    DWORD dwCount,
    PSTR * ppStringArray
    )
{
    DWORD i;

    if ( ppStringArray ) {
        for(i = 0; i < dwCount; i++)
        {
            if (ppStringArray[i]) {
                LwNetFreeString(ppStringArray[i]);
            }
        }

        LwNetFreeMemory(ppStringArray);
    }

    return;
}

VOID
LwNetFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    )
{
    PSTR* ppTmp = ppStringArray;

    while (ppTmp && *ppTmp) {

          LwNetFreeString(*ppTmp);

          ppTmp++;
    }

    LwNetFreeMemory(ppStringArray);
}

DWORD
LwNetWC16StringAllocateFromCString(
    OUT PWSTR* ppszNewString,
    IN PCSTR pszOriginalString
    )
{
    return LwNtStatusToWin32Error(
            LwRtlWC16StringAllocateFromCString(
                    ppszNewString,
                    pszOriginalString)
                    );
}

VOID
LwNetWC16StringFree(
    PWSTR pwszString
    )
{
    LTNET_SAFE_FREE_MEMORY(pwszString);
}

VOID
LwNetFreeWC16StringArray(
    DWORD dwCount,
    PWSTR* ppwszArray
    )
{
    DWORD dwIndex = 0;

    if (ppwszArray)
    {
        for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            LwNetWC16StringFree(ppwszArray[dwIndex]);
        }

        LTNET_SAFE_FREE_MEMORY(ppwszArray);
    }
}

DWORD
LwNetAllocateSidFromCString(
    OUT PSID* Sid,
    IN PCSTR StringSid
    )
{
    return LwNtStatusToWin32Error(
            RtlAllocateSidFromCString(
                Sid,
                StringSid
                )
                );
}
