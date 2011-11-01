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
 *        join.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Public Join API
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */
#ifndef __LSA_JOIN_H__
#define __LSA_JOIN_H__

#include <lsa/ad-types.h>

DWORD
LsaJoinDomain(
    PCSTR pszHostname,
    PCSTR pszHostDnsDomain,
    PCSTR pszDomain,
    PCSTR pszOU,
    PCSTR pszUsername,
    PCSTR pszPassword,
    PCSTR pszOSName,
    PCSTR pszOSVersion,
    PCSTR pszOSServicePack,
    LSA_NET_JOIN_FLAGS dwFlags
    );


DWORD
LsaJoinDomainUac(
    PCSTR pszHostname,
    PCSTR pszHostDnsDomain,
    PCSTR pszDomain,
    PCSTR pszOU,
    PCSTR pszUsername,
    PCSTR pszPassword,
    PCSTR pszOSName,
    PCSTR pszOSVersion,
    PCSTR pszOSServicePack,
    LSA_NET_JOIN_FLAGS dwFlags,
    LSA_USER_ACCOUNT_CONTROL_FLAGS dwUac
    );

DWORD
LsaLeaveDomain2(
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszUsername,
    IN OPTIONAL PCSTR pszPassword,
    IN LSA_NET_JOIN_FLAGS dwFlags
    );

DWORD
LsaEnableDomainGroupMembership(
    PCSTR pszDomainName,
    PCSTR pszDomainSID
    );

DWORD
LsaDisableDomainGroupMembership(
    PCSTR pszDomainName,
    PCSTR pszDomainSID
    );

DWORD
LsaMachineChangePassword(
    IN OPTIONAL PCSTR pszDnsDomainName
    );

DWORD
LsaUserChangePassword(
    PWSTR  pwszDCName,
    PWSTR  pwszUserName,
    PWSTR  pwszOldPassword,
    PWSTR  pwszNewPassword
    );

#endif /* __LSA_JOIN_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
