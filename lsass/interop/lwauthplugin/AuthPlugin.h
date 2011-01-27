#ifndef AUTHORIZATION_PLUGIN_H
#define AUTHORIZATION_PLUGIN_H

#include <CoreServices/CoreServices.h>

#include <Security/AuthorizationPlugin.h>

#include <lw/attrs.h>

#define BAIL_ON_OS_ERROR(_osStatus) \
    do { \
        if (_osStatus) { \
           LSA_LOG_DEBUG("Mac OS Error: %s (%d)", \
                   GetMacOSStatusCommentString(_osStatus), _osStatus); \
           goto error; \
        } \
    } while(0)

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
    OSStatus (*Create)(
            IN PLW_AUTH_MECHANISM_INSTANCE pAuthMechanismInstance
            );

    /*
     * Invoke an instance of a mechanism.  It should call SetResult
     * during or after returning from this function.
     */
    OSStatus (*Invoke)(
            IN PLW_AUTH_MECHANISM_INSTANCE pAuthMechanismInstance
            );

    /*
     * Mechanism should respond with a DidDeactivate as soon as possible.
     */
    OSStatus (*Deactivate)(
            IN PLW_AUTH_MECHANISM_INSTANCE pAuthMechanismInstance
            );

    /*
     * Mechanism should clean up and release any resources it is holding.
     */
    OSStatus (*Destroy)(
            IN PLW_AUTH_MECHANISM_INSTANCE pAuthMechanismInstance
            );
};

struct _LW_AUTH_MECHANISM_INSTANCE {
    PLW_AUTH_PLUGIN             pAuthPlugin;

    AuthorizationEngineRef      pAuthEngine;

    PLW_AUTH_MECHANISM          pAuthMechanism;

    PVOID                       pMechanismData;
};

extern VOID
AuthMechanismRegister(
        PLW_AUTH_MECHANISM pAuthMechanism
        );

#endif /* AUTHORIZATION_PLUGIN_H */
