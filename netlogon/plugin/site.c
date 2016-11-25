/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        site.c
 *
 * Abstract:
 *
 *        PBIS Netlogon
 *
 *        Preferred Site Plugin
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "lwnet-plugin.h"
#include "lwnet-utils.h"
#include "lwerror.h"

typedef struct _SITE_PLUGIN_INTERFACE 
{
    LWNET_PLUGIN_INTERFACE iface;
    LW_PSTR pszSiteName;
} SITE_PLUGIN_INTERFACE, *PSITE_PLUGIN_INTERFACE;

typedef struct _SITE_PLUGIN_RESULT 
{
    PDNS_SERVER_INFO pServerInfo;
    LWNET_PLUGIN_SERVER_ADDRESS ServerAddress[0];
} SITE_PLUGIN_RESULT, *PSITE_PLUGIN_RESULT;

static
DWORD
SitePluginAlloc(
    OUT PVOID* ppMemory,
    IN size_t Size
    )
{
    DWORD dwError = 0;
    PVOID pMemory = NULL;

    pMemory = malloc(Size);
    if (!pMemory)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        memset(pMemory, 0, Size);
    }

    *ppMemory = pMemory;
    return dwError;
}

static
VOID
SitePluginCleanup(
    IN OUT PLWNET_PLUGIN_INTERFACE pInterface
    )
{
    if (pInterface) {
        PSITE_PLUGIN_INTERFACE ctx = LW_STRUCT_FROM_FIELD(pInterface, SITE_PLUGIN_INTERFACE, iface);

        if (ctx) {
            if (ctx->pszSiteName) free(ctx->pszSiteName);
            free(ctx);
        }
    }
}

static
VOID
SitePluginFreeResult(LW_IN PSITE_PLUGIN_RESULT pResult)
{
    if (pResult) {
        if (pResult->pServerInfo) free(pResult->pServerInfo);
        free(pResult);
    }
}

static
DWORD
SitePluginGetDcList(
    LW_IN PLWNET_PLUGIN_INTERFACE pInterface,
    LW_IN LW_PCSTR pszDnsDomainName,
    LW_IN LW_OPTIONAL LW_PCSTR pszSiteName,
    LW_IN LW_DWORD dwDsFlags,
    LW_OUT PLWNET_PLUGIN_SERVER_ADDRESS* ppDcArray,
    LW_OUT LW_PDWORD pdwDcCount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSITE_PLUGIN_RESULT pResult = NULL;
    PDNS_SERVER_INFO pServerInfo;
    DWORD dwDcCount = 0;
    PSITE_PLUGIN_INTERFACE ctx = LW_STRUCT_FROM_FIELD(pInterface, SITE_PLUGIN_INTERFACE, iface);

    dwError = LWNetDnsSrvQuery(pszDnsDomainName, pszSiteName ? pszSiteName : ctx->pszSiteName, dwDsFlags, &pServerInfo, &dwDcCount);

    if (dwDcCount > 0) {
        dwError = SitePluginAlloc((PVOID*)&pResult, sizeof(SITE_PLUGIN_RESULT) + sizeof(LWNET_PLUGIN_SERVER_ADDRESS) * dwDcCount);

        if (dwError) goto error;
    }

    for (i = 0; i < dwDcCount; i++)
    {
        pResult->ServerAddress[i].pszDnsName = pServerInfo[i].pszName;
        pResult->ServerAddress[i].pszIpAddress = pServerInfo[i].pszAddress;
    }

error:
    if (dwError)
    {
        SitePluginFreeResult(pResult);
        dwDcCount = 0;
    }

    if (pResult) *ppDcArray = pResult->ServerAddress;
    
    *pdwDcCount = dwDcCount;

    return dwError;
}

static
VOID
SitePluginFreeDcList(
    LW_IN PLWNET_PLUGIN_INTERFACE pInterface,
    LW_IN LW_OUT PLWNET_PLUGIN_SERVER_ADDRESS pDcArray,
    LW_IN LW_DWORD dwDcCount
    )
{
    if (pDcArray)
    {
        PSITE_PLUGIN_RESULT pResult = LW_STRUCT_FROM_FIELD(pDcArray, SITE_PLUGIN_RESULT, ServerAddress);

        SitePluginFreeResult(pResult);
    }
}

static
DWORD
SitePluginReadRegistry(PSITE_PLUGIN_INTERFACE pInterface)
{
    DWORD dwError = LW_ERROR_SUCCESS;

    LWREG_CONFIG_ITEM Config[] =
    {
        {
            "SiteName",
            TRUE, /* Look at policy in case this gets added to Group Policy later. */
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pInterface->pszSiteName,
            NULL
        }
    };

    dwError = RegProcessConfig(
                "Services\\netlogon\\Parameters",
                "Policy\\Services\\netlogon\\Parameters",
                Config,
                sizeof(Config)/sizeof(Config[0]));

    
    return dwError;
}

DWORD
LWNetPluginGetInterface(
    IN DWORD dwVersion,
    OUT PLWNET_PLUGIN_INTERFACE* ppInterface
    )
{
    DWORD dwError = 0;
    PSITE_PLUGIN_INTERFACE pInterface = NULL;

    if (dwVersion != LWNET_PLUGIN_VERSION)
    {
        dwError = LW_ERROR_NOT_SUPPORTED;
        goto error;
    }

    dwError = SitePluginAlloc((PVOID*)&pInterface, sizeof(*pInterface));
    if (dwError) goto error;

    SitePluginReadRegistry(pInterface);

    pInterface->iface.Cleanup = SitePluginCleanup;
    pInterface->iface.GetDcList = SitePluginGetDcList;
    pInterface->iface.FreeDcList = SitePluginFreeDcList;

error:
    if (dwError)
    {
        if (pInterface) free(pInterface);
        pInterface = NULL;
    }

    if (pInterface) *ppInterface = &pInterface->iface;

    return dwError;
}
