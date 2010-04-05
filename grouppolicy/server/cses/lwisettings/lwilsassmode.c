#include "includes.h"

static
CENTERROR
UpdateGlobalPolicySettings( 
    PLSASS_POLICY_SETTINGS pPolicySettings
    );

static
CENTERROR
UpdatePAMPolicySettings( 
    PLSASS_POLICY_SETTINGS pPolicySettings
    );

static
CENTERROR
UpdateADPolicySettings( 
    PLSASS_POLICY_SETTINGS pPolicySettings
    );

static
CENTERROR
UpdatePAMPolicySettings( 
    PLSASS_POLICY_SETTINGS pPolicySettings
    );

static
CENTERROR
UpdateLocalPolicySettings( 
    PLSASS_POLICY_SETTINGS pPolicySettings
    );

static
CENTERROR
SignalLsassDaemon();

static
VOID
FreeLsassStructValues(
    PLSASS_POLICY_SETTINGS pPolicySettings
    );

static
CENTERROR
ConvertHostnamePlaceholderInMemberList(
    PCSTR pszPolicyMemberList,
    PSTR * ppszModifiedMemberList
    );

static
CENTERROR
ValidateSeparatorCharacter(
    CHAR cValue
    );

static
VOID
InitLsassStruct(
    PLSASS_POLICY_SETTINGS pPolicySettings
    );

static
CENTERROR
ProcessLsassSettingsGPItem(
    PGPOLWIGPITEM pGPAuthItem,
    PGPOLWIGPITEM pGPLogonItem
    );

static
CENTERROR
MapAndAddSettings(
    PLSASS_POLICY_SETTINGS *ppSettings,
    PSTR pszName,
    PSTR pszValue
    );

static
CENTERROR
ParseAndProcessGPItem(
    PLSASS_POLICY_SETTINGS pSettings,
    xmlNodePtr root_node
    );

static
CENTERROR
ProcessSectionSettings(
    PLSASS_POLICY_SETTINGS *ppSettings,
    PSTR pszName,
    PSTR pszValue
    );

static
CENTERROR
ProcessPAMSectionSettings(
    PLSASS_POLICY_SETTINGS *ppSettings,
    PSTR pszName,
    PSTR pszValue
    );

CENTERROR
ProcessLsassSettingsMode( 
    PGPOLWIGPITEM pRsopGPAuthItem,
    PGPOLWIGPITEM pRsopGPLogonItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = ResetLsassAuthSettings();
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( pRsopGPAuthItem || pRsopGPLogonItem ) 
    {
        ceError = ProcessLsassSettingsGPItem( pRsopGPAuthItem,
                                              pRsopGPLogonItem);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } 
    
    /* Restart the Lsass daemon */
    ceError = SignalLsassDaemon();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

CENTERROR
ResetLsassAuthSettings()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;

    ceError = GPACheckFileExists( LWI_KRB5_CONF_OLD_FILE,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
        ceError = GPAMoveFileAcrossDevices( LWI_KRB5_CONF_OLD_FILE,
                                           LWI_KRB5_CONF_FILE);
        BAIL_ON_CENTERIS_ERROR(ceError);        
    }

error:

    return ceError;
}

