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


static
VOID
ADUFreeGPOObject(
    PGROUP_POLICY_OBJECT pGPOObject
    )
{
    LW_SAFE_FREE_STRING(pGPOObject->pszPolicyDN);
    LW_SAFE_FREE_STRING(pGPOObject->pszDSPath);
    LW_SAFE_FREE_STRING(pGPOObject->pszDisplayName);
    LW_SAFE_FREE_STRING(pGPOObject->pszgPCFileSysPath);
    LW_SAFE_FREE_STRING(pGPOObject->pszgPCMachineExtensionNames);
    LW_SAFE_FREE_STRING(pGPOObject->pszgPCUserExtensionNames);

    LwFreeMemory(pGPOObject);
}

VOID
ADUFreeGPOList(
    PGROUP_POLICY_OBJECT pGPOObject
    )
{
    PGROUP_POLICY_OBJECT pTemp = NULL;

    while (pGPOObject) {

        pTemp = pGPOObject;
        pGPOObject = pGPOObject->pNext;
        ADUFreeGPOObject(pTemp);
    }
}

static
BOOLEAN
MatchCSEExtension(
    PCSTR pszCSEGUID,
    PSTR  pszCSEExtensions
    )
{
    BOOLEAN fStart = FALSE;
    PSTR pszTmp = NULL;
    int  extLen = 0;
    int  guidLen = 0;

    if ( !pszCSEGUID || !pszCSEExtensions )
    {
        return FALSE;
    }

    pszTmp = pszCSEExtensions;
    extLen = strlen(pszCSEExtensions);
    guidLen = strlen(pszCSEGUID);

    while ( pszTmp && extLen > guidLen )
    {
        switch (pszTmp[0])
        {
        case '[' :
            pszTmp += 1;
            extLen--;
            fStart = TRUE;
            break;

        case '{' :
            if ( fStart && !strncasecmp( pszTmp, pszCSEGUID, guidLen ) )
            {
                return TRUE;
            }
            else
            {
                fStart = FALSE;
                pszTmp += guidLen;
                extLen -= guidLen;
            }
            break;
        case ']' :
            pszTmp += 1;
            extLen--;
            fStart = FALSE;
            break;
        }
    }

    return FALSE;
}

BOOLEAN
ADUFindMatch(
    PGROUP_POLICY_OBJECT pGPOObject,
    PGROUP_POLICY_OBJECT pGPOObjectList,
    PGROUP_POLICY_OBJECT *ppGPOMatchedObject,
    BOOLEAN * pfNewVersion
    )
{
    if (pfNewVersion) {
       *pfNewVersion = TRUE;
    }
    while (pGPOObjectList) {

        if (!strcmp(pGPOObject->pszPolicyDN, pGPOObjectList->pszPolicyDN)){

            *ppGPOMatchedObject = pGPOObjectList;

            if ( pfNewVersion ) {
                if ( pGPOObject->dwVersion == pGPOObjectList->dwVersion ) {
                    *pfNewVersion = FALSE;
                }
                else {
                    *pfNewVersion = TRUE;
                }
            }

            return(TRUE);
        }
        pGPOObjectList = pGPOObjectList->pNext;
    }
    *ppGPOMatchedObject = NULL;
    return(FALSE);
}

static
DWORD
ADUGetAllMCXGPEntries(
    HANDLE hDirectory,
    PCSTR   pszObjectDN,
    PGROUP_POLICY_OBJECT * ppGroupPolicyObjects
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pGPOList = NULL;
    PGROUP_POLICY_OBJECT pTemp = NULL, pPrev = NULL, pDel = NULL;

    dwError = ADUGetAllMCXPolicies(
        hDirectory,
        pszObjectDN,
        &pGPOList
        );
    BAIL_ON_MAC_ERROR(dwError);

    pTemp = pGPOList;
    while (pTemp) {
        dwError =  ADUGetPolicyInformation(
            hDirectory,
            pTemp->pszPolicyDN,
            pTemp
            );
        if (dwError == LDAP_REFERRAL) {
            LOG("LDAP Directory search resulted in LDAP_REFERRAL error, continuing processing...");

            /* Remove GPO from list and resume... */
            pDel = pTemp;
            if (pPrev) {
                /* Maintain a pointer to the node before */
                pPrev->pNext = pTemp->pNext;
            } else {
                /* Are we affecting the first node in our list? */
                if ( pGPOList == pTemp ) {
                    pGPOList = pTemp->pNext;
                }
            }
            pTemp = pTemp->pNext;
            pDel->pNext = NULL;
            ADUFreeGPOObject(pDel);
            dwError = MAC_AD_ERROR_SUCCESS;
            continue;
        }
        BAIL_ON_MAC_ERROR(dwError);

        pPrev = pTemp;
        pTemp = pTemp->pNext;
    }
    
    dwError = ADUReverseGPOList(pGPOList, &pGPOList);
    BAIL_ON_MAC_ERROR(dwError);
    
    *ppGroupPolicyObjects = pGPOList;
    pGPOList = NULL;
    return dwError;

cleanup:

    ADU_SAFE_FREE_GPO_LIST( pGPOList );

    *ppGroupPolicyObjects = NULL;

    return dwError;

error:

    if (ppGroupPolicyObjects)
        *ppGroupPolicyObjects = NULL;

    goto cleanup;
}

