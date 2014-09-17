/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsapstore-api.h
 *
 * Abstract:
 *
 *        LSA Password Store API
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __LSA_PSTORE_API_H__
#define __LSA_PSTORE_API_H__


////////////////////////////////////////////////////////////////////////////
// lsa/lsapstore-api.h
////////////////////////////////////////////////////////////////////////////

#include <lsa/lsapstore-types.h>
#include <lw/attrs.h>

//
// Low-Level API
//
// This is a helper library provided by the LSASS AD provider.  It is used by
// the AD provider and for upgrade tool(s) that must run when LSASS is not
// running.
//
// NOTE: This is a PWSTR-only API.
//
// NOTE: Currently missing logging wrt old API.  It is used by eventfwd and
// lsass.  The former will just use new LSA AD API.  The latter will just log
// on bail macros for now.
//

#define TRUST_ENUMERATIONWAIT_MAXLIMIT 3600
#define TRUST_ENUMERATIONWAIT_DEFAULTVALUE 300

DWORD
LsaPstoreSetDomainWTrustEnumerationWaitTime(
    IN OPTIONAL PCWSTR pwszDnsDomainName
    );

DWORD
LsaPstoreGetDomainTrustEnumerationWaitTime(
   IN OPTIONAL PCSTR pszDnsDomainName,
   OUT PDWORD* ppdwTrustEnumerationWaitSeconds,
   OUT PDWORD* ppdwTrustEnumerationWaitEnabled
   );

DWORD
LsaPstoreDeleteTrustEnumerationWaitInfo(
    IN OPTIONAL PCWSTR pwszDnsDomainName
    );  

VOID
LsaPstoreInitializeLibrary(
    VOID
    );
///<
/// Initialize password store library.
///
/// Initialize the library.
///
/// Can be called multiple times, but each successful call requires a
/// corresponding call to LsaPstoreCleanup().
///
/// @return N/A
///
/// @note The first call must be in a race-free context.
///

VOID
LsaPstoreCleanupLibrary(
    VOID
    );
///<
/// Cleanup password store library.
///
/// This must be called once for each successful call to
/// #LsaPstoreInitializeLibrary().
///
/// @return N/A
///
/// @note The last call must be in a race-free context.
///

DWORD
LsaPstoreGetPasswordInfoA(
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* PasswordInfo
    );

DWORD
LsaPstoreGetPasswordInfoW(
    IN OPTIONAL PCWSTR DnsDomainName,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* PasswordInfo
    );
///<
/// Get password information.
///
/// @param[in] DnsDomainName - If specified, lookup information for
///     specified domain.  Otherwise, lookup information for the default
///     domain.
///
/// @param[out] PasswordInfo - Returns password information.  Free with
///     LsaPstoreFreePaswordInfoW().
///
/// @return Windows error code
/// @retval ERROR_SUCCESS on success
/// @retval NERR_SetupNotJoined if not joined
/// @retval !ERROR_SUCCESS on failure
///

DWORD
LsaPstoreSetPasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A PasswordInfo
    );

DWORD
LsaPstoreSetPasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W PasswordInfo
    );
///<
/// Set password information.
///
/// Saves the specified password information.  If there is no default domain,
/// this becomes the default domain.  If there is already a default domain,
/// this does not modify the default domain.
///
/// @param[in] PasswordInfo - Password info to set.
///
/// @return Windows error code
/// @retval ERROR_SUCCESS on success
/// @retval !ERROR_SUCCESS on failure
///

DWORD
LsaPstoreDeletePasswordInfoA(
    IN OPTIONAL PCSTR DnsDomainName
    );

DWORD
LsaPstoreDeletePasswordInfoW(
    IN OPTIONAL PCWSTR DnsDomainName
    );
///<
/// Delete password information.
///
/// Delete password information.  If the domain is the default domain, this
/// removes the default domain setting as well.
///
/// @param[in] DnsDomainName - If specified, delete information for the
///     specified domain.  Otherwise, delete information for default domain.
///
/// @return Windows error code
/// @retval ERROR_SUCCESS on success (including if not joined)
/// @retval !ERROR_SUCCESS on failure
///

