/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        samr_lookupnames.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrLookupNames function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvLookupNames(
    IN  handle_t        hBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  DWORD           dwNumNames,
    IN  UNICODE_STRING *pNames,
    OUT IDS            *pOutIds,
    OUT IDS            *pOutTypes
    )
{
    PCSTR filterFormat = "(%s=%u AND %s=%Q AND %s=%Q) OR "
                         "(%s=%u AND %s=%Q AND %s=%Q)";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    HANDLE hDirectory = NULL;
    IDS *pIds = NULL;
    IDS *pTypes = NULL;
    PWSTR pwszDn = NULL;
    PWSTR pwszDomainName = NULL;
    PSTR pszDomainName = NULL;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    CHAR szAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    CHAR szAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    CHAR szAttrDomainName[] = DS_ATTR_DOMAIN;
    DWORD dwObjectClassUser = DS_OBJECT_CLASS_USER;
    DWORD dwObjectClassGroup = DS_OBJECT_CLASS_LOCAL_GROUP;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    PWSTR wszAttributes[3];
    DWORD i = 0;
    PWSTR pwszName = NULL;
    PSTR pszName = NULL;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    PWSTR pwszSid = NULL;
    PSID pSid = NULL;
    DWORD dwObjectClass = 0;
    DWORD dwRid = 0;
    DWORD dwType = 0;
    DWORD dwUnknownNamesNum = 0;

    pDomCtx = (PDOMAIN_CONTEXT)hDomain;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    hDirectory     = pDomCtx->pConnCtx->hDirectory;
    pwszDn         = pDomCtx->pwszDn;
    pwszDomainName = pDomCtx->pwszDomainName;

    dwError = LwWc16sToMbs(pwszDomainName,
                           &pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrSrvAllocateMemory((void**)&pIds,
                                     sizeof(*pIds));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pIds->dwCount = dwNumNames;
    ntStatus = SamrSrvAllocateMemory((void**)&(pIds->pIds),
                                     pIds->dwCount * sizeof(pIds->pIds[0]));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = SamrSrvAllocateMemory((void**)&pTypes,
                                     sizeof(*pTypes));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pTypes->dwCount = dwNumNames;
    ntStatus = SamrSrvAllocateMemory((void**)&(pTypes->pIds),
                                     pTypes->dwCount * sizeof(pTypes->pIds[0]));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    for (i = 0; i < dwNumNames; i++)
    {
        UNICODE_STRING *name = &(pNames[i]);

        ntStatus = SamrSrvGetFromUnicodeString(&pwszName,
                                               name);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwError = LwWc16sToMbs(pwszName,
                               &pszName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryAllocateWC16StringFilterPrintf(
                                &pwszFilter,
                                filterFormat,
                                szAttrObjectClass, dwObjectClassUser,
                                szAttrSamAccountName, pszName,
                                szAttrDomainName, pszDomainName,
                                szAttrObjectClass, dwObjectClassGroup,
                                szAttrSamAccountName, pszName,
                                szAttrDomainName, pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        wszAttributes[0] = wszAttrObjectSid;
        wszAttributes[1] = wszAttrObjectClass;
        wszAttributes[2] = NULL;

        pEntry   = NULL;

        dwError = DirectorySearch(hDirectory,
                                  pwszDn,
                                  dwScope,
                                  pwszFilter,
                                  wszAttributes,
                                  FALSE,
                                  &pEntries,
                                  &dwEntriesNum);
        BAIL_ON_LSA_ERROR(dwError);

        if (dwEntriesNum == 1)
        {
            pEntry  = &(pEntries[0]);

            pwszSid       = NULL;
            dwRid         = 0;
            dwType        = 0;
            dwObjectClass = 0;

            dwError = DirectoryGetEntryAttrValueByName(
                                            pEntry,
                                            wszAttrObjectSid,
                                            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                            &pwszSid);
            if (pwszSid && dwError == 0)
            {
                ntStatus = RtlAllocateSidFromWC16String(&pSid, pwszSid);
                BAIL_ON_NTSTATUS_ERROR(ntStatus);

                dwRid = pSid->SubAuthority[pSid->SubAuthorityCount - 1];
            }

            dwError = DirectoryGetEntryAttrValueByName(
                                            pEntry,
                                            wszAttrObjectClass,
                                            DIRECTORY_ATTR_TYPE_INTEGER,
                                            &dwObjectClass);
            if (dwError == 0)
            {
                switch (dwObjectClass) 
                {
                case DS_OBJECT_CLASS_USER:
                    dwType = SID_TYPE_USER;
                    break;

                case DS_OBJECT_CLASS_LOCAL_GROUP:
                    dwType = SID_TYPE_ALIAS;
                    break;

                default:
                    dwType = SID_TYPE_INVALID;
                }

                BAIL_ON_NTSTATUS_ERROR(ntStatus);
            }


            pIds->pIds[i]   = dwRid;
            pTypes->pIds[i] = dwType;

        }
        else if (dwEntriesNum == 0)
        {
            pIds->pIds[i]   = 0;
            pTypes->pIds[i] = SID_TYPE_UNKNOWN;

        }
        else
        {
            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        if (pEntries)
        {
            DirectoryFreeEntries(pEntries, dwEntriesNum);
            pEntries = NULL;
        }

        if (pwszName)
        {
            SamrSrvFreeMemory(pwszName);
            pwszName = NULL;
        }

        if (pSid)
        {
            RTL_FREE(&pSid);
        }

        LW_SAFE_FREE_MEMORY(pszName);
        LW_SAFE_FREE_MEMORY(pwszFilter);
    }

    pOutIds->dwCount    = pIds->dwCount;
    pOutIds->pIds       = pIds->pIds;
    pOutTypes->dwCount  = pTypes->dwCount;
    pOutTypes->pIds     = pTypes->pIds;

    for (i = 0; i < pTypes->dwCount; i++)
    {
        if (pTypes->pIds[i] == SID_TYPE_UNKNOWN)
        {
            dwUnknownNamesNum++;
        }
    }

    if (dwUnknownNamesNum > 0)
    {
        if (dwUnknownNamesNum < pTypes->dwCount)
        {
            ntStatus = LW_STATUS_SOME_NOT_MAPPED;
        }
        else
        {
            ntStatus = STATUS_NONE_MAPPED;
        }
    }

cleanup:
    if (pwszName)
    {
        SamrSrvFreeMemory(pwszName);
    }

    if (pSid)
    {
        RTL_FREE(&pSid);
    }

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwEntriesNum);
    }

    if (pIds)
    {
        SamrSrvFreeMemory(pIds);
    }

    if (pTypes)
    {
        SamrSrvFreeMemory(pTypes);
    }

    LW_SAFE_FREE_MEMORY(pszName);
    LW_SAFE_FREE_MEMORY(pszDomainName);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pOutIds->pIds)
    {
        SamrSrvFreeMemory(pOutIds->pIds);
    }

    if (pOutTypes->pIds)
    {
        SamrSrvFreeMemory(pOutTypes->pIds);
    }

    pOutIds->dwCount    = 0;
    pOutIds->pIds       = NULL;
    pOutTypes->dwCount  = 0;
    pOutTypes->pIds     = NULL;

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
