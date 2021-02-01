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
 *     lsapstore-backend.h
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     Backend Interface
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __LSA_PSTORE_BACKEND_H__
#define __LSA_PSTORE_BACKEND_H__

#include <lsa/lsapstore-api.h>
#include "lsapstore-backend-types.h"

//
// A backend must implement the following functions
//

DWORD
LsaPstorepBackendInitialize(
    OUT PLSA_PSTORE_BACKEND_STATE* State
    );

VOID
LsaPstorepBackendCleanup(
    IN PLSA_PSTORE_BACKEND_STATE State
    );

DWORD
LsaPstorepBackendGetPasswordInfoW(
    IN PLSA_PSTORE_BACKEND_STATE State,
    IN PCWSTR DnsDomainName,
    OUT OPTIONAL PLSA_MACHINE_PASSWORD_INFO_W* PasswordInfo
    );

DWORD
LsaPstorepBackendSetPasswordInfoW(
    IN PLSA_PSTORE_BACKEND_STATE State,
    IN PLSA_MACHINE_PASSWORD_INFO_W PasswordInfo
    );

DWORD
LsaPstorepBackendDeletePasswordInfoW(
    IN PLSA_PSTORE_BACKEND_STATE State,
    IN PCWSTR DnsDomainName
    );

DWORD
LsaPstorepBackendGetDefaultDomainW(
    IN PLSA_PSTORE_BACKEND_STATE State,
    OUT PWSTR* DnsDomainName
    );
///<
/// Get default joined domain name.
///
/// In getting the default domain, the backend should validate
/// the there is password information for the returned domain.
///
/// @param[in] State - State from LsaPstorepBackendInitialize().
/// @param[out] DnsDomainName - Returns the default domain join DNS domain
///     name, or NULL if none.  Free with LsaPstoreFreeMemory().
///
/// @return Windows error code
/// @retval ERROR_SUCCESS on success
/// @retval !ERROR_SUCCESS on other failure
///

DWORD
LsaPstorepBackendSetDefaultDomainW(
    IN PLSA_PSTORE_BACKEND_STATE State,
    IN OPTIONAL PCWSTR DnsDomainName
    );
///<
/// Set default joined domain name.
///
/// @param[in] DnsDomainName - The DNS domain name to set as the default
///     domain join.  If NULL, will remove the default joined domain.
///
/// @return Windows error code
/// @retval ERROR_SUCCESS on success
/// @retval NERR_SetupNotJoined if not joined to specified domain.
/// @retval !ERROR_SUCCESS on other failure
///

DWORD
LsaPstorepBackendGetJoinedDomainsW(
    IN PLSA_PSTORE_BACKEND_STATE State,
    OUT PWSTR** DnsDomainNames,
    OUT PDWORD Count
    );

DWORD
LsaPstorepBackendGetDomainTrustEnumerationWaitTime(
    IN PLSA_PSTORE_BACKEND_STATE State,
    IN OPTIONAL PCWSTR pwszDnsDomainName,
    OUT PDWORD* ppdwTrustEnumerationWaitSeconds,
    OUT PDWORD* ppdwTrustEnumerationWaitEnabled
    );

DWORD
LsaPstoreBackendSetDomainWTrustEnumerationWaitTime(
    IN PLSA_PSTORE_BACKEND_STATE State,
    IN OPTIONAL PCWSTR pwszDnsDomainName
    );

DWORD
LsaPstoreBackendDeleteTrustEnumerationWaitInfo(
    IN PLSA_PSTORE_BACKEND_STATE State,
    IN PCWSTR pwszDnsDomainName
    );

#endif /* __LSA_PSTORE_BACKEND_H__ */
