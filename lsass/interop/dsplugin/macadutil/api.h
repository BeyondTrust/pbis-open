/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
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
SaveMCXValuesForGPOSettingType(
    PMCXVALUE            pMCXValueList,
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PCSTR                pszUserUPN
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
GetMCXValuesForGPOSettingType(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PMCXVALUE *          ppMCXValueList
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

DWORD
SetupMCXLoginScriptsSupport(
    );


#endif /* __ADUTILS_H__ */

