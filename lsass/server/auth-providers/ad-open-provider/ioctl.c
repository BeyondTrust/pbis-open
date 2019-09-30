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
 *        ioctl.c
 *
 * Abstract:
 *
 *        AD Provider IOCTL Handlers
 *
 * Authors: Danilo Almeida <dalmeida@likewise.com>
 *
 */

#include "adprovider.h"

static
DWORD
AD_GetComputerDn(
    IN OPTIONAL PCSTR pszDnsDomainName,
    OUT PSTR* ppszComputerDn
    );

static
DWORD
AD_RawFindComputerDn(
    IN HANDLE hDirectory,
    IN PCSTR pszDomain,
    IN PCSTR pszSamAccountName,
    OUT PSTR* ppszComputerDn
    );


DWORD
AD_IoctlGetMachineAccount(
    IN HANDLE hProvider,
    IN DWORD dwInputBufferSize,
    IN PVOID pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    PVOID pOutputBuffer = NULL;
    size_t outputBufferSize = 0;
    LWMsgContext* pContext = NULL;
    LWMsgDataContext* pDataContext = NULL;
    PSTR pszDnsDomainName = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    //
    // Everyone can call this, so no access check.
    //

    //
    // Do request
    //

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &pContext));
    BAIL_ON_LSA_ERROR(dwError);

    LsaAdIPCSetMemoryFunctions(pContext);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(pContext, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                                  pDataContext,
                                  LsaAdIPCGetStringSpec(),
                                  pInputBuffer,
                                  dwInputBufferSize,
                                  OUT_PPVOID(&pszDnsDomainName)));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_GetMachineAccountInfoA(pszDnsDomainName, &pAccountInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                                  pDataContext,
                                  LsaAdIPCGetMachineAccountInfoSpec(),
                                  pAccountInfo,
                                  &pOutputBuffer,
                                  &outputBufferSize));
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pOutputBuffer)
        {
            LwFreeMemory(pOutputBuffer);
        }
        pOutputBuffer = NULL;
        outputBufferSize = 0;
    }

    LW_SAFE_FREE_STRING(pszDnsDomainName);

    if (pAccountInfo)
    {
        LsaSrvFreeMachineAccountInfoA(pAccountInfo);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    *pdwOutputBufferSize = (DWORD) outputBufferSize;
    *ppOutputBuffer = pOutputBuffer;

    return dwError;
}

DWORD
AD_IoctlGetMachinePassword(
    IN HANDLE hProvider,
    IN DWORD dwInputBufferSize,
    IN PVOID pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    PVOID pOutputBuffer = NULL;
    size_t outputBufferSize = 0;
    PAD_PROVIDER_CONTEXT pProviderContext = (PAD_PROVIDER_CONTEXT)hProvider;
    LWMsgContext* pContext = NULL;
    LWMsgDataContext* pDataContext = NULL;
    PSTR pszDnsDomainName = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;

    //
    // Do access check
    //

    if (pProviderContext->uid)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    //
    // Do request
    //

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &pContext));
    BAIL_ON_LSA_ERROR(dwError);

    LsaAdIPCSetMemoryFunctions(pContext);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(pContext, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                                  pDataContext,
                                  LsaAdIPCGetStringSpec(),
                                  pInputBuffer,
                                  dwInputBufferSize,
                                  OUT_PPVOID(&pszDnsDomainName)));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_GetMachinePasswordInfoA(pszDnsDomainName, &pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                                  pDataContext,
                                  LsaAdIPCGetMachinePasswordInfoSpec(),
                                  pPasswordInfo,
                                  &pOutputBuffer,
                                  &outputBufferSize));
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pOutputBuffer)
        {
            LwFreeMemory(pOutputBuffer);
        }
        pOutputBuffer = NULL;
        outputBufferSize = 0;
    }

    LW_SAFE_FREE_STRING(pszDnsDomainName);

    if (pPasswordInfo)
    {
        LsaSrvFreeMachinePasswordInfoA(pPasswordInfo);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    *pdwOutputBufferSize = (DWORD) outputBufferSize;
    *ppOutputBuffer = pOutputBuffer;

    return dwError;
}

DWORD
AD_IoctlGetComputerDn(
    IN HANDLE hProvider,
    IN DWORD dwInputBufferSize,
    IN PVOID pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    PVOID pOutputBuffer = NULL;
    size_t outputBufferSize = 0;
    PAD_PROVIDER_CONTEXT pProviderContext = (PAD_PROVIDER_CONTEXT)hProvider;
    LWMsgContext* pContext = NULL;
    LWMsgDataContext* pDataContext = NULL;
    PSTR pszDnsDomainName = NULL;
    PSTR pszComputerDn = NULL;

    //
    // Do access check
    //

    if (pProviderContext->uid)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    //
    // Do request
    //

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &pContext));
    BAIL_ON_LSA_ERROR(dwError);

    LsaAdIPCSetMemoryFunctions(pContext);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(pContext, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                                  pDataContext,
                                  LsaAdIPCGetStringSpec(),
                                  pInputBuffer,
                                  dwInputBufferSize,
                                  OUT_PPVOID(&pszDnsDomainName)));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_GetComputerDn(pszDnsDomainName, &pszComputerDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                                  pDataContext,
                                  LsaAdIPCGetStringSpec(),
                                  pszComputerDn,
                                  &pOutputBuffer,
                                  &outputBufferSize));
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        if (pOutputBuffer)
        {
            LwFreeMemory(pOutputBuffer);
        }
        pOutputBuffer = NULL;
        outputBufferSize = 0;
    }

    LW_SAFE_FREE_STRING(pszDnsDomainName);
    LW_SAFE_FREE_STRING(pszComputerDn);

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    *pdwOutputBufferSize = (DWORD) outputBufferSize;
    *ppOutputBuffer = pOutputBuffer;

    return dwError;
}

