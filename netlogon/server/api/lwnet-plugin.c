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
 *        lwnet-plugin.c
 *
 * Abstract:
 *
 *        BeyondTrust Site Manager
 *
 *        Plugin Support
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *          Danilo Almeida <dalmeida@likewise.com>
 */

#include "includes.h"
#include "lwnet-plugin.h"

//
// Plugin State
//

typedef struct _LWNET_PLUGIN_STATE {
    PSTR pszLibraryPath;
    PVOID LibraryHandle;
    PLWNET_PLUGIN_INTERFACE pInterface;
} LWNET_PLUGIN_STATE, *PLWNET_PLUGIN_STATE;

static LWNET_PLUGIN_STATE gLWNetPluginState;

//
// Local Prototypes
//

static
DWORD
LWNetPreferredDcPluginBuildServerArray(
    IN PLWNET_PLUGIN_SERVER_ADDRESS pDcArray,
    IN DWORD dwDcCount,
    OUT PDNS_SERVER_INFO* ppServerArray,
    OUT PDWORD pdwServerCount
    );

//
// Function Implementations
//

DWORD
LWNetInitializePlugin(
    IN PCSTR pszPath
    )
{
    DWORD dwError = 0;
    PCSTR pszError = NULL;
    LWNET_PLUGIN_GET_INTERFACE_CALLBACK pInitializeCallback = NULL;

    LWNetCleanupPlugin();

    if (!pszPath)
    {
        LWNET_LOG_VERBOSE("No plugin configured");

        dwError = 0;
        goto error;
    }

    LWNET_LOG_VERBOSE("Loading plugin '%s'", pszPath);

    dwError = LWNetAllocateString(pszPath, &gLWNetPluginState.pszLibraryPath);
    BAIL_ON_LWNET_ERROR(dwError);

    dlerror();

    gLWNetPluginState.LibraryHandle = dlopen(gLWNetPluginState.pszLibraryPath, RTLD_NOW | RTLD_GLOBAL);
    if (!gLWNetPluginState.LibraryHandle)
    {
        int error = errno;
        pszError = dlerror();

        LWNET_LOG_ERROR("Failed to load %s (%s (%d))", gLWNetPluginState.pszLibraryPath, LWNET_SAFE_LOG_STRING(pszError), error);

        dwError = ERROR_DLL_INIT_FAILED;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dlerror();
    pInitializeCallback = (LWNET_PLUGIN_GET_INTERFACE_CALLBACK) dlsym(gLWNetPluginState.LibraryHandle, LWNET_PLUGIN_GET_INTERFACE_FUNCTION_NAME);
    if (!pInitializeCallback)
    {
        pszError = dlerror();

        LWNET_LOG_ERROR("Failed to load " LWNET_PLUGIN_GET_INTERFACE_FUNCTION_NAME " function from %s (%s)",
                        gLWNetPluginState.pszLibraryPath, LWNET_SAFE_LOG_STRING(pszError));

        dwError = ERROR_DLL_INIT_FAILED;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = pInitializeCallback(
                    LWNET_PLUGIN_VERSION,
                    &gLWNetPluginState.pInterface);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    if (dwError)
    {
        LWNetCleanupPlugin();
    }

    return dwError;
}

VOID
LWNetCleanupPlugin(
    )
{
    if (gLWNetPluginState.pInterface)
    {
        gLWNetPluginState.pInterface->Cleanup(gLWNetPluginState.pInterface);
        gLWNetPluginState.pInterface = NULL;
    }
    if (gLWNetPluginState.LibraryHandle)
    {
        int err = dlclose(gLWNetPluginState.LibraryHandle);
        if (err)
        {
            LWNET_LOG_ERROR("Failed to dlclose() %s", gLWNetPluginState.pszLibraryPath);
        }
        gLWNetPluginState.LibraryHandle = NULL;
    }
    LWNET_SAFE_FREE_STRING(gLWNetPluginState.pszLibraryPath);
}

DWORD
LWNetGetPreferredDcList(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PDNS_SERVER_INFO* ppServerArray,
    OUT PDWORD pdwServerCount
    )
{
    DWORD dwError = 0;
    PLWNET_PLUGIN_SERVER_ADDRESS pDcArray = NULL;
    DWORD dwDcCount = 0;
    PDNS_SERVER_INFO pServerArray = NULL;
    DWORD dwServerCount = 0;

    if (!gLWNetPluginState.pInterface)
    {
        dwError = NERR_DCNotFound;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = gLWNetPluginState.pInterface->GetDcList(
                    gLWNetPluginState.pInterface,
                    pszDnsDomainName,
                    pszSiteName,
                    dwDsFlags,
                    &pDcArray,
                    &dwDcCount);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetPreferredDcPluginBuildServerArray(
                    pDcArray,
                    dwDcCount,
                    &pServerArray,
                    &dwServerCount);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    if (dwError)
    {
        LWNET_SAFE_FREE_MEMORY(pServerArray);
        dwServerCount = 0;
    }

    if (pDcArray)
    {
        gLWNetPluginState.pInterface->FreeDcList(
                gLWNetPluginState.pInterface,
                pDcArray,
                dwDcCount);
    }

    *ppServerArray = pServerArray;
    *pdwServerCount = dwServerCount;

    return dwError;
}

static
DWORD
LWNetPreferredDcPluginBuildServerArray(
    IN PLWNET_PLUGIN_SERVER_ADDRESS pDcArray,
    IN DWORD dwDcCount,
    OUT PDNS_SERVER_INFO* ppServerArray,
    OUT PDWORD pdwServerCount
    )
{
    DWORD dwError = 0;
    PDNS_SERVER_INFO pServerArray = NULL;
    DWORD dwServerCount = 0;
    DWORD i = 0;    
    DWORD dwStringSize = 0;
    DWORD dwRequiredSize = 0;
    PSTR pStringLocation = NULL;

    if (dwDcCount < 1)
    {
        dwError = NERR_DCNotFound;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    for (i = 0; i < dwDcCount; i++)
    {
        PLWNET_PLUGIN_SERVER_ADDRESS pDcInfo = &pDcArray[i];

        if (!pDcInfo->pszDnsName)
        {
            LWNET_LOG_ERROR("Missing DNS name for preferred DC plugin "
                            "at entry number %u (IP = '%s')",
                            i, LWNET_SAFE_LOG_STRING(pDcInfo->pszIpAddress));

            dwError = ERROR_INTERNAL_ERROR;
            BAIL_ON_LWNET_ERROR(dwError);
        }

        if (!pDcInfo->pszIpAddress)
        {
            LWNET_LOG_ERROR("Missing IP address for preferred DC plugin "
                            "at entry number %u (name = '%s')",
                            i, LWNET_SAFE_LOG_STRING(pDcInfo->pszDnsName));

            dwError = ERROR_INTERNAL_ERROR;
            BAIL_ON_LWNET_ERROR(dwError);
        }

        dwStringSize += strlen(pDcInfo->pszDnsName) + 1;
        dwStringSize += strlen(pDcInfo->pszIpAddress) + 1;
    }

    dwServerCount = dwDcCount;

    dwRequiredSize = dwServerCount * sizeof(DNS_SERVER_INFO) + dwStringSize;

    dwError = LWNetAllocateMemory(dwRequiredSize, (PVOID*)&pServerArray);
    BAIL_ON_LWNET_ERROR(dwError);

    pStringLocation = CT_PTR_ADD(pServerArray, dwServerCount * sizeof(DNS_SERVER_INFO));
    for (i = 0; i < dwDcCount; i++)
    {
        PLWNET_PLUGIN_SERVER_ADDRESS pDcInfo = &pDcArray[i];
        PSTR source;

        // Copy the strings into the buffer
        pServerArray[i].pszName = pStringLocation;
        for (source = pDcInfo->pszDnsName; source[0]; source++)
        {
            pStringLocation[0] = source[0];
            pStringLocation++;
        }
        pStringLocation[0] = source[0];
        pStringLocation++;

        pServerArray[i].pszAddress = pStringLocation;
        for (source = pDcInfo->pszIpAddress; source[0]; source++)
        {
            pStringLocation[0] = source[0];
            pStringLocation++;
        }
        pStringLocation[0] = source[0];
        pStringLocation++;
    }

    // TODO: Turns this into ASSERT
    if (CT_PTR_OFFSET(pServerArray, pStringLocation) != dwRequiredSize)
    {
        LWNET_LOG_ERROR("ASSERT - potential buffer overflow");
    }

error:    
    if (dwError)
    {
        LWNET_SAFE_FREE_MEMORY(pServerArray);
        dwServerCount = 0;
    }

    *ppServerArray = pServerArray;
    *pdwServerCount = dwServerCount;

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
