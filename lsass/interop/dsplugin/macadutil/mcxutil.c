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

#include "../includes.h"

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
            LwFreeMemory(pTemp->pValueData);

        LwFreeMemory(pTemp);
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

    if (!ppMCXValue)
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    LOG("Checking file (%s) to see if it exists and has MCX settings", pszMCXValueFile);

    dwError = LwCheckFileTypeExists(pszMCXValueFile, LWFILE_REGULAR, &bFileExists);
    BAIL_ON_MAC_ERROR(dwError);

    if (!bFileExists)
    {
        LOG("File (%s) does not exist, this can be normal if the GPO has no more settings", pszMCXValueFile);
        dwError = MAC_AD_ERROR_INVALID_NAME;
        BAIL_ON_MAC_ERROR_NO_LOG(dwError);
    }

    dwError = LwAllocateMemory(sizeof(MCXVALUE), (PVOID*)&pNew);
    BAIL_ON_MAC_ERROR(dwError);

    fp = fopen(pszMCXValueFile, "r");
    if (!fp)
    {
        LOG("File (%s) could not be opened, this is unexpected since the file was just verified to exist", pszMCXValueFile);
        dwError = MAC_AD_ERROR_INVALID_NAME;
        BAIL_ON_MAC_ERROR(dwError);
    }

    (void) fseek(fp, 0, SEEK_END);
    pNew->iValLen = ftell(fp);
    rewind(fp);

    dwError = LwAllocateMemory(
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

    dwError = LwAllocateStringPrintf(
                    &pszMCXValueFile,
                    "%s/%s.mcx",
                    pszPolicyPath,
                    pszMCXFile);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwCreateDirectory(
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

        dwError = LwAllocateStringPrintf(
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

        dwError = LwAllocateStringPrintf(
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

            dwError = LwAllocateString(
                            "groupofcomputers",
                            &pszFileName);
            BAIL_ON_MAC_ERROR(dwError);

            break;

        case USER_GROUP_POLICY:

            dwError = LwAllocateString(
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
    DWORD dwVersion = 0;
    DWORD dwFileVersion = 0;
    PGROUP_POLICY_OBJECT pRecentGPO = NULL;

    if (!pGPO || !pfMachinePolicyExists || !pfUserPolicyExists || !ppszMachinePolicyPath || !ppszUserPolicyPath)
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

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
        LOG("Going to collect GPO payload and store in the cache location");
        if (IsMCXSettingEnabledForGPO(pRecentGPO, MACHINE_GROUP_POLICY))
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
                LOG_ERROR("Failed to get machine MCX settings for GPO, error %d", dwError);
                dwError = 0;
            }
        }

        if (IsMCXSettingEnabledForGPO(pRecentGPO, USER_GROUP_POLICY))
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
                LOG_ERROR("Failed to get user MCX settings for GPO, error %d", dwError);
                dwError = 0;
            }
        }
    }
    else
    {
        if (IsMCXSettingEnabledForGPO(pRecentGPO, MACHINE_GROUP_POLICY))
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
                LOG_ERROR("Failed to get machine MCX settings for GPO, error %d", dwError);
                dwError = 0;
            }
        }

        if (IsMCXSettingEnabledForGPO(pRecentGPO, USER_GROUP_POLICY))
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
                LOG_ERROR("Failed to get user MCX settings for GPO, error %d", dwError);
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

