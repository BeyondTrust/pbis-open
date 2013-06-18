/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software
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
 *        lsa_wbc_domain.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 * Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */

#include "wbclient.h"
#include "lsawbclient_p.h"
#include <string.h>
#include <lwmem.h>
#include <lwstr.h>
#include <lwnet.h>

static int
FreeWbcDomainInfo(
    IN OUT void* p
    );

static int
FreeWbcDomainInfoArray(
    IN OUT void* p
    );

static DWORD
FillDomainInfo(
    OUT struct wbcDomainInfo *pWbcDomInfo,
    IN  PLSA_TRUSTED_DOMAIN_INFO pLsaDomInfo
    );

/*****************************************************************************
 ****************************************************************************/

wbcErr
wbcDomainInfo(
    IN const char *domain,
    OUT struct wbcDomainInfo **info
    )
{
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    DWORD dwErr = LW_ERROR_INTERNAL;
    HANDLE hLsa = (HANDLE)NULL;
    PLSASTATUS pLsaStatus = NULL;
    struct wbcDomainInfo *pWbcDomInfo = NULL;
    PLSA_TRUSTED_DOMAIN_INFO pLsaDomInfo = NULL;
    PLSA_AUTH_PROVIDER_STATUS pADProvStatus = NULL;
    int i = 0;

    /* Sanity check */

    BAIL_ON_NULL_PTR_PARAM(domain, dwErr);
    BAIL_ON_NULL_PTR_PARAM(info, dwErr);

    /* Work */

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);
    
    dwErr = LsaGetStatus(hLsa, &pLsaStatus);
    BAIL_ON_LSA_ERR(dwErr);

    /* Find the AD provider entry */

    for (i=0; i<pLsaStatus->dwCount; i++)
    {
        if (strcmp(pLsaStatus->pAuthProviderStatusList[i].pszId,
                   LSA_PROVIDER_TAG_AD) == 0)
        {
            pADProvStatus = &pLsaStatus->pAuthProviderStatusList[i];
            break;
        }
    }

    if (pADProvStatus == NULL)
    {
        dwErr = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERR(dwErr);
    }

    /* Find the requested domain */

    for (i=0; i<pADProvStatus->dwNumTrustedDomains; i++)
    {
        PLSA_TRUSTED_DOMAIN_INFO pCursorDomInfo = NULL;

        pCursorDomInfo = &pADProvStatus->pTrustedDomainInfoArray[i];
        if (StrEqual(pCursorDomInfo->pszDnsDomain, domain) ||
            StrEqual(pCursorDomInfo->pszNetbiosDomain, domain))
        {
            pLsaDomInfo = pCursorDomInfo;
            break;            
        }
    }

    if (pLsaDomInfo == NULL)
    {
        dwErr = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERR(dwErr);
    }

    /* Fill in the domain info */

    pWbcDomInfo = _wbc_malloc_zero(
                      sizeof(struct wbcDomainInfo),
                      FreeWbcDomainInfo);
    BAIL_ON_NULL_PTR(pWbcDomInfo, dwErr);

    dwErr = FillDomainInfo(pWbcDomInfo, pLsaDomInfo);
    BAIL_ON_LSA_ERR(dwErr);

    *info = pWbcDomInfo;
    pWbcDomInfo = NULL;

cleanup:
    
    if (pLsaStatus)
    {
        LsaFreeStatus(pLsaStatus);
    }

    if (hLsa != (HANDLE)NULL) {
        LsaCloseServer(hLsa);
    }

    _WBC_FREE(pWbcDomInfo);
    
    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}


/*****************************************************************************
 ****************************************************************************/