static
CENTERROR
ProcessLsassSettingsGPItem(
    PGPOLWIGPITEM pGPAuthItem,
    PGPOLWIGPITEM pGPLogonItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIGPITEM pGPItem = NULL;
    LSASS_POLICY_SETTINGS PolicySettings;

    /* Make a deep copy of the item to remove the sibling link */
    if (pGPAuthItem)
    {
        ceError = GPOCopyGPItem( pGPAuthItem, 
                                 &pGPItem, 
                                 TRUE );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pGPLogonItem && pGPItem)
    {
        ceError = GPOMergeGPItems( pGPLogonItem,
                                   pGPItem);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } 
    else if (pGPLogonItem)
    {
        ceError = GPOCopyGPItem( pGPLogonItem, 
                                 &pGPItem, 
                                 TRUE );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
 
    //Initialize the strucutre
    InitLsassStruct(&PolicySettings);

    //If policy is set then read from xml, else delete the keyvalue name
    if(pGPItem)
    {
        /* Just save out our data directly */
        ceError = ParseAndProcessGPItem(
                      &PolicySettings, 
                      pGPItem->xmlNode);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = UpdateGlobalPolicySettings(&PolicySettings);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdatePAMPolicySettings(&PolicySettings);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateADPolicySettings(&PolicySettings);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateLocalPolicySettings(&PolicySettings);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    FreeLsassStructValues(&PolicySettings);

    if (pGPItem)
    {
        GPODestroyGPItem( pGPItem, 
                          TRUE );
    }

    return ceError;
}

static
CENTERROR
ParseAndProcessGPItem(
    PLSASS_POLICY_SETTINGS pSettings,
    xmlNodePtr root_node
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cur_node = NULL;
    xmlChar *pszNodeText = NULL;
    xmlChar *nameAttrValue = NULL;

    for (cur_node = root_node; cur_node; cur_node = cur_node->next) {
        pszNodeText = NULL;
        if (cur_node->name) {
           if (strcmp( (PCSTR)cur_node->name,
                        (PCSTR)LWI_TAG_SETTING) == 0 ) {
                nameAttrValue = xmlGetProp( cur_node,
                                            (const xmlChar*)LWI_ATTR_NAME );
                if (!IsNullOrEmptyString(nameAttrValue)) {
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

            if (pszNodeText) {
                xmlFree(pszNodeText);
                pszNodeText = NULL;
            }

            if (nameAttrValue) {
                xmlFree(nameAttrValue);
                nameAttrValue = NULL;
            }
        }

        ParseAndProcessGPItem(
            pSettings,
            cur_node->children);
    }

error:

    if ( pszNodeText ) {
        xmlFree( pszNodeText );
        pszNodeText = NULL;
    }

    if ( nameAttrValue ) {
        xmlFree( nameAttrValue );
        nameAttrValue = NULL;
    }

    return ceError;
}

static
CENTERROR
MapAndAddSettings(
    PLSASS_POLICY_SETTINGS *ppSettings,
    PSTR pszName,
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PLSASS_POLICY_SETTINGS pSettings = *ppSettings;

    if (!strcmp(pszName, ENABLE_EVENTLOG))
    {
        pSettings->globalSettings.bUseDefaultEnableEventLog = FALSE; 
        pSettings->globalSettings.bEnableEventLog = GetBoolValue(pszValue); 

    } 
    else if (!strcmp(pszName, LOG_NETWORK_CONN_EVENTS))
    {
        pSettings->globalSettings.bUseDefaultLogNetworkConnectionEvents = FALSE; 
        pSettings->globalSettings.bLogNetworkConnectionEvents = GetBoolValue(pszValue); 

    } 
    else if ( !strcmp( (PCSTR)pszName, (PCSTR)LWI_CLOCKSKEW)) 
    {
        ceError = write_krb5_setting( pszName,
                                      pszValue);
        BAIL_ON_CENTERIS_ERROR( ceError );
    } 
    else
    {
        ceError = ProcessSectionSettings( 
                      &pSettings,
                      pszName,
                      pszValue);     
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return ceError;
}

static
CENTERROR
ProcessSectionSettings(
    PLSASS_POLICY_SETTINGS *ppSettings,
    PSTR pszName,
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PLSASS_POLICY_SETTINGS pSettings = *ppSettings;

    if (!strcmp(pszName, "machine password timeout")) {
        pSettings->adSettings.bUseDefaultMachinePasswordLifespan = FALSE; 
        pSettings->adSettings.dwMachinePasswordLifespan = (DWORD)(atoi(pszValue)*3600); 
    } else if (!strcmp(pszName, "winbind replacement character")) {
        ceError = ValidateSeparatorCharacter(pszValue[0]);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pSettings->adSettings.bUseDefaultSpaceReplacement = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->adSettings.pszSpaceReplacement)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszName, "winbind offline logon")) {
        //TODO check this is not in structure
    } else if (!strcmp(pszName, "winbind cache time")) {
        pSettings->adSettings.bUseDefaultCacheEntryExpiry = FALSE; 
        pSettings->adSettings.dwCacheEntryExpiry = (DWORD)atoi(pszValue); 
    } else if (!strcmp(pszName, "winbind expand groups")) {
        //TODO check this is not in structure
    } else if (!strcmp(pszName, "create_k5login")) {
        pSettings->adSettings.bUseDefaultCreateK5Login = FALSE; 
        pSettings->adSettings.bCreateK5Login = GetBoolValue(pszValue); 
    } else if (!strcmp(pszName, "create_homedir")) {
        pSettings->adSettings.bUseDefaultCreateHomeDir = FALSE; 
        pSettings->adSettings.bCreateHomeDir = GetBoolValue(pszValue); 
    } else if (!strcmp(pszName, "skel")) {
        pSettings->adSettings.bUseDefaultSkeletonDirs = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->adSettings.pszSkeletonDirs)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszName, "umask")) {
        pSettings->adSettings.bUseDefaultHomeDirUMask = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->adSettings.pszHomeDirUMask)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszName, SYNC_SYSTEM_TIME)) {
        pSettings->adSettings.bUseDefaultSyncSystemTime = FALSE; 
        pSettings->adSettings.bSyncSystemTime = GetBoolValue(pszValue); 
    } else if (!strcmp(pszName, ASSUME_DEFAULT_DOMAIN)) {
        pSettings->adSettings.bUseDefaultDomain = FALSE; 
        pSettings->adSettings.bAssumeDefaultDomain = GetBoolValue(pszValue); 
    } else if (!strcmp(pszName, HOMEDIR_PREFIX_PATH)) {
        pSettings->adSettings.bUseDefaultHomeDirPrefix = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->adSettings.pszHomeDirPrefix)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszName, HOMEDIR_TEMPLATE)) {
        pSettings->adSettings.bUseDefaultHomeDirTemplate = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->adSettings.pszHomeDirTemplate)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszName, LOGIN_SHELL_TEMPLATE)) {
        pSettings->adSettings.bUseDefaultLoginShellTemplate = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->adSettings.pszLoginShellTemplate)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszName, LDAP_SIGN_AND_SEAL)) {
        pSettings->adSettings.bUseDefaultLdapSignAndSeal = FALSE; 
        pSettings->adSettings.bLdapSignAndSeal = GetBoolValue(pszValue); 
    } else if (!strcmp(pszName, REFRESH_USER_CREDENTIALS)) {
        pSettings->adSettings.bUseDefaultRefreshUserCredentials = FALSE; 
        pSettings->adSettings.bRefreshUserCredentials = GetBoolValue(pszValue); 
    } else if (!strcmp(pszName, TRIM_USER_MEMBERSHIP)) {
        pSettings->adSettings.bUseDefaultTrimUserMembership = FALSE; 
        pSettings->adSettings.bTrimUserMembership = GetBoolValue(pszValue); 
    } else if (!strcmp(pszName, NSS_GROUP_QUERY_CACHE_ONLY)) {
        pSettings->adSettings.bUseDefaultNssGroupMembersQueryCacheOnly = FALSE; 
        pSettings->adSettings.bNssGroupMembersQueryCacheOnly = GetBoolValue(pszValue); 
    } else if (!strcmp(pszName, NSS_USER_QUERY_CACHE_ONLY)) {
        pSettings->adSettings.bUseDefaultNssUserMembersQueryCacheOnly = FALSE; 
        pSettings->adSettings.bNssUserMembersQueryCacheOnly = GetBoolValue(pszValue); 
    } else if (!strcmp(pszName, NSS_ENUMERATION_ENABLED)) {
        pSettings->adSettings.bUseDefaultNssEnumerationEnabled = FALSE; 
        pSettings->adSettings.bNssEnumerationEnabled = GetBoolValue(pszValue); 
    } else if (!strcmp(pszName, LIKEWISE_CELL_SUPPORT)) {
        pSettings->adSettings.bUseDefaultCellSupport = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->adSettings.pszCellSupport)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else {
        ceError = ProcessPAMSectionSettings( 
                         &pSettings,
                         pszName,
                         pszValue);     
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
 
error:

    return ceError;
}

