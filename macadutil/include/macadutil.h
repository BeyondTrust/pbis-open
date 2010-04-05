/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        macadutil.h
 *
 * Abstract:
 *
 *       Mac Workgroup Manager
 * 
 *       AD Utility (Public Header)
 *
 * Author: Glenn Curtis (glennc@likewisesoftware.com)
 *         Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __MACADUTIL_H__
#define __MACADUTIL_H__

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include <lw/types.h>
#include <lw/attrs.h>

#define MACHINE_GROUP_POLICY 1
#define USER_GROUP_POLICY    2
#define UNKNOWN_GROUP_POLICY 3

/* 
 * Logging
 */
typedef enum
{
    MAC_AD_LOG_LEVEL_ALWAYS = 0,
    MAC_AD_LOG_LEVEL_ERROR,
    MAC_AD_LOG_LEVEL_WARNING,
    MAC_AD_LOG_LEVEL_INFO,
    MAC_AD_LOG_LEVEL_VERBOSE
} MacADLogLevel;

typedef VOID (*PFN_MAC_AD_LOG_MESSAGE)(
                    HANDLE      hLog,
                    MacADLogLevel logLevel,
                    PCSTR       pszFormat,
                    va_list     msgList
                    );


#ifndef GPO_DEFINED
#define GPO_DEFINED 1
typedef struct _GROUP_POLICY_OBJECT {
    DWORD dwOptions;    // GPLink options - do we care?
    DWORD dwVersion;    // Version - extract this from the gpt.ini
    PSTR pszPolicyDN;
    PSTR pszDSPath;
    PSTR pszDisplayName;
    PSTR pszgPCFileSysPath;
    PSTR pszgPCMachineExtensionNames;
    PSTR pszgPCUserExtensionNames;
    DWORD gPCFunctionalityVersion;
    DWORD dwFlags;
    BOOLEAN bNewVersion;
    struct _GROUP_POLICY_OBJECT *pNext;
} GROUP_POLICY_OBJECT, *PGROUP_POLICY_OBJECT;
#endif

#ifndef DSATTRIBUTEVALUE_DEFINED
#define DSATTRIBUTEVALUE_DEFINED 1

typedef struct __DSATTRIBUTEVALUE
{
    uint32_t valLen;
    char *   pszValue;
    struct __DSATTRIBUTEVALUE * pNext;
} DSATTRIBUTEVALUE, *PDSATTRIBUTEVALUE;

#endif /* DSATTRIBUTEVALUE_DEFINED */

#ifndef MCXVALUE_DEFINED
#define MCXVALUE_DEFINED 1

typedef struct __MCXVALUE
{
    char *   pValueData;
    int      iValLen;
    struct __MCXVALUE * pNext;
} MCXVALUE, *PMCXVALUE;

#endif /* MCXVALUE_DEFINED */

#ifndef AD_USER_ATTRIBUTES_DEFINED
#define AD_USER_ATTRIBUTES_DEFINED 1

typedef struct __AD_USER_ATTRIBUTES
{
    /* Name attributes */
    PSTR  pszDisplayName;
    PSTR  pszFirstName;
    PSTR  pszLastName;
    PSTR  pszADDomain;
    PSTR  pszKerberosPrincipal;

    /* Email attributes */
    PSTR  pszEMailAddress;
    PSTR  pszMSExchHomeServerName;
    PSTR  pszMSExchHomeMDB;

    /* Phone attributes */
    PSTR  pszTelephoneNumber;
    PSTR  pszFaxTelephoneNumber;
    PSTR  pszMobileTelephoneNumber;

    /* Address attributes */
    PSTR  pszStreetAddress;
    PSTR  pszPostOfficeBox;
    PSTR  pszCity;
    PSTR  pszState;
    PSTR  pszPostalCode;
    PSTR  pszCountry;

    /* Work attributes */
    PSTR  pszTitle;
    PSTR  pszCompany;
    PSTR  pszDepartment;

    /* Network setting attributes */
    PSTR  pszHomeDirectory;
    PSTR  pszHomeDrive;
    PSTR  pszPasswordLastSet;
    PSTR  pszUserAccountControl;
    PSTR  pszMaxMinutesUntilChangePassword;
    PSTR  pszMinMinutesUntilChangePassword;
    PSTR  pszMaxFailedLoginAttempts;
    PSTR  pszAllowedPasswordHistory;
    PSTR  pszMinCharsAllowedInPassword;

} AD_USER_ATTRIBUTES, *PAD_USER_ATTRIBUTES;