wbcErr
wbcListTrusts(
    OUT struct wbcDomainInfo **domains,
    OUT size_t *num_domains
    )
{
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    DWORD dwErr = LW_ERROR_INTERNAL;
    HANDLE hLsa = (HANDLE)NULL;
    PLSASTATUS pLsaStatus = NULL;
    struct wbcDomainInfo *pWbcDomInfoArray = NULL;
    PLSA_AUTH_PROVIDER_STATUS pADProvStatus = NULL;
    size_t NumDomains = 0;
    int i = 0;

    /* Sanity check */

    BAIL_ON_NULL_PTR_PARAM(domains, dwErr);
    BAIL_ON_NULL_PTR_PARAM(num_domains, dwErr);

    /* Work */

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);
    
    dwErr = LsaGetStatus(hLsa, &pLsaStatus);
    BAIL_ON_LSA_ERR(dwErr);

    /* Find the AD provider entry */

    for (i=0; i<pLsaStatus->dwCount; i++)
    {
        if (strcmp(pLsaStatus->pAuthProviderStatusList[i].pszId,
                   LSA_PROVIDER_TAG_AD) == 0)
        {
            pADProvStatus = &pLsaStatus->pAuthProviderStatusList[i];
            break;
        }
    }

    if (pADProvStatus == NULL)
    {
        dwErr = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERR(dwErr);
    }

    /* Fill in the domain info */

    NumDomains = pADProvStatus->dwNumTrustedDomains;
    pWbcDomInfoArray = _wbc_malloc_zero(
                           sizeof(struct wbcDomainInfo)*(NumDomains+1),
                           FreeWbcDomainInfoArray);
    BAIL_ON_NULL_PTR(pWbcDomInfoArray, dwErr);

    for (i=0; i<NumDomains; i++)
    {
        dwErr = FillDomainInfo(
                    &pWbcDomInfoArray[i], 
                    &pADProvStatus->pTrustedDomainInfoArray[i]);
        BAIL_ON_LSA_ERR(dwErr);
    }

    *domains = pWbcDomInfoArray;
    pWbcDomInfoArray = NULL;

    *num_domains = NumDomains;

cleanup:
    if (hLsa != (HANDLE)NULL) {
        LsaCloseServer(hLsa);
    }

    _WBC_FREE(pWbcDomInfoArray);
    
    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}


/*****************************************************************************
 ****************************************************************************/

wbcErr
wbcCheckTrustCredentials(
    IN  const char *domain,
    OUT struct wbcAuthErrorInfo **error
    )
{
    return WBC_ERR_NOT_IMPLEMENTED;
}

wbcErr
wbcChangeTrustCredentials(
    IN const char *domain,
    OUT struct wbcAuthErrorInfo **error
    )
{
    return WBC_ERR_NOT_IMPLEMENTED;
}

/*****************************************************************************
 ****************************************************************************/

static int
FreeWbcDomainInfo(
    IN OUT void* p
    )
{
    struct wbcDomainInfo *pDomain = (struct wbcDomainInfo*)p;

    if (!pDomain) {
        return 0;
    }

    _WBC_FREE(pDomain->short_name);
    _WBC_FREE(pDomain->dns_name);

    return 0;
}

static int
FreeWbcDomainInfoArray(
    IN OUT void* p
    )
{
    struct wbcDomainInfo *pDomains = (struct wbcDomainInfo*)p;

    if (!pDomains) {
        return 0;
    }

    while (pDomains->short_name)
    {
        FreeWbcDomainInfo(pDomains);
        pDomains++;
    }

    return 0;
}

