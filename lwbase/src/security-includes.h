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
 *        security-includes.h
 *
 * Abstract:
 *
 *        Internal includes for security modules.
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <lw/security-api.h>
#include "security-api2.h"
#include "security-types-internal.h"
#include "security-sddl-internal.h"
#include <lw/rtlgoto.h>
#include <lw/rtlmemory.h>
#include <lw/swab.h>
#include <lw/safeint.h>
#include <assert.h>

inline
static
BOOLEAN
RtlpIsBufferAvailable(
    IN ULONG MaximumSize,
    IN ULONG Offset,
    IN ULONG Size
    )
{
    BOOLEAN isAvailable = TRUE;

    // Check for overflow.
    if ((Offset + Size) < Offset)
    {
        isAvailable = FALSE;
        GOTO_CLEANUP();
    }

    if ((Offset + Size) > MaximumSize)
    {
        isAvailable = FALSE;
        GOTO_CLEANUP();
    }

    isAvailable = TRUE;

cleanup:
    return isAvailable;
}

inline
static
PSID
RtlpGetSidAccessAllowedAce(
    IN PACCESS_ALLOWED_ACE Ace
    )
{
    return (PSID) &Ace->SidStart;
}

BOOLEAN
RtlpIsValidLittleEndianSidBuffer(
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BufferUsed
    );

BOOLEAN
RtlpIsValidLittleEndianAclBuffer(
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BufferUsed
    );

NTSTATUS
RtlpEncodeLittleEndianSid(
    IN PSID Sid,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BufferUsed
    );

NTSTATUS
RtlpEncodeLittleEndianAcl(
    IN PACL Acl,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BufferUsed
    );

VOID
RtlpDecodeLittleEndianSid(
    IN PSID LittleEndianSid,
    OUT PSID Sid
    );

NTSTATUS
RtlpDecodeLittleEndianAcl(
    IN PACL LittleEndianAcl,
    OUT PACL Acl
    );

NTSTATUS
RtlpCreateAbsSecDescFromRelative(
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppAbsSecDesc,
    IN  PSECURITY_DESCRIPTOR_RELATIVE pRelSecDesc
    );

VOID
RtlpFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    );
