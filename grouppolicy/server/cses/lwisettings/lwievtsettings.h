#ifndef __LWIEVTSETTINGS_H__
#define __LWIEVTSETTINGS_H__

#define LWIEVTSETTINGS_ITEM_GUID        "{B60BD7C6-9593-4a9a-B16C-CA3B1FEE45D7}"
#define LWIEVTSETTINGS_CLIENT_GUID      "{0EED766B-2404-46A6-A6B6-F8971164A920}"

#define EVTSETTINGS_XML_NODE_QUERY      "setting"

#define RESOURCE_LIMITS_SECTION         "resource-limits"
#define EVENTLOG_GLOBAL_SECTION         "global"

typedef struct __eventlog_policy_settings {
    BOOL  bUseDefaultAllowReadTo;
    PSTR  pszAllowReadTo;

    BOOL  bUseDefaultAllowWriteTo;
    PSTR  pszAllowWriteTo;

    BOOL  bUseDefaultAllowDeleteTo;
    PSTR  pszAllowDeleteTo;

    BOOL  bUseDefaultMaxDiskUsage;
    DWORD dwMaxDiskUsage;

    BOOL  bUseDefaultMaxEventLifespan;
    DWORD dwMaxEventLifespan;

    BOOL  bUseDefaultMaxNumEvents;
    DWORD dwMaxNumEvents;

    BOOL  bUseDefaultRemoveAsNeeded;
    BOOL  bRemoveAsNeeded;

}EVENTLOG_POLICY_SETTINGS,*PEVENTLOG_POLICY_SETTINGS;

CENTERROR
ApplyLwiEVTSettingsPolicy(
    PGPOLWIGPITEM *ppRsopEVTSettingsItem
    );

#endif /* __LWIEVTSETTINGS_H__ */

