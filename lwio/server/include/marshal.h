#include <lw/base.h>

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

static
inline
NTSTATUS
Align(
    IN PBYTE pBase,
    IN OUT PBYTE* ppCursor,
    IN OUT PULONG pulRemainingSpace,
    IN USHORT usAlignment
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT usRemainder = 0;

    usRemainder = (*ppCursor - pBase) % usAlignment;

    if (usRemainder)
    {
        if (pulRemainingSpace && *pulRemainingSpace < (usAlignment - usRemainder))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            BAIL_ON_NT_STATUS(status);
        }

        if (pulRemainingSpace)
        {
            memset(*ppCursor, 0, (usAlignment - usRemainder));
            *ppCursor += (usAlignment - usRemainder);
            *pulRemainingSpace -= (usAlignment - usRemainder);
        }
    }
    
error:
    
    return status;
}

static
inline
NTSTATUS
Advance(
    IN OUT PBYTE* ppCursor,
    IN OUT PULONG pulRemainingSpace,
    IN ULONG ulOffset
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (pulRemainingSpace && *pulRemainingSpace < ulOffset)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    if (pulRemainingSpace)
    {
        *ppCursor += ulOffset;
        *pulRemainingSpace -= ulOffset;
    }
    
error:
    
    return status;
}

static
inline
NTSTATUS
AdvanceTo(
    IN OUT PBYTE* ppCursor,
    IN OUT PULONG pulRemainingSpace,
    IN PBYTE      pDestination
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (pulRemainingSpace && *pulRemainingSpace < (pDestination - *ppCursor))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    if (pulRemainingSpace)
    {
        *pulRemainingSpace -= (pDestination - *ppCursor);
    }

    *ppCursor = pDestination;
    
error:
    
    return status;
}

static
inline
NTSTATUS
MarshalData(
    IN OUT PBYTE*  ppCursor,
    IN OUT PULONG  pulRemainingSpace,
    IN PBYTE       pData,
    ULONG          ulDataLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if (pulRemainingSpace && *pulRemainingSpace < ulDataLength)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(*ppCursor, pData, ulDataLength);

    if (pulRemainingSpace)
    {
        *ppCursor += ulDataLength;
        *pulRemainingSpace -= ulDataLength;
    }
    
error:
    
    return status;
}

static
inline
NTSTATUS
MarshalByte(
    IN OUT PBYTE*  ppCursor,
    IN OUT PULONG  pulRemainingSpace,
    IN BYTE        data
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if (pulRemainingSpace && *pulRemainingSpace < sizeof(data))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    (*ppCursor)[0] = data;
    
    if (pulRemainingSpace)
    {
        *ppCursor += sizeof(data);
        *pulRemainingSpace -= sizeof(data);
    }

error:

    return status;
}

static
inline
NTSTATUS
MarshalUshort(
    IN OUT PBYTE*  ppCursor,
    IN OUT PULONG  pulRemainingSpace,
    IN USHORT      usData
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if (pulRemainingSpace && *pulRemainingSpace < sizeof(usData))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    (*ppCursor)[0] = usData & 0xFF;
    (*ppCursor)[1] = (usData >> 8) & 0xFF;

    if (pulRemainingSpace)
    {
        *ppCursor += sizeof(usData);
        *pulRemainingSpace -= sizeof(usData);
    }

error:

    return status;
}

static
inline
NTSTATUS
MarshalUlong(
    IN OUT PBYTE*  ppCursor,
    IN OUT PULONG  pulRemainingSpace,
    IN ULONG       ulData
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if (pulRemainingSpace && *pulRemainingSpace < sizeof(ulData))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    (*ppCursor)[0] = ulData & 0xFF;
    (*ppCursor)[1] = (ulData >> 8) & 0xFF;
    (*ppCursor)[2] = (ulData >> 16) & 0xFF;
    (*ppCursor)[3] = (ulData >> 24) & 0xFF;

    if (pulRemainingSpace)
    {
        *ppCursor += sizeof(ulData);
        *pulRemainingSpace -= sizeof(ulData);
    }

error:

    return status;
}

static
inline
NTSTATUS
MarshalUlong64(
    IN OUT PBYTE*  ppCursor,
    IN OUT PULONG  pulRemainingSpace,
    IN ULONG64     ullData
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if (pulRemainingSpace && *pulRemainingSpace < sizeof(ullData))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    (*ppCursor)[0] = ullData & 0xFF;
    (*ppCursor)[1] = (ullData >> 8) & 0xFF;
    (*ppCursor)[2] = (ullData >> 16) & 0xFF;
    (*ppCursor)[3] = (ullData >> 24) & 0xFF;
    (*ppCursor)[4] = (ullData >> 32) & 0xFF;
    (*ppCursor)[5] = (ullData >> 40) & 0xFF;
    (*ppCursor)[6] = (ullData >> 48) & 0xFF;
    (*ppCursor)[7] = (ullData >> 56) & 0xFF;

    if (pulRemainingSpace)
    {
        *ppCursor += sizeof(ullData);
        *pulRemainingSpace -= sizeof(ullData);
    }

error:

    return status;
}

