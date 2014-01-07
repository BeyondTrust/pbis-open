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
 *        ad.h
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

/**
 * @file lsa/ad.h
 * @brief LSASS AD Provider Public Client API
 */

#include <lsa/lsa.h>
#include <sys/types.h>
#include <lsa/ad-types.h>
#include <lsa/lsapstore-types.h>

/**
 * @defgroup ad AD Provider client API
 * @brief AD Provider client API
 *
 * This module provides functions to communicate directory with the lsass
 * Active Directory provider.
 */

/*@{*/

DWORD
LsaAdEmptyCache(
    IN HANDLE hLsaConnection,
    IN OPTIONAL PCSTR pszDomainName
    );

DWORD
LsaAdRemoveUserByNameFromCache(
    IN HANDLE hLsaConnection,
    IN OPTIONAL PCSTR pszDomainName,
    IN PCSTR  pszName
    );

DWORD
LsaAdRemoveUserByIdFromCache(
    IN HANDLE hLsaConnection,
    IN OPTIONAL PCSTR pszDomainName,
    IN uid_t  uid
    );

DWORD
LsaAdEnumUsersFromCache(
    IN HANDLE   hLsaConnection,
    IN OPTIONAL PCSTR pszDomainName,
    IN PSTR*    ppszResume,
    IN DWORD    dwMaxNumUsers,
    OUT PDWORD  pdwUsersFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
LsaAdRemoveGroupByNameFromCache(
    IN HANDLE hLsaConnection,
    IN OPTIONAL PCSTR pszDomainName,
    IN PCSTR  pszGroupName
    );

DWORD
LsaAdRemoveGroupByIdFromCache(
    IN HANDLE hLsaConnection,
    IN OPTIONAL PCSTR pszDomainName,
    IN gid_t  gid
    );

DWORD
LsaAdEnumGroupsFromCache(
    IN HANDLE   hLsaConnection,
    IN OPTIONAL PCSTR pszDomainName,
    IN PSTR*    ppszResume,
    IN DWORD    dwMaxNumGroups,
    OUT PDWORD  pdwGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

/**
 * @brief Join an Active Directory domain
 *
 * Instructs the AD provider to join the computer to
 * an Active Directory domain.  If already joined,
 * and #LW_NET_JOIN_DOMAIN_MULTIPLE is not specified,
 * the AD provider will first leave the default domain.
 *
 * @param[in] hLsaConnection a connection handle
 * @param[in] pszHostname the computer name to join with
 * @param[in] pszHostDnsDomain the DNS domain name of the computer
 * @param[in] pszDomain the fully-qualified domain name to join
 * @param[in] pszOU an optional OU (organizational unit) to join, specified
 * as forward-slash separated components
 * @param[in] pszUsername the name of an AD user with permission with
 * permission to join computers to the target domain
 * @param[in] pszPassword the password for the user
 * @param[in] pszOSName the operating system name to set on
 * the computer object
 * @param[in] pszOSVersion the operating system version to set on
 * the computer object
 * @param[in] pszOSServicePack the service pack level to set on
 * the computer object
 * @param[in] dwFlags additional flags to control join behavior
 * @retval LW_ERROR_SUCCESS success
 */
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
    LSA_NET_JOIN_FLAGS dwFlags
    );

DWORD
LsaAdOuSlashToDn(
    IN PCSTR pDomain,
    IN PCSTR pSlashOu,
    OUT PSTR* ppLdapOu
    );

/**
 * @brief Join Active Directory domain with alternate OU syntax
 *
 * Identical to #LsaAdJoinDomain(), but accepts a raw LDAP DN
 * (distinguished name) for the OU to join.
 *
 * @param[in] hLsaConnection a connection handle
 * @param[in] pHostname the computer name to join with
 * @param[in] pHostDnsDomain the DNS domain name of the computer
 * @param[in] pDomain the fully-qualified domain name to join
 * @param[in] pOu an optional OU (organizational unit) to join
 * specified as a DN.
 * @param[in] pUsername the name of an AD user with permission with
 * permission to join computers to the target domain
 * @param[in] pPassword the password for the user
 * @param[in] pOsName the operating system name to set on
 * the computer object
 * @param[in] pOsVersion the operating system version to set on
 * the computer object
 * @param[in] pOsServicePack the service pack level to set on
 * the computer object
 * @param[in] dwFlags additional flags to control join behavior
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LsaAdJoinDomainDn(
    IN HANDLE hLsaConnection,
    IN PCSTR pHostname,
    IN PCSTR pHostDnsDomain,
    IN PCSTR pDomain,
    IN PCSTR pOu,
    IN PCSTR pUsername,
    IN PCSTR pPassword,
    IN PCSTR pOsName,
    IN PCSTR pOsVersion,
    IN PCSTR pOsServicePack,
    IN LSA_NET_JOIN_FLAGS dwFlags
    );

/**
 * @brief Join Active Directory domain with userAccountControl flags
 *
 * Identical to #LsaAdJoinDomain(), but allows passing User-Account-Control
 * flag values.
 *
 * @param[in] hLsaConnection a connection handle
 * @param[in] pHostname the computer name to join with
 * @param[in] pHostDnsDomain the DNS domain name of the computer
 * @param[in] pDomain the fully-qualified domain name to join
 * @param[in] pOu an optional OU (organizational unit) to join
 * specified as a DN.
 * @param[in] pUsername the name of an AD user with permission with
 * permission to join computers to the target domain
 * @param[in] pPassword the password for the user
 * @param[in] pOsName the operating system name to set on
 * the computer object
 * @param[in] pOsVersion the operating system version to set on
 * the computer object
 * @param[in] pOsServicePack the service pack level to set on
 * the computer object
 * @param[in] dwFlags additional flags to control join behavior
 * @param[in] dwUac additional user account control flags
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LsaAdJoinDomainUac(
    HANDLE hLsaConnection,
    PCSTR pszHostname,
    PCSTR pszHostDnsDomain,
    PCSTR pszDomain,
    PCSTR pszOu,
    PCSTR pszUsername,
    PCSTR pszPassword,
    PCSTR pszOSName,
    PCSTR pszOSVersion,
    PCSTR pszOSServicePack,
    LSA_NET_JOIN_FLAGS dwFlags,
    LSA_USER_ACCOUNT_CONTROL_FLAGS dwUac
    );


/**
 * @brief Leave default Active Directory domain
 *
 * Leaves the currently-joined default AD domain.
 *
 * @param[in] hLsaConnection a connection handle
 * @param[in] pszUsername an optional name of a user with permissions to
 * disable the machine account in AD
 * @param[in] pszPassword an optional password for the provided user
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LsaAdLeaveDomain(
    HANDLE hLsaConnection,
    PCSTR pszUsername,
    PCSTR pszPassword
    );

/**
 * @brief Leave Active Directory domain
 *
 * Leaves a currently-joined AD domain.  This function supports
 * leaving a specific domain when multiple domains are joined
 * and additional flags to control leave behavior.
 *
 * @param[in] hLsaConnection a connection handle
 * @param[in] pszUsername an optional name of a user with permissions to
 * disable the machine account in AD
 * @param[in] pszPassword an optional password for the provided user
 * @param[in] pszDomain the domain to leave
 * @param[in] dwFlags additional leave flags
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LsaAdLeaveDomain2(
    HANDLE hLsaConnection,
    PCSTR pszUsername,
    PCSTR pszPassword,
    PCSTR pszDomain,
    LSA_NET_JOIN_FLAGS dwFlags
    );

/**
 * @brief Set default Active Directory domain
 *
 * Sets the default AD domain.
 *
 * @param[in] hLsaConnection a connection handle
 * @param[in] pszDomain the domain
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LsaAdSetDefaultDomain(
    IN HANDLE hLsaConnection,
    IN PCSTR pszDomain
    );

/**
 * @brief Get joined domain list
 *
 * Gets a list of joined domains.  Free the result with
 * LwFreeStringArray().
 *
 * @param[in] hLsaConnection a connection handle
 * @param[out] pdwNumDomainsFound set to the number of joined domains
 * @param[out] pppszJoinedDomains set to the list of joined domains
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LsaAdGetJoinedDomains(
    IN HANDLE hLsaConnection,
    OUT PDWORD pdwNumDomainsFound,
    OUT PSTR** pppszJoinedDomains
    );

LW_DWORD
LsaAdGetMachineAccountInfo(
    LW_IN LW_HANDLE hLsaConnection,
    LW_IN LW_OPTIONAL LW_PCSTR pszDnsDomainName,
    LW_OUT PLSA_MACHINE_ACCOUNT_INFO_A* ppAccountInfo
    );

LW_VOID
LsaAdFreeMachineAccountInfo(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    );

LW_DWORD
LsaAdGetMachinePasswordInfo(
    LW_IN LW_HANDLE hLsaConnection,
    LW_IN LW_PCSTR pszDnsDomainName,
    LW_OUT PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    );

LW_VOID
LsaAdFreeMachinePasswordInfo(
    LW_IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    );

LW_DWORD
LsaAdGetComputerDn(
    LW_IN LW_HANDLE hLsaConnection,
    LW_IN LW_OPTIONAL LW_PCSTR pszDnsDomainName,
    LW_OUT LW_PSTR* ppszComputerDn
    );

/*@}*/

#endif /* __LSACLIENT_AD_H__ */
