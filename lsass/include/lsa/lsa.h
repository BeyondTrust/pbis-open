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
 *        lsa.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Public Client API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LSA_H__
#define __LSA_H__

/**
 * @file lsa/lsa.h
 * @brief LSASS Public Client API
 */

#include <inttypes.h>
#include <lw/types.h>
#include <lw/attrs.h>
#include <lw/rtllog.h>

#include <lwerror.h>


/**
 * @defgroup lsa Core client API
 * @brief Core client API
 */

/*@{*/

typedef LW_DWORD LSA_DS_FLAGS, *PLSA_DS_FLAGS;

#define LSA_DS_DNS_CONTROLLER_FLAG  0x20000000
#define LSA_DS_DNS_DOMAIN_FLAG      0x40000000
#define LSA_DS_DNS_FOREST_FLAG      0x80000000
#define LSA_DS_DS_FLAG              0x00000010
#define LSA_DS_GC_FLAG              0x00000004
#define LSA_DS_KDC_FLAG             0x00000020
#define LSA_DS_PDC_FLAG             0x00000001
#define LSA_DS_TIMESERV_FLAG        0x00000040
#define LSA_DS_WRITABLE_FLAG        0x00000100

typedef LW_DWORD LSA_DM_DOMAIN_FLAGS, *PLSA_DM_DOMAIN_FLAGS;

#define LSA_DM_DOMAIN_FLAG_PRIMARY               0x00000001
#define LSA_DM_DOMAIN_FLAG_OFFLINE               0x00000002
#define LSA_DM_DOMAIN_FLAG_FORCE_OFFLINE         0x00000004
#define LSA_DM_DOMAIN_FLAG_TRANSITIVE_1WAY_CHILD 0x00000008
#define LSA_DM_DOMAIN_FLAG_FOREST_ROOT           0x00000010
#define LSA_DM_DOMAIN_FLAG_GC_OFFLINE            0x00000020


#define LSA_DM_DOMAIN_FLAGS_VALID_MASK \
    ( \
        LSA_DM_DOMAIN_FLAG_PRIMARY | \
        LSA_DM_DOMAIN_FLAG_OFFLINE | \
        LSA_DM_DOMAIN_FLAG_FORCE_OFFLINE | \
        LSA_DM_DOMAIN_FLAG_TRANSITIVE_1WAY_CHILD | \
        LSA_DM_DOMAIN_FLAG_FOREST_ROOT | \
        LSA_DM_DOMAIN_FLAG_GC_OFFLINE | \
        0 \
    )

typedef LW_DWORD LSA_DM_STATE_FLAGS, *PLSA_DM_STATE_FLAGS;

// Controls whether to enable offline reporting.
// Offline state is always tracked internally,
// but this controls whether to honor that state.
#define LSA_DM_STATE_FLAG_OFFLINE_ENABLED        0x00000001
// Whether forced globally offline (by user).
#define LSA_DM_STATE_FLAG_FORCE_OFFLINE          0x00000002
// Whether globally offline due to media sense.
#define LSA_DM_STATE_FLAG_MEDIA_SENSE_OFFLINE    0x00000004
// Whether to ignore trusts.
#define LSA_DM_STATE_FLAG_IGNORE_ALL_TRUSTS      0x00000008

#define LSA_DM_STATE_FLAGS_VALID_MASK \
    ( \
        LSA_DM_STATE_FLAG_OFFLINE_ENABLED | \
        LSA_DM_STATE_FLAG_FORCE_OFFLINE | \
        LSA_DM_STATE_FLAG_MEDIA_SENSE_OFFLINE | \
        LSA_DM_STATE_FLAG_IGNORE_ALL_TRUSTS | \
        0 \
    )

typedef LW_DWORD LSA_TRUST_TYPE, *PLSA_TRUST_TYPE;

#define LSA_TRUST_TYPE_DOWNLEVEL            0x00000001
#define LSA_TRUST_TYPE_UPLEVEL              0x00000002
#define LSA_TRUST_TYPE_MIT                  0x00000003
#define LSA_TRUST_TYPE_DCE                  0x00000004

typedef LW_DWORD LSA_TRUST_ATTRIBUTE, *PLSA_TRUST_ATTRIBUTE;

#define LSA_TRUST_ATTRIBUTE_NON_TRANSITIVE     0x00000001
#define LSA_TRUST_ATTRIBUTE_UPLEVEL_ONLY       0x00000002
#define LSA_TRUST_ATTRIBUTE_FILTER_SIDS        0x00000004
#define LSA_TRUST_ATTRIBUTE_FOREST_TRANSITIVE  0x00000008
#define LSA_TRUST_ATTRIBUTE_CROSS_ORGANIZATION 0x00000010
#define LSA_TRUST_ATTRIBUTE_WITHIN_FOREST      0x00000020

typedef LW_DWORD LSA_TRUST_FLAG, *PLSA_TRUST_FLAG;

#define LSA_TRUST_FLAG_IN_FOREST    0x00000001
#define LSA_TRUST_FLAG_OUTBOUND     0x00000002
#define LSA_TRUST_FLAG_TREEROOT     0x00000004
#define LSA_TRUST_FLAG_PRIMARY      0x00000008
#define LSA_TRUST_FLAG_NATIVE       0x00000010
#define LSA_TRUST_FLAG_INBOUND      0x00000020

typedef LW_DWORD LSA_TRUST_DIRECTION;

#define LSA_TRUST_DIRECTION_UNKNOWN  0x00000000
#define LSA_TRUST_DIRECTION_ZERO_WAY 0x00000001
#define LSA_TRUST_DIRECTION_ONE_WAY  0x00000002
#define LSA_TRUST_DIRECTION_TWO_WAY  0x00000003
#define LSA_TRUST_DIRECTION_SELF     0x00000004

typedef LW_DWORD LSA_TRUST_MODE;

#define LSA_TRUST_MODE_UNKNOWN       0x00000000
#define LSA_TRUST_MODE_EXTERNAL      0x00000001
#define LSA_TRUST_MODE_MY_FOREST     0x00000002
#define LSA_TRUST_MODE_OTHER_FOREST  0x00000003

#define LSA_NIS_MAP_NAME_NETGROUPS  "netgroup"
#define LSA_NIS_MAP_NAME_SERVICES   "services"
#define LSA_NIS_MAP_NAME_AUTOMOUNTS "automounts"

typedef LW_DWORD LSA_NIS_MAP_QUERY_FLAGS;

#define LSA_NIS_MAP_QUERY_KEYS       0x00000001
#define LSA_NIS_MAP_QUERY_VALUES     0x00000002
#define LSA_NIS_MAP_QUERY_ALL        (LSA_NIS_MAP_QUERY_KEYS | LSA_NIS_MAP_QUERY_VALUES)

/**
 * @brief Find flags
 *
 * Flags that can be used to change the behavior of query processing
 */
typedef LW_DWORD LSA_FIND_FLAGS, *PLSA_FIND_FLAGS;

/**
 * @brief Query with NSS semantics
 *
 * Indicates that the query is to fulfill an NSS (name service switch)
 * request.  This hint may be used to omit irrelevant results or avoid
 * unduly expensive network operations.
 * @hideinitializer
 */
#define LSA_FIND_FLAGS_NSS        0x00000001
/**
 * @brief Query only for local objects
 *
 * Indicates that only objects in local databases should be queried.
 * @hideinitializer
 */
#define LSA_FIND_FLAGS_LOCAL      0x00000002
/**
 * @brief Query only for cached objects
 *
 * Indicates that only cached objects should be queried
 * (avoiding network operations).
 * @hideinitializer
 */
#define LSA_FIND_FLAGS_CACHE_ONLY 0x00000004

typedef struct __LW_LSA_DATA_BLOB
{
    LW_DWORD dwLen;
    LW_PBYTE pData;
} LW_LSA_DATA_BLOB, *PLW_LSA_DATA_BLOB;

#ifndef LW_STRICT_NAMESPACE
typedef LW_LSA_DATA_BLOB LSA_DATA_BLOB;
typedef PLW_LSA_DATA_BLOB PLSA_DATA_BLOB;
#endif

/*
 * Tracing support
 */
#define LSA_TRACE_FLAG_USER_GROUP_QUERIES        1
#define LSA_TRACE_FLAG_AUTHENTICATION            2
#define LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION 3
#define LSA_TRACE_FLAG_SENTINEL                  4

typedef struct __LSA_TRACE_INFO
{
    LW_DWORD dwTraceFlag;
    LW_BOOLEAN bStatus;
} LSA_TRACE_INFO, *PLSA_TRACE_INFO;

typedef struct __LSA_TRACE_INFO_LIST
{
    LW_DWORD dwNumFlags;
    PLSA_TRACE_INFO pTraceInfoArray;
} LSA_TRACE_INFO_LIST, *PLSA_TRACE_INFO_LIST;


