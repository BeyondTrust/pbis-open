/*
 * Copyright Likewise Software    2004-2009
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
 *        string.c
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - NFS
 *
 *        Utilities
 *
 *        Strings
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
NfsMbsToWc16s(
    IN  PCSTR  pszString,
    OUT PWSTR* ppwszString
    )
{
    return LwRtlWC16StringAllocateFromCString(ppwszString, pszString);
}

NTSTATUS
NfsWc16sToMbs(
    IN  PCWSTR pwszString,
    OUT PSTR*  ppszString
    )
{
    return LwRtlCStringAllocateFromWC16String(ppszString, pwszString);
}

NTSTATUS
NfsAllocateStringW(
    PWSTR  pwszInputString,
    PWSTR* ppwszOutputString
    )
{
    return LwRtlWC16StringDuplicate(ppwszOutputString, pwszInputString);
}

NTSTATUS
NfsAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    va_list args;

    va_start(args, pszFormat);

    ntStatus = RtlCStringAllocatePrintfV(
                      ppszOutputString,
                      pszFormat,
                      args);

    va_end(args);

    return ntStatus;
}
