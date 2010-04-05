#include "includes.h"

//Global variable to preserve the previously calculated MD5Sum
static char g_szMd5Sum[256] = "";
static BOOLEAN gbMonitor; 
static CHAR gScriptFile[PATH_MAX+1];

static
void
FreeActionList(
    PFILESPOLICYACTION pActionList
    )
{
    PFILESPOLICYACTION pAction = NULL;

    while (pActionList) {
        pAction = pActionList;

        pActionList = pActionList->pNext;

        LW_SAFE_FREE_STRING(pAction->pszSourcePath);
        LW_SAFE_FREE_STRING(pAction->pszTargetPath);
        LW_SAFE_FREE_STRING(pAction->pszCommand);

        LwFreeMemory(pAction);
    }
}

static
CENTERROR
CrackAbsoluteFilePath(
    PCSTR pszAbsFilePath,
    PSTR* ppszDirPath,
    PSTR* ppszFilename
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCSTR pCh = NULL;

    if ( IsNullOrEmptyString(pszAbsFilePath) ||
         (*pszAbsFilePath != '/')) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (!strcmp(pszAbsFilePath, "/")) {
        ceError = LwAllocateString( pszAbsFilePath, 
                                    ppszDirPath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        *ppszFilename = NULL;

        goto done;
    }

    pCh = pszAbsFilePath + strlen(pszAbsFilePath)-1;

    while ((pCh != pszAbsFilePath) && (*pCh != '/')) {
        pCh--;
    }
  
    if ( pCh == pszAbsFilePath ) {
        /* The entire path is the file name */
        ceError = LwGetCurrentDirectoryPath(ppszDirPath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwAllocateString( pszAbsFilePath, 
                                    ppszFilename);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else {
        DWORD dwDirnameLen = strlen(pszAbsFilePath) - strlen(pCh);

        ceError = LwAllocateMemory( dwDirnameLen + 1, 
                                    (PVOID*)ppszDirPath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strncpy( *ppszDirPath, 
                 pszAbsFilePath, 
                 dwDirnameLen);

        ceError = LwAllocateString( pCh + 1, 
                                    ppszFilename);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
  
done:
error:

    return ceError;
}

static
CENTERROR
BackupFile(
    PSTR pszPolicyCachePath,
    PSTR pszTargetFilePath,
    PFILESPOLICYACTION* ppUndoAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDirPath = NULL;
    PSTR pszFilename = NULL;
    PSTR pszNewPath = NULL;
    BOOLEAN bExists = FALSE;
    PFILESPOLICYACTION pUndoAction = NULL;
    CHAR szBackupDirPath[PATH_MAX+1];
    uid_t uid;
    gid_t gid;
    mode_t mode;

    sprintf( szBackupDirPath, 
             "%s/backup", 
             pszPolicyCachePath);

    ceError = GPACheckDirectoryExists( szBackupDirPath, 
                                      &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bExists) {
        ceError = LwCreateDirectory( szBackupDirPath, 
                                     S_IRWXU);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CrackAbsoluteFilePath( pszTargetFilePath, 
                                     &pszDirPath, 
                                     &pszFilename);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( strlen(szBackupDirPath) + strlen(pszDirPath) + 2, 
                                (PVOID*)&pszNewPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf( pszNewPath, 
             "%s/%s", 
             szBackupDirPath, 
             pszDirPath);

    ceError = GPACheckDirectoryExists( pszNewPath, 
                                      &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);
  
    if (!bExists) {    
        ceError = LwCreateDirectory( pszNewPath, 
                                     S_IRWXU);
        BAIL_ON_CENTERIS_ERROR(ceError);    
    }
  
    ceError = LwReallocMemory( pszNewPath,
                               (PVOID*)&pszNewPath,
                               (strlen(szBackupDirPath) + strlen(pszDirPath) + strlen(pszTargetFilePath)+ 2));

    BAIL_ON_CENTERIS_ERROR(ceError);

    /* we expect an absolute path always for the target file path */
    sprintf( pszNewPath, 
             "%s%s", 
             szBackupDirPath, 
             pszTargetFilePath);
  
    ceError = LwAllocateMemory( sizeof(FILESPOLICYACTION), 
                                (PVOID*)&pUndoAction);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pUndoAction->actionType = MOVE_FILE;
    ceError = LwAllocateString( pszNewPath, 
                                &pUndoAction->pszSourcePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszTargetFilePath, 
                                &pUndoAction->pszTargetPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwGetOwnerAndPermissions( pszTargetFilePath, 
                                        &uid, 
                                        &gid, 
                                        &mode);
    BAIL_ON_CENTERIS_ERROR(ceError);
  
    pUndoAction->uid = uid;
    pUndoAction->gid = gid;
    pUndoAction->permissions = mode;
  
    /* Move the file to the cache */
    ceError = GPAMoveFileAcrossDevices( pszTargetFilePath, 
                                       pszNewPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppUndoAction = pUndoAction;
    pUndoAction = NULL;

error:

    SAFE_FREE_ACTION_LIST(pUndoAction);

    LW_SAFE_FREE_STRING(pszNewPath);
    LW_SAFE_FREE_STRING(pszDirPath);
    LW_SAFE_FREE_STRING(pszFilename);

    return ceError;

}

static
PFILESPOLICY
ReversePolicyHierarchy(
    PFILESPOLICY pPolicyHierarchy
    )
{
    PFILESPOLICY pPrev = NULL;
    PFILESPOLICY pNext = NULL;
    PFILESPOLICY pCur = pPolicyHierarchy;

    while (pCur) {
        pNext = pCur->pNext;
        pCur->pNext = pPrev;
        pPrev = pCur;
        pCur = pNext;
    }

    return pPrev;
}

static
void
FreePolicyHierarchy(
    PFILESPOLICY pPolicyList
    )
{
    PFILESPOLICY pPolicy = NULL;
    while (pPolicyList) {
        pPolicy = pPolicyList;
        pPolicyList = pPolicyList->pNext;

        LW_SAFE_FREE_STRING(pPolicy->pszPolicyIdentifier);

        if (pPolicy->pGPItem) {
            GPODestroyGPItem(pPolicy->pGPItem, FALSE);
        }
        if (pPolicy->pLwidata) {
            GPODestroyLwiData(pPolicy->pLwidata);
        }
        LwFreeMemory(pPolicy);
    }
}

static
CENTERROR
BuildPolicyHierarchy(
    PFILESPOLICY* ppPolicyHierarchy
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szPolicyGUID[65];
    PFILESPOLICY pPolicyHierarchy = NULL;
    PFILESPOLICY pPolicy = NULL;
    BOOLEAN bExists = FALSE;
    FILE* fp = NULL;

    /* Each line in this file is a policy guid */
    ceError = GPACheckFileExists( LWIPOLICYSEQUENCEPATH, 
                                 &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);
  
    if (!bExists) {
        *ppPolicyHierarchy = NULL;
        goto done;
    }

    ceError = GPAOpenFile( LWIPOLICYSEQUENCEPATH, 
                          "r", 
                          &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *szPolicyGUID = '\0';

    while (!feof(fp)) {
        if (!fgets(szPolicyGUID, 64, fp)) {
            if (feof(fp)) {
                break;
            }
            else {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        LwStripWhitespace(szPolicyGUID,1,1);

        if (IsNullOrEmptyString(szPolicyGUID))
            continue;

        ceError = LwAllocateMemory( sizeof(FILESPOLICY), 
                                    (PVOID*)&pPolicy);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwAllocateString( szPolicyGUID, 
                                    &pPolicy->pszPolicyIdentifier);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pPolicy->pNext = pPolicyHierarchy;
        pPolicyHierarchy = pPolicy;
        pPolicy = NULL;
    }

    *ppPolicyHierarchy = ReversePolicyHierarchy(pPolicyHierarchy);
    pPolicyHierarchy = NULL;

done:
error:

    if (pPolicyHierarchy)
        FreePolicyHierarchy(pPolicyHierarchy);

    if (fp)
        GPACloseFile(fp);

    return ceError;
}

static
CENTERROR
UpdatePolicyHierarchy(
    PGROUP_POLICY_OBJECT pGPO,
    PFILESPOLICY* ppPolicy
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    PFILESPOLICY pPolicy = NULL;

    ceError = GPACrackFileSysPath( pGPO->pszgPCFileSysPath,
                                     &pszDomainName,
                                   &pszSourcePath,
                                   &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( sizeof(FILESPOLICY), 
                                (PVOID*)&pPolicy);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszPolicyIdentifier, 
                                &pPolicy->pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pPolicy->pGPO = pGPO;

    ceError = GPOGetGPItem( MACHINE_GROUP_POLICY,
                            pPolicy->pLwidata,
                            LWIFILES_ITEM_GUID,
                            &pPolicy->pGPItem);
    if ( !CENTERROR_IS_OK(ceError) &&
         CENTERROR_EQUAL( ceError, 
                          CENTERROR_GP_XML_GPITEM_NOT_FOUND)) {
        pPolicy->pGPItem = NULL;
        ceError = CENTERROR_SUCCESS;
    }
    if ( !CENTERROR_IS_OK(ceError) &&
         CENTERROR_EQUAL( ceError, 
                          CENTERROR_GP_LWIDATA_NOT_INITIALIZED)) {

        /* GPO should be treated like a new version since it appears to not be cached */
        FreePolicyHierarchy(pPolicy);
        pPolicy = NULL;

        ceError = AddFilesPolicy(pGPO, &pPolicy);
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppPolicy = pPolicy;
    pPolicy = NULL;

error:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    if (pPolicy)
        FreePolicyHierarchy(pPolicy);

    return ceError;

}

static
CENTERROR
UnsetFileAction(
    PSTR pszPolicyCachePath,
    PFILESPOLICYACTION pAction,
    PFILESPOLICYACTION* ppUndoAction
    )
{
    GPA_LOG_ERROR( "Unexpected file action type [%d]", 
                   pAction->actionType);
    return CENTERROR_GP_UNEXPECTED_ACTION_TYPE;
}

static
CENTERROR
RemoveFileAction(
    PSTR pszPolicyCachePath,
    PFILESPOLICYACTION pAction,
    PFILESPOLICYACTION* ppUndoAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN   bExists = FALSE;
    PFILESPOLICYACTION pUndoAction = NULL;

    GPA_LOG_VERBOSE( "Removing file at [%s]", 
                     pAction->pszSourcePath);

    ceError = GPACheckFileExists( pAction->pszSourcePath, 
                                 &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bExists) {
        goto done;
    }

    /* Do we want to possibly revert to this file */
    if (pAction->bDeleteOnRevert && ppUndoAction) {
        ceError = BackupFile( pszPolicyCachePath, 
                              pAction->pszSourcePath, 
                              &pUndoAction);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
  
    ceError = LwRemoveFile(pAction->pszSourcePath);
    BAIL_ON_CENTERIS_ERROR(ceError);
  
    if (ppUndoAction) {
        *ppUndoAction = pUndoAction;
        pUndoAction = NULL;
    }

done:
error:

    SAFE_FREE_ACTION_LIST(pUndoAction);

    return ceError;

}

static
CENTERROR
CopyFileAction(
    PSTR pszPolicyCachePath,
    PFILESPOLICYACTION pAction,
    PFILESPOLICYACTION* ppUndoAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICYACTION pUndoAction = NULL;
    PFILESPOLICYACTION pUndoCopyAction = NULL;
    BOOLEAN bExists = FALSE;

    ceError = GPACheckFileExists( pAction->pszSourcePath, 
                                 &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bExists)
        goto done;

    GPA_LOG_VERBOSE( "Copying file from [%s] to [%s]", 
                     pAction->pszSourcePath, 
                     pAction->pszTargetPath);

    if (pAction->bDeleteOnRevert && ppUndoAction) {
        uid_t uid;
        gid_t gid;
        mode_t mode;
    
        ceError = GPACheckFileExists(pAction->pszTargetPath, &bExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bExists) {
            /* Back up the target path */
            /* Create an action to copy the cached file back to target path */
            ceError = BackupFile( pszPolicyCachePath, 
                                  pAction->pszTargetPath, 
                                  &pUndoAction);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = LwGetOwnerAndPermissions( pAction->pszSourcePath, 
                                            &uid, 
                                            &gid, 
                                            &mode);
        BAIL_ON_CENTERIS_ERROR(ceError);

        /* However, when undoing, we first need to copy the target path back to source */
        ceError = LwAllocateMemory( sizeof(FILESPOLICYACTION),
                                    (PVOID*)&pUndoCopyAction);
        BAIL_ON_CENTERIS_ERROR(ceError);
    
        pUndoCopyAction->actionType = COPY_FILE;
        pUndoCopyAction->uid = uid;
        pUndoCopyAction->gid = gid;
        pUndoCopyAction->permissions = mode;
    
        ceError = LwAllocateString( pAction->pszSourcePath, 
                                    &pUndoCopyAction->pszTargetPath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    
        ceError = LwAllocateString( pAction->pszTargetPath, 
                                    &pUndoCopyAction->pszSourcePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    
        pUndoCopyAction->pNext = pUndoAction;
        pUndoAction = pUndoCopyAction;
        pUndoCopyAction = NULL;
    }

    ceError = GPACopyFileWithPerms( pAction->pszSourcePath, 
                                   pAction->pszTargetPath, 
                                   pAction->permissions);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwChangeOwner( pAction->pszTargetPath, 
                             pAction->uid, pAction->gid);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (ppUndoAction) {
        *ppUndoAction = pUndoAction;
        pUndoAction = NULL;
    }

done:
error:

    SAFE_FREE_ACTION_LIST(pUndoAction);
    SAFE_FREE_ACTION_LIST(pUndoCopyAction);
  
    return ceError;

}

static
CENTERROR
MoveFileAction(
    PSTR pszPolicyCachePath,
    PFILESPOLICYACTION pAction,
    PFILESPOLICYACTION* ppUndoAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICYACTION pUndoAction = NULL;
    BOOLEAN bExists = FALSE;
    PSTR pszNewMd5Sum = NULL;

    ceError = GPACheckFileExists( pAction->pszSourcePath, 
                                 &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bExists) {
        GPA_LOG_WARNING( "Cannot move file. Source path [%s] does not exist", 
                         pAction->pszSourcePath);
        goto done;
    }

    ceError = GPACheckFileExists( pAction->pszTargetPath, 
                                 &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( bExists && gbMonitor) {

        if (!strcmp(pAction->pszTargetPath,gScriptFile)) {

            GPA_LOG_VERBOSE("Monitoring files Flag is set, Going to monitor file");
            ceError = MonitorSystemFiles( gScriptFile,
                                          g_szMd5Sum,
                                          &pszNewMd5Sum);
            BAIL_ON_CENTERIS_ERROR(ceError);

            LW_SAFE_FREE_STRING(pszNewMd5Sum);
        }
    }

    GPA_LOG_VERBOSE( "Moving file from [%s] to [%s]", 
                     pAction->pszSourcePath, 
                     pAction->pszTargetPath);

    if (pAction->bDeleteOnRevert && ppUndoAction) {    
        ceError = GPACheckFileExists( pAction->pszTargetPath, 
                                     &bExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bExists) {
            /* Back up the target path */
            /* Create an action to copy the cached file back to target path */
            ceError = BackupFile( pszPolicyCachePath,
                                  pAction->pszTargetPath,
                                  &pUndoAction);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else {
            ceError = LwAllocateMemory( sizeof(FILESPOLICYACTION),
                                        (PVOID*)&pUndoAction);
            BAIL_ON_CENTERIS_ERROR(ceError);
    
            pUndoAction->actionType = REMOVE_FILE;

            ceError = LwAllocateString( pAction->pszTargetPath,
                                       &pUndoAction->pszSourcePath);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    ceError = GPAMoveFileAcrossDevices( pAction->pszSourcePath,
                                       pAction->pszTargetPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwChangeOwnerAndPermissions( pAction->pszTargetPath,
                                           pAction->uid,
                                           pAction->gid,
                                           pAction->permissions);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(gbMonitor) {

        GPA_LOG_VERBOSE("Going to calculate md5Sum for the new script file");

        ceError = MonitorSystemFiles( pAction->pszTargetPath,
                                      g_szMd5Sum,
                                      &pszNewMd5Sum);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strncpy( g_szMd5Sum,
                 pszNewMd5Sum,
                 (strlen(pszNewMd5Sum)));

        strcpy(gScriptFile,pAction->pszTargetPath);
    }

    if (ppUndoAction) {
        *ppUndoAction = pUndoAction;
        pUndoAction = NULL;
    }

done:
error:

    SAFE_FREE_ACTION_LIST(pUndoAction);

    LW_SAFE_FREE_STRING(pszNewMd5Sum);
  
    return ceError;

}

static
CENTERROR
ExecuteCommandAction(
    PSTR pszPolicyCachePath,
    PFILESPOLICYACTION pAction,
    PFILESPOLICYACTION* ppUndoAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (system(pAction->pszCommand) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *ppUndoAction = NULL;
error:

    return ceError;
}

static
CENTERROR
CreateDirectoryAction(
    PSTR pszPolicyCachePath,
    PFILESPOLICYACTION pAction,
    PFILESPOLICYACTION* ppUndoAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICYACTION pUndoAction = NULL;
    BOOLEAN bExists = FALSE;

    ceError = GPACheckDirectoryExists( pAction->pszTargetPath, 
                                      &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bExists) {
        uid_t uid;
        gid_t gid;
        mode_t mode;

        ceError = LwGetOwnerAndPermissions( pAction->pszTargetPath, 
                                            &uid, 
                                            &gid, 
                                            &mode);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if ( (uid != pAction->uid) ||
             (gid != pAction->gid) ||
             (mode != pAction->permissions)) {
            ceError = LwChangeOwnerAndPermissions( pAction->pszTargetPath, 
                                                   pAction->uid, 
                                                   pAction->gid, 
                                                   pAction->permissions);
            if (ceError != CENTERROR_SUCCESS) {
                GPA_LOG_VERBOSE( "Unable to change owner/permissions for [%s]. Hence, skipping it.", pAction->pszTargetPath);
                ceError = CENTERROR_SUCCESS;
            }
        }
    }
    else {
        GPA_LOG_VERBOSE( "Creating Directory at [%s] with uid as %d and gid as %d", 
                         pAction->pszTargetPath,
                         pAction->uid, 
                         pAction->gid);

        ceError = LwCreateDirectory( pAction->pszTargetPath, 
                                     pAction->permissions);
        BAIL_ON_CENTERIS_ERROR(ceError);
    
        ceError = LwChangeOwner( pAction->pszTargetPath, 
                                 pAction->uid, 
                                 pAction->gid);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pAction->bDeleteOnRevert && ppUndoAction) {
        ceError = LwAllocateMemory( sizeof(FILESPOLICYACTION),
                                    (PVOID*)&pUndoAction);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pUndoAction->actionType = REMOVE_DIRECTORY;

        ceError = LwAllocateString( pAction->pszTargetPath,
                                    &pUndoAction->pszSourcePath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        *ppUndoAction = pUndoAction;
        pUndoAction = NULL;
    }

error:

    SAFE_FREE_ACTION_LIST(pUndoAction);

    return ceError;

}

static
CENTERROR
RemoveDirectoryAction(
    PSTR pszPolicyCachePath,
    PFILESPOLICYACTION pAction,
    PFILESPOLICYACTION* ppUndoAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bExists = FALSE;

    ceError = GPACheckDirectoryExists( pAction->pszSourcePath, 
                                      &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bExists)
        goto done;

    GPA_LOG_VERBOSE( "Removing directory at [%s]", 
                     pAction->pszSourcePath);

    if (pAction->bDeleteOnRevert && ppUndoAction) {
        // TODO: We need to tar gzip the directory into the backup folder.
    }

    ceError = GPARemoveDirectory(pAction->pszSourcePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

done:
error:

    if (ppUndoAction)
        *ppUndoAction = NULL;

    return ceError;

}

static
CENTERROR
CreateLinkAction(
    PSTR pszPolicyCachePath,
    PFILESPOLICYACTION pAction,
    PFILESPOLICYACTION* ppUndoAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bExists = FALSE;
    PSTR pszSymlinkTargetPath = NULL;
    PFILESPOLICYACTION pUndoAction = NULL;

    ceError = GPACheckLinkExists( pAction->pszSourcePath, 
                                 &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);
  
    if (bExists) {
        // A link exists at the target path
        ceError = GPAGetSymLinkTarget( pAction->pszSourcePath,
                                      &pszSymlinkTargetPath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!strcmp(pAction->pszTargetPath, pszSymlinkTargetPath)) {
            /* If it exists already, we don't have to undo it */
            goto done;
        }

        /* The target is different */
        if (pAction->bDeleteOnRevert && ppUndoAction) {
            ceError = LwAllocateMemory( sizeof(FILESPOLICYACTION),
                                        (PVOID*)&pUndoAction);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pUndoAction->actionType = CREATE_LINK;

            ceError = LwAllocateString( pAction->pszSourcePath,
                                       &pUndoAction->pszSourcePath);
            BAIL_ON_CENTERIS_ERROR(ceError);
      
            ceError = LwAllocateString( pAction->pszTargetPath,
                                       &pUndoAction->pszTargetPath);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = LwRemoveFile(pAction->pszSourcePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else {
        /* No link exists. So, we must undo */
        if ( pAction->bDeleteOnRevert && ppUndoAction ) {
            ceError = LwAllocateMemory( sizeof(FILESPOLICYACTION),
                                        (PVOID*)&pUndoAction);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pUndoAction->actionType = REMOVE_LINK;

            ceError = LwAllocateString( pAction->pszSourcePath,
                                       &pUndoAction->pszSourcePath);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    GPA_LOG_VERBOSE( "Creating link from [%s] to [%s]", 
                     pAction->pszSourcePath, 
                     pAction->pszTargetPath);

    ceError = GPACreateSymLink( pAction->pszTargetPath, 
                               pAction->pszSourcePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (ppUndoAction) {
        *ppUndoAction = pUndoAction;
        pUndoAction = NULL;
    }

done:
error:

    LW_SAFE_FREE_STRING(pszSymlinkTargetPath);

    SAFE_FREE_ACTION_LIST(pUndoAction);

    return ceError;
}

static
CENTERROR
RemoveLinkAction(
    PSTR pszPolicyCachePath,
    PFILESPOLICYACTION pAction,
    PFILESPOLICYACTION* ppUndoAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICYACTION pUndoAction = NULL;
    BOOLEAN bExists = FALSE;

    ceError = GPACheckLinkExists( pAction->pszSourcePath, 
                                 &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bExists)
        goto done;

    GPA_LOG_VERBOSE( "Removing link at [%s]", 
                     pAction->pszSourcePath);

    if (pAction->bDeleteOnRevert && ppUndoAction) {
        ceError = LwAllocateMemory( sizeof(FILESPOLICYACTION),
                                    (PVOID*)&pUndoAction);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pUndoAction->actionType = CREATE_LINK;

        /* The target of the current link becomes the target of the undo action */
        ceError = GPAGetSymLinkTarget( pAction->pszSourcePath,
                                      &pUndoAction->pszTargetPath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        /* The source of the undo action will be the path we are deleting now */
        ceError = LwAllocateString( pAction->pszSourcePath,
                                    &pUndoAction->pszSourcePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwRemoveFile(pAction->pszSourcePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (ppUndoAction) {
        *ppUndoAction = pUndoAction;
        pUndoAction = NULL;
    }

done:
error:

    SAFE_FREE_ACTION_LIST(pUndoAction);

    return ceError;

}

typedef CENTERROR (*PFNActionExecutor)(PSTR pszPolicyCachePath, PFILESPOLICYACTION pAction, PFILESPOLICYACTION* ppUndoAction);

PFNActionExecutor gActionExecutors[] = { &UnsetFileAction,
                                         &RemoveFileAction,
                                         &CopyFileAction,
                                         &MoveFileAction,
                                         &CreateDirectoryAction,
                                         &RemoveDirectoryAction,
                                         &CreateLinkAction,
                                         &RemoveLinkAction,
                                         &ExecuteCommandAction };

static
CENTERROR
BuildSourcePathXMLElement(
    xmlDocPtr pDoc,
    PFILESPOLICYACTION pAction,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    xmlNodePtr pCDATA = NULL;

    pNode = xmlNewNode( NULL, 
                        (const xmlChar*)"source");
    if (!pNode) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pCDATA = xmlNewCDataBlock( pDoc,
                               (const xmlChar*)pAction->pszSourcePath,
                               strlen(pAction->pszSourcePath));
    if (!pCDATA) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_CDATA;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (NULL == xmlAddChild(pNode, pCDATA)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pCDATA = NULL;

    *ppNode = pNode;
    pNode = NULL;

error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    if (pCDATA) {
        xmlFreeNode(pCDATA);
    }

    return ceError;

}  

static
CENTERROR
BuildTargetPathXMLElement(
    xmlDocPtr pDoc,
    PFILESPOLICYACTION pAction,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    xmlNodePtr pCDATA = NULL;
    CHAR szBuf[256];

    pNode = xmlNewNode(NULL, (const xmlChar*)"target");
    if (!pNode) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pCDATA = xmlNewCDataBlock( pDoc,
                               (const xmlChar*)pAction->pszTargetPath,
                               strlen(pAction->pszTargetPath));
    if (!pCDATA) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_CDATA;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (NULL == xmlAddChild(pNode, pCDATA)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pCDATA = NULL;

    sprintf( szBuf, 
             "%d", 
             (int)pAction->uid);

    if (NULL == xmlSetProp( pNode, 
                            (const xmlChar*)UID_TAG, 
                            (const xmlChar*)szBuf)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    sprintf( szBuf, 
             "%d", 
             (int)pAction->gid);
    if (NULL == xmlSetProp( pNode, 
                            (const xmlChar*)GID_TAG, 
                            (const xmlChar*)szBuf)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    sprintf( szBuf, 
             "%o", 
             (int)pAction->permissions);
    if (NULL == xmlSetProp( pNode, 
                            (const xmlChar*)MODE_TAG, 
                            (const xmlChar*)szBuf)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (NULL == xmlSetProp( pNode, 
                            (const xmlChar*)DELETE_ON_POLICY_REVERT_TAG, 
                            (const xmlChar*)(pAction->bDeleteOnRevert ? "1" : "0"))) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *ppNode = pNode;
    pNode = NULL;

error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    if (pCDATA) {
        xmlFreeNode(pCDATA);
    }

    return ceError;

}

static
CENTERROR
ConfigureUnsetFileAction(
    xmlDocPtr pDoc,
    PFILESPOLICYACTION pAction,
    xmlNodePtr* ppNode
    )
{
    GPA_LOG_ERROR( "Unexpected file action type [%d]", 
                   pAction->actionType);
    return CENTERROR_GP_UNEXPECTED_ACTION_TYPE;
}

static
CENTERROR
ConfigureRemoveFileAction(
    xmlDocPtr pDoc,
    PFILESPOLICYACTION pAction,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    xmlNodePtr pSrcNode = NULL;

    pNode = xmlNewNode( NULL,
                        (const xmlChar*)ACTION_TAG);
    if (!pNode) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (NULL == xmlSetProp(pNode, (const xmlChar*)TYPE_TAG, (const xmlChar*)REMOVE_FILE_TAG)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = BuildSourcePathXMLElement( pDoc, 
                                         pAction, 
                                         &pSrcNode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (NULL == xmlAddChild( pNode, 
                             pSrcNode)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pSrcNode = NULL;

    *ppNode = pNode;
    pNode = NULL;
  
error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    if (pSrcNode) {
        xmlFreeNode(pSrcNode);
    }

    return ceError;

}

static
CENTERROR
ConfigureCopyFileAction(
    xmlDocPtr pDoc,
    PFILESPOLICYACTION pAction,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    xmlNodePtr pSrcNode = NULL;
    xmlNodePtr pTargetNode = NULL;

    pNode = xmlNewNode( NULL, 
                        (const xmlChar*)ACTION_TAG);
    if (!pNode) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (NULL == xmlSetProp( pNode, 
                            (const xmlChar*)TYPE_TAG, 
                            (const xmlChar*)COPY_FILE_TAG)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = BuildSourcePathXMLElement( pDoc, 
                                         pAction, 
                                         &pSrcNode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (NULL == xmlAddChild( pNode, 
                             pSrcNode)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pSrcNode = NULL;

    ceError = BuildTargetPathXMLElement( pDoc, 
                                         pAction, 
                                         &pTargetNode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (NULL == xmlAddChild( pNode, 
                             pTargetNode)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pTargetNode = NULL;

    *ppNode = pNode;
    pNode = NULL;

error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    if (pSrcNode) {
        xmlFreeNode(pSrcNode);
    }

    if (pTargetNode) {
        xmlFreeNode(pTargetNode);
    }

    return ceError;

}

static
CENTERROR
ConfigureMoveFileAction(
    xmlDocPtr pDoc,
    PFILESPOLICYACTION pAction,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    xmlNodePtr pSrcNode = NULL;
    xmlNodePtr pTargetNode = NULL;

    pNode = xmlNewNode( NULL, 
                        (const xmlChar*)ACTION_TAG);
    if (!pNode) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (NULL == xmlSetProp( pNode, 
                            (const xmlChar*)TYPE_TAG, 
                            (const xmlChar*)MOVE_FILE_TAG)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = BuildSourcePathXMLElement( pDoc, 
                                         pAction, 
                                         &pSrcNode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (NULL == xmlAddChild(pNode, pSrcNode)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pSrcNode = NULL;

    ceError = BuildTargetPathXMLElement( pDoc, 
                                         pAction, 
                                         &pTargetNode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (NULL == xmlAddChild(pNode, pTargetNode)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pTargetNode = NULL;

    *ppNode = pNode;
    pNode = NULL;

error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    if (pSrcNode) {
        xmlFreeNode(pSrcNode);
    }

    if (pTargetNode) {
        xmlFreeNode(pTargetNode);
    }

    return ceError;

}

static
CENTERROR
ConfigureCreateDirectoryAction(
    xmlDocPtr pDoc,
    PFILESPOLICYACTION pAction,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    xmlNodePtr pTargetNode = NULL;

    pNode = xmlNewNode( NULL, 
                        (const xmlChar*)ACTION_TAG);
    if (!pNode) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (NULL == xmlSetProp( pNode, 
                            (const xmlChar*)TYPE_TAG, 
                            (const xmlChar*)CREATE_DIR_TAG)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = BuildTargetPathXMLElement( pDoc, 
                                         pAction, 
                                         &pTargetNode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (NULL == xmlAddChild( pNode, 
                             pTargetNode)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pTargetNode = NULL;

    *ppNode = pNode;
    pNode = NULL;

error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    if (pTargetNode) {
        xmlFreeNode(pTargetNode);
    }

    return ceError;

}

static
CENTERROR
ConfigureRemoveDirectoryAction(
    xmlDocPtr pDoc,
    PFILESPOLICYACTION pAction,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    xmlNodePtr pSrcNode = NULL;

    pNode = xmlNewNode( NULL, 
                        (const xmlChar*)ACTION_TAG);
    if (!pNode) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (NULL == xmlSetProp( pNode, 
                            (const xmlChar*)TYPE_TAG, 
                            (const xmlChar*)REMOVE_DIR_TAG)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = BuildSourcePathXMLElement( pDoc, 
                                         pAction, 
                                         &pSrcNode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (NULL == xmlAddChild( pNode, 
                             pSrcNode)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pSrcNode = NULL;

    *ppNode = pNode;
    pNode = NULL;

error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    if (pSrcNode) {
        xmlFreeNode(pSrcNode);
    }

    return ceError;

}

static
CENTERROR
ConfigureCreateLinkAction(
    xmlDocPtr pDoc,
    PFILESPOLICYACTION pAction,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    xmlNodePtr pSrcNode = NULL;
    xmlNodePtr pTargetNode = NULL;

    pNode = xmlNewNode( NULL, 
                        (const xmlChar*)ACTION_TAG);
    if (!pNode) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (NULL == xmlSetProp( pNode, 
                            (const xmlChar*)TYPE_TAG, 
                            (const xmlChar*)CREATE_LINK_TAG)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = BuildSourcePathXMLElement( pDoc, 
                                         pAction, 
                                         &pSrcNode);
    BAIL_ON_CENTERIS_ERROR(ceError);
  
    if (NULL == xmlAddChild( pNode, 
                             pSrcNode)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
  
    pSrcNode = NULL;
  
    ceError = BuildTargetPathXMLElement( pDoc, 
                                         pAction, 
                                         &pTargetNode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (NULL == xmlAddChild( pNode, 
                             pTargetNode)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pTargetNode = NULL;

    *ppNode = pNode;
    pNode = NULL;

error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    if (pSrcNode) {
        xmlFreeNode(pSrcNode);
    }

    if (pTargetNode) {
        xmlFreeNode(pTargetNode);
    }

    return ceError;

}

static
CENTERROR
ConfigureRemoveLinkAction(
    xmlDocPtr pDoc,
    PFILESPOLICYACTION pAction,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    xmlNodePtr pSrcNode = NULL;

    pNode = xmlNewNode( NULL,
                        (const xmlChar*)ACTION_TAG);
    if (!pNode) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (NULL == xmlSetProp( pNode, 
                            (const xmlChar*)TYPE_TAG, 
                            (const xmlChar*)REMOVE_LINK_TAG)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = BuildSourcePathXMLElement( pDoc, 
                                         pAction, 
                                         &pSrcNode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (NULL == xmlAddChild( pNode, 
                             pSrcNode)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pSrcNode = NULL;

    *ppNode = pNode;
    pNode = NULL;

error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    if (pSrcNode) {
        xmlFreeNode(pSrcNode);
    }

    return ceError;

}

typedef CENTERROR (*PFNUndoActionConfigurator)(xmlDocPtr pDoc, PFILESPOLICYACTION pUndoAction, xmlNodePtr* ppNode);

PFNUndoActionConfigurator gUndoActionConfigurators[] = { &ConfigureUnsetFileAction,
                                                         &ConfigureRemoveFileAction,
                                                         &ConfigureCopyFileAction,
                                                         &ConfigureMoveFileAction,
                                                         &ConfigureCreateDirectoryAction,
                                                         &ConfigureRemoveDirectoryAction,
                                                         &ConfigureCreateLinkAction,
                                                         &ConfigureRemoveLinkAction };

static
CENTERROR
GetFirstPathFromCDATA(
    xmlNodePtr pNode,
    PSTR* ppszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszPath = NULL;
    xmlChar* pszXmlContent = NULL;
    xmlNodePtr pChildNode = NULL;

    pChildNode = (pNode ? pNode->children : NULL);

    while (pChildNode) {    
        if (pChildNode->type == XML_CDATA_SECTION_NODE) {
            pszXmlContent = xmlNodeGetContent(pChildNode);
            if (pszXmlContent) {
                ceError = LwAllocateString( (PCSTR)pszXmlContent,
                                            &pszPath);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
            break;
        }

        pNode = pNode->next;
    }

    *ppszPath = pszPath;
    pszPath = NULL;
  
error:

    LW_SAFE_FREE_STRING(pszPath);

    if (pszXmlContent) {
        xmlFree(pszXmlContent);
    }

    return ceError;

}

static
CENTERROR
BuildActionCommandInfo(
    xmlNodePtr xmlNode,
    FileActionType actionType,
    PFILESPOLICYACTION pAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = GetFirstPathFromCDATA( xmlNode, 
                                     &pAction->pszCommand);
    BAIL_ON_CENTERIS_ERROR(ceError);

    LwStripWhitespace(pAction->pszCommand,1,1);

error:

    return ceError;

}

static
CENTERROR
BuildActionTargetInfo(
    xmlNodePtr xmlNode,
    FileActionType actionType,
    PFILESPOLICYACTION pAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodeSetPtr pXmlNodeSetPtr = NULL;
    xmlChar* pAttrValue = NULL;

    if(actionType == CREATE_LINK) {
        pAttrValue = xmlGetProp( xmlNode, 
                                 (const xmlChar*) DELETE_ON_POLICY_REVERT_TAG);
        if (pAttrValue) {
            char ch = (char)*pAttrValue;
            pAction->bDeleteOnRevert = ((ch == 'Y') || (ch == 'y') || (ch == '1') || (ch == 'T') || (ch == 't'));
            xmlFree(pAttrValue);
            pAttrValue = NULL;
        } else {
            /* This should match the way we set it on the AD side */
            pAction->bDeleteOnRevert = TRUE;
        }    
    }

    ceError = GPOXmlSelectNodes( xmlNode, 
                                 "target", 
                                 &pXmlNodeSetPtr);
    BAIL_ON_CENTERIS_ERROR(ceError);
  
    if (pXmlNodeSetPtr && pXmlNodeSetPtr->nodeNr == 1) {    
        xmlNodePtr actionNode = pXmlNodeSetPtr->nodeTab[0];

        ceError = GetFirstPathFromCDATA( actionNode, 
                                         &pAction->pszTargetPath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        LwStripWhitespace(pAction->pszTargetPath,1,1);
    
        pAttrValue = xmlGetProp( actionNode, 
                                 (const xmlChar*) UID_TAG);
        if (pAttrValue) {
            ceError = GPAGetUserUID( (PCSTR)pAttrValue, &pAction->uid);
            if (ceError != CENTERROR_SUCCESS) {
                GPA_LOG_VERBOSE( "User [%s] not found on this system.", (PCSTR)pAttrValue);
                goto error;
            }
            xmlFree(pAttrValue);
            pAttrValue = NULL;
        }

        pAttrValue = xmlGetProp( actionNode, 
                                 (const xmlChar*) GID_TAG);
        if (pAttrValue) {
            ceError = GPAGetUserGID( (PCSTR)pAttrValue, &pAction->gid);
            if (ceError != CENTERROR_SUCCESS) {
                GPA_LOG_VERBOSE( "User group [%s] not found on this system.", (PCSTR)pAttrValue);
                goto error;
            }

            xmlFree(pAttrValue);
            pAttrValue = NULL;
        }

        pAttrValue = xmlGetProp( actionNode, 
                                 (const xmlChar*) MODE_TAG);
        if (pAttrValue) {
            pAction->permissions = strtoul( (PCSTR)pAttrValue, 
                                            NULL, 
                                            8);
            xmlFree(pAttrValue);
            pAttrValue = NULL;
        }

        if(actionType != CREATE_LINK) {
            pAttrValue = xmlGetProp( actionNode, 
                                     (const xmlChar*) DELETE_ON_POLICY_REVERT_TAG);
            if (pAttrValue) {
                char ch = (char)*pAttrValue;
                pAction->bDeleteOnRevert = ((ch == 'Y') || (ch == 'y') || (ch == '1') || (ch == 'T') || (ch == 't'));
                xmlFree(pAttrValue);
                pAttrValue = NULL;
            } else {
                /* This should match the way we set it on the AD side */
                pAction->bDeleteOnRevert = TRUE;
            }    
        }
    }

error:

    if (pAttrValue) {
        xmlFree(pAttrValue);
        pAttrValue = NULL;
    }

    if (pXmlNodeSetPtr) {
        xmlXPathFreeNodeSet(pXmlNodeSetPtr);
    }

    return ceError;

}

static
CENTERROR
BuildActionSourceInfo(
    xmlNodePtr xmlNode,
    FileActionType actionType,
    PCSTR pszSourcePathPrefix,
    PFILESPOLICYACTION pAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodeSetPtr pXmlNodeSetPtr = NULL;
    PSTR pszSourcePath = NULL;

    ceError = GPOXmlSelectNodes( xmlNode, 
                                 "source", 
                                 &pXmlNodeSetPtr);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pXmlNodeSetPtr && pXmlNodeSetPtr->nodeNr == 1) {    
        xmlNodePtr actionNode = pXmlNodeSetPtr->nodeTab[0];
    
        if (IsNullOrEmptyString(pszSourcePathPrefix)) {      
            ceError = GetFirstPathFromCDATA( actionNode, 
                                             &pAction->pszSourcePath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            LwStripWhitespace(pAction->pszSourcePath,1,1);      
        } 
        else {      
            ceError = GetFirstPathFromCDATA( actionNode, 
                                             &pszSourcePath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            LwStripWhitespace(pszSourcePath,1,1);
      
            if (pszSourcePathPrefix) {
                CHAR szPathBuf[PATH_MAX+1];
      
                sprintf( szPathBuf, 
                         "%s/%s", 
                         pszSourcePathPrefix, 
                         pszSourcePath);
      
                ceError = LwAllocateString( szPathBuf, 
                                            &pAction->pszSourcePath);
                BAIL_ON_CENTERIS_ERROR(ceError);
            } 
            else {
                pAction->pszSourcePath = pszSourcePath;
                pszSourcePath = NULL;
            }      
        }    
    }
  
error:

    if (pXmlNodeSetPtr) {
        xmlXPathFreeNodeSet(pXmlNodeSetPtr);
    }

    LW_SAFE_FREE_STRING(pszSourcePath);

    return ceError;

}

static
CENTERROR
BuildActionWithSourceInfo(
    xmlNodePtr xmlNode,
    FileActionType actionType,
    PCSTR pszSourcePathPrefix,
    PFILESPOLICYACTION* ppAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICYACTION pAction = NULL;

    ceError = LwAllocateMemory( sizeof(FILESPOLICYACTION), 
                                (PVOID*)&pAction);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pAction->actionType = actionType;

    ceError = BuildActionSourceInfo( xmlNode,
                                     actionType,
                                     pszSourcePathPrefix,
                                     pAction);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (IsNullOrEmptyString(pAction->pszSourcePath)) {
        ceError = CENTERROR_INVALID_VALUE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *ppAction = pAction;
    pAction = NULL;

error:

    SAFE_FREE_ACTION_LIST(pAction);

    return ceError;

}

static
CENTERROR
BuildActionWithCommandInfo(
    xmlNodePtr xmlNode,
    FileActionType actionType,
    PFILESPOLICYACTION* ppAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICYACTION pAction = NULL;

    ceError = LwAllocateMemory( sizeof(FILESPOLICYACTION), 
                                (PVOID*)&pAction);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pAction->actionType = actionType;

    ceError = BuildActionCommandInfo( xmlNode,
                                      actionType,
                                      pAction);
    if (ceError == CENTERROR_SUCCESS) {
        if (IsNullOrEmptyString(pAction->pszCommand)) {
            ceError = CENTERROR_INVALID_VALUE;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        *ppAction = pAction;
        pAction = NULL;
    } else {
        *ppAction = NULL;
        ceError = CENTERROR_SUCCESS;
    }

error:

    SAFE_FREE_ACTION_LIST(pAction);

    return ceError;
}

static
CENTERROR
BuildActionWithTargetInfo(
    xmlNodePtr xmlNode,
    FileActionType actionType,
    PFILESPOLICYACTION* ppAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICYACTION pAction = NULL;

    ceError = LwAllocateMemory( sizeof(FILESPOLICYACTION), 
                                (PVOID*)&pAction);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pAction->actionType = actionType;

    ceError = BuildActionTargetInfo( xmlNode,
                                     actionType,
                                     pAction);
    if (ceError == CENTERROR_SUCCESS) {
        if (IsNullOrEmptyString(pAction->pszTargetPath)) {
            ceError = CENTERROR_INVALID_VALUE;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        *ppAction = pAction;
        pAction = NULL;
    } else {
        *ppAction = NULL;
        ceError = CENTERROR_SUCCESS;
    }

error:

    SAFE_FREE_ACTION_LIST(pAction);

    return ceError;
}

static
CENTERROR
FixFilePaths(
    PFILESPOLICYACTION pAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bExists = FALSE;
    PSTR    pszSrcDirPath = NULL;
    PSTR    pszSrcFilename = NULL;

    if ( IsNullOrEmptyString(pAction->pszTargetPath) ||
         IsNullOrEmptyString(pAction->pszSourcePath)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPACheckDirectoryExists( pAction->pszTargetPath, 
                                      &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bExists) {
        DWORD dwNewLen = 0;

        ceError = CrackAbsoluteFilePath( pAction->pszSourcePath,
                                         &pszSrcDirPath,
                                         &pszSrcFilename);
        BAIL_ON_CENTERIS_ERROR(ceError);

        GPA_LOG_VERBOSE( "Including filename [%s] in target directory path [%s]", 
                         pszSrcFilename, 
                         pAction->pszTargetPath); 

        dwNewLen = strlen(pAction->pszTargetPath);
        dwNewLen += strlen(pszSrcFilename);
        dwNewLen += 2;

        ceError = LwReallocMemory( pAction->pszTargetPath,
                                   (PVOID*)&pAction->pszTargetPath,
                                    dwNewLen);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcat( pAction->pszTargetPath, 
                "/");
        strcat( pAction->pszTargetPath, 
                pszSrcFilename);
    }

error:

    LW_SAFE_FREE_STRING(pszSrcDirPath);
    LW_SAFE_FREE_STRING(pszSrcFilename);

    return ceError;

}

static
CENTERROR
BuildActionWithSourceAndTargetInfo(
    xmlNodePtr xmlNode,
    FileActionType actionType,
    PCSTR pszSourcePathPrefix,
    PFILESPOLICYACTION* ppAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICYACTION pAction = NULL;

    ceError = LwAllocateMemory( sizeof(FILESPOLICYACTION), 
                                (PVOID*)&pAction);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pAction->actionType = actionType;

    ceError = BuildActionSourceInfo( xmlNode, 
                                     actionType, 
                                     pszSourcePathPrefix, 
                                     pAction);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = BuildActionTargetInfo( xmlNode, 
                                     actionType, 
                                     pAction);
    if (ceError == CENTERROR_SUCCESS) {
        if ( IsNullOrEmptyString(pAction->pszSourcePath) ||
             IsNullOrEmptyString(pAction->pszTargetPath)) {
            ceError = CENTERROR_INVALID_VALUE;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        /* 
         * If the source path was a file and the target path was an existing folder
         * we need to change the target path to include the file name. Otherwise,
         * we will get an error when we copy or move the file
         */
        if ( (actionType == MOVE_FILE) ||
             (actionType == COPY_FILE)) {
            ceError = FixFilePaths(pAction);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        *ppAction = pAction;
        pAction = NULL;
    } else {
        *ppAction = NULL;
        ceError = CENTERROR_SUCCESS;
    }

error:

    SAFE_FREE_ACTION_LIST(pAction);

    return ceError;
}

static
CENTERROR
BuildDeleteFilePathActionFromString(
    PSTR pszPath,
    FileActionType actionType,
    PFILESPOLICYACTION* ppAction
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICYACTION pAction = NULL;

    ceError = LwAllocateMemory( sizeof(FILESPOLICYACTION), 
                                (PVOID*)&pAction);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pAction->actionType = actionType;

    ceError = LwAllocateString( pszPath, 
                                &pAction->pszSourcePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppAction = pAction;
    pAction = NULL;

error:

    SAFE_FREE_ACTION_LIST(pAction);

    return ceError;

}

static
CENTERROR
BuildFilesActionList(
    PSTR szDestFolderPath,
    PGPOLWIGPITEM pGPItem,
    PFILESPOLICYACTION * ppActionList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICYACTION pActionListHead = NULL;
    PFILESPOLICYACTION pActionListTail = NULL;
    PFILESPOLICYACTION pActionList = NULL;
    xmlNodeSetPtr pXmlNodeSetPtr = NULL;
    xmlNodePtr    pXmlNodePtr = NULL;
    xmlNodePtr    pChildNodePtr = NULL;
    int iNode = 0;

    ceError = GPOXmlSelectNodes( pGPItem->xmlNode,
                                 "lwifilesys/setting/filesys",
                                 &pXmlNodeSetPtr);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!pXmlNodeSetPtr) {
        goto done;
    }

    for (iNode = 0; iNode < pXmlNodeSetPtr->nodeNr; iNode++) {    
        pXmlNodePtr = pXmlNodeSetPtr->nodeTab[iNode];    

        for (pChildNodePtr = pXmlNodePtr->children; pChildNodePtr != NULL; pChildNodePtr = pChildNodePtr->next) {
            if (pChildNodePtr->type == XML_ELEMENT_NODE) {
                if (!strcmp( (PCSTR)pChildNodePtr->name, 
                             "file")) {
                    ceError = BuildActionWithSourceAndTargetInfo( pChildNodePtr, 
                                                                  MOVE_FILE, 
                                                                  szDestFolderPath, 
                                                                  &pActionList);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    APPEND_TO_LIST( pActionListHead, 
                                    pActionListTail, 
                                    pActionList);
                    pActionList = NULL;
                }
                else if (!strcmp( (PCSTR)pChildNodePtr->name, 
                                  "directory")) {
                    ceError = BuildActionWithTargetInfo( pChildNodePtr, 
                                                         CREATE_DIRECTORY, 
                                                         &pActionList);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    APPEND_TO_LIST( pActionListHead, 
                                    pActionListTail, 
                                    pActionList);
                    pActionList = NULL;
                }
                else if (!strcmp( (PCSTR)pChildNodePtr->name, 
                                  "link")) {
                    ceError = BuildActionWithSourceAndTargetInfo( pChildNodePtr, 
                                                                  CREATE_LINK, 
                                                                  NULL, 
                                                                  &pActionList);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    APPEND_TO_LIST( pActionListHead, 
                                    pActionListTail, 
                                    pActionList);
                    pActionList = NULL;
                }
                else if (!strcmp( (PCSTR)pChildNodePtr->name, 
                                  "command")) {
                    ceError = BuildActionWithCommandInfo( pChildNodePtr, 
                                                          EXECUTE_COMMAND, 
                                                          &pActionList);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    APPEND_TO_LIST( pActionListHead, 
                                    pActionListTail, 
                                    pActionList);
                    pActionList = NULL;
                }
                else {
                    GPA_LOG_WARNING( "Unrecognized child node [Name: %s] in lwifiles configuration", 
                                     pChildNodePtr->name);
                }
            }
        }
    }

    *ppActionList = pActionListHead;
    pActionListHead = NULL;

done:
error:

    if (pXmlNodeSetPtr) {
        xmlXPathFreeNodeSet(pXmlNodeSetPtr);
    }

    SAFE_FREE_ACTION_LIST(pActionListHead);
    SAFE_FREE_ACTION_LIST(pActionList);

    return ceError;

}

static
CENTERROR
BuildActionList(
    PFILESPOLICY pPolicy,
    PFILESPOLICYACTION* ppActionList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szPolicyHomeDir[PATH_MAX+1];

    if (!pPolicy->pGPItem) {
        *ppActionList = NULL;
        goto done;
    }

    sprintf( szPolicyHomeDir, 
             "%s/{%s}", 
             LWIFILES_CACHEDIR, 
             pPolicy->pszPolicyIdentifier);

    ceError = BuildFilesActionList( szPolicyHomeDir,
                                    pPolicy->pGPItem,
                                    ppActionList);
    BAIL_ON_CENTERIS_ERROR(ceError);

done:
error:

    return ceError;

}

static
CENTERROR
BuildRevertActionList(
    PSTR pszPolicyCacheDirPath,
    PFILESPOLICYACTION* ppActionList,
    BOOLEAN bRemoveFolder
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICYACTION pActionListHead = NULL;
    PFILESPOLICYACTION pActionListTail = NULL;
    PFILESPOLICYACTION pAction = NULL;
    CHAR szFilePath[PATH_MAX+1];
    BOOLEAN bExists = FALSE;
    xmlDocPtr xmlDoc = NULL;
    xmlNodeSetPtr pXmlNodeSetPtr = NULL;
    xmlChar* pszActionType = NULL;

    sprintf( szFilePath, 
             "%s/%s", 
             pszPolicyCacheDirPath, 
             LWIFILES_UNDOFILENAME);

    ceError = GPACheckFileExists( szFilePath, 
                                 &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bExists) {    
        xmlDoc = xmlReadFile( szFilePath, 
                              NULL, 
                              0);
        if (xmlDoc == NULL) {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    
        ceError = GPOXmlSelectNodes( (xmlNodePtr)xmlDoc,
                                     "lwifiles/action",
                                     &pXmlNodeSetPtr);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (pXmlNodeSetPtr && pXmlNodeSetPtr->nodeNr > 0) {
            int iNode = 0;

            for (iNode = 0; iNode < pXmlNodeSetPtr->nodeNr; iNode++) {
                xmlNodePtr actionNode = pXmlNodeSetPtr->nodeTab[iNode];
    
                pszActionType = xmlGetProp(actionNode, (xmlChar*)TYPE_TAG);
    
                if (!pszActionType) {
                    GPA_LOG_WARNING("Warning: action type is not set in lwi_gp_undo.xml");
                    continue;
                }
    
                if (!strcmp( (PCSTR)pszActionType, 
                             REMOVE_LINK_TAG)) {
                    ceError = BuildActionWithSourceInfo( actionNode, 
                                                         REMOVE_LINK, 
                                                         NULL, 
                                                         &pAction);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
                else if (!strcmp( (PCSTR)pszActionType, 
                                  CREATE_LINK_TAG)) {
                    ceError = BuildActionWithSourceAndTargetInfo( actionNode, 
                                                                  CREATE_LINK, 
                                                                  NULL, 
                                                                  &pAction);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
                else if (!strcmp( (PCSTR)pszActionType, 
                                  REMOVE_FILE_TAG)) {
                    ceError = BuildActionWithSourceInfo( actionNode, 
                                                         REMOVE_FILE, 
                                                         NULL, 
                                                         &pAction);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
                else if (!strcmp( (PCSTR)pszActionType, 
                                  MOVE_FILE_TAG)) {
                    ceError = BuildActionWithSourceAndTargetInfo( actionNode, 
                                                                  MOVE_FILE, 
                                                                  NULL, 
                                                                  &pAction);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
                else if (!strcmp( (PCSTR)pszActionType, 
                                  COPY_FILE_TAG)) {
                    ceError = BuildActionWithSourceAndTargetInfo( actionNode, 
                                                                  COPY_FILE, 
                                                                  NULL, 
                                                                  &pAction);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
                else if (!strcmp( (PCSTR)pszActionType, 
                                  REMOVE_DIR_TAG)) {
                    ceError = BuildActionWithSourceInfo( actionNode, 
                                                         REMOVE_DIRECTORY, 
                                                         NULL, 
                                                         &pAction);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
                else if (!strcmp( (PCSTR)pszActionType, 
                                  CREATE_DIR_TAG)) {
                    ceError = BuildActionWithTargetInfo( actionNode, 
                                                         CREATE_DIRECTORY, 
                                                         &pAction);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
                else {
                    GPA_LOG_WARNING( "Warning: Unrecognized action tag [%s]", 
                                     pszActionType);
                }   

                if (pszActionType) {
                    xmlFree(pszActionType);
                    pszActionType = NULL;
                }

                APPEND_TO_LIST( pActionListHead, 
                                pActionListTail, 
                                pAction);
                pAction = NULL;
            }
        }

        /* Remove the undo file itself */
        ceError = BuildDeleteFilePathActionFromString( szFilePath, 
                                                       REMOVE_FILE, &pAction);
        BAIL_ON_CENTERIS_ERROR(ceError);
    
        APPEND_TO_LIST( pActionListHead, 
                        pActionListTail, 
                        pAction);
        pAction = NULL;
    }

    /* Remove the backup folder if it exists */
      sprintf( szFilePath, 
               "%s/backup", 
               pszPolicyCacheDirPath);

    ceError = GPACheckDirectoryExists( szFilePath, 
                                      &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bExists) {
        ceError = BuildDeleteFilePathActionFromString( szFilePath,
                                                       REMOVE_DIRECTORY,
                                                          &pAction);
        BAIL_ON_CENTERIS_ERROR(ceError);
  
        APPEND_TO_LIST( pActionListHead, 
                        pActionListTail, 
                        pAction);
        pAction = NULL;
    }

    if (bRemoveFolder) {
        /*
         * Finally delete the policy directory itself
         * This is because we are reverting the entire policy
         */
        ceError = BuildDeleteFilePathActionFromString( pszPolicyCacheDirPath,
                                                       REMOVE_DIRECTORY,
                                                       &pAction);
        BAIL_ON_CENTERIS_ERROR(ceError);
  
        APPEND_TO_LIST( pActionListHead, 
                        pActionListTail, 
                        pAction);
        pAction = NULL;
    }

    *ppActionList = pActionListHead;
    pActionListHead = NULL;

error:

    if (pXmlNodeSetPtr) {
        xmlXPathFreeNodeSet(pXmlNodeSetPtr);
    }

    if (pszActionType) {
        xmlFree(pszActionType);
    }

    if (xmlDoc) {
        xmlFreeDoc(xmlDoc);
    }

    SAFE_FREE_ACTION_LIST(pActionListHead);
    SAFE_FREE_ACTION_LIST(pAction);

    return ceError;

}

static
CENTERROR
RevertPolicy(
    PSTR pszPolicyIdentifier,
    BOOLEAN bRemoveFolder
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICYACTION pActionList = NULL;
    PFILESPOLICYACTION pAction = NULL;
    CHAR szPolicyHomeDir[PATH_MAX+1];
    BOOLEAN bExists = FALSE;

    sprintf( szPolicyHomeDir, 
             "%s/{%s}", 
             LWIFILES_CACHEDIR, 
             pszPolicyIdentifier);

    ceError = GPACheckDirectoryExists( szPolicyHomeDir, 
                                      &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bExists)
        goto done;
    
    ceError = BuildRevertActionList( szPolicyHomeDir,
                                     &pActionList,
                                     bRemoveFolder);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for (pAction = pActionList; pAction; pAction = pAction->pNext) {
        ceError = gActionExecutors[pAction->actionType](szPolicyHomeDir, pAction, NULL);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

done:
error:

    SAFE_FREE_ACTION_LIST(pActionList);

    return ceError;

}

static
CENTERROR
RemoveFilesPolicy(
    PGROUP_POLICY_OBJECT pGPO
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

    GPA_LOG_VERBOSE( "Removing Files Policy [%s]", 
                     pszPolicyIdentifier);

    ceError = RevertPolicy( pszPolicyIdentifier, 
                            TRUE);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    return ceError;

}

static
CENTERROR
AddFilesPolicy(
    PGROUP_POLICY_OBJECT pGPO,
    PFILESPOLICY * ppPolicy
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDomainName = NULL;
    PSTR pszDC = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    CHAR szSrcFolderPath[PATH_MAX+1];
    CHAR szDestFolderPath[PATH_MAX+1];
    BOOLEAN bExists = FALSE;
    BOOLEAN bRemoveFolder = FALSE;
    BOOLEAN bMonitor = FALSE;
    PFILESPOLICY pPolicy = NULL;

    *szSrcFolderPath = '\0';
    *szDestFolderPath = '\0';
  
    ceError = GPACrackFileSysPath( pGPO->pszgPCFileSysPath,
                                   &pszDomainName,
                                   &pszSourcePath,
                                   &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetPreferredDomainController(pszDomainName, &pszDC);
    if (!CENTERROR_IS_OK(ceError)) {
        if ( CENTERROR_EQUAL(ceError, CENTERROR_GP_NO_SMB_KRB5_SITE_KDC_INFO) ||
             CENTERROR_EQUAL(ceError, CENTERROR_GP_NO_SMB_KRB5_SITE_INFO)) {
            GPA_LOG_ALWAYS( "GPAgent unable to obtain preferred server for AD site: FQDN(%s)", 
                            pszDomainName);
            ceError = CENTERROR_SUCCESS;
        } else {
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    sprintf( szDestFolderPath, 
             "%s/{%s}", 
             LWIFILES_CACHEDIR, 
             pszPolicyIdentifier);

    ceError = GPACheckDirectoryExists( szDestFolderPath, 
                                      &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bExists) {    
        ceError = LwCreateDirectory( szDestFolderPath, 
                                     S_IRWXU);
        BAIL_ON_CENTERIS_ERROR(ceError);

        bRemoveFolder = TRUE;
    }

    sprintf( szSrcFolderPath, 
             "/%s/Machine/Centeris/Identity/%s", 
             pszSourcePath, 
             LWIFILES_CLIENT_GUID);

    ceError = GPOLwioCopyFileMultiple(NULL,
                                      pszDomainName,
                                      pszDC,
                                      szSrcFolderPath,
                                      szDestFolderPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( sizeof(FILESPOLICY), 
                                (PVOID*)&pPolicy);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszPolicyIdentifier, 
                                &pPolicy->pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pPolicy->pGPO = pGPO;

    // Look in new policy file location
    ceError = GPOInitLwiData( NULL,
                              MACHINE_GROUP_POLICY,
                              (PGPOLWIDATA*)&pPolicy->pLwidata,
                              pPolicy->pGPO->pszgPCFileSysPath,
                              NULL,
                              LWIFILES_CLIENT_GUID );
    if ( ceError ) {
        // Look in old policy file location
        ceError = GPOInitLwiData( NULL,
                                  MACHINE_GROUP_POLICY,
                                  (PGPOLWIDATA*)&pPolicy->pLwidata,
                                  pPolicy->pGPO->pszgPCFileSysPath,
                                  NULL,
                                  NULL );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPOGetGPItem( MACHINE_GROUP_POLICY,
                            pPolicy->pLwidata,
                            LWIFILES_ITEM_GUID,
                            &pPolicy->pGPItem);
    if ( !CENTERROR_IS_OK(ceError) &&
         CENTERROR_EQUAL( ceError, 
                          CENTERROR_GP_XML_GPITEM_NOT_FOUND)) {
        pPolicy->pGPItem = NULL;
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GetMonitorFile( pPolicy->pGPItem->xmlNode,
                              "file",
                              &bMonitor);
    BAIL_ON_CENTERIS_ERROR(ceError);

    //set the monitor file flag
    gbMonitor = bMonitor;

    *ppPolicy = pPolicy;
    pPolicy = NULL;

    bRemoveFolder = FALSE;

error:

    if (bRemoveFolder) {
        GPARemoveDirectory(szDestFolderPath);
    }

    if (pPolicy)
        FreePolicyHierarchy(pPolicy);

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszDC);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    return ceError;

}

static
CENTERROR
SavePolicyUndoActions(
    PSTR pszPolicyHomeDir,
    PFILESPOLICYACTION pActionList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICYACTION pAction = NULL;
    FILE* fp = NULL;
    xmlDocPtr pDoc = NULL;
    xmlNodePtr pRoot = NULL;
    xmlNodePtr pElem = NULL;
    BOOLEAN bDeleteRootNode = FALSE;
    CHAR szPathBuf[PATH_MAX+1];

    pDoc = xmlNewDoc( (const xmlChar*)"1.0");
    if (pDoc == NULL) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_DOC;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pRoot = xmlNewDocNode( pDoc, 
                           NULL, 
                           (const xmlChar*)"lwifiles", 
                           NULL);
    if (pRoot == NULL) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    bDeleteRootNode = TRUE;

    xmlDocSetRootElement( pDoc, 
                          pRoot);

    bDeleteRootNode = FALSE;

    for (pAction = pActionList; pAction; pAction = pAction->pNext) {
        ceError = gUndoActionConfigurators[pAction->actionType]( pDoc,
                                                                 pAction,
                                                                 &pElem);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (NULL == xmlAddChild( pRoot, 
                                 pElem)) {
            ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        pElem = NULL;
    }

    sprintf( szPathBuf, 
             "%s/%s", 
             pszPolicyHomeDir, 
             LWIFILES_UNDOFILENAME);

    ceError = GPAOpenFile( szPathBuf, 
                          "w", 
                          &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( xmlDocDump( fp, 
                     pDoc) < 0) {
        ceError = CENTERROR_GP_XML_FAILED_TO_WRITE_DOC;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pDoc) {
        xmlFreeDoc(pDoc);
        pDoc = NULL;
    }

    ceError = GPACloseFile(fp);
    BAIL_ON_CENTERIS_ERROR(ceError);
    fp = NULL;

error:

    if (fp) {
        GPACloseFile(fp);
    }

    if (pElem) {
        xmlFreeNode(pElem);
    }

    if (bDeleteRootNode && pRoot) {
        xmlFreeNode(pRoot);
    }

    if (pDoc) {
        xmlFreeDoc(pDoc);
        pDoc = NULL;
    }

    return ceError;

}

static
CENTERROR
ApplyFilesPolicy(
    PFILESPOLICY pPolicyHierarchy
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICY pPolicy = NULL;
    PFILESPOLICY pGoodPolicy = NULL;
    PFILESPOLICYACTION pActionList = NULL;
    PFILESPOLICYACTION pUndoActionList = NULL;
    PFILESPOLICYACTION pUndoAction = NULL;
    PFILESPOLICYACTION pAction = NULL;
    PFILESPOLICYACTION pTmpAction = NULL;
    CHAR szPolicyHomeDir[PATH_MAX+1];
    BOOLEAN bExists = FALSE;

    /* Revert policies that may be applied prior to this time */
    for (pPolicy = pPolicyHierarchy; pPolicy; pPolicy = pPolicy->pNext) {
        ceError = RevertPolicy(pPolicy->pszPolicyIdentifier, FALSE);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pPolicy = pPolicyHierarchy;
    while (pPolicy && pPolicy->pNext) {
        if (pPolicy->pGPItem) {
            pGoodPolicy = pPolicy;
        }
        pPolicy = pPolicy->pNext;
    }

    if (pPolicy && pPolicy->pGPItem) {
        pGoodPolicy = pPolicy;
    }

    if (!pGoodPolicy) {
        goto done;
    }

    ceError = GPACheckDirectoryExists(LWIFILES_CACHEDIR, &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bExists) {
        ceError = LwCreateDirectory(LWIFILES_CACHEDIR, S_IRWXU);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
  
    GPA_LOG_VERBOSE( "Applying Files Policy [%s]", 
                     pGoodPolicy->pszPolicyIdentifier);

    sprintf( szPolicyHomeDir,
             "%s/{%s}",
             LWIFILES_CACHEDIR,
             pGoodPolicy->pszPolicyIdentifier);

    ceError = GPACheckDirectoryExists( szPolicyHomeDir, 
                                      &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bExists)
        goto done;
    
    ceError = BuildActionList( pGoodPolicy, 
                               &pActionList);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for (pAction = pActionList; pAction; pAction = pAction->pNext) {
        ceError = gActionExecutors[pAction->actionType]( szPolicyHomeDir,
                                                         pAction,
                                                         &pUndoAction);
        BAIL_ON_CENTERIS_ERROR(ceError);

        /*
         * We could get multiple undo actions
         * Add them in reverse order
         */
        while (pUndoAction) {
            pTmpAction = pUndoAction->pNext;
            pUndoAction->pNext = pUndoActionList;
            pUndoActionList = pUndoAction;
            pUndoAction = pTmpAction;
        }
        pUndoAction = NULL;
    }

    ceError = SavePolicyUndoActions( szPolicyHomeDir, 
                                     pUndoActionList);
    BAIL_ON_CENTERIS_ERROR(ceError);

done:
error:

    SAFE_FREE_ACTION_LIST(pActionList);
    SAFE_FREE_ACTION_LIST(pUndoAction);
    SAFE_FREE_ACTION_LIST(pUndoActionList);

    return ceError;

}

static
CENTERROR
ExtractPolicyIdentifierFromPath(
    PCSTR pszSrcPath,
    PSTR* ppszPolicyIdentifier
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDirPath = NULL;
    PSTR pszFilename = NULL;
    PSTR pszPolicyIdentifier = NULL;

    ceError = CrackAbsoluteFilePath( pszSrcPath, 
                                     &pszDirPath, 
                                     &pszFilename);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( strlen(pszFilename)-1,
                                (PVOID*)&pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    strncpy( pszPolicyIdentifier, 
             pszFilename + 1, 
             strlen(pszFilename) - 2);

    *ppszPolicyIdentifier = pszPolicyIdentifier;
    pszPolicyIdentifier = NULL;

error:

    LW_SAFE_FREE_STRING(pszDirPath);
    LW_SAFE_FREE_STRING(pszFilename);

    if (pszPolicyIdentifier) {
        LwFreeMemory(pszPolicyIdentifier);
    }

    return ceError;

}

static
CENTERROR
RemoveDefunctPolicies(
     PFILESPOLICY pPolicyList
     )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR* ppszDirPaths = NULL;
    DWORD dwNPaths = 0;
    int iPath = 0;
    BOOLEAN bExists = FALSE;
    BOOLEAN bFound = FALSE;
    PSTR pszPolicyIdentifier = NULL;
    PFILESPOLICY pPolicy = NULL;

    ceError = GPACheckDirectoryExists( LWIFILES_CACHEDIR, 
                                      &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bExists)
        goto done;

    ceError = GPAGetMatchingDirPathsInFolder( LWIFILES_CACHEDIR,
                                                "^\\{.*\\}$",
                                             &ppszDirPaths,
                                             &dwNPaths);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    for (iPath = 0; iPath < dwNPaths; iPath++) {      
        bFound = FALSE;
        for ( pPolicy = pPolicyList; pPolicy; pPolicy = pPolicy->pNext) {    
            if (strstr(ppszDirPaths[iPath], pPolicy->pszPolicyIdentifier)) {
                bFound = TRUE;
                break;
            }
        }
      
        if (!bFound) {    
            ceError = ExtractPolicyIdentifierFromPath( ppszDirPaths[iPath], 
                                                       &pszPolicyIdentifier);
            BAIL_ON_CENTERIS_ERROR(ceError);

            // This PolicyIdentifier is defunct
            ceError = RevertPolicy( pszPolicyIdentifier, 
                                   TRUE);
            BAIL_ON_CENTERIS_ERROR(ceError);

            LW_SAFE_FREE_STRING(pszPolicyIdentifier);
        }
    }

done:
error:

    if (dwNPaths && ppszDirPaths) {
        LwFreeStringArray( ppszDirPaths, 
                           dwNPaths);
    }

    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    return ceError;

}

static
CENTERROR
SavePolicyHierarchy(
    PFILESPOLICY pPolicyHierarchy
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE* fp = NULL;

    if (!pPolicyHierarchy) {
        BOOLEAN bExists = FALSE;

        ceError = GPACheckFileExists( LWIPOLICYSEQUENCEPATH, 
                                     &bExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bExists) {
            ceError = LwRemoveFile(LWIPOLICYSEQUENCEPATH);
            BAIL_ON_CENTERIS_ERROR(ceError);
            ceError = RemoveDefunctPolicies(pPolicyHierarchy);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        goto done;
    }
  
    ceError = GPAOpenFile( LWIPOLICYSEQUENCEPATH, 
                          "w", 
                          &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);
  
    while (pPolicyHierarchy) {
        fprintf( fp, 
                 "%s\n", 
                 pPolicyHierarchy->pszPolicyIdentifier);

        pPolicyHierarchy = pPolicyHierarchy->pNext;
  }

done:
error:
  
    if (fp) {
        GPACloseFile(fp);
    }
  
    return ceError;

}

CENTERROR
ProcessFilesGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICY pPolicyHierarchy = NULL;
    PFILESPOLICY pPolicy = NULL;
    PSTR pszNewMd5Sum = NULL;
    BOOLEAN bFileExists = FALSE;

    GPA_LOG_FUNCTION_ENTER();

    /*
     * Process old policies to remove
     */
    while ( pGPODeletedList ) {
        ceError = RemoveFilesPolicy(pGPODeletedList);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pGPODeletedList = pGPODeletedList->pNext;
    }

    /*
     * Process new files policies to add
     */
    while ( pGPOModifiedList )  {      
        BOOLEAN applicable;

        ceError = GPOXmlVerifyPlatformApplicable( NULL,
                                                  MACHINE_GROUP_POLICY,
                                                  pGPOModifiedList,
                                                  NULL,
                                                  &applicable);

        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!applicable) {
            GPA_LOG_VERBOSE( "GPO(%s) disabled by platform targeting",
                             pGPOModifiedList->pszDisplayName);

            ceError = RemoveFilesPolicy(pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);    
        } else if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE( "GPO(%s) is disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);

            ceError = RemoveFilesPolicy(pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);    
        }
        else if (pGPOModifiedList->bNewVersion) {

            ceError =  AddFilesPolicy( pGPOModifiedList, 
                                       &pPolicy);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else {
            ceError = UpdatePolicyHierarchy( pGPOModifiedList, 
                                             &pPolicy);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if(pPolicy) {
            pPolicy->pNext = pPolicyHierarchy;
            pPolicyHierarchy = pPolicy;
            pPolicy = NULL;
        }
      
        pGPOModifiedList = pGPOModifiedList->pNext;
    }

    pPolicyHierarchy = ReversePolicyHierarchy(pPolicyHierarchy);

    ceError = ApplyFilesPolicy(pPolicyHierarchy);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = SavePolicyHierarchy(pPolicyHierarchy);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(gbMonitor) {

        ceError = GPACheckFileExists( gScriptFile,
                                      &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if(bFileExists) {
            GPA_LOG_VERBOSE("Monitoring files Flag is set, Going to monitor file");
            ceError = MonitorSystemFiles( gScriptFile,
                                          g_szMd5Sum,
                                          &pszNewMd5Sum);
            BAIL_ON_CENTERIS_ERROR(ceError);

            strncpy( g_szMd5Sum,
                     pszNewMd5Sum,
                    (strlen(pszNewMd5Sum)));
        }
    }


error:

    if (pPolicyHierarchy)
        FreePolicyHierarchy(pPolicyHierarchy);

    if (pPolicy)
        FreePolicyHierarchy(pPolicy);

    if (pszNewMd5Sum) {
        LwFreeString(pszNewMd5Sum);
    }

    GPA_LOG_FUNCTION_LEAVE(ceError);
    
    return ceError;;

}

CENTERROR
ResetFilesGroupPolicy(
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFILESPOLICY pPolicyHierarchy = NULL;

    ceError = BuildPolicyHierarchy(&pPolicyHierarchy);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = RemoveDefunctPolicies(pPolicyHierarchy);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if (pPolicyHierarchy)
        FreePolicyHierarchy(pPolicyHierarchy);

    GPA_LOG_VERBOSE( "GP Extension (Reset) exit code: %.8x", 
                     ceError);
    
    return CENTERROR_SUCCESS;

}