static
CENTERROR
ProcessPAMSectionSettings(
    PLSASS_POLICY_SETTINGS *ppSettings,
    PSTR pszName,
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PLSASS_POLICY_SETTINGS pSettings = *ppSettings;
    PSTR pszMemberList = NULL;

    if (!strcmp(pszName, "require_membership_of")) {
        ceError = ConvertHostnamePlaceholderInMemberList(pszValue, &pszMemberList);
        BAIL_ON_CENTERIS_ERROR(ceError);
        pSettings->adSettings.bUseDefaultRequireMembershipOf = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->adSettings.pszRequireMembershipOf)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszName, "not_a_member_error")) {
        pSettings->pamSettings.bUseDefaultUserNotAllowedError = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->pamSettings.pszUserNotAllowedError)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszName, "debug")) {
        pSettings->pamSettings.bUseDefaultLogLevel = FALSE; 
        if (!strcmp(pszValue, "true")) {
            ceError = LwAllocateString("error", &(pSettings->pamSettings.pszLogLevel)); 
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            ceError = LwAllocateString("disabled", &(pSettings->pamSettings.pszLogLevel)); 
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    } else if (!strcmp(pszName, "warn_pwd_expire")) {
        //TODO: to chk
        /*ceError = AddLsassSettings( LOCAL_PROVIDER_SECTION,
                                    PASSWD_EXPIRATION_WARNING,
                                    pszValue,
                                    TypeDword,
                                    TRUE);
        BAIL_ON_CENTERIS_ERROR(ceError);
        */
    } else if (!strcmp(pszName, PASSWORD_LIFESPAN)) {
        //TODO: to chk
        /*ceError = AddLsassSettings( LOCAL_PROVIDER_SECTION,
                                    PASSWORD_LIFESPAN,
                                    pszValue,
                                    TypeDword,
                                    TRUE);
        BAIL_ON_CENTERIS_ERROR(ceError);
        */
    } else if (!strcmp(pszName, DISPLAY_MOTD)) {
        pSettings->pamSettings.bUseDefaultDisplayMotd = FALSE; 
        pSettings->pamSettings.bDisplayMotd = GetBoolValue(pszValue); 
    } else if (!strcmp(pszName, "local_create_homedir")) {
        pSettings->localSettings.bUseDefaultCreateHomeDir = FALSE; 
        pSettings->localSettings.bCreateHomeDir = GetBoolValue(pszValue); 
    } else if (!strcmp(pszName, "local-skeleton-dirs")) {
        pSettings->localSettings.bUseDefaultSkeletonDirs = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->localSettings.pszSkeletonDirs)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszName, "local-homedir-umask")) {
        pSettings->localSettings.bUseDefaultHomeDirUMask = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->localSettings.pszHomeDirUMask)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszName, "local-homedir-prefix")) {
        pSettings->localSettings.bUseDefaultHomeDirPrefix = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->localSettings.pszHomeDirPrefix)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszName, "local-homedir-template")) {
        pSettings->localSettings.bUseDefaultHomeDirTemplate = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->localSettings.pszHomeDirTemplate)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszName, "local-login-shell-template")) {
        pSettings->localSettings.bUseDefaultLoginShellTemplate = FALSE; 
        ceError = LwAllocateString(pszValue, &(pSettings->localSettings.pszLoginShellTemplate)); 
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    LW_SAFE_FREE_STRING(pszMemberList);
    return ceError;
}

