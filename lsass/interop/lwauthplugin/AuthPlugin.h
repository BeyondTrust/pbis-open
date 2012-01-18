#ifndef AUTHORIZATION_PLUGIN_H
#define AUTHORIZATION_PLUGIN_H

#include <syslog.h>

#include <CoreServices/CoreServices.h>

#include <Security/AuthorizationPlugin.h>

#include <lw/attrs.h>
#include <lw/types.h>

#include <lwdef.h>

#define PLUGIN_NAME     "com.likewise"

extern PCSTR gMechanismName;

#define _AUTH_LOG(_level, _fmt, ...) \
    syslog(_level, PLUGIN_NAME ":%s%s " _fmt, \
            (gMechanismName != NULL) ? gMechanismName : "", \
            (gMechanismName != NULL) ? ":" : "" , ## __VA_ARGS__)

#define AUTH_LOG_DEBUG(_fmt, ...) \
    _AUTH_LOG(LOG_DEBUG, _fmt , ## __VA_ARGS__)

#define AUTH_LOG_INFO(_fmt, ...) \
    _AUTH_LOG(LOG_INFO, _fmt , ## __VA_ARGS__)

#define AUTH_LOG_WARNING(_fmt, ...) \
    _AUTH_LOG(LOG_WARNING, _fmt , ## __VA_ARGS__)

#define AUTH_LOG_ERROR(_fmt, ...) \
    _AUTH_LOG(LOG_ERR, _fmt , ## __VA_ARGS__)

#define AUTH_BAIL(_dwError, _fmt, ...) \
    do { \
        dwError = _dwError; \
        AUTH_LOG_DEBUG(_fmt " # %s:%d" , ## __VA_ARGS__, __FILE__, __LINE__); \
        goto error; \
    } while (0)

#define AUTH_BAIL_ON(_expr, _dwError, _fmt, ...) \
    do { \
        if (_expr) { \
            AUTH_BAIL(_dwError, _fmt , ## __VA_ARGS__); \
        } \
    } while (0)

#define BAIL_ON_AUTH_ERROR(dwError) \
    AUTH_BAIL_ON(dwError, dwError, "Error code: %u (symbol: %s)", dwError, \
            LW_SAFE_LOG_STRING(LwWin32ExtErrorToName(dwError))); \

#define BAIL_ON_OS_ERROR(_osStatus) \
    AUTH_BAIL_ON(_osStatus != noErr, LW_ERROR_INTERNAL, \
            "Error code: %u (symbol: %s)", dwError, \
            LW_SAFE_LOG_STRING(LwWin32ExtErrorToName(dwError)))

#define BAIL_ON_UNIX_ERROR(_expr, _fmt, ...) \
    AUTH_BAIL_ON(_expr, LwMapErrnoToLwError(errno), \
            _fmt ": %s (%d)" , ## __VA_ARGS__, strerror(errno), errno)

#define AUTH_PLUGIN_ALLOCATE(__ptr) \
    LwAllocateMemory(sizeof(*(__ptr)), (LW_PVOID*) &(__ptr))

typedef struct _LW_AUTH_PLUGIN {
    const AuthorizationCallbacks        *pAuthCallbacks;
} LW_AUTH_PLUGIN, *PLW_AUTH_PLUGIN;

struct _LW_AUTH_MECHANISM;
typedef struct _LW_AUTH_MECHANISM LW_AUTH_MECHANISM, *PLW_AUTH_MECHANISM;

struct _LW_AUTH_MECHANISM_INSTANCE;
typedef struct _LW_AUTH_MECHANISM_INSTANCE LW_AUTH_MECHANISM_INSTANCE,
        *PLW_AUTH_MECHANISM_INSTANCE;

struct _LW_AUTH_MECHANISM {
    PLW_AUTH_MECHANISM  pNext;

    PSTR                name;

    /*
     * Create the per-instance data for this mechanism.
     */
    DWORD (*Create)(
            IN PLW_AUTH_MECHANISM_INSTANCE pAuthMechanismInstance
            );

    /*
     * Invoke an instance of a mechanism.  It should call SetResult
     * during or after returning from this function.
     */
    DWORD (*Invoke)(
            IN PLW_AUTH_MECHANISM_INSTANCE pAuthMechanismInstance
            );

    /*
     * Mechanism should respond with a DidDeactivate as soon as possible.
     */
    DWORD (*Deactivate)(
            IN PLW_AUTH_MECHANISM_INSTANCE pAuthMechanismInstance
            );

    /*
     * Mechanism should clean up and release any resources it is holding.
     */
    DWORD (*Destroy)(
            IN PLW_AUTH_MECHANISM_INSTANCE pAuthMechanismInstance
            );
};

struct _LW_AUTH_MECHANISM_INSTANCE {
    PLW_AUTH_PLUGIN             pAuthPlugin;

    AuthorizationEngineRef      pAuthEngine;

    PLW_AUTH_MECHANISM          pAuthMechanism;

    PVOID                       pMechanismData;
};

VOID
AuthMechanismRegister(
    PLW_AUTH_MECHANISM pAuthMechanism
    );

VOID
AuthMechanismUnRegister(
    PLW_AUTH_MECHANISM pAuthMechanism
    );

PLW_AUTH_MECHANISM
AuthMechanismFind(
    PCSTR name
    );

typedef enum _LW_AUTH_MECHANISM_ACTION
{
    LW_AUTH_MECHANISM_SKIP,
    LW_AUTH_MECHANISM_COPY,
    LW_AUTH_MECHANISM_DONE = 0x1000,
} LW_AUTH_MECHANISM_ACTION;

typedef 
LW_AUTH_MECHANISM_ACTION
(*AuthMechanismProcessFunction)(
    CFStringRef mechanismName,
    CFMutableArrayRef newMechanismList,
    PVOID pPrivateData
    );

DWORD
AuthPluginProcessMechanismList(
    const char *rightName,
    AuthMechanismProcessFunction pProcessFunction,
    PVOID pPrivateData
    );

#endif /* AUTHORIZATION_PLUGIN_H */
