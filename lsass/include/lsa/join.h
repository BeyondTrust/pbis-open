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

#define LSA_NET_JOIN_DOMAIN_NOTIMESYNC 1
#define LSA_NET_JOIN_DOMAIN_MULTIPLE   2

DWORD
LsaNetJoinInitialize(
    );

VOID
LsaNetJoinShutdown(
    );

DWORD
LsaNetTestJoinDomain(
    IN OPTIONAL PCSTR pszDomainName,
    OUT PBOOLEAN pbIsJoined
    );

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
    DWORD dwFlags
    );

DWORD
LsaLeaveDomain(
    PCSTR pszUsername,
    PCSTR pszPassword
    );

DWORD
LsaLeaveDomain2(
    PCSTR pszUsername,
    PCSTR pszPassword,
    PCSTR pszDomain,
    DWORD dwFlags
    );

DWORD
LsaNetGetShortDomainName(
    PCSTR pszDomainFQDN,
    PSTR* ppszShortDomainName
    );

DWORD
LsaNetGetRwDCName(
    PCSTR pszDomainName,
    PSTR* ppszDCName
    );

DWORD
LsaGetDnsDomainName(
    PSTR* ppszDnsDomainName
    );

DWORD
LsaGetComputerDN(
    PSTR* ppszComputerDN
    );

VOID
LsaEnableDebugLog(
    VOID
    );

VOID
LsaDisableDebugLog(
    VOID
    );

VOID
LsaNetFreeString(
    PSTR pszString
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
LsaChangeDomainGroupMembership(
    IN  PCSTR    pszDomainName,
    IN  PCSTR    pszDomainSID,
    IN  BOOLEAN  bEnable
    );


DWORD
LsaMachineChangePassword(
    IN OPTIONAL PCSTR    pszDomainName
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
