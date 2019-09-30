/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        keytab.h
 *
 * Abstract:
 *
 *        Kerberos 5 keytab functions
 *
 * Authors: pbis (pbis@beyondtrust.com)
 * 
 */

#ifndef _BT_KEYTAB_H_
#define _BT_KEYTAB_H_

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
KtKrb5GetUserSaltingPrincipalA(
    PCSTR pszUserName,
    PCSTR pszRealmName,
    PCSTR pszDcName,
    PCSTR pszBaseDn,
    PSTR *pszSalt);

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
