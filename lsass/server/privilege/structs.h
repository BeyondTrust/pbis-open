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
 *        structs.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Local Privileges structures and typedefs
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef __LSASRV_PRIVILEGE_STRUCTS_H__
#define __LSASRV_PRIVILEGE_STRUCTS_H__


typedef struct _LSASRV_PRIVILEGE_GLOBALS
{
    pthread_rwlock_t privilegesRwLock;
    PLW_HASH_TABLE pPrivileges;

    pthread_rwlock_t accountsRwLock;
    PLW_HASH_TABLE pAccounts;

    PSECURITY_DESCRIPTOR_ABSOLUTE pPrivilegesSecDesc;
    PSECURITY_DESCRIPTOR_RELATIVE pAccountsSecDescRelative;
    DWORD accountsSecDescRelativeSize;

    PLW_MAP_SECURITY_CONTEXT pSecurityContext;

} LSASRV_PRIVILEGE_GLOBALS, *PLSASRV_PRIVILEGE_GLOBALS;


typedef struct _LSA_PRIVILEGE
{
    PSTR     pszName;
    PWSTR    pwszDescription;
    LUID     Luid;
    BOOLEAN  EnabledByDefault;

} LSA_PRIVILEGE, *PLSA_PRIVILEGE;


typedef struct _LSA_ACCOUNT
{
    pthread_rwlock_t accountRwLock;
    LONG Refcount;
    BOOLEAN Delete;

    PSID pSid;
    LUID_AND_ATTRIBUTES Privileges[LSA_MAX_PRIVILEGES_COUNT];
    DWORD NumPrivileges;
    DWORD SystemAccessRights;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDesc;

} LSA_ACCOUNT, *PLSA_ACCOUNT;


struct _LSA_ACCOUNT_CONTEXT
{
    PLSA_ACCOUNT pAccount;
    BOOLEAN Dirty;

    PACCESS_TOKEN accessToken;
    ACCESS_MASK grantedAccess;
    BOOLEAN releaseAccessToken;
};


typedef struct _LSASRV_PRIVILEGE_CLIENT_STATE
{
    DWORD peerUID;
    DWORD peerGID;
    DWORD peerPID;

} LSASRV_PRIVILEGE_CLIENT_STATE, *PLSASRV_PRIVILEGE_CLIENT_STATE;


#endif /* __LSASRV_PRIVILEGE_STRUCTS_H__ */
