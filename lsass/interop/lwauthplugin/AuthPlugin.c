#include <config.h>

#include <CoreServices/CoreServices.h>

#include <Security/AuthorizationPlugin.h>

#include <lwmem.h>
#include <lw/attrs.h>

#include <lsautils.h>

#include <dlfcn.h>
#include <glob.h>

#include "AuthPlugin.h"

typedef struct _AUTH_MECHANISM_MODULE
{
    struct _AUTH_MECHANISM_MODULE       *pNextModule;
    PVOID                               pMemory;
} AUTH_MECHANISM_MODULE, *PAUTH_MECHANISM_MODULE;

static PAUTH_MECHANISM_MODULE gAuthMechanismModules;

static OSStatus
AuthPluginDestroy(
    IN AuthorizationPluginRef pPluginRef
    );

static OSStatus
AuthMechanismCreate(
    IN AuthorizationPluginRef pPluginRef,
    IN AuthorizationEngineRef pEngineRef,
    IN AuthorizationMechanismId mechanismId,
    OUT AuthorizationMechanismRef *ppMechanismRef
    );

static OSStatus
AuthMechanismInvoke(
    IN AuthorizationMechanismRef pMechanismRef
    );

static OSStatus
AuthMechanismDeactivate(
    IN AuthorizationMechanismRef pMechanismRef
    );

static OSStatus
AuthMechanismDestroy(
    IN AuthorizationMechanismRef pMechanismRef
    );

static const AuthorizationPluginInterface gPluginInterface = {
    .version             = kAuthorizationPluginInterfaceVersion,
    .PluginDestroy       = AuthPluginDestroy,
    .MechanismCreate     = AuthMechanismCreate,
    .MechanismInvoke     = AuthMechanismInvoke,
    .MechanismDeactivate = AuthMechanismDeactivate,
    .MechanismDestroy    = AuthMechanismDestroy,
};

extern OSStatus
AuthorizationPluginCreate(
    const AuthorizationCallbacks        *pAuthCallbacks,
    AuthorizationPluginRef              *ppPlugin,
    const AuthorizationPluginInterface  **ppPluginInterface
    )
{
    LW_AUTH_PLUGIN *pPlugin = NULL;
    OSStatus osStatus = noErr;
    PAUTH_MECHANISM_MODULE pModule;
    PVOID pMemory;
    glob_t gl = { 0 };
    int result;
    DWORD dwError = LW_ERROR_SUCCESS;

    result = glob(AUTH_MECHANISM_DIR "/*.so", GLOB_NOSORT, NULL, &gl);
    BAIL_ON_UNIX_ERROR(
        result != 0 && result != GLOB_NOMATCH,
        "glob(\"%s\" failed", AUTH_MECHANISM_DIR "/*.so");

    if (result != GLOB_NOMATCH)
    {
        int i;

        for (i = 0; i < gl.gl_pathc; ++i)
        {
            VOID (*pRegisterFunction)(VOID);

            if ((pMemory = dlopen(gl.gl_pathv[i], RTLD_NOW | RTLD_LOCAL)) ==
                    NULL)
            {
                AUTH_LOG_ERROR("%s", dlerror());
                continue;
            }

            pRegisterFunction = dlsym(pMemory, "Register");
            if (pRegisterFunction == NULL)
            {
                AUTH_LOG_ERROR("%s", dlerror());
                dlclose(pMemory);
                pMemory = NULL;
                continue;
            }

            pRegisterFunction();

            dwError = AUTH_PLUGIN_ALLOCATE(pModule);
            BAIL_ON_AUTH_ERROR(dwError);

            pModule->pMemory = pMemory;
            pModule->pNextModule = gAuthMechanismModules;
            gAuthMechanismModules = pModule;
        }
    }

    dwError = AUTH_PLUGIN_ALLOCATE(pPlugin);
    BAIL_ON_AUTH_ERROR(dwError);

    pPlugin->pAuthCallbacks = pAuthCallbacks;

    *ppPlugin = pPlugin;
    *ppPluginInterface = &gPluginInterface;

cleanup:
    return osStatus;

error:
    if (pMemory)
    {
        dlclose(pMemory);
    }

    LW_SAFE_FREE_MEMORY(pModule);
    LW_SAFE_FREE_MEMORY(pPlugin);

    /*
     * The documentation says all errors should return
     * errAuthorizationInternal.
     */
    osStatus = errAuthorizationInternal;
    goto cleanup;
}

static OSStatus
AuthPluginDestroy(
        IN AuthorizationPluginRef pPluginRef
        )
{
    PAUTH_MECHANISM_MODULE pModule;
    PAUTH_MECHANISM_MODULE pNextModule;

    for (pModule = gAuthMechanismModules;
                pModule != NULL;
                pModule = pNextModule)
    {
        VOID (*pUnRegisterFunction)(VOID);

        pUnRegisterFunction = dlsym(pModule->pMemory, "UnRegister");
        if (pUnRegisterFunction != NULL)
        {
            pUnRegisterFunction();
        }

        dlclose(pModule->pMemory);

        pNextModule = pModule->pNextModule;
        LwFreeMemory(pModule);
    }

    LW_SAFE_FREE_MEMORY(pPluginRef);
    return noErr;
}

