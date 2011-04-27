/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 *        lwstr.c
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi) String Utilities
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
LwIsSpace(
    CHAR Character
    )
{
    return isspace((int)Character);
}

DWORD
LwAllocateString(
    PCSTR  pszInputString,
    PSTR* ppszOutputString
    )
{
    DWORD dwError = 0;
    DWORD dwLen = 0;
    PSTR  pszOutputString = NULL;

    if (!pszInputString) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwLen = strlen(pszInputString);

    dwError = LwAllocateMemory(dwLen+1, OUT_PPVOID(&pszOutputString));
    BAIL_ON_LW_ERROR(dwError);

    if (dwLen) {
       memcpy(pszOutputString, pszInputString, dwLen);
    }

    *ppszOutputString = pszOutputString;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszOutputString);

    *ppszOutputString = NULL;

    goto cleanup;
}

void
LwFreeString(
    PSTR pszString
    )
{
    LwFreeMemory(pszString);
}

DWORD
LwDuplicateStringArray(
    PSTR** pppNewStringArray,
    PDWORD pdwNewCount,
    PSTR* ppStringArray,
    DWORD dwCount
    )
{
    DWORD dwError = 0;
    PSTR* ppNewStringArray = NULL;
    DWORD dwNewCount = 0;

    if (dwCount)
    {
        DWORD i = 0;

        dwError = LwAllocateMemory(
                        dwCount * sizeof(*ppNewStringArray),
                        OUT_PPVOID(&ppNewStringArray));
        BAIL_ON_LW_ERROR(dwError);

        dwNewCount = dwCount;

        for (i = 0; i < dwCount; i++)
        {
            dwError = LwAllocateString(
                            ppStringArray[i],
                            &ppNewStringArray[i]);
            BAIL_ON_LW_ERROR(dwError);
        }
    }

cleanup:

    *pppNewStringArray = ppNewStringArray;
    if (pdwNewCount)
    {
        *pdwNewCount = dwNewCount;
    }

    return dwError;

error:

    LwFreeStringArray(ppNewStringArray, dwNewCount);
    ppNewStringArray = NULL;
    dwNewCount = 0;

    goto cleanup;
}

void
LwFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    )
{
    DWORD i;

    if ( ppStringArray )
    {
        for(i = 0; i < dwCount; i++)
        {
            if (ppStringArray[i])
            {
                LwFreeMemory(ppStringArray[i]);
            }
        }

        LwFreeMemory(ppStringArray);
    }

    return;
}

DWORD
LwAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    )
{
    DWORD dwError = 0;
    va_list args;

    va_start(args, pszFormat);

    dwError = LwAllocateStringPrintfV(
                      ppszOutputString,
                      pszFormat,
                      args);

    va_end(args);

    return dwError;
}

DWORD
LwAllocateStringPrintfV(
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
        dwError = LwAllocateMemory(
                        dwBufsize,
                        OUT_PPVOID(&pszSmallBuffer));
        BAIL_ON_LW_ERROR(dwError);

        requiredLength = vsnprintf(
                              pszSmallBuffer,
                              dwBufsize,
                              pszFormat,
                              args);
        if (requiredLength < 0)
        {
            dwBufsize *= 2;
        }
        LwFreeMemory(pszSmallBuffer);
        pszSmallBuffer = NULL;

    } while (requiredLength < 0);

    if (requiredLength >= (UINT32_MAX - 1))
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                    requiredLength + 2,
                    OUT_PPVOID(&pszOutputString));
    BAIL_ON_LW_ERROR(dwError);

    dwNewRequiredLength = vsnprintf(
                            pszOutputString,
                            requiredLength + 1,
                            pszFormat,
                            args2);
    if (dwNewRequiredLength < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }
    else if (dwNewRequiredLength > requiredLength)
    {
        /* unexpected, ideally should log something, or use better error code */
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LW_ERROR(dwError);
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

    LW_SAFE_FREE_MEMORY(pszOutputString);

    *ppszOutputString = NULL;

    goto cleanup;
}

void
LwFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    )
{
    PSTR* ppTmp = ppStringArray;

    while (ppTmp && *ppTmp) {

          LwFreeString(*ppTmp);

          ppTmp++;
    }

    LwFreeMemory(ppStringArray);
}

void
LwStripLeadingWhitespace(
    PSTR pszString
)
{
    PSTR pszNew = pszString;
    PSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0' || !LwIsSpace(*pszString)) {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0' && LwIsSpace(*pszTmp)) {
        pszTmp++;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        *pszNew++ = *pszTmp++;
    }
    *pszNew = '\0';
}


