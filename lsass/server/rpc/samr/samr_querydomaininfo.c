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
 *        samr_querydomaininfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrQueryDomainInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
SamrSrvFillDomainInfo1(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo2(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo3(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo4(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo5(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo6(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo7(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo8(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo9(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo11(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo12(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo13(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );



NTSTATUS
SamrSrvQueryDomainInfo(
    /* [in] */ handle_t hBinding,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UINT16 level,
    /* [out] */ DomainInfo **ppInfo
    )
{
    const wchar_t wszFilterFmt[] = L"(%ws=%u OR %ws=%u) AND %ws='%ws'";

    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrDomain[] = DS_ATTR_DOMAIN;
    WCHAR wszAttrComment[] = DS_ATTR_COMMENT;
    WCHAR wszAttrCreatedTime[] = DS_ATTR_CREATED_TIME;
    WCHAR wszAttrMinPasswordLen[] = DS_ATTR_MIN_PWD_LENGTH;
    WCHAR wszAttrPasswordHistoryLen[] = DS_ATTR_PWD_HISTORY_LENGTH;
    WCHAR wszAttrPasswordProperties[] = DS_ATTR_PWD_PROPERTIES;
    WCHAR wszAttrMaxPasswordAge[] = DS_ATTR_MAX_PWD_AGE;
    WCHAR wszAttrMinPasswordAge[] = DS_ATTR_MIN_PWD_AGE;
    WCHAR wszAttrForceLogoffTime[] = DS_ATTR_FORCE_LOGOFF_TIME;
    WCHAR wszAttrRole[] = DS_ATTR_ROLE;
    WCHAR wszAttrLockoutDuration[] = DS_ATTR_LOCKOUT_DURATION;
    WCHAR wszAttrLockoutWindow[] = DS_ATTR_LOCKOUT_WINDOW;
    WCHAR wszAttrLockoutThreshold[] = DS_ATTR_LOCKOUT_THRESHOLD;

    PWSTR wszAttributeLevel1[] = {
        wszAttrDn,
        wszAttrMinPasswordLen,
        wszAttrPasswordHistoryLen,
        wszAttrPasswordProperties,
        wszAttrMaxPasswordAge,
        wszAttrMinPasswordAge,
        NULL
    };

    PWSTR wszAttributeLevel2[] = {
        wszAttrDn,
        wszAttrForceLogoffTime,
        wszAttrDomain,
        wszAttrComment,
        wszAttrRole,
        NULL
    };

    PWSTR wszAttributeLevel3[] = {
        wszAttrDn,
        wszAttrForceLogoffTime,
        NULL
    };

    PWSTR wszAttributeLevel4[] = {
        wszAttrDn,
        wszAttrComment,
        NULL
    };

    PWSTR wszAttributeLevel5[] = {
        wszAttrDn,
        wszAttrDomain,
        NULL
    };

    PWSTR wszAttributeLevel6[] = {
        wszAttrDn,
        NULL
    };

    PWSTR wszAttributeLevel7[] = {
        wszAttrDn,
        wszAttrRole,
        NULL
    };

    PWSTR wszAttributeLevel8[] = {
        wszAttrDn,
        wszAttrCreatedTime,
        NULL
    };

    PWSTR wszAttributeLevel9[] = {
        wszAttrDn,
        NULL
    };

    PWSTR wszAttributeLevel10[] = {
        wszAttrDn,
        NULL
    };

    PWSTR wszAttributeLevel11[] = {
        wszAttrDn,
        wszAttrForceLogoffTime,
        wszAttrDomain,
        wszAttrComment,
        wszAttrRole,
        wszAttrLockoutDuration,
        wszAttrLockoutWindow,
        wszAttrLockoutThreshold,
        NULL
    };

    PWSTR wszAttributeLevel12[] = {
        wszAttrDn,
        wszAttrLockoutDuration,
        wszAttrLockoutWindow,
        wszAttrLockoutThreshold,
        NULL
    };

    PWSTR wszAttributeLevel13[] = {
        wszAttrDn,
        wszAttrCreatedTime,
        NULL
    };

    PWSTR *pwszAttributes[] = {
        wszAttributeLevel1,
        wszAttributeLevel2,
        wszAttributeLevel3,
        wszAttributeLevel4,
        wszAttributeLevel5,
        wszAttributeLevel6,
        wszAttributeLevel7,
        wszAttributeLevel8,
        wszAttributeLevel9,
        wszAttributeLevel10,
        wszAttributeLevel11,
        wszAttributeLevel12,
        wszAttributeLevel13
    };

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    PWSTR pwszDn = NULL;
    DWORD dwScope = 0;
    DWORD dwObjectClassDomain = DS_OBJECT_CLASS_DOMAIN;
    DWORD dwObjectClassBuiltin = DS_OBJECT_CLASS_BUILTIN_DOMAIN;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    DomainInfo *pInfo = NULL;

    pDomCtx  = (PDOMAIN_CONTEXT)hDomain;
    pConnCtx = pDomCtx->pConnCtx;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!(pDomCtx->dwAccessGranted & DOMAIN_ACCESS_LOOKUP_INFO_2))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pwszDn   = pDomCtx->pwszDn;
    pwszBase = pDomCtx->pwszDn;

    dwFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR)) - 1) +
                  10 +
                  ((sizeof(wszAttrObjectClass)/sizeof(WCHAR)) - 1) +
                  10 +
                  ((sizeof(wszAttrDn)/sizeof(WCHAR)) - 1) +
                  wc16slen(pwszDn) +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    dwError = LwAllocateMemory(dwFilterLen * sizeof(WCHAR),
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                    wszAttrObjectClass,
                    dwObjectClassDomain,
                    wszAttrObjectClass,
                    dwObjectClassBuiltin,
                    wszAttrDn,
                    pwszDn) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              pwszAttributes[level - 1],
                              FALSE,
                              &pEntry,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrSrvAllocateMemory(OUT_PPVOID(&pInfo),
                                     sizeof(*pInfo));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    switch (level) {
    case 1:
        ntStatus = SamrSrvFillDomainInfo1(pDomCtx,
                                          pEntry,
                                          pInfo);
        break;

    case 2:
        ntStatus = SamrSrvFillDomainInfo2(pDomCtx,
                                          pEntry,
                                          pInfo);
        break;

    case 3:
        ntStatus = SamrSrvFillDomainInfo3(pDomCtx,
                                          pEntry,
                                          pInfo);
        break;

    case 4:
        ntStatus = SamrSrvFillDomainInfo4(pDomCtx,
                                          pEntry,
                                          pInfo);
        break;

    case 5:
        ntStatus = SamrSrvFillDomainInfo5(pDomCtx,
                                          pEntry,
                                          pInfo);
        break;

    case 6:
        ntStatus = SamrSrvFillDomainInfo6(pDomCtx,
                                          pEntry,
                                          pInfo);
        break;

    case 7:
        ntStatus = SamrSrvFillDomainInfo7(pDomCtx,
                                          pEntry,
                                          pInfo);
        break;

    case 8:
        ntStatus = SamrSrvFillDomainInfo8(pDomCtx,
                                          pEntry,
                                          pInfo);
        break;

    case 9:
        ntStatus = SamrSrvFillDomainInfo9(pDomCtx,
                                          pEntry,
                                          pInfo);
        break;

    case 11:
        ntStatus = SamrSrvFillDomainInfo11(pDomCtx,
                                           pEntry,
                                           pInfo);
        break;

    case 12:
        ntStatus = SamrSrvFillDomainInfo12(pDomCtx,
                                           pEntry,
                                           pInfo);
        break;

    case 13:
        ntStatus = SamrSrvFillDomainInfo13(pDomCtx,
                                           pEntry,
                                           pInfo);
        break;

    default:
        ntStatus = STATUS_INVALID_INFO_CLASS;
        break;
    }

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppInfo = pInfo;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntry) {
        DirectoryFreeEntries(pEntry, dwEntriesNum);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pInfo) {
        SamrSrvFreeMemory(pInfo);
    }

    *ppInfo = NULL;
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo1(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrMinPasswordLen[] = DS_ATTR_MIN_PWD_LENGTH;
    WCHAR wszAttrPasswordHistoryLen[] = DS_ATTR_PWD_HISTORY_LENGTH;
    WCHAR wszAttrPasswordProperties[] = DS_ATTR_PWD_PROPERTIES;
    WCHAR wszAttrMaxPasswordAge[] = DS_ATTR_MAX_PWD_AGE;
    WCHAR wszAttrMinPasswordAge[] = DS_ATTR_MIN_PWD_AGE;
    DomainInfo1 *pInfo1 = NULL;
    ULONG ulMinPassLength = 0;
    ULONG ulPassHistoryLength = 0;
    ULONG ulPassProperties = 0;
    LONG64 llMaxPassAge = 0;
    LONG64 llMinPassAge = 0;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrMinPasswordLen,
                                               DIRECTORY_ATTR_TYPE_INTEGER,
                                               &ulMinPassLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrPasswordHistoryLen,
                                               DIRECTORY_ATTR_TYPE_INTEGER,
                                               &ulPassHistoryLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrPasswordProperties,
                                               DIRECTORY_ATTR_TYPE_INTEGER,
                                               &ulPassProperties);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrMaxPasswordAge,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llMaxPassAge);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrMinPasswordAge,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llMinPassAge);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo1 = &pInfo->info1;

    pInfo1->min_pass_length     = (USHORT)ulMinPassLength;
    pInfo1->pass_history_length = (USHORT)ulPassHistoryLength;
    pInfo1->pass_properties     = ulPassProperties;
    pInfo1->max_pass_age        = llMaxPassAge;
    pInfo1->min_pass_age        = llMinPassAge;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo2(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    const wchar_t wszFilterFmt[] = L"%ws=%u AND %ws='%ws'";

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PCONNECT_CONTEXT pConnCtx = NULL;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrComment[] = DS_ATTR_COMMENT;
    WCHAR wszAttrDomainName[] = DS_ATTR_DOMAIN;
    WCHAR wszAttrForceLogoffTime[] = DS_ATTR_FORCE_LOGOFF_TIME;
    WCHAR wszAttrRole[] = DS_ATTR_ROLE;
    PWSTR pwszComment = NULL;
    PWSTR pwszDomainName = NULL;
    LONG64 llForceLogoffTime = 0;
    ULONG ulRole = 0;
    DomainInfo2 *pInfo2 = NULL;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    size_t sDomainNameLen = 0;
    DWORD dwObjectClass = 0;
    DWORD dwFilterLen = DS_OBJECT_CLASS_UNKNOWN;
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwNumUsers = 0;
    DWORD dwNumAliases = 0;

    PWSTR wszAttributes[] = {
        wszAttrDn,
        NULL
    };

    pConnCtx       = pDomCtx->pConnCtx;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrComment,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszComment);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDomainName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrForceLogoffTime,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llForceLogoffTime);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrRole,
                                               DIRECTORY_ATTR_TYPE_INTEGER,
                                               &ulRole);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszDomainName, &sDomainNameLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR)) - 1) +
                  10 +
                  ((sizeof(wszAttrDomainName)/sizeof(WCHAR)) - 1) +
                  sDomainNameLen + 
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwFilterLen,
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Get the number of user accounts
     */
    dwObjectClass = DS_OBJECT_CLASS_USER;

    sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                wszAttrObjectClass,
                dwObjectClass,
                wszAttrDomainName,
                pwszDomainName);

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntries,
                              &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    dwNumUsers = dwNumEntries;

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
        pEntries = NULL;
    }

    /*
     * Get the number of user accounts
     */
    dwObjectClass = DS_OBJECT_CLASS_LOCAL_GROUP;

    sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                wszAttrObjectClass,
                dwObjectClass,
                wszAttrDomainName,
                pwszDomainName);

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntries,
                              &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    dwNumAliases = dwNumEntries;

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
        pEntries = NULL;
    }

    pInfo2 = &pInfo->info2;

    /* force_logoff_time */
    pInfo2->force_logoff_time = llForceLogoffTime;

    /* comment */
    ntStatus = SamrSrvInitUnicodeString(&pInfo2->comment,
                                        pwszComment);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* domain_name */
    ntStatus = SamrSrvInitUnicodeString(&pInfo2->domain_name,
                                      pwszDomainName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* primary */
    ntStatus = SamrSrvInitUnicodeString(&pInfo2->primary, NULL);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* sequence_num */
    /* unknown1 */

    /* role */
    pInfo2->role = ulRole;

    /* unknown2 */
    /* num_users */
    pInfo2->num_users = dwNumUsers;

    /* num_groups (domain member doesn't support domain groups) */
    pInfo2->num_users = 0;

    /* num_aliases */
    pInfo2->num_aliases = dwNumAliases;

cleanup:
    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo3(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrForceLogoffTime[] = DS_ATTR_FORCE_LOGOFF_TIME;
    DomainInfo3 *pInfo3 = NULL;
    LONG64 llForceLogoffTime = 0;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrForceLogoffTime,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llForceLogoffTime);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo3 = &pInfo->info3;
    pInfo3->force_logoff_time = llForceLogoffTime;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo4(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrComment[] = DS_ATTR_COMMENT;
    PWSTR pwszComment = NULL;
    DomainInfo4 *pInfo4 = NULL;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrComment,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszComment);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo4 = &pInfo->info4;

    ntStatus = SamrSrvInitUnicodeString(&pInfo4->comment,
                                      pwszComment);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo5(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrDomainName[] = DS_ATTR_DOMAIN;
    PWSTR pwszDomainName = NULL;
    DomainInfo5 *pInfo5 = NULL;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDomainName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo5 = &pInfo->info5;

    ntStatus = SamrSrvInitUnicodeString(&pInfo5->domain_name,
                                      pwszDomainName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo6(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DomainInfo6 *pInfo6 = NULL;

    pInfo6 = &pInfo->info6;

    ntStatus = SamrSrvInitUnicodeString(&pInfo6->primary,
                                      NULL);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo7(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrRole[] = DS_ATTR_ROLE;
    DomainInfo7 *pInfo7 = NULL;
    ULONG ulRole = 0;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrRole,
                                               DIRECTORY_ATTR_TYPE_INTEGER,
                                               &ulRole);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo7 = &pInfo->info7;
    pInfo7->role = ulRole;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo8(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrCreatedTime[] = DS_ATTR_CREATED_TIME;
    LONG64 llCreatedTime = 0;
    DomainInfo8 *pInfo8 = NULL;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrCreatedTime,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llCreatedTime);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo8 = &pInfo->info8;
    pInfo8->domain_create_time = llCreatedTime;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo9(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DomainInfo9 *pInfo9 = NULL;

    pInfo9 = &pInfo->info9;
    pInfo9->unknown = 0;
    
    return ntStatus;
}


static
NTSTATUS
SamrSrvFillDomainInfo11(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrLockoutDuration[] = DS_ATTR_LOCKOUT_DURATION;
    WCHAR wszAttrLockoutWindow[] = DS_ATTR_LOCKOUT_WINDOW;
    WCHAR wszAttrLockoutThreshold[] = DS_ATTR_LOCKOUT_THRESHOLD;
    DomainInfo11 *pInfo11 = NULL;
    LONG64 llLockoutDuration = 0;
    LONG64 llLockoutWindow = 0;
    LONG64 llLockoutThreshold = 0;

    pInfo11 = &pInfo->info11;

    ntStatus = SamrSrvFillDomainInfo2(pDomCtx,
                                   pEntry,
                                   (DomainInfo*)pInfo11);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrLockoutDuration,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llLockoutDuration);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrLockoutWindow,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llLockoutWindow);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrLockoutThreshold,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llLockoutThreshold);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo11->lockout_duration  = llLockoutDuration;
    pInfo11->lockout_window    = llLockoutWindow;
    pInfo11->lockout_threshold = llLockoutThreshold;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo12(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrLockoutDuration[] = DS_ATTR_LOCKOUT_DURATION;
    WCHAR wszAttrLockoutWindow[] = DS_ATTR_LOCKOUT_WINDOW;
    WCHAR wszAttrLockoutThreshold[] = DS_ATTR_LOCKOUT_THRESHOLD;
    DomainInfo12 *pInfo12 = NULL;
    LONG64 llLockoutDuration = 0;
    LONG64 llLockoutWindow = 0;
    LONG64 llLockoutThreshold = 0;

    pInfo12 = &pInfo->info12;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrLockoutDuration,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llLockoutDuration);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrLockoutWindow,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llLockoutWindow);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrLockoutThreshold,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llLockoutThreshold);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo12->lockout_duration  = llLockoutDuration;
    pInfo12->lockout_window    = llLockoutWindow;
    pInfo12->lockout_threshold = llLockoutThreshold;

cleanup:
    return ntStatus;

error:
    goto cleanup;

}


static
NTSTATUS
SamrSrvFillDomainInfo13(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrCreatedTime[] = DS_ATTR_CREATED_TIME;
    LONG64 llCreatedTime = 0;
    DomainInfo13 *pInfo13 = NULL;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrCreatedTime,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llCreatedTime);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo13 = &pInfo->info13;

    pInfo13->domain_create_time = llCreatedTime;

cleanup:
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
