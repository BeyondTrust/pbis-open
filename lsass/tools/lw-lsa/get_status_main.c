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
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Tool to get status from LSA Server
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "config.h"
#include "common.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"
#include "lsa/ad.h"

#include "lsaclient.h"
#include "lsaipc.h"
#include "lsaadprovider.h"
#include "lsalocalprovider.h"

#define LSA_MODE_STRING_UNKNOWN          "Unknown"
#define LSA_MODE_STRING_UNPROVISIONED    "Un-provisioned"
#define LSA_MODE_STRING_DEFAULT_CELL     "Default Cell"
#define LSA_MODE_STRING_NON_DEFAULT_CELL "Non-default Cell"
#define LSA_MODE_STRING_LOCAL            "Local system"

#define LSA_SUBMODE_STRING_UNKNOWN       "Unknown"
#define PBIS_SUBMODE_STRING_SCHEMA       "Directory Integrated"
#define PBIS_SUBMODE_STRING_NON_SCHEMA   "Schemaless"
#define LSA_SUBMODE_STRING_SCHEMA        "Schema"
#define LSA_SUBMODE_STRING_NON_SCHEMA    "Non-schema"

#define LSA_STATUS_STRING_UNKNOWN        "Unknown"
#define LSA_STATUS_STRING_ONLINE         "Online"
#define LSA_STATUS_STRING_OFFLINE        "Offline"

#define LSA_TRUST_TYPE_STRING_UNKNOWN    "Unknown"
#define LSA_TRUST_TYPE_STRING_DOWNLEVEL  "Down Level"
#define LSA_TRUST_TYPE_STRING_UPLEVEL    "Up Level"
#define LSA_TRUST_TYPE_STRING_MIT        "MIT"
#define LSA_TRUST_TYPE_STRING_DCE        "DCE"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

extern int pb_mode;

static
VOID
ParseArgs(
    IN int    argc,
    IN char*  argv[],
    OUT PBOOLEAN pbAll,
    OUT PCSTR* ppszDomainName
    );

static
VOID
ShowUsage();

static
VOID
PrintStatus(
    PLSASTATUS pStatus,
    PBOOLEAN bpPrintedServerStatus
    );

static
PCSTR
GetStatusString(
    LsaAuthProviderStatus status
    );

static
PCSTR
GetModeString(
    LsaAuthProviderMode mode
    );

static
PCSTR
GetSubmodeString(
    LsaAuthProviderSubMode subMode
    );

static
PCSTR
GetTrustTypeString(
    DWORD dwTrustType
    );

static
DWORD
MapErrorCode(
    DWORD dwError
    );


