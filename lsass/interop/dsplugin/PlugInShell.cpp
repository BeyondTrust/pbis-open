/*
  File:         CPlugInShell.c

  Version:      Directory Services 1.0

  Copyright:    ¬© 1999-2001 by Apple Computer, Inc., all rights reserved.

  *****************************

  Add plug-in functionality to this file.

  *****************************
  */

#include "includes.h"
#include "LWIPlugIn.h"
#include "LWIRecordListQuery.h"
#include "LWIAttrValDataQuery.h"
#include "LWIDirNodeQuery.h"
#include "LWIRecordQuery.h"

// Mac OS X Version Names
#define MAC_OS_X_VERSION_NAME_10_4 "Tiger"
#define MAC_OS_X_VERSION_NAME_10_5 "Leopard"
#define MAC_OS_X_VERSION_NAME_10_6 "Snow Leopard"
#define MAC_OS_X_VERSION_NAME_10_7 "Snow Leopard"

// Local helper functions
//

static long Activate(void);
static long Deactivate(void);
static long RefreshGPONodes(void);


// ----------------------------------------------------------------------------
// * Private Globals
// ----------------------------------------------------------------------------
typedef struct _PLUGIN_STATE {
    unsigned long PluginState;
    unsigned long Signature;
    tDirReference DsRoot;
    tDataListPtr NodeNameList;
    CFMutableDictionaryRef NodeDictionary;
    pthread_rwlock_t Lock;
    bool IsLockInitialized;
    bool IsInitialized;
    bool IsJoinedToAD;
    bool IsStartupComplete;
    LWE_DS_FLAGS Flags;
    pthread_mutex_t PeriodicTaskMutex;
    bool IsPeriodicTaskMutexInitialized;
    PSTR pszRealm;
    PGROUP_POLICY_OBJECT pGPOs;
    bool fDomainControllerNotAvailable;
    int OfflineTimerCount;
    PNETADAPTERINFO pNetAdapterList;
    PSTR pszCurrentAllowedAdminsList;
    PVOID pAllowAdminCheckData;
    pthread_rwlock_t AdminAccessListLock;
    bool IsAdminAccessListLockInitialized;
} PLUGIN_STATE, *PPLUGIN_STATE;

static PLUGIN_STATE GlobalState = { 0 };

#define _GS_ACQUIRE(OpCode, OpLiteral) \
    do { \
        if (pthread_rwlock_ ## OpCode ## lock(&GlobalState.Lock) < 0) \
        { \
            int libcError = errno; \
            LOG_FATAL("Error acquiring lock for " OpLiteral ": %s (%d)", strerror(libcError), libcError); \
        } \
    } while (0)

#define GS_ACQUIRE_EXCLUSIVE() \
    _GS_ACQUIRE(wr, "write")

#define GS_ACQUIRE_SHARED() \
    _GS_ACQUIRE(rd, "read")

#define GS_RELEASE() \
    do { \
        if (pthread_rwlock_unlock(&GlobalState.Lock) < 0) \
        { \
            int libcError = errno; \
            LOG_FATAL("Error releasing lock: %s (%d)", strerror(libcError), libcError); \
        } \
    } while (0)

#define _GS_ACQUIRE_ADMIN_ACCESS_LIST(OpCode, OpLiteral) \
    do { \
        if (pthread_rwlock_ ## OpCode ## lock(&GlobalState.AdminAccessListLock) < 0) \
        { \
            int libcError = errno; \
            LOG_FATAL("Error acquiring access list lock for " OpLiteral ": %s (%d)", strerror(libcError), libcError); \
        } \
    } while (0)

#define GS_ACQUIRE_EXCLUSIVE_ADMIN_ACCESS_LIST() \
    _GS_ACQUIRE_ADMIN_ACCESS_LIST(wr, "write")

#define GS_ACQUIRE_SHARED_ADMIN_ACCESS_LIST() \
    _GS_ACQUIRE_ADMIN_ACCESS_LIST(rd, "read")

#define GS_RELEASE_ADMIN_ACCESS_LIST() \
    do { \
        if (pthread_rwlock_unlock(&GlobalState.AdminAccessListLock) < 0) \
        { \
            int libcError = errno; \
            LOG_FATAL("Error releasing access list lock: %s (%d)", strerror(libcError), libcError); \
        } \
    } while (0)

#define GS_VERIFY_INITIALIZED(macError) \
    do { \
        if (!GlobalState.IsInitialized) \
        { \
            LOG_ERROR("Not initialized"); \
            macError = ePlugInFailedToInitialize; \
            GOTO_CLEANUP(); \
        } \
    } while (0)

static
long RegisterGPONode(
    PSTR pszDomain,
    PSTR pszGPOName
	)
{
    long macError = eDSNoErr;
    tDataListPtr nodeNameList = NULL;
    char szChildNodeName[1024];
	
	if (!pszDomain)
	{
	    macError = eDSEmptyParameter;
		GOTO_CLEANUP_ON_MACERROR( macError );
	}
	
	/* Build up node names for each child node found */
    nodeNameList = dsDataListAllocate(0);
    if ( !nodeNameList )
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR( macError );
    }
        
    memset(szChildNodeName, 0, sizeof(szChildNodeName));
    strcpy(szChildNodeName, PLUGIN_ROOT_PATH);
    strcat(szChildNodeName, "/");
    strcat(szChildNodeName, pszDomain);
    strcat(szChildNodeName, "/");
    strcat(szChildNodeName, pszGPOName);
                    
    macError = dsBuildListFromPathAlloc(0, nodeNameList, szChildNodeName, "/");
    GOTO_CLEANUP_ON_MACERROR( macError );
        
    macError = DSRegisterNode(GlobalState.Signature, nodeNameList, kDirNodeType);

cleanup:

    if (nodeNameList)
    {
        dsDataListDeallocate(0, nodeNameList);
        free(nodeNameList);
    }
    
    return macError;
}


static
long UnregisterGPONode(
    PSTR pszDomain,
    PSTR pszGPOName
	)
{
    long macError = eDSNoErr;
    tDataListPtr nodeNameList = NULL;
    char szChildNodeName[1024];

	if (!pszDomain)
	{
	    macError = eDSEmptyParameter;
		GOTO_CLEANUP_ON_MACERROR( macError );
	}
	
	/* Build up node names for each child node found */
    nodeNameList = dsDataListAllocate(0);
    if ( !nodeNameList )
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR( macError );
    }
        
    memset(szChildNodeName, 0, sizeof(szChildNodeName));
    strcpy(szChildNodeName, PLUGIN_ROOT_PATH);
    strcat(szChildNodeName, "/");
    strcat(szChildNodeName, pszDomain);
    strcat(szChildNodeName, "/");
    strcat(szChildNodeName, pszGPOName);
                    
    macError = dsBuildListFromPathAlloc(0, nodeNameList, szChildNodeName, "/");
    GOTO_CLEANUP_ON_MACERROR( macError );
        
    macError = DSUnregisterNode(GlobalState.Signature, nodeNameList);

cleanup:

    if (nodeNameList)
    {
        dsDataListDeallocate(0, nodeNameList);
        free(nodeNameList);
    }
    
    return macError;
}

