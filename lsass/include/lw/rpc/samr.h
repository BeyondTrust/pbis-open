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
 *        samr.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Samr rpc client library
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#ifndef _RPC_SAMR_H_
#define _RPC_SAMR_H_


/* Connect access mask flags */
#define SAMR_ACCESS_CONNECT_TO_SERVER          0x00000001
#define SAMR_ACCESS_SHUTDOWN_SERVER            0x00000002
#define SAMR_ACCESS_INITIALIZE_SERVER          0x00000004
#define SAMR_ACCESS_CREATE_DOMAIN              0x00000008
#define SAMR_ACCESS_ENUM_DOMAINS               0x00000010
#define SAMR_ACCESS_OPEN_DOMAIN                0x00000020

/* Domain access mask flags */
#define DOMAIN_ACCESS_LOOKUP_INFO_1            0x00000001
#define DOMAIN_ACCESS_SET_INFO_1               0x00000002
#define DOMAIN_ACCESS_LOOKUP_INFO_2            0x00000004
#define DOMAIN_ACCESS_SET_INFO_2               0x00000008
#define DOMAIN_ACCESS_CREATE_USER              0x00000010
#define DOMAIN_ACCESS_CREATE_GROUP             0x00000020
#define DOMAIN_ACCESS_CREATE_ALIAS             0x00000040
#define DOMAIN_ACCESS_LOOKUP_ALIAS             0x00000080
#define DOMAIN_ACCESS_ENUM_ACCOUNTS            0x00000100
#define DOMAIN_ACCESS_OPEN_ACCOUNT             0x00000200
#define DOMAIN_ACCESS_SET_INFO_3               0x00000400

/* User access mask flags */
#define USER_ACCESS_GET_NAME_ETC               0x00000001
#define USER_ACCESS_GET_LOCALE                 0x00000002
#define USER_ACCESS_SET_LOC_COM                0x00000004
#define USER_ACCESS_GET_LOGONINFO              0x00000008
#define USER_ACCESS_GET_ATTRIBUTES             0x00000010
#define USER_ACCESS_SET_ATTRIBUTES             0x00000020
#define USER_ACCESS_CHANGE_PASSWORD            0x00000040
#define USER_ACCESS_SET_PASSWORD               0x00000080
#define USER_ACCESS_GET_GROUPS                 0x00000100
#define USER_ACCESS_GET_GROUP_MEMBERSHIP       0x00000200
#define USER_ACCESS_CHANGE_GROUP_MEMBERSHIP    0x00000400

/* Alias access mask flags */
#define ALIAS_ACCESS_ADD_MEMBER                0x00000001
#define ALIAS_ACCESS_REMOVE_MEMBER             0x00000002
#define ALIAS_ACCESS_GET_MEMBERS               0x00000004
#define ALIAS_ACCESS_LOOKUP_INFO               0x00000008
#define ALIAS_ACCESS_SET_INFO                  0x00000010

/* Group access mask flags */
#define GROUP_ACCESS_LOOKUP_INFO               0x00000001
#define GROUP_ACCESS_SET_INFO                  0x00000002
#define GROUP_ACCESS_ADD_MEMBER                0x00000004
#define GROUP_ACCESS_REMOVE_MEMBER             0x00000008
#define GROUP_ACCESS_GET_MEMBERS               0x00000010

/* Account type flags */
#define ACB_DISABLED                 0x00000001
#define ACB_HOMDIRREQ                0x00000002
#define ACB_PWNOTREQ                 0x00000004
#define ACB_TEMPDUP                  0x00000008
#define ACB_NORMAL                   0x00000010
#define ACB_MNS                      0x00000020
#define ACB_DOMTRUST                 0x00000040
#define ACB_WSTRUST                  0x00000080
#define ACB_SVRTRUST                 0x00000100
#define ACB_PWNOEXP                  0x00000200
#define ACB_AUTOLOCK                 0x00000400
#define ACB_ENC_TXT_PWD_ALLOWED      0x00000800
#define ACB_SMARTCARD_REQUIRED       0x00001000
#define ACB_TRUSTED_FOR_DELEGATION   0x00002000
#define ACB_NOT_DELEGATED            0x00004000
#define ACB_USE_DES_KEY_ONLY         0x00008000
#define ACB_DONT_REQUIRE_PREAUTH     0x00010000
#define ACB_PW_EXPIRED               0x00020000
#define ACB_NO_AUTH_DATA_REQD        0x00080000

