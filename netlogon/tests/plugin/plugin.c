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
 *        plugin.c
 *
 * Abstract:
 *
 *        BeyondTrust Netlogon
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
