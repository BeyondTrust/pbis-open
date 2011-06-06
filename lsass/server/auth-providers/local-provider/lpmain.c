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
 *        lpmain.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Gerald Carter <gcarter@likewise.com>
 */

#include "includes.h"

static
DWORD
LocalSetDomainName(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN gid_t   peerGID,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    );

static
DWORD
LocalSetDomainSid(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN gid_t   peerGID,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    );

static
DWORD
LocalInitializeProvider(
    OUT PCSTR* ppszProviderName,
    OUT PLSA_PROVIDER_FUNCTION_TABLE* ppFunctionTable
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LOCAL_CONFIG config;
    BOOLEAN bEventLogEnabled = FALSE;
    PWSTR   pwszUserDN = NULL;
    PWSTR   pwszCredentials = NULL;
    ULONG   ulMethod = 0;

    memset(&config, 0, sizeof(config));

    pthread_mutex_init(&gLPGlobals.cfgMutex, NULL);

    dwError = LocalCfgInitialize(&config);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_rwlock_init(&gLPGlobals.rwlock, NULL);

    ntStatus = LwMapSecurityCreateContext(&gLPGlobals.pSecCtx);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LocalSyncDomainInfo(
                    pwszUserDN,
                    pwszCredentials,
                    ulMethod,
                    &gLPGlobals);
    BAIL_ON_LSA_ERROR(dwError);

    LocalCfgReadRegistry(&config);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCfgTransferContents(
                    &config,
                    &gLPGlobals.cfg);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCfgIsEventlogEnabled(&bEventLogEnabled);
    BAIL_ON_LSA_ERROR(dwError);

    if (bEventLogEnabled)
    {
        LocalEventLogServiceStart(dwError);
    }

    *ppszProviderName = gpszLocalProviderName;
    *ppFunctionTable = &gLocalProviderAPITable;

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:

    if (bEventLogEnabled)
    {
        LocalEventLogServiceStart(dwError);
    }

    LocalCfgFreeContents(&config);

    LwMapSecurityFreeContext(&gLPGlobals.pSecCtx);

    *ppszProviderName = NULL;
    *ppFunctionTable = NULL;

    goto cleanup;
}

DWORD
LocalOpenHandle(
    HANDLE hServer,
    PCSTR pszInstance,
    PHANDLE phProvider
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = NULL;

    dwError = LwAllocateMemory(
                    sizeof(LOCAL_PROVIDER_CONTEXT),
                    (PVOID*)&pContext);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_mutex_init(&pContext->mutex, NULL);
    pContext->pMutex = &pContext->mutex;

    LsaSrvGetClientId(
        hServer,
        &pContext->uid,
        &pContext->gid,
        &pContext->pid);

    pContext->localAdminState = LOCAL_ADMIN_STATE_NOT_DETERMINED;

    dwError = DirectoryOpen(&pContext->hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    *phProvider = (HANDLE)pContext;

cleanup:

    return dwError;

error:

    *(phProvider) = (HANDLE)NULL;

    if (pContext)
    {
        LocalCloseHandle((HANDLE)pContext);
    }

    goto cleanup;
}

void
LocalCloseHandle(
    HANDLE hProvider
    )
{
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    if (pContext->hDirectory)
    {
        DirectoryClose(pContext->hDirectory);
    }

    if (pContext->pMutex)
    {
        pthread_mutex_destroy(&pContext->mutex);
    }

    if (pContext)
    {
        LwFreeMemory(pContext);
    }
}

DWORD
LocalServicesDomain(
    PCSTR pszDomain,
    BOOLEAN *pbServicesDomain
    )
{
    DWORD dwError = 0;

    *pbServicesDomain = LocalServicesDomainInternal(pszDomain);

    return dwError;
}

BOOLEAN
LocalServicesDomainInternal(
    PCSTR pszDomain
    )
{
    BOOLEAN bResult = FALSE;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszDomain) &&
        (!strcasecmp(pszDomain, gLPGlobals.pszNetBIOSName) ||
         !strcasecmp(pszDomain, gLPGlobals.pszLocalDomain) ||
         !strcasecmp(pszDomain, gLPGlobals.pszBuiltinDomain)))
    {
        bResult = TRUE;
    }

    return bResult;
}

