#include "includes.h"

static
VOID
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

static
BOOLEAN
MatchCSEExtension(
    PSTR pszCSEGUID,
    PSTR pszCSEExtensions
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

static
BOOLEAN
GPAFindMatch(
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
CENTERROR
GPAGetGPEntries(
    PGPUSER pUser,
    HANDLE hDirectory,
    PSTR   pszObjectDN,
    BOOLEAN bAddEnforcedOnly,
    PGROUP_POLICY_OBJECT * ppGroupPolicyObjects,
    DWORD * pdwOptions
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pGPOList = NULL;
    PGROUP_POLICY_OBJECT pTemp = NULL, pPrev = NULL, pDel = NULL;
    DWORD dwOptions = 0;

    ceError = GPAGetPolicyLinkInfo(
        hDirectory,
        pszObjectDN,
        bAddEnforcedOnly, /* indicates that we are blocking inheritance */
        &pGPOList,
        &dwOptions /* indicates subsequent GPOs must be filtered for ones that are enforced */
        );
    BAIL_ON_CENTERIS_ERROR(ceError);

    pTemp = pGPOList;
    while (pTemp) {
        ceError =  GPAGetPolicyInformation(
            pUser,
            hDirectory,
            pTemp->pszPolicyDN,
            pTemp
            );
        if (CENTERROR_EQUAL(ceError, LDAP_REFERRAL) ||
            CENTERROR_EQUAL(ceError, CENTERROR_GP_LDAP_NO_VALUE_FOUND)) {
            if (CENTERROR_EQUAL(ceError, LDAP_REFERRAL)) {
                GPA_LOG_VERBOSE("LDAP Directory search resulted in LDAP_REFERRAL error, going to skip this GPO (%s) and continuing processing...", pTemp->pszPolicyDN);
            } else {
                GPA_LOG_VERBOSE("LDAP query resulted in an error reading required attributes for GPO (%s), going to skip this GPO and continuing processing...", pTemp->pszPolicyDN);
            }

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
            GPAFreeGPOObject(pDel);
            ceError = CENTERROR_SUCCESS;
            continue;
        }
        BAIL_ON_CENTERIS_ERROR(ceError);

        pPrev = pTemp;
        pTemp = pTemp->pNext;
    }

    *ppGroupPolicyObjects = pGPOList;
    *pdwOptions = dwOptions;
    return(ceError);

error:
    GPA_SAFE_FREE_GPO_LIST( pGPOList );

    *ppGroupPolicyObjects = NULL;

    return(ceError);
}

static
CENTERROR
GPAComputeDeletedList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pGPOExistingList,
    PGROUP_POLICY_OBJECT * ppGPODeletedList
    )
{
    CENTERROR ceError =  CENTERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pGPODeletedList = NULL;
    PGROUP_POLICY_OBJECT pGPODeletedObject = NULL;
    PGROUP_POLICY_OBJECT pGPOMatchedObject = NULL;

    while (pGPOExistingList) {
        BOOLEAN fMatchFound = FALSE;
        BOOLEAN fNewVersion = FALSE;
        fMatchFound = GPAFindMatch( pGPOExistingList, pGPOCurrentList, &pGPOMatchedObject, &fNewVersion );
        if ( !fMatchFound || fNewVersion ) {
            ceError = GPACopyGPOObject(
                pGPOExistingList,
                &pGPODeletedObject
                );
            BAIL_ON_CENTERIS_ERROR(ceError);
            pGPODeletedObject->pNext = pGPODeletedList;
            pGPODeletedList = pGPODeletedObject;
        }
        pGPOExistingList = pGPOExistingList->pNext;
    }

    *ppGPODeletedList = pGPODeletedList;

    return(ceError);

error:

    GPA_SAFE_FREE_GPO_LIST(pGPODeletedList);

    *ppGPODeletedList = NULL;
    return(ceError);
}

static
CENTERROR
GPAComputeModifiedList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pGPOExistingList,
    PGROUP_POLICY_OBJECT * ppGPOModifiedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
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
           fMatchFound = GPAFindMatch( pGPOCurrentList,
                                       pGPOExistingList,
                                       &pGPOMatchedObject,
                                       &fNewVersion );
        }
        ceError = GPACopyGPOObject(
                pGPOCurrentList,
                &pGPOModifiedObject
                );
       BAIL_ON_CENTERIS_ERROR(ceError);
       pGPOModifiedObject->bNewVersion = fNewVersion;
       pGPOModifiedObject->pNext = pGPOModifiedList;
       pGPOModifiedList = pGPOModifiedObject;

       pGPOCurrentList = pGPOCurrentList->pNext;
    }

    ceError = GPAReverseGPOList(
        pGPOModifiedList,
        &pGPOModifiedList
        );
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppGPOModifiedList = pGPOModifiedList;
    pGPOModifiedList = NULL;

