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
 *       lw-gp-admin
 *
 *       AD Utility API (Private Header)
 *
 * Author: Glenn Curtis (glennc@likewisesoftware.com)
 *         Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __ADUTILS_H__
#define __ADUTILS_H__

#if 0
/* Function signatures for lsa access library */

typedef DWORD (*PFNLSAACCESSGETDATA)(
    PCSTR * pczConfigData,
    PVOID * ppAccessData
    );

typedef DWORD (*PFNLSAACCESSCHECKDATA)(
    PCSTR pczUserName,
    PCVOID pAccessData
    );

typedef DWORD (*PFNLSAACCESSFREEDATA)(
    PVOID pAccessData
    );

#define LSA_SYMBOL_NAME_ACCESS_GET_DATA "LsaAccessGetData"
#define LSA_SYMBOL_NAME_ACCESS_CHECK_DATA "LsaAccessCheckData"
#define LSA_SYMBOL_NAME_ACCESS_FREE_DATA "LsaAccessFreeData"
#endif

DWORD
GetADDomain(
    PSTR* ppszDomain
    );

DWORD
EnumEnabledGPOs(
    DWORD                dwPolicyType,
    PCSTR                pszDomainName,
    PSTR                 pszClientGUID,
    PGROUP_POLICY_OBJECT* ppGPOs
    );

DWORD
GetSpecificGPO(
    PCSTR                 pszDomainName,
    PCSTR                 pszGPOName,
    PGROUP_POLICY_OBJECT* ppGPO
    );

DWORD
IsSettingEnabledForGPO(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PSTR                 pszClientGUID,
    PBOOLEAN             pbEnabled
    );

DWORD
SaveValuesForGPOSettingType(
    PGROUP_POLICY_OBJECT pGPO,
    PSTR                 pszClientGUID,
    DWORD                dwPolicyType,
    PSTR                 pszPath
    );

DWORD
GetValuesForGPOSettingType(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PSTR				 pszClientGUID,
    PSTR				 pszPath
    );

DWORD
AuthenticateUser(
    PCSTR pszUsername,
    PCSTR pszPassword
    );

DWORD
GetUserPrincipalName(
    uid_t  uid,
    PSTR * ppszUserPrincipalName
    );

DWORD
NotifyUserLogon(
    PCSTR pszUserName
    );

DWORD
NotifyUserLogoff(
    PCSTR pszUserName
    );

DWORD
GetADUserInfo(
    uid_t uid,
    PLWUTIL_USER_ATTRIBUTES * ppadUserInfo
    );

DWORD
GetAccessCheckData(
    PSTR    pszAdminAllowList,
    PVOID * ppAccessData
    );

DWORD
CheckUserForAccess(
    PCSTR  pszUsername,
    PCVOID pAccessData
    );

DWORD
FreeAccessCheckData(
    PVOID pAccessData
    );


#endif /* __ADUTILS_H__ */
