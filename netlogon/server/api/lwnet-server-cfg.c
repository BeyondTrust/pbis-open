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
 *        lwnet-server-cfg.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#include "includes.h"

//
// Internal Module Globals
//

typedef struct _LWNET_SERVER_CONFIG {
    PSTR pszPluginPath;
    DWORD dwPingAgainTimeoutSeconds;
    DWORD dwNegativeCacheTimeoutSeconds;
    DWORD dwWritableRediscoveryTimeoutSeconds;
    DWORD dwWritableTimestampMinimumChangeSeconds;
    DWORD dwCLdapMaximumConnections;
    DWORD dwCLdapSearchTimeoutSeconds;
    DWORD dwCLdapSingleConnectionTimeoutSeconds;
    DWORD dwNetBiosUdpTimeout;
    PSTR pszWinsPrimaryServer;
    PSTR pszWinsSecondaryServer;
    PSTR pszResolveNameOrder;
} LWNET_SERVER_CONFIG, *PLWNET_SERVER_CONFIG;

#define LWNET_PING_AGAIN_TIMEOUT_SECONDS (15 * 60)
#define LWNET_NEGATIVE_CACHE_TIMEOUT_SECONDS (1 * 60)
    
#define LWNET_WRITABLE_REDISCOVERY_TIMEOUT_SECONDS (30 * 60)
#define LWNET_WRITABLE_TIMESTAMP_MINIMUM_CHANGE_SECONDS (0 * 60)

LWNET_SERVER_CONFIG gLWNetServerConfig = {
    .pszPluginPath = NULL,
    .dwPingAgainTimeoutSeconds = LWNET_PING_AGAIN_TIMEOUT_SECONDS,
    .dwNegativeCacheTimeoutSeconds = LWNET_NEGATIVE_CACHE_TIMEOUT_SECONDS,
    .dwWritableRediscoveryTimeoutSeconds = LWNET_WRITABLE_REDISCOVERY_TIMEOUT_SECONDS,
    .dwWritableTimestampMinimumChangeSeconds = LWNET_WRITABLE_TIMESTAMP_MINIMUM_CHANGE_SECONDS,
    .dwCLdapMaximumConnections = LWNET_CLDAP_DEFAULT_MAXIMUM_CONNECTIONS,
    .dwCLdapSearchTimeoutSeconds = LWNET_CLDAP_DEFAULT_TIMEOUT_SECONDS,
    .dwCLdapSingleConnectionTimeoutSeconds = LWNET_CLDAP_DEFAULT_TIMEOUT_SECONDS,
    .dwNetBiosUdpTimeout = 2,
};

static
LWREG_CONFIG_ITEM gConfig[] =
{
    {
        "PluginPath",
        FALSE, /* Don't look at policy. */
        LwRegTypeString,
        0,
        -1,
        NULL,
        &gLWNetServerConfig.pszPluginPath,
        NULL
    },
    {
        "PingAgainTimeout",
        TRUE, /* Try policy. */
        LwRegTypeDword,
        0,
        -1,
        NULL,
        &gLWNetServerConfig.dwPingAgainTimeoutSeconds,
        NULL
    },
    {
        "NegativeCacheTimeout",
        TRUE,
        LwRegTypeDword,
        0,
        -1,
        NULL,
        &gLWNetServerConfig.dwNegativeCacheTimeoutSeconds,
        NULL
    },
    {
        "WritableRediscoveryTimeout",
        TRUE,
        LwRegTypeDword,
        0,
        -1,
        NULL,
        &gLWNetServerConfig.dwWritableRediscoveryTimeoutSeconds,
        NULL
    },
    {
        "WritableTimestampMinimumChange",
        TRUE,
        LwRegTypeDword,
        0,
        -1,
        NULL,
        &gLWNetServerConfig.dwWritableTimestampMinimumChangeSeconds,
        NULL
    },
    {
        "CLdapMaximumConnections",
        TRUE,
        LwRegTypeDword,
        0,
        -1,
        NULL,
        &gLWNetServerConfig.dwCLdapMaximumConnections,
        NULL
    },
    {
        "CLdapSearchTimeout",
        TRUE,
        LwRegTypeDword,
        0,
        -1,
        NULL,
        &gLWNetServerConfig.dwCLdapSearchTimeoutSeconds,
        NULL
    },
    {
        "CLdapSingleConnectionTimeout",
        TRUE,
        LwRegTypeDword,
        0,
        -1,
        NULL,
        &gLWNetServerConfig.dwCLdapSingleConnectionTimeoutSeconds
    },
    {
        "NetBiosUdpTimeout",
        TRUE,
        LwRegTypeDword,
        0,
        60,
        NULL,
        &gLWNetServerConfig.dwNetBiosUdpTimeout,
        NULL
    },
    {
        "NetBiosWinsPrimary",
        TRUE,
        0,
        -1,
        60,
        NULL,
        &gLWNetServerConfig.pszWinsPrimaryServer
    },
    {
        "NetBiosWinsSecondary",
        TRUE,
        0,
        -1,
        60,
        NULL,
        &gLWNetServerConfig.pszWinsSecondaryServer,
        NULL
    },
    {
        "ResolveNameOrder",
        FALSE, /* Don't look at policy. */
        LwRegTypeString,
        0,
        -1,
        NULL,
        &gLWNetServerConfig.pszResolveNameOrder,
        NULL
    },
};

