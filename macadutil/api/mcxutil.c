#include "includes.h"

void
FreeMCXValueList(
    PMCXVALUE pValueList
	)
{
    while (pValueList)
    {
        PMCXVALUE pTemp = pValueList;

        pValueList = pValueList->pNext;

        if (pTemp->pValueData)
            LWFreeMemory(pTemp->pValueData);

        LWFreeMemory(pTemp);
    }
}

DWORD
ReadMCXValueFromFile(
    PSTR pszMCXValueFile,
    PMCXVALUE * ppMCXValue
    )
{
    DWORD dwError = 0;
    BOOLEAN bFileExists = FALSE;
    FILE * fp = NULL;
    PMCXVALUE pNew = NULL;

    BAIL_ON_INVALID_POINTER(ppMCXValue);

    MAC_AD_LOG_INFO("Checking file (%s) to see if it exists and has MCX settings", pszMCXValueFile);

    dwError = LWCheckFileExists(
                    pszMCXValueFile,
                    &bFileExists);
    BAIL_ON_MAC_ERROR(dwError);

    if (!bFileExists)
    {
        MAC_AD_LOG_INFO("File (%s) does not exist, this can be normal if the GPO has no more settings", pszMCXValueFile);
        dwError = MAC_AD_ERROR_INVALID_NAME;
        BAIL_ON_MAC_ERROR_NO_LOG(dwError);
    }

    dwError = LWAllocateMemory(sizeof(MCXVALUE), (PVOID*)&pNew);
    BAIL_ON_MAC_ERROR(dwError);

    fp = fopen(pszMCXValueFile, "r");
    if (!fp)
    {
        MAC_AD_LOG_INFO("File (%s) could not be opened, this is unexpected since the file was just verified to exist", pszMCXValueFile);
        dwError = MAC_AD_ERROR_INVALID_NAME;
        BAIL_ON_MAC_ERROR(dwError);
    }

    (void) fseek(fp, 0, SEEK_END);
    pNew->iValLen = ftell(fp);
    rewind(fp);

    dwError = LWAllocateMemory(
                    pNew->iValLen,
                    (PVOID*) &pNew->pValueData);
    BAIL_ON_MAC_ERROR(dwError);

    (void) fread(pNew->pValueData, sizeof(char), pNew->iValLen, fp);

    *ppMCXValue = pNew;

cleanup:

    if (fp)
    {
        fclose(fp);
    }

    return dwError;

error:

    if (pNew)
    {
        FreeMCXValueList(pNew);
    }

    if (ppMCXValue)
    {
        *ppMCXValue = NULL;
    }

    goto cleanup;
}

