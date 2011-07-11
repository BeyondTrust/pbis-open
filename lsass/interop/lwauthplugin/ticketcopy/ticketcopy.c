#include <Security/AuthorizationDB.h>
#include <Kerberos/krb5.h>

#include <lw/types.h>
#include "../AuthPlugin.h"

#include <lwerror.h>

#include <Kerberos/krb5.h>

#include <lsa/lsa.h>
#include <lsautils.h>

#define HOMEDIR_MECHANISM_PREFIX        "HomeDirMechanism:"
#define TICKETCOPY_MECHANISM            "com.likewise:ticketcopy,privileged"

#define BAIL_ON_KRB5_ERROR(_ctx, _krb5_err)                     \
    do {                                                        \
        if (_krb5_err)                                          \
        {                                                       \
            AUTH_BAIL(LW_ERROR_INTERNAL,                        \
                        "kerberos error %d",                    \
                        _krb5_err);                             \
        }                                                       \
    } while (0)

static DWORD
TicketCopy(
    IN PLW_AUTH_MECHANISM_INSTANCE pMechanismInstance
    )
{
    OSStatus osStatus;
    const AuthorizationCallbacks *pAuthCallbacks;
    AuthorizationEngineRef pAuthEngine;
    AuthorizationContextFlags flags;
    const AuthorizationValue *value;
    uid_t uid;
    HANDLE lsaConnection = (HANDLE) NULL;
    PVOID pUserInfo = NULL;
    krb5_context krb5Context = NULL;
    char krb5CachePath[PATH_MAX];
    krb5_ccache krb5SourceCache = NULL;
    krb5_ccache krb5DestCache = NULL;
    krb5_cc_cursor krb5Cursor = NULL;
    krb5_creds krb5Credentials = { 0 };
    krb5_principal krb5Principal = NULL;
    krb5_error_code krb5Error;
    DWORD dwError;

    pAuthCallbacks = pMechanismInstance->pAuthPlugin->pAuthCallbacks;
    pAuthEngine = pMechanismInstance->pAuthEngine;

    /* Regardless of whether copying the tickets works, let the user log in. */
    osStatus = pAuthCallbacks->SetResult(
                    pAuthEngine,
                    kAuthorizationResultAllow);
    BAIL_ON_OS_ERROR(osStatus);

    osStatus = pAuthCallbacks->GetContextValue(
                                pAuthEngine,
                                "uid",
                                &flags,
                                &value);
    BAIL_ON_OS_ERROR(osStatus);

    if (value->length != sizeof(uid_t))
    {
        BAIL_WITH_LSA_ERROR(LW_ERROR_INVALID_PARAMETER);
    }

    uid = *((uid_t *) value->data);

    /* Make sure we're running as an AD user. */
    dwError = LsaOpenServer(&lsaConnection);
    BAIL_ON_AUTH_ERROR(dwError);

    dwError = LsaFindUserById(
                    lsaConnection,
                    uid,
                    0,
                    &pUserInfo);
    if (dwError == LW_ERROR_NO_SUCH_USER)
    {
        AUTH_LOG_DEBUG("uid %lu is not an AD user", (unsigned long) uid);
        dwError = LW_ERROR_SUCCESS;
        goto cleanup;
    }
    BAIL_ON_AUTH_ERROR(dwError);

    BAIL_ON_UNIX_ERROR(seteuid(uid) == -1, "seteuid(%u) failed", uid);

    krb5Error = krb5_init_context(&krb5Context);
    BAIL_ON_KRB5_ERROR(krb5Context, krb5Error);

    snprintf(
        krb5CachePath,
        sizeof(krb5CachePath),
        "FILE:/tmp/krb5cc_%lu",
        (unsigned long) uid);

    krb5Error = krb5_cc_resolve(
                    krb5Context,
                    krb5CachePath,
                    &krb5SourceCache);
    BAIL_ON_KRB5_ERROR(krb5Context, krb5Error);

    krb5Error = krb5_cc_default(krb5Context, &krb5DestCache);
    BAIL_ON_KRB5_ERROR(krb5Context, krb5Error);

    /*
     * Turn off KRB5_TC_OPENCLOSE so the file will be opened once
     * and kept open.  This causes it to actually attempt to open
     * the file, so this is where we check for the file not
     * existing.
     */
    krb5Error = krb5_cc_set_flags(krb5Context, krb5SourceCache, 0);
    if (krb5Error == KRB5_FCC_NOFILE)
    {
        dwError = LW_ERROR_SUCCESS;
        goto cleanup;
    }
    BAIL_ON_KRB5_ERROR(krb5Context, krb5Error);

    krb5Error = krb5_cc_start_seq_get(
                    krb5Context,
                    krb5SourceCache,
                    &krb5Cursor);
    BAIL_ON_KRB5_ERROR(krb5Context, krb5Error);

    while ((krb5Error = krb5_cc_next_cred(
                            krb5Context,
                            krb5SourceCache,
                            &krb5Cursor,
                            &krb5Credentials)) == 0)
    {
        krb5Error = krb5_cc_store_cred(
                        krb5Context,
                        krb5DestCache,
                        &krb5Credentials);
        if (krb5Error == KRB5_FCC_NOFILE)
        {
            krb5Error = krb5_cc_get_principal(
                            krb5Context,
                            krb5SourceCache,
                            &krb5Principal);
            BAIL_ON_KRB5_ERROR(krb5Context, krb5Error);

            krb5Error = krb5_cc_initialize(
                            krb5Context,
                            krb5DestCache,
                            krb5Principal);
            BAIL_ON_KRB5_ERROR(krb5Context, krb5Error);

            krb5Error = krb5_cc_store_cred(
                            krb5Context,
                            krb5DestCache,
                            &krb5Credentials);
        }
        BAIL_ON_KRB5_ERROR(krb5Context, krb5Error);

        krb5_free_cred_contents(krb5Context, &krb5Credentials);
    }

    if (krb5Error != KRB5_CC_END)
    {
        BAIL_ON_KRB5_ERROR(krb5Context, krb5Error);
    }

    krb5Error = krb5_cc_end_seq_get(
                    krb5Context,
                    krb5SourceCache,
                    &krb5Cursor);
    krb5Cursor = NULL;
    BAIL_ON_KRB5_ERROR(krb5Context, krb5Error);

    AUTH_LOG_DEBUG("copied tickets from %s", krb5CachePath);

cleanup:
    krb5_free_cred_contents(krb5Context, &krb5Credentials);

    if (krb5Cursor)
    {
        krb5_cc_end_seq_get(krb5Context, krb5SourceCache, &krb5Cursor);
    }

    if (krb5SourceCache)
    {
        krb5_cc_close(krb5Context, krb5SourceCache);
    }

    if (krb5DestCache)
    {
        krb5_cc_close(krb5Context, krb5DestCache);
    }

    if (krb5Principal)
    {
        krb5_free_principal(krb5Context, krb5Principal);
    }

    if (krb5Context)
    {
        krb5_free_context(krb5Context);
    }

    if (pUserInfo)
    {
        LsaFreeUserInfo(0, pUserInfo);
    }

    if (lsaConnection != (HANDLE) NULL)
    {
        LsaCloseServer(lsaConnection);
    }

    seteuid(0);

    return dwError;

error:
    /* Login should never fail as a result of this module. */
    dwError = LW_ERROR_SUCCESS;

    goto cleanup;
}

