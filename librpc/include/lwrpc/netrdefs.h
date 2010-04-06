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
 * Abstract: Netlogon interface definitions (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _NETRDEFS_H_
#define _NETRDEFS_H_

#include <lwrpc/types.h>
#include <lwrpc/unistrdef.h>
#include <lwrpc/rid.h>
#include <lwrpc/samrdefs.h>
#include <lwrpc/lsadefs.h>


/*
 * Domain trust definitions
 */

/* Netlogon challenge/response negotiation flags */
#define NETLOGON_NEG_ACCOUNT_LOCKOUT	         0x00000001
#define NETLOGON_NEG_PERSISTENT_SAMREPL          0x00000002
#define NETLOGON_NEG_ARCFOUR                     0x00000004
#define NETLOGON_NEG_PROMOTION_COUNT             0x00000008
#define NETLOGON_NEG_CHANGELOG_BDC               0x00000010
#define NETLOGON_NEG_FULL_SYNC_REPL              0x00000020
#define NETLOGON_NEG_MULTIPLE_SIDS               0x00000040
#define NETLOGON_NEG_REDO                        0x00000080
#define NETLOGON_NEG_PASSWORD_CHANGE_REFUSAL     0x00000100
#define NETLOGON_NEG_SEND_PASSWORD_INFO_PDC      0x00000200
#define NETLOGON_NEG_GENERIC_PASSTHROUGH         0x00000400
#define NETLOGON_NEG_CONCURRENT_RPC              0x00000800
#define NETLOGON_NEG_AVOID_ACCOUNT_DB_REPL       0x00001000
#define NETLOGON_NEG_AVOID_SECURITYAUTH_DB_REPL  0x00002000
#define NETLOGON_NEG_128BIT                      0x00004000
#define NETLOGON_NEG_TRANSITIVE_TRUSTS           0x00008000
#define NETLOGON_NEG_DNS_DOMAIN_TRUSTS           0x00010000
#define NETLOGON_NEG_PASSWORD_SET2               0x00020000
#define NETLOGON_NEG_GETDOMAININFO               0x00040000
#define NETLOGON_NEG_CROSS_FOREST_TRUSTS         0x00080000
#define NETLOGON_NEG_NEUTRALIZE_NT4_EMULATION    0x00100000
#define NETLOGON_NEG_RODC_PASSTHROUGH            0x00200000
#define NETLOGON_NEG_AUTHENTICATED_RPC_LSASS     0x20000000
#define NETLOGON_NEG_SCHANNEL                    0x40000000

#define NETLOGON_NEG_AUTH2_FLAGS   (NETLOGON_NEG_ACCOUNT_LOCKOUT         | \
                                    NETLOGON_NEG_PERSISTENT_SAMREPL      | \
                                    NETLOGON_NEG_ARCFOUR                 | \
                                    NETLOGON_NEG_PROMOTION_COUNT         | \
                                    NETLOGON_NEG_CHANGELOG_BDC           | \
                                    NETLOGON_NEG_FULL_SYNC_REPL          | \
                                    NETLOGON_NEG_MULTIPLE_SIDS           | \
                                    NETLOGON_NEG_REDO                    | \
                                    NETLOGON_NEG_PASSWORD_CHANGE_REFUSAL | \
                                    NETLOGON_NEG_DNS_DOMAIN_TRUSTS       | \
                                    NETLOGON_NEG_PASSWORD_SET2           | \
                                    NETLOGON_NEG_GETDOMAININFO)

