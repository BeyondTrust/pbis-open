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
 *        Active Directory Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "adprovider.h"

#define BAIL_ON_DCE_ERROR(dest, status)                   \
    if ((status) != 0) {                    \
        LSA_LOG_ERROR("DCE Error [Code:%d]", (status));   \
        (dest) = LW_ERROR_DCE_CALL_FAILED;               \
        goto error;                                       \
    }

static
DWORD
AD_CheckExpiredMemberships(
    IN size_t sCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMemberships,
    IN BOOLEAN bCheckNullParentSid,
    OUT PBOOLEAN pbHaveExpired,
    OUT PBOOLEAN pbIsComplete
    );

DWORD
AD_OnlineFindCellDN(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszComputerDN,
    IN PCSTR pszRootDN,
    OUT PSTR* ppszCellDN
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszParentDN = NULL;
    PSTR  pszCellDN = NULL;
    PSTR  pszTmpDN = NULL;

    dwError = LwLdapGetParentDN(pszComputerDN, &pszParentDN);
    BAIL_ON_LSA_ERROR(dwError);

    //
    // Note: We keep looking at all parents of the current DN
    //       until we find a cell or hit the top domain DN.
    for (;;)
    {
        dwError = ADGetCellInformation(pConn, pszParentDN, &pszCellDN);
        if (dwError == LW_ERROR_NO_SUCH_CELL)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);

        if (!LW_IS_NULL_OR_EMPTY_STR(pszCellDN))
            break;

        if (!strcasecmp(pszRootDN, pszParentDN))
            break;

        LW_SAFE_FREE_STRING(pszTmpDN);

        pszTmpDN = pszParentDN;
        pszParentDN = NULL;

        dwError = LwLdapGetParentDN(pszTmpDN, &pszParentDN);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszCellDN = pszCellDN;

cleanup:

    LW_SAFE_FREE_STRING(pszParentDN);
    LW_SAFE_FREE_STRING(pszTmpDN);
    return dwError;

error:

    LW_SAFE_FREE_STRING(pszCellDN);
    *ppszCellDN = NULL;
    goto cleanup;
}

