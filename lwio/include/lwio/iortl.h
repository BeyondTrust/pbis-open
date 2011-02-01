/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        iortl.h
 *
 * Abstract:
 *
 *        Likewise I/O Manager RTL - usable by UM and KM.
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
