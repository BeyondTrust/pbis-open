/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lsaldap.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        LDAP API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#include "includes.h"

DWORD
LsaLdapOpenDirectoryDomain(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszPrimaryDomain,
    IN DWORD dwFlags,
    OUT PHANDLE phDirectory
    )
{
    return LsaLdapOpenDirectoryWithReaffinity(pszDnsDomainName,
                                              pszPrimaryDomain,
                                              dwFlags,
                                              FALSE,
                                              phDirectory);
}

DWORD
LsaLdapOpenDirectoryGc(
    IN PCSTR pszDnsForestName,
    IN PCSTR pszPrimaryDomain,
    IN DWORD dwFlags,
    OUT PHANDLE phDirectory
    )
{
    return LsaLdapOpenDirectoryWithReaffinity(pszDnsForestName,
                                              pszPrimaryDomain,
                                              dwFlags,
                                              TRUE,
                                              phDirectory);
}


DWORD
LsaLdapOpenDirectoryWithReaffinity(
    IN PCSTR pszDnsDomainOrForestName,
    IN PCSTR pszPrimaryDomain,
    IN DWORD dwFlags,
    IN BOOLEAN bNeedGc,
    OUT PHANDLE phDirectory
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = 0;
#define MAX_SERVERS_TO_TRY 5
    PSTR ppszBlackList[MAX_SERVERS_TO_TRY] = {0};
    DWORD dwBlackListCount = 0;
    DWORD dwGetDcNameFlags = 0;
    PLWNET_DC_INFO pDCInfo = NULL;
    DWORD dwIndex = 0;

    if (dwFlags & LW_LDAP_OPT_GLOBAL_CATALOG)
    {
        LSA_LOG_DEBUG("Cannot specify GC option unless calling server API directly");
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (bNeedGc)
    {
        dwGetDcNameFlags |= DS_GC_SERVER_REQUIRED;
        dwFlags |= LW_LDAP_OPT_GLOBAL_CATALOG;
    }

    while (TRUE)
    {
        LWNET_SAFE_FREE_DC_INFO(pDCInfo);

        if (dwBlackListCount == 1)
        {
            // Try to update netlogon's affinity cache for all programs (not
            // just lsass). Netlogon will not update its cache if a blacklist
            // is passed in. So calling without the blacklist will trigger
            // netlogon to update its cache. Afterwards, the blacklist will be
            // passed in. If it matches what's in netlogon's cache, no network
            // queries will be issued.
            dwError = LWNetGetDCNameExt(
                            NULL,
                            pszDnsDomainOrForestName,
                            NULL,
                            bNeedGc ? pszPrimaryDomain : NULL,
                            dwGetDcNameFlags | DS_FORCE_REDISCOVERY,
                            0,
                            NULL,
                            &pDCInfo);
            LWNET_SAFE_FREE_DC_INFO(pDCInfo);
        }

        dwError = LWNetGetDCNameExt(
                        NULL,
                        pszDnsDomainOrForestName,
                        NULL,
                        bNeedGc ? pszPrimaryDomain : NULL,
                        dwGetDcNameFlags,
                        dwBlackListCount,
                        ppszBlackList,
                        &pDCInfo);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_LOG_DEBUG("Using DC '%s' for domain '%s' (affinitization attempt %u)",
                      pDCInfo->pszDomainControllerName,
                      pDCInfo->pszFullyQualifiedDomainName,
                      dwBlackListCount);

        dwError = LwLdapOpenDirectoryServer(
                        pDCInfo->pszDomainControllerAddress,
                        pDCInfo->pszDomainControllerName,
                        dwFlags,
                        &hDirectory);
        if (!dwError)
        {
            break;
        }

        LSA_LOG_DEBUG("Ldap open failed for %s '%s' (dwError = %u (symbol: %s))",
                      bNeedGc ? "forest" : "domain",
                      pszDnsDomainOrForestName,
                      dwError,
                      LwWin32ExtErrorToName(dwError));

        if ((dwBlackListCount >= MAX_SERVERS_TO_TRY) ||
            (dwError == SEC_E_NO_CREDENTIALS))
        {
            LSA_ASSERT(dwError);
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LwAllocateString(
                        pDCInfo->pszDomainControllerAddress,
                        &ppszBlackList[dwBlackListCount]);
        BAIL_ON_LSA_ERROR(dwError);
        dwBlackListCount++;
    }

    *phDirectory = hDirectory;

cleanup:
    for (dwIndex = 0; dwIndex < dwBlackListCount; dwIndex++)
    {
        LW_SAFE_FREE_STRING(ppszBlackList[dwIndex]);
    }
    LWNET_SAFE_FREE_DC_INFO(pDCInfo);
    return dwError;

error:
    LwLdapCloseDirectory(hDirectory);
    hDirectory = 0;
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