/*
 * Logging
 */
typedef enum
{
    LSA_LOG_LEVEL_ALWAYS = LW_RTL_LOG_LEVEL_ALWAYS,
    LSA_LOG_LEVEL_ERROR = LW_RTL_LOG_LEVEL_ERROR,
    LSA_LOG_LEVEL_WARNING = LW_RTL_LOG_LEVEL_WARNING,
    LSA_LOG_LEVEL_INFO = LW_RTL_LOG_LEVEL_INFO,
    LSA_LOG_LEVEL_VERBOSE = LW_RTL_LOG_LEVEL_VERBOSE,
    LSA_LOG_LEVEL_DEBUG = LW_RTL_LOG_LEVEL_DEBUG,
    LSA_LOG_LEVEL_TRACE = LW_RTL_LOG_LEVEL_TRACE
} LsaLogLevel;

typedef struct __LSA_USER_INFO_0
{
    uid_t uid;
    gid_t gid;
    LW_PSTR pszName;
    LW_PSTR pszPasswd;
    LW_PSTR pszGecos;
    LW_PSTR pszShell;
    LW_PSTR pszHomedir;
    LW_PSTR pszSid;
} LSA_USER_INFO_0, *PLSA_USER_INFO_0;

typedef struct __LSA_USER_INFO_1
{
    union
    {
        struct
        {
            uid_t uid;
            gid_t gid;
            LW_PSTR pszName;
            LW_PSTR pszPasswd;
            LW_PSTR pszGecos;
            LW_PSTR pszShell;
            LW_PSTR pszHomedir;
            LW_PSTR pszSid;
        };
        LSA_USER_INFO_0 info0;
    };
    LW_PSTR pszDN;
    LW_PSTR pszUPN;
    LW_DWORD bIsGeneratedUPN;
    LW_DWORD bIsLocalUser;
    LW_PBYTE pLMHash;
    LW_DWORD dwLMHashLen;
    LW_PBYTE pNTHash;
    LW_DWORD dwNTHashLen;
} LSA_USER_INFO_1, *PLSA_USER_INFO_1;

typedef struct __LSA_USER_INFO_2
{
    union
    {
        struct
        {
            uid_t uid;
            gid_t gid;
            LW_PSTR pszName;
            LW_PSTR pszPasswd;
            LW_PSTR pszGecos;
            LW_PSTR pszShell;
            LW_PSTR pszHomedir;
            LW_PSTR pszSid;
            LW_PSTR pszDN;
            LW_PSTR pszUPN;
            LW_DWORD bIsGeneratedUPN;
            LW_DWORD bIsLocalUser;
            LW_PBYTE pLMHash;
            LW_DWORD dwLMHashLen;
            LW_PBYTE pNTHash;
            LW_DWORD dwNTHashLen;
        };
        LSA_USER_INFO_1 info1;
    };
    LW_DWORD dwDaysToPasswordExpiry;
    LW_BOOLEAN bPasswordExpired;
    LW_BOOLEAN bPasswordNeverExpires;
    LW_BOOLEAN bPromptPasswordChange;
    LW_BOOLEAN bUserCanChangePassword;
    LW_BOOLEAN bAccountDisabled;
    LW_BOOLEAN bAccountExpired;
    LW_BOOLEAN bAccountLocked;
} LSA_USER_INFO_2, *PLSA_USER_INFO_2;

typedef struct __LSA_USER_INFO_LIST
{
    LW_DWORD dwUserInfoLevel;
    LW_DWORD dwNumUsers;
    union _USER_INFO_LIST
    {
        PLSA_USER_INFO_0* ppInfoList0;
        PLSA_USER_INFO_1* ppInfoList1;
        PLSA_USER_INFO_2* ppInfoList2;
    } ppUserInfoList;
} LSA_USER_INFO_LIST, *PLSA_USER_INFO_LIST;

typedef struct __LSA_USER_MOD_INFO
{
    uid_t uid;

    struct _usermod_actions {
        LW_BOOLEAN bEnableUser;
        LW_BOOLEAN bDisableUser;
        LW_BOOLEAN bUnlockUser;
        LW_BOOLEAN bSetChangePasswordOnNextLogon;
        LW_BOOLEAN bSetPasswordNeverExpires;
        LW_BOOLEAN bSetPasswordMustExpire;
        LW_BOOLEAN bAddToGroups;
        LW_BOOLEAN bRemoveFromGroups;
        LW_BOOLEAN bSetAccountExpiryDate;
        LW_BOOLEAN bSetHomedir;
        LW_BOOLEAN bSetShell;
        LW_BOOLEAN bSetGecos;
        LW_BOOLEAN bSetPrimaryGroup;
        LW_BOOLEAN bSetNtPasswordHash;
        LW_BOOLEAN bSetLmPasswordHash;
        LW_BOOLEAN bSetPassword;
    } actions;

    gid_t   gid;
    LW_PSTR pszAddToGroups;
    LW_PSTR pszRemoveFromGroups;
    LW_PSTR pszExpiryDate;
    LW_PSTR pszHomedir;
    LW_PSTR pszShell;
    LW_PSTR pszGecos;
    LW_PSTR pszPassword;
    PLW_LSA_DATA_BLOB pNtPasswordHash;
    PLW_LSA_DATA_BLOB pLmPasswordHash;

} LSA_USER_MOD_INFO, *PLSA_USER_MOD_INFO;

typedef struct __LSA_GROUP_INFO_0
{
    gid_t gid;
    LW_PSTR pszName;
    LW_PSTR pszSid;
} LSA_GROUP_INFO_0, *PLSA_GROUP_INFO_0;

typedef struct __LSA_GROUP_INFO_1
{
    union
    {
        struct
        {
            gid_t gid;
            LW_PSTR pszName;
            LW_PSTR pszSid;
        };
        LSA_GROUP_INFO_0 info0;
    };
    LW_PSTR pszDN;
    LW_PSTR pszPasswd;
    LW_PSTR* ppszMembers;
} LSA_GROUP_INFO_1, *PLSA_GROUP_INFO_1;

typedef struct __LSA_GROUP_INFO_LIST
{
    LW_DWORD dwGroupInfoLevel;
    LW_DWORD dwNumGroups;
    union _GROUP_INFO_LIST
    {
        PLSA_GROUP_INFO_0* ppInfoList0;
        PLSA_GROUP_INFO_1* ppInfoList1;
    } ppGroupInfoList;
} LSA_GROUP_INFO_LIST, *PLSA_GROUP_INFO_LIST;

typedef struct __LSA_GROUP_MEMBER_INFO
{
    LW_PSTR pszSid;
} LSA_GROUP_MEMBER_INFO, *PLSA_GROUP_MEMBER_INFO;

typedef struct __LSA_GROUP_MOD_INFO
{
    gid_t gid;

    struct _groupmod_actions {
        LW_BOOLEAN bAddMembers;
        LW_BOOLEAN bRemoveMembers;
    } actions;

    LW_DWORD dwAddMembersNum;
    PLSA_GROUP_MEMBER_INFO pAddMembers;

    LW_DWORD dwRemoveMembersNum;
    PLSA_GROUP_MEMBER_INFO pRemoveMembers;
} LSA_GROUP_MOD_INFO, *PLSA_GROUP_MOD_INFO;

typedef struct __LSA_ENUM_OBJECTS_INFO
{
    LW_DWORD dwObjectInfoLevel;
    LW_DWORD dwNumMaxObjects;
    LW_PSTR pszGUID;
} LSA_ENUM_OBJECTS_INFO, *PLSA_ENUM_OBJECTS_INFO;

typedef struct __LSA_NSS_ARTEFACT_INFO_0
{
    LW_PSTR pszName;
    LW_PSTR pszValue;
} LSA_NSS_ARTEFACT_INFO_0, *PLSA_NSS_ARTEFACT_INFO_0;

typedef struct __LSA_NSS_ARTEFACT_INFO_LIST
{
    LW_DWORD dwNssArtefactInfoLevel;
    LW_DWORD dwNumNssArtefacts;
    union _NSS_ARTEFACT_INFO_LIST
    {
        PLSA_NSS_ARTEFACT_INFO_0* ppInfoList0;
    } ppNssArtefactInfoList;
} LSA_NSS_ARTEFACT_INFO_LIST, *PLSA_NSS_ARTEFACT_INFO_LIST;

typedef LW_UINT8 ADAccountType;

#define AccountType_NotFound LSA_OBJECT_TYPE_UNDEFINED
#define AccountType_Group    LSA_OBJECT_TYPE_GROUP
#define AccountType_User     LSA_OBJECT_TYPE_USER
#define AccountType_Domain   LSA_OBJECT_TYPE_DOMAIN

typedef struct __LSA_SID_INFO
{
    LW_UINT8 accountType;
    LW_PSTR pszSamAccountName;
    LW_PSTR pszDomainName;
} LSA_SID_INFO, *PLSA_SID_INFO;

