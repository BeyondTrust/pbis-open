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
 *        samr_queryuserinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrQueryUserInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

#define GET_UNICODE_STRING_VALUE(attr, field)                           \
    {                                                                   \
        WCHAR wszEmpty[] = { '\0' };                                    \
        PWSTR pwszValue = NULL;                                         \
        dwError = DirectoryGetEntryAttrValueByName(                     \
                          pEntry,                                       \
                          (attr),                                       \
                          DIRECTORY_ATTR_TYPE_UNICODE_STRING,           \
                          &pwszValue);                                  \
        BAIL_ON_LSA_ERROR(dwError);                                     \
                                                                        \
        ntStatus = SamrSrvInitUnicodeString(                            \
                          &(field),                                     \
                          (pwszValue) ? pwszValue : wszEmpty);          \
        BAIL_ON_NTSTATUS_ERROR(ntStatus);                               \
    }

#define GET_NTTIME_VALUE(attr, field)                                   \
    {                                                                   \
        LONG64 llValue = 0;                                             \
        dwError = DirectoryGetEntryAttrValueByName(                     \
                          pEntry,                                       \
                          (attr),                                       \
                          DIRECTORY_ATTR_TYPE_LARGE_INTEGER,            \
                          &llValue);                                    \
        BAIL_ON_LSA_ERROR(dwError);                                     \
                                                                        \
        field = llValue;                                        \
    }

#define GET_RID_VALUE(attr, field)                                      \
    {                                                                   \
        PWSTR pwszSid = NULL;                                           \
        PSID pSid = NULL;                                               \
        dwError = DirectoryGetEntryAttrValueByName(                     \
                          pEntry,                                       \
                          (attr),                                       \
                          DIRECTORY_ATTR_TYPE_UNICODE_STRING,           \
                          &pwszSid);                                    \
        BAIL_ON_LSA_ERROR(dwError);                                     \
                                                                        \
        ntStatus = RtlAllocateSidFromWC16String(                          \
                          &pSid,                                        \
                          pwszSid);                                     \
        BAIL_ON_NTSTATUS_ERROR(ntStatus);                                 \
                                                                        \
        field = pSid->SubAuthority[pSid->SubAuthorityCount - 1];        \
                                                                        \
        RTL_FREE(&pSid);                                                \
    }

#define GET_UINT32_VALUE(attr, field)                                   \
    {                                                                   \
        ULONG ulValue = 0;                                              \
        dwError = DirectoryGetEntryAttrValueByName(                     \
                          pEntry,                                       \
                          (attr),                                       \
                          DIRECTORY_ATTR_TYPE_INTEGER,                  \
                          &ulValue);                                    \
        BAIL_ON_LSA_ERROR(dwError);                                     \
                                                                        \
        field = (UINT32)ulValue;                                        \
    }

#define GET_UINT16_VALUE(attr, field)                                   \
    {                                                                   \
        ULONG ulValue = 0;                                              \
        dwError = DirectoryGetEntryAttrValueByName(                     \
                          pEntry,                                       \
                          (attr),                                       \
                          DIRECTORY_ATTR_TYPE_INTEGER,                  \
                          &ulValue);                                    \
        BAIL_ON_LSA_ERROR(dwError);                                     \
                                                                        \
        field = (UINT16)ulValue;                                        \
    }

#define GET_LOGON_HOURS(attr, field)                                    \
    {                                                                   \
        POCTET_STRING pLogonHours = NULL;                               \
        DWORD dwLogonHoursSize = 1260;                                  \
        DWORD dwLogonHoursLen = 0;                                      \
        DWORD dwUnitsPerWeek = 0;                                       \
        DWORD iUnit = 0;                                                \
        DWORD iByte = 0;                                                \
                                                                        \
        dwError = DirectoryGetEntryAttrValueByName(                     \
                          pEntry,                                       \
                          (attr),                                       \
                          DIRECTORY_ATTR_TYPE_OCTET_STREAM,             \
                          &pLogonHours);                                \
        BAIL_ON_LSA_ERROR(dwError);                                     \
                                                                        \
        ntStatus = SamrSrvAllocateMemory(OUT_PPVOID(&(field)->units),   \
                                         dwLogonHoursSize);             \
        BAIL_ON_NTSTATUS_ERROR(ntStatus);                               \
                                                                        \
        memset((field)->units, 0, dwLogonHoursSize);                    \
                                                                        \
        /*                                                              \
         * Each byte is a unit in the blob while                        \
         * in LogonHours it is one bit                                  \
         */                                                             \
        dwUnitsPerWeek  = pLogonHours->ulNumBytes;                      \
        dwLogonHoursLen = pLogonHours->ulNumBytes / 8;                  \
                                                                        \
        for (iUnit = 0, iByte = 0;                                      \
             iUnit < dwLogonHoursLen && iByte < dwUnitsPerWeek;         \
             iUnit++, iByte += 8)                                       \
        {                                                               \
            BYTE Unit = 0;                                              \
                                                                        \
            Unit |= pLogonHours->pBytes[iByte];                         \
            Unit |= pLogonHours->pBytes[iByte+1] << 1;                  \
            Unit |= pLogonHours->pBytes[iByte+2] << 2;                  \
            Unit |= pLogonHours->pBytes[iByte+3] << 3;                  \
            Unit |= pLogonHours->pBytes[iByte+4] << 4;                  \
            Unit |= pLogonHours->pBytes[iByte+5] << 5;                  \
            Unit |= pLogonHours->pBytes[iByte+6] << 6;                  \
            Unit |= pLogonHours->pBytes[iByte+7] << 7;                  \
                                                                        \
            (field)->units[iUnit] = Unit;                               \
        }                                                               \
                                                                        \
        (field)->units_per_week = dwUnitsPerWeek;                       \
    }



