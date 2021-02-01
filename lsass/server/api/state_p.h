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
 *        state_p.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Server State Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __STATE_P_H__
#define __STATE_P_H__

#include "auth_provider_p.h"

typedef struct __LSA_SRV_PROVIDER_STATE
{
    PLSA_AUTH_PROVIDER pProvider;
    HANDLE hProvider;
    HANDLE hResume;

    struct __LSA_SRV_PROVIDER_STATE* pNext;
} LSA_SRV_PROVIDER_STATE, *PLSA_SRV_PROVIDER_STATE;

typedef struct __LSA_SRV_ENUM_STATE
{
    DWORD dwNumMaxRecords;
    DWORD dwInfoLevel;
    DWORD dwMapFlags;
    BOOLEAN bCheckOnline;
    LSA_FIND_FLAGS FindFlags;
    PSTR  pszMapName;
    PLSA_SRV_PROVIDER_STATE pProviderStateList;
    PLSA_SRV_PROVIDER_STATE pCurProviderState;

    BOOLEAN bInLock;
} LSA_SRV_ENUM_STATE, *PLSA_SRV_ENUM_STATE;

typedef struct __LSA_SRV_API_STATE
{
    uid_t  peerUID;
    gid_t  peerGID;
    gid_t  peerPID;
} LSA_SRV_API_STATE, *PLSA_SRV_API_STATE;

DWORD
LsaSrvGetTargetElements(
    IN PCSTR pszTargetProvider,
    OUT PSTR* ppszTargetProviderName,
    OUT PSTR* ppszTargetInstance
    );

DWORD
LsaSrvFindProviderByName(
    IN PCSTR pszProvider,
    OUT PLSA_AUTH_PROVIDER* ppProvider
    );

DWORD
LsaSrvOpenProvider(
    HANDLE  hServer,
    PLSA_AUTH_PROVIDER pProvider,
    PCSTR pszInstance,
    PHANDLE phProvider
    );

VOID
LsaSrvCloseProvider(
    PLSA_AUTH_PROVIDER pProvider,
    HANDLE hProvider
    );

DWORD
LsaSrvCreateNSSArtefactEnumState(
    HANDLE  hServer,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    DWORD   dwNSSArtefactInfoLevel,
    DWORD   dwMaxNumArtefacts,
    PLSA_SRV_ENUM_STATE* ppEnumState
    );

VOID
LsaSrvFreeProviderStateList(
    PLSA_SRV_PROVIDER_STATE pStateList
    );

PLSA_SRV_PROVIDER_STATE
LsaSrvReverseProviderStateList(
    PLSA_SRV_PROVIDER_STATE pStateList
    );

VOID
LsaSrvFreeEnumState(
    PLSA_SRV_ENUM_STATE pState
    );

#endif /* __STATE_P_H__ */

