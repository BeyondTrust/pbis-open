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