static
bool
SafeIsJoined(
    void
    )
{
    bool result;

    // We assume that we are initialized because this function
    // should only be called after checking that we are
    // initialized.
    pthread_mutex_lock(&GlobalState.PeriodicTaskMutex);
    result = GlobalState.IsJoinedToAD;
    pthread_mutex_unlock(&GlobalState.PeriodicTaskMutex);

    return result;
}

// -------------------------------------------------------------------------
// * PlugInShell_Validate ()
//
//  inVersionStr:  Version string of current running Directory Services server.
//  inSignature :  Token handed to the plug-in by the server.  This is needed
//                 by the plug-ins to register/unregister nodes.
//
//                 This routine is called once during plug-in loading.
// -------------------------------------------------------------------------
long
PlugInShell_Validate (
    const char *inVersionStr,
    unsigned long inSignature
    )
{
    long macError = eDSNoErr;

    LOG_ENTER("Version = %s, Signature = %d", SAFE_LOG_STR(inVersionStr), inSignature);
    // Note that is is called before Initialize, so we do not need to synchronize.
    GlobalState.Signature = inSignature;
    LOG_LEAVE("--> %d", macError);

    return macError;
}

// ----------------------------------------------------------------------------
// * PlugInShell_Initialize ()
//
// This routine is called once after all plug-ins have been loaded during
// the server startup process.
// ----------------------------------------------------------------------------

