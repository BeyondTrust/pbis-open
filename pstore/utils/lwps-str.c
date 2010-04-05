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
 *        lwps-str.c
 *
 * Abstract:
 *
 *        Likewise Password Storage (LWPS)
 *
 *        String Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#include "lwps-utils.h"
#include "wc16str.h"
#include "lwps-validate.h"

#if !defined(HAVE_STRTOLL)

long long int
strtoll(
    const char* nptr,
    char**      endptr,
    int         base
    )
{
#if defined(HAVE___STRTOLL)
    return __strtoll(nptr, endptr, base);
#else
#error strtoll support is not available
#endif
}

#endif /* defined(HAVE_STRTOLL) */


DWORD
LwpsAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    )
{
    DWORD dwError = 0;
    va_list args;
    
    va_start(args, pszFormat);
    
    dwError = LwpsAllocateStringPrintfV(
                      ppszOutputString,
                      pszFormat,
                      args);
    
    va_end(args);

    return dwError;
}

DWORD
LwpsAllocateStringPrintfV(
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
        dwError = LwpsAllocateMemory(
                        dwBufsize, 
                        (PVOID*) &pszSmallBuffer);
        BAIL_ON_LWPS_ERROR(dwError);
        
        requiredLength = vsnprintf(
                              pszSmallBuffer,
                              dwBufsize,
                              pszFormat,
                              args);
        if (requiredLength < 0)
        {
            dwBufsize *= 2;
        }
        LwpsFreeMemory(pszSmallBuffer);
        pszSmallBuffer = NULL;
        
    } while (requiredLength < 0);

    if (requiredLength >= (UINT32_MAX - 1))
    {
        dwError = ENOMEM;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    dwError = LwpsAllocateMemory(
                    requiredLength + 2,
                    (PVOID*)&pszOutputString);
    BAIL_ON_LWPS_ERROR(dwError);

    dwNewRequiredLength = vsnprintf(
                            pszOutputString,
                            requiredLength + 1,
                            pszFormat,
                            args2);
    if (dwNewRequiredLength < 0)
    {
        dwError = errno;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    else if (dwNewRequiredLength > requiredLength)
    {
        /* unexpected, ideally should log something, or use better error code */
        dwError = ENOMEM;
        BAIL_ON_LWPS_ERROR(dwError);
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

    LWPS_SAFE_FREE_MEMORY(pszOutputString);
    
    *ppszOutputString = NULL;

    goto cleanup;
}

void
LwpsStripLeadingWhitespace(
    PSTR pszString
)
{
    PSTR pszNew = pszString;
    PSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0' || !isspace((int)*pszString)) {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0' && isspace((int)*pszTmp)) {
        pszTmp++;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        *pszNew++ = *pszTmp++;
    }
    *pszNew = '\0';
}


DWORD
LwpsStrIsAllSpace(
    PCSTR pszString,
    PBOOLEAN pbIsAllSpace
    )
{
    DWORD dwError = 0;
    PCSTR pszTmp = NULL;
    BOOLEAN bIsAllSpace = TRUE;
    
    BAIL_ON_INVALID_POINTER(pszString);
    
    for (pszTmp = pszString; *pszTmp; pszTmp++)
    {
        if (!isspace((int)*pszTmp))
        {
            bIsAllSpace = FALSE;
            break;
        }
    }
    
    
    *pbIsAllSpace = bIsAllSpace;
    
cleanup:

    return dwError;

error:

    *pbIsAllSpace = FALSE;
    goto cleanup;
}

void
LwpsStripTrailingWhitespace(
    PSTR pszString
)
{
    PSTR pszLastSpace = NULL;
    PSTR pszTmp = pszString;

    if (IsNullOrEmptyString(pszString))
        return;

    while (pszTmp != NULL && *pszTmp != '\0') {
        pszLastSpace = (isspace((int)*pszTmp) ? (pszLastSpace ? pszLastSpace : pszTmp) : NULL);
        pszTmp++;
    }

    if (pszLastSpace != NULL) {
        *pszLastSpace = '\0';
    }
}

void
LwpsStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
)
{
    if (IsNullOrEmptyString(pszString))
        return;

    if (bLeading) {
        LwpsStripLeadingWhitespace(pszString);
    }

    if (bTrailing) {
        LwpsStripTrailingWhitespace(pszString);
    }
}

