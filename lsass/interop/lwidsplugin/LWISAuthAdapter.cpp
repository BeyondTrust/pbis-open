/*
 *  LWISAuthAdapter.cpp
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/23/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "LWISAuthAdapter.h"

/* user functions */

typedef void (*PFN_SETPWENT)();
typedef void (*PFN_ENDPWENT)();
typedef long (*PFN_GETPWENT_R)(
               struct passwd *result,
               char *buffer,
               size_t buflen,
               int *errnop);
typedef long (*PFN_GETPWUID_R)(
               uid_t uid,
               struct passwd *result,
               char *buffer,
               size_t buflen,
               int *errnop);
typedef long (*PFN_GETPWNAM_R)(
               const char *name,
               struct passwd *result,
               char *buffer,
               size_t buflen,
               int *errnop);

/* group functions */

typedef void (*PFN_SETGRENT)();
typedef void (*PFN_ENDGRENT)();
typedef long (*PFN_GETGRENT_R)(
               struct group *result,
               char *buffer,
               size_t buflen,
               int *errnop);
typedef long (*PFN_GETGRNAM_R)(
               const char *name,
               struct group *result,
               char *buffer,
               size_t buflen,
               int *errnop);
typedef long (*PFN_GETGRGID_R)(
               gid_t gid,
               struct group *result,
               char *buffer,
               size_t buflen,
               int *errnop);

/* additional functions */

typedef uint32_t (*PFN_AUTHENTICATE)(
                   const char *username,
                   const char *password,
                   bool is_auth_only);
typedef uint32_t (*PFN_CHANGE_PASSWORD)(
                  const char *username,
                  const char *old_password,
                  const char *password);
typedef uint32_t (*PFN_GET_PRINCIPAL)(
                   const char* username,
                   char** principal_name);
typedef void (*PFN_FREE_PRINCIPAL)(
               char* principal_name);
typedef uint32_t (*PFN_GET_USER_GROUPS)(
                  const char *user,
                  gid_t **groups,
                  int *num_groups);
typedef void (*PFN_FREE_USER_GROUPS)(
               gid_t * groups);

typedef struct _NSS_MODULE_FUNCTIONS {
    PFN_SETPWENT pfnsetpwent;
    PFN_ENDPWENT pfnendpwent;
    PFN_GETPWENT_R pfngetpwent_r;
    PFN_GETPWUID_R pfngetpwuid_r;
    PFN_GETPWNAM_R pfngetpwnam_r;

    PFN_SETGRENT pfnsetgrent;
    PFN_ENDGRENT pfnendgrent;
    PFN_GETGRENT_R pfngetgrent_r;
    PFN_GETGRGID_R pfngetgrgid_r;
    PFN_GETGRNAM_R pfngetgrnam_r;

    PFN_AUTHENTICATE pfnauthenticate;
    PFN_CHANGE_PASSWORD pfnchange_password;
    PFN_GET_PRINCIPAL pfnget_principal;
    PFN_FREE_PRINCIPAL pfnfree_principal;
    PFN_GET_USER_GROUPS pfnget_user_groups;
    PFN_FREE_USER_GROUPS pfnfree_user_groups;
} NSS_LSASS_MODULE_FUNCTIONS;

typedef struct _NSS_LSASS_MODULE {
    void *LibHandle;
    NSS_LSASS_MODULE_FUNCTIONS Functions;
} NSS_LSASS_MODULE;

static NSS_LSASS_MODULE LsassModuleState;

#define NSS_LSASS_LIB_PATH "/usr/lib/libnss_lsass.so"

#define NSS_LSASS_MODULE_FUNC(function) \
    (LsassModuleState.Functions.function)

#define LOAD_LSASS_NSS_FUNCTION(functionSymName, functionPtr) \
    LoadFunction(LsassModuleState.LibHandle, functionSymName, (void**)&LsassModuleState.Functions.functionPtr)

