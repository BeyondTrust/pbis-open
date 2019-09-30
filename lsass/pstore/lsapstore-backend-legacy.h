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
 *     lsapstore-backend-legacy.h
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     Legacy Backend Common Includes
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __LSA_PSTORE_BACKEND_LEGACY__
#define __LSA_PSTORE_BACKEND_LEGACY__

#include <lw/types.h>
#include <lw/attrs.h>

#include <lsa/lsapstore-types.h>


#define PSTOREDB_REGISTRY_TRUSTENUMERATIONWAIT_VALUE \
    "TrustEnumerationWait"
#define PSTOREDB_REGISTRY_TRUSTENUMERATIONWAITSECONDS_VALUE \
    "TrustEnumerationWaitSeconds"


//
// From lsapstore-backend-legacy-internal.c
//

typedef struct _LWPS_LEGACY_STATE *PLWPS_LEGACY_STATE;

DWORD
LwpsLegacyOpenProvider(
    OUT PLWPS_LEGACY_STATE* ppContext
    );

VOID
LwpsLegacyCloseProvider(
    IN PLWPS_LEGACY_STATE pContext
    );

DWORD
LwpsLegacyReadPassword(
    IN PLWPS_LEGACY_STATE pContext,
    IN PCSTR pszDnsDomainName,
    OUT OPTIONAL PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    );

DWORD
LwpsLegacyWritePassword(
    IN PLWPS_LEGACY_STATE pContext,
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    );

DWORD
LwpsLegacyDeletePassword(
    IN PLWPS_LEGACY_STATE pContext,
    IN PCSTR pszDomainName
    );

DWORD
LwpsLegacyGetDefaultJoinedDomain(
    IN PLWPS_LEGACY_STATE pContext,
    OUT PSTR* ppszDomainName
    );

DWORD
LwpsLegacySetDefaultJoinedDomain(
    IN PLWPS_LEGACY_STATE pContext,
    IN OPTIONAL PCSTR pszDomainName
    );

DWORD
LwpsLegacyGetJoinedDomains(
    IN PLWPS_LEGACY_STATE pContext,
    OUT PSTR** pppszDomainList,
    OUT PDWORD pdwDomainCount
    );

VOID
LwpsLegacyFreeStringArray(
    IN PSTR* ppszDomainList,
    IN DWORD dwCount
    );

typedef struct _LSA_PSTORE_BACKEND_STATE {
    PLWPS_LEGACY_STATE OldStoreHandle;
} LSA_PSTORE_BACKEND_STATE;


DWORD
LwpsLegacySetJoinedDomainTrustEnumerationWaitTime(
    IN PLWPS_LEGACY_STATE pContext,
    IN OPTIONAL PCSTR pszDomainName
    );

DWORD
LwpsLegacyGetJoinedDomainTrustEnumerationWaitTime(
    IN PLWPS_LEGACY_STATE pContext,
    IN OPTIONAL PCSTR pszDomain,
    OUT PDWORD* ppdwTrustEnumerationWaitSeconds,
    OUT PDWORD* ppdwTrustEnumerationWaitEnabled
    );

DWORD
LwpsLegacyDeleteTrustEnumerationWaitInfo(
    IN PLWPS_LEGACY_STATE pContext,
    IN PCSTR pszDomainName
    );

#endif
