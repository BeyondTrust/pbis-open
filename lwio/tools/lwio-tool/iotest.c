/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        iotest.c
 *
 * Abstract:
 *
 *        LW IO Tool IO Test Module
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "lwio-tool.h"
#include <iotestctl.h>

static
NTSTATUS
ItLibTestRundown(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    IO_FILE_HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IO_FILE_NAME fileName = { 0 };

    status = RtlUnicodeStringAllocateFromCString(&fileName.Name, "/iotest");
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = LwNtCreateFile(
                    &fileHandle,
                    NULL,
                    &ioStatusBlock,
                    &fileName,
                    NULL, // SecurityDescriptor
                    NULL, // QOS
                    SYNCHRONIZE,
                    0, // AllocationSize
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN,
                    0, // CreateOptions
                    NULL, // EA
                    0, // EA Length
                    NULL, // ECP List
                    NULL);
    LWIO_ASSERT(IS_BOTH_OR_NEITHER(NT_SUCCESS(status), fileHandle));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = LwNtDeviceIoControlFile(
                    fileHandle,
                     NULL,
                     &ioStatusBlock,
                     IOTEST_IOCTL_TEST_RUNDOWN,
                     NULL,
                     0,
                     NULL,
                     0);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    RTL_UNICODE_STRING_FREE(&fileName.Name);

    if (fileHandle)
    {
        NtCloseFile(fileHandle);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IoTestMain(
    IN OUT PLW_PARSE_ARGS pParseArgs,
    OUT PSTR* ppszUsageError
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PSTR pszUsageError = NULL;
    PCSTR pszCommand = NULL;

    pszCommand = LwParseArgsNext(pParseArgs);
    if (!pszCommand)
    {
        status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Missing command.\n");
        assert(!status && pszUsageError);
        GOTO_CLEANUP_EE(EE);
    }

    if (!strcmp(pszCommand, "rundown"))
    {
        if (LwParseArgsGetRemaining(pParseArgs) > 1)
        {
            status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Too many arguments.\n");
            assert(!status && pszUsageError);
            GOTO_CLEANUP_EE(EE);
        }

        status = ItLibTestRundown();
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    else
    {
        status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Invalid command '%s'\n", pszCommand);
        assert(!status);
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (pszUsageError)
    {
        status = STATUS_INVALID_PARAMETER;
    }

    *ppszUsageError = pszUsageError;

    LOG_LEAVE_IF_STATUS_EE(status, EE);

    return status;
}
