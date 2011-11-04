/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        LMaccess.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Network Management API, aka LanMan API (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */



#ifndef _LM_ACCESS_H_
#define _LM_ACCESS_H_

#include <lw/security-types.h>

//
// USER_INFO levels
//

typedef struct _USER_INFO_0
{
    PWSTR  usri0_name;

} USER_INFO_0, *PUSER_INFO_0;

typedef struct _USER_INFO_1
{
    PWSTR  usri1_name;
    PWSTR  usri1_password;
    DWORD  usri1_password_age;
    DWORD  usri1_priv;
    PWSTR  usri1_home_dir;
    PWSTR  usri1_comment;
    DWORD  usri1_flags;
    PWSTR  usri1_script_path;

} USER_INFO_1, *PUSER_INFO_1;

typedef struct _USER_INFO_2
{
    PWSTR  usri2_name;
    PWSTR  usri2_password;
    DWORD  usri2_password_age;
    DWORD  usri2_priv;
    PWSTR  usri2_home_dir;
    PWSTR  usri2_comment;
    DWORD  usri2_flags;
    PWSTR  usri2_script_path;
    DWORD  usri2_auth_flags;
    PWSTR  usri2_full_name;
    PWSTR  usri2_usr_comment;
    PWSTR  usri2_parms;
    PWSTR  usri2_workstations;
    DWORD  usri2_last_logon;
    DWORD  usri2_last_logoff;
    DWORD  usri2_acct_expires;
    DWORD  usri2_max_storage;
    DWORD  usri2_units_per_week;
    PBYTE  usri2_logon_hours;
    DWORD  usri2_bad_pw_count;
    DWORD  usri2_num_logons;
    PWSTR  usri2_logon_server;
    DWORD  usri2_country_code;
    DWORD  usri2_code_page;

} USER_INFO_2, *PUSER_INFO_2;

typedef struct _USER_INFO_3
{
    PWSTR  usri3_name;
    PWSTR  usri3_password;
    DWORD  usri3_password_age;
    DWORD  usri3_priv;
    PWSTR  usri3_home_dir;
    PWSTR  usri3_comment;
    DWORD  usri3_flags;
    PWSTR  usri3_script_path;
    DWORD  usri3_auth_flags;
    PWSTR  usri3_full_name;
    PWSTR  usri3_usr_comment;
    PWSTR  usri3_parms;
    PWSTR  usri3_workstations;
    DWORD  usri3_last_logon;
    DWORD  usri3_last_logoff;
    DWORD  usri3_acct_expires;
    DWORD  usri3_max_storage;
    DWORD  usri3_units_per_week;
    PBYTE  usri3_logon_hours;
    DWORD  usri3_bad_pw_count;
    DWORD  usri3_num_logons;
    PWSTR  usri3_logon_server;
    DWORD  usri3_country_code;
    DWORD  usri3_code_page;
    DWORD  usri3_user_id;
    DWORD  usri3_primary_group_id;
    PWSTR  usri3_profile;
    PWSTR  usri3_home_dir_drive;
    DWORD  usri3_password_expired;

} USER_INFO_3, *PUSER_INFO_3;

typedef struct _USER_INFO_4
{
    PWSTR  usri4_name;
    PWSTR  usri4_password;
    DWORD  usri4_password_age;
    DWORD  usri4_priv;
    PWSTR  usri4_home_dir;
    PWSTR  usri4_comment;
    DWORD  usri4_flags;
    PWSTR  usri4_script_path;
    DWORD  usri4_auth_flags;
    PWSTR  usri4_full_name;
    PWSTR  usri4_usr_comment;
    PWSTR  usri4_parms;
    PWSTR  usri4_workstations;
    DWORD  usri4_last_logon;
    DWORD  usri4_last_logoff;
    DWORD  usri4_acct_expires;
    DWORD  usri4_max_storage;
    DWORD  usri4_units_per_week;
    PBYTE  usri4_logon_hours;
    DWORD  usri4_bad_pw_count;
    DWORD  usri4_num_logons;
    PWSTR  usri4_logon_server;
    DWORD  usri4_country_code;
    DWORD  usri4_code_page;
    PSID   usri4_user_sid;
    DWORD  usri4_primary_group_id;
    PWSTR  usri4_profile;
    PWSTR  usri4_home_dir_drive;
    DWORD  usri4_password_expired;

} USER_INFO_4, *PUSER_INFO_4;

typedef struct _USER_INFO_10
{
    PWSTR  usri10_name;
    PWSTR  usri10_comment;
    PWSTR  usri10_usr_comment;
    PWSTR  usri10_full_name;

} USER_INFO_10, *PUSER_INFO_10;

typedef struct _USER_INFO_11
{
    PWSTR  usri11_name;
    PWSTR  usri11_comment;
    PWSTR  usri11_usr_comment;
    PWSTR  usri11_full_name;
    DWORD  usri11_priv;
    DWORD  usri11_auth_flags;
    DWORD  usri11_password_age;
    PWSTR  usri11_home_dir;
    PWSTR  usri11_parms;
    DWORD  usri11_last_logon;
    DWORD  usri11_last_logoff;
    DWORD  usri11_bad_pw_count;
    DWORD  usri11_num_logons;
    PWSTR  usri11_logon_server;
    DWORD  usri11_country_code;
    PWSTR  usri11_workstations;
    DWORD  usri11_max_storage;
    DWORD  usri11_units_per_week;
    PBYTE  usri11_logon_hours;
    DWORD  usri11_code_page;

} USER_INFO_11, *PUSER_INFO_11;

typedef struct _USER_INFO_20
{
    PWSTR  usri20_name;
    PWSTR  usri20_full_name;
    PWSTR  usri20_comment;
    DWORD  usri20_flags;
    DWORD  usri20_user_id;

} USER_INFO_20, *PUSER_INFO_20;

typedef struct _USER_INFO_23
{
    PWSTR  usri23_name;
    PWSTR  usri23_full_name;
    PWSTR  usri23_comment;
    PSID   usri23_user_sid;

} USER_INFO_23, *PUSER_INFO_23;

typedef struct _USER_INFO_1003
{
    PWSTR  usri1003_password;

} USER_INFO_1003, *PUSER_INFO_1003;

typedef struct _USER_INFO_1007
{
    PWSTR  usri1007_comment;

} USER_INFO_1007, *PUSER_INFO_1007;

typedef struct _USER_INFO_1008
{
    DWORD  usri1008_flags;

} USER_INFO_1008, *PUSER_INFO_1008;

typedef struct _USER_INFO_1011
{
    PWSTR  usri1011_full_name;

} USER_INFO_1011, *PUSER_INFO_1011;

typedef struct _USER_INFO_1012
{
    PWSTR  usri1012_usr_comment;

} USER_INFO_1012, *PUSER_INFO_1012;


/*
 * Possible values returned in pdwParmErr pointer indicating
 * invalid parameter when ERROR_INVALID_PARAMETER is returned
 * from NetUserAdd or NetUserSetInfo
 */

#define USER_NAME_PARMNUM           (1)
#define USER_PASSWORD_PARMNUM       (2)
#define USER_PASSWORD_AGE_PARMNUM   (3)
#define USER_PRIV_PARMNUM           (4)
#define USER_HOME_DIR_PARMNUM       (5)
#define USER_COMMENT_PARMNUM        (6)
#define USER_FLAGS_PARMNUM          (7)
#define USER_SCRIPT_PATH_PARMNUM    (8)
#define USER_AUTH_FLAGS_PARMNUM     (9)
#define USER_FULL_NAME_PARMNUM      (10)
#define USER_USR_COMMENT_PARMNUM    (11)
#define USER_PARMS_PARMNUM          (12)
#define USER_WORKSTATIONS_PARMNUM   (13)
#define USER_LAST_LOGON_PARMNUM     (14)
#define USER_LAST_LOGOFF_PARMNUM    (15)
#define USER_ACCT_EXPIRES_PARMNUM   (16)
#define USER_MAX_STORAGE_PARMNUM    (17)
#define USER_UNITS_PER_WEEK_PARMNUM (18)
#define USER_LOGON_HOURS_PARMNUM    (19)
#define USER_BAD_PW_COUNT_PARMNUM   (20)
#define USER_NUM_LOGONS_PARMNUM     (21)
#define USER_LOGON_SERVER_PARMNUM   (22)
#define USER_COUNTRY_CODE_PARMNUM   (23)
#define USER_CODE_PAGE_PARMNUM      (24)
#define USER_PRIMARY_GROUP_PARMNUM  (25)
#define USER_PROFILE_PARMNUM        (26)
#define USER_HOME_DIR_DRIVE_PARMNUM (27)

/*
 * Maximum length of user account name
 */
#define USER_NAME_LEN   (20)


/*
 * Maximum length of user account password
 */
#define PWLEN   (64)


//
// LOCALGROUP_USERS_INFO levels
//

typedef struct _LOCALGROUP_USERS_INFO_0
{
    PWSTR  lgrui0_name;

} LOCALGROUP_USERS_INFO_0, *PLOCALGROUP_USERS_INFO_0;


//
// LOCALGROUP_INFO levels
//

typedef struct _LOCALGROUP_INFO_0
{
    PWSTR  lgrpi0_name;

} LOCALGROUP_INFO_0, *PLOCALGROUP_INFO_0;

typedef struct _LOCALGROUP_INFO_1
{
    PWSTR  lgrpi1_name;
    PWSTR  lgrpi1_comment;

} LOCALGROUP_INFO_1, *PLOCALGROUP_INFO_1;

typedef struct _LOCALGROUP_INFO_1002
{
    PWSTR  lgrpi1002_comment;

} LOCALGROUP_INFO_1002, *PLOCALGROUP_INFO_1002;

typedef struct _LOCALGROUP_MEMBERS_INFO_0
{
    PSID   lgrmi0_sid;

} LOCALGROUP_MEMBERS_INFO_0, *PLOCALGROUP_MEMBERS_INFO_0;

typedef struct _LOCALGROUP_MEMBERS_INFO_3
{
    PWSTR  lgrmi3_domainandname;

} LOCALGROUP_MEMBERS_INFO_3, *PLOCALGROUP_MEMBERS_INFO_3;


typedef struct _NET_DISPLAY_USER
{
    PWSTR  usri1_name;
    PWSTR  usri1_comment;
    DWORD  usri1_flags;
    PWSTR  usri1_full_name;
    DWORD  usri1_user_id;
    DWORD  usri1_next_index;

} NET_DISPLAY_USER, *PNET_DISPLAY_USER;

typedef struct _NET_DISPLAY_MACHINE
{
    PWSTR  usri2_name;
    PWSTR  usri2_comment;
    DWORD  usri2_flags;
    DWORD  usri2_user_id;
    DWORD  usri2_next_index;

} NET_DISPLAY_MACHINE, *PNET_DISPLAY_MACHINE;

typedef struct _NET_DISPLAY_GROUP
{
    PWSTR  grpi3_name;
    PWSTR  grpi3_comment;
    DWORD  grpi3_group_id;
    DWORD  grpi3_attributes;
    DWORD  grpi3_next_index;

} NET_DISPLAY_GROUP, *PNET_DISPLAY_GROUP;


#ifndef NET_API_STATUS_DEFINED
typedef WINERROR NET_API_STATUS;

#define NET_API_STATUS_DEFINED
#endif


NET_API_STATUS
NetUserEnum(
    PCWSTR  pwszHostname,
    DWORD   dwLevel,
    DWORD   dwFilter,
    PVOID  *ppBuffer,
    DWORD   dwMaxBufferSize,
    PDWORD  pdwNumEntries,
    PDWORD  pdwTotalNumEntries,
    PDWORD  pdwResume
    );


NET_API_STATUS
NetUserAdd(
    PCWSTR  pwszHostname,
    DWORD   dwLevel,
    PVOID   pBuffer,
    PDWORD  pdwParmErr
    );


NET_API_STATUS
NetUserDel(
    PCWSTR  pwszHostname,
    PCWSTR  pwszUsername
    );


NET_API_STATUS
NetUserGetInfo(
    PCWSTR  pwszHostname,
    PCWSTR  pwszUsername,
    DWORD   dwLevel,
    PVOID  *ppBuffer
    );


NET_API_STATUS
NetUserSetInfo(
    PCWSTR  pwszHostname,
    PCWSTR  pwszUsername,
    DWORD   dwLevel,
    PVOID   pBuffer,
    PDWORD  pdwParmErr
    );


NET_API_STATUS
NetUserGetLocalGroups(
    PCWSTR  pwszHostname,
    PCWSTR  pwszUsername,
    DWORD   dwLevel,
    DWORD   dwFlags,
    PVOID  *ppBuffer,
    DWORD   dwMaxBufferSize,
    PDWORD  pdwNumEntries,
    PDWORD  pdwTotalNumEntries
    );


NET_API_STATUS
NetLocalGroupAdd(
    PCWSTR  pwszHostname,
    DWORD   dwLevel,
    PVOID   pBuffer,
    PDWORD  pdwParmErr
    );


NET_API_STATUS
NetLocalGroupDel(
    PCWSTR  pwszHostname,
    PCWSTR  pwszGroupname
    );


NET_API_STATUS
NetLocalGroupEnum(
    PCWSTR  pwszHostname,
    DWORD   dwLevel,
    PVOID  *ppBuffer,
    DWORD   dwBufferMaxSize,
    PDWORD  pdwNumEntries,
    PDWORD  pdwTotalNumEntries,
    PDWORD  pdwResume
    );


NET_API_STATUS
NetLocalGroupSetInfo(
    PCWSTR  pwszHostname,
    PCWSTR  pwszGroupname,
    DWORD   dwLevel,
    PVOID   pBuffer,
    PDWORD  pdwParmErr
    );


NET_API_STATUS
NetLocalGroupGetInfo(
    PCWSTR  pwszHostname,
    PCWSTR  pwszGroupname,
    DWORD   dwLevel,
    PVOID  *ppBuffer
    );


NET_API_STATUS
NetLocalGroupAddMembers(
    PCWSTR  pwszHostname,
    PCWSTR  pwszGroupname,
    DWORD   dwLevel,
    PVOID   pBuffer,
    DWORD   dwNumEntries
    );


NET_API_STATUS
NetLocalGroupDelMembers(
    PCWSTR  pwszHostname,
    PCWSTR  pwszGroupname,
    DWORD   dwLevel,
    PVOID   pBuffer,
    DWORD   dwNumEntries
    );


NET_API_STATUS
NetLocalGroupGetMembers(
    PCWSTR  pwszHostname,
    PCWSTR  pwszLocalGroupName,
    DWORD   dwLevel,
    PVOID  *ppBuffer,
    DWORD   dwMaxBufferSize,
    PDWORD  pdwNumEntries,
    PDWORD  pdwTotalNumEntries,
    PDWORD  pdwResume
    );


NET_API_STATUS
NetQueryDisplayInformation(
    PCWSTR   pwszHostname,
    DWORD    dwLevel,
    DWORD    dwIndex,
    DWORD    dwEntriesRequested,
    DWORD    dwMaxBufferSize,
    PDWORD   pdwNumEntries,
    PVOID   *pBuffer
    );


NET_API_STATUS
NetApiBufferFree(
    PVOID   pBuffer
    );


NET_API_STATUS
NetUserChangePassword(
    PCWSTR  pwszDomain,
    PCWSTR  pwszUsername,
    PCWSTR  pwszOldPassword,
    PCWSTR  pwszNewPassword
    );


NET_API_STATUS
NetGetDomainName(
    PCWSTR  pwszHostname,
    PWSTR  *ppwszDomainName
    );


/* this allows functions to return as much data as available */
#define MAX_PREFERRED_LENGTH                          (-1)

/* filter flags for NetUserEnum function */
#define FILTER_TEMP_DUPLICATE_ACCOUNT                 0x0001
#define FILTER_NORMAL_ACCOUNT                         0x0002
#define FILTER_INTERDOMAIN_TRUST_ACCOUNT              0x0008
#define FILTER_WORKSTATION_TRUST_ACCOUNT              0x0010
#define FILTER_SERVER_TRUST_ACCOUNT                   0x0020

/* account flags */
#define UF_SCRIPT                                     0x00000001
#define UF_ACCOUNTDISABLE                             0x00000002
#define UF_HOMEDIR_REQUIRED                           0x00000008
#define UF_LOCKOUT                                    0x00000010
#define UF_PASSWD_NOTREQD                             0x00000020
#define UF_PASSWD_CANT_CHANGE                         0x00000040
#define UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED            0x00000080
#define UF_TEMP_DUPLICATE_ACCOUNT                     0x00000100
#define UF_NORMAL_ACCOUNT                             0x00000200
#define UF_INTERDOMAIN_TRUST_ACCOUNT                  0x00000800
#define UF_WORKSTATION_TRUST_ACCOUNT                  0x00001000
#define UF_SERVER_TRUST_ACCOUNT                       0x00002000
#define UF_DONT_EXPIRE_PASSWD                         0x00010000
#define UF_SMARTCARD_REQUIRED                         0x00040000
#define UF_TRUSTED_FOR_DELEGATION                     0x00080000
#define UF_NOT_DELEGATED                              0x00100000
#define UF_USE_DES_KEY_ONLY                           0x00200000
#define UF_DONT_REQUIRE_PREAUTH                       0x00400000
#define UF_PASSWORD_EXPIRED                           0x00800000
#define UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION     0x01000000

/* user privileges flags */
#define USER_PRIV_GUEST                               0x00000000
#define USER_PRIV_USER                                0x00000001
#define USER_PRIV_ADMIN                               0x00000002

#endif /* _LM_ACCESS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
