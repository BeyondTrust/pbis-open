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
 *        domaininfo.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 * 
 *        Utilities to work with LSA_TRUSTED_DOMAIN_INFO structures
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

VOID
LsaFreeDomainInfoArray(
    DWORD dwNumDomains,
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfoArray
    )
{
    DWORD iDomain = 0;
    
    for (iDomain = 0; iDomain < dwNumDomains; iDomain++)
    {
        LsaFreeDomainInfoContents(&pDomainInfoArray[iDomain]);
    }
    
    LwFreeMemory(pDomainInfoArray);
}

VOID
LsaFreeDomainInfo(
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfo
    )
{
    LsaFreeDomainInfoContents(pDomainInfo);
    
    LwFreeMemory(pDomainInfo);
}

VOID
LsaFreeDomainInfoContents(
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfo
    )
{
    LW_SAFE_FREE_STRING(pDomainInfo->pszDnsDomain);
    LW_SAFE_FREE_STRING(pDomainInfo->pszNetbiosDomain);
    LW_SAFE_FREE_STRING(pDomainInfo->pszTrusteeDnsDomain);
    LW_SAFE_FREE_STRING(pDomainInfo->pszDomainSID);
    LW_SAFE_FREE_STRING(pDomainInfo->pszDomainGUID);
    LW_SAFE_FREE_STRING(pDomainInfo->pszForestName);
    LW_SAFE_FREE_STRING(pDomainInfo->pszClientSiteName);

    if (pDomainInfo->pDCInfo)
    {
        LsaFreeDCInfo(pDomainInfo->pDCInfo);
        pDomainInfo->pDCInfo = NULL;
    }
    
    if (pDomainInfo->pGCInfo)
    {
        LsaFreeDCInfo(pDomainInfo->pGCInfo);
        pDomainInfo->pGCInfo = NULL;
    }
}
