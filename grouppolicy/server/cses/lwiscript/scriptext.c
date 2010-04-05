#include "includes.h"

static PPOLICYLIST g_ScriptPolicyRemoveList = NULL;

//Global variable to preserve the previously calculated MD5Sum
static char g_szMd5Sum[256] = "";
static BOOLEAN gbMonitor; 
static CHAR gScriptFile[PATH_MAX+1];

static
CENTERROR
ExecutePolicyScript(
    PSTR pszFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szCommand[PATH_MAX + 1];

    sprintf(szCommand, "%s", pszFileName);

    GPA_LOG_VERBOSE( "Script to process: %s", 
                     szCommand);

    if (system(szCommand) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return ceError;
}

static
CENTERROR
AddPolicyFileToRemoveList(
    PSTR szFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pTemp = g_ScriptPolicyRemoveList;
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
        g_ScriptPolicyRemoveList = pNew;
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
    PPOLICYLIST pTemp = g_ScriptPolicyRemoveList;
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
                g_ScriptPolicyRemoveList = pTemp;
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
    while (g_ScriptPolicyRemoveList) {
        PPOLICYLIST pTemp = g_ScriptPolicyRemoveList;
        g_ScriptPolicyRemoveList = g_ScriptPolicyRemoveList->next;
        if (pTemp->FileName) {
            LwFreeString(pTemp->FileName);
        }
        LwFreeMemory( pTemp );
    }
}

static
CENTERROR
GetCurrentListOfScriptPolicies()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];
    FILE* pFile = NULL;
    BOOLEAN fExists = FALSE;

    ceError = GPACheckDirectoryExists(CENTERIS_GP_DIRECTORY,
                                     &fExists );
    BAIL_ON_CENTERIS_ERROR( ceError );

#if defined(__LWI_DARWIN__)
    sprintf( szBuf, "ls %s*_likewise_script 2> /dev/null", CENTERIS_GP_DIRECTORY);
