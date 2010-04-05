#include "includes.h"

static MAC_GP_BUNDLE_MODULE g_module;

static
int
LoadFunction(
    const char* pszFunctionName,
    void** function
    )
{
    int err = 0;
    void* target = NULL;

    target = dlsym(g_module.LibHandle, pszFunctionName);
    if (target == NULL) {
        g_module.pszError = dlerror();
        err = 1;
        goto cleanup;
    }
    *function = target;

cleanup:

    return err;
}

#define LOAD_MAC_GP_FUNCTION(function_name, function)   \
{                                                       \
    if (LoadFunction(function_name, function))          \
        goto cleanup;                                   \
}

// Called when library is loaded

__attribute__((constructor))

static void
mac_policy_init()
{
    memset(&g_module, 0, sizeof(MAC_GP_BUNDLE_MODULE));
    dlerror();
    g_module.LibHandle = dlopen(MAC_GP_BUNDLE_PATH, RTLD_LAZY);
    if (!g_module.LibHandle) {
       g_module.pszError = dlerror();
       goto cleanup;
    }
    
    LOAD_MAC_GP_FUNCTION("GPRequirePasswordForEachSecureSystemPreference",    (void**)&g_module.functions.GPRequirePasswordForEachSecureSystemPreference);
    LOAD_MAC_GP_FUNCTION("GPIsPasswordForEachSecureSystemPreferenceRequired", (void**)&g_module.functions.GPIsPasswordRequiredForEachSecureSystemPreference);
    LOAD_MAC_GP_FUNCTION("GPDisableAutomaticLogin",                           (void**)&g_module.functions.GPDisableAutomaticLogin);
    LOAD_MAC_GP_FUNCTION("GPIsAutomaticLoginDisabled",                        (void**)&g_module.functions.GPIsAutomaticLoginDisabled);
    LOAD_MAC_GP_FUNCTION("GPUseSecureVirtualMemory",                          (void**)&g_module.functions.GPUseSecureVirtualMemory);
    LOAD_MAC_GP_FUNCTION("GPIsSecureVirtualMemoryUsed",                       (void**)&g_module.functions.GPIsSecureVirtualMemoryUsed);
    LOAD_MAC_GP_FUNCTION("GPLogoutInMinutesOfInactivity",                     (void**)&g_module.functions.GPLogoutInMinutesOfInactivity);
    LOAD_MAC_GP_FUNCTION("GPGetMinutesOfInactivityToLogout",                  (void**)&g_module.functions.GPGetMinutesOfInactivityToLogout);
    LOAD_MAC_GP_FUNCTION("GPEditFirewallState",                               (void**)&g_module.functions.GPEditFirewallState);
    LOAD_MAC_GP_FUNCTION("GPIsFirewallEnabled",                               (void**)&g_module.functions.GPIsFirewallEnabled);
    LOAD_MAC_GP_FUNCTION("GPEnableFirewallLogging",                           (void**)&g_module.functions.GPEnableFirewallLogging);
    LOAD_MAC_GP_FUNCTION("GPIsFirewallLoggingEnabled",                        (void**)&g_module.functions.GPIsFirewallLoggingEnabled);
    LOAD_MAC_GP_FUNCTION("GPBlockUDPTraffic",                                 (void**)&g_module.functions.GPBlockUDPTraffic);
    LOAD_MAC_GP_FUNCTION("GPIsUDPTrafficBlocked",                             (void**)&g_module.functions.GPIsUDPTrafficBlocked);
    LOAD_MAC_GP_FUNCTION("GPEnableStealthMode",                               (void**)&g_module.functions.GPEnableStealthMode);
    LOAD_MAC_GP_FUNCTION("GPIsStealthModeEnabled",                            (void**)&g_module.functions.GPIsStealthModeEnabled);
    LOAD_MAC_GP_FUNCTION("GPEnableBluetoothControllerState",                  (void**)&g_module.functions.GPEnableBluetoothControllerState);
    LOAD_MAC_GP_FUNCTION("GPApplyBluetoothSetupAssistantSetting",             (void**)&g_module.functions.GPApplyBluetoothSetupAssistantSetting);
    LOAD_MAC_GP_FUNCTION("GPApplyBluetoothShareInternetConnectionSetting",    (void**)&g_module.functions.GPApplyBluetoothShareInternetConnectionSetting);
    LOAD_MAC_GP_FUNCTION("GPApplyAppleTalkSettings",                          (void**)&g_module.functions.GPApplyAppleTalkSettings);
    LOAD_MAC_GP_FUNCTION("GPApplyDNSSettings",                                (void**)&g_module.functions.GPApplyDNSSettings);
    LOAD_MAC_GP_FUNCTION("GPApplyEnergySaverSettings",                        (void**)&g_module.functions.GPApplyEnergySaverSettings);

cleanup:

    return;
}

// Called before library is unloaded

__attribute__((destructor))
static void
mac_policy_cleanup()
{
    if (g_module.LibHandle)
       dlclose(g_module.LibHandle);
       
    memset(&g_module, 0, sizeof(MAC_GP_BUNDLE_MODULE));
}

static
void
FreeFirewallSetting(
    void* pValue
    )
{
    // TODO
    LW_SAFE_FREE_MEMORY(pValue);
}

static
void
FreeMacSetting(
    void* pSetting
    )
{
    if (pSetting) {
        PMAC_SETTING pMacSetting = (PMAC_SETTING)pSetting;
       
        switch(pMacSetting->settingType) {
            case MAC_SETTING_BOOLEAN:
            case MAC_SETTING_NUMBER:
            {
                LW_SAFE_FREE_MEMORY(pMacSetting->pValue);
                LwFreeMemory(pMacSetting);
            }
            break;
            case MAC_SETTING_FIREWALL:
            case MAC_SETTING_APPLETALK:
            {
                FreeFirewallSetting(pMacSetting->pValue);
                LwFreeMemory(pMacSetting);
            }
            break;
            case MAC_SETTING_DNS:
            {
                PDNS_SETTING pDNSMacSetting = pMacSetting->pValue;
                LW_SAFE_FREE_STRING(pDNSMacSetting->pszServerAddresses);
                LW_SAFE_FREE_STRING(pDNSMacSetting->pszSearchDomains);
                LwFreeMemory(pDNSMacSetting);
            }
            break;
        }
    }
}

