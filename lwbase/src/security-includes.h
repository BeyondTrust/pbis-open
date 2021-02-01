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

inline
static
PSID
RtlpGetSidAccessAllowedObjectAce(
    IN PACCESS_ALLOWED_OBJECT_ACE Ace
    )
{
    PSID psid = NULL;
    
    switch (Ace->Flags) {
        case 0:
            psid = (PSID) &Ace->ObjectType;
            break;
        case ACE_OBJECT_TYPE_PRESENT:
        case ACE_INHERITED_OBJECT_TYPE_PRESENT:
            psid = (PSID) &Ace->InheritedObjectType;
            break;
        default:
            psid = (PSID) &Ace->SidStart;
            break;
    }
    
    return psid;
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