static
DWORD
ADUGetMCXGPEntry(
    HANDLE hDirectory,
    PCSTR   pszObjectDN,
    PCSTR   pszGPOName,
    PGROUP_POLICY_OBJECT * ppGPO
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pGPO = NULL;

    dwError = ADUGetMCXPolicy(
        hDirectory,
        pszObjectDN,
        pszGPOName,
        &pGPO
        );
    BAIL_ON_MAC_ERROR(dwError);

    if (pGPO) {
        dwError = ADUGetPolicyInformation(hDirectory,
                                          pGPO->pszPolicyDN,
                                          pGPO);
        if (dwError == LDAP_REFERRAL) {
            LOG("LDAP Directory search resulted in LDAP_REFERRAL error, continuing processing...");

            ADUFreeGPOObject(pGPO);
            dwError = MAC_AD_ERROR_SUCCESS;
        }
        BAIL_ON_MAC_ERROR(dwError);
    }
    
    *ppGPO = pGPO;
    pGPO = NULL;

cleanup:

    ADU_SAFE_FREE_GPO_LIST( pGPO );

    return dwError;

error:

    if (ppGPO)
        *ppGPO = NULL;

    goto cleanup;
}

DWORD
ADUComputeDeletedList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pGPOExistingList,
    PGROUP_POLICY_OBJECT * ppGPODeletedList
    )
{
    DWORD dwError =  MAC_AD_ERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pGPODeletedList = NULL;
    PGROUP_POLICY_OBJECT pGPODeletedObject = NULL;
    PGROUP_POLICY_OBJECT pGPOMatchedObject = NULL;

    while (pGPOExistingList) {
        BOOLEAN fMatchFound = FALSE;
        BOOLEAN fNewVersion = FALSE;
        fMatchFound = ADUFindMatch( pGPOExistingList, pGPOCurrentList, &pGPOMatchedObject, &fNewVersion );
        if ( !fMatchFound || fNewVersion ) {
            dwError = ADUCopyGPOObject(
                pGPOExistingList,
                &pGPODeletedObject
                );
            BAIL_ON_MAC_ERROR(dwError);
            pGPODeletedObject->pNext = pGPODeletedList;
            pGPODeletedList = pGPODeletedObject;
        }
        pGPOExistingList = pGPOExistingList->pNext;
    }

    *ppGPODeletedList = pGPODeletedList;

    return dwError;

cleanup:

    ADU_SAFE_FREE_GPO_LIST(pGPODeletedList);

    *ppGPODeletedList = NULL;
    return dwError;

error:

    if (ppGPODeletedList)
        *ppGPODeletedList = NULL;

    goto cleanup;
}

DWORD
ADUComputeModifiedList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pGPOExistingList,
    PGROUP_POLICY_OBJECT * ppGPOModifiedList
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    BOOLEAN fMatchFound = FALSE;
    BOOLEAN fNewVersion = FALSE;
    PGROUP_POLICY_OBJECT pGPOModifiedList = NULL;
    PGROUP_POLICY_OBJECT pGPOModifiedObject = NULL;
    PGROUP_POLICY_OBJECT pGPOMatchedObject = NULL;

    while (pGPOCurrentList) {
        if (!pGPOExistingList) {
	   fMatchFound = FALSE;
           fNewVersion = TRUE;
        } else {
           fMatchFound = ADUFindMatch( pGPOCurrentList,
                                       pGPOExistingList,
                                       &pGPOMatchedObject,
                                       &fNewVersion );
        }
        dwError = ADUCopyGPOObject(
                pGPOCurrentList,
                &pGPOModifiedObject
                );
       BAIL_ON_MAC_ERROR(dwError);
       pGPOModifiedObject->bNewVersion = fNewVersion;
       pGPOModifiedObject->pNext = pGPOModifiedList;
       pGPOModifiedList = pGPOModifiedObject;

       pGPOCurrentList = pGPOCurrentList->pNext;
    }

    dwError = ADUReverseGPOList(
        pGPOModifiedList,
        &pGPOModifiedList
        );
    BAIL_ON_MAC_ERROR(dwError);

    *ppGPOModifiedList = pGPOModifiedList;
    pGPOModifiedList = NULL;

