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
 *        ktstr.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) String Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "ktutils.h"
#if HAVE_WC16STR_H
#include <wc16str.h>
#endif
#if HAVE_WC16PRINTF_H
#include <wc16printf.h>
#endif


DWORD
KtAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    )
{
    DWORD dwError = 0;
    va_list args;
    
    va_start(args, pszFormat);
    
    dwError = KtAllocateStringPrintfV(
                      ppszOutputString,
                      pszFormat,
                      args);
    
    va_end(args);

    return dwError;
}


DWORD
KtAllocateStringPrintfV(
    PSTR*   ppszOutputString,
    PCSTR   pszFormat,
    va_list args
    )
{
    DWORD dwError = 0;
    PSTR  pszSmallBuffer = NULL;
    DWORD dwBufsize = 0;
    INT   requiredLength = 0;
    DWORD dwNewRequiredLength = 0;
    PSTR  pszOutputString = NULL;
    va_list args2;

    va_copy(args2, args);

    dwBufsize = 4;
    /* Use a small buffer in case libc does not like NULL */
    do
    {
        dwError = KtAllocateMemory(
                        dwBufsize, 
                        (PVOID*) &pszSmallBuffer);
        BAIL_ON_KT_ERROR(dwError);
        
        requiredLength = vsnprintf(
                              pszSmallBuffer,
                              dwBufsize,
                              pszFormat,
                              args);
        if (requiredLength < 0)
        {
            dwBufsize *= 2;
        }
        KtFreeMemory(pszSmallBuffer);
        pszSmallBuffer = NULL;
        
    } while (requiredLength < 0);

    if (requiredLength >= (UINT32_MAX - 1))
    {
        dwError = KT_STATUS_OUT_OF_MEMORY;
        BAIL_ON_KT_ERROR(dwError);
    }

    dwError = KtAllocateMemory(
                    requiredLength + 2,
                    (PVOID*)&pszOutputString);
    BAIL_ON_KT_ERROR(dwError);

    dwNewRequiredLength = vsnprintf(
                            pszOutputString,
                            requiredLength + 1,
                            pszFormat,
                            args2);
    if (dwNewRequiredLength < 0)
    {
        dwError = errno;
        BAIL_ON_KT_ERROR(dwError);
    }
    else if (dwNewRequiredLength > requiredLength)
    {
        /* unexpected, ideally should log something, or use better error code */
        dwError = KT_STATUS_OUT_OF_MEMORY;
        BAIL_ON_KT_ERROR(dwError);
    }
    else if (dwNewRequiredLength < requiredLength)
    {
        /* unexpected, ideally should log something -- do not need an error, though */
    }
    
    *ppszOutputString = pszOutputString;

cleanup:

    va_end(args2);
    
    return dwError;
    
error:

    KT_SAFE_FREE_MEMORY(pszOutputString);
    
    *ppszOutputString = NULL;

    goto cleanup;
}


void
KtStrToUpper(
    PSTR pszString
)
{
    if (IsNullOrEmptyString(pszString)) {
        return;
    }

    while (*pszString != '\0') {
        *pszString = toupper((int)*pszString);
        pszString++;
    }
}


void
KtStrToLower(
    PSTR pszString
    )
{
    if (IsNullOrEmptyString(pszString)) {
        return;
    }

    while (*pszString != '\0') {
        *pszString = tolower((int)*pszString);
        pszString++;
    }
}


void
KtStrChr(
    PCSTR pszInputString,
    CHAR c,
    PSTR *ppszOutputString
    )
{
    PSTR pszFound = NULL;

    if (ppszOutputString == NULL) return;

    if (pszInputString == NULL)
    {
        *ppszOutputString = NULL;
        return;
    }

    pszFound = strchr(pszInputString, c);
    if (pszFound == NULL) {
        *ppszOutputString = NULL;

    } else {
        *ppszOutputString = pszFound;
    }
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
