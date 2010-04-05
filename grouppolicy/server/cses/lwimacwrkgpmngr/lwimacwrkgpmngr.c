#include "includes.h"

static
PSTR
GetClientGUID(
    DWORD dwPolicyType
    )
{
    if (dwPolicyType == MACHINE_GROUP_POLICY)
        return LWIMACWRKGPMNGR_MACHINE_CLIENT_GUID;
    else
        return LWIMACWRKGPMNGR_USER_CLIENT_GUID;
}

static
PSTR
GetSysvolPath(
    DWORD dwPolicyType
    )
{
    if (dwPolicyType == MACHINE_GROUP_POLICY)
        return LIKEWISE_COMPUTER_SYSVOL_PATH;
    else
        return LIKEWISE_USER_SYSVOL_PATH;
}

static
PSTR
GetItemGUID(
    DWORD dwPolicyType
    )
{
    if (dwPolicyType == MACHINE_GROUP_POLICY)
        return LWIMACWRKGPMNGR_MACHINE_ITEM_GUID;
    else
        return LWIMACWRKGPMNGR_USER_ITEM_GUID;
}

static
DWORD
GetPolicyType(
    PGPUSER pUser
    )
{
    if (pUser)
        return USER_GROUP_POLICY; 
    else 
        return MACHINE_GROUP_POLICY; 
}

static
CENTERROR
GetMCXFilePath(
    PGPUSER pUser,
    PSTR *ppszFileDir,
    PSTR *ppszFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID*)ppszFileDir);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID*)ppszFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pUser) {
        sprintf( *ppszFileDir,
                 "%s%ld",
                 LWE_MAC_USER_MCX_DIR,
                 (long) pUser->uid);

        sprintf( *ppszFilePath,
                 "%s%ld/%s",
                 LWE_MAC_USER_MCX_DIR,
                 (long) pUser->uid,
                 LWE_MAC_USER_MCX_FILE);
    }
    else {
        sprintf( *ppszFileDir,
                 "%s",
                 LWE_MAC_MACHINE_MCX_DIR);

        sprintf( *ppszFilePath,
                 "%s",
                 LWE_MAC_MACHINE_MCX_FILEPATH);
    }

error:

    return ceError;
}

