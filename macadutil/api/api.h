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

LONG
LWMCInitialize(
    HANDLE                 hLog,
    PFN_MAC_AD_LOG_MESSAGE pfnLogHandler,
    MacADLogLevel          maxLogLevel,
    PMACADUTIL_FUNC_TABLE* ppFnTable
    );

LONG
LWMCShutdown(
    PMACADUTIL_FUNC_TABLE pFnTable
    );

LONG
GetADDomain(
    PSTR* ppszDomain
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

LONG
IsMCXSettingEnabledForGPO(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PBOOLEAN             pbEnabled
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
    PCSTR pszPassword
    );

LONG
GetUserPrincipalNames(
    PCSTR pszUsername,
    PSTR * ppszUserPrincipalName,
    PSTR * ppszUserSamAccount,
    PSTR * ppszUserDomainFQDN
    );

void
GetLsaStatus(
    PBOOLEAN pbIsStarted
    );

LONG
NotifyUserLogon(
    PCSTR pszUserName
    );

LONG
NotifyUserLogoff(
    PCSTR pszUserName
    );

LONG
GetADUserInfo(
    uid_t uid,
    PAD_USER_ATTRIBUTES * ppadUserInfo
    );

LONG
GetConfigurationSettings(
    BOOLEAN * pbMergeModeMCX,
    BOOLEAN * pbEnableForceHomedirOnStartupDisk,
    BOOLEAN * pbUseADUNCForHomeLocation,
    PSTR *    ppszUNCProtocolForHomeLocation,
    PSTR *    ppszAllowAdministrationBy,
    BOOLEAN * pbMergeAdmins
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


#endif /* __ADUTILS_H__ */