CENTERROR
UpdateGlobalPolicySettings( 
    PLSASS_POLICY_SETTINGS pPolicySettings
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY hKey = NULL;

    ceError = GPGetRegOpenKey(
                "Policy\\Services\\lsass\\Parameters",
                &hReg,
                &hKey);
    if (ceError)
    {
        GPA_LOG_ERROR( "Unable to open registry root key");
        ceError = 0;
        goto error;
    }

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->globalSettings.bUseDefaultEnableEventLog,
                     "EnableEventlog",
                     pPolicySettings->globalSettings.bEnableEventLog);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->globalSettings.bUseDefaultLogNetworkConnectionEvents,
                     "LogNetworkConnectionEvents",
                     pPolicySettings->globalSettings.bLogNetworkConnectionEvents);
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

CENTERROR
UpdatePAMPolicySettings( 
    PLSASS_POLICY_SETTINGS pPolicySettings
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY hKey = NULL;

    ceError = GPGetRegOpenKey(
                "Policy\\Services\\lsass\\Parameters\\PAM",
                &hReg,
                &hKey);
    if (ceError)
    {
        GPA_LOG_ERROR( "Unable to open registry root key");
        ceError = 0;
        goto error;
    }

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->pamSettings.bUseDefaultLogLevel,
                     "LogLevel",
                     pPolicySettings->pamSettings.pszLogLevel);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->pamSettings.bUseDefaultDisplayMotd,
                     "DisplayMotd",
                     pPolicySettings->pamSettings.bDisplayMotd);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->pamSettings.bUseDefaultUserNotAllowedError,
                     "UserNotAllowedError",
                     pPolicySettings->pamSettings.pszUserNotAllowedError);
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

