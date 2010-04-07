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