typedef struct __LSA_FIND_NAMES_BY_SIDS
{
    size_t sCount;
    PLSA_SID_INFO pSIDInfoList;
    LW_CHAR chDomainSeparator;
} LSA_FIND_NAMES_BY_SIDS, *PLSA_FIND_NAMES_BY_SIDS;

typedef struct __LSA_METRIC_PACK_0
{
    LW_UINT64 failedAuthentications;
    LW_UINT64 failedUserLookupsByName;
    LW_UINT64 failedUserLookupsById;
    LW_UINT64 failedGroupLookupsByName;
    LW_UINT64 failedGroupLookupsById;
    LW_UINT64 failedOpenSession;
    LW_UINT64 failedCloseSession;
    LW_UINT64 failedChangePassword;
} LSA_METRIC_PACK_0, *PLSA_METRIC_PACK_0;

typedef struct __LSA_METRIC_PACK_1
{
    LW_UINT64 successfulAuthentications;
    LW_UINT64 failedAuthentications;
    LW_UINT64 rootUserAuthentications;
    LW_UINT64 successfulUserLookupsByName;
    LW_UINT64 failedUserLookupsByName;
    LW_UINT64 successfulUserLookupsById;
    LW_UINT64 failedUserLookupsById;
    LW_UINT64 successfulGroupLookupsByName;
    LW_UINT64 failedGroupLookupsByName;
    LW_UINT64 successfulGroupLookupsById;
    LW_UINT64 failedGroupLookupsById;
    LW_UINT64 successfulOpenSession;
    LW_UINT64 failedOpenSession;
    LW_UINT64 successfulCloseSession;
    LW_UINT64 failedCloseSession;
    LW_UINT64 successfulChangePassword;
    LW_UINT64 failedChangePassword;
} LSA_METRIC_PACK_1, *PLSA_METRIC_PACK_1;

typedef struct __LSA_METRIC_PACK
{
    LW_DWORD dwInfoLevel;
    union _METRIC_PACK
    {
        PLSA_METRIC_PACK_0 pMetricPack0;
        PLSA_METRIC_PACK_1 pMetricPack1;
    } pMetricPack;
} LSA_METRIC_PACK, *PLSA_METRIC_PACK;

typedef enum
{
    LSA_PROVIDER_MODE_UNKNOWN = 0,
    LSA_PROVIDER_MODE_UNPROVISIONED,
    LSA_PROVIDER_MODE_DEFAULT_CELL,
    LSA_PROVIDER_MODE_NON_DEFAULT_CELL,
    LSA_PROVIDER_MODE_LOCAL_SYSTEM
} LsaAuthProviderMode;

typedef enum
{
    LSA_AUTH_PROVIDER_SUBMODE_UNKNOWN = 0,
    LSA_AUTH_PROVIDER_SUBMODE_SCHEMA,
    LSA_AUTH_PROVIDER_SUBMODE_NONSCHEMA
} LsaAuthProviderSubMode;

typedef enum
{
    LSA_AUTH_PROVIDER_STATUS_UNKNOWN = 0,
    LSA_AUTH_PROVIDER_STATUS_ONLINE,
    LSA_AUTH_PROVIDER_STATUS_OFFLINE,
    LSA_AUTH_PROVIDER_STATUS_FORCED_OFFLINE
} LsaAuthProviderStatus;

typedef struct __LSA_DC_INFO
{
    LW_PSTR pszName;
    LW_PSTR pszAddress;
    LW_PSTR pszSiteName;
    LSA_DS_FLAGS dwFlags;
} LSA_DC_INFO, *PLSA_DC_INFO;

typedef struct __LW_LSA_TRUSTED_DOMAIN_INFO
{
    LW_PSTR pszDnsDomain;
    LW_PSTR pszNetbiosDomain;
    LW_PSTR pszTrusteeDnsDomain;
    LW_PSTR pszDomainSID;
    LW_PSTR pszDomainGUID;
    LW_PSTR pszForestName;
    LW_PSTR pszClientSiteName;
    LSA_TRUST_FLAG dwTrustFlags;
    LSA_TRUST_TYPE dwTrustType;
    LSA_TRUST_ATTRIBUTE dwTrustAttributes;
    LSA_TRUST_DIRECTION dwTrustDirection;
    LSA_TRUST_MODE dwTrustMode;
    LSA_DM_DOMAIN_FLAGS dwDomainFlags;
    PLSA_DC_INFO pDCInfo;
    PLSA_DC_INFO pGCInfo;
} LW_LSA_TRUSTED_DOMAIN_INFO, *PLW_LSA_TRUSTED_DOMAIN_INFO;

#ifndef LW_STRICT_NAMESPACE
typedef LW_LSA_TRUSTED_DOMAIN_INFO LSA_TRUSTED_DOMAIN_INFO;
typedef PLW_LSA_TRUSTED_DOMAIN_INFO PLSA_TRUSTED_DOMAIN_INFO;
#endif

typedef struct __LSA_AUTH_PROVIDER_STATUS
{
    LW_PSTR pszId;
    LsaAuthProviderMode mode;
    LsaAuthProviderSubMode subMode;
    LsaAuthProviderStatus status;
    LW_PSTR pszDomain;
    LW_PSTR pszDomainSid;
    LW_PSTR pszForest;
    LW_PSTR pszSite;
    LW_PSTR pszCell;
    LW_DWORD dwNetworkCheckInterval;
    LW_DWORD dwNumTrustedDomains;
    PLW_LSA_TRUSTED_DOMAIN_INFO pTrustedDomainInfoArray;
} LSA_AUTH_PROVIDER_STATUS, *PLSA_AUTH_PROVIDER_STATUS;

typedef struct __LSA_VERSION
{
    LW_DWORD dwMajor;
    LW_DWORD dwMinor;
    LW_DWORD dwBuild;
    LW_DWORD dwRevision;
} LSA_VERSION, *PLSA_VERSION;

typedef struct __LSASTATUS
{
    LW_DWORD dwUptime;

    LSA_VERSION lsassVersion;
    LSA_VERSION productVersion;

    LW_DWORD dwCount;
    PLSA_AUTH_PROVIDER_STATUS pAuthProviderStatusList;
} LSASTATUS, *PLSASTATUS;


/*
 * AuthenticateUserEx() parameters
 */

typedef enum
{
    LSA_AUTH_PLAINTEXT = 1,
    LSA_AUTH_CHAP
} LsaAuthType;

typedef struct __LSA_AUTH_CLEARTEXT_PARAM
{
    LW_PSTR pszPassword;
} LSA_AUTH_CLEARTEXT_PARAM, *PLSA_AUTH_CLEARTEXT_PARAM;

typedef struct __LSA_AUTH_CHAP_PARAM
{
    PLW_LSA_DATA_BLOB pChallenge;
    PLW_LSA_DATA_BLOB pLM_resp;
    PLW_LSA_DATA_BLOB pNT_resp;
} LSA_AUTH_CHAP_PARAM, *PLSA_AUTH_CHAP_PARAM;

typedef struct __LSA_AUTH_USER_PARAMS
{
    LsaAuthType AuthType;
    LW_PSTR pszAccountName;
    LW_PSTR pszDomain;
    LW_PSTR pszWorkstation;
    union _PASS {
        LSA_AUTH_CLEARTEXT_PARAM clear;
        LSA_AUTH_CHAP_PARAM chap;
    } pass;
} LSA_AUTH_USER_PARAMS, *PLSA_AUTH_USER_PARAMS;

typedef struct __LSA_AUTH_USER_PAM_PARAMS
{
    LW_DWORD dwFlags;
    LW_PCSTR pszLoginName;
    LW_PCSTR pszPassword;
    LW_PCSTR pszPamSource;
} LSA_AUTH_USER_PAM_PARAMS, *PLSA_AUTH_USER_PAM_PARAMS;

#define LSA_AUTH_USER_PAM_FLAG_RETURN_MESSAGE   0x00000001
#define LSA_AUTH_USER_PAM_FLAG_SMART_CARD       0x00000002

typedef struct __LSA_AUTH_USER_PAM_INFO
{
    LW_PSTR    pszMessage;
    LW_BOOLEAN bOnlineLogon;
} LSA_AUTH_USER_PAM_INFO, *PLSA_AUTH_USER_PAM_INFO;

#define LSA_MAX_SID_SUB_AUTHORITIES  15

typedef struct __LSA_SID_ATTRIB
{
    LW_PSTR pszSid;
    LW_DWORD dwAttrib;
} LSA_SID_ATTRIB, *PLSA_SID_ATTRIB;

typedef struct __LSA_RID_ATTRIB
{
    LW_UINT32 Rid;
    LW_DWORD dwAttrib;
} LSA_RID_ATTRIB, *PLSA_RID_ATTRIB;