static LW_AUTH_MECHANISM gTicketCopyAuthMechanism = {
    .name = "ticketcopy",
    .Invoke = TicketCopy,
};

VOID
Register(
    VOID
    )
{
    AuthMechanismRegister(&gTicketCopyAuthMechanism);
}

VOID
UnRegister(
    VOID
    )
{
    AuthMechanismUnRegister(&gTicketCopyAuthMechanism);
}

static
LW_AUTH_MECHANISM_ACTION
AddMechanism(
    CFStringRef mechanism,
    CFMutableArrayRef newMechanismList,
    PVOID pPrivateData
    )
{
    LW_AUTH_MECHANISM_ACTION action = LW_AUTH_MECHANISM_COPY;

    if (CFStringCompareWithOptions(mechanism,
                CFSTR(HOMEDIR_MECHANISM_PREFIX),
                CFRangeMake(0, sizeof(HOMEDIR_MECHANISM_PREFIX) - 1), 0) ==
            kCFCompareEqualTo)
    {
        CFArrayAppendValue(newMechanismList, CFSTR(TICKETCOPY_MECHANISM));
        action = LW_AUTH_MECHANISM_COPY | LW_AUTH_MECHANISM_DONE;
    }

    return action;
}

static
LW_AUTH_MECHANISM_ACTION
RemoveMechanism(
    CFStringRef mechanism,
    CFMutableArrayRef newMechanismList,
    PVOID pPrivateData
    )
{
    LW_AUTH_MECHANISM_ACTION action = LW_AUTH_MECHANISM_COPY;

    if (CFStringCompare(mechanism, CFSTR(TICKETCOPY_MECHANISM), 0) ==
            kCFCompareEqualTo)
    {
        action = LW_AUTH_MECHANISM_SKIP | LW_AUTH_MECHANISM_DONE;
    }

    return action;
}

DWORD
Enable(
    VOID
    )
{
    return AuthPluginProcessMechanismList(
                "system.login.console",
                AddMechanism,
                NULL);
}

DWORD
Disable(
    VOID
    )
{
    return AuthPluginProcessMechanismList(
                "system.login.console",
                RemoveMechanism,
                NULL);
}