#define SAMR_MAX_PREFERRED_SIZE      (0xffffffff)


/*
 * User info structures
 */
typedef struct user_info1 {
    UNICODE_STRING account_name;
    UNICODE_STRING full_name;
    UINT32 primary_gid;
    UNICODE_STRING description;
    UNICODE_STRING comment;
} UserInfo1;

typedef struct user_info2 {
    UNICODE_STRING comment;
    UNICODE_STRING unknown1;
    UINT16 country_code;
	UINT16 code_page;
} UserInfo2;

typedef struct logon_hours {
    UINT16 units_per_week;
#ifdef _DCE_IDL_
    [size_is(1260), length_is(units_per_week/8)]
#endif
    UINT8 *units;
} LogonHours;

typedef struct user_info3 {
    UNICODE_STRING account_name;
    UNICODE_STRING full_name;
    UINT32 rid;
    UINT32 primary_gid;
    UNICODE_STRING home_directory;
    UNICODE_STRING home_drive;
    UNICODE_STRING logon_script;
    UNICODE_STRING profile_path;
    UNICODE_STRING workstations;
    LONG64 last_logon;
    LONG64 last_logoff;
    LONG64 last_password_change;
    LONG64 allow_password_change;
    LONG64 force_password_change;
    LogonHours logon_hours;
    UINT16 bad_password_count;
    UINT16 logon_count;
    UINT32 account_flags;
} UserInfo3;

typedef struct user_info4 {
    LogonHours logon_hours;
} UserInfo4;

typedef struct user_info5 {
	UNICODE_STRING account_name;
	UNICODE_STRING full_name;
	UINT32 rid;
	UINT32 primary_gid;
	UNICODE_STRING home_directory;
	UNICODE_STRING home_drive;
	UNICODE_STRING logon_script;
	UNICODE_STRING profile_path;
	UNICODE_STRING description;
	UNICODE_STRING workstations;
	LONG64 last_logon;
	LONG64 last_logoff;
	LogonHours logon_hours;
	UINT16 bad_password_count;
	UINT16 logon_count;
	LONG64 last_password_change;
	LONG64 account_expiry;
	UINT32 account_flags;
} UserInfo5;

typedef struct user_info6 {
	UNICODE_STRING account_name;
	UNICODE_STRING full_name;
} UserInfo6;

typedef struct user_info7 {
	UNICODE_STRING account_name;
} UserInfo7;

typedef struct user_info8 {
	UNICODE_STRING full_name;
} UserInfo8;

typedef struct user_info9 {
	UINT32 primary_gid;
} UserInfo9;

typedef struct user_info10 {
	UNICODE_STRING home_directory;
	UNICODE_STRING home_drive;
} UserInfo10;

typedef struct user_info11 {
	UNICODE_STRING logon_script;
} UserInfo11;

typedef struct user_info12 {
	UNICODE_STRING profile_path;
} UserInfo12;

typedef struct user_info13 {
	UNICODE_STRING description;
} UserInfo13;

typedef struct user_info14 {
	UNICODE_STRING workstations;
} UserInfo14;

typedef struct user_info16 {
	UINT32 account_flags;
} UserInfo16;

typedef struct user_info17 {
	LONG64 account_expiry;
} UserInfo17;

typedef struct user_info20 {
	UNICODE_STRING parameters;
} UserInfo20;