long PlugInShell_Initialize(void)
{
    long macError = eDSNoErr;
    bool gotUnameInfo = false;
    PSTR pszVersion = NULL;
    PCSTR pszVersionName = NULL;
    bool isUnsupported = false;
    PNETADAPTERINFO pTempNetInfo = NULL;
    struct utsname info;
    BOOLEAN bMergeModeMCX = FALSE;
    BOOLEAN bEnableForceHomedirOnStartupDisk = FALSE;
    BOOLEAN bUseADUNCForHomeLocation = FALSE;
    PSTR pszUNCProtocolForHomeLocation = NULL;
    PSTR pszAllowAdministrationBy = NULL;
    BOOLEAN bMergeAdmins = FALSE;
    PVOID pAllowAdminCheckData = NULL;

    memset(info.sysname, 0, sizeof(info.sysname));
    memset(info.nodename, 0, sizeof(info.nodename));
    memset(info.release, 0, sizeof(info.release));
    memset(info.version, 0, sizeof(info.version));
    memset(info.machine, 0, sizeof(info.machine));

    LOG_ENTER("");
    LOG("Current State = 0x%08x", GlobalState.PluginState);

    //
    // We expect to be called exactly once, except if we fail to initialize.
    // When that happens, we can get called again several times to try to
    // initialize successfully.
    //

    if (GlobalState.IsInitialized)
    {
        LOG("Plug-in already initialized");
        GOTO_CLEANUP();
    }

    /* Clear all values for GlobalState */
    GlobalState.IsInitialized = false;
    GlobalState.PluginState = kUnknownState;
    GlobalState.IsJoinedToAD = false;
    GlobalState.Flags = LWE_DS_FLAG_NO_OPTIONS_SET;
    GlobalState.NodeNameList = NULL;
    GlobalState.pGPOs = NULL;
    GlobalState.pszRealm = NULL;
    macError = InitializeContextList();
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (uname(&info))
    {
        gotUnameInfo = false;

        macError = LWCaptureOutput((char*)"sw_vers -productVersion", &pszVersion);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        if (strstr(pszVersion, "10.4.") == pszVersion)
        {
            GlobalState.Flags = GlobalState.Flags & (~LWE_DS_FLAG_IS_LEOPARD);
            pszVersionName = MAC_OS_X_VERSION_NAME_10_4;
        }
        else if (strstr(pszVersion, "10.5.") == pszVersion)
        {
            GlobalState.Flags = GlobalState.Flags | LWE_DS_FLAG_IS_LEOPARD;
            pszVersionName = MAC_OS_X_VERSION_NAME_10_5;
        }
        else if (strstr(pszVersion, "10.6.") == pszVersion)
        {
            GlobalState.Flags = GlobalState.Flags | LWE_DS_FLAG_IS_LEOPARD | LWE_DS_FLAG_IS_SNOW_LEOPARD;
            pszVersionName = MAC_OS_X_VERSION_NAME_10_6;
        }
        else if (strstr(pszVersion, "10.7.") == pszVersion)
        {
            GlobalState.Flags = GlobalState.Flags | LWE_DS_FLAG_IS_LEOPARD | LWE_DS_FLAG_IS_SNOW_LEOPARD;
            pszVersionName = MAC_OS_X_VERSION_NAME_10_7;
        }
        else
        {
            isUnsupported = true;
        }
    }
    else
    {
        gotUnameInfo = true;

        macError = LwAllocateString(info.release, &pszVersion);
        GOTO_CLEANUP_ON_MACERROR(macError);

        if (strstr(pszVersion, "8.") == pszVersion)
        {
            GlobalState.Flags = GlobalState.Flags & ~LWE_DS_FLAG_IS_LEOPARD;
            pszVersionName = MAC_OS_X_VERSION_NAME_10_4;
        }
        else if (strstr(pszVersion, "9.") == pszVersion)
        {
            GlobalState.Flags = GlobalState.Flags | LWE_DS_FLAG_IS_LEOPARD;
            pszVersionName = MAC_OS_X_VERSION_NAME_10_5;
        }
        else if (strstr(pszVersion, "10.") == pszVersion)
        {
            GlobalState.Flags = GlobalState.Flags | LWE_DS_FLAG_IS_LEOPARD | LWE_DS_FLAG_IS_SNOW_LEOPARD;
            pszVersionName = MAC_OS_X_VERSION_NAME_10_6;
        }
        else if (strstr(pszVersion, "11.") == pszVersion)
        {
            GlobalState.Flags = GlobalState.Flags | LWE_DS_FLAG_IS_LEOPARD | LWE_DS_FLAG_IS_SNOW_LEOPARD;
            pszVersionName = MAC_OS_X_VERSION_NAME_10_6;
        }
        else
        {
            isUnsupported = true;
        }
    }
    if (isUnsupported)
    {
        pszVersionName = "unsupported";
    }
    LOG("Starting up Likewise - Active directory DS plug-in, detected %s Mac OS X %s(%s)",
        pszVersionName, gotUnameInfo ? "kernel " : "", pszVersion);
    if (isUnsupported)
    {
        macError = ePlugInFailedToInitialize;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    /* Get the network adpater details - We only care about the ENetAddress info */
    macError = LWGetNetAdapterList(true, &GlobalState.pNetAdapterList);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (!GlobalState.pNetAdapterList)
    {
        LOG("Could not find an ethernet network adapter, will retry later. Computer settings maybe not be applied till one is found.");
    }

    pTempNetInfo = GlobalState.pNetAdapterList;
    while (pTempNetInfo)
    {
        LOG("Found network adapter...");
        LOG("  Name: %s", pTempNetInfo->pszName);
        LOG("  ENet: %s", pTempNetInfo->pszENetAddress ? pTempNetInfo->pszENetAddress : "----");
        LOG("  IP: %s", pTempNetInfo->pszIPAddress ? pTempNetInfo->pszIPAddress : "----");
        LOG("  Up: %s", pTempNetInfo->IsUp ? "yes" : "no");
        LOG("  Running: %s", pTempNetInfo->IsRunning ? "yes" : "no");
        pTempNetInfo = pTempNetInfo->pNext;
    }
    
    macError = GetConfigurationSettings(&bMergeModeMCX,
                                        &bEnableForceHomedirOnStartupDisk,
                                        &bUseADUNCForHomeLocation,
                                        &pszUNCProtocolForHomeLocation,
                                        &pszAllowAdministrationBy,
                                        &bMergeAdmins);
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* See if MCX setting aggregation feature is to be supported */
    if (bMergeModeMCX)
    {
        GlobalState.Flags = GlobalState.Flags | LWE_DS_FLAG_MERGE_MODE_MCX;
        LOG("Merge mode MCX is enabled. Settings from multiple Group Policy Objects will be merged for the AD user accounts at logon.");
    }

    /* See if Force Home Directory On Startup Disk feature is to be supported */
    if (bEnableForceHomedirOnStartupDisk)
    {
        GlobalState.Flags = GlobalState.Flags | LWE_DS_FLAG_FORCE_LOCAL_HOME_DIRECTORY_ON_STARTUP_DISK;
        LOG("Force Home Directory On Startup Disk is enabled.");
    }

    /* See if Use AD UNC for Home Location - SMB feature is to be supported */
    if (bUseADUNCForHomeLocation)
    {
        if (pszUNCProtocolForHomeLocation && !strcmp(pszUNCProtocolForHomeLocation, "smb"))
        {
            GlobalState.Flags = GlobalState.Flags | LWE_DS_FLAG_USE_AD_UNC_FOR_HOME_LOCATION_SMB;
            LOG("Use AD UNC for Home Location - SMB is enabled.");
        }
        else if (pszUNCProtocolForHomeLocation && !strcmp(pszUNCProtocolForHomeLocation, "afp"))
        {
            /* See if Use AD UNC for Home Location - AFP feature is to be supported */
            GlobalState.Flags = GlobalState.Flags | LWE_DS_FLAG_USE_AD_UNC_FOR_HOME_LOCATION_AFP;
            LOG("Use AD UNC for Home Location - AFP is enabled.");
        }
        else
        {
            GlobalState.Flags = GlobalState.Flags | LWE_DS_FLAG_USE_AD_UNC_FOR_HOME_LOCATION_SMB;
            LOG("Use AD UNC for Home Location - defaulting to SMB protocol.");
        }
    }

    if (pszAllowAdministrationBy)
    {
        GlobalState.pszCurrentAllowedAdminsList = pszAllowAdministrationBy;
        pszAllowAdministrationBy = NULL;

        macError = GetAccessCheckData(GlobalState.pszCurrentAllowedAdminsList, &pAllowAdminCheckData);
        if (macError)
        {
            if (macError == eDSAuthUnknownUser)
            {
                LOG("GetAccessCheckData(%s) failed with error: eDSAuthUnknownUser. AD user accounts will not be added to admin group (GID:80), since the list provided is incorrectly specified. This error suggests that you have a user or group that is not recognized by Likewise authentication daemon. Recommend checking that system administrator has enabled the items here in the Likewise cell that applies to this computer.", GlobalState.pszCurrentAllowedAdminsList);
            }
            else
            {
                LOG("Failed to GetAccessCheckData(%s) with error: %d", GlobalState.pszCurrentAllowedAdminsList, macError);
            }

            LW_SAFE_FREE_STRING(GlobalState.pszCurrentAllowedAdminsList);
            GlobalState.pszCurrentAllowedAdminsList = NULL;
            GlobalState.pAllowAdminCheckData = NULL;
            macError = eDSNoErr;
        }
        else
        {
            LOG("AllowAdministrationBy configured to: %s", GlobalState.pszCurrentAllowedAdminsList);
            GlobalState.pAllowAdminCheckData = pAllowAdminCheckData;
            pAllowAdminCheckData = NULL;
        }
    }

    /* See if Merge Admins feature is to be supported */
    if (bMergeAdmins)
    {
        GlobalState.Flags = GlobalState.Flags | LWE_DS_FLAG_DONT_REMOVE_LOCAL_ADMINS;
        LOG("Option to override allow-administration-by with local computer changes to the admin group is enabled.");
    }

    if (pthread_rwlock_init(&GlobalState.Lock, NULL) < 0)
    {
        int libcError = errno;
        LOG_ERROR("Failied to init lock: %s (%d)", strerror(libcError), libcError);
        macError = ePlugInInitError;
        GOTO_CLEANUP();
    }
    GlobalState.IsLockInitialized = true;

    if (pthread_mutex_init(&GlobalState.PeriodicTaskMutex, NULL) < 0)
    {
        int libcError = errno;
        LOG_ERROR("Failied to init mutex: %s (%d)", strerror(libcError), libcError);
        macError = ePlugInInitError;
        GOTO_CLEANUP();
    }
    GlobalState.IsPeriodicTaskMutexInitialized = true;

    if (pthread_rwlock_init(&GlobalState.AdminAccessListLock, NULL) < 0)
    {
        int libcError = errno;
        LOG_ERROR("Failied to init admin access list lock: %s (%d)", strerror(libcError), libcError);
        macError = ePlugInInitError;
        GOTO_CLEANUP();
    }
    GlobalState.IsAdminAccessListLockInitialized = true;

    macError = LWIAttrLookup::Initialize();
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIRecTypeLookup::Initialize();
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIDirNodeQuery::Initialize();
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIRecordQuery::Initialize();
    GOTO_CLEANUP_ON_MACERROR(macError);

    LWICRC::Initialize();

    // Allow WGM policy options to define Login/Logoff Hook scripts. We do this to automatically support settings
    // we get from group policies.
    macError = SetupMCXLoginScriptsSupport();
    GOTO_CLEANUP_ON_MACERROR(macError);

    GlobalState.IsInitialized = true;
    GlobalState.PluginState = kInitialized | kInactive;

    //
    // If we supported custom calls while not active, we would need to create a
    // node for that here.
    //

cleanup:

    if (pszVersion)
    {
        LwFreeMemory(pszVersion);
    }

    if (pszUNCProtocolForHomeLocation)
    {
        LW_SAFE_FREE_STRING(pszUNCProtocolForHomeLocation);
    }

    if (pszAllowAdministrationBy)
    {
        LW_SAFE_FREE_STRING(pszUNCProtocolForHomeLocation);
    }

    if (pAllowAdminCheckData)
    {
        FreeAccessCheckData(pAllowAdminCheckData);
    }

    if (macError)
    {
        // This is the only error code allowed in the failure case.
        macError = ePlugInInitError;

        PlugInShell_Shutdown();
        GlobalState.PluginState = kFailedToInit | kInactive;
    }

    LOG("Final State = 0x%08x", GlobalState.PluginState);

    LOG_LEAVE("--> %d", macError);

    return macError;
}

// ---------------------------------------------------------------------------
// * PlugInShell_ProcessRequest
//
//  inData      : A pointer to a data structure representing the current
//                server call.
//
//  This routine is called continuiously throughout the operation of the
//  Directory Services process.
// ---------------------------------------------------------------------------
long
PlugInShell_ProcessRequest(void *inData)
{
    long macError = eDSNoErr;
    bool isAcquired = false;
    sHeader * pMsgHdr = (sHeader *)inData;
    unsigned long msgType = pMsgHdr ? pMsgHdr->fType : 0;

    LOG_ENTER("inData = @%p => { fType = %lu (%s) }", inData, msgType, TypeToString(msgType));

    GS_VERIFY_INITIALIZED(macError);

    if (!inData)
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP();
    }

    GS_ACQUIRE_SHARED();
    isAcquired = true;

    //
    // We currently do not handle anything while not "active".
    //
    if ( !FlagOn(GlobalState.PluginState, kActive) )
    {
        macError = ePlugInNotActive;
        GOTO_CLEANUP();
    }

    //
    // We also do not handle anything while not "startup complete".
    //
    if (GlobalState.IsStartupComplete == false && 
        msgType != kOpenDirNode &&
        msgType != kDoPlugInCustomCall &&
        msgType != kServerRunLoop &&
        msgType != kKerberosMutex)
    {
       macError = ePlugInNotActive;
       LOG("Startup of dependent services not complete, therefore network accounts are offline");
       GOTO_CLEANUP();
    }

    // ISSUE-2007/05/30-dalmeida -- We should use r/w locks instead so that
    // we can behave sensibly when someone tries to de-activate the plug-in
    // while we are processing something...

    try
    {
        switch ( msgType )
        {
        case kHandleNetworkTransition:
            LOG("Got Network Transition change notice");
            macError = eDSNoErr;
            break;
    
        case kHandleSystemWillSleep:
            LOG("Got Handle System Will Sleep notice");
            macError = eDSNoErr;
            break;
    
        case kHandleSystemWillPowerOn:
            LOG("Got Handle System Will Power On notice");
            macError = eDSNoErr;
            break;
    
        case kOpenDirNode:
            LOG("Start of directory node query");
            macError = LWIDirNodeQuery::Open((sOpenDirNode *)inData);
            break;

        case kDoDirNodeAuth:
            GS_ACQUIRE_SHARED_ADMIN_ACCESS_LIST();
            macError = LWIDirNodeQuery::DoDirNodeAuth((sDoDirNodeAuth *)inData,
                                                       SafeIsJoined(),
                                                       GlobalState.pAllowAdminCheckData,
                                                       GlobalState.Flags);
            GS_RELEASE_ADMIN_ACCESS_LIST();
            break;
            
        case kCloseDirNode:
            macError = LWIDirNodeQuery::Close((sCloseDirNode *)inData);
            break;

        case kGetDirNodeInfo:
            macError = LWIDirNodeQuery::GetInfo((sGetDirNodeInfo *)inData,
                                                GlobalState.Flags,
                                                GlobalState.pNetAdapterList);
            break;

        case kGetAttributeEntry:
            macError = LWIDirNodeQuery::GetAttributeEntry((sGetAttributeEntry *)inData);
            break;

        case kGetRecordEntry:
            macError = LWIDirNodeQuery::GetAttributeEntry((sGetAttributeEntry *)inData);
            break;

        case kGetAttributeValue:
            macError = LWIDirNodeQuery::GetAttributeValue((sGetAttributeValue *)inData);
            break;

        case kCloseAttributeValueList:
            macError = LWIDirNodeQuery::CloseValueList((sCloseAttributeValueList *)inData);
            break;

         case kCloseAttributeList:
            macError = LWIDirNodeQuery::CloseAttributeList((sCloseAttributeList *)inData);
            break;

        case kGetRecordList:
            macError = LWIRecordListQuery::Run((sGetRecordList *)inData, GlobalState.Flags, GlobalState.pNetAdapterList);
            break;

        case kReleaseContinueData:
            macError = LWIRecordListQuery::ReleaseContinueData((sReleaseContinueData *)inData);
            break;

        case kDoAttributeValueSearch:
        case kDoAttributeValueSearchWithData:
            macError = LWIAttrValDataQuery::Run((sDoAttrValueSearchWithData *)inData, GlobalState.Flags, GlobalState.pNetAdapterList);
            break;
        
        case kDoMultipleAttributeValueSearch:
        case kDoMultipleAttributeValueSearchWithData:
            macError = LWIAttrValDataQuery::Run((sDoMultiAttrValueSearchWithData *)inData, GlobalState.Flags, GlobalState.pNetAdapterList);
            break;

        case kOpenRecord:
            LOG("Start of record query");
            macError = LWIRecordQuery::Open((sOpenRecord*)inData, GlobalState.Flags, GlobalState.pNetAdapterList);
            break;

        case kGetRecordReferenceInfo:
            macError = LWIRecordQuery::GetReferenceInfo((sGetRecRefInfo*)inData);
            break;

        case kCloseRecord:
            macError = LWIRecordQuery::Close((sCloseRecord*)inData);
            break;

        case kGetRecordAttributeInfo:
            macError = LWIRecordQuery::GetAttributeInfo((sGetRecAttribInfo*)inData);
            break;

        case kGetRecordAttributeValueByID:
            macError = LWIRecordQuery::GetAttributeValueByID((sGetRecordAttributeValueByID*)inData);
            break;

        case kGetRecordAttributeValueByIndex:
            macError = LWIRecordQuery::GetAttributeValueByIndex((sGetRecordAttributeValueByIndex*)inData);
            break;
			
        /* Supported update operations */
        case kAddAttribute:
            macError = LWIRecordQuery::AddAttribute((sAddAttribute*)inData);
	    break;

        case kAddAttributeValue:
            macError = LWIRecordQuery::AddAttributeValue((sAddAttributeValue*)inData);
            break;

        case kRemoveAttribute:
            macError = LWIRecordQuery::RemoveAttribute((sRemoveAttribute*)inData);
            break;

        case kFlushRecord:
            macError = LWIRecordQuery::FlushRecord((sFlushRecord*)inData);
            break;
            
        case kSetAttributeValues:
            macError = LWIRecordQuery::SetAttributeValues((sSetAttributeValues*)inData);
            break;
            
        case kSetAttributeValue:
            macError = LWIRecordQuery::SetAttributeValue((sSetAttributeValue*)inData);
            break;

        case kSetRecordName:
        case kSetRecordType:
        case kDeleteRecord:
        case kCreateRecord:
        case kCreateRecordAndOpen: /* sCreateRecord */
        case kRemoveAttributeValue:
        case kDoPlugInCustomCall:
        default:
            if ((msgType < kDSPlugInCallsBegin) || (msgType > kDSPlugInCallsEnd))
            {
                LOG("Unsupported request type: %lu (%s)", msgType, TypeToString(msgType));
            }
            else
            {
                LOG("Unknown request type: %lu", msgType);
            }
            macError = eNotHandledByThisNode;
            break;
        }
    }
    catch (LWIException& lwi)
    {
        macError = lwi.getErrorCode();
    }

cleanup:

    if (isAcquired)
    {
        GS_RELEASE();
    }

    if (pMsgHdr)
    {
        pMsgHdr->fResult = macError;
    }

    LOG_LEAVE("--> %d", macError);

    return macError;
}

