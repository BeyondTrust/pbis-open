#include "includes.h"

static
CENTERROR
CheckIPTablesSupport(
    PBOOLEAN pbValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bIPTablesExists = FALSE;

    ceError = GPACheckFileExists( IPTABLES_INIT_FILE,
                                 &bIPTablesExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *pbValue = bIPTablesExists;

error:

    return ceError;
}

static
CENTERROR
PerformPostApplyPolicyTasks()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = GPARunCommand(IPTABLES_SAVE_COMMAND);
    if (ceError != CENTERROR_SUCCESS) {
        GPA_LOG_ERROR( "Failed to save group policy version of iptables rules: Error code [%d]", ceError);
        ceError = CENTERROR_SUCCESS;
        goto done;
    }

    ceError = GPARunCommand(IPTABLES_RESTART_COMMAND);
    if (ceError != CENTERROR_SUCCESS) {
        GPA_LOG_ERROR( "Failed to restart iptables: Error code [%d]", ceError);
        ceError = CENTERROR_SUCCESS;
        goto done;
    }

done:

    return ceError;
}

static
CENTERROR
RemoveCacheFile(
    PIPTABLESLIST pNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szFilePath[STATIC_PATH_BUFFER_SIZE];

    sprintf( szFilePath,
             "%s%s/%s",
             CENTERIS_GP_DIRECTORY,
             pNode->pszGUID,
             LWIIPTABLES_MACHINE_CLIENT_GUID);

    ceError = GPARemoveDirectory(szFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
ApplyOrRestorePolicy(
    PSTR pszFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szCommand[STATIC_PATH_BUFFER_SIZE];
    BOOLEAN bFileExists = FALSE;

    ceError = GPACheckFileExists( pszFilePath,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
        sprintf( szCommand,
                 "iptables-restore < %s",
                 pszFilePath);

        ceError = GPARunCommand(szCommand);
        if (ceError != CENTERROR_SUCCESS) {
            GPA_LOG_ERROR( "Failed to process iptables rules: [%s] Error code: [%d]", (PSTR)szCommand, ceError);
            ceError = CENTERROR_SUCCESS;
            goto done;
        }
    }

done:
error:

    return ceError;
}

CENTERROR
ResetIPTablesGroupPolicy()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;

    ceError = CheckIPTablesSupport(&bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
        while (g_PolicyHierarchyList) {
            PIPTABLESLIST pTemp = g_PolicyHierarchyList;
            g_PolicyHierarchyList = g_PolicyHierarchyList->pNext;

            ceError = RemoveCacheFile(pTemp);
            BAIL_ON_CENTERIS_ERROR(ceError);

            LW_SAFE_FREE_STRING(pTemp->pszGUID);
            LW_SAFE_FREE_STRING(pTemp->pszFilePath);

            LwFreeMemory(pTemp);
        }

        bFileExists = FALSE;

        ceError = GPACheckFileExists( LWI_IPTABLES_BKP_FILEPATH,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            ceError = ApplyOrRestorePolicy(LWI_IPTABLES_BKP_FILEPATH);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = LwRemoveFile(LWI_IPTABLES_BKP_FILEPATH);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = PerformPostApplyPolicyTasks();
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        GPA_LOG_VERBOSE( "ResetIPTablesGroupPolicy: Resetting IPTables Grouppolicy settings done.");
    }

error:

    return ceError;
}

static
CENTERROR
PopulateEntry(
    PSTR pszDisplayName,
    PSTR pszGUID,
    PSTR pszIPtablesRules,
    PSTR pszDestFilePath,
    PIPTABLESLIST *ppNew
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = LwAllocateMemory( sizeof(IPTABLESLIST),
                                (PVOID*)&(*ppNew));
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateStringPrintf( &(*ppNew)->pszDisplayName,
                                      "%s",
                                      pszDisplayName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateStringPrintf( &(*ppNew)->pszGUID,
                                      "{%s}",
                                      pszGUID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pszIPtablesRules)  {
        ceError = LwAllocateStringPrintf( &(*ppNew)->pszRules,
                                          "%s",
                                          pszIPtablesRules);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwAllocateStringPrintf( &(*ppNew)->pszFilePath,
                                      "%s",
                                      pszDestFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
AddPolicyFileToHierarchy(
    PSTR pszDisplayName,
    PSTR pszGUID,
    PSTR pszIPtablesRules,
    PSTR pszDestFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PIPTABLESLIST pTemp = g_PolicyHierarchyList;
    PIPTABLESLIST pPrev = NULL;
    PIPTABLESLIST pNew = NULL;

    while (pTemp) {
        if (!strcmp(pTemp->pszGUID,
                    pszGUID)) {
            goto done;
        }
        pTemp = pTemp->pNext;
    }
 
    pTemp = g_PolicyHierarchyList;

    ceError = PopulateEntry( pszDisplayName,
                             pszGUID,
                             pszIPtablesRules,
                             pszDestFilePath,
                             &pNew);
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

done:
error:

    if (pNew) {
        LW_SAFE_FREE_STRING(pNew->pszDisplayName);
        LW_SAFE_FREE_STRING(pNew->pszGUID);
        LW_SAFE_FREE_STRING(pNew->pszRules);
        LW_SAFE_FREE_STRING(pNew->pszFilePath);
        LwFreeMemory(pNew);
    }

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
                              LWIIPTABLES_MACHINE_CLIENT_GUID);
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
ParseIPTablesGPItem(
    xmlNodePtr root_node,
    PSTR *ppszPolicyFileName,
    PSTR *ppszIPTablesRules
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cur_node = NULL;
    xmlChar *nameAttrValue = NULL;
    xmlChar* pszValue = NULL;
    PSTR pszPolicyFileName = NULL;
    PSTR pszIPTablesRules = NULL;
    PSTR pszPolicyFileName_child = NULL;
    PSTR pszIPTablesRules_child = NULL;

    for (cur_node = root_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->name && !strcmp( (char*)cur_node->name,
                                       (char*)"setting")) {
                nameAttrValue = xmlGetProp( cur_node,
                                            (const xmlChar*)"name");
                if (nameAttrValue && !strcmp((PCSTR)nameAttrValue, (PCSTR)"iptables file"))
                {
                	pszValue = xmlNodeGetContent(cur_node);
                	
                	LW_SAFE_FREE_STRING(pszPolicyFileName);
                	
                	if (pszValue)
                	{
                		ceError = LwAllocateString((PCSTR)pszValue, &pszPolicyFileName);
                		BAIL_ON_CENTERIS_ERROR(ceError);
                	}
                }
                else if (nameAttrValue && !strcmp((PCSTR)nameAttrValue, (PCSTR)"iptables rule"))
                {
                	pszValue = xmlNodeGetContent(cur_node);
                	                	
                	LW_SAFE_FREE_STRING(pszIPTablesRules);
                	                	
                	if (pszValue)
                	{
                	    ceError = LwAllocateString((PCSTR)pszValue, &pszIPTablesRules);
                	    BAIL_ON_CENTERIS_ERROR(ceError);
                	}
                }
        }

        if ( nameAttrValue ) {
            xmlFree( nameAttrValue );
            nameAttrValue = NULL;
        }
        
        if ( pszValue ) {
            xmlFree( pszValue );
            pszValue = NULL;
        }

        ceError = ParseIPTablesGPItem(
        				cur_node->children,
                        &pszPolicyFileName_child,
                        &pszIPTablesRules_child);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        if (pszPolicyFileName_child)
        {
        	LwFreeMemory(pszPolicyFileName);
        	pszPolicyFileName = pszPolicyFileName_child;
        	pszPolicyFileName_child = NULL;
        }
        
        if (pszIPTablesRules_child)
        {
        	LwFreeMemory(pszIPTablesRules);
        	pszIPTablesRules = pszIPTablesRules_child;
        	pszIPTablesRules_child = NULL;
        }
    }
    
    *ppszPolicyFileName = pszPolicyFileName; 
    *ppszIPTablesRules = pszIPTablesRules;
    
cleanup:

    if ( nameAttrValue ) {
        xmlFree( nameAttrValue );
        nameAttrValue = NULL;
    }

    if ( pszValue ) {
        xmlFree( pszValue );
        pszValue = NULL;
    }

    return ceError;
    
error:

	LW_SAFE_FREE_STRING(pszPolicyFileName);
	LW_SAFE_FREE_STRING(pszIPTablesRules);
	LW_SAFE_FREE_STRING(pszPolicyFileName_child);
	LW_SAFE_FREE_STRING(pszIPTablesRules_child);
	
    *ppszPolicyFileName = NULL; 
    *ppszIPTablesRules = NULL;

	goto cleanup;
}

static
CENTERROR
ParseLWISettingsXMLFile(
    PGPOLWIDATA pLwidata,
    PSTR pszDomainName,
    PSTR pszPolicyIdentifier,
    PSTR *ppszPolicyFileName,
    PSTR *ppszIPTablesRules
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIGPITEM pGPItem = NULL;

    ceError = GPOGetGPItem( MACHINE_GROUP_POLICY,
                            pLwidata,
                            LWIIPTABLES_MACHINE_ITEM_GUID,
                            &pGPItem );
    if (ceError != CENTERROR_SUCCESS) {
        GPA_LOG_WARNING( "IPTables Group policy gpitem not found for: Domain Name [%s] Policy [%s]",
                        pszDomainName,
                        pszPolicyIdentifier);
        GPA_LOG_WARNING("Treating this as a disabled or stale policy.");
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    ceError = ParseIPTablesGPItem( pGPItem->xmlNode,
                                   ppszPolicyFileName,
                                   ppszIPTablesRules);
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
GetIPTablesPolicyFile(
    PSTR pszDomainName,
    PSTR pszSourcePath,
    PSTR pszPolicyIdentifier,
    PSTR pszPolicyFileName,
    PSTR *ppszDestFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDC = NULL;
    CHAR szSourceFilePath[STATIC_PATH_BUFFER_SIZE];
    CHAR szDestFilePath[STATIC_PATH_BUFFER_SIZE];

    ceError = GPAGetPreferredDomainController(pszDomainName, &pszDC);
    if (!CENTERROR_IS_OK(ceError)) {
        if ( CENTERROR_EQUAL(ceError, CENTERROR_GP_NO_SMB_KRB5_SITE_KDC_INFO)    ||
             CENTERROR_EQUAL(ceError, CENTERROR_GP_NO_SMB_KRB5_SITE_INFO) ) {
            GPA_LOG_WARNING( "GPAgent unable to obtain preferred server for AD site: FQDN(%s)",
                            pszDomainName);
           ceError = CENTERROR_SUCCESS;
       } else {
           BAIL_ON_CENTERIS_ERROR(ceError);
       }
    }

    sprintf( szSourceFilePath,
             "%s%s%s\\%s",
             pszSourcePath,
             CENTERIS_SYSVOL_PATH,
             LWIIPTABLES_MACHINE_CLIENT_GUID,
             pszPolicyFileName);

    sprintf( szDestFilePath,
             "%s{%s}/%s/%s",
             CENTERIS_GP_DIRECTORY,
             pszPolicyIdentifier,
             LWIIPTABLES_MACHINE_CLIENT_GUID,
             pszPolicyFileName);

    ceError = LwAllocateStringPrintf( ppszDestFilePath,
                                      "%s",
                                      szDestFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOLwioCopyFile(NULL,
                             pszDomainName,
                             pszDC,
                             szSourceFilePath,
                             szDestFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    LW_SAFE_FREE_STRING(pszDC);

    return ceError;
}

static
CENTERROR
ApplyIPTablesRulesPolicy(
    PSTR pszRules
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszTkn = NULL;
    PSTR pszLasts = NULL;

    pszTkn = (PSTR) strtok_r(pszRules,"\n",&pszLasts);
    while(pszTkn) {

        GPA_LOG_INFO("Applying iptables rules: %s", pszTkn);

        ceError = GPARunCommand(pszTkn);
        if (ceError != CENTERROR_SUCCESS) {
            GPA_LOG_ERROR( "Failed to apply group policy version of iptables rules: Error code [%d]", ceError);
            ceError = CENTERROR_SUCCESS;
        }

        pszTkn = (PSTR)strtok_r(NULL,"\n",&pszLasts);

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
    PSTR *ppszDestFilePath,
    PSTR *ppszIPtablesRules
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIDATA pLwidata = NULL;
    PSTR pszPolicyFileName = NULL;
    
    ceError = GetLWISettingsXMLFile( &pLwidata,
                                     pszgPCFileSysPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ParseLWISettingsXMLFile( pLwidata,
                                       pszDomainName,
                                       pszPolicyIdentifier,
                                       &pszPolicyFileName,
                                       ppszIPtablesRules);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pszPolicyFileName) {
        ceError = GetIPTablesPolicyFile( pszDomainName,
                                         pszSourcePath,
                                         pszPolicyIdentifier,
                                         pszPolicyFileName,
                                         ppszDestFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if ( pLwidata ) {
        GPODestroyLwiData(pLwidata);
    }

    LW_SAFE_FREE_STRING(pszPolicyFileName);

    return ceError;
}

static
CENTERROR
AddIPTablesGroupPolicy(
    PGROUP_POLICY_OBJECT pGPOList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    PSTR pszDestFilePath = NULL;
    PSTR pszIPtablesRules = NULL;

    ceError = GPACrackFileSysPath( pGPOList->pszgPCFileSysPath,
                                   &pszDomainName,
                                   &pszSourcePath,
                                   &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ProcessSettingsXMLFile( pszDomainName,
                                      pszSourcePath,
                                      pszPolicyIdentifier,
                                      pGPOList->pszgPCFileSysPath,
                                      &pszDestFilePath,
                                      &pszIPtablesRules);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = AddPolicyFileToHierarchy( pGPOList->pszDisplayName,
                                        pszPolicyIdentifier,
                                        pszIPtablesRules,
                                        pszDestFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);
    LW_SAFE_FREE_STRING(pszDestFilePath);
    LW_SAFE_FREE_STRING(pszIPtablesRules);

    return ceError;
}

static
CENTERROR
BackupSystemIPTablesConf()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;
    CHAR szCommand[STATIC_PATH_BUFFER_SIZE];

    ceError = GPACheckFileExists( LWI_IPTABLES_BKP_FILEPATH,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bFileExists) {
        sprintf( szCommand,
                 "iptables-save > %s",
                 LWI_IPTABLES_BKP_FILEPATH);

        ceError = GPARunCommand(szCommand);
        if (ceError != CENTERROR_SUCCESS) {
            GPA_LOG_ERROR( "Failed to backup system iptables conf: [%s] Error code: [%d]", (PSTR)szCommand, ceError);
            ceError = CENTERROR_SUCCESS;
        }
    }

error:

    return ceError;
}

static
CENTERROR
ApplyIPTablesGroupPolicy()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PIPTABLESLIST pPolicyList = g_PolicyHierarchyList;

    while (pPolicyList && pPolicyList->pNext) {
        pPolicyList = pPolicyList->pNext;
    }

    if (pPolicyList) {
        ceError = BackupSystemIPTablesConf();
        BAIL_ON_CENTERIS_ERROR(ceError);
    
        GPA_LOG_VERBOSE("Applying group policy version of IPTables settings: [%s]", pPolicyList->pszFilePath);

        ceError = ApplyOrRestorePolicy(pPolicyList->pszFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (pPolicyList->pszRules) { 
            ceError = ApplyIPTablesRulesPolicy(pPolicyList->pszRules);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    } else {
        /* When user undefines all policies, we need to restore back the old settings*/
        ceError = ResetIPTablesGroupPolicy();
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

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
    PIPTABLESLIST pTemp = g_PolicyHierarchyList;
    PIPTABLESLIST pPrev = NULL;

    while(pTemp) {
        if(!strcmp( pGPOList->pszDisplayName,
                    pTemp->pszDisplayName)) {
            /* If its the first node */
            if (pTemp == g_PolicyHierarchyList) {
                g_PolicyHierarchyList = g_PolicyHierarchyList->pNext;
                ceError = RemoveCacheFile(pTemp);
                BAIL_ON_CENTERIS_ERROR(ceError);
                LW_SAFE_FREE_STRING(pTemp->pszGUID);
                LW_SAFE_FREE_STRING(pTemp->pszRules);
                LW_SAFE_FREE_STRING(pTemp->pszFilePath);
                LwFreeMemory(pTemp);
                pTemp = NULL;
            } else {
                pPrev->pNext = pTemp->pNext; 
                ceError = RemoveCacheFile(pTemp);
                BAIL_ON_CENTERIS_ERROR(ceError);
                LW_SAFE_FREE_STRING(pTemp->pszGUID);
                LW_SAFE_FREE_STRING(pTemp->pszRules);
                LW_SAFE_FREE_STRING(pTemp->pszFilePath);
                LwFreeMemory(pTemp);
                pTemp = NULL;
            }
        } else {
            pPrev = pTemp;
            pTemp = pTemp->pNext;
        }
    }

error:

    return ceError;
}

CENTERROR
ProcessIPTablesGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bApplicable = FALSE;
    BOOLEAN bGPModified = FALSE;

    GPA_LOG_FUNCTION_ENTER();

    ceError = CheckIPTablesSupport(&bApplicable);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bApplicable) {
        GPA_LOG_VERBOSE("IPTables is not installed on this system, hence skipping the IPTables group policy...");
        ceError = CENTERROR_SUCCESS;
        goto done;
    }

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
            if (pGPOModifiedList->bNewVersion) {
                bGPModified = TRUE;
                ceError = AddIPTablesGroupPolicy(pGPOModifiedList);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        pGPOModifiedList = pGPOModifiedList->pNext;
    }

    if (bGPModified && g_PolicyHierarchyList) {
        ceError = ApplyIPTablesGroupPolicy();
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = PerformPostApplyPolicyTasks();
        BAIL_ON_CENTERIS_ERROR(ceError); 
    }

done:
error:

    GPA_LOG_FUNCTION_LEAVE(ceError);

    return ceError;
}

