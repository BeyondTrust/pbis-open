/*
  File:         CPlugInShell.c

  Version:      Directory Services 1.0

  Copyright:    © 1999-2001 by Apple Computer, Inc., all rights reserved.

  *****************************

  Add plug-in functionality to this file.

  *****************************
  */

#include "LWIPlugIn.h"
#include "LWIRecordListQuery.h"
#include "LWIAttrValDataQuery.h"
#include "LWIDirNodeQuery.h"
#include "LWIRecordQuery.h"
#include "lwerror.h"

// Local helper functions
//

static long Activate(void);
static long Deactivate(void);
static long ProcessDirNodeAuth(sDoDirNodeAuth* pDirNodeAuth);


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
    bool IsInitialized;
} PLUGIN_STATE, *PPLUGIN_STATE;

static PLUGIN_STATE GlobalState;

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

#define GS_VERIFY_INITIALIZED(macError) \
    do { \
        if (!GlobalState.IsInitialized) \
        { \
            LOG_ERROR("Not initialized"); \
            macError = ePlugInFailedToInitialize; \
            GOTO_CLEANUP(); \
        } \
    } while (0)


#define GET_NODE_STR(node) \
                    SAFE_LOG_STR((node) ? (node)->fBufferData : 0)

#define GET_NODE_LEN(node) \
                    ((node) ? (node)->fBufferLength : 0)

#define GET_NODE_SIZE(node) \
                    ((node) ? (node)->fBufferSize : 0)


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

    macError = LWIAttrLookup::Initialize();
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIRecTypeLookup::Initialize();
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIDirNodeQuery::Initialize();
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIRecordQuery::Initialize();
    GOTO_CLEANUP_ON_MACERROR(macError);

    LWICRC::Initialize();

    if (pthread_rwlock_init(&GlobalState.Lock, NULL) < 0)
    {
        int libcError = errno;
        LOG_ERROR("Failied to init mutex: %s (%d)", strerror(libcError), libcError);
        macError = ePlugInInitError;
        GOTO_CLEANUP();
    }

    GlobalState.IsInitialized = true;
    GlobalState.PluginState = kInitialized | kInactive;

    //
    // If we supported custom calls while not active, we would need to create a
    // node for that here.
    //

    cleanup:
    if (macError)
    {
        // This is the only error code allowed in the failure case.
        macError = ePlugInInitError;

        if (!GlobalState.IsInitialized)
        {
            PlugInShell_Shutdown();
            GlobalState.PluginState = kFailedToInit | kInactive;
        }
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

    //
    // There are a few cases where we can expect to be called before we are "active":
    //
    //   kServerRunLoop
    //   kKerberosMutex
    //
    // If we so choose to support custom calls while "inactive", we would also expect
    // to get:
    //
    //   kOpenDirNode
    //   kDoPlugInCustomCall
    //
    // In any case, we should be initialized.
    //

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

    // ISSUE-2007/05/30-dalmeida -- We should use r/w locks instead so that
    // we can behave sensibly when someone tries to de-activate the plug-in
    // while we are processing something...

    try
    {
        switch ( msgType )
        {
        case kOpenDirNode:
            macError = LWIDirNodeQuery::Open((sOpenDirNode *)inData);
            break;

        case kCloseDirNode:
            macError = LWIDirNodeQuery::Close((sCloseDirNode *)inData);
            break;

        case kGetDirNodeInfo:
            macError = LWIDirNodeQuery::GetInfo((sGetDirNodeInfo *)inData);
            break;

        case kGetAttributeEntry:
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
            macError = LWIRecordListQuery::Run((sGetRecordList *)inData);
            break;

        case kDoAttributeValueSearch:
        case kDoAttributeValueSearchWithData:
            macError = LWIAttrValDataQuery::Run((sDoAttrValueSearchWithData *)inData);
            break;

        case kDoDirNodeAuth:
            macError = ProcessDirNodeAuth((sDoDirNodeAuth *)inData);
            break;

        case kOpenRecord:
            macError = LWIRecordQuery::Open((sOpenRecord*)inData);
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
			
        case kReleaseContinueData:
        case kGetRecordEntry:
        case kSetRecordName:
        case kSetRecordType:
        case kDeleteRecord:
        case kCreateRecord:
        case kCreateRecordAndOpen: /* sCreateRecord */
        case kRemoveAttributeValue:
        case kSetAttributeValue:
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

    // No logging since function is called every 30 seconds
    // or so (on Mac OS X 10.4.7).

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
        GOTO_CLEANUP_ON_MACERROR(macError);

        pthread_rwlock_destroy(&GlobalState.Lock);
        GlobalState.IsInitialized = false;
    }

    LWIRecTypeLookup::Cleanup();
    LWIAttrLookup::Cleanup();
    LWIDirNodeQuery::Cleanup();
    LWIRecordQuery::Cleanup();
    LWICRC::Cleanup();

    GlobalState.PluginState = kUninitialized | kInactive;

cleanup:
    LOG_LEAVE("--> %d", macError);

    return macError;
}


static long Activate(void)
{
    long macError = eDSNoErr;
    tDataListPtr nodeNameList = NULL;
    bool isAcquired = false;

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

        GS_ACQUIRE_EXCLUSIVE();
        isAcquired = true;

        GlobalState.NodeNameList = nodeNameList;
        nodeNameList = NULL;

        GS_RELEASE();
        isAcquired = false;
    }

    if ( !GlobalState.NodeDictionary )
    {
        GS_ACQUIRE_EXCLUSIVE();
        isAcquired = true;

        GlobalState.NodeDictionary = CFDictionaryCreateMutable(NULL, 0,
                                                               &kCFCopyStringDictionaryKeyCallBacks,
                                                               &kCFTypeDictionaryValueCallBacks);

        GS_RELEASE();
        isAcquired = false;
    }

cleanup:

    if (isAcquired)
    {
        GS_RELEASE();
    }

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

    return macError;
}


