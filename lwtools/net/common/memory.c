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
 *        BeyondTrust System NET Utilities
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