MACERROR LWISAuthAdapter::LoadFunction(void* LibHandle, const char* FunctionName, void** FunctionPointer)
{
    MACERROR macError = eDSNoErr;
    void* function;

    function = dlsym(LibHandle, FunctionName);
    if (!function)
    {
        LOG_ERROR("Failed to load symbol \"%s\" from library \"" NSS_LSASS_LIB_PATH "\" with error: %s", FunctionName, dlerror());
        macError = ePlugInInitError;
    }

    *FunctionPointer = function;

    return macError;
}

void
LWISAuthAdapter::EnterNSSLock()
{
    pthread_mutex_lock(&_nssLock);
}

void
LWISAuthAdapter::LeaveNSSLock()
{
    pthread_mutex_unlock(&_nssLock);
}

LWISAuthAdapter::LWISAuthAdapter()
{
}

LWISAuthAdapter::~LWISAuthAdapter()
{
}

MACERROR LWISAuthAdapter::Initialize()
{
    MACERROR macError = eDSNoErr;

    if (!LsassModuleState.LibHandle)
    {
        LsassModuleState.LibHandle = dlopen(NSS_LSASS_LIB_PATH, RTLD_LAZY);
        if (!LsassModuleState.LibHandle)
        {
            int libcError = errno;
            LOG_ERROR("Failed to load " NSS_LSASS_LIB_PATH ": %s (%d)", strerror(libcError), libcError);
            macError = ePlugInInitError;
            GOTO_CLEANUP();
        }
    }

    /* clear any existing error */
    dlerror();

    /* user functions */

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_setpwent", pfnsetpwent);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_endpwent", pfnendpwent);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_getpwent_r", pfngetpwent_r);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_getpwuid_r", pfngetpwuid_r);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_getpwnam_r", pfngetpwnam_r);
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* group functions */

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_setgrent", pfnsetgrent);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_endgrent", pfnendgrent);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_getgrent_r", pfngetgrent_r);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_getgrgid_r", pfngetgrgid_r);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_getgrnam_r", pfngetgrnam_r);
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* additional functions */

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_authenticate", pfnauthenticate);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_change_password", pfnchange_password);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_get_principal", pfnget_principal);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_free_principal", pfnfree_principal);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_get_user_groups", pfnget_user_groups);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LOAD_LSASS_NSS_FUNCTION("_nss_lsass_free_user_groups", pfnfree_user_groups);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    pthread_mutex_init(&_nssLock, NULL);

cleanup:

    if (macError)
    {
        Cleanup();
    }

    return macError;
}

void LWISAuthAdapter::Cleanup()
{
    if (LsassModuleState.LibHandle)
    {
        memset(&LsassModuleState.Functions, 0, sizeof(LsassModuleState.Functions));
        dlclose(LsassModuleState.LibHandle);
        LsassModuleState.LibHandle = NULL;
        
        pthread_mutex_destroy(&_nssLock);
    }
}

void
LWISAuthAdapter::setpwent(void)
{
    EnterNSSLock();
    
    NSS_LSASS_MODULE_FUNC(pfnsetpwent)();
    
    LeaveNSSLock();
}

void
LWISAuthAdapter::endpwent(void)
{
    EnterNSSLock();
    
    NSS_LSASS_MODULE_FUNC(pfnendpwent)();
    
    LeaveNSSLock();
}

long
LWISAuthAdapter::getpwent(struct passwd *result,
                         char *buffer,
                         size_t buflen,
                         int *errnop)
{
    long macError = eDSNoErr;

    EnterNSSLock();
    
    macError = NSS_LSASS_MODULE_FUNC(pfngetpwent_r)(result, buffer, buflen, errnop);

    LeaveNSSLock();

    return macError;
}

long
LWISAuthAdapter::getpwuid(
    uid_t uid,
    struct passwd *result,
    char *buffer,
    size_t buflen,
    int *errnop
    )
{
    long macError = eDSNoErr;

    EnterNSSLock();
    
    macError = NSS_LSASS_MODULE_FUNC(pfngetpwuid_r)(uid, result, buffer, buflen, errnop);

    LeaveNSSLock();

    return macError;
}

