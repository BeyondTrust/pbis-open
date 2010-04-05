/*
 *  ADUtils.h
 *  LWEDSPlugIn
 *
 *  Created by Glenn Curtis on 7/8/08.
 *  Copyright 2008 Likewise Software. All rights reserved.
 *
 */

#include <LWIStruct.h>

/* Active Directory helper functions used in DSPlugin for Workgroup Manager features */
long
MacADUtilInterface_Initialize(
    void
    );

VOID
MacADUtilInterface_Cleanup(
    void
    );

long ADUAdapter_GetADDomain(
    PSTR* ppszDomain
    );
    
long ADUAdapter_EnumWorkgroupManagerEnabledGPOs(
    PSTR                  pszDomainName,
    PGROUP_POLICY_OBJECT* ppMCXGPOs
    );

long ADUAdapter_GetSpecificGPO(
    PSTR                  pszDomainName,
    PSTR                  pszGPOName,
    PGROUP_POLICY_OBJECT* ppGPO
    );

bool ADUAdapter_IsMCXSettingEnabledForGPO(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType
    );

long ADUAdapter_ConvertDSAttributeValuesToMCXValues(
    PDSATTRIBUTEVALUE    pValues,
    PMCXVALUE *          ppMCXValues
    );

long ADUAdapter_SaveMCXValuesForGPOSettingType(
    PMCXVALUE            pMCXValueList,
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PCSTR                pszUserUPN
    );

long ADUAdapter_ConvertMCXSettingsToMCXValues(
    PSTR  pszPolicyPath,
    DWORD dwPolicyType,
    PMCXVALUE * ppMCXValueList
    );

long ADUAdapter_GetHomeDirectoryDockMCXValue(
    PMCXVALUE * ppMCXValueList
    );
    
long ADUAdapter_GetMCXValuesForGPOSettingType(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PMCXVALUE *          ppMCXValueList
    );

long ADUAdapter_LookupComputerGroupGPO(
    PCSTR  pszName,
    PSTR * ppszGPOGUID
    );

long ADUAdapter_LookupComputerListGPO(
    PCSTR  pszName,
    PSTR * ppszGPOGUID
    );

long
ADUAdapter_AuthenticateUser(
    PCSTR pszUserPrincipal,
    PCSTR pszPassword
    );

long
ADUAdapter_GetUserPrincipalNames(
    PCSTR pszUserName,
    PSTR * pszUserPrincipalName,
    PSTR * pszUserSamAccount,
    PSTR * pszUserDomainFQDN
    );

void
ADUAdapter_GetLsaStatus(
    PBOOLEAN pbIsStarted
    );

long
ADUAdapter_NotifyUserLogon(
    PCSTR pszUserPrincipal
    );

long
ADUAdapter_NotifyUserLogoff(
    PCSTR pszUserPrincipal
    );

long
ADUAdapter_GetUserADInfo(
    uid_t uid,
    PAD_USER_ATTRIBUTES * ppadUserADInfo
    );

void
ADUAdapter_FreeADUserInfo(
	PAD_USER_ATTRIBUTES pUserADInfo
	);

long
ADUAdapter_CopyADUserInfo(
	PAD_USER_ATTRIBUTES   pUserADInfo,
	PAD_USER_ATTRIBUTES * ppUserADInfoCopy
	);

long
ADUAdapter_GetConfigurationSettings(
    BOOLEAN * pbMergeModeMCX,
    BOOLEAN * pbEnableForceHomedirOnStartupDisk,
    BOOLEAN * pbUseADUNCForHomeLocation,
    PSTR *    ppszUNCProtocolForHomeLocation,
    PSTR *    ppszAllowAdministrationBy,
    BOOLEAN * pbMergeAdmins
    );

long
ADUAdapter_GetAccessCheckData(
    PSTR    pszAdminAllowList,
    PVOID * ppAccessData
    );

long
ADUAdapter_CheckUserForAccess(
    PCSTR  pszUsername,
    PCVOID pAccessData
    );

long
ADUAdapter_FreeAccessCheckData(
    PVOID pAccessData
    );