// --------------------------------------------------------------------------------
// * PlugInShell_SetPluginState ()
//
// inNewState : New transition state that the plug-in must become
//
// This routine is called when the state of the plug-in needs to change.  The
// plug-in needs to handle these state changes.
// --------------------------------------------------------------------------------

long PlugInShell_SetPluginState(const unsigned long inNewState)
{
    long macError = eDSNoErr;
    bool isAcquired = false;

    LOG_ENTER("inNewState = 0x%08x (%s)", inNewState, StateToString(inNewState));

    GS_VERIFY_INITIALIZED(macError);

    if (FlagOn(inNewState, ~(kActive | kInactive)))
    {
        LOG("Ignoring unexpected state flags: 0x%08x", FlagOn(inNewState, ~(kActive | kInactive)));
    }

    if (!FlagOn(inNewState, kActive | kInactive))
    {
        // Nothing to do.
        LOG("Nothing to do because inactive/active flags are not specified.");
        macError = eDSNoErr;
        GOTO_CLEANUP();
    }

    if (FlagOn(inNewState, kActive) && FlagOn(inNewState, kInactive))
    {
        LOG_ERROR("Cannot set active and inactive at the same time.");
        macError = ePlugInError;
        GOTO_CLEANUP();
    }

    GS_ACQUIRE_EXCLUSIVE();
    isAcquired = true;

    LOG("Current State = 0x%08x", GlobalState.PluginState);

    if ( (FlagOn(inNewState, kActive | kInactive) == FlagOn(GlobalState.PluginState, kActive | kInactive)) )
    {
        // Nothing to do.
        LOG("Nothing to do because the state matches");
        macError = eDSNoErr;
        GOTO_CLEANUP();
    }

    if ( FlagOn(inNewState, kActive) )
    {
        LOG("Activating");
        macError = Activate();
        GOTO_CLEANUP_ON_MACERROR(macError);

        SetFlag(GlobalState.PluginState, kActive);
        ClearFlag(GlobalState.PluginState, kInactive);
    }
    else if ( FlagOn(inNewState, kInactive) )
    {
        LOG("De-activating");
        macError = Deactivate();
        GOTO_CLEANUP_ON_MACERROR(macError);

        ClearFlag(GlobalState.PluginState, kActive);
        SetFlag(GlobalState.PluginState, kInactive);
    }
    else
    {
        // This should never happen.
        LOG_ERROR("Benign unexpected code path.");
        macError = eDSNoErr;
    }

cleanup:

    if (isAcquired)
    {
        LOG("Final State = 0x%08x", GlobalState.PluginState);
        GS_RELEASE();
    }

    LOG_LEAVE("--> %d", macError);
    return macError;
}

