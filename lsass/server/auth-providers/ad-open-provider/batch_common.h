/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        batch_common.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#ifndef _BATCH_COMMON_H_
#define _BATCH_COMMON_H_

#define XXX

#define LSA_XFER_STRING(Source, Target) \
    ((Target) = (Source), (Source) = NULL)

#define AD_LDAP_CLASS_LW_USER      "centerisLikewiseUser"
#define AD_LDAP_CLASS_LW_GROUP     "centerisLikewiseGroup"
#define AD_LDAP_CLASS_SCHEMA_USER  "posixAccount"
#define AD_LDAP_CLASS_SCHEMA_GROUP "posixGroup"
#define AD_LDAP_CLASS_NON_SCHEMA   "serviceConnectionPoint"

typedef UINT8 LSA_AD_BATCH_DOMAIN_ENTRY_FLAGS;
// If this is set, we are not supposed to process
// this domain.
#define LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_SKIP             0x01
// If this is set, we are dealing with one-way trust scenario.
#define LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_IS_ONE_WAY_TRUST 0x02


typedef UINT8 LSA_AD_BATCH_OBJECT_TYPE, *PLSA_AD_BATCH_OBJECT_TYPE;
#define LSA_AD_BATCH_OBJECT_TYPE_UNDEFINED 0
#define LSA_AD_BATCH_OBJECT_TYPE_USER      1
#define LSA_AD_BATCH_OBJECT_TYPE_GROUP     2

typedef UINT8 LSA_AD_BATCH_ITEM_FLAGS, *PLSA_AD_BATCH_ITEM_FLAGS;
#define LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO             0x01
#define LSA_AD_BATCH_ITEM_FLAG_HAVE_REAL               0x02
#define LSA_AD_BATCH_ITEM_FLAG_DISABLED                0x04
#define LSA_AD_BATCH_ITEM_FLAG_ERROR                   0x08
#define LSA_AD_BATCH_ITEM_FLAG_ALLOCATED_MATCH_TERM    0x10
#define LSA_AD_BATCH_ITEM_FLAG_ACCOUNT_INFO_KNOWN      0x20

typedef struct _LSA_AD_BATCH_DOMAIN_ENTRY {
    PSTR pszDnsDomainName;
    PSTR pszNetbiosDomainName;
    LSA_AD_BATCH_DOMAIN_ENTRY_FLAGS Flags;
    LSA_AD_BATCH_QUERY_TYPE QueryType;

    // The presence of these depend on the query type.
    union {
        struct {
            // pszDcPart is not allocated, but points to
            // strings that last longer than this structure.
            PCSTR pszDcPart;
        } ByDn;
        struct {
            // Allocated
            PSTR pszDomainSid;
            size_t sDomainSidLength;
        } BySid;
        struct {
            size_t sNetbiosDomainNameLength;
            size_t sDnsDomainNameLength;
        } ByNT4;
    } QueryMatch;

    // Number of items in BatchItemList.
    DWORD dwBatchItemCount;
    // List of LSA_AD_BATCH_ITEM.
    LSA_LIST_LINKS BatchItemList;

    // Links for list of domain entry.
    LSA_LIST_LINKS DomainEntryListLinks;
} LSA_AD_BATCH_DOMAIN_ENTRY, *PLSA_AD_BATCH_DOMAIN_ENTRY;

typedef struct _LSA_AD_BATCH_QUERY_TERM {
    LSA_AD_BATCH_QUERY_TYPE Type;
    union {
        // This can be a DN, SID, SAM account name, or alias.
        // It is not allocated.  Rather, it points to data
        // that lasts longer than this structure.
        // NOTE: For an "by NT4" query, we just put the SAM
        // account name here.
        PCSTR pszString;
        // This can be a uid or gid.
        DWORD dwId;
    };
} LSA_AD_BATCH_QUERY_TERM, *PLSA_AD_BATCH_QUERY_TERM;

