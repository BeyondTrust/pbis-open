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
            ipAddrFamily = PF_INET;
            pIpAddr = ppAddressList[i]->Address.Ip4Addr;
        }
        else if (ppAddressList[i]->AddressType == LWNET_IP_ADDR_V6)
        {
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
