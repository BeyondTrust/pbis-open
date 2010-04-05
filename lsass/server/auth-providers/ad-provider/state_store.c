/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2010
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
 *        state_store.c
 *
 * Abstract:
 *
 *        Caching for AD Provider Database Interface
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *          Adam Bernstein (adam.bernstein@likewise.com)
 *
 */

#include "adprovider.h"

// The database consists of several tables.  Because we're
// using a flat file and the amount of data is small, the
// easiest way to update it is to replace the entire file.
// However, we may be asked to write some of it before we
// have been asked to read all of it.  The simplest solution
// is to preload everything into memory and hold it until
// either we're asked for it or we determine it isn't needed.
typedef struct _ADSTATE_CONNECTION
{
    pthread_rwlock_t lock;
} ADSTATE_CONNECTION, *PADSTATE_CONNECTION;

// This is the maximum number of characters necessary to store a guid in
// string form.
#define UUID_STR_SIZE 37

typedef struct _AD_REGB_DOMAIN_INFO
{
    PSTR pszDnsDomainName;
    PSTR pszNetbiosDomainName;
    PSTR pszSid;
    PSTR pszGuid;
    PSTR pszTrusteeDnsDomainName;
    DWORD dwTrustFlags;
    DWORD dwTrustType;
    DWORD dwTrustAttributes;
    LSA_TRUST_DIRECTION dwTrustDirection;
    LSA_TRUST_MODE dwTrustMode;
    // Can be NULL (e.g. external trust)
    PSTR pszForestName;
    PSTR pszClientSiteName;
    LSA_DM_DOMAIN_FLAGS Flags;
    PLSA_DM_DC_INFO DcInfo;
    PLSA_DM_DC_INFO GcInfo;
} AD_REGDB_DOMAIN_INFO, *PAD_REGDB_DOMAIN_INFO;

static
DWORD
ADState_ReadFromRegistry(
    OUT OPTIONAL PAD_PROVIDER_DATA* ppProviderData,
    OUT OPTIONAL PDLINKEDLIST* ppDomainList
    );

DWORD
ADState_ReadRegProviderData(
    OUT PAD_PROVIDER_DATA *ppProviderData
    );

DWORD
ADState_ReadRegCellEntry(
    IN OUT PDLINKEDLIST *ppCellList
    );

DWORD
ADState_ReadRegDomainEntry(
    PDLINKEDLIST *ppDomainList
    );

static
VOID
ADState_FreeEnumDomainInfoCallback(
    IN OUT PVOID pData,
    IN PVOID pUserData
    );

VOID
ADState_FreeEnumDomainInfo(
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo
    );

static
DWORD
ADState_WriteToRegistry(
    IN OPTIONAL PAD_PROVIDER_DATA pProviderData,
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo,
    IN OPTIONAL DWORD dwDomainInfoCount,
    IN PLSA_DM_ENUM_DOMAIN_INFO pDomainInfoAppend
    );

static
DWORD
ADState_WriteRegProviderData(
    IN PAD_PROVIDER_DATA pProviderData
    );

DWORD
ADState_WriteRegDomainEntry(
    IN PLSA_DM_ENUM_DOMAIN_INFO pDomainInfoEntry
    );

DWORD
ADState_WriteRegCellEntry(
    IN PAD_LINKED_CELL_INFO pCellEntry
    );


