#include "includes.h"

static PPOLICYLIST g_PromptPolicyHierarchyList = NULL;
static PPOLICYLIST g_PromptPolicyRemoveList = NULL;


static
CENTERROR
AddPolicyFileToHierarchy(
    PSTR szFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pTemp = g_PromptPolicyHierarchyList;
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
        g_PromptPolicyHierarchyList = pNew;
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
    PPOLICYLIST pTemp = g_PromptPolicyRemoveList;
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
        g_PromptPolicyRemoveList = pNew;
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
    PPOLICYLIST pTemp = g_PromptPolicyRemoveList;
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
                g_PromptPolicyRemoveList = pTemp;
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
    while (g_PromptPolicyRemoveList) {
        PPOLICYLIST pTemp = g_PromptPolicyRemoveList;
        g_PromptPolicyRemoveList = g_PromptPolicyRemoveList->next;
        if (pTemp->FileName) {
            LwFreeString(pTemp->FileName);
        }
        LwFreeMemory( pTemp );
    }
}

static
CENTERROR
GetCurrentListOfLoginPromptPolicies()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];
    FILE* pFile = NULL;
    BOOLEAN fExists = FALSE;

    ceError = GPACheckDirectoryExists(CENTERIS_GP_DIRECTORY,
                                     &fExists );
    BAIL_ON_CENTERIS_ERROR( ceError );

#if defined(__LWI_DARWIN__)
    sprintf( szBuf, "ls %s*_likewise_issue 2> /dev/null", CENTERIS_GP_DIRECTORY);