static
NTSTATUS
SamrFillUserInfo1(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo2(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo3(
    PACCOUNT_CONTEXT pAcctCtx,
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo4(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo5(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo6(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo7(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo8(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo9(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo10(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo11(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo12(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo13(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo14(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo16(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo17(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo20(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo21(
    PACCOUNT_CONTEXT pAcctCtx,
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


NTSTATUS
SamrSrvQueryUserInfo(
    /* [in] */ handle_t hBinding,
    /* [in] */ ACCOUNT_HANDLE hUser,
    /* [in] */ UINT16 level,
    /* [out] */ UserInfo **info
    )
{
    wchar_t wszFilterFmt[] = L"%ws='%ws'";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    WCHAR wszAttrPrimaryGroup[] = DS_ATTR_PRIMARY_GROUP;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    WCHAR wszAttrComment[] = DS_ATTR_COMMENT;
    WCHAR wszAttrCountryCode[] = DS_ATTR_COUNTRY_CODE;
    WCHAR wszAttrCodePage[] = DS_ATTR_CODE_PAGE;
    WCHAR wszAttrHomeDirectory[] = DS_ATTR_HOME_DIR;
    WCHAR wszAttrHomeDrive[] = DS_ATTR_HOME_DRIVE;
    WCHAR wszAttrLogonScript[] = DS_ATTR_LOGON_SCRIPT;
    WCHAR wszAttrProfilePath[] = DS_ATTR_PROFILE_PATH;
    WCHAR wszAttrWorkstations[]= DS_ATTR_WORKSTATIONS;
    WCHAR wszAttrParameters[] = DS_ATTR_PARAMETERS;
    WCHAR wszAttrLastLogon[] = DS_ATTR_LAST_LOGON;
    WCHAR wszAttrLastLogoff[] = DS_ATTR_LAST_LOGOFF;
    WCHAR wszAttrPasswordLastSet[] = DS_ATTR_PASSWORD_LAST_SET;
    WCHAR wszAttrAllowPasswordChange[] = DS_ATTR_ALLOW_PASSWORD_CHANGE;
    WCHAR wszAttrForcePasswordChange[] = DS_ATTR_FORCE_PASSWORD_CHANGE;
    WCHAR wszAttrLogonHours[] = DS_ATTR_LOGON_HOURS;
    WCHAR wszAttrBadPasswordCount[] = DS_ATTR_BAD_PASSWORD_COUNT;
    WCHAR wszAttrLogonCount[] = DS_ATTR_LOGON_COUNT;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrAccountExpiry[] = DS_ATTR_ACCOUNT_EXPIRY;
    WCHAR wszAttrLmHash[] = DS_ATTR_LM_HASH;
    WCHAR wszAttrNtHash[] = DS_ATTR_NT_HASH;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    UserInfo *pUserInfo = NULL;

    PWSTR wszAttributesLevel1[] = {
        wszAttrSamAccountName,
        wszAttrFullName,
        wszAttrPrimaryGroup,
        wszAttrDescription,
        wszAttrComment,
        NULL
    };

    PWSTR wszAttributesLevel2[] = {
        wszAttrComment,
        wszAttrCountryCode,
        wszAttrCodePage,
        NULL
    };

    PWSTR wszAttributesLevel3[] = {
        wszAttrSamAccountName,
        wszAttrFullName,
        wszAttrObjectSid,
        wszAttrPrimaryGroup,
        wszAttrHomeDirectory,
        wszAttrHomeDrive,
        wszAttrLogonScript,
        wszAttrProfilePath,
        wszAttrWorkstations,
        wszAttrLastLogon,
        wszAttrLastLogoff,
        wszAttrPasswordLastSet,
        wszAttrAllowPasswordChange,
        wszAttrForcePasswordChange,
        wszAttrLogonHours,
        wszAttrBadPasswordCount,
        wszAttrLogonCount,
        wszAttrAccountFlags,
        NULL
    };

    PWSTR wszAttributesLevel4[] = {
        wszAttrLogonHours,
        NULL
    };

    PWSTR wszAttributesLevel5[] = {
        wszAttrSamAccountName,
        wszAttrFullName,
        wszAttrObjectSid,
        wszAttrPrimaryGroup,
        wszAttrHomeDirectory,
        wszAttrHomeDrive,
        wszAttrLogonScript,
        wszAttrProfilePath,
        wszAttrDescription,
        wszAttrWorkstations,
        wszAttrLastLogon,
        wszAttrLastLogoff,
        wszAttrLogonHours,
        wszAttrBadPasswordCount,
        wszAttrLogonCount,
        wszAttrPasswordLastSet,
        wszAttrAccountExpiry,
        wszAttrAccountFlags,
        NULL
    };

    PWSTR wszAttributesLevel6[] = {
        wszAttrSamAccountName,
        wszAttrFullName,
        NULL
    };

    PWSTR wszAttributesLevel7[] = {
        wszAttrSamAccountName,
        NULL
    };

    PWSTR wszAttributesLevel8[] = {
        wszAttrFullName,
        NULL
    };

    PWSTR wszAttributesLevel9[] = {
        wszAttrPrimaryGroup,
        NULL
    };

    PWSTR wszAttributesLevel10[] = {
        wszAttrHomeDirectory,
        wszAttrHomeDrive,
        NULL
    };

    PWSTR wszAttributesLevel11[] = {
        wszAttrLogonScript,
        NULL
    };

    PWSTR wszAttributesLevel12[] = {
        wszAttrProfilePath,
        NULL
    };

    PWSTR wszAttributesLevel13[] = {
        wszAttrDescription,
        NULL
    };

    PWSTR wszAttributesLevel14[] = {
        wszAttrWorkstations,
        NULL
    };

    PWSTR wszAttributesLevel16[] = {
        wszAttrAccountFlags,
        NULL
    };

    PWSTR wszAttributesLevel17[] = {
        wszAttrAccountExpiry,
        NULL
    };

    PWSTR wszAttributesLevel20[] = {
        wszAttrParameters,
        NULL
    };

    PWSTR wszAttributesLevel21[] = {
        wszAttrLastLogon,
        wszAttrLastLogoff,
        wszAttrPasswordLastSet,
        wszAttrAccountExpiry,
        wszAttrAllowPasswordChange,
        wszAttrForcePasswordChange,
        wszAttrSamAccountName,
        wszAttrFullName,
        wszAttrHomeDirectory,
        wszAttrHomeDrive,
        wszAttrLogonScript,
        wszAttrProfilePath,
        wszAttrDescription,
        wszAttrWorkstations,
        wszAttrComment,
        wszAttrParameters,
        wszAttrObjectSid,
        wszAttrPrimaryGroup,
        wszAttrAccountFlags,
        wszAttrLogonHours,
        wszAttrBadPasswordCount,
        wszAttrLogonCount,
        wszAttrCountryCode,
        wszAttrCodePage,
        wszAttrLmHash,
        wszAttrNtHash,
        NULL
    };

    PWSTR *pwszAttributes[] = {
        wszAttributesLevel1,
        wszAttributesLevel2,
        wszAttributesLevel3,
        wszAttributesLevel4,
        wszAttributesLevel5,
        wszAttributesLevel6,
        wszAttributesLevel7,
        wszAttributesLevel8,
        wszAttributesLevel9,
        wszAttributesLevel10,
        wszAttributesLevel11,
        wszAttributesLevel12,
        wszAttributesLevel13,
        wszAttributesLevel14,
        NULL,
        wszAttributesLevel16,
        wszAttributesLevel17,
        NULL,
        NULL,
        wszAttributesLevel20,
        wszAttributesLevel21
    };

    pAcctCtx = (PACCOUNT_CONTEXT)hUser;

    if (pAcctCtx == NULL || pAcctCtx->Type != SamrContextAccount)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    switch (level)
    {
    case 15:
    case 18:
    case 19:
        ntStatus = STATUS_INVALID_INFO_CLASS;
        break;

    default:
        ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pDomCtx  = pAcctCtx->pDomCtx;
    pConnCtx = pDomCtx->pConnCtx;

    pwszBase = pDomCtx->pwszDn;

    dwFilterLen = ((sizeof(wszAttrDn)/sizeof(WCHAR)) - 1) +
                  wc16slen(pAcctCtx->pwszDn) + 
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    ntStatus = SamrSrvAllocateMemory((void**)&pwszFilter,
                                   dwFilterLen * sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                wszAttrDn, pAcctCtx->pwszDn);

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              pwszAttributes[level - 1],
                              FALSE,
                              &pEntry,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwEntriesNum == 0)
    {
        ntStatus = STATUS_INVALID_HANDLE;

    }
    else if (dwEntriesNum > 1)
    {
        ntStatus = STATUS_INTERNAL_ERROR;
    }

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = SamrSrvAllocateMemory((void**)&pUserInfo,
                                     sizeof(*pUserInfo));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    switch (level)
    {
    case 1:
    case 6:
    case 7:
    case 8:
    case 13:
        if (!(pAcctCtx->dwAccessGranted & USER_ACCESS_GET_NAME_ETC) ||
            !(pAcctCtx->dwAccessGranted & USER_ACCESS_GET_ATTRIBUTES))
        {
            ntStatus = STATUS_ACCESS_DENIED;
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }
        break;

    case 2:
        if (!(pAcctCtx->dwAccessGranted & USER_ACCESS_GET_LOCALE) ||
            !(pAcctCtx->dwAccessGranted & USER_ACCESS_GET_ATTRIBUTES))
        {
            ntStatus = STATUS_ACCESS_DENIED;
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }
        break;

    case 4:
    case 5:
    case 10:
    case 11:
    case 12:
    case 14:
    case 17:
        if (!(pAcctCtx->dwAccessGranted & USER_ACCESS_GET_LOGONINFO) ||
            !(pAcctCtx->dwAccessGranted & USER_ACCESS_GET_ATTRIBUTES))
        {
            ntStatus = STATUS_ACCESS_DENIED;
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }
        break;

    case 3:
    case 9:
    case 16:
    case 20:
    case 21:
        if (!(pAcctCtx->dwAccessGranted & USER_ACCESS_GET_ATTRIBUTES))
        {
            ntStatus = STATUS_ACCESS_DENIED;
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }
        break;
    }

    switch (level) {
    case 1:
        ntStatus = SamrFillUserInfo1(pEntry, pUserInfo);
        break;

    case 2:
        ntStatus = SamrFillUserInfo2(pEntry, pUserInfo);
        break;

    case 3:
        ntStatus = SamrFillUserInfo3(pAcctCtx, pEntry, pUserInfo);
        break;

    case 4:
        ntStatus = SamrFillUserInfo4(pEntry, pUserInfo);
        break;

    case 5:
        ntStatus = SamrFillUserInfo5(pEntry, pUserInfo);
        break;

    case 6:
        ntStatus = SamrFillUserInfo6(pEntry, pUserInfo);
        break;

    case 7:
        ntStatus = SamrFillUserInfo7(pEntry, pUserInfo);
        break;

    case 8:
        ntStatus = SamrFillUserInfo8(pEntry, pUserInfo);
        break;

    case 9:
        ntStatus = SamrFillUserInfo9(pEntry, pUserInfo);
        break;

    case 10:
        ntStatus = SamrFillUserInfo10(pEntry, pUserInfo);
        break;

    case 11:
        ntStatus = SamrFillUserInfo11(pEntry, pUserInfo);
        break;

    case 12:
        ntStatus = SamrFillUserInfo12(pEntry, pUserInfo);
        break;

    case 13:
        ntStatus = SamrFillUserInfo13(pEntry, pUserInfo);
        break;

    case 14:
        ntStatus = SamrFillUserInfo14(pEntry, pUserInfo);
        break;

    case 16:
        ntStatus = SamrFillUserInfo16(pEntry, pUserInfo);
        break;

    case 17:
        ntStatus = SamrFillUserInfo17(pEntry, pUserInfo);
        break;

    case 20:
        ntStatus = SamrFillUserInfo20(pEntry, pUserInfo);
        break;

    case 21:
        ntStatus = SamrFillUserInfo21(pAcctCtx, pEntry, pUserInfo);
        break;

    default:
        ntStatus = STATUS_INVALID_INFO_CLASS;
    }
    
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *info = pUserInfo;
    
cleanup:
    if (pwszFilter) {
        SamrSrvFreeMemory(pwszFilter);
    }

    if (pEntry) {
        DirectoryFreeEntries(pEntry, dwEntriesNum);
    }

    return ntStatus;

error:
    if (pUserInfo) {
        SamrSrvFreeMemory(pUserInfo);
    }

    *info = NULL;
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo1(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo1 *pInfo1 = NULL;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    WCHAR wszAttrPrimaryGid[] = DS_ATTR_PRIMARY_GROUP;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    WCHAR wszAttrComment[] = DS_ATTR_COMMENT;

    pInfo1 = &(pInfo->info1);

    GET_UNICODE_STRING_VALUE(wszAttrSamAccountName, pInfo1->account_name);
    GET_UNICODE_STRING_VALUE(wszAttrFullName, pInfo1->full_name);
    GET_UINT32_VALUE(wszAttrPrimaryGid, pInfo1->primary_gid);
    GET_UNICODE_STRING_VALUE(wszAttrDescription, pInfo1->description);
    GET_UNICODE_STRING_VALUE(wszAttrComment, pInfo1->comment);

cleanup:
    return ntStatus;

error:
    memset(pInfo1, 0, sizeof(*pInfo1));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo2(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo2 *pInfo2 = NULL;
    WCHAR wszAttrComment[] = DS_ATTR_COMMENT;
    WCHAR wszAttrCountryCode[] = DS_ATTR_COUNTRY_CODE;
    WCHAR wszAttrCodePage[] = DS_ATTR_CODE_PAGE;

    pInfo2 = &(pInfo->info2);

    GET_UNICODE_STRING_VALUE(wszAttrComment, pInfo2->comment);

    /* unknown1 */

    GET_UINT16_VALUE(wszAttrCountryCode, pInfo2->country_code);
    GET_UINT16_VALUE(wszAttrCodePage, pInfo2->code_page);

cleanup:
    return ntStatus;

error:
    memset(pInfo2, 0, sizeof(*pInfo2));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo3(
    PACCOUNT_CONTEXT pAcctCtx,
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo3 *pInfo3 = NULL;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    WCHAR wszAttrPrimaryGid[] = DS_ATTR_PRIMARY_GROUP;
    WCHAR wszAttrHomeDirectory[] = DS_ATTR_HOME_DIR;
    WCHAR wszAttrHomeDrive[] = DS_ATTR_HOME_DRIVE;
    WCHAR wszAttrLogonScript[] = DS_ATTR_LOGON_SCRIPT;
    WCHAR wszAttrProfilePath[] = DS_ATTR_PROFILE_PATH;
    WCHAR wszAttrWorkstations[] = DS_ATTR_WORKSTATIONS;
    WCHAR wszAttrLastLogon[] = DS_ATTR_LAST_LOGON;
    WCHAR wszAttrLastLogoff[] = DS_ATTR_LAST_LOGOFF;
    WCHAR wszAttrLastPasswordChange[] = DS_ATTR_PASSWORD_LAST_SET;
    WCHAR wszAttrLogonHours[] = DS_ATTR_LOGON_HOURS;
    WCHAR wszAttrBadPasswordCount[] = DS_ATTR_BAD_PASSWORD_COUNT;
    WCHAR wszAttrLogonCount[] = DS_ATTR_LOGON_COUNT;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    LONG64 ntMinPasswordAge = pAcctCtx->pDomCtx->ntMinPasswordAge;
    LONG64 ntMaxPasswordAge = pAcctCtx->pDomCtx->ntMinPasswordAge;

    pInfo3 = &(pInfo->info3);

    GET_UNICODE_STRING_VALUE(wszAttrSamAccountName, pInfo3->account_name);
    GET_UNICODE_STRING_VALUE(wszAttrFullName, pInfo3->full_name);
    GET_RID_VALUE(wszAttrObjectSid, pInfo3->rid);
    GET_UINT32_VALUE(wszAttrPrimaryGid, pInfo3->primary_gid);
    GET_UNICODE_STRING_VALUE(wszAttrHomeDirectory, pInfo3->home_directory);
    GET_UNICODE_STRING_VALUE(wszAttrHomeDrive, pInfo3->home_drive);
    GET_UNICODE_STRING_VALUE(wszAttrLogonScript, pInfo3->logon_script);
    GET_UNICODE_STRING_VALUE(wszAttrProfilePath, pInfo3->profile_path);
    GET_UNICODE_STRING_VALUE(wszAttrWorkstations, pInfo3->workstations);
    GET_NTTIME_VALUE(wszAttrLastLogon, pInfo3->last_logon);
    GET_NTTIME_VALUE(wszAttrLastLogoff, pInfo3->last_logoff);
    GET_NTTIME_VALUE(wszAttrLastPasswordChange, pInfo3->last_password_change);
    pInfo3->allow_password_change = pInfo3->last_password_change +
                                    ntMinPasswordAge;
    pInfo3->force_password_change = pInfo3->last_password_change +
                                    ntMaxPasswordAge;
    GET_LOGON_HOURS(wszAttrLogonHours, &pInfo3->logon_hours);
    GET_UINT16_VALUE(wszAttrBadPasswordCount, pInfo3->bad_password_count);
    GET_UINT16_VALUE(wszAttrLogonCount, pInfo3->logon_count);
    GET_UINT32_VALUE(wszAttrAccountFlags, pInfo3->account_flags);

cleanup:
    return ntStatus;

error:
    memset(pInfo3, 0, sizeof(*pInfo3));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo4(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo4 *pInfo4 = NULL;
    WCHAR wszAttrLogonHours[] = DS_ATTR_LOGON_HOURS;

    pInfo4 = &(pInfo->info4);

    GET_LOGON_HOURS(wszAttrLogonHours, &pInfo4->logon_hours);

cleanup:
    return ntStatus;

error:
    memset(pInfo4, 0, sizeof(*pInfo4));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo5(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo5 *pInfo5 = NULL;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    WCHAR wszAttrPrimaryGid[] = DS_ATTR_PRIMARY_GROUP;
    WCHAR wszAttrHomeDirectory[] = DS_ATTR_HOME_DIR;
    WCHAR wszAttrHomeDrive[] = DS_ATTR_HOME_DRIVE;
    WCHAR wszAttrLogonScript[] = DS_ATTR_LOGON_SCRIPT;
    WCHAR wszAttrProfilePath[] = DS_ATTR_PROFILE_PATH;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    WCHAR wszAttrWorkstations[] = DS_ATTR_WORKSTATIONS;
    WCHAR wszAttrLastLogon[] = DS_ATTR_LAST_LOGON;
    WCHAR wszAttrLastLogoff[] = DS_ATTR_LAST_LOGOFF;
    WCHAR wszAttrLogonHours[] = DS_ATTR_LOGON_HOURS;
    WCHAR wszAttrBadPasswordCount[] = DS_ATTR_BAD_PASSWORD_COUNT;
    WCHAR wszAttrLogonCount[] = DS_ATTR_LOGON_COUNT;
    WCHAR wszAttrLastPasswordChange[] = DS_ATTR_PASSWORD_LAST_SET;
    WCHAR wszAttrAccountExpiry[] = DS_ATTR_ACCOUNT_EXPIRY;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;

    pInfo5 = &(pInfo->info5);

    GET_UNICODE_STRING_VALUE(wszAttrSamAccountName, pInfo5->account_name);
    GET_UNICODE_STRING_VALUE(wszAttrFullName, pInfo5->full_name);
    GET_RID_VALUE(wszAttrObjectSid, pInfo5->rid);
    GET_UINT32_VALUE(wszAttrPrimaryGid, pInfo5->primary_gid);
    GET_UNICODE_STRING_VALUE(wszAttrHomeDirectory, pInfo5->home_directory);
    GET_UNICODE_STRING_VALUE(wszAttrHomeDrive, pInfo5->home_drive);
    GET_UNICODE_STRING_VALUE(wszAttrLogonScript, pInfo5->logon_script);
    GET_UNICODE_STRING_VALUE(wszAttrProfilePath, pInfo5->profile_path);
    GET_UNICODE_STRING_VALUE(wszAttrDescription, pInfo5->description);
    GET_UNICODE_STRING_VALUE(wszAttrWorkstations, pInfo5->workstations);
    GET_NTTIME_VALUE(wszAttrLastLogon, pInfo5->last_logon);
    GET_NTTIME_VALUE(wszAttrLastLogoff, pInfo5->last_logoff);
    GET_LOGON_HOURS(wszAttrLogonHours, &pInfo5->logon_hours);
    GET_UINT16_VALUE(wszAttrBadPasswordCount, pInfo5->bad_password_count);
    GET_UINT16_VALUE(wszAttrLogonCount, pInfo5->logon_count);
    GET_NTTIME_VALUE(wszAttrLastPasswordChange, pInfo5->last_password_change);
    GET_NTTIME_VALUE(wszAttrAccountExpiry, pInfo5->account_expiry);
    GET_UINT32_VALUE(wszAttrAccountFlags, pInfo5->account_flags);

cleanup:
    return ntStatus;

error:
    memset(pInfo5, 0, sizeof(*pInfo5));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo6(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo6 *pInfo6 = NULL;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;

    pInfo6 = &(pInfo->info6);

    GET_UNICODE_STRING_VALUE(wszAttrSamAccountName, pInfo6->account_name);
    GET_UNICODE_STRING_VALUE(wszAttrFullName, pInfo6->full_name);

cleanup:
    return ntStatus;

error:
    memset(pInfo6, 0, sizeof(*pInfo6));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo7(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo7 *pInfo7 = NULL;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;

    pInfo7 = &(pInfo->info7);

    GET_UNICODE_STRING_VALUE(wszAttrSamAccountName, pInfo7->account_name);

cleanup:
    return ntStatus;

error:
    memset(pInfo7, 0, sizeof(*pInfo7));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo8(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo8 *pInfo8 = NULL;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;

    pInfo8 = &(pInfo->info8);

    GET_UNICODE_STRING_VALUE(wszAttrFullName, pInfo8->full_name);

cleanup:
    return ntStatus;

error:
    memset(pInfo8, 0, sizeof(*pInfo8));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo9(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo9 *pInfo9 = NULL;
    WCHAR wszAttrPrimaryGid[] = DS_ATTR_PRIMARY_GROUP;

    pInfo9 = &(pInfo->info9);

    GET_UINT32_VALUE(wszAttrPrimaryGid, pInfo9->primary_gid);

cleanup:
    return ntStatus;

error:
    memset(pInfo9, 0, sizeof(*pInfo9));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo10(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo10 *pInfo10 = NULL;
    WCHAR wszAttrHomeDirectory[] = DS_ATTR_HOME_DIR;
    WCHAR wszAttrHomeDrive[] = DS_ATTR_HOME_DRIVE;

    pInfo10 = &(pInfo->info10);

    GET_UNICODE_STRING_VALUE(wszAttrHomeDirectory, pInfo10->home_directory);
    GET_UNICODE_STRING_VALUE(wszAttrHomeDrive, pInfo10->home_drive);

cleanup:
    return ntStatus;

error:
    memset(pInfo10, 0, sizeof(*pInfo10));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo11(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo11 *pInfo11 = NULL;
    WCHAR wszAttrLogonScript[] = DS_ATTR_LOGON_SCRIPT;

    pInfo11 = &(pInfo->info11);

    GET_UNICODE_STRING_VALUE(wszAttrLogonScript, pInfo11->logon_script);

cleanup:
    return ntStatus;

error:
    memset(pInfo11, 0, sizeof(*pInfo11));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo12(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo12 *pInfo12 = NULL;
    WCHAR wszAttrProfilePath[] = DS_ATTR_PROFILE_PATH;

    pInfo12 = &(pInfo->info12);

    GET_UNICODE_STRING_VALUE(wszAttrProfilePath, pInfo12->profile_path);

cleanup:
    return ntStatus;

error:
    memset(pInfo12, 0, sizeof(*pInfo12));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo13(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo13 *pInfo13 = NULL;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;

    pInfo13 = &(pInfo->info13);

    GET_UNICODE_STRING_VALUE(wszAttrDescription, pInfo13->description);

cleanup:
    return ntStatus;

error:
    memset(pInfo13, 0, sizeof(*pInfo13));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo14(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo14 *pInfo14 = NULL;
    WCHAR wszAttrWorkstations[] = DS_ATTR_WORKSTATIONS;

    pInfo14 = &(pInfo->info14);

    GET_UNICODE_STRING_VALUE(wszAttrWorkstations, pInfo14->workstations);

cleanup:
    return ntStatus;

error:
    memset(pInfo14, 0, sizeof(*pInfo14));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo16(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo16 *pInfo16 = NULL;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;

    pInfo16 = &(pInfo->info16);

    GET_UINT32_VALUE(wszAttrAccountFlags, pInfo16->account_flags);

cleanup:
    return ntStatus;

error:
    memset(pInfo16, 0, sizeof(*pInfo16));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo17(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo17 *pInfo17 = NULL;
    WCHAR wszAttrAccountExpiry[] = DS_ATTR_ACCOUNT_EXPIRY;

    pInfo17 = &(pInfo->info17);

    GET_NTTIME_VALUE(wszAttrAccountExpiry, pInfo17->account_expiry);

cleanup:
    return ntStatus;

error:
    memset(pInfo17, 0, sizeof(*pInfo17));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo20(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo20 *pInfo20 = NULL;
    WCHAR wszAttrParameters[] = DS_ATTR_PARAMETERS;

    pInfo20 = &(pInfo->info20);

    GET_UNICODE_STRING_VALUE(wszAttrParameters, pInfo20->parameters);

cleanup:
    return ntStatus;

error:
    memset(pInfo20, 0, sizeof(*pInfo20));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo21(
    PACCOUNT_CONTEXT pAcctCtx,
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo21 *pInfo21 = NULL;
    WCHAR wszAttrLastLogon[] = DS_ATTR_LAST_LOGON;
    WCHAR wszAttrLastLogoff[] = DS_ATTR_LAST_LOGOFF;
    WCHAR wszAttrLastPasswordChange[] = DS_ATTR_PASSWORD_LAST_SET;
    WCHAR wszAttrAccountExpiry[] = DS_ATTR_ACCOUNT_EXPIRY;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    WCHAR wszAttrHomeDirectory[] = DS_ATTR_HOME_DIR;
    WCHAR wszAttrHomeDrive[] = DS_ATTR_HOME_DRIVE;
    WCHAR wszAttrLogonScript[] = DS_ATTR_LOGON_SCRIPT;
    WCHAR wszAttrProfilePath[] = DS_ATTR_PROFILE_PATH;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    WCHAR wszAttrWorkstations[] = DS_ATTR_WORKSTATIONS;
    WCHAR wszAttrComment[] = DS_ATTR_COMMENT;
    WCHAR wszAttrParameters[] = DS_ATTR_PARAMETERS;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrPrimaryGid[] = DS_ATTR_PRIMARY_GROUP;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrLogonHours[] = DS_ATTR_LOGON_HOURS;
    WCHAR wszAttrBadPasswordCount[] = DS_ATTR_BAD_PASSWORD_COUNT;
    WCHAR wszAttrLogonCount[] = DS_ATTR_LOGON_COUNT;
    WCHAR wszAttrCountryCode[] = DS_ATTR_COUNTRY_CODE;
    WCHAR wszAttrCodePage[] = DS_ATTR_CODE_PAGE;
    LONG64 ntMinPasswordAge = pAcctCtx->pDomCtx->ntMinPasswordAge;
    LONG64 ntMaxPasswordAge = pAcctCtx->pDomCtx->ntMaxPasswordAge;
    LONG64 ntCurrentTime = 0;

    pInfo21 = &(pInfo->info21);

    GET_NTTIME_VALUE(wszAttrLastLogon, pInfo21->last_logon);
    pInfo21->fields_present |= SAMR_FIELD_LAST_LOGON;

    GET_NTTIME_VALUE(wszAttrLastLogoff, pInfo21->last_logoff);
    pInfo21->fields_present |= SAMR_FIELD_LAST_LOGOFF;

    GET_NTTIME_VALUE(wszAttrLastPasswordChange, pInfo21->last_password_change);
    pInfo21->fields_present |= SAMR_FIELD_LAST_PWD_CHANGE;

    GET_NTTIME_VALUE(wszAttrAccountExpiry, pInfo21->account_expiry);
    pInfo21->fields_present |= SAMR_FIELD_ACCT_EXPIRY;

    pInfo21->allow_password_change = pInfo21->last_password_change +
                                     ntMinPasswordAge;
    pInfo21->fields_present |= SAMR_FIELD_ALLOW_PWD_CHANGE;

    pInfo21->force_password_change = pInfo21->last_password_change +
                                     ntMaxPasswordAge;
    pInfo21->fields_present |= SAMR_FIELD_FORCE_PWD_CHANGE;

    GET_UNICODE_STRING_VALUE(wszAttrSamAccountName, pInfo21->account_name);
    pInfo21->fields_present |= SAMR_FIELD_ACCOUNT_NAME;

    GET_UNICODE_STRING_VALUE(wszAttrFullName, pInfo21->full_name);
    pInfo21->fields_present |= SAMR_FIELD_FULL_NAME;

    GET_UNICODE_STRING_VALUE(wszAttrHomeDirectory, pInfo21->home_directory);
    pInfo21->fields_present |= SAMR_FIELD_HOME_DIRECTORY;

    GET_UNICODE_STRING_VALUE(wszAttrHomeDrive, pInfo21->home_drive);
    pInfo21->fields_present |= SAMR_FIELD_HOME_DRIVE;
    
    GET_UNICODE_STRING_VALUE(wszAttrLogonScript, pInfo21->logon_script);
    pInfo21->fields_present |= SAMR_FIELD_LOGON_SCRIPT;
    
    GET_UNICODE_STRING_VALUE(wszAttrProfilePath, pInfo21->profile_path);
    pInfo21->fields_present |= SAMR_FIELD_PROFILE_PATH;

    GET_UNICODE_STRING_VALUE(wszAttrDescription, pInfo21->description);
    pInfo21->fields_present |= SAMR_FIELD_DESCRIPTION;

    GET_UNICODE_STRING_VALUE(wszAttrWorkstations, pInfo21->workstations);
    pInfo21->fields_present |= SAMR_FIELD_WORKSTATIONS;

    GET_UNICODE_STRING_VALUE(wszAttrComment, pInfo21->comment);
    pInfo21->fields_present |= SAMR_FIELD_COMMENT;

    GET_UNICODE_STRING_VALUE(wszAttrParameters, pInfo21->parameters);
    pInfo21->fields_present |= SAMR_FIELD_PARAMETERS;

    /* unknown1 */
    /* unknown2 */
    /* unknown3 */

    GET_RID_VALUE(wszAttrObjectSid, pInfo21->rid);
    pInfo21->fields_present |= SAMR_FIELD_RID;

    GET_UINT32_VALUE(wszAttrPrimaryGid, pInfo21->primary_gid);
    pInfo21->fields_present |= SAMR_FIELD_PRIMARY_GID;

    GET_UINT32_VALUE(wszAttrAccountFlags, pInfo21->account_flags);
    pInfo21->fields_present |= SAMR_FIELD_ACCT_FLAGS;

    GET_LOGON_HOURS(wszAttrLogonHours, &pInfo21->logon_hours);
    pInfo21->fields_present |= SAMR_FIELD_LOGON_HOURS;

    GET_UINT16_VALUE(wszAttrBadPasswordCount, pInfo21->bad_password_count);
    pInfo21->fields_present |= SAMR_FIELD_BAD_PWD_COUNT;

    GET_UINT16_VALUE(wszAttrLogonCount, pInfo21->logon_count);
    pInfo21->fields_present |= SAMR_FIELD_NUM_LOGONS;

    GET_UINT16_VALUE(wszAttrCountryCode, pInfo21->country_code);
    pInfo21->fields_present |= SAMR_FIELD_COUNTRY_CODE;

    GET_UINT16_VALUE(wszAttrCodePage, pInfo21->code_page);
    pInfo21->fields_present |= SAMR_FIELD_CODE_PAGE;

    dwError = LwGetNtTime((PULONG64)&ntCurrentTime);
    BAIL_ON_LSA_ERROR(dwError);

    if (ntCurrentTime >= pInfo21->last_password_change + ntMaxPasswordAge)
    {
        pInfo21->password_expired = 1;
    }

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    memset(pInfo21, 0, sizeof(*pInfo21));
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
