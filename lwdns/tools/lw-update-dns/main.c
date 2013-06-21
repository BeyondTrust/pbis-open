/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#include "includes.h"
#include "../../lwdns/includes.h"
#include <lsa/ad.h>

typedef struct _ARGS {
    BOOLEAN bShowArguments;
    BOOLEAN bUseMachineCredentials;
    PSTR pszHostname;
    PSTR pszHostDnsSuffix;
    PSOCKADDR_IN pAddressArray;
    PSOCKADDR_IN6 pAddress6Array;
    DWORD dwIPV4Count;
    DWORD dwIPV6Count;
    LWDNSLogLevel LogLevel;
    PFN_LWDNS_LOG_MESSAGE pfnLogger;
} ARGS, *PARGS;

#define HAVE_MORE_ARGS(Argc, LastArgIndex, ArgsNeeded) \
    (((Argc) - (LastArgIndex)) > (ArgsNeeded))

static
DWORD
ParseArgs(
    IN int argc,
    IN const char* argv[],
    OUT PARGS pArgs
    );

static
DWORD
GetHostname(
    PSTR* ppszHostname
    );

static
DWORD
GetDnsSuffixByHostname(
    IN PCSTR pszHostname,
    OUT PSTR *ppszHostDnsSuffix
    );

static
DWORD
GetAllInterfaceAddresses(
    OUT PSOCKADDR_IN* ppAddressArray,
    OUT PSOCKADDR_IN6* ppAddress6Array,
    OUT PDWORD pdwIPV4Count,
    OUT PDWORD pdwIPV6Count
    );

static
DWORD
SetupCredentials(
    IN PCSTR pszHostname,
    OUT PSTR* ppszHostDnsSuffix
    );

static
VOID
CleanupCredentials(
    VOID
    );

static
VOID
LogMessage(
    LWDNSLogLevel logLevel,
    PCSTR         pszFormat,
    va_list       msgList
    );

static
VOID
ShowUsage(
    PCSTR pszProgramName
    );

static
VOID
PrintError(
    IN DWORD dwError
    );

