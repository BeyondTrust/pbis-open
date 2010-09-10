/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        adutils.h
 *
 * Abstract:
 *
 *       Mac Workgroup Manager
 *
 *       AD Utility API (Private Header)
 *
 * Author: Glenn Curtis (glennc@likewisesoftware.com)
 *         Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __ADUTILS_H__
#define __ADUTILS_H__


LONG
AuthenticateUser(
    PCSTR pszUsername,
    PCSTR pszPassword,
    BOOLEAN fAuthOnly
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