CENTERROR
UpdateADPolicySettings( 
    PLSASS_POLICY_SETTINGS pPolicySettings
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY hKey = NULL;

    ceError = GPGetRegOpenKey(
                "Policy\\Services\\lsass\\Parameters\\Providers\\ActiveDirectory",
                &hReg,
                &hKey);
    if (ceError)
    {
        GPA_LOG_ERROR( "Unable to open registry root key");
        ceError = 0;
        goto error;
    }

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultLoginShellTemplate,
                     "LoginShellTemplate",
                     pPolicySettings->adSettings.pszLoginShellTemplate);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultHomeDirPrefix,
                     "HomeDirPrefix",
                     pPolicySettings->adSettings.pszHomeDirPrefix);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultHomeDirTemplate,
                     "HomeDirTemplate",
                     pPolicySettings->adSettings.pszHomeDirTemplate);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultLdapSignAndSeal,
                     "LdapSignAndSeal",
                     pPolicySettings->adSettings.bLdapSignAndSeal);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateDwordValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultCacheEntryExpiry,
                     "CacheEntryExpiry",
                     pPolicySettings->adSettings.dwCacheEntryExpiry);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateDwordValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultMachinePasswordLifespan,
                     "MachinePasswordLifespan",
                     pPolicySettings->adSettings.dwMachinePasswordLifespan);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultSpaceReplacement,
                     "SpaceReplacement",
                     pPolicySettings->adSettings.pszSpaceReplacement);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultDomain,
                     "AssumeDefaultDomain",
                     pPolicySettings->adSettings.bAssumeDefaultDomain);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultSyncSystemTime,
                     "SyncSystemTime",
                     pPolicySettings->adSettings.bSyncSystemTime);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateMultiStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultRequireMembershipOf,
                     "RequireMembershipOf",
                     pPolicySettings->adSettings.pszRequireMembershipOf);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultCreateK5Login,
                     "CreateK5Login",
                     pPolicySettings->adSettings.bCreateK5Login);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultCreateHomeDir,
                     "CreateHomeDir",
                     pPolicySettings->adSettings.bCreateHomeDir);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultHomeDirUMask,
                     "HomeDirUMask",
                     pPolicySettings->adSettings.pszHomeDirUMask);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultSkeletonDirs,
                     "SkeletonDirs",
                     pPolicySettings->adSettings.pszSkeletonDirs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultRefreshUserCredentials,
                     "RefreshUserCredentials",
                     pPolicySettings->adSettings.bRefreshUserCredentials);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultCellSupport,
                     "CellSupport",
                     pPolicySettings->adSettings.pszCellSupport);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultTrimUserMembership,
                     "TrimUserMembership",
                     pPolicySettings->adSettings.bTrimUserMembership);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultNssGroupMembersQueryCacheOnly,
                     "NssGroupMembersQueryCacheOnly",
                     pPolicySettings->adSettings.bNssGroupMembersQueryCacheOnly);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultNssUserMembersQueryCacheOnly,
                     "NssUserMembersQueryCacheOnly",
                     pPolicySettings->adSettings.bNssUserMembersQueryCacheOnly);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultNssEnumerationEnabled,
                     "NssEnumberationEnabled",
                     pPolicySettings->adSettings.bNssEnumerationEnabled);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateDwordValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultDomainManagerCheckDomainOnlineInterval,
                     "DomainManagerCheckDomainOnlineInterval",
                     pPolicySettings->adSettings.dwDomainManagerCheckDomainOnlineInterval);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateDwordValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultDomainManagerUnknownDomainCacheTimeout,
                     "DomainManagerUnknownDomainCacheTimeout",
                     pPolicySettings->adSettings.dwDomainManagerUnknownDomainCacheTimeout);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultCacheType,
                     "CacheType",
                     pPolicySettings->adSettings.pszCacheType);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateDwordValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->adSettings.bUseDefaultMemoryCacheSizeCap,
                     "MemoryCacheSizeCap",
                     pPolicySettings->adSettings.dwMemoryCacheSizeCap);
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

