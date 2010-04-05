#ifndef __LWIEVTFWD_H__
#define __LWIEVTFWD_H__

#define LWIEVTFWD_ITEM_GUID        "{E12B76B6-1378-41a5-A218-24AC9C8E9F14}"
#define LWIEVTFWD_CLIENT_GUID      "{0EED766B-2404-46A6-A6B6-F8971164A920}"

#define EVTSETTINGS_XML_NODE_QUERY      "setting"

#define RESOURCE_LIMITS_SECTION         "resource-limits"
#define EVENTFWD_GLOBAL_SECTION         "global"

static const DWORD BUFSIZE                  =  4096;


typedef struct __evtfwd_policy_settings {
    PSTR pszCollector;                                     // Collector Name or Address
    BOOL bUseDefaultCollector;

    PSTR pszCollectorPrincipal;                             // Service Principal
    BOOL bUseDefaultCollectorPrincipal;
} EVTFWD_POLICY_SETTINGS, *PEVTFWD_POLICY_SETTINGS;

CENTERROR
ApplyLwiEVTFWDSettingsPolicy(
    PGPOLWIGPITEM *ppRsopEVTFWDSettingsItem
    );

#endif /* __LWIEVTFWD_H__ */