DWORD
ADState_OpenDb(
    ADSTATE_CONNECTION_HANDLE* phDb
    )
{
    DWORD dwError = 0;
    BOOLEAN bLockCreated = FALSE;
    PADSTATE_CONNECTION pConn = NULL;

    dwError = LwAllocateMemory(
                    sizeof(ADSTATE_CONNECTION),
                    (PVOID*)&pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pthread_rwlock_init(&pConn->lock, NULL);
    BAIL_ON_LSA_ERROR(dwError);
    bLockCreated = TRUE;

    *phDb = pConn;

cleanup:

    return dwError;

error:

    if (pConn != NULL)
    {
        if (bLockCreated)
        {
            pthread_rwlock_destroy(&pConn->lock);
        }

        LW_SAFE_FREE_MEMORY(pConn);
    }
    *phDb = NULL;

    goto cleanup;
}

void
ADState_SafeCloseDb(
    ADSTATE_CONNECTION_HANDLE* phDb
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    ADSTATE_CONNECTION_HANDLE hDb = NULL;

    if (phDb == NULL || *phDb == NULL)
    {
        goto cleanup;
    }

    hDb = *phDb;

    dwError = pthread_rwlock_destroy(&hDb->lock);
    if (dwError != LW_ERROR_SUCCESS)
    {
        LSA_LOG_ERROR("Error destroying lock [%d]", dwError);
        dwError = LW_ERROR_SUCCESS;
    }

    LW_SAFE_FREE_MEMORY(hDb);

cleanup:
    return;
}

DWORD
ADState_EmptyDb(
    ADSTATE_CONNECTION_HANDLE hDb
    )
{
    DWORD dwError = 0;

    dwError = ADState_WriteToRegistry(
                  NULL,
                  NULL,
                  0,
                  NULL);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
ADState_GetProviderData(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    OUT PAD_PROVIDER_DATA* ppResult
    )
{
    DWORD dwError = 0;

    dwError = ADState_ReadFromRegistry(
                  ppResult,
                  NULL);
    return dwError;
}

DWORD
ADState_StoreProviderData(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    IN PAD_PROVIDER_DATA pProvider
    )
{
    DWORD dwError = 0;

    if (pProvider)
    {
        dwError = ADState_WriteToRegistry(
                      pProvider,
                      NULL,
                      0,
                      NULL);
    }

    return dwError;
}

DWORD
ADState_GetDomainTrustList(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    OUT PDLINKEDLIST* ppList
    )
{
    return ADState_ReadFromRegistry(
               NULL,
               ppList);
}

DWORD
ADState_AddDomainTrust(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    IN PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo
    )
{
    DWORD dwError = 0;

    if (pDomainInfo)
    {
        dwError = ADState_WriteToRegistry(
                      NULL,
                      NULL,
                      0,
                      pDomainInfo);
    }

    return dwError;
}

DWORD
ADState_StoreDomainTrustList(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    IN PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo,
    IN DWORD dwDomainInfoCount
    )
{
    DWORD dwError = 0;

    if (ppDomainInfo && dwDomainInfoCount)
    {
        dwError = ADState_WriteToRegistry(
                      NULL,
                      ppDomainInfo,
                      dwDomainInfoCount,
                      NULL);
    }

    return dwError;
}


static
DWORD
ADState_ReadFromRegistry(
    OUT OPTIONAL PAD_PROVIDER_DATA* ppProviderData,
    OUT OPTIONAL PDLINKEDLIST* ppDomainList
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pRegCellList = NULL;
    PDLINKEDLIST pRegDomainList = NULL;
    PAD_PROVIDER_DATA pRegProviderData = NULL;

    if (ppProviderData)
    {
        dwError = ADState_ReadRegProviderData(
                      &pRegProviderData);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADState_ReadRegCellEntry(
                      &pRegCellList);
        BAIL_ON_LSA_ERROR(dwError);

        if (pRegProviderData)
        {
            pRegProviderData->pCellList = pRegCellList;
            pRegCellList = NULL;
        }
        *ppProviderData = pRegProviderData;
        pRegProviderData = NULL;
    }

    if (ppDomainList)
    {
        dwError = ADState_ReadRegDomainEntry(
                      &pRegDomainList);
        BAIL_ON_LSA_ERROR(dwError);
        *ppDomainList = pRegDomainList;
    }
    cleanup:
        return dwError;
    error:
        goto cleanup;
}


VOID
ADState_FreeEnumDomainInfoList(
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    IN OUT PDLINKEDLIST pList
    )
{
    LsaDLinkedListForEach(
        pList,
        ADState_FreeEnumDomainInfoCallback,
        NULL);

    LsaDLinkedListFree(pList);
}

static
VOID
ADState_FreeEnumDomainInfoCallback(
    IN OUT PVOID pData,
    IN PVOID pUserData
    )
{
    PLSA_DM_ENUM_DOMAIN_INFO pInfo =
        (PLSA_DM_ENUM_DOMAIN_INFO)pData;

    ADState_FreeEnumDomainInfo(pInfo);
}

VOID
ADState_FreeEnumDomainInfo(
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo
    )
{
    if (pDomainInfo)
    {
        LW_SAFE_FREE_STRING(pDomainInfo->pszDnsDomainName);
        LW_SAFE_FREE_STRING(pDomainInfo->pszNetbiosDomainName);
        LW_SAFE_FREE_MEMORY(pDomainInfo->pSid);
        LW_SAFE_FREE_MEMORY(pDomainInfo->pGuid);
        LW_SAFE_FREE_STRING(pDomainInfo->pszTrusteeDnsDomainName);
        LW_SAFE_FREE_STRING(pDomainInfo->pszForestName);
        LW_SAFE_FREE_STRING(pDomainInfo->pszClientSiteName);
        if (pDomainInfo->DcInfo)
        {
            // ISSUE-2008/09/10-dalmeida -- need ASSERT macro
            LSA_LOG_ALWAYS("ASSERT!!! - DcInfo should never be set by DB code!");
        }
        if (pDomainInfo->GcInfo)
        {
            // ISSUE-2008/09/10-dalmeida -- need ASSERT macro
            LSA_LOG_ALWAYS("ASSERT!!! - GcInfo should never be set by DB code!");
        }
        LwFreeMemory(pDomainInfo);
    }
}

static
DWORD
ADState_WriteToRegistry(
    IN OPTIONAL PAD_PROVIDER_DATA pProviderData,
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo,
    IN OPTIONAL DWORD dwDomainInfoCount,
    IN PLSA_DM_ENUM_DOMAIN_INFO pDomainInfoAppend
    )
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    PDLINKEDLIST pCellList = NULL;
    HANDLE hReg = NULL;

    /* Handle the ADState_EmptyDb case */
    if (!pProviderData && !ppDomainInfo && !pDomainInfoAppend)
    {
        dwError = RegOpenServer(&hReg);
        BAIL_ON_LSA_ERROR(dwError);
        
        /* Don't care if these fail, these keys may not exist */
        RegUtilDeleteTree(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY,
                      AD_PROVIDER_DATA_REGKEY);
        RegUtilDeleteTree(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY,
                      AD_DOMAIN_TRUST_REGKEY);
        RegUtilDeleteTree(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY,
                      AD_LINKEDCELL_REGKEY);
    }
    else if (pProviderData)
    {
        /* Don't care if this fails, value may not exist yet */
        dwError = RegUtilDeleteValue(
                      NULL,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY, 
                      AD_LINKEDCELL_REGKEY,
                      "CellList");

        dwError = ADState_WriteRegProviderData(
                      pProviderData);
        BAIL_ON_LSA_ERROR(dwError);
        pCellList = pProviderData->pCellList;
        while (pCellList)
        {
            dwError = ADState_WriteRegCellEntry(
                          pCellList->pItem);
            BAIL_ON_LSA_ERROR(dwError);

            pCellList = pCellList->pNext;
        }
    }

    if (ppDomainInfo)
    {
        for (dwCount=0; dwCount<dwDomainInfoCount; dwCount++)
        {
            dwError = ADState_WriteRegDomainEntry(
                          ppDomainInfo[dwCount]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        if (pDomainInfoAppend)
        {
            dwError = ADState_WriteRegDomainEntry(
                          pDomainInfoAppend);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    RegCloseServer(hReg);
    return dwError;
error: 
    goto cleanup;
}


static
DWORD
ADState_ReadRegProviderDataValue(
    HANDLE hReg,
    PSTR pszFullKeyPath,
    PSTR pszSubKey,
    PSTR pszValueName,
    DWORD regType,
    PVOID pValue,
    PDWORD pdwValueLen)
{
    DWORD dwError = 0;
    PSTR pszValue = NULL;

    if (regType == REG_SZ)
    {
        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      pszFullKeyPath,
                      pszSubKey,
                      pszValueName,
                      NULL,
                      (PVOID) &pszValue,
                      pdwValueLen);
        memcpy(pValue, pszValue, *pdwValueLen);
        LW_SAFE_FREE_MEMORY(pszValue);
    }
    else
    {
        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      pszFullKeyPath,
                      pszSubKey,
                      pszValueName,
                      NULL,
                      pValue,
                      pdwValueLen);
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
    
}

static
DWORD
ADState_WriteRegProviderDataValue(
    HANDLE hReg,
    PSTR pszFullKeyPath,
    PSTR pszSubKey,
    PSTR pszValueName,
    DWORD dwType,
    PVOID pValue,
    DWORD dwValueLen)
{
    DWORD dwError = 0;
    DWORD dwDataLen = 0;
    DWORD dwData = 0;
    PVOID pData = NULL;
    PSTR pszValue = NULL;

    switch (dwType)
    {
        case REG_SZ:
            pszValue = (PSTR) pValue;
            if (pszValue)
            {
                dwDataLen = strlen(pszValue);
                pData = pszValue;
            }
            else
            {
                pszValue = "";
                dwDataLen = 0;
                pData = (PVOID) pszValue;
            }
            break;

        case REG_DWORD:
        default:
            if (dwValueLen == sizeof(WORD))
            {
                dwData = *((WORD *) pValue);
            }
            else
            {
                dwData = *((DWORD *) pValue);
            }
            pData = (PVOID) &dwData;
            dwDataLen = sizeof(DWORD);
            break;

        case REG_BINARY:
            dwDataLen = dwValueLen;
            pData = (PVOID) pValue;
            break;
    }

    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  pszFullKeyPath,
                  pszSubKey,
                  pszValueName,
                  dwType,
                  pData,
                  dwDataLen);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
ADState_ReadRegProviderData(
    OUT PAD_PROVIDER_DATA *ppProviderData
    )
{
    PAD_PROVIDER_DATA pProviderData = NULL;
    DWORD dwError = 0;
    DWORD dwValueLen = 0;

    dwError = LwAllocateMemory(sizeof(*pProviderData), (PVOID) &pProviderData);
    BAIL_ON_LSA_ERROR(dwError);

    HANDLE hReg = NULL;
    dwError = RegOpenServer(&hReg);
    BAIL_ON_LSA_ERROR(dwError);


    dwError = RegUtilIsValidKey(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_PROVIDER_DATA_REGKEY);
    if (dwError)
    {
        dwError = 0;
        goto cleanup;
    }

    dwError = ADState_ReadRegProviderDataValue(
                  hReg,
                  AD_PROVIDER_REGKEY,
                  AD_PROVIDER_DATA_REGKEY,
                  "DirectoryMode",
                  REG_DWORD,
                  (PVOID) &pProviderData->dwDirectoryMode,
                  &dwValueLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_ReadRegProviderDataValue(
                  hReg,
                  AD_PROVIDER_REGKEY,
                  AD_PROVIDER_DATA_REGKEY,
                  "ADConfigurationMode",
                  REG_DWORD,
                  (PVOID) &pProviderData->adConfigurationMode,
                  &dwValueLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_ReadRegProviderDataValue(
                  hReg,
                  AD_PROVIDER_REGKEY,
                  AD_PROVIDER_DATA_REGKEY,
                  "Domain",
                  REG_SZ,
                  (PVOID) &pProviderData->szDomain,
                  &dwValueLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_ReadRegProviderDataValue(
                  hReg,
                  AD_PROVIDER_REGKEY,
                  AD_PROVIDER_DATA_REGKEY,
                  "ShortDomain",
                  REG_SZ,
                  (PVOID) &pProviderData->szShortDomain,
                  &dwValueLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_ReadRegProviderDataValue(
                  hReg,
                  AD_PROVIDER_REGKEY,
                  AD_PROVIDER_DATA_REGKEY,
                  "ComputerDN",
                  REG_SZ,
                  (PVOID) &pProviderData->szComputerDN,
                  &dwValueLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_ReadRegProviderDataValue(
                  hReg,
                  AD_PROVIDER_REGKEY,
                  AD_PROVIDER_DATA_REGKEY,
                  "CellDN",
                  REG_SZ,
                  (PVOID) &pProviderData->cell.szCellDN,
                  &dwValueLen);
    BAIL_ON_LSA_ERROR(dwError);

    *ppProviderData = pProviderData;
cleanup:
     
    RegCloseServer(hReg);
    return dwError;

error:
    goto cleanup;
}


DWORD
ADState_ReadRegCellEntry(
    PDLINKEDLIST *ppCellList)
{
    PAD_LINKED_CELL_INFO pListEntry = NULL;
    DWORD dwError = 0;
    HANDLE hReg = NULL;
    DWORD i = 0;
    DWORD dwValueLen = 0;
    PSTR *ppszMultiCellListOrder = NULL;
    DWORD dwMultiCellListOrder = 0;
    DWORD dwIsForestCell = 0;

    dwError = LwAllocateMemory(sizeof(*pListEntry), (PVOID) &pListEntry);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegOpenServer(&hReg);
    BAIL_ON_LSA_ERROR(dwError);


    dwError = RegUtilIsValidKey(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_LINKEDCELL_REGKEY);
    if (dwError)
    {
        dwError = 0;
        goto cleanup;
    }

    /* Ordered list of cells saved in REG_MULTI_SZ value */
    dwError = RegUtilGetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY,
                  AD_LINKEDCELL_REGKEY,
                  "CellList",
                  NULL,
                  (PVOID) &ppszMultiCellListOrder,
                  &dwMultiCellListOrder);
    BAIL_ON_LSA_ERROR(dwError);

    for (i=0; i<dwMultiCellListOrder; i++)
    {
        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_LINKEDCELL_REGKEY,
                      ppszMultiCellListOrder[i],
                      "CellDN",
                      NULL,
                      (PVOID) &pListEntry->pszCellDN,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_LINKEDCELL_REGKEY,
                      ppszMultiCellListOrder[i],
                      "Domain",
                      NULL,
                      (PVOID) &pListEntry->pszDomain,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);
        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_LINKEDCELL_REGKEY,
                      ppszMultiCellListOrder[i],
                      "IsForestCell",
                      NULL,
                      (PVOID) &dwIsForestCell,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);
        pListEntry->bIsForestCell = dwIsForestCell ? 1 : 0;

        dwError = LsaDLinkedListAppend(
                      ppCellList,
                      pListEntry);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    for (i=0; i<dwMultiCellListOrder; i++)
    {
        LW_SAFE_FREE_STRING(ppszMultiCellListOrder[i]);
    }
    LW_SAFE_FREE_MEMORY(ppszMultiCellListOrder);
    return dwError;

error:
    goto cleanup;
    return dwError;
}


DWORD
ADState_ReadRegDomainEntry(
    PDLINKEDLIST *ppDomainList)
{
    HANDLE hReg = NULL;
    PAD_REGDB_DOMAIN_INFO pDomainInfo = NULL;
    PLSA_DM_ENUM_DOMAIN_INFO pListEntry = NULL;
    DWORD dwError = 0;
    PWSTR *ppwszSubKeys = NULL;
    PSTR pszSubKey = NULL;
    PSTR pszSubKeyPtr = NULL;
    PSTR pszSID = NULL;
    PSTR pszGUID = NULL;
    DWORD dwSubKeysLen = 0;
    DWORD dwValueLen = 0;
    DWORD i = 0;

    pDomainInfo = NULL;
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegOpenServer(&hReg);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegUtilIsValidKey(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY);
    if (dwError)
    {
        dwError = 0;
        goto cleanup;
    }

    dwError = RegUtilGetKeys(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY,
                  AD_DOMAIN_TRUST_REGKEY,
                  &ppwszSubKeys,
                  &dwSubKeysLen);
    BAIL_ON_LSA_ERROR(dwError);

    for (i=0; i<dwSubKeysLen; i++)
    {
        LW_SAFE_FREE_STRING(pszSID);
        LW_SAFE_FREE_STRING(pszGUID);

        dwError = LwWc16sToMbs(ppwszSubKeys[i], &pszSubKey);
        BAIL_ON_LSA_ERROR(dwError);
        pszSubKeyPtr = strrchr(pszSubKey, '\\');
        if (pszSubKeyPtr)
        {
            pszSubKeyPtr++;
        }
        else
        {
            pszSubKeyPtr = pszSubKey;
        }
        
        dwError = LwAllocateMemory(
                      sizeof(*pListEntry),
                      (PVOID*)&pListEntry);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                      pszSubKeyPtr,
                      "DNSDomainName",
                      NULL,
                      (PVOID) &pListEntry->pszDnsDomainName,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);
        if (pListEntry->pszDnsDomainName &&
	    !*pListEntry->pszDnsDomainName)
        {
            LW_SAFE_FREE_STRING(pListEntry->pszDnsDomainName);
        }

        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                      pszSubKeyPtr,
                      "NetBiosDomainName",
                      NULL,
                      (PVOID) &pListEntry->pszNetbiosDomainName,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);
        if (pListEntry->pszNetbiosDomainName &&
	    !*pListEntry->pszNetbiosDomainName)
        {
            LW_SAFE_FREE_STRING(pListEntry->pszNetbiosDomainName);
        }

        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                      pszSubKeyPtr,
                      "SID",
                      NULL,
                      (PVOID) &pszSID,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);
        dwError = LsaAllocateSidFromCString(
                      &pListEntry->pSid,
                      pszSID);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                      pszSubKeyPtr,
                      "GUID",
                      NULL,
                      (PVOID) &pszGUID,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);
        if (pszGUID)
        {
            dwError = LwAllocateMemory(
                          UUID_STR_SIZE,
                          (PVOID*)&pListEntry->pGuid);
            BAIL_ON_LSA_ERROR(dwError);
    
            if (uuid_parse(
                    pszGUID,
                    *pListEntry->pGuid) < 0)
            {
                // uuid_parse returns -1 on error, but does not set errno
                dwError = LW_ERROR_INVALID_OBJECTGUID;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                      pszSubKeyPtr,
                      "TrusteeDomainName",
                      NULL,
                      (PVOID) &pListEntry->pszTrusteeDnsDomainName,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);
        if (pListEntry->pszTrusteeDnsDomainName &&
	    !*pListEntry->pszTrusteeDnsDomainName)
        {
            LW_SAFE_FREE_STRING(pListEntry->pszTrusteeDnsDomainName);
        }

        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                      pszSubKeyPtr,
                      "TrustFlags",
                      NULL,
                      (PVOID) &pListEntry->dwTrustFlags,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                      pszSubKeyPtr,
                      "TrustType",
                      NULL,
                      (PVOID) &pListEntry->dwTrustType,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                      pszSubKeyPtr,
                      "TrustAttributes",
                      NULL,
                      (PVOID) &pListEntry->dwTrustAttributes,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                      pszSubKeyPtr,
                      "TrustDirection",
                      NULL,
                      (PVOID) &pListEntry->dwTrustDirection,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                      pszSubKeyPtr,
                      "TrustMode",
                      NULL,
                      (PVOID) &pListEntry->dwTrustMode,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                      pszSubKeyPtr,
                      "ForestName",
                      NULL,
                      (PVOID) &pListEntry->pszForestName,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);
        if (pListEntry->pszForestName && !*pListEntry->pszForestName)
        {
            LW_SAFE_FREE_STRING(pListEntry->pszForestName);
        }

        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                      pszSubKeyPtr,
                      "ClientSiteName",
                      NULL,
                      (PVOID) &pListEntry->pszClientSiteName,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);
        if (pListEntry->pszClientSiteName && !*pListEntry->pszClientSiteName)
        {
            LW_SAFE_FREE_STRING(pListEntry->pszClientSiteName);
        }

        dwError = RegUtilGetValue(
                      hReg,
                      HKEY_THIS_MACHINE,
                      AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                      pszSubKeyPtr,
                      "Flags",
                      NULL,
                      (PVOID) &pListEntry->Flags,
                      &dwValueLen);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDLinkedListAppend(
                      ppDomainList,
                      pListEntry);
        BAIL_ON_LSA_ERROR(dwError);
        pListEntry = NULL;
        LW_SAFE_FREE_STRING(pszSubKey);
    }

cleanup:
    if (hReg)
    {
        RegCloseServer(hReg);
    }
    LW_SAFE_FREE_STRING(pszSubKey);
    LW_SAFE_FREE_STRING(pszSID);
    LW_SAFE_FREE_STRING(pszGUID);
    for (i = 0; i < dwSubKeysLen; i++)
    {
        LW_SAFE_FREE_MEMORY(ppwszSubKeys[i]);
    }
    LW_SAFE_FREE_MEMORY(ppwszSubKeys);
    return dwError;

error:
    goto cleanup;
    return dwError;
}


DWORD
ADState_WriteRegDomainEntry(
    IN PLSA_DM_ENUM_DOMAIN_INFO pDomainInfoEntry
    )
{
    HANDLE hReg = NULL;
    DWORD dwError = 0;
    PSTR pszSid = NULL;
    char szGuid[UUID_STR_SIZE] = {0};

    dwError = RegOpenServer(&hReg);
    BAIL_ON_LSA_ERROR(dwError);

    /* Add top level AD DomainTrust data registry key */
    dwError = RegUtilAddKey(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY,
                  AD_DOMAIN_TRUST_REGKEY);
    BAIL_ON_LSA_ERROR(dwError);

    /* Add top level AD DomainTrust data registry key */
    dwError = RegUtilAddKey(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                  pDomainInfoEntry->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    /* Write DomainTrust data entries to registry */
    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                  pDomainInfoEntry->pszNetbiosDomainName,
                  "DNSDomainName",
                  REG_SZ,
                  pDomainInfoEntry->pszDnsDomainName,
                  pDomainInfoEntry->pszDnsDomainName ?
                      strlen(pDomainInfoEntry->pszDnsDomainName) : 0);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                  pDomainInfoEntry->pszNetbiosDomainName,
                  "NetBiosDomainName",
                  REG_SZ,
                  pDomainInfoEntry->pszNetbiosDomainName,
                  pDomainInfoEntry->pszNetbiosDomainName ?
                      strlen(pDomainInfoEntry->pszNetbiosDomainName) : 0);
    BAIL_ON_LSA_ERROR(dwError);

    if (pDomainInfoEntry->pSid != NULL)
    {
        dwError = LsaAllocateCStringFromSid(
                &pszSid,
                pDomainInfoEntry->pSid);
        BAIL_ON_LSA_ERROR(dwError);
    }
    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                  pDomainInfoEntry->pszNetbiosDomainName,
                  "SID",
                  REG_SZ,
                  pszSid,
                  pszSid ? strlen(pszSid) : 0);
    BAIL_ON_LSA_ERROR(dwError);

    if (pDomainInfoEntry->pGuid)
    {
        // Writes into a 37-byte caller allocated string
        uuid_unparse(*pDomainInfoEntry->pGuid, szGuid);

    }
    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                  pDomainInfoEntry->pszNetbiosDomainName,
                  "GUID",
                  REG_SZ,
                  szGuid,
                  strlen(szGuid));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                  pDomainInfoEntry->pszNetbiosDomainName,
                  "TrusteeDomainName",
                  REG_SZ,
                  pDomainInfoEntry->pszTrusteeDnsDomainName,
                  pDomainInfoEntry->pszTrusteeDnsDomainName ?
                      strlen(pDomainInfoEntry->pszTrusteeDnsDomainName) : 0);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                  pDomainInfoEntry->pszNetbiosDomainName,
                  "TrustFlags",
                  REG_DWORD,
                  &pDomainInfoEntry->dwTrustFlags,
                  sizeof(pDomainInfoEntry->dwTrustFlags));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                  pDomainInfoEntry->pszNetbiosDomainName,
                  "TrustType",
                  REG_DWORD,
                  &pDomainInfoEntry->dwTrustType,
                  sizeof(pDomainInfoEntry->dwTrustType));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                  pDomainInfoEntry->pszNetbiosDomainName,
                  "TrustAttributes",
                  REG_DWORD,
                  &pDomainInfoEntry->dwTrustAttributes,
                  sizeof(pDomainInfoEntry->dwTrustAttributes));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                  pDomainInfoEntry->pszNetbiosDomainName,
                  "TrustDirection",
                  REG_DWORD,
                  &pDomainInfoEntry->dwTrustDirection,
                  sizeof(pDomainInfoEntry->dwTrustDirection));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                  pDomainInfoEntry->pszNetbiosDomainName,
                  "TrustMode",
                  REG_DWORD,
                  &pDomainInfoEntry->dwTrustMode,
                  sizeof(pDomainInfoEntry->dwTrustMode));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                  pDomainInfoEntry->pszNetbiosDomainName,
                  "ForestName",
                  REG_SZ,
                  pDomainInfoEntry->pszForestName,
                  pDomainInfoEntry->pszForestName ?
                      strlen(pDomainInfoEntry->pszForestName) : 0);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                  pDomainInfoEntry->pszNetbiosDomainName,
                  "ClientSiteName",
                  REG_SZ,
                  pDomainInfoEntry->pszClientSiteName,
                  pDomainInfoEntry->pszClientSiteName ?
                      strlen(pDomainInfoEntry->pszClientSiteName) : 0);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_DOMAIN_TRUST_REGKEY,
                  pDomainInfoEntry->pszNetbiosDomainName,
                  "Flags",
                  REG_DWORD,
                  &pDomainInfoEntry->Flags,
                  sizeof(pDomainInfoEntry->Flags));
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pszSid);
     
    RegCloseServer(hReg);
    return dwError;

