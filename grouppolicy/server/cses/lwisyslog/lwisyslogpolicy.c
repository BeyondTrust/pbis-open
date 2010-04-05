#include <includes.h>

static CHAR gSyslogTempFile[PATH_MAX+1];
static CHAR gSyslogConfFile [PATH_MAX+1];
static CHAR gSyslogConfFileOriginal [PATH_MAX+1];
static CHAR gSyslogConfFileBackup [PATH_MAX+1];

//Global variable to preserve the previously calculated MD5Sum
static char g_szMd5Sum[256] = "";
static BOOLEAN gbMonitor; 

static
VOID
SetValidSyslogFile(
    BOOLEAN bUseNG
    )
{
    if ( bUseNG )
    {
        strcpy(gSyslogTempFile,LWISYSLOG_NG_TEMP_FILE);
        strcpy(gSyslogConfFile,LWISYSLOG_NG_CONF_FILE);
        strcpy(gSyslogConfFileOriginal,LWISYSLOG_NG_CONF_FILE_ORIGINAL);
        strcpy(gSyslogConfFileBackup,LWISYSLOG_NG_CONF_FILE_BACKUP);
    }
    else
    {
        if ( IsRsyslogSupported() )
        {
            strcpy(gSyslogTempFile,LWIRSYSLOG_TEMP_FILE);
            strcpy(gSyslogConfFile,LWIRSYSLOG_CONF_FILE);
            strcpy(gSyslogConfFileOriginal,LWIRSYSLOG_CONF_FILE_ORIGINAL);
            strcpy(gSyslogConfFileBackup,LWIRSYSLOG_CONF_FILE_BACKUP);
        }
        else 
        {
            strcpy(gSyslogTempFile,LWISYSLOG_TEMP_FILE);
            strcpy(gSyslogConfFile,LWISYSLOG_CONF_FILE);
            strcpy(gSyslogConfFileOriginal,LWISYSLOG_CONF_FILE_ORIGINAL);
            strcpy(gSyslogConfFileBackup,LWISYSLOG_CONF_FILE_BACKUP);
        }
    }
}

