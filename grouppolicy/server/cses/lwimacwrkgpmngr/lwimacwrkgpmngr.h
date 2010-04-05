#ifndef __LWIMACWRKGPMNGR_H__
#define __LWIMACWRKGPMNGR_H__

#define LWIMACWRKGPMNGR_MACHINE_CLIENT_GUID "{B9BF896E-F9EB-49b5-8E67-11E2EDAED06C}"
#define LWIMACWRKGPMNGR_USER_CLIENT_GUID    "{07E500C4-20FD-4829-8F38-B5FF63FA0493}"

#define LWIMACWRKGPMNGR_MACHINE_ITEM_GUID   "{D6086B42-EBC0-4da5-8D77-CD154CE84A28}"
#define LWIMACWRKGPMNGR_USER_ITEM_GUID      "{1DB31D88-03FC-4660-823B-2B927733BC2C}"

#define STATIC_PATH_BUFFER_SIZE              256

static const PSTR LWE_MAC_USER_MCX_FILE         = ".lwe-user-mcx";
static const PSTR LWE_MAC_MACHINE_MCX_FILE      = ".lwe-computer-mcx";
static const PSTR LWE_MAC_MACHINE_MCX_DIR       = CACHEDIR "/mcx/computer/";
static const PSTR LWE_MAC_MACHINE_MCX_FILEPATH  = CACHEDIR "/mcx/computer/.lwe-computer-mcx";
static const PSTR LWE_MAC_USER_MCX_DIR          = CACHEDIR "/mcx/users/";
static const PSTR LWE_MAC_ALL_USERS_MCX_FILEPATH = CACHEDIR "/mcx/.lwe-allusers-mcx";
static const PSTR MCX_USER_GPO_MAP_DIRECTORY    = CACHEDIR "/mcx/map";
static const PSTR MCX_NEW_USER_MAP_FILEPATH     = CACHEDIR "/mcx/map/newmap";
static const PSTR LIKEWISE_GP_DIRECTORY         = CACHEDIR "/";
static const PSTR LIKEWISE_COMPUTER_SYSVOL_PATH = "\\Machine\\Centeris\\Identity\\";
static const PSTR LIKEWISE_USER_SYSVOL_PATH     = "\\User\\Centeris\\Identity\\";

typedef struct __MAC_WRKGPMNGRLIST
{
    PSTR pszDisplayName;
    PSTR pszGUID;
    struct __MAC_WRKGPMNGRLIST *pNext;
} MAC_WRKGPMNGRLIST, *PMAC_WRKGPMNGRLIST;

CENTERROR
ProcessWorkgroupManagerGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    );

CENTERROR
ResetWorkgroupManagerGroupPolicy(
    PGPUSER pUser
    );


#endif