error:
    goto cleanup;
}
 

DWORD
ADState_WriteRegCellEntry(
    IN PAD_LINKED_CELL_INFO pCellEntry
    )
{
    HANDLE hReg = NULL;
    DWORD dwError = 0;
    DWORD dwBooleanValue = 0;
    DWORD dwValueLen = 0;
    PSTR *ppszMultiCellListOrder = NULL;
    PSTR *ppszNewMultiCellListOrder = NULL;

    dwError = RegOpenServer(&hReg);
    BAIL_ON_LSA_ERROR(dwError);

    /* Add top level AD CellEntry data registry key */
    dwError = RegUtilAddKey(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY,
                  AD_LINKEDCELL_REGKEY);
    BAIL_ON_LSA_ERROR(dwError);

    /* Add cell-specific key entry */
    dwError = RegUtilAddKey(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_LINKEDCELL_REGKEY,
                  pCellEntry->pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegUtilGetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY,
                  AD_LINKEDCELL_REGKEY,
                  "CellList",
                  NULL,
                  (PVOID) &ppszMultiCellListOrder,
                  &dwValueLen);
    dwError = LwReallocMemory(
                  ppszMultiCellListOrder,
                  (PVOID) &ppszNewMultiCellListOrder,
                  (dwValueLen+2) * sizeof(PSTR));
    BAIL_ON_LSA_ERROR(dwError);
    ppszMultiCellListOrder = ppszNewMultiCellListOrder;
    ppszMultiCellListOrder[dwValueLen] = pCellEntry->pszCellDN;
    ppszMultiCellListOrder[dwValueLen+1] = NULL;

    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY,
                  AD_LINKEDCELL_REGKEY,
                  "CellList",
                  REG_MULTI_SZ,
                  (PVOID) ppszMultiCellListOrder,
                  dwValueLen);
    BAIL_ON_LSA_ERROR(dwError);
                     
    /* Write cell data entries to registry */
    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_LINKEDCELL_REGKEY,
                  pCellEntry->pszCellDN,
                  "CellDN",
                  REG_SZ,
                  pCellEntry->pszCellDN,
                  strlen(pCellEntry->pszCellDN));
    BAIL_ON_LSA_ERROR(dwError);
    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_LINKEDCELL_REGKEY,
                  pCellEntry->pszCellDN,
                  "Domain",
                  REG_SZ,
                  pCellEntry->pszDomain,
                  strlen(pCellEntry->pszDomain));
    BAIL_ON_LSA_ERROR(dwError);

    dwBooleanValue = pCellEntry->bIsForestCell;
    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY "\\" AD_LINKEDCELL_REGKEY,
                  pCellEntry->pszCellDN,
                  "IsForestCell",
                  REG_DWORD,
                  &dwBooleanValue,
                  sizeof(dwBooleanValue));
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
     
    RegCloseServer(hReg);
    return dwError;

