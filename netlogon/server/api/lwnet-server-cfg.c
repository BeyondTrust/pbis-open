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
    PSTR pszBlacklistDCList;
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
    .pszBlacklistDCList = NULL
};

static VOID LWNetSrvApiInitConfig(PLWNET_SERVER_CONFIG pConfig);
static VOID LWNetSrvApiFreeConfig(PLWNET_SERVER_CONFIG pConfig);
static DWORD LwNetSetConfig_BlackListDC(PCSTR pszBlackListDCList);
VOID LWNetFreeMemberInList( PVOID pItem, PVOID pUserData);

static pthread_mutex_t gLWNetServerConfigLock = PTHREAD_MUTEX_INITIALIZER;
static PLW_DLINKED_LIST gBlackListedDC = NULL;


//
// Implementation
//
DWORD
LWNetSrvReadRegistry()
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LWNET_SERVER_CONFIG StagingConfig;

    LWREG_CONFIG_ITEM Config[] =
    {
        {
            "PluginPath",
            FALSE, /* Don't look at policy. */
            LwRegTypeString,
            0,
            -1,
            NULL,
            &StagingConfig.pszPluginPath,
            NULL
        },
        {
            "PingAgainTimeout",
            TRUE, /* Try policy. */
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &StagingConfig.dwPingAgainTimeoutSeconds,
            NULL
        },
        {
            "NegativeCacheTimeout",
            TRUE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &StagingConfig.dwNegativeCacheTimeoutSeconds,
            NULL
        },
        {
            "WritableRediscoveryTimeout",
            TRUE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &StagingConfig.dwWritableRediscoveryTimeoutSeconds,
            NULL
        },
        {
            "WritableTimestampMinimumChange",
            TRUE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &StagingConfig.dwWritableTimestampMinimumChangeSeconds,
            NULL
        },
        {
            "CLdapMaximumConnections",
            TRUE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &StagingConfig.dwCLdapMaximumConnections,
            NULL
        },
        {
            "CLdapSearchTimeout",
            TRUE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &StagingConfig.dwCLdapSearchTimeoutSeconds,
            NULL
        },
        {
            "CLdapSingleConnectionTimeout",
            TRUE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &StagingConfig.dwCLdapSingleConnectionTimeoutSeconds
        },
        {
            "NetBiosUdpTimeout",
            TRUE,
            LwRegTypeDword,
            0,
            60,
            NULL,
            &StagingConfig.dwNetBiosUdpTimeout,
            NULL
        },
        {
            "NetBiosWinsPrimary",
            TRUE,
            0,
            -1,
            60,
            NULL,
            &StagingConfig.pszWinsPrimaryServer
        },
        {
            "NetBiosWinsSecondary",
            TRUE,
            0,
            -1,
            60,
            NULL,
            &StagingConfig.pszWinsSecondaryServer,
            NULL
        },
        {
            "ResolveNameOrder",
            FALSE, /* Don't look at policy. */
            LwRegTypeString,
            0,
            -1,
            NULL,
            &StagingConfig.pszResolveNameOrder,
            NULL
        },
        {
            "BlacklistDC",
            TRUE,
            LwRegTypeMultiString,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.pszBlacklistDCList,
            NULL
        }
    };

    memset(&StagingConfig, 0, sizeof(StagingConfig));

    LWNetSrvApiInitConfig(&StagingConfig);

    dwError = RegProcessConfig(
                "Services\\netlogon\\Parameters",
                "Policy\\Services\\netlogon\\Parameters",
                Config,
                sizeof(Config)/sizeof(Config[0]));
    BAIL_ON_LWNET_ERROR(dwError);


    // Transfer to global config declaration. First free up the old
    // config.
    pthread_mutex_lock(&gLWNetServerConfigLock);

    LWNetSrvApiFreeConfig(&gLWNetServerConfig);

    // Prepare to get the new configuration.
    LWNetSrvApiInitConfig(&gLWNetServerConfig);

    if (StagingConfig.pszPluginPath)
    {
      dwError = LWNetAllocateString(StagingConfig.pszPluginPath, &gLWNetServerConfig.pszPluginPath );
      BAIL_ON_LWNET_ERROR(dwError);
    }

    gLWNetServerConfig.dwPingAgainTimeoutSeconds = StagingConfig.dwPingAgainTimeoutSeconds;
    gLWNetServerConfig.dwNegativeCacheTimeoutSeconds = StagingConfig.dwNegativeCacheTimeoutSeconds;
    gLWNetServerConfig.dwWritableRediscoveryTimeoutSeconds = StagingConfig.dwWritableRediscoveryTimeoutSeconds;
    gLWNetServerConfig.dwWritableTimestampMinimumChangeSeconds = StagingConfig.dwWritableTimestampMinimumChangeSeconds;
    gLWNetServerConfig.dwCLdapMaximumConnections = StagingConfig.dwCLdapMaximumConnections;
    gLWNetServerConfig.dwCLdapSearchTimeoutSeconds = StagingConfig.dwCLdapSearchTimeoutSeconds;
    gLWNetServerConfig.dwCLdapSingleConnectionTimeoutSeconds = StagingConfig.dwCLdapSingleConnectionTimeoutSeconds;
    gLWNetServerConfig.dwNetBiosUdpTimeout = StagingConfig.dwNetBiosUdpTimeout;

    dwError = LWNetAllocateString(StagingConfig.pszWinsPrimaryServer, &gLWNetServerConfig.pszWinsPrimaryServer);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(StagingConfig.pszWinsSecondaryServer, &gLWNetServerConfig.pszWinsSecondaryServer);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(StagingConfig.pszResolveNameOrder, &gLWNetServerConfig.pszResolveNameOrder);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwNetSetConfig_BlackListDC(StagingConfig.pszBlacklistDCList);

    LWNetSrvApiFreeConfig(&StagingConfig);

