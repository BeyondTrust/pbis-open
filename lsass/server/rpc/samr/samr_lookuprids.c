/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 *        samr_lookuprids.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrLookupRids function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvLookupRids(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UINT32 dwNumRids,
    /* [in] */ UINT32 *pdwRids,
    /* [out] */ UNICODE_STRING_ARRAY *pNames,
    /* [out] */ IDS *pTypes
    )
{
    const wchar_t wszFilterFmt[] = L"%ws='%ws'";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    HANDLE hDirectory = NULL;
    DWORD i = 0;
    PSID pDomSid = NULL;
    DWORD dwSidLen = 0;
    PSID pSid = NULL;
    PWSTR pwszSid = NULL;
    size_t sSidStrLen = 0;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    DWORD dwFilterLen = 0;
    PWSTR pwszFilter = NULL;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    PWSTR pwszAccountName = NULL;
    DWORD dwObjectClass = 0;
    UNICODE_STRING_ARRAY Names = {0};
    IDS Types = {0};
    DWORD dwUnknownNamesNum = 0;

    PWSTR wszAttributes[] = {
        wszAttrSamAccountName,
        wszAttrObjectClass,
        NULL
    };

    pDomCtx    = (PDOMAIN_CONTEXT)hDomain;
    pDomSid    = pDomCtx->pDomainSid;
    hDirectory = pDomCtx->pConnCtx->hDirectory;

    dwSidLen = RtlLengthRequiredSid(pDomSid->SubAuthorityCount + 1);

    dwError = LwAllocateMemory(dwSidLen,
                               OUT_PPVOID(&pSid));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlCopySid(dwSidLen,
                        pSid,
                        pDomSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pSid->SubAuthorityCount++;

    ntStatus = SamrSrvAllocateMemory(OUT_PPVOID(&Types.pIds),
                                   sizeof(Types.pIds[0]) * dwNumRids);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = SamrSrvAllocateMemory(OUT_PPVOID(&Names.pNames),
                                   sizeof(Names.pNames[0]) * dwNumRids);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    for (i = 0; i < dwNumRids; i++)
    {
        pSid->SubAuthority[pSid->SubAuthorityCount - 1] = pdwRids[i];

        ntStatus = RtlAllocateWC16StringFromSid(&pwszSid,
                                              pSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwError = LwWc16sLen(pwszSid, &sSidStrLen);
        BAIL_ON_LSA_ERROR(dwError);

        dwFilterLen = ((sizeof(wszAttrObjectSid)/sizeof(WCHAR)) - 1) +
                      sSidStrLen +
                      (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

        dwError = LwAllocateMemory(sizeof(WCHAR) * dwFilterLen,
                                   OUT_PPVOID(&pwszFilter));

        sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                    wszAttrObjectSid,
                    pwszSid);

        dwError = DirectorySearch(hDirectory,
                                  pwszBase,
                                  dwScope,
                                  pwszFilter,
                                  wszAttributes,
                                  FALSE,
                                  &pEntry,
                                  &dwEntriesNum);
        BAIL_ON_LSA_ERROR(dwError);

        pwszAccountName = NULL;
        dwObjectClass   = DS_OBJECT_CLASS_UNKNOWN;

        if (dwEntriesNum == 0)
        {
            ntStatus = SamrSrvInitUnicodeString(
                                      &Names.pNames[i],
                                      pwszAccountName);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            Names.dwCount++;

            Types.pIds[i] = SID_TYPE_UNKNOWN;
            Types.dwCount++;
        }
        else if (dwEntriesNum > 1)
        {
            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }
        else
        {
            dwError = DirectoryGetEntryAttrValueByName(
                                      pEntry,
                                      wszAttrSamAccountName,
                                      DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                      &pwszAccountName);
            BAIL_ON_LSA_ERROR(dwError);

            ntStatus = SamrSrvInitUnicodeString(
                                      &Names.pNames[i],
                                      pwszAccountName);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            Names.dwCount++;

            dwError = DirectoryGetEntryAttrValueByName(
                                      pEntry,
                                      wszAttrObjectClass,
                                      DIRECTORY_ATTR_TYPE_INTEGER,
                                      &dwObjectClass);
            BAIL_ON_LSA_ERROR(dwError);

            switch (dwObjectClass)
            {
            case DS_OBJECT_CLASS_LOCAL_GROUP:
                Types.pIds[i] = SID_TYPE_ALIAS;
                break;

            case DS_OBJECT_CLASS_USER:
                Types.pIds[i] = SID_TYPE_USER;
                break;

            case DS_OBJECT_CLASS_LOCALGRP_MEMBER:
            case DS_OBJECT_CLASS_DOMAIN:
            case DS_OBJECT_CLASS_BUILTIN_DOMAIN:
            case DS_OBJECT_CLASS_CONTAINER: 
            default:
                Types.pIds[i] = SID_TYPE_INVALID;
                break;
            }

            Types.dwCount++;
        }

        if (pEntry)
        {
            DirectoryFreeEntries(pEntry, dwEntriesNum);
            pEntry = NULL;
        }

        LW_SAFE_FREE_MEMORY(pwszFilter);
        RTL_FREE(&pwszSid);
        pwszFilter = NULL;
    }

    pNames->pNames   = Names.pNames;
    pNames->dwCount  = Names.dwCount;
    pTypes->pIds     = Types.pIds;
    pTypes->dwCount  = Types.dwCount;

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
    if (pEntry)
    {
        DirectoryFreeEntries(pEntry, dwEntriesNum);
    }

    LW_SAFE_FREE_MEMORY(pwszFilter);
    RTL_FREE(&pwszSid);
    RTL_FREE(&pSid);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    for (i = 0; i < Names.dwCount; i++)
    {
        SamrSrvFreeUnicodeString(&(Names.pNames[i]));
    }
    SamrSrvFreeMemory(Names.pNames);
    SamrSrvFreeMemory(Types.pIds);

    pNames->pNames   = NULL;
    pNames->dwCount  = 0;
    pTypes->pIds     = NULL;
    pTypes->dwCount  = 0;

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