static long Deactivate(void)
{
    long macError = eDSNoErr;
    bool isAcquired = false;

    GS_ACQUIRE_EXCLUSIVE();
    isAcquired = true;

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

    if ( GlobalState.DsRoot )
    {
        dsCloseDirService( GlobalState.DsRoot );
        GlobalState.DsRoot = 0;
    }

    if (isAcquired)
    {
        GS_RELEASE();
    }

    return macError;
}


static long GetCountedString(tDataBufferPtr BufferData, size_t* Offset, char** Result, size_t* Length)
{
    long macError = eDSNoErr;
    int EE = 0;
    size_t offset = *Offset;
    char* result = NULL;
    int32_t length;

    LOG_ENTER("");

    if (!BufferData->fBufferData)
    {
        macError = eDSNullDataBuff;
        GOTO_CLEANUP_EE(EE);
    }

    if (offset >= BufferData->fBufferLength)
    {
        macError = eDSAuthInBuffFormatError;
        GOTO_CLEANUP_EE(EE);
    }

    if ((BufferData->fBufferLength - offset) < sizeof(int32_t))
    {
        macError = eDSAuthInBuffFormatError;
        GOTO_CLEANUP_EE(EE);
    }

    memcpy(&length, BufferData->fBufferData + offset, sizeof(int32_t));
    offset += sizeof(int32_t);

    if (length < 0)
    {
        macError = eDSAuthInBuffFormatError;
        GOTO_CLEANUP_EE(EE);
    }

    result = (char*) malloc(length + 1);
    if (!result)
    {
        macError = eMemoryAllocError;
        GOTO_CLEANUP_EE(EE);
    }

    memcpy(result, BufferData->fBufferData + offset, length);
    result[length] = 0;
    offset += length;

    macError = eDSNoErr;

cleanup:
    if (macError)
    {
        if (result)
        {
            free(result);
            result = NULL;
        }
        length = 0;
        offset = *Offset;
    }

    *Result = result;
    if (Length)
    {
        *Length = length;
    }
    if (!macError)
    {
        *Offset = offset;
    }

    LOG_LEAVE("--> %d (EE = %d)", macError, EE);

    return macError;
}


#ifdef DEBUG_PASSWORD
#define DEBUG_USER_PASSWORD(username, oldPassword, password) \
    do { \
        if (oldPassword) { \
            LOG_PARAM("username = \"%s\", oldPassword = \"%s\", newPassword = \"%s\"", \
                      username, oldPassword, password); \
        } else { \
            LOG_PARAM("username = \"%s\", password = \"%s\"", \
                      username, password); \
        } \
    } while (0);
#else
#define DEBUG_USER_PASSWORD(username, oldPassword, password) \
    do { \
        LOG_PARAM("username = \"%s\"", username); \
    } while (0)
#endif