error:

    pthread_mutex_unlock(&gLWNetServerConfigLock);

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

static
DWORD
LwNetSetConfig_BlackListDC(PCSTR pszBlackListDCList)        
{
    DWORD dwError = 0;
    PCSTR pszIter = NULL;
    PSTR  pszMember = NULL;

    if (gBlackListedDC)
    {
        LwDLinkedListForEach( gBlackListedDC, &LWNetFreeMemberInList, NULL);
        gBlackListedDC = NULL;
    }

    pszIter = pszBlackListDCList;
    while (pszIter != NULL && *pszIter != '\0')
    {
        PSTR pszEnd;

        while (*pszIter == ' ')
        {
            ++pszIter;
        }

        dwError = LwStrDupOrNull(
                        pszIter,
                        &pszMember);
        BAIL_ON_LWNET_ERROR(dwError);

        pszEnd = pszMember + strlen(pszMember);

        while (pszEnd > pszMember && pszEnd[-1] == ' ')
        {
            --pszEnd;
        }

        *pszEnd = '\0';

        dwError = LwDLinkedListAppend(&gBlackListedDC, pszMember);
        BAIL_ON_LWNET_ERROR(dwError);

        pszMember = NULL;

        pszIter += strlen(pszIter) + 1;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszMember);

    return dwError;

error:

    goto cleanup;
}

DWORD
LWNet_GetConfiguredBlackListDC(
    PDWORD pdwBlackListCount, 
    PSTR ppszBlackList[]
    )
{
    DWORD dwError = 0;
    DWORD dwNumMembers = 0;
    PLW_DLINKED_LIST pIter = NULL;

    for (pIter = gBlackListedDC; pIter; pIter = pIter->pNext)
    {
        dwNumMembers++;
    }

    if (dwNumMembers)
    {
        DWORD iMember = 0;

        for (pIter = gBlackListedDC; pIter; pIter = pIter->pNext, iMember++)
        {
            dwError = LwAllocateString((PSTR)pIter->pItem, &ppszBlackList[iMember]);
            BAIL_ON_LWNET_ERROR(dwError);
        }
    }

    *pdwBlackListCount = dwNumMembers;

cleanup:

    return dwError;

error:

    *pdwBlackListCount = 0;

    goto cleanup;
}


static
VOID
LWNetSrvApiInitConfig(PLWNET_SERVER_CONFIG pConfig)
{
    pConfig->dwPingAgainTimeoutSeconds = LWNET_PING_AGAIN_TIMEOUT_SECONDS,
    pConfig->dwNegativeCacheTimeoutSeconds = LWNET_NEGATIVE_CACHE_TIMEOUT_SECONDS,
    pConfig->dwWritableRediscoveryTimeoutSeconds = LWNET_WRITABLE_REDISCOVERY_TIMEOUT_SECONDS,
    pConfig->dwWritableTimestampMinimumChangeSeconds = LWNET_WRITABLE_TIMESTAMP_MINIMUM_CHANGE_SECONDS,
    pConfig->dwCLdapMaximumConnections = LWNET_CLDAP_DEFAULT_MAXIMUM_CONNECTIONS,
    pConfig->dwCLdapSearchTimeoutSeconds = LWNET_CLDAP_DEFAULT_TIMEOUT_SECONDS,
    pConfig->dwCLdapSingleConnectionTimeoutSeconds = LWNET_CLDAP_DEFAULT_TIMEOUT_SECONDS,
    pConfig->dwNetBiosUdpTimeout = 2,

    pConfig->pszBlacklistDCList = NULL;
    pConfig->pszPluginPath = NULL;
    pConfig->pszWinsPrimaryServer = NULL;
    pConfig->pszWinsSecondaryServer = NULL;
    pConfig->pszResolveNameOrder = NULL;

   return;
}

static
VOID
LWNetSrvApiFreeConfig(PLWNET_SERVER_CONFIG pConfig)
{
    LW_SAFE_FREE_STRING(pConfig->pszBlacklistDCList);
    pConfig->pszBlacklistDCList = NULL;

    LW_SAFE_FREE_STRING(pConfig->pszPluginPath);
    pConfig->pszPluginPath = NULL;

    LW_SAFE_FREE_STRING(pConfig->pszWinsPrimaryServer);
    pConfig->pszWinsPrimaryServer = NULL;

    LW_SAFE_FREE_STRING(pConfig->pszWinsSecondaryServer);
    pConfig->pszWinsSecondaryServer = NULL;

    LW_SAFE_FREE_STRING(pConfig->pszResolveNameOrder);
    pConfig->pszResolveNameOrder = NULL;

   return;
}

DWORD 
LWNetSrvRefreshConfiguration(HANDLE hServer)
{
   DWORD dwError = 0;
   PLWNET_SRV_API_STATE pServerState = (PLWNET_SRV_API_STATE)hServer;

   if (pServerState->peerUID)
   {
       dwError = LW_ERROR_ACCESS_DENIED;
       BAIL_ON_LWNET_ERROR(dwError);
   }

    dwError = LWNetSrvReadRegistry();
    BAIL_ON_LWNET_ERROR(dwError);


cleanup:

    return dwError;


error:

    goto cleanup;

}


VOID
LWNetFreeMemberInList(
    PVOID pItem,
    PVOID pUserData
    )
{
    LW_SAFE_FREE_MEMORY(pItem);
}
