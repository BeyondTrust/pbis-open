/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        ktmem.h
 *
 * Abstract:
 *
 *        Kerberos 5 keytab management library
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef __KTMEM_H__
#define __KTMEM_H__


typedef struct _KT_STRING_BUFFER
{
    PSTR pszBuffer;
    // length of the string excluding terminating null
    size_t sLen;
    // capacity of the buffer excluding terminating null
    size_t sCapacity;
} KT_STRING_BUFFER;

DWORD
KtInitializeStringBuffer(
        KT_STRING_BUFFER *pBuffer,
        size_t sCapacity);

DWORD
KtAppendStringBuffer(
        KT_STRING_BUFFER *pBuffer,
        PCSTR pszAppend);

void
KtFreeStringBufferContents(
        KT_STRING_BUFFER *pBuffer);

DWORD
KtAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    );

DWORD
KtDuplicateMemory(
    PVOID pMemory,
    DWORD dwSize,
    PVOID *ppNewMemory
    );

DWORD
KtReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
    );


void
KtFreeMemory(
    PVOID pMemory
    );


DWORD
KtAllocateString(
    PCSTR pszInputString, 
    PSTR *ppszOutputString
    );


void
KtFreeString(
    PSTR pszString
    );

void
KtFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    );

void
KtFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    );

#if defined(__LWI_AIX__) || defined(__LWI_HP_UX__)

#if !defined(HAVE_RPL_MALLOC)

void*
rpl_malloc(
    size_t n
    );

#endif /* ! HAVE_RPL_MALLOC */

#if !defined(HAVE_RPL_REALLOC)

void*
rpl_realloc(
    void* buf,
    size_t n
    );

#endif /* ! HAVE_RPL_REALLOC */

#endif /* defined(__LWI_AIX__) || defined(__LWI_HP_UX__) */

#endif /* __KTMEM_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
