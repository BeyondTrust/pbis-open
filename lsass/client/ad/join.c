/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        join.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Domain membership management API
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 */
#include "adclient.h"

#define LSA_JOIN_OU_PREFIX "OU="
#define LSA_JOIN_CN_PREFIX "CN="
#define LSA_JOIN_DC_PREFIX "DC="

static
DWORD
LsaAdJoinDomainInternal(
    IN HANDLE hLsaConnection,
    IN PCSTR pHostname,
    IN PCSTR pHostDnsDomain,
    IN PCSTR pDomain,
    IN PCSTR pOu,
    IN PCSTR pUsername,
    IN PCSTR pPassword,
    IN PCSTR pOsName,
    IN PCSTR pOsVersion,
    IN PCSTR pOsServicePack,
    IN LSA_NET_JOIN_FLAGS dwFlags,
    IN LSA_USER_ACCOUNT_CONTROL_FLAGS dwUac
    );

DWORD
LsaAdOuSlashToDn(
    IN PCSTR pDomain,
    IN PCSTR pSlashOu,
    OUT PSTR* ppLdapOu
    )
{
    DWORD dwError = 0;
    PSTR  pLdapOu = NULL;
    // Do not free
    PSTR  pOutputPos = NULL;
    PCSTR pInputPos = NULL;
    PCSTR pszInputSectionEnd = NULL;
    size_t sOutputDnLen = 0;
    size_t sSectionLen = 0;
    DWORD nDomainParts = 0;
    
    BAIL_ON_INVALID_STRING(pDomain);
    BAIL_ON_INVALID_STRING(pSlashOu);
    
    // Figure out the length required to write the OU DN
    pInputPos = pSlashOu;

    // skip leading slashes
    sSectionLen = strspn(pInputPos, "/");
    pInputPos += sSectionLen;

    while ((sSectionLen = strcspn(pInputPos, "/")) != 0) {
        sOutputDnLen += sizeof(LSA_JOIN_OU_PREFIX) - 1;
        sOutputDnLen += sSectionLen;
        // For the separating comma
        sOutputDnLen++;
        
        pInputPos += sSectionLen;
        
        sSectionLen = strspn(pInputPos, "/");
        pInputPos += sSectionLen;
    }
    
    // Figure out the length required to write the Domain DN
    pInputPos = pDomain;
    while ((sSectionLen = strcspn(pInputPos, ".")) != 0) {
        sOutputDnLen += sizeof(LSA_JOIN_DC_PREFIX) - 1;
        sOutputDnLen += sSectionLen;
        nDomainParts++;
        
        pInputPos += sSectionLen;
        
        sSectionLen = strspn(pInputPos, ".");
        pInputPos += sSectionLen;
    }

    // Add in space for the separating commas
    if (nDomainParts > 1)
    {
        sOutputDnLen += nDomainParts - 1;
    }
    
    dwError = LwAllocateMemory(
                    sizeof(CHAR) * (sOutputDnLen + 1),
                    (PVOID*)&pLdapOu);
    BAIL_ON_LSA_ERROR(dwError);

    pOutputPos = pLdapOu;
    // Iterate through pSlashOu backwards and write to pLdapOu forwards
    pInputPos = pSlashOu + strlen(pSlashOu) - 1;

    while(TRUE)
    {
        // strip trailing slashes
        while (pInputPos >= pSlashOu && *pInputPos == '/')
        {
            pInputPos--;
        }

        if (pInputPos < pSlashOu)
        {
            break;
        }

        // Find the end of this section (so that we can copy it to
        // the output string in forward order).
        pszInputSectionEnd = pInputPos;
        while (pInputPos >= pSlashOu && *pInputPos != '/')
        {
            pInputPos--;
        }
        sSectionLen = pszInputSectionEnd - pInputPos;

        // Only "Computers" as the first element is a CN.
        if ((pOutputPos ==  pLdapOu) &&
            (sSectionLen == sizeof("Computers") - 1) &&
            !strncasecmp(pInputPos + 1, "Computers", sizeof("Computers") - 1))
        {
            // Add CN=<name>,
            memcpy(pOutputPos, LSA_JOIN_CN_PREFIX,
                    sizeof(LSA_JOIN_CN_PREFIX) - 1);
            pOutputPos += sizeof(LSA_JOIN_CN_PREFIX) - 1;
        }
        else
        {
            // Add OU=<name>,
            memcpy(pOutputPos, LSA_JOIN_OU_PREFIX,
                    sizeof(LSA_JOIN_OU_PREFIX) - 1);
            pOutputPos += sizeof(LSA_JOIN_OU_PREFIX) - 1;
        }

        memcpy(pOutputPos,
                pInputPos + 1,
                sSectionLen);
        pOutputPos += sSectionLen;

        *pOutputPos++ = ',';
    }

    // Make sure to overwrite any initial "CN=Computers" as "OU=Computers".
    // Note that it is safe to always set "OU=" as the start of the DN
    // unless the DN so far is exacly "CN=Computers,".
    if (strcasecmp(pLdapOu, LSA_JOIN_CN_PREFIX "Computers,"))
    {
        memcpy(pLdapOu, LSA_JOIN_OU_PREFIX, sizeof(LSA_JOIN_OU_PREFIX) - 1);
    }
    
    // Read the domain name foward in sections and write it back out
    // forward.
    pInputPos = pDomain;
    while (TRUE)
    {
        sSectionLen = strcspn(pInputPos, ".");

        memcpy(pOutputPos,
                LSA_JOIN_DC_PREFIX,
                sizeof(LSA_JOIN_DC_PREFIX) - 1);
        pOutputPos += sizeof(LSA_JOIN_DC_PREFIX) - 1;
        
        memcpy(pOutputPos, pInputPos, sSectionLen);
        pOutputPos += sSectionLen;
        
        pInputPos += sSectionLen;
        
        sSectionLen = strspn(pInputPos, ".");
        pInputPos += sSectionLen;

        if (*pInputPos != 0)
        {
            // Add a comma for the next entry
            *pOutputPos++ = ',';
        }
        else
            break;

    }
    
    assert(pOutputPos == pLdapOu + sizeof(CHAR) * (sOutputDnLen));
    *pOutputPos = 0;

    *ppLdapOu = pLdapOu;
    
cleanup:

    return dwError;
    
error:

    *ppLdapOu = NULL;
    
    LW_SAFE_FREE_STRING(pLdapOu);

    goto cleanup;
}

