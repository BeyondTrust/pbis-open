#include "includes.h"

static PPOLICYLIST g_MOTDPolicyHierarchyList = NULL;
static PPOLICYLIST g_MOTDPolicyRemoveList = NULL;

//Global variable to preserve the previously calculated MD5Sum
static char g_szMd5Sum[256] = "";
static BOOLEAN gbMonitor; 

static
CENTERROR
AddPolicyFileToHierarchy(
    PSTR szFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pTemp = g_MOTDPolicyHierarchyList;
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
        g_MOTDPolicyHierarchyList = pNew;
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
    PPOLICYLIST pTemp = g_MOTDPolicyRemoveList;
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
        g_MOTDPolicyRemoveList = pNew;
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
    PPOLICYLIST pTemp = g_MOTDPolicyRemoveList;
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
                g_MOTDPolicyRemoveList = pTemp;
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
    while (g_MOTDPolicyRemoveList) {
        PPOLICYLIST pTemp = g_MOTDPolicyRemoveList;
        g_MOTDPolicyRemoveList = g_MOTDPolicyRemoveList->next;
        if (pTemp->FileName) {
            LwFreeString(pTemp->FileName);
        }
        LwFreeMemory( pTemp );
    }
}

static
CENTERROR
GetCurrentListOfMOTDPolicies()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];
    FILE* pFile = NULL;
    BOOLEAN fExists = FALSE;

    ceError = GPACheckDirectoryExists(CENTERIS_GP_DIRECTORY,
                                      &fExists );
    BAIL_ON_CENTERIS_ERROR( ceError );

#if defined(__LWI_DARWIN__)
    sprintf( szBuf, "ls %s*_likewise_motd 2> /dev/null", CENTERIS_GP_DIRECTORY);
