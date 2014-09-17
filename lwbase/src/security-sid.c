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
 *        security-sid.c
 *
 * Abstract:
 *
 *        SID Functions in Security Module.
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#include "security-includes.h"
// TODO-Move s*printf replacement functions to rtlstring.h
// For snprintf
#include <stdio.h>

//
// SID Functions
//

NTSTATUS
RtlInitializeSid(
    OUT PSID Sid,
    IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    IN UCHAR SubAuthorityCount
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!Sid || !IdentifierAuthority)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (SubAuthorityCount > SID_MAX_SUB_AUTHORITIES)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    Sid->Revision = SID_REVISION;
    Sid->SubAuthorityCount = SubAuthorityCount;
    memcpy(&Sid->IdentifierAuthority, IdentifierAuthority, sizeof(*IdentifierAuthority));
    memset(Sid->SubAuthority, 0, sizeof(Sid->SubAuthority[0]) * SubAuthorityCount);

cleanup:
    return status;
}

ULONG
RtlLengthRequiredSid(
    IN ULONG SubAuthorityCount
    )
{
    return _SID_GET_SIZE_REQUIRED(SubAuthorityCount);
}

ULONG
RtlLengthSid(
    IN PSID Sid
    )
{
    return RtlLengthRequiredSid(Sid->SubAuthorityCount);
}

BOOLEAN
RtlValidSid(
    IN PSID Sid
    )
{
    return ((Sid != NULL) &&
            (Sid->Revision == SID_REVISION) &&
            (Sid->SubAuthorityCount <= SID_MAX_SUB_AUTHORITIES));
}

BOOLEAN
RtlEqualSid(
    IN PSID Sid1,
    IN PSID Sid2
    )
{
    return ((Sid1->SubAuthorityCount == Sid2->SubAuthorityCount) &&
            RtlEqualMemory(Sid1, Sid2, RtlLengthSid(Sid1)));
}

BOOLEAN
RtlEqualPrefixSid(
    IN PSID Sid1,
    IN PSID Sid2
    )
{
    BOOLEAN isEqual = FALSE;
    if (Sid1->SubAuthorityCount == Sid2->SubAuthorityCount)
    {
        UCHAR count = Sid1->SubAuthorityCount;
        if (count > 0)
        {
            count--;
        }
        isEqual = RtlEqualMemory(Sid1, Sid2, RtlLengthRequiredSid(count));
    }
    return isEqual;
}

BOOLEAN
RtlIsPrefixSid(
    IN PSID Prefix,
    IN PSID Sid
    )
{
    /*
     * memory-compare only the part starting from IdentifierAuthority
     * (basically, skip SubAuthorityCount which is different in both SIDs)
     */
    ULONG ulAuthSidLength = RtlLengthSid(Prefix) -
                            (sizeof(Prefix->Revision) +
                             sizeof(Prefix->SubAuthorityCount));

    return ((Prefix->SubAuthorityCount <= Sid->SubAuthorityCount) &&
            (Prefix->Revision == Sid->Revision) &&
            RtlEqualMemory(&Prefix->IdentifierAuthority,
                           &Sid->IdentifierAuthority,
                           ulAuthSidLength));
}

NTSTATUS
RtlCopySid(
    IN ULONG DestinationSidLength,
    OUT PSID DestinationSid,
    IN PSID SourceSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG length = RtlLengthSid(SourceSid);

    if (DestinationSidLength < length)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    RtlCopyMemory(DestinationSid, SourceSid, length);

cleanup:
    return status;
}

NTSTATUS
RtlAppendRidSid(
    IN ULONG SidLength,
    IN OUT PSID Sid,
    IN ULONG Rid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG length = 0;

    if (Sid->SubAuthorityCount >= SID_MAX_SUB_AUTHORITIES)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    length = RtlLengthRequiredSid(Sid->SubAuthorityCount + 1);
    if (SidLength < length)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    Sid->SubAuthority[Sid->SubAuthorityCount] = Rid;
    Sid->SubAuthorityCount++;

cleanup:
    return status;
}