// --------------------------------------------------------------------------------
// * PlugInShell_PeriodicTask ()
//
// This routine is called periodically while the Directory Services server
// is running.  This can be used by the plug-in to do housekeeping tasks.
// --------------------------------------------------------------------------------

long PlugInShell_PeriodicTask(void)
{
    long macError = eDSNoErr;
    bool isAcquired = false;
    BOOLEAN bMergeModeMCX = FALSE;
    BOOLEAN bEnableForceHomedirOnStartupDisk = FALSE;
    BOOLEAN bUseADUNCForHomeLocation = FALSE;
    BOOLEAN bAdminListChanged = FALSE;
    PSTR pszUNCProtocolForHomeLocation = NULL;
    PSTR pszAllowAdministrationBy = NULL;
    BOOLEAN bMergeAdmins = FALSE;
    BOOLEAN bIsStarted = FALSE;
    LWE_DS_FLAGS NewFlags = LWE_DS_FLAG_NO_OPTIONS_SET;
    PVOID pAllowAdminCheckData = NULL;
    PNETADAPTERINFO pTempNetInfo = NULL;

    // No enter/leave logging since function is called every 30 seconds
    // or so (on Mac OS X 10.4.7).

    GS_VERIFY_INITIALIZED(macError);

    GS_ACQUIRE_SHARED();
    pthread_mutex_lock(&GlobalState.PeriodicTaskMutex);
    isAcquired = true;

    if (!GlobalState.pNetAdapterList)
    {
        /* Get the network adpater details - We only care about the ENetAddress info */
        macError = LWGetNetAdapterList(true, &GlobalState.pNetAdapterList);
        GOTO_CLEANUP_ON_MACERROR(macError);

        pTempNetInfo = GlobalState.pNetAdapterList;
        while (pTempNetInfo)
        {
            LOG("Finally found a valid ethernet  network adapter...");
            LOG("  Name: %s", pTempNetInfo->pszName);
            LOG("  ENet: %s", pTempNetInfo->pszENetAddress ? pTempNetInfo->pszENetAddress : "----");
            LOG("  IP: %s", pTempNetInfo->pszIPAddress ? pTempNetInfo->pszIPAddress : "----");
            LOG("  Up: %s", pTempNetInfo->IsUp ? "yes" : "no");
            LOG("  Running: %s", pTempNetInfo->IsRunning ? "yes" : "no");
            pTempNetInfo = pTempNetInfo->pNext;
        }
    }

    if (GlobalState.IsStartupComplete == false)
    {
        // Re-verify that startup has completed successfully for lsass service.
        LOG("Re-verify that LSASS service is operational");
        GetLsaStatus(&bIsStarted);
        if (bIsStarted)
        {
            LOG("LSASS service is now operational");
            GlobalState.IsStartupComplete = true;
        }
    }

    macError = GetConfigurationSettings(&bMergeModeMCX,
                                        &bEnableForceHomedirOnStartupDisk,
                                        &bUseADUNCForHomeLocation,
                                        &pszUNCProtocolForHomeLocation,
                                        &pszAllowAdministrationBy,
                                        &bMergeAdmins);
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* Make sure to preserve the flag that tells us this is Leopard or not */
    if (GlobalState.Flags & LWE_DS_FLAG_IS_LEOPARD)
    {
        NewFlags = NewFlags | LWE_DS_FLAG_IS_LEOPARD;;
    }

    /* Make sure to preserve the flag that tells us this is Snow Leopard or not */
    if (GlobalState.Flags & LWE_DS_FLAG_IS_SNOW_LEOPARD)
    {
        NewFlags = NewFlags | LWE_DS_FLAG_IS_SNOW_LEOPARD;;
    }

    /* See if MCX setting aggregation feature is to be supported */
    if (bMergeModeMCX)
    {
        NewFlags = NewFlags | LWE_DS_FLAG_MERGE_MODE_MCX;
        if (GlobalState.Flags & LWE_DS_FLAG_MERGE_MODE_MCX == 0)
        {
            LOG("Merge mode MCX is now enabled. Settings from multiple Group Policy Objects will be merged for the AD user accounts at logon.");
        }
    }

    /* See if Force Home Directory On Startup Disk feature is to be supported */
    if (bEnableForceHomedirOnStartupDisk)
    {
        NewFlags = NewFlags | LWE_DS_FLAG_FORCE_LOCAL_HOME_DIRECTORY_ON_STARTUP_DISK;
        if (GlobalState.Flags & LWE_DS_FLAG_FORCE_LOCAL_HOME_DIRECTORY_ON_STARTUP_DISK == 0)
        {
            LOG("Force Home Directory On Startup Disk is now enabled.");
        }
    }

    /* See if Use AD UNC for Home Location - SMB feature is to be supported */
    if (bUseADUNCForHomeLocation)
    {
        if (pszUNCProtocolForHomeLocation && !strcmp(pszUNCProtocolForHomeLocation, "smb"))
        {
            NewFlags = NewFlags | LWE_DS_FLAG_USE_AD_UNC_FOR_HOME_LOCATION_SMB;
        }
        else if (pszUNCProtocolForHomeLocation && !strcmp(pszUNCProtocolForHomeLocation, "afp"))
        {
            /* See if Use AD UNC for Home Location - AFP feature is to be supported */
            NewFlags = NewFlags | LWE_DS_FLAG_USE_AD_UNC_FOR_HOME_LOCATION_AFP;
        }
        else
        {
            NewFlags = NewFlags | LWE_DS_FLAG_USE_AD_UNC_FOR_HOME_LOCATION_SMB;
        }
    }

    if (pszAllowAdministrationBy)
    {
        if (GlobalState.pszCurrentAllowedAdminsList)
        {
            if (strcmp(GlobalState.pszCurrentAllowedAdminsList, pszAllowAdministrationBy))
            {
                // Setting changed from one value to another
                bAdminListChanged = true;
            }

            // Release the former cached list
            LW_SAFE_FREE_STRING(GlobalState.pszCurrentAllowedAdminsList);
        }
        else
        {
            // Former empty value is to be updated to new
            bAdminListChanged = true;
        }

        // Now replace cached list
        GlobalState.pszCurrentAllowedAdminsList = pszAllowAdministrationBy;
        pszAllowAdministrationBy = NULL;

        if (bAdminListChanged)
        {
            macError = GetAccessCheckData(GlobalState.pszCurrentAllowedAdminsList, &pAllowAdminCheckData);
            if (macError)
            {
                if (macError == eDSAuthUnknownUser)
                {
                    LOG("GetAccessCheckData(%s) failed with error: eDSAuthUnknownUser. AD user accounts will not be added to admin group (GID:80), since the list provided is incorrectly specified. This error suggests that you have a user or group that is not recognized by Likewise authentication daemon. Recommend checking that system administrator has enabled the items here in the Likewise cell that applies to this computer.", GlobalState.pszCurrentAllowedAdminsList);
                }
                else
                {
                    LOG("Failed to GetAllowData(%s) with error: %d", GlobalState.pszCurrentAllowedAdminsList, macError);
                }

                LW_SAFE_FREE_STRING(GlobalState.pszCurrentAllowedAdminsList);
                GlobalState.pszCurrentAllowedAdminsList = NULL;
                pAllowAdminCheckData = NULL;
                macError = eDSNoErr;
            }
            else
            {
                LOG("AllowAdministrationBy updated to (%s)", GlobalState.pszCurrentAllowedAdminsList);
            }
        }
    }
    else
    {
        if (GlobalState.pszCurrentAllowedAdminsList)
        {
            // Former value being set to empty
            bAdminListChanged = true;
            LW_SAFE_FREE_STRING(GlobalState.pszCurrentAllowedAdminsList);
            GlobalState.pszCurrentAllowedAdminsList = NULL;
            LOG("AllowAdministrationBy updated to (not set)");
        }
    }

    /* See if Merge Admins feature is to be supported */
    if (bMergeAdmins)
    {
        NewFlags = NewFlags | LWE_DS_FLAG_DONT_REMOVE_LOCAL_ADMINS;
        if (GlobalState.Flags & LWE_DS_FLAG_DONT_REMOVE_LOCAL_ADMINS == 0)
        {
            LOG("Option to override allow-administration-by with local computer changes to the admin group is now enabled.");
        }
    }

    if (bAdminListChanged)
    {
        /* Now update the GlobalState to reflect new pAllowAdminCheckData */
        GS_ACQUIRE_EXCLUSIVE_ADMIN_ACCESS_LIST();

        if (GlobalState.pAllowAdminCheckData)
        {
            FreeAccessCheckData(GlobalState.pAllowAdminCheckData);
            GlobalState.pAllowAdminCheckData = NULL;
        }

        if (pAllowAdminCheckData)
        {
            GlobalState.pAllowAdminCheckData = pAllowAdminCheckData;
            pAllowAdminCheckData = NULL;
        }

        GS_RELEASE_ADMIN_ACCESS_LIST();
    }

    /* Now update the GlobalState to reflect new flags */
    GlobalState.Flags = NewFlags;

    if (GlobalState.fDomainControllerNotAvailable &&
        (GlobalState.OfflineTimerCount < 5))
    {
        GlobalState.OfflineTimerCount++;
        macError = eDSNoErr;
        goto cleanup;
    }

    GlobalState.fDomainControllerNotAvailable = false;
    GlobalState.OfflineTimerCount = 0;

    if (isAcquired)
    {
        pthread_mutex_unlock(&GlobalState.PeriodicTaskMutex);
        GS_RELEASE();
        isAcquired = false;
    }

    macError = RefreshGPONodes();
    if (macError)
    {
        LOG("Encountered error %d from refresh GPO nodes", macError);
        macError = eDSNoErr;
    }

cleanup:
    
    if (pszUNCProtocolForHomeLocation)
    {
        LW_SAFE_FREE_STRING(pszUNCProtocolForHomeLocation);
    }

    if (pszAllowAdministrationBy)
    {
        LW_SAFE_FREE_STRING(pszAllowAdministrationBy);
    }

    if (isAcquired)
    {
        pthread_mutex_unlock(&GlobalState.PeriodicTaskMutex);
        GS_RELEASE();
    }

    return macError;
}

