#include "includes.h"

//Function prototype
static
CENTERROR
UpdatePolicyInRegistry(
    HANDLE hConnection,
    HKEY   hKey,
    PGPAGENT_POLICY_SETTINGS pgpPolicySettings
    );

static
CENTERROR
MapAndAddSettings(
    PGPAGENT_POLICY_SETTINGS  *ppgpPolicySettings, 
    struct _xmlAttr *pAttrNode,
    xmlNodePtr cur_node
    );

static
CENTERROR
ReadXMLTree(
    PGPAGENT_POLICY_SETTINGS  pgpPolicySettings, 
    xmlNodePtr root_node
    );

static
VOID
InitGPStruct(
    PGPAGENT_POLICY_SETTINGS pgpPolicySettings
    );

static
CENTERROR
UpdateRegistryWithPolicySettings(
    PGPOLWIGPITEM pGPItem
    );


CENTERROR
ApplyLwiGPSettingsPolicy(
    PGPOLWIGPITEM *ppRsopGPSettingsItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if(*ppRsopGPSettingsItem)
    {
        ceError = UpdateRegistryWithPolicySettings( *ppRsopGPSettingsItem );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return ceError;
}

static
CENTERROR
UpdateRegistryWithPolicySettings(
    PGPOLWIGPITEM pGPItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    GPAGENT_POLICY_SETTINGS  gpPolicySettings; 
    HANDLE hReg = (HANDLE)NULL;
    HKEY hKey = NULL;

    //set default flag to true
    InitGPStruct(&gpPolicySettings);

    //If policy is set then read from xml. Else delete the keyvaluename
    if (pGPItem)
    {
        ceError = ReadXMLTree( 
                    &gpPolicySettings, 
                    (xmlNodePtr)pGPItem->xmlNode);

        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPGetRegOpenKey(
                "Policy\\Services\\gpagent\\Parameters",
                &hReg,
                &hKey);
    if (ceError)
    {
        GPA_LOG_ERROR( "Unable to open registry root key");
        ceError = 0;
        goto error;
    }

    ceError = UpdatePolicyInRegistry( 
                hReg,
                hKey,
                &gpPolicySettings);
                      
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if(hReg && hKey)
    {
        RegCloseKey(
            hReg,
            hKey);
    }

    RegCloseServer(hReg);

    return ceError;
}

static
VOID
InitGPStruct(
    PGPAGENT_POLICY_SETTINGS pgpPolicySettings
    )
{
    //set default flag to true
    pgpPolicySettings->bUseDefaultComputerPolicyRefreshInterval  = TRUE;
    pgpPolicySettings->bUseDefaultUserPolicyRefreshInterval      = TRUE;
    pgpPolicySettings->bUseDefaultUserLoopbackMode               = TRUE;
    pgpPolicySettings->bUseDefaultMonitorSudoers                 = TRUE;
    pgpPolicySettings->bUseDefaultEnableEventLog                 = TRUE;
}

static
CENTERROR
ReadXMLTree(
    PGPAGENT_POLICY_SETTINGS  pgpPolicySettings, 
    xmlNodePtr root_node
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cur_node = NULL;
    xmlChar *pszNodeText = NULL;

    struct _xmlAttr *pAttrNode = NULL;

    for (cur_node = root_node; cur_node != NULL ; cur_node = cur_node->next) {

        pszNodeText = NULL;

        if (cur_node->name) {
            if ( !strcmp( (const char*)cur_node->name,
                          (const char*)"setting")) {

                for ( pAttrNode = cur_node->properties; pAttrNode; pAttrNode = pAttrNode->next ) {
                    if ( !strcmp( (const char*)pAttrNode->name,
                                  (const char*)"name")) {
                        break;
                    }                    
                }
                ceError = MapAndAddSettings( &pgpPolicySettings,
                                             pAttrNode,
                                             cur_node);
                BAIL_ON_CENTERIS_ERROR(ceError);

            }

        }

        ReadXMLTree( pgpPolicySettings,
                     cur_node->children);
    }

    return ceError;

error:

    return ceError;
}

static
CENTERROR
MapAndAddSettings(
    PGPAGENT_POLICY_SETTINGS  *ppgpPolicySettings, 
    struct _xmlAttr *pAttrNode,
    xmlNodePtr cur_node
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlChar *pszNodeText = NULL;

    if ( !strcmp( (const char*)pAttrNode->children->content,
                  (const char*)"ComputerPolicyRefreshInterval")) {

        ceError = get_node_text( cur_node,
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );

        (*ppgpPolicySettings)->bUseDefaultComputerPolicyRefreshInterval = FALSE;
        (*ppgpPolicySettings)->dwComputerPolicyRefreshInterval = (DWORD)atoi((const char*)pszNodeText);
    }
    else if ( !strcmp( (const char*)pAttrNode->children->content,
                       (const char*)"UserPolicyRefreshInterval")) {
        ceError = get_node_text( cur_node,
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );

        (*ppgpPolicySettings)->bUseDefaultUserPolicyRefreshInterval = FALSE;
        (*ppgpPolicySettings)->dwUserPolicyRefreshInterval = (DWORD)atoi((const char*)pszNodeText);

    }
    else if ( !strcmp( (const char*)pAttrNode->children->content,
                       (const char*)"UserPolicyLoopbackProcessingMode")) {
        ceError = get_node_text( cur_node,
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );

        (*ppgpPolicySettings)->bUseDefaultUserLoopbackMode = FALSE;

        if( !strcmp((const char*)pszNodeText, (const char*)"useronly"))
            (*ppgpPolicySettings)->uplm = 0;
        else if( !strcmp((const char*)pszNodeText, (const char*)"replace"))
            (*ppgpPolicySettings)->uplm = 1;
        else if( !strcmp((const char*)pszNodeText, (const char*)"merge"))
            (*ppgpPolicySettings)->uplm = 2;

    }
    else if ( !strcmp( (const char*)pAttrNode->children->content,
                       (const char*)"MonitorSudoers")) {
        ceError = get_node_text( cur_node,
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );

        (*ppgpPolicySettings)->bUseDefaultMonitorSudoers = FALSE;

        if( !strcmp((const char*)pszNodeText, (const char*)"true"))
            (*ppgpPolicySettings)->bMonitorSudoers = TRUE;
        else
            (*ppgpPolicySettings)->bMonitorSudoers = FALSE;


    }
    else if ( !strcmp( (const char*)pAttrNode->children->content,
                       (const char*)"EnableEventLog")) {
        ceError = get_node_text( cur_node,
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );

        (*ppgpPolicySettings)->bUseDefaultEnableEventLog = FALSE;

        if( !strcmp((const char*)pszNodeText, (const char*)"true"))
            (*ppgpPolicySettings)->bEnableEventLog = TRUE;
        else
            (*ppgpPolicySettings)->bEnableEventLog = FALSE;

    }

error:

    if ( pszNodeText ) {
        xmlFree( pszNodeText );
        pszNodeText = NULL;
    }

    return ceError;
}

static
CENTERROR
UpdatePolicyInRegistry(
    HANDLE hReg,
    HKEY   hKey,
    PGPAGENT_POLICY_SETTINGS pgpPolicySettings
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = UpdateDwordValueInRegistry(
                     hReg,
                     hKey,
                     pgpPolicySettings->bUseDefaultComputerPolicyRefreshInterval,
                     "ComputerPolicyRefreshInterval",
                     pgpPolicySettings->dwComputerPolicyRefreshInterval);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateDwordValueInRegistry(
                     hReg,
                     hKey,
                     pgpPolicySettings->bUseDefaultUserPolicyRefreshInterval,
                     "UserPolicyRefreshInterval",
                     pgpPolicySettings->dwUserPolicyRefreshInterval);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateDwordValueInRegistry(
                     hReg,
                     hKey,
                     pgpPolicySettings->bUseDefaultUserLoopbackMode,
                     "UserPolicyLoopbackProcessingMode",
                     pgpPolicySettings->uplm);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pgpPolicySettings->bUseDefaultMonitorSudoers,
                     "MonitorSudoers",
                     pgpPolicySettings->bMonitorSudoers);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pgpPolicySettings->bUseDefaultEnableEventLog,
                     "EnableEventLog",
                     pgpPolicySettings->bEnableEventLog);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}
