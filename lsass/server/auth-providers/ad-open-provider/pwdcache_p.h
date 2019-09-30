/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        pwdcache_p.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
