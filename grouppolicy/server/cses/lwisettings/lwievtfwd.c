#include "includes.h"

static
CENTERROR
ProcessXMLData(
    PGPOLWIGPITEM pGPItem
    );

static
CENTERROR
ReadXMLTree(
    PEVTFWD_POLICY_SETTINGS pevtfwdPolicySettings,
    xmlNodePtr root_node
    );

static
CENTERROR
MapAndAddSettings(
    PEVTFWD_POLICY_SETTINGS *ppevtfwdPolicySettings,
    struct _xmlAttr *pAttrNode,
    xmlNodePtr cur_node
    );

static
CENTERROR
UpdatePolicyInRegistry(
    HANDLE hConnection,
    HKEY   hKey,
    PEVTFWD_POLICY_SETTINGS pevtPolicySettings
    );

static
CENTERROR
SendHUPToEventFwdD();

CENTERROR
ApplyLwiEVTFWDSettingsPolicy(
    PGPOLWIGPITEM *ppRsopEVTFwdSettingsItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if(*ppRsopEVTFwdSettingsItem)
    {
        ceError = ProcessXMLData( *ppRsopEVTFwdSettingsItem );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return ceError;
}

static
CENTERROR
ProcessXMLData(
    PGPOLWIGPITEM pGPItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    EVTFWD_POLICY_SETTINGS evtfwdPolicySettings;
    HANDLE hReg = (HANDLE)NULL;
    HKEY hKey = NULL;
    
    evtfwdPolicySettings.bUseDefaultCollector = TRUE;
    evtfwdPolicySettings.bUseDefaultCollectorPrincipal = TRUE;
    evtfwdPolicySettings.pszCollector = NULL;
    evtfwdPolicySettings.pszCollectorPrincipal = NULL;

    if(pGPItem)
    {
        ceError = ReadXMLTree( 
                        &evtfwdPolicySettings,
                        (xmlNodePtr)pGPItem->xmlNode);
    
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPGetRegOpenKey(
                "Policy\\Services\\eventfwd\\Parameters",
                &hReg,
                &hKey);
    if (ceError)
    {
        GPA_LOG_ERROR( " Unable to open registry root key");
        ceError = 0;
        goto error;
    }

    ceError = UpdatePolicyInRegistry(
                hReg,
                hKey,
                &evtfwdPolicySettings);

    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = SendHUPToEventFwdD();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    LW_SAFE_FREE_STRING(evtfwdPolicySettings.pszCollector);
    LW_SAFE_FREE_STRING(evtfwdPolicySettings.pszCollectorPrincipal);

    if( hReg && hKey )
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
ReadXMLTree(
    PEVTFWD_POLICY_SETTINGS pevtfwdPolicySettings,
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
            if ( !strcmp( (PCSTR)cur_node->name,
                          (PCSTR)"setting")) {

                for ( pAttrNode = cur_node->properties; pAttrNode; pAttrNode = pAttrNode->next ) {
                    if ( !strcmp( (PCSTR)pAttrNode->name,
                                  (PCSTR)"name")) {
                        break;
                    }
                }

                ceError = MapAndAddSettings( &pevtfwdPolicySettings,
                                             pAttrNode,
                                             cur_node);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

        }

        ReadXMLTree( pevtfwdPolicySettings, 
                     cur_node->children );
    }

error:

    return ceError;
}

static
CENTERROR
MapAndAddSettings(
    PEVTFWD_POLICY_SETTINGS *ppevtfwdPolicySettings,
    struct _xmlAttr *pAttrNode,
    xmlNodePtr cur_node
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlChar *pszNodeText = NULL;

    if ( !strcmp( (PCSTR)pAttrNode->children->content,
                  (PCSTR)"collector")) {
        ceError = get_node_text( cur_node,
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );
        
        (*ppevtfwdPolicySettings)->bUseDefaultCollector = FALSE;
        ceError = LwAllocateString((const char*)pszNodeText,
                                   &((*ppevtfwdPolicySettings)->pszCollector));
        BAIL_ON_CENTERIS_ERROR(ceError);

    } else if ( !strcmp( (PCSTR)pAttrNode->children->content,
                         (PCSTR)"collector-principal")) {
        ceError = get_node_text( cur_node,
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );
        
        (*ppevtfwdPolicySettings)->bUseDefaultCollectorPrincipal = FALSE;
        ceError = LwAllocateString((const char*)pszNodeText,
                                   &((*ppevtfwdPolicySettings)->pszCollectorPrincipal));
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
    PEVTFWD_POLICY_SETTINGS pevtfwdPolicySettings
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    
    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pevtfwdPolicySettings->bUseDefaultCollector,
                     "Collector",
                     pevtfwdPolicySettings->pszCollector);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = UpdateStringValueInRegistry(
                     hReg,
                     hKey,
                     pevtfwdPolicySettings->bUseDefaultCollectorPrincipal,
                     "CollectorPrincipal",
                     pevtfwdPolicySettings->pszCollectorPrincipal);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
SendHUPToEventFwdD()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    pid_t pid_eventfwd = 0;
    FILE *fp = NULL;
    char szBuf[BUFSIZE];
    BOOLEAN bFileExists = FALSE;
    PSTR pszBuf = NULL;

    memset(szBuf, 0, BUFSIZE);

    ceError = GPACheckFileExists("/var/run/eventfwdd.pid",
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists)
    {
        ceError = GPAOpenFile("/var/run/eventfwdd.pid",
                              "r",
                              &fp);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if ( fgets(szBuf, BUFSIZE, fp) != NULL ) {
            if (!IsNullOrEmptyString(szBuf))
            {                pid_eventfwd = atoi(szBuf);
            }
            else
            {
                GPA_LOG_VERBOSE( "Could not determine eventfwdd process pid");
                goto error;            }
        }

        if ( pid_eventfwd ) {
            GPA_LOG_VERBOSE( "Sending SIGHUP to eventfwdd of pid: %d",                             pid_eventfwd);

            if ( kill(pid_eventfwd, SIGHUP) < 0)
            {
                ceError = CTMapSystemError(errno);
                GPA_LOG_VERBOSE( "Could not HUP eventfwdd process: [%d]", ceError);
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

    LW_SAFE_FREE_STRING(pszBuf);

    return CENTERROR_SUCCESS;

error:

    GPA_LOG_VERBOSE( "EventFwdD daemon does not appear to be running. Going to try start the eventfwdd daemon");
    GPACaptureOutput("/opt/likewise/bin/domainjoin-cli configure --enable eventfwdd", &pszBuf);

    if (IsNullOrEmptyString(pszBuf))
    {
        GPA_LOG_VERBOSE( "/opt/likewise/bin/domainjoin-cli configure --enable eventfwdd command returned no result");
    }
    else
    {
        GPA_LOG_VERBOSE( "/opt/likewise/bin/domainjoin-cli configure --enable eventfwdd command returned: %s", pszBuf);
    }

    goto cleanup;
}
