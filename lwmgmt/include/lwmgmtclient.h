#ifndef __LWMGMTCLIENT_H__
#define __LWMGMTCLIENT_H__

typedef DWORD LSA_DS_FLAGS, *PLSA_DS_FLAGS;

#define LSA_DS_DNS_CONTROLLER_FLAG  0x20000000
#define LSA_DS_DNS_DOMAIN_FLAG      0x40000000
#define LSA_DS_DNS_FOREST_FLAG      0x80000000
#define LSA_DS_DS_FLAG              0x00000010
#define LSA_DS_GC_FLAG              0x00000004
#define LSA_DS_KDC_FLAG             0x00000020
#define LSA_DS_PDC_FLAG             0x00000001
#define LSA_DS_TIMESERV_FLAG        0x00000040
#define LSA_DS_WRITABLE_FLAG        0x00000100

typedef DWORD LSA_DM_DOMAIN_FLAGS, *PLSA_DM_DOMAIN_FLAGS;

#define LSA_DM_DOMAIN_FLAG_PRIMARY               0x00000001
#define LSA_DM_DOMAIN_FLAG_OFFLINE               0x00000002
#define LSA_DM_DOMAIN_FLAG_FORCE_OFFLINE         0x00000004
#define LSA_DM_DOMAIN_FLAG_TRANSITIVE_1WAY_CHILD 0x00000008
#define LSA_DM_DOMAIN_FLAG_FOREST_ROOT           0x00000010
#define LSA_DM_DOMAIN_FLAG_GC_OFFLINE            0x00000020

typedef DWORD LSA_TRUST_TYPE, *PLSA_TRUST_TYPE;

#define LSA_TRUST_TYPE_DOWNLEVEL            0x00000001
#define LSA_TRUST_TYPE_UPLEVEL              0x00000002
#define LSA_TRUST_TYPE_MIT                  0x00000003
#define LSA_TRUST_TYPE_DCE                  0x00000004

typedef DWORD LSA_TRUST_ATTRIBUTE, *PLSA_TRUST_ATTRIBUTE;

#define LSA_TRUST_ATTRIBUTE_NON_TRANSITIVE     0x00000001
#define LSA_TRUST_ATTRIBUTE_UPLEVEL_ONLY       0x00000002
#define LSA_TRUST_ATTRIBUTE_FILTER_SIDS        0x00000004
#define LSA_TRUST_ATTRIBUTE_FOREST_TRANSITIVE  0x00000008
#define LSA_TRUST_ATTRIBUTE_CROSS_ORGANIZATION 0x00000010
#define LSA_TRUST_ATTRIBUTE_WITHIN_FOREST      0x00000020

typedef DWORD LSA_TRUST_FLAG, *PLSA_TRUST_FLAG;

#define LSA_TRUST_FLAG_IN_FOREST    0x00000001
#define LSA_TRUST_FLAG_OUTBOUND     0x00000002
#define LSA_TRUST_FLAG_TREEROOT     0x00000004
#define LSA_TRUST_FLAG_PRIMARY      0x00000008
#define LSA_TRUST_FLAG_NATIVE       0x00000010
#define LSA_TRUST_FLAG_INBOUND      0x00000020


typedef struct __LSA_METRIC_PACK_0
{
    UINT64 failedAuthentications;
    UINT64 failedUserLookupsByName;
    UINT64 failedUserLookupsById;
    UINT64 failedGroupLookupsByName;
    UINT64 failedGroupLookupsById;
    UINT64 failedOpenSession;
    UINT64 failedCloseSession;
    UINT64 failedChangePassword;
    UINT64 unauthorizedAccesses;

} LSA_METRIC_PACK_0, *PLSA_METRIC_PACK_0;

typedef struct __LSA_METRIC_PACK_1
{
    UINT64 successfulAuthentications;
    UINT64 failedAuthentications;
    UINT64 rootUserAuthentications;
    UINT64 successfulUserLookupsByName;
    UINT64 failedUserLookupsByName;
    UINT64 successfulUserLookupsById;
    UINT64 failedUserLookupsById;
    UINT64 successfulGroupLookupsByName;
    UINT64 failedGroupLookupsByName;
    UINT64 successfulGroupLookupsById;
    UINT64 failedGroupLookupsById;
    UINT64 successfulOpenSession;
    UINT64 failedOpenSession;
    UINT64 successfulCloseSession;
    UINT64 failedCloseSession;
    UINT64 successfulChangePassword;
    UINT64 failedChangePassword;
    UINT64 unauthorizedAccesses;

} LSA_METRIC_PACK_1, *PLSA_METRIC_PACK_1;

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
    LSA_AUTH_PROVIDER_STATUS_OFFLINE
} LsaAuthProviderStatus;

