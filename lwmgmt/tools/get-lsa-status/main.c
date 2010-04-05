#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

#include "config.h"
#include "lwmgmtsys.h"
#include "lwmgmtdefs.h"
#include "lwmgmt.h"
#include "lwmgmtcommon.h"
#include "lwmgmtclient.h"

#define LSA_MODE_STRING_UNKNOWN          "Unknown"
#define LSA_MODE_STRING_UNPROVISIONED    "Un-provisioned"
#define LSA_MODE_STRING_DEFAULT_CELL     "Default Cell"
#define LSA_MODE_STRING_NON_DEFAULT_CELL "Non-default Cell"
#define LSA_MODE_STRING_LOCAL            "Local system"

#define LSA_SUBMODE_STRING_UNKNOWN       "Unknown"
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

#define LWMGMT_SECONDS_IN_MINUTE (60)
#define LWMGMT_SECONDS_IN_HOUR   (60 * LWMGMT_SECONDS_IN_MINUTE)
#define LWMGMT_SECONDS_IN_DAY    (24 * LWMGMT_SECONDS_IN_HOUR)

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszHostname
    );

VOID
ShowUsage();

VOID
PrintLsaStatus(
    PLSA_STATUS pLsaStatus
    );

PCSTR
GetStatusString(
    LsaAuthProviderStatus status
    );

PCSTR
GetModeString(
    LsaAuthProviderMode mode
    );

PCSTR
GetSubmodeString(
    LsaAuthProviderSubMode subMode
    );

PCSTR
GetTrustTypeString(
    DWORD dwTrustType
    );

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSTR  pszHostname = NULL;
    PLSA_STATUS pLsaStatus = NULL;
    
    dwError = ParseArgs(
                    argc,
                    argv,
                    &pszHostname);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LWMGMTQueryLsaStatus(
                  pszHostname,
                  &pLsaStatus);
    BAIL_ON_LWMGMT_ERROR(dwError);
    
    PrintLsaStatus(pLsaStatus);
    
cleanup:

    LWMGMT_SAFE_FREE_STRING(pszHostname);
    
    if (pLsaStatus)
    {
        LWMGMTFreeLsaStatus(pLsaStatus);
    }

    return dwError;

error:

    fprintf(stderr, "Failed to query LSA status. [Error code: %d]\n", dwError);

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszHostname
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0
        } ParseMode;
        
    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszHostname = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;

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
                else
                {
                    LWMGMT_SAFE_FREE_STRING(pszHostname);
                    
                    dwError = LWMGMTAllocateString(pszArg, &pszHostname);
                    BAIL_ON_LWMGMT_ERROR(dwError);
                }
                break;
        }
        
    } while (iArg < argc);

    *ppszHostname = pszHostname;

cleanup:
    
    return dwError;

error:

    *ppszHostname = NULL;

    LWMGMT_SAFE_FREE_STRING(pszHostname);

    goto cleanup;
}

void
ShowUsage()
{
    printf("Usage: lwmgmt-get-lsa-status {-h | --help} {hostname}\n");
}