#define LSA_SID_ATTR_GROUP_MANDATORY             0x00000001
#define LSA_SID_ATTR_GROUP_ENABLED_BY_DEFAULT    0x00000002
#define LSA_SID_ATTR_GROUP_ENABLED               0x00000004
#define LSA_SID_ATTR_GROUP_OWNER                 0x00000008
#define LSA_SID_ATTR_GROUP_USEFOR_DENY_ONLY      0x00000010
#define LSA_SID_ATTR_GROUP_RESOURCE              0x20000000
#define LSA_SID_ATTR_GROUP_LOGON_ID              0xC0000000

typedef struct __LSA_AUTH_USER_INFO
{
    LW_DWORD dwUserFlags;

    LW_PSTR pszAccount;
    LW_PSTR pszUserPrincipalName;
    LW_PSTR pszFullName;
    LW_PSTR pszDomain;
    LW_PSTR pszDnsDomain;

    LW_DWORD dwAcctFlags;
    PLW_LSA_DATA_BLOB pSessionKey;
    PLW_LSA_DATA_BLOB pLmSessionKey;

    LW_UINT16 LogonCount;
    LW_UINT16 BadPasswordCount;

    LW_INT64 LogonTime;
    LW_INT64 LogoffTime;
    LW_INT64 KickoffTime;
    LW_INT64 LastPasswordChange;
    LW_INT64 CanChangePassword;
    LW_INT64 MustChangePassword;

    LW_PSTR pszLogonServer;
    LW_PSTR pszLogonScript;
    LW_PSTR pszProfilePath;
    LW_PSTR pszHomeDirectory;
    LW_PSTR pszHomeDrive;

    LW_PSTR pszDomainSid;
    LW_DWORD dwUserRid;
    LW_DWORD dwPrimaryGroupRid;

    LW_DWORD dwNumRids;
    PLSA_RID_ATTRIB pRidAttribList;

    LW_DWORD dwNumSids;
    PLSA_SID_ATTRIB pSidAttribList;
} LSA_AUTH_USER_INFO, *PLSA_AUTH_USER_INFO;

#define LSA_PAM_LOG_LEVEL_DISABLED 0
#define LSA_PAM_LOG_LEVEL_ALWAYS   1
#define LSA_PAM_LOG_LEVEL_ERROR    2
#define LSA_PAM_LOG_LEVEL_WARNING  3
#define LSA_PAM_LOG_LEVEL_INFO     4
#define LSA_PAM_LOG_LEVEL_VERBOSE  5
#define LSA_PAM_LOG_LEVEL_DEBUG    6

typedef struct _LSA_PAM_CONFIG
{
    LW_DWORD dwLogLevel;
    LW_BOOLEAN bLsaPamDisplayMOTD;
    LW_PSTR pszAccessDeniedMessage;
    LW_DWORD dwNumSmartCardServices;
    LW_PSTR *ppszSmartCardServices;
    LW_DWORD dwNumSmartCardPromptGecos;
    LW_PSTR *ppszSmartCardPromptGecos;
} LSA_PAM_CONFIG, *PLSA_PAM_CONFIG;

/**
 * @brief Open connection to local lsass server
 *
 * Creates a connection handle to the local lsass server.
 *
 * @param[out] phConnection the created connection handle
 * @retval LW_ERROR_SUCCESS success
 * @retval LW_ERROR_ERRNO_ECONNREFUSED the connection was refused
 * @retval LW_ERROR_ERRNO_ENOENT the lsass domain socket was not found
 */
LW_DWORD
LsaOpenServer(
    LW_PHANDLE phConnection
    );

/**
 * @ingroup connection
 * @brief Open connection to local lsass server (thread-safe)
 *
 * Creates a connection handle to the local lsass server.
 * The handle may safely be used by multiple threads.
 * This function is only available when linking with
 * lsaclientthr
 *
 * @param[out] phConnection the created connection handle
 * @retval LW_ERROR_SUCCESS success
 * @retval LW_ERROR_ERRNO_ECONNREFUSED the connection was refused
 * @retval LW_ERROR_ERRNO_ENOENT the lsass domain socket was not found
 */
DWORD
LsaOpenServerThreaded(
    PHANDLE phConnection
    );

LW_DWORD
LsaSetTraceFlags(
    LW_HANDLE hLsaConnection,
    PLSA_TRACE_INFO pTraceFlagArray,
    LW_DWORD dwNumFlags
    );

LW_DWORD
LsaEnumTraceFlags(
    LW_HANDLE hLsaConnection,
    PLSA_TRACE_INFO* ppTraceFlagArray,
    LW_PDWORD pdwNumFlags
    );

LW_DWORD
LsaGetTraceFlag(
    LW_HANDLE hLsaConnection,
    LW_DWORD dwTraceFlag,
    PLSA_TRACE_INFO* ppTraceFlag
    );

LW_DWORD
LsaAddGroup(
    LW_HANDLE hLsaConnection,
    LW_PVOID pGroupInfo,
    LW_DWORD dwGroupInfoLevel
    );

LW_DWORD
LsaModifyGroup(
    LW_HANDLE hLsaConnection,
    PLSA_GROUP_MOD_INFO pGroupModInfo
    );

LW_DWORD
LsaDeleteGroupById(
    LW_HANDLE hLsaConnection,
    gid_t gid
    );

LW_DWORD
LsaDeleteGroupByName(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszName
    );

LW_DWORD
LsaGetGidsForUserByName(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszUserName,
    LW_PDWORD pdwGroupFound,
    gid_t** ppGidResults
    );

LW_DWORD
LsaGetGroupsForUserByName(
    LW_IN LW_HANDLE hLsaConnection,
    LW_IN LW_PCSTR pszUserName,
    LW_IN LSA_FIND_FLAGS FindFlags,
    LW_IN LW_DWORD dwGroupInfoLevel,
    LW_OUT LW_PDWORD pdwGroupsFound,
    LW_OUT LW_PVOID** pppGroupInfoList
    );

LW_DWORD
LsaGetGroupsForUserById(
    LW_HANDLE hLsaConnection,
    uid_t uid,
    LSA_FIND_FLAGS FindFlags,
    LW_DWORD dwGroupInfoLevel,
    LW_PDWORD pdwGroupsFound,
    LW_PVOID** pppGroupInfoList
    );

LW_DWORD
LsaFindGroupByName(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszGroupName,
    LSA_FIND_FLAGS FindFlags,
    LW_DWORD dwGroupInfoLevel,
    LW_PVOID* ppGroupInfo
    );

LW_DWORD
LsaFindGroupById(
    LW_HANDLE hLsaConnection,
    gid_t gid,
    LSA_FIND_FLAGS FindFlags,
    LW_DWORD dwGroupInfoLevel,
    LW_PVOID* ppGroupInfo
    );

LW_DWORD
LsaBeginEnumGroups(
    LW_HANDLE hLsaConnection,
    LW_DWORD dwGroupInfoLevel,
    LW_DWORD dwMaxNumGroups,
    LSA_FIND_FLAGS FindFlags,
    LW_PHANDLE phResume
    );

LW_DWORD
LsaBeginEnumGroupsWithCheckOnlineOption(
    LW_HANDLE hLsaConnection,
    LW_DWORD dwGroupInfoLevel,
    LW_DWORD dwMaxNumGroups,
    LW_BOOLEAN bCheckGroupMembersOnline,
    LSA_FIND_FLAGS FindFlags,
    LW_PHANDLE phResume
    );

LW_DWORD
LsaEnumGroups(
    LW_HANDLE hLsaConnection,
    LW_HANDLE hResume,
    LW_PDWORD pdwNumGroupsFound,
    LW_PVOID** pppGroupInfoList
    );

LW_DWORD
LsaEndEnumGroups(
    LW_HANDLE hLsaConnection,
    LW_HANDLE hResume
    );

LW_VOID
LsaFreeGroupInfoList(
    LW_DWORD dwLevel,
    LW_PVOID* pGroupInfoList,
    LW_DWORD dwNumGroups
    );

LW_VOID
LsaFreeGroupInfo(
    LW_DWORD dwLevel,
    LW_PVOID pGroupInfo
    );

LW_VOID
LsaFreeEnumObjectsInfo(
    PLSA_ENUM_OBJECTS_INFO pInfo
    );

LW_VOID
LsaFreeNSSArtefactInfoList(
    LW_DWORD dwLevel,
    LW_PVOID* pNSSArtefactInfoList,
    LW_DWORD dwNumNSSArtefacts
    );

LW_VOID
LsaFreeNSSArtefactInfo(
    LW_DWORD dwLevel,
    LW_PVOID pNSSArtefactInfo
    );

LW_DWORD
LsaAddUser(
    LW_HANDLE hLsaConnection,
    LW_PVOID pUserInfo,
    LW_DWORD dwUserInfoLevel
    );

LW_DWORD
LsaModifyUser(
    LW_HANDLE hLsaConnection,
    PLSA_USER_MOD_INFO pUserModInfo
    );