#define SAMR_FIELD_ACCOUNT_NAME       0x00000001
#define SAMR_FIELD_FULL_NAME          0x00000002
#define SAMR_FIELD_RID                0x00000004
#define SAMR_FIELD_PRIMARY_GID        0x00000008
#define SAMR_FIELD_DESCRIPTION        0x00000010
#define SAMR_FIELD_COMMENT            0x00000020
#define SAMR_FIELD_HOME_DIRECTORY     0x00000040
#define SAMR_FIELD_HOME_DRIVE         0x00000080
#define SAMR_FIELD_LOGON_SCRIPT       0x00000100
#define SAMR_FIELD_PROFILE_PATH       0x00000200
#define SAMR_FIELD_WORKSTATIONS       0x00000400
#define SAMR_FIELD_LAST_LOGON         0x00000800
#define SAMR_FIELD_LAST_LOGOFF        0x00001000
#define SAMR_FIELD_LOGON_HOURS        0x00002000
#define SAMR_FIELD_BAD_PWD_COUNT      0x00004000
#define SAMR_FIELD_NUM_LOGONS         0x00008000
#define SAMR_FIELD_ALLOW_PWD_CHANGE   0x00010000
#define SAMR_FIELD_FORCE_PWD_CHANGE   0x00020000
#define SAMR_FIELD_LAST_PWD_CHANGE    0x00040000
#define SAMR_FIELD_ACCT_EXPIRY        0x00080000
#define SAMR_FIELD_ACCT_FLAGS         0x00100000
#define SAMR_FIELD_PARAMETERS         0x00200000
#define SAMR_FIELD_COUNTRY_CODE       0x00400000
#define SAMR_FIELD_CODE_PAGE          0x00800000
#define SAMR_FIELD_PASSWORD           0x01000000
#define SAMR_FIELD_PASSWORD2          0x02000000
#define SAMR_FIELD_PRIVATE_DATA       0x04000000
#define SAMR_FIELD_EXPIRED_FLAG       0x08000000
#define SAMR_FIELD_SEC_DESC           0x10000000
#define SAMR_FIELD_OWF_PWD            0x20000000


typedef struct user_info21 {
	LONG64 last_logon;
	LONG64 last_logoff;
	LONG64 last_password_change;
	LONG64 account_expiry;
	LONG64 allow_password_change;
	LONG64 force_password_change;
	UNICODE_STRING account_name;
	UNICODE_STRING full_name;
	UNICODE_STRING home_directory;
	UNICODE_STRING home_drive;
	UNICODE_STRING logon_script;
	UNICODE_STRING profile_path;
	UNICODE_STRING description;
	UNICODE_STRING workstations;
	UNICODE_STRING comment;
	UNICODE_STRING parameters;
	UNICODE_STRING unknown1;
	UNICODE_STRING unknown2;
	UNICODE_STRING unknown3;
	UINT32 buf_count;
#ifdef _DCE_IDL_
	[size_is(buf_count)]
#endif
	UINT8 *buffer;
	UINT32 rid;
	UINT32 primary_gid;
	UINT32 account_flags;
	UINT32 fields_present;
	LogonHours logon_hours;
	UINT16 bad_password_count;
	UINT16 logon_count;
	UINT16 country_code;
	UINT16 code_page;
	UINT8 nt_password_set;
	UINT8 lm_password_set;
	UINT8 password_expired;
	UINT8 unknown4;
} UserInfo21;

typedef struct hash_pass {
	UINT8 data[16];
} HashPassword;

typedef struct crypt_password {
	UINT8 data[516];
} CryptPassword;

typedef struct user_info23 {
	UserInfo21 info;
	CryptPassword password;
} UserInfo23;

typedef struct user_info24 {
	CryptPassword password;
	UINT8 password_len;
} UserInfo24;

typedef struct crypt_password_ex {
	UINT8 data[532];
} CryptPasswordEx;

typedef struct user_info25 {
	UserInfo21 info;
	CryptPasswordEx password;
} UserInfo25;

typedef struct user_info26 {
	CryptPasswordEx password;
	UINT8 password_len;
} UserInfo26;

typedef struct samr_pw_info {
    UINT16 min_password_length;
    UINT32 password_properties;
} PwInfo;

