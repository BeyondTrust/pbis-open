/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2010
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
 *        lwbuffer.h
 *
 * Abstract:
 *
 *        Memory buffer allocation functions.
 *
 *        Functions enabling allocation of arbitrary structures
 *        in flat memory buffer (returned from rpc client functions).
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


LW_BEGIN_EXTERN_C

DWORD
LwBufferAllocByte(
    OUT PVOID   pBuffer,
    IN OUT PDWORD  pdwOffset,
    IN OUT PDWORD  pdwSpaceLeft,
    IN BYTE   ubSource,
    IN OUT PDWORD  pdwSize
    );


DWORD
LwBufferAllocWord(
    OUT PVOID   pBuffer,
    IN OUT PDWORD  pdwOffset,
    IN OUT PDWORD  pdwSpaceLeft,
    IN WORD   swSource,
    IN OUT PDWORD  pdwSize
    );


DWORD
LwBufferAllocDword(
    OUT PVOID   pBuffer,
    IN OUT PDWORD  pdwOffset,
    IN OUT PDWORD  pdwSpaceLeft,
    IN DWORD   dwSource,
    IN OUT PDWORD  pdwSize
    );


DWORD
LwBufferAllocUlong64(
    OUT PVOID   pBuffer,
    IN OUT PDWORD  pdwOffset,
    IN OUT PDWORD  pdwSpaceLeft,
    IN ULONG64   ullSource,
    IN OUT PDWORD  pdwSize
    );


DWORD
LwBufferAllocWC16String(
    OUT PVOID        pBuffer,
    IN OUT PDWORD    pdwOffset,
    IN OUT PDWORD    pdwSpaceLeft,
    IN PCWSTR        pwszSource,
    IN OUT PDWORD    pdwSize
    );


DWORD
LwBufferAllocUnicodeString(
    OUT PVOID           pBuffer,
    IN OUT PDWORD       pdwOffset,
    IN OUT PDWORD       pdwSpaceLeft,
    IN PUNICODE_STRING  pSource,
    IN OUT PDWORD       pdwSize
    );


DWORD
LwBufferAllocAnsiString(
    OUT PVOID           pBuffer,
    IN OUT PDWORD       pdwOffset,
    IN OUT PDWORD       pdwSpaceLeft,
    IN PANSI_STRING     pSource,
    IN OUT PDWORD       pdwSize
    );


DWORD
LwBufferAllocWC16StringFromUnicodeString(
    OUT PVOID            pBuffer,
    IN OUT PDWORD           pdwOffset,
    IN OUT PDWORD           pdwSpaceLeft,
    IN PUNICODE_STRING  pSource,
    IN OUT PDWORD           pdwSize
    );


DWORD
LwBufferAllocUnicodeStringFromWC16String(
    OUT PVOID       pBuffer,
    IN OUT PDWORD   pdwOffset,
    IN OUT PDWORD   pdwSpaceLeft,
    IN PCWSTR       pwszSource,
    IN OUT PDWORD   pdwSize
    );


DWORD
LwBufferAllocUnicodeStringExFromWC16String(
    OUT PVOID       pBuffer,
    IN OUT PDWORD   pdwOffset,
    IN OUT PDWORD   pdwSpaceLeft,
    IN PCWSTR       pwszSource,
    IN OUT PDWORD   pdwSize
    );


DWORD
LwBufferAllocSid(
    OUT PVOID       pBuffer,
    IN OUT PDWORD   pdwOffset,
    IN OUT PDWORD   pdwSpaceLeft,
    IN PSID         pSourceSid,
    IN DWORD        dwSourceSidLength,
    IN OUT PDWORD   pdwSize
    );


DWORD
LwBufferAllocFixedBlob(
    OUT PVOID       pBuffer,
    IN OUT PDWORD   pdwOffset,
    IN OUT PDWORD   pdwSpaceLeft,
    IN PVOID        pBlob,
    IN DWORD        dwBlobSize,
    IN OUT PDWORD   pdwSize
    );


LW_END_EXTERN_C

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