NTSTATUS
RtlGetRidSid(
    OUT PULONG Rid,
    IN PSID Sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!Rid || !Sid)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }
    
    if (Sid->SubAuthorityCount == 0)
    {
        status = STATUS_INVALID_SUB_AUTHORITY;
        GOTO_CLEANUP_ON_STATUS(status);
    }
    
    *Rid = Sid->SubAuthority[Sid->SubAuthorityCount-1];

cleanup:

    return status;
}



NTSTATUS
RtlDuplicateSid(
    OUT PSID* NewSid,
    IN PSID OriginalSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG length = RtlLengthSid(OriginalSid);
    PSID resultSid = NULL;

    status = RTL_ALLOCATE(&resultSid, SID, length);
    GOTO_CLEANUP_ON_STATUS(status);

    RtlCopyMemory(resultSid, OriginalSid, length);

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&resultSid);
    }

    *NewSid = resultSid;

    return status;
}

BOOLEAN
RtlpIsValidLittleEndianSidBuffer(
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BufferUsed
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID littleEndianSid = (PSID) Buffer;
    ULONG size = 0;

    if (BufferSize < SID_MIN_SIZE)
    {
        status = STATUS_INVALID_SID;
        GOTO_CLEANUP();
    }

    size = RtlLengthRequiredSid(LW_LTOH8(littleEndianSid->SubAuthorityCount));
    if (!RtlpIsBufferAvailable(BufferSize, 0, size))
    {
        status = STATUS_INVALID_SID;
        GOTO_CLEANUP();
    }

    // This is ok since it only looks at 1-byte fields:
    status = RtlValidSid(littleEndianSid) ? STATUS_SUCCESS : STATUS_INVALID_SID;

cleanup:
    *BufferUsed = NT_SUCCESS(status) ? size : 0;

    return NT_SUCCESS(status);
}

NTSTATUS
RtlpEncodeLittleEndianSid(
    IN PSID Sid,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BufferUsed
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID littleEndianSid = (PSID) Buffer;
    ULONG size = RtlLengthSid(Sid);
    ULONG i = 0;

    if (BufferSize < size)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    littleEndianSid->Revision = LW_HTOL8(Sid->Revision);
    littleEndianSid->SubAuthorityCount = LW_HTOL8(Sid->SubAuthorityCount);
    // sequence of bytes
    littleEndianSid->IdentifierAuthority = Sid->IdentifierAuthority;

    for (i = 0; i < Sid->SubAuthorityCount; i++)
    {
        littleEndianSid->SubAuthority[i] = LW_HTOL32(Sid->SubAuthority[i]);
    }

    status = STATUS_SUCCESS;

cleanup:
    *BufferUsed = NT_SUCCESS(status) ? size : 0;

    return status;
}

VOID
RtlpDecodeLittleEndianSid(
    IN PSID LittleEndianSid,
    OUT PSID Sid
    )
{
    ULONG i = 0;

    Sid->Revision = LW_LTOH8(LittleEndianSid->Revision);
    Sid->SubAuthorityCount = LW_LTOH8(LittleEndianSid->SubAuthorityCount);
    // sequence of bytes
    Sid->IdentifierAuthority = LittleEndianSid->IdentifierAuthority;

    for (i = 0; i < Sid->SubAuthorityCount; i++)
    {
        Sid->SubAuthority[i] = LW_LTOH32(LittleEndianSid->SubAuthority[i]);
    }
}

//
// SID <-> String Conversion Functions
//

//
// A string SID is represented as:
//
//   "S-" REV "-" AUTH ("-" SUB_AUTH ) * SubAuthorityCount
//
// where:
//
//   - REV is a decimal UCHAR (max 3 characters).
//
//   - AUTH is either a decimal ULONG (max 10 characters) or
//     "0x" followed by a 6-byte hex value (max 2 + 12 = 14 characters).
//
//   - SUB_AUTH is a decimal ULONG (max 10 characters).
//

#define RTLP_STRING_SID_MAX_CHARS(SubAuthorityCount) \
    (2 + 3 + 1 + 14 + (1 + 10) * (SubAuthorityCount) + 1)

