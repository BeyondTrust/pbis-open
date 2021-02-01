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
 *        main.c
 *
 * Abstract:
 *
 *        BeyondTrust Site Manager
 *
 *        Client Test Program -  
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#include "config.h"
#include "lwnet-system.h"
#include "lwnet-def.h"
#include "lwnet.h"
#include "lwnet-server.h"
#include "lwnet-cachedb.h"
#include "lwerror.h"

#define TABLOC_PING     0
#define TABLOC_DC_FQDN  11
#define TABLOC_DC_IP    51
#define TABLOC_DC_SITE  70
#define TABLOC_GC       85
#define TABLOC_KDC      91
#define TABLOC_PDC      97
#define TABLOC_BORDER   " | " 

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

typedef enum {
    DISPLAY_MODE_CSV = 0,
    DISPLAY_MODE_TABLE
} DisplayMode;

static
void
ShowUsage(
    void
    )
{
    printf("Usage: lw-export-dc-cache [-t] <targetFQDN>\n");
    printf("    -t option will print values in a simplified table\n");
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszTargetFQDN,
    DisplayMode* pMode
    )
{

    DisplayMode mode = DISPLAY_MODE_CSV;
    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszTargetFQDN = NULL;

    
    do {
        pszArg = argv[iArg++];
        if (IsNullOrEmptyString(pszArg))
        {
            break;
        }
        if ((strcmp(pszArg, "--help") == 0) ||
            (strcmp(pszArg, "-h") == 0))
        {
            ShowUsage();
            exit(0);
        }
        else if(strcmp(pszArg, "-t") == 0)
        {
            mode = DISPLAY_MODE_TABLE;
        }
        else
        {
            dwError = LWNetAllocateString(pszArg, &pszTargetFQDN);
            BAIL_ON_LWNET_ERROR(dwError);
        }
    } while (iArg < argc);

    
    if(IsNullOrEmptyString(pszTargetFQDN))
    {
        ShowUsage();
        exit(0);
    }
    
cleanup:
    *ppszTargetFQDN = pszTargetFQDN;
    *pMode = mode;
    return dwError;

error:
    LWNET_SAFE_FREE_STRING(pszTargetFQDN);
    *ppszTargetFQDN = NULL;
    *pMode = DISPLAY_MODE_CSV;
    goto cleanup;
}

VOID
PrintCacheTableRow(
    PLWNET_DC_INFO pInfo
    )
{
    DWORD dwPing = 0;
    char buf[256];
    INT iCategory = 0;
    
    if(pInfo == NULL)
    {
        printf("<NULL>\n");
        return;
    }

    //fields: Ping(ms), DC-FQDN, DC-Site, GC?, KDC?, PDC?                                                                                                                               

    memset(buf, ' ', 255);

    dwPing = pInfo->dwPingTime;
    
    sprintf(buf,                  "%5.3f", 
            ((double)dwPing / (double)LWNET_MICROSECONDS_IN_MILLISECOND));

    sprintf(buf+TABLOC_DC_FQDN,     "%s%s", TABLOC_BORDER,
        IsNullOrEmptyString(pInfo->pszDomainControllerName) ? 
                "<null>" : (PSTR)pInfo->pszDomainControllerName);

    sprintf(buf+TABLOC_DC_IP,     "%s%s", TABLOC_BORDER,
        IsNullOrEmptyString(pInfo->pszDomainControllerAddress) ? 
                "<null>" : (PSTR)pInfo->pszDomainControllerAddress);

    sprintf(buf+TABLOC_DC_SITE,     "%s%s", TABLOC_BORDER,
        IsNullOrEmptyString(pInfo->pszDCSiteName) ? 
                "<null>" : (PSTR)pInfo->pszDCSiteName);
    
    sprintf(buf+TABLOC_GC,     "%s%s", TABLOC_BORDER,
        (pInfo->dwFlags & DS_GC_FLAG) > 0 ? 
                " x " : "   ");
    
    sprintf(buf+TABLOC_KDC,     "%s%s", TABLOC_BORDER,
        (pInfo->dwFlags & DS_KDC_FLAG) ? 
                " x " : "   ");
    
    sprintf(buf+TABLOC_PDC,     "%s%s", TABLOC_BORDER,
        (pInfo->dwFlags & DS_PDC_FLAG) ? 
                " x " : "   ");
    
    for (iCategory = 0; iCategory <= TABLOC_PDC; iCategory++) {
        if (buf[iCategory] == (char)0)
        {
            buf[iCategory] = ' ';
        }
    }

    printf("%s\n", buf);

}

VOID
PrintCacheTableHeader(
    VOID
    )
{
    char buf[256];
    INT iCategory = 0;
    
    //fields: Ping(ms), DC-FQDN, DC-Site, GC?, KDC?, PDC?                                                                                                                               

    memset(buf, ' ', 255);
    
    sprintf(buf,                "Ping(ms)"); 

    sprintf(buf+TABLOC_DC_FQDN, "%s%s", TABLOC_BORDER, "DC FQDN");

    sprintf(buf+TABLOC_DC_IP,   "%s%s", TABLOC_BORDER, "DC IPAddr");

    sprintf(buf+TABLOC_DC_SITE, "%s%s", TABLOC_BORDER, "DC Site");
    
    sprintf(buf+TABLOC_GC,      "%s%s", TABLOC_BORDER, "GC");
    
    sprintf(buf+TABLOC_KDC,     "%s%s", TABLOC_BORDER, "KDC");
    
    sprintf(buf+TABLOC_PDC,     "%s%s", TABLOC_BORDER, "PDC");
    
    for (iCategory = 0; iCategory <= TABLOC_PDC; iCategory++) {
        if (buf[iCategory] == (char)0)
        {
            buf[iCategory] = ' ';
        }
    }

    printf("%s\n", buf);
    
}