VOID
PrintLsaStatus(
    PLSA_STATUS pLsaStatus
    )
{
    DWORD iCount = 0;
    DWORD dwDays = pLsaStatus->dwUptime/LWMGMT_SECONDS_IN_DAY;
    DWORD dwHours = (pLsaStatus->dwUptime - (dwDays * LWMGMT_SECONDS_IN_DAY))/LWMGMT_SECONDS_IN_HOUR;
    DWORD dwMins = (pLsaStatus->dwUptime - 
                    (dwDays * LWMGMT_SECONDS_IN_DAY) - 
                    (dwHours * LWMGMT_SECONDS_IN_HOUR))/LWMGMT_SECONDS_IN_MINUTE;
    DWORD dwSecs = (pLsaStatus->dwUptime -
                    (dwDays * LWMGMT_SECONDS_IN_DAY) - 
                    (dwHours * LWMGMT_SECONDS_IN_HOUR) - 
                    (dwMins * LWMGMT_SECONDS_IN_MINUTE));
    
    printf("LSA Server Status:\n\n");
    printf("Agent version: %u.%u.%u\n",
                    pLsaStatus->version.dwMajor,
                    pLsaStatus->version.dwMinor,
                    pLsaStatus->version.dwBuild);
    printf("Uptime:        %u days %u hours %u minutes %u seconds\n",
                    dwDays,
                    dwHours,
                    dwMins,
                    dwSecs);
    
    for (iCount = 0; iCount < pLsaStatus->dwCount; iCount++)
    {
        PLSA_AUTH_PROVIDER_STATUS pProviderStatus =
            &pLsaStatus->pAuthProviderStatusArray[iCount];
        
        printf("\n[Authentication provider: %s]\n\n", 
                        IsNullOrEmptyString(pProviderStatus->pszId) ? "" : pProviderStatus->pszId);
        
        printf("\tStatus:   %s\n", GetStatusString(pProviderStatus->status));
        printf("\tMode:     %s\n", GetModeString(pProviderStatus->mode));
    
        switch (pProviderStatus->mode)
        {
            case LSA_PROVIDER_MODE_LOCAL_SYSTEM:
                break;
            
            case LSA_PROVIDER_MODE_UNPROVISIONED:
            case LSA_PROVIDER_MODE_DEFAULT_CELL:
            case LSA_PROVIDER_MODE_NON_DEFAULT_CELL:
                
                printf("\tDomain:   %s\n", IsNullOrEmptyString(pProviderStatus->pszDomain) ? "" : pProviderStatus->pszDomain);
                printf("\tForest:   %s\n", IsNullOrEmptyString(pProviderStatus->pszForest) ? "" : pProviderStatus->pszForest);
                printf("\tSite:     %s\n", IsNullOrEmptyString(pProviderStatus->pszSite) ? "" : pProviderStatus->pszSite);
                printf("\tOnline check interval:  %d seconds\n", pProviderStatus->dwNetworkCheckInterval);
                
                break;
                
            default:
                
                break;
        }
        
        switch (pProviderStatus->mode)
        {
            case LSA_PROVIDER_MODE_DEFAULT_CELL:
                
                printf("\tSub mode: %s\n", GetSubmodeString(pProviderStatus->subMode));
                
                break;
                
            case LSA_PROVIDER_MODE_NON_DEFAULT_CELL:
                
                printf("\tSub mode: %s\n", GetSubmodeString(pProviderStatus->subMode));
                
                printf("\tCell:     %s\n", IsNullOrEmptyString(pProviderStatus->pszCell) ? "" : pProviderStatus->pszCell);
                
                break;
                
            default:
                
                break;
        }
        
        if (pProviderStatus->pTrustedDomainInfoArray)
        {
            DWORD iDomain = 0;
            
            printf("\n\t[Trusted Domains: %d]\n\n", pProviderStatus->dwNumTrustedDomains);
            
            for (; iDomain < pProviderStatus->dwNumTrustedDomains; iDomain++)
            {
                PLSA_TRUSTED_DOMAIN_INFO pDomainInfo =
                    &pProviderStatus->pTrustedDomainInfoArray[iDomain];
                
                printf("\n\t[Domain: %s]\n\n", IsNullOrEmptyString(pDomainInfo->pszNetbiosDomain) ? "" : pDomainInfo->pszNetbiosDomain);
                
                printf("\t\tDNS Domain:       %s\n", IsNullOrEmptyString(pDomainInfo->pszDnsDomain) ? "" : pDomainInfo->pszDnsDomain);
                printf("\t\tNetbios name:     %s\n", IsNullOrEmptyString(pDomainInfo->pszNetbiosDomain) ? "" : pDomainInfo->pszNetbiosDomain);
                printf("\t\tForest name:      %s\n", IsNullOrEmptyString(pDomainInfo->pszForestName) ? "" : pDomainInfo->pszForestName);
                printf("\t\tTrustee DNS name: %s\n", IsNullOrEmptyString(pDomainInfo->pszTrusteeDnsDomain) ? "" : pDomainInfo->pszTrusteeDnsDomain);
                printf("\t\tClient site name: %s\n", IsNullOrEmptyString(pDomainInfo->pszClientSiteName) ? "" : pDomainInfo->pszClientSiteName);
                printf("\t\tDomain SID:       %s\n", IsNullOrEmptyString(pDomainInfo->pszDomainSID) ? "" : pDomainInfo->pszDomainSID);
                printf("\t\tDomain GUID:      %s\n", IsNullOrEmptyString(pDomainInfo->pszDomainGUID) ? "" : pDomainInfo->pszDomainGUID);
                printf("\t\tTrust Flags:      0x%x\n", pDomainInfo->dwTrustFlags);
                printf("\t\tTrust type:       %s\n", GetTrustTypeString(pDomainInfo->dwTrustType));
                printf("\t\tTrust Attributes: 0x%x\n", pDomainInfo->dwTrustAttributes);
                printf("\t\tDomain flags:     0x%x\n", pDomainInfo->dwDomainFlags);
                
                if (pDomainInfo->pDCInfo)
                {
                    printf("\n\t\t[Domain Controller (DC) Information]\n\n");
                    printf("\t\t\tDC Name:              %s\n", IsNullOrEmptyString(pDomainInfo->pDCInfo->pszName) ? "" : pDomainInfo->pDCInfo->pszName);
                    printf("\t\t\tDC Address:           %s\n", IsNullOrEmptyString(pDomainInfo->pDCInfo->pszAddress) ? "" : pDomainInfo->pDCInfo->pszAddress);
                    printf("\t\t\tDC Site:              %s\n", IsNullOrEmptyString(pDomainInfo->pDCInfo->pszSiteName) ? "" : pDomainInfo->pDCInfo->pszSiteName);
                    printf("\t\t\tDC Flags:             0x%x\n", pDomainInfo->pDCInfo->dwFlags);
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
                    printf("\t\t\tGC Name:              %s\n", IsNullOrEmptyString(pDomainInfo->pGCInfo->pszName) ? "" : pDomainInfo->pGCInfo->pszName);
                    printf("\t\t\tGC Address:           %s\n", IsNullOrEmptyString(pDomainInfo->pGCInfo->pszAddress) ? "" : pDomainInfo->pGCInfo->pszAddress);
                    printf("\t\t\tGC Site:              %s\n", IsNullOrEmptyString(pDomainInfo->pGCInfo->pszSiteName) ? "" : pDomainInfo->pGCInfo->pszSiteName);
                    printf("\t\t\tGC Flags:             0x%x\n", pDomainInfo->pGCInfo->dwFlags);
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
            
            pszSubmodeString = LSA_SUBMODE_STRING_SCHEMA;
            
            break;
            
        case LSA_AUTH_PROVIDER_SUBMODE_NONSCHEMA:
            
            pszSubmodeString = LSA_SUBMODE_STRING_NON_SCHEMA;
            
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