#ifndef _DCE_IDL_
typedef union user_info {
	UserInfo1 info1;
	UserInfo2 info2;
	UserInfo3 info3;
	UserInfo4 info4;
	UserInfo5 info5;
	UserInfo6 info6;
	UserInfo7 info7;
	UserInfo8 info8;
	UserInfo9 info9;
	UserInfo10 info10;
	UserInfo11 info11;
	UserInfo12 info12;
	UserInfo13 info13;
	UserInfo14 info14;
	UserInfo16 info16;
	UserInfo17 info17;
	UserInfo20 info20;
	UserInfo21 info21;
	UserInfo23 info23;
	UserInfo24 info24;
	UserInfo25 info25;
	UserInfo26 info26;
} UserInfo;
#endif


/*
 * Alias (a.k.a. local groups) info structures
 */
#define ALIAS_INFO_ALL          1
#define ALIAS_INFO_NAME         2
#define ALIAS_INFO_DESCRIPTION  3

typedef struct alias_info_all {
	UNICODE_STRING name;
	UINT32 num_members;
	UNICODE_STRING description;
} AliasInfoAll;


#ifndef _DCE_IDL_
typedef union alias_info {
	AliasInfoAll all;            /* read-only infolevel */
	UNICODE_STRING name;
	UNICODE_STRING description;
} AliasInfo;
#endif


/*
 * Connect info structures for SamrConnect[45]
 */

#define SAMR_CONNECT_PRE_WIN2K                 (1)
#define SAMR_CONNECT_WIN2K                     (2)
#define SAMR_CONNECT_POST_WIN2K                (3)


typedef struct samr_connect_info1 {
    UINT32 client_version;
    UINT32 unknown1;
} SamrConnectInfo1, SAMR_CONNECT_INFO_1, *PSAMR_CONNECT_INFO_1;


#ifndef _DCE_IDL_
typedef union samr_connect_info {
    SamrConnectInfo1  info1;
} SamrConnectInfo, SAMR_CONNECT_INFO, *PSAMR_CONNECT_INFO;
#endif


/*
 * Domain info structues
 */
typedef struct domain_info_1 {
    UINT16 min_pass_length;
    UINT16 pass_history_length;
    UINT32 pass_properties;
    INT64  max_pass_age;
    INT64  min_pass_age;
} DomainInfo1;

typedef struct domain_info_2 {
    LONG64 force_logoff_time;
    UNICODE_STRING comment;
    UNICODE_STRING domain_name;
    UNICODE_STRING primary;
    UINT64 sequence_num;
    UINT32 unknown1;
    UINT32 role;
    UINT32 unknown2;
    UINT32 num_users;
    UINT32 num_groups;
    UINT32 num_aliases;
} DomainInfo2;

typedef struct domain_info_3 {
    LONG64 force_logoff_time;
} DomainInfo3;

typedef struct domain_info_4 {
    UNICODE_STRING comment;
} DomainInfo4;

typedef struct domain_info_5 {
    UNICODE_STRING domain_name;
} DomainInfo5;

typedef struct domain_info_6 {
    UNICODE_STRING primary;
} DomainInfo6;

/* role */
#define SAMR_ROLE_STANDALONE      0
#define SAMR_ROLE_DOMAIN_MEMBER   1
#define SAMR_ROLE_DOMAIN_BDC      2
#define SAMR_ROLE_DOMAIN_PDC      3

typedef struct domain_info_7 {
    UINT32 role;
} DomainInfo7;

typedef struct domain_info_8 {
    UINT64 sequence_number;
    LONG64 domain_create_time;
} DomainInfo8;

typedef struct domain_info_9 {
    UINT32 unknown;
} DomainInfo9;

typedef struct domain_info_11 {
    DomainInfo2 info2;
    UINT64 lockout_duration;
    UINT64 lockout_window;
    UINT16 lockout_threshold;
} DomainInfo11;

typedef struct domain_info_12 {
    UINT64 lockout_duration;
    UINT64 lockout_window;
    UINT16 lockout_threshold;
} DomainInfo12;

typedef struct domain_info_13 {
    UINT64 sequence_number;
    LONG64 domain_create_time;
    UINT32 unknown1;
    UINT32 unknown2;
} DomainInfo13;