cleanup:

    ADU_SAFE_FREE_GPO_LIST(pGPOModifiedList);

    return dwError;

error:

    if (ppGPOModifiedList)
        *ppGPOModifiedList = NULL;

    goto cleanup;
}

DWORD
ADUGetAllMCXGPOList(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    PGROUP_POLICY_OBJECT * ppGPOList
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    /*LDAPMessage *pResults = NULL;*/
    PGROUP_POLICY_OBJECT pGroupPolicyObjects = NULL;

    if (!pszObjectDN || !*pszObjectDN) {
        return(MAC_AD_ERROR_INVALID_PARAMETER);
    }

    dwError = ADUGetAllMCXGPEntries(
        hDirectory,
        pszObjectDN,
        &pGroupPolicyObjects);
    BAIL_ON_MAC_ERROR(dwError);

    *ppGPOList = pGroupPolicyObjects;
    pGroupPolicyObjects = NULL;

cleanup:

    ADU_SAFE_FREE_GPO_LIST(pGroupPolicyObjects);

    return dwError;

error:

    if (ppGPOList)
        *ppGPOList = NULL;

    goto cleanup;
}

DWORD
ADUGetMCXGPO(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    PCSTR  pszGPOName,
    PGROUP_POLICY_OBJECT * ppGPO
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pGPO = NULL;

    if (!pszObjectDN || !*pszObjectDN || !pszGPOName || !*pszGPOName) {
        return(MAC_AD_ERROR_INVALID_PARAMETER);
    }

    dwError = ADUGetMCXGPEntry(
        hDirectory,
        pszObjectDN,
        pszGPOName,
        &pGPO);
    BAIL_ON_MAC_ERROR(dwError);

    LOG("pszObjectDN: %s pszGPOName: %s pGPO=[%.8x]", pszObjectDN, pszGPOName, pGPO);

    *ppGPO = pGPO;
    pGPO = NULL;

cleanup:

    ADU_SAFE_FREE_GPO_LIST(pGPO);

    return dwError;

error:

    if (ppGPO)
        *ppGPO = NULL;

    goto cleanup;
}


DWORD
ADUPrependGPList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pPrependGPOList,
    PGROUP_POLICY_OBJECT * ppGPOCurrentList
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pTemp = NULL;

    if (!pPrependGPOList) {
        *ppGPOCurrentList = pGPOCurrentList;
        return (dwError);
    }

    pTemp = pPrependGPOList;

    while(pPrependGPOList->pNext != NULL) {
        pPrependGPOList = pPrependGPOList->pNext;
    }
    pPrependGPOList->pNext = pGPOCurrentList;

    *ppGPOCurrentList = pTemp;

    return dwError;
}

DWORD
ADUReverseGPOList(
    PGROUP_POLICY_OBJECT pGPOList,
    PGROUP_POLICY_OBJECT *ppGPOList
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pTemp = NULL;
    PGROUP_POLICY_OBJECT pGPONewList = NULL;

    while (pGPOList){
        pTemp = pGPOList;
        pGPOList = pTemp->pNext;

        pTemp->pNext = pGPONewList;
        pGPONewList = pTemp;
    }
    *ppGPOList = pGPONewList;

    return dwError;
}

DWORD
ADUComputeModDelGroupPolicyList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pGPOExistingList,
    PGROUP_POLICY_OBJECT *ppGPOModifiedList,
    PGROUP_POLICY_OBJECT *ppGPODeletedList
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pGPODeletedList = NULL;
    PGROUP_POLICY_OBJECT pGPOModifiedList = NULL;

    dwError = ADUComputeDeletedList(
        pGPOCurrentList,
        pGPOExistingList,
        &pGPODeletedList
        );
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUComputeModifiedList(
        pGPOCurrentList,
        pGPOExistingList,
        &pGPOModifiedList
        );
    BAIL_ON_MAC_ERROR(dwError);

    *ppGPODeletedList =  pGPODeletedList;
    *ppGPOModifiedList = pGPOModifiedList;

    return dwError;

cleanup:

    ADU_SAFE_FREE_GPO_LIST (pGPODeletedList);
    ADU_SAFE_FREE_GPO_LIST (pGPOModifiedList);

    *ppGPODeletedList = NULL;
    *ppGPOModifiedList = NULL;

    return dwError;

error:

    if (ppGPODeletedList)
        *ppGPODeletedList = NULL;

    goto cleanup;
}

