/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 *        net_memory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetAPI memory allocation functions.
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#ifndef _NET_MEMORY_H_
#define _NET_MEMORY_H_


#define ALIGN_PTR_IN_BUFFER(type, field, cursor, size, left)         \
    {                                                                \
        DWORD dwFieldOffset = ((size_t)(&(((type*)(0))->field)));    \
        DWORD dwFieldSize   = sizeof((((type*)(0))->field));         \
        DWORD dwAlign = (dwFieldOffset + dwFieldSize)                \
                         % sizeof(PVOID);                         \
        if (cursor)                                                  \
        {                                                            \
            (cursor) += dwAlign;                                     \
            left -= dwAlign;                                         \
        }                                                            \
                                                                     \
        size += dwAlign;                                             \
    }


typedef enum _NET_VALIDATION_LEVEL
{
    NET_VALIDATION_NONE      = 0,
    NET_VALIDATION_OPTIONAL,
    NET_VALIDATION_REQUIRED,
    NET_VALIDATION_USER_ADD,
    NET_VALIDATION_USER_SET

} NET_VALIDATION_LEVEL;


NTSTATUS
NetAllocateMemory(
    OUT PVOID *ppOut,
    IN  size_t sSize
    );


VOID
NetFreeMemory(
    IN PVOID pPtr
    );


DWORD
NetAllocBufferByte(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    BYTE    ubSource,
    PDWORD  pdwSize
    );


DWORD
NetAllocBufferWord(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    WORD    wSource,
    PDWORD  pdwSize
    );


DWORD
NetAllocBufferDword(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    DWORD   dwSource,
    PDWORD  pdwSize
    );


DWORD
NetAllocBufferUlong64(
    PVOID   *ppCursor,
    PDWORD   pdwSpaceLeft,
    ULONG64  ullSource,
    PDWORD   pdwSize
    );


DWORD
NetAllocBufferWinTimeFromNtTime(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    LONG64  ntTime,
    PDWORD  pdwSize
    );


DWORD
NetAllocBufferNtTimeFromWinTime(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    DWORD   dwTime,
    PDWORD  pdwSize
    );


DWORD
NetAllocBufferUserFlagsFromAcbFlags(
    PVOID *ppCursor,
    PDWORD pdwSpaceLeft,
    DWORD  dwAcbFlags,
    PDWORD pdwSize
    );


DWORD
NetAllocBufferAcbFlagsFromUserFlags(
    PVOID *ppCursor,
    PDWORD pdwSpaceLeft,
    DWORD  dwUserFlags,
    PDWORD pdwSize
    );


DWORD
NetAllocBufferWC16String(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PCWSTR                pwszSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    );


DWORD
NetAllocBufferWC16StringFromUnicodeString(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    UNICODE_STRING       *pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    );


DWORD
NetAllocBufferUnicodeStringFromWC16String(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PCWSTR                pwszSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    );


DWORD
NetAllocBufferLogonHours(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    LogonHours           *pHours,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    );


DWORD
NetAllocBufferSamrLogonHoursFromNetLogonHours(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PDWORD                pdwHours,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    );


DWORD
NetAllocBufferSid(
    PVOID               *ppCursor,
    PDWORD               pdwSpaceLeft,
    PSID                 pSourceSid,
    DWORD                dwSourceSidLength,
    PDWORD               pdwSize,
    NET_VALIDATION_LEVEL eValidation
    );


DWORD
NetAllocBufferByteBlob(
    PVOID               *ppCursor,
    PDWORD               pdwSpaceLeft,
    PBYTE                pbBlob,
    DWORD                dwBlobSize,
    PDWORD               pdwSize,
    NET_VALIDATION_LEVEL eValidation
    );


DWORD
NetAllocBufferFixedBlob(
    PVOID                *ppCursor,
    PDWORD                pdwSpaceLeft,
    PBYTE                 pbBlob,
    DWORD                 dwBlobSize,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    );


DWORD
NetAllocBufferNT4Name(
    PVOID   *ppCursor,
    PDWORD   pdwSpaceLeft,
    PWSTR    pwszDomainName,
    PWSTR    pwszAccountName,    
    PDWORD   pdwSize
    );


#endif /* _NET_MEMORY_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
