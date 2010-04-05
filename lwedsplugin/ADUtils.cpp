/*
 *  ADUtils.cpp
 *  LWEDSPlugIn
 *
 *  Created by Glenn Curtis on 7/8/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "LWIPlugIn.h"

// #define DEMO_MODE 1

typedef struct __MACADUTIL_INTERFACE
{
    PVOID pLibHandle;
    PMACADUTIL_FUNC_TABLE  pFuncTable;
    PFN_MACADUTIL_SHUTDOWN pfnShutdownMacADUtil;
} MACADUTIL_INTERFACE, *PMACADUTIL_INTERFACE;

static PMACADUTIL_INTERFACE gpMacADUtilItf = NULL;

static
long
ValidateMacADUtilInterface(
    PMACADUTIL_FUNC_TABLE pFuncTable
    )
{
    long macError = eDSNoErr;

    if (!pFuncTable ||
        !pFuncTable->pfnGetADDomain ||
        !pFuncTable->pfnEnumGPOsEnabledForWorkgroupManager ||
        !pFuncTable->pfnGetSpecificGPO ||
        !pFuncTable->pfnIsMCXSettingEnabledForGPO ||
        !pFuncTable->pfnConvertDSAttrValsToMCX ||
        !pFuncTable->pfnSaveMCXValuesForGPOSettingType ||
        !pFuncTable->pfnConvertMCXSettingsToValues ||
        !pFuncTable->pfnGetHomeDirectoryDockMCXValue ||
        !pFuncTable->pfnGetMCXValuesForGPOSettingType ||
        !pFuncTable->pfnLookupComputerGroupGPO ||
        !pFuncTable->pfnLookupComputerListGPO ||
        !pFuncTable->pfnAuthenticateUser ||
        !pFuncTable->pfnGetUserPrincipalNames ||
        !pFuncTable->pfnGetLsaStatus ||
        !pFuncTable->pfnNotifyUserLogon ||
        !pFuncTable->pfnNotifyUserLogoff ||
        !pFuncTable->pfnGetADUserInfo ||
        !pFuncTable->pfnGetConfigurationSettings ||
        !pFuncTable->pfnGetAccessCheckData ||
        !pFuncTable->pfnCheckUserForAccess ||
        !pFuncTable->pfnFreeAccessCheckData)
    {
       macError = ePlugInFailedToInitialize;
    }

    return macError;
}

#ifdef __cplusplus
extern "C"
#endif
VOID
LoggerCallback(
    HANDLE      hLog,
    MacADLogLevel logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    )
{
    LogMessageV(pszFormat, msgList);
}

long
MacADUtilInterface_Initialize(
    void
    )
{
    long macError = eDSNoErr;
    PMACADUTIL_INTERFACE pMacADUtilItf = NULL;
    PVOID pLibHandle = NULL;
    PCSTR pszError = NULL;
    CHAR szMacADUtilLibPath[PATH_MAX+1];
    PMACADUTIL_FUNC_TABLE  pFuncTable = NULL;
    PFN_MACADUTIL_INITIALIZE pfnInitMacADUtil = NULL;
    PFN_MACADUTIL_SHUTDOWN   pfnShutdownMacADUtil = NULL;

    LOG_ENTER("");

    /* Are we already initialized */
    if (gpMacADUtilItf)
    {
        LOG("libmacadutil library already initialized");
        GOTO_CLEANUP();
    }

    sprintf(szMacADUtilLibPath, "/opt/likewise/lib/libmacadutil.so");

    dlerror();
    pLibHandle = dlopen(szMacADUtilLibPath, RTLD_NOW | RTLD_GLOBAL);
    if (pLibHandle == NULL) {
        pszError = dlerror();
        LOG("Error: Failed to load Likewise libmacadutil Module [%s]",
            IsNullOrEmptyString(pszError) ? "" : pszError);
        macError = ePlugInFailedToInitialize;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    dlerror();
    pfnInitMacADUtil = (PFN_MACADUTIL_INITIALIZE)dlsym(
                                pLibHandle,
                                MACADUTIL_INITIALIZE);
    if (pfnInitMacADUtil == NULL)
    {
        pszError = dlerror();
        LOG("Error: Failed to lookup symbol %s in libmacadutil Module [%s]",
            MACADUTIL_INITIALIZE,
            IsNullOrEmptyString(pszError) ? "" : pszError);
        macError = ePlugInFailedToInitialize;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    dlerror();
    pfnShutdownMacADUtil = (PFN_MACADUTIL_SHUTDOWN)dlsym(
                                pLibHandle,
                                MACADUTIL_SHUTDOWN);
    if (pfnShutdownMacADUtil == NULL)
    {
        pszError = dlerror();
        LOG("Error: Failed to lookup symbol %s in libmacadutil Module [%s]",
            MACADUTIL_SHUTDOWN,
            IsNullOrEmptyString(pszError) ? "" : pszError);
        macError = ePlugInFailedToInitialize;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = pfnInitMacADUtil((HANDLE)NULL, &LoggerCallback, MAC_AD_LOG_LEVEL_VERBOSE, &pFuncTable);
    GOTO_CLEANUP_ON_MACERROR(macError);

    LOG("Init call for libmacadutil completed");

    macError = ValidateMacADUtilInterface(pFuncTable);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWAllocateMemory(sizeof(MACADUTIL_INTERFACE), (PVOID*)&pMacADUtilItf);
    GOTO_CLEANUP_ON_MACERROR(macError);

    LOG("validate function table completed");

    pMacADUtilItf->pFuncTable = pFuncTable;
    pMacADUtilItf->pLibHandle = pLibHandle;
    pLibHandle = NULL;
    pMacADUtilItf->pfnShutdownMacADUtil = pfnShutdownMacADUtil;
    pfnShutdownMacADUtil = NULL;
    gpMacADUtilItf = pMacADUtilItf;

    LOG("libmacadutil library loaded successfully");
    macError = eDSNoErr;

cleanup:
    if (macError)
    {
        MacADUtilInterface_Cleanup();
    }

    if (pfnShutdownMacADUtil)
    {
        pfnShutdownMacADUtil(pFuncTable);
    }

    if (pLibHandle)
    {
        dlclose(pLibHandle);
    }

    LOG_LEAVE("--> %d", macError);

    return macError;
}

VOID
MacADUtilInterface_Cleanup(
    void
    )
{
    PMACADUTIL_INTERFACE pMacADUtilItf = gpMacADUtilItf;
    if (pMacADUtilItf)
    {
        if (pMacADUtilItf->pfnShutdownMacADUtil)
        {
            pMacADUtilItf->pfnShutdownMacADUtil(pMacADUtilItf->pFuncTable);
        }
        if (pMacADUtilItf->pLibHandle)
        {
            dlclose(pMacADUtilItf->pLibHandle);
        }
        LWFreeMemory(pMacADUtilItf);
    }
    gpMacADUtilItf = NULL;
}

long ADUAdapter_GetADDomain(
    PSTR* ppszDomain
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnGetADDomain(ppszDomain);
    else
    {
        *ppszDomain = NULL;
        return ePlugInInitError;
    }
}

long ADUAdapter_EnumWorkgroupManagerEnabledGPOs(
    PSTR                  pszDomainName,
    PGROUP_POLICY_OBJECT* ppMCXGPOs
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnEnumGPOsEnabledForWorkgroupManager(pszDomainName, ppMCXGPOs);
    else
        return ePlugInInitError;
}

long ADUAdapter_GetSpecificGPO(
    PSTR                  pszDomainName,
    PSTR                  pszGPOName,
    PGROUP_POLICY_OBJECT* ppGPO
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnGetSpecificGPO(pszDomainName, pszGPOName, ppGPO);
    else
        return ePlugInInitError;
}

bool ADUAdapter_IsMCXSettingEnabledForGPO(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType
    )
{
    long macError = eDSNoErr;
    BOOLEAN bEnabled = FALSE;

#if DEMO_MODE_1
    return true;
#else	
    if (gpMacADUtilItf)
    {
        macError = gpMacADUtilItf->pFuncTable->pfnIsMCXSettingEnabledForGPO(pGPO, dwPolicyType, &bEnabled);
        if (macError)
            return false;
        else
            return (bool)bEnabled;
    }
    else
        return false;
#endif
}

long ADUAdapter_ConvertDSAttributeValuesToMCXValues(
    PDSATTRIBUTEVALUE    pValues,
    PMCXVALUE *          ppMCXValues
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnConvertDSAttrValsToMCX(pValues, ppMCXValues);
    else
        return ePlugInInitError;
}

long ADUAdapter_SaveMCXValuesForGPOSettingType(
    PMCXVALUE            pMCXValueList,
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PCSTR                pszUserUPN
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnSaveMCXValuesForGPOSettingType(pMCXValueList, pGPO, dwPolicyType, pszUserUPN);
    else
        return ePlugInInitError;
}

long ADUAdapter_ConvertMCXSettingsToMCXValues(
    PSTR  pszPolicyPath,
    DWORD dwPolicyType,
    PMCXVALUE * ppMCXValueList
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnConvertMCXSettingsToValues(pszPolicyPath, dwPolicyType, ppMCXValueList);
    else
        return ePlugInInitError;
}

long ADUAdapter_GetHomeDirectoryDockMCXValue(
    PMCXVALUE * ppMCXValueList
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnGetHomeDirectoryDockMCXValue(ppMCXValueList);
    else
        return ePlugInInitError;
}

long ADUAdapter_GetMCXValuesForGPOSettingType(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PMCXVALUE *          ppMCXValueList
    )
{
#if DEMO_MODE_2
    return eDSNoErr;
#else
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnGetMCXValuesForGPOSettingType(pGPO, dwPolicyType, ppMCXValueList);
    else
        return ePlugInInitError;
#endif
}

long ADUAdapter_LookupComputerGroupGPO(
    PCSTR  pszName,
    PSTR * ppszGPOGUID
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnLookupComputerGroupGPO(pszName, ppszGPOGUID);
    else
        return ePlugInInitError;
}

long ADUAdapter_LookupComputerListGPO(
    PCSTR  pszName,
    PSTR * ppszGPOGUID
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnLookupComputerListGPO(pszName, ppszGPOGUID);
    else
        return ePlugInInitError;
}

long
ADUAdapter_AuthenticateUser(
    PCSTR pszUserPrincipal,
    PCSTR pszPassword
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnAuthenticateUser(pszUserPrincipal, pszPassword);
    else
        return ePlugInInitError;
}

long
ADUAdapter_GetUserPrincipalNames(
    PCSTR  pszUserName,
    PSTR * ppszUserPrincipalName,
    PSTR * ppszUserSamAccount,
    PSTR * ppszUserDomainFQDN
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnGetUserPrincipalNames(pszUserName,
                                                                    ppszUserPrincipalName,
                                                                    ppszUserSamAccount,
                                                                    ppszUserDomainFQDN);
    else
        return ePlugInInitError;
}

void
ADUAdapter_GetLsaStatus(
    PBOOLEAN pbIsStarted
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnGetLsaStatus(pbIsStarted);
    else
    {
        *pbIsStarted = FALSE;
    }
}

long
ADUAdapter_NotifyUserLogon(
    PCSTR pszUserPrincipal
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnNotifyUserLogon(pszUserPrincipal);
    else
        return ePlugInInitError;
}

long
ADUAdapter_NotifyUserLogoff(
    PCSTR pszUserPrincipal
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnNotifyUserLogoff(pszUserPrincipal);
    else
        return ePlugInInitError;
}

long
ADUAdapter_GetUserADInfo(
    uid_t uid,
    PAD_USER_ATTRIBUTES * ppadUserADInfo
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnGetADUserInfo(uid, ppadUserADInfo);
    else
        return ePlugInInitError;
}

void
ADUAdapter_FreeADUserInfo(
    PAD_USER_ATTRIBUTES pUserADInfo
	)
{
    if (pUserADInfo)
    {
        if (pUserADInfo->pszDisplayName)
            LWFreeString(pUserADInfo->pszDisplayName);
		
        if (pUserADInfo->pszFirstName)
            LWFreeString(pUserADInfo->pszFirstName);
		
        if (pUserADInfo->pszLastName)
            LWFreeString(pUserADInfo->pszLastName);
		
        if (pUserADInfo->pszADDomain)
            LWFreeString(pUserADInfo->pszADDomain);
		
        if (pUserADInfo->pszKerberosPrincipal)
            LWFreeString(pUserADInfo->pszKerberosPrincipal);
		
        if (pUserADInfo->pszEMailAddress)
            LWFreeString(pUserADInfo->pszEMailAddress);
		
        if (pUserADInfo->pszMSExchHomeServerName)
            LWFreeString(pUserADInfo->pszMSExchHomeServerName);
		
        if (pUserADInfo->pszMSExchHomeMDB)
            LWFreeString(pUserADInfo->pszMSExchHomeMDB);
		
        if (pUserADInfo->pszTelephoneNumber)
            LWFreeString(pUserADInfo->pszTelephoneNumber);
		
        if (pUserADInfo->pszFaxTelephoneNumber)
            LWFreeString(pUserADInfo->pszFaxTelephoneNumber);
		
        if (pUserADInfo->pszMobileTelephoneNumber)
            LWFreeString(pUserADInfo->pszMobileTelephoneNumber);
		
        if (pUserADInfo->pszStreetAddress)
            LWFreeString(pUserADInfo->pszStreetAddress);
		
        if (pUserADInfo->pszPostOfficeBox)
            LWFreeString(pUserADInfo->pszPostOfficeBox);
		
        if (pUserADInfo->pszCity)
            LWFreeString(pUserADInfo->pszCity);
		
        if (pUserADInfo->pszState)
            LWFreeString(pUserADInfo->pszState);
		
        if (pUserADInfo->pszPostalCode)
            LWFreeString(pUserADInfo->pszPostalCode);
		
        if (pUserADInfo->pszCountry)
            LWFreeString(pUserADInfo->pszCountry);
		
        if (pUserADInfo->pszTitle)
            LWFreeString(pUserADInfo->pszTitle);
		
        if (pUserADInfo->pszCompany)
            LWFreeString(pUserADInfo->pszCompany);
		
        if (pUserADInfo->pszDepartment)
            LWFreeString(pUserADInfo->pszDepartment);
		
        if (pUserADInfo->pszHomeDirectory)
            LWFreeString(pUserADInfo->pszHomeDirectory);
		
        if (pUserADInfo->pszHomeDrive)
            LWFreeString(pUserADInfo->pszHomeDrive);
		
        if (pUserADInfo->pszPasswordLastSet)
            LWFreeString(pUserADInfo->pszPasswordLastSet);
		
        if (pUserADInfo->pszUserAccountControl)
            LWFreeString(pUserADInfo->pszUserAccountControl);

        if (pUserADInfo->pszMaxMinutesUntilChangePassword)
            LWFreeString(pUserADInfo->pszMaxMinutesUntilChangePassword);

        if (pUserADInfo->pszMinMinutesUntilChangePassword)
            LWFreeString(pUserADInfo->pszMinMinutesUntilChangePassword);
		
        if (pUserADInfo->pszMaxFailedLoginAttempts)
            LWFreeString(pUserADInfo->pszMaxFailedLoginAttempts);
		
        if (pUserADInfo->pszAllowedPasswordHistory)
            LWFreeString(pUserADInfo->pszAllowedPasswordHistory);
		
        if (pUserADInfo->pszMinCharsAllowedInPassword)
            LWFreeString(pUserADInfo->pszMinCharsAllowedInPassword);
		
        LWFreeMemory(pUserADInfo);
    }
}

long
ADUAdapter_CopyADUserInfo(
	PAD_USER_ATTRIBUTES pUserADInfo,
	PAD_USER_ATTRIBUTES * ppUserADInfoCopy
	)
{
    LONG macError = eDSNoErr;
    PAD_USER_ATTRIBUTES pNew = NULL;
	
    if (pUserADInfo)
    {
	macError = LWAllocateMemory(sizeof(AD_USER_ATTRIBUTES), (PVOID *) &pNew);
	GOTO_CLEANUP_ON_MACERROR(macError);
		
        if (pUserADInfo->pszDisplayName)
        {
            macError = LWAllocateString(pUserADInfo->pszDisplayName, &pNew->pszDisplayName);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
		
        if (pUserADInfo->pszFirstName)
        {
            macError = LWAllocateString(pUserADInfo->pszFirstName, &pNew->pszFirstName);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
		
        if (pUserADInfo->pszLastName)
        {
            macError = LWAllocateString(pUserADInfo->pszLastName, &pNew->pszLastName);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
		
        if (pUserADInfo->pszADDomain)
        {
            macError = LWAllocateString(pUserADInfo->pszADDomain, &pNew->pszADDomain);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
            
        if (pUserADInfo->pszKerberosPrincipal)
        {
            macError = LWAllocateString(pUserADInfo->pszKerberosPrincipal, &pNew->pszKerberosPrincipal);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
            
        if (pUserADInfo->pszEMailAddress)
        {
            macError = LWAllocateString(pUserADInfo->pszEMailAddress, &pNew->pszEMailAddress);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
            
        if (pUserADInfo->pszMSExchHomeServerName)
        {
            macError = LWAllocateString(pUserADInfo->pszMSExchHomeServerName, &pNew->pszMSExchHomeServerName);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
            
        if (pUserADInfo->pszMSExchHomeMDB)
        {
            macError = LWAllocateString(pUserADInfo->pszMSExchHomeMDB, &pNew->pszMSExchHomeMDB);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
            
        if (pUserADInfo->pszTelephoneNumber)
        {
            macError = LWAllocateString(pUserADInfo->pszTelephoneNumber, &pNew->pszTelephoneNumber);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
            
        if (pUserADInfo->pszFaxTelephoneNumber)
        {
            macError = LWAllocateString(pUserADInfo->pszFaxTelephoneNumber, &pNew->pszFaxTelephoneNumber);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        
        if (pUserADInfo->pszMobileTelephoneNumber)
        {
            macError = LWAllocateString(pUserADInfo->pszMobileTelephoneNumber, &pNew->pszMobileTelephoneNumber);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszStreetAddress)
        {
            macError = LWAllocateString(pUserADInfo->pszStreetAddress, &pNew->pszStreetAddress);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszPostOfficeBox)
        {
            macError = LWAllocateString(pUserADInfo->pszPostOfficeBox, &pNew->pszPostOfficeBox);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszCity)
        {
            macError = LWAllocateString(pUserADInfo->pszCity, &pNew->pszCity);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszState)
        {
            macError = LWAllocateString(pUserADInfo->pszState, &pNew->pszState);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszPostalCode)
        {
            macError = LWAllocateString(pUserADInfo->pszPostalCode, &pNew->pszPostalCode);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszCountry)
        {
            macError = LWAllocateString(pUserADInfo->pszCountry, &pNew->pszCountry);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszTitle)
        {
            macError = LWAllocateString(pUserADInfo->pszTitle, &pNew->pszTitle);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszCompany)
        {
            macError = LWAllocateString(pUserADInfo->pszCompany, &pNew->pszCompany);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszDepartment)
        {
            macError = LWAllocateString(pUserADInfo->pszDepartment, &pNew->pszDepartment);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszHomeDirectory)
        {
            macError = LWAllocateString(pUserADInfo->pszHomeDirectory, &pNew->pszHomeDirectory);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszHomeDrive)
        {
            macError = LWAllocateString(pUserADInfo->pszHomeDrive, &pNew->pszHomeDrive);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszPasswordLastSet)
        {
            macError = LWAllocateString(pUserADInfo->pszPasswordLastSet, &pNew->pszPasswordLastSet);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszUserAccountControl)
        {
            macError = LWAllocateString(pUserADInfo->pszUserAccountControl, &pNew->pszUserAccountControl);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
 
        if (pUserADInfo->pszMaxMinutesUntilChangePassword)
        {
            macError = LWAllocateString(pUserADInfo->pszMaxMinutesUntilChangePassword, &pNew->pszMaxMinutesUntilChangePassword);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
 
        if (pUserADInfo->pszMinMinutesUntilChangePassword)
        {
            macError = LWAllocateString(pUserADInfo->pszMinMinutesUntilChangePassword, &pNew->pszMinMinutesUntilChangePassword);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszMaxFailedLoginAttempts)
        {
            macError = LWAllocateString(pUserADInfo->pszMaxFailedLoginAttempts, &pNew->pszMaxFailedLoginAttempts);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszAllowedPasswordHistory)
        {
            macError = LWAllocateString(pUserADInfo->pszAllowedPasswordHistory, &pNew->pszAllowedPasswordHistory);
            GOTO_CLEANUP_ON_MACERROR(macError);
	}
        
        if (pUserADInfo->pszMinCharsAllowedInPassword)
        {
            macError = LWAllocateString(pUserADInfo->pszMinCharsAllowedInPassword, &pNew->pszMinCharsAllowedInPassword);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }
    
    *ppUserADInfoCopy = pNew;
    pNew = NULL;

cleanup:
	
	if (pNew)
	{
	    ADUAdapter_FreeADUserInfo(pNew);
	}
    
    return macError;
}

long 
ADUAdapter_GetConfigurationSettings(
    BOOLEAN * pbMergeModeMCX,
    BOOLEAN * pbEnableForceHomedirOnStartupDisk,
    BOOLEAN * pbUseADUNCForHomeLocation,
    PSTR *    ppszUNCProtocolForHomeLocation,
    PSTR *    ppszAllowAdministrationBy,
    BOOLEAN * pbMergeAdmins
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnGetConfigurationSettings(pbMergeModeMCX,
                                                                       pbEnableForceHomedirOnStartupDisk,
                                                                       pbUseADUNCForHomeLocation,
                                                                       ppszUNCProtocolForHomeLocation,
                                                                       ppszAllowAdministrationBy,
                                                                       pbMergeAdmins);
    else
        return ePlugInInitError;
}

long
ADUAdapter_GetAccessCheckData(
    PSTR    pszAdminAllowList,
    PVOID * ppAccessData
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnGetAccessCheckData(pszAdminAllowList,
                                                                 ppAccessData);
    else
        return ePlugInInitError;
}

long
ADUAdapter_CheckUserForAccess(
    PCSTR  pszUsername,
    PCVOID pAccessData
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnCheckUserForAccess(pszUsername,
                                                                 pAccessData);
    else
        return ePlugInInitError;
}

long
ADUAdapter_FreeAccessCheckData(
    PVOID pAccessData
    )
{
    if (gpMacADUtilItf)
        return gpMacADUtilItf->pFuncTable->pfnFreeAccessCheckData(pAccessData);
    else
        return ePlugInInitError;
}


