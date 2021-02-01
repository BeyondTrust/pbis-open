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
 *        assert.c
 *
 * Abstract:
 *
 *        BeyondTrust IO (LWIO) Assert Module
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "includes.h"

VOID
LwIoAssertionFailed(
    IN PCSTR Expression,
    IN OPTIONAL PCSTR Message,
    IN PCSTR Function,
    IN PCSTR File,
    IN ULONG Line
    )
{
    if (Message)
    {
        LW_RTL_LOG_RAW(
            LWIO_LOG_LEVEL_ERROR,
            "lwio",
            Function,
            File,
            Line,
            "ASSERTION FAILED: Expression = (%s), Message = '%s'",
            Expression,
            Message);
        fprintf(
            stderr,
            "ASSERTION FAILED: Expression = (%s), Message = '%s'",
            Expression,
            Message);
    }
    else
    {
        LW_RTL_LOG_RAW(
            LWIO_LOG_LEVEL_ERROR,
            "lwio",
            Function,
            File,
            Line,
            "ASSERTION FAILED: Expression = (%s)",
            Expression);
        fprintf(
            stderr,
            "ASSERTION FAILED: Expression = (%s)",
            Expression);
    }
    fprintf(stderr, "\n");
    abort();
}

VOID
LwIoAssertionFailedFormat(
    IN PCSTR Expression,
    IN PCSTR Format,
    IN PCSTR Function,
    IN PCSTR File,
    IN ULONG Line,
    ...
    )
{
    NTSTATUS status ATTRIBUTE_UNUSED = STATUS_SUCCESS;
    PSTR message = NULL;
    va_list args;

    va_start(args, Line);
    status = LwRtlCStringAllocatePrintfV(&message, Format, args);
    va_end(args);

    LwIoAssertionFailed(Expression, message ? message : Format, Function, File, Line);

    // unreachable
    RTL_FREE(&message);
}