error:

    GPA_SAFE_FREE_GPO_LIST(pGPOModifiedList);

    return(ceError);
}

CENTERROR
GPAGetGPOList(
    PGPUSER pUser,
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    PGROUP_POLICY_OBJECT * ppGPOList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    /*LDAPMessage *pResults = NULL;*/
    PSTR pszChildDN = NULL;
    PSTR pszParentDN = NULL;
    PGROUP_POLICY_OBJECT pGPOPrependList = NULL;
    PGROUP_POLICY_OBJECT pGroupPolicyObjects = NULL;
    BOOLEAN bAddEnforcedOnly = FALSE;
    DWORD dwOptions = 0;

    if (!pszObjectDN || !*pszObjectDN) {
        return(CENTERROR_INVALID_PARAMETER);
    }

    ceError = LwAllocateString(
        pszObjectDN,
        &pszChildDN
        );
    BAIL_ON_CENTERIS_ERROR(ceError);

    while(pszChildDN) {

        ceError = GPAGetGPEntries(
            pUser,
            hDirectory,
            pszChildDN,
            bAddEnforcedOnly,
            &pGPOPrependList,
            &dwOptions
            );
        BAIL_ON_CENTERIS_ERROR(ceError);

        GPA_LOG_VERBOSE("pszChildDN: %s%s pCurrentGroupPolicyObjects=[%.8x], dwOptions=%d", pszChildDN, bAddEnforcedOnly ? "[FILTERED]" : "", pGPOPrependList, dwOptions);

        ceError = GPAPrependGPList(
            pGroupPolicyObjects,
            pGPOPrependList,
            &pGroupPolicyObjects
            );
        BAIL_ON_CENTERIS_ERROR(ceError);

        /* Now test to see if we are going to block inheritance from higher DNs */
        if (dwOptions == 1) {
            bAddEnforcedOnly = TRUE;
        }

        if (!strncasecmp(pszChildDN, DC_PREFIX, sizeof(DC_PREFIX)-1 )) {
            break;
        }

        ceError = GPOGetParentDN(
            pszChildDN,
            &pszParentDN
            );
        if (ceError != CENTERROR_GP_NO_PARENT_DN) {
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            ceError = CENTERROR_SUCCESS;
            pszParentDN = NULL;
        }

        if(pszChildDN) {
            LwFreeString(pszChildDN);
        }

        pszChildDN = pszParentDN;
    }

    if (pszChildDN) {
        LwFreeString(pszChildDN);
    }

    *ppGPOList = pGroupPolicyObjects;

    return(ceError);

error:

    if (pszChildDN) {
        LwFreeString(pszChildDN);
    }

    GPA_SAFE_FREE_GPO_LIST(pGroupPolicyObjects);

    *ppGPOList = NULL;

    return(ceError);
}

CENTERROR
GPAPrependGPList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pPrependGPOList,
    PGROUP_POLICY_OBJECT * ppGPOCurrentList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pTemp = NULL;

    if (!pPrependGPOList) {
        *ppGPOCurrentList = pGPOCurrentList;
        return (ceError);
    }

    pTemp = pPrependGPOList;

    while(pPrependGPOList->pNext != NULL) {
        pPrependGPOList = pPrependGPOList->pNext;
    }
    pPrependGPOList->pNext = pGPOCurrentList;

    *ppGPOCurrentList = pTemp;
    return(ceError);
}

CENTERROR
GPAReverseGPOList(
    PGROUP_POLICY_OBJECT pGPOList,
    PGROUP_POLICY_OBJECT *ppGPOList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pTemp = NULL;
    PGROUP_POLICY_OBJECT pGPONewList = NULL;

    while (pGPOList){
        pTemp = pGPOList;
        pGPOList = pTemp->pNext;

        pTemp->pNext = pGPONewList;
        pGPONewList = pTemp;
    }
    *ppGPOList = pGPONewList;

    return(ceError);
}

