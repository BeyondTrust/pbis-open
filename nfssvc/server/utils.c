/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 * Server Service Utilities
 *
 */

#include "includes.h"


DWORD
NfsSvcNfsAllocateWC16StringFromUnicodeStringEx(
    OUT PWSTR            *ppwszOut,
    IN  UnicodeStringEx  *pIn
    )
{
    WINERR dwError = 0;
    PWSTR pwszStr = NULL;

    BAIL_ON_INVALID_PTR(ppwszOut, dwError);
    BAIL_ON_INVALID_PTR(pIn, dwError);

    dwError = NfsSvcNfsAllocateMemory(pIn->size * sizeof(WCHAR),
                                      OUT_PPVOID(&pwszStr));
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwWc16snCpy(pwszStr,
                          pIn->string,
                          pIn->len / sizeof(WCHAR));
    BAIL_ON_NFSSVC_ERROR(dwError);

    *ppwszOut = pwszStr;

cleanup:
    return dwError;

error:
    if (pwszStr)
    {
        NfsSvcNfsFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
    goto cleanup;
}


DWORD
NfsSvcNfsAllocateWC16String(
    OUT PWSTR  *ppwszOut,
    IN  PCWSTR  pwszIn
    )
{
    DWORD dwError = ERROR_SUCCESS;
    size_t sStrLen = 0;
    PWSTR pwszStr = NULL;

    BAIL_ON_INVALID_PTR(ppwszOut, dwError);
    BAIL_ON_INVALID_PTR(pwszIn, dwError);

    dwError = LwWc16sLen(pwszIn, &sStrLen);
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = NfsSvcNfsAllocateMemory(sizeof(WCHAR) * (sStrLen + 1),
                                      OUT_PPVOID(&pwszStr));
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwWc16snCpy(pwszStr, pwszIn, sStrLen);
    BAIL_ON_NFSSVC_ERROR(dwError);

    *ppwszOut = pwszStr;

cleanup:
    return dwError;

error:
    if (pwszStr)
    {
        NfsSvcNfsFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;

    goto cleanup;
}


DWORD
NfsSvcNfsAllocateWC16StringFromCString(
    OUT PWSTR  *ppwszOut,
    IN  PCSTR   pszIn
    )
{
    DWORD dwError = ERROR_SUCCESS;
    size_t sStrLen = 0;
    PWSTR pwszIn = NULL;
    PWSTR pwszStr = NULL;

    BAIL_ON_INVALID_PTR(ppwszOut, dwError);
    BAIL_ON_INVALID_PTR(pszIn, dwError);

    dwError = LwMbsToWc16s(pszIn, &pwszIn);
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwWc16sLen(pwszIn, &sStrLen);
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = NfsSvcNfsAllocateMemory(sizeof(WCHAR) * (sStrLen + 1),
                                      OUT_PPVOID(&pwszStr));
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwWc16snCpy(pwszStr, pwszIn, sStrLen);
    BAIL_ON_NFSSVC_ERROR(dwError);

    *ppwszOut = pwszStr;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszIn);

    return dwError;

error:
    if (pwszStr)
    {
        NfsSvcNfsFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
