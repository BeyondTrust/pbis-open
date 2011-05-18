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
 *        lsastatus.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Status
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

VOID
LsaFreeStatus(
    PLSASTATUS pLsaStatus
    )
{
    DWORD iCount = 0;

    for (iCount = 0; iCount < pLsaStatus->dwCount; iCount++)
    {
       PLSA_AUTH_PROVIDER_STATUS pStatus = 
                 &pLsaStatus->pAuthProviderStatusList[iCount];

       LW_SAFE_FREE_STRING(pStatus->pszId);
       LW_SAFE_FREE_STRING(pStatus->pszDomain);
       LW_SAFE_FREE_STRING(pStatus->pszDomainSid);
       LW_SAFE_FREE_STRING(pStatus->pszForest);
       LW_SAFE_FREE_STRING(pStatus->pszSite);
       LW_SAFE_FREE_STRING(pStatus->pszCell);
       
       if (pStatus->pTrustedDomainInfoArray)
       {
           LsaFreeDomainInfoArray(
                           pStatus->dwNumTrustedDomains,
                           pStatus->pTrustedDomainInfoArray);
       }
    }

    LW_SAFE_FREE_MEMORY(pLsaStatus->pAuthProviderStatusList);

    LwFreeMemory(pLsaStatus);
}

DWORD
LsaReadVersionFile(
    PLSA_VERSION pVersion
    )
{  
    DWORD dwError = 0;
    DWORD dwMajor = 0;
    DWORD dwMinor = 0;
    DWORD dwBuild = 0;
    DWORD dwRevision = 0;
    int versionFile = -1;
    // A typical version file is 40 bytes long. The whole file can be read into
    // a static buffer, because if the file is too long, then it is invalid.
    char szFileBuffer[200];
    ssize_t dwCount = 0;
    // Do not free
    PSTR pszPos = szFileBuffer;

#ifdef MINIMAL_LSASS
    versionFile = open(LOCALSTATEDIR "/VERSION", O_RDONLY, 0);
#else
    versionFile = open(PREFIXDIR "/data/ENTERPRISE_VERSION", O_RDONLY, 0);
    if (versionFile < 0 && errno == ENOENT)
    {
        versionFile = open(PREFIXDIR "/data/VERSION", O_RDONLY, 0);
    }
#endif
    if (versionFile < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwCount = read(versionFile, szFileBuffer, sizeof(szFileBuffer));
    if (dwCount < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwCount == sizeof(szFileBuffer))
    {
        dwError = LW_ERROR_INVALID_AGENT_VERSION;
        BAIL_ON_LSA_ERROR(dwError);
    }
    szFileBuffer[dwCount] = 0;

    while (*pszPos)
    {
        LwStripWhitespace(pszPos, TRUE, TRUE);
        if (!strncmp(pszPos, "VERSION=", sizeof("VERSION=") - 1))
        {
            pszPos += sizeof("VERSION=") - 1;

            errno = 0;
            dwMajor = strtoul(pszPos, &pszPos, 10);

            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);

            if (pszPos[0] != '.')
            {
                dwError = LW_ERROR_INVALID_AGENT_VERSION;
                BAIL_ON_LSA_ERROR(dwError);
            }
            pszPos++;
            dwMinor = strtoul(pszPos, &pszPos, 10);

            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!strncmp(pszPos, "BUILD=", sizeof("BUILD=") - 1))
        {
            pszPos += sizeof("BUILD=") - 1;

            errno = 0;
            dwBuild = strtoul(pszPos, &pszPos, 10);
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!strncmp(pszPos, "REVISION=", sizeof("REVISION=") - 1))
        {
            pszPos += sizeof("REVISION=") - 1;

            errno = 0;
            dwRevision = strtoul(pszPos, &pszPos, 10);
            dwError = LwMapErrnoToLwError(errno);
            if (dwError != 0)
            {
                LSA_LOG_DEBUG("Unable to parse revision due to error %u", dwError);
                dwRevision = 0;
                dwError = 0;
            }
        }
        pszPos = strchr(pszPos, '\n');
        if (!pszPos)
        {
            break;
        }
        // Skip the \n
        pszPos++;
        if (*pszPos == '\r')
        {
            pszPos++;
        }
    }
    
    pVersion->dwMajor = dwMajor;
    pVersion->dwMinor = dwMinor;
    pVersion->dwBuild = dwBuild;
    pVersion->dwRevision = dwRevision;
    
cleanup:
    if (versionFile != -1)
    {
        close(versionFile);
    }
    return dwError;
    
error:
    memset(pVersion, 0, sizeof(*pVersion));
    goto cleanup;
}
