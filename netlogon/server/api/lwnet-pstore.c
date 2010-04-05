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
 *        lwnet-pstore.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        KRB5 API
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#include "includes.h"

DWORD
LWNetSrvGetCurrentDomain(
    PSTR* ppszDomain
    )
{
    DWORD dwError = 0;
    HANDLE hStore = 0;
    PLWPS_PASSWORD_INFO pPassInfo = NULL; 
    PSTR pszDomain = NULL;
    
    dwError = LwpsOpenPasswordStore(
                LWPS_PASSWORD_STORE_DEFAULT,
                &hStore);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwpsGetPasswordByCurrentHostName(
                hStore,
                &pPassInfo);
    if (dwError || pPassInfo == NULL || pPassInfo->pwszDnsDomainName == NULL) 
    {
        // ISSUE-2008/09/15-dalmeida -- Bad error propagation.
        // We used to LWNET_ERROR_DOMAIN_NOT_FOUND here, but now
        // we at least return something more sensible (and what
        // LSASS checks for).  Note that pstore's (lack of proper)
        // error propagation/translation is the underlying problem.
        dwError = ERROR_NOT_JOINED;
    }
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwWc16sToMbs(
                pPassInfo->pwszDnsDomainName,
                &pszDomain
                );
    BAIL_ON_LWNET_ERROR(dwError);
        
    *ppszDomain = pszDomain;
        
cleanup:
    
    if (pPassInfo) {
        LwpsFreePasswordInfo(hStore, pPassInfo);
    }
    if (hStore != (HANDLE)NULL) {
        LwpsClosePasswordStore(hStore);
    }
    return dwError;
    
error:
    LWNET_SAFE_FREE_STRING(pszDomain);
    *ppszDomain = NULL;
    goto cleanup;
}