#else
    sprintf( szBuf, "UNIX95=1 ls %s*_likewise_issue 2> /dev/null", CENTERIS_GP_DIRECTORY);
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
ProcessLoginPromptPolicyFiles(
    PSTR pszConfigFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pPolicyList = g_PromptPolicyHierarchyList;
    BOOLEAN bFileExists = 0;
    mode_t dwPerms = 0440;
    uid_t uid = 0;
    gid_t gid = 0;

    while (pPolicyList && pPolicyList->next) {
        // Walk to last issue file in our list
        pPolicyList = pPolicyList->next;
    }

    /* If no pPolicyList item, then we should restore system to original setup. */
    if (!pPolicyList) {
        ceError = GPACheckFileExists( ISSUE_OLD_FILE, 
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            GPA_LOG_VERBOSE("Reverting ISSUE to original system file.");
            /* Delete the issue file in /etc */
            LwRemoveFile(pszConfigFilePath);

            /* Move the saved original file to it's proper place. */
            GPAMoveFileAcrossDevices( ISSUE_OLD_FILE, 
                                     pszConfigFilePath);
        }
    } else {

        ceError = GPACheckFileExists( ISSUE_OLD_FILE, 
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!bFileExists) {

            /* Back up the original issue file so that we are able to revert to it. */
            ceError = GPACheckFileExists( pszConfigFilePath, 
                                         &bFileExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (bFileExists) {
                GPA_LOG_VERBOSE("Backing up original ISSUE system file.");
                GPAMoveFileAcrossDevices( pszConfigFilePath, 
                                         ISSUE_OLD_FILE);
            }
        }

        ceError = GPACheckFileExists( ISSUE_OLD_FILE, 
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            ceError = LwGetOwnerAndPermissions( ISSUE_OLD_FILE, 
                                                &uid, 
                                                &gid, 
                                                &dwPerms);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        /* Now move a copy of the new issue file into it's proper place. */
        GPA_LOG_VERBOSE("Applying group policy version of ISSUE to system file.");
        ceError = GPACopyFileWithOriginalPerms( pPolicyList->FileName, 
                                               pszConfigFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwChangeOwnerAndPermissions( pszConfigFilePath, 
                                               uid, 
                                               gid, 
                                               dwPerms);
        BAIL_ON_CENTERIS_ERROR(ceError);

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
    PPOLICYLIST pPolicyList = g_PromptPolicyRemoveList;
    BOOLEAN bFileExists = 0;

    GPA_LOG_INFO("Processing Login Prompt remove list...");

    while (pPolicyList) {
        GPA_LOG_INFO( "Login Prompt Remove List Entry: %s",
                      pPolicyList->FileName);

        ceError = GPACheckFileExists( pPolicyList->FileName,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            GPA_LOG_VERBOSE("Removing stale Login Prompt file.");
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
    while (g_PromptPolicyHierarchyList) {
        PPOLICYLIST pTemp = g_PromptPolicyHierarchyList;
        g_PromptPolicyHierarchyList = g_PromptPolicyHierarchyList->next;
        if (pTemp->FileName) {
            LwFreeString(pTemp->FileName);
        }
        LwFreeMemory( pTemp );
    }
}

static
CENTERROR
AddLoginPromptPolicy(
    PGROUP_POLICY_OBJECT pGPO
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szFilePath[PATH_MAX+1];
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
    PSTR pszLoginPromptText = NULL;
    xmlNodePtr pItemNode = NULL;
    FILE *fp = NULL;

    if ( !pGPO ) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    szFilePath[0] = '\0';
    szNewFilePath[0] = '\0';

    pszgPCFileSysPath = pGPO->pszgPCFileSysPath;
    fNewPolicy = pGPO->bNewVersion;

    ceError =  GPACrackFileSysPath( pszgPCFileSysPath,
                                    &pszDomainName,
                                    &pszSourcePath,
                                    &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf( szNewFilePath, 
             "%s%s_likewise_issue",
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
            GPA_LOG_ALWAYS( "Former issue policy has been mysteriously removed from system, will attempt to recover: Domain Name [%s] Dest Path [%s]", 
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
                                  LWIPROMPT_CLIENT_GUID );
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
                                LWIPROMPT_ITEM_GUID,
                                &pGPItem);
        if ( ceError != CENTERROR_SUCCESS ) {
            GPA_LOG_ALWAYS( "Login Prompt policy gpitem not found for: Domain Name [%s] Policy [%s]", 
                            pszDomainName, 
                            pszPolicyIdentifier);
            GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");

            ceError = CENTERROR_SUCCESS;
            goto error;
        }

        /*
         * Fetch the Login Prompt node from the GPItem
         */
        ceError = GPOXmlSelectSingleNode( pGPItem->xmlNode,
                                          LOGIN_PROMPT_XML_NODE_QUERY,
                                          &pItemNode );
        if ( ceError != CENTERROR_SUCCESS ) {
            GPA_LOG_ALWAYS( "Login Prompt policy setting not found for: Domain Name [%s] Policy [%s]", 
                            pszDomainName, 
                            pszPolicyIdentifier);
            GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");

            ceError = CENTERROR_SUCCESS;
            goto error;
        }

        ceError = GPOXmlGetInnerText( pItemNode,
                                      &pszLoginPromptText );
        if ( ceError != CENTERROR_SUCCESS ) {
            GPA_LOG_ALWAYS( "Login Prompt policy setting value could not be read for: Domain Name [%s] Policy [%s]", 
                            pszDomainName, 
                            pszPolicyIdentifier);
            GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");

            ceError = CENTERROR_SUCCESS;
            goto error;
        }

        //Fix for white spaces as libxml2 is adding extra newline and whitespace characters

        //Remove leading white spaces only. Preserve newline character.
        LwStripLeadingWhitespace( pszLoginPromptText );

        //Remove trailing white spaces only. Preserve newline character.
        LwStripTrailingWhitespace( pszLoginPromptText );

        ceError = GPAOpenFile( szNewFilePath,
                              "w",
                              &fp);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPAFilePrintf( fp,
                                "%s",
                                pszLoginPromptText);
        BAIL_ON_CENTERIS_ERROR(ceError);

        GPA_LOG_VERBOSE( "Received issue policy: Domain Name [%s] Source Path [%s] Dest Path [%s]", 
                         pszDomainName, 
                         szFilePath, 
                         szNewFilePath);

        ceError = AddPolicyFileToHierarchy(szNewFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = RemovePolicyFileFromRemoveList(szNewFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if ( fp ) {
        fclose( fp );
    }		

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

    if (pszLoginPromptText) {
        LwFreeString(pszLoginPromptText);
    }

    if (pGPItem) {
       GPODestroyGPItem(pGPItem, FALSE);
    }

    if (pLwidata) {
        GPODestroyLwiData(pLwidata);
    }

    return(ceError);
}

CENTERROR
ResetLoginPromptGroupPolicy(
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* There is nothing to reset for this policy */

    return ceError;
}

CENTERROR
ProcessLoginPromptGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = 0;
    PSTR pszLoginPromptPath = NULL;

    GPA_LOG_FUNCTION_ENTER();

    ceError = LwAllocateString( ISSUE_FILE, 
                                &pszLoginPromptPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /*
     * Reset our in memory issue policy list, since we need to re-determine it
     * and apply the most appropriate policy based on the domain hierarchy.
     */
    ResetPolicyFileHierarchy();
    ResetPolicyRemoveList();

    /*
     * Determine the list of Login Prompt type policy files that we may have currently
     * in effect. We will cross off the list those that seem to still be valid.
     */
    GetCurrentListOfLoginPromptPolicies();

    /*
     * Process old issue policies to remove
     */
    while ( pGPODeletedList ) {
        ceError =  GPARemovePolicy( pGPODeletedList,
                                    "issue");
        BAIL_ON_CENTERIS_ERROR(ceError);

        pGPODeletedList = pGPODeletedList->pNext;
    }

    /*
     * Process new issue policies to add
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
                                       "issue" );
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE( "GPO(%s) is disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);
            ceError =  GPARemovePolicy( pGPOModifiedList,
                                        "issue");
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            ceError = AddLoginPromptPolicy(pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        pGPOModifiedList = pGPOModifiedList->pNext;
    }

    ceError = ProcessLoginPromptPolicyFiles(pszLoginPromptPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Now remove any stale Login Prompt type policy files left around */
    ceError = ProcessPolicyRemoveList();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if (pszLoginPromptPath) {
      LwFreeString(pszLoginPromptPath);
    }

    GPA_LOG_FUNCTION_LEAVE(ceError);

    return(ceError);
}

