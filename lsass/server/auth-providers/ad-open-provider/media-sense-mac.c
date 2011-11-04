/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        media-sense-mac.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Media Sense Support for Mac OS X.
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <CoreFoundation/CoreFoundation.h>
#include <SystemConfiguration/SystemConfiguration.h>

#define MEDIA_SENSE_KEY_PATH "State:/Network/Global/IPv4"
#define MEDIA_SENSE_RUN_LOOP_TIMEOUT (100.0 * 365.0 * 3600.0)

struct _MEDIA_SENSE_HANDLE_DATA {
    pthread_t* Thread;
    // Error startup up the thread.
    int StartError;
    // Store reference for figuring out initial state.
    SCDynamicStoreRef StoreRef;
    // This is the event source that the thread needs to add/remove to/from
    // its run loop.
    CFRunLoopSourceRef RunLoopSourceRef;
    // The thread stores its run loop here so that it can be stopped by
    // other code.  This is necessary because the run loop is per-thread.
    CFRunLoopRef RunLoopRef;
    // The mutex and condition are used to communicate the run loop ref
    // back the the code that started the thread.
    pthread_mutex_t* Mutex;
    pthread_cond_t* Condition;
    // The online/offline transition callback.
    MEDIA_SENSE_TRANSITION_CALLBACK TransitionCallback;
    void* TransitionCallbackContext;
};

#define GOTO_CLEANUP() \
    goto cleanup

#define GOTO_CLEANUP_EE(ee) \
    do { \
        (ee) = __LINE__; \
        GOTO_CLEANUP(); \
    } while (0)

#define GOTO_CLEANUP_ON_ERROR(error) \
    do { \
        if (error) \
        { \
            GOTO_CLEANUP(); \
        } \
    } while (0)

#define GOTO_CLEANUP_ON_ERROR_EE(error, ee) \
    do { \
        if (error) \
        { \
            GOTO_CLEANUP_EE(ee); \
        } \
    } while (0)

#define _RELEASE_HELPER(Pointer, ReleaseFunction) \
    do { \
        if (Pointer) \
        { \
            (ReleaseFunction)(Pointer); \
            (Pointer) = NULL; \
        } \
    } while (0)
        
#define CF_RELEASE(Reference) \
    _RELEASE_HELPER(Reference, CFRelease)

#define FREE(Pointer) \
    _RELEASE_HELPER(Pointer, free)