static
DWORD
AD_GetComputerDn(
    IN OPTIONAL PCSTR pszDnsDomainName,
    OUT PSTR* ppszComputerDn
    )
{
    DWORD dwError = 0;
    PSTR pszComputerDn = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;
    PSTR pszUserPrincipalName = NULL;
    PCSTR pszKrb5CachePath = "MEMORY:lsass_get_computer_dn";
    PSTR pszPreviousKrb5CachePath = NULL;
    HANDLE hDirectory = NULL;
    // Lock to protect the cache:
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    BOOLEAN isLocked = FALSE;
    BOOLEAN needDestroyCache = FALSE;

    //
    // In the future, if we have cached DN information in the domain state,
    // we will use that.  However, since that does not currently exist,
    // we will fetch it directly.
    //
    // Since this code should work regardless of the internal state,
    // we will need to use a private memory cache to establish credentials.
    //

    dwError = AD_GetMachinePasswordInfoA(pszDnsDomainName, &pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszUserPrincipalName,
                    "%s@%s",
                    pPasswordInfo->Account.SamAccountName,
                    pPasswordInfo->Account.DnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMapErrnoToLwError(pthread_mutex_lock(&mutex));
    BAIL_ON_LSA_ERROR(dwError);
    isLocked = TRUE;

    dwError = LwKrb5InitializeCredentials(
                    pszUserPrincipalName,
                    pPasswordInfo->Password,
                    pszKrb5CachePath,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);
    needDestroyCache = TRUE;

    dwError = LwKrb5SetThreadDefaultCachePath(pszKrb5CachePath, &pszPreviousKrb5CachePath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapOpenDirectoryDomain(
                    pPasswordInfo->Account.DnsDomainName,
                    NULL,
                    0,
                    &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_RawFindComputerDn(
                    hDirectory,
                    pPasswordInfo->Account.DnsDomainName,
                    pPasswordInfo->Account.SamAccountName,
                    &pszComputerDn);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        LW_SAFE_FREE_STRING(pszComputerDn);
    }

    if (needDestroyCache)
    {
        LwKrb5DestroyCache(pszKrb5CachePath);
    }

    if (pszPreviousKrb5CachePath)
    {
        LwKrb5SetThreadDefaultCachePath(pszPreviousKrb5CachePath, NULL);
    }

    if (isLocked)
    {
        int localError = pthread_mutex_unlock(&mutex);
        LSA_ASSERT(!localError);
    }

    LsaSrvFreeMachinePasswordInfoA(pPasswordInfo);
    LW_SAFE_FREE_STRING(pszUserPrincipalName);
    LW_SAFE_FREE_STRING(pszPreviousKrb5CachePath);

    if (hDirectory)
    {
        LwLdapCloseDirectory(hDirectory);
    }

    *ppszComputerDn = pszComputerDn;

    return dwError;
}

static
DWORD
AD_RawFindComputerDn(
    IN HANDLE hDirectory,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszSamAccountName,
    OUT PSTR* ppszComputerDn
    )
{
    DWORD dwError = 0;
    LDAP* pLd = NULL;
    PSTR pszDirectoryRoot = NULL;
    PSTR pszEscapedSamAccountName = NULL;
    PSTR szAttributeList[] = {"*", NULL};
    PSTR pszQuery = NULL;
    LDAPMessage *pMessage = NULL;
    int count = 0;
    PSTR pszComputerDn = NULL;

    pLd = LwLdapGetSession(hDirectory);

    dwError = LwLdapConvertDomainToDN(
                    pszDnsDomainName,
                    &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwLdapEscapeString(
                    &pszEscapedSamAccountName,
                    pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);
    
    LwStrToUpper(pszEscapedSamAccountName);

    dwError = LwAllocateStringPrintf(
                    &pszQuery,
                    "(sAMAccountName=%s)",
                    pszEscapedSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwLdapDirectorySearch(
                    hDirectory,
                    pszDirectoryRoot,
                    LDAP_SCOPE_SUBTREE,
                    pszQuery,
                    szAttributeList,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    count = ldap_count_entries(pLd, pMessage);
    if (count < 0)
    {
        dwError = LW_ERROR_LDAP_ERROR;
    }
    else if (count == 0)
    {
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
    }
    else if (count > 1)
    {
        dwError = LW_ERROR_DUPLICATE_DOMAINNAME;
    }
    BAIL_ON_LSA_ERROR(dwError);   

    dwError = LwLdapGetDN(hDirectory, pMessage, &pszComputerDn);
    BAIL_ON_LSA_ERROR(dwError);

    if (LW_IS_NULL_OR_EMPTY_STR(pszComputerDn))
    {
        dwError = LW_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);        
    }

error:
    if (dwError)
    {
        LW_SAFE_FREE_STRING(pszComputerDn);
    }

    LW_SAFE_FREE_STRING(pszDirectoryRoot);
    LW_SAFE_FREE_STRING(pszEscapedSamAccountName);
    LW_SAFE_FREE_STRING(pszQuery);

    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }

    *ppszComputerDn = pszComputerDn;

    return dwError;
}
