/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lpmisc.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Miscellaneous
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
LocalBuildDN(
    PLSA_LOGIN_NAME_INFO pLoginInfo,
    PWSTR*               ppwszDN
    )
{
    DWORD  dwError = 0;
    CHAR   szCNPrefix[] = LOCAL_DIR_CN_PREFIX_ANSI;
    DWORD  dwLenCNPrefix = sizeof(LOCAL_DIR_CN_PREFIX_ANSI) - 1;
    CHAR   szDCPrefix[] = LOCAL_DIR_DC_PREFIX_ANSI;
    DWORD  dwLenDCPrefix = sizeof(LOCAL_DIR_DC_PREFIX_ANSI) - 1;
    CHAR   szDelimiter[] = LOCAL_DIR_DELIMITER_COMMA;
    DWORD  dwLenDelimiter = sizeof(LOCAL_DIR_DELIMITER_COMMA) - 1;
    DWORD  dwLenName = 0;
    PSTR   pszDN = NULL;
    PSTR   pszDNCursor = NULL;
    PWSTR  pwszDN = NULL;
    size_t sLenRequired = 0;

    BAIL_ON_INVALID_POINTER(pLoginInfo);
    BAIL_ON_INVALID_STRING(pLoginInfo->pszName);

    // Build something like CN=sam,DC=xyz,DC=corp,DC=com
    sLenRequired += dwLenCNPrefix;

    dwLenName = strlen (pLoginInfo->pszName);
    sLenRequired += dwLenName;

    if (!LW_IS_NULL_OR_EMPTY_STR(pLoginInfo->pszDomain))
    {
        PCSTR pszCursor = pLoginInfo->pszDomain;
        size_t sLenAvailable = strlen(pLoginInfo->pszDomain);
        size_t sLen2 = 0;

        do
        {
            sLen2 = strcspn(pszCursor, LOCAL_DIR_DELIMITER_DOT);

            sLenRequired += dwLenDelimiter;
            sLenRequired += dwLenDCPrefix;

            sLenRequired += sLen2;

            pszCursor += sLen2;
            sLenAvailable -= sLen2;

            sLen2 = strspn(pszCursor, LOCAL_DIR_DELIMITER_DOT);
            pszCursor += sLen2;
            sLenAvailable -= sLen2;

        } while (sLenAvailable > 0);
    }

    sLenRequired++;

    dwError = LwAllocateMemory(
                    sLenRequired,
                    (PVOID*)&pszDN);
    BAIL_ON_LSA_ERROR(dwError);

    pszDNCursor = pszDN;
    memcpy(pszDNCursor, &szCNPrefix[0], dwLenCNPrefix);
    pszDNCursor += dwLenCNPrefix;

    memcpy(pszDNCursor, pLoginInfo->pszName, dwLenName);
    pszDNCursor += dwLenName;

    if (!LW_IS_NULL_OR_EMPTY_STR(pLoginInfo->pszDomain))
    {
        PCSTR pszCursor = pLoginInfo->pszDomain;
        size_t sLenAvailable = strlen(pLoginInfo->pszDomain);
        size_t sLen2 = 0;

        do
        {
            sLen2 = strcspn(pszCursor, LOCAL_DIR_DELIMITER_DOT);

            *pszDNCursor++ = szDelimiter[0];

            memcpy(pszDNCursor, &szDCPrefix[0], dwLenDCPrefix);
            pszDNCursor += dwLenDCPrefix;

            memcpy(pszDNCursor, pszCursor, sLen2);
            pszDNCursor += sLen2;

            sLenAvailable -= sLen2;

            sLen2 = strspn(pszCursor, LOCAL_DIR_DELIMITER_DOT);
            pszCursor += sLen2;
            sLenAvailable -= sLen2;

        } while (sLenAvailable > 0);
    }

    dwError = LwMbsToWc16s(
                    pszDN,
                    &pwszDN);
    BAIL_ON_LSA_ERROR(dwError);

    *ppwszDN = pwszDN;

cleanup:

    LW_SAFE_FREE_MEMORY(pszDN);

    return dwError;

error:

    *ppwszDN = NULL;

    LW_SAFE_FREE_MEMORY(pwszDN);

    goto cleanup;
}