DWORD
LwStrIsAllSpace(
    PCSTR pszString,
    PBOOLEAN pbIsAllSpace
    )
{
    DWORD dwError = 0;
    PCSTR pszTmp = NULL;
    BOOLEAN bIsAllSpace = TRUE;

    LW_BAIL_ON_INVALID_POINTER(pszString);

    for (pszTmp = pszString; *pszTmp; pszTmp++)
    {
        if (!LwIsSpace(*pszTmp))
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
LwStripTrailingWhitespace(
    PSTR pszString
)
{
    PSTR pszLastSpace = NULL;
    PSTR pszTmp = pszString;

    if (LW_IS_NULL_OR_EMPTY_STR(pszString))
        return;

    while (pszTmp != NULL && *pszTmp != '\0') {
        pszLastSpace = (LwIsSpace(*pszTmp) ? (pszLastSpace ? pszLastSpace : pszTmp) : NULL);
        pszTmp++;
    }

    if (pszLastSpace != NULL) {
        *pszLastSpace = '\0';
    }
}

void
LwStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
)
{
    if (LW_IS_NULL_OR_EMPTY_STR(pszString))
        return;

    if (bLeading) {
        LwStripLeadingWhitespace(pszString);
    }

    if (bTrailing) {
        LwStripTrailingWhitespace(pszString);
    }
}

void
LwStrToUpper(
    PSTR pszString
)
{
    if (LW_IS_NULL_OR_EMPTY_STR(pszString))
        return;

    while (*pszString != '\0') {
        *pszString = toupper((int)*pszString);
        pszString++;
    }
}

void
LwStrnToUpper(
    PSTR  pszString,
    DWORD dwLen
    )
{
    if (!LW_IS_NULL_OR_EMPTY_STR(pszString))
    {
       DWORD iCh = 0;

       while ((iCh++ < dwLen) && *pszString != '\0')
       {
           *pszString = toupper((int)*pszString);
           pszString++;
       }
    }
}

void
LwStrToLower(
    PSTR pszString
    )
{
    if (LW_IS_NULL_OR_EMPTY_STR(pszString))
        return;

    while (*pszString != '\0') {
        *pszString = tolower((int)*pszString);
        pszString++;
    }
}

void
LwStrnToLower(
    PSTR  pszString,
    DWORD dwLen
    )
{

    if (!LW_IS_NULL_OR_EMPTY_STR(pszString))
    {
        DWORD iCh = 0;

        while ((iCh++ < dwLen) && *pszString != '\0')
        {
            *pszString = tolower((int)*pszString);
            pszString++;
        }
    }
}

DWORD
LwEscapeString(
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
        BAIL_ON_LW_ERROR(dwError);
    }

    while(pszTmp && *pszTmp)
    {
        if (*pszTmp=='\'') {
            nQuotes++;
        }
        pszTmp++;
    }

    if (!nQuotes) {
        dwError = LwAllocateString(pszOrig, &pszNew);
        BAIL_ON_LW_ERROR(dwError);
    } else {
        /*
         * We are going to escape each single quote and enclose it in two other
         * single-quotes
         */
        dwError = LwAllocateMemory( strlen(pszOrig)+3*nQuotes+1, OUT_PPVOID(&pszNew));
        BAIL_ON_LW_ERROR(dwError);

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

    LW_SAFE_FREE_MEMORY(pszNew);

    if (ppszEscapedString)
    {
        *ppszEscapedString = NULL;
    }

    goto cleanup;
}

DWORD
LwStrndup(
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
        BAIL_ON_LW_ERROR(dwError);
    }

    for (copylen = 0; copylen < size && pszInputString[copylen]; copylen++);

    dwError = LwAllocateMemory(copylen+1, OUT_PPVOID(&pszOutputString));
    BAIL_ON_LW_ERROR(dwError);

    memcpy(pszOutputString, pszInputString, copylen);
    pszOutputString[copylen] = 0;

    *ppszOutputString = pszOutputString;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszOutputString);
    goto cleanup;
}

