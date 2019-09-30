/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lsadns.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS) DNS Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Glenn Curtis (gcurtis@likewisesoftware.com)
 *          Danilo Alameida (dalmeida@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
LsaDnsGetHostInfo(
    PSTR* ppszHostname
    )
{
    DWORD dwError = 0;
    CHAR szBuffer[256];
    PSTR pszLocal = NULL;
    PSTR pszDot = NULL;
    int len = 0;
    PSTR pszHostname = NULL;

    if ( gethostname(szBuffer, sizeof(szBuffer)) != 0 )
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    len = strlen(szBuffer);
    if ( len > strlen(".local") )
    {
        pszLocal = &szBuffer[len - strlen(".local")];
        if ( !strcasecmp( pszLocal, ".local" ) )
        {
            pszLocal[0] = '\0';
        }
    }
    
    /* Test to see if the name is still dotted. If so we will chop it down to
       just the hostname field. */
    pszDot = strchr(szBuffer, '.');
    if ( pszDot )
    {
        pszDot[0] = '\0';
    }

    dwError = LwAllocateString(szBuffer, &pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    if ( ppszHostname ) {
        *ppszHostname = pszHostname;
        pszHostname = NULL;
    }
        
cleanup:

    LW_SAFE_FREE_STRING(pszHostname);

    return dwError;

error:
    
    if (ppszHostname)
        *ppszHostname = NULL;
    
    goto cleanup;
}


DWORD
LsaWc16sHash(
    PCWSTR  pwszStr,
    PDWORD  pdwResult
    )
{
    DWORD  dwError = ERROR_SUCCESS;
    size_t sLen = 0;
    DWORD  dwPos = 0;
    DWORD  dwResult = 0;
    PSTR   pszData = (PSTR)pwszStr;

    BAIL_ON_INVALID_POINTER(pwszStr);
    BAIL_ON_INVALID_POINTER(pdwResult);

    dwError = LwWc16sLen(pwszStr, &sLen);
    BAIL_ON_LSA_ERROR(dwError);

    sLen *= 2;

    for (dwPos = 0; dwPos < sLen ; dwPos++)
    {
        if (pszData[dwPos])
        {
            // rotate result to the left 3 bits with wrap around
            dwResult = (dwResult << 3) | (dwResult >> (sizeof(DWORD)*8 - 3));
            dwResult += pszData[dwPos];
        }
    }

    *pdwResult = dwResult;

cleanup:
    return dwError;

error:
    if (pdwResult)
    {
        *pdwResult = 0;
    }

    goto cleanup;
}


DWORD
LsaStrHash(
    PCSTR   pszStr,
    PDWORD  pdwResult
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszStr = NULL;

    BAIL_ON_INVALID_POINTER(pszStr);
    BAIL_ON_INVALID_POINTER(pdwResult);

    dwError = LwMbsToWc16s(pszStr, &pwszStr);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaWc16sHash(pwszStr, pdwResult);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszStr);

    return dwError;

error:
    if (pdwResult)
    {
        *pdwResult = 0;
    }

    goto cleanup;
}


DWORD
LsaHashToWc16s(
    DWORD   dwHash,
    PWSTR  *ppwszHashStr
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszValidChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    DWORD dwNumValid = strlen(pszValidChars);
    CHAR pszHashStr[16] = {0};
    DWORD dwHashLocal = dwHash;
    DWORD dwPos = 0;
    DWORD dwNewChar = 0;
    PWSTR pwszHashStr = NULL;

    BAIL_ON_INVALID_POINTER(ppwszHashStr);

    memset(pszHashStr, 0, sizeof(pszHashStr));

    while(dwHashLocal)
    {
        dwNewChar = dwHashLocal % dwNumValid;
        pszHashStr[dwPos++] = pszValidChars[dwNewChar];
        dwHashLocal /= dwNumValid;
    }

    dwError = LwMbsToWc16s(pszHashStr, &pwszHashStr);
    BAIL_ON_LSA_ERROR(dwError);

    *ppwszHashStr = pwszHashStr;

cleanup:
    return dwError;

error:
    if (*ppwszHashStr)
    {
        *ppwszHashStr = NULL;
    }

    goto cleanup;
}


DWORD
LsaHashToStr(
    DWORD   dwHash,
    PSTR   *ppszHashStr
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszHashStr = NULL;
    PSTR pszHashStr = NULL;

    BAIL_ON_INVALID_POINTER(ppszHashStr);

    dwError = LsaHashToWc16s(dwHash, &pwszHashStr);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszHashStr, &pszHashStr);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszHashStr = pszHashStr;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszHashStr);

    return dwError;

error:
    if (*ppszHashStr)
    {
        *ppszHashStr = NULL;
    }

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