CENTERROR
GPAComputeModDelGroupPolicyList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pGPOExistingList,
    PGROUP_POLICY_OBJECT *ppGPOModifiedList,
    PGROUP_POLICY_OBJECT *ppGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pGPODeletedList = NULL;
    PGROUP_POLICY_OBJECT pGPOModifiedList = NULL;

    ceError = GPAComputeDeletedList(
        pGPOCurrentList,
        pGPOExistingList,
        &pGPODeletedList
        );
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAComputeModifiedList(
        pGPOCurrentList,
        pGPOExistingList,
        &pGPOModifiedList
        );
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppGPODeletedList =  pGPODeletedList;
    *ppGPOModifiedList = pGPOModifiedList;

    return(ceError);

error:

    GPA_SAFE_FREE_GPO_LIST (pGPODeletedList);
    GPA_SAFE_FREE_GPO_LIST (pGPOModifiedList);

    *ppGPODeletedList = NULL;
    *ppGPOModifiedList = NULL;

    return(ceError);
}

CENTERROR
GPACopyGPOObject(
    PGROUP_POLICY_OBJECT pGPOObject,
    PGROUP_POLICY_OBJECT *ppGPOCopyObject
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pGPOCopyObject = NULL;

    ceError = LwAllocateMemory(
        sizeof(GROUP_POLICY_OBJECT),
        (PVOID *)&pGPOCopyObject
        );
    BAIL_ON_CENTERIS_ERROR(ceError);

    pGPOCopyObject->dwOptions = pGPOObject->dwOptions;

    pGPOCopyObject->dwVersion = pGPOObject->dwVersion;

    pGPOCopyObject->bNewVersion = pGPOObject->bNewVersion;

    pGPOCopyObject->gPCFunctionalityVersion = pGPOObject->gPCFunctionalityVersion;
    pGPOCopyObject->dwFlags = pGPOObject->dwFlags;

    if (pGPOObject->pszPolicyDN) {
        ceError = LwAllocateString(
            pGPOObject->pszPolicyDN,
            &pGPOCopyObject->pszPolicyDN
            );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pGPOObject->pszDSPath) {
        ceError =  LwAllocateString(
            pGPOObject->pszDSPath,
            &pGPOCopyObject->pszDSPath
            );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pGPOObject->pszDisplayName) {
        ceError =  LwAllocateString(
            pGPOObject->pszDisplayName,
            &pGPOCopyObject->pszDisplayName
            );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pGPOObject->pszgPCFileSysPath) {
        ceError =  LwAllocateString(
            pGPOObject->pszgPCFileSysPath,
            &pGPOCopyObject->pszgPCFileSysPath
            );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pGPOObject->pszgPCMachineExtensionNames) {
        ceError =  LwAllocateString(
            pGPOObject->pszgPCMachineExtensionNames,
            &pGPOCopyObject->pszgPCMachineExtensionNames
            );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pGPOObject->pszgPCUserExtensionNames) {
        ceError =  LwAllocateString(
            pGPOObject->pszgPCUserExtensionNames,
            &pGPOCopyObject->pszgPCUserExtensionNames
            );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *ppGPOCopyObject = pGPOCopyObject;
    
    return ceError;

error:
    GPA_SAFE_FREE_GPO_LIST (pGPOCopyObject);

    *ppGPOCopyObject = NULL;

    return(ceError);
}

static
CENTERROR
GPAComputeCSEList(
    DWORD dwCSEType,
    PSTR  pszCSEGUID,
    PGROUP_POLICY_OBJECT  pGPOList,
    PGROUP_POLICY_OBJECT * ppGPOCSEList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
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

            ceError = GPACopyGPOObject(pGPOList, &pGPOCopyObject);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pGPOCopyObject->pNext = pGPOCSEList;
            pGPOCSEList = pGPOCopyObject;
        }

        pGPOList = pGPOList->pNext;
    }

    ceError = GPAReverseGPOList( pGPOCSEList, &pGPOCSEList );
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppGPOCSEList = pGPOCSEList;

    return(ceError);

error:

    GPA_SAFE_FREE_GPO_LIST (pGPOCSEList);

    *ppGPOCSEList = pGPOCSEList;

    return(ceError);
}

CENTERROR
GPAComputeCSEModDelList(
    DWORD dwCSEType,
    PSTR pszCSEGUID,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList,
    PGROUP_POLICY_OBJECT *ppGPOCSEModList,
    PGROUP_POLICY_OBJECT *ppGPOCSEDelList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = GPAComputeCSEList(
        dwCSEType,
        pszCSEGUID,
        pGPOModifiedList,
        ppGPOCSEModList
        );
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAComputeCSEList(
        dwCSEType,
        pszCSEGUID,
        pGPODeletedList,
        ppGPOCSEDelList
        );
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return(ceError);
}