#ifndef _DCE_IDL_
typedef union domain_info {
    DomainInfo1 info1;
    DomainInfo2 info2;
    DomainInfo3 info3;
    DomainInfo4 info4;
    DomainInfo5 info5;
    DomainInfo6 info6;
    DomainInfo7 info7;
    DomainInfo8 info8;
    DomainInfo9 info9;
    DomainInfo11 info11;
    DomainInfo12 info12;
    DomainInfo13 info13;
} DomainInfo;
#endif /* _DCE_IDL_ */


/*
 * Display info structures
 */
typedef struct samr_display_entry_full {
    UINT32 idx;
    UINT32 rid;
    UINT32 account_flags;
    UNICODE_STRING account_name;
    UNICODE_STRING description;
    UNICODE_STRING full_name;
} SamrDisplayEntryFull;


typedef struct samr_display_info_full {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SamrDisplayEntryFull *entries;
} SamrDisplayInfoFull;


typedef struct samr_display_entry_general {
    UINT32 idx;
    UINT32 rid;
    UINT32 account_flags;
    UNICODE_STRING account_name;
    UNICODE_STRING description;
} SamrDisplayEntryGeneral;


typedef struct samr_display_info_general {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SamrDisplayEntryGeneral *entries;
} SamrDisplayInfoGeneral;


typedef struct samr_display_entry_general_group {
    UINT32 idx;
    UINT32 rid;
    UINT32 account_flags;
    UNICODE_STRING account_name;
    UNICODE_STRING description;
} SamrDisplayEntryGeneralGroup;

typedef struct samr_display_info_general_groups {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SamrDisplayEntryGeneralGroup *entries;
} SamrDisplayInfoGeneralGroups;


typedef struct samr_display_entry_ascii {
    UINT32 idx;
    ANSI_STRING account_name;
} SamrDisplayEntryAscii;


typedef struct samr_display_info_ascii {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SamrDisplayEntryAscii *entries;
} SamrDisplayInfoAscii;


#ifndef _DCE_IDL_
typedef union samr_display_info  {
    SamrDisplayInfoFull          info1;
    SamrDisplayInfoGeneral       info2;
    SamrDisplayInfoGeneralGroups info3;
    SamrDisplayInfoAscii         info4;
    SamrDisplayInfoAscii         info5;
} SamrDisplayInfo;
#endif /* _DCE_IDL_ */


typedef struct _SAMR_SECURITY_DESCRIPTOR_BUFFER
{
    ULONG ulBufferLen;
#ifdef _DCE_IDL_
    [size_is(ulBufferLen)]
#endif
    PBYTE pBuffer;
}
SAMR_SECURITY_DESCRIPTOR_BUFFER, *PSAMR_SECURITY_DESCRIPTOR_BUFFER;


typedef struct _UNICODE_STRING_ARRAY
{
    DWORD dwCount;
#ifdef _DCE_IDL_
    [size_is(dwCount)]
#endif
    PUNICODE_STRING pNames;
} UNICODE_STRING_ARRAY, *PUNICODE_STRING_ARRAY;


typedef struct _IDS {
#ifdef _DCE_IDL_
	[range(0,1024)]
#endif
    DWORD dwCount;
#ifdef _DCE_IDL_
	[size_is(dwCount)]
#endif
    PDWORD pIds;
} IDS, *PIDS;


typedef struct _RID_WITH_ATTRIBUTE
{
    DWORD  dwRid;
    DWORD  dwAttributes;
} RID_WITH_ATTRIBUTE, *PRID_WITH_ATTRIBUTE;


typedef struct _RID_WITH_ATTRIBUTE_ARRAY
{
    DWORD  dwCount;
#ifdef _DCE_IDL_
    [size_is(dwCount)]
#endif
    PRID_WITH_ATTRIBUTE pRids;
} RID_WITH_ATTRIBUTE_ARRAY, *PRID_WITH_ATTRIBUTE_ARRAY;


typedef struct _RID_NAME
{
    DWORD dwRid;
    UNICODE_STRING Name;
} RID_NAME, *PRID_NAME;


typedef struct _RID_NAME_ARRAY
{
    DWORD dwCount;
#ifdef _DCE_IDL_
    [size_is(dwCount)]
#endif
    PRID_NAME pEntries;
} RID_NAME_ARRAY, *PRID_NAME_ARRAY;