static OSStatus
AuthMechanismCreate(
        IN AuthorizationPluginRef pPluginRef,
        IN AuthorizationEngineRef pEngineRef,
        IN AuthorizationMechanismId mechanismId,
        OUT AuthorizationMechanismRef *ppMechanismRef
        )
{
    PLW_AUTH_MECHANISM pAuthMechanism = NULL;
    PLW_AUTH_MECHANISM_INSTANCE pMechanismInstance = NULL;
    OSStatus osStatus = noErr;
    DWORD dwError = LW_ERROR_SUCCESS;

    pAuthMechanism = AuthMechanismFind(mechanismId);

    if (pAuthMechanism == NULL)
    {
        AUTH_LOG_ERROR("Auth mechanism %s not found", mechanismId);
        goto error;
    }

    dwError = AUTH_PLUGIN_ALLOCATE(pMechanismInstance);
    BAIL_ON_AUTH_ERROR(dwError);

    pMechanismInstance->pAuthPlugin = pPluginRef;
    pMechanismInstance->pAuthEngine = pEngineRef;
    pMechanismInstance->pAuthMechanism = pAuthMechanism;
    pMechanismInstance->pMechanismData = NULL;

    if (pAuthMechanism->Create)
    {
        gMechanismName = pAuthMechanism->name;

        if (pAuthMechanism->Create(pMechanismInstance) != noErr)
        {
            AUTH_LOG_ERROR("Create failed");
            goto error;
        }
    }

cleanup:
    gMechanismName = NULL;
    *ppMechanismRef = pMechanismInstance;
    return osStatus;

error:
    if (pMechanismInstance)
    {
        if (pMechanismInstance->pAuthMechanism &&
                pMechanismInstance->pAuthMechanism->Destroy)
        {
            pMechanismInstance->pAuthMechanism->Destroy(pMechanismInstance);
        }

        LwFreeMemory(pMechanismInstance);
    }

    /*
     * The documentation says all errors should return
     * errAuthorizationInternal.
     */
    osStatus = errAuthorizationInternal;
    goto cleanup;
}

static OSStatus
AuthMechanismInvoke(
        IN AuthorizationMechanismRef pMechanismRef
        )
{
    PLW_AUTH_MECHANISM_INSTANCE pMechanismInstance = pMechanismRef;
    PLW_AUTH_MECHANISM pAuthMechanism;
    OSStatus osStatus = noErr;
    DWORD dwError;

    if (pMechanismInstance == NULL)
    {
        AUTH_LOG_ERROR(
            "AuthMechanismInvoke called with NULL mechanism instance");
        goto error;
    }

    pAuthMechanism = pMechanismInstance->pAuthMechanism;
    if (pAuthMechanism == NULL)
    {
        AUTH_LOG_ERROR(
            "AuthMechanismInvoke called with NULL mechanism pointer");
        goto error;
    }

    if (pAuthMechanism->Invoke != NULL)
    {
        gMechanismName = pAuthMechanism->name;

        dwError = pAuthMechanism->Invoke(pMechanismInstance);
        BAIL_ON_AUTH_ERROR(dwError);
    }

cleanup:
    gMechanismName = NULL;
    return osStatus;

error:
    /*
     * The documentation says all errors should return
     * errAuthorizationInternal.
     */
    osStatus = errAuthorizationInternal;
    goto cleanup;
}

static OSStatus
AuthMechanismDeactivate(
        IN AuthorizationMechanismRef pMechanismRef
        )
{
    PLW_AUTH_MECHANISM_INSTANCE pMechanismInstance = pMechanismRef;
    PLW_AUTH_MECHANISM pAuthMechanism;
    OSStatus osStatus = noErr;
    DWORD dwError;

    if (pMechanismInstance == NULL)
    {
        AUTH_LOG_ERROR(
            "AuthMechanismDeactivate called with NULL mechanism instance");
        goto error;
    }

    pAuthMechanism = pMechanismInstance->pAuthMechanism;
    if (pAuthMechanism == NULL)
    {
        AUTH_LOG_ERROR(
            "AuthMechanismDeactivate called with NULL mechanism pointer");
        goto error;
    }

    if (pAuthMechanism->Deactivate != NULL)
    {
        gMechanismName = pAuthMechanism->name;

        dwError = pAuthMechanism->Deactivate(pMechanismInstance);
        BAIL_ON_AUTH_ERROR(dwError);
    }

cleanup:
    gMechanismName = NULL;
    return osStatus;

error:
    /*
     * The documentation says all errors should return
     * errAuthorizationInternal.
     */
    osStatus = errAuthorizationInternal;
    goto cleanup;
}

static OSStatus
AuthMechanismDestroy(
        IN AuthorizationMechanismRef pMechanismRef
        )
{
    PLW_AUTH_MECHANISM_INSTANCE pMechanismInstance = pMechanismRef;
    PLW_AUTH_MECHANISM pAuthMechanism;
    OSStatus osStatus = noErr;
    DWORD dwError;

    if (pMechanismInstance == NULL)
    {
        AUTH_LOG_ERROR(
            "AuthMechanismDestroy called with NULL mechanism instance");
        goto error;
    }

    pAuthMechanism = pMechanismInstance->pAuthMechanism;
    if (pAuthMechanism == NULL)
    {
        AUTH_LOG_ERROR(
            "AuthMechanismDestroy called with NULL mechanism pointer");
        goto error;
    }

    if (pAuthMechanism->Destroy != NULL)
    {
        gMechanismName = pAuthMechanism->name;

        dwError = pAuthMechanism->Destroy(pMechanismInstance);
        BAIL_ON_AUTH_ERROR(dwError);
    }

cleanup:
    return osStatus;

error:
    /*
     * The documentation says all errors should return
     * errAuthorizationInternal.
     */
    osStatus = errAuthorizationInternal;
    goto cleanup;
}