#define NETLOGON_NET_ADS_FLAGS   (NETLOGON_NEG_ACCOUNT_LOCKOUT             | \
                                  NETLOGON_NEG_PERSISTENT_SAMREPL          | \
                                  NETLOGON_NEG_ARCFOUR                     | \
                                  NETLOGON_NEG_PROMOTION_COUNT             | \
                                  NETLOGON_NEG_CHANGELOG_BDC               | \
                                  NETLOGON_NEG_FULL_SYNC_REPL              | \
                                  NETLOGON_NEG_MULTIPLE_SIDS               | \
                                  NETLOGON_NEG_REDO                        | \
                                  NETLOGON_NEG_PASSWORD_CHANGE_REFUSAL     | \
                                  NETLOGON_NEG_SEND_PASSWORD_INFO_PDC      | \
                                  NETLOGON_NEG_GENERIC_PASSTHROUGH         | \
                                  NETLOGON_NEG_CONCURRENT_RPC              | \
                                  NETLOGON_NEG_AVOID_ACCOUNT_DB_REPL       | \
                                  NETLOGON_NEG_AVOID_SECURITYAUTH_DB_REPL  | \
                                  NETLOGON_NEG_128BIT                      | \
                                  NETLOGON_NEG_TRANSITIVE_TRUSTS           | \
                                  NETLOGON_NEG_DNS_DOMAIN_TRUSTS           | \
                                  NETLOGON_NEG_PASSWORD_SET2               | \
                                  NETLOGON_NEG_GETDOMAININFO               | \
                                  NETLOGON_NEG_AUTHENTICATED_RPC_LSASS     | \
                                  NETLOGON_NEG_SCHANNEL)


/* Netlogon trust flags */
#define NETR_TRUST_FLAG_IN_FOREST    0x00000001
#define NETR_TRUST_FLAG_OUTBOUND     0x00000002
#define NETR_TRUST_FLAG_TREEROOT     0x00000004
#define NETR_TRUST_FLAG_PRIMARY      0x00000008
#define NETR_TRUST_FLAG_NATIVE       0x00000010
#define NETR_TRUST_FLAG_INBOUND      0x00000020

/* Netlogon trust type */
#define NETR_TRUST_TYPE_DOWNLEVEL    1
#define NETR_TRUST_TYPE_UPLEVEL      2
#define NETR_TRUST_TYPE_MIT          3
#define NETR_TRUST_TYPE_DCE          4

/* Netlogon trust attributes */
#define NETR_TRUST_ATTR_NON_TRANSITIVE       0x00000001
#define NETR_TRUST_ATTR_UPLEVEL_ONLY         0x00000002
#define NETR_TRUST_ATTR_QUARANTINED_DOMAIN   0x00000004
#define NETR_TRUST_ATTR_FOREST_TRANSITIVE    0x00000008
#define NETR_TRUST_ATTR_CROSS_ORGANIZATION   0x00000010
#define NETR_TRUST_ATTR_WITHIN_FOREST        0x00000020
#define NETR_TRUST_ATTR_TREAT_AS_EXTERNAL    0x00000040


typedef struct _GUID {
    ULONG   ulTimeLow;
    USHORT  usTimeMid;
    USHORT  usTimeHiAndVersion;
    BYTE    ClockSeqHiAndReserved;
    BYTE    ClockSeqLow;
    BYTE    Node[6];
} GUID, *PGUID;

typedef struct netr_domain_trust {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *netbios_name;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *dns_name;
    UINT32 trust_flags;
    UINT32 parent_index;
    UINT16 trust_type;
    UINT32 trust_attrs;
    PSID sid;
    GUID guid;
} NetrDomainTrust;


typedef struct netr_domain_trust_list {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    NetrDomainTrust *array;
} NetrDomainTrustList;


/*
 * Sam netlogon definitions
 */

/* Secure Channel types */
#define SCHANNEL_WKSTA     2
#define SCHANNEL_DOMAIN    4
#define SCHANNEL_BDC       6


/* NetrLogonSamLogon types */
#define NETR_LOGON_TYPE_INTERACTIVE     1
#define NETR_LOGON_TYPE_NETWORK         2


#define MSV1_0_CLEARTEXT_PASSWORD_ALLOWED       (0x00000002)
#define MSV1_0_UPDATE_LOGON_STATISTICS          (0x00000004)
#define MSV1_0_RETURN_USER_PARAMETERS           (0x00000008)
#define MSV1_0_ALLOW_SERVER_TRUST_ACCOUNT       (0x00000020)
#define MSV1_0_RETURN_PROFILE_PATH              (0x00000200)
#define MSV1_0_ALLOW_WORKSTATION_TRUST_ACCOUNT  (0x00000800)