static long ProcessDirNodeAuth(sDoDirNodeAuth* pDoDirNodeAuth)
{
    long macError = eDSAuthFailed;
    int EE = 0;
    size_t offset = 0;
    char* username = NULL;
    char* oldPassword = NULL;
    char* password = NULL;
    bool isChangePassword = false;
    bool isSetPassword = false;
    bool isAuthPassword = false;
    bool isAuthOnly = false;

    LOG_ENTER("fType = %d, fInNodeRef = %u, fInAuthMethod = %s, fInDirNodeAuthOnlyFlag = %d, fInAuthStepData = @%p => { length = %d }, fResult = %d",
              pDoDirNodeAuth->fType,
              pDoDirNodeAuth->fInNodeRef,
              GET_NODE_STR(pDoDirNodeAuth->fInAuthMethod),
              pDoDirNodeAuth->fInDirNodeAuthOnlyFlag,
              pDoDirNodeAuth->fInAuthStepData,
              GET_NODE_LEN(pDoDirNodeAuth->fInAuthStepData),
              pDoDirNodeAuth->fResult);

    if (!pDoDirNodeAuth->fInAuthMethod || !pDoDirNodeAuth->fInAuthMethod->fBufferData)
    {
        macError = eDSNullDataBuff;
        GOTO_CLEANUP_EE(EE);
    }

    isAuthOnly = pDoDirNodeAuth->fInDirNodeAuthOnlyFlag ? false : true; /* Meaning is opposite what you would expect...
                                                                           An interactive logon, or ssh logon will have
                                                                           fInDirNodeAuthOnlyFlag set to false. Workgroup Manager
                                                                           admin authentication will have this set to true. Therefore,
                                                                           to make the rest of the code in this function make more
                                                                           sense, we use isAuthOnly with meaning:
                                                                           true - user is just authenticating
                                                                           false - user is logging on and creating a session connection */

    isAuthPassword = !(strcmp(pDoDirNodeAuth->fInAuthMethod->fBufferData, kDSStdAuthClearText) &&
                       strcmp(pDoDirNodeAuth->fInAuthMethod->fBufferData, kDSStdAuthCrypt) &&
                       strcmp(pDoDirNodeAuth->fInAuthMethod->fBufferData, kDSStdAuthNodeNativeClearTextOK) &&
                       strcmp(pDoDirNodeAuth->fInAuthMethod->fBufferData, kDSStdAuthNodeNativeNoClearText));

    isChangePassword = !isAuthPassword && !strcmp(pDoDirNodeAuth->fInAuthMethod->fBufferData, kDSStdAuthChangePasswd);
    isSetPassword = !isAuthPassword && !strcmp(pDoDirNodeAuth->fInAuthMethod->fBufferData, kDSStdAuthSetPasswd);

    if (!isAuthPassword && !isChangePassword && !isSetPassword)
    {
        macError = eDSAuthMethodNotSupported;
        GOTO_CLEANUP_EE(EE);
    }

    if (!pDoDirNodeAuth->fInAuthStepData)
    {
        macError = eDSNullDataBuff;
        GOTO_CLEANUP_EE(EE);
    }
	
	if (isSetPassword)
	{
	    LOG("dsAuthMethodStandard:dsAuthSetPasswd operation not supported here");
		macError = eDSAuthMethodNotSupported;
            GOTO_CLEANUP_EE(EE);
	}

    macError = GetCountedString(pDoDirNodeAuth->fInAuthStepData, &offset, &username, NULL);
    GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);

    if (isChangePassword)
    {
        macError = GetCountedString(pDoDirNodeAuth->fInAuthStepData, &offset, &oldPassword, NULL);
        GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);
    }

    macError = GetCountedString(pDoDirNodeAuth->fInAuthStepData, &offset, &password, NULL);
    GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);

    DEBUG_USER_PASSWORD(username, oldPassword, password);

    if (isChangePassword)
    {
	LOG("Going to change password for user %s", username);
        macError = ChangePassword(username, oldPassword, password);
        GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);
    }
    else
    {
	LOG("Going to logon user: %s, Auth only: %s", username, isAuthOnly ? "yes" : "no");
        macError = AuthenticateUser(username, password, isAuthOnly);
        GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);
    }

cleanup:

    if (oldPassword)
    {
        free(oldPassword);
    }

    if (password)
    {
        free(password);
    }

    if (username)
    {
        free(username);
    }

    LOG_LEAVE("--> %d (EE = %d)", macError, EE);

    return macError;
}


