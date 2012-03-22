/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        mapsecurity.c
 *
 * Abstract:
 *
 *        Likewise Map Security - Access Token Create Information
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "config.h"
#include <lw/base.h>
#include <lw/mapsecurity-plugin.h>
#include <lw/security-api.h>
#include <lw/rtlgoto.h>
#include <lw/safeint.h>
#include <pthread.h>
#include <stdlib.h>
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#define ENABLE_LOGGING 0

#if ENABLE_LOGGING
#include <stdio.h>
#endif

#define SAFE_LOG_STRING(String) \
    ( (String) ? (String) : "<null>" )

#if ENABLE_LOGGING
#define LOG_ERROR(Format, ...) \
    fprintf(stderr, Format, ## __VA_ARGS__)
#else
#define LOG_ERROR(Format, ...)
#endif

#define ASSERT(Expression) \
    do { \
        if (!(Expression)) \
        { \
            LOG_ERROR("ASSERTION FAILED: " # Expression); \
            abort(); \
        } \
    } while (0)

#define LOCK_MUTEX(pMutex, bInLock) \
    do { \
        int _unixError = pthread_mutex_lock(pMutex); \
        if (_unixError) \
        { \
            LOG_ERROR("ABORTING: Failed to lock mutex (error = %d).", _unixError); \
            abort(); \
        } \
        bInLock = TRUE; \
    } while (0)

#define UNLOCK_MUTEX(pMutex, bInLock) \
    do { \
        if (bInLock) \
        { \
            int _unixError = pthread_mutex_unlock(pMutex); \
            if (_unixError) \
            { \
                LOG_ERROR("ABORTING: Failed to unlock mutex (error = %d).", _unixError); \
                abort(); \
            } \
            bInLock = FALSE; \
        } \
    } while (0)

#define LW_MAP_SECURITY_PLUGIN_PATH LIBDIR "/lw-map-sec/lsass" MOD_EXT

//
// Unmapped Unix User and Group SIDs
//
// S-1-22-1-UID for users
// S-1-22-2-GID for groups
//

#define SECURITY_UNMAPPED_UNIX_AUTHORITY    { 0, 0, 0, 0, 0, 22 }
#define SECURITY_UNMAPPED_UNIX_UID_RID      1
#define SECURITY_UNMAPPED_UNIX_GID_RID      2
#define SECURITY_UNMAPPED_UNIX_RID_COUNT    2

#define SECURITY_UNMAPPED_UNIX_UID_PREFIX "S-1-22-1-"
#define SECURITY_UNMAPPED_UNIX_GID_PREFIX "S-1-22-2-"

//
// Structures
//

typedef struct _LW_MAP_SECURITY_CONTEXT {
    PSTR LibraryPath;
    PVOID LibraryHandle;
    PLW_MAP_SECURITY_PLUGIN_CONTEXT PluginContext;
    PLW_MAP_SECURITY_PLUGIN_INTERFACE PluginInterface;
} LW_MAP_SECURITY_CONTEXT;

typedef struct _LW_MAP_SECURITY_STATE {
    pthread_mutex_t Mutex;
    LONG InitCount;
    LONG RefCount;
    PLW_MAP_SECURITY_CONTEXT Context;
    LWMSP_CREATE_CONTEXT_CALLBACK pCreateContextCallback;
} LW_MAP_SECURITY_STATE, *PLW_MAP_SECURITY_STATE;

//
// Global State
//

LW_MAP_SECURITY_STATE gLwMapSecurityState = {
    .Mutex                  = PTHREAD_MUTEX_INITIALIZER,
    .pCreateContextCallback = NULL
};

//
// Prototypes
//

static
NTSTATUS
LwMapSecurityCreateContextInternal(
    OUT PLW_MAP_SECURITY_CONTEXT* Context,
    IN  LWMSP_CREATE_CONTEXT_CALLBACK pCreateContextCallback
    );

static
VOID
LwMapSecurityFreeContextInternal(
    IN OUT PLW_MAP_SECURITY_CONTEXT* Context
    );

static
VOID
LwMapSecurityFreeContextInLock(
    IN OUT PLW_MAP_SECURITY_CONTEXT* Context
    );

//
// Plugin Functions
//

NTSTATUS
LwMapSecurityInitializeSidFromUnmappedId(
    IN ULONG SidSize,
    OUT PSID Sid,
    IN BOOLEAN IsUser,
    IN ULONG Id
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_UNMAPPED_UNIX_AUTHORITY };

    if (SidSize < RtlLengthRequiredSid(SECURITY_UNMAPPED_UNIX_RID_COUNT))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    status = RtlInitializeSid(
                    Sid,
                    &identifierAuthority,
                    SECURITY_UNMAPPED_UNIX_RID_COUNT);
    GOTO_CLEANUP_ON_STATUS(status);

    if (IsUser)
    {
        Sid->SubAuthority[0] = SECURITY_UNMAPPED_UNIX_UID_RID;
    }
    else
    {
        Sid->SubAuthority[0] = SECURITY_UNMAPPED_UNIX_GID_RID;
    }

    Sid->SubAuthority[1] = Id;

cleanup:
    return status;
}

//
// API Functions
//

NTSTATUS
LwMapSecurityInitialize(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    LOCK_MUTEX(&gLwMapSecurityState.Mutex, bInLock);

    gLwMapSecurityState.InitCount++;
    ASSERT(gLwMapSecurityState.InitCount >= 1);

    if (!gLwMapSecurityState.Context)
    {
        // Ignore failure
        status = LwMapSecurityCreateContextInternal(
                                 &gLwMapSecurityState.Context,
                                 gLwMapSecurityState.pCreateContextCallback);
        status = STATUS_SUCCESS;
    }

    UNLOCK_MUTEX(&gLwMapSecurityState.Mutex, bInLock);

    return status;
}

VOID
LwMapSecurityCleanup(
    VOID
    )
{
    BOOLEAN bInLock = FALSE;

    LOCK_MUTEX(&gLwMapSecurityState.Mutex, bInLock);

    gLwMapSecurityState.InitCount--;
    ASSERT(gLwMapSecurityState.InitCount >= 0);

    if (0 == gLwMapSecurityState.InitCount)
    {
        if (0 == gLwMapSecurityState.RefCount)
        {
            LwMapSecurityFreeContextInternal(&gLwMapSecurityState.Context);
        }
        else
        {
            LOG_ERROR("lwmapsecurity refcount != 0 (%d) - probably resource leak",
                      gLwMapSecurityState.RefCount);
        }
    }

    UNLOCK_MUTEX(&gLwMapSecurityState.Mutex, bInLock);
}

NTSTATUS
LwMapSecurityCreateContext(
    OUT PLW_MAP_SECURITY_CONTEXT* Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PLW_MAP_SECURITY_CONTEXT pContext = NULL;
    BOOLEAN bInLock = FALSE;

    LOCK_MUTEX(&gLwMapSecurityState.Mutex, bInLock);

    if (!gLwMapSecurityState.Context)
    {
        status = LwMapSecurityCreateContextInternal(
                                  &gLwMapSecurityState.Context,
                                  gLwMapSecurityState.pCreateContextCallback);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    pContext = gLwMapSecurityState.Context;
    gLwMapSecurityState.RefCount++;
    ASSERT(gLwMapSecurityState.RefCount >= 1);

cleanup:
    UNLOCK_MUTEX(&gLwMapSecurityState.Mutex, bInLock);

    *Context = pContext;

    return status;
}

static
VOID
LwMapSecurityFreeContextInLock(
    IN OUT PLW_MAP_SECURITY_CONTEXT* Context
    )
{
    PLW_MAP_SECURITY_CONTEXT context = *Context;

    // We must be coming into this block with the MapSecurityState
    // mutex locked 
    if (context)
    {
        if (context != gLwMapSecurityState.Context)
        {
            LwMapSecurityFreeContextInternal(&context);
            goto cleanup;
        }

        // This is our global Context

        gLwMapSecurityState.RefCount--;
        if (gLwMapSecurityState.RefCount < 0)
        {
            // Logic error.  Bail out.  May leak memory.
            goto cleanup;
        }
            
        if ((0 == gLwMapSecurityState.RefCount) &&
            (0 == gLwMapSecurityState.InitCount))
        {
            LwMapSecurityFreeContextInternal(&gLwMapSecurityState.Context);
        }
    }

cleanup:
    *Context = NULL;

    return;
}

VOID
LwMapSecurityFreeContext(
    IN OUT PLW_MAP_SECURITY_CONTEXT* Context
    )
{
    BOOLEAN bInLock = FALSE;

    LOCK_MUTEX(&gLwMapSecurityState.Mutex, bInLock);

    LwMapSecurityFreeContextInLock(Context);

    UNLOCK_MUTEX(&gLwMapSecurityState.Mutex, bInLock);
}

static
NTSTATUS
LwMapSecurityCreateContextInternal(
    OUT PLW_MAP_SECURITY_CONTEXT*     Context,
    IN  LWMSP_CREATE_CONTEXT_CALLBACK pCallback
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PCSTR pszError = NULL;
    PLW_MAP_SECURITY_CONTEXT pContext = NULL;
    LWMSP_CREATE_CONTEXT_CALLBACK pCreateContextCallback = NULL;

    status = RTL_ALLOCATE(&pContext, LW_MAP_SECURITY_CONTEXT, sizeof(*pContext));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (!pCallback)
    {
        status = RtlCStringDuplicate(&pContext->LibraryPath, LW_MAP_SECURITY_PLUGIN_PATH);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        dlerror();

        pContext->LibraryHandle = dlopen(pContext->LibraryPath, RTLD_NOW | RTLD_GLOBAL);
        if (!pContext->LibraryHandle)
        {
#if ENABLE_LOGGING
            int error = errno;
#endif
            pszError = dlerror();

            LOG_ERROR("Failed to load %s (%s (%d))", pContext->LibraryPath,
                      SAFE_LOG_STRING(pszError), error);

            status = STATUS_UNSUCCESSFUL;
            GOTO_CLEANUP_EE(EE);
        }

        dlerror();
        pCreateContextCallback = (LWMSP_CREATE_CONTEXT_CALLBACK)
                                      dlsym(pContext->LibraryHandle,
                                            LWMSP_CREATE_CONTEXT_FUNCTION_NAME);
        if (!pCreateContextCallback)
        {
            pszError = dlerror();

            LOG_ERROR("Failed to load " LWMSP_CREATE_CONTEXT_FUNCTION_NAME
                      " function from %s (%s)",
                      pContext->LibraryPath, SAFE_LOG_STRING(pszError));

            status = STATUS_UNSUCCESSFUL;
            GOTO_CLEANUP_EE(EE);
        }
    }
    else
    {
        pCreateContextCallback = pCallback;
        if (!pCreateContextCallback)
        {
            LOG_ERROR("Couldn't find any context create callback function.");

            status = STATUS_UNSUCCESSFUL;
            GOTO_CLEANUP_EE(EE);
        }
    }

    status = pCreateContextCallback(&pContext->PluginContext,
                                    &pContext->PluginInterface);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (!NT_SUCCESS(status))
    {
        LwMapSecurityFreeContextInLock(&pContext);
    }

    *Context = pContext;

    return status;
}

static
VOID
LwMapSecurityFreeContextInternal(
    IN OUT PLW_MAP_SECURITY_CONTEXT* Context
    )
{
    PLW_MAP_SECURITY_CONTEXT context = *Context;

    if (context)
    {
        if (context->PluginContext)
        {
            context->PluginInterface->FreeContext(&context->PluginContext);
        }
        if (context->LibraryHandle)
        {
            int err = dlclose(context->LibraryHandle);
            if (err)
            {
                LOG_ERROR("Failed to dlclose() %s", context->LibraryPath);
            }
        }
        RtlCStringFree(&context->LibraryPath);
        RtlMemoryFree(context);
        *Context = NULL;
    }
}

NTSTATUS
LwMapSecurityGetIdFromSid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PBOOLEAN IsUser,
    OUT PULONG Id,
    IN PSID Sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SID_IDENTIFIER_AUTHORITY unixIdAuthority = { SECURITY_UNMAPPED_UNIX_AUTHORITY };
    SID_IDENTIFIER_AUTHORITY ntIdAuthority = { SECURITY_NT_AUTHORITY };
    BOOLEAN isUser = FALSE;
    ULONG id = 0;

    if (RtlEqualMemory(&Sid->IdentifierAuthority,
                       &unixIdAuthority,
                       sizeof(unixIdAuthority)))
    {
        if (Sid->SubAuthorityCount != SECURITY_UNMAPPED_UNIX_RID_COUNT)
        {
            status = STATUS_INVALID_SID;
            GOTO_CLEANUP();
        }
        switch (Sid->SubAuthority[0])
        {
            case SECURITY_UNMAPPED_UNIX_UID_RID:
                isUser = TRUE;
                id = Sid->SubAuthority[1];
                break;
            case SECURITY_UNMAPPED_UNIX_GID_RID:
                isUser = FALSE;
                id = Sid->SubAuthority[1];
                break;
            default:
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
        }
    }
    else if (Sid->SubAuthorityCount == 1 &&
             Sid->SubAuthority[0] == SECURITY_LOCAL_SYSTEM_RID &&
             RtlEqualMemory(&Sid->IdentifierAuthority,
                            &ntIdAuthority,
                            sizeof(ntIdAuthority)))
    {
        isUser = TRUE;
        id = 0;
    }
    else
    {
        status = Context->PluginInterface->GetIdFromSid(
                        Context->PluginContext,
                        &isUser,
                        &id,
                        Sid);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:
    if (!NT_SUCCESS(status))
    {
        isUser = FALSE;
        id = (ULONG) -1;
    }

    *IsUser = isUser;
    *Id = id;

    return status;
}

NTSTATUS
LwMapSecurityGetSidFromId(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PSID* Sid,
    IN BOOLEAN IsUser,
    IN ULONG Id
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;

    if (0 == Id)
    {
        union {
            SID Sid;
            BYTE Buffer[SID_MAX_SIZE];
        } sidBuffer;
        ULONG ulSidSize = sizeof(sidBuffer);
        
        if (IsUser)
        {
            status = RtlCreateWellKnownSid(WinLocalSystemSid,
                                           NULL,
                                           &sidBuffer.Sid,
                                           &ulSidSize);
            GOTO_CLEANUP_ON_STATUS(status);
        }
        else
        {
            status = LwMapSecurityInitializeSidFromUnmappedId(
                                           sizeof(sidBuffer),
                                           &sidBuffer.Sid ,
                                           IsUser,
                                           Id);
            GOTO_CLEANUP_ON_STATUS(status);
        }

        status = Context->PluginInterface->DuplicateSid(
                        Context->PluginContext,
                        &sid,
                        &sidBuffer.Sid);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        status = Context->PluginInterface->GetSidFromId(
                        Context->PluginContext,
                        &sid,
                        IsUser,
                        Id);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:
    if (!NT_SUCCESS(status) && sid)
    {
        Context->PluginInterface->FreeSid(
                        Context->PluginContext,
                        &sid);
    }

    *Sid = sid;

    return status;
}

NTSTATUS
LwMapSecurityGetSidFromName(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PSID* Sid,
    IN BOOLEAN IsUser,
    IN PCSTR Name
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;

    status = Context->PluginInterface->GetSidFromName(
                    Context->PluginContext,
                    &sid,
                    IsUser,
                    Name);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status) && sid)
    {
        Context->PluginInterface->FreeSid(
                        Context->PluginContext,
                        &sid);
    }

    *Sid = sid;

    return status;
}

NTSTATUS
LwMapSecurityGetNameFromSid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PSTR* Domain,
    OUT PSTR* Name,
    OUT PBOOLEAN IsUser,
    IN PSID Sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN isUser = FALSE;
    PSTR name = NULL;
    PSTR domain = NULL;

    status = Context->PluginInterface->GetNameFromSid(
                    Context->PluginContext,
                    &domain,
                    &name,
                    &isUser,
                    Sid);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        isUser = FALSE;

        if (name)
        {
            Context->PluginInterface->FreeCString(
                Context->PluginContext,
                &name);
            name = NULL;
        }
 
       if (domain)
        {
            Context->PluginInterface->FreeCString(
                Context->PluginContext,
                &domain);
            domain = NULL;
        }

    }

    *Domain = domain;
    *Name = name;
    *IsUser =isUser;

    return status;
}


