#include "includes.h"

static PPOLICYLIST g_SudoPolicyHierarchyList = NULL;
static PPOLICYLIST g_SudoPolicyRemoveList = NULL;

//Global variable to preserve the previously calculated MD5Sum
static char g_szMd5Sum[256] = "";
static BOOLEAN gbMonitor; 

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

static
void
FixCfgString(
    char *string
    )
{
    size_t len = 0;

    LwStripWhitespace(string,1,1);

    len = strlen(string);

    if(string[len - 1] == ';')
        len--;
    if(string[len - 1] == '"')
        len--;

    string[len] = 0;

    if(string[0] == '"') {
        //Since the string is shrinking by one character, copying len
        //characters will move the null too.
        memmove(string, string + 1, len);
    }
}

#if 0
CENTERROR
GetMonitorSudoers(
    PDWORD pdwMonitorSudoers
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD dwMonitorSudoers = FALSE;
    GPA_CONFIG_TABLE ConfigValue = { "MonitorSudoers", 
                                     FALSE, 
                                     GPATypeBoolean, 
                                     0, 
                                     -1, 
                                     NULL, 
                                     &dwMonitorSudoers};

    ceError = GPAProcessConfig(
                "Services\\gpagent\\Parameters",
                "Policy\\Services\\gpagent\\Parameters",
                &ConfigValue,
                1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *pdwMonitorSudoers = dwMonitorSudoers;

error:
    return(ceError);
}
#endif

static
CENTERROR
GetPathFromRegistry(
    PSTR pszPathString,
    PSTR *ppszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY pRootKey = NULL;
    CHAR szValue[MAX_VALUE_LENGTH];
    DWORD dwType = 0;
    DWORD dwSize = 0;
    PSTR pszValue = NULL;

    ceError = RegOpenServer(&hReg);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = RegOpenKeyExA(
                hReg,
                NULL,
                HKEY_THIS_MACHINE,
                0,
                0,
                &pRootKey);
    if (ceError)
    {
        GPA_LOG_ERROR( "Failed to open registry root key %s",HKEY_THIS_MACHINE);
        goto error;
    }

    dwSize = sizeof(szValue); 
    memset(szValue,0,dwSize);

    ceError = RegGetValueA(
                hReg,
                pRootKey,
                "Services\\gpagent\\GPExtensions\\{20D139DE-D892-419f-96E5-0C3A997CB9C4}",
                pszPathString,
                RRF_RT_REG_SZ,
                &dwType,
                szValue,
                &dwSize);
    if(ceError)
    {
        ceError = 0;
        goto error;
    }
    else
    {
        ceError = LwAllocateString(szValue, &pszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);

        *ppszValue = pszValue;
        pszValue = NULL;
    }

cleanup:

    RegCloseKey(hReg, pRootKey);
    pRootKey = NULL;

    RegCloseServer(hReg);
    hReg = NULL;

    return(ceError);

error:
   
    goto cleanup;

}


static
CENTERROR
GetSudoersPath(
    PSTR* ppszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR searchPath = NULL;
    PSTR visudoPath = NULL;
    PSTR visudoOutput = NULL;
    PSTR sudoersPath = NULL;

    /*
     * First try to run visudo and ask it where the sudoers file is.
     * This protects us from sudoers files that happen to be on the system but are not used by sudo.
     */
    ceError = GetPathFromRegistry("VisudoSearchPath",&searchPath);
    GCE(ceError);

    if (!searchPath) {
        // The "Community Software for Solaris" version of sudo uses /opt/csw
        // Some sites install sudo to /opt/sudo.
        ceError = LwAllocateString( "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/opt/sudo/sbin:/opt/sudo/bin:/opt/csw/sbin:/opt/csw/bin",
                                    &searchPath);
    }

    GCE(ceError);
    FixCfgString(searchPath);

    GPA_LOG_VERBOSE( "Searching for visudo in %s",
                     searchPath);
    ceError = GPAFindFileInPath( "visudo",
                               searchPath,
                               &visudoPath);
    if( ceError == CENTERROR_FILE_NOT_FOUND )
    {
        ceError = CENTERROR_SUCCESS;
    }
    GCE(ceError);

    if( visudoPath != NULL )
    {
        PCSTR args[] = { visudoPath, "-c", (char*) NULL };
        PSTR strtokSavePtr = NULL;
        PSTR token = NULL;

        GPA_LOG_VERBOSE( "Found visudo at %s",
                         visudoPath);
        ceError = GPACaptureOutputWithStderrEx( args[0],
                                               args,
                                               TRUE,
                                               &visudoOutput,
                                               NULL);
        if( ceError == CENTERROR_COMMAND_FAILED )
            ceError = CENTERROR_SUCCESS;
        GCE(ceError);

        /*
         * Colon is included in the deliminator list because if the sudoers
         * file does not exist on HP-UX, visudo says this:
         * # /usr/local/sbin/visudo -c
         * visudo: unable to open /usr/local/etc/sudoers: No such file or directory
         */
        token = strtok_r( visudoOutput,
                          " :\t\r\n",
                          &strtokSavePtr);
        while( token != NULL ) {
            if( GPAStrStartsWith( token,
                                 "/")   &&
                GPAStrEndsWith( token,
                               "sudoers") ) {
                //Found the sudoers path
                GCE( ceError = LwAllocateString(token, &sudoersPath) );
            }
            token = strtok_r( NULL,
                              " :\t\r\n",
                              &strtokSavePtr);
        }
    }

    if(sudoersPath == NULL)
    {
        GPA_LOG_VERBOSE("Attempting to find sudoers file instead of using visudo");

        LW_SAFE_FREE_STRING(searchPath);

        ceError = GetPathFromRegistry("SudoersSearchPath",&searchPath);

        if ( !searchPath){
            // The "Community Software for Solaris" version of sudo uses /opt/csw
            // Some sites install sudo to /opt/sudo.
            ceError = LwAllocateString( "/usr/local/etc:/usr/etc:/etc:/opt/sudo/etc:/opt/csw/etc:/opt/sfw/etc",
                                        &searchPath);
        }

        GCE(ceError);
        FixCfgString(searchPath);
        ceError = GPAFindFileInPath( "sudoers",
                                   searchPath,
                                   &sudoersPath);
        if( ceError == CENTERROR_FILE_NOT_FOUND )
            ceError = CENTERROR_SUCCESS;
        GCE(ceError);
    }

    if(sudoersPath == NULL)
    {
        GPA_LOG_VERBOSE("Unable to find existing sudoers file. Installing in default path.");
        //Use the default path
        ceError = GetPathFromRegistry("ConfigFilePath",&sudoersPath);
        if (!sudoersPath) {
            ceError = LwAllocateString( "/etc/sudoers",
                                        &sudoersPath);
        }
        GCE(ceError);
        FixCfgString(sudoersPath);
    }

    GPA_LOG_VERBOSE( "Final sudoers path is '%s'",
                     sudoersPath);
    *ppszPath = sudoersPath;
    sudoersPath = NULL;

cleanup:

    LW_SAFE_FREE_STRING(searchPath);
    LW_SAFE_FREE_STRING(visudoPath);
    LW_SAFE_FREE_STRING(visudoOutput);
    LW_SAFE_FREE_STRING(sudoersPath);

    return ceError;
}

static
CENTERROR
AddPolicyFileToHierarchy(
    PSTR szFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pTemp = g_SudoPolicyHierarchyList;
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
        g_SudoPolicyHierarchyList = pNew;
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
    PPOLICYLIST pTemp = g_SudoPolicyRemoveList;
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
        g_SudoPolicyRemoveList = pNew;
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
    PPOLICYLIST pTemp = g_SudoPolicyRemoveList;
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
                g_SudoPolicyRemoveList = pTemp;
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
void
ResetPolicyRemoveList(
    )
{
    while (g_SudoPolicyRemoveList) {
        PPOLICYLIST pTemp = g_SudoPolicyRemoveList;
        g_SudoPolicyRemoveList = g_SudoPolicyRemoveList->next;
        if (pTemp->FileName) {
            LwFreeString(pTemp->FileName);
        }
        LwFreeMemory( pTemp );
    }
}

static
CENTERROR
GetCurrentListOfSudoPolicies()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];
    FILE* pFile = NULL;
    BOOLEAN fExists = FALSE;

    ceError = GPACheckDirectoryExists(CENTERIS_GP_DIRECTORY,
                                     &fExists );
    BAIL_ON_CENTERIS_ERROR( ceError );

#if defined(__LWI_DARWIN__)
    sprintf( szBuf, "ls %s*_likewise_sudoers 2> /dev/null", CENTERIS_GP_DIRECTORY);
#else
    sprintf( szBuf, "UNIX95=1 ls %s*_likewise_sudoers 2> /dev/null", CENTERIS_GP_DIRECTORY);
#endif

    if ( fExists ) {

        errno = 0;
        pFile = popen( szBuf,
                       "r");
        if (pFile == NULL) {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);

            // popen returns NULL without changing errno if popen runs out of
            // memory.
            ceError = CENTERROR_OUT_OF_MEMORY;
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
ProcessSudoPolicyFiles(
    PSTR pszConfigFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pPolicyList = g_SudoPolicyHierarchyList;
    BOOLEAN bFileExists = 0;
    CHAR szSudoOldFile[PATH_MAX+1];
    mode_t mode = 0440;
    uid_t  uid = 0;
    /* Setting gid = 0 for solaris too. Since sudo expects gid to be 0 - Bug #7237 */
    gid_t  gid = 0;

    memset(szSudoOldFile,0,sizeof(szSudoOldFile));

    while (pPolicyList && pPolicyList->next) {
        // Walk to last sudo file in our list
        pPolicyList = pPolicyList->next;
    }

    // Check sudoers file exists
    //Backup the old file where actual sudoers is present
    
    //Get the directory where sudoers is present. Strip off the last 7 bytes
    sprintf( szSudoOldFile,
             "%s.lwidentity.orig",
             pszConfigFilePath);

    ceError = GPACheckFileExists( szSudoOldFile,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* If no pPolicyList item, then we should restore system to original setup. */
    if (!pPolicyList) {

        if (bFileExists) {
            GPA_LOG_VERBOSE("Reverting Sudo to original system file.");

            /* Delete the sudoers file in /etc */
            LwRemoveFile(pszConfigFilePath);

            /* Move the saved original file to it's proper place. */
            ceError = GPAMoveFileAcrossDevices( szSudoOldFile,
                                               pszConfigFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            //Reset md5sum
            memset(g_szMd5Sum,0,sizeof(g_szMd5Sum));
        }
    } else {

        if (!bFileExists) {

            /* Back up the original sudoers file so that we are able to revert to it. */
            ceError = GPACheckFileExists( pszConfigFilePath,
                                         &bFileExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (bFileExists) {
                GPA_LOG_VERBOSE("Backing up original Sudo system file.");

                //Get UID GID and mode before backing up
                ceError = LwGetOwnerAndPermissions( pszConfigFilePath,
                                                    &uid,
                                                    &gid,
                                                    &mode);
                BAIL_ON_CENTERIS_ERROR(ceError);

                ceError = GPAMoveFileAcrossDevices( pszConfigFilePath,
                                                   szSudoOldFile);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        } else {

            //If the file exists, get the permission from sudoers.lwidentity.orig
            ceError = LwGetOwnerAndPermissions( szSudoOldFile,
                                                &uid,
                                                &gid,
                                                &mode);
            BAIL_ON_CENTERIS_ERROR(ceError);

        }

        /* Now move a copy of the new sudoer file into it's proper place. */
        GPA_LOG_VERBOSE("Applying group policy version of Sudo to system file.");

        ceError = GPACopyFileWithOriginalPerms( pPolicyList->FileName,
                                               pszConfigFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwChangeOwnerAndPermissions( pszConfigFilePath,
                                               uid,
                                               gid,
                                               mode);
        BAIL_ON_CENTERIS_ERROR(ceError);

        //Reset md5sum
        memset(g_szMd5Sum,0,sizeof(g_szMd5Sum));

        GPA_LOG_VERBOSE( "Source Path [%s] Dest Path [%s]",
                         pPolicyList->FileName,
                         pszConfigFilePath);
   }

error:

    return ceError;
}

static
CENTERROR
ProcessPolicyRemoveList()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pPolicyList = g_SudoPolicyRemoveList;
    BOOLEAN bFileExists = 0;

    GPA_LOG_INFO("Processing Sudoers remove list...");

    while (pPolicyList) {
        GPA_LOG_INFO( "Sudoers Remove List Entry: %s",
                      pPolicyList->FileName);

        ceError = GPACheckFileExists( pPolicyList->FileName,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            GPA_LOG_VERBOSE("Removing stale Sudoers file.");
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
ResetPolicyFileHierarchy()
{
    while (g_SudoPolicyHierarchyList) {
        PPOLICYLIST pTemp = g_SudoPolicyHierarchyList;
        g_SudoPolicyHierarchyList = g_SudoPolicyHierarchyList->next;
        if (pTemp->FileName) {
            LwFreeString(pTemp->FileName);
        }
        LwFreeMemory( pTemp );
    }
}

static
CENTERROR
AddSudoPolicy(
    PGROUP_POLICY_OBJECT pGPO
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szFilePath[PATH_MAX+1];
    char szNewFilePath[PATH_MAX+1];
    BOOLEAN bFileExists = 0;
    BOOLEAN fNewPolicy = FALSE;
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    PSTR pszPolicyFileName = NULL;
    PSTR pszDC = NULL;
    PSTR pszgPCFileSysPath = NULL;
    PGPOLWIGPITEM pGPItem = NULL;
    PGPOLWIDATA pLwidata = NULL;
    xmlNodePtr pXmlNodePtr = NULL;
    BOOLEAN bMonitor = FALSE;

    if ( !pGPO ) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pszgPCFileSysPath = pGPO->pszgPCFileSysPath;
    fNewPolicy = pGPO->bNewVersion;

    ceError =  GPACrackFileSysPath( pszgPCFileSysPath,
                                    &pszDomainName,
                                    &pszSourcePath,
                                    &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf( szNewFilePath,
             "%s%s_likewise_sudoers",
             CENTERIS_GP_DIRECTORY,
             pszPolicyIdentifier);


    if ( !fNewPolicy ) {
        ceError = GPACheckFileExists( szNewFilePath,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!bFileExists) {
            fNewPolicy = TRUE;
            GPA_LOG_ALWAYS( "Former sudo policy has been mysteriously removed from system, will attempt to recover: Domain Name [%s] Dest Path [%s]",
                            pszDomainName,
                            szNewFilePath);
        } else {
            ceError = AddPolicyFileToHierarchy(szNewFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = RemovePolicyFileFromRemoveList(szNewFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    if (fNewPolicy) {

        // Look in new policy file location
        ceError = GPOInitLwiData( NULL,
                                  MACHINE_GROUP_POLICY,
                                  (PGPOLWIDATA*)&pLwidata,
                                  pszgPCFileSysPath,
                                  NULL,
                                  LWISUDO_CLIENT_GUID );
        if ( ceError ) {
            // Look in old policy file location
            ceError = GPOInitLwiData( NULL,
                                      MACHINE_GROUP_POLICY,
                                      (PGPOLWIDATA*)&pLwidata,
                                      pszgPCFileSysPath,
                                      NULL,
                                      NULL );
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = GPOGetGPItem( MACHINE_GROUP_POLICY,
                                pLwidata,
                                LWISUDO_ITEM_GUID,
                                &pGPItem );
        if ( ceError != CENTERROR_SUCCESS ) {
            GPA_LOG_ALWAYS( "Sudo policy gpitem not found for: Domain Name [%s] Policy [%s]",
                            pszDomainName,
                            pszPolicyIdentifier);
            GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");
            ceError = CENTERROR_SUCCESS;
            goto error;
        }

        ceError = GPOXmlSelectSingleNode( pGPItem->xmlNode,
                                          "sudo/setting",
                                          &pXmlNodePtr);
        if ( ceError != CENTERROR_SUCCESS ) {
            GPA_LOG_ALWAYS( "Sudo policy setting not found for: Domain Name [%s] Policy [%s]",
                            pszDomainName,
                            pszPolicyIdentifier);
            GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");
            ceError = CENTERROR_SUCCESS;
            goto error;
        }

        ceError = GPOXmlGetInnerText( pXmlNodePtr,
                                      &pszPolicyFileName);
        if ( ceError != CENTERROR_SUCCESS ) {
            GPA_LOG_ALWAYS( "Sudo policy setting value could not be read for: Domain Name [%s] Policy [%s]",
                            pszDomainName,
                            pszPolicyIdentifier);
            GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");
            ceError = CENTERROR_SUCCESS;
            goto error;
        }

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

        ceError = GetMonitorFile( pGPItem->xmlNode,
                                  "setting",
                                  &bMonitor);
        BAIL_ON_CENTERIS_ERROR(ceError);

        //set the monitor file flag
        gbMonitor = bMonitor;

        sprintf( szFilePath,
                 "%s%s%s\\%s",
                 pszSourcePath,
                 CENTERIS_SYSVOL_PATH,
                 LWISUDO_CLIENT_GUID,
                 pszPolicyFileName);

        ceError = GPOLwioCopyFile( NULL,
                                  pszDomainName,
                                  pszDC,
                                  szFilePath,
                                  SUDO_TMP_FILE);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPACheckFileExists( SUDO_TMP_FILE,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            ceError = LwChangeOwnerAndPermissions( SUDO_TMP_FILE,
                                                   0,
                                                   0,
                                                   S_IRUSR|S_IRGRP);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = GPAMoveFileAcrossDevices( SUDO_TMP_FILE,
                                               szNewFilePath);
            if (ceError) {
                LwRemoveFile(SUDO_TMP_FILE);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            GPA_LOG_VERBOSE( "Received sudo policy: Domain Name [%s] Source Path [%s] Dest Path [%s]",
                             pszDomainName,
                             szFilePath,
                             szNewFilePath);

            ceError = AddPolicyFileToHierarchy(szNewFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = RemovePolicyFileFromRemoveList(szNewFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            GPA_LOG_ALWAYS( "Sudo policy not found for: Domain Name [%s] Source Path [%s] ",
                            pszDomainName,
                            szFilePath);
            GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");
        }
    }

error:

    if ( pGPItem ) {
        GPODestroyGPItem( pGPItem, FALSE );
    }

    if ( pLwidata ) {
        GPODestroyLwiData(pLwidata);
    }

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);
    LW_SAFE_FREE_STRING(pszPolicyFileName);
    LW_SAFE_FREE_STRING(pszDC);

    return(ceError);
}

CENTERROR
ResetSudoGroupPolicy(
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* There is nothing to reset for this policy */ 

    return ceError;
}


CENTERROR
ProcessSudoGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = 0;
    PSTR pszSudoersPath = NULL;
    PSTR pszNewMd5Sum = NULL;

    GPA_LOG_FUNCTION_ENTER();

    ceError = GetSudoersPath(&pszSudoersPath);
    if (!CENTERROR_IS_OK(ceError) ||
        IsNullOrEmptyString(pszSudoersPath)) {

       GPA_LOG_ERROR("Failed to get configured sudoers path");

       ceError = LwAllocateString( SUDOERS_FILE,
                                   &pszSudoersPath);
       BAIL_ON_CENTERIS_ERROR(ceError);

    }

    /*
     * Reset our in memory sudo policy list, since we need to re-determine it
     * and apply the most appropriate policy based on the domain hierarchy.
     */
    ResetPolicyFileHierarchy();
    ResetPolicyRemoveList();

    /*
     * Determine the list of Sudo type policy files that we may have currently
     * in effect. We will cross off the list those that seem to still be valid.
     */
    GetCurrentListOfSudoPolicies();

    /*
     * Process old sudo policies to remove
     */
    while ( pGPODeletedList ) {
        ceError = GPARemovePolicy( pGPODeletedList,
                                   "sudoers");
        BAIL_ON_CENTERIS_ERROR(ceError);
        pGPODeletedList = pGPODeletedList->pNext;
    }

    /*
     * Process new sudo policies to add
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
             ceError = GPARemovePolicy( pGPOModifiedList,
                                        "sudoers");
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE( "GPO(%s) is disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);
            ceError = GPARemovePolicy( pGPOModifiedList,
                                       "sudoers");
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {

            ceError = AddSudoPolicy(pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        pGPOModifiedList = pGPOModifiedList->pNext;
    }

    //Read from registry whether to monitor sudoers is set or not
    //ceError = GetMonitorSudoers(&dwMonitorSudoers);
    //BAIL_ON_CENTERIS_ERROR(ceError);

    //Monitor sudoers file
    if(gbMonitor) {

        GPA_LOG_VERBOSE("Monitoring sudoers Flag is set, Going to monitor sudoers file");

        ceError = MonitorSystemFiles( pszSudoersPath,
                                      g_szMd5Sum,
                                      &pszNewMd5Sum);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strncpy( g_szMd5Sum,
                 pszNewMd5Sum,
                (strlen(pszNewMd5Sum)));
    }

    ceError = ProcessSudoPolicyFiles(pszSudoersPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Now remove any stale Sudo type policy files left around */
    ceError = ProcessPolicyRemoveList();
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(gbMonitor) {

        GPA_LOG_VERBOSE("Going to calculate md5Sum for the new sudoers file");

        ceError = MonitorSystemFiles( pszSudoersPath,
                                      g_szMd5Sum,
                                      &pszNewMd5Sum);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strncpy( g_szMd5Sum,
                 pszNewMd5Sum,
                (strlen(pszNewMd5Sum)));

    }

error:

    LW_SAFE_FREE_STRING(pszSudoersPath);
    LW_SAFE_FREE_STRING(pszNewMd5Sum);

    GPA_LOG_FUNCTION_LEAVE(ceError);

    return(ceError);
}
