/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        lsaadclient.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Client API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __LSACLIENT_AD_H__
#define __LSACLIENT_AD_H__

#include <lsa/lsa2.h>
#include <sys/types.h>

DWORD
LsaAdEmptyCache(
    IN HANDLE hLsaConnection
    );

DWORD
LsaAdRemoveUserByNameFromCache(
    IN HANDLE hLsaConnection,
    IN PCSTR  pszName
    );

DWORD
LsaAdRemoveUserByIdFromCache(
    IN HANDLE hLsaConnection,
    IN uid_t  uid
    );

DWORD
LsaAdEnumUsersFromCache(
    IN HANDLE   hLsaConnection,
    IN PSTR*    ppszResume,
    IN DWORD    dwMaxNumUsers,
    OUT PDWORD  pdwUsersFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
LsaAdRemoveGroupByNameFromCache(
    IN HANDLE hLsaConnection,
    IN PCSTR  pszGroupName
    );

DWORD
LsaAdRemoveGroupByIdFromCache(
    IN HANDLE hLsaConnection,
    IN gid_t  gid
    );

DWORD
LsaAdEnumGroupsFromCache(
    IN HANDLE   hLsaConnection,
    IN PSTR*    ppszResume,
    IN DWORD    dwMaxNumGroups,
    OUT PDWORD  pdwGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
LsaAdJoinDomain(
    HANDLE hLsaConnection,
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
LsaAdLeaveDomain(
    HANDLE hLsaConnection,
    PCSTR pszUsername,
    PCSTR pszPassword
    );

#endif /* __LSACLIENT_AD_H__ */
