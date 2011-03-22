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
 *        lsa.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Lsa rpc client library
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _RPC_LSA_H_
#define _RPC_LSA_H_


#define LSA_LOOKUP_NAMES_ALL                   1
#define LSA_LOOKUP_NAMES_DOMAINS_ONLY          2
#define LSA_LOOKUP_NAMES_PRIMARY_DOMAIN_ONLY   3
#define LSA_LOOKUP_NAMES_UPLEVEL_TRUSTS_ONLY   4
#define LSA_LOOKUP_NAMES_FOREST_TRUSTS         5
#define LSA_LOOKUP_NAMES_UPLEVEL_TRUSTS_ONLY2  6

#define LSA_ACCESS_LOOKUP_NAMES_SIDS           0x00000800
#define LSA_ACCESS_ENABLE_LSA                  0x00000400
#define LSA_ACCESS_ADMIN_AUDIT_LOG_ATTRS       0x00000200
#define LSA_ACCESS_CHANGE_SYS_AUDIT_REQS       0x00000100
#define LSA_ACCESS_SET_DEFAULT_QUOTA           0x00000080
#define LSA_ACCESS_CREATE_PRIVILEGE            0x00000040
#define LSA_ACCESS_CREATE_SECRET_OBJECT        0x00000020
#define LSA_ACCESS_CREATE_SPECIAL_ACCOUNTS     0x00000010
#define LSA_ACCESS_CHANGE_DOMTRUST_RELATION    0x00000008
#define LSA_ACCESS_GET_SENSITIVE_POLICY_INFO   0x00000004
#define LSA_ACCESS_VIEW_SYS_AUDIT_REQS         0x00000002
#define LSA_ACCESS_VIEW_POLICY_INFO            0x00000001

#define LSA_ACCOUNT_VIEW                       0x00000001
#define LSA_ACCOUNT_ADJUST_PRIVILEGES          0x00000002
#define LSA_ACCOUNT_ADJUST_QUOTAS              0x00000004
#define LSA_ACCOUNT_ADJUST_SYSTEM_ACCESS       0x00000008


typedef struct _object_attribute {
    ULONG len;
    PBYTE root_dir;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR object_name;
    ULONG attributes;
    PBYTE sec_desc;
    PSECURITY_QUALITY_OF_SERVICE sec_qos;
} ObjectAttribute;

typedef struct audit_log_info {
    UINT32 percent_full;
    UINT32 log_size;
    LONG64 retention_time;
    UINT8  shutdown_in_progress;
    LONG64 time_to_shutdown;
    UINT32 next_audit_record;
    UINT32 unknown;
} AuditLogInfo;

typedef struct audit_events_info {
    UINT32 auditing_mode;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    UINT32 *settings;
    UINT32 count;
} AuditEventsInfo;

typedef struct lsa_domain_info {
    UNICODE_STRING name;
    PSID sid;
} LsaDomainInfo;

typedef struct pd_account_info {
    UNICODE_STRING name;
} PDAccountInfo;

typedef struct server_role {
    UINT16 unknown;
    UINT16 role;
} ServerRole;

typedef struct replica_source_info {
    UNICODE_STRING source;
    UNICODE_STRING account;
} ReplicaSourceInfo;

typedef struct default_quota_info {
    UINT32 paged_pool;
    UINT32 non_paged_pool;
    UINT32 min_wss;
    UINT32 max_wss;
    UINT32 pagefile;
    UINT64 unknown;
} DefaultQuotaInfo;

typedef struct modification_info {
    UINT64 modified_id;
    LONG64 db_create_time;
} ModificationInfo;

typedef struct audit_full_set_info {
    UINT8 shutdown_on_full;
} AuditFullSetInfo;

typedef struct audit_full_query_info {
    UINT16 unknown;
    UINT8 shutdown_on_full;
    UINT8 log_is_full;
} AuditFullQueryInfo;

typedef struct _guid {
    UINT32 time_low;
    UINT16 time_mid;
    UINT16 time_hi_and_version;
    UINT8  clock_seq[2];
    UINT8  node[6];
} Guid;

