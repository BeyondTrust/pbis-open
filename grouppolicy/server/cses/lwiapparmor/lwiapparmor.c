#include "includes.h"

static
CENTERROR
CheckApparmorSupport(
    PBOOLEAN pbValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bAppArmorFileExists = FALSE;

    ceError = GPACheckFileExists( APPARMOR_INIT_FILE_SUSE,
                                 &bAppArmorFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(!bAppArmorFileExists) {
        ceError = GPACheckFileExists( APPARMOR_INIT_FILE_UBUNTU,
                                     &bAppArmorFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *pbValue = bAppArmorFileExists;

error:

    return ceError;
}

static
CENTERROR
AddPolicyFileToHierarchy(
    PSTR pszFileName,
    PSTR pszFilePath,
    PSTR pszMode,
    PSTR pszPolicyIdentifier
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PAPPARMORPOLICYLIST pTemp = g_PolicyHierarchyList;
    PAPPARMORPOLICYLIST pPrev = NULL;
    PAPPARMORPOLICYLIST pNew = NULL;

    ceError = LwAllocateMemory( sizeof(APPARMORPOLICYLIST),
                                (PVOID*)&pNew);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszFileName, 
                                &pNew->pszFileName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszFilePath, 
                                &pNew->pszFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszMode, 
                                &pNew->pszMode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszPolicyIdentifier, 
                                &pNew->pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while (pTemp) {
        pPrev = pTemp;
        pTemp = pTemp->pNext;
    }

    if (pPrev) {
        pPrev->pNext = pNew;
    } else {
        g_PolicyHierarchyList = pNew;
    }

    return ceError;

error:

    if (pNew) {
        if (pNew->pszFilePath) {
            LwFreeString(pNew->pszFilePath);
        }
        LwFreeMemory(pNew);
    }
    
    return ceError;
}

static
void
RemovePolicyFolderFromHierarchy(
    PSTR pszFilePath
    )
{
    PAPPARMORPOLICYLIST pTemp = g_PolicyHierarchyList;
    PAPPARMORPOLICYLIST pPrev = NULL;

    while ( pTemp ) {
        if ( !strcasecmp( (PCSTR)pTemp->pszFilePath,
                          (PCSTR)pszFilePath) ) {
            if (pPrev) {
                pPrev->pNext = pTemp->pNext;
            } else {
                g_PolicyHierarchyList = pTemp->pNext;
            }

            LwFreeString(pTemp->pszFileName);
            pTemp->pszFileName = NULL;

            LwFreeString(pTemp->pszFilePath);
            pTemp->pszFilePath = NULL;

            LwFreeString(pTemp->pszMode);
            pTemp->pszMode = NULL;

            LwFreeString(pTemp->pszPolicyIdentifier);
            pTemp->pszPolicyIdentifier = NULL;

            LwFreeMemory( pTemp );
            pTemp = NULL;

            break;
        }

        pPrev = pTemp;
        pTemp = pTemp->pNext;
    }
}

static 
CENTERROR
ParseAppArmorGPItem(
    xmlNodePtr root_node,
    PSTR pszDestFolderPath,
    PSTR pszPolicyIdentifier
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cur_node = NULL;
    xmlChar *filenameAttrValue = NULL;
    xmlChar *modeAttrValue = NULL;

    for (cur_node = root_node; cur_node; cur_node = cur_node->next) {

        if (cur_node->name && !strcmp( (char*)cur_node->name,
                                       LWIAPPARMOR_FILE)) {

            filenameAttrValue = xmlGetProp( cur_node,
                                            (const xmlChar*)LWIAPPARMOR_FILE_NAME);

            modeAttrValue = xmlGetProp( cur_node,
                                        (const xmlChar*)LWIAPPARMOR_MODE_NAME);
            ceError = AddPolicyFileToHierarchy( (char*)filenameAttrValue,
                                                pszDestFolderPath,
                                                (char*)modeAttrValue, 
                                                (char*)pszPolicyIdentifier);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ParseAppArmorGPItem( cur_node->children, 
                             pszDestFolderPath,
                             pszPolicyIdentifier);
    }

error:

    if ( filenameAttrValue ) {
        xmlFree( filenameAttrValue );
    }

    if ( modeAttrValue ) {
        xmlFree( modeAttrValue );
    }

    return ceError;
}

static
CENTERROR
AddAppArmorPolicy(
    PGROUP_POLICY_OBJECT pGPO
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szSourceFolderPath[256];
    char szDestFolderPath[256];
    char szNewFilePath[256];
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    BOOLEAN bFolderExists = FALSE;
    PSTR pszDC = NULL;
    PGPOLWIGPITEM pGPItem = NULL;
    PGPOLWIDATA pLwidata = NULL;

    if (!pGPO) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    memset( szSourceFolderPath, 
            0, 
            sizeof(szSourceFolderPath));

    memset( szDestFolderPath, 
            0, 
            sizeof(szDestFolderPath));

    memset( szNewFilePath, 
            0, 
            sizeof(szNewFilePath));
  
    ceError =  GPACrackFileSysPath( pGPO->pszgPCFileSysPath,
                                    &pszDomainName,
                                    &pszSourcePath,
                                    &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    // Look in new policy file location
    ceError = GPOInitLwiData( NULL,
                              MACHINE_GROUP_POLICY,
                              (PGPOLWIDATA*)&pLwidata,
                              pGPO->pszgPCFileSysPath,
                              NULL,
                              LWIAPPARMOR_CLIENT_GUID );
    if ( ceError ) {
        // Look in old policy file location
        ceError = GPOInitLwiData( NULL,
                                  MACHINE_GROUP_POLICY,
                                  (PGPOLWIDATA*)&pLwidata,
                                  pGPO->pszgPCFileSysPath,
                                  NULL,
                                  NULL );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPOGetGPItem( MACHINE_GROUP_POLICY,
                            pLwidata,
                            LWIAPPARMOR_ITEM_GUID,
                            &pGPItem);
    if (!CENTERROR_IS_OK(ceError) &&
        CENTERROR_EQUAL( ceError, 
                         CENTERROR_GP_XML_GPITEM_NOT_FOUND)) {
        ceError = CENTERROR_SUCCESS;
        goto done;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetPreferredDomainController(pszDomainName, &pszDC);
    if (!CENTERROR_IS_OK(ceError)) {
        if (CENTERROR_EQUAL( ceError, 
                             CENTERROR_GP_NO_SMB_KRB5_SITE_KDC_INFO)    ||
            CENTERROR_EQUAL( ceError, 
                             CENTERROR_GP_NO_SMB_KRB5_SITE_INFO)) {

            GPA_LOG_ALWAYS( "GPAgent unable to obtain preferred server for AD site: FQDN(%s)", 
                            pszDomainName);
            ceError = CENTERROR_SUCCESS;
       } else {
           BAIL_ON_CENTERIS_ERROR(ceError);
       }
    }

    sprintf( szSourceFolderPath, 
             "%s\\Machine\\Centeris\\Identity\\%s", 
             pszSourcePath, 
             LWIAPPARMOR_CLIENT_GUID);

    sprintf( szDestFolderPath,
             "%s%s_likewise_apparmor",
             CENTERIS_GP_DIRECTORY,
             pszPolicyIdentifier);

    ceError = GPACheckDirectoryExists( szDestFolderPath, 
                                      &bFolderExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFolderExists) {
        ceError = GPARemoveDirectory(szDestFolderPath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwCreateDirectory( szDestFolderPath,
                                 S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPOLwioCopyFileMultiple(NULL,
                                      pszDomainName,
                                      pszDC,
                                      szSourceFolderPath,
                                      szDestFolderPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ParseAppArmorGPItem( pGPItem->xmlNode,
                                   szDestFolderPath,
                                   pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

done:
error:

    if (pszDomainName) {
        LwFreeString(pszDomainName);
    }

    if (pszSourcePath) {
        LwFreeString(pszSourcePath);
    }

    if (pszPolicyIdentifier) {
        LwFreeString(pszPolicyIdentifier);
    }

    if (pszDC) {
        LwFreeString(pszDC);
    }

    if ( pGPItem ) {
        GPODestroyGPItem( pGPItem,
                          FALSE );
    }

    if ( pLwidata ) {
        GPODestroyLwiData(pLwidata);
    }

    return(ceError);
}

static
CENTERROR
BackupAppmrSysPrfFile()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;

    ceError = GPACheckFileExists( LWIAPPARMOR_SYSTEM_PROFILE_FILEPATH_GP,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(!bFileExists) {
        ceError = GPACheckFileExists( LWIAPPARMOR_SYSTEM_PROFILE_FILEPATH,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if(bFileExists) {
            GPA_LOG_VERBOSE( "Backing up Apparmor profile system file. Source Path [%s] Dest Path [%s]",
                             LWIAPPARMOR_SYSTEM_PROFILE_FILEPATH,
                             LWIAPPARMOR_SYSTEM_PROFILE_FILEPATH_GP);

            ceError = GPACopyFileWithOriginalPerms( LWIAPPARMOR_SYSTEM_PROFILE_FILEPATH,
                                                   LWIAPPARMOR_SYSTEM_PROFILE_FILEPATH_GP);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

error:

    return ceError;
}

static
CENTERROR
ValidateAndBackupFile(
    PAPPARMORPOLICYLIST pPolicyList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;

    CHAR szBuf[PATH_MAX+1];
    CHAR szSrcBuf[PATH_MAX+1];
    CHAR szDestPath[PATH_MAX+1];

    sprintf( szBuf,
             "%s/%s_likewise_apparmor/%s",
             CENTERIS_GP_DIRECTORY,
             pPolicyList->pszPolicyIdentifier,
             pPolicyList->pszFileName);

    ceError = GPACheckFileExists( szBuf,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bFileExists) {
        sprintf( szSrcBuf,
                 "%s/%s_likewise_gp",
                 LWI_APPARMOR_PATH,
                 pPolicyList->pszFileName);

        /* Back up the original so that we are able to revert to it. */
        ceError = GPACheckFileExists( szSrcBuf,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            sprintf( szDestPath,
                     "%s/%s_likewise_apparmor",
                     CENTERIS_GP_DIRECTORY,
                     pPolicyList->pszPolicyIdentifier);

            ceError = GPACheckDirectoryExists( szDestPath,
                                              &bFileExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (!bFileExists) {
                ceError = LwCreateDirectory( szDestPath,
                                             S_IRUSR|S_IWUSR);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            GPA_LOG_VERBOSE( "Backing up original %s apparmor profile file. Source Path [%s] Dest Path [%s]",
                             pPolicyList->pszFileName,
                             szSrcBuf,
                             szBuf);

            ceError = GPAMoveFileAcrossDevices( szSrcBuf,
                                               szBuf);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

error:

    return ceError;
}


static
CENTERROR
ApplyPolicy(
    PAPPARMORPOLICYLIST pPolicyList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    CHAR szBuf[PATH_MAX+1];
    CHAR szSrcBuf[PATH_MAX+1];
    CHAR szCommand[2 * PATH_MAX + 1];

    sprintf( szSrcBuf,
             "%s%s_likewise_apparmor/%s",
             CENTERIS_GP_DIRECTORY,
             pPolicyList->pszPolicyIdentifier,
             pPolicyList->pszFileName);

    sprintf( szBuf,
             "%s/%s_likewise_gp",
             LWI_APPARMOR_PATH,
             pPolicyList->pszFileName);

    /* Now move a copy of the new apparmor file to /etc/apparmor.d */
    GPA_LOG_VERBOSE("Applying group policy version of apparmor profiles.");

    ceError = GPACopyFileWithOriginalPerms( szSrcBuf,
                                           szBuf);
    BAIL_ON_CENTERIS_ERROR(ceError);

    GPA_LOG_VERBOSE( "Source Path [%s] Dest Path [%s]",
                     szSrcBuf,
                     LWI_APPARMOR_PATH);

    /* Execute the command(s) here */
    sprintf( szCommand,
             "%s %s/%s_likewise_gp",
             pPolicyList->pszMode,
             LWI_APPARMOR_PATH,
             pPolicyList->pszFileName);

    GPA_LOG_VERBOSE("AppArmor policy CSE going to execute command [%s]", szCommand);

    ceError = GPARunCommand(szCommand);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(ceError != CENTERROR_SUCCESS) {
        ceError = CENTERROR_SUCCESS;
        goto done;
    }

done:
error:

    return ceError;
}

static
CENTERROR
ProcessAppArmorPolicyFiles()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PAPPARMORPOLICYLIST pPolicyList = g_PolicyHierarchyList;

    ceError = BackupAppmrSysPrfFile();
    BAIL_ON_CENTERIS_ERROR(ceError);

    while (pPolicyList) {
        ceError = ValidateAndBackupFile(pPolicyList);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = ApplyPolicy(pPolicyList);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pPolicyList = pPolicyList->pNext;
    }

error:

    return ceError;
}

static
CENTERROR
UpdateApparmorProfileDB()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szCommand[PATH_MAX];
    PSTR pszAppArmorCtrlPath = NULL;
    BOOLEAN bAppArmorFileExists = FALSE;

    ceError = GPAFindFileInPath( "rcapparmor", "/sbin:/bin",
                                &pszAppArmorCtrlPath);
    if(ceError == CENTERROR_FILE_NOT_FOUND) {
        ceError = GPACheckFileExists( APPARMOR_INIT_FILE_UBUNTU,
                                     &bAppArmorFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if(bAppArmorFileExists) {
            ceError = LwAllocateMemory( PATH_MAX,
                                        (PVOID *)&pszAppArmorCtrlPath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pszAppArmorCtrlPath,
                     "%s",
                     APPARMOR_INIT_FILE_UBUNTU);
        }        
    }

    if (pszAppArmorCtrlPath) {
        sprintf( szCommand,
                 "%s restart",
                 pszAppArmorCtrlPath);

        ceError = GPARunCommand(szCommand);
        if(ceError != CENTERROR_SUCCESS) {
            ceError = CENTERROR_SUCCESS;
            goto done;
        }
    } else {
        GPA_LOG_VERBOSE("Could not find AppArmor daemon. Hence, skipping updating the AppArmor system profile file.");
    }

done:
error:

    LwFreeString(pszAppArmorCtrlPath);

    return ceError;
}

static
CENTERROR
RemoveApmrProfileFiles(
    PSTR pszPolicyIdentifier
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PAPPARMORPOLICYLIST pTemp = g_PolicyHierarchyList;
    CHAR szFilePath[PATH_MAX];
    CHAR szFilePathGP[PATH_MAX];
    BOOLEAN bAppmrPrfFileExists = FALSE;

    while(pTemp) {
        if( !strcmp( (PCSTR)pszPolicyIdentifier,
                     (PCSTR)pTemp->pszPolicyIdentifier)) {
            sprintf( szFilePathGP,
                     "%s/%s_likewise_apparmor/%s",
                     CENTERIS_GP_DIRECTORY,
                     pszPolicyIdentifier,
                     pTemp->pszFileName);

            sprintf( szFilePath,
                     "%s/%s_likewise_gp",
                     LWI_APPARMOR_PATH,
                     pTemp->pszFileName);

            ceError = GPACheckFileExists( szFilePathGP,
                                         &bAppmrPrfFileExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if(bAppmrPrfFileExists) {
                GPA_LOG_VERBOSE( "Reverting apparmor profile %s from Path [%s] Dest Path [%s]",
                                 pTemp->pszFileName,
                                 szFilePathGP,
                                 szFilePath);

                ceError = GPAMoveFileAcrossDevices( szFilePathGP,
                                                   szFilePath);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
            else {
                ceError = LwRemoveFile(szFilePath);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            RemovePolicyFolderFromHierarchy(pTemp->pszFileName);
        }

        pTemp = pTemp->pNext;
    }

    ceError = UpdateApparmorProfileDB();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
RemoveAppArmorPolicy(
    PSTR pszgPCFileSyspath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szFolderPath[PATH_MAX];
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    BOOLEAN bDirExists = FALSE;

    ceError = GPACrackFileSysPath( pszgPCFileSyspath,
                                   &pszDomainName,
                                   &pszSourcePath,
                                   &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = RemoveApmrProfileFiles(pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf( szFolderPath,
             "%s/%s_likewise_apparmor",
             CENTERIS_GP_DIRECTORY,
             pszPolicyIdentifier);

    ceError = GPACheckDirectoryExists( szFolderPath,
                                      &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bDirExists) {
        ceError = GPARemoveDirectory(szFolderPath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    sprintf( szFolderPath, 
             "/bin/rm -rf %s/%s_likewise_apparmor* >/dev/null 2>&1",
             CENTERIS_GP_DIRECTORY,
             pszPolicyIdentifier);

    ceError = GPARunCommand(szFolderPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateApparmorProfileDB();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    LwFreeString(pszDomainName);
    pszDomainName = NULL;

    LwFreeString(pszSourcePath);
    pszSourcePath = NULL;

    LwFreeString(pszPolicyIdentifier);
    pszPolicyIdentifier = NULL;

    return ceError;
}

static
void
ResetPolicyFileHierarchy()
{
    PAPPARMORPOLICYLIST pTemp = NULL;

    while(g_PolicyHierarchyList) {

        pTemp = g_PolicyHierarchyList;
        g_PolicyHierarchyList = g_PolicyHierarchyList->pNext;

        LwFreeString(pTemp->pszFileName);
        pTemp->pszFileName = NULL;

        LwFreeString(pTemp->pszFilePath);
        pTemp->pszFilePath = NULL;

        LwFreeString(pTemp->pszMode);
        pTemp->pszMode = NULL;

        LwFreeString(pTemp->pszPolicyIdentifier);
        pTemp->pszPolicyIdentifier = NULL;

        if(pTemp) {
            LwFreeMemory(pTemp);
            pTemp = NULL;
        }
    }
}

static
void
ResetApparmorProfileCache()
{
    PAPPARMORPOLICYLIST pTemp = NULL;

    while(g_PolicyHierarchyList) {
        pTemp = g_PolicyHierarchyList;
        g_PolicyHierarchyList = g_PolicyHierarchyList->pNext;

        LwFreeString(pTemp->pszFileName);
        pTemp->pszFileName = NULL;

        LwFreeString(pTemp->pszFilePath);
        pTemp->pszFilePath = NULL;

        LwFreeString(pTemp->pszMode);
        pTemp->pszMode = NULL;

        LwFreeString(pTemp->pszPolicyIdentifier);
        pTemp->pszPolicyIdentifier = NULL;

        if(pTemp) {
            LwFreeMemory(pTemp);
            pTemp = NULL;
        }
    }
}

static
CENTERROR
RemoveAppArmorGPCacheFiles()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR  szCommand[2 * PATH_MAX + 1];

    sprintf( szCommand, 
             "/bin/rm -rf %s >/dev/null 2>&1",
             CACHEDIR "/*apparmor");

    ceError = GPARunCommand(szCommand);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
RemoveAppArmorGPSystemFiles()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR  szCommand[2 * PATH_MAX + 1];

    sprintf( szCommand, 
             "/bin/rm -rf %s >/dev/null 2>&1",
             "/etc/apparmor.d/*likewise_gp");

    ceError = GPARunCommand(szCommand);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
RemoveAppArmorSysBackedUpFile()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR  szCommand[2 * PATH_MAX + 1];

    sprintf( szCommand, 
             "/bin/rm -rf %s >/dev/null 2>&1",
             LWIAPPARMOR_SYSTEM_PROFILE_FILEPATH_GP);

    ceError = GPARunCommand(szCommand);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
RemoveAppArmorGPFiles()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = RemoveAppArmorGPCacheFiles();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = RemoveAppArmorGPSystemFiles();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = RemoveAppArmorSysBackedUpFile();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}
#if 0
static
CENTERROR
ClearPrevAppArmorGroupPolicy()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    //Reset before applying policy.Fix for Bug#7475
    ceError = RemoveAppArmorGPFiles();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ResetApparmorProfileCache();

    ceError = UpdateApparmorProfileDB();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}
#endif

CENTERROR
ResetAppArmorGroupPolicy(
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOG_ALWAYS( "Resetting AppArmor grouppolicy settings.");

    ceError = RemoveAppArmorGPFiles();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ResetApparmorProfileCache();

    ceError = UpdateApparmorProfileDB();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:
    return ceError;
}

CENTERROR
ProcessAppArmorGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bApparmorSupport = FALSE;
    BOOLEAN bModListExists = FALSE;
  
    GPA_LOG_FUNCTION_ENTER();
  
    ceError = CheckApparmorSupport(&bApparmorSupport);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bApparmorSupport) {
        GPA_LOG_VERBOSE("The AppArmor is not installed, hence skipping the Apparmor group policy...");
        ceError = CENTERROR_SUCCESS;
        goto done;
    }

    /*
     * Process old apparmor policies to remove
     */
    while ( pGPODeletedList ) {    
        ceError = RemoveAppArmorPolicy( pGPODeletedList->pszgPCFileSysPath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pGPODeletedList = pGPODeletedList->pNext;
    }

    ResetPolicyFileHierarchy();

    /*
     * Process new apparmor policies to add
     */
    while ( pGPOModifiedList ) {
        BOOLEAN applicable;
        
        bModListExists = TRUE;

        ceError = GPOXmlVerifyPlatformApplicable( NULL,
                                                  MACHINE_GROUP_POLICY,
                                                  pGPOModifiedList,
                                                  NULL,
                                                  &applicable);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        if (!applicable) {
            GPA_LOG_VERBOSE( "GPO(%s) disabled by platform targeting",
                             pGPOModifiedList->pszDisplayName);

            ceError = RemoveAppArmorPolicy( pGPOModifiedList->pszgPCFileSysPath);
            BAIL_ON_CENTERIS_ERROR(ceError);

        } else if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE( "GPO(%s) is disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);

            ceError = RemoveAppArmorPolicy( pGPOModifiedList->pszgPCFileSysPath);
            BAIL_ON_CENTERIS_ERROR(ceError);

        } else {

            ceError = AddAppArmorPolicy(pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    
        pGPOModifiedList = pGPOModifiedList->pNext;
    }

    if( g_PolicyHierarchyList ) {  
        ceError = ProcessAppArmorPolicyFiles();
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = UpdateApparmorProfileDB();
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else {

       	ceError = ResetAppArmorGroupPolicy(pUser);
       	BAIL_ON_CENTERIS_ERROR(ceError);
    }
  
done:
error:
  
    GPA_LOG_FUNCTION_LEAVE(ceError);
  
    return(ceError);
}
