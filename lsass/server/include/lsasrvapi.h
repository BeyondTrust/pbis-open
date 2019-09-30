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
 *        lsasrvapi.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Server API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LSASRVAPI_H__
#define __LSASRVAPI_H__

#include <lsa/provider.h>

DWORD
LsaSrvApiInit(
    PLSA_STATIC_PROVIDER pStaticProviders
    );

DWORD
LsaSrvApiShutdown(
    VOID
    );

DWORD
LsaSrvOpenServer(
    uid_t peerUID,
    gid_t peerGID,
    pid_t peerPID,
    PHANDLE phServer
    );

VOID
LsaSrvGetClientId(
    HANDLE hServer,
    uid_t* pUid,
    gid_t* pGid,
    pid_t* pPid
    );

void
LsaSrvCloseServer(
    HANDLE hServer
    );

DWORD
LsaSrvAuthenticateUserPam(
    HANDLE hServer,
    PLSA_AUTH_USER_PAM_PARAMS pParams,
    PLSA_AUTH_USER_PAM_INFO* ppPamAuthInfo
    );

DWORD
LsaSrvAuthenticateUserEx(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_AUTH_USER_PARAMS pUserParms,
    PLSA_AUTH_USER_INFO *ppUserInfo
    );


DWORD
LsaSrvValidateUser(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    );

DWORD
LsaSrvCheckUserInList(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszListName
    );

DWORD
LsaSrvChangePassword(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword
    );

DWORD
LsaSrvSetPassword(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    );

DWORD
LsaSrvAddGroup2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_GROUP_ADD_INFO pGroupAddInfo
    );

DWORD
LsaSrvModifyGroup2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo
    );

DWORD
LsaSrvDeleteObject(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PCSTR pszSid
    );

DWORD
LsaSrvAddUser2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_USER_ADD_INFO pUserAddInfo
    );

DWORD
LsaSrvModifyUser2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_USER_MOD_INFO_2 pUserModInfo
    );

DWORD
LsaSrvFindNSSArtefactByKey(
    HANDLE hServer,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    DWORD  dwMapInfoLevel,
    PVOID* ppNSSArtefactInfo
    );

DWORD
LsaSrvBeginEnumNSSArtefacts(
    HANDLE hServer,
    PCSTR  pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    DWORD  dwGroupInfoLevel,
    DWORD  dwNumMaxGroups,
    PHANDLE phState
    );

DWORD
LsaSrvEnumNSSArtefacts(
    HANDLE  hServer,
    HANDLE  hState,
    PDWORD  pdwGroupInfoLevel,
    PVOID** pppGroupInfoList,
    PDWORD  pdwNumGroupsFound
    );

DWORD
LsaSrvEndEnumNSSArtefacts(
    HANDLE hServer,
    HANDLE hState
    );

DWORD
LsaSrvComputeLMHash(
    PCSTR pszPassword,
    PBYTE* ppszHash,
    PDWORD pdwHashLen
    );

DWORD
LsaSrvComputeNTHash(
    PCSTR pszPassword,
    PBYTE* ppszHash,
    PDWORD pdwHashLen
    );

DWORD
LsaSrvOpenSession(
    HANDLE hServer,
    PCSTR  pszLoginId
    );

DWORD
LsaSrvCloseSession(
    HANDLE hServer,
    PCSTR  pszLoginId
    );

DWORD
LsaSrvSetTraceFlags(
    HANDLE          hServer,
    PLSA_TRACE_INFO pTraceFlagArray,
    DWORD           dwNumFlags
    );

DWORD
LsaSrvGetTraceInfo(
    HANDLE hServer,
    DWORD  dwTraceFlag,
    PLSA_TRACE_INFO* ppTraceInfo
    );

DWORD
LsaSrvEnumTraceFlags(
    HANDLE           hServer,
    PLSA_TRACE_INFO* ppTraceFlagArray,
    PDWORD           pdwNumFlags
    );

DWORD
LsaSrvGetMetrics(
    HANDLE hServer,
    DWORD  dwInfoLevel,
    PVOID* ppMetricPack
    );

DWORD
LsaSrvGetStatus(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSASTATUS* ppLsaStatus
    );

DWORD
LsaSrvRefreshConfiguration(
    HANDLE hServer
    );

//
// For targeting specific providers
//

DWORD
LsaSrvProviderIoControl(
    IN HANDLE  hServer,
    IN PCSTR   pszProvider,
    IN DWORD   dwIoControlCode,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    );

//
// Server-Side only APIs for calling specific providers
//

DWORD
LsaSrvProviderGetMachineAccountInfoA(
    IN PCSTR pszProvider,
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A* ppAccountInfo
    );

DWORD
LsaSrvProviderGetMachineAccountInfoW(
    IN PCSTR pszProvider,
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_ACCOUNT_INFO_W* ppAccountInfo
    );

DWORD
LsaSrvProviderGetMachinePasswordInfoA(
    IN PCSTR pszProvider,
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    );

DWORD
LsaSrvProviderGetMachinePasswordInfoW(
    IN PCSTR pszProvider,
    IN OPTIONAL PCSTR DnsDomainName,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* ppPasswordInfo
    );

VOID
LsaSrvFreeMachineAccountInfoA(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    );

VOID
LsaSrvFreeMachineAccountInfoW(
    IN PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo
    );

VOID
LsaSrvFreeMachinePasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    );

VOID
LsaSrvFreeMachinePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    );

DWORD
LsaSrvDuplicateMachineAccountInfoA(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A* ppNewAccountInfo
    );

DWORD
LsaSrvDuplicateMachineAccountInfoW(
    IN PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_W* ppNewAccountInfo
    );

DWORD
LsaSrvDuplicateMachinePasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppNewPasswordInfo
    );

DWORD
LsaSrvDuplicateMachinePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* ppNewPasswordInfo
    );

DWORD
LsaSrvProviderServicesDomain(
    IN PCSTR pszProvider,
    IN PCSTR pszDomainName,
    OUT PBOOLEAN pbServicesDomain
    );

DWORD
LsaSrvGetPamConfig(
    IN HANDLE hServer,
    OUT PLSA_PAM_CONFIG *ppPamConfig
    );

VOID
LsaSrvFreePamConfig(
    IN PLSA_PAM_CONFIG pConfig
    );

#endif /* __LSASRVAPI_H__ */