DWORD
LocalAuthenticateUserPam(
    HANDLE hProvider,
    LSA_AUTH_USER_PAM_PARAMS* pParams,
    PLSA_AUTH_USER_PAM_INFO* ppPamAuthInfo
    )
{
    DWORD dwError = 0;
    DWORD dwUpdateError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    PWSTR pwszUserDN = NULL;
    PWSTR pwszPassword = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
    DWORD dwBadPasswordCount = 0;
    PLSA_AUTH_USER_PAM_INFO pPamAuthInfo = NULL;
    BOOLEAN bUserIsGuest = FALSE;

    if (ppPamAuthInfo)
    {
        *ppPamAuthInfo = NULL;
    }

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(*pPamAuthInfo),
                    (PVOID*)&pPamAuthInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirFindObjectByGenericName(
        hProvider,
        0,
        LSA_OBJECT_TYPE_USER,
        pParams->pszLoginName,
        &pObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(
        pObject->pszDN,
        &pwszUserDN);
    BAIL_ON_LSA_ERROR(dwError);

    /* Check for disable, expired, etc..  accounts */

    dwError = LocalCheckIsGuest(pObject, &bUserIsGuest);
    BAIL_ON_LSA_ERROR(dwError);

    if (bUserIsGuest)
    {
        dwError = LW_ERROR_LOGON_FAILURE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalCheckAccountFlags(pObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalGetUserLogonInfo(
                  hProvider,
                  pObject->pszDN,
                  NULL,
                  &dwBadPasswordCount);
    BAIL_ON_LSA_ERROR(dwError);

    if (pParams->pszPassword)
    {
        dwError = LwMbsToWc16s(
                        pParams->pszPassword,
                        &pwszPassword);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = DirectoryVerifyPassword(
                    pContext->hDirectory,
                    pwszUserDN,
                    pwszPassword);
    if (dwError == LW_ERROR_PASSWORD_MISMATCH)
    {
        dwBadPasswordCount++;

        dwUpdateError = LocalSetUserLogonInfo(
                        hProvider,
                        pObject->pszDN,
                        NULL,
                        &dwBadPasswordCount,
                        NULL,
                        NULL);
        BAIL_ON_LSA_ERROR(dwUpdateError);
        
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (ppPamAuthInfo)
    {
        *ppPamAuthInfo = pPamAuthInfo;
    }

cleanup:

    LsaUtilFreeSecurityObject(pObject);

    LW_SAFE_FREE_MEMORY(pwszUserDN);
    LW_SECURE_FREE_WSTRING(pwszPassword);

    if (dwUpdateError != ERROR_SUCCESS)
    {
        dwError = dwUpdateError;
    }
    return dwError;

error:

    if (pPamAuthInfo)
    {
        LsaFreeAuthUserPamInfo(pPamAuthInfo);
    }

    goto cleanup;
}


DWORD
LocalAuthenticateUserEx(
    HANDLE                hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO*  ppUserInfo
    )
{
    return LocalAuthenticateUserExInternal(hProvider,
                                           pUserParams,
                                           ppUserInfo);
}

DWORD
LocalValidateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pObject = NULL;

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirFindObjectByGenericName(
        hProvider,
        0,
        LSA_OBJECT_TYPE_USER,
        pszLoginId,
        &pObject);

    if (dwError != LW_ERROR_SUCCESS)
    {
        LSA_LOG_DEBUG(
                "Failed to find user '%s' while validating login "
                "[error code:%u]",
                pszLoginId,
                dwError);
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pObject->userInfo.bPasswordExpired)
    {
        dwError = LW_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
cleanup:

    LsaUtilFreeSecurityObject(pObject);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalCheckUserInList(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszListName
    )
{
    // require-membership-of is currently not supported
    // for the local provider.  So just return success here.

    return LW_ERROR_SUCCESS;
}

DWORD
LocalChangePassword(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword
    )
{
    DWORD dwError = 0;
    PWSTR pwszUserDN  = NULL;
    PWSTR pwszOldPassword = NULL;
    PWSTR pwszNewPassword = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
  
    BAIL_ON_INVALID_HANDLE(hProvider);

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirFindObjectByGenericName(
        hProvider,
        0,
        LSA_OBJECT_TYPE_USER,
        pszLoginId,
        &pObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(
        pObject->pszDN,
        &pwszUserDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCheckForPasswordChangeAccess(
                    hProvider,
                    pObject->userInfo.uid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCheckPasswordPolicy(
                    pObject,
                    pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(
                        (pszPassword ? pszPassword : ""),
                        &pwszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(
                        (pszOldPassword ? pszOldPassword : ""),
                        &pwszOldPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirChangePassword(
                    hProvider,
                    pwszUserDN,
                    pwszOldPassword,
                    pwszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaUtilFreeSecurityObject(pObject);

    LW_SECURE_FREE_WSTRING(pwszNewPassword);
    LW_SECURE_FREE_WSTRING(pwszOldPassword);
    LW_SAFE_FREE_MEMORY(pwszUserDN);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalSetPassword(
    HANDLE hProvider,
    PCSTR pszLoginId,
    PCSTR pszPassword
    )
{
    DWORD dwError = 0;
    PWSTR pwszUserDN = NULL;
    PWSTR pwszNewPassword = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
  
    BAIL_ON_INVALID_HANDLE(hProvider);

    dwError = LocalDirFindObjectByGenericName(
        hProvider,
        0,
        LSA_OBJECT_TYPE_USER,
        pszLoginId,
        &pObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(
        pObject->pszDN,
        &pwszUserDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCheckPasswordPolicy(
                    pObject,
                    pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(
                        (pszPassword ? pszPassword : ""),
                        &pwszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirSetPassword(
                    hProvider,
                    pwszUserDN,
                    pwszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaUtilFreeSecurityObject(pObject);

    LW_SECURE_FREE_WSTRING(pwszNewPassword);
    LW_SAFE_FREE_MEMORY(pwszUserDN);

    return dwError;

error:
    goto cleanup;
}

DWORD
LocalAddUser(
    HANDLE hProvider,
    PLSA_USER_ADD_INFO pUserAddInfo
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(hProvider);
    BAIL_ON_INVALID_POINTER(pUserAddInfo);

    dwError = LocalCheckForAddAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirAddUser(
                    hProvider,
                    pUserAddInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO_2 pUserModInfo
    )
{
    DWORD   dwError = 0;

    dwError = LocalCheckForModifyAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirModifyUser(
                    hProvider,
                    pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
LocalDeleteObject(
    HANDLE hProvider,
    PCSTR pszSid
    )
{
    DWORD dwError = 0;
    PWSTR pwszDN  = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;

    BAIL_ON_INVALID_HANDLE(hProvider);

    dwError = LocalCheckForDeleteAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    QueryList.ppszStrings = &pszSid;

    dwError = LocalFindObjects(
        hProvider,
        0,
        LSA_OBJECT_TYPE_UNDEFINED,
        LSA_QUERY_TYPE_BY_SID,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LwMbsToWc16s(
        ppObjects[0]->pszDN,
        &pwszDN);
    BAIL_ON_LSA_ERROR(dwError);
    

    switch (ppObjects[0]->type)
    {
    case LSA_OBJECT_TYPE_USER:
        dwError = LocalDirDeleteUser(
            hProvider,
            pwszDN);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case LSA_OBJECT_TYPE_GROUP:
        dwError = LocalDirDeleteGroup(
            hProvider,
            pwszDN);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    LW_SAFE_FREE_MEMORY(pwszDN);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalAddGroup(
    HANDLE hProvider,
    PLSA_GROUP_ADD_INFO pGroupAddInfo
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(hProvider);
    BAIL_ON_INVALID_POINTER(pGroupAddInfo);

    dwError = LocalCheckForAddAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirAddGroup(
                    hProvider,
                    pGroupAddInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalModifyGroup(
    HANDLE hProvider,
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo
    )
{
    DWORD dwError = 0;

    dwError = LocalCheckForModifyAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirModifyGroup(
                    hProvider,
                    pGroupModInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
LocalOpenSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    BOOLEAN bCreateHomedir = FALSE;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    PLSA_SECURITY_OBJECT pObject = NULL;
    DWORD dwLogonCount = 0;
    LONG64 llLastLogonTime = 0;
  
    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_HANDLE(hProvider);

    dwError = LocalDirFindObjectByGenericName(
        hProvider,
        0,
        LSA_OBJECT_TYPE_USER,
        pszLoginId,
        &pObject);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (pObject && !pObject->enabled)
    {
        dwError = ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Allow directory creation only if this is
    //
    if ((pContext->uid != 0) &&
        (pContext->uid != (pObject->userInfo.uid)))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalCfgMustCreateHomedir(&bCreateHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    if (bCreateHomedir)
    {
        dwError = LocalCreateHomeDirectory(pObject);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalGetUserLogonInfo(
                  hProvider,
                  pObject->pszDN,
                  &dwLogonCount,
                  NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwLogonCount++;

    dwError = LwGetNtTime((PULONG64)&llLastLogonTime);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalSetUserLogonInfo(
                    hProvider,
                    pObject->pszDN,
                    &dwLogonCount,
                    NULL,
                    &llLastLogonTime,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaUtilFreeSecurityObject(pObject);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalCloseSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pObject = NULL;
    LONG64 llLastLogoffTime = 0;

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirFindObjectByGenericName(
        hProvider,
        0,
        LSA_OBJECT_TYPE_USER,
        pszLoginId,
        &pObject);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject && !pObject->enabled)
    {
        dwError = ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwGetNtTime((PULONG64)&llLastLogoffTime);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalSetUserLogonInfo(
                    hProvider,
                    pObject->pszDN,
                    NULL,
                    NULL,
                    NULL,
                    &llLastLogoffTime);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaUtilFreeSecurityObject(pObject);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalFindNSSArtefactByKey(
    HANDLE hProvider,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
    *ppNSSArtefactInfo = NULL;

    return LW_ERROR_NOT_HANDLED;
}


DWORD
LocalBeginEnumNSSArtefacts(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;

    dwError = LW_ERROR_NOT_HANDLED;

    *phResume = (HANDLE)NULL;

    return dwError;
}

DWORD
LocalEnumNSSArtefacts(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxNSSArtefacts,
    PDWORD   pdwNSSArtefactsFound,
    PVOID**  pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;

    dwError = LW_ERROR_NOT_HANDLED;

    *pdwNSSArtefactsFound = 0;
    *pppNSSArtefactInfoList = NULL;

    return dwError;
}

VOID
LocalEndEnumNSSArtefacts(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    return;
}

DWORD
LocalGetStatus(
    HANDLE hProvider,
    PLSA_AUTH_PROVIDER_STATUS* ppProviderStatus
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus = NULL;
    BOOLEAN bInLock = FALSE;

    dwError = LwAllocateMemory(
                   sizeof(LSA_AUTH_PROVIDER_STATUS),
                   (PVOID*)&pProviderStatus);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    gpszLocalProviderName,
                    &pProviderStatus->pszId);
    BAIL_ON_LSA_ERROR(dwError);

    pProviderStatus->mode = LSA_PROVIDER_MODE_LOCAL_SYSTEM;
    pProviderStatus->status = LSA_AUTH_PROVIDER_STATUS_ONLINE;

    LOCAL_RDLOCK_RWLOCK(bInLock, &gLPGlobals.rwlock);

    dwError = LwAllocateString(
                    gLPGlobals.pszLocalDomain,
                    &pProviderStatus->pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlAllocateCStringFromSid(
                    &pProviderStatus->pszDomainSid,
                    gLPGlobals.pLocalDomainSID);
    dwError = LwNtStatusToWin32Error(ntStatus);
    BAIL_ON_LSA_ERROR(dwError);

    *ppProviderStatus = pProviderStatus;

cleanup:

    LOCAL_UNLOCK_RWLOCK(bInLock, &gLPGlobals.rwlock);

    return dwError;

error:

    *ppProviderStatus = NULL;

    if (pProviderStatus)
    {
        LocalFreeStatus(pProviderStatus);
    }

    goto cleanup;
}

DWORD
LocalRefreshConfiguration(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    LOCAL_CONFIG config = {0};
    BOOLEAN bInLock = FALSE;

    dwError = LocalCfgReadRegistry(&config);
    BAIL_ON_LSA_ERROR(dwError);

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    dwError = LocalCfgTransferContents(
                    &config,
                    &gLPGlobals.cfg);
    BAIL_ON_LSA_ERROR(dwError);

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    LocalEventLogConfigReload();

cleanup:

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    return dwError;

error:

    LocalCfgFreeContents(&config);

    goto cleanup;
}

VOID
LocalFreeStatus(
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus
    )
{
    LW_SAFE_FREE_STRING(pProviderStatus->pszId);
    LW_SAFE_FREE_STRING(pProviderStatus->pszDomain);
    LW_SAFE_FREE_STRING(pProviderStatus->pszDomainSid);
    LW_SAFE_FREE_STRING(pProviderStatus->pszForest);
    LW_SAFE_FREE_STRING(pProviderStatus->pszSite);
    LW_SAFE_FREE_STRING(pProviderStatus->pszCell);

    LwFreeMemory(pProviderStatus);
}

DWORD
LocalIoControl(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN gid_t   peerGID,
    IN DWORD   dwIoControlCode,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    PVOID pOutputBuffer = NULL;
    DWORD dwOutputBufferSize = 0;

    switch (dwIoControlCode)
    {
    case LSA_LOCAL_IO_SETDOMAINNAME:
        dwError = LocalSetDomainName(hProvider,
                                     peerUID,
                                     peerGID,
                                     dwInputBufferSize,
                                     pInputBuffer,
                                     NULL,
                                     NULL);
        break;

    case LSA_LOCAL_IO_SETDOMAINSID:
        dwError = LocalSetDomainSid(hProvider,
                                    peerUID,
                                    peerGID,
                                    dwInputBufferSize,
                                    pInputBuffer,
                                    NULL,
                                    NULL);
        break;

    default:
        dwError = LW_ERROR_NOT_HANDLED;
        break;
    }
    BAIL_ON_LSA_ERROR(dwError);

    *pdwOutputBufferSize = dwOutputBufferSize;
    *ppOutputBuffer      = pOutputBuffer;

cleanup:
    return dwError;

error:
    *pdwOutputBufferSize = 0;
    *ppOutputBuffer      = NULL;

    goto cleanup;
}

static
DWORD
LocalSetDomainName(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN gid_t   peerGID,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD dwError = ERROR_SUCCESS;

    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalDirSetDomainName((PSTR)pInputBuffer);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LocalSetDomainSid(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN gid_t   peerGID,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD dwError = ERROR_SUCCESS;

    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalDirSetDomainSid((PSTR)pInputBuffer);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LocalFindObjects(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;

    dwError = LocalDirFindObjects(
        hProvider,
        FindFlags,
        ObjectType,
        QueryType,
        dwCount,
        QueryList,
        pppObjects);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalOpenEnumObjects(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    return LocalDirOpenEnumObjects(
        hProvider,
        phEnum,
        FindFlags,
        ObjectType,
        pszDomainName);
}

DWORD
LocalEnumObjects(
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    return LocalDirEnumObjects(
        hEnum,
        dwMaxObjectsCount,
        pdwObjectsCount,
        pppObjects);
}

DWORD
LocalOpenEnumMembers(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid
    )
{
    return LocalDirOpenEnumMembers(
        hProvider,
        phEnum,
        FindFlags,
        pszSid);
}

DWORD
LocalEnumMembers(
    IN HANDLE hEnum,
    IN DWORD dwMaxMemberSidCount,
    OUT PDWORD pdwMemberSidCount,
    OUT PSTR** pppszMemberSids
    )
{
    return LocalDirEnumMembers(
        hEnum,
        dwMaxMemberSidCount,
        pdwMemberSidCount,
        pppszMemberSids);
}

VOID
LocalCloseEnum(
    IN OUT HANDLE hEnum
    )
{
    return LocalDirCloseEnum(hEnum);
}

DWORD
LocalQueryMemberOf(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    )
{
    return LocalDirQueryMemberOf(
        hProvider,
        FindFlags,
        dwSidCount,
        ppszSids,
        pdwGroupSidCount,
        pppszGroupSids);
}

DWORD
LocalGetSmartCardUserObject(
    IN HANDLE hProvider,
    OUT PLSA_SECURITY_OBJECT* ppObject,
    OUT PSTR* ppszSmartCardReader
    )
{
    *ppObject = NULL;
    *ppszSmartCardReader = NULL;

    return LW_ERROR_NOT_HANDLED;
}

DWORD
LocalShutdownProvider(
    VOID
    )
{
    DWORD dwError = ERROR_SUCCESS;
    BOOLEAN bLocked = FALSE;
    BOOLEAN bCfgLocked = FALSE;

    LOCAL_WRLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    LwMapSecurityFreeContext(&gLPGlobals.pSecCtx);

    LW_SAFE_FREE_STRING(gLPGlobals.pszLocalDomain);
    LW_SAFE_FREE_STRING(gLPGlobals.pszNetBIOSName);
    RTL_FREE(&gLPGlobals.pLocalDomainSID);

    LOCAL_LOCK_MUTEX(bCfgLocked, &gLPGlobals.cfgMutex);

    LocalCfgFreeContents(&gLPGlobals.cfg);

    LOCAL_UNLOCK_MUTEX(bCfgLocked, &gLPGlobals.cfgMutex);
    LOCAL_UNLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    pthread_rwlock_destroy(&gLPGlobals.rwlock);

    return dwError;
}

DWORD
LsaInitializeProvider(
    OUT PCSTR* ppszProviderName,
    OUT PLSA_PROVIDER_FUNCTION_TABLE* ppFunctionTable
    )
{
    return LocalInitializeProvider(ppszProviderName, ppFunctionTable);
}