DWORD
LsaPstoreGetDefaultDomainA(
    OUT PSTR* DnsDomainName
    );

DWORD
LsaPstoreGetDefaultDomainW(
    OUT PWSTR* DnsDomainName
    );
///<
/// Get default joined domain name.
///
/// @param[out] DnsDomainName - Returns the default domain join DNS domain
///     name, or NULL if none.  Free with LsaPstoreFreeMemory().
///
/// @return Windows error code
/// @retval ERROR_SUCCESS on success
/// @retval !ERROR_SUCCESS on other failure
///

DWORD
LsaPstoreSetDefaultDomainA(
    IN OPTIONAL PCSTR DnsDomainName
    );

DWORD
LsaPstoreSetDefaultDomainW(
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
LsaPstoreGetJoinedDomainsA(
    OUT PSTR** DnsDomainNames,
    OUT PDWORD Count
    );

DWORD
LsaPstoreGetJoinedDomainsW(
    OUT PWSTR** DnsDomainNames,
    OUT PDWORD Count
    );
///<
/// Get list of joined domains.
///
/// @param[out] DnsDomainNames - Returns DNS domain names for which there
///     is join information.  Returns NULL if not joined to any domains.
///     Free with LsaPstoreFreeStringArrayW().
///
/// @param[out] Count - Returns count of domains for which there is password
///     info.  Returns 0 if not joined to any domains.
///
/// @return Windows error code
/// @retval ERROR_SUCCESS on success
/// @retval !ERROR_SUCCESS on other failure
///

VOID
LsaPstoreFreeMemory(
    IN PVOID Pointer
    );
///<
/// Free memory buffer.
///
/// @param[in] Pointer - Buffer to free.
///
/// @return N/A
///

#define LSA_PSTORE_FREE(pPointer) \
    LW_RTL_MAKE_CUSTOM_FREE(LsaPstoreFreeMemory, pPointer)

VOID
LsaPstoreFreeStringArrayA(
    IN PSTR* StringArray,
    IN DWORD Count
    );

VOID
LsaPstoreFreeStringArrayW(
    IN PWSTR* StringArray,
    IN DWORD Count
    );
///<
/// Free array of strings.
///
/// @param[in] StringArray - Array of strings.
/// @param[in] Count - Items in the string array.
///
/// @return N/A
///

#define LSA_PSTORE_FREE_STRING_ARRAY_A(pStringArray, pCount) \
    do { \
        if (*(pStringArray)) \
        { \
            LsaPstoreFreeStringArrayA(*(pStringArray), *(pCount)); \
            *(pStringArray) = NULL; \
        } \
        *(pCount) = 0; \
    } while (0)

#define LSA_PSTORE_FREE_STRING_ARRAY_W(pStringArray, pCount) \
    do { \
        if (*(pStringArray)) \
        { \
            LsaPstoreFreeStringArrayW(*(pStringArray), *(pCount)); \
            *(pStringArray) = NULL; \
        } \
        *(pCount) = 0; \
    } while (0)

VOID
LsaPstoreFreePasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A PasswordInfo
    );

VOID
LsaPstoreFreePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W PasswordInfo
    );
///<
/// Free password information.
///
/// @param[in] PasswordInfo - Password info to free.
///
/// @return N/A
///

#define LSA_PSTORE_FREE_PASSWORD_INFO_A(ppPasswordInfo) \
    LW_RTL_MAKE_CUSTOM_FREE(LsaPstoreFreePasswordInfoA, ppPasswordInfo)

#define LSA_PSTORE_FREE_PASSWORD_INFO_W(ppPasswordInfo) \
    LW_RTL_MAKE_CUSTOM_FREE(LsaPstoreFreePasswordInfoW, ppPasswordInfo)

#endif /* __LSA_PSTORE_API_H__ */
