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
 *        pwdcache_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Machine Password Sync API (Private Header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#ifndef __PWDCACHE_P_H__
#define __PWDCACHE_P_H__

// Craete/Destroy Functions

DWORD
LsaPcacheCreate(
    IN PCSTR pszDomainName,
    OUT PLSA_MACHINEPWD_CACHE_HANDLE ppPcache
    );

VOID
LsaPcacheDestroy(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache
    );

// Get Functions - Call corresponding release

DWORD
LsaPcacheGetMachineAccountInfoA(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A* ppAccountInfo
    );

DWORD
LsaPcacheGetMachineAccountInfoW(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache,
    OUT PLSA_MACHINE_ACCOUNT_INFO_W* ppAccountInfo
    );

DWORD
LsaPcacheGetMachinePasswordInfoA(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    );

DWORD
LsaPcacheGetMachinePasswordInfoW(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* ppPasswordInfo
    );

// Release Functions

VOID
LsaPcacheReleaseMachineAccountInfoA(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    );

VOID
LsaPcacheReleaseMachineAccountInfoW(
    IN PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo
    );

VOID
LsaPcacheReleaseMachinePasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    );

VOID
LsaPcacheReleaseMachinePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    );

// Reset Function

VOID
LsaPcacheClearPasswordInfo(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache
    );

#endif /* __PWDCACHE_P_H__ */
