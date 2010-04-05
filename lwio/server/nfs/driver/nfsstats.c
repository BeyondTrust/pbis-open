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



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        stats.c
 *
 * Abstract:
 *
 *        Likewise SMB Server Driver (NFS)
 *
 *        Server Statistics
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
NTSTATUS
NfsGetStatistics(
    PIO_STATISTICS_INFO_INPUT_BUFFER pInBuf,
    PBYTE                            lpOutBuffer,
    ULONG                            ulOutBufferSize,
    PULONG                           pulBytesTransferred
    );

static
NTSTATUS
NfsGetStatistics_level_0(
    PBYTE  lpOutBuffer,
    ULONG  ulOutBufferSize,
    PULONG pulBytesTransferred
    );

NTSTATUS
NfsProcessStatistics(
    IN     PBYTE  lpInBuffer,
    IN     ULONG  ulInBufferSize,
    IN OUT PBYTE  lpOutBuffer,
    IN     ULONG  ulOutBufferSize,
    IN OUT PULONG pulBytesTransferred
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    IO_STATISTICS_INFO_INPUT_BUFFER inBuf = {0};
    ULONG    ulBytesTransferred = 0;

    if (ulInBufferSize != sizeof(IO_STATISTICS_INFO_INPUT_BUFFER))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy((PBYTE)&inBuf, lpInBuffer, ulInBufferSize);

    switch (inBuf.ulAction)
    {
        case IO_STATISTICS_ACTION_TYPE_GET:

            ntStatus = NfsGetStatistics(
                            &inBuf,
                            lpOutBuffer,
                            ulOutBufferSize,
                            &ulBytesTransferred);

            break;

        case IO_STATISTICS_ACTION_TYPE_RESET:

            ntStatus = NfsElementsResetStats();

            break;

        default:

            ntStatus = STATUS_INVALID_PARAMETER;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesTransferred = ulBytesTransferred;

cleanup:

    return ntStatus;

error:

    *pulBytesTransferred = 0;

    goto cleanup;
}

static
NTSTATUS
NfsGetStatistics(
    PIO_STATISTICS_INFO_INPUT_BUFFER pInBuf,
    PBYTE                            lpOutBuffer,
    ULONG                            ulOutBufferSize,
    PULONG                           pulBytesTransferred
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG    ulBytesTransferred = 0;

    switch (pInBuf->ulInfoLevel)
    {
        case 0:

            ntStatus = NfsGetStatistics_level_0(
                            lpOutBuffer,
                            ulOutBufferSize,
                            &ulBytesTransferred);

            break;

        default:

            ntStatus = STATUS_NOT_SUPPORTED;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesTransferred = ulBytesTransferred;

cleanup:

    return ntStatus;

error:

    *pulBytesTransferred = 0;

    goto cleanup;
}

static
NTSTATUS
NfsGetStatistics_level_0(
    PBYTE  lpOutBuffer,
    ULONG  ulOutBufferSize,
    PULONG pulBytesTransferred
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NFS_ELEMENTS_STATISTICS stats = {0};
    IO_STATISTICS_INFO_0 statBuf = {0};

    if (ulOutBufferSize < sizeof(statBuf))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = NfsElementsGetStats(&stats);
    BAIL_ON_NT_STATUS(ntStatus);

    statBuf.llNumConnections = stats.llNumConnections;
    statBuf.llMaxNumConnections = stats.llMaxNumConnections;

    statBuf.llNumSessions = stats.llNumSessions;
    statBuf.llMaxNumSessions = stats.llMaxNumSessions;

    statBuf.llNumTreeConnects = stats.llNumTreeConnects;
    statBuf.llMaxNumTreeConnects = stats.llMaxNumTreeConnects;

    statBuf.llNumOpenFiles = stats.llNumOpenFiles;
    statBuf.llMaxNumOpenFiles = stats.llMaxNumOpenFiles;

    memcpy(lpOutBuffer, (PBYTE)&statBuf, sizeof(statBuf));

    *pulBytesTransferred = sizeof(statBuf);

cleanup:

    return ntStatus;

error:

    *pulBytesTransferred = 0;

    goto cleanup;
}
