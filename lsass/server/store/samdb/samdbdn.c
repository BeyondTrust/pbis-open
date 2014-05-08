/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samdbdn.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      Distinguished Name handling
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"

static
DWORD
SamDbGetDnToken(
    PWSTR            pwszObjectNameCursor,
    DWORD            dwAvailableLen,
    PSAMDB_DN_TOKEN* ppDnToken,
    PDWORD           pdwLenUsed
    );

static
PSAMDB_DN_TOKEN
SamDbReverseTokenList(
    PSAMDB_DN_TOKEN pTokenList
    );

DWORD
SamDbParseDN(
    PWSTR       pwszObjectDN,
    PSAM_DB_DN* ppDN
    )
{
    DWORD dwError = 0;
    PWSTR pwszObjectNameCursor = NULL;
    DWORD dwAvailableLen = 0;
    PSAM_DB_DN pDN = NULL;

    if (!pwszObjectDN || !*pwszObjectDN)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = DirectoryAllocateMemory(
                    sizeof(SAM_DB_DN),
                    (PVOID*)&pDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = DirectoryAllocateStringW(
                    pwszObjectDN,
                    &pDN->pwszDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwAvailableLen = wc16slen(pwszObjectDN);
    pwszObjectNameCursor = pDN->pwszDN;

    do
    {
        PSAMDB_DN_TOKEN pToken = NULL;
        DWORD dwLenUsed;

        dwError = SamDbGetDnToken(
                        pwszObjectNameCursor,
                        dwAvailableLen,
                        &pToken,
                        &dwLenUsed);
        BAIL_ON_SAMDB_ERROR(dwError);

        pToken->pNext = pDN->pTokenList;
        pDN->pTokenList = pToken;

        pwszObjectNameCursor += dwLenUsed;
        dwAvailableLen -= dwLenUsed;

    } while (dwAvailableLen);

    pDN->pTokenList = SamDbReverseTokenList(pDN->pTokenList);

    *ppDN = pDN;

cleanup:

    return dwError;

error:

    *ppDN = NULL;

    if (pDN)
    {
        SamDbFreeDN(pDN);
    }

    goto cleanup;
}

DWORD
SamDbGetDNComponents(
    PSAM_DB_DN pDN,
    PWSTR*     ppwszObjectName,
    PWSTR*     ppwszDomainName,
    PWSTR*     ppwszParentDN
    )
{
    DWORD dwError = 0;
    wchar16_t wszDot[] = { '.', 0};
    PSAMDB_DN_TOKEN pToken = pDN->pTokenList;
    PSAMDB_DN_TOKEN pDCToken = NULL;
    PSAMDB_DN_TOKEN pParentDNToken = NULL;
    DWORD dwObjectNameLen = 0;
    DWORD dwDomainNameLen = 0;
    PWSTR pwszObjectName = NULL;
    PWSTR pwszParentDN = NULL;
    PWSTR pwszDomainName = NULL;

    if (!pDN || !pDN->pTokenList)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    if (pToken->tokenType != SAMDB_DN_TOKEN_TYPE_DC)
    {
        dwObjectNameLen = pToken->dwLen * sizeof(wchar16_t);

        pToken = pToken->pNext;

        pParentDNToken = pToken;
    }

    while (pToken && (pToken->tokenType != SAMDB_DN_TOKEN_TYPE_DC))
    {
        pToken = pToken->pNext;
    }
    pDCToken = pToken;

    while (pToken)
    {
        if (dwDomainNameLen)
        {
            dwDomainNameLen += sizeof(wszDot[0]);
        }
        dwDomainNameLen += pToken->dwLen * sizeof(wchar16_t);

        if (pToken->tokenType != SAMDB_DN_TOKEN_TYPE_DC)
        {
            dwError = LW_ERROR_INVALID_LDAP_DN;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

	pToken = pToken->pNext;
    }

    if (dwObjectNameLen)
    {
        dwError = DirectoryAllocateMemory(
                        dwObjectNameLen + sizeof(wchar16_t),
                        (PVOID*)&pwszObjectName);
        BAIL_ON_SAMDB_ERROR(dwError);

        pToken = pDN->pTokenList;

        memcpy( (PBYTE)pwszObjectName,
                (PBYTE)pToken->pwszToken,
                pToken->dwLen * sizeof(wchar16_t));
    }

    if (pParentDNToken)
    {
        dwError = DirectoryAllocateStringW(
                    pParentDNToken->pwszDN,
                    &pwszParentDN);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    if (dwDomainNameLen)
    {
        PWSTR pwszCursor = NULL;

        dwError = DirectoryAllocateMemory(
                        dwDomainNameLen + sizeof(wchar16_t),
                        (PVOID*)&pwszDomainName);
        BAIL_ON_SAMDB_ERROR(dwError);

        pwszCursor = pwszDomainName;
        for (pToken = pDCToken;pToken; pToken = pToken->pNext)
        {
            if (pToken != pDCToken)
            {
                *pwszCursor = wszDot[0];

                pwszCursor++;
            }

            memcpy( (PBYTE)pwszCursor,
                    (PBYTE)pToken->pwszToken,
                    pToken->dwLen * sizeof(wchar16_t));

            pwszCursor += pToken->dwLen;
        }
    }

    *ppwszObjectName = pwszObjectName;
    *ppwszParentDN   = pwszParentDN;
    *ppwszDomainName = pwszDomainName;

cleanup:

    return dwError;

error:

    *ppwszObjectName = NULL;
    *ppwszParentDN   = NULL;
    *ppwszDomainName = NULL;

    DIRECTORY_FREE_MEMORY(pwszObjectName);
    DIRECTORY_FREE_MEMORY(pwszParentDN);
    DIRECTORY_FREE_MEMORY(pwszDomainName);

    goto cleanup;
}

VOID
SamDbFreeDN(
    PSAM_DB_DN pDN
    )
{
    while (pDN->pTokenList)
    {
        PSAMDB_DN_TOKEN pToken = pDN->pTokenList;

        pDN->pTokenList = pDN->pTokenList->pNext;

        DirectoryFreeMemory(pToken);
    }

    DIRECTORY_FREE_MEMORY(pDN->pwszDN);

    DirectoryFreeMemory(pDN);
}

static
DWORD
SamDbGetDnToken(
    PWSTR            pwszObjectNameCursor,
    DWORD            dwAvailableLen,
    PSAMDB_DN_TOKEN* ppDnToken,
    PDWORD           pdwLenUsed
    )
{
    DWORD  dwError = 0;
    wchar16_t wszCNPrefix[] = {'C', 'N', '=', 0};
    DWORD dwLenCNPrefix = (sizeof(wszCNPrefix)-sizeof(wchar16_t))/sizeof(wchar16_t);
    wchar16_t wszDCPrefix[] = {'D', 'C', '=', 0};
    DWORD dwLenDCPrefix = (sizeof(wszDCPrefix)-sizeof(wchar16_t))/sizeof(wchar16_t);
    wchar16_t wszOUPrefix[] = {'O', 'U', '=', 0};
    DWORD dwLenOUPrefix = (sizeof(wszOUPrefix)-sizeof(wchar16_t))/sizeof(wchar16_t);
    wchar16_t wszComma[] = {',', 0};
    DWORD dwLenUsed = 0;
    PSAMDB_DN_TOKEN pToken = NULL;

    dwError = DirectoryAllocateMemory(
                sizeof(SAMDB_DN_TOKEN),
                (PVOID*)&pToken);
    BAIL_ON_SAMDB_ERROR(dwError);

    if ((dwAvailableLen > dwLenCNPrefix) &&
        !memcmp((PBYTE)pwszObjectNameCursor,
                (PBYTE)&wszCNPrefix[0],
                dwLenCNPrefix * sizeof(wchar16_t)))
    {
        pToken->tokenType = SAMDB_DN_TOKEN_TYPE_CN;

        pToken->pwszDN = pwszObjectNameCursor;

        pwszObjectNameCursor += dwLenCNPrefix;
        dwAvailableLen -= dwLenCNPrefix;
        dwLenUsed += dwLenCNPrefix;
    }
    else
    if ((dwAvailableLen > dwLenOUPrefix) &&
        !memcmp((PBYTE)pwszObjectNameCursor,
                (PBYTE)&wszOUPrefix[0],
                dwLenOUPrefix * sizeof(wchar16_t)))
    {
        pToken->tokenType = SAMDB_DN_TOKEN_TYPE_OU;

        pToken->pwszDN = pwszObjectNameCursor;

        pwszObjectNameCursor += dwLenOUPrefix;
        dwAvailableLen -= dwLenOUPrefix;
        dwLenUsed += dwLenOUPrefix;
    }
    else
    if ((dwAvailableLen > dwLenDCPrefix) &&
        !memcmp((PBYTE)pwszObjectNameCursor,
                (PBYTE)&wszDCPrefix[0],
                dwLenDCPrefix * sizeof(wchar16_t)))
    {
        pToken->tokenType = SAMDB_DN_TOKEN_TYPE_DC;

        pToken->pwszDN = pwszObjectNameCursor;

        pwszObjectNameCursor += dwLenDCPrefix;
        dwAvailableLen -= dwLenDCPrefix;
        dwLenUsed += dwLenDCPrefix;
    }
    else
    {
        dwError = LW_ERROR_INVALID_LDAP_DN;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    if (!dwAvailableLen)
    {
        dwError = LW_ERROR_INVALID_LDAP_DN;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pToken->pwszToken = pwszObjectNameCursor;
    while (dwAvailableLen && (*pwszObjectNameCursor != wszComma[0]))
    {
        dwAvailableLen--;
        pToken->dwLen++;
        dwLenUsed++;
        pwszObjectNameCursor++;
    }

    if (dwAvailableLen && (*pwszObjectNameCursor == wszComma[0]))
    {
        dwLenUsed++;
    }

    *ppDnToken = pToken;
    *pdwLenUsed = dwLenUsed;

cleanup:

    return dwError;

error:

    *ppDnToken = NULL;
    *pdwLenUsed = 0;

    DIRECTORY_FREE_MEMORY(pToken);

    goto cleanup;
}

static
PSAMDB_DN_TOKEN
SamDbReverseTokenList(
    PSAMDB_DN_TOKEN pTokenList
    )
{
    PSAMDB_DN_TOKEN pP = NULL;
    PSAMDB_DN_TOKEN pQ = pTokenList;
    PSAMDB_DN_TOKEN pR = NULL;

    while( pQ ) {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }

    return pP;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
