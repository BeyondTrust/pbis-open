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
 *        site.c
 *
 * Abstract:
 *
 *        AD Bridge Netlogon
 *
 *        Preferred Site Plugin
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "includes.h"

#include "lwnet-plugin.h"
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