int
get_status_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PLSASTATUS pLsaStatus = NULL;
    HANDLE hLsaConnection = (HANDLE)NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    BOOLEAN bAll = FALSE;
    PSTR pszProviderInstance = NULL;
    PSTR* ppszDomains = NULL;
    DWORD dwNumDomains = 0;
    DWORD dwIndex = 0;
    BOOLEAN bPrintedServerStatus = FALSE;
    PCSTR pszDomainName = NULL;
    
    ParseArgs(argc, argv, &bAll, &pszDomainName);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (bAll)
    {
        dwError = LsaAdGetJoinedDomains(
                      hLsaConnection,
                      &dwNumDomains,
                      &ppszDomains);
        BAIL_ON_LSA_ERROR(dwError);

        if (dwNumDomains)
        {
            for (dwIndex = 0 ; dwIndex < dwNumDomains ; dwIndex++)
            {
                dwError = LwAllocateStringPrintf(
                              &pszProviderInstance,
                              "%s:%s",
                              LSA_PROVIDER_TAG_AD,
                              ppszDomains[dwIndex]);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaGetStatus2(
                              hLsaConnection,
                              pszProviderInstance,
                              &pLsaStatus);
                BAIL_ON_LSA_ERROR(dwError);

                PrintStatus(pLsaStatus, &bPrintedServerStatus);

                LsaFreeStatus(pLsaStatus);
                pLsaStatus = NULL;

                LW_SAFE_FREE_STRING(pszProviderInstance);
            }
        }
        else
        {
            dwError = LsaGetStatus2(
                          hLsaConnection,
                          LSA_PROVIDER_TAG_AD,
                          &pLsaStatus);
            BAIL_ON_LSA_ERROR(dwError);

            PrintStatus(pLsaStatus, &bPrintedServerStatus);

            LsaFreeStatus(pLsaStatus);
            pLsaStatus = NULL;

            LW_SAFE_FREE_STRING(pszProviderInstance);
        }

        dwError = LsaGetStatus2(
                      hLsaConnection,
                      LSA_PROVIDER_TAG_LOCAL,
                      &pLsaStatus);
        BAIL_ON_LSA_ERROR(dwError);
    
        PrintStatus(pLsaStatus, &bPrintedServerStatus);
    }
    else
    {
        if(pszDomainName)
        {
            dwError = LwAllocateStringPrintf(
                          &pszProviderInstance,
                          "%s:%s",
                          LSA_PROVIDER_TAG_AD,
                          pszDomainName);
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaGetStatus2(
                      hLsaConnection,
                      pszProviderInstance,
                      &pLsaStatus);
        BAIL_ON_LSA_ERROR(dwError);
    
        PrintStatus(pLsaStatus, &bPrintedServerStatus);
    }

cleanup:

    if (pLsaStatus) {
       LsaFreeStatus(pLsaStatus);
    }
    
    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    LwFreeStringArray(ppszDomains, dwNumDomains);

    LW_SAFE_FREE_STRING(pszProviderInstance);

    return (dwError);

error:

    dwError = MapErrorCode(dwError);
    
    dwErrorBufferSize = LwGetErrorString(dwError, NULL, 0);
    
    if (dwErrorBufferSize > 0)
    {
        DWORD dwError2 = 0;
        PSTR   pszErrorBuffer = NULL;
        
        dwError2 = LwAllocateMemory(
                    dwErrorBufferSize,
                    (PVOID*)&pszErrorBuffer);
        
        if (!dwError2)
        {
            DWORD dwLen = LwGetErrorString(dwError, pszErrorBuffer, dwErrorBufferSize);
            
            if ((dwLen == dwErrorBufferSize) && !LW_IS_NULL_OR_EMPTY_STR(pszErrorBuffer))
            {
                fprintf(
                    stderr,
                    "Failed to query status from LSA service.  Error code %u (%s).\n%s\n",
                    dwError,
                    LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)),
                    pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }
        
        LW_SAFE_FREE_STRING(pszErrorBuffer);
    }
    
    if (bPrintOrigError)
    {
        fprintf(
            stderr,
            "Failed to query status from LSA service.  Error code %u (%s).\n",
            dwError,
            LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
    }

    goto cleanup;
}

VOID
ParseArgs(
    IN int    argc,
    IN char*  argv[],
    OUT PBOOLEAN pbAll,
    OUT PCSTR* ppszDomainName
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0
        } ParseMode;
        
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    BOOLEAN bAll = FALSE;
    PCSTR pszDomainName = NULL;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }
        
        switch (parseMode)
        {
            case PARSE_MODE_OPEN:
        
                if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
                {
                    ShowUsage();
                    exit(0);
                }
                else if (strcmp(pszArg, "--all") == 0)
                {
                    bAll = TRUE;
                }
                else
                {
                    if (pszDomainName)
                    {
                        ShowUsage();
                        exit(1);
                    }

                    pszDomainName = pszArg;
                }

                break;
        }
        
    } while (iArg < argc);

    *pbAll = bAll;
    *ppszDomainName = pszDomainName;
}

void
ShowUsage()
{
    printf("Usage: get-status [--all] [domain]\n");
}

