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
 *        strings.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Utilities
 *
 *        Strings
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */
#include "includes.h"

// This is for Solaris, where isspace() indexes into an array.
static
int
SMBIsSpace(
    CHAR Character
    )
{
    return isspace((int)Character);
}

DWORD
SMBAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    )
{
    DWORD dwError = 0;
    va_list args;
    
    va_start(args, pszFormat);
    
    dwError = SMBAllocateStringPrintfV(
                      ppszOutputString,
                      pszFormat,
                      args);
    
    va_end(args);

    return dwError;
}

DWORD
SMBAllocateStringPrintfV(
    PSTR*   ppszOutputString,
    PCSTR   pszFormat,
    va_list args
    )
{
    DWORD dwError = 0;
    PSTR  pszSmallBuffer = NULL;
    DWORD dwBufsize = 0;
    INT   requiredLength = 0;
    INT   newRequiredLength = 0;
    PSTR  pszOutputString = NULL;
    va_list args2;

    va_copy(args2, args);

    dwBufsize = 4;
    /* Use a small buffer in case libc does not like NULL */
    do
    {
        dwError = LwIoAllocateMemory(
                        dwBufsize, 
                        (PVOID*) &pszSmallBuffer);
        BAIL_ON_LWIO_ERROR(dwError);
        
        requiredLength = vsnprintf(
                              pszSmallBuffer,
                              dwBufsize,
                              pszFormat,
                              args);
        if (requiredLength < 0)
        {
            dwBufsize *= 2;
        }
        LwIoFreeMemory(pszSmallBuffer);
        pszSmallBuffer = NULL;
        
    } while (requiredLength < 0);

    if (requiredLength >= (UINT32_MAX - 1))
    {
        dwError = ENOMEM;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    dwError = LwIoAllocateMemory(
                    requiredLength + 2,
                    (PVOID*)&pszOutputString);
    BAIL_ON_LWIO_ERROR(dwError);

    newRequiredLength = vsnprintf(
                            pszOutputString,
                            requiredLength + 1,
                            pszFormat,
                            args2);
    if (newRequiredLength < 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }
    else if (newRequiredLength > requiredLength)
    {
        /* unexpected, ideally should log something, or use better error code */
        dwError = ENOMEM;
        BAIL_ON_LWIO_ERROR(dwError);
    }
    else if (newRequiredLength < requiredLength)
    {
        /* unexpected, ideally should log something -- do not need an error, though */
    }
    
    *ppszOutputString = pszOutputString;

cleanup:

    va_end(args2);
    
    return dwError;
    
error:

    LWIO_SAFE_FREE_MEMORY(pszOutputString);
    
    *ppszOutputString = NULL;

    goto cleanup;
}

void
SMBStripLeadingWhitespace(
    PSTR pszString
)
{
    PSTR pszNew = pszString;
    PSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0' || !SMBIsSpace(*pszString)) {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0' && SMBIsSpace(*pszTmp)) {
        pszTmp++;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        *pszNew++ = *pszTmp++;
    }
    *pszNew = '\0';
}


DWORD
SMBStrIsAllSpace(
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
        if (!SMBIsSpace(*pszTmp))
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
SMBStripTrailingWhitespace(
    PSTR pszString
)
{
    PSTR pszLastSpace = NULL;
    PSTR pszTmp = pszString;

    if (IsNullOrEmptyString(pszString))
        return;

    while (pszTmp != NULL && *pszTmp != '\0') {
        pszLastSpace = (SMBIsSpace(*pszTmp) ? (pszLastSpace ? pszLastSpace : pszTmp) : NULL);
        pszTmp++;
    }

    if (pszLastSpace != NULL) {
        *pszLastSpace = '\0';
    }
}

void
SMBStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
)
{
    if (IsNullOrEmptyString(pszString))
        return;

    if (bLeading) {
        SMBStripLeadingWhitespace(pszString);
    }

    if (bTrailing) {
        SMBStripTrailingWhitespace(pszString);
    }
}

void
SMBStrToUpper(
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
SMBStrnToUpper(
    PSTR  pszString,
    DWORD dwLen
    )
{
    if (!IsNullOrEmptyString(pszString))
    {
       DWORD iCh = 0;

       while (*pszString != '\0' && (iCh++ < dwLen)) {
           *pszString = toupper((int)*pszString);
           pszString++;
       }
    }
}

void
SMBStrToLower(
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

void
SMBStrnToLower(
    PSTR  pszString,
    DWORD dwLen
    )
{

    if (!IsNullOrEmptyString(pszString))
    {
        DWORD iCh = 0;
    
        while (*pszString != '\0' && (iCh++ < dwLen)) {
            *pszString = tolower((int)*pszString);
            pszString++;
        }
    }
}

DWORD
SMBEscapeString(
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
        BAIL_ON_LWIO_ERROR(dwError);
    }

    while(pszTmp && *pszTmp)
    {
        if (*pszTmp=='\'') {
            nQuotes++;
        }
        pszTmp++;
    }

    if (!nQuotes) {
        dwError = SMBAllocateString(pszOrig, &pszNew);
        BAIL_ON_LWIO_ERROR(dwError);
    } else {
        /*
         * We are going to escape each single quote and enclose it in two other
         * single-quotes
         */
        dwError = LwIoAllocateMemory(
                      strlen(pszOrig)+3*nQuotes+1,
                      (PVOID*)&pszNew );
        BAIL_ON_LWIO_ERROR(dwError);

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

    LWIO_SAFE_FREE_MEMORY(pszNew);

    *ppszEscapedString = NULL;

    goto cleanup;
}

DWORD
SMBStrndup(
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
        BAIL_ON_LWIO_ERROR(dwError);
    }

    copylen = strlen(pszInputString);
    if (copylen > size)
        copylen = size;

    dwError = LwIoAllocateMemory(copylen+1, (PVOID *)&pszOutputString);
    BAIL_ON_LWIO_ERROR(dwError);

    memcpy(pszOutputString, pszInputString, copylen);
    pszOutputString[copylen] = 0;

    *ppszOutputString = pszOutputString;
    
cleanup:
    return dwError;
    
error:
    LWIO_SAFE_FREE_STRING(pszOutputString);
    goto cleanup;
}

VOID
SMBStrCharReplace(
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

DWORD
SMBStrDupOrNull(
    PCSTR pszInputString, 
    PSTR *ppszOutputString
    )
{
    if (pszInputString == NULL)
    {
        *ppszOutputString = NULL;
        return LWIO_ERROR_SUCCESS;
    }
    else
    {
        return SMBAllocateString(pszInputString, ppszOutputString);
    }
}


PCSTR
SMBEmptyStrForNull(
    PCSTR pszInputString
    )
{
    if (pszInputString == NULL)
        return "";
    else
        return pszInputString;
}


void
SMBStrChr(
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