static
inline
NTSTATUS
MarshalPwstr(
    IN OUT PBYTE* ppCursor,
    IN OUT PULONG pulRemainingSpace,
    IN PCWSTR pwszString,
    LONG lLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (lLength == -1)
    {
        lLength = (LwRtlWC16StringNumChars(pwszString) + 1) * sizeof(WCHAR);
    }

    if (pulRemainingSpace && *pulRemainingSpace < lLength)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

#ifdef WORDS_BIGENDIAN
    swab(pwszString, *ppCursor, lLength);
#else
    memcpy(*ppCursor, pwszString, lLength);
#endif

    *ppCursor += lLength;

    if (pulRemainingSpace)
    {
        *pulRemainingSpace -= lLength;
    }

error:

    return status;
}

static
inline
NTSTATUS
UnmarshalData(
    IN OUT PBYTE*  ppCursor,
    IN OUT PULONG  pulRemainingSpace,
    OUT PBYTE      pData,
    IN ULONG       ulDataLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if (pulRemainingSpace && *pulRemainingSpace < ulDataLength)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(pData, *ppCursor, ulDataLength);

    if (pulRemainingSpace)
    {
        *ppCursor += ulDataLength;
        *pulRemainingSpace -= ulDataLength;
    }

error:
    
    return status;
}

static
inline
NTSTATUS
UnmarshalByte(
    IN OUT PBYTE*  ppCursor,
    IN OUT PULONG  pulRemainingSpace,
    OUT PBYTE      pData
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if (pulRemainingSpace && *pulRemainingSpace < sizeof(*pData))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    *pData = (*ppCursor)[0];

    if (pulRemainingSpace)
    {
        *ppCursor += sizeof(*pData);
        *pulRemainingSpace -= sizeof(*pData);
    }

error:

    return status;
}

static
inline
NTSTATUS
UnmarshalUshort(
    IN OUT PBYTE*  ppCursor,
    IN OUT PULONG  pulRemainingSpace,
    OUT PUSHORT    pusData
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if (pulRemainingSpace && *pulRemainingSpace < sizeof(*pusData))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    *pusData = 
        ((*ppCursor)[0]) |
        ((*ppCursor)[1] << 8);

    if (pulRemainingSpace)
    {
        *ppCursor += sizeof(*pusData);
        *pulRemainingSpace -= sizeof(*pusData);
    }

error:

    return status;
}

static
inline
NTSTATUS
UnmarshalUlong(
    IN OUT PBYTE*  ppCursor,
    IN OUT PULONG  pulRemainingSpace,
    OUT PULONG     pulData
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if (pulRemainingSpace && *pulRemainingSpace < sizeof(*pulData))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    *pulData = 
        ((*ppCursor)[0]) |
        ((*ppCursor)[1] << 8) |
        ((*ppCursor)[2] << 16) |
        ((*ppCursor)[3] << 24);

    if (pulRemainingSpace)
    {
        *ppCursor += sizeof(*pulData);
        *pulRemainingSpace -= sizeof(*pulData);
    }

error:

    return status;
}

static
inline
NTSTATUS
UnmarshalUlong64(
    IN OUT PBYTE*  ppCursor,
    IN OUT PULONG  pulRemainingSpace,
    OUT PULONG64   pullData
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if (pulRemainingSpace && *pulRemainingSpace < sizeof(*pullData))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    *pullData = 
        (((ULONG64) (*ppCursor)[0])) |
        (((ULONG64) (*ppCursor)[1]) << 8) |
        (((ULONG64) (*ppCursor)[2]) << 16) |
        (((ULONG64) (*ppCursor)[3]) << 24) |
        (((ULONG64) (*ppCursor)[3]) << 32) |
        (((ULONG64) (*ppCursor)[3]) << 40) |
        (((ULONG64) (*ppCursor)[3]) << 48) |
        (((ULONG64) (*ppCursor)[3]) << 56);

    if (pulRemainingSpace)
    {
        *ppCursor += sizeof(*pullData);
        *pulRemainingSpace -= sizeof(*pullData);
    }

error:

    return status;
}