VOID
LwMapSecurityFreeSid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    IN OUT PSID* Sid
    )
{
    if (*Sid)
    {
        Context->PluginInterface->FreeSid(
            Context->PluginContext,
            Sid);
    }
}

VOID
LwMapSecurityFreeCString(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    IN OUT PSTR* String
    )
{
    if (*String)
    {
        Context->PluginInterface->FreeCString(
            Context->PluginContext,
            String);
    }
}

static
VOID
LwMapSecurityFreeAccessTokenCreateInformation(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    IN OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation
    )
{
    if (*CreateInformation)
    {
        Context->PluginInterface->FreeAccessTokenCreateInformation(
            Context->PluginContext,
            CreateInformation);
    }
}

static
NTSTATUS
LwMapSecurityCreateExtendedGroups(
    OUT PTOKEN_GROUPS* ExtendedTokenGroups,
    IN PTOKEN_GROUPS OriginalTokenGroups,
    IN ULONG SidCount,
    IN PSID* Sids
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PTOKEN_GROUPS tokenGroups = NULL;
    ULONG groupCount = OriginalTokenGroups->GroupCount + SidCount;
    ULONG size = 0;
    ULONG i = 0;

    status = RtlSafeMultiplyULONG(&size, groupCount, sizeof(tokenGroups->Groups[0]));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*tokenGroups));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RTL_ALLOCATE(&tokenGroups, TOKEN_GROUPS, size);
    GOTO_CLEANUP_ON_STATUS(status);

    for (i = 0; i < SidCount; i++)
    {
        tokenGroups->Groups[tokenGroups->GroupCount].Attributes = SE_GROUP_ENABLED;
        tokenGroups->Groups[tokenGroups->GroupCount].Sid = Sids[i];
        tokenGroups->GroupCount++;
    }

    for (i = 0; i < OriginalTokenGroups->GroupCount; i++)
    {
        tokenGroups->Groups[tokenGroups->GroupCount] = OriginalTokenGroups->Groups[i];
        tokenGroups->GroupCount++;
    }

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&tokenGroups);
    }

    *ExtendedTokenGroups = tokenGroups;

    return status;
}