static DWORD
FillDomainInfo(
    OUT struct wbcDomainInfo *pWbcDomInfo,
    IN  PLSA_TRUSTED_DOMAIN_INFO pLsaDomInfo
    )
{
    DWORD dwErr = LW_ERROR_INTERNAL;

    if (pLsaDomInfo->pszDnsDomain)
    {
        pWbcDomInfo->dns_name = _wbc_strdup(pLsaDomInfo->pszDnsDomain);
        BAIL_ON_NULL_PTR(pLsaDomInfo->pszDnsDomain, dwErr);
    }
    
    if (pLsaDomInfo->pszNetbiosDomain)
    {
        pWbcDomInfo->short_name = _wbc_strdup(pLsaDomInfo->pszNetbiosDomain);
        BAIL_ON_NULL_PTR(pLsaDomInfo->pszNetbiosDomain, dwErr);
    }
    
    if (pLsaDomInfo->pszDomainSID)
    {
        dwErr = wbcStringToSid(pLsaDomInfo->pszDomainSID, &pWbcDomInfo->sid);
        BAIL_ON_LSA_ERR(dwErr);
    }

    /* Domain flags */

    pWbcDomInfo->domain_flags = WBC_DOMINFO_DOMAIN_AD;

    if (pLsaDomInfo->dwDomainFlags & LSA_DM_DOMAIN_FLAG_PRIMARY)
    {
        pWbcDomInfo->domain_flags |= WBC_DOMINFO_DOMAIN_PRIMARY;
        pWbcDomInfo->trust_flags |= WBC_DOMINFO_TRUST_INCOMING;
        pWbcDomInfo->trust_flags |= WBC_DOMINFO_TRUST_OUTGOING;
        pWbcDomInfo->trust_flags |= WBC_DOMINFO_TRUST_TRANSITIVE;        
    }

    if ((pLsaDomInfo->dwDomainFlags & LSA_DM_DOMAIN_FLAG_OFFLINE) ||
        (pLsaDomInfo->dwDomainFlags & LSA_DM_DOMAIN_FLAG_FORCE_OFFLINE))
    {
        pWbcDomInfo->domain_flags |= WBC_DOMINFO_DOMAIN_OFFLINE;
    }

    /* Trust Flags */

    if (pLsaDomInfo->dwTrustFlags & LSA_TRUST_FLAG_INBOUND) {
        pWbcDomInfo->trust_flags |= WBC_DOMINFO_TRUST_INCOMING;
    }
    if (pLsaDomInfo->dwTrustFlags & LSA_TRUST_FLAG_OUTBOUND) {
        pWbcDomInfo->trust_flags |= WBC_DOMINFO_TRUST_OUTGOING;
    }
    if ((pLsaDomInfo->dwTrustAttributes & 
        (LSA_TRUST_ATTRIBUTE_WITHIN_FOREST|
         LSA_TRUST_ATTRIBUTE_FOREST_TRANSITIVE)) ||
        (pLsaDomInfo->dwTrustFlags & LSA_TRUST_FLAG_IN_FOREST))
    {
        pWbcDomInfo->trust_flags |= WBC_DOMINFO_TRUST_TRANSITIVE;
    }

    /* Trust Type */

    if (pLsaDomInfo->dwTrustAttributes & LSA_TRUST_ATTRIBUTE_WITHIN_FOREST) {
        pWbcDomInfo->trust_type |= WBC_DOMINFO_TRUSTTYPE_IN_FOREST;
    }
    if (pLsaDomInfo->dwTrustAttributes & LSA_TRUST_ATTRIBUTE_FOREST_TRANSITIVE) {
        pWbcDomInfo->trust_type |= WBC_DOMINFO_TRUSTTYPE_FOREST;
    }
    if (pLsaDomInfo->dwTrustAttributes & LSA_TRUST_ATTRIBUTE_NON_TRANSITIVE) {
        pWbcDomInfo->trust_type |= WBC_DOMINFO_TRUSTTYPE_EXTERNAL;
    }

    switch (pLsaDomInfo->dwTrustMode)
    {
    case LSA_TRUST_MODE_MY_FOREST:
        pWbcDomInfo->trust_type = WBC_DOMINFO_TRUSTTYPE_IN_FOREST;
        pWbcDomInfo->trust_flags |= (WBC_DOMINFO_TRUST_INCOMING|
                                     WBC_DOMINFO_TRUST_OUTGOING);
        break;

    case LSA_TRUST_MODE_OTHER_FOREST:
        pWbcDomInfo->trust_type = WBC_DOMINFO_TRUSTTYPE_FOREST;
        break;

    case LSA_TRUST_MODE_EXTERNAL:
        pWbcDomInfo->trust_type = WBC_DOMINFO_TRUSTTYPE_EXTERNAL;
        break;
    }

cleanup:
    return dwErr;
}

static
int
FreeDomainControllerEx(
    IN void *p
    )
{
    struct wbcDomainControllerInfoEx *pController =
            (struct wbcDomainControllerInfoEx *)p;

    if (!pController)
    {
        return 0;
    }

    if (pController->dc_unc)
    {
        LwFreeString((char *) pController->dc_unc);
    }

    if (pController->dc_address)
    {
        LwFreeString((char *) pController->dc_address);
    }

    LW_SAFE_FREE_MEMORY(pController->domain_guid);

    if (pController->domain_name)
    {
        LwFreeString((char *) pController->domain_name);
    }

    if (pController->forest_name)
    {
        LwFreeString((char *) pController->forest_name);
    }

    if (pController->dc_site_name)
    {
        LwFreeString((char *) pController->dc_site_name);
    }

    if (pController->client_site_name)
    {
        LwFreeString((char *) pController->client_site_name);
    }

    return 0;
}

static
int
FreeDomainController(
    IN void *p
    )
{
    struct wbcDomainControllerInfo *pController = NULL;

    if (!p)
    {
        return 0;
    }

    LW_SAFE_FREE_STRING(pController->dc_name);

    return 0;
}

wbcErr
wbcLookupDomainController(
    const char *domain,
    uint32_t flags,
    struct wbcDomainControllerInfo **dc_info
    )
{
    DWORD error;
    PLWNET_DC_INFO pDCInfo = NULL;
    struct wbcDomainControllerInfo *pResult = NULL;

    error = LWNetGetDCName(
            NULL,
            domain,
            NULL,
            flags,
            &pDCInfo);
    BAIL_ON_LSA_ERR(error);

    pResult = _wbc_malloc_zero(sizeof(*pResult), FreeDomainController);
    BAIL_ON_NULL_PTR(pResult, error);

    error = LwAllocateString(
                pDCInfo->pszDomainControllerName,
                &pResult->dc_name);
    BAIL_ON_LSA_ERR(error);

    *dc_info = pResult;

cleanup:
    if (error)
    {
        *dc_info = NULL;
        _WBC_FREE(pResult);
    }
    return map_error_to_wbc_status(error);
}

