/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        config.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Test Program for stress testing AD Provider
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "includes.h"

DWORD
LADSParseConfig(
    PCSTR pszConfigFilePath
    )
{
    DWORD dwError = 0;

    return dwError;
}
#if 0

    LADS_CONFIG_DATA configData = {0};
    
    dwError = LsaParseConfigFile(
                pszConfigFilePath,
                LSA_CFG_OPTION_STRIP_ALL,
                &LADSConfigStartSection,
                &LADSConfigEndSection,
                &LADSConfigNameValuePair,
                NULL,
                (PVOID*)&configData);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    LADSFreeConfigDataContents(&configData);

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LADSConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bSkipSection = FALSE;
    PLADS_CONFIG_DATA pConfigData = NULL;
    
    pConfigData = (PLADS_CONFIG_DATA)pData;
    BAIL_ON_INVALID_POINTER(pConfigData);
    
    LADSFreeConfigDataContents(pConfigData);
    
    pConfigData->dwNumThreads = 1;
    
    if (LW_IS_NULL_OR_EMPTY_STR(pszSectionName))
    {
        bSkipSection = TRUE;
        goto cleanup;
    }
    else if (!strcasecmp(pszSectionName, LADS_SECTION_NAME_AUTH_PROVIDER))
    {
        pConfigData->itemType = LADS_AUTH_PROVIDER;
    }
    else if (!strcasecmp(pszSectionName, LADS_SECTION_NAME_FIND_USER_BY_NAME))
    {
        pConfigData->itemType = LADS_FIND_USER_BY_NAME;
    }
    else if (!strcasecmp(pszSectionName, LADS_SECTION_NAME_FIND_USER_BY_ID))
    {
        pConfigData->itemType = LADS_FIND_USER_BY_ID;
    }
    else if (!strcasecmp(pszSectionName, LADS_SECTION_NAME_ENUM_USERS))
    {
        pConfigData->itemType = LADS_ENUM_USERS;
    }
    else if (!strcasecmp(pszSectionName, LADS_SECTION_NAME_FIND_GROUP_BY_NAME))
    {
        pConfigData->itemType = LADS_FIND_GROUP_BY_NAME;
    }
    else if (!strcasecmp(pszSectionName, LADS_SECTION_NAME_FIND_GROUP_BY_ID))
    {
        pConfigData->itemType = LADS_FIND_GROUP_BY_ID;
    }
    else if (!strcasecmp(pszSectionName, LADS_SECTION_NAME_ENUM_GROUPS))
    {
        pConfigData->itemType = LADS_ENUM_GROUPS;
    }
    else
    {
        bSkipSection = TRUE;
        goto cleanup;
    }
    
cleanup:

    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

    return dwError;
    
error:

    bSkipSection = TRUE;
    bContinue = FALSE;

    goto cleanup;
}

