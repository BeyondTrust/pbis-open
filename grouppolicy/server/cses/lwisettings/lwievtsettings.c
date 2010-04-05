#include "includes.h"

static
CENTERROR
UpdateRegistryWithPolicySettings(
    PGPOLWIGPITEM pGPItem
    );

static
VOID
InitEVTStruct(
    PEVENTLOG_POLICY_SETTINGS  pevtPolicySettings
    );

static
CENTERROR
ReadXMLTree(
    PEVENTLOG_POLICY_SETTINGS  pevtPolicySettings, 
    xmlNodePtr root_node
    );

static
CENTERROR
MapAndAddSettings(
    PEVENTLOG_POLICY_SETTINGS  *ppevtPolicySettings, 
    struct _xmlAttr *pAttrNode,
    xmlNodePtr cur_node
    );

static
CENTERROR
UpdatePolicyInRegistry(
    HANDLE hConnection,
    HKEY   hKey,
    PEVENTLOG_POLICY_SETTINGS pevtPolicySettings
    );

static
CENTERROR
SendHUPToEventLogD();

CENTERROR
ApplyLwiEVTSettingsPolicy(
    PGPOLWIGPITEM *ppRsopEVTSettingsItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if(*ppRsopEVTSettingsItem)
    {
        ceError = UpdateRegistryWithPolicySettings( *ppRsopEVTSettingsItem );
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
    EVENTLOG_POLICY_SETTINGS  evtPolicySettings; 
    HANDLE hReg = (HANDLE)NULL;
    HKEY hKey = NULL;

    //set default flag to true
    InitEVTStruct(&evtPolicySettings);

    //If policy is set then read from xml. Else delete the keyvaluename
    if (pGPItem)
    {
        ceError = ReadXMLTree( 
                    &evtPolicySettings, 
                    (xmlNodePtr)pGPItem->xmlNode);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPGetRegOpenKey(
                "Policy\\Services\\eventlog\\Parameters",
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
                &evtPolicySettings);
                      
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = SendHUPToEventLogD();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    LW_SAFE_FREE_STRING(evtPolicySettings.pszAllowReadTo);
    LW_SAFE_FREE_STRING(evtPolicySettings.pszAllowWriteTo);
    LW_SAFE_FREE_STRING(evtPolicySettings.pszAllowDeleteTo);

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
InitEVTStruct(
    PEVENTLOG_POLICY_SETTINGS  pevtPolicySettings
    )
{
    //set default flag to true
    pevtPolicySettings->bUseDefaultAllowReadTo           = TRUE;
    pevtPolicySettings->bUseDefaultAllowWriteTo          = TRUE;
    pevtPolicySettings->bUseDefaultAllowDeleteTo         = TRUE;
    pevtPolicySettings->bUseDefaultMaxDiskUsage          = TRUE;
    pevtPolicySettings->bUseDefaultMaxEventLifespan      = TRUE;
    pevtPolicySettings->bUseDefaultMaxNumEvents          = TRUE;
    pevtPolicySettings->bUseDefaultRemoveAsNeeded        = TRUE;
    pevtPolicySettings->pszAllowReadTo                   = NULL;
    pevtPolicySettings->pszAllowWriteTo                  = NULL;
    pevtPolicySettings->pszAllowDeleteTo                 = NULL;
}

static
CENTERROR
ReadXMLTree(
    PEVENTLOG_POLICY_SETTINGS  pevtPolicySettings, 
    xmlNodePtr root_node
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cur_node = NULL;
    struct _xmlAttr *pAttrNode = NULL;

    for (cur_node = root_node; cur_node != NULL ; cur_node = cur_node->next) {

        if (cur_node->name) {
            if ( !strcmp( (PCSTR)cur_node->name,
                          (PCSTR)"setting")) {

                for ( pAttrNode = cur_node->properties; pAttrNode; pAttrNode = pAttrNode->next ) {
                    if ( !strcmp( (PCSTR)pAttrNode->name,
                                  (PCSTR)"name")) {
                        break;
                    }
                }

                ceError = MapAndAddSettings( &pevtPolicySettings,
                                             pAttrNode,
                                             cur_node);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

        }

        ReadXMLTree( pevtPolicySettings,
                     cur_node->children);
    }

    return ceError;

error:

    return ceError;
}

static
CENTERROR
MapAndAddSettings(
    PEVENTLOG_POLICY_SETTINGS  *ppevtPolicySettings, 
    struct _xmlAttr *pAttrNode,
    xmlNodePtr cur_node
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlChar *pszNodeText = NULL;

    if ( !strcmp( (PCSTR)pAttrNode->children->content,
                  (PCSTR)"max-disk-usage")) {
        ceError = get_node_text( cur_node,
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );

        (*ppevtPolicySettings)->bUseDefaultMaxDiskUsage = FALSE;

        (*ppevtPolicySettings)->dwMaxDiskUsage = (DWORD)atoi((const char*)pszNodeText);

    } else if ( !strcmp( (PCSTR)pAttrNode->children->content,
                         (PCSTR)"max-num-events")) {
        ceError = get_node_text( cur_node,
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );

        (*ppevtPolicySettings)->bUseDefaultMaxNumEvents = FALSE;

        (*ppevtPolicySettings)->dwMaxNumEvents = (DWORD)atoi((const char*)pszNodeText);

    } else if ( !strcmp( (PCSTR)pAttrNode->children->content,
                         (PCSTR)"max-event-lifespan")) {
        ceError = get_node_text( cur_node,
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );

        (*ppevtPolicySettings)->bUseDefaultMaxEventLifespan = FALSE;

        (*ppevtPolicySettings)->dwMaxEventLifespan = (DWORD)atoi((const char*)pszNodeText);

    } else if ( !strcmp( (PCSTR)pAttrNode->children->content,
                         (PCSTR)"remove-events-as-needed")) {
        ceError = get_node_text( cur_node,
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );

        (*ppevtPolicySettings)->bUseDefaultRemoveAsNeeded = FALSE;

        if( !strcmp((const char*)pszNodeText, (const char*)"true"))
            (*ppevtPolicySettings)->bRemoveAsNeeded = TRUE;
        else
            (*ppevtPolicySettings)->bRemoveAsNeeded = FALSE;

     } else if ( !strcmp( (PCSTR)pAttrNode->children->content,
                          (PCSTR)"allow-read-to")) {
        ceError = get_node_text( cur_node,
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );

        (*ppevtPolicySettings)->bUseDefaultAllowReadTo = FALSE;

        ceError = LwAllocateString((const char*)pszNodeText,
                                   &((*ppevtPolicySettings)->pszAllowReadTo));

        BAIL_ON_CENTERIS_ERROR(ceError);

     } else if ( !strcmp( (PCSTR)pAttrNode->children->content,
                          (PCSTR)"allow-write-to")) {
        ceError = get_node_text( cur_node,
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );

        (*ppevtPolicySettings)->bUseDefaultAllowWriteTo = FALSE;

        ceError = LwAllocateString((const char*)pszNodeText,
                                   &((*ppevtPolicySettings)->pszAllowWriteTo));

        BAIL_ON_CENTERIS_ERROR(ceError);

     } else if ( !strcmp( (PCSTR)pAttrNode->children->content,
                          (PCSTR)"allow-delete-to")) {
        ceError = get_node_text( cur_node,
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );

        (*ppevtPolicySettings)->bUseDefaultAllowDeleteTo = FALSE;

        ceError = LwAllocateString((const char*)pszNodeText,
                                   &((*ppevtPolicySettings)->pszAllowDeleteTo));

        BAIL_ON_CENTERIS_ERROR(ceError);
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
    PEVENTLOG_POLICY_SETTINGS pevtPolicySettings
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pevtPolicySettings->bUseDefaultAllowReadTo,
                     "AllowReadTo",
                     pevtPolicySettings->pszAllowReadTo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pevtPolicySettings->bUseDefaultAllowWriteTo,
                     "AllowWriteTo",
                     pevtPolicySettings->pszAllowWriteTo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pevtPolicySettings->bUseDefaultAllowDeleteTo,
                     "AllowDeleteTo",
                     pevtPolicySettings->pszAllowDeleteTo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateDwordValueInRegistry(
                     hReg,
                     hKey,
                     pevtPolicySettings->bUseDefaultMaxDiskUsage,
                     "MaxDiskUsage",
                     (1024*pevtPolicySettings->dwMaxDiskUsage));
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateDwordValueInRegistry(
                     hReg,
                     hKey,
                     pevtPolicySettings->bUseDefaultMaxEventLifespan,
                     "MaxEventLifespan",
                     pevtPolicySettings->dwMaxEventLifespan);//24*60*60 = 86400
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateDwordValueInRegistry(
                     hReg,
                     hKey,
                     pevtPolicySettings->bUseDefaultMaxNumEvents,
                     "MaxNumEvents",
                     pevtPolicySettings->dwMaxNumEvents);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateBoolValueInRegistry(
                     hReg,
                     hKey,
                     pevtPolicySettings->bUseDefaultRemoveAsNeeded,
                     "RemoveEventsAsNeeded",
                     pevtPolicySettings->bRemoveAsNeeded);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
SendHUPToEventLogD()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    pid_t pid_eventlog = 0;
    FILE *fp = NULL;
    char szBuf[BUFSIZE];
    BOOLEAN bFileExists = FALSE;

    memset(szBuf, 0, BUFSIZE);

    ceError = GPACheckFileExists( "/var/run/eventlogd.pid",
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        ceError = GPAOpenFile("/var/run/eventlogd.pid",
                             "r",
                             &fp);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if ( fgets(szBuf, BUFSIZE, fp) != NULL ) {
            if (!IsNullOrEmptyString(szBuf))
            {
                pid_eventlog = atoi(szBuf);
            }
            else
            {
                GPA_LOG_VERBOSE( "Could not determine eventlogd process pid");
                goto error;
            }
        }

        if ( pid_eventlog ) {
            GPA_LOG_VERBOSE( "Sending SIGHUP to eventlogd of pid: %d",
                             pid_eventlog);

            if ( kill(pid_eventlog, SIGHUP) < 0)
            {
                ceError = CTMapSystemError(errno);
                GPA_LOG_VERBOSE( "Could not HUP eventlogd process: [%d]", ceError);
                goto error;
            }
        }
    }
    else
    {
        goto error;
    }

cleanup:

    if (fp)
        GPACloseFile(fp);

    return CENTERROR_SUCCESS;

error:

    goto cleanup;
}
