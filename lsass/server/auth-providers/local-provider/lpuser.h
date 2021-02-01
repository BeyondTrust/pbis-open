/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lpuser.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        User Management Routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LP_USER_H__
#define __LP_USER_H__

DWORD
LocalDirGetUserInfoFlags(
    HANDLE hProvider,
    uid_t  uid,
    PDWORD pdwUserInfoFlags
    );

DWORD
LocalDirAddUser(
    HANDLE           hProvider,
    PLSA_USER_ADD_INFO pUserInfo
    );

DWORD
LocalDirModifyUser(
    HANDLE             hProvider,
    PLSA_USER_MOD_INFO_2 pUserModInfo
    );

DWORD
LocalDirDeleteUser(
    HANDLE hProvider,
    PWSTR  pwszUserDN
    );

DWORD
LocalDirChangePassword(
    HANDLE hProvider,
    PWSTR  pwszUserDN,
    PWSTR  pwszOldPassword,
    PWSTR  pwszNewPassword
    );

DWORD
LocalDirSetPassword(
    HANDLE hProvider,
    PWSTR  pwszUserDN,
    PWSTR  pwszNewPassword
    );

DWORD
LocalCreateHomeDirectory(
    PLSA_SECURITY_OBJECT pObject
    );

DWORD
LocalProvisionHomeDir(
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszHomedirPath
    );

DWORD
LocalCheckIsGuest(
    PLSA_SECURITY_OBJECT pObject,
    PBOOLEAN pbUserIsGuest
    );

DWORD
LocalCheckAccountFlags(
    PLSA_SECURITY_OBJECT pObject
    );

DWORD
LocalCheckPasswordPolicy(
    PLSA_SECURITY_OBJECT pObject,
    PCSTR                pszPassword
    );

DWORD
LocalGetUserLogonInfo(
    HANDLE   hProvider,
    PSTR     pszUserDn,
    PDWORD   pdwLogonCount,
    PDWORD   pdwBadPasswordCount
    );

DWORD
LocalSetUserLogonInfo(
    HANDLE  hProvider,
    PSTR    pszUserDn,
    PDWORD  pdwLogonCount,
    PDWORD  pdwBadPasswordCount,
    PLONG64 pllLastLogonTime,
    PLONG64 pllLastLofffTime
    );

DWORD
LocalUpdateUserLoginTime(
    HANDLE hProvider,
    PWSTR  pwszUserDN
    );

DWORD
LocalUpdateUserLogoffTime(
    HANDLE hProvider,
    PWSTR  pwszUserDN
    );

DWORD
LocalDirCheckIfAdministrator(
    HANDLE   hProvider,
    uid_t    uid,
    PBOOLEAN pbIsAdmin
    );

#endif /* __LP_USER_H__ */