LW_DWORD
LsaDeleteUserById(
    LW_HANDLE hLsaConnection,
    uid_t uid
    );

LW_DWORD
LsaDeleteUserByName(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszName
    );

LW_DWORD
LsaFindUserByName(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszName,
    LW_DWORD dwUserInfoLevel,
    LW_PVOID* ppUserInfo
    );

LW_DWORD
LsaFindUserById(
    LW_HANDLE hLsaConnection,
    uid_t uid,
    LW_DWORD dwUserInfoLevel,
    LW_PVOID* ppUserInfo
    );

LW_DWORD
LsaGetNamesBySidList(
    LW_IN LW_HANDLE hLsaConnection,
    LW_IN size_t sCount,
    LW_IN LW_PSTR* ppszSidList,
    LW_OUT PLSA_SID_INFO* ppSIDInfoList,
    LW_OUT LW_OPTIONAL LW_CHAR *pchDomainSeparator
    );

LW_VOID
LsaFreeSIDInfoList(
    PLSA_SID_INFO ppSIDInfoList,
    size_t stNumSID
    );

LW_VOID
LsaFreeSIDInfo(
    PLSA_SID_INFO pSIDInfo
    );

LW_DWORD
LsaBeginEnumUsers(
    LW_HANDLE hLsaConnection,
    LW_DWORD dwUserInfoLevel,
    LW_DWORD dwMaxNumUsers,
    LSA_FIND_FLAGS FindFlags,
    LW_PHANDLE phResume
    );

LW_DWORD
LsaEnumUsers(
    LW_HANDLE hLsaConnection,
    LW_HANDLE hResume,
    LW_PDWORD pdwNumUsersFound,
    LW_PVOID** pppUserInfoList
    );

LW_DWORD
LsaEndEnumUsers(
    LW_HANDLE hLsaConnection,
    LW_HANDLE hResume
    );

LW_VOID
LsaFreeUserInfoList(
    LW_DWORD dwLevel,
    LW_PVOID* pUserInfoList,
    LW_DWORD dwNumUsers
    );

LW_VOID
LsaFreeUserInfo(
    LW_DWORD dwLevel,
    LW_PVOID pUserInfo
    );

LW_DWORD
LsaAuthenticateUser(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszLoginName,
    LW_PCSTR pszPassword,
    LW_PSTR* ppszMessage
    );

LW_DWORD
LsaFreeAuthUserInfo(
    PLSA_AUTH_USER_INFO* ppAuthUserInfo
    );

LW_DWORD
LsaFreeAuthUserParams(
    PLSA_AUTH_USER_PARAMS* ppAuthUserParams
    );

LW_DWORD
LsaAuthenticateUserEx(
    LW_IN LW_HANDLE hLsaConnection,
    LW_IN LW_PCSTR pszTargetProvider,
    LW_IN LSA_AUTH_USER_PARAMS* pParams,
    LW_OUT PLSA_AUTH_USER_INFO* ppUserInfo
    );

LW_DWORD
LsaAuthenticateUserPam(
    LW_IN LW_HANDLE hLsaConnection,
    LW_IN LSA_AUTH_USER_PAM_PARAMS* pParams,
    LW_OUT PLSA_AUTH_USER_PAM_INFO* ppPamAuthInfo
    );

LW_VOID
LsaFreeAuthUserPamInfo(
    LW_IN LW_OUT PLSA_AUTH_USER_PAM_INFO pPamAuthInfo
    );

LW_DWORD
LsaValidateUser(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszLoginName,
    LW_PCSTR pszPassword
    );

LW_DWORD
LsaCheckUserInList(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszLoginName,
    LW_PCSTR pszListName
    );

LW_DWORD
LsaChangePassword(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszLoginName,
    LW_PCSTR pszNewPassword,
    LW_PCSTR pszOldPassword
    );

LW_DWORD
LsaSetPassword(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszLoginName,
    LW_PCSTR pszNewPassword
    );

LW_DWORD
LsaOpenSession(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszLoginId
    );

LW_DWORD
LsaCloseSession(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszLoginId
    );

LW_DWORD
LsaGetMetrics(
    LW_HANDLE hLsaConnection,
    LW_DWORD dwInfoLevel,
    LW_PVOID* ppMetricPack
    );

LW_DWORD
LsaGetStatus(
    LW_HANDLE hLsaConnection,
    PLSASTATUS* ppLsaStatus
    );

LW_DWORD
LsaGetStatus2(
    LW_HANDLE hLsaConnection,
    PCSTR pszStr,
    PLSASTATUS* ppLsaStatus
    );

LW_DWORD
LsaSetMachineSid(
    LW_IN LW_HANDLE hLsaConnection,
    LW_IN LW_PCSTR  pszSid
    );

LW_DWORD
LsaSetMachineName(
    LW_IN LW_HANDLE hLsaConnection,
    LW_IN LW_PCSTR  pszMachineName
    );

LW_VOID
LsaFreeStatus(
    PLSASTATUS pLsaStatus
    );

DWORD
LsaReadVersionFile(
    PLSA_VERSION pVersion
    );

/**
 * @brief Closes connection to lsass server
 *
 * Closes a connection handle opened with #LsaOpenServer()
 * or #LsaOpenServerThreaded().
 *
 * @param[in,out] hConnection the connection handle to close
 * @retval LW_ERROR_SUCCESS success
 */
LW_DWORD
LsaCloseServer(
    LW_HANDLE hConnection
    );

/**
 * @brief Frees a connection to lsass server
 *
 * This frees the local resources associated with a connection handle
 * opened by #LsaOpenServer(), but does not explicitly terminate
 * the session with the server.  This is important to prevent a
 * child process from interfering with its parent after a fork().
 *
 * @param[in,out] hConnection the connection handle to close
 * @retval LW_ERROR_SUCCESS success
 */
LW_DWORD
LsaDropServer(
    LW_HANDLE hConnection
    );

LW_DWORD
LsaGetErrorMessageForLoggingEvent(
    LW_DWORD dwError,
    LW_PSTR* ppszErrorMsg
    );

/*
 * LW_LSA_DATA_BLOB access functions and methods
 */

LW_DWORD
LsaDataBlobAllocate(
    PLW_LSA_DATA_BLOB* ppBlob,
    LW_DWORD dwSize
    );

LW_VOID
LsaDataBlobFree(
    PLW_LSA_DATA_BLOB* ppBlob
    );

LW_DWORD
LsaDataBlobStore(
    PLW_LSA_DATA_BLOB* ppBlob,
    LW_DWORD dwSize,
    const LW_PBYTE pBuffer
    );

LW_DWORD
LsaDataBlobCopy(
    PLW_LSA_DATA_BLOB* ppDst,
    PLW_LSA_DATA_BLOB pSrc
    );

LW_DWORD
LsaDataBlobLength(
    PLW_LSA_DATA_BLOB pBlob
    );

LW_PBYTE
LsaDataBlobBuffer(
    PLW_LSA_DATA_BLOB pBlob
    );


//
// NIS Map Routines
//

LW_DWORD
LsaFindNSSArtefactByKey(
    LW_HANDLE hLsaConnection,
    LW_DWORD dwMapInfoLevel,
    LW_PCSTR pszKeyName,
    LW_PCSTR pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    LW_PVOID* ppNSSArtefactInfo
    );

LW_DWORD
LsaBeginEnumNSSArtefacts(
    LW_HANDLE hLsaConnection,
    LW_DWORD dwInfoLevel,
    LW_PCSTR pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    LW_DWORD dwMaxNumNSSArtefacts,
    LW_PHANDLE phResume
    );

LW_DWORD
LsaEnumNSSArtefacts(
    LW_HANDLE hLsaConnection,
    LW_HANDLE hResume,
    LW_PDWORD pdwNumNSSArtefactsFound,
    LW_PVOID** pppNSSArtefactInfoList
    );

LW_DWORD
LsaEndEnumNSSArtefacts(
    LW_HANDLE hLsaConnection,
    LW_HANDLE hResume
    );

//
// Provider-Specific IOCTL Support
//

LW_DWORD
LsaProviderIoControl(
    LW_IN LW_HANDLE hLsaConnection,
    LW_IN LW_PCSTR pszProviderId,
    LW_IN LW_DWORD dwIoControlCode,
    LW_IN LW_DWORD dwInputBufferSize,
    LW_IN LW_PVOID pInputBuffer,
    LW_OUT LW_OPTIONAL LW_DWORD* pdwOutputBufferSize,
    LW_OUT LW_OPTIONAL LW_PVOID* ppOutputBuffer
    );

LW_DWORD
LsaGetPamConfig(
    LW_IN LW_HANDLE hLsaConnection,
    LW_OUT PLSA_PAM_CONFIG *ppPamConfig
    );

LW_VOID
LsaFreePamConfig(
    LW_IN PLSA_PAM_CONFIG pPamConfig
    );