#endif /* AD_USER_ATTRIBUTES_DEFINED */

/* 
 * Active Directory helper functions
 * used in DSPlugin
 * for Workgroup Manager features 
 */

typedef LONG (*PFN_GET_AD_DOMAIN)(
                    PSTR* ppszDomain
                    );

typedef LONG (*PFN_ENUM_WORKGROUPMGR_ENABLED_GPOS)(
                    PCSTR pszDomainName,
                    PGROUP_POLICY_OBJECT* ppMCXGPOs
                    );

typedef LONG (*PFN_GET_SPECIFIC_GPO)(
                    PCSTR pszDomainName,
                    PCSTR pszGPOName,
                    PGROUP_POLICY_OBJECT* ppGPO
                    );

typedef LONG (*PFN_IS_MCX_SETTING_ENABLED_FOR_GPO)(
                    PGROUP_POLICY_OBJECT pGPO,
                    DWORD                dwPolicyType,
                    PBOOLEAN             pbEnabled
                    );

typedef LONG (*PFN_CONVERT_DSATTR_VALUES_TO_MCX_VALUES)(
                    PDSATTRIBUTEVALUE pValues,
                    PMCXVALUE*        ppMCXValues
                    );

typedef LONG (*PFN_SAVE_MCX_VALUES_FOR_GPO_SETTING_TYPE)(
                    PMCXVALUE            pMCXValueList,
                    PGROUP_POLICY_OBJECT pGPO,
                    DWORD                dwPolicyType,
                    PCSTR                pszUserUPN
                    );

typedef LONG (*PFN_CONVERT_MCX_SETTINGS_TO_MCX_VALUES)(
                    PCSTR pszPolicyPath,
                    DWORD dwPolicyType,
                    PMCXVALUE* ppMCXValueList
                    );

typedef LONG (*PFN_GET_MCX_VALUES_FOR_GPO_SETTING_TYPE)(
                    PGROUP_POLICY_OBJECT pGPO,
                    DWORD                dwPolicyType,
                    PMCXVALUE*           ppMCXValueList
                    );

typedef LONG (*PFN_LOOKUP_COMPUTER_GROUP_GPO)(
                    PCSTR pszName,
                    PSTR* ppszGPOGUID
                    );

typedef LONG (*PFN_LOOKUP_COMPUTER_LIST_GPO)(
                    PCSTR pszName,
                    PSTR* ppszGPOGUID
                    );

typedef LONG (*PFN_AUTHENTICATE_USER)(
                    PCSTR pszUsername,
                    PCSTR pszPassword
                    );

typedef LONG (*PFN_GET_USER_PRINCIPAL_NAMES)(
                    PCSTR  pszUsername,
                    PSTR * ppszUserPrincipalName,
                    PSTR * ppszUserSamAccount,
                    PSTR * ppszUserDomainFQDN
                    );

typedef void (*PFN_GET_LSA_STATUS)(
                    PBOOLEAN pbIsStarted
                    );

typedef LONG (*PFN_NOTIFY_USER_LOGON)(
                    PCSTR  pszUsername
                    );

typedef LONG (*PFN_NOTIFY_USER_LOGOFF)(
                    PCSTR  pszUsername
                    );

typedef LONG (*PFN_GET_HOME_DIRECTORY_DOCK_MCX_VALUE)(
                    PMCXVALUE * ppMCXValueList
                    );

typedef LONG (*PFN_GET_AD_USER_INFO)(
                    uid_t uid,
                    PAD_USER_ATTRIBUTES * ppadUserInfo
                    );