static
DWORD
AD_OnlineInitializeDomainTrustsInfo(
    IN PSTR pszPrimaryDomainName
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pDomains = NULL;
    // Do not free pDomain
    PLSA_DM_ENUM_DOMAIN_INFO pDomain = NULL;
    const DLINKEDLIST* pPos = NULL;
    PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo = NULL;
    DWORD dwDomainInfoCount = 0;
    PSTR pszDomainSid = NULL;
    PSTR pszSid = NULL;

    dwError = ADState_GetDomainTrustList(
                gpLsaAdProviderState->hStateConnection,
                &pDomains);
    BAIL_ON_LSA_ERROR(dwError);

    pPos = pDomains;
    while (pPos != NULL)
    {
        pDomain = (PLSA_DM_ENUM_DOMAIN_INFO)pPos->pItem;

        if (!pDomain
            || !IsSetFlag(pDomain->Flags, LSA_DM_DOMAIN_FLAG_TRANSITIVE_1WAY_CHILD)
            || LsaDmIsDomainPresent(pDomain->pszDnsDomainName)
           )
        {
            pPos = pPos->pNext;
            continue;
        }

        dwError = LsaDmWrapNetLookupObjectSidByName(
                     pszPrimaryDomainName,
                     pDomain->pszNetbiosDomainName,
                     &pszSid,
                     NULL);
        if (LW_ERROR_NO_SUCH_OBJECT == dwError)
        {
            pPos = pPos->pNext;
            dwError = 0;
            continue;
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmAddTrustedDomain(
            pDomain->pszDnsDomainName,
            pDomain->pszNetbiosDomainName,
            pDomain->pSid,
            pDomain->pGuid,
            pDomain->pszTrusteeDnsDomainName,
            pDomain->dwTrustFlags,
            pDomain->dwTrustType,
            pDomain->dwTrustAttributes,
            pDomain->dwTrustDirection,
            pDomain->dwTrustMode,
            TRUE,
            pDomain->pszForestName,
            NULL);
        if (IsSetFlag(pDomain->dwTrustFlags, NETR_TRUST_FLAG_PRIMARY))
        {
            BAIL_ON_LSA_ERROR(dwError);
        }

        pPos = pPos->pNext;
    }

    dwError = LsaDmEnumDomainInfo(
                NULL,
                NULL,
                &ppDomainInfo,
                &dwDomainInfoCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_StoreDomainTrustList(
                gpLsaAdProviderState->hStateConnection,
                ppDomainInfo,
                dwDomainInfoCount);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pszDomainSid);
    LW_SAFE_FREE_STRING(pszSid);
    ADState_FreeEnumDomainInfoList(pDomains);
    LsaDmFreeEnumDomainInfoArray(ppDomainInfo);

    return dwError;

error:
    goto cleanup;
}

DWORD
AD_OnlineInitializeOperatingMode(
    OUT PAD_PROVIDER_DATA* ppProviderData,
    IN PCSTR pszDomain,
    IN PCSTR pszHostName
    )
{
    DWORD dwError = 0;
    PSTR  pszComputerDN = NULL;
    PSTR  pszCellDN = NULL;
    PSTR  pszRootDN = NULL;
    ADConfigurationMode adConfMode = UnknownMode;
    PAD_PROVIDER_DATA pProviderData = NULL;
    PSTR pszNetbiosDomainName = NULL;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;
    AD_CELL_SUPPORT adCellSupport = AD_CELL_SUPPORT_FULL;

    dwError = LwKrb5SetDefaultCachePath(LSASS_CACHE_PATH, NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pProviderData), (PVOID*)&pProviderData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmEngineDiscoverTrusts(pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmLdapOpenDc(
                    pszDomain,
                    &pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwLdapConvertDomainToDN(pszDomain, &pszRootDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADFindComputerDN(pConn, pszHostName, pszDomain,
                               &pszComputerDN);
    BAIL_ON_LSA_ERROR(dwError);

    adCellSupport = AD_GetCellSupport();
    switch (adCellSupport)
    {
        case AD_CELL_SUPPORT_UNPROVISIONED:
            LSA_LOG_INFO("Disabling cell support due to cell-support configuration setting");
            break;

        case AD_CELL_SUPPORT_DEFAULT_SCHEMA:
            LSA_LOG_INFO("Using default schema mode due to cell-support configuration setting");

            dwError = LwAllocateStringPrintf(&pszCellDN, "CN=$LikewiseIdentityCell,%s", pszRootDN);
            BAIL_ON_LSA_ERROR(dwError);

            adConfMode = SchemaMode;

            break;

        default:
            dwError = AD_OnlineFindCellDN(
                            pConn,
                            pszComputerDN,
                            pszRootDN,
                            &pszCellDN);
            BAIL_ON_LSA_ERROR(dwError);
    }

    if (LW_IS_NULL_OR_EMPTY_STR(pszCellDN))
    {
        pProviderData->dwDirectoryMode = UNPROVISIONED_MODE;
    }
    else
    {
        PSTR pszValue = pszCellDN + sizeof("CN=$LikewiseIdentityCell,") - 1;

        if (!strcasecmp(pszValue, pszRootDN))
        {
            pProviderData->dwDirectoryMode = DEFAULT_MODE;
            strcpy(pProviderData->cell.szCellDN, pszCellDN);
        }
        else {
            pProviderData->dwDirectoryMode = CELL_MODE;
            strcpy(pProviderData->cell.szCellDN, pszCellDN);
         }
    }

    dwError = ADGetDomainMaxPwdAge(pConn, pszDomain,
                                   &pProviderData->adMaxPwdAge);
    BAIL_ON_LSA_ERROR(dwError);

    if (UnknownMode == adConfMode)
    {
        switch (pProviderData->dwDirectoryMode)
        {
            case DEFAULT_MODE:
            case CELL_MODE:
                dwError = ADGetConfigurationMode(pConn, pszCellDN,
                                                 &adConfMode);
                BAIL_ON_LSA_ERROR(dwError);
                break;
        }
    }

    dwError = LsaDmWrapGetDomainName(pszDomain, NULL, &pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    strcpy(pProviderData->szDomain, pszDomain);
    strcpy(pProviderData->szComputerDN, pszComputerDN);
    strcpy(pProviderData->szShortDomain, pszNetbiosDomainName);

    pProviderData->adConfigurationMode = adConfMode;

    if (pProviderData->dwDirectoryMode == CELL_MODE)
    {
        dwError = AD_GetLinkedCellInfo(pConn,
                    pszCellDN,
                    pszDomain,
                    &pProviderData->pCellList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADState_StoreProviderData(
                gpLsaAdProviderState->hStateConnection,
                pProviderData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_OnlineInitializeDomainTrustsInfo(
                pProviderData->szDomain);
    BAIL_ON_LSA_ERROR(dwError);

    *ppProviderData = pProviderData;

cleanup:
    LW_SAFE_FREE_STRING(pszNetbiosDomainName);
    LW_SAFE_FREE_STRING(pszRootDN);
    LW_SAFE_FREE_STRING(pszComputerDN);
    LW_SAFE_FREE_STRING(pszCellDN);
    LsaDmLdapClose(pConn);

    return dwError;

error:
    *ppProviderData = NULL;

    if (pProviderData)
    {
        ADProviderFreeProviderData(pProviderData);
        pProviderData = NULL;
    }

    goto cleanup;
}

DWORD
AD_GetLinkedCellInfo(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszCellDN,
    IN PCSTR pszDomain,
    OUT PDLINKEDLIST* ppCellList
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LDAP *pLd = NULL;
    LDAPMessage *pCellMessage1 = NULL;
    LDAPMessage *pCellMessage2 = NULL;
    DWORD dwCount = 0;
    PSTR szAttributeList[] =
                    {AD_LDAP_DESCRIP_TAG,
                     NULL
                    };
    PSTR szAttributeListCellName[] =
                    {AD_LDAP_NAME_TAG,
                     NULL
                    };
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    DWORD iValue = 0;
    PSTR  pszLinkedCell = NULL;
    PSTR  pszLinkedCellGuid = NULL;
    PSTR  pszDirectoryRoot = NULL;
    CHAR  szQuery[1024];
    BOOLEAN bValidADEntry = FALSE;
    PSTR pszStrTokSav = NULL;
    PCSTR pszDelim = ";";
    HANDLE hGCDirectory = (HANDLE)NULL;
    PLSA_DM_LDAP_CONNECTION pGcConn = NULL;
    LDAP* pGCLd = NULL;
    PDLINKEDLIST pCellList = NULL;
    HANDLE hDirectory = NULL;

    dwError = LsaDmLdapDirectorySearch(
                      pConn,
                      pszCellDN,
                      LDAP_SCOPE_BASE,
                      "(objectClass=*)",
                      szAttributeList,
                      &hDirectory,
                      &pCellMessage1);
    BAIL_ON_LSA_ERROR(dwError);

    pLd = LwLdapGetSession(hDirectory);

    dwCount = ldap_count_entries(
                      pLd,
                      pCellMessage1);
    if (dwCount < 0) {
       dwError = LW_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
       dwError = LW_ERROR_NO_SUCH_CELL;
    } else if (dwCount > 1) {
       dwError = LW_ERROR_DUPLICATE_CELLNAME;
    }
    BAIL_ON_LSA_ERROR(dwError);

    //Confirm the entry we obtain from AD is valid by retrieving its DN
    dwError = LwLdapIsValidADEntry(
                    hDirectory,
                    pCellMessage1,
                    &bValidADEntry);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bValidADEntry){
        dwError = LW_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwLdapGetStrings(
                     hDirectory,
                     pCellMessage1,
                     AD_LDAP_DESCRIP_TAG,
                     &ppszValues,
                     &dwNumValues);
    BAIL_ON_LSA_ERROR(dwError);

    for (iValue = 0; iValue < dwNumValues; iValue++)
    {
        if (!strncasecmp(ppszValues[iValue], "linkedCells=", sizeof("linkedCells=")-1))
        {
            pszLinkedCell = ppszValues[iValue] + sizeof("linkedCells=") - 1;
           break;
        }
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pszLinkedCell)){
        dwError = LwLdapConvertDomainToDN(
                        pszDomain,
                        &pszDirectoryRoot);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmLdapOpenGc(pszDomain, &pGcConn);
        BAIL_ON_LSA_ERROR(dwError);

        pszLinkedCellGuid = strtok_r (pszLinkedCell, pszDelim, &pszStrTokSav);
        while (pszLinkedCellGuid != NULL)
        {
            PSTR  pszHexStr = NULL;
            PAD_LINKED_CELL_INFO pLinkedCellInfo = NULL;
            PSTR  pszCellDirectoryRoot = NULL;
            PSTR  pszLinkedCellDN = NULL;
            PSTR  pszCellDN = NULL;

            dwError = LwAllocateMemory(
                    sizeof(AD_LINKED_CELL_INFO),
                    (PVOID*)&pLinkedCellInfo);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = ADGuidStrToHex(
                            pszLinkedCellGuid,
                            &pszHexStr);
            BAIL_ON_LSA_ERROR(dwError);

            sprintf(szQuery, "(objectGuid=%s)", pszHexStr);
            LW_SAFE_FREE_STRING(pszHexStr);

            //Search in root node's GC for cell DN given cell's GUID
            dwError = LsaDmLdapDirectorySearch(
                              pGcConn,
                              pszDirectoryRoot,
                              LDAP_SCOPE_SUBTREE,
                              szQuery,
                              szAttributeListCellName,
                              &hGCDirectory,
                              &pCellMessage2);
            BAIL_ON_LSA_ERROR(dwError);

            pGCLd = LwLdapGetSession(hGCDirectory);

            dwCount = ldap_count_entries(
                              pGCLd,
                              pCellMessage2);
            if (dwCount < 0) {
               dwError = LW_ERROR_LDAP_ERROR;
            } else if (dwCount == 0) {
               dwError = LW_ERROR_NO_SUCH_CELL;
            } else if (dwCount > 1) {
               dwError = LW_ERROR_DUPLICATE_CELLNAME;
            }
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwLdapGetDN(
                            hGCDirectory,
                            pCellMessage2,
                            &pszLinkedCellDN);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateStringPrintf(
                             &pLinkedCellInfo->pszCellDN,
                             "CN=$LikewiseIdentityCell,%s",
                             pszLinkedCellDN);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwLdapConvertDNToDomain(
                             pszLinkedCellDN,
                             &pLinkedCellInfo->pszDomain);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwLdapConvertDomainToDN(
                            pLinkedCellInfo->pszDomain,
                            &pszCellDirectoryRoot);
            BAIL_ON_LSA_ERROR(dwError);

            pszCellDN = pLinkedCellInfo->pszCellDN + sizeof("CN=$LikewiseIdentityCell,") - 1;
            //if pszLinkedCellDN is equal to pLinkedCellInfo->pszDomain, it is a default cell, hence a forest cell
            if (!strcasecmp(pszCellDN,
                            pszCellDirectoryRoot)){
                pLinkedCellInfo->bIsForestCell = TRUE;
            }
            else{
                pLinkedCellInfo->bIsForestCell = FALSE;
            }

            dwError = LsaDLinkedListAppend(&pCellList, pLinkedCellInfo);
            BAIL_ON_LSA_ERROR(dwError);

            pszLinkedCellGuid = strtok_r (NULL, pszDelim, &pszStrTokSav);

            LW_SAFE_FREE_STRING (pszCellDirectoryRoot);
            LW_SAFE_FREE_STRING (pszLinkedCellDN);

            if (pCellMessage2){
                ldap_msgfree(pCellMessage2);
                pCellMessage2 =  NULL;
            }
        }
    }

    *ppCellList = pCellList;

cleanup:

    if (pCellMessage1) {
        ldap_msgfree(pCellMessage1);
    }

    if (pCellMessage2){
        ldap_msgfree(pCellMessage2);
    }

    if (ppszValues) {
        LwFreeStringArray(ppszValues, dwNumValues);
    }

    LsaDmLdapClose(pGcConn);

    LW_SAFE_FREE_STRING (pszDirectoryRoot);

    return dwError;

error:
    *ppCellList = NULL;

    if (pCellList)
    {
        ADProviderFreeCellList(pCellList);
    }

    goto cleanup;
}

DWORD
AD_DetermineTrustModeandDomainName(
    IN PCSTR pszDomain,
    OUT OPTIONAL LSA_TRUST_DIRECTION* pdwTrustDirection,
    OUT OPTIONAL LSA_TRUST_MODE* pdwTrustMode,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName
    )
{
    DWORD dwError = 0;
    PSTR pszDnsDomainName = NULL;
    PSTR pszNetbiosDomainName = NULL;
    DWORD dwTrustFlags = 0;
    DWORD dwTrustType = 0;
    DWORD dwTrustAttributes = 0;
    LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
    LSA_TRUST_MODE dwTrustMode = LSA_TRUST_MODE_UNKNOWN;

    if (LW_IS_NULL_OR_EMPTY_STR(pszDomain) ||
        LW_IS_NULL_OR_EMPTY_STR(gpADProviderData->szDomain) ||
        LW_IS_NULL_OR_EMPTY_STR(gpADProviderData->szShortDomain))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

#if 0
    if (!strcasecmp(gpADProviderData->szDomain, pszDomain) ||
        !strcasecmp(gpADProviderData->szShortDomain, pszDomain))
    {
        dwTrustDirection = LSA_TRUST_DIRECTION_SELF;
        if (ppszDnsDomainName)
        {
            dwError = LwAllocateString(gpADProviderData->szDomain,
                                        &pszDnsDomainName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        if (ppszNetbiosDomainName)
        {
            dwError = LwAllocateString(gpADProviderData->szShortDomain,
                                        &pszNetbiosDomainName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        dwError = 0;
        goto cleanup;
    }
#endif

    dwError = LsaDmQueryDomainInfo(pszDomain,
                                   ppszDnsDomainName ? &pszDnsDomainName : NULL,
                                   ppszNetbiosDomainName ? &pszNetbiosDomainName : NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &dwTrustFlags,
                                   &dwTrustType,
                                   &dwTrustAttributes,
                                   &dwTrustDirection,
                                   &dwTrustMode,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);
    if (LW_ERROR_NO_SUCH_DOMAIN == dwError)
    {
        LSA_LOG_WARNING("Domain '%s' is unknown.", pszDomain);
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pdwTrustDirection)
    {
        *pdwTrustDirection = dwTrustDirection;
    }
    if (pdwTrustMode)
    {
        *pdwTrustMode = dwTrustMode;
    }

    if (ppszDnsDomainName)
    {
        *ppszDnsDomainName = pszDnsDomainName;
    }
    if (ppszNetbiosDomainName)
    {
        *ppszNetbiosDomainName = pszNetbiosDomainName;
    }

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszDnsDomainName);
    LW_SAFE_FREE_STRING(pszNetbiosDomainName);
    goto cleanup;
}

static
DWORD
AD_PacRidsToSidStringList(
    IN OPTIONAL PSID pDomainSid,
    IN PRID_WITH_ATTRIBUTE_ARRAY pRids,
    OUT PDWORD pdwSidCount,
    OUT PSTR** pppszSidList
    )
{
    DWORD dwError = 0;
    PSID pDomainBasedSid = NULL;
    DWORD i = 0;
    DWORD dwSidCount = 0;
    PSTR* ppszSidList = NULL;

    if (!pDomainSid)
    {
        if (pRids->dwCount != 0)
        {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }
        // No SIDs here, so return empty list.
        dwError = 0;
        goto error;
    }

    dwSidCount = pRids->dwCount;

    dwError = LwAllocateMemory(sizeof(ppszSidList[0]) * dwSidCount,
                                (PVOID*)&ppszSidList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateSidAppendRid(
                    &pDomainBasedSid,
                    pDomainSid,
                    0);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < pRids->dwCount; i++)
    {
        pDomainBasedSid->SubAuthority[pDomainBasedSid->SubAuthorityCount - 1] =
            pRids->pRids[i].dwRid;

        dwError = LsaAllocateCStringFromSid(
                        &ppszSidList[i],
                        pDomainBasedSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pdwSidCount = dwSidCount;
    *pppszSidList = ppszSidList;

cleanup:
    LW_SAFE_FREE_MEMORY(pDomainBasedSid);
    return dwError;

error:
    *pdwSidCount = 0;
    *pppszSidList = NULL;

    LwFreeStringArray(ppszSidList, dwSidCount);
    goto cleanup;
}

static
DWORD
AD_PacAttributedSidsToSidStringList(
    IN DWORD dwSidCount,
    IN NetrSidAttr* pAttributedSids,
    OUT PDWORD pdwSidCount,
    OUT PSTR** pppszSidList,
    OUT PDWORD* ppdwSidAttributeList
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR* ppszSidList = NULL;
    PDWORD pdwSidAttributeList = NULL;

    dwError = LwAllocateMemory(sizeof(ppszSidList[0]) * dwSidCount,
                                (PVOID*)&ppszSidList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(pdwSidAttributeList[0]) * dwSidCount,
                                (PVOID*)&pdwSidAttributeList);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwSidCount;  i++)
    {
        dwError = LsaAllocateCStringFromSid(
                        &ppszSidList[i],
                        pAttributedSids[i].sid);
        BAIL_ON_LSA_ERROR(dwError);

        pdwSidAttributeList[i] = pAttributedSids[i].attribute;
    }

    *pdwSidCount = dwSidCount;
    *pppszSidList = ppszSidList;
    *ppdwSidAttributeList = pdwSidAttributeList;

cleanup:
    return dwError;

error:
    *pdwSidCount = 0;
    *pppszSidList = NULL;
    *ppdwSidAttributeList = NULL;

    LwFreeStringArray(ppszSidList, dwSidCount);
    LW_SAFE_FREE_MEMORY(pdwSidAttributeList);
    goto cleanup;
}

static
DWORD
AD_SidStringListsFromPac(
    IN PAC_LOGON_INFO* pPac,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSidList,
    OUT PDWORD pdwResourceSidCount,
    OUT PSTR** pppszResourceSidList,
    OUT PDWORD pdwExtraSidCount,
    OUT PSTR** pppszExtraSidList,
    OUT PDWORD* ppdwExtraSidAttributeList
    )
{
    DWORD dwError = 0;
    DWORD dwGroupSidCount = 0;
    PSTR* ppszGroupSidList = NULL;
    DWORD dwResourceGroupSidCount = 0;
    PSTR* ppszResourceGroupSidList = NULL;
    DWORD dwExtraSidCount = 0;
    PSTR* ppszExtraSidList = NULL;
    PDWORD pdwExtraSidAttributeList = NULL;

    // PAC group membership SIDs

    dwError = AD_PacRidsToSidStringList(
                    pPac->info3.base.domain_sid,
                    &pPac->info3.base.groups,
                    &dwGroupSidCount,
                    &ppszGroupSidList);
    BAIL_ON_LSA_ERROR(dwError);

    // PAC resource domain group membership SIDs

    dwError = AD_PacRidsToSidStringList(
                    pPac->res_group_dom_sid,
                    &pPac->res_groups,
                    &dwResourceGroupSidCount,
                    &ppszResourceGroupSidList);
    BAIL_ON_LSA_ERROR(dwError);

    // PAC extra SIDs

    dwError = AD_PacAttributedSidsToSidStringList(
                    pPac->info3.sidcount,
                    pPac->info3.sids,
                    &dwExtraSidCount,
                    &ppszExtraSidList,
                    &pdwExtraSidAttributeList);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (dwError)
    {
        LwFreeStringArray(ppszGroupSidList, dwGroupSidCount);
        dwGroupSidCount = 0;
        ppszGroupSidList = NULL;
        LwFreeStringArray(ppszResourceGroupSidList, dwResourceGroupSidCount);
        dwResourceGroupSidCount = 0;
        ppszResourceGroupSidList = NULL;
        LwFreeStringArray(ppszExtraSidList, dwExtraSidCount);
        dwExtraSidCount = 0;
        ppszExtraSidList = NULL;
        LW_SAFE_FREE_MEMORY(pdwExtraSidAttributeList);
    }

    *pdwGroupSidCount = dwGroupSidCount;
    *pppszGroupSidList = ppszGroupSidList;
    *pdwResourceSidCount = dwResourceGroupSidCount;
    *pppszResourceSidList = ppszResourceGroupSidList;
    *pdwExtraSidCount = dwExtraSidCount;
    *pppszExtraSidList = ppszExtraSidList;
    *ppdwExtraSidAttributeList = pdwExtraSidAttributeList;

    return dwError;

error:
    // Handle error in cleanup for simplicity.
    goto cleanup;
}

static
DWORD
AD_PacMembershipFilterWithLdap(
    IN HANDLE hProvider,
    IN LSA_TRUST_DIRECTION dwTrustDirection,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    IN DWORD dwMembershipCount,
    IN OUT PLSA_GROUP_MEMBERSHIP* ppMemberships
    )
{
    DWORD dwError = 0;
    int iPrimaryGroupIndex = -1;
    size_t sLdapGroupCount = 0;
    PLSA_SECURITY_OBJECT* ppLdapGroups = NULL;
    LSA_HASH_TABLE* pMembershipHashTable = NULL;
    size_t i = 0;
    PLSA_GROUP_MEMBERSHIP* ppCacheMemberships = NULL;
    size_t sCacheMembershipCount = 0;
    size_t sCacheNonNullCount = 0;
    BOOLEAN bGroupsMatch = TRUE;
    BOOLEAN bExpired = FALSE;
    BOOLEAN bIsComplete = FALSE;

    if (LSA_TRUST_DIRECTION_ONE_WAY == dwTrustDirection)
    {
        goto cleanup;
    }

    dwError = LsaHashCreate(
                    dwMembershipCount,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pMembershipHashTable);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwMembershipCount; i++)
    {
        // Ignore the NULL entry
        if (ppMemberships[i]->pszParentSid)
        {
            dwError = LsaHashSetValue(pMembershipHashTable,
                                      ppMemberships[i]->pszParentSid,
                                      ppMemberships[i]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = ADCacheGetGroupsForUser(
                    gpLsaAdProviderState->hCacheConnection,
                    pUserInfo->pszObjectSid,
                    TRUE,
                    &sCacheMembershipCount,
                    &ppCacheMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_CheckExpiredMemberships(
                    sCacheMembershipCount,
                    ppCacheMemberships,
                    TRUE,
                    &bExpired,
                    &bIsComplete);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExpired || !bIsComplete)
    {
        bGroupsMatch = FALSE;
    }

    for (i = 0; bGroupsMatch && i < sCacheMembershipCount; i++)
    {
        PLSA_GROUP_MEMBERSHIP pMembership = NULL;

        // Ignore the NULL entry
        if (ppCacheMemberships[i]->pszParentSid)
        {
            sCacheNonNullCount++;
            dwError = LsaHashGetValue(pMembershipHashTable,
                                      ppCacheMemberships[i]->pszParentSid,
                                      (PVOID*)&pMembership);
            if (LW_ERROR_SUCCESS == dwError)
            {
                pMembership->bIsDomainPrimaryGroup =
                    ppCacheMemberships[i]->bIsDomainPrimaryGroup;
                pMembership->bIsInLdap = ppCacheMemberships[i]->bIsInLdap;
            }
            else if (dwError == ERROR_NOT_FOUND)
            {
                bGroupsMatch = FALSE;
                LSA_LOG_VERBOSE(
                        "The user group membership information for user %s\\%s does not match what is in the cache, because group '%s' is in the cache, but not in the pac. The group membership now needs to be compared against LDAP.",
                        LSA_SAFE_LOG_STRING(pUserInfo->pszNetbiosDomainName),
                        LSA_SAFE_LOG_STRING(pUserInfo->pszSamAccountName),
                        LSA_SAFE_LOG_STRING(ppCacheMemberships[i]->pszParentSid));
                dwError = LW_ERROR_SUCCESS;
            }
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    if (bGroupsMatch && sCacheNonNullCount != pMembershipHashTable->sCount)
    {
        LSA_LOG_VERBOSE(
                "The user group membership information for user %s\\%s does not match what is in the cache, because the cache contains %d memberships, but the pac contains %d memberships. The group membership now needs to be compared against LDAP.",
                LSA_SAFE_LOG_STRING(pUserInfo->pszNetbiosDomainName),
                LSA_SAFE_LOG_STRING(pUserInfo->pszSamAccountName),
                sCacheNonNullCount,
                pMembershipHashTable->sCount);
        bGroupsMatch = FALSE;
    }
    
    if (bGroupsMatch)
    {
        goto cleanup;
    }

    // Reset the membership flags obtained from the cache
    for (i = 0; i < dwMembershipCount; i++)
    {
        // Ignore the NULL entry
        if (ppMemberships[i]->pszParentSid)
        {
            ppMemberships[i]->bIsDomainPrimaryGroup = FALSE;
            ppMemberships[i]->bIsInLdap = FALSE;
        }
    }

    // Grab the membership information available in LDAP.
    dwError = ADLdap_GetObjectGroupMembership(
                    hProvider,
                    pUserInfo,
                    &iPrimaryGroupIndex,
                    &sLdapGroupCount,
                    &ppLdapGroups);
    BAIL_ON_LSA_ERROR(dwError);

    if (sLdapGroupCount < 1)
    {
        goto cleanup;
    }

    // For anything that we find via LDAP, make it expirable or primary.
    for (i = 0; i < sLdapGroupCount; i++)
    {
        PLSA_GROUP_MEMBERSHIP pMembership = NULL;

        dwError = LsaHashGetValue(pMembershipHashTable,
                                  ppLdapGroups[i]->pszObjectSid,
                                  (PVOID*)&pMembership);
        if (LW_ERROR_SUCCESS == dwError)
        {
            if ((DWORD)iPrimaryGroupIndex == i)
            {
                pMembership->bIsDomainPrimaryGroup = TRUE;
            }
            pMembership->bIsInLdap = TRUE;
        }
        else if (dwError == ERROR_NOT_FOUND)
        {
            dwError = LW_ERROR_SUCCESS;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    ADCacheSafeFreeGroupMembershipList(sCacheMembershipCount, &ppCacheMemberships);
    ADCacheSafeFreeObjectList(sLdapGroupCount, &ppLdapGroups);
    LsaHashSafeFree(&pMembershipHashTable);
    return dwError;

error:
    goto cleanup;
}

DWORD
AD_CacheGroupMembershipFromPac(
    IN HANDLE hProvider,
    IN LSA_TRUST_DIRECTION dwTrustDirection,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    IN PAC_LOGON_INFO* pPac
    )
{
    DWORD dwError = 0;
    time_t now = 0;
    DWORD dwGroupSidCount = 0;
    PSTR* ppszGroupSidList = NULL;
    DWORD dwResourceGroupSidCount = 0;
    PSTR* ppszResourceGroupSidList = NULL;
    DWORD dwExtraSidCount = 0;
    PSTR* ppszExtraSidList = NULL;
    PDWORD pdwExtraSidAttributeList = NULL;
    DWORD i = 0;
    DWORD dwIgnoreExtraSidCount = 0;
    DWORD dwMembershipCount = 0;
    PLSA_GROUP_MEMBERSHIP* ppMemberships = NULL;
    DWORD dwMembershipIndex = 0;
    struct {
        PDWORD pdwCount;
        PSTR** pppszSidList;
    } SidsToCombine[] = {
        { &dwGroupSidCount, &ppszGroupSidList },
        { &dwResourceGroupSidCount, &ppszResourceGroupSidList },
        { &dwExtraSidCount, &ppszExtraSidList }
    };
    DWORD dwSidsToCombineIndex = 0;
    PLSA_GROUP_MEMBERSHIP pMembershipBuffers = NULL;
    PSTR pszPrimaryGroupSid = NULL;

    LSA_LOG_VERBOSE(
            "Updating user group membership for uid %lu with PAC information",
             (unsigned long)pUserInfo->userInfo.uid);

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_SidStringListsFromPac(
                    pPac,
                    &dwGroupSidCount,
                    &ppszGroupSidList,
                    &dwResourceGroupSidCount,
                    &ppszResourceGroupSidList,
                    &dwExtraSidCount,
                    &ppszExtraSidList,
                    &pdwExtraSidAttributeList);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwGroupSidCount; i++)
    {
        LSA_LOG_VERBOSE("uid %lu's #%lu PAC group membership is %s",
                        (unsigned long)pUserInfo->userInfo.uid,
                        (unsigned long)i,
                        ppszGroupSidList[i]);
    }

    for (i = 0; i < dwResourceGroupSidCount; i++)
    {
        LSA_LOG_VERBOSE("uid %lu's #%lu PAC resource group membership is %s",
                        (unsigned long)pUserInfo->userInfo.uid,
                        (unsigned long)i,
                        ppszResourceGroupSidList[i]);
    }

    for (i = 0; i < dwExtraSidCount; i++)
    {
        LSA_LOG_VERBOSE("uid %lu's #%lu PAC extra membership is %s (attributes = 0x%08x)",
                        (unsigned long)pUserInfo->userInfo.uid,
                        (unsigned long)i,
                        ppszExtraSidList[i],
                        pdwExtraSidAttributeList[i]);

        // Filter out unwanted SIDs.

        // ISSUE-2008/11/03-dalmeida -- Revisit this piece.
        // Apparently, we still let user sids through here.
        // Perhaps we should not filterat all.

        // universal groups seem to have this set to 7
        // local groups seem to have this set to 0x20000007
        // we don't want to treat sids from the sid history like groups.
        if (pdwExtraSidAttributeList[i] != 7 &&
            pdwExtraSidAttributeList[i] != 0x20000007)
        {
            LSA_LOG_VERBOSE("Ignoring non-group SID %s (attribute is 0x%x)",
                            ppszExtraSidList[i],
                            pdwExtraSidAttributeList[i]);
            LW_SAFE_FREE_STRING(ppszExtraSidList[i]);
            dwIgnoreExtraSidCount++;
        }
    }

    // Allocate one extra for NULL entry
    dwMembershipCount = (dwGroupSidCount + dwResourceGroupSidCount +
                         dwExtraSidCount - dwIgnoreExtraSidCount + 1);

    dwError = LwAllocateMemory(
                    sizeof(ppMemberships[0]) * dwMembershipCount,
                    (PVOID*)&ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(pMembershipBuffers[0]) * dwMembershipCount,
                    (PVOID*)&pMembershipBuffers);
    BAIL_ON_LSA_ERROR(dwError);

    dwMembershipIndex = 0;
    for (dwSidsToCombineIndex = 0;
         dwSidsToCombineIndex < sizeof(SidsToCombine)/sizeof(SidsToCombine[0]);
         dwSidsToCombineIndex++)
    {
        DWORD dwSidCount = *SidsToCombine[dwSidsToCombineIndex].pdwCount;
        for (i = 0; i < dwSidCount; i++)
        {
            PLSA_GROUP_MEMBERSHIP* ppMembership = &ppMemberships[dwMembershipIndex];
            PLSA_GROUP_MEMBERSHIP pMembership = &pMembershipBuffers[dwMembershipIndex];
            PSTR* ppszSidList = *SidsToCombine[dwSidsToCombineIndex].pppszSidList;
            if (ppszSidList[i])
            {
                *ppMembership = pMembership;
                pMembership->version.qwDbId = -1;
                pMembership->pszParentSid = ppszSidList[i];
                pMembership->pszChildSid = pUserInfo->pszObjectSid;
                pMembership->bIsInPac = TRUE;
                dwMembershipIndex++;
            }
        }
    }

    assert((dwMembershipCount - 1) == dwMembershipIndex);

    // Set up NULL entry.
    ppMemberships[dwMembershipIndex] = &pMembershipBuffers[dwMembershipIndex];
    ppMemberships[dwMembershipIndex]->version.qwDbId = -1;
    ppMemberships[dwMembershipIndex]->pszChildSid = pUserInfo->pszObjectSid;

    if (AD_GetTrimUserMembershipEnabled())
    {
        dwError = AD_PacMembershipFilterWithLdap(
                        hProvider,
                        dwTrustDirection,
                        pUserInfo,
                        dwMembershipCount,
                        ppMemberships);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheStoreGroupsForUser(
                        gpLsaAdProviderState->hCacheConnection,
                        pUserInfo->pszObjectSid,
                        dwMembershipCount,
                        ppMemberships,
                        TRUE);
    BAIL_ON_LSA_ERROR(dwError);

    /* Create primary group sid from pac */
    dwError = LsaReplaceSidRid(
        pUserInfo->pszObjectSid,
        pPac->info3.base.primary_gid,
        &pszPrimaryGroupSid);
    BAIL_ON_LSA_ERROR(dwError);

    /* If we did not previously know the primary group sid, or
       it has changed, update the object and re-insert it into
       the cache */
    if (!pUserInfo->userInfo.pszPrimaryGroupSid ||
        strcmp(pszPrimaryGroupSid, pUserInfo->userInfo.pszPrimaryGroupSid))
    {
		LW_SAFE_FREE_STRING(pUserInfo->userInfo.pszPrimaryGroupSid);

        pUserInfo->userInfo.pszPrimaryGroupSid = pszPrimaryGroupSid;
        pszPrimaryGroupSid = NULL;
        
        dwError = ADCacheStoreObjectEntry(
            gpLsaAdProviderState->hCacheConnection,
            pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LW_SAFE_FREE_MEMORY(ppMemberships);
    LW_SAFE_FREE_MEMORY(pMembershipBuffers);
    LwFreeStringArray(ppszGroupSidList, dwGroupSidCount);
    LwFreeStringArray(ppszResourceGroupSidList, dwResourceGroupSidCount);
    LwFreeStringArray(ppszExtraSidList, dwExtraSidCount);
    LW_SAFE_FREE_MEMORY(pdwExtraSidAttributeList);
    LW_SAFE_FREE_STRING(pszPrimaryGroupSid);

    return dwError;

error:
    goto cleanup;
}

static
void
AD_MarshalUserAccountFlags(
    DWORD dwAcctFlags,
    IN OUT PLSA_SECURITY_OBJECT_USER_INFO pObjectUserInfo
    )
{
    pObjectUserInfo->bPasswordNeverExpires = IsSetFlag(dwAcctFlags, LSA_ACB_PWNOEXP);
    if (pObjectUserInfo->bPasswordNeverExpires)
    {
        pObjectUserInfo->bPasswordExpired = FALSE;
    }
    else
    {
        pObjectUserInfo->bPasswordExpired = IsSetFlag(dwAcctFlags, LSA_ACB_PW_EXPIRED);
    }
    pObjectUserInfo->bAccountDisabled = IsSetFlag(dwAcctFlags, LSA_ACB_DISABLED);
    //pObjectUserInfo->bAccountLocked = IsSetFlag(dwAcctFlags, LSA_ACB_AUTOLOCK);
}

DWORD
AD_CacheUserRealInfoFromPac(
    IN OUT PLSA_SECURITY_OBJECT pUserInfo,
    IN PAC_LOGON_INFO* pPac
    )
{
    DWORD dwError = 0;

    LSA_LOG_VERBOSE(
            "Updating user for uid %lu information from from one-way trusted domain with PAC information",
             (unsigned long)pUserInfo->userInfo.uid);

    pUserInfo->userInfo.qwPwdLastSet =
               (uint64_t)WinTimeToInt64(pPac->info3.base.last_password_change);
    pUserInfo->userInfo.qwPwdExpires =
               (uint64_t)WinTimeToInt64(pPac->info3.base.force_password_change);
    pUserInfo->userInfo.qwAccountExpires =
               (uint64_t)WinTimeToInt64(pPac->info3.base.acct_expiry);

    AD_MarshalUserAccountFlags(
               pPac->info3.base.acct_flags,
               &pUserInfo->userInfo);

    dwError = LsaAdBatchMarshalUserInfoAccountExpires(
               pUserInfo->userInfo.qwAccountExpires,
               &pUserInfo->userInfo,
               pUserInfo->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAdBatchMarshalUserInfoPasswordExpires(
               pUserInfo->userInfo.qwPwdExpires,
               &pUserInfo->userInfo,
               pUserInfo->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->userInfo.bIsAccountInfoKnown = TRUE;

    dwError = ADCacheStoreObjectEntry(
               gpLsaAdProviderState->hCacheConnection,
               pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
AD_GetCachedPasswordHash(
    IN PCSTR pszSamAccount,
    IN PCSTR pszPassword,
    OUT PBYTE *ppbHash
    )
{
    PWSTR pwszPassword = NULL;
    PBYTE pbPrehashedVerifier = NULL;
    size_t sPrehashedVerifierLen = 0;
    PBYTE pbHash = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sConvertedChars = 0;
    size_t sSamAccountCch = mbstrlen(pszSamAccount);

    // Allocate space to store the NT hash with the username appended
    sPrehashedVerifierLen = 16 + sSamAccountCch * sizeof(wchar16_t);
    dwError = LwAllocateMemory(
                    sPrehashedVerifierLen + sizeof(wchar16_t),
                    (PVOID*)&pbPrehashedVerifier);
    BAIL_ON_LSA_ERROR(dwError);

    // Compute the NT hash (which only depends on the password) and store
    // it in the first 16 bytes of pbPrehashedVerifier

    dwError = LwMbsToWc16s(
            pszPassword,
            &pwszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    MD4(
        (UCHAR *)pwszPassword,
        strlen(pszPassword) * sizeof(wchar16_t),
        pbPrehashedVerifier);

    // Append the username in UCS-2 encoding to the NT hash
    sConvertedChars = mbstowc16s(
            (wchar16_t *)(pbPrehashedVerifier + 16),
            pszSamAccount,
            sSamAccountCch + 1);
    if (sConvertedChars != sSamAccountCch)
    {
        dwError = LW_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Calculate the password verifier in binary format
    dwError = LwAllocateMemory(
                    16,
                    (PVOID*)&pbHash);
    BAIL_ON_LSA_ERROR(dwError);

    MD4(
        pbPrehashedVerifier,
        sPrehashedVerifierLen,
        pbHash);

    *ppbHash = pbHash;

cleanup:

    LW_SAFE_FREE_MEMORY(pwszPassword);
    LW_SAFE_FREE_MEMORY(pbPrehashedVerifier);

    return dwError;

error:

    *ppbHash = NULL;
    LW_SAFE_FREE_MEMORY(pbHash);

    goto cleanup;
}

static
DWORD
AD_OnlineCachePasswordVerifier(
    IN PLSA_SECURITY_OBJECT pUserInfo,
    IN PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    PLSA_PASSWORD_VERIFIER pVerifier = NULL;
    struct timeval current_tv;
    PBYTE pbHash = NULL;

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                    sizeof(*pVerifier),
                    (PVOID*)&pVerifier);
    BAIL_ON_LSA_ERROR(dwError);

    pVerifier->version.tLastUpdated = current_tv.tv_sec;
    pVerifier->version.qwDbId = -1;

    dwError = LwAllocateString(
                    pUserInfo->pszObjectSid,
                    &pVerifier->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_GetCachedPasswordHash(
                pUserInfo->pszSamAccountName,
                pszPassword,
                &pbHash);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaByteArrayToHexStr(
                pbHash,
                16,
                &pVerifier->pszPasswordVerifier);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheStorePasswordVerifier(
                gpLsaAdProviderState->hCacheConnection,
                pVerifier);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_DB_SAFE_FREE_PASSWORD_VERIFIER(pVerifier);
    LW_SAFE_FREE_MEMORY(pbHash);

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_ServicesDomainWithDiscovery(
    IN PCSTR pszNetBiosName,
    OUT PBOOLEAN pbFoundDomain
    )
{
    BOOLEAN bFoundDomain = FALSE;
    DWORD dwError = 0;

    bFoundDomain = AD_ServicesDomain(pszNetBiosName);

    if (!bFoundDomain)
    {
        dwError = LsaDmEngineGetDomainNameWithDiscovery(
                     pszNetBiosName,
                     NULL,
                     NULL);
        if (!dwError)
        {
            bFoundDomain = TRUE;
        }
        else if (LW_ERROR_NO_SUCH_DOMAIN == dwError)
        {
            dwError = 0;
            goto cleanup;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!bFoundDomain)
    {
        LSA_LOG_INFO("Unknown domain '%s'", pszNetBiosName);
    }

cleanup:
    *pbFoundDomain = bFoundDomain;

    return dwError;

error:
    goto cleanup;
}

DWORD
AD_OnlineCheckUserPassword(
    HANDLE hProvider,
    PLSA_SECURITY_OBJECT pUserInfo,
    PCSTR  pszPassword,
    PDWORD pdwGoodUntilTime
    )
{
    DWORD dwError = 0;
    PSTR pszHostname = NULL;
    PSTR pszMachineAccountName = NULL;
    PSTR pszServicePassword = NULL;
    PSTR pszDomainDnsName = NULL;
    PSTR pszHostDnsDomain = NULL;
    PSTR pszServicePrincipal = NULL;
    PSTR pszUpn = NULL;
    PSTR pszUserDnsDomainName = NULL;
    PSTR pszFreeUpn = NULL;
    PSTR pszUserRealm = NULL;
    char* pchNdrEncodedPac = NULL;
    size_t sNdrEncodedPac = 0;
    PAC_LOGON_INFO *pPac = NULL;
    LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
    NTSTATUS ntStatus = 0;

    dwError = AD_DetermineTrustModeandDomainName(
                        pUserInfo->pszNetbiosDomainName,
                        &dwTrustDirection,
                        NULL,
                        NULL,
                        NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToLower(pszHostname);

    dwError = LwKrb5GetMachineCreds(
                    &pszMachineAccountName,
                    &pszServicePassword,
                    &pszDomainDnsName,
                    &pszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    // Leave the realm empty so that kerberos referrals are turned on.
    dwError = LwAllocateStringPrintf(
                        &pszServicePrincipal,
                        "host/%s.%s@",
                        pszHostname,
                        pszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (pUserInfo->userInfo.bIsGeneratedUPN)
    {
        pszUpn = pUserInfo->userInfo.pszUPN;
    }
    else
    {
        BOOLEAN bIsGeneratedUpn = FALSE;

        LSA_LOG_DEBUG("Using generated UPN instead of '%s'", pUserInfo->userInfo.pszUPN);

        dwError = LsaDmEngineGetDomainNameAndSidByObjectSidWithDiscovery(
                       pUserInfo->pszObjectSid,
                       &pszUserDnsDomainName,
                       NULL,
                       NULL);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADGetLDAPUPNString(
                        0,
                        NULL,
                        pszUserDnsDomainName,
                        pUserInfo->pszSamAccountName,
                        &pszFreeUpn,
                        &bIsGeneratedUpn);
        BAIL_ON_LSA_ERROR(dwError);

        pszUpn = pszFreeUpn;
    }

    if ((pszUserRealm = strchr(pszUpn, '@')) == NULL)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    ++pszUserRealm;

    if (LsaDmIsDomainOffline(pszUserRealm))
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LwSetupUserLoginSession(
                    pUserInfo->userInfo.uid,
                    pUserInfo->userInfo.gid,
                    pszUpn,
                    pszPassword,
                    KRB5_File_Cache,
                    pszServicePrincipal,
                    pszDomainDnsName,
                    pszServicePassword,
                    &pchNdrEncodedPac,
                    &sNdrEncodedPac,
                    pdwGoodUntilTime);
    if (dwError == LW_ERROR_KRB5_S_PRINCIPAL_UNKNOWN)
    {
        LW_SAFE_FREE_STRING(pszServicePrincipal);

        // Perhaps the host has no SPN or UPN.  Try again
        // Using the sAMAccountName@REALM
        dwError = LwAllocateStringPrintf(
                      &pszServicePrincipal,
                      "%s@%s",
                      pszMachineAccountName,
                      pszDomainDnsName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwSetupUserLoginSession(
                      pUserInfo->userInfo.uid,
                      pUserInfo->userInfo.gid,
                      pszUpn,
                      pszPassword,
                      KRB5_File_Cache,
                      pszServicePrincipal,
                      pszDomainDnsName,
                      pszServicePassword,
                      &pchNdrEncodedPac,
                      &sNdrEncodedPac,
                      pdwGoodUntilTime);
    }
    
    if (dwError == LW_ERROR_DOMAIN_IS_OFFLINE)
    {
        LsaDmTransitionOffline(pszUserRealm, FALSE);
    }

    BAIL_ON_LSA_ERROR(dwError);

    if (sNdrEncodedPac)
    {
        // This function will abort if it is passed a zero sized PAC.
        ntStatus = DecodePacLogonInfo(
            pchNdrEncodedPac,
            sNdrEncodedPac,
            &pPac);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pPac != NULL)
    {
        dwError = AD_CacheGroupMembershipFromPac(
                        hProvider,
                        dwTrustDirection,
                        pUserInfo,
                        pPac);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_CacheUserRealInfoFromPac(
                        pUserInfo,
                        pPac);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_ASSERT(pUserInfo->userInfo.bIsAccountInfoKnown);
    }
    else
    {
        LSA_LOG_ERROR("no pac was received for %s\\%s (uid %d). The user's group memberships and password expiration may show up incorrectly on this machine.", 
                    LSA_SAFE_LOG_STRING(pUserInfo->pszNetbiosDomainName),
                    LSA_SAFE_LOG_STRING(pUserInfo->pszSamAccountName),
                    pUserInfo->userInfo.uid);
    }

cleanup:
    if (pPac)
    {
        FreePacLogonInfo(pPac);
    }
    LW_SAFE_FREE_STRING(pszHostname);
    LW_SAFE_FREE_STRING(pszMachineAccountName);
    LW_SAFE_FREE_STRING(pszServicePassword);
    LW_SAFE_FREE_STRING(pszDomainDnsName);
    LW_SAFE_FREE_STRING(pszHostDnsDomain);
    LW_SAFE_FREE_STRING(pszServicePrincipal);
    LW_SAFE_FREE_STRING(pszUserDnsDomainName);
    LW_SAFE_FREE_STRING(pszFreeUpn);
    LW_SAFE_FREE_MEMORY(pchNdrEncodedPac);

    return dwError;

error:
    *pdwGoodUntilTime = 0;

    goto cleanup;
}

DWORD
AD_OnlineAuthenticateUserPam(
    HANDLE hProvider,
    LSA_AUTH_USER_PAM_PARAMS* pParams,
    PLSA_AUTH_USER_PAM_INFO* ppPamAuthInfo
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PLSA_SECURITY_OBJECT pUserInfo = NULL;
    DWORD dwGoodUntilTime = 0;
    BOOLEAN bFoundDomain = FALSE;
    PSTR pszNT4UserName = NULL;
    PLSA_AUTH_USER_PAM_INFO pPamAuthInfo = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*pPamAuthInfo),
                    (PVOID*)&pPamAuthInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCrackDomainQualifiedName(
                    pParams->pszLoginName,
                    gpADProviderData->szDomain,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_ServicesDomainWithDiscovery(
                    pLoginInfo->pszDomainNetBiosName,
                    &bFoundDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bFoundDomain)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_FindUserObjectByName(
                    hProvider,
                    pParams->pszLoginName,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_OnlineCheckUserPassword(
                    hProvider,
                    pUserInfo,
                    pParams->pszPassword,
                    &dwGoodUntilTime);
    BAIL_ON_LSA_ERROR(dwError);

    ADCacheSafeFreeObject(&pUserInfo);

    dwError = AD_FindUserObjectByName(
                    hProvider,
                    pParams->pszLoginName,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_VerifyUserAccountCanLogin(
                pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_OnlineCachePasswordVerifier(
                    pUserInfo,
                    pParams->pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
        &pszNT4UserName,
        "%s\\%s",
        pUserInfo->pszNetbiosDomainName,
        pUserInfo->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUmAddUser(
                  pUserInfo->userInfo.uid,
                  pszNT4UserName,
                  pParams->pszPassword,
                  dwGoodUntilTime);
    BAIL_ON_LSA_ERROR(dwError);

    *ppPamAuthInfo = pPamAuthInfo;

cleanup:

    LW_SAFE_FREE_STRING(pszNT4UserName);

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    ADCacheSafeFreeObject(&pUserInfo);

    return dwError;

error:
    *ppPamAuthInfo = NULL;
    if (pPamAuthInfo)
    {
        LsaFreeAuthUserPamInfo(pPamAuthInfo);
    }

    goto cleanup;
}

DWORD
AD_CheckExpiredObject(
    IN OUT PLSA_SECURITY_OBJECT* ppCachedUser
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    time_t now = 0;
    time_t expirationDate;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    expirationDate = (*ppCachedUser)->version.tLastUpdated +
        AD_GetCacheEntryExpirySeconds();

    if (expirationDate <= now)
    {
        LSA_LOG_VERBOSE(
                "Cache entry for sid %s expired %ld seconds ago",
                (*ppCachedUser)->pszObjectSid,
                now - expirationDate);

        //Pretend like the object couldn't be found in the cache
        ADCacheSafeFreeObject(ppCachedUser);
        dwError = LW_ERROR_NOT_HANDLED;
    }
    else
    {
        LSA_LOG_VERBOSE(
                "Using cache entry for sid %s, updated %ld seconds ago",
                (*ppCachedUser)->pszObjectSid,
                now - (*ppCachedUser)->version.tLastUpdated);
    }

error:
    return dwError;
}

DWORD
AD_StoreAsExpiredObject(
    IN OUT PLSA_SECURITY_OBJECT* ppCachedUser
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    // Set the last update value so low that the next read will force a refresh.
    (*ppCachedUser)->version.tLastUpdated = 0;

    // Update the cache with the now stale item
    dwError = ADCacheStoreObjectEntry(
                    gpLsaAdProviderState->hCacheConnection,
                    *ppCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}

// Note: We only return whether complete if not expired.
static
DWORD
AD_CheckExpiredMemberships(
    IN size_t sCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMemberships,
    IN BOOLEAN bCheckNullParentSid,
    OUT PBOOLEAN pbHaveExpired,
    OUT PBOOLEAN pbIsComplete
    )
{
    DWORD dwError = 0;
    size_t sIndex = 0;
    time_t now = 0;
    DWORD dwCacheEntryExpirySeconds = 0;
    BOOLEAN bHaveExpired = FALSE;
    BOOLEAN bIsComplete = FALSE;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    //
    // Whenever a membership is cached, an extra "null" entry is added.
    // This entry has the opposite (parent or child) field set such
    // that we can tell whether we cached a user's groups (child set)
    // or a group's members (parent set).
    //
    // If the NULL entry is missing, this means that we got the data
    // because we cached something else (e.g., we cached user's groups
    // but are not trying to find a group's members).
    //
    dwCacheEntryExpirySeconds = AD_GetCacheEntryExpirySeconds();
    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        PLSA_GROUP_MEMBERSHIP pMembership = ppMemberships[sIndex];

        // Ignore what cannot expire (assumes that we already
        // filtered out PAC entries that should not be returned).
        if (pMembership->bIsInPac ||
            pMembership->bIsDomainPrimaryGroup)
        {
            continue;
        }

        if ((pMembership->version.tLastUpdated > 0) &&
            (pMembership->version.tLastUpdated + dwCacheEntryExpirySeconds <= now))
        {
            bHaveExpired = TRUE;
            // Note that we only return whether complete
            // if not expired.
            break;
        }

        // Check for NULL entry
        if (bCheckNullParentSid)
        {
            if (pMembership->pszParentSid == NULL)
            {
                bIsComplete = TRUE;
            }
        }
        else
        {
            if (pMembership->pszChildSid == NULL)
            {
                bIsComplete = TRUE;
            }
        }
    }

error:
    *pbHaveExpired = bHaveExpired;
    *pbIsComplete = bIsComplete;
    return dwError;
}

static
DWORD
AD_FilterExpiredMemberships(
    IN OUT size_t* psCount,
    IN OUT PLSA_GROUP_MEMBERSHIP* ppMemberships
    )
{
    DWORD dwError = 0;
    size_t sIndex = 0;
    time_t now = 0;
    DWORD dwCacheEntryExpirySeconds = 0;
    size_t sCount = *psCount;
    size_t sOutputCount = 0;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    // Cannot fail after this.
    dwCacheEntryExpirySeconds = AD_GetCacheEntryExpirySeconds();
    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        PLSA_GROUP_MEMBERSHIP pMembership = ppMemberships[sIndex];

        if (pMembership->bIsInPac ||
            pMembership->bIsDomainPrimaryGroup ||
            ((pMembership->version.tLastUpdated > 0) &&
             (pMembership->version.tLastUpdated + dwCacheEntryExpirySeconds <= now)))
        {
            // Keep
            if (sOutputCount != sIndex)
            {
                ppMemberships[sOutputCount] = ppMemberships[sIndex];
            }
            sOutputCount++;
        }
        else
        {
            ADCacheSafeFreeGroupMembership(&ppMemberships[sIndex]);
        }
    }

    *psCount = sOutputCount;

error:
    return dwError;
}

static
BOOLEAN
AD_IncludeOnlyUnexpirableGroupMembershipsCallback(
    IN PLSA_GROUP_MEMBERSHIP pMembership
    )
{
    BOOLEAN bInclude = FALSE;

    if (pMembership->bIsInPac || pMembership->bIsDomainPrimaryGroup)
    {
        bInclude = TRUE;
    }

    return bInclude;
}

static
DWORD
AD_CacheMembershipFromRelatedObjects(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN int iPrimaryGroupIndex,
    IN BOOLEAN bIsParent,
    IN size_t sCount,
    IN PLSA_SECURITY_OBJECT* ppRelatedObjects
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_MEMBERSHIP* ppMemberships = NULL;
    PLSA_GROUP_MEMBERSHIP pMembershipBuffers = NULL;
    size_t sMaxMemberships = 0;
    size_t sIndex = 0;
    size_t sMembershipCount = 0;
    PLSA_SECURITY_OBJECT pPrimaryGroup = NULL;

    if (iPrimaryGroupIndex >= 0)
    {
        pPrimaryGroup = ppRelatedObjects[iPrimaryGroupIndex];
    }

    // Generate a list of AD_GROUP_MEMBERSHIP objects.  Include a
    // NULL entry to indicate that the member list is authoritative
    // parent or child SID (depending on bIsParent).

    // Need an extra entry for the NULL entry that
    // signals a complete list.
    sMaxMemberships = sCount + 1;

    dwError = LwAllocateMemory(
                    sizeof(*ppMemberships) * sMaxMemberships,
                    (PVOID*)&ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(pMembershipBuffers[0]) * sMaxMemberships,
                    (PVOID*)&pMembershipBuffers);
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        PLSA_GROUP_MEMBERSHIP* ppMembership = &ppMemberships[sMembershipCount];
        PLSA_GROUP_MEMBERSHIP pMembership = &pMembershipBuffers[sMembershipCount];
        if (ppRelatedObjects[sIndex])
        {
            *ppMembership = pMembership;
            pMembership->version.qwDbId = -1;
            if (bIsParent)
            {
                pMembership->pszParentSid = (PSTR)pszSid;
                pMembership->pszChildSid = ppRelatedObjects[sIndex]->pszObjectSid;
            }
            else
            {
                pMembership->pszParentSid = ppRelatedObjects[sIndex]->pszObjectSid;
                pMembership->pszChildSid = (PSTR)pszSid;
                if (pPrimaryGroup == ppRelatedObjects[sIndex])
                {
                    pMembership->bIsDomainPrimaryGroup = TRUE;
                }
            }
            pMembership->bIsInLdap = TRUE;
            sMembershipCount++;
        }
    }

    // Set up NULL entry.
    ppMemberships[sMembershipCount] = &pMembershipBuffers[sMembershipCount];
    ppMemberships[sMembershipCount]->version.qwDbId = -1;
    if (bIsParent)
    {
        ppMemberships[sMembershipCount]->pszParentSid = (PSTR)pszSid;
    }
    else
    {
        ppMemberships[sMembershipCount]->pszChildSid = (PSTR)pszSid;
    }
    sMembershipCount++;

    if (bIsParent)
    {
        dwError = ADCacheStoreGroupMembership(
                        hDb,
                        pszSid,
                        sMembershipCount,
                        ppMemberships);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = ADCacheStoreGroupsForUser(
                        hDb,
                        pszSid,
                        sMembershipCount,
                        ppMemberships,
                        FALSE);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LW_SAFE_FREE_MEMORY(ppMemberships);
    LW_SAFE_FREE_MEMORY(pMembershipBuffers);
    return dwError;

error:
    goto cleanup;
}

static
VOID
AD_TransferCacheObjets(
    IN OUT size_t* psFromObjectsCount,
    IN OUT PLSA_SECURITY_OBJECT* ppFromObjects,
    IN OUT size_t* psToObjectsCount,
    IN OUT PLSA_SECURITY_OBJECT* ppToObjects
    )
{
    size_t bytes = sizeof(ppFromObjects[0]) * (*psFromObjectsCount);

    memcpy(ppToObjects + *psToObjectsCount,
           ppFromObjects,
           bytes);
    *psToObjectsCount += *psFromObjectsCount;

    memset(ppFromObjects, 0, bytes);
    *psFromObjectsCount = 0;
}

static
DWORD
AD_CombineCacheObjects(
    IN OUT size_t* psFromObjectsCount1,
    IN OUT PLSA_SECURITY_OBJECT* ppFromObjects1,
    IN OUT size_t* psFromObjectsCount2,
    IN OUT PLSA_SECURITY_OBJECT* ppFromObjects2,
    OUT size_t* psCombinedObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppCombinedObjects
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppCombinedObjects = NULL;
    size_t sCombinedObjectsCount = 0;

    sCombinedObjectsCount = *psFromObjectsCount1 + *psFromObjectsCount2;
    dwError = LwAllocateMemory(
                    sizeof(*ppCombinedObjects) * sCombinedObjectsCount,
                    (PVOID*)&ppCombinedObjects);
    BAIL_ON_LSA_ERROR(dwError);

    // Cannot fail after this point!
    sCombinedObjectsCount = 0;

    AD_TransferCacheObjets(
            psFromObjectsCount1,
            ppFromObjects1,
            &sCombinedObjectsCount,
            ppCombinedObjects);

    AD_TransferCacheObjets(
            psFromObjectsCount2,
            ppFromObjects2,
            &sCombinedObjectsCount,
            ppCombinedObjects);

    *pppCombinedObjects = ppCombinedObjects;
    *psCombinedObjectsCount = sCombinedObjectsCount;

cleanup:
    return dwError;

error:
    *pppCombinedObjects = NULL;
    *psCombinedObjectsCount = 0;

    LW_SAFE_FREE_MEMORY(ppCombinedObjects);
    sCombinedObjectsCount = 0;
    goto cleanup;
}

static
DWORD
AD_CombineCacheObjectsRemoveDuplicates(
    IN OUT size_t* psFromObjectsCount1,
    IN OUT PLSA_SECURITY_OBJECT* ppFromObjects1,
    IN OUT size_t* psFromObjectsCount2,
    IN OUT PLSA_SECURITY_OBJECT* ppFromObjects2,
    OUT size_t* psCombinedObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppCombinedObjects
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppCombinedObjects = NULL;
    size_t sCombinedObjectsCount = 0;
    PLSA_HASH_TABLE pHashTable = NULL;
    size_t i = 0;
    LSA_HASH_ITERATOR hashIterator;
    LSA_HASH_ENTRY* pHashEntry = NULL;

    dwError = LsaHashCreate(
                20,
                AD_CompareObjectSids,
                AD_HashObjectSid,
                NULL,
                NULL,
                &pHashTable);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < *psFromObjectsCount1; i++)
    {
        if (!LsaHashExists(pHashTable, ppFromObjects1[i]))
        {
            dwError = LsaHashSetValue(
                            pHashTable,
                            ppFromObjects1[i],
                            ppFromObjects1[i]);
            BAIL_ON_LSA_ERROR(dwError);
            ppFromObjects1[i] = NULL;
        }
    }

    for (i = 0; i < *psFromObjectsCount2; i++)
    {
        if (!LsaHashExists(pHashTable, ppFromObjects2[i]))
        {
            dwError = LsaHashSetValue(
                            pHashTable,
                            ppFromObjects2[i],
                            ppFromObjects2[i]);
            BAIL_ON_LSA_ERROR(dwError);
            ppFromObjects2[i] = NULL;
        }
    }

    sCombinedObjectsCount = pHashTable->sCount;
    dwError = LwAllocateMemory(
                    sizeof(*ppCombinedObjects) * sCombinedObjectsCount,
                    (PVOID*)&ppCombinedObjects);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashGetIterator(pHashTable, &hashIterator);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; (pHashEntry = LsaHashNext(&hashIterator)) != NULL; i++)
    {
        PLSA_SECURITY_OBJECT pObject = (PLSA_SECURITY_OBJECT) pHashEntry->pKey;

        dwError = LsaHashRemoveKey(pHashTable, pObject);
        BAIL_ON_LSA_ERROR(dwError);

        ppCombinedObjects[i] = pObject;
    }

    if (i != sCombinedObjectsCount)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppCombinedObjects = ppCombinedObjects;
    *psCombinedObjectsCount = sCombinedObjectsCount;

cleanup:
    if (pHashTable)
    {
        pHashTable->fnFree = AD_FreeHashObject;
        LsaHashSafeFree(&pHashTable);
    }
    // Free any remaining objects.
    for (i = 0; i < *psFromObjectsCount1; i++)
    {
        ADCacheSafeFreeObject(&ppFromObjects1[i]);
    }
    *psFromObjectsCount1 = 0;
    for (i = 0; i < *psFromObjectsCount2; i++)
    {
        ADCacheSafeFreeObject(&ppFromObjects2[i]);
    }
    *psFromObjectsCount2 = 0;
    return dwError;

error:
    *pppCombinedObjects = NULL;
    *psCombinedObjectsCount = 0;

    ADCacheSafeFreeObjectList(sCombinedObjectsCount, &ppCombinedObjects);
    sCombinedObjectsCount = 0;
    goto cleanup;
}

void
AD_FilterNullEntries(
    IN OUT PLSA_SECURITY_OBJECT* ppEntries,
    IN OUT size_t* psCount
    )
{
    size_t sInput = 0;
    size_t sOutput = 0;

    for (; sInput < *psCount; sInput++)
    {
        if (ppEntries[sInput] != NULL)
        {
            ppEntries[sOutput++] = ppEntries[sInput];
        }
    }
    for (sInput = sOutput; sInput < *psCount; sInput++)
    {
        ppEntries[sInput] = (PLSA_SECURITY_OBJECT)-1;
    }

    *psCount = sOutput;
}

DWORD
AD_OnlineFindUserObjectById(
    HANDLE  hProvider,
    uid_t   uid,
    PLSA_SECURITY_OBJECT *ppResult
    )
{
    DWORD dwError =  0;
    PLSA_SECURITY_OBJECT pCachedUser = NULL;

    if (uid == 0) {
    	dwError = LW_ERROR_NO_SUCH_USER;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheFindUserById(
                    gpLsaAdProviderState->hCacheConnection,
                    uid,
                    &pCachedUser);
    if (dwError == LW_ERROR_SUCCESS)
    {
        // Frees object if it is expired
        dwError = AD_CheckExpiredObject(&pCachedUser);
    }

    if (dwError == LW_ERROR_NOT_HANDLED)
    {
        dwError = AD_FindObjectByIdTypeNoCache(
                    hProvider,
                    uid,
                    LSA_OBJECT_TYPE_USER,
                    &pCachedUser);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppResult = pCachedUser;

cleanup:
    return dwError;

error:

    *ppResult = NULL;
    ADCacheSafeFreeObject(&pCachedUser);

    LSA_REMAP_FIND_USER_BY_ID_ERROR(dwError, FALSE, uid);

    goto cleanup;
}

DWORD
AD_OnlineGetUserGroupObjectMembership(
    IN HANDLE hProvider,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    IN BOOLEAN bIsCacheOnlyMode,
    OUT size_t* psCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sMembershipCount = 0;
    PLSA_GROUP_MEMBERSHIP* ppMemberships = NULL;
    BOOLEAN bExpired = FALSE;
    BOOLEAN bIsComplete = FALSE;
    BOOLEAN bUseCache = FALSE;
    size_t sUnexpirableResultsCount = 0;
    PLSA_SECURITY_OBJECT* ppUnexpirableResults = NULL;
    size_t sResultsCount = 0;
    PLSA_SECURITY_OBJECT* ppResults = NULL;
    size_t sFilteredResultsCount = 0;
    PLSA_SECURITY_OBJECT* ppFilteredResults = NULL;
    // Only free top level array, do not free string pointers.
    PSTR* ppszSids = NULL;
    PCSTR pszSid = NULL;
    int iPrimaryGroupIndex = -1;

    pszSid = pUserInfo->pszObjectSid;

    dwError = ADCacheGetGroupsForUser(
                    gpLsaAdProviderState->hCacheConnection,
                    pszSid,
                    AD_GetTrimUserMembershipEnabled(),
                    &sMembershipCount,
                    &ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_CheckExpiredMemberships(
                    sMembershipCount,
                    ppMemberships,
                    TRUE,
                    &bExpired,
                    &bIsComplete);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExpired)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for user's group membership for sid %s is expired",
            pszSid);
    }
    else if (!bIsComplete)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for user's group membership for sid %s is incomplete",
            pszSid);
    }

    if (bExpired || !bIsComplete)
    {
        LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;

        dwError = AD_DetermineTrustModeandDomainName(
                        pUserInfo->pszNetbiosDomainName,
                        &dwTrustDirection,
                        NULL,
                        NULL,
                        NULL);
        BAIL_ON_LSA_ERROR(dwError);

        if (dwTrustDirection == LSA_TRUST_DIRECTION_ONE_WAY)
        {
            bUseCache = TRUE;
        }
    }
    else
    {
        bUseCache = TRUE;
    }

    if (!bUseCache && bIsCacheOnlyMode)
    {
        dwError = AD_FilterExpiredMemberships(&sMembershipCount, ppMemberships);
        BAIL_ON_LSA_ERROR(dwError);

        bUseCache = TRUE;
    }

    if (!bUseCache)
    {
        // Get the complete object for the unexpirable entries (the ones
        // that we can't get through regular LDAP queries).  Later on, this
        // list will be appended to the list we get from LDAP.

        dwError = AD_GatherSidsFromGroupMemberships(
                        TRUE,
                        AD_IncludeOnlyUnexpirableGroupMembershipsCallback,
                        sMembershipCount,
                        ppMemberships,
                        &sUnexpirableResultsCount,
                        &ppszSids);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_FindObjectsBySidList(
                        hProvider,
                        sUnexpirableResultsCount,
                        ppszSids,
                        &sUnexpirableResultsCount,
                        &ppUnexpirableResults);
        BAIL_ON_LSA_ERROR(dwError);

        LW_SAFE_FREE_MEMORY(ppszSids);
        ADCacheSafeFreeGroupMembershipList(sMembershipCount, &ppMemberships);

        dwError = ADLdap_GetObjectGroupMembership(
                         hProvider,
                         pUserInfo,
                         &iPrimaryGroupIndex,
                         &sResultsCount,
                         &ppResults);
        BAIL_ON_LSA_ERROR(dwError);

        AD_FilterNullEntries(ppResults, &sResultsCount);

        dwError = AD_CacheMembershipFromRelatedObjects(
                        gpLsaAdProviderState->hCacheConnection,
                        pszSid,
                        iPrimaryGroupIndex,
                        FALSE,
                        sResultsCount,
                        ppResults);
        BAIL_ON_LSA_ERROR(dwError);

        // Combine and filter out duplicates.
        dwError = AD_CombineCacheObjectsRemoveDuplicates(
                        &sResultsCount,
                        ppResults,
                        &sUnexpirableResultsCount,
                        ppUnexpirableResults,
                        &sFilteredResultsCount,
                        &ppFilteredResults);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        /* If the cache is valid, get the rest of the object info with
         * AD_FindObjectsBySidList.
         */
        LSA_LOG_VERBOSE(
              "Using cached user's group membership for sid %s",
              pszSid);

        dwError = AD_GatherSidsFromGroupMemberships(
                        TRUE,
                        NULL,
                        sMembershipCount,
                        ppMemberships,
                        &sResultsCount,
                        &ppszSids);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_FindObjectsBySidList(
                        hProvider,
                        sResultsCount,
                        ppszSids,
                        &sFilteredResultsCount,
                        &ppFilteredResults);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *psCount = sFilteredResultsCount;
    *pppResults = ppFilteredResults;

cleanup:
    LW_SAFE_FREE_MEMORY(ppszSids);
    ADCacheSafeFreeGroupMembershipList(sMembershipCount, &ppMemberships);
    ADCacheSafeFreeObjectList(sResultsCount, &ppResults);
    ADCacheSafeFreeObjectList(sUnexpirableResultsCount, &ppUnexpirableResults);

    return dwError;

error:
    *psCount = 0;
    *pppResults = NULL;

    if ( dwError != LW_ERROR_DOMAIN_IS_OFFLINE && pUserInfo )
    {
        LSA_LOG_ERROR("Failed to find memberships for user '%s\\%s' (error = %d)",
                      pUserInfo->pszNetbiosDomainName,
                      pUserInfo->pszSamAccountName,
                      dwError);
    }

    ADCacheSafeFreeObjectList(sFilteredResultsCount, &ppFilteredResults);
    sFilteredResultsCount = 0;

    goto cleanup;
}

DWORD
AD_OnlineFindGroupObjectByName(
    HANDLE  hProvider,
    PCSTR   pszGroupName,
    PLSA_SECURITY_OBJECT*  ppResult
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pGroupNameInfo = NULL;
    PSTR  pszGroupName_copy = NULL;
    PLSA_SECURITY_OBJECT pCachedGroup = NULL;

    BAIL_ON_INVALID_STRING(pszGroupName);
    dwError = LwAllocateString(
                    pszGroupName,
                    &pszGroupName_copy);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrCharReplace(pszGroupName_copy, AD_GetSpaceReplacement(),' ');

    dwError = LsaCrackDomainQualifiedName(
                        pszGroupName_copy,
                        gpADProviderData->szDomain,
                        &pGroupNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if ((pGroupNameInfo->nameType == NameType_Alias) &&
    	!strcasecmp(pGroupNameInfo->pszName, "root"))
    {
    	dwError = LW_ERROR_NO_SUCH_GROUP;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheFindGroupByName(
                    gpLsaAdProviderState->hCacheConnection,
                    pGroupNameInfo,
                    &pCachedGroup);
    if (dwError == LW_ERROR_SUCCESS)
    {
        dwError = AD_CheckExpiredObject(&pCachedGroup);
    }

    if (dwError == 0)
    {
        goto cleanup;
    }

    if (dwError != LW_ERROR_NOT_HANDLED){
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Otherwise, look up the group
    dwError = AD_FindObjectByNameTypeNoCache(
                    hProvider,
                    pszGroupName_copy,
                    pGroupNameInfo->nameType,
                    LSA_OBJECT_TYPE_GROUP,
                    &pCachedGroup);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheStoreObjectEntry(
                    gpLsaAdProviderState->hCacheConnection,
                    pCachedGroup);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (dwError)
    {
        ADCacheSafeFreeObject(&pCachedGroup);
        LSA_REMAP_FIND_GROUP_BY_NAME_ERROR(dwError, FALSE, pszGroupName);
    }
    *ppResult = pCachedGroup;

    if (pGroupNameInfo)
    {
        LsaFreeNameInfo(pGroupNameInfo);
    }
    LW_SAFE_FREE_STRING(pszGroupName_copy);

    return dwError;

error:
    // Do not handle error here, but in cleanup, since there is a goto cleanup
    goto cleanup;
}

DWORD
AD_OnlineChangePassword(
    HANDLE hProvider,
    PCSTR pszLoginId,
    PCSTR pszPassword,
    PCSTR pszOldPassword
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PLSA_SECURITY_OBJECT pCachedUser = NULL;
    PSTR pszFullDomainName = NULL;
    BOOLEAN bFoundDomain = FALSE;
    LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
    PLWNET_DC_INFO pDcInfo = NULL;
    DWORD dwGoodUntilTime = 0;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    gpADProviderData->szDomain,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_ServicesDomainWithDiscovery(
                    pLoginInfo->pszDomainNetBiosName,
                    &bFoundDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bFoundDomain)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_FindUserObjectByName(
                     hProvider,
                     pszLoginId,
                     &pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_UpdateObject(pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pCachedUser->userInfo.bUserCanChangePassword) {
        dwError = LW_ERROR_USER_CANNOT_CHANGE_PASSWD;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pCachedUser->userInfo.bAccountDisabled) {
        dwError = LW_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pCachedUser->userInfo.bAccountExpired) {
        dwError = LW_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pCachedUser->userInfo.bAccountLocked) {
        dwError = LW_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Make sure that we are affinitized.
    dwError = LsaDmEngineGetDomainNameAndSidByObjectSidWithDiscovery(
                       pCachedUser->pszObjectSid,
                       &pszFullDomainName,
                       NULL,
                       NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LWNetGetDCName(
                    NULL,
                    pszFullDomainName,
                    NULL,
                    DS_WRITABLE_REQUIRED,
                    &pDcInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_DetermineTrustModeandDomainName(
                                       pszFullDomainName,
                                       &dwTrustDirection,
                                       NULL,
                                       NULL,
                                       NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_NetUserChangePassword(pDcInfo->pszDomainControllerName,
                                       LSA_TRUST_DIRECTION_ONE_WAY == dwTrustDirection,
                                       pCachedUser->pszSamAccountName,
                                       pCachedUser->userInfo.pszUPN,
                                       pszOldPassword,
                                       pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    // Now that the user password is updated, we need to expire the cache entry.
    dwError = AD_StoreAsExpiredObject(&pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

    // Ignore errors because password change succeeded
    LsaUmModifyUser(
        pCachedUser->userInfo.uid,
        pszPassword);

    // Run a check against the new password. This will download a pac for the
    // user and store their user kerberos creds.
    dwError = AD_OnlineCheckUserPassword(
                    hProvider,
                    pCachedUser,
                    pszPassword,
                    &dwGoodUntilTime);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);
    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    ADCacheSafeFreeObject(&pCachedUser);

    LW_SAFE_FREE_STRING(pszFullDomainName);


    return dwError;

error:

    goto cleanup;
}

DWORD
AD_CreateHomeDirectory(
    PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;

    if (LW_IS_NULL_OR_EMPTY_STR(pObject->userInfo.pszHomedir)) {
        dwError = LW_ERROR_FAILED_CREATE_HOMEDIR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaCheckDirectoryExists(
                    pObject->userInfo.pszHomedir,
                    &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists && AD_ShouldCreateHomeDir()) {
        dwError = AD_CreateHomeDirectory_Generic(pObject);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    LSA_LOG_ERROR("Failed to create home directory for user (%s), actual error %d", LSA_SAFE_LOG_STRING(pObject->userInfo.pszUnixName), dwError);
    dwError = LW_ERROR_FAILED_CREATE_HOMEDIR;

    goto cleanup;
}

DWORD
AD_CreateHomeDirectory_Generic(
    PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = 0;
    mode_t  umask = 0;
    mode_t  perms = (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BOOLEAN bRemoveDir = FALSE;

    umask = AD_GetUmask();

    dwError = LsaCreateDirectory(
                 pObject->userInfo.pszHomedir,
                 perms);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaChangePermissions(
                 pObject->userInfo.pszHomedir,
                 perms & (~umask));
    BAIL_ON_LSA_ERROR(dwError);

    bRemoveDir = TRUE;

    dwError = LsaChangeOwner(
                 pObject->userInfo.pszHomedir,
                 pObject->userInfo.uid,
                 pObject->userInfo.gid);
    BAIL_ON_LSA_ERROR(dwError);

    bRemoveDir = FALSE;

    dwError = AD_ProvisionHomeDir(
                    pObject->userInfo.uid,
                    pObject->userInfo.gid,
                    pObject->userInfo.pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    if (bRemoveDir) {
       LsaRemoveDirectory(pObject->userInfo.pszHomedir);
    }

    LSA_LOG_ERROR("Failed to create home directory for user (%s), actual error %d", pObject->userInfo.pszUnixName, dwError);
    dwError = LW_ERROR_FAILED_CREATE_HOMEDIR;

    goto cleanup;
}

DWORD
AD_ProvisionHomeDir(
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszHomedirPath
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    PSTR pszSkelPaths = NULL;
    PSTR pszSkelPath = NULL;
    PSTR pszIter = NULL;
    size_t stLen = 0;

    dwError = AD_GetSkelDirs(&pszSkelPaths);
    BAIL_ON_LSA_ERROR(dwError);

    if (LW_IS_NULL_OR_EMPTY_STR(pszSkelPaths))
    {
        goto cleanup;
    }

    pszIter = pszSkelPaths;
    while ((stLen = strcspn(pszIter, ",")) != 0)
    {
        dwError = LwStrndup(
                      pszIter,
                      stLen,
                      &pszSkelPath);
        BAIL_ON_LSA_ERROR(dwError);

        LwStripWhitespace(pszSkelPath, TRUE, TRUE);

        if (LW_IS_NULL_OR_EMPTY_STR(pszSkelPath))
        {
            LW_SAFE_FREE_STRING(pszSkelPath);
            continue;
        }

        dwError = LsaCheckDirectoryExists(
                        pszSkelPath,
                        &bExists);
        BAIL_ON_LSA_ERROR(dwError);

        if (bExists)
        {
            dwError = LsaCopyDirectory(
                        pszSkelPath,
                        ownerUid,
                        ownerGid,
                        pszHomedirPath);
            BAIL_ON_LSA_ERROR(dwError);
        }

        LW_SAFE_FREE_STRING(pszSkelPath);

        pszIter += stLen;
        stLen = strspn(pszIter, ",");
        pszIter += stLen;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszSkelPath);
    LW_SAFE_FREE_STRING(pszSkelPaths);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_WriteFileData(
    int fd,
    PVOID pBuffer,
    DWORD dwLen
    )
{
    DWORD dwError = 0;
    DWORD dwRemaining = dwLen;
    PSTR pStr = (PSTR) pBuffer;

    while (dwRemaining > 0)
    {
        int nWritten = write(fd, pStr, dwRemaining);
        if (nWritten < 0)
        {
            if (errno != EAGAIN && errno != EINTR)
            {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else
        {
            dwRemaining -= nWritten;
            pStr += nWritten;
        }
    }

error:
    return dwError;
}

DWORD
AD_CreateK5Login(
    PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD   dwError = 0;
    PSTR    pszK5LoginPath = NULL;
    PSTR    pszK5LoginPath_tmp = NULL;
    PSTR    pszData = NULL;
    BOOLEAN bExists = FALSE;
    PSTR pszUpnCopy = NULL;
    PSTR pszUpnCopyLower = NULL;
    int     fd = -1;
    BOOLEAN bRemoveFile = FALSE;

    BAIL_ON_INVALID_STRING(pObject->userInfo.pszHomedir);
    BAIL_ON_INVALID_STRING(pObject->userInfo.pszUPN);

    dwError = LwAllocateStringPrintf(
                    &pszK5LoginPath,
                    "%s/.k5login",
                    pObject->userInfo.pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckFileExists(
                    pszK5LoginPath,
                    &bExists);
    if (dwError == LW_ERROR_ACCESS_DENIED)
    {
        LSA_LOG_WARNING("Failed to stat k5login file at '%s' due to insufficient permissions. Most likely the user's home directory is NFS mounted from a server with root squash enabled.",
                      pszK5LoginPath);
        dwError = 0;
        goto cleanup;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (bExists) {
        goto cleanup;
    }

    // Create a copy of the UPN to make sure that the realm is uppercase,
    // but preserving the case of the non-realm part.
    dwError = LwAllocateString(
                    pObject->userInfo.pszUPN,
                    &pszUpnCopy);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPrincipalRealmToUpper(pszUpnCopy);

    // Create another copy of the UPN that has lowercase non-realm part.
    dwError = LwAllocateString(
                    pszUpnCopy,
                    &pszUpnCopyLower);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPrincipalNonRealmToLower(pszUpnCopyLower);

    if (!strcmp(pszUpnCopy, pszUpnCopyLower))
    {
        // If the UPNs are the same, just need to write one.
        dwError = LwAllocateStringPrintf(
                        &pszData,
                        "%s\n",
                        pszUpnCopy);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        // Otherwise, they are different and we want both.
        dwError = LwAllocateStringPrintf(
                        &pszData,
                        "%s\n%s\n",
                        pszUpnCopy,
                        pszUpnCopyLower);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(
                    &pszK5LoginPath_tmp,
                    "%s_lsass",
                    pszK5LoginPath);
    BAIL_ON_LSA_ERROR(dwError);

    fd = open(
            pszK5LoginPath_tmp,
            O_CREAT|O_WRONLY|O_EXCL,
            S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd < 0)
    {
        if (errno == EEXIST)
        {
            goto cleanup;
        }
        else if (errno == EACCES)
        {
            LSA_LOG_WARNING("Failed to create temporary k5login file at '%s' due to insufficient permissions. Most likely the user's home directory is NFS mounted from a server with root squash enabled.",
                          pszK5LoginPath_tmp);
            goto cleanup;
        }
        else
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    bRemoveFile = TRUE;

    dwError = AD_WriteFileData(
                    fd,
                    pszData,
                    strlen(pszData));
    BAIL_ON_LSA_ERROR(dwError);

    close(fd);
    fd = -1;

    dwError = LsaMoveFile(
                    pszK5LoginPath_tmp,
                    pszK5LoginPath);
    BAIL_ON_LSA_ERROR(dwError);

    bRemoveFile = FALSE;

    dwError = LsaChangeOwnerAndPermissions(
                    pszK5LoginPath,
                    pObject->userInfo.uid,
                    pObject->userInfo.gid,
                    S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH
                    );
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (fd >= 0) {
        close(fd);
    }

    if (bRemoveFile) {

        DWORD dwError2 = LsaRemoveFile(pszK5LoginPath_tmp);
        if (dwError2) {
            LSA_LOG_ERROR("Failed to remove file at [%s][Error code: %d]",
                          pszK5LoginPath_tmp,
                          dwError2);
        }

    }

    LW_SAFE_FREE_STRING(pszData);
    LW_SAFE_FREE_STRING(pszUpnCopy);
    LW_SAFE_FREE_STRING(pszUpnCopyLower);
    LW_SAFE_FREE_STRING(pszK5LoginPath_tmp);
    LW_SAFE_FREE_STRING(pszK5LoginPath);

    return dwError;

error:

    goto cleanup;
}

int
AD_CompareObjectSids(
        PCVOID pObjectA,
        PCVOID pObjectB)
{
    return strcasecmp(
            ((PLSA_SECURITY_OBJECT)pObjectA)->pszObjectSid,
            ((PLSA_SECURITY_OBJECT)pObjectB)->pszObjectSid);
}

size_t
AD_HashObjectSid(
        PCVOID pObject)
{
    return LsaHashCaselessStringHash(((PLSA_SECURITY_OBJECT)pObject)->pszObjectSid);
}

void
AD_FreeHashObject(
    const LSA_HASH_ENTRY *pEntry)
{
    ADCacheSafeFreeObject((PLSA_SECURITY_OBJECT *)&pEntry->pKey);
}

DWORD
AD_OnlineGetGroupMembers(
    IN HANDLE hProvider,
    IN PCSTR pszDomainName,
    IN PCSTR pszSid,
    IN BOOLEAN bIsCacheOnlyMode,
    OUT size_t* psCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sMembershipCount = 0;
    PLSA_GROUP_MEMBERSHIP* ppMemberships = NULL;
    BOOLEAN bExpired = FALSE;
    BOOLEAN bIsComplete = FALSE;
    BOOLEAN bUseCache = FALSE;
    size_t sUnexpirableResultsCount = 0;
    PLSA_SECURITY_OBJECT* ppUnexpirableResults = NULL;
    size_t sResultsCount = 0;
    PLSA_SECURITY_OBJECT* ppResults = NULL;
    size_t sFilteredResultsCount = 0;
    PLSA_SECURITY_OBJECT* ppFilteredResults = NULL;
    // Only free top level array, do not free string pointers.
    PSTR* ppszSids = NULL;

    dwError = ADCacheGetGroupMembers(
                    gpLsaAdProviderState->hCacheConnection,
                    pszSid,
                    AD_GetTrimUserMembershipEnabled(),
                    &sMembershipCount,
                    &ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_CheckExpiredMemberships(
                    sMembershipCount,
                    ppMemberships,
                    FALSE,
                    &bExpired,
                    &bIsComplete);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExpired)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for group membership for sid %s is expired",
            pszSid);
    }
    else if (!bIsComplete)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for group membership for sid %s is incomplete",
            pszSid);
    }

    if (bExpired || !bIsComplete)
    {
        LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;

        dwError = AD_DetermineTrustModeandDomainName(
                        pszDomainName,
                        &dwTrustDirection,
                        NULL,
                        NULL,
                        NULL);
        BAIL_ON_LSA_ERROR(dwError);

        if (dwTrustDirection == LSA_TRUST_DIRECTION_ONE_WAY)
        {
            bUseCache = TRUE;
        }
    }
    else
    {
        bUseCache = TRUE;
    }

    if (!bUseCache && bIsCacheOnlyMode)
    {
        dwError = AD_FilterExpiredMemberships(&sMembershipCount, ppMemberships);
        BAIL_ON_LSA_ERROR(dwError);

        bUseCache = TRUE;
    }

    if (!bUseCache)
    {
        // Get the complete object for the unexpirable entries (the ones
        // that we can't get through regular LDAP queries).  Later on, this
        // list will be appended to the list we get from LDAP.

        dwError = AD_GatherSidsFromGroupMemberships(
                        FALSE,
                        AD_IncludeOnlyUnexpirableGroupMembershipsCallback,
                        sMembershipCount,
                        ppMemberships,
                        &sUnexpirableResultsCount,
                        &ppszSids);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_FindObjectsBySidList(
                        hProvider,
                        sUnexpirableResultsCount,
                        ppszSids,
                        &sUnexpirableResultsCount,
                        &ppUnexpirableResults);
        BAIL_ON_LSA_ERROR(dwError);

        LW_SAFE_FREE_MEMORY(ppszSids);
        ADCacheSafeFreeGroupMembershipList(sMembershipCount, &ppMemberships);

        dwError = ADLdap_GetGroupMembers(
                        hProvider,
                        pszDomainName,
                        pszSid,
                        &sResultsCount,
                        &ppResults);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_CacheMembershipFromRelatedObjects(
                        gpLsaAdProviderState->hCacheConnection,
                        pszSid,
                        -1,
                        TRUE,
                        sResultsCount,
                        ppResults);
        BAIL_ON_LSA_ERROR(dwError);

        // Combine and let the caller will filter out duplicates.
        dwError = AD_CombineCacheObjects(
                        &sResultsCount,
                        ppResults,
                        &sUnexpirableResultsCount,
                        ppUnexpirableResults,
                        &sFilteredResultsCount,
                        &ppFilteredResults);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        /* If the cache is valid, get the rest of the object info with
         * AD_FindObjectsBySidList.
         */
        LSA_LOG_VERBOSE(
              "Using cached group membership for sid %s",
              pszSid);

        dwError = AD_GatherSidsFromGroupMemberships(
                        FALSE,
                        NULL,
                        sMembershipCount,
                        ppMemberships,
                        &sResultsCount,
                        &ppszSids);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_FindObjectsBySidList(
                        hProvider,
                        sResultsCount,
                        ppszSids,
                        &sFilteredResultsCount,
                        &ppFilteredResults);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *psCount = sFilteredResultsCount;
    *pppResults = ppFilteredResults;

cleanup:
    LW_SAFE_FREE_MEMORY(ppszSids);
    ADCacheSafeFreeGroupMembershipList(sMembershipCount, &ppMemberships);
    ADCacheSafeFreeObjectList(sResultsCount, &ppResults);
    ADCacheSafeFreeObjectList(sUnexpirableResultsCount, &ppUnexpirableResults);

    return dwError;

error:
    *psCount = 0;
    *pppResults = NULL;

    ADCacheSafeFreeObjectList(sFilteredResultsCount, &ppFilteredResults);
    sFilteredResultsCount = 0;

    goto cleanup;
}

static
DWORD
AD_FindObjectBySidNoCache(
    IN HANDLE hProvider,
    IN PCSTR pszSid,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    return LsaAdBatchFindSingleObject(
                LSA_AD_BATCH_QUERY_TYPE_BY_SID,
                pszSid,
                NULL,
                ppObject);
}

static
DWORD
AD_FindObjectByNT4NameNoCache(
    IN HANDLE hProvider,
    IN PCSTR pszNT4Name,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    return LsaAdBatchFindSingleObject(
                LSA_AD_BATCH_QUERY_TYPE_BY_NT4,
                pszNT4Name,
                NULL,
                ppObject);
}

static
DWORD
AD_FindObjectByUpnNoCache(
    IN HANDLE hProvider,
    IN PCSTR pszUpn,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PSTR pszSid = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;

    dwError = LsaDmWrapNetLookupObjectSidByName(
                    gpADProviderData->szDomain,
                    pszUpn,
                    &pszSid,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_FindObjectBySidNoCache(
                    hProvider,
                    pszSid,
                    &pObject);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pszSid);

    *ppObject = pObject;

    return dwError;

error:
    ADCacheSafeFreeObject(&pObject);
    goto cleanup;
}

static
DWORD
AD_FindObjectByAliasNoCache(
    IN HANDLE hProvider,
    IN PCSTR pszAlias,
    BOOLEAN bIsUserAlias,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    return LsaAdBatchFindSingleObject(
                   bIsUserAlias ? LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS : LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS,
                   pszAlias,
                   NULL,
                   ppResult);
}

DWORD
AD_FindObjectByNameTypeNoCache(
    IN HANDLE hProvider,
    IN PCSTR pszName,
    IN ADLogInNameType NameType,
    IN LSA_OBJECT_TYPE AccountType,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsUser = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;

    switch (AccountType)
    {
        case LSA_OBJECT_TYPE_USER:
            bIsUser = TRUE;
            break;
        case LSA_OBJECT_TYPE_GROUP:
            bIsUser = FALSE;
            break;
        default:
            if (NameType == NameType_Alias)
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
            }
            break;
    }

    switch (NameType)
    {
        case NameType_NT4:
            dwError = AD_FindObjectByNT4NameNoCache(
                            hProvider,
                            pszName,
                            &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case NameType_UPN:
            dwError = AD_FindObjectByUpnNoCache(
                            hProvider,
                            pszName,
                            &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case NameType_Alias:
            dwError = AD_FindObjectByAliasNoCache(
                            hProvider,
                            pszName,
                            bIsUser,
                            &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    // Check whether the object we find is correct type or not
    if (AccountType != LSA_OBJECT_TYPE_UNDEFINED && AccountType != pObject->type)
    {
        dwError = bIsUser ? LW_ERROR_NO_SUCH_USER : LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *ppObject = pObject;

    return dwError;

error:
    if (LW_ERROR_NO_SUCH_OBJECT == dwError)
    {
        dwError = bIsUser ? LW_ERROR_NO_SUCH_USER : LW_ERROR_NO_SUCH_GROUP;
    }
    ADCacheSafeFreeObject(&pObject);
    goto cleanup;
}

DWORD
AD_FindObjectByIdTypeNoCache(
    IN HANDLE hProvider,
    IN DWORD dwId,
    IN LSA_OBJECT_TYPE AccountType,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsUser = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;

    switch (AccountType)
    {
        case LSA_OBJECT_TYPE_USER:
            bIsUser = TRUE;
            dwError = LsaAdBatchFindSingleObject(
                           LSA_AD_BATCH_QUERY_TYPE_BY_UID,
                           NULL,
                           &dwId,
                           &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        case LSA_OBJECT_TYPE_GROUP:
            bIsUser = FALSE;
            dwError = LsaAdBatchFindSingleObject(
                           LSA_AD_BATCH_QUERY_TYPE_BY_GID,
                           NULL,
                           &dwId,
                           &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:
            LSA_ASSERT(FALSE);
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    // Check whether the object we find is correct type or not
    if (AccountType != pObject->type)
    {
        dwError = bIsUser ? LW_ERROR_NO_SUCH_USER : LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *ppObject = pObject;

    return dwError;

error:
    if (LW_ERROR_NO_SUCH_OBJECT == dwError)
    {
        dwError = bIsUser ? LW_ERROR_NO_SUCH_USER : LW_ERROR_NO_SUCH_GROUP;
    }
    ADCacheSafeFreeObject(&pObject);
    goto cleanup;
}

static
DWORD
AD_FindObjectsByListNoCache(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN PSTR* ppszList,
    OUT PDWORD pdwCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    return LsaAdBatchFindObjects(
                QueryType,
                dwCount,
                ppszList,
                NULL,
                pdwCount,
                pppObjects);
}

DWORD
AD_FindObjectBySid(
    IN HANDLE hProvider,
    IN PCSTR pszSid,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_SECURITY_OBJECT* ppResultArray = NULL;
    size_t objectCount = 0;

    dwError = AD_FindObjectsBySidList(
                    hProvider,
                    1,
                    (PSTR*)&pszSid,
                    &objectCount,
                    &ppResultArray);
    BAIL_ON_LSA_ERROR(dwError);

    if (objectCount < 1)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppResult = ppResultArray[0];

cleanup:
    LW_SAFE_FREE_MEMORY(ppResultArray);
    return dwError;

error:
    *ppResult = NULL;
    ADCacheSafeFreeObjectList(objectCount, &ppResultArray);
    goto cleanup;
}

DWORD
AD_FindObjectsByList(
    IN LSA_AD_CACHEDB_FIND_OBJECTS_BY_LIST_CALLBACK pFindInCacheCallback,
    IN LSA_AD_LDAP_FIND_OBJECTS_BY_LIST_BATCHED_CALLBACK pFindByListBatchedCallback,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN size_t sCount,
    IN PSTR* ppszList,
    OUT OPTIONAL size_t* psResultsCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_SECURITY_OBJECT* ppResults = NULL;
    size_t sResultsCount = 0;
    size_t sFoundInCache = 0;
    size_t sFoundInAD = 0;
    DWORD  dwFoundInAD = 0;
    size_t sRemainNumsToFoundInAD = 0;
    size_t sIndex;
    time_t now = 0;
    // Do not free the strings that ppszRemainSidsList point to
    PSTR* ppszRemainingList = NULL;
    PLSA_SECURITY_OBJECT *ppRemainingObjectsResults = NULL;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);
    /*
     * Lookup as many objects as possible from the cache.
     */
    dwError = pFindInCacheCallback(
                    gpLsaAdProviderState->hCacheConnection,
                    sCount,
                    ppszList,
                    &ppResults);
    BAIL_ON_LSA_ERROR(dwError);
    sResultsCount = sCount;

    dwError = LwAllocateMemory(
                    sCount*sizeof(*ppszRemainingList),
                    (PVOID*)&ppszRemainingList);
    BAIL_ON_LSA_ERROR(dwError);

    for (sFoundInCache = 0, sRemainNumsToFoundInAD = 0, sIndex = 0;
         sIndex < sCount;
         sIndex++)
    {
        if ((ppResults[sIndex] != NULL) &&
            (ppResults[sIndex]->version.tLastUpdated >= 0) &&
            (ppResults[sIndex]->version.tLastUpdated +
            AD_GetCacheEntryExpirySeconds() <= now))
        {
            switch (QueryType)
            {
                case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
                    LSA_LOG_VERBOSE("Cache entry for Sid %s is expired",
                         LSA_SAFE_LOG_STRING(ppResults[sIndex]->pszObjectSid));

                    break;

                case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
                    LSA_LOG_VERBOSE("Cache entry for DN %s is expired",
                         LSA_SAFE_LOG_STRING(ppResults[sIndex]->pszDN));

                    break;

                default:
                    LSA_ASSERT(FALSE);
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(dwError);
            }
            ADCacheSafeFreeObject(&ppResults[sIndex]);
        }

        if (ppResults[sIndex] != NULL)
        {
            sFoundInCache++;
            continue;
        }
        ppszRemainingList[sRemainNumsToFoundInAD++] = ppszList[sIndex];
    }

    AD_FilterNullEntries(ppResults, &sResultsCount);
    assert(sResultsCount == sFoundInCache);

    if (!sRemainNumsToFoundInAD)
    {
        goto cleanup;
    }

    dwError = pFindByListBatchedCallback(
                     QueryType,
                     sRemainNumsToFoundInAD,
                     ppszRemainingList,
                     &dwFoundInAD,
                     &ppRemainingObjectsResults);
    BAIL_ON_LSA_ERROR(dwError);

    sFoundInAD = dwFoundInAD;

    dwError = ADCacheStoreObjectEntries(
                    gpLsaAdProviderState->hCacheConnection,
                    sFoundInAD,
                    ppRemainingObjectsResults);
    BAIL_ON_LSA_ERROR(dwError);

    // Filtering the null entries created enough space to append the
    // entries looked up through AD.
    memcpy(ppResults + sFoundInCache,
           ppRemainingObjectsResults,
           sizeof(*ppRemainingObjectsResults) * sFoundInAD);
    memset(ppRemainingObjectsResults,
           0,
           sizeof(*ppRemainingObjectsResults) * sFoundInAD);
    sResultsCount += sFoundInAD;

cleanup:

    // We handle error here instead of error label
    // because there is a goto cleanup above.
    if (dwError)
    {
        ADCacheSafeFreeObjectList(sResultsCount, &ppResults);
        sResultsCount = 0;
    }
    else
    {
        assert(sResultsCount == (sFoundInCache + sFoundInAD));
    }

    *pppResults = ppResults;
    if (psResultsCount)
    {
        *psResultsCount = sResultsCount;
    }

    ADCacheSafeFreeObjectList(sFoundInAD, &ppRemainingObjectsResults);
    LW_SAFE_FREE_MEMORY(ppszRemainingList);

    return dwError;

error:

    // Do not actually handle any errors here.
    goto cleanup;
}

DWORD
AD_FindObjectsBySidList(
    IN HANDLE hProvider,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT OPTIONAL size_t* psResultsCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    return AD_FindObjectsByList(
               ADCacheFindObjectsBySidList,
               AD_FindObjectsByListNoCache,
               LSA_AD_BATCH_QUERY_TYPE_BY_SID,
               sCount,
               ppszSidList,
               psResultsCount,
               pppResults);
}

DWORD
AD_FindObjectsByDNList(
    IN HANDLE hProvider,
    IN size_t sCount,
    IN PSTR* ppszDNList,
    OUT OPTIONAL size_t* psResultsCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    return AD_FindObjectsByList(
               ADCacheFindObjectsByDNList,
               AD_FindObjectsByListNoCache,
               LSA_AD_BATCH_QUERY_TYPE_BY_DN,
               sCount,
               ppszDNList,
               psResultsCount,
               pppResults);
}

DWORD
AD_OnlineFindUserObjectByName(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    PLSA_SECURITY_OBJECT* ppCachedUser
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pUserNameInfo = NULL;
    PSTR  pszLoginId_copy = NULL;
    PLSA_SECURITY_OBJECT pCachedUser = NULL;

    BAIL_ON_INVALID_STRING(pszLoginId);
    dwError = LwAllocateString(
                    pszLoginId,
                    &pszLoginId_copy);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrCharReplace(
            pszLoginId_copy,
            AD_GetSpaceReplacement(),
            ' ');

    dwError = LsaCrackDomainQualifiedName(
                        pszLoginId_copy,
                        gpADProviderData->szDomain,
                        &pUserNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheFindUserByName(
                    gpLsaAdProviderState->hCacheConnection,
                    pUserNameInfo,
                    &pCachedUser);
    if (dwError == LW_ERROR_SUCCESS)
    {
        dwError = AD_CheckExpiredObject(&pCachedUser);
    }

    if (dwError == 0){
        goto cleanup;
    }

    if (dwError != LW_ERROR_NOT_HANDLED){
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_FindObjectByNameTypeNoCache(
                    hProvider,
                    pszLoginId_copy,
                    pUserNameInfo->nameType,
                    LSA_OBJECT_TYPE_USER,
                    &pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheStoreObjectEntry(
                    gpLsaAdProviderState->hCacheConnection,
                    pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (dwError)
    {
        ADCacheSafeFreeObject(&pCachedUser);
        LSA_REMAP_FIND_USER_BY_NAME_ERROR(dwError, FALSE, pszLoginId);
    }
    *ppCachedUser = pCachedUser;

    if (pUserNameInfo)
    {
        LsaFreeNameInfo(pUserNameInfo);
    }
    LW_SAFE_FREE_STRING(pszLoginId_copy);

    return dwError;

error:
    // Do not actually handle error here, handle error in cleanup, since there is 'goto cleanup'
    goto cleanup;
}

DWORD
AD_OnlineFindNSSArtefactByKey(
    HANDLE hProvider,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
    DWORD dwError = 0;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;

    dwError = LsaDmLdapOpenDc(
                    gpADProviderData->szDomain,
                    &pConn);
    BAIL_ON_LSA_ERROR(dwError);

    switch (gpADProviderData->dwDirectoryMode)
    {
    case DEFAULT_MODE:

        dwError = DefaultModeFindNSSArtefactByKey(
                        pConn,
                        gpADProviderData->cell.szCellDN,
                        gpADProviderData->szShortDomain,
                        pszKeyName,
                        pszMapName,
                        dwInfoLevel,
                        dwFlags,
                        ppNSSArtefactInfo);
        break;

    case CELL_MODE:

        dwError = CellModeFindNSSArtefactByKey(
                        pConn,
                        gpADProviderData->cell.szCellDN,
                        gpADProviderData->szShortDomain,
                        pszKeyName,
                        pszMapName,
                        dwInfoLevel,
                        dwFlags,
                        ppNSSArtefactInfo);
        break;

    case UNPROVISIONED_MODE:

        dwError = LW_ERROR_NOT_SUPPORTED;
        break;
    }

cleanup:

    LsaDmLdapClose(pConn);

    return dwError;

error:

    *ppNSSArtefactInfo = NULL;

    goto cleanup;
}

DWORD
AD_OnlineEnumNSSArtefacts(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;
    PAD_ENUM_STATE pEnumState = (PAD_ENUM_STATE)hResume;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;

    dwError = LsaDmLdapOpenDc(
                    gpADProviderData->szDomain,
                    &pConn);
    BAIL_ON_LSA_ERROR(dwError);

    switch (gpADProviderData->dwDirectoryMode)
    {
    case DEFAULT_MODE:
        dwError = DefaultModeEnumNSSArtefacts(
                pConn,
                gpADProviderData->cell.szCellDN,
                gpADProviderData->szShortDomain,
                pEnumState,
                dwMaxNSSArtefacts,
                pdwNSSArtefactsFound,
                pppNSSArtefactInfoList
                );
        break;

    case CELL_MODE:
        dwError = CellModeEnumNSSArtefacts(
                pConn,
                gpADProviderData->cell.szCellDN,
                gpADProviderData->szShortDomain,
                pEnumState,
                dwMaxNSSArtefacts,
                pdwNSSArtefactsFound,
                pppNSSArtefactInfoList
                );
        break;

    case UNPROVISIONED_MODE:

        dwError = LW_ERROR_NOT_SUPPORTED;
        break;
    }

cleanup:

    LsaDmLdapClose(pConn);

    return dwError;

error:

    *pdwNSSArtefactsFound = 0;
    *pppNSSArtefactInfoList = NULL;

    goto cleanup;
}

DWORD
AD_UpdateUserObjectFlags(
    IN OUT PLSA_SECURITY_OBJECT pUser
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    struct timeval current_tv = {0};
    UINT64 u64current_NTtime = 0;

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }
    ADConvertTimeUnix2Nt(current_tv.tv_sec,
                         &u64current_NTtime);

    if (pUser->userInfo.bIsAccountInfoKnown)
    {
        if (pUser->userInfo.qwAccountExpires != 0LL &&
            pUser->userInfo.qwAccountExpires != 9223372036854775807LL &&
            u64current_NTtime >= pUser->userInfo.qwAccountExpires)
        {
            pUser->userInfo.bAccountExpired = TRUE;
        }

        if ((!pUser->userInfo.bPasswordNeverExpires &&
             pUser->userInfo.qwPwdExpires != 0 &&
             u64current_NTtime >= pUser->userInfo.qwPwdExpires) ||
            pUser->userInfo.qwPwdLastSet == 0)
        {
            //password is expired already
            pUser->userInfo.bPasswordExpired = TRUE;
        }
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_VerifyUserAccountCanLogin(
    IN PLSA_SECURITY_OBJECT pUserInfo
    )
{
    DWORD dwError = 0;

    dwError = AD_UpdateUserObjectFlags(
                pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pUserInfo->userInfo.bAccountDisabled) {
        dwError = LW_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->userInfo.bAccountLocked) {
        dwError = LW_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->userInfo.bAccountExpired) {
        dwError = LW_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->userInfo.bPasswordExpired) {
        dwError = LW_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
AD_OnlineFindObjectByName(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN PCSTR pszLoginName,
    IN PLSA_LOGIN_NAME_INFO pUserNameInfo,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pCachedUser = NULL;

    switch(ObjectType)
    {
    case LSA_OBJECT_TYPE_USER:
        dwError = ADCacheFindUserByName(
            gpLsaAdProviderState->hCacheConnection,
            pUserNameInfo,
            &pCachedUser);
        break;
    case LSA_OBJECT_TYPE_GROUP:
        dwError = ADCacheFindGroupByName(
            gpLsaAdProviderState->hCacheConnection,
            pUserNameInfo,
            &pCachedUser);
        break;
    default:
        dwError = ADCacheFindUserByName(
            gpLsaAdProviderState->hCacheConnection,
            pUserNameInfo,
            &pCachedUser);
        if (dwError == LW_ERROR_NO_SUCH_USER ||
            dwError == LW_ERROR_NOT_HANDLED)
        {
            dwError = ADCacheFindGroupByName(
                gpLsaAdProviderState->hCacheConnection,
                pUserNameInfo,
                &pCachedUser);
        }
        break;
    }
    
    if (dwError == LW_ERROR_SUCCESS)
    {
        dwError = AD_CheckExpiredObject(&pCachedUser);
    }
    
    switch (dwError)
    {
    case LW_ERROR_SUCCESS:
        break;
    case LW_ERROR_NOT_HANDLED:
    case LW_ERROR_NO_SUCH_USER:
    case LW_ERROR_NO_SUCH_GROUP:
    case LW_ERROR_NO_SUCH_OBJECT:
        dwError = AD_FindObjectByNameTypeNoCache(
            hProvider,
            pszLoginName,
            pUserNameInfo->nameType,
            ObjectType,
            &pCachedUser);
        switch (dwError)
        {
        case LW_ERROR_SUCCESS:
            dwError = ADCacheStoreObjectEntry(
                gpLsaAdProviderState->hCacheConnection,
                pCachedUser);
            BAIL_ON_LSA_ERROR(dwError);
            
            break;
        case LW_ERROR_NO_SUCH_USER:
        case LW_ERROR_NO_SUCH_GROUP:
        case LW_ERROR_NO_SUCH_OBJECT:
        case LW_ERROR_DOMAIN_IS_OFFLINE:
            dwError = LW_ERROR_SUCCESS;
            break;
        default:
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        break;
    default:
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppObject = pCachedUser;

cleanup:

    return dwError;

error:

    *ppObject = NULL;

    if (pCachedUser)
    {
        LsaUtilFreeSecurityObject(pCachedUser);
    }

    goto cleanup;
}

static
DWORD
AD_OnlineFindObjectsByName(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pUserNameInfo = NULL;
    PSTR  pszLoginId_copy = NULL;
    DWORD dwIndex = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_TYPE type = LSA_QUERY_TYPE_UNDEFINED;

    dwError = LwAllocateMemory(sizeof(*ppObjects) * dwCount, OUT_PPVOID(&ppObjects));
    BAIL_ON_LSA_ERROR(dwError);

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        dwError = LwAllocateString(
            QueryList.ppszStrings[dwIndex],
            &pszLoginId_copy);
        BAIL_ON_LSA_ERROR(dwError);

        LwStrCharReplace(
            pszLoginId_copy,
            AD_GetSpaceReplacement(),
            ' ');
        
        dwError = LsaCrackDomainQualifiedName(
            pszLoginId_copy,
            gpADProviderData->szDomain,
            &pUserNameInfo);
        BAIL_ON_LSA_ERROR(dwError);

        switch(pUserNameInfo->nameType)
        {
        case NameType_NT4:
            type = LSA_QUERY_TYPE_BY_NT4;
            break;
        case NameType_UPN:
            type = LSA_QUERY_TYPE_BY_UPN;
            break;
        case NameType_Alias:
            type = LSA_QUERY_TYPE_BY_ALIAS;
            break;
        }

        if (type != QueryType)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = AD_OnlineFindObjectByName(
            hProvider,
            FindFlags,
            ObjectType,
            QueryType,
            pszLoginId_copy,
            pUserNameInfo,
            &ppObjects[dwIndex]);

        switch (dwError)
        {
        case LW_ERROR_SUCCESS:
            break;
        case LW_ERROR_NOT_HANDLED:
        case LW_ERROR_NO_SUCH_USER:
        case LW_ERROR_NO_SUCH_GROUP:
        case LW_ERROR_NO_SUCH_OBJECT:
        case LW_ERROR_NOT_SUPPORTED:
            ppObjects[dwIndex] = NULL;
            dwError = LW_ERROR_SUCCESS;
            
            if (QueryType == LSA_QUERY_TYPE_BY_ALIAS &&
                AD_ShouldAssumeDefaultDomain())
            {
                LW_SAFE_FREE_STRING(pszLoginId_copy);
                LsaFreeNameInfo(pUserNameInfo);
                pUserNameInfo = NULL;

                dwError = LwAllocateStringPrintf(
                    &pszLoginId_copy,
                    "%s\\%s",
                    gpADProviderData->szShortDomain,
                    QueryList.ppszStrings[dwIndex]);
                BAIL_ON_LSA_ERROR(dwError);

                LwStrCharReplace(
                    pszLoginId_copy,
                    AD_GetSpaceReplacement(),
                    ' ');

                dwError = LsaCrackDomainQualifiedName(
                    pszLoginId_copy,
                    gpADProviderData->szDomain,
                    &pUserNameInfo);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = AD_OnlineFindObjectByName(
                    hProvider,
                    FindFlags,
                    ObjectType,
                    LSA_QUERY_TYPE_BY_NT4,
                    pszLoginId_copy,
                    pUserNameInfo,
                    &ppObjects[dwIndex]);
                switch (dwError)
                {
                case LW_ERROR_SUCCESS:
                    break;
                case LW_ERROR_NOT_HANDLED:
                case LW_ERROR_NO_SUCH_USER:
                case LW_ERROR_NO_SUCH_GROUP:
                case LW_ERROR_NO_SUCH_OBJECT:
                    ppObjects[dwIndex] = NULL;
                    dwError = LW_ERROR_SUCCESS;
                    break;
                default:
                    BAIL_ON_LSA_ERROR(dwError);
                }
            }
            break;
        default:
            BAIL_ON_LSA_ERROR(dwError);
        }

        LW_SAFE_FREE_STRING(pszLoginId_copy);
        LsaFreeNameInfo(pUserNameInfo);
        pUserNameInfo = NULL;
    }

    *pppObjects = ppObjects;

cleanup:

    LW_SAFE_FREE_STRING(pszLoginId_copy);

    if (pUserNameInfo)
    {
        LsaFreeNameInfo(pUserNameInfo);
    }

    return dwError;

error:

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwCount, ppObjects);
    }

    goto cleanup;
}

static
DWORD
AD_OnlineFindObjectsById(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pCachedUser = NULL;
    DWORD dwIndex = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    dwError = LwAllocateMemory(sizeof(*ppObjects) * dwCount, OUT_PPVOID(&ppObjects));
    BAIL_ON_LSA_ERROR(dwError);

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        switch(ObjectType)
        {
        case LSA_OBJECT_TYPE_USER:
            dwError = ADCacheFindUserById(
                gpLsaAdProviderState->hCacheConnection,
                QueryList.pdwIds[dwIndex],
                &pCachedUser);
            break;
        case LSA_OBJECT_TYPE_GROUP:
            dwError = ADCacheFindGroupById(
                gpLsaAdProviderState->hCacheConnection,
                QueryList.pdwIds[dwIndex],
                &pCachedUser);
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
        if (dwError == LW_ERROR_SUCCESS)
        {
            dwError = AD_CheckExpiredObject(&pCachedUser);
        }
        
        switch (dwError)
        {
        case LW_ERROR_SUCCESS:
            ppObjects[dwIndex] = pCachedUser;
            pCachedUser = NULL;
            break;
        case LW_ERROR_NOT_HANDLED:
        case LW_ERROR_NO_SUCH_USER:
        case LW_ERROR_NO_SUCH_GROUP:
        case LW_ERROR_NO_SUCH_OBJECT:
            dwError = AD_FindObjectByIdTypeNoCache(
                hProvider,
                QueryList.pdwIds[dwIndex],
                ObjectType,
                &pCachedUser);
            switch (dwError)
            {
            case LW_ERROR_SUCCESS:
                dwError = ADCacheStoreObjectEntry(
                    gpLsaAdProviderState->hCacheConnection,
                    pCachedUser);
                BAIL_ON_LSA_ERROR(dwError);
                
                ppObjects[dwIndex] = pCachedUser;
                pCachedUser = NULL;
                break;
            case LW_ERROR_NO_SUCH_USER:
            case LW_ERROR_NO_SUCH_GROUP:
            case LW_ERROR_NO_SUCH_OBJECT:
            case LW_ERROR_DOMAIN_IS_OFFLINE:
                dwError = LW_ERROR_SUCCESS;
                break;
            default:
                BAIL_ON_LSA_ERROR(dwError);
                break;
            }
            break;
        default:
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *pppObjects = ppObjects;

cleanup:

    return dwError;

error:

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwCount, ppObjects);
    }

    goto cleanup;
}

static
DWORD
AD_OnlineDistributeObjects(
    BOOLEAN bByDn,
    IN DWORD dwKeyCount,
    IN PSTR* ppszKeys,
    IN DWORD dwObjectCount,
    IN PLSA_SECURITY_OBJECT* ppObjects,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    DWORD dwKeyIndex = 0;
    DWORD dwObjectIndex = 0;
    PLSA_SECURITY_OBJECT* ppDistObjects = NULL;

    dwError = LwAllocateMemory(
        sizeof(*ppDistObjects) * dwKeyCount,
        OUT_PPVOID(&ppDistObjects));
    BAIL_ON_LSA_ERROR(dwError);

    for (dwObjectIndex = 0; dwObjectIndex < dwObjectCount; dwObjectIndex++)
    {
        for (dwKeyIndex = 0; dwKeyIndex < dwKeyCount; dwKeyIndex++)
        {
            if (ppDistObjects[dwKeyIndex] == NULL &&
                !strcmp(bByDn ? 
                        ppObjects[dwObjectIndex]->pszDN :
                        ppObjects[dwObjectIndex]->pszObjectSid,
                        ppszKeys[dwKeyIndex]))
            {
                ppDistObjects[dwKeyIndex] = ppObjects[dwObjectIndex];
                ppObjects[dwObjectIndex] = NULL;
                break;
            }
        }
    }

    *pppObjects = ppDistObjects;
    
cleanup:

    return dwError;

error:

    goto cleanup;
}


DWORD
AD_OnlineFindObjects(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppUnorderedObjects = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_OBJECT_TYPE type = LSA_OBJECT_TYPE_UNDEFINED;
    DWORD dwIndex = 0;
    size_t sObjectCount = 0;

    switch(QueryType)
    {
    case LSA_QUERY_TYPE_BY_SID:
        dwError = AD_FindObjectsBySidList(
            hProvider,
            dwCount,
            (PSTR*) QueryList.ppszStrings,
            &sObjectCount,
            &ppUnorderedObjects);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_OnlineDistributeObjects(
            FALSE,
            dwCount,
            (PSTR*) QueryList.ppszStrings,
            (DWORD) sObjectCount,
            ppUnorderedObjects,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case LSA_QUERY_TYPE_BY_DN:
         dwError = AD_FindObjectsByDNList(
            hProvider,
            dwCount,
            (PSTR*) QueryList.ppszStrings,
            &sObjectCount,
            &ppUnorderedObjects);
         BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_OnlineDistributeObjects(
            TRUE,
            dwCount,
            (PSTR*) QueryList.ppszStrings,
            (DWORD) sObjectCount,
            ppUnorderedObjects,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
         break;
    case LSA_QUERY_TYPE_BY_NT4:
    case LSA_QUERY_TYPE_BY_UPN:
    case LSA_QUERY_TYPE_BY_ALIAS:
        dwError = AD_OnlineFindObjectsByName(
            hProvider,
            FindFlags,
            ObjectType,
            QueryType,
            dwCount,
            QueryList,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case LSA_QUERY_TYPE_BY_UNIX_ID:
        dwError = AD_OnlineFindObjectsById(
            hProvider,
            FindFlags,
            ObjectType,
            QueryType,
            dwCount,
            QueryList,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        if (ppObjects[dwIndex])
        {
            switch (ppObjects[dwIndex]->type)
            {
            case LSA_OBJECT_TYPE_GROUP:
                type = LSA_OBJECT_TYPE_GROUP;
                break;
            case LSA_OBJECT_TYPE_USER:
                type = LSA_OBJECT_TYPE_USER;
                break;
                /*
            case LSA_OBJECT_TYPE_DOMAIN:
                type = LSA_OBJECT_TYPE_DOMAIN;
                break;
                */
            }

            if (ObjectType != LSA_OBJECT_TYPE_UNDEFINED && type != ObjectType)
            {
                LsaUtilFreeSecurityObject(ppObjects[dwIndex]);
                ppObjects[dwIndex] = NULL;
            }
        }
    }

    *pppObjects = ppObjects;

cleanup:

    if (ppUnorderedObjects)
    {
        LsaUtilFreeSecurityObjectList((DWORD) sObjectCount, ppUnorderedObjects);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_OnlineEnumObjects(
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PAD_ENUM_HANDLE pEnum = hEnum;
    BOOLEAN bIsEnumerationEnabled = TRUE;
    LSA_FIND_FLAGS FindFlags = pEnum->FindFlags;

    if (FindFlags & LSA_FIND_FLAGS_NSS)
    {
        bIsEnumerationEnabled = AD_GetNssEnumerationEnabled();
    }

    if (!bIsEnumerationEnabled || pEnum->CurrentObjectType == LSA_OBJECT_TYPE_UNDEFINED)
    {
        dwError = ERROR_NO_MORE_ITEMS;
        goto cleanup;
    }

    do
    {
        switch (pEnum->CurrentObjectType)
        {
        case LSA_OBJECT_TYPE_USER:
            dwError = LsaAdBatchEnumObjects(
                &pEnum->Cookie,
                LSA_OBJECT_TYPE_USER,
                dwMaxObjectsCount,
                pdwObjectsCount,
                pppObjects);
            break;
        case LSA_OBJECT_TYPE_GROUP:
            dwError = LsaAdBatchEnumObjects(
                &pEnum->Cookie,
                LSA_OBJECT_TYPE_GROUP,
                dwMaxObjectsCount,
                pdwObjectsCount,
                pppObjects);
            break;
        }

        if ((dwError == LW_ERROR_NO_MORE_USERS ||
             dwError == LW_ERROR_NO_MORE_GROUPS))
        {
            dwError = ERROR_NO_MORE_ITEMS;

            if (pEnum->ObjectType == LSA_OBJECT_TYPE_UNDEFINED &&
                pEnum->CurrentObjectType < LSA_OBJECT_TYPE_GROUP)
            {
                pEnum->CurrentObjectType++;
                LwFreeCookieContents(&pEnum->Cookie);
                LwInitCookie(&pEnum->Cookie);
                continue;
            }
            else
            {
                pEnum->CurrentObjectType = LSA_OBJECT_TYPE_UNDEFINED;
                break;
            }
        }
    } while (dwError == ERROR_NO_MORE_ITEMS);
    
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    *pdwObjectsCount = 0;
    *pppObjects = NULL;

    goto cleanup;
}

static
DWORD
AD_OnlineQueryMemberOfForSid(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN PSTR pszSid,
    IN OUT PLSA_HASH_TABLE pGroupHash
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sMembershipCount = 0;
    PLSA_GROUP_MEMBERSHIP* ppMemberships = NULL;
    BOOLEAN bIsCacheOnlyMode = FALSE;
    BOOLEAN bExpired = FALSE;
    BOOLEAN bIsComplete = FALSE;
    BOOLEAN bUseCache = FALSE;
    size_t sResultsCount = 0;
    PLSA_SECURITY_OBJECT* ppResults = NULL;
    // Only free top level array, do not free string pointers.
    PSTR pszGroupSid = NULL;
    int iPrimaryGroupIndex = -1;
    PLSA_SECURITY_OBJECT pUserInfo = NULL;
    DWORD dwIndex = 0;

    if (FindFlags & LSA_FIND_FLAGS_NSS)
    {
        bIsCacheOnlyMode = AD_GetNssUserMembershipCacheOnlyEnabled();
    }

    dwError = AD_FindObjectBySid(hProvider, pszSid, &pUserInfo);
    if (dwError == LW_ERROR_NO_SUCH_OBJECT)
    {
        /* Skip over unknown SIDs without failing */
        dwError = LW_ERROR_SUCCESS;
        goto cleanup;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheGetGroupsForUser(
                    gpLsaAdProviderState->hCacheConnection,
                    pszSid,
                    AD_GetTrimUserMembershipEnabled(),
                    &sMembershipCount,
                    &ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_CheckExpiredMemberships(
                    sMembershipCount,
                    ppMemberships,
                    TRUE,
                    &bExpired,
                    &bIsComplete);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExpired)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for user's group membership for sid %s is expired",
            pszSid);
    }
    else if (!bIsComplete)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for user's group membership for sid %s is incomplete",
            pszSid);
    }

    if (bExpired || !bIsComplete)
    {
        LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;

        dwError = AD_DetermineTrustModeandDomainName(
                        pUserInfo->pszNetbiosDomainName,
                        &dwTrustDirection,
                        NULL,
                        NULL,
                        NULL);
        BAIL_ON_LSA_ERROR(dwError);

        if (dwTrustDirection == LSA_TRUST_DIRECTION_ONE_WAY)
        {
            bUseCache = TRUE;
        }
    }
    else
    {
        bUseCache = TRUE;
    }

    if (!bUseCache && bIsCacheOnlyMode)
    {
        dwError = AD_FilterExpiredMemberships(&sMembershipCount, ppMemberships);
        BAIL_ON_LSA_ERROR(dwError);

        bUseCache = TRUE;
    }

    if (!bUseCache)
    {
        dwError = ADLdap_GetObjectGroupMembership(
                         hProvider,
                         pUserInfo,
                         &iPrimaryGroupIndex,
                         &sResultsCount,
                         &ppResults);
        BAIL_ON_LSA_ERROR(dwError);

        for (dwIndex = 0; dwIndex < sResultsCount; dwIndex++)
        {
            if (ppResults[dwIndex] &&
                AdIsSpecialDomainSidPrefix(ppResults[dwIndex]->pszObjectSid))
            {
                ADCacheSafeFreeObject(&ppResults[dwIndex]);
            }
        }

        AD_FilterNullEntries(ppResults, &sResultsCount);

        dwError = AD_CacheMembershipFromRelatedObjects(
                        gpLsaAdProviderState->hCacheConnection,
                        pszSid,
                        iPrimaryGroupIndex,
                        FALSE,
                        sResultsCount,
                        ppResults);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (ppMemberships)
    {
        for (dwIndex = 0; dwIndex < sMembershipCount; dwIndex++)
        {
            if (ppMemberships[dwIndex]->pszParentSid &&
                !LsaHashExists(pGroupHash, ppMemberships[dwIndex]->pszParentSid) &&
                (ppMemberships[dwIndex]->bIsInPac ||
                 ppMemberships[dwIndex]->bIsDomainPrimaryGroup ||
                 bUseCache))
            {

                dwError = LwAllocateString(
                    ppMemberships[dwIndex]->pszParentSid,
                    &pszGroupSid);
                BAIL_ON_LSA_ERROR(dwError);
                
                dwError = LsaHashSetValue(pGroupHash, pszGroupSid, pszGroupSid);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = AD_OnlineQueryMemberOfForSid(
                    hProvider,
                    FindFlags,
                    pszGroupSid,
                    pGroupHash);
                pszGroupSid = NULL;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    if (ppResults)
    {
        for (dwIndex = 0; dwIndex < sResultsCount; dwIndex++)
        {
            if (!LsaHashExists(pGroupHash, ppResults[dwIndex]->pszObjectSid))
            {
                dwError = LwAllocateString(
                    ppResults[dwIndex]->pszObjectSid,
                    &pszGroupSid);
                BAIL_ON_LSA_ERROR(dwError);
                
                dwError = LsaHashSetValue(pGroupHash, pszGroupSid, pszGroupSid);
                BAIL_ON_LSA_ERROR(dwError);
                
                dwError = AD_OnlineQueryMemberOfForSid(
                    hProvider,
                    FindFlags,
                    pszGroupSid,
                    pGroupHash);
                pszGroupSid = NULL;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pszGroupSid);
    ADCacheSafeFreeObject(&pUserInfo);
    ADCacheSafeFreeGroupMembershipList(sMembershipCount, &ppMemberships);
    ADCacheSafeFreeObjectList(sResultsCount, &ppResults);

    return dwError;

error:

    if ( dwError != LW_ERROR_DOMAIN_IS_OFFLINE && pUserInfo )
    {
        LSA_LOG_ERROR("Failed to find memberships for user '%s\\%s' (error = %d)",
                      pUserInfo->pszNetbiosDomainName,
                      pUserInfo->pszSamAccountName,
                      dwError);
    }

    goto cleanup;
}

static
VOID
AD_OnlineFreeMemberOfHashEntry(
    const LSA_HASH_ENTRY* pEntry
    )
{
    if (pEntry->pValue)
    {
        LwFreeMemory(pEntry->pValue);
    }
}

DWORD
AD_OnlineQueryMemberOf(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    PLSA_HASH_TABLE   pGroupHash = NULL;
    LSA_HASH_ITERATOR hashIterator = {0};
    LSA_HASH_ENTRY*   pHashEntry = NULL;
    DWORD dwGroupSidCount = 0;
    PSTR* ppszGroupSids = NULL;

    dwError = LsaHashCreate(
        13,
        LsaHashCaselessStringCompare,
        LsaHashCaselessStringHash,
        AD_OnlineFreeMemberOfHashEntry,
        NULL,
        &pGroupHash);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (dwIndex = 0; dwIndex < dwSidCount; dwIndex++)
    {
        if (AdIsSpecialDomainSidPrefix(ppszSids[dwIndex]))
        {
            continue;
        }

        dwError = AD_OnlineQueryMemberOfForSid(
            hProvider,
            FindFlags,
            ppszSids[dwIndex],
            pGroupHash);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwGroupSidCount = (DWORD) LsaHashGetKeyCount(pGroupHash);
    
    if (dwGroupSidCount)
    {
        dwError = LwAllocateMemory(
            sizeof(*ppszGroupSids) * dwGroupSidCount,
            OUT_PPVOID(&ppszGroupSids));
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = LsaHashGetIterator(pGroupHash, &hashIterator);
        BAIL_ON_LSA_ERROR(dwError);
        
        for(dwIndex = 0; (pHashEntry = LsaHashNext(&hashIterator)) != NULL; dwIndex++)
        {
            ppszGroupSids[dwIndex] = (PSTR) pHashEntry->pValue;
            pHashEntry->pValue = NULL;
        }
    }

    *pdwGroupSidCount = dwGroupSidCount;
    *pppszGroupSids = ppszGroupSids;

cleanup:

    LsaHashSafeFree(&pGroupHash);

    return dwError;

error:

    if (ppszGroupSids)
    {
        LwFreeStringArray(ppszGroupSids, dwGroupSidCount);
    }

    goto cleanup;
}

DWORD
AD_OnlineGetGroupMemberSids(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid,
    OUT PDWORD pdwSidCount,
    OUT PSTR** pppszSids
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sMembershipCount = 0;
    PLSA_GROUP_MEMBERSHIP* ppMemberships = NULL;
    BOOLEAN bExpired = FALSE;
    BOOLEAN bIsComplete = FALSE;
    BOOLEAN bUseCache = FALSE;
    BOOLEAN bIsCacheOnlyMode = FALSE;
    size_t sResultsCount = 0;
    PLSA_SECURITY_OBJECT* ppResults = NULL;
    // Only free top level array, do not free string pointers.
    PSTR* ppszSids = NULL;
    DWORD dwSidCount = 0;
    DWORD dwIndex = 0;
    PLSA_SECURITY_OBJECT pGroupInfo = NULL;

    if (FindFlags & LSA_FIND_FLAGS_NSS)
    {
        bIsCacheOnlyMode = AD_GetNssGroupMembersCacheOnlyEnabled();
    }

    dwError = AD_FindObjectBySid(hProvider, pszSid, &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheGetGroupMembers(
                    gpLsaAdProviderState->hCacheConnection,
                    pszSid,
                    AD_GetTrimUserMembershipEnabled(),
                    &sMembershipCount,
                    &ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_CheckExpiredMemberships(
                    sMembershipCount,
                    ppMemberships,
                    FALSE,
                    &bExpired,
                    &bIsComplete);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExpired)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for group membership for sid %s is expired",
            pszSid);
    }
    else if (!bIsComplete)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for group membership for sid %s is incomplete",
            pszSid);
    }

    if (bExpired || !bIsComplete)
    {
        LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;

        dwError = AD_DetermineTrustModeandDomainName(
            pGroupInfo->pszNetbiosDomainName,
            &dwTrustDirection,
            NULL,
            NULL,
            NULL);
        BAIL_ON_LSA_ERROR(dwError);

        if (dwTrustDirection == LSA_TRUST_DIRECTION_ONE_WAY)
        {
            bUseCache = TRUE;
        }
    }
    else
    {
        bUseCache = TRUE;
    }

    if (!bUseCache && bIsCacheOnlyMode)
    {
        dwError = AD_FilterExpiredMemberships(&sMembershipCount, ppMemberships);
        BAIL_ON_LSA_ERROR(dwError);

        bUseCache = TRUE;
    }

    if (!bUseCache)
    {
        dwError = ADLdap_GetGroupMembers(
                        hProvider,
                        pGroupInfo->pszNetbiosDomainName,
                        pszSid,
                        &sResultsCount,
                        &ppResults);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_CacheMembershipFromRelatedObjects(
                        gpLsaAdProviderState->hCacheConnection,
                        pszSid,
                        -1,
                        TRUE,
                        sResultsCount,
                        ppResults);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
        sizeof(*ppszSids) * (sResultsCount + sMembershipCount),
        OUT_PPVOID(&ppszSids));

    if (ppMemberships)
    {
        for (dwIndex = 0; dwIndex < sMembershipCount; dwIndex++)
        {
            if (ppMemberships[dwIndex]->pszChildSid)
            {
                dwError = LwAllocateString(ppMemberships[dwIndex]->pszChildSid, &ppszSids[dwSidCount++]);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    if (ppResults)
    {
        for (dwIndex = 0; dwIndex < sResultsCount; dwIndex++)
        {
            if (ppResults[dwIndex])
            {
                dwError = LwAllocateString(ppResults[dwIndex]->pszObjectSid, &ppszSids[dwSidCount++]);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    *pdwSidCount = dwSidCount;
    *pppszSids = ppszSids;

cleanup:

    ADCacheSafeFreeGroupMembershipList(sMembershipCount, &ppMemberships);
    ADCacheSafeFreeObjectList(sResultsCount, &ppResults);
    ADCacheSafeFreeObject(&pGroupInfo);
    
    return dwError;

error:

    *pdwSidCount = 0;
    *pppszSids = NULL;

    if (ppszSids)
    {
        LwFreeStringArray(ppszSids, dwSidCount);
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