typedef struct _ENTRY
{
    DWORD dwIndex;
    UNICODE_STRING Name;
} ENTRY, *PENTRY;

typedef struct _ENTRY_ARRAY
{
    DWORD dwCount;
#ifdef _DCE_IDL_
    [size_is(dwCount)]
#endif
    PENTRY pEntries;
} ENTRY_ARRAY, *PENTRY_ARRAY;


/*
 * Samr context handles
 */

typedef
#ifdef _DCE_IDL_
[context_handle]
#endif
void* CONNECT_HANDLE;

typedef
#ifdef _DCE_IDL_
[context_handle]
#endif
void* DOMAIN_HANDLE;

typedef
#ifdef _DCE_IDL_
[context_handle]
#endif
void* ACCOUNT_HANDLE;


#ifndef _DCE_IDL_

typedef
void* SAMR_BINDING;

typedef
SAMR_BINDING *PSAMR_BINDING;


NTSTATUS
SamrInitBindingDefault(
    OUT PSAMR_BINDING  phBinding,
    IN  PCWSTR         pwszHostname,
    IN  PIO_CREDS      pCreds
    );


NTSTATUS
SamrInitBindingFull(
    OUT PSAMR_BINDING  phBinding,
    IN  PCWSTR         pwszProtSeq,
    IN  PCWSTR         pwszHostname,
    IN  PCWSTR         pwszEndpoint,
    IN  PCWSTR         pwszUuid,
    IN  PCWSTR         pwszOptions,
    IN  PIO_CREDS      pCreds
    );


#define SamrInitBindingFromBindingString(binding_ptr, binding_str, creds_ptr) \
    RpcInitBindingFromBindingString((handle_t*)(binding_ptr),                 \
                                    (binding_str),                            \
                                    (creds_ptr));


VOID
SamrFreeBinding(
    IN  PSAMR_BINDING  phBinding
    );


NTSTATUS
SamrConnect2(
    IN  SAMR_BINDING    hSamrBinding,
    IN  PCWSTR          pwszSysName,
    IN  UINT32          AccessMask,
    OUT CONNECT_HANDLE *phConn
    );

NTSTATUS
SamrConnect3(
    IN  SAMR_BINDING    hSamrBinding,
    IN  PCWSTR          pwszSysName,
    IN  UINT32          AccessMask,
    OUT CONNECT_HANDLE *phConn
    );

NTSTATUS
SamrConnect4(
    IN  SAMR_BINDING    hSamrBinding,
    IN  PCWSTR          pwszSysName,
    IN  UINT32          ClientVersion,
    IN  UINT32          AccessMask,
    OUT CONNECT_HANDLE *phConn
    );

NTSTATUS
SamrConnect5(
    IN  SAMR_BINDING     hSamrBinding,
    IN  PCWSTR           pwszSysName,
    IN  UINT32           AccessMask,
    IN  UINT32           LevelIn,
    IN  SamrConnectInfo *pInfoIn,
    IN  PUINT32          pLevelOut,
    OUT SamrConnectInfo *pInfoOut,
    OUT CONNECT_HANDLE  *phConn
    );

NTSTATUS
SamrClose(
    IN  SAMR_BINDING  hBinding,
    IN  void         *hIn
    );

NTSTATUS
SamrQuerySecurity(
    IN  SAMR_BINDING                   hSamrBinding,
    IN  void                          *hObject,
    IN  ULONG                          ulSecurityInfo,
    OUT PSECURITY_DESCRIPTOR_RELATIVE *ppSecDesc,
    OUT PUINT32                        pulSecDescLen
    );

NTSTATUS
SamrLookupDomain(
    IN  SAMR_BINDING    hSamrBinding,
    IN  CONNECT_HANDLE  hConn,
    IN  PCWSTR          pwszDomainName,
    OUT PSID           *ppSid
    );

NTSTATUS
SamrOpenDomain(
    IN  SAMR_BINDING    hSamrBinding,
    IN  CONNECT_HANDLE  hConn,
    IN  UINT32          AccessMask,
    IN  PSID            pSid,
    OUT DOMAIN_HANDLE  *phDomain
    );

