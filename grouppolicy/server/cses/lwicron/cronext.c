#include "includes.h"


static PPOLICYLIST g_CronPolicyHierarchyList = NULL;
static PPOLICYLIST g_CronPolicyRemoveList    = NULL;

//Global variable to preserve the previously calculated MD5Sum
static char g_szMd5Sum[256] = "";
static BOOLEAN gbMonitor; 
static CHAR gCronFile[PATH_MAX+1];


static
CENTERROR
CheckCrontabSupport(
    PBOOLEAN pbValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bCrontabFileExists = FALSE;
    PSTR crontabPath = NULL;

    ceError = GPAFindFileInPath( "crontab", "/usr/local/bin:/usr/bin:/bin", 
                                &crontabPath);
    if(ceError == CENTERROR_FILE_NOT_FOUND) {
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    bCrontabFileExists = TRUE;

error:

    LW_SAFE_FREE_STRING(crontabPath);

    *pbValue = bCrontabFileExists;

    return ceError;
}

static
CENTERROR
BackupCrontab(
    PSTR pszCronFile
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szCommand[PATH_MAX + 25];
    BOOLEAN bFileExists = 0;
    mode_t mode = 0644;
    uid_t  uid  = 0;
    gid_t  gid  = 0;

    GPA_LOG_VERBOSE( "Creating backup of root crontab to file: %s", 
                     pszCronFile);

    ceError = GPACheckFileExists( CRONTAB_FILE,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {

        GPA_LOG_VERBOSE("Backing up original crontab file.");
        //Get UID GID and mode before backing up
        ceError = LwGetOwnerAndPermissions( CRONTAB_FILE,
                                            &uid,
                                            &gid,
                                            &mode);
        BAIL_ON_CENTERIS_ERROR(ceError);

    }

    /*
     * Backup current root crontab settings.
     */
    memset( szCommand, 
            0, 
            PATH_MAX + 25);

    sprintf ( szCommand, 
              "crontab -l >> %s", 
              pszCronFile);

    if (system(szCommand) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwChangeOwnerAndPermissions( pszCronFile, 
                                           uid,
                                           gid,
                                           mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
UpdateCrontab(
    PSTR pszCronFile
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szCommand[PATH_MAX + 1];

    if (pszCronFile) {
        GPA_LOG_VERBOSE( "Updating root crontab with setting from policy: %s", 
                         pszCronFile);

        memset( szCommand, 
                0, 
                PATH_MAX + 1);

        sprintf( szCommand, 
                 "crontab %s", 
                 pszCronFile);

        // Apply policy file to root crontab settings.
        if (system(szCommand) < 0) {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

error:

    return ceError;
}

static
CENTERROR
AddPolicyFileToHierarchy(
    PSTR szFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pTemp = g_CronPolicyHierarchyList;
    PPOLICYLIST pPrev = NULL;
    PPOLICYLIST pNew = NULL;

    ceError = LwAllocateMemory( sizeof(POLICYLIST), 
                                (PVOID*)&pNew);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( szFileName, 
                                &pNew->FileName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while (pTemp) {
        pPrev = pTemp;
        pTemp = pTemp->next;
    }

    if (pPrev) {
        pPrev->next = pNew;
    } else {
        g_CronPolicyHierarchyList = pNew;
    }

    return ceError;

error:

    if (pNew) {
        if (pNew->FileName) {
            LwFreeString(pNew->FileName);
        }
        LwFreeMemory(pNew);
    }

    return ceError;
}

static
CENTERROR
AddPolicyFileToRemoveList(
    PSTR szFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pTemp = g_CronPolicyRemoveList;
    PPOLICYLIST pPrev = NULL;
    PPOLICYLIST pNew = NULL;

    ceError = LwAllocateMemory( sizeof(POLICYLIST), 
                                (PVOID*)&pNew);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( szFileName, 
                                &pNew->FileName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while (pTemp) {
        pPrev = pTemp;
        pTemp = pTemp->next;
    }

    if (pPrev) {
        pPrev->next = pNew;
    } else {
        g_CronPolicyRemoveList = pNew;
    }

    return ceError;

error:

    if (pNew) {
        if (pNew->FileName) {
            LwFreeString(pNew->FileName);
        }
        LwFreeMemory(pNew);
    }

    return ceError;
}

static
CENTERROR
RemovePolicyFileFromRemoveList(
    PSTR szFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pTemp = g_CronPolicyRemoveList;
    PPOLICYLIST pPrev = NULL;
    PPOLICYLIST pDelete = NULL;

    while (pTemp) {
        if ( strcasecmp( pTemp->FileName, 
                         szFileName ) == 0 ) {
            pDelete = pTemp;
            if ( pPrev ) {
                pPrev->next = pTemp->next;
                pTemp = pTemp->next;
            } else {
                pTemp = pTemp->next;
                g_CronPolicyRemoveList = pTemp;
            }
            LwFreeString(pDelete->FileName);
            LwFreeMemory( pDelete );
        } else {
            pPrev = pTemp;
            pTemp = pTemp->next;
        }
    }

    return ceError;
}

static
CENTERROR
GetCurrentListOfCronDPolicies()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];
    FILE* pFile = NULL;
    BOOLEAN fExists = FALSE;

    ceError = GPACheckDirectoryExists( "/etc/cron.d", 
                                      &fExists );
    BAIL_ON_CENTERIS_ERROR( ceError );

#if defined(__LWI_DARWIN__)
    sprintf( szBuf, 
             "ls /etc/cron.d/*_likewise_crond 2> /dev/null");
#else
    sprintf( szBuf, 
             "UNIX95=1 ls /etc/cron.d/*_likewise_crond 2> /dev/null");
#endif

    if ( fExists ) {

        pFile = popen( szBuf, 
                       "r");
        if (pFile == NULL) {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        while (TRUE) {
            if (NULL == fgets( szBuf, 
                               PATH_MAX, 
                               pFile)) {
                if (feof(pFile))
                     break;
                else {
                    ceError = LwMapErrnoToLwError(errno);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
            }

            LwStripWhitespace(szBuf,1,1);
            if (!IsNullOrEmptyString(szBuf)) {
                ceError = AddPolicyFileToRemoveList( szBuf );
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }
    }

error:

    if (pFile)
        pclose(pFile);

    return ceError;
}

static
CENTERROR
GetCurrentListOfCrontabPolicies()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];
    FILE* pFile = NULL;
    BOOLEAN fExists = FALSE;

    ceError = GPACheckDirectoryExists(CENTERIS_GP_DIRECTORY, 
                                     &fExists );
    BAIL_ON_CENTERIS_ERROR( ceError );

#if defined(__LWI_DARWIN__)
    sprintf( szBuf, "ls %s*_likewise_crontab 2> /dev/null", CENTERIS_GP_DIRECTORY);
#else
    sprintf( szBuf, "UNIX95=1 ls %s*_likewise_crontab 2> /dev/null", CENTERIS_GP_DIRECTORY);
#endif

    if ( fExists ) {

        pFile = popen( szBuf, 
                       "r");
        if (pFile == NULL) {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        while (TRUE) {
            if (NULL == fgets( szBuf, 
                               PATH_MAX, 
                               pFile)) {
                if (feof(pFile))
                     break;
                else {
                    ceError = LwMapErrnoToLwError(errno);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
            }

            LwStripWhitespace(szBuf,1,1);
            if (!IsNullOrEmptyString(szBuf)) {
                ceError = AddPolicyFileToRemoveList( szBuf );
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }
    }

error:

    if (pFile)
        pclose(pFile);

    return ceError;
}

static
CENTERROR
ProcessCrontabPolicyFiles(
    BOOLEAN bSupportCronD
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pPolicyList = g_CronPolicyHierarchyList;
    BOOLEAN bFileExists = FALSE;
    mode_t mode = 0644;
    uid_t  uid  = 0;
    gid_t  gid  = 0;

    GPA_LOG_INFO("Processing Crontab list...");

    while (pPolicyList && pPolicyList->next) {
        GPA_LOG_INFO( "Crontab List Entry: %s", 
                      pPolicyList->FileName);
        // Walk to last crontab file in our list
        pPolicyList = pPolicyList->next;
    }

    GPA_LOG_INFO( "The last crontab list entry: %s", 
                  pPolicyList ? pPolicyList->FileName : "<none>");

    ceError = GPACheckFileExists( CRONTAB_OLD_FILE, 
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* If no pPolicyList item, then we should restore system to original setup. */
    if (!pPolicyList) {

        if (bFileExists && !gIsCronD) {
            GPA_LOG_VERBOSE("Reverting root crontab to original settings.");

            LwRemoveFile(CRONTAB_FILE);

            ceError = GPAMoveFileAcrossDevices( CRONTAB_OLD_FILE,
                                                CRONTAB_FILE);
            BAIL_ON_CENTERIS_ERROR(ceError);

            /* Update the root crontab settings */
            ceError = UpdateCrontab(CRONTAB_FILE);
            BAIL_ON_CENTERIS_ERROR(ceError);

            //Reset md5sum
            memset(g_szMd5Sum,0,sizeof(g_szMd5Sum));
        }
    } else {
        if (!bFileExists) {
            /* Back up the original crontab settings to a file so that we are able to revert to it. */
            ceError = BackupCrontab(CRONTAB_OLD_FILE);
            BAIL_ON_CENTERIS_ERROR(ceError);

        } else {
            //If the file exists, get the permission from crontab.lwidentity.orig
            ceError = LwGetOwnerAndPermissions( CRONTAB_OLD_FILE,
                                                &uid,
                                                &gid,
                                                &mode);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        /* Now apply the current policy crontab file to the root account crontab settings. */
        GPA_LOG_VERBOSE("Applying group policy version of crontab to root account crontab settings.");

        ceError = GPACopyFileWithOriginalPerms( pPolicyList->FileName,
                                               CRONTAB_FILE);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwChangeOwnerAndPermissions( CRONTAB_FILE,
                                               uid,
                                               gid,
                                               mode);
        BAIL_ON_CENTERIS_ERROR(ceError);

        //Reset md5sum
        memset(g_szMd5Sum,0,sizeof(g_szMd5Sum));

        GPA_LOG_VERBOSE( "Copied from Source Path [%s] Dest Path [%s]",
                         pPolicyList->FileName,
                         CRONTAB_FILE);

        ceError = UpdateCrontab(CRONTAB_FILE);
        BAIL_ON_CENTERIS_ERROR(ceError);


    }

error:

    return ceError;
}

static
CENTERROR
ProcessPolicyRemoveList()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pPolicyList = g_CronPolicyRemoveList;
    BOOLEAN bFileExists = 0;

    GPA_LOG_INFO("Processing CrontD remove list...");

    while (pPolicyList) {
        GPA_LOG_INFO( "CronD Remove List Entry: %s", 
                      pPolicyList->FileName);

        ceError = GPACheckFileExists( pPolicyList->FileName, 
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            GPA_LOG_VERBOSE("Removing stale CronD file.");
            LwRemoveFile(pPolicyList->FileName);
        }
        pPolicyList = pPolicyList->next;
    }

    /* Clear the list now that we have processed it */
    ResetPolicyRemoveList();

error:

    return ceError;
}

static
void
ResetPolicyFileHierarchy(
    )
{
    while (g_CronPolicyHierarchyList) {
        PPOLICYLIST pTemp = g_CronPolicyHierarchyList;
        g_CronPolicyHierarchyList = g_CronPolicyHierarchyList->next;
        if (pTemp->FileName) {
            LwFreeString(pTemp->FileName);
        }
        LwFreeMemory( pTemp );
    }

    //reset the gIsCronD Flag
    gIsCronD = FALSE;
}

static
void
ResetPolicyRemoveList(
    )
{
    while (g_CronPolicyRemoveList) {
        PPOLICYLIST pTemp = g_CronPolicyRemoveList;
        g_CronPolicyRemoveList = g_CronPolicyRemoveList->next;
        if (pTemp->FileName) {
            LwFreeString(pTemp->FileName);
        }
        LwFreeMemory( pTemp );
    }
}

#if defined(__LWI_SOLARIS__) || defined(__LWI_DARWIN__)
static
CENTERROR
RefreshCron()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

#if defined(__LWI_SOLARIS__)
    char szCommand[PATH_MAX + 1];

    memset( szCommand, 
            0, 
            PATH_MAX + 1);

    sprintf( szCommand, 
             "svcadm refresh svc:/system/cron");

    GPA_LOG_VERBOSE("Refreshing Cron on Solaris");

    if (system(szCommand) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        GPA_LOG_VERBOSE( "Could not HUP cron process: [%d]", ceError);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
#elif defined (__LWI_DARWIN__)
    pid_t pid_cron = 0;
    FILE *fp = NULL;
    char  szBuf[BUFSIZE+1];
    BOOLEAN bFileExists = FALSE;

    memset( szBuf, 
            0, 
            BUFSIZE+1);

    ceError = GPACheckFileExists( MAC_CRON_PID_FILE,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
        ceError = GPAOpenFile( MAC_CRON_PID_FILE,
                              "r",
                              &fp);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if ( fgets( szBuf, 
                    BUFSIZE, 
                    fp) != NULL ) {
            if (!IsNullOrEmptyString(szBuf)) {
                pid_cron = atoi(szBuf);
            }
        }

        if ( pid_cron ) {
            GPA_LOG_VERBOSE( "Sending SIGHUP to cron of pid: %d", 
                             pid_cron);

            if ( kill( pid_cron, 
                      SIGHUP) < 0) {
                ceError = LwMapErrnoToLwError(errno);
                GPA_LOG_VERBOSE( "Could not HUP cron process: [%d]", ceError);
                ceError = CENTERROR_SUCCESS;
            }
        }

        if (fp)
            GPACloseFile(fp);
    }
#endif

error:

    return ceError;
}
#else
static
CENTERROR
SendHUPToCron(
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szCommand[PATH_MAX + 1];
    FILE * pFile = NULL;
    char  szBuf[BUFSIZE+1];
    pid_t pid_cron = 0;
    PSTR pszToken = NULL; 
    DWORD iArg = 0;

    /*
     * Find Cron process id
     */
    memset( szCommand, 
            0, 
            PATH_MAX + 1);
#if defined(__LWI_DARWIN__)
    sprintf( szCommand, 
             "ps -axo ppid,pid,command | grep cron");
#elif defined(__LWI_FREEBSD__)
    sprintf( szCommand, 
             "UNIX95=1 ps -U root -o ppid,pid,comm | grep cron");
#else
    sprintf( szCommand, 
             "UNIX95=1 ps -u root -o ppid,pid,comm | grep cron");
#endif

    GPA_LOG_VERBOSE( "Find Cron process id command: %s", 
                     szCommand);

    pFile = popen( szCommand, 
                   "r");
    if (pFile == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        GPA_LOG_VERBOSE( "Could not HUP cron process: [%d]", ceError);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    while (!pid_cron) {
        if (NULL == fgets( szBuf, 
                           BUFSIZE, 
                           pFile)) {
            if (feof(pFile))
                break;
            else {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        LwStripWhitespace(szBuf,1,1);
        pszToken = strtok( szBuf, 
                           " \t");
        iArg = 0;

        while (!IsNullOrEmptyString(pszToken)) {
            if (iArg == 0) {
                if (atoi(pszToken) != 1)
                   break;
	         } else if (iArg == 1) {
                pid_cron = atoi(pszToken);
                break;
             }
             pszToken = strtok(NULL, " \t");
             iArg++;
        } 
    }

    if (pid_cron <= 0) {
        ceError = CENTERROR_GP_PROCESS_NOT_FOUND;
        GPA_LOG_VERBOSE( "Could not HUP cron process: [%d]", ceError);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    GPA_LOG_VERBOSE( "Send HUP Command to process id: %d", 
                     pid_cron);

    if (kill( pid_cron, 
              SIGHUP) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        GPA_LOG_VERBOSE( "Could not HUP cron process: [%d]", ceError);
        ceError = CENTERROR_SUCCESS;
    }

error:

    if (pFile != NULL) {
        pclose(pFile);
    }

    return ceError;
}
#endif

static
CENTERROR
RemoveCronPolicy(
    PGROUP_POLICY_OBJECT pGPO
)
{
    CENTERROR ceError = 0;
    char szNewFilePath[PATH_MAX+1];
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    BOOLEAN bFileExists = 0;
    PSTR pszgPCFileSysPath = NULL;
    PSTR pszDisplayName = NULL;

    if ( !pGPO ) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pszgPCFileSysPath = pGPO->pszgPCFileSysPath;
    pszDisplayName = pGPO->pszDisplayName;

    ceError =  GPACrackFileSysPath( pszgPCFileSysPath,
                                    &pszDomainName,
                                    &pszSourcePath,
                                    &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf( szNewFilePath,
             "%s%s_likewise_crontab",
             CENTERIS_GP_DIRECTORY,
             pszPolicyIdentifier);

    ceError = GPACheckFileExists( szNewFilePath, 
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
        GPA_LOG_VERBOSE( "Need to remove crontab policy file: %s [%s]", 
                         pszDisplayName,
                         szNewFilePath);
        LwRemoveFile(szNewFilePath);

        gIsCronD = FALSE;
    } else {
        sprintf( szNewFilePath,
                 "%s%s_likewise_crond",
                 CROND_DIRECTORY,
                 pszPolicyIdentifier);

        GPA_LOG_VERBOSE( "Need to remove crond policy file: %s [%s]", 
                         pszDisplayName, 
                         szNewFilePath);
        LwRemoveFile(szNewFilePath);

        gIsCronD = TRUE;
    }

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

    return(ceError);
}

static
CENTERROR
AddIfNoNewPolicy( 
    PSTR pszPolicyFileName,
    PSTR pszPolicyIdentifier,
    PBOOLEAN pbNewPolicy    
    )
{
    char szFilePath[PATH_MAX+1];
    BOOLEAN bFileExists = 0;
    BOOLEAN bDirExists = FALSE;
    CENTERROR ceError = 0;

    if ( strcmp( pszPolicyFileName, 
                 "crontab.crond") == 0 ) {
        /* The policy is of type crond file */
        strcpy( szFilePath, 
                CROND_DIRECTORY);
        ceError = GPACheckDirectoryExists( szFilePath, 
                                          &bDirExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if ( bDirExists ) {
            sprintf( szFilePath,
                     "%s%s_likewise_crond",
                     CROND_DIRECTORY,
                     pszPolicyIdentifier);

            ceError = GPACheckFileExists( szFilePath, 
                                         &bFileExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (!bFileExists) {
                *pbNewPolicy = TRUE;
                GPA_LOG_ALWAYS( "Former CRON policy has been mysteriously removed from system, will attempt to recover: Dest Path [%s]", 
                                szFilePath);
            } else {
                /* There does exist a crontab file for this policy, add it to our in memory list. */
                ceError = AddPolicyFileToHierarchy(szFilePath);
                BAIL_ON_CENTERIS_ERROR(ceError);

                ceError = RemovePolicyFileFromRemoveList(szFilePath);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }
    }
    else if ( strcmp( pszPolicyFileName, 
                      "crontab" ) == 0 ) {
        /* This policy is for a crontab type file */
        sprintf( szFilePath, 
                 "%s%s_likewise_crontab",
                 CENTERIS_GP_DIRECTORY,
                 pszPolicyIdentifier);

        ceError = GPACheckFileExists( szFilePath, 
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!bFileExists) {
            *pbNewPolicy = TRUE;
            GPA_LOG_ALWAYS( "Former CRON policy has been mysteriously removed from system, will attempt to recover: Dest Path [%s]", 
                            szFilePath);
        } else {
            /* There does exist a crontab file for this policy, add it to our in memory list. */
            ceError = AddPolicyFileToHierarchy(szFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = RemovePolicyFileFromRemoveList(szFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    } else {
        GPA_LOG_ALWAYS( "Unknown Cron policy file type: Policy Identifier [%s] Filename [%s] ", 
                        pszPolicyIdentifier, 
                        pszPolicyFileName);
        GPA_LOG_ALWAYS("Ignoring this policy.");
    }

error:

    return(ceError);
}


static
CENTERROR
AddCronPolicy(
    PGROUP_POLICY_OBJECT pGPO,
    BOOLEAN bSupportCronD,
    PBOOLEAN pbNeedHUP
    )
{
    CENTERROR ceError = 0;
    char szFilePath[PATH_MAX+1];
    char szNewFilePath[PATH_MAX+1];
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyFileName = NULL;
    PSTR pszPolicyIdentifier = NULL;
    BOOLEAN bFileExists = 0;
    PSTR pszDC = NULL;
    BOOLEAN fNewPolicy = FALSE;
    PGPOLWIGPITEM pGPItem = NULL;
    PGPOLWIDATA pLwidata = NULL;
    xmlNodePtr pXmlNodePtr = NULL;
    BOOLEAN bMonitor = FALSE;

    if ( !pGPO ) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fNewPolicy = pGPO->bNewVersion;

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
                              LWICRON_CLIENT_GUID );
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
                            LWICRON_ITEM_GUID,
                            &pGPItem );
    if (!CENTERROR_IS_OK(ceError) &&
        CENTERROR_EQUAL( ceError, 
                         CENTERROR_GP_XML_GPITEM_NOT_FOUND)) {
        ceError = CENTERROR_SUCCESS;
        goto done;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOXmlSelectSingleNode( pGPItem->xmlNode,
                                      "crontab/setting/crontabFile",
                                      &pXmlNodePtr);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /*
     * Read the name of the cron policy file from the lwisettings.xml data for this policy object
     */
    ceError = GPOXmlGetInnerText( pXmlNodePtr, 
                                  &pszPolicyFileName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetPreferredDomainController(pszDomainName, &pszDC);
    if (!CENTERROR_IS_OK(ceError)) {
        if (CENTERROR_EQUAL( ceError, 
                             CENTERROR_GP_NO_SMB_KRB5_SITE_KDC_INFO)   || 
            CENTERROR_EQUAL( ceError, 
                             CENTERROR_GP_NO_SMB_KRB5_SITE_INFO)) {
            GPA_LOG_ALWAYS( "GPAgent unable to obtain preferred server for AD site: FQDN(%s)", 
                            pszDomainName);
            ceError = CENTERROR_SUCCESS;
        } else {
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    ceError = GetMonitorFile( pGPItem->xmlNode,
                              "crontabFile",
                              &bMonitor);
    BAIL_ON_CENTERIS_ERROR(ceError);

    //set the monitor file flag
    gbMonitor = bMonitor;

    if ( !fNewPolicy ) {
        ceError = AddIfNoNewPolicy( pszPolicyFileName,
                                    pszPolicyIdentifier,
                                    &fNewPolicy);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if ( fNewPolicy ) {
    	/* Get the policy file path */
        sprintf( szFilePath, 
                 "%s%s%s%s%s",
                 pszSourcePath,
                 CENTERIS_SYSVOL_PATH,
                 LWICRON_CLIENT_GUID,
                 "\\",
                 pszPolicyFileName);

        ceError = GPOLwioCopyFile(NULL,
                                  pszDomainName, 
                                  pszDC, 
                                  szFilePath, 
                                  CRON_TMP_FILE);
        BAIL_ON_CENTERIS_ERROR(ceError);

    	ceError = GPACheckFileExists(CRON_TMP_FILE, 
                                    &bFileExists);
    	BAIL_ON_CENTERIS_ERROR(ceError);

        if ( strcmp( pszPolicyFileName, 
                     "crontab.crond") == 0 ) {
            if(bSupportCronD) {
                if (bFileExists ) {
            	    ceError = LwChangeOwnerAndPermissions( CRON_TMP_FILE, 
                                                           0, 
                                                           0, 
                                                           S_IRUSR|S_IWUSR);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    sprintf( szNewFilePath,
                             "%s%s_likewise_crond",
                             CROND_DIRECTORY,
                             pszPolicyIdentifier);

                    strcpy(gCronFile,szNewFilePath);

            	    ceError = GPAMoveFileAcrossDevices( CRON_TMP_FILE, 
                                                        szNewFilePath);
            	    if (ceError) {
                        LwRemoveFile(CRON_TMP_FILE);
                        BAIL_ON_CENTERIS_ERROR(ceError);
                    }

                    /* Make sure we don't remove it after we complete the processing */
                    ceError = RemovePolicyFileFromRemoveList(szNewFilePath);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    GPA_LOG_VERBOSE( "Received crond policy: Domain Name [%s] Source Path [%s] Dest Path [%s]", 
                                     pszDomainName, 
                                     szFilePath, 
                                     szNewFilePath);

                }
                *pbNeedHUP = TRUE;
            } else {
                GPA_LOG_ALWAYS( "Received crond policy, however it is not supported on this platform (ignoring): Domain Name [%s]", 
                                pszDomainName);
                goto done;
            }

            gIsCronD = TRUE;

        } else if ( strcmp( pszPolicyFileName, 
                            "crontab" ) == 0 ) {
            if (bFileExists) {
                ceError = LwChangeOwnerAndPermissions( CRON_TMP_FILE, 
                                                       0, 
                                                       0, 
                                                       S_IRUSR|S_IWUSR);
                BAIL_ON_CENTERIS_ERROR(ceError);

                sprintf( szNewFilePath, 
                         "%s%s_likewise_crontab",
                         CENTERIS_GP_DIRECTORY,
                         pszPolicyIdentifier);

                strcpy(gCronFile,CRONTAB_FILE);

                ceError = GPAMoveFileAcrossDevices( CRON_TMP_FILE, 
                                                   szNewFilePath);
                if (ceError) {
                    LwRemoveFile(CRON_TMP_FILE);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

                GPA_LOG_VERBOSE( "Received crontab policy: Domain Name [%s] Source Path [%s] Dest Path [%s]", 
                                 pszDomainName, 
                                 szFilePath, 
                                 szNewFilePath);

                /* Since the policy is for a crontab type, add it to our hierarchy list */
                ceError = AddPolicyFileToHierarchy(szNewFilePath);
                BAIL_ON_CENTERIS_ERROR(ceError);

                ceError = RemovePolicyFileFromRemoveList(szFilePath);
                BAIL_ON_CENTERIS_ERROR(ceError);

                *pbNeedHUP = TRUE;
            } else {
                GPA_LOG_ALWAYS( "Cron policy not found for: Domain Name [%s] Source Path [%s] ", 
                                pszDomainName, 
                                szFilePath);
                GPA_LOG_ALWAYS( "Treating this as a disabled or stale policy.");
            }
        } else {
            GPA_LOG_ALWAYS( "Cron policy file type unsupported: Domain Name [%s] Policy Identifier [%s] Filename [%s] ", 
                            pszDomainName, 
                            pszPolicyIdentifier, 
                            pszPolicyFileName);
            GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");
        }
    }

done:
error:

    if ( pGPItem ) {
        GPODestroyGPItem( pGPItem, 
                          FALSE );
    }

    if ( pLwidata ) {
        GPODestroyLwiData(pLwidata);
    }

    if (pszDomainName) {
        LwFreeString(pszDomainName);
    }

    if (pszSourcePath) {
        LwFreeString(pszSourcePath);
    }

    if (pszPolicyFileName) {
        LwFreeString(pszPolicyFileName);
    }

    if (pszPolicyIdentifier) {
        LwFreeString(pszPolicyIdentifier);
    }

    if (pszDC) {
        LwFreeString(pszDC);
    }

    return(ceError);
}

CENTERROR
ResetCronGroupPolicy(
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* There is nothing to reset for this policy */

    return ceError;
}

CENTERROR
ProcessCronGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN   bNeedHUP = FALSE;
    BOOLEAN   bCrontabSupport = TRUE;
    BOOLEAN   bSupportCronD = FALSE;
    PSTR pszNewMd5Sum = NULL;
 
    GPA_LOG_FUNCTION_ENTER();

    /*
     * Check crontab support
     */
    ceError = CheckCrontabSupport(&bCrontabSupport);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( !bCrontabSupport ) {
        GPA_LOG_VERBOSE("The system does not have crontab installed, hence skipping crontab policy");
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    /*
     * check crond support
     */
    ceError = GPACheckDirectoryExists( "/etc/cron.d", 
                                      &bSupportCronD );
    BAIL_ON_CENTERIS_ERROR( ceError );
 
    /*
     * Reset our in memory cron policy list, since we need to re-determine it
     * and apply the most appropriate policy based on the domain hierarchy.
     */
    ResetPolicyFileHierarchy();
    ResetPolicyRemoveList();

    /*
     * Determine the list of crond and cron type policy files that we may have currently
     * in effect. We will cross off the list those that seem to still be valid.
     */
    GetCurrentListOfCronDPolicies();
    GetCurrentListOfCrontabPolicies();

    /*
     * Process old CronTab policies to remove
     */
    while ( pGPODeletedList ) {
        ceError = RemoveCronPolicy( pGPODeletedList );
        BAIL_ON_CENTERIS_ERROR(ceError);

        bNeedHUP = TRUE;

        pGPODeletedList = pGPODeletedList->pNext;
    }

    /*
     * Process new CronTab policies to add
     */
    while (pGPOModifiedList) {
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

            ceError = RemoveCronPolicy(pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);

            bNeedHUP = TRUE;
        } else if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE( "GPO(%s) is disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);

            ceError = RemoveCronPolicy(pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);

            bNeedHUP = TRUE;
        } else {
            ceError = AddCronPolicy( pGPOModifiedList,
                                     bSupportCronD,
                                     &bNeedHUP);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        pGPOModifiedList = pGPOModifiedList->pNext;
    }

    //Monitor sudoers file
    if(gbMonitor) {

        GPA_LOG_VERBOSE("Monitoring crontab Flag is set, Going to monitor crontab file");

        ceError = MonitorSystemFiles( gCronFile,
                                      g_szMd5Sum,
                                      &pszNewMd5Sum);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strncpy( g_szMd5Sum,
                 pszNewMd5Sum,
                (strlen(pszNewMd5Sum)));
    }

    ceError = ProcessCrontabPolicyFiles(bSupportCronD);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Now remove any stale CronD/Crontab type policy files left around */
    ceError = ProcessPolicyRemoveList();
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(gbMonitor) {

        GPA_LOG_VERBOSE("Going to calculate md5Sum for the new crontab file");

        ceError = MonitorSystemFiles( gCronFile,
                                      g_szMd5Sum,
                                      &pszNewMd5Sum);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strncpy( g_szMd5Sum,
                 pszNewMd5Sum,
                (strlen(pszNewMd5Sum)));
    }

    /* send HUP to get cron to check for cron.d file changes */
    if (bNeedHUP) {
#if defined(__LWI_SOLARIS__) || defined(__LWI_DARWIN__) 
        ceError = RefreshCron();
        BAIL_ON_CENTERIS_ERROR(ceError);
#else
        ceError = SendHUPToCron();
        BAIL_ON_CENTERIS_ERROR(ceError);
#endif
    }

error:
 
    LW_SAFE_FREE_STRING(pszNewMd5Sum);

    GPA_LOG_FUNCTION_LEAVE(ceError);

    return(ceError);
}