typedef struct netr_cred {
	UINT8 data[8];
} NetrCred;


typedef struct netr_auth {
    NetrCred cred;
    UINT32 timestamp;
} NetrAuth;


typedef struct netr_identity_info {
    UnicodeString domain_name;
    UINT32        param_control;
    UINT32        logon_id_low;
    UINT32        logon_id_high;
    UnicodeString account_name;
    UnicodeString workstation;
} NetrIdentityInfo;


typedef struct netr_password_info {
    NetrIdentityInfo identity;
    HashPassword     lmpassword;
    HashPassword     ntpassword;
} NetrPasswordInfo;


typedef struct netr_challenge_response {
    UINT16 length;
    UINT16 size;
#ifdef _DCE_IDL_
    [size_is(length),length_is(length)]
#endif
    UINT8 *data;
} NetrChallengeResponse;


typedef struct netr_network_info {
    NetrIdentityInfo identity;
    UINT8 challenge[8];
    NetrChallengeResponse nt;
    NetrChallengeResponse lm;
} NetrNetworkInfo;


#ifndef _DCE_IDL_
typedef union netr_logon_info {
    NetrPasswordInfo *password1;
    NetrNetworkInfo  *network2;
    NetrPasswordInfo *password3;
    NetrPasswordInfo *password5;
    NetrNetworkInfo  *network6;
} NetrLogonInfo;
#endif /* _DCE_IDL_ */


typedef struct netr_credentials {
    UINT32 negotiate_flags;
    UINT8  pass_hash[16];
    UINT8  session_key[16];
    UINT16 channel_type;
    UINT32 sequence;

    NetrCred  cli_chal;
    NetrCred  srv_chal;
    NetrCred  seed;
} NetrCredentials;


typedef struct win_nt_time {
    UINT32  low;
    UINT32  high;
} WinNtTime;


typedef struct netr_user_session_key {
    UINT8 key[16];
} NetrUserSessionKey;


typedef struct netr_lm_session_key {
    UINT8 key[8];
} NetrLMSessionKey;


typedef struct netr_sam_base_info {
    WinNtTime last_logon;
    WinNtTime last_logoff;
    WinNtTime acct_expiry;
    WinNtTime last_password_change;
    WinNtTime allow_password_change;
    WinNtTime force_password_change;
    UnicodeStringEx account_name;
    UnicodeStringEx full_name;
    UnicodeStringEx logon_script;
    UnicodeStringEx profile_path;
    UnicodeStringEx home_directory;
    UnicodeStringEx home_drive;
    UINT16 logon_count;
    UINT16 bad_password_count;
    UINT32 rid;
    UINT32 primary_gid;
    RidWithAttributeArray groups;
    UINT32 user_flags;
    NetrUserSessionKey key;
    UnicodeStringEx logon_server;
    UnicodeStringEx domain;
    PSID domain_sid;
    NetrLMSessionKey lmkey;
    UINT32 acct_flags;
    UINT32 unknown[7];
} NetrSamBaseInfo;


typedef struct netr_sam_info2 {
    NetrSamBaseInfo base;
} NetrSamInfo2;


typedef struct netr_sid_attr {
    PSID sid;
    UINT32 attribute;
} NetrSidAttr;


typedef struct netr_sam_info3 {
    NetrSamBaseInfo base;
    UINT32 sidcount;
#ifdef _DCE_IDL_
    [size_is(sidcount)]
#endif
    NetrSidAttr *sids;
} NetrSamInfo3;


typedef struct netr_sam_info6 {
    NetrSamBaseInfo base;
    UINT32 sidcount;
#ifdef _DCE_IDL_
    [size_is(sidcount)]
#endif
    NetrSidAttr *sids;
    UnicodeString forest;
    UnicodeString principal;
    UINT32 unknown[20];
} NetrSamInfo6;