typedef struct dns_domain_info {
    UNICODE_STRING name;
    UNICODE_STRING dns_domain;
    UNICODE_STRING dns_forest;
    Guid domain_guid;
    PSID sid;
} DnsDomainInfo;


#define LSA_POLICY_INFO_AUDIT_LOG          1
#define LSA_POLICY_INFO_AUDIT_EVENTS       2
#define LSA_POLICY_INFO_DOMAIN             3
#define LSA_POLICY_INFO_PD                 4
#define LSA_POLICY_INFO_ACCOUNT_DOMAIN     5
#define LSA_POLICY_INFO_ROLE               6
#define LSA_POLICY_INFO_REPLICA            7
#define LSA_POLICY_INFO_QUOTA              8
#define LSA_POLICY_INFO_DB                 9
#define LSA_POLICY_INFO_AUDIT_FULL_SET    10
#define LSA_POLICY_INFO_AUDIT_FULL_QUERY  11
#define LSA_POLICY_INFO_DNS               12


#ifndef _DCE_IDL_
typedef union lsa_policy_information { 
    AuditLogInfo        audit_log;
    AuditEventsInfo     audit_events;
    LsaDomainInfo       domain;
    PDAccountInfo       pd;
    LsaDomainInfo       account_domain;
    ServerRole          role;
    ReplicaSourceInfo   replica;
    DefaultQuotaInfo    quota;
    ModificationInfo    db;
    AuditFullSetInfo    audit_set;
    AuditFullQueryInfo  audit_query;
    DnsDomainInfo       dns;
} LsaPolicyInformation;
#endif


typedef UINT16 LsaSidType;

#define SID_TYPE_USE_NONE   0
#define SID_TYPE_USER       1
#define SID_TYPE_DOM_GRP    2
#define SID_TYPE_DOMAIN     3
#define SID_TYPE_ALIAS      4
#define SID_TYPE_WKN_GRP    5
#define SID_TYPE_DELETED    6
#define SID_TYPE_INVALID    7
#define SID_TYPE_UNKNOWN    8
#define SID_TYPE_COMPUTER   9
#define SID_TYPE_LABEL      10


typedef struct ref_domain_list {
#ifdef _DCE_IDL_
    [range(0,1000)]
#endif
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    LsaDomainInfo *domains;
    UINT32 max_size;
} RefDomainList;


typedef struct translated_sid {
    UINT16 type;     /* SID_TYPE_ */
    UINT32 rid;
    UINT32 index;
} TranslatedSid;


typedef struct translated_sid_array {
#ifdef _DCE_IDL_
    [range(0,1000)]
#endif
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    TranslatedSid *sids;
} TranslatedSidArray;


typedef struct translated_sid2 {
    UINT16 type;     /* SID_TYPE_ */
    UINT32 rid;
    UINT32 index;
    UINT32 unknown1;
} TranslatedSid2;


typedef struct translated_sid_array2 {
#ifdef _DCE_IDL_
    [range(0,1000)]
#endif
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    TranslatedSid2 *sids;
} TranslatedSidArray2;


typedef struct translated_sid3 {
    UINT16 type;     /* SID_TYPE_ */
    PSID   sid;
    UINT32 index;
    UINT32 unknown1;
} TranslatedSid3;


typedef struct translated_sid_array3 {
#ifdef _DCE_IDL_
    [range(0,1000)]
#endif
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    TranslatedSid3 *sids;
} TranslatedSidArray3;


typedef struct translated_name {
    UINT16 type;             /* SID_TYPE_ */
    UNICODE_STRING name;
    UINT32 sid_index;
} TranslatedName;


typedef struct translated_name_array {
#ifdef _DCE_IDL_
    [range(0,1000)]
#endif
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    TranslatedName *names;
} TranslatedNameArray;


typedef struct translated_name2 {
    UINT16 type;             /* SID_TYPE_ */
    UNICODE_STRING name;
    UINT32 sid_index;
    UINT32 unknown1;
} TranslatedName2;


