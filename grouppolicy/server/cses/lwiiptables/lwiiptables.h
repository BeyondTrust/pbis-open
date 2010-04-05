#ifndef __LWIIPTABLES_H__
#define __LWIIPTABLES_H__

#define LWIIPTABLES_MACHINE_CLIENT_GUID     "{7B438710-C0DD-4802-B7B9-F67C891BDC84}"
#define LWIIPTABLES_MACHINE_ITEM_GUID       "{95064786-DAE8-490b-B596-A8CB39BBB1D8}"
#define IPTABLES_INIT_FILE                  "/etc/init.d/iptables"
#define IPTABLES_SAVE_COMMAND               "/etc/init.d/iptables save"
#define IPTABLES_RESTART_COMMAND            "/etc/init.d/iptables restart"

#define STATIC_PATH_BUFFER_SIZE             256

static const PSTR LWI_IPTABLES_BKP_FILEPATH  = CACHEDIR "/systemfiles/iptables-config.old";
static const PSTR CENTERIS_GP_DIRECTORY      = CACHEDIR "/";
static const PSTR CENTERIS_SYSVOL_PATH       = "\\Machine\\Centeris\\Identity\\";

typedef struct __IPTABLESLIST
{
    PSTR pszDisplayName;
    PSTR pszGUID;
    PSTR pszRules;
    PSTR pszFilePath;
    struct __IPTABLESLIST *pNext;
} IPTABLESLIST, *PIPTABLESLIST;

static PIPTABLESLIST g_PolicyHierarchyList = NULL;

CENTERROR
ProcessIPTablesGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    );

CENTERROR
ResetIPTablesGroupPolicy();


#endif