LONG64
LocalGetNTTime(
    time_t timeVal
    )
{
    return (timeVal + 11644473600LL) * 10000000LL;
}

DWORD
LocalBuildHomeDirPathFromTemplate(
    PCSTR pszSamAccountName,
    PCSTR pszNetBIOSDomainName,
    PSTR* ppszHomedir
    )
{
    DWORD dwError = 0;
    PSTR pszHomedir = NULL;
    DWORD dwOffset = 0;
    PSTR  pszHomedirPrefix = NULL;
    PSTR  pszHomedirTemplate = NULL;
    PCSTR pszIterTemplate = NULL;
    DWORD dwBytesAllocated = 0;
    DWORD dwNetBIOSDomainNameLength = 0;
    DWORD dwSamAccountNameLength = 0;
    DWORD dwHomedirPrefixLength = 0;
    PSTR pszHostName = NULL;
    DWORD dwHostNameLength = 0;

    BAIL_ON_INVALID_STRING(pszNetBIOSDomainName);
    BAIL_ON_INVALID_STRING(pszSamAccountName);

    dwError = LocalCfgGetHomedirTemplate(&pszHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    pszIterTemplate = pszHomedirTemplate;

    if (strstr(pszHomedirTemplate, "%H"))
    {
        dwError = LocalCfgGetHomedirPrefix(&pszHomedirPrefix);
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

    dwNetBIOSDomainNameLength = strlen(pszNetBIOSDomainName);
    dwSamAccountNameLength = strlen(pszSamAccountName);

    // Handle common case where we might use all replacements.
    dwBytesAllocated = (strlen(pszHomedirTemplate) +
                        dwNetBIOSDomainNameLength +
                        dwSamAccountNameLength +
                        dwHomedirPrefixLength +
                        dwHostNameLength +
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

    LW_SAFE_FREE_STRING(pszHomedirTemplate);
    LW_SAFE_FREE_STRING(pszHomedirPrefix);
    LW_SAFE_FREE_STRING(pszHostName);

    return dwError;

error:

    *ppszHomedir = NULL;

    LW_SAFE_FREE_MEMORY(pszHomedir);

    goto cleanup;
}


BOOLEAN
LocalDirIsBuiltinAccount(
    PSID pDomainSid,
    PSID pAccountSid
    )
{
    BOOLEAN bBuiltin = FALSE;
    DWORD dwRid = 0;
    union {
        SID BuiltinSid;
        UCHAR Buffer[SID_MAX_SIZE];
    } sidBuffer = { .Buffer = { 0 } };
    ULONG ulSidSize = sizeof(sidBuffer);

    RtlCreateWellKnownSid(WinBuiltinDomainSid,
                          NULL,
                          &sidBuffer.BuiltinSid,
                          &ulSidSize);

    if (RtlIsPrefixSid(pDomainSid,
                       pAccountSid))
    {
        /*
         * We're only interested in subauthority immediately
         * following the domain prefix
         */
        dwRid = pAccountSid->SubAuthority[pDomainSid->SubAuthorityCount];
        bBuiltin = (dwRid <= DOMAIN_USER_RID_MAX);
    }
    else if (RtlIsPrefixSid(&sidBuffer.BuiltinSid,
                            pAccountSid))
    {
        /*
         * We're only interested in subauthority immediately
         * following the domain prefix
         */
        dwRid = pAccountSid->SubAuthority[sidBuffer.BuiltinSid.SubAuthorityCount];
        bBuiltin = (dwRid <= DOMAIN_USER_RID_MAX);
    }

    return bBuiltin;
}