typedef struct translated_name_array2 {
#ifdef _DCE_IDL_
    [range(0,1000)]
#endif
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    TranslatedName2 *names;
} TranslatedNameArray2;


typedef struct _SID_PTR
{
    PSID pSid;
} SID_PTR, *PSID_PTR;


typedef struct _SID_ARRAY
{
#ifdef _DCE_IDL_
    [range(0,1000)]
#endif
    DWORD dwNumSids;
#ifdef _DCE_IDL_
    [size_is(dwNumSids)]
#endif
    SID_PTR* pSids;
} SID_ARRAY, *PSID_ARRAY;


typedef struct _LSA_POLICY_PRIVILEGE_DEF {
    UNICODE_STRING Name;
    LUID Value;
} LSA_POLICY_PRIVILEGE_DEF, *PLSA_POLICY_PRIVILEGE_DEF;


typedef struct _LSA_PRIVILEGE_ENUM_BUFFER {
    DWORD NumPrivileges;
#ifdef _DCE_IDL_
    [size_is(NumPrivileges)]
#endif
    PLSA_POLICY_PRIVILEGE_DEF pPrivilege;
} LSA_PRIVILEGE_ENUM_BUFFER, *PLSA_PRIVILEGE_ENUM_BUFFER;


typedef struct _LSA_ACCOUNT_RIGHTS {
    DWORD NumAccountRights;
#ifdef _DCE_IDL_
    [size_is(NumAccountRights)]
#endif
    PUNICODE_STRING pAccountRight;
} LSA_ACCOUNT_RIGHTS, *PLSA_ACCOUNT_RIGHTS;


typedef struct _LSA_ACCOUNT_INFO {
    PSID pSid;
} LSA_ACCOUNT_INFO, *PLSA_ACCOUNT_INFO;


typedef struct _LSA_ACCOUNT_ENUM_BUFFER {
    DWORD NumAccounts;
#ifdef _DCE_IDL_
    [size_is(NumAccounts)]
#endif
    PLSA_ACCOUNT_INFO pAccount;
} LSA_ACCOUNT_ENUM_BUFFER, *PLSA_ACCOUNT_ENUM_BUFFER;


typedef struct _LSA_SECURITY_DESCRIPTOR_BUFFER
{
    DWORD BufferLen;
#ifdef _DCE_IDL_
    [size_is(BufferLen)]
#endif
    PBYTE pBuffer;
}
LSA_SECURITY_DESCRIPTOR_BUFFER, *PLSA_SECURITY_DESCRIPTOR_BUFFER;


typedef
#ifdef _DCE_IDL_
[context_handle]
#endif
void* POLICY_HANDLE;

typedef
#ifdef _DCE_IDL_
[context_handle]
#endif
void* LSAR_ACCOUNT_HANDLE;


#ifndef _DCE_IDL_

typedef
void* LSA_BINDING;

typedef
LSA_BINDING *PLSA_BINDING;


NTSTATUS
LsaInitBindingDefault(
    OUT PLSA_BINDING   phBinding,
    IN  PCWSTR         pwszHostname,
    IN  LW_PIO_CREDS   pCreds
    );


NTSTATUS
LsaInitBindingFull(
    OUT PLSA_BINDING   phBinding,
    IN  PCWSTR         pwszProtSeq,
    IN  PCWSTR         pwszHostname,
    IN  PCWSTR         pwszEndpoint,
    IN  PCWSTR         pwszUuid,
    IN  PCWSTR         pwszOptions,
    IN  LW_PIO_CREDS   pCreds
    );


#define LsaInitBindingFromBindingString(binding_ptr, binding_str, creds_ptr) \
    RpcInitBindingFromBindingString((handle_t*)(binding_ptr),                \
                                    (binding_str),                           \
                                    (creds_ptr));


VOID
LsaFreeBinding(
    IN  PLSA_BINDING   phBinding
    );


NTSTATUS
LsaOpenPolicy2(
    IN  LSA_BINDING    hBinding,
    IN  PCWSTR         pwszSysname,
    IN  PVOID          attrib,
    IN  UINT32         AccessMask,
    OUT POLICY_HANDLE *phPolicy
    );


