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
 *        batch_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Wei Fu (wfu@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 */
#ifndef __BATCH_P_H__
#define __BATCH_P_H__

#include "batch_common.h"

// zero means unlimited
#define LSA_AD_BATCH_MAX_QUERY_SIZE 0
#define LSA_AD_BATCH_MAX_QUERY_COUNT 1000

typedef DWORD LSA_PROVISIONING_MODE, *PLSA_PROVISIONING_MODE;
#define LSA_PROVISIONING_MODE_DEFAULT_CELL     1
#define LSA_PROVISIONING_MODE_NON_DEFAULT_CELL 2
#define LSA_PROVISIONING_MODE_UNPROVISIONED    3

static
DWORD
LsaAdBatchCreateDomainEntry(
    OUT PLSA_AD_BATCH_DOMAIN_ENTRY* ppEntry,
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszQueryTerm
    );

static
VOID
LsaAdBatchDestroyDomainEntry(
    IN OUT PLSA_AD_BATCH_DOMAIN_ENTRY* ppEntry
    );

static
DWORD
LsaAdBatchCreateBatchItem(
    OUT PLSA_AD_BATCH_ITEM* ppItem,
    IN PLSA_AD_BATCH_DOMAIN_ENTRY pDomainEntry,
    IN LSA_AD_BATCH_QUERY_TYPE QueryTermType,
    IN OPTIONAL PCSTR pszString,
    IN OPTIONAL PDWORD pdwId
    );

static
VOID
LsaAdBatchDestroyBatchItem(
    IN OUT PLSA_AD_BATCH_ITEM* ppItem
    );

static
DWORD
LsaAdBatchFindObjectsForDomainEntry(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN BOOLEAN bResolvePseudoObjects,
    IN OUT PLSA_AD_BATCH_DOMAIN_ENTRY pEntry
    );

static
DWORD
LsaAdBatchFindObjectsForDomain(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN BOOLEAN bIsOneWayTrust,
    IN BOOLEAN bResolvePseudoObjects,
    IN DWORD dwCount,
    IN OUT PLSA_LIST_LINKS pBatchItemList
    );

// Resolve Functions

static
DWORD
LsaAdBatchResolveRpcObjects(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    );

static
DWORD
LsaAdBatchResolveRealObjects(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    );

static
DWORD
LsaAdBatchResolvePseudoObjects(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    OUT OPTIONAL PBOOLEAN pbResolvedPseudo
    );

static
DWORD
LsaAdBatchResolvePseudoObjectsWithLinkedCells(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    );

// Process Functions

static
DWORD
LsaAdBatchProcessRpcObject(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PLSA_AD_BATCH_ITEM pItem,
    IN PSTR pszObjectNT4NameOrSid,
    IN PLSA_TRANSLATED_NAME_OR_SID pTranslatedName
    );

static
DWORD
LsaAdBatchProcessRealObject(
    IN PAD_PROVIDER_DATA pProviderData,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pStartBatchItemListLinks,
    IN PLSA_LIST_LINKS pEndBatchItemListLinks,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    );

static
DWORD
LsaAdBatchProcessPseudoObject(
    IN PAD_PROVIDER_DATA pProviderData,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pStartBatchItemListLinks,
    IN PLSA_LIST_LINKS pEndBatchItemListLinks,
    OUT PDWORD pdwFoundCount,
    IN BOOLEAN bIsGcSearch,
    IN BOOLEAN bIsSchemaMode,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    );

static
DWORD
LsaAdBatchProcessPseudoObjectDefaultSchema(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pStartBatchItemListLinks,
    IN PLSA_LIST_LINKS pEndBatchItemListLinks,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    );

// Utility Functions

static
PCSTR
LsaAdBatchGetQueryTypeAsString(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType
    );

static
DWORD
LsaAdBatchConvertQTListToBIList(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwQueryItemsCount,
    IN OPTIONAL PSTR* ppszQueryList,
    IN OPTIONAL PDWORD pdwId,
    OUT PLSA_LIST_LINKS pBatchItemList,
    OUT PDWORD pdwTotalBatchItemCount
    );

static
DWORD
LsaAdBatchSplitBIListToBIListPerDomain(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    OUT PLSA_LIST_LINKS pDomainList
    );

static
DWORD
LsaAdBatchResolvePseudoObjectsInternalDefaultSchema(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    OUT OPTIONAL PDWORD pdwTotalItemFoundCount
    );

static
DWORD
LsaAdBatchResolvePseudoObjectsInternalDefaultOrCell(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszCellDn,
    IN BOOLEAN bDoGCSearch,
    IN ADConfigurationMode adMode,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    OUT OPTIONAL PDWORD pdwTotalItemFoundCount
    );

#endif /* __BATCH_P_H__ */