typedef LONG (*PFN_GET_CONFIGURATION_SETTINGS)(
                    BOOLEAN * pbMergeModeMCX,
                    BOOLEAN * pbEnableForceHomedirOnStartupDisk,
                    BOOLEAN * pbUseADUNCForHomeLocation,
                    PSTR *    ppszUNCProtocolForHomeLocation,
                    PSTR *    ppszAllowAdministrationBy,
                    BOOLEAN * pbMergeAdmins
                    );

typedef LONG (*PFN_GET_ACCESS_CHECK_DATA)(
                    PSTR    pszAdminAllowList,
                    PVOID * ppAccessData
                    );

typedef LONG (*PFN_CHECK_USER_FOR_ACCESS)(
                    PCSTR  pszUsername,
                    PCVOID pAccessData
                    );

typedef LONG (*PFN_FREE_ACCESS_CHECK_DATA)(
                    PVOID pAccessData
                    );

typedef struct __MACADUTIL_FUNC_TABLE
{
    PFN_GET_AD_DOMAIN                        pfnGetADDomain;
    PFN_ENUM_WORKGROUPMGR_ENABLED_GPOS       pfnEnumGPOsEnabledForWorkgroupManager;
    PFN_GET_SPECIFIC_GPO                     pfnGetSpecificGPO;
    PFN_IS_MCX_SETTING_ENABLED_FOR_GPO       pfnIsMCXSettingEnabledForGPO;
    PFN_CONVERT_DSATTR_VALUES_TO_MCX_VALUES  pfnConvertDSAttrValsToMCX;
    PFN_SAVE_MCX_VALUES_FOR_GPO_SETTING_TYPE pfnSaveMCXValuesForGPOSettingType;
    PFN_CONVERT_MCX_SETTINGS_TO_MCX_VALUES   pfnConvertMCXSettingsToValues;
    PFN_GET_MCX_VALUES_FOR_GPO_SETTING_TYPE  pfnGetMCXValuesForGPOSettingType;
    PFN_LOOKUP_COMPUTER_GROUP_GPO            pfnLookupComputerGroupGPO;
    PFN_LOOKUP_COMPUTER_LIST_GPO             pfnLookupComputerListGPO;
    PFN_AUTHENTICATE_USER                    pfnAuthenticateUser;
    PFN_GET_USER_PRINCIPAL_NAMES             pfnGetUserPrincipalNames;
    PFN_GET_LSA_STATUS                       pfnGetLsaStatus;
    PFN_NOTIFY_USER_LOGON                    pfnNotifyUserLogon;
    PFN_NOTIFY_USER_LOGOFF                   pfnNotifyUserLogoff;
    PFN_GET_HOME_DIRECTORY_DOCK_MCX_VALUE    pfnGetHomeDirectoryDockMCXValue;
    PFN_GET_AD_USER_INFO                     pfnGetADUserInfo;
    PFN_GET_CONFIGURATION_SETTINGS           pfnGetConfigurationSettings;
    PFN_GET_ACCESS_CHECK_DATA                pfnGetAccessCheckData;
    PFN_CHECK_USER_FOR_ACCESS                pfnCheckUserForAccess;
    PFN_FREE_ACCESS_CHECK_DATA               pfnFreeAccessCheckData;
    
} MACADUTIL_FUNC_TABLE, *PMACADUTIL_FUNC_TABLE;

#define MACADUTIL_INITIALIZE "LWMCInitialize"
#define MACADUTIL_SHUTDOWN   "LWMCShutdown"

typedef LONG (*PFN_MACADUTIL_INITIALIZE)(
                    HANDLE                 hLog,
                    PFN_MAC_AD_LOG_MESSAGE pfnLogHandler,
                    MacADLogLevel          maxLogLevel,
                    PMACADUTIL_FUNC_TABLE* ppFnTable
                    );

typedef LONG (*PFN_MACADUTIL_SHUTDOWN)(
                    PMACADUTIL_FUNC_TABLE pFnTable
                    );

#endif /* __MACADUTIL_H__ */
