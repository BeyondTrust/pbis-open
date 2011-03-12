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
 *        dcinfo.c
 *
 * Abstract:
 * 
 *        Likewise Site Manager
 * 
 *        Domain Controller Info API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#include "includes.h"

LWNET_API
DWORD
LWNetGetDCNameExt(
    PCSTR pszServerFQDN,
    PCSTR pszDomainFQDN,
    PCSTR pszSiteName,
    PCSTR pszPrimaryDomain,
    DWORD dwFlags,
    DWORD dwBlackListCount,
    PSTR* ppszAddressBlackList,
    PLWNET_DC_INFO* ppDCInfo
    )
{
    DWORD dwError = 0;
    PLWNET_DC_INFO pDCInfo = NULL;
    HANDLE hServer = 0;
    DWORD dwFlagsLocal = 0;
    INT iMutuallyExclusiveRequirementCount = 0;
    
    if(!IsNullOrEmptyString(pszServerFQDN))
    {
        LWNET_LOG_WARNING("LWNetGetDcInfo called with pszServerFQDN != NULL.  Non-null value ignored.");
    }
    
    if(dwFlags & (~LWNET_SUPPORTED_DS_INPUT_FLAGS))
    {
        LWNET_LOG_WARNING("LWNetGetDcInfo called with unsupported flags: %.8X", 
                dwFlags & (~LWNET_SUPPORTED_DS_INPUT_FLAGS)); 
    }
    dwFlagsLocal = dwFlags & LWNET_SUPPORTED_DS_INPUT_FLAGS;
    
    if(dwFlags & DS_GC_SERVER_REQUIRED)
    {
        iMutuallyExclusiveRequirementCount++;
    }
    if(dwFlags & DS_PDC_REQUIRED)
    {
        iMutuallyExclusiveRequirementCount++;
    }
    if(dwFlags & DS_KDC_REQUIRED)
    {
        iMutuallyExclusiveRequirementCount++;
    }
    if(iMutuallyExclusiveRequirementCount > 1)
    {
        LWNET_LOG_ERROR("LWNetGetDcInfo may be called with no more than one of the following flags: " \
                        "DS_GC_SERVER_REQUIRED, DS_PDC_REQUIRED, DS_KDC_REQUIRED");
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    iMutuallyExclusiveRequirementCount = 0;
    if(dwFlags & DS_BACKGROUND_ONLY)
    {
        iMutuallyExclusiveRequirementCount++;
    }
    if(dwFlags & DS_FORCE_REDISCOVERY)
    {
        iMutuallyExclusiveRequirementCount++;
    }
    if(iMutuallyExclusiveRequirementCount > 1)
    {
        LWNET_LOG_ERROR("LWNetGetDcInfo may be called with no more than one of the following flags: " \
                        "DS_BACKGROUND_ONLY, DS_FORCE_REDISCOVERY");
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetOpenServer(
                &hServer);
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LWNetTransactGetDCName(
        hServer,
        pszServerFQDN,
        pszDomainFQDN,
        pszSiteName,
        pszPrimaryDomain,
        dwFlagsLocal,
        dwBlackListCount,
        ppszAddressBlackList,
        &pDCInfo);
    BAIL_ON_LWNET_ERROR(dwError);
    
    *ppDCInfo = pDCInfo;

cleanup:

    if (hServer)
    {
        DWORD dwErrorLocal = 0;
        dwErrorLocal = LWNetCloseServer(hServer);
        if(!dwError)
        {
            dwError = dwErrorLocal;
        }
    }
        
    return dwError;
    
error:

    if (pDCInfo)
    {
        LWNetFreeDCInfo(pDCInfo);
    }

    *ppDCInfo = NULL;

    goto cleanup;
}

LWNET_API
DWORD
LWNetGetDCName(
    PCSTR pszServerFQDN,
    PCSTR pszDomainFQDN,
    PCSTR pszSiteName,
    DWORD dwFlags,
    PLWNET_DC_INFO* ppDCInfo
    )
{
    return LWNetGetDCNameExt(
        pszServerFQDN,
        pszDomainFQDN,
        pszSiteName,
        NULL,
        dwFlags,
        0,
        NULL,
        ppDCInfo);
}

LWNET_API
DWORD
LWNetGetDCNameWithBlacklist(
    PCSTR pszServerFQDN,
    PCSTR pszDomainFQDN,
    PCSTR pszSiteName,
    DWORD dwFlags,
    DWORD dwBlackListCount,
    PSTR* ppszAddressBlackList,
    PLWNET_DC_INFO* ppDCInfo
    )
{
    return LWNetGetDCNameExt(
        pszServerFQDN,
        pszDomainFQDN,
        pszSiteName,
        NULL,
        dwFlags,
        dwBlackListCount,
        ppszAddressBlackList,
        ppDCInfo);
}

LWNET_API
LW_DWORD
LWNetGetDCList(
    LW_IN LW_PCSTR pszDomainFQDN,
    LW_IN LW_PCSTR pszSiteName,
    LW_IN LW_DWORD dwFlags,
    LW_OUT PLWNET_DC_ADDRESS* ppDcList,
    LW_OUT LW_PDWORD pdwDcCount
    )
{
    DWORD dwError = 0;
    PLWNET_DC_ADDRESS pDcList = NULL;
    DWORD dwDcCount = 0;
    HANDLE hServer = 0;

    dwError = LWNetOpenServer(&hServer);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetTransactGetDCList(
                    hServer,
                    pszDomainFQDN,
                    pszSiteName,
                    dwFlags,
                    &pDcList,
                    &dwDcCount);
    BAIL_ON_LWNET_ERROR(dwError);

    *ppDcList = pDcList;
    *pdwDcCount = dwDcCount;

cleanup:

    if (hServer)
    {
        DWORD dwErrorLocal = 0;
        dwErrorLocal = LWNetCloseServer(hServer);
        if (!dwError)
        {
            dwError = dwErrorLocal;
        }
    }

    return dwError;

error:

    if (pDcList)
    {
        LWNetFreeDCList(pDcList, dwDcCount);
    }

    *ppDcList = NULL;
    *pdwDcCount = 0;

    goto cleanup;
}

LWNET_API
DWORD
LWNetGetDCTime(
    PCSTR pszDomainFQDN,
    PLWNET_UNIX_TIME_T pDCTime
    )
{
    DWORD dwError = 0;
    HANDLE hServer = 0;
    
    dwError = LWNetOpenServer(
                &hServer);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetTransactGetDCTime(
        hServer,
        pszDomainFQDN,
        pDCTime);
    BAIL_ON_LWNET_ERROR(dwError);
    
cleanup:

    if (hServer)
    {
        DWORD dwErrorLocal = 0;
        dwErrorLocal = LWNetCloseServer(hServer);
        if(!dwError)
        {
            dwError = dwErrorLocal;
        }
    }

    return dwError;
    
error:

    goto cleanup;
}

LWNET_API
DWORD
LWNetGetDomainController(
    PCSTR pszDomainFQDN,
    PSTR* ppszDomainControllerFQDN
    )
{
    DWORD dwError = 0;
    HANDLE hServer = 0;
    
    dwError = LWNetOpenServer(
                &hServer);
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LWNetTransactGetDomainController(
        hServer,
        pszDomainFQDN,
        ppszDomainControllerFQDN);
    BAIL_ON_LWNET_ERROR(dwError);
    
cleanup:

    if (hServer)
    {
        DWORD dwErrorLocal = 0;
        dwErrorLocal = LWNetCloseServer(hServer);
        if(!dwError)
        {
            dwError = dwErrorLocal;
        }
    }

    return dwError;
    
error:

    goto cleanup;
}

LWNET_API
LW_DWORD
LWNetResolveName(
    LW_IN LW_PCWSTR pcwszHostName,
    LW_OUT LW_OPTIONAL LW_PWSTR *ppwszCanonName,
    LW_OUT PLWNET_RESOLVE_ADDR **pppAddressList,
    LW_OUT PDWORD pdwAddressListLen
    )
{
    DWORD dwError = 0;
    HANDLE hServer = 0;
    LW_PWSTR pwszCanonName = NULL;
    PLWNET_RESOLVE_ADDR *ppAddressList = NULL;
    DWORD dwAddressListLen = 0;
   

    dwError = LWNetOpenServer(&hServer);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetTransactResolveName(
        hServer,
        pcwszHostName,
        &pwszCanonName,
        &ppAddressList,
        &dwAddressListLen);
    BAIL_ON_LWNET_ERROR(dwError);

cleanup:
    if (hServer)
    {
        DWORD dwErrorLocal = 0;
        dwErrorLocal = LWNetCloseServer(hServer);
        if(!dwError)
        {
            dwError = dwErrorLocal;
        }
    }

    if (ppwszCanonName)
    {
        *ppwszCanonName = pwszCanonName;
    }
    else
    {
        LWNET_SAFE_FREE_MEMORY(pwszCanonName);
    }
    *pppAddressList = ppAddressList;
    *pdwAddressListLen = dwAddressListLen;

    return dwError;

error:
    goto cleanup;
}


LWNET_API
LW_DWORD
LWNetResolveNameFree(
    LW_IN LW_OPTIONAL LW_PWSTR pwszCanonName,
    LW_IN PLWNET_RESOLVE_ADDR *ppAddressList,
    LW_IN DWORD dwAddressListLen)
{
    DWORD dwError = 0;
    DWORD i = 0;

    BAIL_ON_INVALID_POINTER(ppAddressList);

    for (i=0; i<dwAddressListLen; i++)
    {
        LWNET_SAFE_FREE_MEMORY(ppAddressList[i]);
    }
    LWNET_SAFE_FREE_MEMORY(ppAddressList);
    LWNET_SAFE_FREE_MEMORY(pwszCanonName);

error:
    return dwError;
}
