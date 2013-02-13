/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 *        path.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) RTL Path Routines
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *          Brian Koropoff (bkoropoff@likewise.com)
 */

#include "includes.h"
#include "lwio/iortl.h"
#include <lw/rtlgoto.h>

BOOLEAN
IoRtlPathIsSeparator(
    IN WCHAR Character
    )
{
    return (('/' == Character) || ('\\' == Character)) ? TRUE : FALSE;
}

VOID
IoRtlPathDissect(
    IN PUNICODE_STRING Path,
    OUT OPTIONAL PUNICODE_STRING FirstComponent,
    OUT OPTIONAL PUNICODE_STRING RemainingPath
    )
{
    ULONG i = 0;
    ULONG count = RTL_STRING_NUM_CHARS(Path);
    UNICODE_STRING firstComponent = { 0 };
    UNICODE_STRING remainingPath = { 0 };

    // Skip any initial separators
    while ((i < count) && IoRtlPathIsSeparator(Path->Buffer[i]))
    {
        i++;
    }

    // Stop if no characters remaining
    if ((count - i) < 1)
    {
        GOTO_CLEANUP();
    }

    firstComponent.Buffer = &Path->Buffer[i];

    // Find next separator, if any
    while ((i < count) && !IoRtlPathIsSeparator(Path->Buffer[i]))
    {
        i++;
    }

    remainingPath.Buffer = &Path->Buffer[i];

    firstComponent.Length = LwRtlPointerToOffset(firstComponent.Buffer, remainingPath.Buffer);
    firstComponent.MaximumLength = firstComponent.Length;

    remainingPath.Length = LwRtlPointerToOffset(remainingPath.Buffer, &Path->Buffer[count]);
    remainingPath.MaximumLength = remainingPath.Length + (Path->MaximumLength - Path->Length);

cleanup:
    if (FirstComponent)
    {
        *FirstComponent = firstComponent;
    }
    if (RemainingPath)
    {
        *RemainingPath = remainingPath;
    }
}

VOID
IoRtlPathSkipSeparators(
    IN PUNICODE_STRING Path,
    OUT PUNICODE_STRING NewPath
    )
{
    ULONG i = 0;
    ULONG count = RTL_STRING_NUM_CHARS(Path);
    USHORT skipLength = 0;

    // Skip any initial separators
    while ((i < count) && IoRtlPathIsSeparator(Path->Buffer[i]))
    {
        i++;
    }

    skipLength = LwRtlPointerToOffset(Path->Buffer, &Path->Buffer[i]);

    NewPath->Buffer = &Path->Buffer[i];
    NewPath->Length = Path->Length - skipLength;
    NewPath->MaximumLength = Path->MaximumLength - skipLength;
}

NTSTATUS
IoRtlPathUncToInternal(
    PCWSTR pwszUncPath,
    PWSTR* ppwszInternalPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszCopy = NULL;
    PWSTR pwszIn = NULL;
    PWSTR pwszOut = NULL;
    CHAR szCwd[PATH_MAX];

    status = LwRtlWC16StringDuplicate(&pwszCopy, pwszUncPath);
    BAIL_ON_NT_STATUS(status);

    for(pwszIn = pwszOut = pwszCopy; *pwszIn; pwszIn++)
    {
        if (IoRtlPathIsSeparator(*pwszIn))
        {
            *(pwszOut++) = '/';
            while (IoRtlPathIsSeparator(pwszIn[1]))
            {
                pwszIn++;
            }
        }
        else
        {
            *(pwszOut++) = *pwszIn;
        }
    }

    *pwszOut = '\0';

    if (IoRtlPathIsSeparator(pwszUncPath[0]) && IoRtlPathIsSeparator(pwszUncPath[1]))
    {
        status = LwRtlWC16StringAllocatePrintfW(ppwszInternalPath, L"/rdr%ws", pwszCopy);
        BAIL_ON_NT_STATUS(status);
    }
    else if (IoRtlPathIsSeparator(pwszUncPath[0]))
    {
        status = LwRtlWC16StringAllocatePrintfW(ppwszInternalPath, L"/pvfs%ws", pwszCopy);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        if (getcwd(szCwd, sizeof(szCwd)) == NULL)
        {
            status = STATUS_UNSUCCESSFUL;
            BAIL_ON_NT_STATUS(status);
        }

        status = LwRtlWC16StringAllocatePrintfW(ppwszInternalPath, L"/pvfs%s/%ws", szCwd, pwszCopy);
        BAIL_ON_NT_STATUS(status);
    }

cleanup:

    RTL_FREE(&pwszCopy);

    return status;

error:

    *ppwszInternalPath = NULL;

    goto cleanup;
}