NTSTATUS
SamrQueryDomainInfo(
    IN  SAMR_BINDING    hSamrBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  UINT16          Level,
    OUT DomainInfo    **ppInfo
    );

NTSTATUS
SamrLookupNames(
    IN  SAMR_BINDING    hSamrBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  UINT32          NumNames,
    IN  PWSTR          *ppwszNames,
    OUT UINT32        **ppRids,
    OUT UINT32        **ppTypes,
    OUT UINT32         *pRidsCount
    );

NTSTATUS
SamrOpenUser(
    IN  SAMR_BINDING    hSamrBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  UINT32          AccessMask,
    IN  UINT32          Rid,
    OUT ACCOUNT_HANDLE *phUser
    );

NTSTATUS
SamrQueryUserInfo(
    IN  SAMR_BINDING     hSamrBinding,
    IN  ACCOUNT_HANDLE   hUser,
    IN  UINT16           Level,
    OUT UserInfo       **ppInfo
    );

NTSTATUS
SamrEnumDomainUsers(
    IN  SAMR_BINDING     hSamrBinding,
    IN  ACCOUNT_HANDLE   hDomain,
    IN OUT UINT32       *pResume,
    IN  UINT32           AccountFlags,
    IN  UINT32           MaxSize,
    OUT PWSTR          **pppwszNames,
    OUT UINT32         **ppRids,
    OUT UINT32          *pCount
    );

NTSTATUS
SamrChangePasswordUser2(
    IN  SAMR_BINDING  hBinding,
    IN  PCWSTR        pwszHostname,
    IN  PCWSTR        pwszAccount,
    IN  BYTE          NtPassword[516],
    IN  BYTE          NtVerify[16],
    IN  BYTE          bLmChange,
    IN  BYTE          LmPassword[516],
    IN  BYTE          LmVerify[16]
    );

NTSTATUS
SamrEnumDomains(
    IN  SAMR_BINDING     hSamrBinding,
    IN  CONNECT_HANDLE   hConn,
    IN OUT UINT32       *pResume,
    IN  UINT32           Size,
    OUT PWSTR          **pppwszNames,
    OUT UINT32          *pCount
    );

NTSTATUS
SamrCreateUser(
    IN  SAMR_BINDING    hSamrBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  PWSTR           pwszAccountName,
    IN  UINT32          AccessMask,
    OUT ACCOUNT_HANDLE *phUser,
    OUT PUINT32         pRid
    );

NTSTATUS
SamrDeleteUser(
    IN  SAMR_BINDING    hSamrBinding,
    IN  ACCOUNT_HANDLE  hUser
    );

NTSTATUS
SamrSetUserInfo(
    IN  SAMR_BINDING    hSamrBinding,
    IN  ACCOUNT_HANDLE  hUser,
    IN  UINT16          Level,
    IN  UserInfo       *pInfo
    );

NTSTATUS
SamrSetUserInfo2(
    IN  SAMR_BINDING    hSamrBinding,
    IN  ACCOUNT_HANDLE  hUser,
    IN  UINT16          Level,
    IN  UserInfo       *pInfo
    );

NTSTATUS
SamrCreateDomAlias(
    IN  SAMR_BINDING    hSamrBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  PWSTR           pwszAliasName,
    IN  UINT32          AccessMask,
    OUT ACCOUNT_HANDLE *phAlias,
    OUT PUINT32         pRid
    );

NTSTATUS
SamrDeleteDomAlias(
    IN  SAMR_BINDING    hSamrBinding,
    IN  ACCOUNT_HANDLE  hAlias
    );

NTSTATUS
SamrOpenAlias(
    IN  SAMR_BINDING    hSamrBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  UINT32          AccessMask,
    IN  UINT32          Rid,
    OUT ACCOUNT_HANDLE *phAlias
    );

NTSTATUS
SamrQueryAliasInfo(
    IN  SAMR_BINDING     hSamrBinding,
    IN  ACCOUNT_HANDLE   hAlias,
    IN  UINT16           Level,
    OUT AliasInfo      **ppInfo
    );

