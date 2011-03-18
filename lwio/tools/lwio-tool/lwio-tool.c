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
 *        lwio-tool.c
 *
 * Abstract:
 *
 *        LW IO Tool
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "lwio-tool.h"

static
NTSTATUS
DoTestFileApiCreateFile(
    IN PCSTR pszPath
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_FILE_HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IO_FILE_NAME fileName = { 0 };
    PVOID pSecurityDescriptor = NULL;
    PVOID pSecurityQualityOfService = NULL;
    ACCESS_MASK desiredAccess = FILE_GENERIC_READ;
    LONG64 allocationSize = 0;
    FILE_ATTRIBUTES fileAttributes = 0;
    FILE_SHARE_FLAGS shareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    FILE_CREATE_DISPOSITION createDisposition = FILE_OPEN;
    FILE_CREATE_OPTIONS createOptions = 0;
    PFILE_FULL_EA_INFORMATION pEaBuffer = NULL;
    ULONG EaLength = 0;
    PIO_ECP_LIST pEcpList = NULL;

    if (IsNullOrEmptyString(pszPath))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = RtlUnicodeStringAllocateFromCString(&fileName.Name, pszPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCreateFile(
                    &fileHandle,
                    NULL,
                    &ioStatusBlock,
                    &fileName,
                    pSecurityDescriptor,
                    pSecurityQualityOfService,
                    desiredAccess,
                    allocationSize,
                    fileAttributes,
                    shareAccess,
                    createDisposition,
                    createOptions,
                    pEaBuffer,
                    EaLength,
                    pEcpList,
                    NULL);
    LWIO_ASSERT(IS_BOTH_OR_NEITHER(NT_SUCCESS(status), fileHandle));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LWIO_LOG_ALWAYS("Opened file '%s'", pszPath);

cleanup:
    RTL_UNICODE_STRING_FREE(&fileName.Name);

    if (fileHandle)
    {
        NtCloseFile(fileHandle);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

static
NTSTATUS
DoTestFileApiCreateNamedPipeFile(
    IN PCSTR pszPath
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_FILE_HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IO_FILE_NAME fileName = { 0 };
    PVOID pSecurityDescriptor = NULL;
    PVOID pSecurityQualityOfService = NULL;
    ACCESS_MASK desiredAccess = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
    FILE_SHARE_FLAGS shareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
    FILE_CREATE_DISPOSITION createDisposition = FILE_OPEN_IF;
    FILE_CREATE_OPTIONS createOptions = 0;
    FILE_PIPE_TYPE_MASK namedPipeType = FILE_PIPE_BYTE_STREAM_TYPE | FILE_PIPE_ACCEPT_REMOTE_CLIENTS;
    FILE_PIPE_READ_MODE_MASK readMode = FILE_PIPE_BYTE_STREAM_MODE;
    FILE_PIPE_COMPLETION_MODE_MASK completionMode = FILE_PIPE_COMPLETE_OPERATION;
    ULONG maximumInstances = (ULONG) -1;
    ULONG inboundQuota = 0;
    ULONG outboundQuota = 0;
    PLONG64 defaultTimeout = NULL;

    if (IsNullOrEmptyString(pszPath))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = RtlUnicodeStringAllocateFromCString(&fileName.Name, pszPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCreateNamedPipeFile(
                    &fileHandle,
                    NULL,
                    &ioStatusBlock,
                    &fileName,
                    pSecurityDescriptor,
                    pSecurityQualityOfService,
                    desiredAccess,
                    shareAccess,
                    createDisposition,
                    createOptions,
                    namedPipeType,
                    readMode,
                    completionMode,
                    maximumInstances,
                    inboundQuota,
                    outboundQuota,
                    defaultTimeout);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LWIO_LOG_ALWAYS("Opened named pipe '%s'", pszPath);

cleanup:
    RTL_UNICODE_STRING_FREE(&fileName.Name);

    if (fileHandle)
    {
        NtCloseFile(fileHandle);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

static
NTSTATUS
DoTestFileApi(
    IN OUT PLW_PARSE_ARGS pParseArgs,
    OUT PSTR* ppszUsageError
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PSTR pszUsageError = NULL;
    PCSTR pszCommand = NULL;
    PCSTR pszPath = NULL;

    pszCommand = LwParseArgsNext(pParseArgs);
    if (!pszCommand)
    {
        status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Missing command.\n");
        assert(!status && pszUsageError);
        GOTO_CLEANUP_EE(EE);
    }

    if (!strcmp(pszCommand, "create"))
    {
        pszPath = LwParseArgsNext(pParseArgs);
        if (!pszPath)
        {
            status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Missing path argument.\n");
            assert(!status && pszUsageError);
            GOTO_CLEANUP_EE(EE);
        }

        if (LwParseArgsGetRemaining(pParseArgs) > 1)
        {
            status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Too many arguments.\n");
            assert(!status && pszUsageError);
            GOTO_CLEANUP_EE(EE);
        }

        status = DoTestFileApiCreateFile(pszPath);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    else if (!strcmp(pszCommand, "createnp"))
    {
        pszPath = LwParseArgsNext(pParseArgs);
        if (!pszPath)
        {
            status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Missing path argument.\n");
            assert(!status && pszUsageError);
            GOTO_CLEANUP_EE(EE);
        }

        if (LwParseArgsGetRemaining(pParseArgs) > 1)
        {
            status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Too many arguments.\n");
            assert(!status && pszUsageError);
            GOTO_CLEANUP_EE(EE);
        }

        status = DoTestFileApiCreateNamedPipeFile(pszPath);
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

static
VOID
Usage(
    IN PCSTR pszProgramName
    )
{
    printf("Usage: %s <command> [command-args]\n"
           "\n"
           "  commands:\n"
           "\n"
           "    testfileapi create <path>\n"
           "    testfileapi createnp <path>\n"
           "\n",
           pszProgramName);
    // TODO--We really want something like:
    //
    // <TOOL> testfileapi createfile <path> [options]
    // <TOOL> load <drivername>
    // <TOOL> unload <drivername>
    // etc..
}

int
main(
    IN int argc,
    IN PCSTR argv[]
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PSTR pszUsageError = NULL;
    LW_PARSE_ARGS args = { 0 };
    PCSTR pszProgramName = NULL;
    PCSTR pszCommand = NULL;

    LwParseArgsInit(&args, argc, argv);
    pszProgramName = LwGetProgramName(LwParseArgsGetAt(&args, 0));

    pszCommand = LwParseArgsNext(&args);
    if (!pszCommand)
    {
        status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Missing command.\n");
        assert(!status && pszUsageError);
        GOTO_CLEANUP_EE(EE);
    }

    if (!strcmp(pszCommand, "testfileapi"))
    {
        status = DoTestFileApi(&args, &pszUsageError);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    else if (!strcmp(pszCommand, "iotest"))
    {
        status = IoTestMain(&args, &pszUsageError);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    else if (!strcmp(pszCommand, "srvtest"))
    {
        status = SrvTestMain(&args, &pszUsageError);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    else
    {
        status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Invalid command '%s'\n", pszCommand);
        assert(!status && pszUsageError);
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (pszUsageError)
    {
        printf("%s", pszUsageError);
        RtlCStringFree(&pszUsageError);
        Usage(pszProgramName);
        status = STATUS_INVALID_PARAMETER;
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status ? 1 : 0;
}
