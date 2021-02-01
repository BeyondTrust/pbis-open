/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        iortl.h
 *
 * Abstract:
 *
 *        BeyondTrust I/O Manager RTL - usable by UM and KM.
 *        This contains RTL code that is specific to I/O
 *        Manager.
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#ifndef __IO_RTL_H__
#define __IO_RTL_H__

#include <lwio/io-types.h>

typedef VOID (*PIO_ECP_FREE_CONTEXT_CALLBACK)(IN PVOID pContext);

NTSTATUS
IoRtlEcpListAllocate(
    OUT PIO_ECP_LIST* ppEcpList
    );

// Will automatically clean up ECPs in the list.
VOID
IoRtlEcpListFree(
    IN OUT PIO_ECP_LIST* ppEcpList
    );

ULONG
IoRtlEcpListGetCount(
    IN OPTIONAL PIO_ECP_LIST pEcpList
    );

NTSTATUS
IoRtlEcpListGetNext(
    IN PIO_ECP_LIST pEcpList,
    IN OPTIONAL PCSTR pszCurrentType,
    OUT PCSTR* ppszNextType,
    OUT OPTIONAL PVOID* ppNextContext,
    OUT OPTIONAL PULONG pNextContextSize
    );

NTSTATUS
IoRtlEcpListFind(
    IN PIO_ECP_LIST pEcpList,
    IN PCSTR pszType,
    OUT OPTIONAL PVOID* ppContext,
    OUT OPTIONAL PULONG pContextSize
    );

NTSTATUS
IoRtlEcpListInsert(
    IN PIO_ECP_LIST pEcpList,
    IN PCSTR pszType,
    IN PVOID pContext,
    IN ULONG ContextSize,
    IN OPTIONAL PIO_ECP_FREE_CONTEXT_CALLBACK pfnFreeContextCallback
    );

NTSTATUS
IoRtlEcpListRemove(
    IN PIO_ECP_LIST pEcpList,
    IN PCSTR pszType,
    OUT PVOID* ppContext,
    OUT OPTIONAL PULONG pContextSize,
    OUT PIO_ECP_FREE_CONTEXT_CALLBACK* ppfnFreeContextCallback
    );

NTSTATUS
IoRtlEcpListAcknowledge(
    IN PIO_ECP_LIST pEcpList,
    IN PCSTR pszType
    );

BOOLEAN
IoRtlEcpListIsAcknowledged(
    IN PIO_ECP_LIST pEcpList,
    IN PCSTR pszType
    );

BOOLEAN
IoRtlPathIsSeparator(
    IN WCHAR Character
    );

VOID
IoRtlPathDissect(
    IN PUNICODE_STRING Path,
    OUT OPTIONAL PUNICODE_STRING FirstComponent,
    OUT OPTIONAL PUNICODE_STRING RemainingPath
    );

VOID
IoRtlPathSkipSeparators(
    IN PUNICODE_STRING Path,
    OUT PUNICODE_STRING NewPath
    );

NTSTATUS
IoRtlPathUncToInternal(
    PCWSTR pwszUncPath,
    PWSTR* ppwszInternalPath
    );

#endif /* __IO_RTL_H__ */
