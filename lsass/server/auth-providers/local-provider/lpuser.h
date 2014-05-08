/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lpuser.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
