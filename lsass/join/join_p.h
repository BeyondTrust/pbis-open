/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        join.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Join to Active Directory
 *
 *        Private Header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __JOIN_P_H__
#define __JOIN_P_H__

#include <lwio/lwio.h>

typedef struct __LSA_MACHINE_ACCT_INFO
{
    PSTR   pszDomainName;
    PSTR   pszDnsDomainName;
    PSTR   pszSID;
    PSTR   pszHostname;
    PSTR   pszMachineAccount;
    PSTR   pszMachinePassword;
    time_t last_change_time;
    DWORD  dwSchannelType;

} LSA_MACHINE_ACCT_INFO, *PLSA_MACHINE_ACCT_INFO;

size_t
LsaNetGetErrorString(
    DWORD  dwErrorCode,
    PSTR   pszBuffer,
    size_t bufSize
    );

DWORD
LsaBuildOrgUnitDN(
    PCSTR pszDomain,
    PCSTR pszOU,
    PSTR* ppszOU_DN
    );

DWORD
LsaSrvJoinFindComputerDN(
    HANDLE hDirectory,
    PCSTR  pszHostName,
    PCSTR  pszDomain,
    PSTR*  ppszComputerDN
    );

DWORD
LsaSyncTimeToDC(
    PCSTR  pszDomain
    );

DWORD
LsaBuildMachineAccountInfo(
    PLWPS_PASSWORD_INFO pInfo,
    PLSA_MACHINE_ACCT_INFO* ppAcctInfo
    );

VOID
LsaFreeMachineAccountInfo(
    PLSA_MACHINE_ACCT_INFO pAcctInfo
    );

#endif /* __JOIN_P_H__ */