VOID
PrintStatus(
    PLSASTATUS pStatus,
    PBOOLEAN pbPrintedServerStatus
    )
{
    DWORD iCount = 0;
    DWORD dwDays = pStatus->dwUptime/LSA_SECONDS_IN_DAY;
    DWORD dwHours = (pStatus->dwUptime - (dwDays * LSA_SECONDS_IN_DAY))/LSA_SECONDS_IN_HOUR;
    DWORD dwMins = (pStatus->dwUptime - 
                    (dwDays * LSA_SECONDS_IN_DAY) - 
                    (dwHours * LSA_SECONDS_IN_HOUR))/LSA_SECONDS_IN_MINUTE;
    DWORD dwSecs = (pStatus->dwUptime - 
                    (dwDays * LSA_SECONDS_IN_DAY) - 
                    (dwHours * LSA_SECONDS_IN_HOUR) - 
                    (dwMins * LSA_SECONDS_IN_MINUTE));

    if (!*pbPrintedServerStatus)
    {
        printf("LSA Server Status:\n\n");
        printf("Compiled daemon version: %u.%u.%u.%u\n",
                    pStatus->lsassVersion.dwMajor,
                    pStatus->lsassVersion.dwMinor,
                    pStatus->lsassVersion.dwBuild,
                    pStatus->lsassVersion.dwRevision);
        printf("Packaged product version: %u.%u.%u.%u\n",
                    pStatus->productVersion.dwMajor,
                    pStatus->productVersion.dwMinor,
                    pStatus->productVersion.dwBuild,
                    pStatus->productVersion.dwRevision);
        printf("Uptime:        %u days %u hours %u minutes %u seconds\n", dwDays, dwHours, dwMins, dwSecs);

        *pbPrintedServerStatus = TRUE;
    }
    
    for (iCount = 0; iCount < pStatus->dwCount; iCount++)
    {
        PLSA_AUTH_PROVIDER_STATUS pProviderStatus =
            &pStatus->pAuthProviderStatusList[iCount];
        
        printf("\n[Authentication provider: %s]\n\n", 
                        LW_IS_NULL_OR_EMPTY_STR(pProviderStatus->pszId) ? "" : pProviderStatus->pszId);
        
        printf("\tStatus:        %s\n", GetStatusString(pProviderStatus->status));
        printf("\tMode:          %s\n", GetModeString(pProviderStatus->mode));

        if (pProviderStatus->mode != LSA_PROVIDER_MODE_UNKNOWN)
        {
            printf("\tDomain:        %s\n", LW_IS_NULL_OR_EMPTY_STR(pProviderStatus->pszDomain) ? "" : pProviderStatus->pszDomain);
            printf("\tDomain SID:    %s\n", LW_IS_NULL_OR_EMPTY_STR(pProviderStatus->pszDomainSid) ? "" : pProviderStatus->pszDomainSid);
        }

        if (pProviderStatus->mode == LSA_PROVIDER_MODE_UNPROVISIONED ||
            pProviderStatus->mode == LSA_PROVIDER_MODE_DEFAULT_CELL ||
            pProviderStatus->mode == LSA_PROVIDER_MODE_NON_DEFAULT_CELL)
        {
            printf("\tForest:        %s\n", LW_IS_NULL_OR_EMPTY_STR(pProviderStatus->pszForest) ? "" : pProviderStatus->pszForest);
            printf("\tSite:          %s\n", LW_IS_NULL_OR_EMPTY_STR(pProviderStatus->pszSite) ? "" : pProviderStatus->pszSite);
            printf("\tOnline check interval:  %u seconds\n", pProviderStatus->dwNetworkCheckInterval);
        }

        switch (pProviderStatus->mode)
        {
            case LSA_PROVIDER_MODE_DEFAULT_CELL:
                
                printf("\tSub mode:      %s\n", GetSubmodeString(pProviderStatus->subMode));
                
                break;
                
            case LSA_PROVIDER_MODE_NON_DEFAULT_CELL:
                
                printf("\tSub mode:      %s\n", GetSubmodeString(pProviderStatus->subMode));
                
                printf("\tCell:          %s\n", LW_IS_NULL_OR_EMPTY_STR(pProviderStatus->pszCell) ? "" : pProviderStatus->pszCell);
                
                break;
                
            default:
                
                break;
        }
        
        if (pProviderStatus->pTrustedDomainInfoArray)
        {
            DWORD iDomain = 0;
            
            printf("\t[Trusted Domains: %u]\n\n", pProviderStatus->dwNumTrustedDomains);
            
            for (; iDomain < pProviderStatus->dwNumTrustedDomains; iDomain++)
            {
                PLSA_TRUSTED_DOMAIN_INFO pDomainInfo =
                    &pProviderStatus->pTrustedDomainInfoArray[iDomain];
                
                printf("\n\t[Domain: %s]\n\n", LW_IS_NULL_OR_EMPTY_STR(pDomainInfo->pszNetbiosDomain) ? "" : pDomainInfo->pszNetbiosDomain);
                
                printf("\t\tDNS Domain:       %s\n", LW_IS_NULL_OR_EMPTY_STR(pDomainInfo->pszDnsDomain) ? "" : pDomainInfo->pszDnsDomain);
                printf("\t\tNetbios name:     %s\n", LW_IS_NULL_OR_EMPTY_STR(pDomainInfo->pszNetbiosDomain) ? "" : pDomainInfo->pszNetbiosDomain);
                printf("\t\tForest name:      %s\n", LW_IS_NULL_OR_EMPTY_STR(pDomainInfo->pszForestName) ? "" : pDomainInfo->pszForestName);
                printf("\t\tTrustee DNS name: %s\n", LW_IS_NULL_OR_EMPTY_STR(pDomainInfo->pszTrusteeDnsDomain) ? "" : pDomainInfo->pszTrusteeDnsDomain);
                printf("\t\tClient site name: %s\n", LW_IS_NULL_OR_EMPTY_STR(pDomainInfo->pszClientSiteName) ? "" : pDomainInfo->pszClientSiteName);
                printf("\t\tDomain SID:       %s\n", LW_IS_NULL_OR_EMPTY_STR(pDomainInfo->pszDomainSID) ? "" : pDomainInfo->pszDomainSID);
                printf("\t\tDomain GUID:      %s\n", LW_IS_NULL_OR_EMPTY_STR(pDomainInfo->pszDomainGUID) ? "" : pDomainInfo->pszDomainGUID);
                printf("\t\tTrust Flags:      [0x%.04x]\n", pDomainInfo->dwTrustFlags);
                if (pDomainInfo->dwTrustFlags & LSA_TRUST_FLAG_IN_FOREST)
                    printf("\t\t                  [0x%.04x - In forest]\n", LSA_TRUST_FLAG_IN_FOREST);
                if (pDomainInfo->dwTrustFlags & LSA_TRUST_FLAG_OUTBOUND)
                    printf("\t\t                  [0x%.04x - Outbound]\n", LSA_TRUST_FLAG_OUTBOUND);
                if (pDomainInfo->dwTrustFlags & LSA_TRUST_FLAG_TREEROOT)
                    printf("\t\t                  [0x%.04x - Tree root]\n", LSA_TRUST_FLAG_TREEROOT);
                if (pDomainInfo->dwTrustFlags & LSA_TRUST_FLAG_PRIMARY)
                    printf("\t\t                  [0x%.04x - Primary]\n", LSA_TRUST_FLAG_PRIMARY);
                if (pDomainInfo->dwTrustFlags & LSA_TRUST_FLAG_NATIVE)
                    printf("\t\t                  [0x%.04x - Native]\n", LSA_TRUST_FLAG_NATIVE);
                if (pDomainInfo->dwTrustFlags & LSA_TRUST_FLAG_INBOUND)
                    printf("\t\t                  [0x%.04x - Inbound]\n", LSA_TRUST_FLAG_INBOUND);

                printf("\t\tTrust type:       %s\n", GetTrustTypeString(pDomainInfo->dwTrustType));
                printf("\t\tTrust Attributes: [0x%.04x]\n", pDomainInfo->dwTrustAttributes);
                if (pDomainInfo->dwTrustAttributes & LSA_TRUST_ATTRIBUTE_NON_TRANSITIVE)
                    printf("\t\t                  [0x%.04x - Non-transitive]\n", LSA_TRUST_ATTRIBUTE_NON_TRANSITIVE);
                if (pDomainInfo->dwTrustAttributes & LSA_TRUST_ATTRIBUTE_UPLEVEL_ONLY)
                    printf("\t\t                  [0x%.04x - Uplevel only]\n", LSA_TRUST_ATTRIBUTE_UPLEVEL_ONLY);
                if (pDomainInfo->dwTrustAttributes & LSA_TRUST_ATTRIBUTE_FILTER_SIDS)
                    printf("\t\t                  [0x%.04x - Filter SIDs]\n", LSA_TRUST_ATTRIBUTE_FILTER_SIDS);
                if (pDomainInfo->dwTrustAttributes & LSA_TRUST_ATTRIBUTE_FOREST_TRANSITIVE)
                    printf("\t\t                  [0x%.04x - Forest transitive]\n", LSA_TRUST_ATTRIBUTE_FOREST_TRANSITIVE);
                if (pDomainInfo->dwTrustAttributes & LSA_TRUST_ATTRIBUTE_CROSS_ORGANIZATION)
                    printf("\t\t                  [0x%.04x - Cross organization]\n", LSA_TRUST_ATTRIBUTE_CROSS_ORGANIZATION);
                if (pDomainInfo->dwTrustAttributes & LSA_TRUST_ATTRIBUTE_WITHIN_FOREST)
                    printf("\t\t                  [0x%.04x - Within forest]\n", LSA_TRUST_ATTRIBUTE_WITHIN_FOREST);

                printf("\t\tTrust Direction:  ");
                switch (pDomainInfo->dwTrustDirection)
                {
                    case LSA_TRUST_DIRECTION_ZERO_WAY:
                        printf("Zeroway Trust\n");
                        break;
                    case LSA_TRUST_DIRECTION_ONE_WAY:
                        printf("Oneway Trust\n");
                        break;
                    case LSA_TRUST_DIRECTION_TWO_WAY:
                        printf("Twoway Trust\n");
                        break;
                    
                    case LSA_TRUST_DIRECTION_SELF:
                        printf("Primary Domain\n");
                        break;
                    default:
                        printf("Unknown trust direction\n");                
                }
                printf("\t\tTrust Mode:       ");
                switch (pDomainInfo->dwTrustMode)
                {
                    case LSA_TRUST_MODE_EXTERNAL:
                        printf("External Trust (ET)\n");
                        break;
                    case LSA_TRUST_MODE_MY_FOREST:
                        printf("In my forest Trust (MFT)\n");
                        break;
                    case LSA_TRUST_MODE_OTHER_FOREST:
                        printf("In other forest Trust (OFT)\n");
                        break;
                    default:
                        printf("Unknown trust mode\n");                
                }
                
                printf("\t\tDomain flags:     [0x%.04x]\n", pDomainInfo->dwDomainFlags);
                if (pDomainInfo->dwDomainFlags & LSA_DM_DOMAIN_FLAG_PRIMARY)
                    printf("\t\t                  [0x%.04x - Primary]\n", LSA_DM_DOMAIN_FLAG_PRIMARY);
                if (pDomainInfo->dwDomainFlags & LSA_DM_DOMAIN_FLAG_OFFLINE)
                    printf("\t\t                  [0x%.04x - Offline]\n", LSA_DM_DOMAIN_FLAG_OFFLINE);
                if (pDomainInfo->dwDomainFlags & LSA_DM_DOMAIN_FLAG_FORCE_OFFLINE)
                    printf("\t\t                  [0x%.04x - Force offline]\n", LSA_DM_DOMAIN_FLAG_FORCE_OFFLINE);
                if (pDomainInfo->dwDomainFlags & LSA_DM_DOMAIN_FLAG_TRANSITIVE_1WAY_CHILD)
                    printf("\t\t                  [0x%.04x - Transitive 1 way child]\n", LSA_DM_DOMAIN_FLAG_TRANSITIVE_1WAY_CHILD);
                if (pDomainInfo->dwDomainFlags & LSA_DM_DOMAIN_FLAG_FOREST_ROOT)
                    printf("\t\t                  [0x%.04x - Forest root]\n", LSA_DM_DOMAIN_FLAG_FOREST_ROOT);
                if (pDomainInfo->dwDomainFlags & LSA_DM_DOMAIN_FLAG_GC_OFFLINE)
                    printf("\t\t                  [0x%.04x - GC offline]\n", LSA_DM_DOMAIN_FLAG_GC_OFFLINE);
                
                if (pDomainInfo->pDCInfo)
                {
                    printf("\n\t\t[Domain Controller (DC) Information]\n\n");
                    printf("\t\t\tDC Name:              %s\n", LW_IS_NULL_OR_EMPTY_STR(pDomainInfo->pDCInfo->pszName) ? "" : pDomainInfo->pDCInfo->pszName);
                    printf("\t\t\tDC Address:           %s\n", LW_IS_NULL_OR_EMPTY_STR(pDomainInfo->pDCInfo->pszAddress) ? "" : pDomainInfo->pDCInfo->pszAddress);
                    printf("\t\t\tDC Site:              %s\n", LW_IS_NULL_OR_EMPTY_STR(pDomainInfo->pDCInfo->pszSiteName) ? "" : pDomainInfo->pDCInfo->pszSiteName);
                    printf("\t\t\tDC Flags:             [0x%.08x]\n", pDomainInfo->pDCInfo->dwFlags);
                    printf("\t\t\tDC Is PDC:            %s\n",
                                    (pDomainInfo->pDCInfo->dwFlags & LSA_DS_PDC_FLAG) ? "yes" : "no");
                    printf("\t\t\tDC is time server:    %s\n",
                                    (pDomainInfo->pDCInfo->dwFlags & LSA_DS_TIMESERV_FLAG) ? "yes" : "no");
                    printf("\t\t\tDC has writeable DS:  %s\n",
                                    (pDomainInfo->pDCInfo->dwFlags & LSA_DS_WRITABLE_FLAG) ? "yes" : "no");
                    printf("\t\t\tDC is Global Catalog: %s\n",
                                    (pDomainInfo->pDCInfo->dwFlags & LSA_DS_GC_FLAG) ? "yes" : "no");
                    printf("\t\t\tDC is running KDC:    %s\n",
                                    (pDomainInfo->pDCInfo->dwFlags & LSA_DS_KDC_FLAG) ? "yes" : "no");         
                }
                
                if (pDomainInfo->pGCInfo)
                {
                    printf("\n\t\t[Global Catalog (GC) Information]\n\n");
                    printf("\t\t\tGC Name:              %s\n", LW_IS_NULL_OR_EMPTY_STR(pDomainInfo->pGCInfo->pszName) ? "" : pDomainInfo->pGCInfo->pszName);
                    printf("\t\t\tGC Address:           %s\n", LW_IS_NULL_OR_EMPTY_STR(pDomainInfo->pGCInfo->pszAddress) ? "" : pDomainInfo->pGCInfo->pszAddress);
                    printf("\t\t\tGC Site:              %s\n", LW_IS_NULL_OR_EMPTY_STR(pDomainInfo->pGCInfo->pszSiteName) ? "" : pDomainInfo->pGCInfo->pszSiteName);
                    printf("\t\t\tGC Flags:             [0x%.08x]\n", pDomainInfo->pGCInfo->dwFlags);
                    printf("\t\t\tGC Is PDC:            %s\n",
                                    (pDomainInfo->pGCInfo->dwFlags & LSA_DS_PDC_FLAG) ? "yes" : "no");
                    printf("\t\t\tGC is time server:    %s\n",
                                    (pDomainInfo->pGCInfo->dwFlags & LSA_DS_TIMESERV_FLAG) ? "yes" : "no");
                    printf("\t\t\tGC has writeable DS:  %s\n",
                                    (pDomainInfo->pGCInfo->dwFlags & LSA_DS_WRITABLE_FLAG) ? "yes" : "no");
                    printf("\t\t\tGC is running KDC:    %s\n",
                                    (pDomainInfo->pGCInfo->dwFlags & LSA_DS_KDC_FLAG) ? "yes" : "no");  
                }
            }
        }
    }
}