// --------------------------------------------------------------------------------
// * PlugInShell_Configure ()
//
// This routine is called when the plug-in needs to invoke its configuration
// application/process.
// --------------------------------------------------------------------------------

long PlugInShell_Configure(void)
{
    long macError = eDSNoErr;

    //
    // Note that, in Mac OS X 10.4.9, at least, this is never called.
    //

    LOG_ENTER("");
    LOG_LEAVE("--> %d", macError);

    return macError;
}

// --------------------------------------------------------------------------------
// * PlugInShell_Shutdown ()
//
// This routine is called just once during the Directory Services server
// shutdown process.  The plug-in needs to perform any clean-up/shutdown
// operations at this time.
// --------------------------------------------------------------------------------

long PlugInShell_Shutdown(void)
{
    long macError = eDSNoErr;

    //
    // Note that, in Mac OS X 10.4.9, at least, this is never called.
    //

    LOG_ENTER("");

    if (GlobalState.IsInitialized)
    {
        macError = Deactivate();
        if (macError)
        {
            LOG("Deactive error '%s' (%d)", MacErrorToString(macError), macError);
        }
    }

    UninitializeContextList();
    LWICRC::Cleanup();
    LWIRecordQuery::Cleanup();
    LWIDirNodeQuery::Cleanup();
    LWIRecTypeLookup::Cleanup();
    LWIAttrLookup::Cleanup();

    if (GlobalState.IsPeriodicTaskMutexInitialized)
    {
        pthread_mutex_destroy(&GlobalState.PeriodicTaskMutex);
        GlobalState.IsPeriodicTaskMutexInitialized = false;
    }

    if (GlobalState.IsAdminAccessListLockInitialized)
    {
        pthread_rwlock_destroy(&GlobalState.AdminAccessListLock);
        GlobalState.IsAdminAccessListLockInitialized = false;
    }

    if (GlobalState.IsLockInitialized)
    {
        pthread_rwlock_destroy(&GlobalState.Lock);
        GlobalState.IsLockInitialized = false;
    }

    GlobalState.PluginState = kUninitialized | kInactive;
    GlobalState.IsInitialized = false;

    LOG_LEAVE("--> %d", macError);

    return macError;
}

