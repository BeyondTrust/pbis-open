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
 *        dsr_memory.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        DsSetup rpc memory management functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _DSR_MEMORY_H_
#define _DSR_MEMORY_H_


#define LWBUF_ALLOC_WORD(buffer, val)                           \
    dwError = LwBufferAllocWord((buffer),                       \
                                 pdwOffset,                     \
                                 pdwSpaceLeft,                  \
                                 (val),                         \
                                 pdwSize);                      \
    BAIL_ON_WIN_ERROR(dwError)

#define LWBUF_ALLOC_DWORD(buffer, val)                          \
    dwError = LwBufferAllocDword((buffer),                      \
                                 pdwOffset,                     \
                                 pdwSpaceLeft,                  \
                                 (val),                         \
                                 pdwSize);                      \
    BAIL_ON_WIN_ERROR(dwError)

#define LWBUF_ALLOC_PWSTR(buffer, val)                          \
    dwError = LwBufferAllocWC16String((buffer),                 \
                                      pdwOffset,                \
                                      pdwSpaceLeft,             \
                                      (val),                    \
                                      pdwSize);                 \
    BAIL_ON_WIN_ERROR(dwError)

#define LWBUF_ALLOC_BLOB(buffer, size, ptr)                     \
    dwError = LwBufferAllocFixedBlob((buffer),                  \
                                     pdwOffset,                 \
                                     pdwSpaceLeft,              \
                                     (ptr),                     \
                                     (size),                    \
                                     pdwSize);                  \
    BAIL_ON_WIN_ERROR(dwError)


#define LWBUF_ALIGN_TYPE(offset_ptr, size_ptr, space_ptr, type)     \
    {                                                               \
        DWORD dwAlign = (*(offset_ptr)) % sizeof(type);             \
                                                                    \
        dwAlign   = (dwAlign) ? (sizeof(type) - dwAlign) : 0;       \
        if ((size_ptr))                                             \
        {                                                           \
            (*(size_ptr)) += dwAlign;                               \
        }                                                           \
                                                                    \
        (*(offset_ptr)) += dwAlign;                                 \
                                                                    \
        if (space_ptr)                                              \
        {                                                           \
            (*(space_ptr)) -= dwAlign;                              \
        }                                                           \
    }

#define LWBUF_ALIGN(offset_ptr, size_ptr, space_ptr)                \
    LWBUF_ALIGN_TYPE(offset_ptr, size_ptr, space_ptr, PVOID)


DWORD
DsrAllocateMemory(
    OUT PVOID *ppOut,
    IN  size_t sSize
    );


DWORD
DsrAllocateDsRoleInfo(
    OUT PDSR_ROLE_INFO  pOut,
    IN OUT PDWORD       pdwOffset,
    IN OUT PDWORD       pdwSpaceLeft,
    IN  PDSR_ROLE_INFO  pIn,
    IN  WORD            swLevel,
    IN OUT PDWORD       dwSize
    );


#endif /* _DSR_MEMORY_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