/**
 * @brief Query type enumeration
 *
 * Specifies the type of key used when querying
 */
enum _LSA_QUERY_TYPE
{
    /**
     * @brief Undefined
     * @hideinitializer
     */
	LSA_QUERY_TYPE_UNDEFINED     = 0,
	/**
	 * @brief Query by distinguished name
	 * @hideinitializer
	 */
	LSA_QUERY_TYPE_BY_DN         = 1,
	/**
	 * @brief Query by SID
	 * @hideinitializer
	 */
	LSA_QUERY_TYPE_BY_SID        = 2,
	/**
	 * @brief Query by NT4-style name
	 * @hideinitializer
	 */
	LSA_QUERY_TYPE_BY_NT4        = 3,
	/**
	 * @brief Query by User Principal Name
	 * @hideinitializer
	 */
	LSA_QUERY_TYPE_BY_UPN        = 4,
	/**
	 * @brief Query by alias
	 * @hideinitializer
	 */
	LSA_QUERY_TYPE_BY_ALIAS      = 5,
	/**
	 * @brief Query by uid or gid
	 * @hideinitializer
	 */
	LSA_QUERY_TYPE_BY_UNIX_ID    = 6,
	/**
	 * @brief Query by generic name
	 * @hideinitializer
	 */
	LSA_QUERY_TYPE_BY_NAME       = 7
};

/**
 * @brief Query type
 */
typedef LW_UINT8 LSA_QUERY_TYPE, *PLSA_QUERY_TYPE;

/**
 * @brief Object type enumeration
 *
 * Designates the type of a security object or the
 * desired type in a query.
 */
enum _LSA_OBJECT_TYPE
{
    /**
     * @brief Undefined
     * @hideinitializer
     */
    LSA_OBJECT_TYPE_UNDEFINED = 0,
    /**
     * @brief Group object
     * @hideinitializer
     */
    LSA_OBJECT_TYPE_GROUP     = 1,
    /**
     * @brief User object
     * @hideinitializer
     */
    LSA_OBJECT_TYPE_USER      = 2,
#ifndef DOXYGEN
    LSA_OBJECT_TYPE_DOMAIN    = 3,
    LSA_OBJECT_TYPE_COMPUTER  = 4
#endif
};

typedef LW_UINT8 LSA_OBJECT_TYPE, *PLSA_OBJECT_TYPE;

typedef struct __LSA_SECURITY_OBJECT_VERSION_INFO
{
    // This value is set to -1 if the value is not stored in the
    // database (it only exists in memory). Otherwise, this is an index into
    // the database.
    int64_t qwDbId;
    time_t tLastUpdated;
    // Sum of the size of all objects that use this version info (only used by
    // memory backend)
    LW_DWORD dwObjectSize;
    // Importance of this object (for internal use by the memory backend)
    float fWeight;
} LSA_SECURITY_OBJECT_VERSION_INFO, *PLSA_SECURITY_OBJECT_VERSION_INFO;

/**
 * @brief User object info
 *
 * Information about a user in a user object
 */
typedef struct _LSA_SECURITY_OBJECT_USER_INFO
{
    /* Windows-like attributes */
    /**
     * @brief Primary group SID
     *
     * The user's primary group SID in string form
     */
    LW_PSTR pszPrimaryGroupSid;
    /**
     * @brief UPN
     *
     * The user's User Principal Name
     */
    LW_PSTR pszUPN;
    /**
     * @brief Alias
     *
     * The user's alias
     */
    LW_PSTR pszAliasName;
    /**
     * @brief Password last set time
     *
     * The last time the user's password was set
     * as an NT time.
     */
    uint64_t qwPwdLastSet;
    /**
     * @brief Password age
     *
     * The maximum password age as an NT time
     */
    uint64_t qwMaxPwdAge;
    /**
     * @brief Password expiry time
     *
     * When the user's password will expire as an
     * NT time.
     */
    uint64_t qwPwdExpires;
    /**
     * @brief Account expiry time
     *
     * When the users's account will expire
     * as an NT time.
     */
    uint64_t qwAccountExpires;

    /**
     * @brief Is UPN generated?
     *
     * Set to TRUE if the user's UPN was synthesized from the NT4 name
     */
    LW_BOOLEAN bIsGeneratedUPN;
    /**
     * @brief Is account info known?
     *
     * Set to TRUE if the following BOOLEAN field have well-defined values.
     */
    LW_BOOLEAN bIsAccountInfoKnown;
    /**
     * @brief Is the password expired?
     */
    LW_BOOLEAN bPasswordExpired;
    /**
     * @brief Does the password never expire?
     */
    LW_BOOLEAN bPasswordNeverExpires;
    /**
     * @brief Should the user be asked to change password?
     */
    LW_BOOLEAN bPromptPasswordChange;
    /**
     * @brief Can the user change password?
     */
    LW_BOOLEAN bUserCanChangePassword;
    /**
     * @brief Is the account disabled?
     */
    LW_BOOLEAN bAccountDisabled;
    /**
     * @brief Is the account expired?
     */
    LW_BOOLEAN bAccountExpired;
    /**
     * @brief Is the account locked?
     */
    LW_BOOLEAN bAccountLocked;

#ifndef DOXYGEN
    LW_DWORD dwLmHashLen;
    LW_PBYTE pLmHash;
    LW_DWORD dwNtHashLen;
    LW_PBYTE pNtHash;
#endif

    /* UNIX-like attributes */
    /**
     * @brief UNIX UID
     */
    uid_t uid;
    /**
     * @brief UNIX primary GID
     */
    gid_t gid;
    /**
     * @brief UNIX account name
     */
    LW_PSTR pszUnixName;
    /**
     * @brief UNIX password
     *
     * This field will usually contain no value
     */
    LW_PSTR pszPasswd;
    /**
     * @brief UNIX GECOS field
     */
    LW_PSTR pszGecos;
    /**
     * @brief UNIX shell
     */
    LW_PSTR pszShell;
    /**
     * @brief UNIX home directory
     */
    LW_PSTR pszHomedir;

    /**
     * @brief Windows Display name. This is initialized whenever the data is
     * available, even for unenabled users.
     */
    LW_PSTR pszDisplayName;
} LSA_SECURITY_OBJECT_USER_INFO;

typedef LSA_SECURITY_OBJECT_USER_INFO *PLSA_SECURITY_OBJECT_USER_INFO;

/**
 * @brief User object info
 *
 * Information about a user in a user object
 */
typedef struct _LSA_SECURITY_OBJECT_GROUP_INFO
{
    /**
     * @brief UNIX GID
     */
    gid_t gid;
    /**
     * @brief Group alias
     */
    LW_PSTR pszAliasName;
    /**
     * @brief UNIX group name
     */
    LW_PSTR pszUnixName;
    /**
     * @brief UNIX password
     *
     * This field usually contains no value.
     */
    LW_PSTR pszPasswd;
} LSA_SECURITY_OBJECT_GROUP_INFO;

typedef LSA_SECURITY_OBJECT_GROUP_INFO *PLSA_SECURITY_OBJECT_GROUP_INFO;

/**
 * @brief Security object
 *
 * Represents a user or group, including all its UNIX-like (uid, gid...) and Windows-like
 * (SAM name, SID...) properties.
 */
typedef struct __LSA_SECURITY_OBJECT
{
#ifndef DOXYGEN
    LSA_SECURITY_OBJECT_VERSION_INFO version;
#endif
    /**
     * @brief Object distinguished name
     */
    LW_PSTR    pszDN;
    /**
     * @brief Object SID as a printed string
     */
    LW_PSTR    pszObjectSid;
    /**
     * @brief Is object enabled?
     *
     * Set to true if the object is enabled for use on Linux
     * and UNIX systems.  Disabled objects lack meaningful values
     * for their UNIX properties such as UID and GID.
     */
    LW_BOOLEAN enabled;
    /**
     * @brief Is object local?
     *
     * Set to true if the object originates from a local account store,
     * such as the local provider.
     */
    LW_BOOLEAN bIsLocal;

    /**
     * @brief NetBIOS domain name
     *
     * The name of the object's NetBIOS domain.
     */
    LW_PSTR    pszNetbiosDomainName;
    /**
     * @brief SAM account name
     *
     * The SAM account name of the object.  An NT4-style
     * name consists of the NetBIOS domain name followed by
     * a backslash and the SAM account name.
     */
    LW_PSTR    pszSamAccountName;

    /**
     * @brief Object type
     *
     * Presently, only user and group object types are actually
     * returned by lsass APIs.
     */
    LSA_OBJECT_TYPE type;

    union
    {
        /**
         * @brief User attributes
         *
         * Attributes that are set if the object type is
         * #LSA_OBJECT_TYPE_USER.
         */
        LSA_SECURITY_OBJECT_USER_INFO userInfo;
        /**
         * @brief Group attributes
         *
         * Attributes that are set if the object type is
         * #LSA_OBJECT_TYPE_GROUP.
         */
        LSA_SECURITY_OBJECT_GROUP_INFO groupInfo;
#ifndef DOXYGEN
        union
        {
            LSA_SECURITY_OBJECT_USER_INFO userInfo;
            LSA_SECURITY_OBJECT_GROUP_INFO groupInfo;
        } typeInfo;
#endif
    };
} LSA_SECURITY_OBJECT;

