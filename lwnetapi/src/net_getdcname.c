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

#include "includes.h"


NTSTATUS
NetpGetRwDcName(
    const wchar16_t *DnsDomainName,
    BOOLEAN Force,
    wchar16_t** DomainControllerName
    )
{
    DWORD dwError = 0;
    wchar16_t *domain_controller_name = NULL;
    char *dns_domain_name_mbs = NULL;
    DWORD get_dc_name_flags = DS_WRITABLE_REQUIRED;
    PLWNET_DC_INFO pDC = NULL;

    if (Force)
    {
        get_dc_name_flags |= DS_FORCE_REDISCOVERY;
    }

    dns_domain_name_mbs = awc16stombs(DnsDomainName);
    if (!dns_domain_name_mbs)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    dwError = LWNetGetDCName(NULL, dns_domain_name_mbs, NULL, get_dc_name_flags, &pDC);
    if (dwError)
    {
        goto cleanup;
    }

    // Add a trailing dot to the name to indicate that the address is
    // fully-qualified and the default domain suffix does not need to be
    // appended.
    domain_controller_name = asw16printfw(
                                L"%hhs.",
                                pDC->pszDomainControllerName);

 cleanup:
    LW_SAFE_FREE_MEMORY(dns_domain_name_mbs);
    LWNET_SAFE_FREE_DC_INFO(pDC);
    if (dwError)
    {
        LW_SAFE_FREE_MEMORY(domain_controller_name);
    }

    *DomainControllerName = domain_controller_name;

    // ISSUE-2008/07/14-dalmeida -- Need to do error code conversion

    return dwError;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
