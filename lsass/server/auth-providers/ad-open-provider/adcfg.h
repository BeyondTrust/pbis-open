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
 *        adcfg.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 *        Wrappers for accessing global configuration variables
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#ifndef __AD_CFG_H__
#define __AD_CFG_H__

#define AD_PROVIDER_REGKEY "Services\\lsass\\Parameters\\Providers\\ActiveDirectory"
#define AD_PROVIDER_DOMAINJOIN_REGKEY AD_PROVIDER_REGKEY "\\DomainJoin"

#define ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState)    \
        if (!bInLock) {                                    \
           AD_ConfigLockAcquireRead(pState);               \
           bInLock = TRUE;                                 \
        }

#define LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState)    \
        if (bInLock) {                                     \
           AD_ConfigLockRelease(pState);                   \
           bInLock = FALSE;                                \
        }

#define ENTER_AD_CONFIG_RW_WRITER_LOCK(bInLock, pState)    \
        if (!bInLock) {                                    \
           AD_ConfigLockAcquireWrite(pState);              \
           bInLock = TRUE;                                 \
        }

#define LEAVE_AD_CONFIG_RW_WRITER_LOCK(bInLock, pState)    \
        if (bInLock) {                                     \
           AD_ConfigLockRelease(pState);                   \
           bInLock = FALSE;                                \
        }

#define DEFAULT_SERVICE_PRINCIPAL_NAME_LIST "HOST"

DWORD
AD_ReadRegistry(
    IN OPTIONAL PCSTR pszDomainName,
    OUT PLSA_AD_CONFIG pConfig
    );

DWORD
AD_ReadRegistryForDomain(
    IN PCSTR pszDomainName,
    OUT PLSA_AD_CONFIG pConfig
    );

DWORD
AD_TransferConfigContents(
    PLSA_AD_CONFIG pSrcConfig,
    PLSA_AD_CONFIG pDstConfig
    );

DWORD
AD_InitializeConfig(
    PLSA_AD_CONFIG pConfig
    );

VOID
AD_FreeConfig(
    PLSA_AD_CONFIG pConfig
    );

VOID
AD_FreeConfigContents(
    PLSA_AD_CONFIG pConfig
    );

VOID
AD_FreeConfigMemberInList(
    PVOID pItem,
    PVOID pUserData
    );

DWORD
AD_GetUnprovisionedModeShell(
    PLSA_AD_PROVIDER_STATE pState,
    PSTR* ppszUnprovisionedModeShell
    );

DWORD
AD_GetHomedirPrefixPath(
    PLSA_AD_PROVIDER_STATE pState,
    PSTR* ppszPath
    );

DWORD
AD_GetUserDomainPrefix(
    PLSA_AD_PROVIDER_STATE pState,
    PSTR* ppszPath
    );

DWORD
AD_GetUnprovisionedModeHomedirTemplate(
    PLSA_AD_PROVIDER_STATE pState,
    PSTR* ppszUnprovisionedModeHomedirTemplate
    );

DWORD
AD_GetUnprovisionedModeRemoteHomeDirTemplate(
    PLSA_AD_PROVIDER_STATE pState,
    PSTR* ppszUnprovisionedModeHomedirTemplate
    );