typedef struct netr_pac_info {
    UINT32 pac_size;
#ifdef _DCE_IDL_
    [size_is(pac_size)]
#endif
    UINT8 *pac;
    UnicodeString logon_domain;
    UnicodeString logon_server;
    UnicodeString principal_name;
    UINT32 auth_size;
#ifdef _DCE_IDL_
    [size_is(auth_size)]
#endif
    UINT8 *auth;
    NetrUserSessionKey user_session_key;
    UINT32 expansionroom[10];
    UnicodeString unknown1;
    UnicodeString unknown2;
    UnicodeString unknown3;
    UnicodeString unknown4;
} NetrPacInfo;


#ifndef _DCE_IDL_
typedef union netr_validation_info {
    NetrSamInfo2 *sam2;
    NetrSamInfo3 *sam3;
    NetrPacInfo  *pac4;
    NetrPacInfo  *pac5;
    NetrSamInfo6 *sam6;
} NetrValidationInfo;

#endif /* _DCE_IDL_ */

typedef struct netr_domain_query_1 {
    UnicodeString unknown1;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *workstation_domain;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *workstation_site;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *unknown2;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *unknown3;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *unknown4;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *unknown5;
    UnicodeString unknown6;
    UnicodeString product;
    UnicodeString unknown7;
    UnicodeString unknown8;
    UINT32 unknown9[4];
} NetrDomainQuery1;


#ifndef _DCE_IDL_

typedef union netr_domain_query {
    NetrDomainQuery1 *query1;
    NetrDomainQuery1 *query2;
} NetrDomainQuery;

#endif /* _DCE_IDL_ */

typedef struct netr_domain_trust_info {
    UnicodeString domain_name;
    UnicodeString full_domain_name;
    UnicodeString forest;
    Guid guid;
    PSID sid;
    UnicodeString unknown1[4];
    UINT32 unknown2[4];
} NetrDomainTrustInfo;


typedef struct netr_domain_info_1 {
    NetrDomainTrustInfo domain_info;
    UINT32 num_trusts;
#ifdef _DCE_IDL_
    [size_is(num_trusts)]
#endif
    NetrDomainTrustInfo *trusts;
    UINT32 unknown1[14];
} NetrDomainInfo1;


#ifndef _DCE_IDL_

typedef union netr_domain_info {
    NetrDomainInfo1 *info1;
    NetrDomainInfo1 *info2;
} NetrDomainInfo;

#endif /* _DCE_IDL_ */


/* address type flags */
#define DS_ADDRESS_TYPE_INET             (0x0001)
#define DS_ADDRESS_TYPE_NETBIOS          (0x0002)

/* DC flags */
#define DS_SERVER_PDC                    (0x00000001)
#define DS_SERVER_GC                     (0x00000004)
#define DS_SERVER_LDAP                   (0x00000008)
#define DS_SERVER_DS                     (0x00000010)
#define DS_SERVER_KDC                    (0x00000020)
#define DS_SERVER_TIMESERV               (0x00000040)
#define DS_SERVER_CLOSEST                (0x00000080)
#define DS_SERVER_WRITABLE               (0x00000100)
#define DS_SERVER_GOOD_TIMESERV          (0x00000200)
#define DS_SERVER_NDNC                   (0x00000400)
#define DS_SERVER_SELECT_SECRET_DOMAIN_6 (0x00000800)
#define DS_SERVER_FULL_SECRET_DOMAIN_6   (0x00001000)
#define DS_DNS_CONTROLLER                (0x20000000)
#define DS_DNS_DOMAIN                    (0x40000000)
#define DS_DNS_FOREST                    (0x80000000)
 

typedef struct dsr_dc_name_info {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *dc_name;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *dc_address;
    UINT16 address_type;
    Guid domain_guid;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *domain_name;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *forest_name;
    UINT32 flags;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *dc_site_name;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *cli_site_name;
} DsrDcNameInfo;


#endif /* _NETRDEFS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
