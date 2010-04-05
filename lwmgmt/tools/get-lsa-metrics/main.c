#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

#include "config.h"
#include "lwmgmtsys.h"
#include "lwmgmtdefs.h"
#include "lwmgmt.h"
#include "lwmgmtcommon.h"
#include "lwmgmtclient.h"

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszHostname,
    PDWORD pdwInfoLevel
    );

VOID
ShowUsage();

DWORD
QueryLsaMetrics_0(
    PCSTR pszHostname
    );

DWORD
QueryLsaMetrics_1(
    PCSTR pszHostname
    );

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSTR  pszHostname = NULL;
    DWORD dwInfoLevel = 0;
    
    dwError = ParseArgs(
                    argc,
                    argv,
                    &pszHostname,
                    &dwInfoLevel);
    BAIL_ON_LWMGMT_ERROR(dwError);
    
    switch (dwInfoLevel)
    {
        case 0:
            
            dwError = QueryLsaMetrics_0(
                            pszHostname);
            BAIL_ON_LWMGMT_ERROR(dwError);
            
            break;
            
        case 1:
            
            dwError = QueryLsaMetrics_1(
                            pszHostname);
            BAIL_ON_LWMGMT_ERROR(dwError);
            
            break;
            
        default:
            
            fprintf(stderr, "An invalid info level [%d] was specified.\n", dwInfoLevel);
            ShowUsage();
            dwError = EINVAL;
            goto error;
    }
    
cleanup:

    LWMGMT_SAFE_FREE_STRING(pszHostname);

    return dwError;

error:

    fprintf(stderr, "Failed to query LSA metrics. [Error code: %d]\n", dwError);

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszHostname,
    PDWORD pdwInfoLevel
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_LEVEL
        } ParseMode;
        
    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszHostname = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    DWORD dwInfoLevel = 0;

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
                else if (!strcmp(pszArg, "--level")) {
                    parseMode = PARSE_MODE_LEVEL;
                }
                else
                {
                    LWMGMT_SAFE_FREE_STRING(pszHostname);
                    
                    dwError = LWMGMTAllocateString(pszArg, &pszHostname);
                    BAIL_ON_LWMGMT_ERROR(dwError);
                }
                break;
                
            case PARSE_MODE_LEVEL:
                
                dwInfoLevel = atoi(pszArg);
                parseMode = PARSE_MODE_OPEN;
                
                break;
        }
        
    } while (iArg < argc);

    *ppszHostname = pszHostname;
    *pdwInfoLevel = dwInfoLevel;

cleanup:
    
    return dwError;

error:

    *ppszHostname = NULL;
    *pdwInfoLevel = 0;

    LWMGMT_SAFE_FREE_STRING(pszHostname);

    goto cleanup;
}

void
ShowUsage()
{
    printf("Usage: lwmgmt-get-lsa-metrics {--level [0, 1]} {-h | --help} {hostname}\n");
}

DWORD
QueryLsaMetrics_0(
    PCSTR pszHostname
    )
{
    DWORD dwError = 0;
    PLSA_METRIC_PACK_0 pPack = NULL;

    dwError = LWMGMTQueryLsaMetrics_0(
                  pszHostname,
                  &pPack);
    BAIL_ON_LWMGMT_ERROR(dwError);

    printf("Failed authentications:       %llu\n", (unsigned long long)pPack->failedAuthentications);
    printf("Failed user lookups by name:  %llu\n", (unsigned long long)pPack->failedUserLookupsByName);
    printf("Failed user lookups by id:    %llu\n", (unsigned long long)pPack->failedUserLookupsById);
    printf("Failed group lookups by name: %llu\n", (unsigned long long)pPack->failedGroupLookupsByName);
    printf("Failed group lookups by id:   %llu\n", (unsigned long long)pPack->failedGroupLookupsById);
    printf("Failed session opens:         %llu\n", (unsigned long long)pPack->failedOpenSession);
    printf("Failed session closures:      %llu\n", (unsigned long long)pPack->failedCloseSession);
    printf("Failed password changes:      %llu\n", (unsigned long long)pPack->failedChangePassword);
    printf("Unauthorized access attempts: %llu\n", (unsigned long long)pPack->unauthorizedAccesses);

cleanup:

    if (pPack)
    {
        LWMGMTFreeLsaMetrics_0(pPack);
    }

    return dwError;
    
error:

    goto cleanup;
}

DWORD
QueryLsaMetrics_1(
    PCSTR pszHostname
    )
{
    DWORD dwError = 0;
    PLSA_METRIC_PACK_1 pPack = NULL;

    dwError = LWMGMTQueryLsaMetrics_1(
                  pszHostname,
                  &pPack);
    BAIL_ON_LWMGMT_ERROR(dwError);

    printf("Successful authentications:       %llu\n", (unsigned long long)pPack->successfulAuthentications);
    printf("Failed authentications:           %llu\n", (unsigned long long)pPack->failedAuthentications);
    printf("Successful user lookups by name:  %llu\n", (unsigned long long)pPack->successfulUserLookupsByName);
    printf("Failed user lookups by name:      %llu\n", (unsigned long long)pPack->failedUserLookupsByName);
    printf("Successful user lookups by id:    %llu\n", (unsigned long long)pPack->successfulUserLookupsById);
    printf("Failed user lookups by id:        %llu\n", (unsigned long long)pPack->failedUserLookupsById);
    printf("Successful group lookups by name: %llu\n", (unsigned long long)pPack->successfulGroupLookupsByName);
    printf("Failed group lookups by name:     %llu\n", (unsigned long long)pPack->failedGroupLookupsByName);
    printf("Successful group lookups by id:   %llu\n", (unsigned long long)pPack->successfulGroupLookupsById);
    printf("Failed group lookups by id:       %llu\n", (unsigned long long)pPack->failedGroupLookupsById);
    printf("Successful session opens:         %llu\n", (unsigned long long)pPack->successfulOpenSession);
    printf("Failed session opens:             %llu\n", (unsigned long long)pPack->failedOpenSession);
    printf("Successful session closures:      %llu\n", (unsigned long long)pPack->successfulCloseSession);
    printf("Failed session closures:          %llu\n", (unsigned long long)pPack->failedCloseSession);
    printf("Successful password changes:      %llu\n", (unsigned long long)pPack->successfulChangePassword);
    printf("Failed password changes:          %llu\n", (unsigned long long)pPack->failedChangePassword);
    printf("Unauthorized access attempts:     %llu\n", (unsigned long long)pPack->unauthorizedAccesses);

cleanup:

    if (pPack)
    {
        LWMGMTFreeLsaMetrics_1(pPack);
    }

    return dwError;
    
error:

    goto cleanup;    
}