NTSTATUS
RtlAllocateUnicodeStringFromSid(
    OUT PUNICODE_STRING StringSid,
    IN PSID Sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR resultBuffer = NULL;
    UNICODE_STRING result = { 0 };

    if (!StringSid)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    status = RtlAllocateWC16StringFromSid(&resultBuffer, Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlUnicodeStringInitEx(&result, resultBuffer);
    GOTO_CLEANUP_ON_STATUS(status);
    resultBuffer = NULL;

    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        RtlUnicodeStringFree(&result);
    }
    RTL_FREE(&resultBuffer);

    if (StringSid)
    {
        *StringSid = result;
    }

    return status;
}

NTSTATUS
RtlAllocateAnsiStringFromSid(
    OUT PANSI_STRING StringSid,
    IN PSID Sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR resultBuffer = NULL;
    ANSI_STRING result = { 0 };

    if (!StringSid)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    status = RtlAllocateCStringFromSid(&resultBuffer, Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlAnsiStringInitEx(&result, resultBuffer);
    GOTO_CLEANUP_ON_STATUS(status);
    resultBuffer = NULL;

    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        RtlAnsiStringFree(&result);
    }
    RTL_FREE(&resultBuffer);

    if (StringSid)
    {
        *StringSid = result;
    }

    return status;
}

NTSTATUS
RtlAllocateWC16StringFromSid(
    OUT PWSTR* StringSid,
    IN PSID Sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR result = NULL;
    PSTR convertString = NULL;

    if (!StringSid)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    status = RtlAllocateCStringFromSid(&convertString, Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlWC16StringAllocateFromCString(&result, convertString);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&result);
    }
    RTL_FREE(&convertString);

    if (StringSid)
    {
        *StringSid = result;
    }

    return status;
    
}

NTSTATUS
RtlAllocateCStringFromSid(
    OUT PSTR* StringSid,
    IN PSID Sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR result = NULL;
    size_t size = 0;
    int count = 0;
    ULONG i = 0;

    if (!StringSid || !RtlValidSid(Sid))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    size = RTLP_STRING_SID_MAX_CHARS(Sid->SubAuthorityCount);

    status = RTL_ALLOCATE(&result, CHAR, size);
    GOTO_CLEANUP_ON_STATUS(status);

    if (Sid->IdentifierAuthority.Value[0] || Sid->IdentifierAuthority.Value[1])
    {
        count += snprintf(result + count,
                          size - count,
                          "S-%u-0x%.2X%.2X%.2X%.2X%.2X%.2X",
                          Sid->Revision,
                          Sid->IdentifierAuthority.Value[0],
                          Sid->IdentifierAuthority.Value[1],
                          Sid->IdentifierAuthority.Value[2],
                          Sid->IdentifierAuthority.Value[3],
                          Sid->IdentifierAuthority.Value[4],
                          Sid->IdentifierAuthority.Value[5]);
    }
    else
    {
        ULONG value = 0;

        value |= (ULONG) Sid->IdentifierAuthority.Value[5];
        value |= (ULONG) Sid->IdentifierAuthority.Value[4] << 8;
        value |= (ULONG) Sid->IdentifierAuthority.Value[3] << 16;
        value |= (ULONG) Sid->IdentifierAuthority.Value[2] << 24;

        count += snprintf(result + count,
                          size - count,
                          "S-%u-%u",
                          Sid->Revision,
                          value);
    }

    for (i = 0; i < Sid->SubAuthorityCount; i++)
    {
        count += snprintf(result + count,
                          size - count,
                          "-%u", 
                          Sid->SubAuthority[i]);
    }

    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&result);
    }

    if (StringSid)
    {
        *StringSid = result;
    }

    return status;
}