/**
 * @brief Security object pointer
 */
typedef LSA_SECURITY_OBJECT *PLSA_SECURITY_OBJECT;

/**
 * @brief Constant security object pointer
 */
typedef const LSA_SECURITY_OBJECT *PCLSA_SECURITY_OBJECT;

/**
 * @brief Query item
 *
 * A key used to query for an object.
 */
typedef union _LSA_QUERY_ITEM {
    /**
     * @brief String key
     *
     * This arm of the union should be used when querying by
     * a key that is a strings, such as SID or UPN.
     */
    LW_PCSTR pszString;
    /**
     * @brief Number key
     *
     * This arm of the union should be used when querying by
     * a key that is a number, such as UID or GID.
     */
    LW_DWORD dwId;
} LSA_QUERY_ITEM;

typedef LSA_QUERY_ITEM *PLSA_QUERY_ITEM;

/**
 * @brief Query list
 *
 * A list of keys used to query for objects.
 */
typedef union _LSA_QUERY_LIST {
    /**
     * @brief String keys
     *
     * This arm of the union is an array of strings that
     * should be used when querying by keys that are strings,
     * such as SID or UPN.
     */
    LW_PCSTR* ppszStrings;
    /**
     * @brief Number keys
     *
     * This arm of the union is an array of DWORDs that
     * should be used when querying by keys that are numbers,
     * such as UID or GID.
     */
    LW_PDWORD pdwIds;
} LSA_QUERY_LIST;

typedef LSA_QUERY_LIST *PLSA_QUERY_LIST;

typedef struct __LSA_USER_MOD_INFO_2
{
    LW_PSTR pszSid;

    struct _usermod_actions_2 {
        LW_BOOLEAN bEnableUser;
        LW_BOOLEAN bDisableUser;
        LW_BOOLEAN bUnlockUser;
        LW_BOOLEAN bSetChangePasswordOnNextLogon;
        LW_BOOLEAN bSetPasswordNeverExpires;
        LW_BOOLEAN bSetPasswordMustExpire;
        LW_BOOLEAN bAddToGroups;
        LW_BOOLEAN bRemoveFromGroups;
        LW_BOOLEAN bSetAccountExpiryDate;
        LW_BOOLEAN bSetHomedir;
        LW_BOOLEAN bSetShell;
        LW_BOOLEAN bSetGecos;
        LW_BOOLEAN bSetPrimaryGroup;
        LW_BOOLEAN bSetNtPasswordHash;
        LW_BOOLEAN bSetLmPasswordHash;
        LW_BOOLEAN bSetPassword;
    } actions;

    LW_PSTR pszPrimaryGroupSid;
    LW_PSTR pszAddToGroups;
    LW_PSTR pszRemoveFromGroups;
    LW_PSTR pszExpiryDate;
    LW_PSTR pszHomedir;
    LW_PSTR pszShell;
    LW_PSTR pszGecos;
    LW_PSTR pszPassword;
    PLW_LSA_DATA_BLOB pNtPasswordHash;
    PLW_LSA_DATA_BLOB pLmPasswordHash;

} LSA_USER_MOD_INFO_2, *PLSA_USER_MOD_INFO_2;

typedef struct __LSA_GROUP_MOD_INFO_2
{
    LW_PSTR pszSid;

    struct _groupmod_actions_2 {
        LW_BOOLEAN bAddMembers;
        LW_BOOLEAN bRemoveMembers;
    } actions;

    LW_DWORD dwAddMembersNum;
    LW_PSTR* ppszAddMembers;

    LW_DWORD dwRemoveMembersNum;
    LW_PSTR* ppszRemoveMembers;
} LSA_GROUP_MOD_INFO_2, *PLSA_GROUP_MOD_INFO_2;

typedef struct _LSA_USER_ADD_INFO
{
    LW_PSTR pszName;
    uid_t uid;
    LW_PSTR pszPrimaryGroupSid;
    LW_PSTR pszPassword;
    LW_PSTR pszGecos;
    LW_PSTR pszShell;
    LW_PSTR pszHomedir;
} LSA_USER_ADD_INFO, *PLSA_USER_ADD_INFO;

typedef struct _LSA_GROUP_ADD_INFO
{
    LW_PSTR pszName;
    gid_t gid;
    LW_DWORD dwMemberCount;
    LW_PSTR* ppszMemberSids;
} LSA_GROUP_ADD_INFO, *PLSA_GROUP_ADD_INFO;

/**
 * @brief Resolve security objects
 *
 * Resolves a homogeneous list of keys to a list of security objects of equal length.
 * The returned list should be freed with #LsaFreeSecurityObjectList().
 *
 * @param[in] hLsa a connection handle
 * @param[in] pszTargetProvider an optional provider name.  If provided, only that provider
 * will be queried.  Otherwise, all providers will be queried
 * @param[in] FindFlags flags that can modify query behavior
 * @param[in] ObjectType the type of object to return.  If #LSA_OBJECT_TYPE_UNDEFINED,
 * any type of object matching the query will be returned.
 * @param[in] QueryType the type of key to query by
 * @param[in] dwCount the number of keys to search for
 * @param[in] QueryList a list of keys to search for
 * @param[out] pppObjects on success, set to an array of security object pointers equal in
 * length to the query list.  Each element of the array may be NULL if the key could not be found.
 * @retval LW_ERROR_SUCCESS success
 */
LW_DWORD
LsaFindObjects(
    LW_IN LW_HANDLE hLsa,
    LW_IN LW_PCSTR pszTargetProvider,
    LW_IN LSA_FIND_FLAGS FindFlags,
    LW_IN LW_OPTIONAL LSA_OBJECT_TYPE ObjectType,
    LW_IN LSA_QUERY_TYPE QueryType,
    LW_IN LW_DWORD dwCount,
    LW_IN LSA_QUERY_LIST QueryList,
    LW_OUT PLSA_SECURITY_OBJECT** pppObjects
    );

/**
 * @brief Begin object enumeration
 *
 * Returns a handle that can be used to enumerate all security objects
 * matching the query criteria.  The handle should be freed with
 * #LsaCloseEnum().
 *
 * @param[in] hLsa a connection handle
 * @param[in] pszTargetProvider an optional provider name.  If provided, only that provider
 * will be queried.  Otherwise, all providers will be queried
 * @param[out] phEnum set to a handle which can be subsequently used with #LsaEnumObjects()
 * to retrieve results.
 * @param[in] FindFlags flags that can modify query behavior
 * @param[in] ObjectType the type of object to enumerate.  If #LSA_OBJECT_TYPE_UNDEFINED,
 * all enumerable objects will be returned.
 * @param[in] pszDomainName an optional domain name to further filter results
 * @retval LW_ERROR_SUCCESS success
 */
LW_DWORD
LsaOpenEnumObjects(
    LW_IN LW_HANDLE hLsa,
    LW_IN LW_PCSTR pszTargetProvider,
    LW_OUT LW_PHANDLE phEnum,
    LW_IN LSA_FIND_FLAGS FindFlags,
    LW_IN LSA_OBJECT_TYPE ObjectType,
    LW_IN LW_OPTIONAL LW_PCSTR pszDomainName
    );

/**
 * @brief Enumerate objects
 *
 * Returns objects from an in-progress enumeration.  The returned list
 * should be freed with #LsaFreeSecurityObjectList().
 *
 * @param[in] hLsa a connection handle
 * @param[in] hEnum an enumeration handle from #LsaOpenEnumObjects()
 * @param[in] dwMaxObjectsCount the maximum number of objects to return
 * @param[out] pdwObjectsCount set to the length of the returned list
 * @param[out] pppObjects set to a list of returned security objects.
 * Unlike #LsaFindObjects(), entries will not be NULL.
 * @retval LW_ERROR_SUCCESS success
 */
LW_DWORD
LsaEnumObjects(
    LW_IN LW_HANDLE hLsa,
    LW_IN LW_HANDLE hEnum,
    LW_IN LW_DWORD dwMaxObjectsCount,
    LW_OUT LW_PDWORD pdwObjectsCount,
    LW_OUT PLSA_SECURITY_OBJECT** pppObjects
    );

