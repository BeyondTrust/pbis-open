/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        wkssvc_memeory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        WksSvc memory allocation manager
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


DWORD
WkssSrvAllocateMemory(
    PVOID  *ppOut,
    DWORD  dwSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    void *pOut = NULL;

    pOut = rpc_ss_allocate(dwSize);
    BAIL_ON_NO_MEMORY(pOut, dwError);

    memset(pOut, 0, dwSize);

    *ppOut = pOut;

cleanup:
    return dwError;

error:
    *ppOut = NULL;
    goto cleanup;
}


VOID
WkssSrvFreeMemory(
    PVOID pPtr
    )
{
    rpc_ss_free(pPtr);
}


DWORD
WkssSrvAllocateWC16StringFromUnicodeStringEx(
    OUT PWSTR            *ppwszOut,
    IN  UNICODE_STRING   *pIn
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszStr = NULL;

    BAIL_ON_INVALID_PTR(ppwszOut, dwError);
    BAIL_ON_INVALID_PTR(pIn, dwError);

    if (pIn->MaximumLength > 0 && pIn->Buffer == NULL)
    {
        goto cleanup;
    }

    dwError = WkssSrvAllocateMemory(OUT_PPVOID(&pwszStr),
                                    pIn->MaximumLength * sizeof(WCHAR));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16snCpy(pwszStr,
                          pIn->Buffer,
                          pIn->Length / sizeof(WCHAR));
    BAIL_ON_LSA_ERROR(dwError);

    *ppwszOut = pwszStr;

cleanup:
    return dwError;

error:
    if (pwszStr)
    {
        WkssSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
    goto cleanup;
}


DWORD
WkssSrvAllocateWC16StringFromCString(
    OUT PWSTR    *ppwszOut,
    IN  PSTR      pszIn
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszStr = NULL;
    size_t sLen = 0;

    BAIL_ON_INVALID_PTR(ppwszOut, dwError);
    BAIL_ON_INVALID_PTR(pszIn, dwError);

    sLen = strlen(pszIn);
    dwError = WkssSrvAllocateMemory(OUT_PPVOID(&pwszStr),
                                    sizeof(pwszStr[0]) * (sLen + 1));
    BAIL_ON_LSA_ERROR(dwError);

    mbstowc16s(pwszStr, pszIn, sLen);

    *ppwszOut = pwszStr;

cleanup:
    return dwError;

error:
    if (pwszStr)
    {
        WkssSrvFreeMemory(pwszStr);
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
