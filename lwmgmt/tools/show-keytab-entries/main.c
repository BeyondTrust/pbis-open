#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

#include "config.h"
#include "lwmgmtsys.h"
#include "lwmgmtdefs.h"
#include "lwmgmt.h"
#include "lwmgmtcommon.h"
#include "lwmgmtclient.h"

#define ACTION_NONE  1
#define ACTION_READ  1
#define ACTION_COUNT 2

#define ENCTYPE_DES_CBC_CRC             1
#define ENCTYPE_DES_CBC_MD5             3
#define ENCTYPE_RC4_HMAC                23
#define ENCTYPE_AES128_CTS_HMAC_SHA1_96 17
#define ENCTYPE_AES256_CTS_HMAC_SHA1_96 18

#define STR_ENCTYPE_UNKNOWN                 "unknown"
#define STR_ENCTYPE_DES_CBC_CRC             "des-cbc-crc"
#define STR_ENCTYPE_DES_CBC_MD5             "des-cbc-md5"
#define STR_ENCTYPE_RC4_HMAC                "rc4-hmac"
#define STR_ENCTYPE_AES128_CTS_HMAC_SHA1_96 "aes128-ctx-hmac-sha1-96"
#define STR_ENCTYPE_AES256_CTS_HMAC_SHA1_96 "aes256-cts-hmac-sha1-96"

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PDWORD action,
    PSTR*  ppszHostname,
    PSTR*  ppszKeyTabPath,
    PDWORD pdwBatchSize
    );

static
PSTR
GetProgramName(
    PSTR pszFullProgramPath
    );

VOID
ShowUsage(
    PCSTR pszProgramName
    );

VOID
PrintKeyTabEntries(
    PLWMGMT_LSA_KEYTAB_ENTRIES pKeyTabEntries
    );

PCSTR
GetEnctypeString(
    DWORD enctype
    );

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    DWORD dwBatchSize = 0;
    DWORD dwNumEntriesFound = 0;
    DWORD dwTotalEntriesFound = 0;
    DWORD action = ACTION_NONE;
    PSTR  pszHostname = NULL;
    PSTR  pszKeyTabPath = NULL;
    PLWMGMT_LSA_KEYTAB_ENTRIES pKeyTabEntries = NULL;
    
    dwError = ParseArgs(
                  argc,
                  argv,
                  &action,
                  &pszHostname,
                  &pszKeyTabPath,
                  &dwBatchSize);
    BAIL_ON_LWMGMT_ERROR(dwError);

    if ( action == ACTION_COUNT )
    {
        dwError = LWMGMTCountKeyTabEntries(
                      pszHostname,
                      pszKeyTabPath,
                      &dwTotalEntriesFound);
        BAIL_ON_LWMGMT_ERROR(dwError);

        printf("Number of key table entries found for %s at %s: %u\n",
            pszKeyTabPath ? pszKeyTabPath : "default key table",
            pszHostname ? pszHostname : "",
            dwTotalEntriesFound);

    }
    else
    {
   
        printf("Key Table Entries for %s at %s:\n",
            pszKeyTabPath ? pszKeyTabPath : "default key table",
            pszHostname ? pszHostname : "");
    
        do
        {
            dwError = LWMGMTReadKeyTab(
                          pszHostname,
                          pszKeyTabPath,
                          dwTotalEntriesFound,
                          dwBatchSize,
                          &pKeyTabEntries);
            if ( dwError == LWMGMT_ERROR_KRB5_KT_END ) {
                dwError = 0;
                break;
            }
            BAIL_ON_LWMGMT_ERROR(dwError);

            dwNumEntriesFound = pKeyTabEntries->dwCount;
            dwTotalEntriesFound += pKeyTabEntries->dwCount;

            PrintKeyTabEntries(pKeyTabEntries);
            LWMGMTFreeKeyTabEntries(pKeyTabEntries);
            pKeyTabEntries = NULL;
     
        } while (dwNumEntriesFound);

        fprintf(stdout,
            "\nTotal Key Table Entries found:  %u\n",
             dwTotalEntriesFound);
    }
    
cleanup:

    LWMGMT_SAFE_FREE_STRING(pszHostname);
    LWMGMT_SAFE_FREE_STRING(pszKeyTabPath);
    
    if (pKeyTabEntries)
    {
        LWMGMTFreeKeyTabEntries(pKeyTabEntries);
    }

    return dwError;

