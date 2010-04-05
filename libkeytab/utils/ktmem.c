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
 *        ktmem.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Memory Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "ktutils.h"

DWORD
KtAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    )
{
    DWORD dwError = 0;
    PVOID pMemory = NULL;

    pMemory = malloc(dwSize);
    if (!pMemory){
        dwError = ENOMEM;
        *ppMemory = NULL;
    }else {
        memset(pMemory,0, dwSize);
        *ppMemory = pMemory;
    }

    return dwError;
}

DWORD
KtDuplicateMemory(
    PVOID pMemory,
    DWORD dwSize,
    PVOID *ppNewMemory
    )
{
    DWORD dwError = 0;
    PVOID pNewMemory = NULL;

    if (pMemory != NULL) {
        dwError = KtAllocateMemory(dwSize, &pNewMemory);
        if (dwError != 0) return dwError;

        memcpy(pNewMemory, pMemory, dwSize);

    } else {
        dwError = KT_STATUS_INVALID_PARAMETER;
    }

    *ppNewMemory = pMemory;

    return dwError;
}

DWORD
KtReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
    )
{
    DWORD dwError = 0;
    PVOID pNewMemory = NULL;

    if (pMemory == NULL) {
       pNewMemory = malloc(dwSize);
       memset(pNewMemory, 0, dwSize);
    }else {
       pNewMemory = realloc(pMemory, dwSize);
    }
    if (!pNewMemory){
       dwError = KT_STATUS_OUT_OF_MEMORY;
       *ppNewMemory = NULL;
    }else {
       *ppNewMemory = pNewMemory;
    }

    return dwError;
}

void
KtFreeMemory(
    PVOID pMemory
    )
{
    free(pMemory);
}


DWORD
KtAllocateString(
    PCSTR  pszInputString,
    PSTR* ppszOutputString
    )
{
    DWORD dwError = 0;
    DWORD dwLen = 0;
    PSTR  pszOutputString = NULL;
    
    if (!pszInputString) {
        dwError = KT_STATUS_INVALID_PARAMETER;
        BAIL_ON_KT_ERROR(dwError);
    }

    dwLen = strlen(pszInputString);
       
    dwError = KtAllocateMemory(dwLen+1, (PVOID *)&pszOutputString);
    BAIL_ON_KT_ERROR(dwError);

    if (dwLen) {
       memcpy(pszOutputString, pszInputString, dwLen);
    }
    
    *ppszOutputString = pszOutputString;

cleanup:
    return dwError;

error:
    KT_SAFE_FREE_STRING(pszOutputString);

    *ppszOutputString = NULL;

    goto cleanup;
}


void
KtFreeString(
    PSTR pszString
    )
{
    KtFreeMemory(pszString);
}

#if defined(__LWI_AIX__) || defined(__LWI_HP_UX__)

#if !defined(HAVE_RPL_MALLOC)
#undef malloc

void* malloc(size_t n);

//See http://wiki.buici.com/wiki/Autoconf_and_RPL_MALLOC
void*
rpl_malloc(size_t n)
{
    if (n == 0)
        n = 1;
    return malloc(n);
}

#endif /* ! HAVE_RPL_MALLOC */

#endif /* defined(__LWI_AIX__) || defined(__LWI_HP_UX__) */

#if defined(__LWI_AIX__) || defined(__LWI_HP_UX__)

#if !defined(HAVE_RPL_REALLOC)
#undef realloc

void* realloc(void* buf, size_t n);

void*
rpl_realloc(void* buf, size_t n)
{
    return realloc(buf, n);
}

#endif /* ! HAVE_RPL_REALLOC */

#endif /* defined(__LWI_AIX__) || defined(__LWI_HP_UX__) */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
