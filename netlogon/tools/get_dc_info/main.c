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
 *        Likewise Site Manager
 *
 *        Client Test Program - LWNetGetDcName
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#include "config.h"
#include "lwnet-system.h"
#include "lwnet-def.h"
#include "lwnet.h"
#include "lwnet-utils.h"
#include "lwerror.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
void
ShowUsage()
{
    printf("Usage: get-dc-name <target domain FQDN> [--site <site name>]\n");
    printf("        [--force] [--ds-required] [--gc-required]\n");
    printf("        [--pdc-required] [--background-only] [--kdc-required]\n");
    printf("        [--timeserv-required] [--writeable-required] [--good-timeserv-required]\n");
    printf("        [--avoid-self] [--preferred-domain <domain name>]\n\n"); 
}

DWORD
AddFlag(
    DWORD dwFlag,
    PDWORD pdwFlags
    )
{
    DWORD dwError = 0;
    DWORD dwFlags = *pdwFlags;
    
    if(dwFlags & dwFlag)
    {
        printf("Duplicate flag entered: [0x%.8X]\n", dwFlag);
        dwError = ERROR_INVALID_PARAMETER;
    }
    else if(!(dwFlag & LWNET_SUPPORTED_DS_INPUT_FLAGS))
    {
        printf("Invalid flag entered: [0x%.8X]\n", dwFlag);
        dwError = ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwFlags |= dwFlag;
    
    *pdwFlags = dwFlags;
    
    
error:
    return dwError;
    
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszTargetFQDN,
    PSTR*  ppszSiteName,
    PSTR*  ppszPrimaryDomain,
    PDWORD pdwFlags
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_SITENAME,
            PARSE_MODE_PREFERRED_DOMAIN,
            PARSE_MODE_OPTIONS
        } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    PSTR pszTargetFQDN = NULL;
    PSTR pszSiteName = NULL;
    PSTR pszPrimaryDomain = NULL;
    DWORD dwFlags = 0;

    do {
        pszArg = argv[iArg++];
        if (IsNullOrEmptyString(pszArg))
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
                    if (strncmp(pszArg, "--", 2) == 0)
                    {
                        ShowUsage();
                        exit(0);
                    }

                    dwError = LWNetAllocateString(pszArg, &pszTargetFQDN);
                    BAIL_ON_LWNET_ERROR(dwError);
        
                    parseMode = PARSE_MODE_OPTIONS;
                }
                break;

            case PARSE_MODE_OPTIONS:
                if(strcmp(pszArg, "--site") == 0)
                {
                    parseMode = PARSE_MODE_SITENAME;
                }
                else if(strcmp(pszArg, "--preferred-domain") == 0)
                {
                    parseMode = PARSE_MODE_PREFERRED_DOMAIN;
                }
                else if(strcmp(pszArg, "--force") == 0)
                {
                    dwError = AddFlag(DS_FORCE_REDISCOVERY, &dwFlags);
                    BAIL_ON_LWNET_ERROR(dwError);
                }
                else if(strcmp(pszArg, "--ds-required") == 0)
                {
                    dwError = AddFlag(DS_DIRECTORY_SERVICE_REQUIRED, &dwFlags);
                    BAIL_ON_LWNET_ERROR(dwError);
                }
                else if(strcmp(pszArg, "--gc-required") == 0)
                {
                    dwError = AddFlag(DS_GC_SERVER_REQUIRED, &dwFlags);
                    BAIL_ON_LWNET_ERROR(dwError);
                }
                else if(strcmp(pszArg, "--pdc-required") == 0)
                {
                    dwError = AddFlag(DS_PDC_REQUIRED, &dwFlags);
                    BAIL_ON_LWNET_ERROR(dwError);
                }
                else if(strcmp(pszArg, "--background-only") == 0)
                {
                    dwError = AddFlag(DS_BACKGROUND_ONLY, &dwFlags);
                    BAIL_ON_LWNET_ERROR(dwError);
                }
                else if(strcmp(pszArg, "--kdc-required") == 0)
                {
                    dwError = AddFlag(DS_KDC_REQUIRED, &dwFlags);
                    BAIL_ON_LWNET_ERROR(dwError);
                }
                else if(strcmp(pszArg, "--timeserv-required") == 0)
                {
                    dwError = AddFlag(DS_TIMESERV_REQUIRED, &dwFlags);
                    BAIL_ON_LWNET_ERROR(dwError);
                }
                else if(strcmp(pszArg, "--writeable-required") == 0)
                {
                    dwError = AddFlag(DS_WRITABLE_REQUIRED, &dwFlags);
                    BAIL_ON_LWNET_ERROR(dwError);
                }
                else if(strcmp(pszArg, "--good-timeserv-required") == 0)
                {
                    dwError = AddFlag(DS_GOOD_TIMESERV_REQUIRED, &dwFlags);
                    BAIL_ON_LWNET_ERROR(dwError);
                }
                else if(strcmp(pszArg, "--avoid-self") == 0)
                {
                    dwError = AddFlag(DS_AVOID_SELF, &dwFlags);
                    BAIL_ON_LWNET_ERROR(dwError);
                }
                else 
                {
                    LWNET_LOG_ERROR("Invalid argument: %s", pszArg);
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_LWNET_ERROR(dwError);
                }
                break;
            case PARSE_MODE_SITENAME:
                
                if(!IsNullOrEmptyString(pszSiteName))
                {
                    LWNET_LOG_ERROR("Invalid argument: %s", pszArg);
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_LWNET_ERROR(dwError);
                }
                
                dwError = LWNetAllocateString(pszArg, &pszSiteName);
                BAIL_ON_LWNET_ERROR(dwError);
                
                parseMode = PARSE_MODE_OPTIONS;
                break;
            case PARSE_MODE_PREFERRED_DOMAIN:
                
                if(!IsNullOrEmptyString(pszPrimaryDomain))
                {
                    LWNET_LOG_ERROR("Invalid argument: %s", pszArg);
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_LWNET_ERROR(dwError);
                }
                
                dwError = LWNetAllocateString(pszArg, &pszPrimaryDomain);
                BAIL_ON_LWNET_ERROR(dwError);
                
                parseMode = PARSE_MODE_OPTIONS;
                break;
        }
        
    } while (iArg < argc);

    
    if(IsNullOrEmptyString(pszTargetFQDN))
    {
        ShowUsage();
        exit(0);
    }
    