static
NTSTATUS
RtlpConvertUnicodeStringSidToSidEx(
    IN PUNICODE_STRING StringSid,
    OUT OPTIONAL PSID* AllocateSid,
    OUT OPTIONAL PSID SidBuffer,
    IN OUT OPTIONAL PULONG SidBufferSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR sidBuffer[SID_MAX_SIZE] = { 0 };
    PSID sid = (PSID) sidBuffer;
    BOOLEAN haveRevision = FALSE;
    BOOLEAN haveAuthority = FALSE;
    UNICODE_STRING remaining = { 0 };
    PSID newSid = NULL;
    ULONG sizeRequired = 0;

    if (!StringSid ||
        (AllocateSid && (SidBuffer || SidBufferSize)) ||
        !(AllocateSid || SidBufferSize))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    // Must have at least 2 characters and they must be "S-"
    if (!((StringSid->Length > (2 * sizeof(StringSid->Buffer[0]))) &&
          ((StringSid->Buffer[0] == 'S') || (StringSid->Buffer[0] == 's')) &&
          (StringSid->Buffer[1] == '-')))
    {
        status = STATUS_INVALID_SID;
        GOTO_CLEANUP();
    }

    // Skip the "S-" prefix
    remaining.Buffer = &StringSid->Buffer[2];
    remaining.Length = StringSid->Length - (2 * sizeof(StringSid->Buffer[0]));
    remaining.MaximumLength = remaining.Length;

    // TODO-Handle S-1-0xHEX-... for more than 4 bytes in IdentifierAuth.
    for (;;)
    {
        ULONG value = 0;

        status = LwRtlUnicodeStringParseULONG(&value, &remaining, &remaining);
        if (!NT_SUCCESS(status))
        {
            break;
        }

        if (remaining.Length)
        {
            if (remaining.Buffer[0] != '-')
            {
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
            }
            remaining.Buffer++;
            remaining.Length -= sizeof(remaining.Buffer[0]);
            remaining.MaximumLength = remaining.Length;
        }

        if (!haveRevision)
        {
            if (value > MAXUCHAR)
            {
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
            }
            sid->Revision = (UCHAR) value;
            haveRevision = TRUE;
        }
        else if (!haveAuthority)
        {
            // Authority is represented as a 32-bit number.
            sid->IdentifierAuthority.Value[5] = (value & 0x000000FF);
            sid->IdentifierAuthority.Value[4] = (value & 0x0000FF00) >> 8;
            sid->IdentifierAuthority.Value[3] = (value & 0x00FF0000) >> 16;
            sid->IdentifierAuthority.Value[2] = (value & 0xFF000000) >> 24;
            haveAuthority = TRUE;
        }
        else
        {
            if (sid->SubAuthorityCount >= SID_MAX_SUB_AUTHORITIES)
            {
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
            }
            sid->SubAuthority[sid->SubAuthorityCount] = value;
            sid->SubAuthorityCount++;
        }
    }

    if (!haveAuthority || remaining.Length || !RtlValidSid(sid))
    {
        status = STATUS_INVALID_SID;
        GOTO_CLEANUP();
    }

    sizeRequired = RtlLengthSid(sid);

    if (AllocateSid)
    {
        status = RTL_ALLOCATE(&newSid, SID, sizeRequired);
        GOTO_CLEANUP_ON_STATUS(status);

        RtlCopyMemory(newSid, sid, sizeRequired);
    }
    else if (SidBufferSize)
    {
        if (*SidBufferSize < sizeRequired)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            GOTO_CLEANUP();
        }
        if (SidBuffer)
        {
            RtlCopyMemory(SidBuffer, sid, sizeRequired);
        }
    }
    else
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&newSid);
        sid = NULL;
    }

    if (AllocateSid)
    {
        *AllocateSid = newSid;
    }

    if (SidBufferSize)
    {
        *SidBufferSize = sizeRequired;
    }

    return status;
}

