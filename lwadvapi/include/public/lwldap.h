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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwldap.h
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi)
 *
 *        LDAP API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LWLDAP_H__
#define __LWLDAP_H__

#ifndef LDAP_DEPRECATED
#define LDAP_DEPRECATED 1
#include <ldap.h>
#endif

//maximum length of LDAP query, in bytes.
#define MAX_LDAP_QUERY_LENGTH 4096

#define LW_LDAP_OPT_GLOBAL_CATALOG 0x00000001
#define LW_LDAP_OPT_SIGN_AND_SEAL  0x00000002
#define LW_LDAP_OPT_ANNONYMOUS     0x00000004

typedef void (*PFNLW_COOKIE_FREE)(PVOID);

typedef struct __LW_SEARCH_COOKIE
{
    BOOLEAN bSearchFinished;
    PVOID pvData;
    PFNLW_COOKIE_FREE pfnFree;
} LW_SEARCH_COOKIE, *PLW_SEARCH_COOKIE;


LW_BEGIN_EXTERN_C

DWORD
LwCLdapOpenDirectory(
    IN PCSTR pszServerName,
    OUT PHANDLE phDirectory
    );

DWORD
LwLdapPingTcp(
    PCSTR pszHostAddress,
    DWORD dwTimeoutSeconds
    );

DWORD
LwLdapOpenDirectoryServer(
    IN PCSTR pszServerAddress,
    IN PCSTR pszServerName,
    IN DWORD dwFlags,
    OUT PHANDLE phDirectory
    );
///<
/// Open a connection to an LDAP server.
///
/// @param[in] pszServerAddress - Server's address.
/// @param[in] pszServerName - Server's name (used for authentication).
/// @param[in] dwFlags - LW_LDAP_OPT_* flags.
/// @param[out] phDirectory - Returns handle to LDAP connection.
///
/// @return Windows error code
/// @retval ERROR_SUCCESS on success
/// @retval SEC_E_NO_CREDENTIALS if no usable credentials
/// @retval !ERROR_SUCCESS on failure
///

DWORD
LwLdapConvertDomainToDN(
    PCSTR pszDomainName,
    PSTR * ppszDomainDN
    );

DWORD
LwLdapConvertDNToDomain(
    PCSTR pszDN,
    PSTR* ppszDomainName
    );

void
LwLdapCloseDirectory(
    HANDLE hDirectory
    );

DWORD
LwLdapGetParentDN(
    PCSTR pszObjectDN,
    PSTR* ppszParentDN
    );

DWORD
LwLdapDirectorySearch(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    int    scope,
    PCSTR  pszQuery,
    PSTR * ppszAttributeList,
    LDAPMessage **ppMessage
    );

DWORD
LwLdapDirectorySearchEx(
    HANDLE hDirectory,
    PCSTR pszObjectDN,
    int scope,
    PCSTR pszQuery,
    PSTR* ppszAttributeList,
    LDAPControl** ppServerControls,
    DWORD dwNumMaxEntries,
    LDAPMessage** ppMessage
    );

DWORD
LwLdapEnablePageControlOption(
    HANDLE hDirectory
    );

DWORD
LwLdapDisablePageControlOption(
    HANDLE hDirectory
    );

DWORD
LwLdapDirectoryOnePagedSearch(
    HANDLE         hDirectory,
    PCSTR          pszObjectDN,
    PCSTR          pszQuery,
    PSTR*          ppszAttributeList,
    DWORD          dwPageSize,
    PLW_SEARCH_COOKIE pCookie,
    int            scope,
    LDAPMessage**  ppMessage
    );

DWORD
LwLdapCountEntries(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PDWORD pdwCount
    );

DWORD
LwLdapModify(
    HANDLE hDirectory,
    PCSTR pszDN,
    LDAPMod** ppMods
    );

LDAPMessage*
LwLdapFirstEntry(
    HANDLE hDirectory,
    LDAPMessage *pMessage
    );

LDAPMessage*
LwLdapNextEntry(
    HANDLE hDirectory,
    LDAPMessage* pMessage
    );

LDAP *
LwLdapGetSession(
    HANDLE hDirectory
    );

DWORD
LwLdapGetBytes(
        HANDLE hDirectory,
        LDAPMessage* pMessage,
        PSTR pszFieldName,
        PBYTE* ppszByteValue,
        PDWORD pszByteLen
        );

DWORD
LwLdapGetString(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PCSTR pszFieldName,
    PSTR* ppszValue
    );

DWORD
LwLdapGetGUID(
    IN HANDLE       hDirectory,
    IN LDAPMessage* pMessage,
    IN PSTR         pszFieldName,
    OUT PSTR*       ppszGUID
    );

DWORD
LwLdapGetDN(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR* ppszValue
    );

DWORD
LwLdapIsValidADEntry(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PBOOLEAN pbValidADEntry
    );

DWORD
LwLdapGetUInt32(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PCSTR pszFieldName,
    PDWORD pdwValue
    );

DWORD
LwLdapGetUInt64(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    IN PCSTR pszFieldName,
    OUT UINT64* pqwValue
    );

DWORD
LwLdapGetInt64(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    IN PCSTR pszFieldName,
    OUT int64_t * pqwValue
    );

DWORD
LwLdapGetStrings(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    IN PCSTR pszFieldName,
    OUT PSTR** pppszValues,
    OUT PDWORD pdwNumValues
    );

DWORD
LwLdapGetStringsWithExtDnResult(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    IN PCSTR pszFieldName,
    IN BOOLEAN bDoSidParsing,
    OUT PSTR** pppszValues,
    OUT PDWORD pdwNumValues
    );

DWORD
LwLdapPutString(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PCSTR  pszFieldName,
    PSTR   pszValue
    );

DWORD
LwLdapPutUInt32(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PCSTR  pszFieldName,
    DWORD  dwValue
    );

DWORD
LwLdapEscapeString(
    PSTR *ppszResult,
    PCSTR pszInput
    );

VOID
LwLdapFreeCookie(
    PVOID pCookie
    );

VOID
LwFreeCookieContents(
    IN OUT PLW_SEARCH_COOKIE pCookie
    );

VOID
LwInitCookie(
    OUT PLW_SEARCH_COOKIE pCookie
    );

DWORD
LwLdapParseExtendedDNResult(
    IN PCSTR pszResult,
    OUT PSTR* ppszSid);

DWORD
LwLdapDirectoryExtendedDNSearch(
    IN HANDLE hDirectory,
    IN PCSTR pszObjectDN,
    IN PCSTR pszQuery,
    IN PSTR* ppszAttributeList,
    IN int scope,
    OUT LDAPMessage** ppMessage
    );

DWORD
LwLdapBindDirectoryAnonymous(
    HANDLE hDirectory
    );

DWORD
LwLdapBindDirectorySasl(
    LDAP *ld,
    PCSTR pszServerName,
    BOOLEAN bSeal
    );

LW_END_EXTERN_C


#endif /* __LWLDAP_H__ */