CENTERROR
UpdateLocalPolicySettings( 
    PLSASS_POLICY_SETTINGS pPolicySettings
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY hKey = NULL;

    ceError = GPGetRegOpenKey(
                "Policy\\Services\\lsass\\Parameters\\Providers\\Local",
                &hReg,
                &hKey);
    if (ceError)
    {
        GPA_LOG_ERROR( "Unable to open registry root key");
        ceError = 0;
        goto error;
    }

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->localSettings.bUseDefaultLoginShellTemplate,
                     "LoginShellTemplate",
                     pPolicySettings->localSettings.pszLoginShellTemplate);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->localSettings.bUseDefaultHomeDirPrefix,
                     "HomeDirPrefix",
                     pPolicySettings->localSettings.pszHomeDirPrefix);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->localSettings.bUseDefaultHomeDirTemplate,
                     "HomeDirTemplate",
                     pPolicySettings->localSettings.pszHomeDirTemplate);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->localSettings.bUseDefaultCreateHomeDir,
                     "CreateHomeDir",
                     pPolicySettings->localSettings.bCreateHomeDir);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->localSettings.bUseDefaultHomeDirUMask,
                     "HomeDirUMask",
                     pPolicySettings->localSettings.pszHomeDirUMask);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pPolicySettings->localSettings.bUseDefaultSkeletonDirs,
                     "SkeletonDirs",
                     pPolicySettings->localSettings.pszSkeletonDirs);
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
CENTERROR
SignalLsassDaemon()
{
    CENTERROR ceError = CENTERROR_SUCCESS;    
    HANDLE hLsaConnection = (HANDLE)NULL;
    
    ceError = LsaOpenServer(&hLsaConnection);
    if(ceError != CENTERROR_SUCCESS)
    {
        ceError = CENTERROR_LSASS_NOT_RUNNING;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LsaRefreshConfiguration(hLsaConnection);
    if(ceError != CENTERROR_SUCCESS)
    {
        ceError = CENTERROR_LSASS_ERROR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

cleanup:
    
    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    return ceError;

error:

    goto cleanup;
}

static
VOID
InitLsassStruct(
    PLSASS_POLICY_SETTINGS pPolicySettings
    )
{
    //set default flag to true
    pPolicySettings->globalSettings.bUseDefaultEnableEventLog                = TRUE;
    pPolicySettings->globalSettings.bUseDefaultLogNetworkConnectionEvents      = TRUE;
    //pam default set to true
    pPolicySettings->pamSettings.bUseDefaultLogLevel      = TRUE;
    pPolicySettings->pamSettings.bUseDefaultDisplayMotd      = TRUE;
    pPolicySettings->pamSettings.bUseDefaultUserNotAllowedError      = TRUE;
    pPolicySettings->pamSettings.pszUserNotAllowedError = NULL;
    pPolicySettings->pamSettings.pszLogLevel = NULL;
    //Ad settings
    pPolicySettings->adSettings.bUseDefaultLoginShellTemplate          = TRUE;
    pPolicySettings->adSettings.bUseDefaultHomeDirPrefix               = TRUE;
    pPolicySettings->adSettings.bUseDefaultHomeDirTemplate             = TRUE;
    pPolicySettings->adSettings.bUseDefaultLdapSignAndSeal             = TRUE;
    pPolicySettings->adSettings.bUseDefaultCacheEntryExpiry            = TRUE;
    pPolicySettings->adSettings.bUseDefaultMachinePasswordLifespan     = TRUE;
    pPolicySettings->adSettings.bUseDefaultSpaceReplacement            = TRUE;
    pPolicySettings->adSettings.bUseDefaultDomain                      = TRUE;
    pPolicySettings->adSettings.bUseDefaultSyncSystemTime              = TRUE;
    pPolicySettings->adSettings.bUseDefaultRequireMembershipOf         = TRUE;
    pPolicySettings->adSettings.bUseDefaultCreateK5Login               = TRUE;
    pPolicySettings->adSettings.bUseDefaultCreateHomeDir               = TRUE;
    pPolicySettings->adSettings.bUseDefaultHomeDirUMask                = TRUE;
    pPolicySettings->adSettings.bUseDefaultSkeletonDirs                = TRUE;
    pPolicySettings->adSettings.bUseDefaultRefreshUserCredentials      = TRUE;
    pPolicySettings->adSettings.bUseDefaultCellSupport                 = TRUE;
    pPolicySettings->adSettings.bUseDefaultTrimUserMembership          = TRUE;
    pPolicySettings->adSettings.bUseDefaultNssGroupMembersQueryCacheOnly = TRUE;
    pPolicySettings->adSettings.bUseDefaultNssUserMembersQueryCacheOnly  = TRUE;
    pPolicySettings->adSettings.bUseDefaultNssEnumerationEnabled         = TRUE;
    pPolicySettings->adSettings.bUseDefaultDomainManagerCheckDomainOnlineInterval = TRUE;
    pPolicySettings->adSettings.bUseDefaultDomainManagerUnknownDomainCacheTimeout = TRUE;
    pPolicySettings->adSettings.bUseDefaultCacheType                              = TRUE;
    pPolicySettings->adSettings.bUseDefaultMemoryCacheSizeCap                     = TRUE;
    pPolicySettings->adSettings.pszLoginShellTemplate                             = NULL;
    pPolicySettings->adSettings.pszHomeDirPrefix                                  = NULL;
    pPolicySettings->adSettings.pszHomeDirTemplate                                = NULL;
    pPolicySettings->adSettings.pszSpaceReplacement                               = NULL;
    pPolicySettings->adSettings.pszRequireMembershipOf                            = NULL;
    pPolicySettings->adSettings.pszHomeDirUMask                                   = NULL;
    pPolicySettings->adSettings.pszSkeletonDirs                                   = NULL;
    pPolicySettings->adSettings.pszCellSupport                                    = NULL;
    pPolicySettings->adSettings.pszCacheType                                      = NULL;
    //local settings
    pPolicySettings->localSettings.bUseDefaultLoginShellTemplate           = TRUE;
    pPolicySettings->localSettings.bUseDefaultHomeDirPrefix                = TRUE;
    pPolicySettings->localSettings.bUseDefaultHomeDirTemplate              = TRUE;
    pPolicySettings->localSettings.bUseDefaultCreateHomeDir                = TRUE;
    pPolicySettings->localSettings.bUseDefaultHomeDirUMask                 = TRUE;
    pPolicySettings->localSettings.bUseDefaultSkeletonDirs                 = TRUE;
    pPolicySettings->localSettings.pszLoginShellTemplate                   = NULL;
    pPolicySettings->localSettings.pszHomeDirPrefix                        = NULL;
    pPolicySettings->localSettings.pszHomeDirTemplate                      = NULL;
    pPolicySettings->localSettings.pszHomeDirUMask                         = NULL;
    pPolicySettings->localSettings.pszSkeletonDirs                         = NULL;
}

static
CENTERROR
ValidateSeparatorCharacter(
    CHAR cValue
    )
{
    DWORD dwError = 0;

    if (!ispunct((int)cValue))
    {
        GPA_LOG_ERROR( "Error: space-replacement must be punctuation; value provided is \"%c\"",
                       cValue);
        dwError = CENTERROR_INVALID_PARAMETER;
    }
    else if( cValue == '@'  ||
             cValue == '#'  ||
             cValue == '/'  ||
             cValue == '\\')
    {
        GPA_LOG_ERROR( "Error: space-replacement may not be @, #, /, or \\; value provided is \"%c\"",
                       cValue);
        dwError = CENTERROR_INVALID_PARAMETER;
    }

    BAIL_ON_CENTERIS_ERROR(dwError);

error:

    return dwError;

}

static
CENTERROR
ConvertHostnamePlaceholderInMemberList(
    PCSTR pszPolicyMemberList,
    PSTR * ppszModifiedMemberList
    )
{
    DWORD dwError = 0;
    PSTR pszTempList = NULL;
    PSTR pszMemberList = NULL;
    PSTR pTemp = NULL;
    PSTR pReplace = NULL;
    PSTR pszHostname = NULL;

    dwError = GPAGetDnsSystemNames(&pszHostname, NULL, NULL);
    BAIL_ON_CENTERIS_ERROR(dwError);

    if (pszPolicyMemberList != NULL)
    {
        dwError = LwAllocateMemory(strlen(pszPolicyMemberList) * 3,
                                   (PVOID *) &pszTempList);
        BAIL_ON_CENTERIS_ERROR(dwError);

        pTemp = (PSTR) pszPolicyMemberList;

        while (pTemp && *pTemp)
        {
            pReplace = strstr(pTemp, "%hostname");

            if (pReplace == NULL)
            {
                strcat(pszTempList, pTemp);
                pTemp = NULL;
                break;
            }

            strncat(pszTempList, pTemp, pReplace - pTemp);
            strcat(pszTempList, pszHostname);

            pTemp = pReplace + strlen("%hostname");
        }

        dwError = LwAllocateString(pszTempList, &pszMemberList);
        BAIL_ON_CENTERIS_ERROR(dwError);
    }

    *ppszModifiedMemberList = pszMemberList;
    pszMemberList = NULL;

error:

    LW_SAFE_FREE_STRING(pszTempList);
    LW_SAFE_FREE_STRING(pszHostname);

    return dwError;
}

static
VOID
FreeLsassStructValues(
    PLSASS_POLICY_SETTINGS pPolicySettings
    )
{

    LW_SAFE_FREE_STRING(pPolicySettings->pamSettings.pszUserNotAllowedError);
    LW_SAFE_FREE_STRING(pPolicySettings->pamSettings.pszLogLevel);

    LW_SAFE_FREE_STRING(pPolicySettings->adSettings.pszLoginShellTemplate);
    LW_SAFE_FREE_STRING(pPolicySettings->adSettings.pszHomeDirPrefix);
    LW_SAFE_FREE_STRING(pPolicySettings->adSettings.pszHomeDirTemplate);
    LW_SAFE_FREE_STRING(pPolicySettings->adSettings.pszSpaceReplacement);
    LW_SAFE_FREE_STRING(pPolicySettings->adSettings.pszRequireMembershipOf);
    LW_SAFE_FREE_STRING(pPolicySettings->adSettings.pszHomeDirUMask);
    LW_SAFE_FREE_STRING(pPolicySettings->adSettings.pszSkeletonDirs);
    LW_SAFE_FREE_STRING(pPolicySettings->adSettings.pszCellSupport);
    LW_SAFE_FREE_STRING(pPolicySettings->adSettings.pszCacheType);

    LW_SAFE_FREE_STRING(pPolicySettings->localSettings.pszLoginShellTemplate);
    LW_SAFE_FREE_STRING(pPolicySettings->localSettings.pszHomeDirPrefix);
    LW_SAFE_FREE_STRING(pPolicySettings->localSettings.pszHomeDirTemplate);
    LW_SAFE_FREE_STRING(pPolicySettings->localSettings.pszHomeDirUMask);
    LW_SAFE_FREE_STRING(pPolicySettings->localSettings.pszSkeletonDirs);
}
