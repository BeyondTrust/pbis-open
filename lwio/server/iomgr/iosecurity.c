/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#include "iop.h"

#define IO_SECURITY_INVALID_UID  (uid_t)-1
#define IO_SECURITY_INVALID_GID  (gid_t)-1

typedef struct _IO_CREATE_SECURITY_CONTEXT {
    LONG ReferenceCount;
    IO_SECURITY_CONTEXT_PROCESS_INFORMATION Process;
    PACCESS_TOKEN AccessToken;
    LW_PIO_CREDS Credentials;
} IO_CREATE_SECURITY_CONTEXT;

PIO_SECURITY_CONTEXT_PROCESS_INFORMATION
IoSecurityGetProcessInfo(
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext
    )
{
    if ((SecurityContext->Process.Uid == IO_SECURITY_INVALID_UID) ||
        (SecurityContext->Process.Gid == IO_SECURITY_INVALID_GID))
    {
        return NULL;
    }
    
    return &SecurityContext->Process;
}

PACCESS_TOKEN
IoSecurityGetAccessToken(
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext
    )
{
    return SecurityContext->AccessToken;
}

LW_PIO_CREDS
IoSecurityGetCredentials(
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext
    )
{
    return SecurityContext->Credentials;
}

static
VOID
IopSecurityFreeSecurityContext(
    IN OUT PIO_CREATE_SECURITY_CONTEXT* SecurityContext
    )
{
    PIO_CREATE_SECURITY_CONTEXT securityContext = *SecurityContext;

    if (securityContext)
    {
        LWIO_ASSERT(0 == LwInterlockedRead(&securityContext->ReferenceCount));
        RtlReleaseAccessToken(&securityContext->AccessToken);
        IO_FREE(&securityContext);
        *SecurityContext = NULL;
    }
}

VOID
IopSecurityReferenceSecurityContext(
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext
    )
{
    LONG count = InterlockedIncrement(&SecurityContext->ReferenceCount);
    LWIO_ASSERT(count > 1);
}

VOID
IoSecurityDereferenceSecurityContext(
    IN OUT PIO_CREATE_SECURITY_CONTEXT* SecurityContext
    )
{
    PIO_CREATE_SECURITY_CONTEXT securityContext = *SecurityContext;

    if (securityContext)
    {
        LONG count = InterlockedDecrement(&securityContext->ReferenceCount);
        LWIO_ASSERT(count >= 0);
        if (0 == count)
        {
            IopSecurityFreeSecurityContext(&securityContext);
        }
        *SecurityContext = NULL;
    }
}

static
NTSTATUS
IopSecurityCreateSecurityContext(
    OUT PIO_CREATE_SECURITY_CONTEXT* SecurityContext,
    IN uid_t Uid,
    IN gid_t Gid,
    IN PACCESS_TOKEN AccessToken,
    IN OPTIONAL LW_PIO_CREDS Credentials
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_CREATE_SECURITY_CONTEXT securityContext = NULL;

    status = RTL_ALLOCATE(&securityContext, IO_CREATE_SECURITY_CONTEXT, sizeof(*securityContext));
    GOTO_CLEANUP_ON_STATUS(status);

    securityContext->ReferenceCount = 1;
    securityContext->Process.Uid = Uid;
    securityContext->Process.Gid = Gid;
    securityContext->AccessToken = AccessToken;
    RtlReferenceAccessToken(AccessToken);
    securityContext->Credentials = Credentials;

    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        IoSecurityDereferenceSecurityContext(&securityContext);
    }

    *SecurityContext = securityContext;

    return status;
}

NTSTATUS
IoSecurityCreateSecurityContextFromUidGid(
    OUT PIO_CREATE_SECURITY_CONTEXT* SecurityContext,
    IN uid_t Uid,
    IN gid_t Gid,
    IN OPTIONAL LW_PIO_CREDS Credentials
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_MAP_SECURITY_CONTEXT context = NULL;
    PACCESS_TOKEN accessToken = NULL;
    PIO_CREATE_SECURITY_CONTEXT securityContext = NULL;

    // do not free this context
    status = IopGetMapSecurityContext(&context);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateAccessTokenFromUidGid(
                    context,
                    &accessToken,
                    Uid,
                    Gid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = IopSecurityCreateSecurityContext(
                    &securityContext,
                    Uid,
                    Gid,
                    accessToken,
                    Credentials);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        IoSecurityDereferenceSecurityContext(&securityContext);
    }

    RtlReleaseAccessToken(&accessToken);

    *SecurityContext = securityContext;

    return status;
}

NTSTATUS
IoSecurityCreateSecurityContextFromUsername(
    OUT PIO_CREATE_SECURITY_CONTEXT* SecurityContext,
    IN PUNICODE_STRING Username
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_MAP_SECURITY_CONTEXT context = NULL;
    PACCESS_TOKEN accessToken = NULL;
    PIO_CREATE_SECURITY_CONTEXT securityContext = NULL;
    TOKEN_UNIX tokenUnix = { 0 };

    // do not free this context
    status = IopGetMapSecurityContext(&context);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
                    context,
                    &accessToken,
                    Username);
    GOTO_CLEANUP_ON_STATUS(status);

    // TODO-Do we want to keep process information as the
    // current process or just take TOKEN_UNIX info?

    status = RtlQueryAccessTokenUnixInformation(
                    accessToken,
                    &tokenUnix);
    if (status == STATUS_NOT_FOUND)
    {
       tokenUnix.Uid = IO_SECURITY_INVALID_UID;
       tokenUnix.Gid = IO_SECURITY_INVALID_GID;
       status = STATUS_SUCCESS;
    }
    GOTO_CLEANUP_ON_STATUS(status);

    status = IopSecurityCreateSecurityContext(
                    &securityContext,
                    tokenUnix.Uid,
                    tokenUnix.Gid,
                    accessToken,
                    NULL);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        IoSecurityDereferenceSecurityContext(&securityContext);
    }

    RtlReleaseAccessToken(&accessToken);

    *SecurityContext = securityContext;

    return status;
}