static
NTSTATUS
LwMapSecurityCreateExtendedAccessToken(
    OUT PACCESS_TOKEN* AccessToken,
    IN PTOKEN_USER User,
    IN PTOKEN_GROUPS Groups,
    IN PTOKEN_PRIVILEGES Privileges,
    IN PTOKEN_OWNER Owner,
    IN PTOKEN_PRIMARY_GROUP PrimaryGroup,
    IN PTOKEN_DEFAULT_DACL DefaultDacl,
    IN OPTIONAL PTOKEN_UNIX Unix
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN accessToken = NULL;
    ULONG sidBuffer1[SID_MAX_SIZE / sizeof(ULONG) + 1] = { 0 };
    ULONG sidBuffer2[SID_MAX_SIZE / sizeof(ULONG) + 1] = { 0 };
    PSID sids[2] = { (PSID) sidBuffer1, (PSID) sidBuffer2 };
    ULONG sidCount = 0;
    ULONG size = 0;
    PTOKEN_GROUPS extendedGroups = NULL;
    ULONG ownerRid = 0;

    if (!Owner)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RtlGetRidSid(&ownerRid, Owner->Owner);
    GOTO_CLEANUP_ON_STATUS(status);

    size = sizeof(sidBuffer1);
    status = RtlCreateWellKnownSid(
                    WinWorldSid,
                    NULL,
                    (PSID) sidBuffer1,
                    &size);
    GOTO_CLEANUP_ON_STATUS(status);

    sidCount++;    

    // Only Add "NT AUTHORITY\Authenticated Users" for non-Guest

    if (ownerRid != DOMAIN_USER_RID_GUEST)
    {
        size = sizeof(sidBuffer2);
        status = RtlCreateWellKnownSid(
                     WinAuthenticatedUserSid,
                     NULL,
                     (PSID) sidBuffer2,
                     &size);
        GOTO_CLEANUP_ON_STATUS(status);

        sidCount++;
    }

    status = LwMapSecurityCreateExtendedGroups(
                    &extendedGroups,
                    Groups,
                    sidCount,
                    sids);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCreateAccessToken(
                    &accessToken,
                    User,
                    extendedGroups,
                    Privileges,
                    Owner,
                    PrimaryGroup,
                    DefaultDacl,
                    Unix);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (accessToken)
        {
            RtlReleaseAccessToken(&accessToken);
        }
    }

    RTL_FREE(&extendedGroups);

    *AccessToken = accessToken;

    return status;
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromUidGid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN ULONG Uid,
    IN ULONG Gid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN accessToken = NULL;
    PSID userSid = NULL;
    PSID groupSid = NULL;
    PACL freeDefaultDacl = NULL;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;

    if (0 == Uid)
    {
        TOKEN_USER tokenUser = { { 0 } };
        union {
            TOKEN_GROUPS tokenGroups;
            struct {
                ULONG GroupCount;
                SID_AND_ATTRIBUTES Groups[1];
            };
        } tokenGroupsUnion = { .tokenGroups = { 0 } };
        TOKEN_PRIVILEGES tokenPrivileges = { 0 };
        TOKEN_OWNER tokenOwner = { 0 };
        TOKEN_PRIMARY_GROUP tokenPrimaryGroup = { 0 };
        TOKEN_DEFAULT_DACL tokenDefaultDacl = { 0 };
        TOKEN_UNIX tokenUnix = { 0 };
        ULONG ulDaclSize = 0;

        /* Force gid to be 0 to avoid deadlocking call back into
           lsass when it runs with non-zero gid (Solaris, HP-UX) */
        Gid = 0;

        status = LwMapSecurityGetSidFromId(Context, &userSid, TRUE, Uid);
        GOTO_CLEANUP_ON_STATUS(status);

        status = LwMapSecurityGetSidFromId(Context, &groupSid, FALSE, Gid);
        GOTO_CLEANUP_ON_STATUS(status);

        tokenUser.User.Sid = userSid;

        tokenGroupsUnion.tokenGroups.GroupCount = 1;
        tokenGroupsUnion.tokenGroups.Groups[0].Sid = groupSid;

        tokenOwner.Owner = userSid;
        tokenPrimaryGroup.PrimaryGroup = groupSid;

        tokenUnix.Uid = Uid;
        tokenUnix.Gid = Gid;
        tokenUnix.Umask = 0;

        ulDaclSize = ACL_HEADER_SIZE +
            sizeof(ACCESS_ALLOWED_ACE) +
            RtlLengthSid(userSid) +
            sizeof(ULONG);

        status = LW_RTL_ALLOCATE(
                     &tokenDefaultDacl.DefaultDacl,
                     VOID,
                     ulDaclSize);
        GOTO_CLEANUP_ON_STATUS(status);

        freeDefaultDacl = tokenDefaultDacl.DefaultDacl;

        status = RtlCreateAcl(
                     tokenDefaultDacl.DefaultDacl,
                     ulDaclSize,
                     ACL_REVISION);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlAddAccessAllowedAceEx(
                     tokenDefaultDacl.DefaultDacl,
                     ACL_REVISION,
                     0,
                     GENERIC_ALL,
                     userSid);
        GOTO_CLEANUP_ON_STATUS(status);

        status = LwMapSecurityCreateExtendedAccessToken(
                        &accessToken,
                        &tokenUser,
                        &tokenGroupsUnion.tokenGroups,
                        &tokenPrivileges,
                        &tokenOwner,
                        &tokenPrimaryGroup,
                        &tokenDefaultDacl,
                        &tokenUnix);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        status = Context->PluginInterface->GetAccessTokenCreateInformationFromUid(
                        Context->PluginContext,
                        &createInformation,
                        Uid,
                        &Gid);
        GOTO_CLEANUP_ON_STATUS(status);

        status = LwMapSecurityCreateExtendedAccessToken(
                        &accessToken,
                        createInformation->User,
                        createInformation->Groups,
                        createInformation->Privileges,
                        createInformation->Owner,
                        createInformation->PrimaryGroup,
                        createInformation->DefaultDacl,
                        createInformation->Unix);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (accessToken)
        {
            RtlReleaseAccessToken(&accessToken);
        }
    }

    LwMapSecurityFreeSid(Context, &userSid);
    LwMapSecurityFreeSid(Context, &groupSid);
    RTL_FREE(&freeDefaultDacl);
    LwMapSecurityFreeAccessTokenCreateInformation(Context, &createInformation);

    *AccessToken = accessToken;

    return status;
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PUNICODE_STRING Username
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN accessToken = NULL;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;

    status = Context->PluginInterface->GetAccessTokenCreateInformationFromUsername(
                    Context->PluginContext,
                    &createInformation,
                    Username);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateExtendedAccessToken(
                    &accessToken,
                    createInformation->User,
                    createInformation->Groups,
                    createInformation->Privileges,
                    createInformation->Owner,
                    createInformation->PrimaryGroup,
                    createInformation->DefaultDacl,
                    createInformation->Unix);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (accessToken)
        {
            RtlReleaseAccessToken(&accessToken);
        }
    }

    LwMapSecurityFreeAccessTokenCreateInformation(Context, &createInformation);

    *AccessToken = accessToken;

    return status;
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromGssContext(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN LW_MAP_SECURITY_GSS_CONTEXT GssContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN accessToken = NULL;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;

    status = Context->PluginInterface->GetAccessTokenCreateInformationFromGssContext(
                    Context->PluginContext,
                    &createInformation,
                    GssContext);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateExtendedAccessToken(
                    &accessToken,
                    createInformation->User,
                    createInformation->Groups,
                    createInformation->Privileges,
                    createInformation->Owner,
                    createInformation->PrimaryGroup,
                    createInformation->DefaultDacl,
                    createInformation->Unix);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (accessToken)
        {
            RtlReleaseAccessToken(&accessToken);
        }
    }

    LwMapSecurityFreeAccessTokenCreateInformation(Context, &createInformation);

    *AccessToken = accessToken;

    return status;
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromAnsiStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PANSI_STRING Username
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN accessToken = NULL;
    UNICODE_STRING unicodeUsername = { 0 };

    status = LwRtlUnicodeStringAllocateFromAnsiString(
                    &unicodeUsername,
                    Username);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
                    Context,
                    &accessToken,
                    &unicodeUsername);
    GOTO_CLEANUP_ON_STATUS(status);
    
