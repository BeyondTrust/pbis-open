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
 *        lsa_memory.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Lsa rpc memory management functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _LSA_MEMORY_H_
#define _LSA_MEMORY_H_


#define LWBUF_ALLOC_BYTE(buffer, val)                           \
    dwError = LwBufferAllocByte((buffer),                       \
                                 pdwOffset,                     \
                                 pdwSpaceLeft,                  \
                                 (val),                         \
                                 pdwSize);                      \
    BAIL_ON_WIN_ERROR(dwError)


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


#define LWBUF_ALLOC_ULONG64(buffer, val)                        \
    dwError = LwBufferAllocUlong64((buffer),                    \
                                   pdwOffset,                   \
                                   pdwSpaceLeft,                \
                                   (val),                       \
                                   pdwSize);                    \
    BAIL_ON_WIN_ERROR(dwError)


#define LWBUF_ALLOC_NTTIME(buffer, val)                         \
    LWBUF_ALLOC_ULONG64((buffer), (ULONG64)(val))


#define LWBUF_ALLOC_WC16STR_FROM_UNICODE_STRING(buffer, ptr)    \
    dwError = LwBufferAllocWC16StringFromUnicodeString(         \
                                 (buffer),                      \
                                 pdwOffset,                     \
                                 pdwSpaceLeft,                  \
                                 (ptr),                         \
                                 pdwSize);                      \
    BAIL_ON_WIN_ERROR(dwError)


#define LWBUF_ALLOC_UNICODE_STRING(buffer, ptr)                 \
    dwError = LwBufferAllocUnicodeString(                       \
                                 (buffer),                      \
                                 pdwOffset,                     \
                                 pdwSpaceLeft,                  \
                                 (ptr),                         \
                                 pdwSize);                      \
    BAIL_ON_WIN_ERROR(dwError)


#define LWBUF_ALLOC_ANSI_STRING(buffer, ptr)                    \
    dwError = LwBufferAllocAnsiString(                          \
                                 (buffer),                      \
                                 pdwOffset,                     \
                                 pdwSpaceLeft,                  \
                                 (ptr),                         \
                                 pdwSize);                      \
    BAIL_ON_WIN_ERROR(dwError)


#define LWBUF_ALLOC_PSID(buffer, ptr)                           \
    dwError = LwBufferAllocSid((buffer),                        \
                               pdwOffset,                       \
                               pdwSpaceLeft,                    \
                               (ptr),                           \
                               0,                               \
                               pdwSize);                        \
    BAIL_ON_WIN_ERROR(dwError)


#define LWBUF_ALLOC_BLOB(buffer, size, ptr)                     \
    dwError = LwBufferAllocFixedBlob((buffer),                  \
                                     pdwOffset,                 \
                                     pdwSpaceLeft,              \
                                     (ptr),                     \
                                     (size),                    \
                                     pdwSize);                  \
    BAIL_ON_WIN_ERROR(dwError)


#define LWBUF_ALIGN_SIZE(size)                                      \
    (((size) % sizeof(PVOID)) ?                                     \
        ((size) + sizeof(PVOID) - ((size) % sizeof(PVOID))) :       \
        (size))


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


#define LWBUF_ALIGN_PTR(offset_ptr, size_ptr, space_ptr)            \
    LWBUF_ALIGN_TYPE(offset_ptr, size_ptr, space_ptr, PVOID)


#define BAIL_IF_NOT_ENOUGH_SPACE(size, space_ptr, err)              \
    if ((size) > (*(space_ptr)))                                    \
    {                                                               \
        err = ERROR_INSUFFICIENT_BUFFER;                            \
        BAIL_ON_WIN_ERROR((err));                                   \
    }


#define BAIL_IF_PTR_OVERLAP(type, target_ptr, err)                  \
    if ((pCursor + sizeof(type)) > (target_ptr))                    \
    {                                                               \
        err = ERROR_INSUFFICIENT_BUFFER;                            \
        BAIL_ON_WIN_ERROR(err);                                     \
    }


#define LWBUF_TARGET_PTR(buffer_ptr, target_size, space_ptr)        \
    ((pCursor = (buffer_ptr) + (*pdwOffset)),                       \
     ((pCursor + (*(space_ptr))) - LWBUF_ALIGN_SIZE(target_size)))


NTSTATUS
LsaRpcAllocateMemory(
    OUT PVOID *ppOut,
    IN  size_t Size
    );


NTSTATUS
LsaAllocateTranslatedSids(
    OUT TranslatedSid       *pOut,
    IN OUT PDWORD            pdwOffset,
    IN OUT PDWORD            pdwSpaceLeft,
    IN  TranslatedSidArray  *pIn,
    IN OUT PDWORD            pdwSize
    );


NTSTATUS
LsaAllocateTranslatedSids2(
    OUT TranslatedSid2       *pOut,
    IN OUT PDWORD             pdwOffset,
    IN OUT PDWORD             pdwSpaceLeft,
    IN  TranslatedSidArray2  *pIn,
    IN OUT PDWORD             pdwSize
    );


NTSTATUS
LsaAllocateTranslatedSids3(
    OUT TranslatedSid3       *pOut,
    IN OUT PDWORD             pdwOffset,
    IN OUT PDWORD             pdwSpaceLeft,
    IN  TranslatedSidArray3  *pIn,
    IN OUT PDWORD             pdwSize
    );


NTSTATUS
LsaAllocateRefDomainList(
    OUT RefDomainList *pOut,
    IN OUT PDWORD      pdwOffset,
    IN OUT PDWORD      pdwSpaceLeft,
    IN  RefDomainList *pIn,
    IN OUT PDWORD      pdwSize
    );


NTSTATUS
LsaAllocateTranslatedNames(
    OUT TranslatedName       *pOut,
    IN OUT PDWORD             pdwOffset,
    IN OUT PDWORD             pdwSpaceLeft,
    IN  TranslatedNameArray  *pIn,
    IN OUT PDWORD             pdwSize
    );


NTSTATUS
LsaAllocatePolicyInformation(
    OUT LsaPolicyInformation *pOut,
    IN OUT PDWORD             pdwOffset,
    IN OUT PDWORD             pdwSpaceLeft,
    IN  WORD                  swLevel,
    IN  LsaPolicyInformation *pIn,
    IN OUT PDWORD             pdwSize
    );


NTSTATUS
LsaAllocateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR_RELATIVE   *ppOut,
    IN  PLSA_SECURITY_DESCRIPTOR_BUFFER  pIn
    );


NTSTATUS
LsaAllocateSids(
    OUT PSID                    *pOut,
    IN OUT PDWORD                pdwOffset,
    IN OUT PDWORD                pdwSpaceLeft,
    IN PLSA_ACCOUNT_ENUM_BUFFER  pIn,
    IN OUT PDWORD                pdwSize
    );


NTSTATUS
LsaAllocateAccountRightNames(
    OUT PWSTR               *pOut,
    IN OUT PDWORD            pdwOffset,
    IN OUT PDWORD            pdwSpaceLeft,
    IN PLSA_ACCOUNT_RIGHTS   pIn,
    IN OUT PDWORD            pdwSize
    );


NTSTATUS
LsaAllocatePrivilegeNames(
    OUT PWSTR                     *pOut,
    IN OUT PDWORD                  pdwOffset,
    IN OUT PDWORD                  pdwSpaceLeft,
    IN PLSA_PRIVILEGE_ENUM_BUFFER  pIn,
    IN OUT PDWORD                  pdwSize
    );


#endif /* _LSA_MEMORY_H_ */