static
NTSTATUS
RtlpConvertAnsiStringSidToSidEx(
    IN PANSI_STRING StringSid,
    OUT OPTIONAL PSID* AllocateSid,
    OUT OPTIONAL PSID SidBuffer,
    IN OUT OPTIONAL PULONG SidBufferSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR sidBuffer[SID_MAX_SIZE] = { 0 };
    PSID sid = (PSID) sidBuffer;
    BOOLEAN haveRevision = FALSE;
    BOOLEAN haveAuthority = FALSE;
    ANSI_STRING remaining = { 0 };
    PSID newSid = NULL;
    ULONG sizeRequired = 0;

    if (!StringSid ||
        (AllocateSid && (SidBuffer || SidBufferSize)) ||
        !(AllocateSid || SidBufferSize))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    // Must have at least 2 characters and they must be "S-"
    if (!((StringSid->Length > (2 * sizeof(StringSid->Buffer[0]))) &&
          ((StringSid->Buffer[0] == 'S') || (StringSid->Buffer[0] == 's')) &&
          (StringSid->Buffer[1] == '-')))
    {
        status = STATUS_INVALID_SID;
        GOTO_CLEANUP();
    }

    // Skip the "S-" prefix
    remaining.Buffer = &StringSid->Buffer[2];
    remaining.Length = StringSid->Length - (2 * sizeof(StringSid->Buffer[0]));
    remaining.MaximumLength = remaining.Length;

    // TODO-Handle S-1-0xHEX-... for more than 4 bytes in IdentifierAuth.
    for (;;)
    {
        ULONG value = 0;

        status = LwRtlAnsiStringParseULONG(&value, &remaining, &remaining);
        if (!NT_SUCCESS(status))
        {
            break;
        }

        if (remaining.Length)
        {
            if (remaining.Buffer[0] != '-')
            {
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
            }
            remaining.Buffer++;
            remaining.Length -= sizeof(remaining.Buffer[0]);
            remaining.MaximumLength = remaining.Length;
        }

        if (!haveRevision)
        {
            if (value > MAXUCHAR)
            {
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
            }
            sid->Revision = (UCHAR) value;
            haveRevision = TRUE;
        }
        else if (!haveAuthority)
        {
            // Authority is represented as a 32-bit number.
            sid->IdentifierAuthority.Value[5] = (value & 0x000000FF);
            sid->IdentifierAuthority.Value[4] = (value & 0x0000FF00) >> 8;
            sid->IdentifierAuthority.Value[3] = (value & 0x00FF0000) >> 16;
            sid->IdentifierAuthority.Value[2] = (value & 0xFF000000) >> 24;
            haveAuthority = TRUE;
        }
        else
        {
            if (sid->SubAuthorityCount >= SID_MAX_SUB_AUTHORITIES)
            {
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
            }
            sid->SubAuthority[sid->SubAuthorityCount] = value;
            sid->SubAuthorityCount++;
        }
    }

    if (!haveAuthority || remaining.Length || !RtlValidSid(sid))
    {
        status = STATUS_INVALID_SID;
        GOTO_CLEANUP();
    }

    sizeRequired = RtlLengthSid(sid);

    if (AllocateSid)
    {
        status = RTL_ALLOCATE(&newSid, SID, sizeRequired);
        GOTO_CLEANUP_ON_STATUS(status);

        RtlCopyMemory(newSid, sid, sizeRequired);
    }
    else if (SidBufferSize)
    {
        if (*SidBufferSize < sizeRequired)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            GOTO_CLEANUP();
        }
        if (SidBuffer)
        {
            RtlCopyMemory(SidBuffer, sid, sizeRequired);
        }
    }
    else
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&newSid);
        sid = NULL;
    }

    if (AllocateSid)
    {
        *AllocateSid = newSid;
    }

    if (SidBufferSize)
    {
        *SidBufferSize = sizeRequired;
    }

    return status;
}

NTSTATUS
RtlAllocateSidFromUnicodeString(
    OUT PSID* Sid,
    IN PUNICODE_STRING StringSid
    )
{
    return RtlpConvertUnicodeStringSidToSidEx(StringSid, Sid, NULL, NULL);
}

NTSTATUS
RtlAllocateSidFromAnsiString(
    OUT PSID* Sid,
    IN PANSI_STRING StringSid
    )
{
    return RtlpConvertAnsiStringSidToSidEx(StringSid, Sid, NULL, NULL);
}

