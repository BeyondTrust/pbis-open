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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        assert.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) Assert Module
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
            "ASSERION FAILED: Expression = (%s), Message = '%s'",
            Expression,
            Message);
        fprintf(
            stderr,
            "ASSERION FAILED: Expression = (%s), Message = '%s'",
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
            "ASSERION FAILED: Expression = (%s)",
            Expression);
        fprintf(
            stderr,
            "ASSERION FAILED: Expression = (%s)",
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