DWORD
LsaAdJoinDomainUac(
    HANDLE hLsaConnection,
    PCSTR pszHostname,
    PCSTR pszHostDnsDomain,
    PCSTR pszDomain,
    PCSTR pOu,
    PCSTR pszUsername,
    PCSTR pszPassword,
    PCSTR pszOSName,
    PCSTR pszOSVersion,
    PCSTR pszOSServicePack,
    LSA_NET_JOIN_FLAGS dwFlags,
    LSA_USER_ACCOUNT_CONTROL_FLAGS dwUac
    )
{
    return LsaAdJoinDomainInternal(
               hLsaConnection,
               pszHostname,
               pszHostDnsDomain,
               pszDomain,
               pOu,
               pszUsername,
               pszPassword,
               pszOSName,
               pszOSVersion,
               pszOSServicePack,
               dwFlags,
               dwUac);
}

DWORD
LsaAdJoinDomain(
    HANDLE hLsaConnection,
    PCSTR pszHostname,
    PCSTR pszHostDnsDomain,
    PCSTR pszDomain,
    PCSTR pszOU,
    PCSTR pszUsername,
    PCSTR pszPassword,
    PCSTR pszOSName,
    PCSTR pszOSVersion,
    PCSTR pszOSServicePack,
    LSA_NET_JOIN_FLAGS dwFlags
    )
{
    DWORD dwError = 0;
    PSTR pLdapOu = NULL;

    if (pszOU)
    {
        dwError = LsaAdOuSlashToDn(
                        pszDomain,
                        pszOU,
                        &pLdapOu);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAdJoinDomainDn(
                    hLsaConnection,
                    pszHostname,
                    pszHostDnsDomain,
                    pszDomain,
                    pLdapOu,
                    pszUsername,
                    pszPassword,
                    pszOSName,
                    pszOSVersion,
                    pszOSServicePack,
                    dwFlags);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pLdapOu);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LsaAdJoinDomainInternal(
    IN HANDLE hLsaConnection,
    IN PCSTR pHostname,
    IN PCSTR pHostDnsDomain,
    IN PCSTR pDomain,
    IN PCSTR pOu,
    IN PCSTR pUsername,
    IN PCSTR pPassword,
    IN PCSTR pOsName,
    IN PCSTR pOsVersion,
    IN PCSTR pOsServicePack,
    IN LSA_NET_JOIN_FLAGS dwFlags,
    IN LSA_USER_ACCOUNT_CONTROL_FLAGS dwUac
    )
{
    DWORD dwError = 0;
    LWMsgDataContext* pDataContext = NULL;
    LSA_AD_IPC_JOIN_DOMAIN_REQ request;
    PVOID pBlob = NULL;
    size_t blobSize = 0;

    request.pszHostname = pHostname;
    request.pszHostDnsDomain = pHostDnsDomain;
    request.pszDomain = pDomain;
    request.pszOU = pOu;
    request.pszUsername = pUsername;
    request.pszPassword = pPassword;
    request.pszOSName = pOsName;
    request.pszOSVersion = pOsVersion;
    request.pszOSServicePack = pOsServicePack;
    request.dwFlags = dwFlags;
    request.dwUac = dwUac;

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(NULL, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                                  pDataContext,
                                  LsaAdIPCGetJoinDomainReqSpec(),
                                  &request,
                                  &pBlob,
                                  &blobSize));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderIoControl(
        hLsaConnection,
        LSA_PROVIDER_TAG_AD,
        LSA_AD_IO_JOINDOMAIN,
        (DWORD) blobSize,
        pBlob,
        NULL,
        NULL);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_MEMORY(pBlob);
   
    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaAdJoinDomainDn(
    IN HANDLE hLsaConnection,
    IN PCSTR pHostname,
    IN PCSTR pHostDnsDomain,
    IN PCSTR pDomain,
    IN PCSTR pOu,
    IN PCSTR pUsername,
    IN PCSTR pPassword,
    IN PCSTR pOsName,
    IN PCSTR pOsVersion,
    IN PCSTR pOsServicePack,
    IN LSA_NET_JOIN_FLAGS dwFlags
    )
{
    return LsaAdJoinDomainInternal(
               hLsaConnection,
               pHostname,
               pHostDnsDomain,
               pDomain,
               pOu,
               pUsername,
               pPassword,
               pOsName,
               pOsVersion,
               pOsServicePack,
               dwFlags,
               0);
}

DWORD
LsaAdLeaveDomain(
    HANDLE hLsaConnection,
    PCSTR pszUsername,
    PCSTR pszPassword
    )
{
    return LsaAdLeaveDomain2(
               hLsaConnection,
               pszUsername,
               pszPassword,
               NULL,
               0);
}

DWORD
LsaAdLeaveDomain2(
    HANDLE hLsaConnection,
    PCSTR pszUsername,
    PCSTR pszPassword,
    PCSTR pszDomain,
    LSA_NET_JOIN_FLAGS dwFlags
    )
{
    DWORD dwError = 0;
    LWMsgDataContext* pDataContext = NULL;
    LSA_AD_IPC_LEAVE_DOMAIN_REQ request;
    PVOID pBlob = NULL;
    size_t blobSize = 0;

    request.pszUsername = pszUsername;
    request.pszPassword = pszPassword;
    request.pszDomain = pszDomain;
    request.dwFlags = dwFlags;

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(NULL, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                                  pDataContext,
                                  LsaAdIPCGetLeaveDomainReqSpec(),
                                  &request,
                                  &pBlob,
                                  &blobSize));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderIoControl(
        hLsaConnection,
        LSA_PROVIDER_TAG_AD,
        LSA_AD_IO_LEAVEDOMAIN,
        (DWORD) blobSize,
        pBlob,
        NULL,
        NULL);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_MEMORY(pBlob);
   
    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaAdSetDefaultDomain(
    IN HANDLE hLsaConnection,
    IN PCSTR pszDomain
    )
{
    DWORD dwError = 0;

    if (geteuid() != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderIoControl(
                  hLsaConnection,
                  LSA_PROVIDER_TAG_AD,
                  LSA_AD_IO_SETDEFAULTDOMAIN,
                  strlen(pszDomain) + 1,
                  (PVOID)pszDomain,
                  NULL,
                  NULL);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
LsaAdGetJoinedDomains(
    IN HANDLE hLsaConnection,
    OUT PDWORD pdwNumDomainsFound,
    OUT PSTR** pppszJoinedDomains
    )
{
    DWORD dwError = 0;
    DWORD dwOutputBufferSize = 0;
    PVOID pOutputBuffer = NULL;
    LWMsgContext* context = NULL;
    LWMsgDataContext* pDataContext = NULL;
    PLSA_AD_IPC_GET_JOINED_DOMAINS_RESP response = NULL;

    dwError = LsaProviderIoControl(
                  hLsaConnection,
                  LSA_PROVIDER_TAG_AD,
                  LSA_AD_IO_GETJOINEDDOMAINS,
                  0,
                  NULL,
                  &dwOutputBufferSize,
                  &pOutputBuffer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                              pDataContext,
                              LsaAdIPCGetJoinedDomainsRespSpec(),
                              pOutputBuffer,
                              dwOutputBufferSize,
                              (PVOID*)&response));
    BAIL_ON_LSA_ERROR(dwError);

    *pdwNumDomainsFound = response->dwObjectsCount;
    *pppszJoinedDomains = response->ppszDomains;
    response->ppszDomains = NULL;

cleanup:

    if ( response )
    {
        lwmsg_data_free_graph(
            pDataContext,
            LsaAdIPCGetJoinedDomainsRespSpec(),
            response);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if ( context )
    {
        lwmsg_context_delete(context);
    }

    if ( pOutputBuffer )
    {
        LwFreeMemory(pOutputBuffer);
    }

    return dwError;

error:

    *pdwNumDomainsFound = 0;
    *pppszJoinedDomains = NULL;

    goto cleanup;
}

static
inline
VOID
LsaAdpFreeMachineAccountInfoContents(
    IN OUT PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    )
{
    LW_SAFE_FREE_STRING(pAccountInfo->DnsDomainName);
    LW_SAFE_FREE_STRING(pAccountInfo->NetbiosDomainName);
    LW_SAFE_FREE_STRING(pAccountInfo->DomainSid);
    LW_SAFE_FREE_STRING(pAccountInfo->SamAccountName);
    LW_SAFE_FREE_STRING(pAccountInfo->Fqdn);
}

LW_DWORD
LsaAdGetMachineAccountInfo(
    LW_IN LW_HANDLE hLsaConnection,
    LW_IN LW_OPTIONAL LW_PCSTR pszDnsDomainName,
    LW_OUT PLSA_MACHINE_ACCOUNT_INFO_A* ppAccountInfo
    )
{
    DWORD dwError = 0;
    size_t inputBufferSize = 0;
    PVOID pInputBuffer = NULL;
    DWORD dwOutputBufferSize = 0;
    PVOID pOutputBuffer = NULL;
    LWMsgContext* pContext = NULL;
    LWMsgDataContext* pDataContext = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &pContext));
    BAIL_ON_LSA_ERROR(dwError);

    LsaAdIPCSetMemoryFunctions(pContext);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(pContext, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                                  pDataContext,
                                  LsaAdIPCGetStringSpec(),
                                  (PVOID) pszDnsDomainName,
                                  &pInputBuffer,
                                  &inputBufferSize));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderIoControl(
                  hLsaConnection,
                  LSA_PROVIDER_TAG_AD,
                  LSA_AD_IO_GET_MACHINE_ACCOUNT,
                  inputBufferSize,
                  pInputBuffer,
                  &dwOutputBufferSize,
                  &pOutputBuffer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                              pDataContext,
                              LsaAdIPCGetMachineAccountInfoSpec(),
                              pOutputBuffer,
                              dwOutputBufferSize,
                              OUT_PPVOID(&pAccountInfo)));
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pAccountInfo)
        {
            LsaAdFreeMachineAccountInfo(pAccountInfo);
            pAccountInfo = NULL;
        }
    }

    if (pOutputBuffer)
    {
        LwFreeMemory(pOutputBuffer);
    }

    if (pInputBuffer)
    {
        LwFreeMemory(pInputBuffer);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    *ppAccountInfo = pAccountInfo;

    return dwError;
}

