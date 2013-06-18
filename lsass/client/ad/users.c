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
 *        users.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        User Lookup and Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "adclient.h"

LSASS_API
DWORD
LsaAdRemoveUserByNameFromCache(
    IN HANDLE hLsaConnection,
    IN OPTIONAL PCSTR pszDomainName,
    IN PCSTR  pszName
    )
{
    DWORD dwError = 0;
    PSTR pszTargetProvider = NULL;

    if (geteuid() != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszDomainName)
    {
        dwError = LwAllocateStringPrintf(
                      &pszTargetProvider,
                      "%s:%s",
                      LSA_PROVIDER_TAG_AD,
                      pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderIoControl(
                  hLsaConnection,
                  pszTargetProvider ? pszTargetProvider : LSA_PROVIDER_TAG_AD,
                  LSA_AD_IO_REMOVEUSERBYNAMECACHE,
                  strlen(pszName) + 1,
                  (PVOID)pszName,
                  NULL,
                  NULL);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszTargetProvider);

    return dwError;

error:

    goto cleanup;
}

LSASS_API
DWORD
LsaAdRemoveUserByIdFromCache(
    IN HANDLE hLsaConnection,
    IN OPTIONAL PCSTR pszDomainName,
    IN uid_t  uid
    )
{
    DWORD dwError = 0;
    PSTR pszTargetProvider = NULL;

    if (geteuid() != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszDomainName)
    {
        dwError = LwAllocateStringPrintf(
                      &pszTargetProvider,
                      "%s:%s",
                      LSA_PROVIDER_TAG_AD,
                      pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderIoControl(
                  hLsaConnection,
                  pszTargetProvider ? pszTargetProvider : LSA_PROVIDER_TAG_AD,
                  LSA_AD_IO_REMOVEUSERBYIDCACHE,
                  sizeof(uid),
                  &uid,
                  NULL,
                  NULL);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszTargetProvider);

    return dwError;

error:

    goto cleanup;
}

LSASS_API
DWORD
LsaAdEnumUsersFromCache(
    IN HANDLE   hLsaConnection,
    IN OPTIONAL PCSTR pszDomainName,
    IN PSTR*    ppszResume,
    IN DWORD    dwMaxNumUsers,
    OUT PDWORD  pdwUsersFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PSTR pszTargetProvider = NULL;
    DWORD dwOutputBufferSize = 0;
    PVOID pOutputBuffer = NULL;
    PVOID pBlob = NULL;
    size_t BlobSize = 0;
    LWMsgContext* context = NULL;
    LWMsgDataContext* pDataContext = NULL;
    LSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ request;
    PLSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP response = NULL;

    memset(&request, 0, sizeof(request));

    if (geteuid() != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszDomainName)
    {
        dwError = LwAllocateStringPrintf(
                      &pszTargetProvider,
                      "%s:%s",
                      LSA_PROVIDER_TAG_AD,
                      pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // marshal the request
    request.pszResume = *ppszResume;
    request.dwMaxNumUsers = dwMaxNumUsers;

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataContext,
                              LsaAdIPCGetEnumUsersFromCacheReqSpec(),
                              &request,
                              &pBlob,
                              &BlobSize));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderIoControl(
                  hLsaConnection,
                  pszTargetProvider ? pszTargetProvider : LSA_PROVIDER_TAG_AD,
                  LSA_AD_IO_ENUMUSERSCACHE,
                  BlobSize,
                  pBlob,
                  &dwOutputBufferSize,
                  &pOutputBuffer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                              pDataContext,
                              LsaAdIPCGetEnumUsersFromCacheRespSpec(),
                              pOutputBuffer,
                              dwOutputBufferSize,
                              (PVOID*)&response));
    BAIL_ON_LSA_ERROR(dwError);

    *pdwUsersFound = response->dwNumUsers;
    *pppObjects = response->ppObjects;
    response->ppObjects = NULL;
    if ( *ppszResume )
    {
        LwFreeMemory(*ppszResume);
        *ppszResume = NULL;
    }
    *ppszResume = response->pszResume;
    response->pszResume = NULL;

cleanup:

    if ( response )
    {
        lwmsg_data_free_graph(
            pDataContext,
            LsaAdIPCGetEnumUsersFromCacheRespSpec(),
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

    if ( pBlob )
    {
        LwFreeMemory(pBlob);
    }

    if ( pOutputBuffer )
    {
        LwFreeMemory(pOutputBuffer);
    }

    LW_SAFE_FREE_STRING(pszTargetProvider);

    return dwError;

error:

    if ( *ppszResume )
    {
        LwFreeMemory(*ppszResume);
        *ppszResume = NULL;
    }

    *pdwUsersFound = 0;
    *pppObjects = NULL;

    goto cleanup;
}
