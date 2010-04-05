#include "includes.h"

static PPOLICYLIST g_SELinuxPolicyHierarchyList = NULL;
static PPOLICYLIST g_SELinuxPolicyRemoveList = NULL;


static
CENTERROR
AddPolicyFileToHierarchy(
    PSTR szFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pTemp = g_SELinuxPolicyHierarchyList;
    PPOLICYLIST pPrev = NULL;
    PPOLICYLIST pNew = NULL;

    ceError = LwAllocateMemory(sizeof(POLICYLIST), (PVOID*)&pNew);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString(szFileName, &pNew->FileName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while (pTemp) {
        pPrev = pTemp;
        pTemp = pTemp->next;
    }

    if (pPrev) {
        pPrev->next = pNew;
    } else {
        g_SELinuxPolicyHierarchyList = pNew;
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
    PPOLICYLIST pTemp = g_SELinuxPolicyRemoveList;
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
        g_SELinuxPolicyRemoveList = pNew;
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
    PPOLICYLIST pTemp = g_SELinuxPolicyRemoveList;
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
                g_SELinuxPolicyRemoveList = pTemp;
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
    while (g_SELinuxPolicyRemoveList) {
        PPOLICYLIST pTemp = g_SELinuxPolicyRemoveList;
        g_SELinuxPolicyRemoveList = g_SELinuxPolicyRemoveList->next;
        if (pTemp->FileName) {
            LwFreeString(pTemp->FileName);
        }
        LwFreeMemory( pTemp );
    }
}

static
CENTERROR
GetCurrentListOfSELinuxPolicies()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];
    FILE* pFile = NULL;
    BOOLEAN fExists = FALSE;

    ceError = GPACheckDirectoryExists(CENTERIS_GP_DIRECTORY,
                                     &fExists );
    BAIL_ON_CENTERIS_ERROR( ceError );

#if defined(__MACH__) && defined(__APPLE__)
    sprintf( szBuf, "ls %s*_likewise_selinux 2> /dev/null", CENTERIS_GP_DIRECTORY);
#else
    sprintf( szBuf, "UNIX95=1 ls %s*_likewise_selinux 2> /dev/null", CENTERIS_GP_DIRECTORY);
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
ProcessSELINUXPolicyFiles(
    PSTR pszConfigFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pPolicyList = g_SELinuxPolicyHierarchyList;
    BOOLEAN bFileExists = 0;
    uid_t uid = 0;
    gid_t gid = 0;
    mode_t dwPerms = 0440;

    while (pPolicyList && pPolicyList->next) {
        // Walk to last selinux file in our list
        pPolicyList = pPolicyList->next;
    }

    ceError = GPACheckFileExists(SELINUX_OLD_FILE, &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* If no pPolicyList item, then we should restore system to original setup. */
    if (!pPolicyList) {

        if (bFileExists) {
            GPA_LOG_VERBOSE("Reverting SELINUX to original system file.");
            /* Delete the selinux file in /etc */
            LwRemoveFile(pszConfigFilePath);

            /* Move the saved original file to it's proper place. */
            GPAMoveFileAcrossDevices(SELINUX_OLD_FILE, pszConfigFilePath);
        }
    } else {

        if (!bFileExists) {
            /* Back up the original selinux file so that we are able to revert to it. */
            ceError = GPACheckFileExists(pszConfigFilePath, &bFileExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (bFileExists) {
                GPA_LOG_VERBOSE("Backing up original SELINUX system file.");
                GPAMoveFileAcrossDevices(pszConfigFilePath, SELINUX_OLD_FILE);
            }
        } else {
            /*
             * Get the perms that are set on the original SELinux file, so
             * that we can apply the new file with these same perms.
             */
            ceError = LwGetOwnerAndPermissions( SELINUX_OLD_FILE, &uid, &gid, &dwPerms );
            BAIL_ON_CENTERIS_ERROR(ceError);
        } 

        /* Now move a copy of the new selinux file into it's proper place. */
        GPA_LOG_VERBOSE("Applying group policy version of SELINUX to system file.");
        ceError = GPACopyFileWithPerms(pPolicyList->FileName, pszConfigFilePath, dwPerms);
        BAIL_ON_CENTERIS_ERROR(ceError);

        GPA_LOG_VERBOSE("Source Path [%s] Dest Path [%s]", pPolicyList->FileName, pszConfigFilePath);
   }

error:

    return ceError;
}

static
CENTERROR
ProcessPolicyRemoveList()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pPolicyList = g_SELinuxPolicyRemoveList;
    BOOLEAN bFileExists = 0;

    GPA_LOG_INFO("Processing SELinux remove list...");

    while (pPolicyList) {
        GPA_LOG_INFO( "SELinux Remove List Entry: %s",
                      pPolicyList->FileName);

        ceError = GPACheckFileExists( pPolicyList->FileName,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            GPA_LOG_VERBOSE("Removing stale SELinux file.");
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
    while (g_SELinuxPolicyHierarchyList) {
        PPOLICYLIST pTemp = g_SELinuxPolicyHierarchyList;
        g_SELinuxPolicyHierarchyList = g_SELinuxPolicyHierarchyList->next;
        if (pTemp->FileName) {
            LwFreeString(pTemp->FileName);
        }
        LwFreeMemory( pTemp );
    }
}

CENTERROR
ParseSELINUXGPItem( 
    xmlNodePtr root_node,
    PSTR pszMode,
    PSTR pszType
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cur_node = NULL;
    xmlChar *pszNodeText = NULL;
    PSTR pszTmpMode = pszMode;
    PSTR pszTmpType = pszType;

    for (cur_node = root_node; cur_node; cur_node = cur_node->next) {
        pszNodeText = NULL;

        if (cur_node->name) {

            if (strcmp( (char*)cur_node->name,
                        SELINUX_MODE_TAG_NAME) == 0 ) {
                
                pszNodeText = xmlNodeGetContent( cur_node );
                //Copy only if pszNodeText is not empty
                if(!IsNullOrEmptyString(pszNodeText)) {
                    strcpy(pszMode,(const char*)pszNodeText);
                }
                 
            } else if (strcmp( (char*)cur_node->name,
                        SELINUX_TYPE_TAG_NAME) == 0 ) {
                
                pszNodeText = xmlNodeGetContent( cur_node );
                //Copy only if pszNodeText is not empty
                if(!IsNullOrEmptyString(pszNodeText)) {
                    strcpy(pszType,(const char*)pszNodeText);
                }
            }

            if ( pszNodeText ) {
                xmlFree( pszNodeText );
            }
        }
        ParseSELINUXGPItem( cur_node->children,pszTmpMode,pszTmpType);
    }

    return ceError;
}


static
CENTERROR
WriteToFile(
    PSTR pszNewFilePath,
    PSTR pszMode,
    PSTR pszType
    )
{
    
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE *fp = NULL;

    fp = fopen( pszNewFilePath,
                "w");
    if( !fp ) {
        GPA_LOG_VERBOSE("Failed to create file %s", pszNewFilePath);
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fprintf( fp, 
             "%s%s\n%s=%s\n", 
             LWISELINUX_FILE_HEADER,
             SELINUX_SYSTEM_HEADER,
             SELINUX_MODE_KEYWORD,
             pszMode );

    fprintf( fp, 
             "%s\n%s=%s\n",
             SELINUXTYPE_SYSTEM_HEADER,
             SELINUX_TYPE_KEYWORD,
             pszType );
error:
    if ( fp ) {
        fclose( fp );
    }		
    return ceError;
}

static
CENTERROR
AddSELINUXPolicy(
    PGROUP_POLICY_OBJECT pGPO
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szNewFilePath[PATH_MAX+1];
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    BOOLEAN bFileExists = 0;
    PSTR pszShortDomainName = NULL;
    PSTR pszDCAddress = NULL;
    PSTR pszgPCFileSysPath = NULL;
    BOOLEAN fNewPolicy = FALSE;

    PGPOLWIGPITEM pGPItem = NULL;
    PGPOLWIDATA pLwidata = NULL;
    PSTR pszSELINUXText = NULL;

    char szType[PATH_MAX+1];
    char szMode[PATH_MAX+1];
    char szDir[PATH_MAX+1];
    BOOLEAN bDirExists = FALSE;
                

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
             "%s%s_likewise_selinux",
             CENTERIS_GP_DIRECTORY,
             pszPolicyIdentifier);

    /*
     * If An Existing policy
     */
    if ( !fNewPolicy ) {
        ceError = GPACheckFileExists( szNewFilePath,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!bFileExists) {
            fNewPolicy = TRUE;
            GPA_LOG_ALWAYS("Former selinux policy has been mysteriously removed from system, will attempt to recover: Domain Name [%s] Dest Path [%s]", pszDomainName, szNewFilePath);
        } else {
            ceError = AddPolicyFileToHierarchy(szNewFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = RemovePolicyFileFromRemoveList(szNewFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    } 

    if ( fNewPolicy ) {

        // Look in new policy file location
        ceError = GPOInitLwiData( NULL,
                                  MACHINE_GROUP_POLICY,
                                  (PGPOLWIDATA*)&pLwidata,
                                  pszgPCFileSysPath,
                                  NULL,
                                  LWISELINUX_CLIENT_GUID );
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

        /*
         * Get the GPItem
         */
        ceError = GPOGetGPItem( MACHINE_GROUP_POLICY,
                                pLwidata, 
                                LWISELINUX_ITEM_GUID,
                                &pGPItem);
        if ( ceError != CENTERROR_SUCCESS ) {
            GPA_LOG_ALWAYS("SELinux policy gpitem not found for: Domain Name [%s] Policy [%s]", pszDomainName, pszPolicyIdentifier);
            GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");
            ceError = CENTERROR_SUCCESS;
            goto error;
        }

        //Get szMode and szType
        memset( szMode, 
                0,
                sizeof(szMode));
        memset( szType, 
                0, 
                sizeof(szType));

        ceError = ParseSELINUXGPItem( pGPItem->xmlNode,
                                      szMode,
                                      szType);
        BAIL_ON_CENTERIS_ERROR(ceError);

        //If Mode and type are NULL then log it and return success
        if(IsNullOrEmptyString(szMode) || IsNullOrEmptyString(szType)) {
            GPA_LOG_ALWAYS("SELinux Mode/Type is not set");
            GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");
            ceError = CENTERROR_SUCCESS;
            goto error;
        }

        GPA_LOG_ALWAYS(" SELinux Mode = %s  SELinux Type = %s",szMode,szType);

        //Fix for Bug#5467,Check if targetted or strict directories present before applying policy 
        //Check if refpolicy-targeted directory is present
        if ( !strcmp( szType, "targeted")) {
        	sprintf ( szDir,
          	          "%s%s",
                      SELINUX_DIRECTORY,
                      "refpolicy-targeted");

        	ceError = GPACheckDirectoryExists( szDir,
                                              &bDirExists);
        	BAIL_ON_CENTERIS_ERROR(ceError);

        	if (bDirExists) {
                //Then this is a new type,set szType to 
                 strcpy( szType,
                         "refpolicy-targeted");
            }
            //Reset the buffer 
            memset( szDir, 
                    0, 
                    sizeof(szDir));
        }

        sprintf ( szDir,
                  "%s%s",
                  SELINUX_DIRECTORY,
                  szType);

        GPA_LOG_VERBOSE("Checking for the Directory... %s ", szDir);

        ceError = GPACheckDirectoryExists( szDir,
                                          &bDirExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bDirExists == FALSE) {
            GPA_LOG_ALWAYS(" %s is not present, Hence SELinux group policy is not applicable for this system...",szDir);
            GPA_LOG_ALWAYS("SELinux policy is not applied as SELinux configuration is not complete");
            ceError = CENTERROR_SUCCESS;
            goto error;
        }
        //write to a file
        ceError = WriteToFile( szNewFilePath,
                               szMode,
                               szType);
        BAIL_ON_CENTERIS_ERROR(ceError);

        GPA_LOG_VERBOSE("Received selinux policy: Domain Name [%s]  Dest Path [%s]", pszDomainName, szNewFilePath);
        ceError = AddPolicyFileToHierarchy(szNewFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = RemovePolicyFileFromRemoveList(szNewFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
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

    if (pszDCAddress) {
        LwFreeString(pszDCAddress);
    }

    if (pszShortDomainName) {
        LwFreeString(pszShortDomainName);
    }

    if (pszSELINUXText) {
        LwFreeString(pszSELINUXText);
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

CENTERROR
ResetSELinuxGroupPolicy(
		PGPUSER pUser
		)
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* There is nothing to reset for this policy */

    return ceError;
}

static
BOOLEAN
IsSELinuxSupported()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bDirExists = FALSE;

    /* Fix for bug 4317, if any system has a SELinux configuration directory
     * we will consider this platform suitable for applying this policy */
    ceError = GPACheckDirectoryExists(SELINUX_DIRECTORY, &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bDirExists == FALSE) {
        GPA_LOG_ALWAYS("SELinux group policy is not applicable for this system...");
        ceError = CENTERROR_INVALID_OPERATION;
        goto error;
    }

error:
    return ceError == CENTERROR_SUCCESS;
}

CENTERROR
ProcessSELinuxGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszSELINUXPath = NULL;

    GPA_LOG_FUNCTION_ENTER();

    /* This used to be IsRedhatLinux(), but SELinux can be found on Ubuntu, RHEL, and some
     * newer versions of Redhat. We are now going to only apply this setting if the system
     * has an existing SELinux settings directory /etc/selinux/ */
    if(!IsSELinuxSupported()) {
        goto error;
    }
    
    ceError = LwAllocateString( SELINUX_FILE, 
                                &pszSELINUXPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /*
     * Reset our in memory selinux policy list, since we need to re-determine it
     * and apply the most appropriate policy based on the domain hierarchy.
     */
    ResetPolicyFileHierarchy();
    ResetPolicyRemoveList();

    /*
     * Determine the list of SELinux type policy files that we may have currently
     * in effect. We will cross off the list those that seem to still be valid.
     */
    GetCurrentListOfSELinuxPolicies();

    /*
     * Process old selinux policies to remove
     */
    while ( pGPODeletedList ) {

        ceError =  GPARemovePolicy( pGPODeletedList,
                                    "selinux");
        BAIL_ON_CENTERIS_ERROR(ceError);

        pGPODeletedList = pGPODeletedList->pNext;
    }

    /*
     * Process new selinux policies to add
     */
    while ( pGPOModifiedList ) {
        BOOLEAN applicable;
        
        ceError = GPOXmlVerifyPlatformApplicable( NULL,
                                                  MACHINE_GROUP_POLICY,
                                                  pGPOModifiedList,
                                                  NULL,
                                                  &applicable);

        BAIL_ON_CENTERIS_ERROR(ceError);
        
        if (!applicable) {
            GPA_LOG_VERBOSE("GPO(%s) disabled by platform targeting",
                            pGPOModifiedList->pszDisplayName);
            ceError = GPARemovePolicy( pGPOModifiedList,
                                       "selinux");
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE("GPO(%s) is disabled by flags: 0x%.08x",
                            pGPOModifiedList->pszDisplayName,
                            pGPOModifiedList->dwFlags);

            ceError = GPARemovePolicy( pGPOModifiedList,
                                       "selinux");
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            ceError = AddSELINUXPolicy(pGPOModifiedList);
            if ( ceError != CENTERROR_SUCCESS ) {
                ceError = CENTERROR_SUCCESS;
                goto error;
            }
        }

        pGPOModifiedList = pGPOModifiedList->pNext;
    }

    ceError = ProcessSELINUXPolicyFiles(pszSELINUXPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Now remove any stale SELinux type policy files left around */
    ceError = ProcessPolicyRemoveList();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if (pszSELINUXPath)
      LwFreeString(pszSELINUXPath);

    GPA_LOG_FUNCTION_LEAVE(ceError);

    return(ceError);
}

