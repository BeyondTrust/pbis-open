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
 *        nss-user.h
 *
 * Abstract:
 * 
 *        Name Server Switch (BeyondTrust LSASS)
 * 
 *        Handle NSS User Information
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 */

#ifndef __LSA_NSS_AIX_USER_H__
#define __LSA_NSS_AIX_USER_H__

#ifndef S_PGID
#define S_PGID "pgid"
#endif

#define LSA_NSS_NOPASSWORD "*"

DWORD
LsaNssFindUserByAixName(
    HANDLE hLsaConnection,
    PCSTR  pszName,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    );

void
LsaNssFreeLastUser(
        VOID
        );

DWORD LsaNssAllocateUserFromInfo0(
        PLSA_USER_INFO_0 pInfo,
        struct passwd** ppResult
        );

struct passwd *LsaNssGetPwUid(gid_t gid);

struct passwd *LsaNssGetPwNam(PCSTR pszName);

DWORD
LsaNssListUsers(
        HANDLE hLsaConnection,
        attrval_t* pResult
        );

VOID
LsaNssGetUserAttr(
        HANDLE hLsaConnection,
        PLSA_USER_INFO_2 pInfo,
        PSTR pszAttribute,
        attrval_t* pResult
        );

DWORD
LsaNssGetUserAttrs(
        HANDLE hLsaConnection,
        PSTR pszKey,
        PSTR* ppszAttributes,
        attrval_t* pResults,
        int iAttrCount
        );

#endif
