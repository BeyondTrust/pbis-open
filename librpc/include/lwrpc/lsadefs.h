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
 * Abstract: Lsa interface definitions (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _LSADEFS_H_
#define _LSADEFS_H_

#include <lwrpc/types.h>
#include <lwrpc/unistrdef.h>


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
    NtTime retention_time;
    UINT8  shutdown_in_progress;
    NtTime time_to_shutdown;
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
    UnicodeStringEx name;
    PSID sid;
} LsaDomainInfo;

typedef struct pd_account_info {
    UnicodeString name;
} PDAccountInfo;

typedef struct server_role {
    UINT16 unknown;
    UINT16 role;
} ServerRole;

typedef struct replica_source_info {
    UnicodeString source;
    UnicodeString account;
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
    NtTime db_create_time;
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
    UnicodeStringEx name;
    UnicodeStringEx dns_domain;
    UnicodeStringEx dns_forest;
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
    UnicodeString name;
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
    UnicodeString name;
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

typedef struct _sid_ptr {
    PSID sid;
} SidPtr;

typedef struct _sid_array {
#ifdef _DCE_IDL_
    [range(0,1000)]
#endif
    ULONG num_sids;
#ifdef _DCE_IDL_
    [size_is(num_sids)]
#endif
    SidPtr* sids;
} SidArray;


typedef
#ifdef _DCE_IDL_
[context_handle]
#endif
void* POLICY_HANDLE;


#endif /* _LSADEFS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