error:
    if (dwError)
    {
        LWNET_SAFE_FREE_STRING(pszTargetFQDN);
        LWNET_SAFE_FREE_STRING(pszSiteName);
        LWNET_SAFE_FREE_STRING(pszPrimaryDomain);
        dwFlags = 0;
    }

    *ppszTargetFQDN = pszTargetFQDN;
    *ppszSiteName = pszSiteName;
    *ppszPrimaryDomain = pszPrimaryDomain;
    *pdwFlags = dwFlags;

    return dwError;
}

void
safePrintString(
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

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;

    PSTR pszTargetFQDN = NULL;
    PSTR pszSiteName = NULL;
    PSTR pszPrimaryDomain = NULL;
    PLWNET_DC_INFO pDCInfo = NULL;
    DWORD dwFlags = 0;
    CHAR szErrorBuf[1024];
    
    INT i = 0;
    
    dwError = ParseArgs(
                argc,
                argv,
                &pszTargetFQDN,
                &pszSiteName,
                &pszPrimaryDomain,
                &dwFlags
                );
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetGetDCNameExt(
                NULL,
                pszTargetFQDN,
                pszSiteName,
                pszPrimaryDomain,
                dwFlags,
                0,
                NULL,
                &pDCInfo
                );
    BAIL_ON_LWNET_ERROR(dwError); 

    printf("Printing LWNET_DC_INFO fields:\n");
    printf("===============================\n");
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

error:
    if (dwError)
    {
        DWORD dwLen = LwGetErrorString(dwError, szErrorBuf, 1024);

        if (dwLen)
        {
            fprintf(
                 stderr,
                 "Failed communication with the LWNET Agent.  Error code %u (%s).\n%s\n",
                 dwError,
                 LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)),
                 szErrorBuf);
        }
        else
        {
            fprintf(
                 stderr,
                 "Failed communication with the LWNET Agent.  Error code %u (%s).\n",
                 dwError,
                 LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
        }
    }

    LWNET_SAFE_FREE_DC_INFO(pDCInfo);
    LWNET_SAFE_FREE_STRING(pszTargetFQDN);
    LWNET_SAFE_FREE_STRING(pszSiteName);
    return dwError;
}