#define LOG(Format, ...) \
    LSA_LOG_DEBUG(Format, ## __VA_ARGS__)

#define LOG_LEAVE(error, ee) \
    do { \
        if (error || ee) \
        { \
            LOG("LEAVE: %s() -> %d (ee = %d)", __FUNCTION__, error, ee); \
        } \
    } while(0)

static
int
MediaSenseCreateCFString(
    OUT CFStringRef* ObjectRef,
    IN const char* Data
    )
{
    int error = 0;
    *ObjectRef = CFStringCreateWithCString(NULL, Data, kCFStringEncodingUTF8);
    if (!*ObjectRef)
    {
        error = ENOMEM;
    }
    return error;
}

static
int
MediaSenseCreateCFMutableArray(
    OUT CFMutableArrayRef* ObjectRef
    )
{
    int error = 0;
    *ObjectRef = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
    if (!*ObjectRef)
    {
        error = ENOMEM;
    }
    return error;
}

static
int
MediaSenseAppendStrings(
    IN OUT CFMutableArrayRef ArrayRef,
    IN size_t Count,
    IN const char** Strings
    )
{
    int error = 0;
    CFStringRef stringRef = NULL;
    size_t i;

    for (i = 0; i < Count; i++)
    {
        error = MediaSenseCreateCFString(&stringRef, Strings[i]);
        GOTO_CLEANUP_ON_ERROR(error);
        CFArrayAppendValue(ArrayRef, stringRef);
        CF_RELEASE(stringRef);
    }

cleanup:
    CF_RELEASE(stringRef);
    return error;    
}

static
CFStringRef
MediaSenseDescribeContextInfo(
    IN const void* Info
    )
{
    return CFSTR("N/A");
}

static
int
MediaSenseAllocateStringFromCFStringRef(
    OUT char** String,
    IN CFStringRef StringRef
    )
{
    int error = 0;
    char* result = NULL;
    CFIndex size = 0;
    CFIndex length = 0;

    length = CFStringGetLength(StringRef);
    if (length <= 0)
    {
        GOTO_CLEANUP();
    }
    size = length + 1;
    if (size < length)
    {
        // overflow
        error = ERANGE;
        GOTO_CLEANUP();
    }
    result = malloc(size);
    if (!result)
    {
        error = ENOMEM;
        GOTO_CLEANUP();
    }
    if (!CFStringGetCString(StringRef, result, size, kCFStringEncodingUTF8))
    {
        error = EINVAL;
        GOTO_CLEANUP();
    }
cleanup:
    if (error)
    {
        FREE(result);
    }
    *String = result;
    return error;
}

static
int
MediaSenseDetermineOfflineState(
    IN SCDynamicStoreRef StoreRef,
    OUT bool* IsOffline,
    OUT OPTIONAL const char** ErrorString
    )
{
    int error = 0;
    int ee = 0;
    bool isOffline = false;
    CFStringRef keyRef = NULL;
    CFPropertyListRef propListRef = NULL;
    const char* errorString = NULL;

    // Do not neet to release a CFSTR-based CFStringRef as it points to static
    // text.
    keyRef = CFSTR(MEDIA_SENSE_KEY_PATH);
    if (!keyRef)
    {
        errorString = "failed to allocate static string reference";
        error = ENOMEM;
        GOTO_CLEANUP_ON_ERROR_EE(error, ee);
    }

    propListRef = SCDynamicStoreCopyValue(StoreRef, keyRef);
    if (!propListRef)
    {
        int scErrorCode = SCError();

        switch (scErrorCode)
        {
            case kSCStatusNoKey:
                isOffline = true;
                break;
            case kSCStatusOK:
            case kSCStatusFailed:
            case kSCStatusInvalidArgument:
            case kSCStatusAccessError:
            case kSCStatusKeyExists:
            case kSCStatusLocked:
            case kSCStatusNeedLock:
            case kSCStatusNoStoreSession:
            case kSCStatusNoStoreServer:
            case kSCStatusNotifierActive:
            case kSCStatusNoPrefsSession:
            case kSCStatusPrefsBusy:
            case kSCStatusNoConfigFile:
            case kSCStatusNoLink:
            case kSCStatusStale:
            case kSCStatusMaxLink:
            case kSCStatusReachabilityUnknown:
            default:
                errorString = SCErrorString(scErrorCode);
                if (!errorString)
                {
                    errorString = "*unknown*";
                }
                error = scErrorCode;
                if (!error)
                {
                    error = ENOMEM;
                }
                break;
        }
    }
    else
    {
        isOffline = false;
    }

cleanup:
    CF_RELEASE(propListRef);

    *IsOffline = isOffline;
    if (ErrorString)
    {
        *ErrorString = errorString;
    }

    LOG_LEAVE(error, ee);
    return error;
}

static
void
MediaSenseDynamicStoreCallback(
    IN SCDynamicStoreRef StoreRef,
    IN CFArrayRef ChangedKeysRef,
    IN OPTIONAL void* Context
    )
{
    CFIndex count = 0;
    CFIndex i = 0;
    char* key = NULL;
    MEDIA_SENSE_HANDLE context = (MEDIA_SENSE_HANDLE) Context;

    count = CFArrayGetCount(ChangedKeysRef);
    for (i = 0; i < count; ++i)
    {
        int error = 0;
        CFStringRef changedKeyRef = NULL;
        const char* errorString = NULL;
        bool isOffline = false;

        FREE(key);

        // Note that "get" does not reference.
        changedKeyRef = CFArrayGetValueAtIndex(ChangedKeysRef, i);
        if (!changedKeyRef)
        {
            LOG("Missing key reference at index %d", i);
            continue;
        }

        error = MediaSenseAllocateStringFromCFStringRef(&key,  changedKeyRef);
        if (error)
        {
            LOG("Error allocating string at index %d", i);
            continue;
        }

        if (!key)
        {
            LOG("Missing key string at index %d", i);
            continue;
        }

        if (strcmp(key, MEDIA_SENSE_KEY_PATH))
        {
            LOG("Changed key '%s'", key);
            continue;
        }

        error = MediaSenseDetermineOfflineState(StoreRef, &isOffline, &errorString);
        if (error)
        {
            LOG("Changed key '%s' - error determining state: %s", key, errorString);
        }
        else
        {
            LOG("Changed key '%s' - %s", key, isOffline ? "offline" : "online");
            if (context && context->TransitionCallback)
            {
                context->TransitionCallback(context->TransitionCallbackContext,
                                            isOffline);
            }
        }
    }

    FREE(key);
}

static
int
MediaSenseCreateSCDynamicStore(
    OUT SCDynamicStoreRef* ObjectRef,
    IN CFStringRef NameRef,
    IN OPTIONAL SCDynamicStoreCallBack Callback,
    IN OPTIONAL void* Context
    )
{
    int error = 0;
    SCDynamicStoreContext context = { 0 };

    if (Context)
    {
        context.version = 0;
        context.info = Context;
        // From the spec, it looks like we need this.
        context.copyDescription = MediaSenseDescribeContextInfo;
    }

    *ObjectRef = SCDynamicStoreCreate(NULL, NameRef, Callback, Context ? &context : NULL);
    if (!*ObjectRef)
    {
        LOG("SCDynamicStoreCreate failed with '%s'", SCErrorString(SCError()));
        error = ENOMEM;
    }
    return error;
}

static
int
MediaSenseCreateDynamicStoreCFRunLoopSource(
    OUT CFRunLoopSourceRef* ObjectRef,
    IN SCDynamicStoreRef StoreRef
    )
{
    int error = 0;
    *ObjectRef = SCDynamicStoreCreateRunLoopSource(NULL, StoreRef, 0);
    if (!*ObjectRef)
    {
        LOG("SCDynamicStoreCreateRunLoopSource failed with '%s'", SCErrorString(SCError()));
        error = ENOMEM;
    }
    return error;
}

#define CASE_TO_STR(x) case x: return #x

static
const char*
MediaSenseRunLoopResultToString(
    IN SInt32 RunLoopResultCode
    )
{
    switch (RunLoopResultCode)
    {
        CASE_TO_STR(kCFRunLoopRunFinished);
        CASE_TO_STR(kCFRunLoopRunStopped);
        CASE_TO_STR(kCFRunLoopRunTimedOut);
        CASE_TO_STR(kCFRunLoopRunHandledSource);
        default:
            return NULL;
    }
}

static
int
MediaSenseDetermineInitialState(
    IN MEDIA_SENSE_HANDLE Context
    )
{
    int error = 0;
    bool isOffline = false;
    const char* errorString = NULL;

    error = MediaSenseDetermineOfflineState(Context->StoreRef, &isOffline, &errorString);
    if (error)
    {
        LOG("got error %d - %s", error, errorString);
    }
    else
    {
        LOG("Starting out %s", isOffline ? "offline" : "online");
        if (Context->TransitionCallback)
        {
            Context->TransitionCallback(Context->TransitionCallbackContext,
                                        isOffline);
        }
    }
    return error;
}

static
void*
MediaSenseThreadRoutine(
    IN void* Context
    )
{
    int error = 0;
    int ee = 0;
    MEDIA_SENSE_HANDLE context = (MEDIA_SENSE_HANDLE)Context;
    CFRunLoopRef runLoopRef = CFRunLoopGetCurrent();
    CFStringRef runLoopModeRef = kCFRunLoopDefaultMode;

    // Add source to thread's run loop.
    CFRunLoopAddSource(runLoopRef,
                       context->RunLoopSourceRef,
                       runLoopModeRef);

    // Determine initial state after registering for events.
    error = MediaSenseDetermineInitialState(context);
    if (error)
    {
        // Signal thread state back to thread that started this thread
        // that we failed to start up.
        pthread_mutex_lock(context->Mutex);
        context->StartError = error;
        pthread_mutex_unlock(context->Mutex);
        pthread_cond_signal(context->Condition);
        GOTO_CLEANUP_ON_ERROR_EE(error, ee);
    }

    LOG("Starting media sense loop");

    // Signal thread state back to thread that started this thread
    // that we started up ok.
    pthread_mutex_lock(context->Mutex);
    context->RunLoopRef = runLoopRef;
    CFRetain(context->RunLoopRef);
    pthread_mutex_unlock(context->Mutex);
    pthread_cond_signal(context->Condition);

    for (;;)
    {
        SInt32 runCode = 0;
        const char* runCodeString = NULL;

        runCode = CFRunLoopRunInMode(runLoopModeRef, MEDIA_SENSE_RUN_LOOP_TIMEOUT, true);
        runCodeString = MediaSenseRunLoopResultToString(runCode);
        if (runCodeString)
        {
            LOG("run loop result = %s", runCodeString);
        }
        else
        {
            LOG("run loop result = %d", runCode);
        }
        if (kCFRunLoopRunStopped == runCode)
        {
            break;
        }
    }
    LOG("Exiting media sense loop");

cleanup:
    // Remove source from the run loop.
    CFRunLoopRemoveSource(runLoopRef,
                          context->RunLoopSourceRef,
                          runLoopModeRef);

    return NULL;
}

static
int
MediaSenseAllocate(
    OUT void** Pointer,
    IN size_t Size
    )
{
    int error = 0;
    void* pointer = NULL;

    pointer = malloc(Size);
    if (!pointer)
    {
        error = ENOMEM;
    }
    else
    {
        memset(pointer, 0, Size);
    }
    *Pointer = pointer;
    return error;
}

static
int
MediaSenseCreateMutex(
    OUT pthread_mutex_t** Mutex
    )
{
    int error = 0;
    int ee = 0;
    pthread_mutex_t* mutex = NULL;

    error = MediaSenseAllocate((void**)&mutex, sizeof(*mutex));
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

    error = pthread_mutex_init(mutex, NULL);
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

cleanup:
    if (error)
    {
        FREE(mutex);
    }
    *Mutex = mutex;
    return error;
}

static
int
MediaSenseCreateThread(
    OUT pthread_t** Thread,
    IN void* (*ThreadRoutine)(void*),
    IN void* ThreadContext
    )
{
    int error = 0;
    int ee = 0;
    pthread_t* thread = NULL;

    error = MediaSenseAllocate((void**)&thread, sizeof(*thread));
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

    error = pthread_create(thread, NULL, ThreadRoutine, ThreadContext);
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

cleanup:
    if (error)
    {
        FREE(thread);
    }
    *Thread = thread;
    return error;
}

static
int
MediaSenseCreateCondition(
    OUT pthread_cond_t** Condition
    )
{
    int error = 0;
    int ee = 0;
    pthread_cond_t* condition = NULL;

    error = MediaSenseAllocate((void**)&condition, sizeof(*condition));
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

    error = pthread_cond_init(condition, NULL);
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

cleanup:
    if (error)
    {
        FREE(condition);
    }
    *Condition = condition;
    return error;
}

static
void
MediaSenseDestroyContext(
    IN OUT MEDIA_SENSE_HANDLE* Context
    )
{
    MEDIA_SENSE_HANDLE context = *Context;

    if (context)
    {
        LOG("Destroying");
        // Signal thread to stop.
        if (context->RunLoopRef)
        {
            CFRunLoopStop(context->RunLoopRef);
            CF_RELEASE(context->RunLoopRef);
        }
        // Join the thread.
        if (context->Thread)
        {
            void* result = NULL;
            pthread_join(*context->Thread, &result);
            free(context->Thread);
        }
        CF_RELEASE(context->RunLoopSourceRef);
        CF_RELEASE(context->StoreRef);
        if (context->Mutex)
        {
            pthread_mutex_destroy(context->Mutex);
            free(context->Mutex);
        }
        if (context->Condition)
        {
            pthread_cond_destroy(context->Condition);
            free(context->Condition);
        }
        free(context);
        *Context = NULL;
    }
}

static
int
MediaSenseCreateContext(
    OUT MEDIA_SENSE_HANDLE* Context
    )
{
    int error = 0;
    int ee = 0;
    MEDIA_SENSE_HANDLE context = NULL;

    error = MediaSenseAllocate((void**)&context, sizeof(*context));
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

    error = MediaSenseCreateMutex(&context->Mutex);
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

    error = MediaSenseCreateCondition(&context->Condition);
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

cleanup:
    if (error)
    {
        MediaSenseDestroyContext(&context);
    }
    *Context = context;
    LOG_LEAVE(error, ee);
    return error;
}

static
int
MediaSenseStartEx(
    OUT MEDIA_SENSE_HANDLE* Context,
    IN OPTIONAL MEDIA_SENSE_TRANSITION_CALLBACK TransitionCallback,
    IN OPTIONAL void* TransitionCallbackContext,
    IN const char* Name,
    IN size_t Count,
    IN const char** Strings
    )
{
    int error = 0;
    int ee = 0;
    CFStringRef nameRef = NULL;
    CFMutableArrayRef notificationKeysRef = NULL;
    SCDynamicStoreRef dynamicStoreRef = NULL;
    CFRunLoopSourceRef dynamicStoreSourceRef = NULL;
    MEDIA_SENSE_HANDLE context = NULL;
    bool isAcquired = false;

    error = MediaSenseCreateCFString(&nameRef, Name);
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

    error = MediaSenseCreateCFMutableArray(&notificationKeysRef);
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

    error = MediaSenseAppendStrings(notificationKeysRef, Count, Strings);
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

    error = MediaSenseCreateContext(&context);
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

    error = MediaSenseCreateSCDynamicStore(&dynamicStoreRef,
                                    nameRef,
                                    MediaSenseDynamicStoreCallback,
                                    context);
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

    error = MediaSenseCreateDynamicStoreCFRunLoopSource(&dynamicStoreSourceRef,
                                                 dynamicStoreRef);
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

    // Now tell the store to start notifications.
    if (!SCDynamicStoreSetNotificationKeys(dynamicStoreRef, NULL, notificationKeysRef))
    {
        LOG("SCDynamicStoreCreate failed with '%s'", SCErrorString(SCError()));
        // ISSUE-2008/09/11-dalmeida -- Need better error...
        error = ENOMEM;
        GOTO_CLEANUP_EE(ee);
    }

    context->TransitionCallback = TransitionCallback;
    context->TransitionCallbackContext = TransitionCallbackContext;

    context->StoreRef = dynamicStoreRef;
    CFRetain(context->StoreRef);

    context->RunLoopSourceRef = dynamicStoreSourceRef;
    CFRetain(context->RunLoopSourceRef);

    pthread_mutex_lock(context->Mutex);
    isAcquired = true;

    error = MediaSenseCreateThread(&context->Thread, MediaSenseThreadRoutine, context);
    GOTO_CLEANUP_ON_ERROR_EE(error, ee);

    // Wait for start error or run loop reference to be returned.
    pthread_cond_wait(context->Condition, context->Mutex);
    pthread_mutex_unlock(context->Mutex);

    error = context->StartError;

cleanup:
    if (isAcquired)
    {
        pthread_mutex_unlock(context->Mutex);
    }

    CF_RELEASE(dynamicStoreSourceRef);
    CF_RELEASE(dynamicStoreRef);
    CF_RELEASE(notificationKeysRef);
    CF_RELEASE(nameRef);

    if (error)
    {
        MediaSenseDestroyContext(&context);
    }

    *Context = context;

    LOG_LEAVE(error, ee);
    return error;
}

static
int
MediaSenseStart_Mac(
    OUT MEDIA_SENSE_HANDLE* MediaSenseContext,
    IN OPTIONAL MEDIA_SENSE_TRANSITION_CALLBACK TransitionCallback,
    IN OPTIONAL void* TransitionCallbackContext
    )
{
    const char* Monitor[] = { MEDIA_SENSE_KEY_PATH };
    return MediaSenseStartEx(MediaSenseContext,
                             TransitionCallback,
                             TransitionCallbackContext,
                             "lsassd",
                             1,
                             Monitor);
}

static
void
MediaSenseStop_Mac(
    IN OUT MEDIA_SENSE_HANDLE* MediaSenseContext
    )
{
    MediaSenseDestroyContext(MediaSenseContext);
}