VOID
LwStrCharReplace(
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
LwStrDupOrNull(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    )
{
    if (pszInputString == NULL)
    {
        *ppszOutputString = NULL;
        return LW_ERROR_SUCCESS;
    }
    else
    {
        return LwAllocateString(pszInputString, ppszOutputString);
    }
}


PCSTR
LwEmptyStrForNull(
    PCSTR pszInputString
    )
{
    if (pszInputString == NULL)
        return "";
    else
        return pszInputString;
}


void
LwStrChr(
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

void
LwStrStr(
    PCSTR  pszInputString,
    PCSTR  pszSubstr,
    PSTR  *ppszOutputString
    )
{
    PSTR pszFound = NULL;

    if (ppszOutputString == NULL) return;

    if (pszInputString == NULL)
    {
        *ppszOutputString = NULL;
        return;
    }

    pszFound = strstr(pszInputString, pszSubstr);
    if (pszFound == NULL)
    {
        *ppszOutputString = NULL;
    }
    else
    {
        *ppszOutputString = pszFound;
    }
}

DWORD
LwHexCharToByte(
    CHAR cHexChar,
    UCHAR* pucByte
    )
{
    DWORD dwError = 0;
    UCHAR ucByte = 0;

    if (cHexChar >= '0' && cHexChar <= '9')
    {
       ucByte = (UCHAR)(cHexChar - '0');
    }
    else if (cHexChar >= 'a' && cHexChar <= 'f')
    {
       ucByte = 10 + (UCHAR)(cHexChar - 'a');
    }
    else if (cHexChar >= 'A' && cHexChar <= 'F')
    {
       ucByte = 10 + (UCHAR)(cHexChar - 'A');
    }
    else
    {
       dwError = LW_ERROR_INVALID_PARAMETER;
       BAIL_ON_LW_ERROR(dwError);
    }

    *pucByte = ucByte;

cleanup:

    return dwError;

error:

    *pucByte = 0;

    goto cleanup;
}

DWORD
LwHexStrToByteArray(
    IN PCSTR pszHexString,
    IN OPTIONAL DWORD* pdwHexStringLength,
    OUT UCHAR** ppucByteArray,
    OUT DWORD*  pdwByteArrayLength
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwHexChars = 0;
    UCHAR* pucByteArray = NULL;
    DWORD dwByteArrayLength = 0;

    LW_BAIL_ON_INVALID_POINTER(pszHexString);

    if (pdwHexStringLength)
    {
        dwHexChars = *pdwHexStringLength;
    }
    else
    {
        dwHexChars = strlen(pszHexString);
    }
    dwByteArrayLength = dwHexChars / 2;

    if ((dwHexChars & 0x00000001) != 0)
    {
       dwError = LW_ERROR_INVALID_PARAMETER;
       BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                  sizeof(UCHAR)*(dwByteArrayLength),
                  OUT_PPVOID(&pucByteArray)
                  );
    BAIL_ON_LW_ERROR(dwError);

    for (i = 0; i < dwByteArrayLength; i++)
    {
        CHAR hexHi = pszHexString[2*i];
        CHAR hexLow = pszHexString[2*i + 1];

        UCHAR ucHi = 0;
        UCHAR ucLow = 0;

        dwError = LwHexCharToByte(hexHi, &ucHi);
        BAIL_ON_LW_ERROR(dwError);

        dwError = LwHexCharToByte(hexLow, &ucLow);
        BAIL_ON_LW_ERROR(dwError);

        pucByteArray[i] = (ucHi * 16) + ucLow;
    }

    *ppucByteArray = pucByteArray;
    *pdwByteArrayLength = dwByteArrayLength;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_MEMORY(pucByteArray);
    *ppucByteArray = NULL;
    *pdwByteArrayLength = 0;

    goto cleanup;
}

DWORD
LwByteArrayToHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR pszHexString = NULL;

    dwError = LwAllocateMemory(
                (dwByteArrayLength*2 + 1) * sizeof(CHAR),
                OUT_PPVOID(&pszHexString));
    BAIL_ON_LW_ERROR(dwError);

    for (i = 0; i < dwByteArrayLength; i++)
    {
        sprintf(pszHexString+(2*i), "%.2X", pucByteArray[i]);
    }

    *ppszHexString = pszHexString;

cleanup:

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszHexString);

    *ppszHexString = NULL;
    goto cleanup;
}

PSTR
LwCaselessStringSearch(
    PCSTR pszHaystack,
    PCSTR pszNeedle
    )
{
#ifdef HAVE_STRCASESTR
    return strcasestr(pszHaystack, pszNeedle);
#else
    PSTR pszPos = (PSTR)pszHaystack;
    size_t sNeedle = strlen(pszNeedle);

    while (*pszPos)
    {
        if (!strncasecmp(pszPos, pszNeedle, sNeedle))
        {
            return pszPos;
        }
        pszPos++;
    }
    return NULL;
#endif
}
