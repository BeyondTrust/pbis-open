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
 *        wkssvc_memory.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        WksSvc memory allocation manager
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _WKSSSRV_MEMORY_H_
#define _WKSSSRV_MEMORY_H_


NTSTATUS
WkssSrvAllocateMemory(
    void **ppOut,
    DWORD dwSize
    );


void
WkssSrvFreeMemory(
    void *pPtr
    );


NTSTATUS
WkssSrvAllocateSidFromWC16String(
    PSID *ppSid,
    PCWSTR pwszSidStr
    );


NTSTATUS
WkssSrvDuplicateSid(
    PSID *ppSidOut,
    PSID pSidIn
    );


NTSTATUS
WkssSrvDuplicateWC16String(
    PWSTR *ppwszOut,
    PWSTR pwszIn
    );


NTSTATUS
WkssSrvDuplicateWC16String(
    PWSTR *ppwszOut,
    PWSTR pwszIn
    );


NTSTATUS
WkssSrvGetFromUnicodeString(
    PWSTR *ppwszOut,
    UnicodeString *pIn
    );


NTSTATUS
WkssSrvGetFromUnicodeStringEx(
    PWSTR *ppwszOut,
    UnicodeStringEx *pIn
    );


NTSTATUS
WkssSrvInitUnicodeString(
    UnicodeString *pOut,
    PCWSTR pwszIn
    );


NTSTATUS
WkssSrvInitUnicodeStringEx(
    UnicodeStringEx *pOut,
    PCWSTR pwszIn
    );


NTSTATUS
WkssSrvDuplicateUnicodeString(
    UnicodeString *pOut,
    UnicodeString *pIn
    );


NTSTATUS
WkssSrvSidAppendRid(
    PSID *ppOutSid,
    PSID pInSid,
    DWORD dwRid
    );


#endif /* _WKSSSRV_MEMORY_H_ */
