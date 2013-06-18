/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        provider-main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        AD LDAP User Marshalling
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "adprovider.h"

DWORD
ADGetCurrentNtTime(
    OUT UINT64* pqwResult
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    time_t now = 0;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADConvertTimeUnix2Nt(now, pqwResult);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    *pqwResult = 0;
    goto cleanup;
}

DWORD
ADConvertTimeNt2Unix(
    UINT64 ntTime,
    PUINT64 pUnixTime
        )
{
    UINT64 unixTime = 0;

    unixTime = ntTime/10000000LL - 11644473600LL;

    *pUnixTime = unixTime;

    return 0;
}

DWORD
ADConvertTimeUnix2Nt(
    UINT64 unixTime,
    PUINT64 pNtTime
        )
{
    UINT64 ntTime = 0;

    ntTime = (unixTime+11644473600LL)*10000000LL;

    *pNtTime = ntTime;

    return 0;
}

DWORD
ADNonSchemaKeywordGetString(
    PSTR *ppszValues,
    DWORD dwNumValues,
    PCSTR pszAttributeName,
    PSTR *ppszResult
    )
{
    DWORD dwError = 0;
    size_t i;
    size_t sNameLen = strlen(pszAttributeName);
    PSTR pszResult = NULL;

    for (i = 0; i < dwNumValues; i++)
    {
        PCSTR pszValue = ppszValues[i];

        // Look for ldap values which are in the form <attributename>=<value>
        if (!strncasecmp(pszValue, pszAttributeName, sNameLen) &&
                pszValue[sNameLen] == '=')
        {
            dwError = LwAllocateString(
                        pszValue + sNameLen + 1,
                        &pszResult);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
    }

    *ppszResult = pszResult;

cleanup:
    return dwError;

error:
    *ppszResult = NULL;
    LW_SAFE_FREE_STRING(pszResult);

    goto cleanup;
}

DWORD
ADNonSchemaKeywordGetUInt32(
    PSTR *ppszValues,
    DWORD dwNumValues,
    PCSTR pszAttributeName,
    DWORD *pdwResult
    )
{
    size_t i;
    size_t sNameLen = strlen(pszAttributeName);

    for (i = 0; i < dwNumValues; i++)
    {
        PCSTR pszValue = ppszValues[i];
        // Don't free this
        PSTR pszEndPtr = NULL;

        // Look for ldap values which are in the form <attributename>=<value>
        if (!strncasecmp(pszValue, pszAttributeName, sNameLen) &&
                pszValue[sNameLen] == '=')
        {
            pszValue += sNameLen + 1;
            *pdwResult = strtoul(pszValue, &pszEndPtr, 10);
            if (pszEndPtr == NULL || *pszEndPtr != '\0' || pszEndPtr == pszValue)
            {
                // Couldn't parse the whole number
                return LW_ERROR_INVALID_LDAP_ATTR_VALUE;
            }
            return LW_ERROR_SUCCESS;
        }
    }

    // Couldn't find the attribute
    return LW_ERROR_INVALID_LDAP_ATTR_VALUE;
}

DWORD
AD_BuildHomeDirFromTemplate(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszHomedirTemplate,
    PCSTR pszNetBIOSDomainName,
    PCSTR pszSamAccountName,
    PSTR* ppszHomedir
    )
{
    DWORD dwError = 0;
    PSTR pszHomedirPrefix = NULL;
    PSTR pszHomedir = NULL;
    DWORD dwOffset = 0;
    PCSTR pszIterTemplate = pszHomedirTemplate;
    DWORD dwBytesAllocated = 0;
    DWORD dwNetBIOSDomainNameLength = 0;
    DWORD dwSamAccountNameLength = 0;
    DWORD dwHomedirPrefixLength = 0;
    PSTR pszHostName = NULL;
    DWORD dwHostNameLength = 0;
    CHAR szEscapedPercent[2] = "%";
    DWORD dwEscapedPercentLength = 0;

    BAIL_ON_INVALID_STRING(pszHomedirTemplate);
    BAIL_ON_INVALID_STRING(pszNetBIOSDomainName);
    BAIL_ON_INVALID_STRING(pszSamAccountName);

    if (strstr(pszHomedirTemplate, "%H"))
    {
        dwError = AD_GetHomedirPrefixPath(pState, &pszHomedirPrefix);
        BAIL_ON_LSA_ERROR(dwError);

        BAIL_ON_INVALID_STRING(pszHomedirPrefix);

        dwHomedirPrefixLength = strlen(pszHomedirPrefix);
    }

    if (strstr(pszHomedirTemplate, "%L"))
    {
        dwError = LsaDnsGetHostInfo(&pszHostName);
        BAIL_ON_LSA_ERROR(dwError);

        BAIL_ON_INVALID_STRING(pszHostName);

        dwHostNameLength = strlen(pszHostName);
    }

    if (strstr(pszHomedirTemplate, "%%"))
    {
        dwEscapedPercentLength = strlen(szEscapedPercent);
    }

    dwNetBIOSDomainNameLength = strlen(pszNetBIOSDomainName);
    dwSamAccountNameLength = strlen(pszSamAccountName);

    // Handle common case where we might use all replacements.
    dwBytesAllocated = (strlen(pszHomedirTemplate) +
                        dwNetBIOSDomainNameLength +
                        dwSamAccountNameLength +
                        dwHomedirPrefixLength +
                        dwHostNameLength +
                        dwEscapedPercentLength +
                        1);

    dwError = LwAllocateMemory(
                    sizeof(CHAR) * dwBytesAllocated,
                    (PVOID*)&pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    while (pszIterTemplate[0])
    {
        // Do not count the terminating NULL as "available".
        DWORD dwBytesRemaining = dwBytesAllocated - dwOffset - 1;
        PCSTR pszInsert = NULL;
        DWORD dwInsertLength = 0;
        BOOLEAN bNeedUpper = FALSE;
        BOOLEAN bNeedLower = FALSE;

        LSA_ASSERT(dwOffset < dwBytesAllocated);

        if (pszIterTemplate[0] == '%')
        {
            switch (pszIterTemplate[1])
            {
                case 'D':
                    pszInsert = pszNetBIOSDomainName;
                    dwInsertLength = dwNetBIOSDomainNameLength;
                    bNeedUpper = TRUE;
                    break;
                case 'U':
                    pszInsert = pszSamAccountName;
                    dwInsertLength = dwSamAccountNameLength;
                    bNeedLower = TRUE;
                    break;
                case 'H':
                    pszInsert = pszHomedirPrefix;
                    dwInsertLength = dwHomedirPrefixLength;
                    break;
                case 'L':
                    pszInsert = pszHostName;
                    dwInsertLength = dwHostNameLength;
                    break;
                case '%':
                    pszInsert = szEscapedPercent;
                    dwInsertLength = dwEscapedPercentLength;
                    break;
                default:
                    dwError = LW_ERROR_INVALID_HOMEDIR_TEMPLATE;
                    BAIL_ON_LSA_ERROR(dwError);
            }
            LSA_ASSERT(!(bNeedUpper && bNeedLower));
            pszIterTemplate += 2;
        }
        else
        {
            PCSTR pszEnd = strchr(pszIterTemplate, '%');
            if (!pszEnd)
            {
                dwInsertLength = strlen(pszIterTemplate);
            }
            else
            {
                dwInsertLength = pszEnd - pszIterTemplate;
            }
            pszInsert = pszIterTemplate;
            pszIterTemplate += dwInsertLength;
        }

        if (dwBytesRemaining < dwInsertLength)
        {
            // We will increment by at least a minimum amount.
            DWORD dwAllocate = LSA_MAX(dwInsertLength - dwBytesRemaining, 64);
            PSTR pszNewHomedir = NULL;
            dwError = LwReallocMemory(
                            pszHomedir,
                            (PVOID*)&pszNewHomedir,
                            dwBytesAllocated + dwAllocate);
            BAIL_ON_LSA_ERROR(dwError);
            pszHomedir = pszNewHomedir;
            dwBytesAllocated += dwAllocate;
        }
        memcpy(pszHomedir + dwOffset,
               pszInsert,
               dwInsertLength);
        if (bNeedUpper)
        {
            LwStrnToUpper(pszHomedir + dwOffset, dwInsertLength);
        }
        else if (bNeedLower)
        {
            LwStrnToLower(pszHomedir + dwOffset, dwInsertLength);
        }
        dwOffset += dwInsertLength;
    }
    
    // We should still have enough room for NULL.
    LSA_ASSERT(dwOffset < dwBytesAllocated);

    pszHomedir[dwOffset] = 0;
    dwOffset++;

    *ppszHomedir = pszHomedir;

cleanup:
    LW_SAFE_FREE_STRING(pszHomedirPrefix);
    LW_SAFE_FREE_STRING(pszHostName);

    return dwError;

error:
    *ppszHomedir = NULL;
    LW_SAFE_FREE_MEMORY(pszHomedir);

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