static long Activate(void)
{
    long macError = eDSNoErr;
    tDataListPtr nodeNameList = NULL;
    BOOLEAN bIsStarted = FALSE;
    int i = 0;
    
    LOG_ENTER("");

    LOG("Verify that LSASS service is operational");
    GlobalState.IsStartupComplete = false;

    for (i = 0; i < 24; i++)
    {
        // Verify that startup has completed successfully for lsass service.
        GetLsaStatus(&bIsStarted);
        if (bIsStarted)
        {
            LOG("LSASS service is operational");
            GlobalState.IsStartupComplete = true;
            break;
        }

        sleep(5);
    }

    if ( !GlobalState.DsRoot )
    {
        macError = dsOpenDirService( &GlobalState.DsRoot );
        GOTO_CLEANUP_ON_MACERROR( macError );
    }

    if ( !GlobalState.NodeNameList )
    {
        nodeNameList = dsDataListAllocate(0);
        if ( !nodeNameList )
        {
            macError = eDSAllocationFailed;
            GOTO_CLEANUP_ON_MACERROR( macError );
        }

        macError = dsBuildListFromPathAlloc(0, nodeNameList, PLUGIN_ROOT_PATH, "/");
        GOTO_CLEANUP_ON_MACERROR( macError );

        macError = DSRegisterNode(GlobalState.Signature, nodeNameList, kDirNodeType);
        GOTO_CLEANUP_ON_MACERROR( macError );

        GlobalState.NodeNameList = nodeNameList;
        nodeNameList = NULL;
    }

    if ( !GlobalState.NodeDictionary )
    {
        GlobalState.NodeDictionary = CFDictionaryCreateMutable(NULL, 0,
                                                               &kCFCopyStringDictionaryKeyCallBacks,
                                                               &kCFTypeDictionaryValueCallBacks);
    }

cleanup:
    if (nodeNameList)
    {
        dsDataListDeallocate(0, nodeNameList);
        free(nodeNameList);
    }
    
    if (macError)
    {
        long localMacError = Deactivate();
        if (localMacError)
        {
            LOG_ERROR("Unexpected error: %d", localMacError);
        }
    }
    
    LOG_LEAVE("--> %d", macError);
    return macError;
}