wbcErr
wbcLookupDomainControllerEx(
    const char *domain,
    struct wbcGuid *guid,
    const char *site,
    uint32_t flags,
    struct wbcDomainControllerInfoEx **dc_info
    )
{
    DWORD error;
    PLWNET_DC_INFO pDCInfo = NULL;
    struct wbcDomainControllerInfoEx *pResult = NULL;

    if (guid != NULL)
    {
        return WBC_ERR_NOT_IMPLEMENTED;
    }

    error = LWNetGetDCName(
            NULL,
            domain,
            site,
            flags,
            &pDCInfo);
    BAIL_ON_LSA_ERR(error);

    pResult = _wbc_malloc_zero(sizeof(*pResult), FreeDomainControllerEx);
    BAIL_ON_NULL_PTR(pResult, error);

    error = LwAllocateStringPrintf(
                (char **)&pResult->dc_unc,
                "\\\\%s",
                pDCInfo->pszDomainControllerAddress);
    BAIL_ON_LSA_ERR(error);

    error = LwAllocateString(
                pDCInfo->pszDomainControllerAddress,
                (char **)&pResult->dc_address);
    BAIL_ON_LSA_ERR(error);

    pResult->dc_address_type = pDCInfo->dwDomainControllerAddressType;

    error = LwAllocateMemory(
                sizeof(*pResult->domain_guid),
                (PVOID*)&pResult->domain_guid);
    BAIL_ON_LSA_ERR(error);

    wbcUuidToWbcGuid(
        pDCInfo->pucDomainGUID,
        pResult->domain_guid);

    error = LwAllocateString(
                pDCInfo->pszFullyQualifiedDomainName,
                (char **)&pResult->domain_name);
    BAIL_ON_LSA_ERR(error);

    error = LwAllocateString(
                pDCInfo->pszDnsForestName,
                (char **)&pResult->forest_name);
    BAIL_ON_LSA_ERR(error);

    pResult->dc_flags = pDCInfo->dwFlags;

    error = LwAllocateString(
                pDCInfo->pszDCSiteName,
                (char **)&pResult->dc_site_name);
    BAIL_ON_LSA_ERR(error);

    error = LwAllocateString(
                pDCInfo->pszClientSiteName,
                (char **)&pResult->client_site_name);
    BAIL_ON_LSA_ERR(error);

    *dc_info = pResult;

cleanup:
    if (error)
    {
        *dc_info = NULL;
        _WBC_FREE(pResult);
    }
    return map_error_to_wbc_status(error);
}

wbcErr
wbcDcInfo(
    const char *domain,
    size_t *num_dcs,
    const char ***dc_names,
    const char ***dc_ips
    )
{
    DWORD error;
    PLWNET_DC_INFO pDCInfo = NULL;

    error = LWNetGetDCName(
            NULL,
            domain,
            NULL,
            0,
            &pDCInfo);
    BAIL_ON_LSA_ERR(error);

    *num_dcs = 1;

    *dc_names = _wbc_malloc_zero(sizeof(**dc_names) * 2,
            _wbc_free_string_array);
    BAIL_ON_NULL_PTR(*dc_names, error);

    (*dc_names)[0] = _wbc_strdup(pDCInfo->pszDomainControllerName);
    BAIL_ON_NULL_PTR((*dc_names)[0], error);

    *dc_ips = _wbc_malloc_zero(sizeof(**dc_ips) * 2,
            _wbc_free_string_array);
    BAIL_ON_NULL_PTR(*dc_ips, error);

    (*dc_ips)[0] = _wbc_strdup(pDCInfo->pszDomainControllerAddress);
    BAIL_ON_NULL_PTR((*dc_ips)[0], error);

cleanup:
    if (error)
    {
        _WBC_FREE(*dc_ips);
        _WBC_FREE(*dc_names);
    }
    return map_error_to_wbc_status(error);
}

wbcErr
wbcPingDc(
    const char *domain,
    struct wbcAuthErrorInfo **ppError
    )
{
    DWORD error;
    LWNET_UNIX_TIME_T dCTime = 0;

    error = LWNetGetDCTime(
            domain,
            &dCTime);
    BAIL_ON_LSA_ERR(error);

cleanup:
    wbcFillErrorInfo(map_error_to_wbc_status(error), ppError);
    return map_error_to_wbc_status(error);
}