PCSTR
GetStatusString(
    LsaAuthProviderStatus status
    )
{
    PCSTR pszStatusString = NULL;
    
    switch (status)
    {
        case LSA_AUTH_PROVIDER_STATUS_ONLINE:
            
            pszStatusString = LSA_STATUS_STRING_ONLINE;
            
            break;
            
        case LSA_AUTH_PROVIDER_STATUS_OFFLINE:
            
            pszStatusString = LSA_STATUS_STRING_OFFLINE;
            
            break;
            
        default:
            
            pszStatusString = LSA_STATUS_STRING_UNKNOWN;
            
            break;
    }
    
    return pszStatusString;
}

PCSTR
GetModeString(
    LsaAuthProviderMode mode
    )
{
    PCSTR pszModeString = NULL;
    
    switch (mode)
    {    
        case LSA_PROVIDER_MODE_UNPROVISIONED:
            
            pszModeString = LSA_MODE_STRING_UNPROVISIONED;
            
            break;
            
        case LSA_PROVIDER_MODE_DEFAULT_CELL:
            
            pszModeString = LSA_MODE_STRING_DEFAULT_CELL;
            
            break;
            
        case LSA_PROVIDER_MODE_NON_DEFAULT_CELL:
            
            pszModeString = LSA_MODE_STRING_NON_DEFAULT_CELL;
            
            break;
            
        case LSA_PROVIDER_MODE_LOCAL_SYSTEM:
            
            pszModeString = LSA_MODE_STRING_LOCAL;
            
            break;
            
        default:
            
            pszModeString = LSA_MODE_STRING_UNKNOWN;
            
            break;
    }
    
    return pszModeString;
}

