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
 *        lwnet-info.c
 *
 * Abstract:
 *
 *        BeyondTrust Site Manager
 *
 *        Site Information
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#include "includes.h"

LWNET_API
VOID
LWNetFreeDCInfo(
    PLWNET_DC_INFO pDCInfo
    )
{
    
    LWNET_SAFE_FREE_STRING(pDCInfo->pszDomainControllerName);
    LWNET_SAFE_FREE_STRING(pDCInfo->pszDomainControllerAddress);
    LWNET_SAFE_FREE_STRING(pDCInfo->pszNetBIOSDomainName);
    LWNET_SAFE_FREE_STRING(pDCInfo->pszFullyQualifiedDomainName);
    LWNET_SAFE_FREE_STRING(pDCInfo->pszDnsForestName);
    LWNET_SAFE_FREE_STRING(pDCInfo->pszDCSiteName);
    LWNET_SAFE_FREE_STRING(pDCInfo->pszClientSiteName);
    LWNET_SAFE_FREE_STRING(pDCInfo->pszNetBIOSHostName);
    LWNET_SAFE_FREE_STRING(pDCInfo->pszUserName);

    LWNetFreeMemory(pDCInfo);
}

LWNET_API
LW_VOID
LWNetFreeDCList(
    LW_IN LW_OUT PLWNET_DC_ADDRESS pDcList,
    LW_IN DWORD dwDcCount
    )
{
    DWORD i = 0;
    for (i = 0; i < dwDcCount; i++)
    {
        LWNET_SAFE_FREE_STRING(pDcList[i].pszDomainControllerName);
        LWNET_SAFE_FREE_STRING(pDcList[i].pszDomainControllerAddress);
    }
    LWNetFreeMemory(pDcList);
}