static 
CENTERROR
CheckAndBackupOriginal(
    uid_t * pUid,
    gid_t * pGid,
    mode_t * pPerms
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN originalExists = FALSE;
    BOOLEAN currentExists = FALSE;
    mode_t dwPerms = 0644;
    uid_t uid = 0;
    gid_t gid = 0;
    
    ceError = GPACheckFileExists( gSyslogConfFileOriginal, 
                                  &originalExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPACheckFileExists( gSyslogConfFile, 
                                  &currentExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( currentExists && !originalExists )
    {
        ceError = LwGetOwnerAndPermissions( gSyslogConfFile,
                                            &uid, &gid, &dwPerms);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPACopyFileWithOriginalPerms( gSyslogConfFile,
                                                gSyslogConfFileOriginal);
        BAIL_ON_CENTERIS_ERROR(ceError);  
    }
 
    if ( pUid )
        *pUid = uid;
 
    if ( pGid )
        *pGid = gid;
 
    if ( pPerms )
        *pPerms = dwPerms;
    
error:

    return ceError;
}

static 
CENTERROR
CheckFileToBeImported(
    xmlNodePtr root_node,
    PSTR pszEditMode, //replace or append
    PSTR pszAuditMode 
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszeditmode = pszEditMode;
    PSTR pszauditmode = pszAuditMode;
    xmlNodePtr cur_node = NULL;
    xmlChar* editmode = NULL;
    xmlChar* auditmode = NULL;

    for (cur_node = root_node; cur_node; cur_node = cur_node->next) 
    {
        if ( cur_node->name && !strcmp((char*)cur_node->name, "SYSLOGPOLICY")) 
        {
            editmode  = xmlGetProp(cur_node, (const xmlChar*)"editmode");

            if ( editmode )
                strcpy(pszEditMode,(const char*)editmode);

            auditmode = xmlGetProp(cur_node, (const xmlChar*)"likewiseaudit");

            if ( auditmode )
                strcpy(pszAuditMode,(const char*)auditmode);

            xmlFree(editmode);
            xmlFree(auditmode);

            break;
        }

        CheckFileToBeImported( cur_node->children, 
                               pszeditmode,
                               pszauditmode);
    }

    return ceError;
}

static 
CENTERROR
RestoreOriginal()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    
    BOOLEAN originalExists = FALSE;
    
    ceError = GPACheckFileExists( gSyslogConfFileOriginal, 
                                 &originalExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( originalExists )
    {
        ceError = GPACopyFileWithOriginalPerms( gSyslogConfFileOriginal, 
                                                gSyslogConfFile);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwRemoveFile(gSyslogConfFileOriginal);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    return ceError;
}

static 
CENTERROR
ReadFromOrgFile(
    PCSTR pszOrigFile,
    PSYSLOGNODE* ppMerged
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bSyslogng = IsSysLogNG();
    
    if ( bSyslogng )
    {
        ceError = PrepareListFromNGFile( pszOrigFile,
                                         ppMerged);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = PrepareListFromFile( pszOrigFile,
                                       ppMerged);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    return ceError;
}

static
VOID
FormatTargetFilename(
    PSTR pszFileName, 
    PSTR pszFormatedFilename
    )
{
    if ( StringStartsWithChar(pszFileName, '-') )
    {
        pszFileName++;
    }

    strcpy(pszFormatedFilename, pszFileName);

    StripStartAndEndChar(pszFormatedFilename, '"', NULL);
}

static
CENTERROR
CheckIfTargetFileExists(
    PSTR pszFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE* pFileHandle = NULL;
    BOOLEAN bFileExists = FALSE;
    CHAR szDestinationStr[STATIC_PATH_BUFFER_SIZE] = {0};

    FormatTargetFilename(pszFileName, szDestinationStr);

    ceError = GPACheckFileExists( szDestinationStr, 
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if ( !bFileExists )
    {
        // step 1: ensure directory exists
        char* slash = strrchr(szDestinationStr, '/');
        if ( slash )
        {
            // temporarily cut off file name, leaving only
            // the path to the directory
            *slash = 0;
            ceError = LwCreateDirectory( szDestinationStr, 
                                         0755);
            BAIL_ON_CENTERIS_ERROR(ceError);
            *slash = '/';
        }
        
        // step 2: create file with write permission
        ceError = GPAOpenFile( szDestinationStr, 
                              "w", 
                              &pFileHandle);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwChangePermissions( szDestinationStr, 
                                       S_IRUSR|S_IWUSR);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPACloseFile(pFileHandle);
        BAIL_ON_CENTERIS_ERROR(ceError);
        pFileHandle = NULL;
    }

    //In Suse systems, apparmor doesn't allow to log to a specified file. 
    //We need to add an entry in /etc/apparmor.d/sbin.syslog-ng file
    if ( IsApparmorSupported() && IsSysLogNG() ) 
    {
        ceError = UpdateApparmorProfile(szDestinationStr);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    if ( pFileHandle )
    {
        GPACloseFile(pFileHandle);
        pFileHandle = NULL;
    }

    return (ceError);
}

static
CENTERROR
MergeAuditValueToList(
    PSYSLOGNODE* ppMasterList,
    PSTR pszKey,
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bUseNG = IsSysLogNG();

    if ( bUseNG )
    {
        ceError = AppendNGEntryToList( pszKey,
                                       pszValue,
                                       ppMasterList);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = AppendEntryToList( pszKey, 
                                     pszValue,
                                     ppMasterList);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
error:
    return ceError;
}

static
CENTERROR
UpdateAuditValueToListAndApparmor(
    PSTR pszKey,
    PSTR pszFileName,
    PSYSLOGNODE* ppMasterList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszValue = NULL;
    BOOLEAN bUseNG = IsSysLogNG();
    CHAR szPath[STATIC_PATH_BUFFER_SIZE] = {0};

    strcpy(szPath, "/var/lib/likewise/syslog-reaper");

    if ( bUseNG )
    {
        ceError = LwAllocateStringPrintf( &pszValue,
                                          "-\"%s/%s\"",
                                          szPath,
                                          pszFileName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        // Target is a file, ensure it exists in apparmor
        ceError = CheckIfTargetFileExists(pszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
         ceError = LwAllocateStringPrintf( &pszValue,
                                           "%s/%s",
                                           szPath,
                                           pszFileName);
         BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = MergeAuditValueToList( ppMasterList,
                                     pszKey,
                                     pszValue);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:
    LW_SAFE_FREE_STRING(pszValue);
    return ceError;
}

static
CENTERROR
MergeAuditValuesToList(
    PSYSLOGNODE* ppMasterList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szKey[32] = {0};

    strcpy(szKey,"*.err");
    ceError = UpdateAuditValueToListAndApparmor( szKey,
                                                 "error",
                                                 ppMasterList);
    BAIL_ON_CENTERIS_ERROR(ceError);

    strcpy(szKey,"*.warning");
    ceError = UpdateAuditValueToListAndApparmor( szKey,
                                                 "warning",
                                                 ppMasterList);
    BAIL_ON_CENTERIS_ERROR(ceError);

    strcpy(szKey,"*.debug");
    ceError = UpdateAuditValueToListAndApparmor( szKey,
                                                 "information",
                                                 ppMasterList);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:
    return ceError;
}

static 
CENTERROR
AppendToTmpFile(
    FILE *pHandle,
    BOOLEAN bUseNG,
    PSTR  pszKey,
    PSTR  pszValue,
    DWORD dwFilterCnt
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if ( bUseNG )
    {
        ceError = WriteToNGFile( pHandle, 
                                 pszKey,
                                 pszValue,
                                 ++dwFilterCnt);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = WriteToFile( pHandle,
                               pszKey,
                               pszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    return ceError;
}

static
CENTERROR
WriteListToTmpFile(
    PSYSLOGNODE pNGList,
    PSTR SyslogTempFile
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE* pHandle = NULL;
    DWORD dwFilterCnt = 0;
    PSYSLOGNODE pTmp = pNGList;
    BOOLEAN bUseNG = IsSysLogNG();

    ceError = GetFileHandle( SyslogTempFile,
                             bUseNG, 
                             &pHandle);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for (; pTmp; pTmp = pTmp->pNext)
    {
        ceError = AppendToTmpFile( pHandle, 
                                   bUseNG, 
                                   pTmp->pszKey, 
                                   pTmp->pszValue,
                                   ++dwFilterCnt);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    if ( pHandle )
    {
        GPACloseFile(pHandle);
        pHandle = NULL;
    }

    return ceError;
}

static
CENTERROR
RestartApparmor()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if ( IsApparmorSupported() && IsSysLogNG() ) 
    {
        ceError = GPARunCommand("/etc/init.d/boot.apparmor restart");
        if ( ceError != CENTERROR_SUCCESS ) 
        {
            ceError = GPARunCommand("/etc/init.d/apparmor restart");
            if ( ceError != CENTERROR_SUCCESS ) 
            {
                GPA_LOG_VERBOSE("Could not restart apparmor...");
                ceError = CENTERROR_SUCCESS;
            }
        }
    }

    return ceError;
}

/* Iterates the merged settings table and writes the
   config file */
static 
CENTERROR
BuildSyslogConfFile(
    PSYSLOGNODE* ppListFromAD,
    BOOLEAN bEditMode,
    BOOLEAN bAuditMode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSYSLOGNODE pMasterList = NULL; 
    mode_t dwPerms = 0644;
    uid_t uid = 0;
    gid_t gid = 0;
   
    ceError = CheckAndBackupOriginal( &uid, 
                                      &gid, 
                                      &dwPerms);
    BAIL_ON_CENTERIS_ERROR(ceError);
   
    if ( !ppListFromAD )
    {
        ceError = CENTERROR_INVALID_PARAMETER;
        goto error;
    }

    if ( bEditMode )
    {
        ceError = ReadFromOrgFile( gSyslogConfFileOriginal,
                                   &pMasterList);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = MergeFileListAndADList( ppListFromAD,
                                          &pMasterList);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pMasterList = NULL;

    }

    if ( bAuditMode )
    {
        ceError = MergeAuditValuesToList(ppListFromAD);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    //Replace the file
    ceError = WriteListToTmpFile( *ppListFromAD,
                                  gSyslogTempFile);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwMoveFile( gSyslogTempFile, 
                          gSyslogConfFile);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwChangeOwnerAndPermissions( gSyslogConfFile, 
                                           uid, 
                                           gid, 
                                           dwPerms);
    BAIL_ON_CENTERIS_ERROR(ceError);

    //restart apparmor
    ceError = RestartApparmor();
    BAIL_ON_CENTERIS_ERROR(ceError);
   
    //Reset md5sum
    memset(g_szMd5Sum, 0, sizeof(g_szMd5Sum));

error:
    FreeSyslogList(&pMasterList);
    return ceError;
}

static
VOID
FormatNGFacilityStr(
    PSTR pszFacility
    )
{
    PSTR pColon = NULL, pSquare = NULL, pClose = NULL;

    pSquare = strchr(pszFacility, '[');

    while ( pSquare )
    {
        pColon = strchr(pSquare, ';');
        pClose = strchr(pSquare, ']');
        
        while( pColon > pSquare && pColon < pClose )
        {
            *pColon = '+';
            pColon = strchr(pColon+1, ';');
        }
        pSquare = strchr(pSquare+1, '[');
    }
}

static
CENTERROR
UpdateNodeValue( 
    xmlNodePtr pNode,
    PSTR pszAppend,
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszNodeValue = NULL;
    
    ceError = GPOXmlGetInnerText( pNode, 
                                  &pszNodeValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    LwStripWhitespace(pszNodeValue,1,1);

    strcat(pszValue, pszAppend);
    strcat(pszValue, pszNodeValue);
    strcat(pszValue, ")");

error:
    LW_SAFE_FREE_STRING(pszNodeValue);
    return ceError;
}

//SPN: break it in to multiple functions
static
CENTERROR
UpdateNodeInfo(
    xmlNodePtr Node,
    PSYSLOGNODE* ppListFromAD
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR destinationStr = NULL, facilityStr = NULL, synchStr = NULL;
    PSTR activeStr = NULL, destinationTypeStr = NULL;
    xmlNodePtr facilityNode, destinationNode, destinationTypeNode, synchNode;
    xmlNodePtr activeNode, ownerNode, groupNode, permissionNode;
    CHAR szValue[FILEREAD_BUFSIZE] = {0}, szFacility[FILEREAD_BUFSIZE] = {0};
    char const * synchSymbol = NULL;
    BOOLEAN bSyslogNG = IsSysLogNG();
    BOOLEAN bIfDef = FALSE;

    ceError = GPOXmlSelectSingleNode(Node, "Active", &activeNode);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPOXmlGetInnerText(activeNode, &activeStr);
    BAIL_ON_CENTERIS_ERROR(ceError);
    LwStripWhitespace(activeStr,1,1);

    ceError = GPOXmlSelectSingleNode(Node, "FacilitiesPriorities", &facilityNode);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPOXmlGetInnerText(facilityNode, &facilityStr);
    BAIL_ON_CENTERIS_ERROR(ceError);
    LwStripWhitespace(facilityStr,1,1);

    if( GPAStrStartsWith(facilityStr,"ifdef") )
    {
        ceError = LwAllocateString( "  ",
                                    &destinationStr);
        BAIL_ON_CENTERIS_ERROR(ceError);

        bIfDef = TRUE;
    }
    else
    {
        ceError = GPOXmlSelectSingleNode(Node, "Destination", &destinationNode);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        ceError = GPOXmlGetInnerText(destinationNode, &destinationStr);
        BAIL_ON_CENTERIS_ERROR(ceError);
        LwStripWhitespace(destinationStr,1,1);

        if( GPAStrStartsWith(destinationStr,"ifdef") )
        {
            bIfDef = TRUE;
        }
        else
        {
            ceError = GPOXmlSelectSingleNode(Node, "DestinationType", &destinationTypeNode);
            BAIL_ON_CENTERIS_ERROR(ceError);
            
            ceError = GPOXmlSelectSingleNode(Node, "Synch", &synchNode);
            BAIL_ON_CENTERIS_ERROR(ceError);
            
            ceError = GPOXmlSelectSingleNode(Node, "Owner", &ownerNode);

            ceError = GPOXmlSelectSingleNode(Node, "Group", &groupNode);

            ceError = GPOXmlSelectSingleNode(Node, "Permissions", &permissionNode);

            if ( !facilityNode || !destinationNode || !destinationTypeNode || !synchNode || !activeNode ) 
            {
                ceError = CENTERROR_GP_XML_NODE_NOT_FOUND;
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            ceError = GPOXmlGetInnerText(destinationTypeNode, &destinationTypeStr);
            BAIL_ON_CENTERIS_ERROR(ceError);
            LwStripWhitespace(destinationTypeStr,1,1);
            
            ceError = GPOXmlGetInnerText(synchNode, &synchStr);
            BAIL_ON_CENTERIS_ERROR(ceError);
            LwStripWhitespace(synchStr,1,1);
        }
    }

    if ( !strcmp(activeStr, "No") ) 
    {
        //Activate logging is not set, hence define the policy but comment it
        strcpy (szFacility, "#");
        strcat (szFacility, facilityStr);
    }
    else if ( !strcmp(activeStr,"Yes") ) 
    {
        strcpy (szFacility, facilityStr);
    }

    if ( bSyslogNG )
    {
        FormatNGFacilityStr(szFacility);

        if ( !strcmp(synchStr, "Yes") && !strcmp(destinationTypeStr, "File") )
        {
            synchSymbol = "-";
            strcat(szValue, synchSymbol);
            strcat(szValue, "\"");
            strcat(szValue, destinationStr);
        }
        else if ( !strcmp(destinationTypeStr, "Local Users") )
        {
            strcat(szValue, "\"");
            strcat(szValue, destinationStr);
        }
        else
        {
            strncat(szValue, destinationStr, 1);
            strcat(szValue, "\"");
            strcat(szValue, &destinationStr[1]);
        }
        strcat(szValue, "\"");

        if ( !strcmp(synchStr, "Yes") )
        {
            strcat(szValue, " fsync(yes)");
        }

        if ( ownerNode )
        {
            ceError = UpdateNodeValue( ownerNode,
                                       " owner(",
                                       szValue);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if ( groupNode )
        {
            ceError = UpdateNodeValue( groupNode,
                                       " group(",
                                       szValue);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if ( permissionNode )
        {
            ceError = UpdateNodeValue( permissionNode,
                                       " perm(",
                                       szValue);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }
    else
    {
        strcat(szValue, destinationStr);
    }

    if ( bSyslogNG )
    {
        ceError = AppendNGEntryToList( szFacility,
                                       szValue,
                                       ppListFromAD);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = AppendEntryToList( szFacility, 
                                     szValue,
                                     ppListFromAD);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    // If target is a file, ensure it exists
    if ( !bIfDef && !strcmp(destinationTypeStr, "File") ) 
    {
        ceError = CheckIfTargetFileExists(destinationStr);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

cleanup:
    LW_SAFE_FREE_STRING(destinationStr);
    LW_SAFE_FREE_STRING(facilityStr);
    LW_SAFE_FREE_STRING(destinationTypeStr);
    LW_SAFE_FREE_STRING(synchStr);
    LW_SAFE_FREE_STRING(activeStr);
    return (ceError);

error:
    goto cleanup;
}

static
CENTERROR
AddSyslogPolicy(
    PGROUP_POLICY_OBJECT pGPO,
    PSYSLOGNODE* ppListFromAD,
    PBOOLEAN pbEditMode,
    PBOOLEAN pbAuditMode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIDATA pCTidata = NULL;
    PGPOLWIGPITEM pGPItem = NULL;
    xmlNodeSetPtr logItems = NULL;
    CHAR szEditMode[20] = {0};
    CHAR szAuditMode[20] = {0};
    int nIndex = 0;
    BOOLEAN bMonitor = FALSE;

    if ( !pGPO )
    {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    // Look in new policy file location
    ceError = GPOInitLwiData( NULL,
                              MACHINE_GROUP_POLICY,
                              (PGPOLWIDATA*)&pCTidata,
                              pGPO->pszgPCFileSysPath,
                              NULL,
                              LWISYSLOG_CLIENT_GUID);
    if ( ceError )
    {
        // Look in old policy file location
        ceError = GPOInitLwiData( NULL,
                                  MACHINE_GROUP_POLICY,
                                  (PGPOLWIDATA*)&pCTidata,
                                  pGPO->pszgPCFileSysPath,
                                  NULL,
                                  NULL);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    ceError = GPOGetGPItem( MACHINE_GROUP_POLICY,
                            pCTidata, 
                            LWISYSLOG_ITEM_GUID,
                            &pGPItem);

    if ( !CENTERROR_IS_OK(ceError) && 
          CENTERROR_EQUAL( ceError, CENTERROR_GP_XML_GPITEM_NOT_FOUND) ) 
    {
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CheckFileToBeImported( pGPItem->xmlNode,
                                     szEditMode, //replace or append
                                     szAuditMode); //Likewise Audit
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOXmlSelectNodes( (xmlNodePtr) pGPItem->xmlNode, 
                                 "sysLog/setting/SYSLOGPOLICY/SysLogItem", 
                                 &logItems);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GetMonitorFile( pGPItem->xmlNode,
                              "SYSLOGPOLICY",
                              &bMonitor);
    BAIL_ON_CENTERIS_ERROR(ceError);

    //set the monitor file flag
    gbMonitor = bMonitor;

    if ( logItems )
    {
        for ( nIndex = 0; nIndex < logItems->nodeNr; nIndex++ ) 
        {
            ceError = UpdateNodeInfo( logItems->nodeTab[nIndex], 
                                      ppListFromAD);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    if ( !(strcmp((const char*)szEditMode, "append")) )
        *pbEditMode = TRUE;
    else
        *pbEditMode = FALSE;

    if ( !(strcmp((const char*)szAuditMode, "true")) )
        *pbAuditMode = TRUE;
    else
        *pbAuditMode = FALSE;

cleanup:
    if ( logItems )
    {
        xmlXPathFreeNodeSet(logItems);
    }

    if ( pGPItem )
    {
       GPODestroyGPItem(pGPItem, FALSE);
       pGPItem = NULL;
    }

    if ( pCTidata )
    {
        GPODestroyLwiData(pCTidata);
        pCTidata = NULL;
    }

    return ceError;

error:
    goto cleanup;
}

CENTERROR
ProcessSyslogGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSYSLOGNODE pListFromAD = NULL;
    BOOLEAN bUseNG = IsSysLogNG();
    BOOLEAN bBuildConfFile = FALSE;
    BOOLEAN bNeedHUP = FALSE;
    BOOLEAN bEditMode = FALSE;
    BOOLEAN bAuditMode = FALSE;
    BOOLEAN bApplicable = FALSE;
    PSTR pszBuf = NULL;
    PSTR pszNewMd5Sum = NULL;
 
    GPA_LOG_FUNCTION_ENTER();

    SetValidSyslogFile(bUseNG);

    for (; pGPOModifiedList; pGPOModifiedList = pGPOModifiedList->pNext)
    {
        bApplicable = FALSE;
        ceError = GPOXmlVerifyPlatformApplicable( NULL,
                                                  MACHINE_GROUP_POLICY,
                                                  pGPOModifiedList,
                                                  NULL,
                                                  &bApplicable);

        BAIL_ON_CENTERIS_ERROR(ceError);
        
        if ( !bApplicable )
        {
            GPA_LOG_VERBOSE("GPO(%s) disabled by platform targeting",
                            pGPOModifiedList->pszDisplayName);
            continue;
        } 
        else if ( pGPOModifiedList->dwFlags & 0x00000002 ) 
        {
            GPA_LOG_VERBOSE( "GPO(%s) is disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);

        } 
        else 
        {
            // Set the flag to send HUP
            if ( pGPOModifiedList->bNewVersion )
            {
                bNeedHUP = TRUE;

                ceError = AddSyslogPolicy( pGPOModifiedList,
                                           &pListFromAD,
                                           &bEditMode,
                                           &bAuditMode);
                if ( !CENTERROR_IS_OK(ceError) && CENTERROR_EQUAL(ceError, CENTERROR_GP_XML_GPITEM_NOT_FOUND) ) 
                {
                    ceError = CENTERROR_SUCCESS;
                    continue;
                }
                BAIL_ON_CENTERIS_ERROR(ceError);

                bBuildConfFile = TRUE;
            }
        }
    }
    
    if ( gbMonitor )
    {
        GPA_LOG_VERBOSE("Monitoring syslog Flag is set, Going to monitor syslog file");

        ceError = MonitorSystemFiles( gSyslogConfFile,
                                      g_szMd5Sum,
                                      &pszNewMd5Sum);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strncpy( g_szMd5Sum,
                 pszNewMd5Sum,
                (strlen(pszNewMd5Sum)));
    }

    if ( bBuildConfFile )
    {
        ceError = BuildSyslogConfFile( &pListFromAD,
                                       bEditMode,
                                       bAuditMode);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else if ( pGPODeletedList )
    {
        ceError = RestoreOriginal();
        BAIL_ON_CENTERIS_ERROR(ceError);

        bNeedHUP = TRUE;
    }

    if ( gbMonitor )
    {
        GPA_LOG_VERBOSE("Going to calculate md5Sum for the new syslog file");

        ceError = MonitorSystemFiles( gSyslogConfFile,
                                      g_szMd5Sum,
                                      &pszNewMd5Sum);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strncpy( g_szMd5Sum,
                 pszNewMd5Sum,
                (strlen(pszNewMd5Sum)));
    }

    /* See if this system might be using SELinux to protect itself, if so
       add syslog daemon to process exception list so that any configuration
       actions to not cause SELinux to complain */
    if ( IsSELinuxSupported() )
    {
        ceError = AddSELinuxExceptionForSyslog();
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if ( bAuditMode )
    {
        GPA_LOG_VERBOSE( "Going to configure the reapsysld daemon to make sure it is running.");

        GPACaptureOutput("/opt/likewise/bin/domainjoin-cli configure --enable reapsysld", &pszBuf);

        if ( IsNullOrEmptyString(pszBuf) )
        {
            GPA_LOG_VERBOSE( "/opt/likewise/bin/domainjoin-cli configure --enable reapsysld command returned no result");
        }
        else
        {
            GPA_LOG_VERBOSE( "/opt/likewise/bin/domainjoin-cli configure --enable reapsysld command returned: %s", pszBuf);
        }
    }

    /* send HUP to get syslog to check for syslog.conf */
    if ( bNeedHUP )
    {
        ceError = SendHUPToSyslog();
        BAIL_ON_CENTERIS_ERROR(ceError);
        bNeedHUP = FALSE;
    }

error:

    LW_SAFE_FREE_STRING(pszBuf);
    LW_SAFE_FREE_STRING(pszNewMd5Sum);
    FreeSyslogList(&pListFromAD);
    GPA_LOG_FUNCTION_LEAVE(ceError);
    return ceError;
}

CENTERROR
ResetSyslogGroupPolicy(
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS; 

    /* There is nothing to reset for this policy */

    return ceError;
}

CENTERROR
GetFileHandle(
    PSTR SyslogTempFile,
    BOOLEAN bUseNG,
    FILE**  ppHandle
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE* pHandle = NULL;
    
    ceError = GPAOpenFile( SyslogTempFile, 
                          "w", 
                          &pHandle);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAFileStreamWrite( pHandle, 
                                 LWISYSLOG_CONF_HEADER, 
                                 strlen(LWISYSLOG_CONF_HEADER));
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( bUseNG )
    {
        //Write default options and source
        WriteNGOptions(pHandle);
        WriteNGDefSrc(pHandle);
    }
    else
    {
        //Rsyslog needs imuxsock.so and imklog.so modules to provide support for local sytem logging and kernel logging
        //and also a flag to specifiy time stamp format.
        if ( IsRsyslogSupported() )
        {
            ceError = GPAFileStreamWrite( pHandle,  
                                         LWIRSYSLOG_CONF_MODULE_HEADER, 
                                         strlen(LWIRSYSLOG_CONF_MODULE_HEADER));
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    *ppHandle = pHandle;
    return ceError;

error:
    if ( pHandle )
    {
        GPACloseFile(pHandle);
        pHandle = NULL;
        *ppHandle = NULL;
    }

    return ceError;
}