error:

    fprintf(stderr,
        "Failed to obtain key table entries. [Error code: %d]\n",
        dwError);

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PDWORD action,
    PSTR*  ppszHostname,
    PSTR*  ppszKeyTabPath,
    PDWORD pdwBatchSize
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_BATCHSIZE
        } ParseMode;
        
    DWORD dwError = 0;
    DWORD dwBatchSize = 10;
    DWORD actionLocal = ACTION_READ;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszHostname = NULL;
    PSTR pszKeyTabPath = NULL;
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
                    ShowUsage(GetProgramName(argv[0]));
                    exit(0);
                }
                else if (!strcmp(pszArg, "--batchsize")) {
                    parseMode = PARSE_MODE_BATCHSIZE;
                }
                else if (!strcmp(pszArg, "--count")) {
                    actionLocal = ACTION_COUNT;
                }
                else
                {
                    if ( IsNullOrEmptyString(pszHostname) )
                    {
                        LWMGMT_SAFE_FREE_STRING(pszHostname);
                    
                        dwError = LWMGMTAllocateString(
                                      pszArg,
                                      &pszHostname);
                        BAIL_ON_LWMGMT_ERROR(dwError);
                    }
                    else
                    {
                        LWMGMT_SAFE_FREE_STRING(pszKeyTabPath);
                    
                        dwError = LWMGMTAllocateString(
                                      pszArg,
                                      &pszKeyTabPath);
                        BAIL_ON_LWMGMT_ERROR(dwError);
                    }
                }
                break;

            case PARSE_MODE_BATCHSIZE:

                dwBatchSize = atoi(pszArg);
                if ((dwBatchSize < 1) ||
                    (dwBatchSize > 1000)) {
                    ShowUsage(GetProgramName(argv[0]));
                    exit(1);
                }
                parseMode = PARSE_MODE_OPEN;

                break;

        
        }
        
    } while (iArg < argc);

    *ppszHostname = pszHostname;
    *ppszKeyTabPath = pszKeyTabPath;
    *pdwBatchSize = dwBatchSize;
    *action = actionLocal;

cleanup:
    
    return dwError;

error:

    *ppszHostname = NULL;

    LWMGMT_SAFE_FREE_STRING(pszHostname);

    goto cleanup;
}

static
PSTR
GetProgramName(
    PSTR pszFullProgramPath
    )
{
    if (pszFullProgramPath == NULL || *pszFullProgramPath == '\0') {
        return NULL;
    }

    // start from end of the string
    PSTR pszNameStart = pszFullProgramPath + strlen(pszFullProgramPath);
    do {
        if (*(pszNameStart - 1) == '/') {
            break;
        }

        pszNameStart--;

    } while (pszNameStart != pszFullProgramPath);

    return pszNameStart;
}


void
ShowUsage(
    PCSTR pszProgramName
    )
{
    printf(
        "Usage: %s {-h | --help} {--batchsize [1...1000]} {--count} <hostname> {keytable-path}\n",
        pszProgramName);
}

VOID
PrintKeyTabEntries(
    PLWMGMT_LSA_KEYTAB_ENTRIES pKeyTabEntries
    )
{
    DWORD iCount = 0;
    
    for (iCount = 0; iCount < pKeyTabEntries->dwCount; iCount++)
    {
        PLWMGMT_LSA_KEYTAB_ENTRY pKeyTabEntry =
            &pKeyTabEntries->pLsaKeyTabEntryArray[iCount];
        
        printf("\n[Principal name: %s]\n\n", 
                        IsNullOrEmptyString(pKeyTabEntry->pszPrincipal) ? "" : pKeyTabEntry->pszPrincipal);
        
        printf("\tDate:         %s", ctime((time_t *)&pKeyTabEntry->timestamp));
        printf("\tKey version:  %u\n", pKeyTabEntry->kvno);
        printf("\tEnctype:      %s\n", GetEnctypeString(pKeyTabEntry->enctype));
    }
}

PCSTR
GetEnctypeString(
    DWORD enctype
    )
{
    PCSTR pszEnctypeString = NULL;
    
    switch (enctype)
    {
        case ENCTYPE_DES_CBC_CRC:
            
            pszEnctypeString = STR_ENCTYPE_DES_CBC_CRC;
            
            break;
            
        case ENCTYPE_DES_CBC_MD5:
            
            pszEnctypeString = STR_ENCTYPE_DES_CBC_MD5;
            
            break;
            
        case ENCTYPE_RC4_HMAC:
            
            pszEnctypeString = STR_ENCTYPE_RC4_HMAC;
            
            break;
            
        case ENCTYPE_AES128_CTS_HMAC_SHA1_96:
            
            pszEnctypeString = STR_ENCTYPE_AES128_CTS_HMAC_SHA1_96;
            
            break;
            
        case ENCTYPE_AES256_CTS_HMAC_SHA1_96:
            
            pszEnctypeString = STR_ENCTYPE_AES256_CTS_HMAC_SHA1_96;
            
            break;
            
        default:
            
            pszEnctypeString = STR_ENCTYPE_UNKNOWN;
            
            break;
    }
    
    return pszEnctypeString;
}