error:
    goto cleanup;
}


static
DWORD
ADState_WriteRegProviderData(
    IN PAD_PROVIDER_DATA pProviderData
    )
{
    HANDLE hReg = NULL;
    DWORD dwAdConfigurationMode = 0;
    DWORD dwError = 0;
    PSTR pszString = NULL;

    dwError = RegOpenServer(&hReg);
    BAIL_ON_LSA_ERROR(dwError);
    /* Add top level AD provider provider data registry key */
    dwError = RegUtilAddKey(
                  hReg,
                  HKEY_THIS_MACHINE,
                  AD_PROVIDER_REGKEY,
                  AD_PROVIDER_DATA_REGKEY);
    BAIL_ON_LSA_ERROR(dwError);

    /* Write provider data entries to registry */

    dwError = ADState_WriteRegProviderDataValue(
                  hReg,
                  AD_PROVIDER_REGKEY,
                  AD_PROVIDER_DATA_REGKEY,
                  "DirectoryMode",
                  REG_DWORD,
                  &pProviderData->dwDirectoryMode,
                  sizeof(pProviderData->dwDirectoryMode));
    BAIL_ON_LSA_ERROR(dwError);

    dwAdConfigurationMode = (DWORD) pProviderData->adConfigurationMode;
    dwError = ADState_WriteRegProviderDataValue(
                  hReg,
                  AD_PROVIDER_REGKEY,
                  AD_PROVIDER_DATA_REGKEY,
                  "ADConfigurationMode",
                  REG_DWORD,
                  &dwAdConfigurationMode,
                  sizeof(dwAdConfigurationMode));
    BAIL_ON_LSA_ERROR(dwError);

    pszString = (PSTR) pProviderData->szDomain;
    dwError = ADState_WriteRegProviderDataValue(
                  hReg,
                  AD_PROVIDER_REGKEY,
                  AD_PROVIDER_DATA_REGKEY,
                  "Domain",
                  REG_SZ,
                  pszString,
                  strlen(pProviderData->szDomain));
    BAIL_ON_LSA_ERROR(dwError);

    pszString = (PSTR) pProviderData->szShortDomain;
    dwError = ADState_WriteRegProviderDataValue(
                  hReg,
                  AD_PROVIDER_REGKEY,
                  AD_PROVIDER_DATA_REGKEY,
                  "ShortDomain",
                  REG_SZ,
                  pszString,
                  strlen(pProviderData->szShortDomain));
    BAIL_ON_LSA_ERROR(dwError);

    pszString = (PSTR) pProviderData->szComputerDN;
    dwError = ADState_WriteRegProviderDataValue(
                  hReg,
                  AD_PROVIDER_REGKEY,
                  AD_PROVIDER_DATA_REGKEY,
                  "ComputerDN",
                  REG_SZ,
                  pszString,
                  strlen(pProviderData->szComputerDN));
    BAIL_ON_LSA_ERROR(dwError);

    pszString = (PSTR) pProviderData->cell.szCellDN;
    dwError = ADState_WriteRegProviderDataValue(
                  hReg,
                  AD_PROVIDER_REGKEY,
                  AD_PROVIDER_DATA_REGKEY,
                  "CellDN",
                  REG_SZ,
                  pszString,
                  strlen(pProviderData->cell.szCellDN));
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    RegCloseServer(hReg);
    return dwError;

error:
    goto cleanup;
}