cleanup:
    if (!NT_SUCCESS(status))
    {
        if (accessToken)
        {
            RtlReleaseAccessToken(&accessToken);
        }
    }

    LwRtlUnicodeStringFree(&unicodeUsername);

    *AccessToken = accessToken;

    return status;
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromWC16StringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PCWSTR Username
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN accessToken = NULL;
    UNICODE_STRING unicodeUsername = { 0 };

    status = LwRtlUnicodeStringAllocateFromWC16String(
                    &unicodeUsername,
                    Username);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
                    Context,
                    &accessToken,
                    &unicodeUsername);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (accessToken)
        {
            RtlReleaseAccessToken(&accessToken);
        }
    }

    LwRtlUnicodeStringFree(&unicodeUsername);

    *AccessToken = accessToken;

    return status;
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromCStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PCSTR Username
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN accessToken = NULL;
    UNICODE_STRING unicodeUsername = { 0 };

    status = LwRtlUnicodeStringAllocateFromCString(
                    &unicodeUsername,
                    Username);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
                    Context,
                    &accessToken,
                    &unicodeUsername);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (accessToken)
        {
            RtlReleaseAccessToken(&accessToken);
        }
    }

    LwRtlUnicodeStringFree(&unicodeUsername);

    *AccessToken = accessToken;

    return status;
}