int
main(
    int argc,
    const char* argv[]
    )
{
    DWORD dwError = 0;
    ARGS args = { 0 };
    PSTR pszHostname = NULL;
    PSTR pszHostDnsSuffixFromCreds = NULL;
    PSTR pszHostDnsSuffixFromHostname = NULL;
    PCSTR pszUseDnsSuffix = NULL;
    PCSTR pszUseHostname = NULL;
    PSTR pszHostFQDN = NULL;
    PSTR pszZone = NULL;
    PLW_NS_INFO pNameServerInfos = NULL;
    DWORD dwNameServerInfoCount = 0;
    HANDLE hDNSServer = NULL;
    DWORD iNS = 0;
    BOOLEAN bDNSUpdated = FALSE;
    BOOLEAN bReachedNameServer = FALSE;
    DWORD iAddr = 0;
    CHAR szIPv6[INET6_ADDRSTRLEN] ={0};

    dwError = ParseArgs(
                    argc,
                    argv,
                    &args);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = LWNetExtendEnvironmentForKrb5Affinity(TRUE);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSInitialize();
    BAIL_ON_LWDNS_ERROR(dwError);

    if (args.pfnLogger)
    {
        DNSSetLogParameters(args.LogLevel, args.pfnLogger);
    }

    dwError = GetHostname(&pszHostname);
    BAIL_ON_LWDNS_ERROR(dwError);

    if (args.bUseMachineCredentials)
    {
        if (geteuid() != 0)
        {
            fprintf(stderr, "Please retry with super-user privileges\n");
            exit(EACCES);
        }

        dwError = SetupCredentials(pszHostname, &pszHostDnsSuffixFromCreds);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    DNSStrToUpper(args.pszHostDnsSuffix);
    pszUseDnsSuffix = args.pszHostDnsSuffix;
    if (!pszUseDnsSuffix)
    {
        DNSStrToUpper(pszHostDnsSuffixFromCreds);
        pszUseDnsSuffix = pszHostDnsSuffixFromCreds;
    }
    if (!pszUseDnsSuffix)
    {
        dwError = GetDnsSuffixByHostname(
                        pszHostname,
                        &pszHostDnsSuffixFromHostname);
        if (dwError)
        {
            fprintf(stderr, "Failed to get DNS suffix for host '%s'\n", pszHostname);
        }
        BAIL_ON_LWDNS_ERROR(dwError);

        DNSStrToUpper(pszHostDnsSuffixFromHostname);
        pszUseDnsSuffix = pszHostDnsSuffixFromHostname;
    }

    pszUseHostname = args.pszHostname;
    if (!pszUseHostname)
    {
        pszUseHostname = pszHostname;
    }

    dwError = DNSAllocateMemory(
                    strlen(pszUseHostname) + strlen(pszUseDnsSuffix) + 2,
                    (PVOID*)&pszHostFQDN);
    BAIL_ON_LWDNS_ERROR(dwError);

    sprintf(pszHostFQDN, "%s.%s", pszUseHostname, pszUseDnsSuffix);
    DNSStrToLower(pszHostFQDN);

    if (args.bShowArguments)
    {
        printf("Using FQDN %s with the following addresses:\n", pszHostFQDN);
        for (iAddr = 0; iAddr < args.dwIPV4Count; iAddr++)
        {
            PSOCKADDR_IN pSockAddr = &args.pAddressArray[iAddr];
            printf("  %s\n", inet_ntoa(pSockAddr->sin_addr));
        }
        for (iAddr = 0; iAddr < args.dwIPV6Count; iAddr++)
        {
            PSOCKADDR_IN6 pSock6Addr = &args.pAddress6Array[iAddr];
            printf("  %s\n", inet_ntop(AF_INET6,&(pSock6Addr->sin6_addr),szIPv6,sizeof(szIPv6)));
        }
    }

    dwError = DNSGetNameServers(
                    pszHostFQDN,
                    &pszZone,
                    &pNameServerInfos,
                    &dwNameServerInfoCount);
    BAIL_ON_LWDNS_ERROR(dwError);

    if (!dwNameServerInfoCount)
    {
        dwError = DNS_ERROR_ZONE_HAS_NO_NS_RECORDS;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    for (iNS = 0; !bDNSUpdated && (iNS < dwNameServerInfoCount); iNS++)
    {
        PSTR   pszNameServer = NULL;
        PLW_NS_INFO pNSInfo = NULL;

        pNSInfo = &pNameServerInfos[iNS];
        pszNameServer = pNSInfo->pszNSHostName;

        if (hDNSServer)
        {
            DNSClose(hDNSServer);
        }

        LWDNS_LOG_INFO("Attempting to update name server [%s]", pszNameServer);

        dwError = DNSOpen(
                        pszNameServer,
                        DNS_TCP,
                        &hDNSServer);
        if (dwError)
        {
            LWDNS_LOG_ERROR(
                    "Failed to open connection to Name Server [%s]. [Error code:%d]",
                    pszNameServer,
                    dwError);
            dwError = 0;

            continue;
        }

        bReachedNameServer = TRUE;

        dwError = DNSUpdateSecure(
                        hDNSServer,
                        pszNameServer,
                        pszZone,
                        pszHostFQDN,
                        args.dwIPV4Count,
                        args.dwIPV6Count,
                        args.pAddressArray,
                        args.pAddress6Array);
        if (dwError)
        {
            if (dwError == DNS_ERROR_RCODE_REFUSED)
            {
                // Bail on permission denied
                BAIL_ON_LWDNS_ERROR(dwError);
            }
            LWDNS_LOG_ERROR(
                    "Failed to update Name Server [%s]. [Error code:%d]",
                    pszNameServer,
                    dwError);
            dwError = 0;
            
            continue;
        }

        bDNSUpdated = TRUE;
    }

    if (!bReachedNameServer)
    {
        dwError = ERROR_CONNECTION_REFUSED;
        BAIL_ON_LWDNS_ERROR(dwError);
    }
    if (!bDNSUpdated)
    {
        dwError = LW_ERROR_DNS_UPDATE_FAILED;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    printf("A record successfully updated in DNS\n");

    bDNSUpdated = FALSE;

    for (iAddr = 0; iAddr < args.dwIPV4Count; iAddr++)
    {
        PSOCKADDR_IN pSockAddr = &args.pAddressArray[iAddr];

        dwError = DNSUpdatePtrSecure(
                        pSockAddr,
                        pszHostFQDN);
        if (dwError)
        {
            printf("Unable to register reverse PTR record address %s with hostname %s\n",
                    inet_ntoa(pSockAddr->sin_addr), pszHostFQDN);
            dwError = 0;
        }
        else
        {
            bDNSUpdated = TRUE;
        }
    }

    if (bDNSUpdated)
    {
        printf("IPV4 PTR records successfully updated in DNS\n");
        bDNSUpdated = FALSE;
    }

    for (iAddr = 0; iAddr < args.dwIPV6Count; iAddr++)
    {
        PSOCKADDR_IN6 pSock6Addr = &args.pAddress6Array[iAddr];
        CHAR szIPv6[INET6_ADDRSTRLEN];

        if(inet_ntop(AF_INET6,&(pSock6Addr->sin6_addr),szIPv6,sizeof(szIPv6)) != NULL)
        {
            if(!IS_ADDR_LINKLOCAL(szIPv6))
            {
                dwError = DNSUpdatePtrV6Secure(
                            pSock6Addr,
                            pszHostFQDN);

                if (dwError)
                {  
                    printf("Unable to register reverse PTR record address %s with hostname %s\n",
                               szIPv6, pszHostFQDN);
                    dwError = 0;
                }
                else
                {
                    bDNSUpdated = TRUE;
                }
            }
        }
    }
    
    if (bDNSUpdated)
    {
        printf("IPV6 PTR records successfully updated in DNS\n");
        bDNSUpdated = FALSE;
    }

cleanup:
    if (hDNSServer)
    {
        DNSClose(hDNSServer);
    }

    if (pNameServerInfos)
    {
        DNSFreeNameServerInfoArray(
                pNameServerInfos,
                dwNameServerInfoCount);
    }

    LWDNS_SAFE_FREE_STRING(pszZone);
    LWDNS_SAFE_FREE_STRING(pszHostFQDN);
    LWDNS_SAFE_FREE_STRING(pszHostDnsSuffixFromCreds);
    LWDNS_SAFE_FREE_STRING(pszHostDnsSuffixFromHostname);
    LWDNS_SAFE_FREE_STRING(pszHostname);

    LWDNS_SAFE_FREE_STRING(args.pszHostname);
    LWDNS_SAFE_FREE_STRING(args.pszHostDnsSuffix);
    LWDNS_SAFE_FREE_MEMORY(args.pAddressArray);
    LWDNS_SAFE_FREE_MEMORY(args.pAddress6Array);

    DNSShutdown();

    CleanupCredentials();

    return dwError;

error:
    PrintError(dwError);

    goto cleanup;
}

static
DWORD
ParseArgs(
    IN int argc,
    IN const char* argv[],
    OUT PARGS pArgs
    )
{
    DWORD dwError = 0;
    DWORD iArg = 0;
    PCSTR pszHostFQDN = NULL;
    PSTR pszHostname = NULL;
    PSTR pszHostDnsSuffix = NULL;
    BOOLEAN bShowArguments = FALSE;
    BOOLEAN bUseMachineCredentials = TRUE;
    PSOCKADDR_IN pAddressArray = NULL;
    PSOCKADDR_IN6 pAddress6Array = NULL;
    DWORD dwIPV4Count = 0;
    DWORD dwIPV6Count = 0;
    LWDNSLogLevel LogLevel = LWDNS_LOG_LEVEL_ERROR;
    PFN_LWDNS_LOG_MESSAGE pfnLogger = NULL;
    PCSTR pszProgramName = "update-dns";

    for (iArg = 1; iArg < argc; iArg++)
    {
        PCSTR pszArg = argv[iArg];

        if (!strcasecmp(pszArg, "-h") ||
            !strcasecmp(pszArg, "--help"))
        {
            ShowUsage(pszProgramName);
            exit(0);
        }
        else if (!strcasecmp(pszArg, "--loglevel"))
        {
            if (!HAVE_MORE_ARGS(argc, iArg, 1))
            {
                fprintf(stderr, "Missing argument for %s option.\n", pszArg);
                ShowUsage(pszProgramName);
                exit(1);
            }

            pszArg = argv[iArg + 1];
            iArg++;

            if (!strcasecmp(pszArg, "error"))
            {
                LogLevel = LWDNS_LOG_LEVEL_ERROR;
                pfnLogger = LogMessage;
            }
            else if (!strcasecmp(pszArg, "warning"))
            {
                LogLevel = LWDNS_LOG_LEVEL_WARNING;
                pfnLogger = LogMessage;
            }
            else if (!strcasecmp(pszArg, "info"))
            {
                LogLevel = LWDNS_LOG_LEVEL_INFO;
                pfnLogger = LogMessage;
            }
            else if (!strcasecmp(pszArg, "verbose"))
            {
                LogLevel = LWDNS_LOG_LEVEL_VERBOSE;
                pfnLogger = LogMessage;
            }
            else if (!strcasecmp(pszArg, "debug"))
            {
                LogLevel = LWDNS_LOG_LEVEL_DEBUG;
                pfnLogger = LogMessage;
            }
            else
            {
                fprintf(stderr, "Invalid log level: %s\n", pszArg);
                ShowUsage(pszProgramName);
                exit(1);
            }
        }
        else if (!strcasecmp(pszArg, "--ipaddress"))
        {
            PSOCKADDR_IN pSockAddr = NULL;

            if (!HAVE_MORE_ARGS(argc, iArg, 1))
            {
                fprintf(stderr, "Missing argument for %s option.\n", pszArg);
                ShowUsage(pszProgramName);
                exit(1);
            }

            pszArg = argv[iArg + 1];
            iArg++;

            dwError = DNSReallocMemory(
                            pAddressArray,
                            OUT_PPVOID(&pAddressArray),
                            sizeof(pAddressArray[0]) * (dwIPV4Count + 1));
            BAIL_ON_LWDNS_ERROR(dwError);

            pSockAddr = &pAddressArray[dwIPV4Count];
            pSockAddr->sin_family = AF_INET;
            if (!inet_aton(pszArg, &pSockAddr->sin_addr))
            {
                fprintf(stderr, "Invalid IP address: %s\n", pszArg);
                exit(1);
            }

            dwIPV4Count++;
        }
        else if (!strcasecmp(pszArg, "--ipv6address"))
        {
            PSOCKADDR_IN6 pSock6Addr = NULL;

            if (!HAVE_MORE_ARGS(argc, iArg, 1))
            {
                fprintf(stderr, "Missing argument for %s option.\n", pszArg);
                ShowUsage(pszProgramName);
                exit(1);
            }

            pszArg = argv[iArg + 1];
            iArg++;

            dwError = DNSReallocMemory(
                            pAddress6Array,
                            OUT_PPVOID(&pAddress6Array),
                            sizeof(pAddress6Array[0]) * (dwIPV6Count + 1));
            BAIL_ON_LWDNS_ERROR(dwError);

            pSock6Addr = &pAddress6Array[dwIPV6Count];

            pSock6Addr->sin6_family = AF_INET6;

            inet_pton(AF_INET6, pszArg, &(pSock6Addr->sin6_addr));
           
            //validate the IPV6 address
            dwError = DNSInet6ValidateAddress(pszArg);
            BAIL_ON_LWDNS_ERROR(dwError);

            dwIPV6Count++;
        }
        else if (!strcasecmp(pszArg, "--fqdn"))
        {
            PCSTR pszDot = NULL;

            if (!HAVE_MORE_ARGS(argc, iArg, 1))
            {
                fprintf(stderr, "Missing argument for %s option.\n", pszArg);
                ShowUsage(pszProgramName);
                exit(1);
            }

            if (pszHostFQDN)
            {
                fprintf(stderr, "Can only specify one %s option.\n", pszArg);
                ShowUsage(pszProgramName);
                exit(1);
            }

            pszArg = argv[iArg + 1];
            iArg++;

            pszHostFQDN = pszArg;

            pszDot = strchr(pszHostFQDN, '.');
            if (!pszDot || !pszDot[1])
            {
                fprintf(stderr, "Invalid FQDN: %s\n", pszArg);
                exit(1);
            }

            dwError = DNSAllocateString(
                            &pszDot[1],
                            &pszHostDnsSuffix);
            BAIL_ON_LWDNS_ERROR(dwError);

            dwError = DNSAllocateString(
                            pszHostFQDN,
                            &pszHostname);
            BAIL_ON_LWDNS_ERROR(dwError);

            pszHostname[pszDot - pszHostFQDN] = 0;
        }
        else if (!strcasecmp(pszArg, "--nocreds"))
        {
            bUseMachineCredentials = FALSE;
        }
        else if (!strcasecmp(pszArg, "--show"))
        {
            bShowArguments = TRUE;
        }
        else
        {
            fprintf(stderr, "Unexpected argument: %s\n", pszArg);
            ShowUsage(pszProgramName);
            exit(1);
        }
    }

    if (!dwIPV4Count && !dwIPV6Count)
    {
        dwError = GetAllInterfaceAddresses(&pAddressArray, &pAddress6Array, &dwIPV4Count, &dwIPV6Count);
        if (dwError)
        {
            fprintf(stderr, "Failed to get interface addresses.\n");
            BAIL_ON_LWDNS_ERROR(dwError);
        }
    }
    
cleanup:

    memset(pArgs, 0, sizeof(*pArgs));

    pArgs->bShowArguments = bShowArguments;
    pArgs->bUseMachineCredentials = bUseMachineCredentials;
    pArgs->pszHostname = pszHostname;
    pArgs->pszHostDnsSuffix = pszHostDnsSuffix;
    pArgs->pAddressArray = pAddressArray;
    pArgs->pAddress6Array = pAddress6Array;
    pArgs->dwIPV4Count = dwIPV4Count;
    pArgs->dwIPV6Count = dwIPV6Count;
    pArgs->LogLevel = LogLevel;
    pArgs->pfnLogger = pfnLogger;
    
    return dwError;
    
error:

    bShowArguments = FALSE;
    bUseMachineCredentials = TRUE;
    LWDNS_SAFE_FREE_STRING(pszHostname);
    LWDNS_SAFE_FREE_STRING(pszHostDnsSuffix);
    LWDNS_SAFE_FREE_MEMORY(pAddressArray);
    LWDNS_SAFE_FREE_MEMORY(pAddress6Array);
    dwIPV4Count = 0;
    dwIPV6Count = 0;
    LogLevel = LWDNS_LOG_LEVEL_ERROR;
    pfnLogger = NULL;

    goto cleanup;
}

static
DWORD
GetHostname(
    PSTR* ppszHostname
    )
{
    DWORD dwError = 0;
    CHAR szBuffer[256];
    PSTR pszLocal = NULL;
    PSTR pszDot = NULL;
    int len = 0;
    PSTR pszHostname = NULL;

    if ( gethostname(szBuffer, sizeof(szBuffer)) != 0 )
    {
        dwError = errno;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    len = strlen(szBuffer);
    if ( len > strlen(".local") )
    {
        pszLocal = &szBuffer[len - strlen(".local")];
        if ( !strcasecmp( pszLocal, ".local" ) )
        {
            pszLocal[0] = '\0';
        }
    }

    /* Test to see if the name is still dotted.
     * If so we will chop it down to just the
     * hostname field.
     */
    pszDot = strchr(szBuffer, '.');
    if ( pszDot )
    {
        pszDot[0] = '\0';
    }

    dwError = DNSAllocateString(
                    szBuffer,
                    &pszHostname);
    BAIL_ON_LWDNS_ERROR(dwError);

    *ppszHostname = pszHostname;

cleanup:

    return dwError;

error:

    LWDNS_SAFE_FREE_STRING(pszHostname);

    *ppszHostname = NULL;

    goto cleanup;
}

static
DWORD
GetDnsSuffixByHostname(
    IN PCSTR pszHostname,
    OUT PSTR *ppszHostDnsSuffix
    )
{
    DWORD dwError = 0;
    PSTR pszHostDnsSuffix = NULL;
    struct hostent* pHost = NULL;
    PCSTR pszDot = NULL;
    PCSTR pszFoundDomain = NULL;

    pHost = gethostbyname(pszHostname);
    if (!pHost)
    {
        dwError = DNSMapHerrno(h_errno);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    //
    // We look for the first name that looks like an FQDN.  This is
    // the same heuristics used by other software such as Kerberos and
    // Samba.
    //
    pszDot = strchr(pHost->h_name, '.');
    if (pszDot)
    {
        pszFoundDomain = pszDot + 1;
    }
    else
    {
        int i;
        for (i = 0; pHost->h_aliases[i]; i++)
        {
            pszDot = strchr(pHost->h_aliases[i], '.');
            if (pszDot)
            {
                pszFoundDomain = pszDot + 1;
                break;
            }
        }
    }

    if (!pszFoundDomain)
    {
        dwError = DNS_ERROR_RECORD_DOES_NOT_EXIST;
        BAIL_ON_LWDNS_ERROR(dwError)
    }

    dwError = DNSAllocateString(pszFoundDomain, &pszHostDnsSuffix);
    BAIL_ON_LWDNS_ERROR(dwError);
    
cleanup:
    *ppszHostDnsSuffix = pszHostDnsSuffix;

    return dwError;

error:
    LWDNS_SAFE_FREE_STRING(pszHostDnsSuffix);

    goto cleanup;
}

static
DWORD
GetAllInterfaceAddresses(
    OUT PSOCKADDR_IN* ppAddressArray,
    OUT PSOCKADDR_IN6* ppAddress6Array,
    OUT PDWORD pdwIPV4Count,
    OUT PDWORD pdwIPV6Count
    )
{
    DWORD dwError = 0;
    PSOCKADDR_IN pAddressArray = NULL;
    PSOCKADDR_IN6 pAddress6Array = NULL;
    DWORD dwipv4Count = 0;
    DWORD dwipv6Count = 0;
    DWORD dwAddressCount = 0;
    PLW_INTERFACE_INFO pInterfaceArray = NULL;
    DWORD dwInterfaceCount = 0;
    DWORD iAddr = 0;

    dwError = DNSGetNetworkInterfaces(
                    &pInterfaceArray,
                    &dwInterfaceCount);
    BAIL_ON_LWDNS_ERROR(dwError);

    if (!dwInterfaceCount)
    {
        dwError = ERROR_NOINTERFACE;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    dwError = DNSAllocateMemory(
                    sizeof(SOCKADDR_IN) * dwInterfaceCount,
                    (PVOID*)&pAddressArray);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                    sizeof(SOCKADDR_IN6) * dwInterfaceCount,
                    (PVOID*)&pAddress6Array);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwAddressCount = dwInterfaceCount;

    for (iAddr = 0; iAddr < dwAddressCount; iAddr++)
    {
        PLW_INTERFACE_INFO pInterfaceInfo = &pInterfaceArray[iAddr];
        if(pInterfaceInfo->bIPV6Enabled)
        {
            PSOCKADDR_IN6 pSock6Addr = &pAddress6Array[dwipv6Count];
            pSock6Addr->sin6_family = pInterfaceInfo->ipv6Addr.sin6_family;
            pSock6Addr->sin6_addr = ((PSOCKADDR_IN6)&pInterfaceInfo->ipv6Addr)->sin6_addr;
            dwipv6Count++;
        }
        else
        {
            PSOCKADDR_IN pSockAddr = &pAddressArray[dwipv4Count];
            pSockAddr->sin_family = pInterfaceInfo->ipAddr.sa_family;
            pSockAddr->sin_addr = ((PSOCKADDR_IN)&pInterfaceInfo->ipAddr)->sin_addr;
            dwipv4Count++;
        }
    }

cleanup:
    if (pInterfaceArray)
    {
        DNSFreeNetworkInterfaces(
                pInterfaceArray,
                dwInterfaceCount);
    }

    *ppAddressArray = pAddressArray;
    *ppAddress6Array = pAddress6Array;
    *pdwIPV4Count = dwipv4Count;
    *pdwIPV6Count = dwipv6Count;

    return dwError;

error:
    LWDNS_SAFE_FREE_MEMORY(pAddressArray);
    LWDNS_SAFE_FREE_MEMORY(pAddress6Array);
    dwAddressCount = 0;

    goto cleanup;
}

static
DWORD
SetupCredentials(
    IN PCSTR pszHostname,
    OUT PSTR* ppszHostDnsSuffix
    )
{
    DWORD dwError = 0;
    PSTR pszHostDnsSuffixResult = NULL;
    PCSTR pszHostDnsSuffix = NULL;
    HANDLE hLsaConnection = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = LsaAdGetMachinePasswordInfo(
                    hLsaConnection,
                    NULL,
                    &pPasswordInfo);
    BAIL_ON_LWDNS_ERROR(dwError);

    for (pszHostDnsSuffix = pPasswordInfo->Account.Fqdn;
         *pszHostDnsSuffix;
         pszHostDnsSuffix++)
    {
        if (*pszHostDnsSuffix == '.')
        {
            pszHostDnsSuffix++;
            break;
        }
    }

    if (!*pszHostDnsSuffix)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    dwError = DNSKrb5Init(
                    pPasswordInfo->Account.SamAccountName,
                    pPasswordInfo->Account.DnsDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateString(
                    pszHostDnsSuffix,
                    &pszHostDnsSuffixResult);
    BAIL_ON_LWDNS_ERROR(dwError);

cleanup:
    if (pPasswordInfo)
    {
        LsaAdFreeMachinePasswordInfo(pPasswordInfo);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    *ppszHostDnsSuffix = pszHostDnsSuffixResult;

    return dwError;

error:
    LWDNS_SAFE_FREE_STRING(pszHostDnsSuffixResult);

    goto cleanup;
}

static
VOID
CleanupCredentials(
    VOID
    )
{
    DNSKrb5Shutdown();
}

static
VOID
LogMessage(
    LWDNSLogLevel logLevel,
    PCSTR         pszFormat,
    va_list       msgList
    )
{
    PCSTR pszEntryType = NULL;
    time_t currentTime;
    struct tm tmp = {0};
    char timeBuf[128];

    switch (logLevel)
    {
        case LWDNS_LOG_LEVEL_ALWAYS:
        {
            pszEntryType = LWDNS_INFO_TAG;
            break;
        }
        case LWDNS_LOG_LEVEL_ERROR:
        {
            pszEntryType = LWDNS_ERROR_TAG;
            break;
        }

        case LWDNS_LOG_LEVEL_WARNING:
        {
            pszEntryType = LWDNS_WARN_TAG;
            break;
        }

        case LWDNS_LOG_LEVEL_INFO:
        {
            pszEntryType = LWDNS_INFO_TAG;
            break;
        }

        case LWDNS_LOG_LEVEL_VERBOSE:
        {
            pszEntryType = LWDNS_VERBOSE_TAG;
            break;
        }

        case LWDNS_LOG_LEVEL_DEBUG:
        {
            pszEntryType = LWDNS_DEBUG_TAG;
            break;
        }

        default:
        {
            pszEntryType = LWDNS_VERBOSE_TAG;
            break;
        }
    }

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), LWDNS_LOG_TIME_FORMAT, &tmp);

    fprintf(stdout, "%s:%s:", timeBuf, pszEntryType);
    vfprintf(stdout, pszFormat, msgList);
    fprintf(stdout, "\n");
    fflush(stdout);
}

static
VOID
ShowUsage(
    PCSTR pszProgramName
    )
{
    fprintf(stdout,
            "\n"
            "Usage: %s [options]\n"
            "\n"
            "Registers IP addresses and corresponding PTR records in DNS via a secure\n"
            "dynamic DNS update. By default, will register all interface addresses\n"
            "using the default FQDN as determined by the machine password store or\n"
            "the canonical hostname returned by gethostbyname(gethostname())\n"
            "if --nocreds is used.\n"
            "\n"
            "\tWhere options can be:\n"
            "\n"
            "\t--loglevel LEVEL\n"
            "\t\t\tSets log level, where LEVEL can be one of\n"
            "\t\t\terror, warning, info, verbose, debug.\n"
            "\n"
            "\t--ipaddress IP\n"
            "\t\t\tSets IP address to register for this computer.\n"
            "\t\t\tThis can be specified multiple times.\n"
            "\n"
            "\t--ipv6address IPV6_Address\n"
            "\t\t\tSets IPV6 address to register for this computer.\n"
            "\t\t\tThis can be specified multiple times.\n"
            "\n"
            "\t--fqdn FQDN\n"
            "\t\t\tFQDN to register.\n"
            "\n"
            "\t--nocreds\n"
            "\t\t\tDo not use domain credentials.\n"
            "\n"
            "\t--show\n"
            "\t\t\tShow IP addresses and FQDN used.\n"
            "\n"
            "\tExamples:\n"
            "\n"
            "\t%s --ipaddress 192.168.186.129\n"
            "\t%s --ipaddress 192.168.186.129 --fqdn joe.example.com\n\n"
            "\t%s --ipv6address fd50:973:3783:49d:250:56ff:febd:46c5 --fqdn joe.example.com\n\n"
            "", pszProgramName, pszProgramName, pszProgramName, pszProgramName);
}

static
VOID
PrintError(
    IN DWORD dwError
    )
{
    PCSTR pErrorString = NULL;
    
    switch (dwError)
    {
        case ERROR_CONNECTION_REFUSED:
            pErrorString = "No name servers could be reached";
            break;
        default:
            pErrorString = LwWin32ExtErrorToDescription(dwError);
            break;
    }

    if (pErrorString && pErrorString[0])
    {
        fprintf(stderr, "Failed to update DNS.  %s\n", pErrorString);
    }
    else
    {
        fprintf(stderr, "Failed to update DNS. Error code [%d]\n", dwError);
    }
}