static
DWORD
WriteMCXValueToFile(
    PMCXVALUE pMCXValue,
    PSTR      pszMCXValueFile
    )
{
    DWORD dwError = 0;
    FILE * fp = NULL;
    int len = 0;

    fp = fopen(pszMCXValueFile, "w");
    if (fp)
    {
        len = fwrite(pMCXValue->pValueData, sizeof(char), pMCXValue->iValLen, fp);
        if (len < pMCXValue->iValLen)
        {
            dwError = ENOMEM;
            BAIL_ON_MAC_ERROR(dwError);
        }
    }

cleanup:

    if (fp)
    {
        fclose(fp);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
ConvertMCXValuesToMCXSettings(
    PMCXVALUE pMCXValueList,
    PSTR      pszPolicyPath,
    DWORD     dwPolicyType
    )
{
    DWORD dwError = 0;
    PSTR pszMCXValueFile = NULL;
    PSTR pszMCXFile = NULL;
    int iIter = 0;
    FILE * fp = NULL;

    dwError = GetFileNameForMCXSettings(
                    dwPolicyType,
                    &pszMCXFile);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWAllocateStringPrintf(
                    &pszMCXValueFile,
                    "%s/%s.mcx",
                    pszPolicyPath,
                    pszMCXFile);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWCreateDirectory(
                    pszPolicyPath,
                    S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BAIL_ON_MAC_ERROR(dwError);

    fp = fopen(pszMCXValueFile, "w");
    if (!fp)
    {
        dwError = ENOMEM;
        BAIL_ON_MAC_ERROR(dwError);
    }

    while (pMCXValueList)
    {
        LW_SAFE_FREE_STRING(pszMCXValueFile);

        dwError = LWAllocateStringPrintf(
                        &pszMCXValueFile,
                        "%s/%s-%d.mcx",
                        pszPolicyPath,
                        pszMCXFile,
                        iIter+1);
        BAIL_ON_MAC_ERROR(dwError);

        dwError = WriteMCXValueToFile(
                        pMCXValueList,
                        pszMCXValueFile);
        BAIL_ON_MAC_ERROR(dwError);

        LW_SAFE_FREE_STRING(pszMCXValueFile);

        dwError = LWAllocateStringPrintf(
                        &pszMCXValueFile,
                        "%s-%d.mcx\n",
                        pszMCXFile,
                        iIter+1);
        BAIL_ON_MAC_ERROR(dwError);

        if (fputs(pszMCXValueFile, fp) == EOF)
        {
            dwError = ENOMEM;
            BAIL_ON_MAC_ERROR(dwError);
        }

        pMCXValueList = pMCXValueList->pNext;

        iIter++;
    }

cleanup:

    if (fp)
    {
        fclose(fp);
    }

    LW_SAFE_FREE_STRING(pszMCXValueFile);
    LW_SAFE_FREE_STRING(pszMCXFile);

    return dwError;

error:

    goto cleanup;
}

DWORD
GetFileNameForMCXSettings(
    DWORD  dwPolicyType,
    PSTR * ppszFileName
    )
{
    DWORD dwError = 0;
    PSTR pszFileName = NULL;

    switch(dwPolicyType)
    {
        case MACHINE_GROUP_POLICY:

            dwError = LWAllocateString(
                            "groupofcomputers",
                            &pszFileName);
            BAIL_ON_MAC_ERROR(dwError);

            break;

        case USER_GROUP_POLICY:

            dwError = LWAllocateString(
                            "groupofusers",
                            &pszFileName);
            BAIL_ON_MAC_ERROR(dwError);

            break;

        default:

           dwError = MAC_AD_ERROR_INVALID_PARAMETER;
           BAIL_ON_MAC_ERROR(dwError);
    }

    *ppszFileName = pszFileName;
    pszFileName = NULL;

cleanup:

    LW_SAFE_FREE_STRING(pszFileName);

    return dwError;

error:

    *ppszFileName = NULL;

    goto cleanup;
}

DWORD
GetCurrentMCXSettingsForGPO(
    PGROUP_POLICY_OBJECT pGPO,
    BOOLEAN * pfMachinePolicyExists,
    BOOLEAN * pfUserPolicyExists,
    PSTR *    ppszMachinePolicyPath,
    PSTR *    ppszUserPolicyPath
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
    BAIL_ON_INVALID_POINTER(pfMachinePolicyExists);
    BAIL_ON_INVALID_POINTER(pfUserPolicyExists);
    BAIL_ON_INVALID_POINTER(ppszMachinePolicyPath);
    BAIL_ON_INVALID_POINTER(ppszUserPolicyPath);

    dwError = GetSpecificGPO_authenticated(
                    NULL,
                    pGPO->pszDisplayName,
                    &pRecentGPO);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = IsCacheDataCurrentForGPO(
                    pRecentGPO,
                    &dwVersion,
                    &dwFileVersion,
                    &bCurrent);
    BAIL_ON_MAC_ERROR(dwError);

    if (!bCurrent)
    {
        MAC_AD_LOG_INFO("Going to collect GPO payload and store in the cache location");
        IsMCXSettingEnabledForGPO(pRecentGPO, MACHINE_GROUP_POLICY, &bEnabled);
        if (bEnabled)
        {
            dwError = ADUGetPolicyFiles(
                         MACHINE_GROUP_POLICY,
                         pRecentGPO->pszgPCFileSysPath,
                         (PSTR) COMPUTER_MCX_CSE_GUID,
                         NULL,
                         &pszMachineDestFolder,
                         &fMachinePolicyExists);
            if (dwError)
            {
                MAC_AD_LOG_ERROR("Failed to get machine MCX settings for GPO, error %d", dwError);
                dwError = 0;
            }
        }

        IsMCXSettingEnabledForGPO(pRecentGPO, USER_GROUP_POLICY, &bEnabled);
        if (bEnabled)
        {
            dwError = ADUGetPolicyFiles(
                         USER_GROUP_POLICY,
                         pRecentGPO->pszgPCFileSysPath,
                         (PSTR) USER_MCX_CSE_GUID,
                         NULL,
                         &pszUserDestFolder,
                         &fUserPolicyExists);
            if (dwError)
            {
                MAC_AD_LOG_ERROR("Failed to get user MCX settings for GPO, error %d", dwError);
                dwError = 0;
            }
        }
    }
    else
    {
        IsMCXSettingEnabledForGPO(pRecentGPO, MACHINE_GROUP_POLICY, &bEnabled);
        if (bEnabled)
        {
            dwError = GetCachedPolicyFiles(
                        MACHINE_GROUP_POLICY,
                        pRecentGPO->pszgPCFileSysPath,
                        (PSTR) COMPUTER_MCX_CSE_GUID,
                        NULL,
                        &pszMachineDestFolder,
                        &fMachinePolicyExists);
            if (dwError)
            {
                MAC_AD_LOG_ERROR("Failed to get machine MCX settings for GPO, error %d", dwError);
                dwError = 0;
            }
        }

        IsMCXSettingEnabledForGPO(pRecentGPO, USER_GROUP_POLICY, &bEnabled);
        if (bEnabled)
        {
            dwError = GetCachedPolicyFiles(
                        USER_GROUP_POLICY,
                        pRecentGPO->pszgPCFileSysPath,
                        (PSTR) USER_MCX_CSE_GUID,
                        NULL,
                        &pszUserDestFolder,
                        &fUserPolicyExists);
            if (dwError)
            {
                MAC_AD_LOG_ERROR("Failed to get user MCX settings for GPO, error %d", dwError);
                dwError = 0;
            }
        }
    }

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