#else
    sprintf( szBuf, "UNIX95=1 ls %s*_likewise_script 2> /dev/null", CENTERIS_GP_DIRECTORY);
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
ProcessPolicyRemoveList()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPOLICYLIST pPolicyList = g_ScriptPolicyRemoveList;
    BOOLEAN bFileExists = 0;

    GPA_LOG_INFO("Processing Script remove list...");

    while (pPolicyList) {
        GPA_LOG_INFO( "Script Remove List Entry: %s",
                      pPolicyList->FileName);

        ceError = GPACheckFileExists( pPolicyList->FileName,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            GPA_LOG_VERBOSE("Removing stale Script file.");
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
CENTERROR
AddScriptPolicy(
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
    PSTR pszDC = NULL;
    PSTR pszgPCFileSysPath = NULL;
    BOOLEAN fNewPolicy = FALSE;
    BOOLEAN bMonitor = FALSE;
    PSTR pszNewMd5Sum = NULL;

    PGPOLWIGPITEM pGPItem = NULL;
    PGPOLWIDATA pLwidata = NULL;
    xmlNodePtr pXmlNodePtr = NULL;
    PSTR pszPolicyFilename = NULL;

    if (!pGPO)
    {
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
             "%s%s_likewise_script",
             SCRIPT_DIRECTORY,
             pszPolicyIdentifier);

    strcpy(gScriptFile,szNewFilePath);


    /*
     * If An Existing policy
     */
    if ( !fNewPolicy ) {
        ceError = GPACheckFileExists( szNewFilePath,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!bFileExists) {
            fNewPolicy = TRUE;
            GPA_LOG_ALWAYS( "Former script policy has been mysteriously removed from system, will attempt to recover: Domain Name [%s] Dest Path [%s]",
                            pszDomainName,
                            szNewFilePath);
        } else {
            ceError = RemovePolicyFileFromRemoveList(szNewFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        //Reset md5sum
        memset(g_szMd5Sum,0,sizeof(g_szMd5Sum));
    }

    if (fNewPolicy) {

        // Look in new policy file location
        ceError = GPOInitLwiData( NULL,
                                  MACHINE_GROUP_POLICY,
                                  (PGPOLWIDATA*)&pLwidata,
                                  pGPO->pszgPCFileSysPath,
                                  NULL,
                                  LWISCRIPT_CLIENT_GUID );
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
                                LWISCRIPT_ITEM_GUID,
                                &pGPItem);

        if ( !CENTERROR_IS_OK(ceError)  &&
             CENTERROR_EQUAL( ceError, 
                              CENTERROR_GP_XML_GPITEM_NOT_FOUND) ) {
            ceError = CENTERROR_SUCCESS;
            goto done;
        }
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPOXmlSelectSingleNode( pGPItem->xmlNode,
                                          "script/setting",
                                          &pXmlNodePtr);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPOXmlGetInnerText( pXmlNodePtr,
                                      &pszPolicyFilename);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPAGetPreferredDomainController(pszDomainName, &pszDC);
        if ( !CENTERROR_IS_OK(ceError) ) {
            if ( CENTERROR_EQUAL(ceError, CENTERROR_GP_NO_SMB_KRB5_SITE_KDC_INFO)   ||
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
                 "%s\\Machine\\Centeris\\Identity\\%s\\%s",
                 pszSourcePath,
                 LWISCRIPT_CLIENT_GUID,
                 pszPolicyFilename);

        ceError = GPOLwioCopyFile(NULL,
                                  pszDomainName, 
                                  pszDC, 
                                  szFilePath, 
                                  SCRIPT_TMP_FILE);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPACheckFileExists( SCRIPT_TMP_FILE, 
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            ceError = LwChangeOwnerAndPermissions( SCRIPT_TMP_FILE, 
                                                   0, 
                                                   0, 
                                                   S_IRWXU);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = GPAMoveFileAcrossDevices( SCRIPT_TMP_FILE, 
                                               szNewFilePath);
            if (ceError) {
                LwRemoveFile(SCRIPT_TMP_FILE);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            if(gbMonitor) {

                GPA_LOG_VERBOSE("Going to calculate md5Sum for the new script file");

                ceError = MonitorSystemFiles( szNewFilePath,
                                              g_szMd5Sum,
                                              &pszNewMd5Sum);
                BAIL_ON_CENTERIS_ERROR(ceError);

                strncpy( g_szMd5Sum,
                         pszNewMd5Sum,
                        (strlen(pszNewMd5Sum)));
            }

            GPA_LOG_VERBOSE( "Received Script policy: Domain Name [%s] Source Path [%s] Dest Path [%s]", 
                             pszDomainName, 
                             szFilePath, 
                             szNewFilePath);

            ceError = ExecutePolicyScript( szNewFilePath );
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = RemovePolicyFileFromRemoveList(szNewFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else {
            GPA_LOG_ALWAYS( "Script policy not found for: Domain Name [%s] Source Path [%s] ", 
                            pszDomainName, 
                            szFilePath);
            GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");
        }
    }

done:
error:
 
    if (pszPolicyFilename) {
        LwFreeString(pszPolicyFilename);
    }

    if (pszNewMd5Sum) {
        LwFreeString(pszNewMd5Sum);
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

    if (pszDC) {
        LwFreeString(pszDC);
    }

    if (pGPItem) {
        GPODestroyGPItem(pGPItem, FALSE);
    }

    if ( pLwidata ) {
        GPODestroyLwiData(pLwidata);
    }

    return(ceError);
}

CENTERROR
ResetScriptGroupPolicy(
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* There is nothing to reset for this policy */

    return ceError;
}


CENTERROR
ProcessScriptGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;
    PSTR pszNewMd5Sum = NULL;
 
    GPA_LOG_FUNCTION_ENTER();

    ResetPolicyRemoveList();

    /*
     * Determine the list of Script type policy files that we may have currently
     * in effect. We will cross off the list those that seem to still be valid.
     */
    GetCurrentListOfScriptPolicies();

    /*
     * Process old Script policies to remove
     */
    while ( pGPODeletedList ) {
        ceError =  GPARemovePolicy( pGPODeletedList,
                                    "script");
        BAIL_ON_CENTERIS_ERROR(ceError);
        pGPODeletedList = pGPODeletedList->pNext;
    }

    /*
     * Process new Script policies to add
     */
    while ( pGPOModifiedList ) {
        BOOLEAN applicable;
            
        ceError = GPOXmlVerifyPlatformApplicable( NULL,
                                                  MACHINE_GROUP_POLICY,
                                                  pGPOModifiedList,
                                                  NULL,
                                                  &applicable);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        if ( !applicable ) {
            GPA_LOG_VERBOSE( "GPO(%s) disabled by platform targeting",
                             pGPOModifiedList->pszDisplayName);
            ceError = GPARemovePolicy( pGPOModifiedList,
                                       "script");
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else if ( pGPOModifiedList->dwFlags & 0x00000002 ) {
            GPA_LOG_VERBOSE( "GPO(%s) is disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);
            ceError =  GPARemovePolicy( pGPOModifiedList,
                                        "script");
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else if ( pGPOModifiedList->bNewVersion ) {
            ceError =  AddScriptPolicy(pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        pGPOModifiedList = pGPOModifiedList->pNext;
    }

    if(gbMonitor) {

        ceError = GPACheckFileExists( gScriptFile,
                                      &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if(bFileExists) {

            GPA_LOG_VERBOSE("Monitoring script Flag is set, Going to monitor script file");

            ceError = MonitorSystemFiles( gScriptFile,
                                          g_szMd5Sum,
                                          &pszNewMd5Sum);
            BAIL_ON_CENTERIS_ERROR(ceError);

            strncpy( g_szMd5Sum,
                     pszNewMd5Sum,
                    (strlen(pszNewMd5Sum)));
        }

    }

    /* Now remove any stale Script type policy files left around */
    ceError = ProcessPolicyRemoveList();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:
 
    if (pszNewMd5Sum) {
        LwFreeString(pszNewMd5Sum);
    }

    GPA_LOG_FUNCTION_LEAVE(ceError);

    return(ceError);
}