void
GPAPostCSEEvent(
    CENTERROR ceError,
    PGPUSER pUserInfo,
    PSTR szCSEName,
    PSTR szCSEDllName,
    PGROUP_POLICY_OBJECT pGPOCSEModList,
    PGROUP_POLICY_OBJECT pGPOCSEDelList
    )
{
    char description[2000];
    PGROUP_POLICY_OBJECT pList = pGPOCSEModList;
    BOOLEAN IsGPOChanged = FALSE;
    PSTR pszUser = NULL;

    while(pList)
    {
        if (pList->bNewVersion)
        {
            IsGPOChanged = TRUE;
        }
        pList = pList->pNext;
    }

    if (pGPOCSEDelList)
    {
        IsGPOChanged = TRUE;
    }

    sprintf(description, "Group Policy update %s.\r\n\r\n" \
                         "     Client-Side Extension:   %s\r\n" \
                         "     CSE library name :       %s\r\n",
               CENTERROR_IS_OK(ceError) ? "succeeded" : "failed",
               szCSEName,
               szCSEDllName);

    strcat(description, "     Policy type:             ");
    if (pUserInfo)
    {
        strcat(description, "User\r\n\r\n");
        pszUser = pUserInfo->pszName;
    }
    else
    {
        strcat(description, "Computer\r\n\r\n");
    }

    strcat(description, "     Changes applied...\r\n\r\n");

    if (pGPOCSEDelList)
    {
        strcat(description, "     Operation:               Removed settings\r\n");
        strcat(description, "     GPO(s):\r\n");
        while(pGPOCSEDelList)
        {
            strcat(description, "                              ");
            strcat(description, pGPOCSEDelList->pszDisplayName);
            strcat(description, "\r\n");
            pGPOCSEDelList = pGPOCSEDelList->pNext;
        }
    }

    if (pGPOCSEModList)
    {
        strcat(description, "     Operation:               Added settings\r\n");
        strcat(description, "     GPO(s):\r\n");
        while(pGPOCSEModList)
        {
            if (pGPOCSEModList->bNewVersion)
            {
                strcat(description, "                              ");
                strcat(description, pGPOCSEModList->pszDisplayName);
                strcat(description, "\r\n");
            }
            pGPOCSEModList = pGPOCSEModList->pNext;
        }
    }

    if (IsGPOChanged)
    {
        if (!CENTERROR_IS_OK(ceError)) 
        {
            GPO_FAILURE_EVENT(GPAGENT_EVENT_POLICY_UPDATE_FAILURE, pszUser, description, ceError);
        }
        else
        {
            GPO_SUCCESS_EVENT(GPAGENT_EVENT_POLICY_UPDATED, pszUser, description);
        }
    }
}

static CENTERROR KnownEventIssue[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void
GPAPostAgentEvent(
    CENTERROR ceError,
    int       issue,
    PSTR      szMessage
    )
{
    char description[2000];

    if (issue > 20) {
        GPA_LOG_ERROR("GPAPostAgentEvent called with invalid issue number");
        return;
    }

    if (KnownEventIssue[issue] == ceError) {
        return;
    }

    KnownEventIssue[issue] = ceError;

    if (!CENTERROR_IS_OK(ceError)) {
        // Track the new issue.
        KnownEventIssue[issue] = ceError;

        sprintf(description, "Policy processing issue encountered\r\n\r\n" \
                             "     Issue:                   An error occurred while %s",
                szMessage);

        GPA_ERROR_EVENT(GPAGENT_EVENT_ERROR_POLICY_PROCESSING_ISSUE_ENCOUNTERED, description, ceError);
    } else {
        sprintf(description, "Policy processing issue resolved\r\n\r\n" \
                             "     Previous issue:          An error occurred while %s\r\n" \
                             "     Status:                  No longer an issue.\r\n" \
                             "     Original error code:     %d\r\n" \
                             "     Original error name:     %s",
               szMessage,
               KnownEventIssue[issue],
               CTErrorName(KnownEventIssue[issue]));

        // Now clear the prior issue.
        KnownEventIssue[issue] = ceError;

        GPA_INFO_EVENT(GPAGENT_EVENT_INFO_POLICY_PROCESSING_ISSUE_RESOLVED, description);
    }
}