NTSTATUS
SamrSetAliasInfo(
    IN  SAMR_BINDING    hSamrBinding,
    IN  ACCOUNT_HANDLE  hAlias,
    IN  UINT16          Level,
    IN  AliasInfo      *pInfo
    );

NTSTATUS
SamrEnumDomainAliases(
    IN  SAMR_BINDING  hSamrBinding,
    IN  DOMAIN_HANDLE hDomain,
    IN  PUINT32       pResume,
    IN  UINT32        AccountFlags,
    OUT PWSTR       **pppwszNames,
    OUT PUINT32      *ppRids,
    OUT PUINT32       pCount
    );

NTSTATUS
SamrGetAliasMembership(
    IN  SAMR_BINDING   hSamrBinding,
    IN  DOMAIN_HANDLE  hDomain,
    IN  PSID          *ppSids,
    IN  DWORD          dwNumSids,
    OUT PDWORD        *ppdwRids,
    OUT PDWORD         pdwCount
    );

NTSTATUS
SamrGetMembersInAlias(
    IN  SAMR_BINDING     hSamrBinding,
    IN  ACCOUNT_HANDLE   hAlias,
    OUT PSID           **pppSids,
    OUT UINT32          *pCount
    );

NTSTATUS
SamrAddAliasMember(
    IN  SAMR_BINDING   hSamrBinding,
    IN  ACCOUNT_HANDLE hAlias,
    IN  PSID           pSid
    );

NTSTATUS
SamrDeleteAliasMember(
    IN  SAMR_BINDING   hSamrBinding,
    IN  ACCOUNT_HANDLE hAlias,
    IN  PSID           pSid
    );

NTSTATUS
SamrCreateUser2(
    IN  SAMR_BINDING    hSamrBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  PWSTR           pwszAccountName,
    IN  UINT32          AccountFlags,
    IN  UINT32          AccessMask,
    OUT ACCOUNT_HANDLE *phUser,
    OUT PUINT32         pAccessGranted,
    OUT PUINT32         pRid
    );

NTSTATUS
SamrGetUserPwInfo(
    IN  SAMR_BINDING    hSamrBinding,
    IN  ACCOUNT_HANDLE  hUser,
    OUT PwInfo         *pInfo
    );

NTSTATUS
SamrLookupRids(
    IN  SAMR_BINDING    hSamrBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  UINT32          NumRids,
    IN  UINT32         *pRids,
    OUT PWSTR         **pppwszNames,
    OUT UINT32        **ppTypes
    );

NTSTATUS
SamrGetUserGroups(
    IN  SAMR_BINDING     hSamrBinding,
    IN  ACCOUNT_HANDLE   hUser,
    OUT UINT32         **ppRids,
    OUT UINT32         **ppAttributes,
    OUT UINT32          *pCount
    );

NTSTATUS
SamrQueryDisplayInfo(
    IN  SAMR_BINDING      hSamrBinding,
    IN  DOMAIN_HANDLE     hDomain,
    IN  UINT16            Level,
    IN  UINT32            StartIdx,
    IN  UINT32            MaxEntries,
    IN  UINT32            BufferSize,
    OUT UINT32           *pTotalSize,
    OUT UINT32           *pReturnedSize,
    OUT SamrDisplayInfo **ppInfo
    );

NTSTATUS
SamrCreateDomGroup(
    IN  SAMR_BINDING    hSamrBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  PWSTR           pwszGroupName,
    IN  UINT32          AccessMask,
    OUT ACCOUNT_HANDLE *phGroup,
    OUT PUINT32         pRid
    );

NTSTATUS
SamrDeleteDomGroup(
    IN  SAMR_BINDING    hSamrBinding,
    IN  ACCOUNT_HANDLE  hGroup
    );

NTSTATUS
SamrOpenGroup(
    IN  SAMR_BINDING    hSamrBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  UINT32          AccessMask,
    IN  UINT32          Rid,
    OUT ACCOUNT_HANDLE *phGroup
    );


VOID
SamrFreeMemory(
    PVOID pPtr
    );


#endif /* _DCE_IDL_ */

#endif /* _RPC_SAMR_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