typedef struct _LSA_AD_BATCH_ITEM_USER_INFO {
    // Unix fields in struct passwd order:
    PSTR pszAlias;
    PSTR pszPasswd;
    uid_t uid;
    gid_t gid;
    PSTR pszGecos;
    PSTR pszHomeDirectory;
    PSTR pszShell;
    PSTR pszLocalWindowsHomeFolder;
    // AD-specific fields:
    PSTR pszUserPrincipalName;
    PSTR pszDisplayName;
    PSTR pszWindowsHomeFolder;
    DWORD dwPrimaryGroupRid;
    UINT32 UserAccountControl;
    UINT64 AccountExpires;
    UINT64 PasswordLastSet;
    UINT64 PasswordExpires;
} LSA_AD_BATCH_ITEM_USER_INFO, *PLSA_AD_BATCH_ITEM_USER_INFO;

typedef struct _LSA_AD_BATCH_ITEM_GROUP_INFO {
    // Unix fields in struct group order:
    PSTR pszAlias;
    PSTR pszPasswd;
    gid_t gid;
} LSA_AD_BATCH_ITEM_GROUP_INFO, *PLSA_AD_BATCH_ITEM_GROUP_INFO;

XXX; // eventually remove DN field...
typedef struct _LSA_AD_BATCH_ITEM {
    LSA_AD_BATCH_QUERY_TERM QueryTerm;
    PSTR pszQueryMatchTerm;
    LSA_LIST_LINKS BatchItemListLinks;
    LSA_AD_BATCH_ITEM_FLAGS Flags;

    // Non-specific fields:
    PSTR pszSid;
    PSTR pszSamAccountName;
    PSTR pszDn;
    PSTR pszPseudoDn;
    DWORD FoundPseudoCount;
    LSA_AD_BATCH_OBJECT_TYPE ObjectType;
    // User/Group-specific fields:
    union {
        LSA_AD_BATCH_ITEM_USER_INFO UserInfo;
        LSA_AD_BATCH_ITEM_GROUP_INFO GroupInfo;
    };
} LSA_AD_BATCH_ITEM, *PLSA_AD_BATCH_ITEM;

VOID
LsaAdBatchDestroyBatchItemContents(
    IN OUT PLSA_AD_BATCH_ITEM pItem
    );

BOOLEAN
LsaAdBatchIsDefaultSchemaMode(
    PAD_PROVIDER_DATA pProviderData
    );

BOOLEAN
LsaAdBatchIsUnprovisionedMode(
    PAD_PROVIDER_DATA pProviderData
    );

DWORD
LsaAdBatchIsDefaultCell(
    IN PAD_PROVIDER_DATA pProviderData,
    IN PCSTR pszCellDN,
    OUT PBOOLEAN pbIsDefaultCell
    );

DWORD
LsaAdBatchQueryCellConfigurationMode(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszCellDN,
    OUT ADConfigurationMode* pAdMode
    );

PCSTR
LsaAdBatchFindKeywordAttributeWithEqual(
    IN DWORD dwKeywordValuesCount,
    IN PSTR* ppszKeywordValues,
    IN PCSTR pszAttributeNameWithEqual,
    IN size_t sAttributeNameWithEqualLength
    );

#define LsaAdBatchFindKeywordAttributeStatic(Count, Keywords, StaticString) \
    LsaAdBatchFindKeywordAttributeWithEqual(Count, Keywords, StaticString "=", sizeof(StaticString "=") - 1)

VOID
LsaAdBatchQueryTermDebugInfo(
    IN PLSA_AD_BATCH_QUERY_TERM pQueryTerm,
    OUT OPTIONAL PCSTR* ppszType,
    OUT OPTIONAL PBOOLEAN pbIsString,
    OUT OPTIONAL PCSTR* ppszString,
    OUT OPTIONAL PDWORD pdwId
    );

BOOLEAN
LsaAdBatchIsUserOrGroupObjectType(
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType
    );

LSA_AD_BATCH_OBJECT_TYPE
LsaAdBatchGetObjectTypeFromAccountType(
    IN LSA_OBJECT_TYPE AccountType
    );

LSA_AD_BATCH_OBJECT_TYPE
LsaAdBatchGetObjectTypeFromQueryType(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType
    );

BOOLEAN
LsaAdBatchHasValidCharsForSid(
    IN PCSTR pszSidString
    );

#endif /* BATCH_COMMON_H_ */
