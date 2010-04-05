#include "includes.h"

static
CENTERROR
ProcessLWEDSPluginSettingsGPItem(
    PGPOLWIGPITEM pRsopLWEDSPLUGINSettingsItem
    );

static
VOID
InitLwedStruct(
    PLWED_POLICY_SETTINGS pPolicySettings
    );

static
CENTERROR
ParseAndProcessGPItem(
    PLWED_POLICY_SETTINGS pSettings,
    xmlNodePtr root_node
    );

static
CENTERROR
MapAndAddSettings(
    PLWED_POLICY_SETTINGS *ppSettings,
    PSTR pszName,
    PSTR pszValue
    );

static
CENTERROR
UpdateLwedPolicySettings( 
    PLWED_POLICY_SETTINGS pPolicySettings
    );

static
VOID
FreeLwedStructValues(
    PLWED_POLICY_SETTINGS pPolicySettings
    );


CENTERROR
ApplyLWEDSPluginSettingsPolicy(
    PGPOLWIGPITEM pRsopLWEDSPLUGINSettingsItem 
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = ProcessLWEDSPluginSettingsGPItem(pRsopLWEDSPLUGINSettingsItem);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
error:
    
    return ceError;
}

static
CENTERROR
ProcessLWEDSPluginSettingsGPItem(
    PGPOLWIGPITEM pRsopLWEDSPLUGINSettingsItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIGPITEM pGPItem = NULL;
    LWED_POLICY_SETTINGS PolicySettings;

    /* Make a deep copy of the item to remove the sibling link */
    if (pRsopLWEDSPLUGINSettingsItem) 
    {
        ceError = GPOCopyGPItem( pRsopLWEDSPLUGINSettingsItem, 
                                 &pGPItem, 
                                 TRUE );
        BAIL_ON_CENTERIS_ERROR(ceError);


        //Initialize the structure
        InitLwedStruct(&PolicySettings);

        if(pGPItem)
        {
            /* Just save out our data directly */
            ceError = ParseAndProcessGPItem(
                            &PolicySettings,
                            pGPItem->xmlNode);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = UpdateLwedPolicySettings(&PolicySettings);
        BAIL_ON_CENTERIS_ERROR(ceError);
 
        FreeLwedStructValues(&PolicySettings);
    }

error:

    if (pGPItem)
    {
        GPODestroyGPItem( pGPItem, 
                          TRUE );
    }

    return ceError;
}

static
VOID
InitLwedStruct(
    PLWED_POLICY_SETTINGS pPolicySettings
    )
{
    pPolicySettings->bUseDefaultEnableMergeModeMCX                     = TRUE;
    pPolicySettings->bUseDefaultEnableForceHomedirOnStartupDisk        = TRUE;
    pPolicySettings->bUseDefaultUseADUncForHomeLocation                = TRUE;
    pPolicySettings->bUseDefaultUncProtocolForHomeLocation             = TRUE;
    pPolicySettings->bUseDefaultAllowAdministrationBy                  = TRUE;
    pPolicySettings->bUseDefaultEnableMergeAdmins                      = TRUE;
    pPolicySettings->pszUncProtocolForHomeLocation                     = NULL;
    pPolicySettings->pszAllowAdministrationBy                          = NULL;
}

static
CENTERROR
ParseAndProcessGPItem(
    PLWED_POLICY_SETTINGS pSettings,
    xmlNodePtr root_node
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cur_node = NULL;
    xmlChar *pszNodeText = NULL;
    xmlChar *nameAttrValue = NULL;

    for (cur_node = root_node; cur_node; cur_node = cur_node->next) 
    {
        pszNodeText = NULL;
        if (cur_node->name) 
        {
            if (strcmp( (PCSTR)cur_node->name,
                        (PCSTR)LWI_TAG_SETTING) == 0 ) 
            {
                nameAttrValue = xmlGetProp( cur_node,
                                            (const xmlChar*)LWI_ATTR_NAME );
                if (!IsNullOrEmptyString(nameAttrValue)) 
                {
                    ceError = get_node_text( cur_node,
                                             &pszNodeText );
                    BAIL_ON_CENTERIS_ERROR( ceError );

                    ceError = MapAndAddSettings( 
                                   &pSettings,
                                   (PSTR)nameAttrValue,
                                   (PSTR)pszNodeText);
                    BAIL_ON_CENTERIS_ERROR( ceError );
                }
            }

            if (pszNodeText)
            {
                xmlFree(pszNodeText);
                pszNodeText = NULL;
            }

            if (nameAttrValue)
            {
                xmlFree(nameAttrValue);
                nameAttrValue = NULL;
            }
        }

        ParseAndProcessGPItem(
             pSettings,
             cur_node->children);
    }

error:

    if ( pszNodeText ) 
    {
        xmlFree( pszNodeText );
        pszNodeText = NULL;
    }

    if ( nameAttrValue ) 
    {
        xmlFree( nameAttrValue );
        nameAttrValue = NULL;
    }

    return ceError;
}

static
CENTERROR
MapAndAddSettings(
    PLWED_POLICY_SETTINGS *ppSettings,
    PSTR pszName,
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PLWED_POLICY_SETTINGS pSettings = *ppSettings;

    if (!strcmp(pszName, MERGE_MODE_MCX)) 
    {
        pSettings->bUseDefaultEnableMergeModeMCX = FALSE; 
        pSettings->bEnableMergeModeMCX = GetBoolValue(pszValue); 
    } 
    else if (!strcmp(pszName, FORCE_HOMEDIR_ON_STARTUP_DISK)) 
    {
        pSettings->bUseDefaultEnableForceHomedirOnStartupDisk = FALSE; 
        pSettings->bEnableForceHomedirOnStartupDisk = GetBoolValue(pszValue); 
    } 
    else if (!strcmp(pszName, USE_AD_UNC_PATH))
    {
        if (!strcmp(pszValue, "smb")|| !strcmp(pszValue, "afp"))
        {
            pSettings->bUseDefaultUseADUncForHomeLocation = FALSE; 
            pSettings->bUseADUncForHomeLocation = TRUE;

            pSettings->bUseDefaultUncProtocolForHomeLocation = FALSE; 
            ceError = LwAllocateString(pszValue, &(pSettings->pszUncProtocolForHomeLocation)); 
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else
        {
            pSettings->bUseDefaultUseADUncForHomeLocation = FALSE; 
            pSettings->bUseADUncForHomeLocation = FALSE;
        }
    } 
    else if (!strcmp(pszName, ALLOW_ADMINISTRATION_BY)) 
    {
        pSettings->bUseDefaultAllowAdministrationBy = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->pszAllowAdministrationBy)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    } 
    else if (!strcmp(pszName, MERGE_ADMINS)) 
    {
        pSettings->bUseDefaultEnableMergeAdmins = FALSE; 
        pSettings->bEnableMergeAdmins = GetBoolValue(pszValue); 
    }

error:

    return ceError;
}

static
CENTERROR
UpdateLwedPolicySettings( 
    PLWED_POLICY_SETTINGS pPolicySettings
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY hKey = NULL;

    ceError = GPGetRegOpenKey(
                "Policy\\Services\\lwedsplugin\\Parameters",
                &hReg,
                &hKey);
    if (ceError)
    {
        GPA_LOG_ERROR( "Unable to open registry key");
        ceError = 0;
        goto error;
    }

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->bUseDefaultEnableMergeModeMCX,
                     "EnableMergeModeMCX",
                     pPolicySettings->bEnableMergeModeMCX);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->bUseDefaultEnableForceHomedirOnStartupDisk,
                     "EnableForceHomedirOnStartupDisk",
                     pPolicySettings->bEnableForceHomedirOnStartupDisk);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->bUseDefaultUseADUncForHomeLocation,
                     "UseADUncForHomeLocation",
                     pPolicySettings->bUseADUncForHomeLocation);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->bUseDefaultUncProtocolForHomeLocation,
                     "UncProtocolForHomeLocation",
                     pPolicySettings->pszUncProtocolForHomeLocation);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->bUseDefaultAllowAdministrationBy,
                     "AllowAdministrationBy",
                     pPolicySettings->pszAllowAdministrationBy);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->bUseDefaultEnableMergeAdmins,
                     "EnableMergeAdmins",
                     pPolicySettings->bEnableMergeAdmins);
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
FreeLwedStructValues(
    PLWED_POLICY_SETTINGS pPolicySettings
    )
{
    LW_SAFE_FREE_STRING(pPolicySettings->pszUncProtocolForHomeLocation);
    LW_SAFE_FREE_STRING(pPolicySettings->pszAllowAdministrationBy);
}
