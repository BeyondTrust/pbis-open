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
 *        keytab.h
 *
 * Abstract:
 *
 *        Kerberos 5 keytab functions
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 * 
 */

#ifndef _KEYTAB_H_
#define _KEYTAB_H_

#ifndef _WIN32

#include <lw/types.h>

#endif /* _WIN32 */

#define KT_STATUS_SUCCESS                    (0x00000000)

#define KT_STATUS(code)                      (0x0050 | (code))

/* General errors */
#define KT_STATUS_OUT_OF_MEMORY              KT_STATUS(0x1001)
#define KT_STATUS_INVALID_PARAMETER          KT_STATUS(0x1002)

/* Kerberos errors */
#define KT_STATUS_KRB5_ERROR                 KT_STATUS(0x2000)
#define KT_STATUS_KRB5_CLOCK_SKEW            KT_STATUS(0x2001)
#define KT_STATUS_KRB5_NO_KEYS_FOUND         KT_STATUS(0x2002)
#define KT_STATUS_KRB5_NO_DEFAULT_REALM      KT_STATUS(0x2003)
#define KT_STATUS_KRB5_PASSWORD_EXPIRED      KT_STATUS(0x2004)
#define KT_STATUS_KRB5_PASSWORD_MISMATCH     KT_STATUS(0x2005)
#define KT_STATUS_GSS_CALL_FAILED            KT_STATUS(0x2006)

/* LDAP errors */
#define KT_STATUS_LDAP_ERROR                 KT_STATUS(0x3000)
#define KT_STATUS_LDAP_NO_KVNO_FOUND         KT_STATUS(0x3001)


DWORD
KtKrb5AddKey(
    PCSTR pszPrincipal,
    PVOID pKey,
    DWORD dwKeyLen,
    PCSTR pszSalt,
    PCSTR pszKtPath,
    PCSTR pszDcName,
    DWORD dwKeyVersion
    );


DWORD
KtKrb5AddKeyW(
    PCWSTR pwszPrincipal,
    PVOID pKey,
    DWORD dwKeyLen,
    PCWSTR pwszSalt,
    PCWSTR pwszKtPath,
    PCWSTR pwszDcName,
    DWORD dwKeyVersion
    );


DWORD
KtKrb5GetKey(
    PCSTR pszPrincipal,
    PCSTR pszKtPath,
    DWORD dwEncType,
    PVOID *pKey,
    DWORD *dwKeyLen
    );


DWORD
KtKrb5RemoveKey(
    PSTR pszPrincipal,
    DWORD dwVer,
    PSTR pszKtPath
    );


DWORD
KtKrb5FormatPrincipal(
    PCSTR pszAccount,
    PCSTR pszRealm,
    PSTR *ppszPrincipal
    );


DWORD
KtKrb5FormatPrincipalW(
    PCWSTR pwszAccount,
    PCWSTR pwszRealm,
    PWSTR *ppwszPrincipal
    );

DWORD
KtLdapGetBaseDnA(
    PCSTR  pszDcName,
    PSTR  *pszBaseDn
    );


DWORD
KtLdapGetBaseDnW(
    PCWSTR  pwszDcName,
    PWSTR  *ppwszBaseDn
    );


DWORD
KtLdapGetKeyVersionA(
    PCSTR   pszDcName,
    PCSTR   pszBaseDn,
    PCSTR   pszPrincipal,
    PDWORD  pdwKvno
    );


DWORD
KtLdapGetKeyVersionW(
    PCWSTR   pwszDcName,
    PCWSTR   pwszBaseDn,
    PCWSTR   pwszPrincipal,
    PDWORD   pdwKvno
    );


DWORD
KtLdapGetSaltingPrincipalA(
    PCSTR   pszDcName,
    PCSTR   pszBaseDn,
    PCSTR   pszMachAcctName,
    PSTR   *pszSalt
    );


DWORD
KtLdapGetSaltingPrincipalW(
    PCWSTR  pwszDcName,
    PCWSTR  pwszBaseDn,
    PCWSTR  pwszMachAcctName,
    PWSTR  *ppwszSalt
    );


DWORD
KtGetSaltingPrincipalA(
    PCSTR   pszMachineName,
    PCSTR   pszMachAcctName,
    PCSTR   pszDnsDomainName,
    PCSTR   pszRealmName,
    PCSTR   pszDcName,
    PCSTR   pszBaseDn,
    PSTR   *pszSalt
    );


DWORD
KtGetSaltingPrincipalW(
    PCWSTR   pwszMachineName,
    PCWSTR   pwszMachAcctName,
    PCWSTR   pwszDnsDomainName,
    PCWSTR   pwszRealmName,
    PCWSTR   pwszDcName,
    PCWSTR   pwszBaseDn,
    PWSTR   *pwszSalt
    );


#endif /* _KEYTAB_H_ */
