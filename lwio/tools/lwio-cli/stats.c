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
 *        main.c
 *
 * Abstract:
 *
 *        LWIO client interface program
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
VOID
PrintServerStats_level_0(
    PIO_STATISTICS_INFO_0 pStats
    );

DWORD
ProcessServerStats(
    BOOLEAN bShowStats,
    BOOLEAN bResetStats
    )
{
    DWORD                   dwError   = 0;
    NTSTATUS                ntStatus  = STATUS_SUCCESS;
    WCHAR                   wszName[] = {'\\','s','r','v',0};
    IO_FILE_NAME            fileName  =
                                {
                                        .RootFileHandle = NULL,
                                        .Name = LW_RTL_CONSTANT_STRING(wszName),
                                        .IoNameOptions  = 0
                                };
    IO_FILE_HANDLE          hDevice       = NULL;
    IO_STATUS_BLOCK         ioStatusBlock = {0};
    PIO_ASYNC_CONTROL_BLOCK pAcb          = NULL;

    ntStatus = NtCreateFile(
                  &hDevice,
                  pAcb,
                  &ioStatusBlock,
                  &fileName,
                  NULL,
                  NULL,
                  0,
                  0,
                  0,
                  0,
                  0,
                  0,
                  NULL,
                  0,
                  NULL,
                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (bResetStats)
    {
        IO_STATISTICS_INFO_INPUT_BUFFER inBuf =
        {
                .ulAction    = IO_STATISTICS_ACTION_TYPE_RESET,
                .ulInfoLevel = 0
        };

        ntStatus = NtDeviceIoControlFile(
                        hDevice,
                        pAcb,
                        &ioStatusBlock,
                        IO_DEVICE_CTL_STATISTICS,
                        &inBuf,
                        sizeof(inBuf),
                        NULL,
                        0);
        BAIL_ON_NT_STATUS(ntStatus);

        printf("Successfully reset the server statistics\n");
    }

    if (bShowStats)
    {
        IO_STATISTICS_INFO_INPUT_BUFFER inBuf =
        {
                .ulAction    = IO_STATISTICS_ACTION_TYPE_GET,
                .ulInfoLevel = 0
        };
        IO_STATISTICS_INFO_0 stats = {0};

        ntStatus = NtDeviceIoControlFile(
                        hDevice,
                        pAcb,
                        &ioStatusBlock,
                        IO_DEVICE_CTL_STATISTICS,
                        &inBuf,
                        sizeof(inBuf),
                        &stats,
                        sizeof(stats));
        BAIL_ON_NT_STATUS(ntStatus);

        switch (inBuf.ulInfoLevel)
        {
            case 0:

                PrintServerStats_level_0(&stats);

                break;

            default:

                LWIO_LOG_WARNING("Unsupported info level [%d]\n", inBuf.ulInfoLevel);

                break;
        }
    }

cleanup:

    if (hDevice)
    {
        NtCloseFile(hDevice);
    }

    return dwError;

error:

    fprintf(stderr,
            "Failed to get server statistics [status %s = 0x%08X (%d); %s]\n",
            NtStatusToName(ntStatus),
            ntStatus,
            ntStatus,
            NtStatusToDescription(ntStatus));

    dwError = LwNtStatusToWin32Error(ntStatus);

    goto cleanup;
}

static
VOID
PrintServerStats_level_0(
    PIO_STATISTICS_INFO_0 pStats
    )
{
    printf("Server statistics [level 0]: \n\n");

    printf("Connections   [Current: %lld] [Maximum: %lld]\n",
           (long long)pStats->llNumConnections,
           (long long)pStats->llMaxNumConnections);

    printf("Sessions      [Current: %lld] [Maximum: %lld]\n",
            (long long)pStats->llNumSessions,
            (long long)pStats->llMaxNumSessions);

    printf("Tree connects [Current: %lld] [Maximum: %lld]\n",
            (long long)pStats->llNumTreeConnects,
            (long long)pStats->llMaxNumTreeConnects);

    printf("Files:        [Current: %lld] [Maximum: %lld]\n",
            (long long)pStats->llNumOpenFiles,
            (long long)pStats->llMaxNumOpenFiles);
}