LW_VOID
LsaAdFreeMachineAccountInfo(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    )
{
    if (pAccountInfo)
    {
        LsaAdpFreeMachineAccountInfoContents(pAccountInfo);
        LwFreeMemory(pAccountInfo);
    }
}

LW_DWORD
LsaAdGetMachinePasswordInfo(
    LW_IN LW_HANDLE hLsaConnection,
    LW_IN LW_PCSTR pszDnsDomainName,
    LW_OUT PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    size_t inputBufferSize = 0;
    PVOID pInputBuffer = NULL;
    DWORD dwOutputBufferSize = 0;
    PVOID pOutputBuffer = NULL;
    LWMsgContext* pContext = NULL;
    LWMsgDataContext* pDataContext = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &pContext));
    BAIL_ON_LSA_ERROR(dwError);

    LsaAdIPCSetMemoryFunctions(pContext);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(pContext, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                                  pDataContext,
                                  LsaAdIPCGetStringSpec(),
                                  (PVOID) pszDnsDomainName,
                                  &pInputBuffer,
                                  &inputBufferSize));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderIoControl(
                  hLsaConnection,
                  LSA_PROVIDER_TAG_AD,
                  LSA_AD_IO_GET_MACHINE_PASSWORD,
                  inputBufferSize,
                  pInputBuffer,
                  &dwOutputBufferSize,
                  &pOutputBuffer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                              pDataContext,
                              LsaAdIPCGetMachinePasswordInfoSpec(),
                              pOutputBuffer,
                              dwOutputBufferSize,
                              OUT_PPVOID(&pPasswordInfo)));
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pPasswordInfo)
        {
            LsaAdFreeMachinePasswordInfo(pPasswordInfo);
            pPasswordInfo = NULL;
        }
    }

    if (pOutputBuffer)
    {
        LwFreeMemory(pOutputBuffer);
    }

    if (pInputBuffer)
    {
        LwFreeMemory(pInputBuffer);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    *ppPasswordInfo = pPasswordInfo;

    return dwError;
}