#else
    sprintf( szBuf, "UNIX95=1 ls %s*_likewise_motd 2> /dev/null", CENTERIS_GP_DIRECTORY);
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
ProcessMOTDPolicyFiles(
    PSTR pszConfigFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pPolicyList = g_MOTDPolicyHierarchyList;
    BOOLEAN bFileExists = 0;
    mode_t dwPerms = 0444;
    uid_t uid = 0;
    gid_t gid = 0;

    while (pPolicyList && pPolicyList->next) {
        // Walk to last motd file in our list
        pPolicyList = pPolicyList->next;
    }

    /* If no pPolicyList item, then we should restore system to original setup. */
    if (!pPolicyList) {

        ceError = GPACheckFileExists( MOTD_OLD_FILE, 
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            GPA_LOG_VERBOSE("Reverting MOTD to original system file.");
            /* Delete the motd file in /etc */
            LwRemoveFile(pszConfigFilePath);

            /* Move the saved original file to it's proper place. */
            GPAMoveFileAcrossDevices( MOTD_OLD_FILE, 
                                     pszConfigFilePath);
            //Reset md5sum
            memset(g_szMd5Sum,0,sizeof(g_szMd5Sum));
        }
    } else {

        ceError = GPACheckFileExists( MOTD_OLD_FILE, 
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!bFileExists) {

            /* Back up the original motd file so that we are able to revert to it. */
            ceError = GPACheckFileExists( pszConfigFilePath, 
                                         &bFileExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (bFileExists) {
                GPA_LOG_VERBOSE("Backing up original MOTD system file.");
                GPAMoveFileAcrossDevices( pszConfigFilePath, 
                                         MOTD_OLD_FILE);
            }
        }

        ceError = GPACheckFileExists( MOTD_OLD_FILE, 
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            ceError = LwGetOwnerAndPermissions( MOTD_OLD_FILE, 
                                                &uid, 
                                                &gid,
                                                &dwPerms);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        /* Now move a copy of the new motd file into it's proper place. */
        GPA_LOG_VERBOSE("Applying group policy version of MOTD to system file.");
        ceError = GPACopyFileWithOriginalPerms( pPolicyList->FileName,
                                               pszConfigFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwChangeOwnerAndPermissions( pszConfigFilePath,
                                               uid,
                                               gid, 
                                               dwPerms);
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
    PPOLICYLIST pPolicyList = g_MOTDPolicyRemoveList;
    BOOLEAN bFileExists = 0;

    GPA_LOG_INFO("Processing MOTD remove list...");

    while (pPolicyList) {
        GPA_LOG_INFO( "MOTD Remove List Entry: %s",
                      pPolicyList->FileName);

        ceError = GPACheckFileExists( pPolicyList->FileName,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            GPA_LOG_VERBOSE("Removing stale MOTD file.");
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
    while (g_MOTDPolicyHierarchyList) {
        PPOLICYLIST pTemp = g_MOTDPolicyHierarchyList;
        g_MOTDPolicyHierarchyList = g_MOTDPolicyHierarchyList->next;
        if (pTemp->FileName) {
            LwFreeString(pTemp->FileName);
        }
        LwFreeMemory( pTemp );
    }
}

static
CENTERROR
AddMOTDPolicy(
    PGROUP_POLICY_OBJECT pGPO
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szNewFilePath[PATH_MAX+1];
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    BOOLEAN bFileExists = 0;
    PSTR pszDCAddress = NULL;
    PSTR pszgPCFileSysPath = NULL;
    BOOLEAN fNewPolicy = FALSE;
    BOOLEAN bMonitor = FALSE;

    PGPOLWIGPITEM pGPItem = NULL;
    PGPOLWIDATA pLwidata = NULL;
    PSTR pszMOTDText = NULL;
    xmlNodePtr pItemNode = NULL;
    FILE *fp = NULL;

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
             "%s%s_likewise_motd",
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
            GPA_LOG_ALWAYS( "Former motd policy has been mysteriously removed from system, will attempt to recover: Domain Name [%s] Dest Path [%s]",
                            pszDomainName,
                            szNewFilePath);
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
                                  LWIMOTD_CLIENT_GUID );
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
                                LWIMOTD_ITEM_GUID,
                                &pGPItem);
        if ( ceError != CENTERROR_SUCCESS ) {
            GPA_LOG_ALWAYS( "MOTD policy gpitem not found for: Domain Name [%s] Policy [%s]",
                            pszDomainName,
                            pszPolicyIdentifier);
            GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");

            ceError = CENTERROR_SUCCESS;
            goto error;
        }

        /*
         * Fetch the MOTD node from the GPItem
         */
        ceError = GPOXmlSelectSingleNode( pGPItem->xmlNode,
                                          MOTD_XML_NODE_QUERY,
                                          &pItemNode );
        if ( ceError != CENTERROR_SUCCESS ) {
            GPA_LOG_ALWAYS( "MOTD policy setting not found for: Domain Name [%s] Policy [%s]",
                            pszDomainName, 
                            pszPolicyIdentifier);
            GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");

            ceError = CENTERROR_SUCCESS;
            goto error;
        }

        ceError = GPOXmlGetInnerText( pItemNode,
                                      &pszMOTDText );        
        if ( ceError != CENTERROR_SUCCESS ) {
            GPA_LOG_ALWAYS( "MOTD policy setting value could not be read for: Domain Name [%s] Policy [%s]", 
                            pszDomainName, 
                            pszPolicyIdentifier);
            GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");

            ceError = CENTERROR_SUCCESS;
            goto error;
        }

        ceError = GetMonitorFile( pGPItem->xmlNode,
                                  "setting",
                                  &bMonitor);
        BAIL_ON_CENTERIS_ERROR(ceError);

        //set the monitor file flag
        gbMonitor = bMonitor;

        //Fix for white spaces as libxml2 is adding extra newline and whitespace characters

        // Remove leading whitespace and newline characters which are added by libxml2 only.
        GPARemoveLeadingWhitespacesOnly( pszMOTDText );

        // Remove trailing whitespace and newline characters which are added by libxml2 only.
        GPARemoveTrailingWhitespacesOnly( pszMOTDText );

        ceError = GPAOpenFile( szNewFilePath, 
                               "w", 
                               &fp);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPAFilePrintf( fp,
                                 "%s",
                                 pszMOTDText);
        BAIL_ON_CENTERIS_ERROR(ceError);

        GPA_LOG_VERBOSE( "Received motd policy: Domain Name [%s] MOTDText [%s] Dest Path [%s]",
                         pszDomainName,
                         pszMOTDText,
                         szNewFilePath);

        ceError = AddPolicyFileToHierarchy( szNewFilePath );
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = RemovePolicyFileFromRemoveList(szNewFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if ( fp ) {
        fclose( fp );
    }		

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);
    LW_SAFE_FREE_STRING(pszDCAddress);
    LW_SAFE_FREE_STRING(pszMOTDText);

    if (pGPItem) {
       GPODestroyGPItem(pGPItem, FALSE);
    }

    if (pLwidata) {
        GPODestroyLwiData(pLwidata);
    }

    return(ceError);
}

CENTERROR
ResetMOTDGroupPolicy(
        PGPUSER pUser
        )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* There is nothing to reset for this policy */

    return ceError;
}

CENTERROR
ProcessMOTDGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = 0;
    PSTR pszMOTDPath = NULL;
    PSTR pszNewMd5Sum = NULL;

    GPA_LOG_FUNCTION_ENTER();

    ceError = LwAllocateString( MOTD_FILE, 
                                &pszMOTDPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /*
     * Reset our in memory motd policy list, since we need to re-determine it
     * and apply the most appropriate policy based on the domain hierarchy.
     */
    ResetPolicyFileHierarchy();
    ResetPolicyRemoveList();

    /*
     * Determine the list of MOTD type policy files that we may have currently
     * in effect. We will cross off the list those that seem to still be valid.
     */
    GetCurrentListOfMOTDPolicies();

    /*
     * Process old motd policies to remove
     */
    while ( pGPODeletedList ) {
        ceError = GPARemovePolicy( pGPODeletedList,
                                   "motd");
        BAIL_ON_CENTERIS_ERROR(ceError);

        pGPODeletedList = pGPODeletedList->pNext;
    }

    /*
     * Process new motd policies to add
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
                                       "motd");
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE( "GPO(%s) is disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);
            ceError = GPARemovePolicy( pGPOModifiedList,
                                       "motd");
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            ceError = AddMOTDPolicy(pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        pGPOModifiedList = pGPOModifiedList->pNext;
    }

    //Monitor sudoers file
    if(gbMonitor) {
        GPA_LOG_VERBOSE("Monitoring motd Flag is set, Going to monitor motd file");

        ceError = MonitorSystemFiles( pszMOTDPath,
                                      g_szMd5Sum,
                                      &pszNewMd5Sum);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strncpy( g_szMd5Sum,
                 pszNewMd5Sum,
                (strlen(pszNewMd5Sum)));
    }

    ceError = ProcessMOTDPolicyFiles(pszMOTDPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Now remove any stale MOTD type policy files left around */
    ceError = ProcessPolicyRemoveList();
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(gbMonitor) {

        GPA_LOG_VERBOSE("Going to calculate md5Sum for the new motd file");

        ceError = MonitorSystemFiles( pszMOTDPath,
                                      g_szMd5Sum,
                                      &pszNewMd5Sum);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strncpy( g_szMd5Sum,
                 pszNewMd5Sum,
                (strlen(pszNewMd5Sum)));

    }

error:

    LW_SAFE_FREE_STRING(pszNewMd5Sum);
    LW_SAFE_FREE_STRING(pszMOTDPath);

    //uninitialize cse events
    UninitializeCSEEvents();

    GPA_LOG_FUNCTION_LEAVE(ceError);

    return(ceError);

}
