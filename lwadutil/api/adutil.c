#include "includes.h"

DWORD
GetCurrentSettingsForGPO(
    PGROUP_POLICY_OBJECT pGPO,
    PSTR 	  pszClientGUID,
    BOOLEAN * pfMachinePolicyExists,
    BOOLEAN * pfUserPolicyExists,
    PSTR *    ppszMachinePolicyPath,
    PSTR *    ppszUserPolicyPath,
    PSTR      pszPath
    )
{
    DWORD dwError = 0;
    PSTR pszUserDestFolder = NULL;
    PSTR pszMachineDestFolder = NULL;
    BOOLEAN fUserPolicyExists = FALSE;
    BOOLEAN fMachinePolicyExists = FALSE;
    BOOLEAN bCurrent = FALSE;
    BOOLEAN bEnabled = FALSE;
    DWORD dwVersion = 0;
    DWORD dwFileVersion = 0;
    PGROUP_POLICY_OBJECT pRecentGPO = NULL;

    BAIL_ON_INVALID_POINTER(pGPO);
    BAIL_ON_INVALID_POINTER(pszClientGUID);
    BAIL_ON_INVALID_POINTER(pfMachinePolicyExists);
    BAIL_ON_INVALID_POINTER(pfUserPolicyExists);
    BAIL_ON_INVALID_POINTER(ppszMachinePolicyPath);
    BAIL_ON_INVALID_POINTER(ppszUserPolicyPath);

    dwError = GetSpecificGPO(NULL, pGPO->pszDisplayName, &pRecentGPO);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = IsCacheDataCurrentForGPO(
                    pRecentGPO,
                    &dwVersion,
                    &dwFileVersion,
                    &bCurrent,
                    pszPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    //Fix for bug#8856. Donot consider bcurrent

//    if (!bCurrent)
 //   {
    LWUTIL_LOG_INFO("Going to collect GPO payload and store in the cache location");

    IsSettingEnabledForGPO(pRecentGPO, MACHINE_GROUP_POLICY, pszClientGUID,&bEnabled);

    if (bEnabled)
    {
        dwError = ADUGetPolicyFiles(
                     MACHINE_GROUP_POLICY,
                     pRecentGPO->pszgPCFileSysPath,
                     pszClientGUID,
                     pszPath,
                     &pszMachineDestFolder,
                     &fMachinePolicyExists);
        if (dwError)
        {
            LWUTIL_LOG_ERROR("Failed to get machine gp settings for GPO, error %d", dwError);
            dwError = 0;
        }
    }

    IsSettingEnabledForGPO(pRecentGPO, USER_GROUP_POLICY, pszClientGUID,&bEnabled);
    if (bEnabled)
    {
        dwError = ADUGetPolicyFiles(
                     USER_GROUP_POLICY,
                     pRecentGPO->pszgPCFileSysPath,
                     pszClientGUID,
                     pszPath,
                     &pszUserDestFolder,
                     &fUserPolicyExists);
        if (dwError)
        {
            LWUTIL_LOG_ERROR("Failed to get user gp settings for GPO, error %d", dwError);
            dwError = 0;
        }
    }
#if 0
    }
    else
    {
        IsSettingEnabledForGPO(pRecentGPO, MACHINE_GROUP_POLICY, pszClientGUID,&bEnabled);
        if (bEnabled)
        {
            dwError = GetCachedPolicyFiles(
                        MACHINE_GROUP_POLICY,
                        pRecentGPO->pszgPCFileSysPath,
                        pszClientGUID,
                        pszPath,
                        &pszMachineDestFolder,
                        &fMachinePolicyExists);
            if (dwError)
            {
                LWUTIL_LOG_ERROR("Failed to get machine gp settings for GPO, error %d", dwError);
                dwError = 0;
            }
        }

        IsSettingEnabledForGPO(pRecentGPO, USER_GROUP_POLICY, pszClientGUID,&bEnabled);
        if (bEnabled)
        {
            dwError = GetCachedPolicyFiles(
                        USER_GROUP_POLICY,
                        pRecentGPO->pszgPCFileSysPath,
                        pszClientGUID,
                        pszPath,
                        &pszUserDestFolder,
                        &fUserPolicyExists);
            if (dwError)
            {
                LWUTIL_LOG_ERROR("Failed to get user gp settings for GPO, error %d", dwError);
                dwError = 0;
            }
        }
    }
#endif

    *pfMachinePolicyExists = fMachinePolicyExists;
    *pfUserPolicyExists = fUserPolicyExists;
    *ppszMachinePolicyPath = pszMachineDestFolder;
    pszMachineDestFolder = NULL;
    *ppszUserPolicyPath = pszUserDestFolder;
    pszUserDestFolder = NULL;

cleanup:

    LW_SAFE_FREE_STRING(pszMachineDestFolder);
    LW_SAFE_FREE_STRING(pszUserDestFolder);
    ADU_SAFE_FREE_GPO_LIST(pRecentGPO);

    return dwError;

error:

    if (pfMachinePolicyExists)
    {
        *pfMachinePolicyExists = FALSE;
    }

    if (pfUserPolicyExists)
    {
        *pfUserPolicyExists = FALSE;
    }

    if (ppszMachinePolicyPath)
    {
        *ppszMachinePolicyPath = NULL;
    }

    if (ppszUserPolicyPath)
    {
        *ppszUserPolicyPath = NULL;
    }

    goto cleanup;
}

