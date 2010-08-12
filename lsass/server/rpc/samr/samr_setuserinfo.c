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
 *        samr_setuserinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrSrvSetUserInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
SamrSrvCheckPasswordPolicy(
    IN  PACCOUNT_CONTEXT pAcctCtx,
    IN  PWSTR            pwszPassword
    );


NTSTATUS
SamrSrvSetUserInfo(
    IN  handle_t        hBinding,
    IN  ACCOUNT_HANDLE  hUser,
    IN  UINT16          usLevel,
    IN  UserInfo       *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = SamrSrvSetUserInfoInternal(hBinding,
                                          hUser,
                                          usLevel,
                                          pInfo);
    return ntStatus;
}


#define SET_UNICODE_STRING_VALUE(var, idx, mod)                   \
    do {                                                          \
        PWSTR pwszValue = NULL;                                   \
                                                                  \
        ntStatus = SamrSrvGetFromUnicodeString(&pwszValue,        \
                                               &(var));           \
        BAIL_ON_NTSTATUS_ERROR(ntStatus);                         \
                                                                  \
        AttrValues[(idx)].data.pwszStringValue = pwszValue;       \
        Mods[i++] = mod;                                          \
    } while (0);

#define SET_UINT32_VALUE(var, idx, mod)                           \
    do {                                                          \
        DWORD dwValue = (var);                                    \
                                                                  \
        AttrValues[(idx)].data.ulValue = dwValue;                 \
        Mods[i++] = mod;                                          \
    } while (0);

#define SET_NTTIME_VALUE(var, idx, mod)                           \
    do {                                                          \
        ULONG64 llValue = (var);                                  \
                                                                  \
        AttrValues[(idx)].data.llValue = llValue;                 \
        Mods[i++] = mod;                                          \
    } while (0);

#define SET_BLOB_VALUE(var, len, idx, mod)                        \
    do {                                                          \
        POCTET_STRING pBlob = NULL;                               \
                                                                  \
        dwError = LwAllocateMemory(sizeof(*pBlob),                \
                                   OUT_PPVOID(&pBlob));           \
        BAIL_ON_LSA_ERROR(dwError);                               \
                                                                  \
        pBlob->ulNumBytes = (len);                                \
        pBlob->pBytes     = (var);                                \
                                                                  \
        AttrValues[(idx)].data.pOctetString = pBlob;              \
        Mods[i++] = (mod);                                        \
    } while (0);

#define TEST_ACCOUNT_FIELD_FLAG(field, flag)                      \
    if (!((field) & (flag)))                                      \
    {                                                             \
        break;                                                    \
    }

#define SET_UNICODE_STRING_VALUE_BY_FLAG(pinfo, field, flag,      \
                                         idx, mod)                \
    do {                                                          \
        TEST_ACCOUNT_FIELD_FLAG((pinfo)->fields_present,          \
                                (flag));                          \
        SET_UNICODE_STRING_VALUE((pinfo)->field,                  \
                                 (idx), (mod));                   \
    } while (0);

#define SET_UINT32_VALUE_BY_FLAG(pinfo, field, flag, idx, mod)    \
    do {                                                          \
        TEST_ACCOUNT_FIELD_FLAG((pinfo)->fields_present,          \
                                (flag));                          \
        SET_UINT32_VALUE((pinfo)->field, (idx), (mod));           \
    } while (0);

#define SET_NTTIME_VALUE_BY_FLAG(pinfo, field, flag, idx, mod)    \
    do {                                                          \
        TEST_ACCOUNT_FIELD_FLAG((pinfo)->fields_present,          \
                                (flag));                          \
        SET_NTTIME_VALUE((pinfo)->field, (idx), (mod));           \
    } while (0);

#define EXPIRE_PASSWORD_BY_FLAG(pinfo, field, flag, idx, mod)     \
    do {                                                          \
        TEST_ACCOUNT_FIELD_FLAG((pinfo)->fields_present,          \
                                (flag));                          \
        if ((pinfo)->field)                                       \
        {                                                         \
            SET_NTTIME_VALUE(0, (idx), (mod));                    \
        }                                                         \
    } while (0);
                               