long
LWISAuthAdapter::getpwnam(
    const char *name,
    struct passwd *result,
    char *buffer,
    size_t buflen,
    int *errnop
    )
{
    long macError = eDSNoErr;

    EnterNSSLock();
    
    macError = NSS_LSASS_MODULE_FUNC(pfngetpwnam_r)(name, result, buffer, buflen, errnop);

    LeaveNSSLock();

    return macError;
}

void
LWISAuthAdapter::setgrent(void)
{
    EnterNSSLock();
    
    NSS_LSASS_MODULE_FUNC(pfnsetgrent)();

    LeaveNSSLock();
}

void
LWISAuthAdapter::endgrent(void)
{
    EnterNSSLock();
    
    NSS_LSASS_MODULE_FUNC(pfnendgrent)();

    LeaveNSSLock();
}

long
LWISAuthAdapter::getgrent(
    struct group *result,
    char *buffer,
    size_t buflen,
    int *errnop
    )
{
    long macError = eDSNoErr;

    EnterNSSLock();
    
    macError = NSS_LSASS_MODULE_FUNC(pfngetgrent_r)(result, buffer, buflen, errnop);

    LeaveNSSLock();

    return macError;
}

long
LWISAuthAdapter::getgrgid(
    gid_t gid,
    struct group *result,
    char *buffer,
    size_t buflen,
    int *errnop
    )
{
    long macError = eDSNoErr;

    EnterNSSLock();
    
    macError = NSS_LSASS_MODULE_FUNC(pfngetgrgid_r)(gid, result, buffer, buflen, errnop);

    LeaveNSSLock();

    return macError;
}

long
LWISAuthAdapter::getgrnam(
    const char *name,
    struct group *result,
    char *buffer,
    size_t buflen,
    int *errnop
    )
{
    long macError = eDSNoErr;

    EnterNSSLock();
    
    macError = NSS_LSASS_MODULE_FUNC(pfngetgrnam_r)(name, result, buffer, buflen, errnop);

    LeaveNSSLock();

    return macError;
}

uint32_t
LWISAuthAdapter::authenticate(
    const char *username,
    const char *password,
    bool is_auth_only
    )
{
    uint32_t macError = eDSNoErr;

    EnterNSSLock();
    
    macError = NSS_LSASS_MODULE_FUNC(pfnauthenticate)(username, password, is_auth_only);

    LeaveNSSLock();

    return macError;
}

uint32_t
LWISAuthAdapter::change_password(
    const char *username,
    const char *old_password,
    const char *password
    )
{
    uint32_t macError = eDSNoErr;

    EnterNSSLock();
    
    macError = NSS_LSASS_MODULE_FUNC(pfnchange_password)(username, old_password, password);

    LeaveNSSLock();

    return macError;
}

uint32_t
LWISAuthAdapter::get_principal(
    const char *username,
    char** principal_name
    )
{
    uint32_t macError = eDSNoErr;

    EnterNSSLock();
    
    macError = NSS_LSASS_MODULE_FUNC(pfnget_principal)(username, principal_name);

    LeaveNSSLock();

    return macError;
}

void
LWISAuthAdapter::free_principal(
    char* principal_name
    )
{
    EnterNSSLock();
    
    NSS_LSASS_MODULE_FUNC(pfnfree_principal)(principal_name);

    LeaveNSSLock();
}

uint32_t
LWISAuthAdapter::get_user_groups(
    const char *user,
    gid_t **groups,
    int *num_groups
    )
{
    uint32_t macError = eDSNoErr;

    EnterNSSLock();
    
    macError = NSS_LSASS_MODULE_FUNC(pfnget_user_groups)(user, groups, num_groups);

    LeaveNSSLock();

    return macError;
}

void
LWISAuthAdapter::free_user_groups(
    gid_t *groups
    )
{
    EnterNSSLock();
    
    NSS_LSASS_MODULE_FUNC(pfnfree_user_groups)(groups);

    LeaveNSSLock();
}