NTSTATUS
LwMapSecurityCreateAccessTokenFromNtlmLogon(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* ppAccessToken,
    IN PLW_MAP_SECURITY_NTLM_LOGON_INFO pNtlmInfo,
    OUT PLW_MAP_SECURITY_NTLM_LOGON_RESULT* ppNtlmResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN pAccessToken = NULL;
    PLW_MAP_SECURITY_NTLM_LOGON_RESULT pNtlmResult = NULL;
    PACCESS_TOKEN_CREATE_INFORMATION pCreateInformation = NULL;

    status = Context->PluginInterface->GetAccessTokenCreateInformationFromNtlmLogon(
                    Context->PluginContext,
                    &pCreateInformation,
                    pNtlmInfo,
                    &pNtlmResult);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateExtendedAccessToken(
                    &pAccessToken,
                    pCreateInformation->User,
                    pCreateInformation->Groups,
                    pCreateInformation->Privileges,
                    pCreateInformation->Owner,
                    pCreateInformation->PrimaryGroup,
                    pCreateInformation->DefaultDacl,
                    pCreateInformation->Unix);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:

    if (!NT_SUCCESS(status))
    {
        if (pAccessToken)
        {
            RtlReleaseAccessToken(&pAccessToken);
        }
        if (pNtlmResult)
        {
            LwMapSecurityFreeNtlmLogonResult(Context, &pNtlmResult);
        }
    }

    LwMapSecurityFreeAccessTokenCreateInformation(Context, &pCreateInformation);

    *ppAccessToken = pAccessToken;
    *ppNtlmResult = pNtlmResult;

    return status;
}

VOID
LwMapSecurityFreeNtlmLogonResult(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    IN OUT PLW_MAP_SECURITY_NTLM_LOGON_RESULT* ppNtlmResult
    )
{
    if (*ppNtlmResult)
    {
        Context->PluginInterface->FreeNtlmLogonResult(
                    Context->PluginContext,
                    ppNtlmResult);
    }
}

NTSTATUS
LwMapSecurityGetLocalGuestAccountSid(
    IN PLW_MAP_SECURITY_CONTEXT pContext,
    OUT PSID* ppSid
    )
{
    return pContext->PluginInterface->GetLocalGuestAccountSid(
                    pContext->PluginContext,
                    ppSid);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
