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
AD_PrescreenUserName(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszLoginId
    );

static
DWORD
AD_PrescreenGroupName(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszGroupName
    );

static
DWORD
AD_CheckExpiredMemberships(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN size_t sCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMemberships,
    IN BOOLEAN bCheckNullParentSid,
    OUT PBOOLEAN pbHaveExpired,
    OUT PBOOLEAN pbIsComplete
    );

static
DWORD
AD_UpdateRdrDomainHints(
    PLSA_AD_PROVIDER_STATE pState
    );

static
DWORD
AD_FindObjectBySidNoCache(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszSid,
    OUT PLSA_SECURITY_OBJECT* ppObject
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
AD_OnlineFinishInitializeDomainTrustsInfo(
    IN PLSA_AD_PROVIDER_STATE pState,
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

    //
    // Combine discovered state with previously stored 1-way child trusts
    // (which are not directly discoverable).
    //

    dwError = ADState_GetDomainTrustList(
                  pState->pszDomainName,
                  &pDomains);
    BAIL_ON_LSA_ERROR(dwError);

    pPos = pDomains;
    while (pPos != NULL)
    {
        pDomain = (PLSA_DM_ENUM_DOMAIN_INFO)pPos->pItem;

        if (!pDomain
            || !IsSetFlag(pDomain->Flags, LSA_DM_DOMAIN_FLAG_TRANSITIVE_1WAY_CHILD)
            || LsaDmIsDomainPresent(pState->hDmState, pDomain->pszDnsDomainName)
           )
        {
            pPos = pPos->pNext;
            continue;
        }

        dwError = LsaDmWrapNetLookupObjectSidByName(
                     pState->hDmState,
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
            pState->hDmState,
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

    //
    // Saved combined information into store.
    //

    dwError = LsaDmEnumDomainInfo(
                pState->hDmState,
                NULL,
                NULL,
                &ppDomainInfo,
                &dwDomainInfoCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_StoreDomainTrustList(
                pState->pszDomainName,
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
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszDomain,
    IN PCSTR pszHostName
    )
{
    DWORD dwError = 0;
    PLSA_AD_PROVIDER_STATE pState = pContext->pState;
    PSTR  pszComputerDN = NULL;
    PSTR  pszCellDN = NULL;
    PSTR  pszRootDN = NULL;
    ADConfigurationMode adConfMode = UnknownMode;
    PAD_PROVIDER_DATA pProviderData = NULL;
    PSTR pszNetbiosDomainName = NULL;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;
    AD_CELL_SUPPORT adCellSupport = AD_CELL_SUPPORT_FULL;

    dwError = LwKrb5SetThreadDefaultCachePath(pState->MachineCreds.pszCachePath, NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pProviderData), (PVOID*)&pProviderData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmEngineDiscoverTrusts(
                  pState->hDmState,
                  pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmLdapOpenDc(
                    pContext,
                    pszDomain,
                    &pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwLdapConvertDomainToDN(pszDomain, &pszRootDN);
    BAIL_ON_LSA_ERROR(dwError);

    adCellSupport = AD_GetCellSupport(pState);
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
            dwError = ADFindComputerDN(
                            pConn,
                            pszHostName,
                            pszDomain,
                            &pszComputerDN);
            BAIL_ON_LSA_ERROR(dwError);

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

    dwError = LsaDmWrapGetDomainName(
                  pState->hDmState,
                  pszDomain,
                  NULL,
                  &pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    strcpy(pProviderData->szDomain, pszDomain);
    if (pszComputerDN)
    {
        strcpy(pProviderData->szComputerDN, pszComputerDN);
    }
    strcpy(pProviderData->szShortDomain, pszNetbiosDomainName);

    pProviderData->adConfigurationMode = adConfMode;

    if (pProviderData->dwDirectoryMode == CELL_MODE)
    {
        dwError = AD_GetLinkedCellInfo(
                    pContext,
                    pConn,
                    pszCellDN,
                    pszDomain,
                    &pProviderData->pCellList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADState_StoreProviderData(
                pState->pszDomainName,
                pProviderData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_OnlineFinishInitializeDomainTrustsInfo(
                pState,
                pProviderData->szDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_UpdateRdrDomainHints(pState);
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

    LsaDmResetTrusts(pState->hDmState);

    goto cleanup;
}

static
DWORD
AD_UpdateRdrDomainHints(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszDomains = NULL;
    PLSA_DM_ENUM_DOMAIN_INFO* ppInfo = NULL;
    DWORD dwCount = 0;
    DWORD dwIndex = 0;
    ULONG ulFinalCount = 0;

    if (!pState->bIsDefault)
    {
        goto error;
    }

    dwError = LsaDmEnumDomainInfo(pState->hDmState, NULL, NULL, &ppInfo, &dwCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*ppwszDomains) * dwCount, OUT_PPVOID(&ppwszDomains));
    BAIL_ON_LSA_ERROR(dwError);

    for (dwIndex = 0; dwIndex < dwCount;)
    {
        if (ppInfo[dwIndex]->pszDnsDomainName)
        {
            dwError = LwAllocateWc16sPrintfW(
                &ppwszDomains[dwIndex],
                L"%s:%s",
                ppInfo[dwIndex]->pszDnsDomainName,
                ppInfo[dwIndex]->pszNetbiosDomainName);
            BAIL_ON_LSA_ERROR(dwError);
            dwIndex++;
        }
    }
    ulFinalCount = (ULONG) dwIndex;

    dwError = LwNtStatusToWin32Error(LwIoSetRdrDomainHints(ppwszDomains, ulFinalCount));
    if (dwError)
    {
        /* Do not fail, just log an error */
        LSA_LOG_ERROR("Could not update rdr domain hints: %s", LwWin32ExtErrorToName(dwError));
        dwError = 0;
    }

cleanup:

    if (ppwszDomains)
    {
        for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            LwFreeMemory(ppwszDomains[dwIndex]);
        }
        LwFreeMemory(ppwszDomains);
    }

    LsaDmFreeEnumDomainInfoArray(ppInfo);

    return dwError;

error:

    goto cleanup;
}


DWORD
AD_GetLinkedCellInfo(
    IN PAD_PROVIDER_CONTEXT pContext,
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
    PSTR  pszCellDirectoryRoot = NULL;
    PSTR  pszLinkedCellDN = NULL;
    PAD_LINKED_CELL_INFO pLinkedCellInfo = NULL;

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

        dwError = LsaDmLdapOpenGc(
                      pContext,
                      pszDomain,
                      &pGcConn);
        BAIL_ON_LSA_ERROR(dwError);

        pszLinkedCellGuid = strtok_r (pszLinkedCell, pszDelim, &pszStrTokSav);
        while (pszLinkedCellGuid != NULL)
        {
            PSTR  pszHexStr = NULL;
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
            pLinkedCellInfo = NULL;

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

    if (pLinkedCellInfo)
    {
        ADProviderFreeCellInfo(pLinkedCellInfo);
    }

    LsaDmLdapClose(pGcConn);

    LW_SAFE_FREE_STRING (pszCellDirectoryRoot);
    LW_SAFE_FREE_STRING (pszLinkedCellDN);
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
    IN PLSA_AD_PROVIDER_STATE pState,
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
        LW_IS_NULL_OR_EMPTY_STR(pState->pProviderData->szDomain) ||
        LW_IS_NULL_OR_EMPTY_STR(pState->pProviderData->szShortDomain))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

#if 0
    if (!strcasecmp(pState->pProviderData->szDomain, pszDomain) ||
        !strcasecmp(pState->pProviderData->szShortDomain, pszDomain))
    {
        dwTrustDirection = LSA_TRUST_DIRECTION_SELF;
        if (ppszDnsDomainName)
        {
            dwError = LwAllocateString(pState->pProviderData->szDomain,
                                        &pszDnsDomainName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        if (ppszNetbiosDomainName)
        {
            dwError = LwAllocateString(pState->pProviderData->szShortDomain,
                                        &pszNetbiosDomainName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        dwError = 0;
        goto cleanup;
    }
#endif

    dwError = LsaDmQueryDomainInfo(pState->hDmState,
                                   pszDomain,
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
    IN PAD_PROVIDER_CONTEXT pContext,
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
    LW_HASH_TABLE* pMembershipHashTable = NULL;
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

    dwError = LwHashCreate(
                    dwMembershipCount,
                    LwHashCaselessStringCompare,
                    LwHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pMembershipHashTable);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwMembershipCount; i++)
    {
        // Ignore the NULL entry
        if (ppMemberships[i]->pszParentSid)
        {
            dwError = LwHashSetValue(pMembershipHashTable,
                                      ppMemberships[i]->pszParentSid,
                                      ppMemberships[i]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = ADCacheGetGroupsForUser(
                    pContext->pState->hCacheConnection,
                    pUserInfo->pszObjectSid,
                    TRUE,
                    &sCacheMembershipCount,
                    &ppCacheMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_CheckExpiredMemberships(
                    pContext->pState,
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
            dwError = LwHashGetValue(pMembershipHashTable,
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
                "The user group membership information for user %s\\%s does not match what is in the cache, because the cache contains %zu memberships, but the pac contains %zu memberships. The group membership now needs to be compared against LDAP.",
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
                    pContext,
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

        dwError = LwHashGetValue(pMembershipHashTable,
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
    LwHashSafeFree(&pMembershipHashTable);
    return dwError;

error:
    goto cleanup;
}

DWORD
AD_CacheGroupMembershipFromPac(
    IN PAD_PROVIDER_CONTEXT pContext,
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

    if (AD_GetTrimUserMembershipEnabled(pContext->pState))
    {
        dwError = AD_PacMembershipFilterWithLdap(
                        pContext,
                        dwTrustDirection,
                        pUserInfo,
                        dwMembershipCount,
                        ppMemberships);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheStoreGroupsForUser(
                        pContext->pState->hCacheConnection,
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
            pContext->pState->hCacheConnection,
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
    IN PLSA_AD_PROVIDER_STATE pState,
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
               pState->hCacheConnection,
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
        wc16slen(pwszPassword) * sizeof(wchar16_t),
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

    LW_SECURE_FREE_WSTRING(pwszPassword);
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
    IN PLSA_AD_PROVIDER_STATE pState,
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
                pState->hCacheConnection,
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
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszNetBiosName,
    OUT PBOOLEAN pbFoundDomain
    )
{
    BOOLEAN bFoundDomain = FALSE;
    DWORD dwError = 0;

    bFoundDomain = AD_ServicesDomainInternal(
                       pState,
                       pszNetBiosName);

    if (!bFoundDomain)
    {
        dwError = LsaDmEngineGetDomainNameWithDiscovery(
                     pState->hDmState,
                     pState->pProviderData->szDomain,
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
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    IN PCSTR  pszPassword,
    OUT PDWORD pdwGoodUntilTime,
    OUT OPTIONAL PLSA_SECURITY_OBJECT *ppUpdatedUserInfo
    )
{
    DWORD dwError = 0;
    PSTR pszServicePrincipal = NULL;
    PSTR pszUpn = NULL;
    PSTR pszUserDnsDomainName = NULL;
    PSTR pszFreeUpn = NULL;
    PSTR pszUserRealm = NULL;
    PVOID pNdrEncodedPac = NULL;
    size_t sNdrEncodedPac = 0;
    PAC_LOGON_INFO *pPac = NULL;
    LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
    NTSTATUS ntStatus = 0;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;
    PLSA_SECURITY_OBJECT pUpdatedUserInfo = NULL;
    time_t now = 0;
    BOOLEAN passwordLocked = FALSE;

    dwError = AD_DetermineTrustModeandDomainName(
                        pContext->pState,
                        pUserInfo->pszNetbiosDomainName,
                        &dwTrustDirection,
                        NULL,
                        NULL,
                        NULL);
    BAIL_ON_LSA_ERROR(dwError);

    AD_LOCK_MACHINE_PASSWORD(
                    pContext->pState->hMachinePwdState,
                    passwordLocked);

    dwError = LsaPcacheGetMachinePasswordInfoA(
                  pContext->pState->pPcache,
                  &pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    // Leave the realm empty so that kerberos referrals are turned on.
    dwError = LwAllocateStringPrintf(
                        &pszServicePrincipal,
                        "host/%s@",
                        pPasswordInfo->Account.Fqdn);
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
                       pContext->pState->hDmState,
                       pContext->pState->pProviderData->szDomain,
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

    if (LsaDmIsDomainOffline(pContext->pState->hDmState, pszUserRealm))
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwKrb5InitializeUserLoginCredentials(
                    pszUpn,
                    pszPassword,
                    pUserInfo->userInfo.uid,
                    pUserInfo->userInfo.gid,
                    LW_KRB5_LOGIN_FLAG_UPDATE_CACHE,
                    pszServicePrincipal,
                    pPasswordInfo->Account.DnsDomainName,
                    pPasswordInfo->Password,
                    &pNdrEncodedPac,
                    &sNdrEncodedPac,
                    pdwGoodUntilTime);
    if (dwError == LW_ERROR_KRB5KDC_ERR_S_PRINCIPAL_UNKNOWN)
    {
        // Perhaps the host has no SPN.  Try again
        // using the UPN (sAMAccountName@REALM).

        LW_SAFE_FREE_STRING(pszServicePrincipal);

        dwError = LwAllocateStringPrintf(
                      &pszServicePrincipal,
                      "%s@%s",
                      pPasswordInfo->Account.SamAccountName,
                      pPasswordInfo->Account.DnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwKrb5InitializeUserLoginCredentials(
                        pszUpn,
                        pszPassword,
                        pUserInfo->userInfo.uid,
                        pUserInfo->userInfo.gid,
                        LW_KRB5_LOGIN_FLAG_UPDATE_CACHE,
                        pszServicePrincipal,
                        pPasswordInfo->Account.DnsDomainName,
                        pPasswordInfo->Password,
                        &pNdrEncodedPac,
                        &sNdrEncodedPac,
                        pdwGoodUntilTime);
    }

    AD_UNLOCK_MACHINE_PASSWORD(
                    pContext->pState->hMachinePwdState,
                    passwordLocked);
    
    if (dwError == LW_ERROR_DOMAIN_IS_OFFLINE)
    {
        LsaDmTransitionOffline(
            pContext->pState->hDmState,
            pszUserRealm,
            FALSE);
    }

    BAIL_ON_LSA_ERROR(dwError);

    if (sNdrEncodedPac)
    {
        // This function will abort if it is passed a zero sized PAC.
        ntStatus = DecodePacLogonInfo(
            pNdrEncodedPac,
            sNdrEncodedPac,
            &pPac);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppUpdatedUserInfo &&
        pUserInfo->version.tLastUpdated < now - AD_LOGIN_UPDATE_CACHE_ENTRY_SECS)
    {
        dwError = AD_FindObjectBySidNoCache(
                        pContext,
                        pUserInfo->pszObjectSid,
                        &pUpdatedUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = ADCacheDuplicateObject(
                        &pUpdatedUserInfo,
                        pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pPac != NULL)
    {
        dwError = AD_CacheGroupMembershipFromPac(
                        pContext,
                        dwTrustDirection,
                        pUpdatedUserInfo,
                        pPac);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_CacheUserRealInfoFromPac(
                        pContext->pState,
                        pUpdatedUserInfo,
                        pPac);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_ASSERT(pUpdatedUserInfo->userInfo.bIsAccountInfoKnown);
    }
    else
    {
        LSA_LOG_ERROR("no pac was received for %s\\%s (uid %u). The user's group memberships and password expiration may show up incorrectly on this machine.", 
                    LSA_SAFE_LOG_STRING(pUpdatedUserInfo->pszNetbiosDomainName),
                    LSA_SAFE_LOG_STRING(pUpdatedUserInfo->pszSamAccountName),
                    pUpdatedUserInfo->userInfo.uid);
    }

    if (ppUpdatedUserInfo)
    {
        *ppUpdatedUserInfo = pUpdatedUserInfo;
    }
    else
    {
        ADCacheSafeFreeObject(&pUpdatedUserInfo);
    }

cleanup:
    if (pPac)
    {
        FreePacLogonInfo(pPac);
    }
    LW_SAFE_FREE_STRING(pszServicePrincipal);
    LW_SAFE_FREE_STRING(pszUserDnsDomainName);
    LW_SAFE_FREE_STRING(pszFreeUpn);
    LW_SAFE_FREE_MEMORY(pNdrEncodedPac);

    LsaPcacheReleaseMachinePasswordInfoA(pPasswordInfo);
    AD_UNLOCK_MACHINE_PASSWORD(
                    pContext->pState->hMachinePwdState,
                    passwordLocked);

    return dwError;

error:
    *pdwGoodUntilTime = 0;
    if (ppUpdatedUserInfo)
    {
        *ppUpdatedUserInfo = NULL;
    }
    ADCacheSafeFreeObject(&pUpdatedUserInfo);

    goto cleanup;
}

DWORD
AD_OnlineAuthenticateUserPam(
    PAD_PROVIDER_CONTEXT pContext,
    LSA_AUTH_USER_PAM_PARAMS* pParams,
    PLSA_AUTH_USER_PAM_INFO* ppPamAuthInfo
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pUserInfo = NULL;
    PLSA_SECURITY_OBJECT pUpdatedUserInfo = NULL;
    DWORD dwGoodUntilTime = 0;
    PSTR pszNT4UserName = NULL;
    PLSA_AUTH_USER_PAM_INFO pPamAuthInfo = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*pPamAuthInfo),
                    (PVOID*)&pPamAuthInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_FindUserObjectByName(
                    pContext,
                    pParams->pszLoginName,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_OnlineCheckUserPassword(
                    pContext,
                    pUserInfo,
                    pParams->pszPassword,
                    &dwGoodUntilTime,
                    &pUpdatedUserInfo);
    if (dwError == LW_ERROR_ACCOUNT_DISABLED ||
        dwError == LW_ERROR_ACCOUNT_EXPIRED ||
        dwError == LW_ERROR_PASSWORD_EXPIRED)
    {
        // Fix up account disabled flag.
        if (dwError == LW_ERROR_ACCOUNT_DISABLED)
        {
            pUserInfo->userInfo.bAccountDisabled = TRUE;
        }

        // Fix up account expired flag.
        if (dwError == LW_ERROR_ACCOUNT_EXPIRED)
        {
            pUserInfo->userInfo.bAccountExpired = TRUE;
        }

        // Fix up account password expired flags.
        if (dwError == LW_ERROR_PASSWORD_EXPIRED)
        {
            pUserInfo->userInfo.bPasswordExpired = TRUE;
        }

        // Now update the cache entry with the changes.
        ADCacheStoreObjectEntry(pContext->pState->hCacheConnection, pUserInfo);
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_VerifyUserAccountCanLogin(
                pContext,
                pUpdatedUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_OnlineCachePasswordVerifier(
                    pContext->pState,
                    pUpdatedUserInfo,
                    pParams->pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
        &pszNT4UserName,
        "%s\\%s",
        pUpdatedUserInfo->pszNetbiosDomainName,
        pUpdatedUserInfo->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    if (pContext->pState->bIsDefault)
    {
        dwError = LsaUmAddUser(
                      pUpdatedUserInfo->userInfo.uid,
                      pszNT4UserName,
                      pParams->pszPassword,
                      dwGoodUntilTime);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pPamAuthInfo->bOnlineLogon = TRUE;

    *ppPamAuthInfo = pPamAuthInfo;

cleanup:
    LW_SAFE_FREE_STRING(pszNT4UserName);

    ADCacheSafeFreeObject(&pUserInfo);
    ADCacheSafeFreeObject(&pUpdatedUserInfo);

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
    IN PLSA_AD_PROVIDER_STATE pState,
    IN OUT PLSA_SECURITY_OBJECT* ppCachedUser
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    time_t now = 0;
    time_t expirationDate;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    expirationDate = (*ppCachedUser)->version.tLastUpdated +
        AD_GetCacheEntryExpirySeconds(pState);

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
    IN PLSA_AD_PROVIDER_STATE pState,
    IN OUT PLSA_SECURITY_OBJECT* ppCachedUser
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    // Set the last update value so low that the next read will force a refresh.
    (*ppCachedUser)->version.tLastUpdated = 0;

    // Update the cache with the now stale item
    dwError = ADCacheStoreObjectEntry(
                    pState->hCacheConnection,
                    *ppCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}

// Note: We only return whether complete if not expired.
static
DWORD
AD_CheckExpiredMemberships(
    IN PLSA_AD_PROVIDER_STATE pState,
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
    dwCacheEntryExpirySeconds = AD_GetCacheEntryExpirySeconds(pState);
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
    IN PLSA_AD_PROVIDER_STATE pState,
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
    dwCacheEntryExpirySeconds = AD_GetCacheEntryExpirySeconds(pState);
    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        PLSA_GROUP_MEMBERSHIP pMembership = ppMemberships[sIndex];

        if (pMembership->bIsInPac ||
            pMembership->bIsDomainPrimaryGroup ||
            (pMembership->version.tLastUpdated > 0 &&
             pMembership->version.tLastUpdated + dwCacheEntryExpirySeconds > now))
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
AD_OnlineChangePassword(
    PAD_PROVIDER_CONTEXT pContext,
    PCSTR pszLoginId,
    PCSTR pszPassword,
    PCSTR pszOldPassword
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pCachedUser = NULL;
    PSTR pszFullDomainName = NULL;
    LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
    PLWNET_DC_INFO pDcInfo = NULL;
    DWORD dwGoodUntilTime = 0;

    dwError = AD_FindUserObjectByName(
                     pContext,
                     pszLoginId,
                     &pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_UpdateObject(
                  pContext->pState,
                  pCachedUser);
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
                       pContext->pState->hDmState,
                       pContext->pState->pProviderData->szDomain,
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
                    pContext->pState,
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
    dwError = AD_StoreAsExpiredObject(
                  pContext->pState,
                  &pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

    // Ignore errors because password change succeeded
    if (pContext->pState->bIsDefault)
    {
        LsaUmModifyUser(
            pCachedUser->userInfo.uid,
            pszPassword);
    }

    // Run a check against the new password. This will download a pac for the
    // user and store their user kerberos creds.
    dwError = AD_OnlineCheckUserPassword(
                    pContext,
                    pCachedUser,
                    pszPassword,
                    &dwGoodUntilTime,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_OnlineCachePasswordVerifier(
                    pContext->pState,
                    pCachedUser,
                    pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);

    ADCacheSafeFreeObject(&pCachedUser);

    LW_SAFE_FREE_STRING(pszFullDomainName);


    return dwError;

error:

    goto cleanup;
}

DWORD
AD_CreateHomeDirectory(
    PLSA_AD_PROVIDER_STATE pState,
    PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;

    if (LW_IS_NULL_OR_EMPTY_STR(pObject->userInfo.pszHomedir)) {
        dwError = LW_ERROR_FAILED_CREATE_HOMEDIR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_ShouldCreateHomeDir(pState))
    {
        // If the home directory's parent directory is NFS mounted from a
        // windows server with "allow root" disabled, then root is blocked from
        // even read only access. In that case, this function will fail with
        // LW_ERROR_ACCESS_DENIED.
        dwError = LsaCheckDirectoryExists(
                        pObject->userInfo.pszHomedir,
                        &bExists);
        BAIL_ON_LSA_ERROR(dwError);

        if (!bExists)
        {
            dwError = AD_CreateHomeDirectory_Generic(
                          pState,
                          pObject);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    return dwError;

error:

    LSA_LOG_ERROR("Failed to create home directory for user (%s), actual error %u", LSA_SAFE_LOG_STRING(pObject->userInfo.pszUnixName), dwError);
    dwError = LW_ERROR_FAILED_CREATE_HOMEDIR;

    goto cleanup;
}

DWORD
AD_CreateHomeDirectory_Generic(
    PLSA_AD_PROVIDER_STATE pState,
    PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = 0;
    mode_t  umask = 0;
    mode_t  perms = (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BOOLEAN bRemoveDir = FALSE;

    umask = AD_GetUmask(pState);

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
                    pState,
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

    LSA_LOG_ERROR("Failed to create home directory for user (%s), actual error %u", pObject->userInfo.pszUnixName, dwError);
    dwError = LW_ERROR_FAILED_CREATE_HOMEDIR;

    goto cleanup;
}

DWORD
AD_ProvisionHomeDir(
    PLSA_AD_PROVIDER_STATE pState,
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

    dwError = AD_GetSkelDirs(pState, &pszSkelPaths);
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
    PLSA_AD_PROVIDER_STATE pState,
    PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD   dwError = 0;
    PSTR    pszK5LoginPath = NULL;
    PSTR    pszK5LoginPath_tmp = NULL;
    PSTR    pszData = NULL;
    PSTR    pszNewData = NULL;
    BOOLEAN bExists = FALSE;
    PSTR pszUpnCopy = NULL;
    PSTR pszUpnCopyLower = NULL;
    int     fd = -1;
    BOOLEAN bRemoveFile = FALSE;
    PSTR pszDnsDomainName = NULL;
    PSTR pszGeneratedUpn = NULL;
    PSTR pszGeneratedUpnLower = NULL;

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

    dwError = LsaDmWrapGetDomainName(
                    pState->hDmState,
                    pObject->pszNetbiosDomainName,
                    &pszDnsDomainName,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pszDnsDomainName);

    dwError = LwAllocateStringPrintf(
                    &pszGeneratedUpn,
                    "%s@%s",
                    pObject->pszSamAccountName,
                    pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    // Create another copy of the UPN that has lowercase non-realm part.
    dwError = LwAllocateString(
                    pszGeneratedUpn,
                    &pszGeneratedUpnLower);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPrincipalNonRealmToLower(pszGeneratedUpnLower);

    dwError = LwAllocateStringPrintf(
                    &pszData,
                    "%s\n",
                    pszUpnCopy);
    BAIL_ON_LSA_ERROR(dwError);


    if (strcmp(pszUpnCopy, pszUpnCopyLower))
    {
        // The UPNs are different, add the new one
        dwError = LwAllocateStringPrintf(
                        &pszNewData,
                        "%s%s\n",
                        pszData,
                        pszUpnCopyLower);
        BAIL_ON_LSA_ERROR(dwError);
        LW_SAFE_FREE_STRING(pszData);
        pszData = pszNewData;
        pszNewData = NULL;
    }

    if (strcmp(pszUpnCopy, pszGeneratedUpn))
    {
        // The UPNs are different, add the new one
        dwError = LwAllocateStringPrintf(
                        &pszNewData,
                        "%s%s\n",
                        pszData,
                        pszGeneratedUpn);
        BAIL_ON_LSA_ERROR(dwError);
        LW_SAFE_FREE_STRING(pszData);
        pszData = pszNewData;
        pszNewData = NULL;
    }

    if (strcmp(pszUpnCopyLower, pszGeneratedUpnLower) &&
            strcmp(pszGeneratedUpn, pszGeneratedUpnLower))
    {
        // The UPNs are different, add the new one
        dwError = LwAllocateStringPrintf(
                        &pszNewData,
                        "%s%s\n",
                        pszData,
                        pszGeneratedUpnLower);
        BAIL_ON_LSA_ERROR(dwError);
        LW_SAFE_FREE_STRING(pszData);
        pszData = pszNewData;
        pszNewData = NULL;
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
            LSA_LOG_ERROR("Failed to remove file at [%s][Error code: %u]",
                          pszK5LoginPath_tmp,
                          dwError2);
        }

    }

    LW_SAFE_FREE_STRING(pszData);
    LW_SAFE_FREE_STRING(pszUpnCopy);
    LW_SAFE_FREE_STRING(pszUpnCopyLower);
    LW_SAFE_FREE_STRING(pszK5LoginPath_tmp);
    LW_SAFE_FREE_STRING(pszK5LoginPath);
    LW_SAFE_FREE_STRING(pszNewData);
    LW_SAFE_FREE_STRING(pszDnsDomainName);
    LW_SAFE_FREE_STRING(pszGeneratedUpn);
    LW_SAFE_FREE_STRING(pszGeneratedUpnLower);

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
    return LwHashCaselessStringHash(((PLSA_SECURITY_OBJECT)pObject)->pszObjectSid);
}

void
AD_FreeHashObject(
    const LW_HASH_ENTRY *pEntry)
{
    ADCacheSafeFreeObject((PLSA_SECURITY_OBJECT *)&pEntry->pKey);
}

static
DWORD
AD_FindObjectBySidNoCache(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszSid,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    return LsaAdBatchFindSingleObject(
                pContext,
                LSA_AD_BATCH_QUERY_TYPE_BY_SID,
                pszSid,
                NULL,
                ppObject);
}

static
DWORD
AD_FindObjectByNT4NameNoCache(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszNT4Name,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    return LsaAdBatchFindSingleObject(
                pContext,
                LSA_AD_BATCH_QUERY_TYPE_BY_NT4,
                pszNT4Name,
                NULL,
                ppObject);
}

static
DWORD
AD_FindObjectByUpnNoCache(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszUpn,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PSTR pszSid = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;

    dwError = LsaDmWrapNetLookupObjectSidByName(
                    pContext->pState->hDmState,
                    pContext->pState->pProviderData->szDomain,
                    pszUpn,
                    &pszSid,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_FindObjectBySidNoCache(
                    pContext,
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
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszAlias,
    BOOLEAN bIsUserAlias,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    return LsaAdBatchFindSingleObject(
                   pContext,
                   bIsUserAlias ? LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS : LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS,
                   pszAlias,
                   NULL,
                   ppResult);
}

DWORD
AD_FindObjectByNameTypeNoCache(
    IN PAD_PROVIDER_CONTEXT pContext,
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
                            pContext,
                            pszName,
                            &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case NameType_UPN:
            dwError = AD_FindObjectByUpnNoCache(
                            pContext,
                            pszName,
                            &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case NameType_Alias:
            dwError = AD_FindObjectByAliasNoCache(
                            pContext,
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
    IN PAD_PROVIDER_CONTEXT pContext,
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
                           pContext,
                           LSA_AD_BATCH_QUERY_TYPE_BY_UID,
                           NULL,
                           &dwId,
                           &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        case LSA_OBJECT_TYPE_GROUP:
            bIsUser = FALSE;
            dwError = LsaAdBatchFindSingleObject(
                           pContext,
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
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN PSTR* ppszList,
    OUT PDWORD pdwCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    return LsaAdBatchFindObjects(
                pContext,
                QueryType,
                dwCount,
                ppszList,
                NULL,
                pdwCount,
                pppObjects);
}

DWORD
AD_FindObjectBySid(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszSid,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_SECURITY_OBJECT* ppResultArray = NULL;
    size_t objectCount = 0;

    dwError = AD_FindObjectsBySidList(
                    pContext,
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
    IN PAD_PROVIDER_CONTEXT pContext,
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
    PLSA_AD_PROVIDER_STATE pState = pContext->pState;
    PLSA_SECURITY_OBJECT* ppCachedResults = NULL;
    PLSA_SECURITY_OBJECT* ppResults = NULL;
    size_t sResultsCount = 0;
    size_t sFoundInCache = 0;
    size_t sFoundInAD = 0;
    DWORD  dwFoundInAD = 0;
    size_t sRemainNumsToFoundInAD = 0;
    size_t sIndex = 0;
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
                    pState->hCacheConnection,
                    sCount,
                    ppszList,
                    &ppCachedResults);
    BAIL_ON_LSA_ERROR(dwError);
    sResultsCount = sCount;

    dwError = LwAllocateMemory(
                    sCount*sizeof(*ppszRemainingList),
                    OUT_PPVOID(&ppszRemainingList));
    BAIL_ON_LSA_ERROR(dwError);

    for (sFoundInCache = 0, sRemainNumsToFoundInAD = 0, sIndex = 0;
         sIndex < sCount;
         sIndex++)
    {
        if ((ppCachedResults[sIndex] != NULL) &&
            (ppCachedResults[sIndex]->version.tLastUpdated >= 0) &&
            (ppCachedResults[sIndex]->version.tLastUpdated +
            AD_GetCacheEntryExpirySeconds(pState) <= now))
        {
            switch (QueryType)
            {
                case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
                    LSA_LOG_VERBOSE("Cache entry for Sid %s is expired",
                         LSA_SAFE_LOG_STRING(ppCachedResults[sIndex]->pszObjectSid));

                    break;

                case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
                    LSA_LOG_VERBOSE("Cache entry for DN %s is expired",
                         LSA_SAFE_LOG_STRING(ppCachedResults[sIndex]->pszDN));

                    break;

                default:
                    LSA_ASSERT(FALSE);
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(dwError);
            }
            ADCacheSafeFreeObject(&ppCachedResults[sIndex]);
        }

        if (ppCachedResults[sIndex] != NULL)
        {
            sFoundInCache++;
            continue;
        }
        ppszRemainingList[sRemainNumsToFoundInAD++] = ppszList[sIndex];
    }

    AD_FilterNullEntries(ppCachedResults, &sResultsCount);
    assert(sResultsCount == sFoundInCache);

    if (sRemainNumsToFoundInAD)
    {
        dwError = pFindByListBatchedCallback(
                        pContext,
                        QueryType,
                        sRemainNumsToFoundInAD,
                        ppszRemainingList,
                        &dwFoundInAD,
                        &ppRemainingObjectsResults);
        BAIL_ON_LSA_ERROR(dwError);

        sFoundInAD = dwFoundInAD;

        dwError = ADCacheStoreObjectEntries(
                        pState->hCacheConnection,
                        sFoundInAD,
                        ppRemainingObjectsResults);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                    sCount * sizeof(*ppResults),
                    OUT_PPVOID(&ppResults));
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        PSTR pszQueryTerm = ppszList[sIndex];
        size_t sCacheIndex = 0;
        size_t sFoundIndex = 0;

        for (sCacheIndex = 0;
             sCacheIndex < sFoundInCache;
             sCacheIndex++)
        {
            if (ppCachedResults[sCacheIndex])
            {
                PSTR pszCachedTerm = NULL;

                switch (QueryType)
                {
                    case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
                        pszCachedTerm = ppCachedResults[sCacheIndex]->pszDN;
                        break;

                    case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
                        pszCachedTerm = ppCachedResults[sCacheIndex]->pszObjectSid;
                        break;

                    default:
                        LSA_ASSERT(FALSE);
                        dwError = ERROR_INVALID_PARAMETER;
                        BAIL_ON_LSA_ERROR(dwError);
                }

                if (LwRtlCStringIsEqual(
                             pszQueryTerm,
                             pszCachedTerm,
                             FALSE))
                {
                    ppResults[sIndex] = ppCachedResults[sCacheIndex];
                    ppCachedResults[sCacheIndex] = NULL;
                    break;
                }
            }
        }

        for (sFoundIndex = 0;
             sFoundIndex < sFoundInAD;
             sFoundIndex++)
        {
            if (ppRemainingObjectsResults[sFoundIndex])
            {
                PSTR pszFoundTerm = NULL;

                switch (QueryType)
                {
                    case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
                        pszFoundTerm = ppRemainingObjectsResults[sFoundIndex]->pszDN;
                        break;

                    case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
                        pszFoundTerm = ppRemainingObjectsResults[sFoundIndex]->pszObjectSid;
                        break;

                    default:
                        LSA_ASSERT(FALSE);
                        dwError = ERROR_INVALID_PARAMETER;
                        BAIL_ON_LSA_ERROR(dwError);
                }

                if (LwRtlCStringIsEqual(
                             pszQueryTerm,
                             pszFoundTerm,
                             FALSE))
                {
                    ppResults[sIndex] = ppRemainingObjectsResults[sFoundIndex];
                    ppRemainingObjectsResults[sFoundIndex] = NULL;
                    break;
                }
            }
        }
    }

    sResultsCount = sFoundInCache + sFoundInAD;

cleanup:

    // We handle error here instead of error label
    // because there is a goto cleanup above.
    if (dwError)
    {
        ADCacheSafeFreeObjectList(sFoundInCache, &ppCachedResults);
        ADCacheSafeFreeObjectList(sFoundInAD, &ppRemainingObjectsResults);
        ADCacheSafeFreeObjectList(sCount, &ppResults);
        sResultsCount = 0;
    }

    ADCacheSafeFreeObjectList(sFoundInCache, &ppCachedResults);
    ADCacheSafeFreeObjectList(sFoundInAD, &ppRemainingObjectsResults);
    LW_SAFE_FREE_MEMORY(ppszRemainingList);

    *pppResults = ppResults;
    if (psResultsCount)
    {
        *psResultsCount = sResultsCount;
    }

    return dwError;

error:

    // Do not actually handle any errors here.
    goto cleanup;
}

DWORD
AD_FindObjectsBySidList(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT OPTIONAL size_t* psResultsCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    return AD_FindObjectsByList(
               pContext,
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
    IN PAD_PROVIDER_CONTEXT pContext,
    IN size_t sCount,
    IN PSTR* ppszDNList,
    OUT OPTIONAL size_t* psResultsCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    return AD_FindObjectsByList(
               pContext,
               ADCacheFindObjectsByDNList,
               AD_FindObjectsByListNoCache,
               LSA_AD_BATCH_QUERY_TYPE_BY_DN,
               sCount,
               ppszDNList,
               psResultsCount,
               pppResults);
}

DWORD
AD_OnlineFindNSSArtefactByKey(
    PAD_PROVIDER_CONTEXT pContext,
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
                    pContext,
                    pContext->pState->pProviderData->szDomain,
                    &pConn);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pContext->pState->pProviderData->dwDirectoryMode)
    {
    case DEFAULT_MODE:

        dwError = DefaultModeFindNSSArtefactByKey(
                        pConn,
                        pContext->pState->pProviderData->cell.szCellDN,
                        pContext->pState->pProviderData->szShortDomain,
                        pszKeyName,
                        pszMapName,
                        dwInfoLevel,
                        dwFlags,
                        ppNSSArtefactInfo);
        break;

    case CELL_MODE:

        dwError = CellModeFindNSSArtefactByKey(
                        pConn,
                        pContext->pState->pProviderData->cell.szCellDN,
                        pContext->pState->pProviderData->szShortDomain,
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
    PAD_PROVIDER_CONTEXT pContext,
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
                    pContext,
                    pContext->pState->pProviderData->szDomain,
                    &pConn);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pContext->pState->pProviderData->dwDirectoryMode)
    {
    case DEFAULT_MODE:
        dwError = DefaultModeEnumNSSArtefacts(
                pConn,
                pContext->pState->pProviderData->cell.szCellDN,
                pContext->pState->pProviderData->szShortDomain,
                pEnumState,
                dwMaxNSSArtefacts,
                pdwNSSArtefactsFound,
                pppNSSArtefactInfoList
                );
        break;

    case CELL_MODE:
        dwError = CellModeEnumNSSArtefacts(
                pConn,
                pContext->pState->pProviderData->cell.szCellDN,
                pContext->pState->pProviderData->szShortDomain,
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
AD_VerifyUserAccountCanLogin(
    PAD_PROVIDER_CONTEXT pContext,
    IN PLSA_SECURITY_OBJECT pUserInfo
    )
{
    DWORD dwError = 0;

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

    if (pUserInfo->userInfo.bPasswordExpired &&
        !LsaDmIsDomainOffline(pContext->pState->hDmState,
            pUserInfo->pszNetbiosDomainName))
    {
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
    IN PAD_PROVIDER_CONTEXT pContext,
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
        dwError = AD_PrescreenUserName(
                      pContext->pState,
                      pszLoginName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADCacheFindUserByName(
            pContext->pState->hCacheConnection,
            pUserNameInfo,
            &pCachedUser);
        break;
    case LSA_OBJECT_TYPE_GROUP:
        dwError = AD_PrescreenGroupName(
                      pContext->pState,
                      pszLoginName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADCacheFindGroupByName(
            pContext->pState->hCacheConnection,
            pUserNameInfo,
            &pCachedUser);
        break;
    default:
        dwError = ADCacheFindUserByName(
            pContext->pState->hCacheConnection,
            pUserNameInfo,
            &pCachedUser);
        if ((dwError == LW_ERROR_NO_SUCH_USER ||
            dwError == LW_ERROR_NOT_HANDLED) &&
            QueryType != LSA_QUERY_TYPE_BY_UPN)
        {
            dwError = ADCacheFindGroupByName(
                pContext->pState->hCacheConnection,
                pUserNameInfo,
                &pCachedUser);
        }
        break;
    }
    
    if (dwError == LW_ERROR_SUCCESS)
    {
        dwError = AD_CheckExpiredObject(
                      pContext->pState,
                      &pCachedUser);
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
            pContext,
            pszLoginName,
            pUserNameInfo->nameType,
            ObjectType,
            &pCachedUser);
        switch (dwError)
        {
        case LW_ERROR_SUCCESS:
            dwError = ADCacheStoreObjectEntry(
                pContext->pState->hCacheConnection,
                pCachedUser);
            BAIL_ON_LSA_ERROR(dwError);
            
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
    IN PAD_PROVIDER_CONTEXT pContext,
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
    PSTR pszDefaultPrefix = NULL;

    dwError = AD_GetUserDomainPrefix(
                  pContext->pState,
                  &pszDefaultPrefix);
    BAIL_ON_LSA_ERROR(dwError);

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
            LsaSrvSpaceReplacement(),
            ' ');
        
        dwError = LsaSrvCrackDomainQualifiedName(
            pszLoginId_copy,
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
            pContext,
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
                AD_ShouldAssumeDefaultDomain(pContext->pState))
            {
                LW_SAFE_FREE_STRING(pszLoginId_copy);
                LsaSrvFreeNameInfo(pUserNameInfo);
                pUserNameInfo = NULL;

                dwError = LwAllocateStringPrintf(
                    &pszLoginId_copy,
                    "%s%c%s",
                    pszDefaultPrefix,
                    LsaSrvDomainSeparator(),
                    QueryList.ppszStrings[dwIndex]);
                BAIL_ON_LSA_ERROR(dwError);

                LwStrCharReplace(
                    pszLoginId_copy,
                    LsaSrvSpaceReplacement(),
                    ' ');

                dwError = LsaSrvCrackDomainQualifiedName(
                    pszLoginId_copy,
                    &pUserNameInfo);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = AD_OnlineFindObjectByName(
                    pContext,
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
        LsaSrvFreeNameInfo(pUserNameInfo);
        pUserNameInfo = NULL;
    }

    *pppObjects = ppObjects;

cleanup:

    LW_SAFE_FREE_STRING(pszDefaultPrefix);
    LW_SAFE_FREE_STRING(pszLoginId_copy);

    if (pUserNameInfo)
    {
        LsaSrvFreeNameInfo(pUserNameInfo);
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
    IN PAD_PROVIDER_CONTEXT pContext,
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
                pContext->pState->hCacheConnection,
                QueryList.pdwIds[dwIndex],
                &pCachedUser);
            break;
        case LSA_OBJECT_TYPE_GROUP:
            dwError = ADCacheFindGroupById(
                pContext->pState->hCacheConnection,
                QueryList.pdwIds[dwIndex],
                &pCachedUser);
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
        if (dwError == LW_ERROR_SUCCESS)
        {
            dwError = AD_CheckExpiredObject(
                          pContext->pState,
                          &pCachedUser);
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
                pContext,
                QueryList.pdwIds[dwIndex],
                ObjectType,
                &pCachedUser);
            switch (dwError)
            {
            case LW_ERROR_SUCCESS:
                dwError = ADCacheStoreObjectEntry(
                    pContext->pState->hCacheConnection,
                    pCachedUser);
                BAIL_ON_LSA_ERROR(dwError);
                
                ppObjects[dwIndex] = pCachedUser;
                pCachedUser = NULL;
                break;
            case LW_ERROR_NO_SUCH_USER:
            case LW_ERROR_NO_SUCH_GROUP:
            case LW_ERROR_NO_SUCH_OBJECT:
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


DWORD
AD_OnlineFindObjects(
    IN PAD_PROVIDER_CONTEXT pContext,
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
            pContext,
            dwCount,
            (PSTR*) QueryList.ppszStrings,
            &sObjectCount,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case LSA_QUERY_TYPE_BY_DN:
         dwError = AD_FindObjectsByDNList(
            pContext,
            dwCount,
            (PSTR*) QueryList.ppszStrings,
            &sObjectCount,
            &ppObjects);
         BAIL_ON_LSA_ERROR(dwError);
         break;
    case LSA_QUERY_TYPE_BY_NT4:
    case LSA_QUERY_TYPE_BY_UPN:
    case LSA_QUERY_TYPE_BY_ALIAS:
        dwError = AD_OnlineFindObjectsByName(
            pContext,
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
            pContext,
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
    IN PAD_PROVIDER_CONTEXT pContext,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_AD_PROVIDER_STATE pState = pContext->pState;
    PAD_ENUM_HANDLE pEnum = hEnum;
    BOOLEAN bIsEnumerationEnabled = TRUE;
    LSA_FIND_FLAGS FindFlags = pEnum->FindFlags;

    if (FindFlags & LSA_FIND_FLAGS_NSS)
    {
        bIsEnumerationEnabled = AD_GetNssEnumerationEnabled(pState);
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
                pContext,
                &pEnum->Cookie,
                LSA_OBJECT_TYPE_USER,
                pEnum->pszDomainName,
                dwMaxObjectsCount,
                pdwObjectsCount,
                pppObjects);
            break;
        case LSA_OBJECT_TYPE_GROUP:
            dwError = LsaAdBatchEnumObjects(
                pContext,
                &pEnum->Cookie,
                LSA_OBJECT_TYPE_GROUP,
                pEnum->pszDomainName,
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

    dwError = ADCacheStoreObjectEntries(
        pContext->pState->hCacheConnection,
        *pdwObjectsCount,
        *pppObjects);
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
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_FIND_FLAGS FindFlags,
    IN PSTR pszSid,
    IN OUT PLW_HASH_TABLE pGroupHash
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
        bIsCacheOnlyMode = AD_GetNssUserMembershipCacheOnlyEnabled(pContext->pState);
    }

    dwError = AD_FindObjectBySid(pContext, pszSid, &pUserInfo);
    if (dwError == LW_ERROR_NO_SUCH_OBJECT)
    {
        /* Skip over unknown SIDs without failing */
        dwError = LW_ERROR_SUCCESS;
        goto cleanup;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheGetGroupsForUser(
                    pContext->pState->hCacheConnection,
                    pszSid,
                    AD_GetTrimUserMembershipEnabled(pContext->pState),
                    &sMembershipCount,
                    &ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_CheckExpiredMemberships(
                    pContext->pState,
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
                        pContext->pState,
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
        dwError = AD_FilterExpiredMemberships(
                      pContext->pState,
                      &sMembershipCount,
                      ppMemberships);
        BAIL_ON_LSA_ERROR(dwError);

        bUseCache = TRUE;
    }

    if (!bUseCache)
    {
        dwError = ADLdap_GetObjectGroupMembership(
                         pContext,
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
                        pContext->pState->hCacheConnection,
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
                !LwHashExists(pGroupHash, ppMemberships[dwIndex]->pszParentSid) &&
                (ppMemberships[dwIndex]->bIsInPac ||
                 ppMemberships[dwIndex]->bIsDomainPrimaryGroup ||
                 bUseCache))
            {

                dwError = LwAllocateString(
                    ppMemberships[dwIndex]->pszParentSid,
                    &pszGroupSid);
                BAIL_ON_LSA_ERROR(dwError);
                
                dwError = LwHashSetValue(pGroupHash, pszGroupSid, pszGroupSid);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = AD_OnlineQueryMemberOfForSid(
                    pContext,
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
            if (!LwHashExists(pGroupHash, ppResults[dwIndex]->pszObjectSid))
            {
                dwError = LwAllocateString(
                    ppResults[dwIndex]->pszObjectSid,
                    &pszGroupSid);
                BAIL_ON_LSA_ERROR(dwError);
                
                dwError = LwHashSetValue(pGroupHash, pszGroupSid, pszGroupSid);
                BAIL_ON_LSA_ERROR(dwError);
                
                dwError = AD_OnlineQueryMemberOfForSid(
                    pContext,
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
        LSA_LOG_ERROR("Failed to find memberships for user '%s\\%s' (error = %u)",
                      pUserInfo->pszNetbiosDomainName,
                      pUserInfo->pszSamAccountName,
                      dwError);
    }

    goto cleanup;
}

static
VOID
AD_OnlineFreeMemberOfHashEntry(
    const LW_HASH_ENTRY* pEntry
    )
{
    if (pEntry->pValue)
    {
        LwFreeMemory(pEntry->pValue);
    }
}

DWORD
AD_MoveHashValuesToArray(
    IN OUT PLW_HASH_TABLE pHash,
    OUT PDWORD pCount,
    OUT PVOID** pppValues
    )
{
    LW_HASH_ITERATOR hashIterator = {0};
    DWORD count = (DWORD) LwHashGetKeyCount(pHash);
    DWORD index = 0;
    DWORD dwError = 0;
    PVOID* ppValues = NULL;
    LW_HASH_ENTRY*   pHashEntry = NULL;
    
    if (count)
    {
        dwError = LwAllocateMemory(
            sizeof(ppValues[0]) * count,
            OUT_PPVOID(&ppValues));
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = LwHashGetIterator(pHash, &hashIterator);
        BAIL_ON_LSA_ERROR(dwError);
        
        for (index = 0; (pHashEntry = LwHashNext(&hashIterator)) != NULL; index++)
        {
            ppValues[index] = pHashEntry->pValue;
            pHashEntry->pValue = NULL;
        }
    }

    *pCount = count;
    *pppValues = ppValues;

cleanup:
    return dwError;

error:
    *pCount = 0;
    *pppValues = NULL;
    LW_SAFE_FREE_MEMORY(ppValues);
    goto cleanup;
}

DWORD
AD_OnlineQueryMemberOf(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    PLW_HASH_TABLE   pGroupHash = NULL;
    DWORD dwGroupSidCount = 0;
    PSTR* ppszGroupSids = NULL;

    dwError = LwHashCreate(
        13,
        LwHashCaselessStringCompare,
        LwHashCaselessStringHash,
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
            pContext,
            FindFlags,
            ppszSids[dwIndex],
            pGroupHash);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = AD_MoveHashValuesToArray(
                    pGroupHash,
                    &dwGroupSidCount,
                    (PVOID**)(PVOID)&ppszGroupSids);
    BAIL_ON_LSA_ERROR(dwError);

    *pdwGroupSidCount = dwGroupSidCount;
    *pppszGroupSids = ppszGroupSids;

cleanup:

    LwHashSafeFree(&pGroupHash);

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
    IN PAD_PROVIDER_CONTEXT pContext,
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
        bIsCacheOnlyMode = AD_GetNssGroupMembersCacheOnlyEnabled(pContext->pState);
    }

    dwError = AD_FindObjectBySid(pContext, pszSid, &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheGetGroupMembers(
                    pContext->pState->hCacheConnection,
                    pszSid,
                    AD_GetTrimUserMembershipEnabled(pContext->pState),
                    &sMembershipCount,
                    &ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_CheckExpiredMemberships(
                    pContext->pState,
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
                      pContext->pState,
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
        dwError = AD_FilterExpiredMemberships(
                      pContext->pState,
                      &sMembershipCount,
                      ppMemberships);
        BAIL_ON_LSA_ERROR(dwError);

        bUseCache = TRUE;
    }

    if (!bUseCache)
    {
        dwError = ADLdap_GetGroupMembers(
                        pContext,
                        pGroupInfo->pszNetbiosDomainName,
                        pszSid,
                        &sResultsCount,
                        &ppResults);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_CacheMembershipFromRelatedObjects(
                        pContext->pState->hCacheConnection,
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

static
DWORD
AD_PrescreenUserName(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszLoginId
    )
{
    DWORD dwError = 0;
    BOOLEAN bIgnoreUser = FALSE;

    BAIL_ON_INVALID_STRING(pszLoginId);

    if (!strcasecmp(pszLoginId, "root"))
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_CheckIgnoreUserNameList(
                  pState,
                  pszLoginId,
                  &bIgnoreUser);
    BAIL_ON_LSA_ERROR(dwError);

    if (bIgnoreUser)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
AD_PrescreenGroupName(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszGroupName
    )
{
    DWORD dwError = 0;
    BOOLEAN bIgnoreGroup = FALSE;

    BAIL_ON_INVALID_STRING(pszGroupName);

    if (!strcasecmp(pszGroupName, "root"))
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_CheckIgnoreGroupNameList(
                  pState,
                  pszGroupName,
                  &bIgnoreGroup);
    BAIL_ON_LSA_ERROR(dwError);

    if (bIgnoreGroup)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
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
