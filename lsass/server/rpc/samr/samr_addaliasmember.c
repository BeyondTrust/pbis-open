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
 *        samr_addaliasmember.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrAddAliasMember function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvAddAliasMember(
    IN  handle_t        hBinding,
    IN  ACCOUNT_HANDLE  hAlias,
    IN  PSID            pSid
    )
{
    const DWORD dwPolicyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    const wchar_t wszFilterFmt[] = L"%ws='%ws'";
    const wchar_t wszMemberDnFmt[] = L"CN=%ws,DC=%ws";

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszGroupDn = NULL;
    PWSTR pwszSid = NULL;
    size_t sSidStrLen = 0;
    HANDLE hDirectory = NULL;
    PWSTR pwszBaseDn = NULL;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrDomain[] = DS_ATTR_DOMAIN;
    WCHAR wszAttrNetBIOSName[] = DS_ATTR_NETBIOS_NAME;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    DWORD dwScope = 0;
    DWORD dwFilterLen = 0;
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    PSTR pszDomainFqdn = NULL;
    PSTR pszDcName = NULL;
    PWSTR pwszDcName = NULL;
    LW_PIO_CREDS pCreds = NULL;
    LSA_BINDING hLsaBinding = NULL;
    POLICY_HANDLE hDcPolicy = NULL;
    SID_ARRAY SidsArray = {0};
    RefDomainList *pRemoteDomains = NULL;
    TranslatedName *pRemoteNames = NULL;
    DWORD i = 0;
    DWORD dwLookupLevel = 0;
    DWORD dwRemoteNamesCount = 0;
    PWSTR pwszName = NULL;
    size_t sNameLen = 0;
    DWORD dwObjectClass = DS_OBJECT_CLASS_UNKNOWN;
    PWSTR pwszDomain = NULL;
    size_t sDomainLen = 0;
    PWSTR pwszMemberDn = NULL;
    DWORD dwMemberDnLen = 0;

    PWSTR wszAttributes[] = {
        wszAttrDn,
        wszAttrObjectClass,
        NULL
    };

    enum AttrValueIndex {
        ATTR_IDX_DN,
        ATTR_IDX_OBJECT_CLASS,
        ATTR_IDX_OBJECT_SID,
        ATTR_IDX_DOMAIN,
        ATTR_IDX_NETBIOS_NAME,
        ATTR_IDX_SAM_ACCOUNT_NAME,
        ATTR_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        }
    };

    DIRECTORY_MOD ModDn = {
        .ulOperationFlags = DIR_MOD_FLAGS_ADD,
        .pwszAttrName     = wszAttrDn,
        .ulNumValues      = 1,
        .pAttrValues      = &AttrValues[ATTR_IDX_DN]
    };

    DIRECTORY_MOD ModObjectClass = {
        .ulOperationFlags = DIR_MOD_FLAGS_ADD,
        .pwszAttrName     = wszAttrObjectClass,
        .ulNumValues      = 1,
        .pAttrValues      = &AttrValues[ATTR_IDX_OBJECT_CLASS]
    };

    DIRECTORY_MOD ModObjectSid = {
        .ulOperationFlags = DIR_MOD_FLAGS_ADD,
        .pwszAttrName     = wszAttrObjectSid,
        .ulNumValues      = 1,
        .pAttrValues      = &AttrValues[ATTR_IDX_OBJECT_SID]
    };

    DIRECTORY_MOD ModDomain = {
        .ulOperationFlags = DIR_MOD_FLAGS_ADD,
        .pwszAttrName     = wszAttrDomain,
        .ulNumValues      = 1,
        .pAttrValues      = &AttrValues[ATTR_IDX_DOMAIN]
    };

    DIRECTORY_MOD ModNetBIOSName = {
        .ulOperationFlags = DIR_MOD_FLAGS_ADD,
        .pwszAttrName     = wszAttrNetBIOSName,
        .ulNumValues      = 1,
        .pAttrValues      = &AttrValues[ATTR_IDX_NETBIOS_NAME]
    };

    DIRECTORY_MOD ModSamAccountName = {
        .ulOperationFlags = DIR_MOD_FLAGS_ADD,
        .pwszAttrName     = wszAttrSamAccountName,
        .ulNumValues      = 1,
        .pAttrValues      = &AttrValues[ATTR_IDX_SAM_ACCOUNT_NAME]
    };

    DIRECTORY_MOD Mods[ATTR_IDX_SENTINEL + 1];

    pAcctCtx = (PACCOUNT_CONTEXT)hAlias;

    if (pAcctCtx == NULL || pAcctCtx->Type != SamrContextAccount)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!(pAcctCtx->dwAccessGranted & ALIAS_ACCESS_ADD_MEMBER))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    memset(Mods, 0, sizeof(Mods));

    pDomCtx     = pAcctCtx->pDomCtx;
    pConnCtx    = pDomCtx->pConnCtx;
    hDirectory  = pConnCtx->hDirectory;
    pwszGroupDn = pAcctCtx->pwszDn;

    ntStatus = RtlAllocateWC16StringFromSid(&pwszSid,
                                            pSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwWc16sLen(pwszSid, &sSidStrLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1)) +
                  sSidStrLen +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwFilterLen,
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                    wszAttrObjectSid, pwszSid) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = DirectorySearch(hDirectory,
                              pwszBaseDn,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntry,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwEntriesNum > 1)
    {
        ntStatus = STATUS_INTERNAL_ERROR;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }
    else if (dwEntriesNum == 1)
    {
        dwError = DirectoryGetEntryAttrValueByName(
                                  pEntry,
                                  wszAttrObjectClass,
                                  DIRECTORY_ATTR_TYPE_INTEGER,
                                  &dwObjectClass);
        BAIL_ON_LSA_ERROR(dwError);

        /*
         * Only local users and domain accounts (user/group) can
         * be an alias member
         */
        if (dwObjectClass != DS_OBJECT_CLASS_USER &&
            dwObjectClass != DS_OBJECT_CLASS_LOCALGRP_MEMBER)
        {
            ntStatus = STATUS_INVALID_MEMBER;
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }
    }
    else if (dwEntriesNum == 0)
    {
        dwError = LWNetGetCurrentDomain(&pszDomainFqdn);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LWNetGetDomainController(pszDomainFqdn,
                                           &pszDcName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwMbsToWc16s(pszDcName,
                               &pwszDcName);
        BAIL_ON_LSA_ERROR(dwError);

        ntStatus = SamrSrvGetSystemCreds(&pCreds);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
        
        ntStatus = LsaInitBindingDefault(&hLsaBinding,
                                         pwszDcName,
                                         pCreds);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LsaOpenPolicy2(hLsaBinding,
                                  pwszDcName,
                                  NULL,
                                  dwPolicyAccessMask,
                                  &hDcPolicy);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        SidsArray.dwNumSids = 1;

        dwError = LwAllocateMemory(
                           sizeof(SidsArray.pSids[0]) * SidsArray.dwNumSids,
                           OUT_PPVOID(&SidsArray.pSids));
        BAIL_ON_LSA_ERROR(dwError);

        SidsArray.pSids[0].pSid = pSid;
        dwLookupLevel           = LSA_LOOKUP_NAMES_ALL;

        ntStatus = LsaLookupSids(hLsaBinding,
                                 hDcPolicy,
                                 &SidsArray,
                                 &pRemoteDomains,
                                 &pRemoteNames,
                                 dwLookupLevel,
                                 &dwRemoteNamesCount);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaClose(hLsaBinding,
                            hDcPolicy);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        /*
         * Only domain users and groups can be a member of an alias
         */
        if (pRemoteNames[0].type != SID_TYPE_USER &&
            pRemoteNames[0].type != SID_TYPE_DOM_GRP &&
            pRemoteNames[0].type != SID_TYPE_WKN_GRP)
        {
            ntStatus = STATUS_INVALID_MEMBER;
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        ntStatus = SamrSrvGetFromUnicodeString(&pwszName,
                                               &pRemoteNames[0].name);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwError = LwWc16sLen(pwszName, &sNameLen);
        BAIL_ON_LSA_ERROR(dwError);

        i = pRemoteNames[0].sid_index;
        ntStatus = SamrSrvGetFromUnicodeStringEx(
                                        &pwszDomain,
                                        &pRemoteDomains->domains[i].name);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwError = LwWc16sLen(pwszDomain, &sDomainLen);
        BAIL_ON_LSA_ERROR(dwError);

        dwMemberDnLen = sNameLen + sDomainLen +
                        (sizeof(wszMemberDnFmt)/sizeof(wszMemberDnFmt[0]));

        dwError = LwAllocateMemory(sizeof(WCHAR) * dwMemberDnLen,
                                   OUT_PPVOID(&pwszMemberDn));
        BAIL_ON_LSA_ERROR(dwError);

        sw16printfw(pwszMemberDn, dwMemberDnLen, wszMemberDnFmt,
                    pwszName, pwszDomain);

        AttrValues[ATTR_IDX_OBJECT_CLASS].data.ulValue
            = DS_OBJECT_CLASS_LOCALGRP_MEMBER;
        AttrValues[ATTR_IDX_DN].data.pwszStringValue             = pwszMemberDn;
        AttrValues[ATTR_IDX_OBJECT_SID].data.pwszStringValue       = pwszSid;
        AttrValues[ATTR_IDX_DOMAIN].data.pwszStringValue           = pwszDomain;
        AttrValues[ATTR_IDX_NETBIOS_NAME].data.pwszStringValue     = pwszDomain;
        AttrValues[ATTR_IDX_SAM_ACCOUNT_NAME].data.pwszStringValue = pwszName;

        i = 0;
        Mods[i++] = ModDn;
        Mods[i++] = ModObjectClass;
        Mods[i++] = ModObjectSid;
        Mods[i++] = ModDomain;
        Mods[i++] = ModNetBIOSName;
        Mods[i++] = ModSamAccountName;

        dwError = DirectoryAddObject(hDirectory,
                                     pwszMemberDn,
                                     Mods);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectorySearch(hDirectory,
                                  pwszBaseDn,
                                  dwScope,
                                  pwszFilter,
                                  wszAttributes,
                                  FALSE,
                                  &pEntry,
                                  &dwEntriesNum);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = DirectoryAddToGroup(hDirectory,
                                  pwszGroupDn,
                                  pEntry);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (hLsaBinding)
    {
        LsaFreeBinding(&hLsaBinding);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (pEntry)
    {
        DirectoryFreeEntries(pEntry, dwEntriesNum);
    }

    LW_SAFE_FREE_MEMORY(pwszFilter);
    RTL_FREE(&pwszSid);

    if (pszDomainFqdn)
    {
        LWNetFreeString(pszDomainFqdn);
    }

    if (pszDcName)
    {
        LWNetFreeString(pszDcName);
    }

    LW_SAFE_FREE_MEMORY(pwszDcName);
    LW_SAFE_FREE_MEMORY(SidsArray.pSids);

    if (pRemoteDomains)
    {
        LsaRpcFreeMemory(pRemoteDomains);
    }

    if (pRemoteNames)
    {
        LsaRpcFreeMemory(pRemoteNames);
    }

    if (pwszName)
    {
        SamrSrvFreeMemory(pwszName);
    }

    if (pwszDomain)
    {
        SamrSrvFreeMemory(pwszDomain);
    }

    LW_SAFE_FREE_MEMORY(pwszMemberDn);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
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
