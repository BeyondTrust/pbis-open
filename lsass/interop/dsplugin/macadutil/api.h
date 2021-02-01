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

#ifndef __ADUTILS_H__
#define __ADUTILS_H__


/* Function signatures for lsa access library */

LONG
GetCurrentDomain(
    OUT PSTR* ppszDnsDomainName
    );

LONG
EnumWorkgroupManagerEnabledGPOs(
    PCSTR                 pszDomainName,
    PGROUP_POLICY_OBJECT* ppMCXGPOs
    );

LONG
GetSpecificGPO(
    PCSTR                 pszDomainName,
    PCSTR                 pszGPOName,
    PGROUP_POLICY_OBJECT* ppGPO
    );

LONG
GetSpecificGPO_authenticated(
    PCSTR                 pszDomainName,
    PCSTR                 pszGPOName,
    PGROUP_POLICY_OBJECT* ppGPO
    );

BOOLEAN
IsMCXSettingEnabledForGPO(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType
    );

LONG
ConvertDSAttributeValuesToMCXValues(
    PDSATTRIBUTEVALUE    pValues,
    PMCXVALUE *          ppMCXValues
    );

LONG
ConvertMCXSettingsToMCXValues(
    PCSTR      pszPolicyPath,
    DWORD      dwPolicyType,
    PMCXVALUE* ppMCXValueList
    );

LONG
GetHomeDirectoryDockMCXValue(
    PMCXVALUE * ppMCXValueList
    );

LONG
LookupComputerGroupGPO(
    PCSTR pszName,
    PSTR* ppszGPOGUID
    );

LONG
LookupComputerListGPO(
    PCSTR  pszName,
    PSTR * ppszGPOGUID
    );

LONG
AuthenticateUser(
    PCSTR pszUsername,
    PCSTR pszPassword,
    BOOLEAN fAuthOnly,
    PBOOLEAN pIsOnlineLogon,
    PSTR * ppszMessage
    );

LONG
ChangePassword(
    PCSTR pszUsername,
    PCSTR pszOldPassword,
    PCSTR pszNewPassword
    );

LONG
GetUserPrincipalNames(
    PCSTR pszUsername,
    PSTR * ppszUserPrincipalName,
    PSTR * ppszUserSamAccount,
    PSTR * ppszUserDomainFQDN
    );

LONG
GetUserAccountPolicy(
    PCSTR pszUserName,
    PDWORD pdwDaysToPasswordExpiry,
    PBOOLEAN pbDisabled,
    PBOOLEAN pbExpired,
    PBOOLEAN pbLocked,
    PBOOLEAN pbPasswordNeverExpires,
    PBOOLEAN pbPasswordExpired,
    PBOOLEAN pbPromptForPasswordChange,
    PBOOLEAN pbUserCanChangePassword,
    PBOOLEAN pbLogonRestriction
    );

void
GetLsaStatus(
    PBOOLEAN pbIsStarted
    );

LONG
GetADUserInfo(
    uid_t uid,
    PAD_USER_ATTRIBUTES * ppadUserInfo
    );

void
FreeADUserInfo(
    PAD_USER_ATTRIBUTES pUserADAttrs
    );

#define LWDSPLUGIN_POLICIES "Policy\\Services\\lwdsplugin\\Parameters"
#define LWDSPLUGIN_SETTINGS "Services\\lwdsplugin\\Parameters"

LONG
GetConfigurationSettings(
    BOOLEAN * pbMergeModeMCX,
    BOOLEAN * pbEnableForceHomedirOnStartupDisk,
    BOOLEAN * pbUseADUNCForHomeLocation,
    PSTR *    ppszUNCProtocolForHomeLocation,
    PSTR *    ppszAllowAdministrationBy,
    BOOLEAN * pbMergeAdmins,
    DWORD *   pdwCacheLifeTime
    );

LONG
GetAccessCheckData(
    PSTR    pszAdminAllowList,
    PVOID * ppAccessData
    );

LONG
CheckUserForAccess(
    PCSTR  pszUsername,
    PCVOID pAccessData
    );

LONG
FreeAccessCheckData(
    PVOID pAccessData
    );

LONG
CopyADUserInfo(
    PAD_USER_ATTRIBUTES pUserADInfo,
    PAD_USER_ATTRIBUTES * ppUserADInfoCopy
    );

LONG
GetUserObjects(
    PLSA_SECURITY_OBJECT** pppUserObjects,
    PDWORD                 pdwNumUsersFound
    );

LONG
GetGroupObjects(
    PLSA_SECURITY_OBJECT** pppGroupObjects,
    PDWORD                 pdwNumGroupsFound
    );

LONG
GetUserObjectFromId(
    uid_t                  uid,
    PLSA_SECURITY_OBJECT** pppUserObject
    );

LONG
GetUserObjectFromName(
    PCSTR                  pszName,
    PLSA_SECURITY_OBJECT** pppUserObject
    );

LONG
GetUserGroups(
    PCSTR                  pszUserSid,
    PLSA_SECURITY_OBJECT** pppGroups,
    PDWORD                 pdwNumGroupsFound
    );

LONG
GetGroupObjectFromId(
    gid_t                  gid,
    PLSA_SECURITY_OBJECT** pppGroupObject
    );

LONG
GetGroupObjectFromName(
    PCSTR                  pszName,
    PLSA_SECURITY_OBJECT** pppGroupObject
    );

LONG
ExpandGroupMembers(
    PCSTR                  pszGroupSid,
    PLSA_SECURITY_OBJECT** pppMembers,
    PDWORD                 pdwMemberCount
    );

void
FreeUserInfo(
    PLSA_USER_INFO_2 pUserInfo2
    );

void
FreeUserInfoList(
    PVOID* ppUserInfo2List,
    DWORD  dwNumberOfUsers
    );

void
FreeGroupInfo(
    PLSA_GROUP_INFO_1 pGroupInfo1
    );

void
FreeGroupInfoList(
    PVOID* ppGroupInfo1List,
    DWORD  dwNumberOfGroups
    );

void
FreeObjectList(
    DWORD                 dwCount,
    PLSA_SECURITY_OBJECT* ppObjects
    );

#endif /* __ADUTILS_H__ */