void
LwpsStrToUpper(
    PSTR pszString
)
{
    if (IsNullOrEmptyString(pszString))
        return;

    while (*pszString != '\0') {
        *pszString = toupper((int)*pszString);
        pszString++;
    }
}

void
LwpsStrToLower(
    PSTR pszString
)
{
    if (IsNullOrEmptyString(pszString))
        return;

    while (*pszString != '\0') {
        *pszString = tolower((int)*pszString);
        pszString++;
    }
}

DWORD
LwpsEscapeString(
    PSTR pszOrig,
    PSTR * ppszEscapedString
)
{
    DWORD dwError = 0;
    int nQuotes = 0;
    PSTR pszTmp = pszOrig;
    PSTR pszNew = NULL;
    PSTR pszNewTmp = NULL;

    if ( !ppszEscapedString || !pszOrig ) {
        dwError = EINVAL;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    while(pszTmp && *pszTmp)
    {
        if (*pszTmp=='\'') {
            nQuotes++;
        }
        pszTmp++;
    }

    if (!nQuotes) {
        dwError = LwpsAllocateString(pszOrig, &pszNew);
        BAIL_ON_LWPS_ERROR(dwError);
    } else {
        /*
         * We are going to escape each single quote and enclose it in two other
         * single-quotes
         */
        dwError = LwpsAllocateMemory( strlen(pszOrig)+3*nQuotes+1, (PVOID*)&pszNew );
        BAIL_ON_LWPS_ERROR(dwError);

        pszTmp = pszOrig;
        pszNewTmp = pszNew;

        while(pszTmp && *pszTmp)
        {
            if (*pszTmp=='\'') {
                *pszNewTmp++='\'';
                *pszNewTmp++='\\';
                *pszNewTmp++='\'';
                *pszNewTmp++='\'';
                pszTmp++;
            }
            else {
                *pszNewTmp++ = *pszTmp++;
            }
        }
        *pszNewTmp = '\0';
    }

    *ppszEscapedString = pszNew;

cleanup:

    return dwError;

error:

    LWPS_SAFE_FREE_MEMORY(pszNew);

    *ppszEscapedString = NULL;

    goto cleanup;
}

DWORD
LwpsStrndup(
    PCSTR pszInputString,
    size_t size,
    PSTR * ppszOutputString
)
{
    DWORD dwError = 0;
    size_t copylen = 0;
    PSTR pszOutputString = NULL;

    if (!pszInputString || !ppszOutputString){
        dwError = EINVAL;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    copylen = strlen(pszInputString);
    if (copylen > size)
        copylen = size;

    dwError = LwpsAllocateMemory(copylen+1, (PVOID *)&pszOutputString);
    BAIL_ON_LWPS_ERROR(dwError);

    memcpy(pszOutputString, pszInputString, copylen);
    pszOutputString[copylen] = 0;

error:

    *ppszOutputString = pszOutputString;

    return(dwError);
}

DWORD
LwpsMbsToWc16s(
    PCSTR pszInput,
    PWSTR* ppszOutput
    )
{
    DWORD dwError = 0;
    PWSTR pszOutput = NULL;
    
    if (!pszInput) {
        dwError = LWPS_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    pszOutput = ambstowc16s(pszInput);
    if (!pszOutput) {
        dwError = LWPS_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    *ppszOutput = pszOutput;
    
cleanup:

    return dwError;
    
error:
    
    *ppszOutput = NULL;

    goto cleanup;
}

DWORD
LwpsWc16sToMbs(
    PCWSTR pwszInput,
    PSTR*  ppszOutput
    )
{
    DWORD dwError = 0;
    PSTR pszOutput = NULL;
    
    if (!pwszInput) {
        dwError = LWPS_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    pszOutput = awc16stombs(pwszInput);
    if (!pszOutput) {
        dwError = LWPS_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    *ppszOutput = pszOutput;
    
cleanup:

    return dwError;
    
error:

    *ppszOutput = NULL;

    goto cleanup;
}

VOID
LwpsStrCharReplace(
    PSTR pszStr, 
    CHAR oldCh,
    CHAR newCh)
{
    if (oldCh != newCh)
    {
        while (pszStr && *pszStr)
        {
            if (*pszStr == oldCh){
                *pszStr = newCh;
            }
            pszStr++;
        }
    }
}