static
CENTERROR
AddPolicyFileToHierarchy(
    PMAC_WRKGPMNGRLIST * ppGPOHierarchyList,
    PSTR pszDisplayName,
    PSTR pszGUID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PMAC_WRKGPMNGRLIST pTemp = *ppGPOHierarchyList;
    PMAC_WRKGPMNGRLIST pPrev = NULL;
    PMAC_WRKGPMNGRLIST pNew = NULL;

    while (pTemp) {
        if (!strcmp(pTemp->pszDisplayName,
                    pszDisplayName) &&
            !strcmp(pTemp->pszGUID,
                    pszGUID)) {
            goto done;
        }
        pTemp = pTemp->pNext;
    }
 
    pTemp = *ppGPOHierarchyList;

    ceError = LwAllocateMemory( sizeof(MAC_WRKGPMNGRLIST),
                                (PVOID*)&pNew);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateStringPrintf( &pNew->pszDisplayName,
                                      "%s",
                                      pszDisplayName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateStringPrintf( &pNew->pszGUID,
                                      "{%s}",
                                      pszGUID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while (pTemp) {
        pPrev = pTemp;
        pTemp = pTemp->pNext;
    }

    if (pPrev) {
        pPrev->pNext = pNew;
    } else {
        *ppGPOHierarchyList = pNew;
    }

    return ceError;

done:
error:

    if (pNew) {
        if (pNew->pszDisplayName) {
            LW_SAFE_FREE_STRING(pNew->pszDisplayName);
        }

        if (pNew->pszGUID) {
            LW_SAFE_FREE_STRING(pNew->pszGUID);
        }

        LwFreeMemory(pNew);
    }

    return ceError;
}

static
CENTERROR
ParseLWISettingsXMLFile(
    PGPOLWIDATA pLwidata,
    PSTR pszDomainName,
    PSTR pszPolicyIdentifier,
    DWORD dwPolicyType,
    PSTR *ppszPolicyFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIGPITEM pGPItem = NULL;
    xmlNodePtr pXmlNodePtr = NULL;

    ceError = GPOGetGPItem( dwPolicyType,
                            pLwidata,
                            GetItemGUID(dwPolicyType),
                            &pGPItem );
    if (ceError != CENTERROR_SUCCESS) {
        GPA_LOG_ALWAYS( "Mac Work Group Manager policy gpitem not found for: Domain Name [%s] Policy [%s]",
                        pszDomainName,
                        pszPolicyIdentifier);
        GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    ceError = GPOXmlSelectSingleNode( pGPItem->xmlNode,
                                      "macWorkgroupManagerSettings/section/setting",
                                      &pXmlNodePtr);
    if (ceError != CENTERROR_SUCCESS) {
        GPA_LOG_ALWAYS( "Mac Work Group Manager policy setting not found for: Domain Name [%s] Policy [%s]",
                        pszDomainName,
                        pszPolicyIdentifier);
        GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    ceError = GPOXmlGetInnerText( pXmlNodePtr,
                                  ppszPolicyFileName);
    if (ceError != CENTERROR_SUCCESS) {
        GPA_LOG_ALWAYS( "Mac Work Group Manager policy setting value could not be read for: Domain Name [%s] Policy [%s]",
                        pszDomainName,
                        pszPolicyIdentifier);
        GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

error:

    if ( pGPItem ) {
        GPODestroyGPItem( pGPItem, 
                          FALSE);
    }

    return ceError;
}

static
CENTERROR
ProcessMCXFile(
    PGPUSER pUser,
    PSTR pszDomainName,
    PSTR pszDC,
    PSTR pszADSourcePath, 
    PSTR pszMCXLocalFilePath,
    PSTR pszPolicyIdentifier,
    DWORD dwPolicyType
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szLine[STATIC_PATH_BUFFER_SIZE];
    FILE *fp = NULL;

    memset(&szLine[0], 0, sizeof(szLine));

    ceError = GPAOpenFile( pszMCXLocalFilePath, "r", &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while (fgets(szLine, STATIC_PATH_BUFFER_SIZE, fp) != NULL)
    {
        CHAR szSourceFilePath[STATIC_PATH_BUFFER_SIZE];
        CHAR szDestFilePath[STATIC_PATH_BUFFER_SIZE];

        LwStripWhitespace(szLine,1,1);

        sprintf( szSourceFilePath,
                 "%s%s%s\\%s",
                 pszADSourcePath,
                 GetSysvolPath(dwPolicyType),
                 GetClientGUID(dwPolicyType),
                 szLine);

        if (pUser != NULL)
        {
            sprintf( szDestFilePath,
                     "%suser-cache/%ld/{%s}/%s/%s",
                     LIKEWISE_GP_DIRECTORY,
                     (long int)pUser->uid,
                     pszPolicyIdentifier,
                     GetClientGUID(dwPolicyType),
                     szLine);
        }
        else
        {
            sprintf( szDestFilePath,
                     "%s{%s}/%s/%s",
                     LIKEWISE_GP_DIRECTORY,
                     pszPolicyIdentifier,
                     GetClientGUID(dwPolicyType),
                     szLine);
        }

        ceError = GPOLwioCopyFile(pUser,
                                  pszDomainName,
                                  pszDC,
                                  szSourceFilePath,
                                  szDestFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        memset(&szLine[0], 0, sizeof(szLine));
    }

error:

    if (fp) {
        GPACloseFile(fp);
    }

    return ceError;
}

static
CENTERROR
GetMCXPolicyFile(
    PGPUSER pUser,
    PSTR pszDomainName,
    PSTR pszSourcePath,
    PSTR pszPolicyIdentifier,
    PSTR pszPolicyFileName,
    DWORD dwPolicyType
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDC = NULL;
    CHAR szSourceFilePath[STATIC_PATH_BUFFER_SIZE];
    CHAR szDestFilePath[STATIC_PATH_BUFFER_SIZE];
    BOOLEAN bDirExists = FALSE;

    memset(szDestFilePath, 0, sizeof(szDestFilePath));

    ceError = GPAGetPreferredDomainController(pszDomainName, &pszDC);
    if (!CENTERROR_IS_OK(ceError)) {
        if ( CENTERROR_EQUAL(ceError, CENTERROR_GP_NO_SMB_KRB5_SITE_KDC_INFO)    ||
             CENTERROR_EQUAL(ceError, CENTERROR_GP_NO_SMB_KRB5_SITE_INFO) ) {
            GPA_LOG_ALWAYS( "GPAgent unable to obtain preferred server for AD site: FQDN(%s)",
                            pszDomainName);
           ceError = CENTERROR_SUCCESS;
       } else {
           BAIL_ON_CENTERIS_ERROR(ceError);
       }
    }

    sprintf( szSourceFilePath,
             "%s%s%s\\%s",
             pszSourcePath,
             GetSysvolPath(dwPolicyType),
             GetClientGUID(dwPolicyType),
             pszPolicyFileName);

    if (pUser != NULL)
    {
        sprintf( szDestFilePath,
                 "%suser-cache/%ld/{%s}/%s",
                 LIKEWISE_GP_DIRECTORY,
                 (long int)pUser->uid,
                 pszPolicyIdentifier,
                 GetClientGUID(dwPolicyType));
    }
    else
    {
        sprintf( szDestFilePath,
                 "%s{%s}/%s",
                 LIKEWISE_GP_DIRECTORY,
                 pszPolicyIdentifier,
                 GetClientGUID(dwPolicyType));
    }

    ceError = GPACheckDirectoryExists(szDestFilePath, &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bDirExists == FALSE)
    {
        ceError = LwCreateDirectory(szDestFilePath, S_IRUSR|S_IRGRP|S_IROTH);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    strcat(szDestFilePath, "/");
    strcat(szDestFilePath, pszPolicyFileName);

    ceError = GPOLwioCopyFile(pUser,
                              pszDomainName,
                              pszDC,
                              szSourceFilePath,
                              szDestFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ProcessMCXFile( pUser,
                              pszDomainName,
                              pszDC,
                              pszSourcePath,
                              szDestFilePath,
                              pszPolicyIdentifier,
                              dwPolicyType);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    LW_SAFE_FREE_STRING(pszDC);

    return ceError;
}

static
CENTERROR
ProcessSettingsXMLFile(
    PGPUSER pUser,
    PSTR pszDomainName,
    PSTR pszSourcePath,
    PSTR pszPolicyIdentifier,
    PSTR pszgPCFileSysPath,
    DWORD dwPolicyType
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIDATA pLwidata = NULL;
    PSTR pszPolicyFileName = NULL;
    
    ceError = GPOInitLwiData( pUser,
                              dwPolicyType,
                              &pLwidata,
                              pszgPCFileSysPath,
                              NULL,
                              GetClientGUID(dwPolicyType));
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ParseLWISettingsXMLFile( pLwidata,
                                       pszDomainName,
                                       pszPolicyIdentifier,
                                       dwPolicyType,
                                       &pszPolicyFileName);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if(!pszPolicyFileName) {
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    ceError = GetMCXPolicyFile( pUser,
                                pszDomainName,
                                pszSourcePath,
                                pszPolicyIdentifier,
                                pszPolicyFileName,
                                dwPolicyType);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if ( pLwidata ) {
        GPODestroyLwiData(pLwidata);
    }

    LW_SAFE_FREE_STRING(pszPolicyFileName);

    return ceError;
}

static
CENTERROR
AddMacWorkgroupManagerGroupPolicy(
    PGPUSER pUser,
    PMAC_WRKGPMNGRLIST * ppGPOHierarchyList,
    PGROUP_POLICY_OBJECT pGPO,
    DWORD dwPolicyType
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;

    ceError = GPACrackFileSysPath( pGPO->pszgPCFileSysPath,
                                   &pszDomainName,
                                   &pszSourcePath,
                                   &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pGPO->bNewVersion)
    {
        ceError = ProcessSettingsXMLFile(pUser,
                                         pszDomainName,
                                         pszSourcePath,
                                         pszPolicyIdentifier,
                                         pGPO->pszgPCFileSysPath,
                                         dwPolicyType);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = AddPolicyFileToHierarchy(ppGPOHierarchyList,
                                       pGPO->pszDisplayName,
                                       pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    return ceError;
}

static
CENTERROR
ApplyMacWorkgroupManagerGroupPolicy(
    PMAC_WRKGPMNGRLIST pGPOHierarchyList,
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE *fp = NULL;
    PSTR pszFileDir = NULL;
    PSTR pszFilePath = NULL;
    PMAC_WRKGPMNGRLIST pCur = pGPOHierarchyList;
    BOOLEAN bDirExists = FALSE;
    BOOLEAN bFileExists = FALSE;
    int iPolicyNumber = 0;

    /* How many? */
    while (pCur)
    {
        iPolicyNumber++;
        pCur = pCur->pNext;
    }

    /* Reset pCur for the real work */
    pCur = pGPOHierarchyList;

    ceError = GetMCXFilePath(pUser, &pszFileDir, &pszFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPACheckDirectoryExists(pszFileDir, &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bDirExists == FALSE)
    {
        ceError = LwCreateDirectory(pszFileDir, S_IRUSR|S_IRGRP|S_IROTH);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pCur == NULL) {
        ceError = GPACheckFileExists(pszFilePath, &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            ceError = LwRemoveFile(pszFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    } else {
        ceError = GPAOpenFile(pszFilePath, "w", &fp);
        BAIL_ON_CENTERIS_ERROR(ceError);

        while (pCur) {
            ceError = GPAFilePrintf(fp,
                                   "%03d %s\n%s\n",
                                   iPolicyNumber, /* this is a trick for the Mac to force the OS to order the policies by precedence */
                                   pCur->pszDisplayName,
                                   pCur->pszGUID);
            BAIL_ON_CENTERIS_ERROR(ceError);

            iPolicyNumber--;

            pCur = pCur->pNext;
        }
    }

error:

    LW_SAFE_FREE_STRING(pszFilePath);
    LW_SAFE_FREE_STRING(pszFileDir);

    if (fp) {
        GPACloseFile(fp);
        fp = NULL;
    }

    return ceError;
}

static
CENTERROR
RemoveCacheFile(
    PSTR pszPolicyIdentifier,
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD dwPolicyType = GetPolicyType(pUser);
    CHAR szFilePath[STATIC_PATH_BUFFER_SIZE];
    BOOLEAN bDirExists = FALSE;

    switch (dwPolicyType)
    {
        case USER_GROUP_POLICY:
            sprintf( szFilePath,
                     "%suser-cache/%ld/{%s}/%s",
                     LIKEWISE_GP_DIRECTORY,
                     (long int)pUser->uid,
                     pszPolicyIdentifier,
                     GetClientGUID(dwPolicyType));

            ceError = GPACheckDirectoryExists( szFilePath, 
                                              &bDirExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (bDirExists) {
                ceError = GPARemoveDirectory(szFilePath);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            break;

        case MACHINE_GROUP_POLICY:
            sprintf( szFilePath,
                     "%s{%s}/%s",
                     LIKEWISE_GP_DIRECTORY,
                     pszPolicyIdentifier,
                     GetClientGUID(dwPolicyType));

            ceError = GPACheckDirectoryExists( szFilePath, 
                                              &bDirExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (bDirExists) {
                ceError = GPARemoveDirectory(szFilePath);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            break;
    }

error:

    return ceError;
}

static
CENTERROR
RemovePolicy(
    PGROUP_POLICY_OBJECT pGPO,
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
        
    ceError = GPACrackFileSysPath( pGPO->pszgPCFileSysPath,
                                   &pszDomainName,
                                   &pszSourcePath,
                                   &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = RemoveCacheFile(pszPolicyIdentifier,
                              pUser);
    BAIL_ON_CENTERIS_ERROR(ceError);
        
error:
        
    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    return ceError;
}

static  
void
FreeHierarchyList(
    PMAC_WRKGPMNGRLIST pGPOHierarchyList
    )
{
    PMAC_WRKGPMNGRLIST pTemp = NULL;
    
    while(pGPOHierarchyList)
    {
        pTemp = pGPOHierarchyList;
        pGPOHierarchyList = pGPOHierarchyList->pNext;
        LW_SAFE_FREE_STRING(pTemp->pszDisplayName);
        LW_SAFE_FREE_STRING(pTemp->pszGUID);
        LwFreeMemory(pTemp);
        pTemp = NULL;
    }
}

CENTERROR
ResetWorkgroupManagerGroupPolicy(
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* We don't remove any data in this policy CSE during restart of the gpagentd */

    return ceError;
}

CENTERROR
ProcessWorkgroupManagerGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_WRKGPMNGRLIST pGPOHierarchyList = NULL;
    BOOLEAN bApplicable = FALSE;

    GPA_LOG_FUNCTION_ENTER();

    while (pGPODeletedList) {
        ceError = RemovePolicy(pGPODeletedList,
                               pUser);
        BAIL_ON_CENTERIS_ERROR(ceError);
        pGPODeletedList = pGPODeletedList->pNext;
    }

    while ( pGPOModifiedList ) {
        bApplicable = FALSE;
        ceError = GPOXmlVerifyPlatformApplicable(pUser,
                                                 GetPolicyType(pUser),
                                                 pGPOModifiedList,
                                                 NULL,
                                                 &bApplicable);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        if (!bApplicable) {
            GPA_LOG_VERBOSE( "GPO(%s) disabled by platform targeting",
                             pGPOModifiedList->pszDisplayName);
            ceError = RemovePolicy(pGPOModifiedList,
                                   pUser);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE( "GPO(%s) is disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);
            ceError = RemovePolicy(pGPOModifiedList,
                                   pUser);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            ceError = AddMacWorkgroupManagerGroupPolicy(pUser,
                                                        &pGPOHierarchyList,
                                                        pGPOModifiedList,
                                                        GetPolicyType(pUser));
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        pGPOModifiedList = pGPOModifiedList->pNext;
    }

    ceError = ApplyMacWorkgroupManagerGroupPolicy(pGPOHierarchyList, pUser);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    FreeHierarchyList(pGPOHierarchyList);

    GPA_LOG_FUNCTION_LEAVE(ceError);

    return ceError;
}