void safePrintString(
        PSTR pszStringName, 
        PSTR pszStringValue
        )
{
    if(IsNullOrEmptyString(pszStringName))
    {
        return;
    }
    else if(pszStringValue == NULL)
    {
        printf("%s = <NULL>\n", pszStringName);
    }
    else if(*pszStringValue == '\0')
    {
        printf("%s = <EMPTY>\n", pszStringName);
    }
    else
    {
        printf("%s = %s\n", pszStringName, pszStringValue);
    }
}

static
PCSTR
GetStaticTimeString(
    IN LWNET_UNIX_TIME_T Time
    )
{
    PCSTR staticTimeString = NULL;
    time_t timeValue = (time_t) Time;

    staticTimeString = ctime(&timeValue);
    return staticTimeString ? staticTimeString : "????\n";
}

static
VOID
PrintEntryInfo(
    IN PLWNET_CACHE_DB_ENTRY pEntry
    )
{
    printf("DNS Domain = '%s'\n"
           "Site Name = '%s'\n"
           "Query Type = %u\n",
           pEntry->pszDnsDomainName,
           pEntry->pszSiteName,
           pEntry->QueryType);
    // Separate lines due to static buffers
    printf("LastDiscovered = %s", GetStaticTimeString(pEntry->LastDiscovered));
    printf("LastPinged = %s", GetStaticTimeString(pEntry->LastPinged));
    printf("IsBackoffToWritableDc = %s\n", pEntry->IsBackoffToWritableDc ? "true" : "false");
    printf("LastBackoffToWritableDc = %s", GetStaticTimeString(pEntry->LastBackoffToWritableDc));
    printf("--- DC INFO ---\n");
}

VOID
PrintDCInfo(
    PLWNET_DC_INFO pDCInfo
    )
{
    INT i = 0;
    
    if(pDCInfo == NULL)
    {
        printf("<NULL>");
    }
    else
    {
        printf("dwDomainControllerAddressType = %u\n", pDCInfo->dwDomainControllerAddressType); 
        printf("dwFlags = %u\n", pDCInfo->dwFlags); 
        printf("dwVersion = %u\n", pDCInfo->dwVersion);       
        printf("wLMToken = %u\n", pDCInfo->wLMToken);  
        printf("wNTToken = %u\n", pDCInfo->wNTToken);
        
        safePrintString("pszDomainControllerName", pDCInfo->pszDomainControllerName);
        safePrintString("pszDomainControllerAddress", pDCInfo->pszDomainControllerAddress);
        
        printf("pucDomainGUID(hex) = ");
        for(i = 0; i < LWNET_GUID_SIZE; i++)
        {
            printf("%.2X ", pDCInfo->pucDomainGUID[i]);
        }
        printf("\n");
        
        safePrintString("pszNetBIOSDomainName", pDCInfo->pszNetBIOSDomainName);    
        safePrintString("pszFullyQualifiedDomainName", pDCInfo->pszFullyQualifiedDomainName);     
        safePrintString("pszDnsForestName", pDCInfo->pszDnsForestName);       
        safePrintString("pszDCSiteName", pDCInfo->pszDCSiteName);      
        safePrintString("pszClientSiteName", pDCInfo->pszClientSiteName); 
        safePrintString("pszNetBIOSHostName", pDCInfo->pszNetBIOSHostName);   
        safePrintString("pszUserName", pDCInfo->pszUserName);     
        
    }
    printf("\n\n");
}


int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSTR pszTargetFQDN = NULL;
    DisplayMode mode = DISPLAY_MODE_CSV;
    LWNET_CACHE_DB_HANDLE dbHandle = NULL;
    PLWNET_CACHE_DB_ENTRY pEntries = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;
    CHAR szErrorBuf[1024];

    ParseArgs(argc, argv, &pszTargetFQDN, &mode);
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LWNetCacheDbOpen(NETLOGON_DB, FALSE, &dbHandle);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetCacheDbExport(dbHandle, &pEntries, &dwCount);
    BAIL_ON_LWNET_ERROR(dwError);

    // ISSUE-2008/07/01-dalmeida -- Need to finish plumbing PLWNET_CACHE_DB_ENTRY
    // through this tool.
    if (mode == DISPLAY_MODE_TABLE)
    {
        PrintCacheTableHeader();
        for (i = 0; i < dwCount; i++)
        {
            PrintCacheTableRow(&pEntries[i].DcInfo);
        }
    }
    else
    {
        for (i = 0; i < dwCount; i++)
        {
            printf("Cache entry #%d:\n", i);
            printf("===================\n");
            PrintEntryInfo(&pEntries[i]);
            PrintDCInfo(&pEntries[i].DcInfo);
        }     
    }

error:
    LWNetCacheDbClose(&dbHandle);
    if (dwError)
    {
        DWORD dwLen = LwGetErrorString(dwError, szErrorBuf, 1024);

        if (dwLen)
        {
            fprintf(stderr,
                    "Failed to read entries from cache.  Error code %u (%s).\n%s\n",
                    dwError,
                    LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)),
                    szErrorBuf);
        }
        else
        {
            fprintf(stderr,
                    "Failed to read entries from cache.  Error code %u (%s).\n",
                    dwError,
                    LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
        }
    }
    return dwError;
}