DWORD
AD_GetMachinePasswordSyncPwdLifetime(
    IN PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_GetServicePrincipalNameList(
    IN PLSA_AD_PROVIDER_STATE pState,
    OUT PSTR*  ppszServicePrincipalNameList
    );

DWORD
AD_GetClockDriftSeconds(
    IN PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_GetCacheEntryExpirySeconds(
    IN PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_GetUmask(
    IN PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_GetSkelDirs(
    PLSA_AD_PROVIDER_STATE pState,
    PSTR* ppszSkelDirs
    );

BOOLEAN
AD_GetLDAPSignAndSeal(
    PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_AddAllowedMember(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszSID,
    IN PSTR pszMember,
    IN OUT PLW_HASH_TABLE *pAllowedMemberList
    );

VOID
AD_DeleteFromMembersList(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszMember
    );

DWORD
AD_GetMemberLists(
    PLSA_AD_PROVIDER_STATE pState,
    PSTR** pppszMembers,
    PDWORD pdwNumMembers,
    PLW_HASH_TABLE* ppAllowedMemberList
    );

BOOLEAN
AD_ShouldFilterUserLoginsByGroup(
    IN PLSA_AD_PROVIDER_STATE pState
    );

BOOLEAN
AD_IsMemberAllowed(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR           pszSID,
    PLW_HASH_TABLE pAllowedMemberList
    );

VOID
AD_FreeAllowedSIDs_InLock(
    IN PLSA_AD_PROVIDER_STATE pState
    );

BOOLEAN
AD_ShouldAssumeDefaultDomain(
    IN PLSA_AD_PROVIDER_STATE pState
    );

BOOLEAN
AD_ShouldSyncSystemTime(
    IN PLSA_AD_PROVIDER_STATE pState
    );

BOOLEAN
AD_EventlogEnabled(
    IN PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_CheckIgnoreUserNameList(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszUserName,
    PBOOLEAN pbFoundIt
    );

DWORD
AD_CheckIgnoreGroupNameList(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszGroupName,
    PBOOLEAN pbFoundIt
    );

BOOLEAN
AD_ShouldLogNetworkConnectionEvents(
    IN PLSA_AD_PROVIDER_STATE pState
    );

BOOLEAN
AD_ShouldCreateK5Login(
    IN PLSA_AD_PROVIDER_STATE pState
    );

BOOLEAN
AD_ShouldCreateHomeDir(
    IN PLSA_AD_PROVIDER_STATE pState
    );

BOOLEAN
AD_ShouldRefreshUserCreds(
    IN PLSA_AD_PROVIDER_STATE pState
    );

AD_CELL_SUPPORT
AD_GetCellSupport(
    IN PLSA_AD_PROVIDER_STATE pState
    );

AD_CACHE_BACKEND
AD_GetCacheBackend(
    IN PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_GetCacheSizeCap(
    IN PLSA_AD_PROVIDER_STATE pState
    );

BOOLEAN
AD_GetTrimUserMembershipEnabled(
    IN PLSA_AD_PROVIDER_STATE pState
    );

BOOLEAN
AD_GetNssGroupMembersCacheOnlyEnabled(
    IN PLSA_AD_PROVIDER_STATE pState
    );

BOOLEAN
AD_GetNssUserMembershipCacheOnlyEnabled(
    IN PLSA_AD_PROVIDER_STATE pState
    );

BOOLEAN
AD_GetNssEnumerationEnabled(
    IN PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_GetDomainManagerCheckDomainOnlineSeconds(
    IN PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_GetDomainManagerUnknownDomainCacheTimeoutSeconds(
    IN PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_GetDomainManagerTrustExceptionList(
    IN PLSA_AD_PROVIDER_STATE pState,
    OUT PBOOLEAN pbIgnoreAllTrusts,
    OUT PSTR** pppszTrustsList,
    OUT PDWORD pdwTrustsCount
    );

BOOLEAN
AD_GetAddDomainToLocalGroupsEnabled(
    IN PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_GetTrustEnumerationWait(
    IN PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_GetTrustEnumerationWaitSeconds(
    IN PLSA_AD_PROVIDER_STATE pState
    );

VOID
AD_ConfigLockAcquireRead(
    PLSA_AD_PROVIDER_STATE pState
    );

VOID
AD_ConfigLockAcquireWrite(
    PLSA_AD_PROVIDER_STATE pState
    );

void
AD_ConfigLockRelease(
    PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_GetServicePrincipalNameFromRegistry(PSTR* ppszServicePrincipalNameList);

#endif /* __AD_CFG_H__ */

