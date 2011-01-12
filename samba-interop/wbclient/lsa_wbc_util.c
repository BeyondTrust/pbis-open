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
 *        lsa_wbc.c
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
#include "lwnet.h"
#include <lsa/ad.h>

struct _wbc_err_string {
    wbcErr wbc_err;
    const char *errstr;
};

static struct _wbc_err_string wbcErrorMap[] = {
    { WBC_ERR_SUCCESS, "No error" },
    { WBC_ERR_NOT_IMPLEMENTED, "Function not implemented" },
    { WBC_ERR_UNKNOWN_FAILURE, "Unknown Failure" },
    { WBC_ERR_NO_MEMORY, "Out of memory" },
    { WBC_ERR_INVALID_SID, "Invalid Security Identifier" },
    { WBC_ERR_INVALID_PARAM, "Invalid Parameter" },
    { WBC_ERR_WINBIND_NOT_AVAILABLE, "Security service not available" },
    { WBC_ERR_DOMAIN_NOT_FOUND, "Domain not found" },
    { WBC_ERR_INVALID_RESPONSE, "Invalid response received from security authority" },
    { WBC_ERR_NSS_ERROR, "Name server switch error" },
    { WBC_ERR_AUTH_ERROR, "Authentication error" }
};

/* @brief Convert a wbcErr to a human readable string
 *
 * @param error      Error code to translate
 *
 * @return char*
**/

const char *wbcErrorString(wbcErr error)
{
    int i = 0;
    size_t table_size = sizeof(wbcErrorMap) / sizeof(struct _wbc_err_string);

    for (i=0; i<table_size; i++) {
        if (error == wbcErrorMap[i].wbc_err) {
            return wbcErrorMap[i].errstr;
        }
    }

    return "Unmapped error";
}

void wbcFreeMemory(void* p)
{
    if (p)
        _WBC_FREE(p);

    return;
}

wbcErr wbcPing(void)
{
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;

    /* Just open and close an LsaServerHandle */

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

cleanup:
    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

static int FreeInterfaceDetails(void *p)
{
    struct wbcInterfaceDetails *iface;

    if (!p)
        return 0;

    iface = (struct wbcInterfaceDetails*)p;

    _WBC_FREE_CONST_DISCARD(iface->netbios_domain);
    _WBC_FREE_CONST_DISCARD(iface->dns_domain);

    return 0;
}


wbcErr wbcInterfaceDetails(struct wbcInterfaceDetails **details)
{
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    HANDLE hLsa = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;
    PLWNET_DC_INFO pDcInfo = NULL;

    BAIL_ON_NULL_PTR_PARAM(details, dwErr);

    /* Find our domain */

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaAdGetMachineAccountInfo(hLsa, NULL, &pAccountInfo);
    BAIL_ON_LSA_ERR(dwErr);

    /* Find DC to get the the short domain name */

    dwErr = LWNetGetDCName(NULL, pAccountInfo->DnsDomainName, NULL, 0, &pDcInfo);
    BAIL_ON_NETLOGON_ERR(dwErr);

    /* extra check until API is complete */

    BAIL_ON_NULL_PTR(pDcInfo, dwErr);

    *details = _wbc_malloc(sizeof(struct wbcInterfaceDetails),
                   FreeInterfaceDetails);
    BAIL_ON_NULL_PTR(*details, dwErr);

    (*details)->interface_version = LSA_WBC_INTERFACE_VERSION;
    (*details)->winbind_version   = LSA_WBC_WINBIND_VERSION;
    (*details)->winbind_separator = '\\';

    /* FIXME!  need to fill in real valid strings here */

    (*details)->netbios_name = "";

    (*details)->netbios_domain = _wbc_strdup(pDcInfo->pszNetBIOSDomainName);
    BAIL_ON_NULL_PTR((*details)->netbios_domain, dwErr);

    (*details)->dns_domain = _wbc_strdup(pDcInfo->pszFullyQualifiedDomainName);
    BAIL_ON_NULL_PTR((*details)->dns_domain, dwErr);

cleanup:
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);

    if (pAccountInfo)
    {
        LsaAdFreeMachineAccountInfo(pAccountInfo);
    }

    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

wbcErr wbcLibraryDetails(struct wbcLibraryDetails **details)
{
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;

    BAIL_ON_NULL_PTR_PARAM(details, dwErr);

    *details = _wbc_malloc(sizeof(struct wbcLibraryDetails), NULL);
    BAIL_ON_NULL_PTR(*details, dwErr);

    (*details)->major_version   = LSA_WBC_LIBRARY_MAJOR_VERSION;
    (*details)->minor_version   = LSA_WBC_LIBRARY_MINOR_VERSION;
    (*details)->vendor_version  = LSA_WBC_LIBRARY_VENDOR_STRING;

cleanup:
    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

