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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *     ntipc-unicode_string.c
 *
 * Abstract:
 *
 *     UNICODE_STRING Custom Type Implementation for lwmsg
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "ntipc-unicode_string.h"
#include <lw/types.h>
#include <lw/attrs.h>

typedef struct _NT_IPC_TRANSMIT_UNICODE_STRING {
    // Not NULL-terminated
    PWCHAR Characters;
    USHORT Count;
} NT_IPC_TRANSMIT_UNICODE_STRING, *PNT_IPC_TRANSMIT_UNICODE_STRING;

static
LWMsgTypeSpec gNtIpcTypeSpecTransmitUnicodeString[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_TRANSMIT_UNICODE_STRING),
    LWMSG_MEMBER_UINT16(NT_IPC_TRANSMIT_UNICODE_STRING, Count),
    LWMSG_MEMBER_POINTER(NT_IPC_TRANSMIT_UNICODE_STRING, Characters, LWMSG_UINT16(WCHAR)),
    LWMSG_ATTR_LENGTH_MEMBER(NT_IPC_TRANSMIT_UNICODE_STRING, Count),
    LWMSG_ATTR_ENCODING("utf-16"),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgStatus
NtIpcpMarshalUnicodeString(
    IN LWMsgDataContext* MarshalContext,
    IN LWMsgType* Type,
    IN PVOID ActualObject,
    IN PVOID TransmitObject,
    IN OPTIONAL PVOID CustomContext
    )
{
    PUNICODE_STRING object = (PUNICODE_STRING) ActualObject;
    PNT_IPC_TRANSMIT_UNICODE_STRING transmit = TransmitObject;

    transmit->Characters = object->Buffer;
    transmit->Count = object->Length / sizeof(object->Buffer[0]);

    return LWMSG_STATUS_SUCCESS;
}

static
LWMsgStatus
NtIpcpUnmarshalUnicodeString(
    IN LWMsgDataContext* MarshalContext,
    IN LWMsgType* Type,
    IN PVOID TransmitObject,
    IN PVOID ActualObject,
    IN OPTIONAL PVOID CustomContext
    )
{
    PNT_IPC_TRANSMIT_UNICODE_STRING transmit = TransmitObject;
    PUNICODE_STRING object = (PUNICODE_STRING) ActualObject;

    object->Length = transmit->Count * sizeof(object->Buffer[0]);
    object->MaximumLength = object->Length;
    if (object->Length)
    {
        object->Buffer = transmit->Characters;
        transmit->Characters = NULL;
    }
    else
    {
        object->Buffer = NULL;
    }

    return LWMSG_STATUS_SUCCESS;
}

LWMsgTypeClass gNtIpcTypeClassUnicodeString =
{
    .is_pointer = LWMSG_FALSE,
    .transmit_type = gNtIpcTypeSpecTransmitUnicodeString,
    .marshal = NtIpcpMarshalUnicodeString,
    .unmarshal = NtIpcpUnmarshalUnicodeString,
    .destroy_presented = NULL, // No custom free needed for unmarshalled type
    .destroy_transmitted = NULL, // No custom free neded for marshalled type
    .print = NULL // Use standard print based on transmit type spec
};