typedef struct __LSA_DC_INFO
{
    PSTR         pszName;
    PSTR         pszAddress;
    PSTR         pszSiteName;
    LSA_DS_FLAGS dwFlags;
} LSA_DC_INFO, *PLSA_DC_INFO;

typedef struct __LSA_TRUSTED_DOMAIN_INFO
{
    PSTR                pszDnsDomain;
    PSTR                pszNetbiosDomain;
    PSTR                pszTrusteeDnsDomain;
    PSTR                pszDomainSID;
    PSTR                pszDomainGUID;
    PSTR                pszForestName;
    PSTR                pszClientSiteName;
    LSA_TRUST_FLAG      dwTrustFlags;
    LSA_TRUST_TYPE      dwTrustType;
    LSA_TRUST_ATTRIBUTE dwTrustAttributes;
    LSA_DM_DOMAIN_FLAGS dwDomainFlags;
    PLSA_DC_INFO        pDCInfo;
    PLSA_DC_INFO        pGCInfo;
} LSA_TRUSTED_DOMAIN_INFO, *PLSA_TRUSTED_DOMAIN_INFO;

typedef struct __LWMGMT_LSA_AUTH_PROVIDER_STATUS
{

    PSTR                     pszId;
    LsaAuthProviderMode      mode;
    LsaAuthProviderSubMode   subMode;
    LsaAuthProviderStatus    status;
    PSTR                     pszDomain;
    PSTR                     pszForest;
    PSTR                     pszSite;
    PSTR                     pszCell;
    DWORD                    dwNetworkCheckInterval;
    DWORD                    dwNumTrustedDomains;
    PLSA_TRUSTED_DOMAIN_INFO pTrustedDomainInfoArray;

} LSA_AUTH_PROVIDER_STATUS, *PLSA_AUTH_PROVIDER_STATUS;

typedef struct __LSA_STATUS
{
    struct
    {
        DWORD dwMajor;
        DWORD dwMinor;
        DWORD dwBuild;
    } version;

    DWORD dwUptime;
    DWORD dwCount;
    PLSA_AUTH_PROVIDER_STATUS pAuthProviderStatusArray;

} LSA_STATUS, *PLSA_STATUS;

typedef struct __LWMGMT_LSA_KEYTAB_ENTRY
{
    DWORD timestamp;
    DWORD kvno;
    DWORD enctype;
    PSTR  pszPrincipal;
    PSTR  pszPassword;
} LWMGMT_LSA_KEYTAB_ENTRY, *PLWMGMT_LSA_KEYTAB_ENTRY;

typedef struct __LWMGMT_LSA_KEYTAB_ENTRIES
{
    DWORD                    dwCount;
    PLWMGMT_LSA_KEYTAB_ENTRY pLsaKeyTabEntryArray;
} LWMGMT_LSA_KEYTAB_ENTRIES, *PLWMGMT_LSA_KEYTAB_ENTRIES;

DWORD
LWMGMTQueryLsaMetrics_0(
    PCSTR pszHostname,
    PLSA_METRIC_PACK_0* ppPack
    );

VOID
LWMGMTFreeLsaMetrics_0(
    PLSA_METRIC_PACK_0 pPack
    );

DWORD
LWMGMTQueryLsaMetrics_1(
    PCSTR pszHostname,
    PLSA_METRIC_PACK_1* ppPack
    );

VOID
LWMGMTFreeLsaMetrics_1(
    PLSA_METRIC_PACK_1 pPack
    );

DWORD
LWMGMTQueryLsaStatus(
    PCSTR        pszHostname,
    PLSA_STATUS* ppLsaStatus
    );

VOID
LWMGMTFreeLsaStatus(
    PLSA_STATUS pLsaStatus
    );

DWORD
LWMGMTReadKeyTab(
    PCSTR                        pszHostname,
    PCSTR                        pszKeyTabPath,
    DWORD                        dwLastRecordId,
    DWORD                        dwRecordsPerPage,
    PLWMGMT_LSA_KEYTAB_ENTRIES * ppKeyTabEntries
    );

VOID
LWMGMTFreeKeyTabEntries(
    PLWMGMT_LSA_KEYTAB_ENTRIES pKeyTabEntries
    );

DWORD
LWMGMTCountKeyTabEntries(
    PCSTR  pszHostname,
    PCSTR  pszKeyTabPath,
    PDWORD pdwCount
    );

#endif /* __LWMGMTCLIENT_H__ */