static long Deactivate(void)
{
    long macError = eDSNoErr;
    PGROUP_POLICY_OBJECT pTemp = NULL;

    if ( GlobalState.NodeDictionary )
    {
        CFRelease(GlobalState.NodeDictionary);
        GlobalState.NodeDictionary = NULL;
    }

    if ( GlobalState.NodeNameList )
    {
        macError = DSUnregisterNode( GlobalState.Signature, GlobalState.NodeNameList );
        if (macError)
        {
            LOG_ERROR("Unregister error: %d", macError);
        }

        dsDataListDeallocate(0, GlobalState.NodeNameList);
        free(GlobalState.NodeNameList);
        GlobalState.NodeNameList = NULL;
    }

    while (GlobalState.pGPOs)
    {
        pTemp = GlobalState.pGPOs;
        GlobalState.pGPOs = pTemp->pNext;

        pTemp->pNext = NULL;

        /* Remove node representing the GPO */
        UnregisterGPONode(GlobalState.pszRealm, pTemp->pszDisplayName);

        GPA_SAFE_FREE_GPO_LIST(pTemp);
    }
    
    if ( GlobalState.pszRealm )
    {
        LW_SAFE_FREE_STRING(GlobalState.pszRealm);
        GlobalState.pszRealm = NULL;
    }

    GlobalState.IsJoinedToAD = false;

    if ( GlobalState.pNetAdapterList )
    {
        LWFreeNetAdapterList(GlobalState.pNetAdapterList);
        GlobalState.pNetAdapterList = NULL;
    }

    if ( GlobalState.DsRoot )
    {
        dsCloseDirService( GlobalState.DsRoot );
        GlobalState.DsRoot = 0;
    }

    if (GlobalState.pszCurrentAllowedAdminsList)
    {
        LW_SAFE_FREE_STRING(GlobalState.pszCurrentAllowedAdminsList);
        GlobalState.pszCurrentAllowedAdminsList = NULL;
    }

    if (GlobalState.pAllowAdminCheckData)
    {
        FreeAccessCheckData(GlobalState.pAllowAdminCheckData);
        GlobalState.pAllowAdminCheckData = NULL;
    }

    return macError;
}

static long GetDomainJoinState(PSTR* ppszDomain)
{
    long macError = eDSNoErr;
    PSTR pszDomain = NULL;

    macError = GetCurrentDomain(&pszDomain);
    if (macError)
    {
        // ISSUE-2008/10/07-dalmeida -- Assume not joined?
        macError = eDSNoErr;
    }
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (pszDomain)
    {
        LwStrToUpper(pszDomain);
    }

cleanup:
    if (macError)
    {
        LW_SAFE_FREE_STRING(pszDomain);
        pszDomain = NULL;
    }

    *ppszDomain = pszDomain;

    return macError;
}

static long RefreshGPONodes(void)
{
    long macError = eDSNoErr;
    PGROUP_POLICY_OBJECT pCurrentGPOs = NULL;
    PGROUP_POLICY_OBJECT pDeletedGPOs = NULL;
    PGROUP_POLICY_OBJECT pNewGPOs = NULL;
    PGROUP_POLICY_OBJECT pTemp = NULL;
    PSTR pszDomain = NULL;
    bool isAcquired = false;

    macError = GetDomainJoinState(&pszDomain);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (pszDomain)
    {
        if (pszDomain && GlobalState.pszRealm &&
            strcmp(pszDomain, GlobalState.pszRealm))
        {
            LOG("Unexpected domain name change: '%s' -> '%s'",
                pszDomain, GlobalState.pszRealm);
            // ISSUE-2008/10/07-dalmeida -- To support this, we would
            // need to unregister all nodes.
            macError = eDSOperationFailed;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        macError = EnumWorkgroupManagerEnabledGPOs(pszDomain, &pCurrentGPOs);
        if (macError == eDSReceiveFailed ||
            macError == eDSBogusServer ||
            macError == eDSSendFailed ||
            macError == eDSAuthMasterUnreachable)
        {
            LOG("EnumWorkgroupManagerEnableGPOs failed %d, treating as okay", macError);
            GlobalState.fDomainControllerNotAvailable = true;
            GlobalState.OfflineTimerCount = 1;
            macError = eDSNoErr;
        }

        if (macError)
        {
            LOG("EnumWorkgroupManagerEnableGPOs failed unexpectedly (error = %d)", macError);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }

    GS_ACQUIRE_SHARED();
    pthread_mutex_lock(&GlobalState.PeriodicTaskMutex);
    isAcquired = true;

    macError = GPAComputeDeletedList(pCurrentGPOs, GlobalState.pGPOs, &pDeletedGPOs);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = GPAComputeDeletedList(GlobalState.pGPOs, pCurrentGPOs, &pNewGPOs);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pTemp = pDeletedGPOs;
    while (pTemp)
    {
        LOG("Removing GPO directory node (%s)", pTemp->pszDisplayName);
        UnregisterGPONode(GlobalState.pszRealm, pTemp->pszDisplayName);
        pTemp = pTemp->pNext;
    }

    pTemp = pNewGPOs;
    while (pTemp)
    {
        LOG("Adding GPO directory node (%s)", pTemp->pszDisplayName);
        RegisterGPONode(pszDomain, pTemp->pszDisplayName);
        pTemp = pTemp->pNext;
    }

    GPA_SAFE_FREE_GPO_LIST(GlobalState.pGPOs);
    GlobalState.pGPOs = pCurrentGPOs;
    pCurrentGPOs = NULL;

    GlobalState.IsJoinedToAD = pszDomain ? true : false;
    LW_SAFE_FREE_STRING(GlobalState.pszRealm);
    GlobalState.pszRealm = pszDomain;
    pszDomain = NULL;

cleanup:

    if (isAcquired)
    {
        pthread_mutex_unlock(&GlobalState.PeriodicTaskMutex);
        GS_RELEASE();
    }

    GPA_SAFE_FREE_GPO_LIST(pCurrentGPOs);
    GPA_SAFE_FREE_GPO_LIST(pDeletedGPOs);
    GPA_SAFE_FREE_GPO_LIST(pNewGPOs);
    LW_SAFE_FREE_STRING(pszDomain);

    return macError;
}