PCSTR
GetSubmodeString(
    LsaAuthProviderSubMode subMode
    )
{
    PCSTR pszSubmodeString = NULL;
    
    switch(subMode)
    {
        case LSA_AUTH_PROVIDER_SUBMODE_SCHEMA:
            
            pszSubmodeString = (pb_mode & PB_MODE_PBIS) ? PBIS_SUBMODE_STRING_SCHEMA : LSA_SUBMODE_STRING_SCHEMA;
            
            break;
            
        case LSA_AUTH_PROVIDER_SUBMODE_NONSCHEMA:
            
            pszSubmodeString = (pb_mode & PB_MODE_PBIS) ? PBIS_SUBMODE_STRING_NON_SCHEMA : LSA_SUBMODE_STRING_NON_SCHEMA;
            
            break;
            
        default:
            
            pszSubmodeString = LSA_SUBMODE_STRING_UNKNOWN;
            
            break;
    }

    return pszSubmodeString;
}

PCSTR
GetTrustTypeString(
    DWORD dwTrustType
    )
{
    PCSTR pszTrustTypeString = NULL;
    
    switch (dwTrustType)
    {
        case LSA_TRUST_TYPE_DOWNLEVEL:
            pszTrustTypeString = LSA_TRUST_TYPE_STRING_DOWNLEVEL;
            break;
             
        case LSA_TRUST_TYPE_UPLEVEL:
            pszTrustTypeString = LSA_TRUST_TYPE_STRING_UPLEVEL;
            break;
            
        case LSA_TRUST_TYPE_MIT:
            pszTrustTypeString = LSA_TRUST_TYPE_STRING_MIT;
            break;
            
        case LSA_TRUST_TYPE_DCE:
            pszTrustTypeString = LSA_TRUST_TYPE_STRING_DCE;
            break;
            
        default:
            pszTrustTypeString = LSA_TRUST_TYPE_STRING_UNKNOWN;
            break;
    }
    
    return pszTrustTypeString;
}

DWORD
MapErrorCode(
    DWORD dwError
    )
{
    DWORD dwError2 = dwError;
    
    switch (dwError)
    {
        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:
            
            dwError2 = LW_ERROR_LSA_SERVER_UNREACHABLE;
            
            break;
            
        default:
            
            break;
    }
    
    return dwError2;
}