LW_VOID
LsaAdFreeMachinePasswordInfo(
    LW_IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    )
{
    if (pPasswordInfo)
    {
        LsaAdpFreeMachineAccountInfoContents(&pPasswordInfo->Account);
        LW_SECURE_FREE_STRING(pPasswordInfo->Password);
        LwFreeMemory(pPasswordInfo);
    }
}

LW_DWORD
LsaAdGetComputerDn(
    LW_IN LW_HANDLE hLsaConnection,
    LW_IN LW_OPTIONAL LW_PCSTR pszDnsDomainName,
    LW_OUT LW_PSTR* ppszComputerDn
    )
{
    DWORD dwError = 0;
    size_t inputBufferSize = 0;
    PVOID pInputBuffer = NULL;
    DWORD dwOutputBufferSize = 0;
    PVOID pOutputBuffer = NULL;
    LWMsgContext* pContext = NULL;
    LWMsgDataContext* pDataContext = NULL;
    PSTR pszComputerDn = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &pContext));
    BAIL_ON_LSA_ERROR(dwError);

    LsaAdIPCSetMemoryFunctions(pContext);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(pContext, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                                    pDataContext,
                                    LsaAdIPCGetStringSpec(),
                                    (PVOID) pszDnsDomainName,
                                    &pInputBuffer,
                                    &inputBufferSize));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderIoControl(
                    hLsaConnection,
                    LSA_PROVIDER_TAG_AD,
                    LSA_AD_IO_GET_COMPUTER_DN,
                    inputBufferSize,
                    pInputBuffer,
                    &dwOutputBufferSize,
                    &pOutputBuffer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                                    pDataContext,
                                    LsaAdIPCGetStringSpec(),
                                    pOutputBuffer,
                                    dwOutputBufferSize,
                                    OUT_PPVOID(&pszComputerDn)));
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        LW_SAFE_FREE_STRING(pszComputerDn);
    }

    if (pOutputBuffer)
    {
        LwFreeMemory(pOutputBuffer);
    }

    if (pInputBuffer)
    {
        LwFreeMemory(pInputBuffer);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    *ppszComputerDn = pszComputerDn;

    return dwError;
}