DWORD
ADUCopyGPOObject(
    PGROUP_POLICY_OBJECT pGPOObject,
    PGROUP_POLICY_OBJECT *ppGPOCopyObject
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pGPOCopyObject = NULL;

    dwError = LwAllocateMemory(
        sizeof(GROUP_POLICY_OBJECT),
        (PVOID *)&pGPOCopyObject
        );
    BAIL_ON_MAC_ERROR(dwError);

    pGPOCopyObject->dwOptions = pGPOObject->dwOptions;

    pGPOCopyObject->dwVersion = pGPOObject->dwVersion;

    pGPOCopyObject->bNewVersion = pGPOObject->bNewVersion;

    pGPOCopyObject->gPCFunctionalityVersion = pGPOObject->gPCFunctionalityVersion;
    pGPOCopyObject->dwFlags = pGPOObject->dwFlags;

    if (pGPOObject->pszPolicyDN) {
        dwError = LwAllocateString(
            pGPOObject->pszPolicyDN,
            &pGPOCopyObject->pszPolicyDN
            );
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pGPOObject->pszDSPath) {
        dwError =  LwAllocateString(
            pGPOObject->pszDSPath,
            &pGPOCopyObject->pszDSPath
            );
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pGPOObject->pszDisplayName) {
        dwError =  LwAllocateString(
            pGPOObject->pszDisplayName,
            &pGPOCopyObject->pszDisplayName
            );
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pGPOObject->pszgPCFileSysPath) {
        dwError =  LwAllocateString(
            pGPOObject->pszgPCFileSysPath,
            &pGPOCopyObject->pszgPCFileSysPath
            );
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pGPOObject->pszgPCMachineExtensionNames) {
        dwError =  LwAllocateString(
            pGPOObject->pszgPCMachineExtensionNames,
            &pGPOCopyObject->pszgPCMachineExtensionNames
            );
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pGPOObject->pszgPCUserExtensionNames) {
        dwError =  LwAllocateString(
            pGPOObject->pszgPCUserExtensionNames,
            &pGPOCopyObject->pszgPCUserExtensionNames
            );
        BAIL_ON_MAC_ERROR(dwError);
    }

    *ppGPOCopyObject = pGPOCopyObject;
    
    return dwError;

cleanup:
    ADU_SAFE_FREE_GPO_LIST (pGPOCopyObject);

    *ppGPOCopyObject = NULL;

    return dwError;

error:

    if (ppGPOCopyObject)
        *ppGPOCopyObject = NULL;

    goto cleanup;
}

DWORD
ADUComputeCSEList(
    DWORD dwCSEType,
    PCSTR pszCSEGUID,
    PGROUP_POLICY_OBJECT  pGPOList,
    PGROUP_POLICY_OBJECT * ppGPOCSEList
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pGPOCSEList = NULL;
    PGROUP_POLICY_OBJECT pGPOCopyObject = NULL;
    PSTR pszCSEExtensions = NULL;

    while (pGPOList) {

        if (dwCSEType == MACHINE_GROUP_POLICY) {
            pszCSEExtensions = pGPOList->pszgPCMachineExtensionNames;
        }else {
            pszCSEExtensions = pGPOList->pszgPCUserExtensionNames;
        }

        if (MatchCSEExtension(pszCSEGUID, pszCSEExtensions)) {

            dwError = ADUCopyGPOObject(pGPOList, &pGPOCopyObject);
            BAIL_ON_MAC_ERROR(dwError);

            pGPOCopyObject->pNext = pGPOCSEList;
            pGPOCSEList = pGPOCopyObject;
        }

        pGPOList = pGPOList->pNext;
    }

    dwError = ADUReverseGPOList( pGPOCSEList, &pGPOCSEList );
    BAIL_ON_MAC_ERROR(dwError);

    *ppGPOCSEList = pGPOCSEList;

    return dwError;

cleanup:

    ADU_SAFE_FREE_GPO_LIST (pGPOCSEList);

    *ppGPOCSEList = pGPOCSEList;

    return dwError;

error:

    if (ppGPOCSEList)
        *ppGPOCSEList = NULL;

    goto cleanup;
}

DWORD
ADUComputeCSEModDelList(
    DWORD dwCSEType,
    PSTR pszCSEGUID,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList,
    PGROUP_POLICY_OBJECT *ppGPOCSEModList,
    PGROUP_POLICY_OBJECT *ppGPOCSEDelList
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;

    dwError = ADUComputeCSEList(
        dwCSEType,
        pszCSEGUID,
        pGPOModifiedList,
        ppGPOCSEModList
        );
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUComputeCSEList(
        dwCSEType,
        pszCSEGUID,
        pGPODeletedList,
        ppGPOCSEDelList
        );
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    return dwError;

error:

    if (ppGPOCSEModList)
        *ppGPOCSEModList = NULL;

    if (ppGPOCSEDelList)
        *ppGPOCSEDelList = NULL;

    goto cleanup;
}