/**
 * @brief Begin member enumeration
 *
 * Returns a handle that can be used to enumerate all direct members of a group.
 * The returned SIDs may represent users or other groups.
 * The handle should be freed with #LsaCloseEnum().
 *
 * @param[in] hLsa a connection handle
 * @param[in] pszTargetProvider an optional provider name.  If provided, only that provider
 * will be queried.  Otherwise, all providers will be queried
 * @param[out] phEnum set to a handle which can be subsequently used with #LsaEnumMembers()
 * to retrieve results.
 * @param[in] FindFlags flags that can modify query behavior
 * @param[in] pszSid the SID of the group to enumerate as a printed string
 * @retval LW_ERROR_SUCCESS success
 * @retval LW_ERROR_NO_SUCH_GROUP a group with the specified SID did not exist
 */
LW_DWORD
LsaOpenEnumMembers(
    LW_IN LW_HANDLE hLsa,
    LW_IN LW_PCSTR pszTargetProvider,
    LW_OUT LW_PHANDLE phEnum,
    LW_IN LSA_FIND_FLAGS FindFlags,
    LW_IN LW_PCSTR pszSid
    );

/**
 * @brief Enumerate members
 *
 * Returns member SIDs from an in-progress member enumeration.
 * The returned list should be freed with #LsaFreeSidList().
 *
 * @param[in] hLsa a connection handle
 * @param[in] hEnum an enumeration handle from #LsaOpenEnumMembers()
 * @param[in] dwMaxObjectsCount the maximum number of SIDs to return
 * @param[out] pdwObjectsCount set to the length of the returned list
 * @param[out] pppszMember set to a list of returned member SIDs in
 * printed form
 * @retval LW_ERROR_SUCCESS success
 */
LW_DWORD
LsaEnumMembers(
    LW_IN LW_HANDLE hLsa,
    LW_IN LW_HANDLE hEnum,
    LW_IN LW_DWORD dwMaxObjectsCount,
    LW_OUT LW_PDWORD pdwObjectsCount,
    LW_OUT LW_PSTR** pppszMember
    );

/**
 * @brief Query group membership of objects
 *
 * Given a list of object SIDs, returns a list of group SIDs
 * of which the specified objects are direct or transitive members.
 *
 * @param[in] hLsa a connection handle
 * @param[in] pszTargetProvider an optional provider name.  If provided, only that provider
 * will be queried.  Otherwise, all providers will be queried
 * @param[in] FindFlags flags that can modify query behavior
 * @param[in] dwSidCount the number of SIDs passed
 * @param[in] ppszSids the list of SIDs
 * @param[out] pdwGroupSidCount set to the number of group SIDs returned
 * @param[out] pppszGroupSids set to an array containing group SIDs in printed form
 * @retval LW_ERROR_SUCCESS success
 */
LW_DWORD
LsaQueryMemberOf(
    LW_IN LW_HANDLE hLsa,
    LW_IN LW_PCSTR pszTargetProvider,
    LW_IN LSA_FIND_FLAGS FindFlags,
    LW_DWORD dwSidCount,
    LW_IN LW_PSTR* ppszSids,
    LW_OUT LW_PDWORD pdwGroupSidCount,
    LW_OUT LW_PSTR** pppszGroupSids
    );

/**
 * @brief Close enumeration handle
 *
 * Closes any enumeration handle opened with an enumeration function.
 *
 * @param[in] hLsa a connection handle
 * @param[in,out] hEnum the enum handle to close
 * @retval LW_ERROR_SUCCESS success
 */
LW_DWORD
LsaCloseEnum(
    LW_IN LW_HANDLE hLsa,
    LW_IN LW_OUT LW_HANDLE hEnum
    );

/**
 * @brief Free SID list
 *
 * Frees a list of SID strings returned from a previous API call.
 *
 * @param[in] dwSidCount the number of SIDs in the array
 * @param[in,out] ppszSids the array to free
 */
LW_VOID
LsaFreeSidList(
    LW_IN LW_DWORD dwSidCount,
    LW_IN LW_OUT LW_PSTR* ppszSids
    );

/**
 * @brief Free security object list
 *
 * Frees a list of security objects returned from a previous API call.
 * @param[in] dwObjectCount the number of objects in the array
 * @param[in,out] ppObjects the array to free
 */
LW_VOID
LsaFreeSecurityObjectList(
    LW_IN LW_DWORD dwObjectCount,
    LW_IN LW_OUT PLSA_SECURITY_OBJECT* ppObjects
    );

/**
 * @brief Free security object
 *
 * Frees a single security object
 * @param[in,out] pObject the object to free
 */
LW_VOID
LsaFreeSecurityObject(
    LW_IN LW_OUT PLSA_SECURITY_OBJECT pObject
    );

/**
 * @brief Query expanded group membership
 *
 * Returns a list of security objects of all transitive members
 * of a group.  This is equivalent to recursive calls to
 * #LsaOpenEnumMembers()/#LsaEnumMembers()/#LsaCloseEnum()
 * and #LsaFindObjects().  The returned list should be freed
 * with #LsaFreeSecurityObjectList().
 *
 * @param[in] hLsa a connection handle
 * @param[in] pszTargetProvider an optional provider name.  If provided, only that provider
 * will be queried.  Otherwise, all providers will be queried
 * @param[in] FindFlags flags that can modify query behavior
 * @param[in] ObjectType the type of member to return. If #LSA_OBJECT_TYPE_UNDEFINED, all
 * members are returned
 * @param[in] pszSid the SID of the group to expand as a printed string
 * @param[out] pdwMemberCount set to the number of returned members
 * @param[out] pppMembers set to the list of returned members
 * @retval LW_ERROR_SUCCESS success
 * @retval LW_ERROR_NO_SUCH_GROUP a group with the specified SID did not exist
 */
LW_DWORD
LsaQueryExpandedGroupMembers(
    LW_IN LW_HANDLE hLsa,
    LW_PCSTR pszTargetProvider,
    LW_IN LSA_FIND_FLAGS FindFlags,
    LW_IN LSA_OBJECT_TYPE ObjectType,
    LW_IN LW_PCSTR pszSid,
    LW_OUT LW_PDWORD pdwMemberCount,
    LW_OUT PLSA_SECURITY_OBJECT** pppMembers
    );

/**
 * @brief Find group and its expanded membership list
 *
 * Returns a security object for a group along with a list
 * of its expanded members.  This is equivalent to #LsaFindObjects()
 * followed by #LsaQueryExpandedGroupMembers() of the resulting SID,
 * but with reduced overhead. The returned group security object should
 * be freed with #LsaFreeSecurityObject(), and the returned member
 * list should be freed with #LsaFreeSecurityObjectList().
 *
 * @param[in] hLsa a connection handle
 * @param[in] pszTargetProvider an optional provider name.  If provided, only that provider
 * will be queried.  Otherwise, all providers will be queried
 * @param[in] FindFlags flags that can modify query behavior
 * @param[in] QueryType the type of key to query by
 * @param[in] QueryItem the key to query by
 * @param[out] ppGroupObject set to the security object for the group
 * @param[out] pdwMemberObjectCount set to the number of returned members
 * @param[out] pppMemberObjects set to the list of returned members
 * @retval LW_ERROR_SUCCESS success
 * @retval LW_ERROR_NO_SUCH_GROUP the group was not found
 */
LW_DWORD
LsaFindGroupAndExpandedMembers(
    LW_IN LW_HANDLE hLsa,
    LW_PCSTR pszTargetProvider,
    LW_IN LSA_FIND_FLAGS FindFlags,
    LW_IN LSA_QUERY_TYPE QueryType,
    LW_IN LSA_QUERY_ITEM QueryItem,
    LW_OUT PLSA_SECURITY_OBJECT* ppGroupObject,
    LW_OUT LW_PDWORD pdwMemberObjectCount,
    LW_OUT PLSA_SECURITY_OBJECT** pppMemberObjects
    );

LW_DWORD
LsaModifyUser2(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszTargetProvider,
    PLSA_USER_MOD_INFO_2 pUserModInfo
    );

LW_DWORD
LsaDeleteObject(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszTargetProvider,
    LW_PCSTR pszSid
    );

LW_DWORD
LsaModifyGroup2(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszTargetProvider,
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo
    );

LW_DWORD
LsaAddGroup2(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszTargetProvider,
    PLSA_GROUP_ADD_INFO pGroupAddInfo
    );

LW_DWORD
LsaAddUser2(
    LW_HANDLE hLsaConnection,
    LW_PCSTR pszTargetProvider,
    PLSA_USER_ADD_INFO pUserAddInfo
    );

LW_DWORD
LsaGetSmartCardUserObject(
    LW_IN LW_HANDLE hLsaConnection,
    LW_OUT PLSA_SECURITY_OBJECT* ppObject,
    LW_OUT LW_PSTR* ppszSmartCardReader
    );

//
// Provider Tags
//

#define LSA_PROVIDER_TAG_LOCAL "lsa-local-provider"
#define LSA_PROVIDER_TAG_AD "lsa-activedirectory-provider"

/*@}*/

#endif /* __LSA_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
