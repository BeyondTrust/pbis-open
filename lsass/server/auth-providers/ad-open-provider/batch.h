/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        batch.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Wei Fu (wfu@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 */
#ifndef __BATCH_H__
#define __BATCH_H__

typedef UINT8 LSA_AD_BATCH_QUERY_TYPE, *PLSA_AD_BATCH_QUERY_TYPE;
#define LSA_AD_BATCH_QUERY_TYPE_UNDEFINED      0
#define LSA_AD_BATCH_QUERY_TYPE_BY_DN          1
#define LSA_AD_BATCH_QUERY_TYPE_BY_SID         2
#define LSA_AD_BATCH_QUERY_TYPE_BY_NT4         3
#define LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS  4
#define LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS 5
#define LSA_AD_BATCH_QUERY_TYPE_BY_UID         6
#define LSA_AD_BATCH_QUERY_TYPE_BY_GID         7

DWORD
LsaAdBatchFindObjects(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwQueryItemsCount,
    IN OPTIONAL PSTR* ppszQueryList,
    IN OPTIONAL PDWORD pdwId,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects,
    OUT OPTIONAL PDWORD pdwOfflineDomains,
    OUT OPTIONAL PSTR** pppszOfflineDomains
    );

DWORD
LsaAdBatchFindSingleObject(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN OPTIONAL PCSTR pszQueryTerm,
    IN OPTIONAL PDWORD dwId,
    OUT PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
LsaRemoveAlreadyEnumerated(
    IN OUT PLW_HASH_TABLE pEnumeratedSids,
    IN OUT PDWORD pObjectsCount,
    IN OUT PLSA_SECURITY_OBJECT* ppObjects
    );

DWORD
LsaAdBatchEnumObjects(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN OUT PLW_SEARCH_COOKIE pCookie,
    IN LSA_OBJECT_TYPE AccountType,
    IN OPTIONAL PCSTR pszDomainName,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects,
    OUT PDWORD pdwOfflineDomains,
    OUT PSTR **pppszOfflineDomains
    );

DWORD
LsaAdBatchGetDomainFromNT4Name(
    OUT PSTR* ppszDomainName,
    IN PCSTR pszNT4Name
    );

#endif /* __BATCH_H__ */

