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
 *        adldap_p.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        AD LDAP helper functions (private header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */
#ifndef __ADLDAP_P_H__
#define __ADLDAP_P_H__

typedef struct _LSA_AD_QUERY_LISTS_ENTRY {
    DWORD dwQueryCount;
    PSTR* ppszQueryValues;
} LSA_AD_QUERY_LISTS_ENTRY, *PLSA_AD_QUERY_LISTS_ENTRY;

DWORD
UnprovisionedModeMakeLocalSID(
    PCSTR pszDomainSID,
    DWORD dwID,
    PSTR* ppszLocalSID
    );

DWORD
ADLdap_GetAttributeValuesList(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszDN,
    IN PCSTR pszAttributeName,
    IN BOOLEAN bDoExtDnSearch,
    IN BOOLEAN bDoSidParsing,
    OUT PDWORD pdwTotalCount,
    OUT PSTR** pppszValues
    );

DWORD
ADLdap_MoveHashKeysToArray(
    IN OUT PLW_HASH_TABLE pHash,
    OUT PDWORD pCount,
    OUT PVOID** pppValues
    );

#endif