DWORD
LADSConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PLADS_CONFIG_DATA pConfigData = NULL;
    PSTR  pszValueCopy = NULL;
    
    pConfigData = (PLADS_CONFIG_DATA)pData;
    BAIL_ON_INVALID_POINTER(pConfigData);
    
    if (LW_IS_NULL_OR_EMPTY_STR(pszName) || LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        goto cleanup;
    }
    
    if (!strcasecmp(pszName, LADS_ATTR_NAME_THREADS))
    {
        pConfigData->dwNumThreads = atoi(pszValue);
        goto cleanup;
    }
    else if (!strcasecmp(pszName, LADS_ATTR_NAME_SLEEP))
    {
        pConfigData->dwSleepMSecs = atoi(pszValue);
    }
    else if (!strcasecmp(pszName, LADS_ATTR_NAME_INFO_LEVEL))
    {
        pConfigData->dwInfoLevel = atoi(pszValue);
    }
    
    switch (pConfigData->itemType)
    {
        case LADS_AUTH_PROVIDER:
            
            if (!strcasecmp(pszName, LADS_ATTR_NAME_CONFIG_FILE))
            {
                LW_SAFE_FREE_STRING(gpszProviderConfigFilePath);
                
                dwError = LwAllocateString(
                                pszValue,
                                &gpszProviderConfigFilePath);
                BAIL_ON_LSA_ERROR(dwError);
            }
            else if (!strcasecmp(pszName, LADS_ATTR_NAME_LIBPATH))
            {
                LW_SAFE_FREE_STRING(gpszProviderLibPath);
                
                dwError = LwAllocateString(
                                pszValue,
                                &gpszProviderLibPath);
                BAIL_ON_LSA_ERROR(dwError);
            }
        
            break;
            
        case LADS_FIND_USER_BY_NAME:
        case LADS_FIND_GROUP_BY_NAME:
            
            if (!strcasecmp(pszName, LADS_ATTR_NAME_NAME))
            {
                dwError = LwAllocateString(
                                pszValue,
                                &pszValueCopy);
                BAIL_ON_LSA_ERROR(dwError);
                
                dwError = LsaDLinkedListAppend(
                                &pConfigData->pItemList,
                                pszValueCopy);
                BAIL_ON_LSA_ERROR(dwError);
                
                pConfigData->dwNumItems++;
                
                pszValueCopy = NULL;
            }
            
            break;
            
        case LADS_FIND_GROUP_BY_ID:
        case LADS_FIND_USER_BY_ID:
            
            if (!strcasecmp(pszName, LADS_ATTR_NAME_ID))
            {
                dwError = LwAllocateString(
                                pszValue,
                                &pszValueCopy);
                BAIL_ON_LSA_ERROR(dwError);
                
                dwError = LsaDLinkedListAppend(
                                &pConfigData->pItemList,
                                pszValueCopy);
                BAIL_ON_LSA_ERROR(dwError);
                
                pConfigData->dwNumItems++;
                
                pszValueCopy = NULL;
            }
            
            break;
            
        default:
            
            break;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszValueCopy);

    *pbContinue = TRUE;
   
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LADSConfigEndSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PLADS_CONFIG_DATA pConfigData = NULL;
    PDLINKEDLIST pIter = NULL;
    
    pConfigData = (PLADS_CONFIG_DATA)pData;
    BAIL_ON_INVALID_POINTER(pConfigData);
    
    gLADSStressData[pConfigData->itemType].type = pConfigData->itemType;
    gLADSStressData[pConfigData->itemType].dwNumThreads = pConfigData->dwNumThreads;
    gLADSStressData[pConfigData->itemType].dwSleepMSecs = pConfigData->dwSleepMSecs;
    gLADSStressData[pConfigData->itemType].dwInfoLevel = pConfigData->dwInfoLevel;
    
    switch (pConfigData->itemType)
    {
        case LADS_FIND_USER_BY_NAME:
        case LADS_FIND_GROUP_BY_NAME:
            
            if (!pConfigData->dwNumItems)
            {
                gLADSStressData[pConfigData->itemType].dwNumThreads = 0;
            }
            else
            {
                DWORD iData = 0;
                
                dwError = LwAllocateMemory(
                                pConfigData->dwNumItems * sizeof(PSTR),
                                (PVOID*)&gLADSStressData[pConfigData->itemType].data.ppszNames);
                BAIL_ON_LSA_ERROR(dwError);
                
                for (pIter = pConfigData->pItemList;
                     pIter; pIter = pIter->pNext, iData++)
                {
                    gLADSStressData[pConfigData->itemType].data.ppszNames[iData] =
                        (PSTR)pIter->pItem;
                    pIter->pItem = NULL;
                }
            }
              
            break;
            
        case LADS_FIND_USER_BY_ID:
            
            if (!pConfigData->dwNumItems)
            {
                gLADSStressData[pConfigData->itemType].dwNumThreads = 0;
            }
            else
            {
                DWORD iData = 0;
                
                dwError = LwAllocateMemory(
                                pConfigData->dwNumItems * sizeof(uid_t),
                                (PVOID*)&gLADSStressData[pConfigData->itemType].data.pUidArray);
                BAIL_ON_LSA_ERROR(dwError);
                
                for (pIter = pConfigData->pItemList;
                     pIter; pIter = pIter->pNext, iData++)
                {
                    gLADSStressData[pConfigData->itemType].data.pUidArray[iData] =
                        atol((PSTR)pIter->pItem);
                }
            }
            
            break;
            
        case LADS_FIND_GROUP_BY_ID:
            
            if (!pConfigData->dwNumItems)
            {
                gLADSStressData[pConfigData->itemType].dwNumThreads = 0;
            }
            else
            {
                DWORD iData = 0;
                
                dwError = LwAllocateMemory(
                                pConfigData->dwNumItems * sizeof(gid_t),
                                (PVOID*)&gLADSStressData[pConfigData->itemType].data.pGidArray);
                BAIL_ON_LSA_ERROR(dwError);
                
                for (pIter = pConfigData->pItemList;
                     pIter; pIter = pIter->pNext, iData++)
                {
                    gLADSStressData[pConfigData->itemType].data.pGidArray[iData] =
                        atol((PSTR)pIter->pItem);
                }
            }
            
            break;
            
        default:
            
            break;
    }
    
cleanup:

    return dwError;

error:

    goto cleanup;
}

VOID
LADSFreeConfigDataContents(
    PLADS_CONFIG_DATA pConfigData
    )
{
    if (pConfigData->pItemList)
    {
        LsaDLinkedListForEach(
                        pConfigData->pItemList,
                        &LADSFreeConfigListItem,
                        NULL);
        
        LsaDLinkedListFree(pConfigData->pItemList);
        
        pConfigData->pItemList = NULL;
        
        memset(pConfigData, 0, sizeof(LADS_CONFIG_DATA));
    }
}

VOID
LADSFreeConfigListItem(
    PVOID pItem,
    PVOID pUserData
    )
{
    LW_SAFE_FREE_MEMORY(pItem);
}
#endif
