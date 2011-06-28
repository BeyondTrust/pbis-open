#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFDictionary.h>

#include <Security/AuthorizationDB.h>

#include "../AuthPlugin.h"
#include <lw/types.h>

#include <lwerror.h>

#define CF_SAFE_RELEASE(_cf_obj) \
    if (_cf_obj != NULL) CFRelease(_cf_obj)

PCSTR gMechanismName = NULL;

static PLW_AUTH_MECHANISM gpAuthMechanisms;

VOID
AuthMechanismRegister(
        PLW_AUTH_MECHANISM pAuthMechanism
        )
{
    pAuthMechanism->pNext = gpAuthMechanisms;
    gpAuthMechanisms = pAuthMechanism;
}

VOID
AuthMechanismUnRegister(
        PLW_AUTH_MECHANISM pAuthMechanism
        )
{
    PLW_AUTH_MECHANISM *ppPrev = &gpAuthMechanisms;
    PLW_AUTH_MECHANISM pCurrent;

    for (pCurrent = gpAuthMechanisms;
            pCurrent != NULL;
            ppPrev = &pCurrent->pNext, pCurrent = pCurrent->pNext)
    {
        if (pCurrent == pAuthMechanism)
        {
            *ppPrev = pCurrent->pNext;
            break;
        }
    }
}

PLW_AUTH_MECHANISM
AuthMechanismFind(
    PCSTR name
    )
{
    PLW_AUTH_MECHANISM pAuthMechanism = NULL;

    for (pAuthMechanism = gpAuthMechanisms;
            pAuthMechanism != NULL;
            pAuthMechanism = pAuthMechanism->pNext)
    {
        if (!strcmp(pAuthMechanism->name, name))
        {
            break;
        }
    }

    return pAuthMechanism;
}

DWORD
AuthPluginProcessMechanismList(
    const char *rightName,
    AuthMechanismProcessFunction pProcessFunction,
    PVOID pPrivateData
    )
{
    AuthorizationRef authorization = NULL;
    CFDictionaryRef rights = NULL;
    CFMutableDictionaryRef newRights = NULL;
    CFArrayRef mechanisms = NULL;
    CFMutableArrayRef newMechanisms = NULL;
    CFIndex numMechanisms;
    CFIndex i;
    LW_BOOLEAN done = LW_FALSE;
    DWORD dwError = LW_ERROR_SUCCESS;
    OSStatus osStatus;

    osStatus = AuthorizationRightGet(rightName, &rights);
    BAIL_ON_OS_ERROR(osStatus);

    mechanisms = CFDictionaryGetValue(rights, CFSTR("mechanisms"));
    AUTH_BAIL_ON(mechanisms == NULL, LW_ERROR_NO_SUCH_OBJECT,
            "key 'mechanisms' not found");

    numMechanisms = CFArrayGetCount(mechanisms);
    newMechanisms = CFArrayCreateMutable(NULL, numMechanisms + 1,
            &kCFTypeArrayCallBacks);
    AUTH_BAIL_ON(newMechanisms == NULL, LW_ERROR_OUT_OF_MEMORY,
            "Could not create new mechanisms array");

    for (i = 0; i < numMechanisms; ++i)
    {
        CFStringRef mechanism = CFArrayGetValueAtIndex(mechanisms, i);
        LW_AUTH_MECHANISM_ACTION action = LW_AUTH_MECHANISM_COPY;

        if (!done)
        {
            action = pProcessFunction(mechanism, newMechanisms, pPrivateData);

            if (action & LW_AUTH_MECHANISM_DONE)
            {
                done = LW_TRUE;
                action &= ~LW_AUTH_MECHANISM_DONE;
            }
        }

        if (action == LW_AUTH_MECHANISM_COPY)
        {
            CFArrayAppendValue(newMechanisms, mechanism);
        }
    }

    newRights = CFDictionaryCreateMutableCopy(NULL, 0, rights);
    AUTH_BAIL_ON(newRights == NULL, LW_ERROR_OUT_OF_MEMORY,
            "Could not create new rights dictionary");
    CFDictionaryReplaceValue(newRights, CFSTR("mechanisms"), newMechanisms);
    BAIL_ON_OS_ERROR(osStatus);

    osStatus = AuthorizationCreate(NULL, NULL, 0, &authorization);
    BAIL_ON_OS_ERROR(osStatus);

    osStatus = AuthorizationRightSet(authorization, "system.login.console",
            newRights, NULL, NULL, NULL);

cleanup:
    if (authorization != NULL)
    {
        AuthorizationFree(authorization, 0);
    }

    CF_SAFE_RELEASE(newRights);
    CF_SAFE_RELEASE(newMechanisms);
    CF_SAFE_RELEASE(rights);

    return dwError;

error:
    goto cleanup;
}
