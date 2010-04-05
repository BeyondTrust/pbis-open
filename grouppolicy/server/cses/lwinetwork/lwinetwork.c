#include "includes.h"

static
CENTERROR
PerformPostApplyPolicyTasks()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

#ifdef __LWI_DARWIN__
    goto done;
#else
    ceError = GPARunCommand("/etc/init.d/network restart");
    if (ceError != CENTERROR_SUCCESS) {
        ceError = GPARunCommand("/etc/init.d/networking restart");
        if (ceError != CENTERROR_SUCCESS) {
            GPA_LOG_ALWAYS( "Failed to restart network service...: Error code [%d]", ceError);
            ceError = CENTERROR_SUCCESS;
            goto done;
        }
    }
#endif

done:

    return ceError;
}

static
CENTERROR
ApplyPolicy()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PDNSSETTINGS pDNSSettings = g_TargetDNSSettings;
    FILE *fp = NULL;

    ceError = GPAOpenFile( NETWORK_CONF_FILE_GP,
                          "w",
                          &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while (pDNSSettings) {
        if ( !strcmp( pDNSSettings->pszDNSSettingName,
                      "search") ||
              !strcmp( pDNSSettings->pszDNSSettingName,
                      "SearchDomains")) {
            ceError = GPAFilePrintf( fp,
                                    "search %s\n",
                                    pDNSSettings->pszDNSSettingValue);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        pDNSSettings = (PDNSSETTINGS)pDNSSettings->pNext;
    }

    pDNSSettings = g_TargetDNSSettings;

    while (pDNSSettings) {
        if ( !strcmp( pDNSSettings->pszDNSSettingName,
                      "nameserver") ||
              !strcmp( pDNSSettings->pszDNSSettingName,
                      "ServerAddresses") ) {
            ceError = GPAFilePrintf( fp,
                                    "nameserver %s\n",
                                    pDNSSettings->pszDNSSettingValue);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        pDNSSettings = (PDNSSETTINGS)pDNSSettings->pNext;
    }

error:

    if (fp) {
        GPACloseFile(fp);
        fp = NULL;
    }

    return ceError;
}

static
BOOLEAN
FoundEntry(
    PSTR pszName,
    PSTR pszValue,
    PDNSSETTINGS pDNSSettings
    )
{

   if(pDNSSettings) { 

       while (pDNSSettings) {
           if (!strcmp( pDNSSettings->pszDNSSettingValue,
                         pszValue) &&
                !strcmp( pDNSSettings->pszDNSSettingName,
                         pszName)) {
                return TRUE;
            }
            pDNSSettings = pDNSSettings->pNext;
        }
    }
    
    return FALSE;
}

static
CENTERROR
CacheLocalSettingsValues(
    PSTR pszName,
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PDNSSETTINGS pDNSSettings = NULL;

    if (FoundEntry( pszName, 
                     pszValue, 
                     g_TargetDNSSettings)) {
        goto done;
    }

    ceError = LwAllocateMemory( sizeof(DNSSETTINGS),
                                (PVOID *)&pDNSSettings);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszName,
                                &pDNSSettings->pszDNSSettingName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszValue,
                                &pDNSSettings->pszDNSSettingValue);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pDNSSettings->pNext = NULL;

    if (g_TargetDNSSettings) {
        PDNSSETTINGS pTemp = g_TargetDNSSettings;
        while ( pTemp->pNext ) {
            pTemp = (PDNSSETTINGS)pTemp->pNext;
        }
        pTemp->pNext = (PDNSSETTINGS)pDNSSettings;
    } else {
        g_TargetDNSSettings = pDNSSettings;
    }

    return ceError;

done:

    return ceError;

error:

    if (pDNSSettings) {
        LW_SAFE_FREE_STRING(pDNSSettings->pszDNSSettingName);
        LW_SAFE_FREE_STRING(pDNSSettings->pszDNSSettingValue);
        LwFreeMemory(pDNSSettings);
        pDNSSettings = NULL;
    }

    goto done;
}

static
CENTERROR
MergeLocalAndGPSettings()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PNETWORKSETTINGS pNetworkTemp = g_PolicyHierarchyList;
    PDNSSETTINGS pTargetDNSSettings = g_TargetDNSSettings;
    PDNSSETTINGS pGPDNSSettings = NULL;
    BOOLEAN bMatchFound = FALSE;

    while ( pNetworkTemp && pNetworkTemp->pNext ) {
        pNetworkTemp = pNetworkTemp->pNext;
    }

    if ( pNetworkTemp ) {
        pGPDNSSettings = pNetworkTemp->pDNSSettings;
        if ( pTargetDNSSettings ) {
            while ( pGPDNSSettings ) {
                bMatchFound = FALSE;
                pTargetDNSSettings = g_TargetDNSSettings;
                while ( pTargetDNSSettings ) {
                    if ( !strcmp( pTargetDNSSettings->pszDNSSettingValue,
                                  pGPDNSSettings->pszDNSSettingValue)) {
                        bMatchFound = TRUE;
                        break;
                    }
                    pTargetDNSSettings = (PDNSSETTINGS)pTargetDNSSettings->pNext;
                }

                if (!bMatchFound) {
                    ceError = CacheLocalSettingsValues( pGPDNSSettings->pszDNSSettingName,
                                                        pGPDNSSettings->pszDNSSettingValue);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

                pGPDNSSettings = (PDNSSETTINGS)pGPDNSSettings->pNext;
            }
        }
    } else {
        /* When user undefines all policies, we need to restore back the old settings*/
        ceError = ResetNetworkSettingsGroupPolicy();
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return ceError;
}

static
CENTERROR
ReadResolvConf()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE *fp = NULL;
    CHAR szLine[STATIC_PATH_BUFFER_SIZE];
    PSTR pszName = NULL;
    PSTR pszValue = NULL;
    PSTR pszBuffer = NULL;

    ceError = GPAOpenFile( NETWORK_CONF_FILE,
                          "r",
                          &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while (fgets(szLine, STATIC_PATH_BUFFER_SIZE, fp) != NULL) {

        LwStripWhitespace(szLine,1,1);

        if( strcspn(szLine,"#") != 0 ) {

            pszName = (PSTR)strtok_r( szLine, " ", &pszBuffer);
            pszValue = (PSTR)strtok_r( NULL, " ", &pszBuffer);

            ceError = CacheLocalSettingsValues(pszName, pszValue);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        
    }

error:

    GPACloseFile(fp);

    return ceError;
}

static
void
FreeLocalDNSSettings()
{
    PDNSSETTINGS pTemp = g_TargetDNSSettings; 

    while ( pTemp ) {
        LW_SAFE_FREE_STRING(pTemp->pszDNSSettingName);
        LW_SAFE_FREE_STRING(pTemp->pszDNSSettingValue);
        LwFreeMemory(pTemp);
        pTemp = NULL;
    }

    g_TargetDNSSettings = NULL;
}

static
void
FreeNetworkSettings()
{
    PNETWORKSETTINGS pTemp = g_PolicyHierarchyList;

    while (g_PolicyHierarchyList) {
        pTemp = g_PolicyHierarchyList;
        g_PolicyHierarchyList = g_PolicyHierarchyList->pNext;

        if ( pTemp->pDNSSettings ) {
            PDNSSETTINGS pDNSTemp = pTemp->pDNSSettings;
            PDNSSETTINGS pDNSPrev = NULL;
            while (pDNSTemp) {
                pDNSPrev =  pDNSTemp;
                LW_SAFE_FREE_STRING(pDNSPrev->pszDNSSettingName);
                LW_SAFE_FREE_STRING(pDNSPrev->pszDNSSettingValue);
                LwFreeMemory(pDNSPrev);
                pDNSPrev = NULL;
                pDNSTemp = (PDNSSETTINGS)pDNSTemp->pNext;
            }
            LwFreeMemory(pTemp);
            pTemp = NULL;
        }
    }
    g_PolicyHierarchyList = NULL;
}

CENTERROR
ResetNetworkSettingsGroupPolicy()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOG_VERBOSE( "Resetting Network Group policy settings.");

    FreeNetworkSettings();

    FreeLocalDNSSettings();

    return ceError;
}

CENTERROR
ResetNetworkSettings()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;

    ceError = GPACheckFileExists( LWI_NETWORK_OLD_FILE,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
        ceError = GPAMoveFileAcrossDevices( LWI_NETWORK_OLD_FILE,
                                           NETWORK_CONF_FILE);
        BAIL_ON_CENTERIS_ERROR(ceError);
        ceError = PerformPostApplyPolicyTasks();
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    FreeNetworkSettings();
    FreeLocalDNSSettings();
error:

    return ceError;
}

static
CENTERROR
GetLWISettingsXMLFile(
    PGPOLWIDATA *ppLwidata,
    PSTR pszgPCFileSysPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = GPOInitLwiData( NULL,
                              MACHINE_GROUP_POLICY,
                              ppLwidata,
                              pszgPCFileSysPath,
                              NULL,
                              LWI_NETWORK_MACHINE_CLIENT_GUID);
    if ( ceError ) {
        ceError = GPOInitLwiData( NULL,
                                  MACHINE_GROUP_POLICY,
                                  ppLwidata,
                                  pszgPCFileSysPath,
                                  NULL,
                                  NULL );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return ceError;
}

static
CENTERROR
CacheADSettings(
    PSTR pszDisplayName,
    PSTR pszName,
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PDNSSETTINGS pDNSSettings = NULL;
    BOOLEAN bMatchFound = 0;

    ceError = LwAllocateMemory( sizeof(DNSSETTINGS),
                                (PVOID *)&pDNSSettings);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszName,
                                &pDNSSettings->pszDNSSettingName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszValue,
                                &pDNSSettings->pszDNSSettingValue);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pDNSSettings->pNext = NULL;

    if ( g_PolicyHierarchyList ) {
        PNETWORKSETTINGS pTemp = g_PolicyHierarchyList;
        while ( pTemp ) {
            if (!strcmp( pTemp->pszDisplayName,
                         pszDisplayName)) {
                bMatchFound = 1;
                break;
            }
            pTemp = pTemp->pNext;
        }
        if (bMatchFound) {
            PDNSSETTINGS pTempDNSSettings = pTemp->pDNSSettings;
            while ( pTempDNSSettings->pNext ) {
                pTempDNSSettings = (PDNSSETTINGS)pTempDNSSettings->pNext;    
            }
            pTempDNSSettings->pNext = (PDNSSETTINGS)pDNSSettings;
        } else {
            PNETWORKSETTINGS pTempNetworkSettings = NULL;
            pTemp = g_PolicyHierarchyList;
            while ( pTemp->pNext ) {
                pTemp = pTemp->pNext;
            }
            ceError = LwAllocateMemory( sizeof(NETWORKSETTINGS),
                                        (PVOID *)&pTempNetworkSettings);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = LwAllocateString( pszDisplayName,
                                       &pTempNetworkSettings->pszDisplayName);
            BAIL_ON_CENTERIS_ERROR(ceError);
            
            pTempNetworkSettings->pDNSSettings = pDNSSettings;
            pTempNetworkSettings->pNext = NULL;
            pTemp->pNext = pTempNetworkSettings;
        }
    } else {
        PNETWORKSETTINGS pTempNetworkSettings = NULL;

        ceError = LwAllocateMemory( sizeof(NETWORKSETTINGS),
                                    (PVOID *)&pTempNetworkSettings);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwAllocateString( pszDisplayName,
                                    &pTempNetworkSettings->pszDisplayName);
        BAIL_ON_CENTERIS_ERROR(ceError);
            
        pTempNetworkSettings->pDNSSettings = pDNSSettings;
        pTempNetworkSettings->pNext = NULL;
 
        g_PolicyHierarchyList = pTempNetworkSettings;
    }

error:

    return ceError;
}

static
CENTERROR
BuildADSettingsList(
    PSTR pszName,
    PSTR pszValue,
    PSTR pszDisplayName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szValue[STATIC_PATH_BUFFER_SIZE];
    PSTR pszBuffer = NULL;
    PSTR pszToken = NULL;
    
    sprintf( (PSTR)szValue,
             "%s",
             pszValue);
    
    pszToken = strtok_r( szValue, 
                         ",", 
                         &pszBuffer);

    while ( pszToken ) {
        ceError = CacheADSettings( pszDisplayName,
                                   pszName,
                                   pszToken);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pszToken = strtok_r( NULL, 
                             ",", 
                             &pszBuffer);
    }

error:

    return ceError;
}

static
CENTERROR
ParseNetworkSettingsGPItem(
    xmlNodePtr root_node,
    PSTR pszDisplayName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cur_node = NULL;
    xmlChar *nameAttrValue = NULL;

    for (cur_node = root_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->name && !strcmp( (char*)cur_node->name,
                                       (char*)"ServerAddresses")) {
            nameAttrValue = xmlNodeGetContent(cur_node);
                    
            if (nameAttrValue) {
                ceError = BuildADSettingsList( "ServerAddresses", 
                                               (PSTR)nameAttrValue,
                                               pszDisplayName);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        } 

        if (cur_node->name && !strcmp( (char*)cur_node->name,
                                       (char*)"SearchDomains")) {
            nameAttrValue = xmlNodeGetContent(cur_node);
                    
            if (nameAttrValue) {
                ceError = BuildADSettingsList( "SearchDomains", 
                                               (PSTR)nameAttrValue,
                                               pszDisplayName);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        } 

        if ( nameAttrValue ) {
            xmlFree( nameAttrValue );
            nameAttrValue = NULL;
        }
        
        ceError = ParseNetworkSettingsGPItem( cur_node->children,
                                              pszDisplayName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
cleanup:

    if ( nameAttrValue ) {
        xmlFree( nameAttrValue );
        nameAttrValue = NULL;
    }

    return ceError;
    
error:

    goto cleanup;
}

static
CENTERROR
ParseLWISettingsXMLFile(
    PGPOLWIDATA pLwidata,
    PSTR pszDomainName,
    PSTR pszPolicyIdentifier,
    PSTR pszDisplayName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIGPITEM pGPItem = NULL;

    ceError = GPOGetGPItem( MACHINE_GROUP_POLICY,
                            pLwidata,
                            LWI_NETWORK_MACHINE_ITEM_GUID,
                            &pGPItem );
    if (ceError != CENTERROR_SUCCESS) {
        GPA_LOG_ALWAYS( "Network settings Group policy gpitem not found for: Domain Name [%s] Policy [%s]",
                        pszDomainName,
                        pszPolicyIdentifier);
        GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    ceError = ParseNetworkSettingsGPItem( pGPItem->xmlNode, 
                                          pszDisplayName);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if ( pGPItem ) {
        GPODestroyGPItem( pGPItem, 
                          FALSE);
    }

    return ceError;
}

static
CENTERROR
ProcessSettingsXMLFile(
    PSTR pszDomainName,
    PSTR pszSourcePath,
    PSTR pszPolicyIdentifier,
    PSTR pszgPCFileSysPath,
    PSTR pszDisplayName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIDATA pLwidata = NULL;
    
    ceError = GetLWISettingsXMLFile( &pLwidata,
                                     pszgPCFileSysPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ParseLWISettingsXMLFile( pLwidata,
                                       pszDomainName,
                                       pszPolicyIdentifier,
                                       pszDisplayName);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if ( pLwidata ) {
        GPODestroyLwiData(pLwidata);
    }

    return ceError;
}

static
CENTERROR
AddNetworkSettingsGroupPolicy(
    PGROUP_POLICY_OBJECT pGPOList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;

    ceError = GPACrackFileSysPath( pGPOList->pszgPCFileSysPath,
                                   &pszDomainName,
                                   &pszSourcePath,
                                   &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ProcessSettingsXMLFile( pszDomainName,
                                      pszSourcePath,
                                      pszPolicyIdentifier,
                                      pGPOList->pszgPCFileSysPath,
                                      pGPOList->pszDisplayName);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    return ceError;
}

static
CENTERROR
BackupSystemNetworkConf()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;

    ceError = GPACheckFileExists( LWI_NETWORK_OLD_FILE,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bFileExists) {
        ceError = GPACheckFileExists( NETWORK_CONF_FILE,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);
    
        if ( bFileExists ) {
            GPA_LOG_ALWAYS( "Backing up original network configuration file: Source path [%s] Dest path: [%s]", 
                            (PSTR)NETWORK_CONF_FILE, 
                            (PSTR)LWI_NETWORK_OLD_FILE);

            ceError = GPACopyFileWithOriginalPerms ( NETWORK_CONF_FILE,
                                                    LWI_NETWORK_OLD_FILE);
            BAIL_ON_CENTERIS_ERROR(ceError);   
        }
        else if ( !bFileExists ) {
            GPA_LOG_VERBOSE("resolv.conf system file not found ....");
        }
    }

error:

    return ceError;
}

static
CENTERROR
ApplyNetworkGroupPolicy()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = BackupSystemNetworkConf();
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = ApplyPolicy();
    BAIL_ON_CENTERIS_ERROR(ceError);

    GPA_LOG_VERBOSE("Applying group policy version of network settings to: [%s]", NETWORK_CONF_FILE);

    ceError = GPAMoveFileAcrossDevices( NETWORK_CONF_FILE_GP,
                                       NETWORK_CONF_FILE);
    BAIL_ON_CENTERIS_ERROR(ceError);

    FreeLocalDNSSettings();

error:

    return ceError;
}

static
CENTERROR
RemovePolicy(
    PGROUP_POLICY_OBJECT pGPOList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PNETWORKSETTINGS pTemp = g_PolicyHierarchyList;
    PNETWORKSETTINGS pPrev = NULL;

    while(pTemp) {
        if(!strcmp( pGPOList->pszDisplayName,
                    pTemp->pszDisplayName) &&
            pTemp->pDNSSettings ) {
            if ( pTemp == g_PolicyHierarchyList ) {
                g_PolicyHierarchyList = g_PolicyHierarchyList->pNext;
                LW_SAFE_FREE_STRING(pTemp->pDNSSettings->pszDNSSettingName);
                LW_SAFE_FREE_STRING(pTemp->pDNSSettings->pszDNSSettingValue);
                LwFreeMemory(pTemp->pDNSSettings);
                pTemp->pDNSSettings = NULL;
                LwFreeMemory(pTemp);
                pTemp = NULL;
            } else {
                PDNSSETTINGS pDNSTemp = pTemp->pDNSSettings;
                PDNSSETTINGS pDNSPrev = NULL;
                pPrev->pNext = pTemp->pNext;
                while (pDNSTemp) {
                    pDNSPrev =  pDNSTemp;
                    LW_SAFE_FREE_STRING(pDNSPrev->pszDNSSettingName);
                    LW_SAFE_FREE_STRING(pDNSPrev->pszDNSSettingValue);
                    LwFreeMemory(pDNSPrev);
                    pDNSPrev = NULL;
                    pDNSTemp = pDNSTemp->pNext;
                } 
                LwFreeMemory(pTemp);
                pTemp = NULL;
            }
        } else {
            pPrev = pTemp;
            pTemp = pTemp->pNext;
        }
    }

    return ceError;
}

static
CENTERROR
ApplyNetworkGPSettings()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = ReadResolvConf();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = MergeLocalAndGPSettings();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ApplyNetworkGroupPolicy();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = PerformPostApplyPolicyTasks();
    BAIL_ON_CENTERIS_ERROR(ceError); 

error:

    return ceError; 
}

CENTERROR
ProcessNetworkSettingsGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bApplicable = FALSE;

    GPA_LOG_FUNCTION_ENTER();

    //Reset the in memory policy list
    FreeNetworkSettings();
    FreeLocalDNSSettings();

    while (pGPODeletedList) {
        ceError = RemovePolicy(pGPODeletedList);
        BAIL_ON_CENTERIS_ERROR(ceError);
        pGPODeletedList = pGPODeletedList->pNext;
    }

    while ( pGPOModifiedList ) {
        bApplicable = FALSE;
        ceError = GPOXmlVerifyPlatformApplicable( NULL,
                                                  MACHINE_GROUP_POLICY,
                                                  pGPOModifiedList,
                                                  NULL,
                                                  &bApplicable);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        if (!bApplicable) {
            GPA_LOG_VERBOSE( "GPO(%s) disabled by platform targeting",
                             pGPOModifiedList->pszDisplayName);
            ceError = RemovePolicy(pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE( "GPO(%s) is disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);
            ceError = RemovePolicy(pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {

            ceError = AddNetworkSettingsGroupPolicy(pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        pGPOModifiedList = pGPOModifiedList->pNext;
    }

    if (g_PolicyHierarchyList) {
        ceError = ApplyNetworkGPSettings();
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else {
        ceError = ResetNetworkSettings();
        BAIL_ON_CENTERIS_ERROR(ceError);
    }


error:

    GPA_LOG_FUNCTION_LEAVE(ceError);

    return ceError;
}