NTSTATUS
IoSecurityCreateSecurityContextFromGssContext(
    OUT PIO_CREATE_SECURITY_CONTEXT* SecurityContext,
    IN LW_MAP_SECURITY_GSS_CONTEXT hGssContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_MAP_SECURITY_CONTEXT context = NULL;
    PACCESS_TOKEN accessToken = NULL;
    PIO_CREATE_SECURITY_CONTEXT securityContext = NULL;
    TOKEN_UNIX tokenUnix = { 0 };

    // do not free this context
    status = IopGetMapSecurityContext(&context);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateAccessTokenFromGssContext(
                    context,
                    &accessToken,
                    hGssContext);
    GOTO_CLEANUP_ON_STATUS(status);

    // TODO-Do we want to keep process information as the
    // current process or just take TOKEN_UNIX info?

    status = RtlQueryAccessTokenUnixInformation(
                    accessToken,
                    &tokenUnix);
    if (status == STATUS_NOT_FOUND)
    {
       tokenUnix.Uid = IO_SECURITY_INVALID_UID;
       tokenUnix.Gid = IO_SECURITY_INVALID_GID;
       status = STATUS_SUCCESS;
    }
    GOTO_CLEANUP_ON_STATUS(status);

    status = IopSecurityCreateSecurityContext(
                    &securityContext,
                    tokenUnix.Uid,
                    tokenUnix.Gid,
                    accessToken,
                    NULL);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        IoSecurityDereferenceSecurityContext(&securityContext);
    }

    RtlReleaseAccessToken(&accessToken);

    *SecurityContext = securityContext;

    return status;
}

NTSTATUS
IoSecurityCreateSecurityContextFromNtlmLogon(
    OUT PIO_CREATE_SECURITY_CONTEXT* ppSecurityContext,
    OUT PLW_MAP_SECURITY_NTLM_LOGON_RESULT* ppNtlmLogonResult,
    IN PLW_MAP_SECURITY_NTLM_LOGON_INFO pNtlmLogonInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_MAP_SECURITY_CONTEXT pMapSecurityContext = NULL;
    PACCESS_TOKEN pAccessToken = NULL;
    TOKEN_UNIX tokenUnix = { 0 };

    PIO_CREATE_SECURITY_CONTEXT pSecurityContext = NULL;
    PLW_MAP_SECURITY_NTLM_LOGON_RESULT pNtlmLogonResult = NULL;

    // TOOD - make the context to be a global variable within the module.  Take a look at pvfs code.
    status = LwMapSecurityCreateContext(&pMapSecurityContext);
    GOTO_CLEANUP_ON_STATUS(status);
    
    status = LwMapSecurityCreateAccessTokenFromNtlmLogon(
                    pMapSecurityContext,
                    &pAccessToken,
                    pNtlmLogonInfo,
                    &pNtlmLogonResult);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlQueryAccessTokenUnixInformation(
                    pAccessToken,
                    &tokenUnix);
    if (status == STATUS_NOT_FOUND)
    {
       tokenUnix.Uid = IO_SECURITY_INVALID_UID;
       tokenUnix.Gid = IO_SECURITY_INVALID_GID;
       status = STATUS_SUCCESS;
    }
    GOTO_CLEANUP_ON_STATUS(status);

    status = IopSecurityCreateSecurityContext(
                    &pSecurityContext,
                    tokenUnix.Uid,
                    tokenUnix.Gid,
                    pAccessToken,
                    NULL);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    
    if (!NT_SUCCESS(status))
    {
        IoSecurityDereferenceSecurityContext(&pSecurityContext);

        if (pMapSecurityContext && pNtlmLogonResult)
        {
            LwMapSecurityFreeNtlmLogonResult(pMapSecurityContext, &pNtlmLogonResult);
        }
    }

    if (pAccessToken)
    {
        RtlReleaseAccessToken(&pAccessToken);
    }

    LwMapSecurityFreeContext(&pMapSecurityContext);

    *ppSecurityContext = pSecurityContext;
    *ppNtlmLogonResult = pNtlmLogonResult;

    return status;
}

VOID
IoSecurityFreeNtlmLogonResult(
    IN OUT PLW_MAP_SECURITY_NTLM_LOGON_RESULT* ppNtlmLogonResult
    )
{
    PLW_MAP_SECURITY_CONTEXT pMapSecurityContext = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    PLW_MAP_SECURITY_NTLM_LOGON_RESULT pNtlmLogonResult = *ppNtlmLogonResult;

    if (pNtlmLogonResult)
    {
        status = LwMapSecurityCreateContext(&pMapSecurityContext);
    
        if (status == STATUS_SUCCESS)
        {
            LwMapSecurityFreeNtlmLogonResult(pMapSecurityContext, &pNtlmLogonResult);
        }
        LwMapSecurityFreeContext(&pMapSecurityContext);

        *ppNtlmLogonResult = pNtlmLogonResult;
    }
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
