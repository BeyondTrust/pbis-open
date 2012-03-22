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
#include "gpapolicyutils.h"

static
void
GPAFreeGPOObject(
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

    return;
}

void
GPAFreeGPOList(
    PGROUP_POLICY_OBJECT pGPOObject
    )
{
    PGROUP_POLICY_OBJECT pTemp = NULL;

    while (pGPOObject) {

        pTemp = pGPOObject;
        pGPOObject = pGPOObject->pNext;
        GPAFreeGPOObject(pTemp);
    }
    return;
}

BOOLEAN
GPAFindMatch(
    PGROUP_POLICY_OBJECT pGPOObject,
    PGROUP_POLICY_OBJECT pGPOObjectList,
    PGROUP_POLICY_OBJECT *ppGPOMatchedObject,
    BOOLEAN * pfNewVersion
    )
{
    *pfNewVersion = TRUE;

    while (pGPOObjectList)
    {

        if (!strcmp(pGPOObject->pszPolicyDN, pGPOObjectList->pszPolicyDN))
        {
            *ppGPOMatchedObject = pGPOObjectList;

            if (strcmp(pGPOObject->pszDisplayName, pGPOObjectList->pszDisplayName))
            {
                /* Found the GPO by the GUID, but it appears that the name is now different. Need to mark as new */
                *pfNewVersion = TRUE;
            }
            else
            {
                /* Found the GPO by the GUID, now see if there is a version change to note */
                if ( pGPOObject->dwVersion == pGPOObjectList->dwVersion )
                {
                    *pfNewVersion = FALSE;
                }
                else
                {
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

long
GPAComputeDeletedList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pGPOExistingList,
    PGROUP_POLICY_OBJECT * ppGPODeletedList
    )
{
    long macError =  eDSNoErr;
    PGROUP_POLICY_OBJECT pGPODeletedList = NULL;
    PGROUP_POLICY_OBJECT pGPODeletedObject = NULL;
    PGROUP_POLICY_OBJECT pGPOMatchedObject = NULL;

    while (pGPOExistingList) {
        BOOLEAN fMatchFound = FALSE;
        BOOLEAN fNewVersion = FALSE;
        fMatchFound = GPAFindMatch( pGPOExistingList, pGPOCurrentList, &pGPOMatchedObject, &fNewVersion );
        if ( !fMatchFound || fNewVersion ) {
            macError = GPACopyGPOObject(
                pGPOExistingList,
                &pGPODeletedObject
                );
            if (macError)
            {
                goto cleanup;
            }
            pGPODeletedObject->pNext = pGPODeletedList;
            pGPODeletedList = pGPODeletedObject;
        }
        pGPOExistingList = pGPOExistingList->pNext;
    }

    *ppGPODeletedList = pGPODeletedList;

    return macError;

cleanup:

    GPA_SAFE_FREE_GPO_LIST(pGPODeletedList);

    *ppGPODeletedList = NULL;
    return macError;
}

long
GPACopyGPOObject(
    PGROUP_POLICY_OBJECT pGPOObject,
    PGROUP_POLICY_OBJECT *ppGPOCopyObject
    )
{
    long macError = eDSNoErr;
    PGROUP_POLICY_OBJECT pGPOCopyObject = NULL;

    macError = LwAllocateMemory(
        sizeof(GROUP_POLICY_OBJECT),
        (PVOID *)&pGPOCopyObject
        );
    if (macError)
    {
        goto cleanup;
    }

    pGPOCopyObject->dwOptions = pGPOObject->dwOptions;

    pGPOCopyObject->dwVersion = pGPOObject->dwVersion;

    pGPOCopyObject->bNewVersion = pGPOObject->bNewVersion;

    pGPOCopyObject->gPCFunctionalityVersion = pGPOObject->gPCFunctionalityVersion;
    pGPOCopyObject->dwFlags = pGPOObject->dwFlags;

    if (pGPOObject->pszPolicyDN) {
        macError = LwAllocateString(
            pGPOObject->pszPolicyDN,
            &pGPOCopyObject->pszPolicyDN
            );
        if (macError)
        {
            goto cleanup;
        }
    }

    if (pGPOObject->pszDSPath) {
        macError =  LwAllocateString(
            pGPOObject->pszDSPath,
            &pGPOCopyObject->pszDSPath
            );
        if (macError)
        {
            goto cleanup;
        }
    }

    if (pGPOObject->pszDisplayName) {
        macError =  LwAllocateString(
            pGPOObject->pszDisplayName,
            &pGPOCopyObject->pszDisplayName
            );
        if (macError)
        {
            goto cleanup;
        }
    }

    if (pGPOObject->pszgPCFileSysPath) {
        macError =  LwAllocateString(
            pGPOObject->pszgPCFileSysPath,
            &pGPOCopyObject->pszgPCFileSysPath
            );
        if (macError)
        {
            goto cleanup;
        }
    }

    if (pGPOObject->pszgPCMachineExtensionNames) {
        macError =  LwAllocateString(
            pGPOObject->pszgPCMachineExtensionNames,
            &pGPOCopyObject->pszgPCMachineExtensionNames
            );
        if (macError)
        {
            goto cleanup;
        }
    }

    if (pGPOObject->pszgPCUserExtensionNames) {
        macError =  LwAllocateString(
            pGPOObject->pszgPCUserExtensionNames,
            &pGPOCopyObject->pszgPCUserExtensionNames
            );
        if (macError)
        {
            goto cleanup;
        }
    }

    *ppGPOCopyObject = pGPOCopyObject;
    
    return macError;

cleanup:

    GPA_SAFE_FREE_GPO_LIST (pGPOCopyObject);

    *ppGPOCopyObject = NULL;

    return macError;
}