NTSTATUS
RtlAllocateSidFromCString(
    OUT PSID* Sid,
    IN PCSTR StringSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;
    ANSI_STRING stringSid = { 0 };

    status = RtlAnsiStringInitEx(&stringSid, StringSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpConvertAnsiStringSidToSidEx(&stringSid, &sid, NULL, NULL);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&sid);
    }

    *Sid = sid;

    return status;
}

NTSTATUS
RtlAllocateSidFromWC16String(
    OUT PSID* Sid,
    IN PCWSTR StringSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;
    UNICODE_STRING stringSid = { 0 };

    status = RtlUnicodeStringInitEx(&stringSid, StringSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlAllocateSidFromUnicodeString(&sid, &stringSid);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&sid);
    }

    *Sid = sid;

    return status;
}

//
// Well-Known SID Functions
//

NTSTATUS
RtlCreateWellKnownSid(
    IN WELL_KNOWN_SID_TYPE WellKnownSidType,
    IN OPTIONAL PSID DomainOrComputerSid,
    OUT OPTIONAL PSID Sid,
    IN OUT PULONG SidSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    union {
        SID Sid;
        UCHAR Buffer[SID_MAX_SIZE];
    } sidBuffer = { .Buffer = { 0 } };
    ULONG size = *SidSize;
    ULONG sizeRequired = 0;
    ULONG i = 0;

    // Copy the domain (prefix) part of the SID
    switch (WellKnownSidType)
    {
        case WinWorldSid:
        {
            // S-1-1-0
            SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_WORLD_SID_AUTHORITY };
            status = RtlInitializeSid(&sidBuffer.Sid, &identifierAuthority, 1);
            GOTO_CLEANUP_ON_STATUS(status);
            break;
        }
        case WinNetworkSid:
        {
            // S-1-1-2
            SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_WORLD_SID_AUTHORITY };
            status = RtlInitializeSid(&sidBuffer.Sid, &identifierAuthority, 1);
            GOTO_CLEANUP_ON_STATUS(status);
            sidBuffer.Sid.SubAuthority[0] = SECURITY_NETWORK_RID;
            break;
        }
        case WinInteractiveSid:
        {
            // S-1-1-4
            SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_WORLD_SID_AUTHORITY };
            status = RtlInitializeSid(&sidBuffer.Sid, &identifierAuthority, 1);
            GOTO_CLEANUP_ON_STATUS(status);
            sidBuffer.Sid.SubAuthority[0] = SECURITY_INTERACTIVE_RID;
            break;
        }
        case WinCreatorOwnerSid:
        {
            // S-1-3-0
            SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_CREATOR_SID_AUTHORITY };
            status = RtlInitializeSid(&sidBuffer.Sid, &identifierAuthority, 1);
            GOTO_CLEANUP_ON_STATUS(status);
            sidBuffer.Sid.SubAuthority[0] = SECURITY_CREATOR_OWNER_RID;
            break;
        }   
        case WinCreatorGroupSid:
        {
            // S-1-3-1
            SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_CREATOR_SID_AUTHORITY };
            status = RtlInitializeSid(&sidBuffer.Sid, &identifierAuthority, 1);
            GOTO_CLEANUP_ON_STATUS(status);
            sidBuffer.Sid.SubAuthority[0] = SECURITY_CREATOR_GROUP_RID;
            break;
        }   
        case WinAnonymousSid:
        {
            // S-1-5-7
            SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_CREATOR_SID_AUTHORITY };
            status = RtlInitializeSid(&sidBuffer.Sid, &identifierAuthority, 1);
            GOTO_CLEANUP_ON_STATUS(status);
            sidBuffer.Sid.SubAuthority[0] = SECURITY_ANONYMOUS_LOGON_RID;
            break;
        }
        case WinAuthenticatedUserSid:
        {
            // S-1-5-11
            SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_NT_AUTHORITY };
            status = RtlInitializeSid(&sidBuffer.Sid, &identifierAuthority, 1);
            GOTO_CLEANUP_ON_STATUS(status);
            sidBuffer.Sid.SubAuthority[0] = SECURITY_AUTHENTICATED_USER_RID;
            break;
        }
        case WinLocalSystemSid:
        {
            // S-1-5-18
            SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_NT_AUTHORITY };
            status = RtlInitializeSid(&sidBuffer.Sid, &identifierAuthority, 1);
            GOTO_CLEANUP_ON_STATUS(status);
            sidBuffer.Sid.SubAuthority[0] = SECURITY_LOCAL_SYSTEM_RID;
            break;
        }
        case WinLocalServiceSid:
        {
            // S-1-5-19
            SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_NT_AUTHORITY };
            status = RtlInitializeSid(&sidBuffer.Sid, &identifierAuthority, 1);
            GOTO_CLEANUP_ON_STATUS(status);
            sidBuffer.Sid.SubAuthority[0] = SECURITY_LOCAL_SERVICE_RID;
            break;
        }
        case WinBuiltinDomainSid:
        {
            // (S-1-5-32)
            SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_NT_AUTHORITY };
            status = RtlInitializeSid(&sidBuffer.Sid, &identifierAuthority, 1);
            GOTO_CLEANUP_ON_STATUS(status);
            sidBuffer.Sid.SubAuthority[i++] = SECURITY_BUILTIN_DOMAIN_RID;
            break;
        }

        case WinBuiltinAdministratorsSid:
        case WinBuiltinUsersSid:
        case WinBuiltinGuestsSid:
        case WinBuiltinPowerUsersSid:
        case WinBuiltinPrintOperatorsSid:
        case WinBuiltinBackupOperatorsSid:
        {
            // BUILTIN SIDs (S-1-5-32-X)
            SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_NT_AUTHORITY };
            status = RtlInitializeSid(&sidBuffer.Sid, &identifierAuthority, 2);
            GOTO_CLEANUP_ON_STATUS(status);
            sidBuffer.Sid.SubAuthority[i++] = SECURITY_BUILTIN_DOMAIN_RID;
            break;
        }

        case WinAccountAdministratorSid:
        case WinAccountGuestSid:
        case WinAccountDomainAdminsSid:
        case WinAccountDomainUsersSid:
        case WinAccountDomainGuestsSid:
        {
            if (DomainOrComputerSid == NULL)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_ON_STATUS(status);
            }

            SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_NT_AUTHORITY };
            status = RtlInitializeSid(&sidBuffer.Sid,
                                      &identifierAuthority,
                                      DomainOrComputerSid->SubAuthorityCount + 1);
            GOTO_CLEANUP_ON_STATUS(status);

            for (i = 0; i < DomainOrComputerSid->SubAuthorityCount; i++)
            {
                sidBuffer.Sid.SubAuthority[i] = DomainOrComputerSid->SubAuthority[i];
            }
            break;
        }
        default:
        {
            status = STATUS_NOT_IMPLEMENTED;
            GOTO_CLEANUP();
        }
    }

    // Append the RID
    switch (WellKnownSidType)
    {
        case WinBuiltinAdministratorsSid:
        {
            // S-1-5-32-544
            sidBuffer.Sid.SubAuthority[i] = DOMAIN_ALIAS_RID_ADMINS;
            break;
        }
        case WinBuiltinUsersSid:
        {
            // S-1-5-32-545
            sidBuffer.Sid.SubAuthority[i] = DOMAIN_ALIAS_RID_USERS;
            break;
        }
        case WinBuiltinGuestsSid:
        {
            // S-1-5-32-546
            sidBuffer.Sid.SubAuthority[i] = DOMAIN_ALIAS_RID_GUESTS;
            break;
        }
        case WinBuiltinPowerUsersSid:
        {
            // S-1-5-32-545
            sidBuffer.Sid.SubAuthority[i] = DOMAIN_ALIAS_RID_POWER_USERS;
            break;
        }
        case WinBuiltinPrintOperatorsSid:
        {
            // S-1-5-32-550
            sidBuffer.Sid.SubAuthority[i] = DOMAIN_ALIAS_RID_PRINT_OPS;
            break;
        }
        case WinBuiltinBackupOperatorsSid:
        {
            // S-1-5-32-551
            sidBuffer.Sid.SubAuthority[i] = DOMAIN_ALIAS_RID_BACKUP_OPS;
            break;
        }
        case WinAccountAdministratorSid:
        {
            // S-1-5-21-X-Y-Z-500
            sidBuffer.Sid.SubAuthority[i] = DOMAIN_USER_RID_ADMIN;
            break;
        }
        case WinAccountGuestSid:
        {
            // S-1-5-21-X-Y-Z-501
            sidBuffer.Sid.SubAuthority[i] = DOMAIN_USER_RID_GUEST;
            break;
        }
        case WinAccountDomainAdminsSid:
        {
            // S-1-5-21-X-Y-Z-512
            sidBuffer.Sid.SubAuthority[i] = DOMAIN_GROUP_RID_ADMINS;
            break;
        }
        case WinAccountDomainUsersSid:
        {
            // S-1-5-21-X-Y-Z-513
            sidBuffer.Sid.SubAuthority[i] = DOMAIN_GROUP_RID_USERS;
            break;
        }
        case WinAccountDomainGuestsSid:
        {
            // S-1-5-21-X-Y-Z-514
            sidBuffer.Sid.SubAuthority[i] = DOMAIN_GROUP_RID_GUESTS;
            break;
        }
    }

    sizeRequired = RtlLengthSid(&sidBuffer.Sid);
    if (size < sizeRequired)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    RtlCopyMemory(Sid, &sidBuffer.Sid, sizeRequired);
    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (Sid)
        {
            RtlZeroMemory(Sid, size);
        }
    }

    *SidSize = sizeRequired;

    return status;
}