//
// Implementation
//
DWORD
LWNetSrvReadRegistry(
   )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    dwError = RegProcessConfig(
                "Services\\netlogon\\Parameters",
                "Policy\\Services\\netlogon\\Parameters",
                gConfig,
                sizeof(gConfig)/sizeof(gConfig[0]));
    BAIL_ON_LWNET_ERROR(dwError);

error:

    return dwError;
}


PCSTR
LWNetConfigGetPluginPath(
    VOID
    )
{
    return gLWNetServerConfig.pszPluginPath;
}

DWORD
LWNetConfigGetPingAgainTimeoutSeconds(
    VOID
    )
{
    return gLWNetServerConfig.dwPingAgainTimeoutSeconds;
}

DWORD
LWNetConfigGetNegativeCacheTimeoutSeconds(
    VOID
    )
{
    return gLWNetServerConfig.dwNegativeCacheTimeoutSeconds;
}

DWORD
LWNetConfigGetWritableRediscoveryTimeoutSeconds(
    VOID
    )
{
    return gLWNetServerConfig.dwWritableRediscoveryTimeoutSeconds;
}
DWORD
LWNetConfigGetWritableTimestampMinimumChangeSeconds(
    VOID
    )
{
    return gLWNetServerConfig.dwWritableTimestampMinimumChangeSeconds;
}

DWORD
LWNetConfigGetCLdapMaximumConnections(
    VOID
    )
{
    return gLWNetServerConfig.dwCLdapMaximumConnections;
}

DWORD
LWNetConfigGetCLdapSearchTimeoutSeconds(
    VOID
    )
{
    return gLWNetServerConfig.dwCLdapSearchTimeoutSeconds;
}

DWORD
LWNetConfigGetCLdapSingleConnectionTimeoutSeconds(
    VOID
    )
{
    return gLWNetServerConfig.dwCLdapSingleConnectionTimeoutSeconds;
}

DWORD
LWNetConfigIsNetBiosEnabled(
    VOID
    )
{
    BOOLEAN bNetBiosEnabled = FALSE;

    if (!gLWNetServerConfig.pszResolveNameOrder)
    {
        bNetBiosEnabled = FALSE;
    }
    else
    {
        bNetBiosEnabled |= RtlCStringFindSubstring(
                               gLWNetServerConfig.pszResolveNameOrder,
                               "WINS",
                               FALSE,
                               NULL);
        bNetBiosEnabled |= RtlCStringFindSubstring(
                               gLWNetServerConfig.pszResolveNameOrder,
                               "NETBIOS",
                               FALSE,
                               NULL);
    }
    return bNetBiosEnabled;
}


DWORD
LWNetConfigResolveNameOrder(
    PDWORD *nameOrder,
    PDWORD nameOrderLen
    )
{
    DWORD dwError = 0;
    PDWORD retNameOrder = NULL;
    DWORD i = 0;
    PSTR tmpNameOrder = NULL;
    PSTR strtokPtr = NULL;
    PSTR strToken = NULL;
    BOOLEAN bHasDns = FALSE;

    if (!gLWNetServerConfig.pszResolveNameOrder ||
        !gLWNetServerConfig.pszResolveNameOrder[0])
    {
        dwError = LwRtlCStringDuplicate(
                      &gLWNetServerConfig.pszResolveNameOrder,
                      "DNS");
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetAllocateMemory(3 * sizeof(*retNameOrder), (PVOID*)&retNameOrder);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwRtlCStringDuplicate(&tmpNameOrder, 
                                    gLWNetServerConfig.pszResolveNameOrder);
    BAIL_ON_LWNET_ERROR(dwError);

    strtokPtr = tmpNameOrder;
    do
    {
        strToken = strtok_r(strtokPtr, " ", &strtokPtr);
        if (strToken && i < 3)
        {
            if (!LwRtlCStringCompare(strToken, "DNS", FALSE))
            {
                retNameOrder[i++] = LWNET_RESOLVE_HOST_DNS;
                bHasDns = TRUE;
            }
            else if (!LwRtlCStringCompare(strToken, "NETBIOS", FALSE))
            {
                retNameOrder[i++] = LWNET_RESOLVE_HOST_NETBIOS;
            }
            else if (!LwRtlCStringCompare(strToken, "WINS", FALSE))
            {
                retNameOrder[i++] = LWNET_RESOLVE_HOST_WINS;
            }
        }
    } while (strToken);

    if (!bHasDns)
    {
        /* Must resolve against DNS for lwio/lsass to function */
        retNameOrder[i++] = LWNET_RESOLVE_HOST_DNS;
    }

    *nameOrder = retNameOrder;
    *nameOrderLen = i;
    
cleanup:
    return dwError;

error:
    LWNET_SAFE_FREE_STRING(tmpNameOrder);
    LWNET_SAFE_FREE_MEMORY(retNameOrder);

    goto cleanup;
}


DWORD
LWNetConfigIsNetBiosUdpTimeout(
    VOID
    )
{
    return gLWNetServerConfig.dwNetBiosUdpTimeout;
}


VOID
LwNetConfigGetWinsServers(
    PSTR *primaryServer,
    PSTR *secondaryServer
    )
{
    if (primaryServer)
    {
        *primaryServer = gLWNetServerConfig.pszWinsPrimaryServer;
    }
    if (secondaryServer)
    {
        *secondaryServer = gLWNetServerConfig.pszWinsSecondaryServer;
    }
}
