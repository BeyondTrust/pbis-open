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
 * Copyright (C) BeyondTrust Software. All rights reserved.
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

#include <lwmsg/type.h>
#include <lwmsg/data.h>

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


static
VOID
NtIpcDestroyPresented(
    LWMsgDataContext* context,
    LWMsgType* type,
    void* object,
    void* data
    )
{

    PUNICODE_STRING pObject = (PUNICODE_STRING) object;
    if (pObject->Length > 0)
    {
       free(pObject->Buffer);
       pObject->Buffer = NULL;
       pObject->Length = 0;
    }

    return;
}

static
void
NtIpcDestroyTransmitted(
    LWMsgDataContext* context,
    LWMsgType* type,
    void* object,
    void* data
    )
{
}

LWMsgTypeClass gNtIpcTypeClassUnicodeString =
{
    .is_pointer = LWMSG_FALSE,
    .transmit_type = gNtIpcTypeSpecTransmitUnicodeString,
    .marshal = NtIpcpMarshalUnicodeString,
    .unmarshal = NtIpcpUnmarshalUnicodeString,
    .destroy_presented = NtIpcDestroyPresented, // No custom free neded for unmarshalled type
    .destroy_transmitted = NtIpcDestroyTransmitted, // No custom free neded for marshalled type
    .print = NULL // Use standard print based on transmit type spec
};
