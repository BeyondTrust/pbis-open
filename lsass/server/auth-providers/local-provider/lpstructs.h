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
 *        lpstructs.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider (Structures)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LPSTRUCTS_H__
#define __LPSTRUCTS_H__

typedef struct __LOCAL_PROVIDER_ENUM_STATE
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    DWORD dwInfoLevel;

    PSTR  pszGUID;

    PDIRECTORY_ENTRY pEntries;
    DWORD            dwNumEntries;

    DWORD dwNextStartingId;

    struct __LOCAL_PROVIDER_ENUM_STATE* pNext;

} LOCAL_PROVIDER_ENUM_STATE, *PLOCAL_PROVIDER_ENUM_STATE;

typedef enum
{
    LOCAL_ADMIN_STATE_NOT_DETERMINED = 0,
    LOCAL_ADMIN_STATE_IS_ADMIN,
    LOCAL_ADMIN_STATE_IS_NOT_ADMIN
} LOCAL_ADMIN_STATE;

typedef struct __LOCAL_PROVIDER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    uid_t uid;
    gid_t gid;
    pid_t pid;

    LOCAL_ADMIN_STATE localAdminState;

    HANDLE hDirectory;

} LOCAL_PROVIDER_CONTEXT, *PLOCAL_PROVIDER_CONTEXT;

typedef struct __LOCAL_CONFIG
{
    BOOLEAN   bEnableEventLog;
    DWORD     dwMaxGroupNestingLevel;
    PSTR      pszLoginShell;
    PSTR      pszHomedirPrefix;
    PSTR      pszHomedirTemplate;
    BOOLEAN   bCreateHomedir;
    DWORD     dwHomedirUMask;
    PSTR      pszSkelDirs;
    BOOLEAN   bAcceptNTLMv1;
    BOOLEAN   EnableUnixIds;
} LOCAL_CONFIG, *PLOCAL_CONFIG;

typedef struct _LOCAL_PROVIDER_GLOBALS
{
    pthread_rwlock_t  rwlock;

    PSTR              pszBuiltinDomain;
    PSTR              pszLocalDomain;
    PSTR              pszNetBIOSName;
    PSID              pLocalDomainSID;

    LONG64            llMinPwdAge;
    LONG64            llMaxPwdAge;
    DWORD             dwMinPwdLength;
    LONG64            llPwdChangeTime;
    DWORD             dwLockoutThreshold;
    LONG64            llLockoutDuration;
    LONG64            llLockoutWindow;

    PLW_MAP_SECURITY_CONTEXT pSecCtx;

    pthread_mutex_t   cfgMutex;

    LOCAL_CONFIG      cfg;

} LOCAL_PROVIDER_GLOBALS, *PLOCAL_PROVIDER_GLOBALS;

typedef struct _LOCAL_PROVIDER_GROUP_MEMBER
{
    PSTR pszNetbiosDomain;
    PSTR pszSamAccountName;
    PSTR pszSID;

} LOCAL_PROVIDER_GROUP_MEMBER, *PLOCAL_PROVIDER_GROUP_MEMBER;


#endif /* __LPSTRUCTS_H__ */
