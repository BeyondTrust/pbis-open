/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
