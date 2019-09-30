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
 *        groups.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Group Lookup and Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "adclient.h"

LSASS_API
DWORD
LsaAdRemoveGroupByNameFromCache(
    IN HANDLE hLsaConnection,
    IN OPTIONAL PCSTR pszDomainName,
    IN PCSTR  pszGroupName
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
                  LSA_AD_IO_REMOVEGROUPBYNAMECACHE,
                  strlen(pszGroupName) + 1,
                  (PVOID)pszGroupName,
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
LsaAdRemoveGroupByIdFromCache(
    IN HANDLE hLsaConnection,
    IN OPTIONAL PCSTR pszDomainName,
    IN gid_t  gid
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
                  LSA_AD_IO_REMOVEGROUPBYIDCACHE,
                  sizeof(gid),
                  &gid,
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
LsaAdEnumGroupsFromCache(
    IN HANDLE   hLsaConnection,
    IN OPTIONAL PCSTR pszDomainName,
    IN PSTR*    ppszResume,
    IN DWORD    dwMaxNumGroups,
    OUT PDWORD  pdwGroupsFound,
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
    LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ request;
    PLSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP response = NULL;

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
    request.dwMaxNumGroups = dwMaxNumGroups;

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                                  pDataContext,
                                  LsaAdIPCGetEnumGroupsFromCacheReqSpec(),
                                  &request,
                                  &pBlob,
                                  &BlobSize));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderIoControl(
                  hLsaConnection,
                  pszTargetProvider ? pszTargetProvider : LSA_PROVIDER_TAG_AD,
                  LSA_AD_IO_ENUMGROUPSCACHE,
                  BlobSize,
                  pBlob,
                  &dwOutputBufferSize,
                  &pOutputBuffer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                                  pDataContext,
                                  LsaAdIPCGetEnumGroupsFromCacheRespSpec(),
                                  pOutputBuffer,
                                  dwOutputBufferSize,
                                  (PVOID*)&response));
    BAIL_ON_LSA_ERROR(dwError);

    *pdwGroupsFound = response->dwNumGroups;
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
            LsaAdIPCGetEnumGroupsFromCacheRespSpec(),
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

    *pdwGroupsFound = 0;
    *pppObjects = NULL;

    goto cleanup;
}