NTSTATUS
SamrSrvSetUserInfoInternal(
    IN  handle_t        hBinding,
    IN  ACCOUNT_HANDLE  hUser,
    IN  UINT16          level,
    IN  UserInfo       *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    HANDLE hDirectory = NULL;
    PWSTR pwszAccountDn = NULL;
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
    WCHAR wszAttrLastPasswordChange[] = DS_ATTR_PASSWORD_LAST_SET;
    WCHAR wszAttrAllowPasswordChange[] = DS_ATTR_ALLOW_PASSWORD_CHANGE;
    WCHAR wszAttrForcePasswordChange[] = DS_ATTR_FORCE_PASSWORD_CHANGE;
    WCHAR wszAttrBadPasswordCount[] = DS_ATTR_BAD_PASSWORD_COUNT;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrAccountExpiry[] = DS_ATTR_ACCOUNT_EXPIRY;
    DWORD i = 0;
    PWSTR pwszPassword = NULL;
    DWORD dwPasswordLen = 0;

    enum AttrValueIndex {
        ATTR_VAL_IDX_FULL_NAME = 0,
        ATTR_VAL_IDX_PRIMARY_GROUP,
        ATTR_VAL_IDX_HOME_DIRECTORY,
        ATTR_VAL_IDX_HOME_DRIVE,
        ATTR_VAL_IDX_LOGON_SCRIPT,
        ATTR_VAL_IDX_PROFILE_PATH, 
        ATTR_VAL_IDX_DESCRIPTION,
        ATTR_VAL_IDX_WORKSTATIONS,
        ATTR_VAL_IDX_COMMENT,
        ATTR_VAL_IDX_PARAMETERS,
        ATTR_VAL_IDX_LAST_PASSWORD_CHANGE,
        ATTR_VAL_IDX_ALLOW_PASSWORD_CHANGE,
        ATTR_VAL_IDX_FORCE_PASSWORD_CHANGE,
        ATTR_VAL_IDX_ACCOUNT_EXPIRY,
        ATTR_VAL_IDX_ACCOUNT_FLAGS,
        ATTR_VAL_IDX_BAD_PASSWORD_COUNT,
        ATTR_VAL_IDX_COUNTRY_CODE,
        ATTR_VAL_IDX_CODE_PAGE,
        ATTR_VAL_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {   /* ATTR_VAL_IDX_FULL_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_PRIMARY_GROUP */
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {   /* ATTR_VAL_IDX_HOME_DIRECTORY */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_HOME_DRIVE */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_LOGON_SCRIPT */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_PROFILE_PATH */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_DESCRIPTION */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_WORKSTATIONS */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_COMMENT */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_PARAMETERS */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_LAST_PASSWORD_CHANGE */
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.llValue = 0
        },
        {   /* ATTR_VAL_IDX_ALLOW_PASSWORD_CHANGE */
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.ulValue = 0
        },
        {   /* ATTR_VAL_IDX_FORCE_PASSWORD_CHANGE */
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.llValue = 0
        },
        {   /* ATTR_VAL_IDX_ACCOUNT_EXPIRY */
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.llValue = 0
        },
        {   /* ATTR_VAL_IDX_ACCOUNT_FLAGS */
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {   /* ATTR_VAL_IDX_BAD_PASSWORD_COUNT */
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {   /* ATTR_VAL_IDX_COUNTRY_CODE */
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {   /* ATTR_VAL_IDX_CODE_PAGE */
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        }
    };

    DIRECTORY_MOD ModFullName = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrFullName,
        1,
        &AttrValues[ATTR_VAL_IDX_FULL_NAME]
    };

    DIRECTORY_MOD ModPrimaryGroup = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrPrimaryGroup,
        1,
        &AttrValues[ATTR_VAL_IDX_PRIMARY_GROUP]
    };

    DIRECTORY_MOD ModHomeDirectory = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrHomeDirectory,
        1,
        &AttrValues[ATTR_VAL_IDX_HOME_DIRECTORY]
    };

    DIRECTORY_MOD ModHomeDrive = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrHomeDrive,
        1,
        &AttrValues[ATTR_VAL_IDX_HOME_DRIVE]
    };

    DIRECTORY_MOD ModLogonScript = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrLogonScript,
        1,
        &AttrValues[ATTR_VAL_IDX_LOGON_SCRIPT]
    };

    DIRECTORY_MOD ModProfilePath = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrProfilePath,
        1,
        &AttrValues[ATTR_VAL_IDX_PROFILE_PATH]
    };

    DIRECTORY_MOD ModDescription = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrDescription,
        1,
        &AttrValues[ATTR_VAL_IDX_DESCRIPTION]
    };

    DIRECTORY_MOD ModWorkstations = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrWorkstations,
        1,
        &AttrValues[ATTR_VAL_IDX_WORKSTATIONS]
    };

    DIRECTORY_MOD ModComment = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrComment,
        1,
        &AttrValues[ATTR_VAL_IDX_COMMENT]
    };

    DIRECTORY_MOD ModParameters = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrParameters,
        1,
        &AttrValues[ATTR_VAL_IDX_PARAMETERS]
    };

    DIRECTORY_MOD ModLastPasswordChange = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrLastPasswordChange,
        1,
        &AttrValues[ATTR_VAL_IDX_LAST_PASSWORD_CHANGE]
    };

    DIRECTORY_MOD ModAllowPasswordChange = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrAllowPasswordChange,
        1,
        &AttrValues[ATTR_VAL_IDX_ALLOW_PASSWORD_CHANGE]
    };

    DIRECTORY_MOD ModForcePasswordChange = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrForcePasswordChange,
        1,
        &AttrValues[ATTR_VAL_IDX_FORCE_PASSWORD_CHANGE]
    };

    DIRECTORY_MOD ModAccountExpiry = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrAccountExpiry,
        1,
        &AttrValues[ATTR_VAL_IDX_ACCOUNT_EXPIRY]
    };

    DIRECTORY_MOD ModAccountFlags = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrAccountFlags,
        1,
        &AttrValues[ATTR_VAL_IDX_ACCOUNT_FLAGS]
    };

    DIRECTORY_MOD ModBadPasswordCount = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrBadPasswordCount,
        1,
        &AttrValues[ATTR_VAL_IDX_BAD_PASSWORD_COUNT]
    };

    DIRECTORY_MOD ModCountryCode = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrCountryCode,
        1,
        &AttrValues[ATTR_VAL_IDX_COUNTRY_CODE]
    };

    DIRECTORY_MOD ModCodePage = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrCodePage,
        1,
        &AttrValues[ATTR_VAL_IDX_CODE_PAGE]
    };

    DIRECTORY_MOD Mods[ATTR_VAL_IDX_SENTINEL + 1];
    memset(&Mods, 0, sizeof(Mods));

    pAcctCtx = (PACCOUNT_CONTEXT)hUser;

    if (pAcctCtx == NULL || pAcctCtx->Type != SamrContextAccount)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!(pAcctCtx->dwAccessGranted & USER_ACCESS_SET_ATTRIBUTES))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pDomCtx       = pAcctCtx->pDomCtx;
    pConnCtx      = pDomCtx->pConnCtx;
    hDirectory    = pConnCtx->hDirectory;
    pwszAccountDn = pAcctCtx->pwszDn;

    /*
     * Check if there's an account rename pending
     */
    if (level == 6)
    {
        ntStatus = SamrSrvRenameAccount(hUser,
                                        &pInfo->info6.account_name);
    }
    else if (level == 7)
    {
        ntStatus = SamrSrvRenameAccount(hUser,
                                        &pInfo->info7.account_name);

        /* No further modification of user account is needed */
        goto cleanup;
    }
    else if (level == 21 &&
             pInfo->info21.fields_present & SAMR_FIELD_ACCOUNT_NAME)
    {
        ntStatus = SamrSrvRenameAccount(hUser,
                                        &pInfo->info21.account_name);
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    switch (level)
    {
    case 6:
    case 8:
        if (!(pAcctCtx->dwAccessGranted & USER_ACCESS_SET_LOC_COM) ||
            !(pAcctCtx->dwAccessGranted & USER_ACCESS_SET_ATTRIBUTES))
        {
            ntStatus = STATUS_ACCESS_DENIED;
        }
        break;

    case 25:
        if (!(pAcctCtx->dwAccessGranted & USER_ACCESS_SET_PASSWORD) ||
            !(pAcctCtx->dwAccessGranted & USER_ACCESS_SET_ATTRIBUTES))
        {
            ntStatus = STATUS_ACCESS_DENIED;
        }
        break;

    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 16:
    case 17:
    case 20:
    case 21:
        if (!(pAcctCtx->dwAccessGranted & USER_ACCESS_SET_ATTRIBUTES))
        {
            ntStatus = STATUS_ACCESS_DENIED;
        }
        break;

    case 26:
        if (!(pAcctCtx->dwAccessGranted & USER_ACCESS_SET_PASSWORD))
        {
            ntStatus = STATUS_ACCESS_DENIED;
        }
        break;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Now update other fields
     */
    switch (level)
    {
    case 6:
        SET_UNICODE_STRING_VALUE(pInfo->info6.full_name,
                                 ATTR_VAL_IDX_FULL_NAME,
                                 ModFullName);
        break;

    case 8:
        SET_UNICODE_STRING_VALUE(pInfo->info8.full_name,
                                 ATTR_VAL_IDX_FULL_NAME,
                                 ModFullName);
        break;

    case 9:
        SET_UINT32_VALUE(pInfo->info9.primary_gid,
                         ATTR_VAL_IDX_PRIMARY_GROUP,
                         ModPrimaryGroup);
        break;

    case 10:
        SET_UNICODE_STRING_VALUE(pInfo->info10.home_directory,
                                 ATTR_VAL_IDX_HOME_DIRECTORY,
                                 ModHomeDirectory);
        SET_UNICODE_STRING_VALUE(pInfo->info10.home_drive,
                                 ATTR_VAL_IDX_HOME_DRIVE,
                                 ModHomeDrive);
        break;

    case 11:
        SET_UNICODE_STRING_VALUE(pInfo->info11.logon_script,
                                 ATTR_VAL_IDX_LOGON_SCRIPT,
                                 ModLogonScript);
        break;

    case 12:
        SET_UNICODE_STRING_VALUE(pInfo->info12.profile_path,
                                 ATTR_VAL_IDX_PROFILE_PATH,
                                 ModProfilePath);
        break;

    case 13:
        SET_UNICODE_STRING_VALUE(pInfo->info13.description,
                                 ATTR_VAL_IDX_DESCRIPTION,
                                 ModDescription);
        break;

    case 14:
        SET_UNICODE_STRING_VALUE(pInfo->info14.workstations,
                                 ATTR_VAL_IDX_WORKSTATIONS,
                                 ModWorkstations);
        break;

    case 16:
        SET_UINT32_VALUE(pInfo->info16.account_flags,
                         ATTR_VAL_IDX_ACCOUNT_FLAGS,
                         ModAccountFlags);
        break;

    case 17:
        SET_NTTIME_VALUE(pInfo->info17.account_expiry,
                         ATTR_VAL_IDX_ACCOUNT_EXPIRY,
                         ModAccountExpiry);
        break;

    case 20:
        SET_UNICODE_STRING_VALUE(pInfo->info20.parameters,
                                 ATTR_VAL_IDX_PARAMETERS,
                                 ModParameters);
        break;

    case 21:
        SET_NTTIME_VALUE_BY_FLAG(&pInfo->info21, last_password_change,
                                 SAMR_FIELD_LAST_PWD_CHANGE,
                                 ATTR_VAL_IDX_LAST_PASSWORD_CHANGE,
                                 ModLastPasswordChange);
        SET_NTTIME_VALUE_BY_FLAG(&pInfo->info21, account_expiry,
                                 SAMR_FIELD_ACCT_EXPIRY,
                                 ATTR_VAL_IDX_ACCOUNT_EXPIRY,
                                 ModAccountExpiry);
        SET_NTTIME_VALUE_BY_FLAG(&pInfo->info21, allow_password_change,
                                 SAMR_FIELD_ALLOW_PWD_CHANGE,
                                 ATTR_VAL_IDX_ALLOW_PASSWORD_CHANGE,
                                 ModAllowPasswordChange);
        SET_NTTIME_VALUE_BY_FLAG(&pInfo->info21, force_password_change,
                                 SAMR_FIELD_FORCE_PWD_CHANGE,
                                 ATTR_VAL_IDX_FORCE_PASSWORD_CHANGE,
                                 ModForcePasswordChange);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info21, full_name,
                                         SAMR_FIELD_FULL_NAME,
                                         ATTR_VAL_IDX_FULL_NAME,
                                         ModFullName);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info21, home_directory,
                                         SAMR_FIELD_HOME_DIRECTORY,
                                         ATTR_VAL_IDX_HOME_DIRECTORY,
                                         ModHomeDirectory);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info21, home_drive,
                                         SAMR_FIELD_HOME_DRIVE,
                                         ATTR_VAL_IDX_HOME_DRIVE,
                                         ModHomeDrive);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info21, logon_script,
                                         SAMR_FIELD_LOGON_SCRIPT,
                                         ATTR_VAL_IDX_LOGON_SCRIPT,
                                         ModLogonScript);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info21, profile_path,
                                         SAMR_FIELD_PROFILE_PATH,
                                         ATTR_VAL_IDX_PROFILE_PATH,
                                         ModProfilePath);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info21, description,
                                         SAMR_FIELD_DESCRIPTION,
                                         ATTR_VAL_IDX_DESCRIPTION,
                                         ModDescription);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info21, workstations,
                                         SAMR_FIELD_WORKSTATIONS,
                                         ATTR_VAL_IDX_WORKSTATIONS,
                                         ModWorkstations);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info21, comment,
                                         SAMR_FIELD_COMMENT,
                                         ATTR_VAL_IDX_COMMENT,
                                         ModComment);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info21, parameters,
                                         SAMR_FIELD_PARAMETERS,
                                         ATTR_VAL_IDX_PARAMETERS,
                                         ModLogonScript);
        SET_UINT32_VALUE_BY_FLAG(&pInfo->info21, primary_gid,
                                 SAMR_FIELD_PRIMARY_GID,
                                 ATTR_VAL_IDX_PRIMARY_GROUP,
                                 ModPrimaryGroup);
        SET_UINT32_VALUE_BY_FLAG(&pInfo->info21, account_flags,
                                 SAMR_FIELD_ACCT_FLAGS,
                                 ATTR_VAL_IDX_ACCOUNT_FLAGS,
                                 ModAccountFlags);
        SET_UINT32_VALUE_BY_FLAG(&pInfo->info21, bad_password_count,
                                 SAMR_FIELD_BAD_PWD_COUNT,
                                 ATTR_VAL_IDX_BAD_PASSWORD_COUNT,
                                 ModBadPasswordCount);
        SET_UINT32_VALUE_BY_FLAG(&pInfo->info21, country_code,
                                 SAMR_FIELD_COUNTRY_CODE,
                                 ATTR_VAL_IDX_COUNTRY_CODE,
                                 ModCountryCode);
        SET_UINT32_VALUE_BY_FLAG(&pInfo->info21, code_page,
                                 SAMR_FIELD_CODE_PAGE,
                                 ATTR_VAL_IDX_CODE_PAGE,
                                 ModCodePage);
        EXPIRE_PASSWORD_BY_FLAG(&pInfo->info21, password_expired,
                                SAMR_FIELD_EXPIRED_FLAG,
                                ATTR_VAL_IDX_LAST_PASSWORD_CHANGE,
                                ModLastPasswordChange);
        break;

    case 25:
        SET_NTTIME_VALUE_BY_FLAG(&pInfo->info25.info, last_password_change,
                                 SAMR_FIELD_LAST_PWD_CHANGE,
                                 ATTR_VAL_IDX_LAST_PASSWORD_CHANGE,
                                 ModLastPasswordChange);
        SET_NTTIME_VALUE_BY_FLAG(&pInfo->info25.info, account_expiry,
                                 SAMR_FIELD_ACCT_EXPIRY,
                                 ATTR_VAL_IDX_ACCOUNT_EXPIRY,
                                 ModAccountExpiry);
        SET_NTTIME_VALUE_BY_FLAG(&pInfo->info25.info, allow_password_change,
                                 SAMR_FIELD_ALLOW_PWD_CHANGE,
                                 ATTR_VAL_IDX_ALLOW_PASSWORD_CHANGE,
                                 ModAllowPasswordChange);
        SET_NTTIME_VALUE_BY_FLAG(&pInfo->info25.info, force_password_change,
                                 SAMR_FIELD_FORCE_PWD_CHANGE,
                                 ATTR_VAL_IDX_FORCE_PASSWORD_CHANGE,
                                 ModForcePasswordChange);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info25.info, full_name,
                                         SAMR_FIELD_FULL_NAME,
                                         ATTR_VAL_IDX_FULL_NAME,
                                         ModFullName);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info25.info, home_directory,
                                         SAMR_FIELD_HOME_DIRECTORY,
                                         ATTR_VAL_IDX_HOME_DIRECTORY,
                                         ModHomeDirectory);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info25.info, home_drive,
                                         SAMR_FIELD_HOME_DRIVE,
                                         ATTR_VAL_IDX_HOME_DRIVE,
                                         ModHomeDrive);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info25.info, logon_script,
                                         SAMR_FIELD_LOGON_SCRIPT,
                                         ATTR_VAL_IDX_LOGON_SCRIPT,
                                         ModLogonScript);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info25.info, profile_path,
                                         SAMR_FIELD_PROFILE_PATH,
                                         ATTR_VAL_IDX_PROFILE_PATH,
                                         ModProfilePath);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info25.info, description,
                                         SAMR_FIELD_DESCRIPTION,
                                         ATTR_VAL_IDX_DESCRIPTION,
                                         ModDescription);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info25.info, workstations,
                                         SAMR_FIELD_WORKSTATIONS,
                                         ATTR_VAL_IDX_WORKSTATIONS,
                                         ModWorkstations);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info25.info, comment,
                                         SAMR_FIELD_COMMENT,
                                         ATTR_VAL_IDX_COMMENT,
                                         ModComment);
        SET_UNICODE_STRING_VALUE_BY_FLAG(&pInfo->info25.info, parameters,
                                         SAMR_FIELD_PARAMETERS,
                                         ATTR_VAL_IDX_PARAMETERS,
                                         ModLogonScript);
        SET_UINT32_VALUE_BY_FLAG(&pInfo->info25.info, primary_gid,
                                 SAMR_FIELD_PRIMARY_GID,
                                 ATTR_VAL_IDX_PRIMARY_GROUP,
                                 ModPrimaryGroup);
        SET_UINT32_VALUE_BY_FLAG(&pInfo->info25.info, account_flags,
                                 SAMR_FIELD_ACCT_FLAGS,
                                 ATTR_VAL_IDX_ACCOUNT_FLAGS,
                                 ModAccountFlags);
        SET_UINT32_VALUE_BY_FLAG(&pInfo->info25.info, bad_password_count,
                                 SAMR_FIELD_BAD_PWD_COUNT,
                                 ATTR_VAL_IDX_BAD_PASSWORD_COUNT,
                                 ModBadPasswordCount);
        SET_UINT32_VALUE_BY_FLAG(&pInfo->info25.info, country_code,
                                 SAMR_FIELD_COUNTRY_CODE,
                                 ATTR_VAL_IDX_COUNTRY_CODE,
                                 ModCountryCode);
        SET_UINT32_VALUE_BY_FLAG(&pInfo->info25.info, code_page,
                                 SAMR_FIELD_CODE_PAGE,
                                 ATTR_VAL_IDX_CODE_PAGE,
                                 ModCodePage);
        EXPIRE_PASSWORD_BY_FLAG(&pInfo->info25.info, password_expired,
                                SAMR_FIELD_EXPIRED_FLAG,
                                ATTR_VAL_IDX_LAST_PASSWORD_CHANGE,
                                ModLastPasswordChange);

        ntStatus = SamrSrvDecryptPasswordBlobEx(pConnCtx,
                                                &pInfo->info25.password,
                                                NULL,
                                                0,
                                                0,
                                                &pwszPassword);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = SamrSrvCheckPasswordPolicy(pAcctCtx,
                                              pwszPassword);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwError = DirectorySetPassword(hDirectory,
                                       pwszAccountDn,
                                       pwszPassword);
        BAIL_ON_LSA_ERROR(dwError);

        break;

    case 26:
        dwPasswordLen = pInfo->info26.password_len;
        ntStatus = SamrSrvDecryptPasswordBlobEx(pConnCtx,
                                                &pInfo->info26.password,
                                                NULL,
                                                0,
                                                dwPasswordLen,
                                                &pwszPassword);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = SamrSrvCheckPasswordPolicy(pAcctCtx,
                                              pwszPassword);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwError = DirectorySetPassword(hDirectory,
                                       pwszAccountDn,
                                       pwszPassword);
        BAIL_ON_LSA_ERROR(dwError);
        break;

    default:
        ntStatus = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (i > 0)
    {
        dwError = DirectoryModifyObject(hDirectory,
                                        pwszAccountDn,
                                        Mods);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LW_SECURE_FREE_WSTRING(pwszPassword);

    for (i = ATTR_VAL_IDX_FULL_NAME;
         i < ATTR_VAL_IDX_SENTINEL;
         i++)
    {
        if (AttrValues[i].Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING)
        {
            SamrSrvFreeMemory(AttrValues[i].data.pwszStringValue);
        }
        else if (AttrValues[i].Type == DIRECTORY_ATTR_TYPE_OCTET_STREAM)
        {
            LW_SAFE_FREE_MEMORY(AttrValues[i].data.pOctetString);
        }
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvCheckPasswordPolicy(
    IN  PACCOUNT_CONTEXT pAcctCtx,
    IN  PWSTR            pwszPassword
    )
{
    wchar_t wszFilterFmt[] = L"%ws='%ws'";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrPasswordLastSet[] = DS_ATTR_PASSWORD_LAST_SET;
    PWSTR pwszBase = 0;
    DWORD dwScope = 0;
    size_t sDnLen = 0;
    DWORD dwFilterLen = 0;
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    LONG64 llLastPasswordChange = 0;
    LONG64 llCurrentTime = 0;
    size_t sPasswordLen = 0;

    PWSTR wszAttributes[] = {
        wszAttrPasswordLastSet,
        NULL
    };

    pDomCtx  = pAcctCtx->pDomCtx;
    pConnCtx = pDomCtx->pConnCtx;
    pwszBase = pDomCtx->pwszDn;

    dwError = LwWc16sLen(pAcctCtx->pwszDn, &sDnLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwFilterLen = ((sizeof(wszAttrDn)/sizeof(WCHAR)) - 1) +
                  sDnLen + 
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwFilterLen,
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                    wszAttrDn, pAcctCtx->pwszDn) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntry,
                              &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        ntStatus = STATUS_INVALID_HANDLE;

    }
    else if (dwNumEntries > 1)
    {
        ntStatus = STATUS_INTERNAL_ERROR;
    }

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrPasswordLastSet,
                              DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                              &llLastPasswordChange);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwGetNtTime((PULONG64)&llCurrentTime);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Check if password can be change give its age
     */
    if (llCurrentTime - llLastPasswordChange < pDomCtx->ntMinPasswordAge)
    {
        ntStatus = STATUS_PASSWORD_RESTRICTION;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Check if password is longer than required minimum
     */
    if (sPasswordLen < pDomCtx->dwMinPasswordLen)
    {
        ntStatus = STATUS_PASSWORD_RESTRICTION;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

cleanup:
    if (pEntry)
    {
        DirectoryFreeEntries(pEntry, dwNumEntries);
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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
