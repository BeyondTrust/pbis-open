/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2010
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
 *        Resolve Hostname
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 *
 */
#include "config.h"
#include "lwnet-system.h"
#include "lwnet-def.h"
#include "lwnet.h"
#include "lwnet-utils.h"
#include "lw/rtlstring.h"
#include "client/ipc_client_p.h"

static
void
ShowUsage()
{
    printf("Usage: resolvenameclient hostname\n");
}

static
int
ParseArgs(
    int argc,
    char* argv[])
{
    int iArg = 1;
    PSTR pszArg = NULL;
    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }
        
        if ((strcmp(pszArg, "--help") == 0) || (strcmp(pszArg, "-h") == 0))
        {
            ShowUsage();
            exit(0);
        }
    } while (iArg < argc);
    
    return 0;
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSTR pszHostName = NULL;
    PWSTR pwszHostName = NULL;
    PWSTR pwszCanonName = NULL;
    PSTR pszCanonName = NULL;
    PLWNET_RESOLVE_ADDR *ppAddressList = NULL;
    DWORD dwAddressListLen = 0;
    DWORD i = 0;
    CHAR ipAddressBuf[INET_ADDRSTRLEN];
    PCSTR pszAddress = NULL;
    DWORD ipAddressLen = 0;
    DWORD ipAddrFamily = 0;
    PBYTE pIpAddr = NULL;

    ParseArgs(argc, argv);

    if (argc == 1)
    {
        printf("usage: %s hostname\n", argv[0]);
        return 0;
    }

    pszHostName = argv[1];
    dwError = LwRtlWC16StringAllocateFromCString(&pwszHostName,
                                                 pszHostName);
    BAIL_ON_LWNET_ERROR(dwError);
    dwError = LWNetResolveName(
                  (PCWSTR) pwszHostName,
                  &pwszCanonName,
                  &ppAddressList,
                  &dwAddressListLen);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwRtlCStringAllocateFromWC16String(&pszCanonName,
                                                 pwszCanonName);
    BAIL_ON_LWNET_ERROR(dwError);
 
    for (i=0; i<dwAddressListLen; i++)
    {
        if (ppAddressList[i]->AddressType == LWNET_IP_ADDR_V4)
        {
            ipAddressLen = 4; 
            ipAddrFamily = PF_INET;
            pIpAddr = ppAddressList[i]->Address.Ip4Addr;
        }
        else if (ppAddressList[i]->AddressType == LWNET_IP_ADDR_V6)
        {
            ipAddressLen = 16; 
            ipAddrFamily = PF_INET6;
            pIpAddr = ppAddressList[i]->Address.Ip4Addr;
        }
        pszAddress = inet_ntop(ipAddrFamily,
                               pIpAddr,
                               ipAddressBuf,
                               sizeof(ipAddressBuf));
        if (pszAddress)
        {
            printf("IP Address = %s\n", pszAddress);
        }
    }
    printf("Responses = %d Host: '%s'\n", dwAddressListLen, pszCanonName);

cleanup:
    LWNetResolveNameFree(pwszCanonName, ppAddressList, dwAddressListLen);
    LWNET_SAFE_FREE_MEMORY(pwszHostName);
    LWNET_SAFE_FREE_STRING(pszCanonName);
    return (dwError);
error:
 
    if (dwError == ERROR_BAD_NET_NAME)
    {
        printf("LWNetResolveName() failed DNS/NetBIOS name resolution\n");
    }
    else
    {
        LWNET_LOG_ERROR("Failed communication with likewise-netlogond. "
                        "Error code [%d]\n", dwError);
    } 
    goto cleanup;

}
