/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 * Module Name:
 *
 *        security-privileges.c
 *
 * Abstract:
 *
 *        Privilege support functions in Security Module.
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "security-includes.h"

//
// LUID Functions
//

LUID
RtlConvertLongToLuid(
    IN LONG Long
    )
{
    LUID Luid = {0};

    Luid.LowPart = (ULONG)Long;
    return Luid;
}


LUID
RtlConvertUlongToLuid(
    IN ULONG Ulong
    )
{
    LUID Luid = {0};

    Luid.LowPart = Ulong;
    return Luid;
}


BOOLEAN
RtlEqualLuid(
    IN PLUID Luid1,
    IN PLUID Luid2
    )
{
    return ((Luid1->HighPart == Luid2->HighPart) &&
            (Luid1->LowPart == Luid2->LowPart));
}


//
// PRIVILEGE_SET Functions
//

ULONG
RtlLengthRequiredPrivilegeSet(
    IN ULONG PrivilegeCount
    )
{
    return _PRIVILEGE_SET_GET_SIZE_REQUIRED(PrivilegeCount);
}


ULONG
RtlLengthPrivilegeSet(
    IN PPRIVILEGE_SET PrivilegeSet
    )
{
    return RtlLengthRequiredPrivilegeSet(PrivilegeSet->PrivilegeCount);
}


NTSTATUS
RtlCopyPrivilegeSet(
    IN ULONG DestinationPrivilegeSetLength,
    OUT PPRIVILEGE_SET DestinationPrivilegeSet,
    IN PPRIVILEGE_SET SourcePrivilegeSet
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG length = RtlLengthPrivilegeSet(SourcePrivilegeSet);

    if (DestinationPrivilegeSetLength < length)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    RtlCopyMemory(DestinationPrivilegeSet, SourcePrivilegeSet, length);

cleanup:
    return status;
}