NTSTATUS
RtlAllocateWellKnownSid(
    IN WELL_KNOWN_SID_TYPE WellKnownSidType,
    IN OPTIONAL PSID DomainOrComputerSid,
    OUT PSID* Sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;
    union {
        SID Sid;
        UCHAR Buffer[SID_MAX_SIZE];
    } sidBuffer = { .Buffer = { 0 } };
    ULONG size = sizeof(sidBuffer);

    status = RtlCreateWellKnownSid(
                    WellKnownSidType,
                    DomainOrComputerSid,
                    &sidBuffer.Sid,
                    &size);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlDuplicateSid(&sid, &sidBuffer.Sid);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&sid);
    }

    *Sid = sid;

    return status;
}

NTSTATUS
RtlConvertLittleEndianToSid(
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PSID Sid,
    IN OUT PULONG SidSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID srcSid = NULL;
    UCHAR i = 0;

    if (*SidSize < Length)
    {
        *SidSize = Length;     // Required length
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    srcSid = (PSID)Buffer;

    Sid->Revision = LW_LTOH8(srcSid->Revision);
    Sid->SubAuthorityCount = LW_LTOH8(srcSid->SubAuthorityCount);
    Sid->IdentifierAuthority = srcSid->IdentifierAuthority;   // Byte array
    for (i = 0; i < Sid->SubAuthorityCount; ++i)
    {
        Sid->SubAuthority[i] = LW_LTOH32(srcSid->SubAuthority[i]);
    }
    
    *SidSize = RtlLengthRequiredSid(Sid->SubAuthorityCount);

cleanup:

    return status;
}

NTSTATUS
RtlConvertSidToLittleEndian(
    IN PSID Sid,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT OPTIONAL PULONG UsedLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG sidSize = RtlLengthRequiredSid(Sid->SubAuthorityCount);
    ULONG usedLength = 0;
    PSID dstSid = NULL;
    UCHAR i = 0;

    if (sidSize > Length)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    dstSid = (PSID)Buffer;

    dstSid->Revision = LW_HTOL8(Sid->Revision);
    dstSid->SubAuthorityCount = LW_HTOL8(Sid->SubAuthorityCount);
    dstSid->IdentifierAuthority = Sid->IdentifierAuthority;    // Byte array
    for (i = 0; i < Sid->SubAuthorityCount; ++i)
    {
        dstSid->SubAuthority[i] = LW_HTOL32(Sid->SubAuthority[i]);
    }

    usedLength = sidSize;

cleanup:

    if (UsedLength)
    {
        *UsedLength = usedLength;
    }

    return status;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
