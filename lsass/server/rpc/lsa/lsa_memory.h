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
 *        lsa_memory.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Lsa memory allocation manager
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _LSASRV_MEMORY_H_
#define _LSASRV_MEMORY_H_


NTSTATUS
LsaSrvAllocateMemory(
    void **ppOut,
    DWORD dwSize
    );


void
LsaSrvFreeMemory(
    void *pPtr
    );


NTSTATUS
LsaSrvAllocateSidFromWC16String(
    PSID *ppSid,
    PCWSTR pwszSidStr
    );


NTSTATUS
LsaSrvDuplicateSid(
    PSID *ppSidOut,
    PSID pSidIn
    );


NTSTATUS
LsaSrvDuplicateWC16String(
    PWSTR *ppwszOut,
    PWSTR pwszIn
    );


NTSTATUS
LsaSrvDuplicateWC16String(
    PWSTR *ppwszOut,
    PWSTR pwszIn
    );


NTSTATUS
LsaSrvGetFromUnicodeString(
    PWSTR *ppwszOut,
    UNICODE_STRING *pIn
    );


NTSTATUS
LsaSrvGetFromUnicodeStringEx(
    PWSTR *ppwszOut,
    UNICODE_STRING *pIn
    );


NTSTATUS
LsaSrvInitUnicodeString(
    UNICODE_STRING *pOut,
    PCWSTR pwszIn
    );


NTSTATUS
LsaSrvInitUnicodeStringEx(
    UNICODE_STRING *pOut,
    PCWSTR pwszIn
    );


NTSTATUS
LsaSrvDuplicateUnicodeString(
    UNICODE_STRING *pOut,
    UNICODE_STRING *pIn
    );


NTSTATUS
LsaSrvDuplicateUnicodeStringEx(
    UNICODE_STRING *pOut,
    UNICODE_STRING *pIn
    );


VOID
LsaSrvFreeUnicodeString(
    PUNICODE_STRING pString
    );


NTSTATUS
LsaSrvSidAppendRid(
    PSID *ppOutSid,
    PSID pInSid,
    DWORD dwRid
    );


#endif /* _LSASRV_MEMORY_H_ */
