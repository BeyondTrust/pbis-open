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
