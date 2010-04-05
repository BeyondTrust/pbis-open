
#include "gpldap.h"
#include "LWIStruct.h"
#include "gpapolicyutils.h"
#include "Utilities.h"
#include "PlugInShell.h"
#include <DirectoryService/DirServicesTypes.h>

static
VOID
GPAFreeGPOObject(
    PGROUP_POLICY_OBJECT pGPOObject
    )
{
    CT_SAFE_FREE_STRING(pGPOObject->pszPolicyDN);
    CT_SAFE_FREE_STRING(pGPOObject->pszDSPath);
    CT_SAFE_FREE_STRING(pGPOObject->pszDisplayName);
    CT_SAFE_FREE_STRING(pGPOObject->pszgPCFileSysPath);
    CT_SAFE_FREE_STRING(pGPOObject->pszgPCMachineExtensionNames);
    CT_SAFE_FREE_STRING(pGPOObject->pszgPCUserExtensionNames);

    LWFreeMemory(pGPOObject);

    return;
}

VOID
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
            GOTO_CLEANUP_ON_MACERROR(macError);
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

    macError = LWAllocateMemory(
        sizeof(GROUP_POLICY_OBJECT),
        (PVOID *)&pGPOCopyObject
        );
    GOTO_CLEANUP_ON_MACERROR(macError);

    pGPOCopyObject->dwOptions = pGPOObject->dwOptions;

    pGPOCopyObject->dwVersion = pGPOObject->dwVersion;

    pGPOCopyObject->bNewVersion = pGPOObject->bNewVersion;

    pGPOCopyObject->gPCFunctionalityVersion = pGPOObject->gPCFunctionalityVersion;
    pGPOCopyObject->dwFlags = pGPOObject->dwFlags;

    if (pGPOObject->pszPolicyDN) {
        macError = LWAllocateString(
            pGPOObject->pszPolicyDN,
            &pGPOCopyObject->pszPolicyDN
            );
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pGPOObject->pszDSPath) {
        macError =  LWAllocateString(
            pGPOObject->pszDSPath,
            &pGPOCopyObject->pszDSPath
            );
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pGPOObject->pszDisplayName) {
        macError =  LWAllocateString(
            pGPOObject->pszDisplayName,
            &pGPOCopyObject->pszDisplayName
            );
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pGPOObject->pszgPCFileSysPath) {
        macError =  LWAllocateString(
            pGPOObject->pszgPCFileSysPath,
            &pGPOCopyObject->pszgPCFileSysPath
            );
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pGPOObject->pszgPCMachineExtensionNames) {
        macError =  LWAllocateString(
            pGPOObject->pszgPCMachineExtensionNames,
            &pGPOCopyObject->pszgPCMachineExtensionNames
            );
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pGPOObject->pszgPCUserExtensionNames) {
        macError =  LWAllocateString(
            pGPOObject->pszgPCUserExtensionNames,
            &pGPOCopyObject->pszgPCUserExtensionNames
            );
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    *ppGPOCopyObject = pGPOCopyObject;
    
    return macError;

cleanup:
    GPA_SAFE_FREE_GPO_LIST (pGPOCopyObject);

    *ppGPOCopyObject = NULL;

    return macError;
}