NTSTATUS
LsaClose(
    IN  LSA_BINDING    hBinding,
    IN  void*          hObject
    );


NTSTATUS
LsaQueryInfoPolicy(
    IN  LSA_BINDING            hBinding,
    IN  POLICY_HANDLE          hPolicy,
    IN  WORD                   swLevel,
    OUT LsaPolicyInformation **ppInfo
    );


NTSTATUS
LsaQueryInfoPolicy2(
    IN  LSA_BINDING            hBinding,
    IN  POLICY_HANDLE          hPolicy,
    IN  WORD                   swLevel,
    OUT LsaPolicyInformation **ppInfo
    );


NTSTATUS
LsaLookupNames(
    IN  LSA_BINDING     hBinding,
    IN  POLICY_HANDLE   hPolicy,
    IN  DWORD           dwNumNames,
    IN  PWSTR          *ppwszNames,
    OUT RefDomainList **ppDomList,
    OUT TranslatedSid **ppSids,
    IN  WORD            swLevel,
    IN OUT PDWORD       pdwCount
    );


NTSTATUS
LsaLookupNames2(
    IN  LSA_BINDING      hBinding,
    IN  POLICY_HANDLE    hPolicy,
    IN  DWORD            dwNumNames,
    IN  PWSTR           *ppNames,
    OUT RefDomainList  **ppDomList,
    OUT TranslatedSid2 **ppSids,
    IN  WORD             swLevel,
    IN OUT PDWORD        pdwCount
    );


NTSTATUS
LsaLookupNames3(
    IN  LSA_BINDING      hBinding,
    IN  POLICY_HANDLE    hPolicy,
    IN  DWORD            dwNumNames,
    IN  PWSTR           *ppNames,
    OUT RefDomainList  **ppDomList,
    OUT TranslatedSid3 **ppSids,
    IN  WORD             swLevel,
    IN OUT PDWORD        pdwCount
    );


NTSTATUS
LsaLookupSids(
    IN  LSA_BINDING      hBinding,
    IN  POLICY_HANDLE    hPolicy,
    IN  PSID_ARRAY       pSids,
    OUT RefDomainList  **ppRefDomList,
    OUT TranslatedName **ppTransNames,
    IN  WORD             swLevel,
    IN OUT PDWORD        pdwCount
    );


NTSTATUS
LsaCreateAccount(
    IN  LSA_BINDING            hBinding,
    IN  POLICY_HANDLE          hPolicy,
    IN  PSID                   pAccountSid,
    IN  DWORD                  AccessMask,
    OUT LSAR_ACCOUNT_HANDLE *phAccount
    );


NTSTATUS
LsaOpenAccount(
    IN  LSA_BINDING             hBinding,
    IN  POLICY_HANDLE           hPolicy,
    IN  PSID                    pAccountSid,
    IN  DWORD                   AccessMask,
    OUT LSAR_ACCOUNT_HANDLE  *phAccount
    );


NTSTATUS
LsaEnumPrivileges(
    IN LSA_BINDING     hBinding,
    IN POLICY_HANDLE   hPolicy,
    IN OUT PDWORD      pResume,
    IN DWORD           PreferredMaxSize,
    OUT PWSTR        **ppNames,
    OUT PLUID         *pValues,
    OUT PDWORD         pNumPrivileges
    );


NTSTATUS
LsaEnumAccounts(
    IN LSA_BINDING     hBinding,
    IN POLICY_HANDLE   hPolicy,
    IN OUT PDWORD      pResume,
    OUT PSID         **pppAccounts,
    OUT PDWORD         pNumAccounts,
    IN DWORD           PreferredMaxSize
    );


NTSTATUS
LsaEnumPrivilegesAccount(
    IN  LSA_BINDING          hBinding,
    IN  LSAR_ACCOUNT_HANDLE  hAccount,
    OUT PPRIVILEGE_SET      *ppPrivileges
    );


