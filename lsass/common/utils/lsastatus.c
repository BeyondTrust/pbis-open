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
 *        lsastatus.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
