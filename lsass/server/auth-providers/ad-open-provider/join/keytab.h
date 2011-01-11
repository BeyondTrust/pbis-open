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

#include <lw/types.h>

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
KtKrb5FormatPrincipalW(
    PCWSTR pwszAccount,
    PCWSTR pwszRealm,
    PWSTR *ppwszPrincipal
    );

DWORD
KtKrb5GetSaltingPrincipalA(
    PCSTR   pszMachineName,
    PCSTR   pszMachAcctName,
    PCSTR   pszDnsDomainName,
    PCSTR   pszRealmName,
    PCSTR   pszDcName,
    PCSTR   pszBaseDn,
    PSTR   *pszSalt
    );

DWORD
KtKrb5GetSaltingPrincipalW(
    PCWSTR   pwszMachineName,
    PCWSTR   pwszMachAcctName,
    PCWSTR   pwszDnsDomainName,
    PCWSTR   pwszRealmName,
    PCWSTR   pwszDcName,
    PCWSTR   pwszBaseDn,
    PWSTR   *pwszSalt
    );

#endif /* _KEYTAB_H_ */
