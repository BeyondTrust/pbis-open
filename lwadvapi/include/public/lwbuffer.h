/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