static
CENTERROR
CreateBooleanXMLNode(
    xmlDocPtr pDoc,
    PCSTR     pszNodeName,
    BOOLEAN   bValue,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    xmlNodePtr pCDATA = NULL;
    
    pNode = xmlNewNode(NULL, (const xmlChar*)SETTING_TAG);
    if (!pNode) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    if (NULL == xmlSetProp(pNode, (const xmlChar*)NAME_TAG, (const xmlChar*)pszNodeName)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    if (NULL == xmlSetProp(pNode, (const xmlChar*)TYPE_TAG, (const xmlChar*)TYPE_BOOLEAN)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    if (bValue) {
        pCDATA = xmlNewCDataBlock( pDoc,
                                   (const xmlChar*)TRUE_TAG,
                                   sizeof(TRUE_TAG)-1);
    } else {
        pCDATA = xmlNewCDataBlock( pDoc,
                                   (const xmlChar*)FALSE_TAG,
                                   sizeof(FALSE_TAG)-1);
    }

    if (!pCDATA) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_CDATA;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    if (NULL == xmlAddChild(pNode, pCDATA)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pCDATA = NULL;
    
    *ppNode = pNode;
    pNode = NULL;
    
error:

    if (pNode)
        xmlFreeNode(pNode);
       
    if (pCDATA)
        xmlFreeNode(pCDATA);

    return ceError;
}

static
CENTERROR
CreateNumberXMLNode(
    xmlDocPtr pDoc,
    PCSTR     pszNodeName,
    int       value,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    xmlNodePtr pCDATA = NULL;
    PSTR       pszBuf = NULL;
    
    pNode = xmlNewNode(NULL, (const xmlChar*)SETTING_TAG);
    if (!pNode) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    if (NULL == xmlSetProp(pNode, (const xmlChar*)NAME_TAG, (const xmlChar*)pszNodeName)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    if (NULL == xmlSetProp(pNode, (const xmlChar*)TYPE_TAG, (const xmlChar*)TYPE_NUMBER)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_SET_ATTR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    ceError = LwAllocateStringPrintf(&pszBuf, "%d", value);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pCDATA = xmlNewCDataBlock( pDoc,
                               (const xmlChar*)pszBuf,
                               strlen(pszBuf));
    if (!pCDATA) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_CDATA;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    if (NULL == xmlAddChild(pNode, pCDATA)) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pCDATA = NULL;
        
    *ppNode = pNode;
    pNode = NULL;
    
error:

    LW_SAFE_FREE_STRING(pszBuf);

    if (pNode)
        xmlFreeNode(pNode);
       
    if (pCDATA)
        xmlFreeNode(pCDATA);

    return ceError;
}

static
CENTERROR
GetFirstStringFromCDATA(
    xmlNodePtr pNode,
    PSTR* ppszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszValue = NULL;
    xmlChar* pszXmlContent = NULL;
    xmlNodePtr pChildNode = NULL;
    
    pChildNode = (pNode ? pNode->children : NULL);
    
    while (pChildNode) {
        if (pChildNode->type == XML_CDATA_SECTION_NODE) {
            pszXmlContent = xmlNodeGetContent(pChildNode);
            if (pszXmlContent) {
               ceError = LwAllocateString((const char*)pszXmlContent, &pszValue);
               BAIL_ON_CENTERIS_ERROR(ceError);
            }
            break;
        }
        pNode = pNode->next;
    }
    
    *ppszValue = pszValue;
    pszValue = NULL;
    
error:

    LW_SAFE_FREE_STRING(pszValue);
    
    if (pszXmlContent)
        xmlFree(pszXmlContent);

    return ceError;
}

static
CENTERROR
GetBooleanValue(
    xmlNodePtr pNode,
    PBOOLEAN pbValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszValue = NULL;
    BOOLEAN bValue = FALSE;
    
    ceError = GetFirstStringFromCDATA(pNode,  &pszValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if (!pszValue) {
        GPA_LOG_WARNING("Warning: No value attribute was found for mac setting node");
        goto done;
    }
    
    LwStripWhitespace(pszValue,1,1);
    
    bValue = (!strcasecmp(pszValue, TRUE_TAG) || !strcmp(pszValue, "1"));
    
done:
error:
    
    LW_SAFE_FREE_STRING(pszValue);
       
    *pbValue = bValue;
       
    return ceError;
}

static
CENTERROR
GetNumberValue(
    xmlNodePtr pNode,
    int* pValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszValue = NULL;
    int value = 0;
    
    ceError = GetFirstStringFromCDATA(pNode, &pszValue);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!pszValue) {
        GPA_LOG_WARNING("Warning: No value attribute was found for mac setting node");
        goto done;
    }
    
    value = atoi(pszValue);
    
done:
error:
    
    LW_SAFE_FREE_STRING(pszValue);
           
    *pValue = value;
       
    return ceError;
}

static
CENTERROR
BuildBooleanMacSetting(
    BOOLEAN bValue,
    PMAC_SETTING* ppSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    
    ceError = LwAllocateMemory(sizeof(MAC_SETTING), (PVOID*)&pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pSetting->settingType = MAC_SETTING_BOOLEAN;
    
    ceError = LwAllocateMemory(sizeof(BOOLEAN), (PVOID*)&pSetting->pValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    *((PBOOLEAN)pSetting->pValue) = bValue;
    
    *ppSetting = pSetting;
    pSetting = NULL;
    
error:

    if (pSetting) {
        FreeMacSetting(pSetting);
    }

    return ceError;
}

static
CENTERROR
BuildNumberMacSetting(
    int value,
    PMAC_SETTING* ppSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    
    ceError = LwAllocateMemory(sizeof(MAC_SETTING), (PVOID*)&pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pSetting->settingType = MAC_SETTING_NUMBER;
    
    ceError = LwAllocateMemory(sizeof(int), (PVOID*)&pSetting->pValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    *((int*)pSetting->pValue) = value;
    
    *ppSetting = pSetting;
    pSetting = NULL;
    
error:

    if (pSetting) {
        FreeMacSetting(pSetting);
    }

    return ceError;
}

static
CENTERROR
QueryAgent_RequirePasswordForEachSecureSystemPreference(
    xmlDocPtr   pDoc,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    int bValue = FALSE;
    int err = 0;
    
    err = g_module.functions.GPIsPasswordRequiredForEachSecureSystemPreference(&bValue);
    if (err) {
        GPA_LOG_ERROR("Failed to query system setting \"%s\". Error code: %d", REQUIRE_PASSWORD_FOR_EACH_SECURE_SYSTEM_PREFERENCE_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
    ceError = CreateBooleanXMLNode(pDoc, REQUIRE_PASSWORD_FOR_EACH_SECURE_SYSTEM_PREFERENCE_TAG, bValue, &pNode);
    BAIL_ON_CENTERIS_ERROR(ceError);
        
    *ppNode = pNode;
    pNode = NULL;

error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    return ceError;
}

static
CENTERROR
QueryXML_RequirePasswordForEachSecureSystemPreference(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    BOOLEAN bValue = FALSE;
    PSTR pszKey = NULL;
    
    ceError = GetBooleanValue(pNode, &bValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = LwAllocateString(REQUIRE_PASSWORD_FOR_EACH_SECURE_SYSTEM_PREFERENCE_TAG, &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = BuildBooleanMacSetting(bValue, &pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPAStableHashTableInsert(pTable, pszKey, pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pszKey = NULL;
    pSetting = NULL;
    
error:
   
    LW_SAFE_FREE_STRING(pszKey);

    if (pSetting)
        FreeMacSetting(pSetting);

    return ceError;
}

static
CENTERROR
Set_RequirePasswordForEachSecureSystemPreference(
    PMAC_SETTING pSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int err = 0;
    
    if (pSetting->settingType != MAC_SETTING_BOOLEAN) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    err = g_module.functions.GPRequirePasswordForEachSecureSystemPreference(*((PBOOLEAN)(pSetting->pValue)));
    if (err) {
        GPA_LOG_ERROR("Failed to set system setting \"%s\". Error code: %d", REQUIRE_PASSWORD_FOR_EACH_SECURE_SYSTEM_PREFERENCE_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

error:

    return ceError;
}

static
CENTERROR
QueryAgent_DisableAutomaticLogin(
    xmlDocPtr pDoc,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    int bValue = FALSE;
    int err = 0;
    
    err = g_module.functions.GPIsAutomaticLoginDisabled(&bValue);
    if (err) {
        GPA_LOG_ERROR("Failed to query system setting \"%s\". Error code: %d", ENABLE_AUTOMATIC_LOGIN_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
    ceError = CreateBooleanXMLNode(pDoc, ENABLE_AUTOMATIC_LOGIN_TAG, bValue, &pNode);
    BAIL_ON_CENTERIS_ERROR(ceError);
        
    *ppNode = pNode;
    pNode = NULL;

error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    return ceError;
}

static
CENTERROR
QueryXML_DisableAutomaticLogin(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    BOOLEAN bValue = FALSE;
    PSTR pszKey = NULL;
    
    ceError = GetBooleanValue(pNode, &bValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = LwAllocateString(ENABLE_AUTOMATIC_LOGIN_TAG, &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = BuildBooleanMacSetting(bValue, &pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPAStableHashTableInsert(pTable, pszKey, pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pszKey = NULL;
    pSetting = NULL;
    
error:
   
    LW_SAFE_FREE_STRING(pszKey);

    if (pSetting)
        FreeMacSetting(pSetting);

    return ceError;
}

static
CENTERROR
BuildAppleTalkMacSetting(
    xmlNodePtr pNode,
    PMAC_SETTING* ppSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    PAPPLETALK_SETTING pAppleTalkSetting = NULL;
    xmlChar *pszValue = NULL; 

    ceError = LwAllocateMemory(sizeof(MAC_SETTING), (PVOID*)&pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pSetting->settingType = MAC_SETTING_APPLETALK;

    ceError = LwAllocateMemory(sizeof(APPLETALK_SETTING), (PVOID*)&pAppleTalkSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);

    //Get AppleTalk Mode, NetworkID, NodeID
    while (pNode != NULL) {
        if ((!xmlStrcmp(pNode->name, (const xmlChar *)"AppleTalkMode"))) {
            pszValue = xmlGetProp(pNode,(const xmlChar*)"name");
            if ((!strcmp((const char*)pszValue, (const char*)"Manually"))) {
                pAppleTalkSetting->mode = 1;
            } else if ((!strcmp((const char*)pszValue, (const char*)"Automatically"))) {
                pAppleTalkSetting->mode = 2;
            } else if ((!strcmp((const char*)pszValue, (const char*)"OFF"))) {
                pAppleTalkSetting->mode = 3;
            }
        } else if ((!xmlStrcmp(pNode->name, (const xmlChar *)"NodeID"))) {
            pszValue = xmlGetProp(pNode,(const xmlChar*)"value");
            pAppleTalkSetting->nodeId = atoi((const char*)pszValue);
        } else if ((!xmlStrcmp(pNode->name, (const xmlChar *)"NetworkID"))) {
            pszValue = xmlGetProp(pNode,(const xmlChar*)"value");
            pAppleTalkSetting->networkId = atoi((const char*)pszValue);
        }

        if(pNode->xmlChildrenNode) {
            pNode = pNode->xmlChildrenNode;
        }

        if ( pszValue ) {
            xmlFree( pszValue );
            pszValue = NULL;
        }
        pNode = pNode->next;
    }

    pSetting->pValue = (PAPPLETALK_SETTING)pAppleTalkSetting;

    *ppSetting = pSetting;
    pSetting = NULL;
    
error:

    if (pSetting)
        FreeMacSetting(pSetting);

    if (pszValue)
        xmlFree(pszValue);

    return ceError;
}

static
CENTERROR
QueryXML_AppleTalk(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    PSTR pszKey = NULL;
    
    ceError = LwAllocateString(APPLETALK_TAG, &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = BuildAppleTalkMacSetting(pNode,&pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAStableHashTableInsert(pTable, pszKey, pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszKey = NULL;
    pSetting = NULL;

    
error:
   
    LW_SAFE_FREE_STRING(pszKey);

    if (pSetting)
        LwFreeMemory(pSetting);

    return ceError;
}

static
CENTERROR
BuildDNSSetting(
    xmlNodePtr pNode,
    PMAC_SETTING* ppSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PMAC_SETTING pSetting = NULL;
    PDNS_SETTING pDNSSetting = NULL;

    xmlChar *pszNodeText = NULL;
    xmlChar *pszValue = NULL;

    ceError = LwAllocateMemory( sizeof(MAC_SETTING),
                                (PVOID*)&pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pSetting->settingType = MAC_SETTING_DNS;

    ceError = LwAllocateMemory( sizeof(DNS_SETTING),
                                (PVOID*)&pDNSSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while( pNode != NULL ) {
        if(( !xmlStrcmp( pNode->name,
                         (const xmlChar *)DNS_SERVERS_TAG) ||
             !xmlStrcmp( pNode->name,
                         (const xmlChar *)DNS_SEARCH_DOMAINS_TAG))) {

            pszNodeText = xmlNodeGetContent( pNode );

            if(( !xmlStrcmp( pNode->name,
                             (const xmlChar *)DNS_SERVERS_TAG))) {
                ceError = LwAllocateString( (char *)pszNodeText,
                                            &pDNSSetting->pszServerAddresses);
                BAIL_ON_CENTERIS_ERROR(ceError);
            } else if(( !xmlStrcmp( pNode->name,
                                    (const xmlChar *)DNS_SEARCH_DOMAINS_TAG))) {
                ceError = LwAllocateString( (char *)pszNodeText,
                                            &pDNSSetting->pszSearchDomains);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            if ( pszNodeText ) {
                xmlFree( pszNodeText );
                pszNodeText = NULL;
            }

            if ( pszValue ) {
                xmlFree( pszValue );
                pszValue = NULL;
            }

            pNode = pNode->next->next;
            continue;
        }

        if(pNode->xmlChildrenNode){
            pNode = pNode->xmlChildrenNode;
        }
        pNode = pNode->next;
    }

    pSetting->pValue = (PDNS_SETTING)pDNSSetting;
    
    *ppSetting = pSetting;
    pSetting = NULL;

error:

    if (pSetting) {
        FreeMacSetting(pSetting);
        pSetting = NULL;
    }

    if ( pszNodeText ) {
        xmlFree( pszNodeText );
        pszNodeText = NULL;
    }

    if ( pszValue ) {
        xmlFree( pszValue );
        pszValue = NULL;
    }

    return ceError;

}

static
CENTERROR
QueryXML_DNSSettings(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    PSTR pszKey = NULL;
    
    ceError = LwAllocateString( DNS_SETTING_TAG,
                                &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = BuildDNSSetting( pNode,                               
                               &pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPAStableHashTableInsert( pTable,
                                       pszKey,
                                       pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pszKey = NULL;
    pSetting = NULL;
    
error:
   
    LW_SAFE_FREE_STRING(pszKey);

    if (pSetting) {
        FreeMacSetting(pSetting);
        pSetting = NULL;
    }

    return ceError;
}

static
CENTERROR
QueryXML_EnergySaverSettings(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable,
    PSTR pszSettingName,
    int nBoolean
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    int value = 0;
    BOOLEAN bValue = FALSE;
    PSTR pszKey = NULL;
    
    if (nBoolean) {
        ceError = GetBooleanValue(pNode, &bValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
    
        if (bValue) {
            if( !strcmp(pszSettingName, ENERGY_SAVER_DISK_SLEEP_TIMER)) value = 10;
            else value = 1;
        } else {
            if( !strcmp(pszSettingName, ENERGY_SAVER_DISK_SLEEP_TIMER)) value = 180;
            else value = 0;
        }
    } else {
        ceError = GetNumberValue(pNode, &value);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    ceError = LwAllocateString(pszSettingName, &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = BuildNumberMacSetting(value, &pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError); 

    ceError = GPAStableHashTableInsert(pTable, pszKey, pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pszKey = NULL;
    pSetting = NULL;
    
error:
   
    LW_SAFE_FREE_STRING(pszKey);

    if (pSetting)
        FreeMacSetting(pSetting);

    return ceError;
}

static
CENTERROR
QueryXML_EnableBluetoothController(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    BOOLEAN bValue = FALSE;
    PSTR pszKey = NULL;
    
    ceError = GetBooleanValue(pNode, &bValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = LwAllocateString(ENABLE_BLUETOOTH_TAG, &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = BuildBooleanMacSetting(bValue, &pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPAStableHashTableInsert(pTable, pszKey, pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pszKey = NULL;
    pSetting = NULL;
    
error:
   
    LW_SAFE_FREE_STRING(pszKey);

    if (pSetting)
        FreeMacSetting(pSetting);

    return ceError;
}

static
CENTERROR
QueryXML_BluetoothSetupAssistant(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    BOOLEAN bValue = FALSE;
    PSTR pszKey = NULL;
    
    ceError = GetBooleanValue(pNode, &bValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = LwAllocateString(BLUETOOTH_SETUP_ASSISTANT_TAG, &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = BuildBooleanMacSetting(bValue, &pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPAStableHashTableInsert(pTable, pszKey, pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pszKey = NULL;
    pSetting = NULL;
    
error:
   
    LW_SAFE_FREE_STRING(pszKey);

    if (pSetting)
        FreeMacSetting(pSetting);

    return ceError;
}

static
CENTERROR
QueryXML_BluetoothShareInternetConnection(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    BOOLEAN bValue = FALSE;
    PSTR pszKey = NULL;
    
    ceError = GetBooleanValue(pNode, &bValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = LwAllocateString(BLUETOOTH_SHARE_INTERNET_CONNECTION_TAG, &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = BuildBooleanMacSetting(bValue, &pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPAStableHashTableInsert(pTable, pszKey, pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pszKey = NULL;
    pSetting = NULL;
    
error:
   
    LW_SAFE_FREE_STRING(pszKey);

    if (pSetting)
        FreeMacSetting(pSetting);

    return ceError;
}

static
CENTERROR
Set_DisableAutomaticLogin(
    PMAC_SETTING pSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int err = 0;
    
    if (pSetting->settingType != MAC_SETTING_BOOLEAN) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    err = g_module.functions.GPDisableAutomaticLogin(*((PBOOLEAN)(pSetting->pValue)));
    if (err) {
        GPA_LOG_ERROR("Failed to set system setting \"%s\". Error code: %d", ENABLE_AUTOMATIC_LOGIN_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
error:

    return ceError;
}

static
CENTERROR
QueryAgent_UseSecureVirtualMemory(
    xmlDocPtr pDoc,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    int bValue = FALSE;
    int err = 0;
    
    err = g_module.functions.GPIsSecureVirtualMemoryUsed(&bValue);
    if (err) {
        GPA_LOG_ERROR("Failed to query system setting \"%s\". Error code: %d", USE_SECURE_VIRTUAL_MEMORY_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
    ceError = CreateBooleanXMLNode(pDoc, USE_SECURE_VIRTUAL_MEMORY_TAG, bValue, &pNode);
    BAIL_ON_CENTERIS_ERROR(ceError);
        
    *ppNode = pNode;
    pNode = NULL;
    
error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    return ceError;
}

static
CENTERROR
QueryXML_UseSecureVirtualMemory(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    BOOLEAN bValue = FALSE;
    PSTR pszKey = NULL;
    
    ceError = GetBooleanValue(pNode, &bValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = LwAllocateString(USE_SECURE_VIRTUAL_MEMORY_TAG, &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = BuildBooleanMacSetting(bValue, &pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPAStableHashTableInsert(pTable, pszKey, pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pszKey = NULL;
    pSetting = NULL;
    
error:
   
    LW_SAFE_FREE_STRING(pszKey);

    if (pSetting)
        FreeMacSetting(pSetting);

    return ceError;
}

static
CENTERROR
Set_UseSecureVirtualMemory(
    PMAC_SETTING pSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int err = 0;
    
    if (pSetting->settingType != MAC_SETTING_BOOLEAN) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    err = g_module.functions.GPUseSecureVirtualMemory(*((PBOOLEAN)(pSetting->pValue)));
    if (err) {
        GPA_LOG_ERROR("Failed to set system setting \"%s\". Error code: %d", USE_SECURE_VIRTUAL_MEMORY_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
   
error:

    return ceError;
}

static
CENTERROR
QueryAgent_LogoutInMinutesOfInactivity(
    xmlDocPtr pDoc,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    int value = FALSE;
    int err = 0;
    
    err = g_module.functions.GPGetMinutesOfInactivityToLogout(&value);
    if (err) {
        GPA_LOG_ERROR("Failed to query system setting \"%s\". Error code: %d", MINUTES_OF_INACTIVITY_TO_LOGOUT_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
    ceError = CreateNumberXMLNode(pDoc, MINUTES_OF_INACTIVITY_TO_LOGOUT_TAG, value, &pNode);
    BAIL_ON_CENTERIS_ERROR(ceError);
        
    *ppNode = pNode;
    pNode = NULL;
    
error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    return ceError;
}

static
CENTERROR
QueryXML_LogoutInMinutesOfInactivity(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    int value = 0;
    PSTR pszKey = NULL;
    
    ceError = GetNumberValue(pNode, &value);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = LwAllocateString(MINUTES_OF_INACTIVITY_TO_LOGOUT_TAG, &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
	value = value * 60;
	
    ceError = BuildNumberMacSetting(value, &pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
	    
    ceError = GPAStableHashTableInsert(pTable, pszKey, pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pszKey = NULL;
    pSetting = NULL;
    
error:
   
    LW_SAFE_FREE_STRING(pszKey);

    if (pSetting)
        FreeMacSetting(pSetting);

    return ceError;
}

static
CENTERROR
Set_LogoutInMinutesOfInactivity(
    PMAC_SETTING pSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int err = 0;
    
    if (pSetting->settingType != MAC_SETTING_NUMBER) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    err = g_module.functions.GPLogoutInMinutesOfInactivity(*((int*)(pSetting->pValue)));
    if (err) {
        GPA_LOG_ERROR("Failed to set system setting \"%s\". Error code: %d", MINUTES_OF_INACTIVITY_TO_LOGOUT_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
error:

    return ceError;
}

static
CENTERROR
QueryAgent_EnableFirewall(
    xmlDocPtr pDoc,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    int bValue = FALSE;
    int err = 0;
    
    err = g_module.functions.GPIsFirewallEnabled(&bValue);
    if (err) {
        GPA_LOG_ERROR("Failed to query system setting \"%s\". Error code: %d", ENABLE_FIREWALL_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
    ceError = CreateBooleanXMLNode(pDoc, ENABLE_FIREWALL_TAG, bValue, &pNode);
    BAIL_ON_CENTERIS_ERROR(ceError);
        
    *ppNode = pNode;
    pNode = NULL;
    
error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    return ceError;
}

static
CENTERROR
QueryXML_EnableFirewall(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    BOOLEAN bValue = FALSE;
    PSTR pszKey = NULL;
    
    ceError = GetBooleanValue(pNode, &bValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = LwAllocateString(ENABLE_FIREWALL_TAG, &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = BuildBooleanMacSetting(bValue, &pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPAStableHashTableInsert(pTable, pszKey, pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pszKey = NULL;
    pSetting = NULL;
    
error:
   
    LW_SAFE_FREE_STRING(pszKey);

    if (pSetting)
        FreeMacSetting(pSetting);

    return ceError;
}

static
CENTERROR
Set_EnableFirewall(
    PMAC_SETTING pSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int err = 0;
    
    if (pSetting->settingType != MAC_SETTING_BOOLEAN) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    err = g_module.functions.GPEditFirewallState(*((PBOOLEAN)(pSetting->pValue)));
    if (err) {
        GPA_LOG_ERROR("Failed to set system setting \"%s\". Error code: %d", ENABLE_FIREWALL_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
error:

    return ceError;
}

static
CENTERROR
QueryAgent_EnableFirewallLogging(
    xmlDocPtr pDoc,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    int bValue = FALSE;
    int err = 0;
    
    err = g_module.functions.GPIsFirewallLoggingEnabled(&bValue);
    if (err) {
        GPA_LOG_ERROR("Failed to query system setting \"%s\". Error code: %d", ENABLE_FIREWALL_LOGGING_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
    ceError = CreateBooleanXMLNode(pDoc, ENABLE_FIREWALL_LOGGING_TAG, bValue, &pNode);
    BAIL_ON_CENTERIS_ERROR(ceError);
        
    *ppNode = pNode;
    pNode = NULL;

error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    return ceError;
}

static
CENTERROR
QueryXML_EnableFirewallLogging(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    BOOLEAN bValue = FALSE;
    PSTR pszKey = NULL;
    
    ceError = GetBooleanValue(pNode, &bValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = LwAllocateString(ENABLE_FIREWALL_LOGGING_TAG, &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = BuildBooleanMacSetting(bValue, &pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPAStableHashTableInsert(pTable, pszKey, pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pszKey = NULL;
    pSetting = NULL;
    
error:
   
    LW_SAFE_FREE_STRING(pszKey);

    if (pSetting)
        FreeMacSetting(pSetting);

    return ceError;
}

static
CENTERROR
Set_EnableFirewallLogging(
    PMAC_SETTING pSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int err = 0;
    
    if (pSetting->settingType != MAC_SETTING_BOOLEAN) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    err = g_module.functions.GPEnableFirewallLogging(*((PBOOLEAN)(pSetting->pValue)));
    if (err) {
        GPA_LOG_ERROR("Failed to set system setting \"%s\". Error code: %d", ENABLE_FIREWALL_LOGGING_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
error:

    return ceError;
}

static
CENTERROR
QueryAgent_BlockUDPTraffic(
    xmlDocPtr pDoc,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    int bValue = FALSE;
    int err = 0;
    
    err = g_module.functions.GPIsUDPTrafficBlocked(&bValue);
    if (err) {
        GPA_LOG_ERROR("Failed to query system setting \"%s\". Error code: %d", BLOCK_UDP_TRAFFIC_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
    ceError = CreateBooleanXMLNode(pDoc, BLOCK_UDP_TRAFFIC_TAG, bValue, &pNode);
    BAIL_ON_CENTERIS_ERROR(ceError);
        
    *ppNode = pNode;
    pNode = NULL;

error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    return ceError;
}

static
CENTERROR
QueryXML_BlockUDPTraffic(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    BOOLEAN bValue = FALSE;
    PSTR pszKey = NULL;
    
    ceError = GetBooleanValue(pNode, &bValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = LwAllocateString(BLOCK_UDP_TRAFFIC_TAG, &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = BuildBooleanMacSetting(bValue, &pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPAStableHashTableInsert(pTable, pszKey, pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pszKey = NULL;
    pSetting = NULL;
    
error:
   
    LW_SAFE_FREE_STRING(pszKey);

    if (pSetting)
        FreeMacSetting(pSetting);

    return ceError;
}

static
CENTERROR
Set_BlockUDPTraffic(
    PMAC_SETTING pSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int err = 0;
    
    if (pSetting->settingType != MAC_SETTING_BOOLEAN) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    err = g_module.functions.GPBlockUDPTraffic(*((PBOOLEAN)(pSetting->pValue)));
    if (err) {
        GPA_LOG_ERROR("Failed to set system setting \"%s\". Error code: %d", BLOCK_UDP_TRAFFIC_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
error:

    return ceError;
}

static
CENTERROR
QueryAgent_EnableStealthMode(
    xmlDocPtr pDoc,
    xmlNodePtr* ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pNode = NULL;
    int bValue = FALSE;
    int err = 0;
    
    err = g_module.functions.GPIsStealthModeEnabled(&bValue);
    if (err) {
        GPA_LOG_ERROR("Failed to query system setting \"%s\". Error code: %d", ENABLE_STEALTH_MODE_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
    ceError = CreateBooleanXMLNode(pDoc, ENABLE_STEALTH_MODE_TAG, bValue, &pNode);
    BAIL_ON_CENTERIS_ERROR(ceError);
        
    *ppNode = pNode;
    pNode = NULL;
    
error:

    if (pNode) {
        xmlFreeNode(pNode);
    }

    return ceError;
}

static
CENTERROR
QueryXML_EnableStealthMode(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMAC_SETTING pSetting = NULL;
    BOOLEAN bValue = FALSE;
    PSTR pszKey = NULL;
    
    ceError = GetBooleanValue(pNode, &bValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = LwAllocateString(ENABLE_STEALTH_MODE_TAG, &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = BuildBooleanMacSetting(bValue, &pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPAStableHashTableInsert(pTable, pszKey, pSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pszKey = NULL;
    pSetting = NULL;
    
error:
   
    LW_SAFE_FREE_STRING(pszKey);

    if (pSetting)
        FreeMacSetting(pSetting);

    return ceError;
}

static
CENTERROR
Set_EnableStealthMode(
    PMAC_SETTING pSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int err = 0;
    
    if (pSetting->settingType != MAC_SETTING_BOOLEAN) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    err = g_module.functions.GPEnableStealthMode(*((PBOOLEAN)(pSetting->pValue)));
    if (err) {
        GPA_LOG_ERROR("Failed to set system setting \"%s\". Error code: %d", ENABLE_STEALTH_MODE_TAG, err);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
error:

    return ceError;
}

static
CENTERROR
HUPBluetoothDaemon()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOG_VERBOSE("Sending SIGHUP to bluetooth daemon - blued.");

    ceError = GPARunCommand("ps x | grep blued | grep -v grep > /dev/null 2>&1");
    if (ceError == CENTERROR_SUCCESS) {
        ceError = GPARunCommand("killall -HUP blued > /dev/null 2>&1");
        if (ceError != CENTERROR_SUCCESS) {
            GPA_LOG_ERROR("Failed to HUP blued as it is not running...");
            ceError = CENTERROR_SUCCESS;
        }
    } else {
        ceError = CENTERROR_SUCCESS;
    }

    return ceError;
}

static
CENTERROR
Set_Bluetooth(
    PMAC_SETTING pSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int err = 0;
    
    if (pSetting->settingType != MAC_SETTING_BOOLEAN) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    err = g_module.functions.GPEnableBluetoothControllerState(*((PBOOLEAN)(pSetting->pValue)));
    switch(err)  {
        case CENTERROR_SUCCESS:
        {
            ceError = HUPBluetoothDaemon();
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        break;    
        case GP_MAC_ITF_SETTING_NOT_SUPPORTED:
        {
            GPA_LOG_ALWAYS("\"%s\" setting is not supported on this system. Hence skipping...", ENABLE_BLUETOOTH_TAG);
            ceError = CENTERROR_SUCCESS;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        break;
        default:
        {
            GPA_LOG_ERROR("Failed to set system setting \"%s\". Error code: %d", ENABLE_BLUETOOTH_TAG, err);
            ceError = CENTERROR_SUCCESS;
            goto error;
        }
    }
    
error:

    return ceError;
}

static
CENTERROR
ApplyBluetoothSetupAssistant(
    PMAC_SETTING pSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int err = 0;
    
    if (pSetting->settingType != MAC_SETTING_BOOLEAN) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    err = g_module.functions.GPApplyBluetoothSetupAssistantSetting(*((PBOOLEAN)(pSetting->pValue)));
    switch(err)  {
        case CENTERROR_SUCCESS:
        {
            ceError = HUPBluetoothDaemon();
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        break;
        case GP_MAC_ITF_SETTING_NOT_SUPPORTED:
        {
            GPA_LOG_ALWAYS("\"%s\" setting is not supported on this system. Hence skipping...", BLUETOOTH_SETUP_ASSISTANT_TAG);
            ceError = CENTERROR_SUCCESS;
            BAIL_ON_CENTERIS_ERROR(ceError);			
        }
        break;
        default:
        {
            GPA_LOG_ERROR("Failed to set system setting \"%s\". Error code: %d", BLUETOOTH_SETUP_ASSISTANT_TAG, err);
            ceError = CENTERROR_SUCCESS;
            goto error;
        }
    }
	
error:

    return ceError;
}

static
CENTERROR
ApplyBluetoothShareInternetConnection(
    PMAC_SETTING pSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int err = 0;
    
    if (pSetting->settingType != MAC_SETTING_BOOLEAN) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    err = g_module.functions.GPApplyBluetoothShareInternetConnectionSetting(*((PBOOLEAN)(pSetting->pValue)));
    switch(err)  {
        case CENTERROR_SUCCESS:
        {
            ceError = HUPBluetoothDaemon();
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        break;
        case GP_MAC_ITF_SETTING_NOT_SUPPORTED:
        {
            GPA_LOG_ALWAYS("\"%s\" setting is not supported on this system. Hence skipping...", BLUETOOTH_SHARE_INTERNET_CONNECTION_TAG);
            ceError = CENTERROR_SUCCESS;
            BAIL_ON_CENTERIS_ERROR(ceError);			
        }
        break;
        default:
        {
            GPA_LOG_ERROR("Failed to set system setting \"%s\". Error code: %d", BLUETOOTH_SHARE_INTERNET_CONNECTION_TAG, err);
            ceError = CENTERROR_SUCCESS;
            goto error;
        }
    }
    
error:

    return ceError;
}

#define MANUAL 1
#define AUTO   2
#define OFF    3

static
CENTERROR
ApplyAppleTalkSettings(
    PAPPLETALK_SETTING pSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int err = 0;
    
    switch(pSetting->mode) {
        case MANUAL:
        {
            err = g_module.functions.GPApplyAppleTalkSettings( pSetting->mode, 
                                                               pSetting->nodeId, 
                                                               pSetting->networkId);
            if (err) {
                GPA_LOG_ERROR("Failed to set system setting \"%s\". Error code: %d", APPLETALK_TAG, err);
                ceError = CENTERROR_SUCCESS;
                goto error;
            }
            break;
        }
        case AUTO:
        case OFF:
        {
            err = g_module.functions.GPApplyAppleTalkSettings(pSetting->mode,0,0);
            if (err) {
                GPA_LOG_ERROR("Failed to set system setting \"%s\". Error code: %d", APPLETALK_TAG, err);
                ceError = CENTERROR_SUCCESS;
                goto error;
            }
            break;
        }
    }
    
error:

    return ceError;
}

static
CENTERROR
ApplyDNSSettings(                          
    PDNS_SETTING pSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    int err = 0;

    err = g_module.functions.GPApplyDNSSettings( pSetting->pszServerAddresses,
                                                 pSetting->pszSearchDomains);

    if ( err ) {
        GPA_LOG_ERROR( "Failed to set system setting \"%s\". Error code: %d", DNS_SERVERS_TAG, err );
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

error:

    return ceError;
}

static
CENTERROR
ApplyEnergySaverSettings(
    PMAC_SETTING pSetting,
    PSTR pszSettingName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int err = 0;
    
    if (pSetting->settingType != MAC_SETTING_NUMBER) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    err = g_module.functions.GPApplyEnergySaverSettings( *((int*)(pSetting->pValue)),
                                                         pszSettingName);
    switch(err)  {
        case CENTERROR_SUCCESS:
            break;
        case GP_MAC_ITF_SETTING_NOT_SUPPORTED:
        {
            GPA_LOG_ALWAYS("\"%s\" setting is not supported on this system. Hence skipping...", pszSettingName);
            ceError = CENTERROR_SUCCESS;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        break;
        default:
            GPA_LOG_ERROR("Failed to set system setting \"%s\". Error code: %d", pszSettingName, err);
            ceError = CENTERROR_SUCCESS;
            goto error;
    }
    
error:

    return ceError;
}

/*
   Mac Setting Query Agents
   
   Each agent is responsible for querying a particular system attribute
   and saves its configuration in xml.
 */

typedef CENTERROR (*PFNMacSettingQueryAgent)(xmlDocPtr pDoc, xmlNodePtr* ppNode);

PFNMacSettingQueryAgent gMacQueryAgents[] =
{
    &QueryAgent_RequirePasswordForEachSecureSystemPreference,
    &QueryAgent_DisableAutomaticLogin,
    &QueryAgent_UseSecureVirtualMemory,
    &QueryAgent_LogoutInMinutesOfInactivity,
    &QueryAgent_EnableFirewall,
    &QueryAgent_EnableFirewallLogging,
    &QueryAgent_BlockUDPTraffic,
    &QueryAgent_EnableStealthMode
};

static
CENTERROR
CreateSystemDefaultConfiguration()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE* fp = NULL;
    xmlDocPtr pDoc = NULL;
    xmlNodePtr pRoot = NULL;
    xmlNodePtr pElem = NULL;
    BOOLEAN bDeleteRootNode = FALSE;
    int nAgents = 0;
    int iAgent = 0;
    
    pDoc = xmlNewDoc((const xmlChar*)"1.0");
    if (pDoc == NULL) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_DOC;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    pRoot = xmlNewDocNode(pDoc, NULL, (const xmlChar*)MACPOLICIES_TAG, NULL);
    if (pRoot == NULL) {
        ceError = CENTERROR_GP_XML_FAILED_TO_CREATE_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    bDeleteRootNode = TRUE;
    
    xmlDocSetRootElement(pDoc, pRoot);
    
    bDeleteRootNode = FALSE;
    
    /* Add xml settings for each system configuration */
    nAgents = sizeof(gMacQueryAgents)/sizeof(PFNMacSettingQueryAgent);
    for(iAgent = 0; iAgent < nAgents; iAgent++) {
        ceError = gMacQueryAgents[iAgent](pDoc, &pElem);
        BAIL_ON_CENTERIS_ERROR(ceError);
       
        if (pElem && NULL == xmlAddChild(pRoot, pElem)) {
            /* Log the error and continue for next setting query */
            GPA_LOG_ERROR("Failed to add XML node. Error code: %d", CENTERROR_GP_XML_FAILED_TO_ADD_NODE);
            ceError = CENTERROR_SUCCESS;
        }
        pElem = NULL;
    }
    
    ceError = GPAOpenFile(MAC_GP_SYSTEM_BACKUP, "w", &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if (xmlDocDump(fp, pDoc) < 0) {
        ceError = CENTERROR_GP_XML_FAILED_TO_WRITE_DOC;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    if (pDoc) {
        xmlFreeDoc(pDoc);
        pDoc = NULL;
    }
    
    ceError = GPACloseFile(fp);
    BAIL_ON_CENTERIS_ERROR(ceError);
    fp = NULL;
    
error:

    if (fp) {
        GPACloseFile(fp);
    }
    
    if (pElem) {
        xmlFreeNode(pElem);
    }
    
    if (bDeleteRootNode && pRoot) {
        xmlFreeNode(pRoot);
    }
    
    if (pDoc) {
        xmlFreeDoc(pDoc);
        pDoc = NULL;
    }

    return ceError;
}

static
CENTERROR
EnsureSystemSnapshot()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bExists = FALSE;
    
    ceError = GPACheckDirectoryExists(MAC_GP_CACHE_DIR_PATH, &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if (!bExists) {
        ceError = LwCreateDirectory(MAC_GP_CACHE_DIR_PATH, S_IRWXU);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    ceError = GPACheckFileExists(MAC_GP_SYSTEM_BACKUP, &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if (!bExists) {
        ceError = CreateSystemDefaultConfiguration();
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
error:

    return ceError;
}


static
CENTERROR
MapMacSetting(
    xmlNodePtr pNode,
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlChar* pszSettingName = NULL;
    
    pszSettingName = xmlGetProp( pNode, 
                                 (xmlChar*)NAME_TAG);
    if (!pszSettingName) {
        GPA_LOG_WARNING("Warning: The name attribute is missing for the mac setting type");
        goto done;
    }

	LwStripWhitespace((PSTR)pszSettingName,1,1);
    
    // TODO: Use a hash table to look up the function pointer.
    if (!strcmp((PCSTR)pszSettingName, REQUIRE_PASSWORD_FOR_EACH_SECURE_SYSTEM_PREFERENCE_TAG)) {
        ceError = QueryXML_RequirePasswordForEachSecureSystemPreference(pNode, pTable);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, ENABLE_AUTOMATIC_LOGIN_TAG)) {
        ceError = QueryXML_DisableAutomaticLogin(pNode, pTable);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, USE_SECURE_VIRTUAL_MEMORY_TAG)) {
        ceError = QueryXML_UseSecureVirtualMemory(pNode, pTable);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, MINUTES_OF_INACTIVITY_TO_LOGOUT_TAG)) {
        ceError = QueryXML_LogoutInMinutesOfInactivity(pNode, pTable);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, ENABLE_FIREWALL_TAG)) {
        ceError = QueryXML_EnableFirewall(pNode, pTable);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, ENABLE_FIREWALL_LOGGING_TAG)) {
        ceError = QueryXML_EnableFirewallLogging(pNode, pTable);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, BLOCK_UDP_TRAFFIC_TAG)) {
        ceError = QueryXML_BlockUDPTraffic(pNode, pTable);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, ENABLE_STEALTH_MODE_TAG)) {
        ceError = QueryXML_EnableStealthMode(pNode, pTable);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, ENABLE_BLUETOOTH_TAG)) {
        ceError = QueryXML_EnableBluetoothController(pNode, pTable);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, BLUETOOTH_SETUP_ASSISTANT_TAG)) {
        ceError = QueryXML_BluetoothSetupAssistant(pNode, pTable);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, BLUETOOTH_SHARE_INTERNET_CONNECTION_TAG)) {
        ceError = QueryXML_BluetoothShareInternetConnection(pNode, pTable);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, APPLETALK_TAG)) {
        ceError = QueryXML_AppleTalk(pNode, pTable);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, DNS_SETTING_TAG)) {
        ceError = QueryXML_DNSSettings(pNode, pTable);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, ENERGY_SAVER_SYSTEM_SLEEP_TIMER)) {
        ceError = QueryXML_EnergySaverSettings(pNode, pTable, ENERGY_SAVER_SYSTEM_SLEEP_TIMER, 0);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, ENERGY_SAVER_DISPLAY_SLEEP_TIMER)) {
        ceError = QueryXML_EnergySaverSettings(pNode, pTable, ENERGY_SAVER_DISPLAY_SLEEP_TIMER, 0);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, ENERGY_SAVER_DISK_SLEEP_TIMER)) {
        ceError = QueryXML_EnergySaverSettings(pNode, pTable, ENERGY_SAVER_DISK_SLEEP_TIMER, 1);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, ENERGY_SAVER_WAKE_ON_MODEM_RING)) {
        ceError = QueryXML_EnergySaverSettings(pNode, pTable, ENERGY_SAVER_WAKE_ON_MODEM_RING, 1);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, ENERGY_SAVER_WAKE_ON_LAN)) {
        ceError = QueryXML_EnergySaverSettings(pNode, pTable, ENERGY_SAVER_WAKE_ON_LAN, 1);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, ENERGY_SAVER_SLEEP_ON_POWER_BUTTON)) {
        ceError = QueryXML_EnergySaverSettings(pNode, pTable, ENERGY_SAVER_SLEEP_ON_POWER_BUTTON, 1);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp((PCSTR)pszSettingName, ENERGY_SAVER_AUTO_RESTART_ON_POWER_LOSS)) {
        ceError = QueryXML_EnergySaverSettings(pNode, pTable, ENERGY_SAVER_AUTO_RESTART_ON_POWER_LOSS, 1);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else {
        GPA_LOG_WARNING("Warning: No handler found for setting %s", pszSettingName);
    }

done:
error:

    if (pszSettingName)
        xmlFree(pszSettingName);

    return ceError;
}

static
CENTERROR
ReadSystemSnapshot(
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlDocPtr pDoc = NULL;
    xmlNodeSetPtr pXmlNodeSetPtr = NULL;
    BOOLEAN bExists = FALSE;
    
    ceError = GPACheckFileExists( MAC_GP_SYSTEM_BACKUP, 
                                 &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if (!bExists)
        goto done;
    
    pDoc = xmlReadFile( MAC_GP_SYSTEM_BACKUP, 
                        NULL, 
                        0);
    if (pDoc == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    ceError = GPOXmlSelectNodes( (xmlNodePtr)pDoc,
                                 MACPOLICIES_TAG "/" SETTING_TAG,
                                 &pXmlNodeSetPtr);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if (pXmlNodeSetPtr && (pXmlNodeSetPtr->nodeNr > 0)) {
        int iNode = 0;
       
        for (iNode = 0; iNode < pXmlNodeSetPtr->nodeNr; iNode++) {
            ceError = MapMacSetting( pXmlNodeSetPtr->nodeTab[iNode], 
                                     pTable);
            BAIL_ON_CENTERIS_ERROR(ceError);
       }
    }
    
done:
error:

    if (pXmlNodeSetPtr)
        xmlXPathFreeNodeSet(pXmlNodeSetPtr);
       
    if (pDoc)
        xmlFreeDoc(pDoc);

    return ceError;
}

//TODO: have a common function pass pvoid
static
CENTERROR
ApplyPolicy(
    PCSTR pszSettingName,
    PMAC_SETTING pSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    
    // TODO: Use a hash table to look up the function pointer.
    if (!strcmp(pszSettingName, REQUIRE_PASSWORD_FOR_EACH_SECURE_SYSTEM_PREFERENCE_TAG)) {
        ceError = Set_RequirePasswordForEachSecureSystemPreference(pSetting);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, ENABLE_AUTOMATIC_LOGIN_TAG)) {
        ceError = Set_DisableAutomaticLogin(pSetting);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, USE_SECURE_VIRTUAL_MEMORY_TAG)) {
        ceError = Set_UseSecureVirtualMemory(pSetting);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, MINUTES_OF_INACTIVITY_TO_LOGOUT_TAG)) {
        ceError = Set_LogoutInMinutesOfInactivity(pSetting);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, ENABLE_FIREWALL_TAG)) {
        ceError = Set_EnableFirewall(pSetting);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, ENABLE_FIREWALL_LOGGING_TAG)) {
        ceError = Set_EnableFirewallLogging(pSetting);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, BLOCK_UDP_TRAFFIC_TAG)) {
        ceError = Set_BlockUDPTraffic(pSetting);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, ENABLE_STEALTH_MODE_TAG)) {
        ceError = Set_EnableStealthMode(pSetting);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, ENABLE_BLUETOOTH_TAG)) {
        ceError = Set_Bluetooth(pSetting);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, BLUETOOTH_SETUP_ASSISTANT_TAG)) {
        ceError = ApplyBluetoothSetupAssistant(pSetting);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, BLUETOOTH_SHARE_INTERNET_CONNECTION_TAG)) {
        ceError = ApplyBluetoothShareInternetConnection(pSetting);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, APPLETALK_TAG)) {
        ceError = ApplyAppleTalkSettings(pSetting->pValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, DNS_SETTING_TAG)) {
        ceError = ApplyDNSSettings(pSetting->pValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, ENERGY_SAVER_SYSTEM_SLEEP_TIMER)) {
        ceError = ApplyEnergySaverSettings(pSetting, ENERGY_SAVER_SYSTEM_SLEEP_TIMER);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, ENERGY_SAVER_DISPLAY_SLEEP_TIMER)) {
        ceError = ApplyEnergySaverSettings(pSetting, ENERGY_SAVER_DISPLAY_SLEEP_TIMER);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, ENERGY_SAVER_DISK_SLEEP_TIMER)) {
        ceError = ApplyEnergySaverSettings(pSetting, ENERGY_SAVER_DISK_SLEEP_TIMER);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, ENERGY_SAVER_WAKE_ON_MODEM_RING)) {
        ceError = ApplyEnergySaverSettings(pSetting, ENERGY_SAVER_WAKE_ON_MODEM_RING);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, ENERGY_SAVER_WAKE_ON_LAN)) {
        ceError = ApplyEnergySaverSettings(pSetting, ENERGY_SAVER_WAKE_ON_LAN);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, ENERGY_SAVER_SLEEP_ON_POWER_BUTTON)) {
        ceError = ApplyEnergySaverSettings(pSetting, ENERGY_SAVER_SLEEP_ON_POWER_BUTTON);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (!strcmp(pszSettingName, ENERGY_SAVER_AUTO_RESTART_ON_POWER_LOSS)) {
        ceError = ApplyEnergySaverSettings(pSetting, ENERGY_SAVER_AUTO_RESTART_ON_POWER_LOSS);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else {
        GPA_LOG_WARNING("Warning: No handler found to set %s", pszSettingName);
    }

error:

    return ceError;
}

static
CENTERROR
ApplyMacPolicies(
    PGPASHASH_TABLE pTable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    GPAList* iter = NULL;

    for (iter = pTable->list; iter != NULL; iter = iter->next) {
        PGPASHASH_PAIR pair = (PGPASHASH_PAIR) iter->data;

        if( pair && pair->key && pair->value ) {
            ceError = ApplyPolicy( (PCSTR)pair->key,
                                   (PMAC_SETTING)pair->value);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }
    
error:

    return ceError;
}

static
CENTERROR
ParseSettingsFromGPItem(
    PGPOLWIGPITEM pGPItem,
    PGPASHASH_TABLE pTable,
	int nIndex
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodeSetPtr pXmlNodeSetPtr = NULL;
    
    ceError = GPOXmlSelectNodes( (xmlNodePtr)pGPItem->xmlNode, 
                                 g_pszGPItemQueryList[nIndex], 
                                 &pXmlNodeSetPtr);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if (pXmlNodeSetPtr && (pXmlNodeSetPtr->nodeNr > 0)) {
        int iNode = 0;
        for (iNode = 0; iNode < pXmlNodeSetPtr->nodeNr; iNode++) {
            ceError = MapMacSetting( pXmlNodeSetPtr->nodeTab[iNode], 
                                     pTable);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }
    
error:
    
    if (pXmlNodeSetPtr)
        xmlXPathFreeNodeSet(pXmlNodeSetPtr);
       
    return ceError;
}

CENTERROR
ResetMacGroupPolicy()
{
    return CENTERROR_SUCCESS;
}

static
CENTERROR
TakeSystemSnapshotAndLoadIt(
    PGPASHASH_TABLE pMergedSettings
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = EnsureSystemSnapshot();
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPAStableHashTableInit( pMergedSettings,
                                     gpa_str_hash,
                                     gpa_str_equal,
                                     (GPADestroyNotify) LwFreeString,
                                     (GPADestroyNotify) FreeMacSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = ReadSystemSnapshot(pMergedSettings);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
InitXMLData(
    PGPOLWIDATA *ppLwiData,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PCSTR pszClientGUID
    )    
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    // Look in new policy file location
    ceError = GPOInitLwiData( NULL,
                              MACHINE_GROUP_POLICY,
                              ppLwiData,
                              pGPOModifiedList->pszgPCFileSysPath,
                              NULL,
                              (PSTR)pszClientGUID);
    if ( ceError ) {
        // Look in old policy file location
        ceError = GPOInitLwiData( NULL,
                                  MACHINE_GROUP_POLICY,
                                  ppLwiData,
                                  pGPOModifiedList->pszgPCFileSysPath,
                                  NULL,
                                  NULL);

        if (!CENTERROR_IS_OK(ceError) &&
            CENTERROR_EQUAL(ceError, CENTERROR_GP_FILE_COPY_FAILED)) {
            ceError = CENTERROR_SUCCESS;
            goto error;
        }

        BAIL_ON_CENTERIS_ERROR(ceError);

    }

error:

    return ceError;
}

static
CENTERROR
ProcessGPItem(
    PGPOLWIDATA pLwidata,
    PGPASHASH_TABLE pMergedSettings
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIGPITEM pGPItem = NULL;

    int nNumGPItem = sizeof(g_pszGPItemGUIDList) / 4;
    int i = 0;
    for ( ; i < nNumGPItem; i++) {
        ceError = GPOGetGPItem( MACHINE_GROUP_POLICY,
                                pLwidata,
                                (PSTR)g_pszGPItemGUIDList[i],
                                &pGPItem );
        if (CENTERROR_EQUAL(ceError, CENTERROR_GP_XML_GPITEM_NOT_FOUND)) {
            ceError = CENTERROR_SUCCESS;
            continue;
        }
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        ceError = ParseSettingsFromGPItem( pGPItem, 
                                           pMergedSettings, 
                                           i);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        if (pGPItem) {
            GPODestroyGPItem(pGPItem, FALSE);
            pGPItem = NULL;
        }
    }

error:

    if (pGPItem) {
        GPODestroyGPItem(pGPItem, FALSE);
    }

    return ceError;
}

static
CENTERROR
ProcessXMLData(
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGPASHASH_TABLE pMergedSettings
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIDATA pLwidata = NULL;

    ceError = InitXMLData( &pLwidata,
		           pGPOModifiedList,
		           LWI_MAC_POLICY_GUID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ProcessGPItem( pLwidata,
			     pMergedSettings);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if ( pLwidata ) {
        GPODestroyLwiData(pLwidata);
    }
    
    return ceError;
}

CENTERROR
ProcessMacGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    GPASHASH_TABLE mergedSettings;

    GPA_LOG_FUNCTION_ENTER();

    if ( !IsNullOrEmptyString(g_module.pszError) ||
         !g_module.LibHandle) {
        GPA_LOG_ERROR("Error opening lwimacinterface bundle. %s", !IsNullOrEmptyString(g_module.pszError) ? g_module.pszError : ""); 
        goto error;
    }

    ceError = TakeSystemSnapshotAndLoadIt(&mergedSettings);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for (; pGPOModifiedList; pGPOModifiedList = pGPOModifiedList->pNext) {
        if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE( "GPO(%s) is disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);
            continue;
        }

        ceError = ProcessXMLData( pGPOModifiedList,
                                  &mergedSettings);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }   
    
    if (mergedSettings.list) {
        ceError = ApplyMacPolicies(&mergedSettings);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
error:
 
    GPAStableHashTableFree(&mergedSettings);

    GPA_LOG_FUNCTION_LEAVE(ceError);

    return(CENTERROR_SUCCESS);
}