NTSTATUS
LsaAddPrivilegesToAccount(
    IN  LSA_BINDING          hBinding,
    IN  LSAR_ACCOUNT_HANDLE  hAccount,
    IN  PPRIVILEGE_SET       pPrivileges
    );


NTSTATUS
LsaRemovePrivilegesFromAccount(
    IN  LSA_BINDING          hBinding,
    IN  LSAR_ACCOUNT_HANDLE  hAccount,
    IN  BOOLEAN              AllPrivileges,
    IN  PPRIVILEGE_SET       pPrivileges
    );


NTSTATUS
LsaGetSystemAccessAccount(
    IN  LSA_BINDING          hBinding,
    IN  LSAR_ACCOUNT_HANDLE  hAccount,
    OUT PDWORD               pSystemAccess
    );


NTSTATUS
LsaSetSystemAccessAccount(
    IN  LSA_BINDING          hBinding,
    IN  LSAR_ACCOUNT_HANDLE  hAccount,
    IN  DWORD                SystemAccess
    );


NTSTATUS
LsaLookupPrivilegeValue(
    IN  LSA_BINDING      hBinding,
    IN  POLICY_HANDLE    hPolicy,
    IN  PWSTR            pwszName,
    OUT PLUID            pLuid
    );


NTSTATUS
LsaLookupPrivilegeName(
    IN  LSA_BINDING      hBinding,
    IN  POLICY_HANDLE    hPolicy,
    IN  PLUID            pValue,
    OUT PWSTR           *ppwszName
    );


NTSTATUS
LsaLookupPrivilegeDisplayName(
    IN LSA_BINDING      hBinding,
    IN POLICY_HANDLE    hPolicy,
    IN PWSTR            pwszName,
    IN SHORT            ClientLanguage,
    IN SHORT            ClientSystemLanguage,
    OUT PWSTR          *ppwszDisplayName,
    OUT PUSHORT          pLanguage
    );


NTSTATUS
LsaEnumAccountsWithUserRight(
    IN LSA_BINDING     hBinding,
    IN POLICY_HANDLE   hPolicy,
    IN PCWSTR          UserRight,
    OUT PSID         **pppAccounts,
    OUT PDWORD         pNumAccounts
    );


NTSTATUS
LsaEnumAccountRights(
    IN LSA_BINDING     hBinding,
    IN POLICY_HANDLE   hPolicy,
    IN PSID            pAccountSid,
    OUT PWSTR        **ppAccountRights,
    OUT PDWORD         pNumAccountRights
    );


NTSTATUS
LsaAddAccountRights(
    IN LSA_BINDING     hBinding,
    IN POLICY_HANDLE   hPolicy,
    IN PSID            pAccountSid,
    IN PWSTR          *pAccountRights,
    IN DWORD           NumAccountRights
    );


NTSTATUS
LsaRemoveAccountRights(
    IN LSA_BINDING     hBinding,
    IN POLICY_HANDLE   hPolicy,
    IN PSID            pAccountSid,
    IN BOOLEAN         RemoveAll,
    IN PWSTR          *pAccountRights,
    IN DWORD           NumAccountRights
    );


NTSTATUS
LsaQuerySecurity(
    IN  LSA_BINDING                    hBinding,
    IN  void                          *hObject,
    IN  DWORD                          SecurityInfo,
    OUT PSECURITY_DESCRIPTOR_RELATIVE *ppSecDesc,
    OUT PDWORD                         pSecDescLen
    );


NTSTATUS
LsaSetSecurity(
    IN LSA_BINDING                    hBinding,
    IN void                          *hObject,
    IN DWORD                          SecurityInfo,
    IN PSECURITY_DESCRIPTOR_RELATIVE  pSecDesc,
    IN DWORD                          SecDescLen
    );


NTSTATUS
LsaRpcDeleteObject(
    IN  LSA_BINDING  hBinding,
    IN  PVOID        hObject
    );


VOID
LsaRpcFreeMemory(
    IN PVOID pPtr
    );


#endif /* _DCE_IDL_ */

#endif /* _RPC_LSA_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
