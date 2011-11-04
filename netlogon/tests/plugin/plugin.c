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
 *        plugin.c
 *
 * Abstract:
 *
 *        Likewise Netlogon
 *
 *        Test Plugin
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#include "lwnet-plugin.h"
// For error codes, we currently need lwnet.h.  In the future,
// lwnet-plugin.h will pull in error codes directly from lwerror.h
#include "lwnet.h"

#include <stdlib.h>
#include <string.h>

static LWNET_PLUGIN_SERVER_ADDRESS gDcList[] = {
    { "a1.b.c", "10.100.100.101" },
    { "a2.b.c", "10.100.100.102" },
    { "a3.b.c", "10.100.100.103" },
    { "a4.b.c", "10.100.100.104" },
};

#define DC_LIST_COUNT (sizeof(gDcList)/sizeof(gDcList[0]))

//
// Prototypes
//


static
VOID
TestPluginFreeDcList(
    LW_IN PLWNET_PLUGIN_INTERFACE pInterface,
    LW_IN LW_OUT PLWNET_PLUGIN_SERVER_ADDRESS pDcArray,
    LW_IN LW_DWORD dwDcCount
    );

//
// Implementations
//

static
DWORD
TestPluginAlloc(
    OUT PVOID* ppMemory,
    IN size_t Size
    )
{
    DWORD dwError = 0;
    PVOID pMemory = NULL;

    pMemory = malloc(Size);
    if (!pMemory)
    {
        dwError = LWNET_ERROR_OUT_OF_MEMORY;
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
TestPluginFree(
    IN OUT PVOID pMemory
    )
{
    if (pMemory)
    {
        free(pMemory);
    }
}

static
DWORD
TestPluginStrdup(
    OUT PSTR* ppszAllocatedString,
    IN PCSTR pszSource
    )
{
    DWORD dwError = 0;
    PSTR pszResult = NULL;

    if (pszSource)
    {
        pszResult = strdup(pszSource);
        if (!pszResult)
        {
            dwError = LWNET_ERROR_OUT_OF_MEMORY;
        }
    }

    *ppszAllocatedString = pszResult;

    return dwError;
}

static
VOID
TestPluginCleanup(
    IN OUT PLWNET_PLUGIN_INTERFACE pInterface
    )
{
    TestPluginFree(pInterface);
}

static
DWORD
TestPluginGetDcList(
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
    PLWNET_PLUGIN_SERVER_ADDRESS pDcArray = NULL;
    DWORD dwDcCount = 0;

    dwDcCount = DC_LIST_COUNT;

    dwError = TestPluginAlloc((PVOID*)&pDcArray, sizeof(pDcArray[0]) * dwDcCount);
    if (dwError)
    {
        goto error;
    }

    for (i = 0; i < dwDcCount; i++)
    {
        dwError = TestPluginStrdup(&pDcArray[i].pszDnsName, gDcList[i].pszDnsName);
        if (dwError)
        {
            goto error;
        }

        dwError = TestPluginStrdup(&pDcArray[i].pszIpAddress, gDcList[i].pszIpAddress);
        if (dwError)
        {
            goto error;
        }
    }

error:
    if (dwError)
    {
        TestPluginFreeDcList(pInterface, pDcArray, dwDcCount);
        pDcArray = NULL;
        dwDcCount = 0;
    }

    *ppDcArray = pDcArray;
    *pdwDcCount = dwDcCount;

    return dwError;
}

static
VOID
TestPluginFreeDcList(
    LW_IN PLWNET_PLUGIN_INTERFACE pInterface,
    LW_IN LW_OUT PLWNET_PLUGIN_SERVER_ADDRESS pDcArray,
    LW_IN LW_DWORD dwDcCount
    )
{
    if (pDcArray)
    {
        DWORD i = 0;
        for (i = 0; i < dwDcCount; i++)
        {
            TestPluginFree(pDcArray[i].pszDnsName);
            TestPluginFree(pDcArray[i].pszIpAddress);
        }
        TestPluginFree(pDcArray);
    }
}

DWORD
LWNetPluginGetInterface(
    IN DWORD dwVersion,
    OUT PLWNET_PLUGIN_INTERFACE* ppInterface
    )
{
    DWORD dwError = 0;
    PLWNET_PLUGIN_INTERFACE pInterface = NULL;

    if (dwVersion != LWNET_PLUGIN_VERSION)
    {
        dwError = LWNET_ERROR_NOT_SUPPORTED;
        goto error;
    }

    dwError = TestPluginAlloc((PVOID*)&pInterface, sizeof(*pInterface));
    if (dwError)
    {
        goto error;
    }

    pInterface->Cleanup = TestPluginCleanup;
    pInterface->GetDcList = TestPluginGetDcList;
    pInterface->FreeDcList = TestPluginFreeDcList;

error:
    if (dwError)
    {
        TestPluginCleanup(pInterface);
        pInterface = NULL;
    }

    *ppInterface = pInterface;

    return dwError;
}
