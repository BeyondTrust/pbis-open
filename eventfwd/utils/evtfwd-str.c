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
 *        evtfwd-str.c
 *
 * Abstract:
 *
 *        Event forwarder from eventlogd to collector service
 *
 *        String Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

#if !HAVE_DECL_ISBLANK
int isblank(int c)
{
    return c == '\t' || c == ' ';
}
#endif

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

#if !defined(HAVE_STRTOULL)

unsigned long long int
strtoull(
    const char* nptr,
    char**      endptr,
    int         base
    )
{
#if defined(HAVE___STRTOULL)
    return __strtoull(nptr, endptr, base);
#else
#error strtoull support is not available
#endif
}

#endif /* defined(HAVE_STRTOULL) */

void
EfdStripLeadingWhitespace(
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
EfdStrIsAllSpace(
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
EfdStripTrailingWhitespace(
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
EfdStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
)
{
    if (IsNullOrEmptyString(pszString))
        return;

    if (bLeading) {
        EfdStripLeadingWhitespace(pszString);
    }

    if (bTrailing) {
        EfdStripTrailingWhitespace(pszString);
    }
}

void
EfdStrToUpper(
    PSTR pszString
)
{
    if (IsNullOrEmptyString(pszString))
        return;

    while (*pszString != '\0') {
        *pszString = toupper(*pszString);
        pszString++;
    }
}

void
EfdStrToLower(
    PSTR pszString
)
{
    if (IsNullOrEmptyString(pszString))
        return;

    while (*pszString != '\0') {
        *pszString = tolower(*pszString);
        pszString++;
    }
}

DWORD
EfdStrndup(
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
        BAIL_ON_EFD_ERROR(dwError);
    }

    copylen = strlen(pszInputString);
    if (copylen > size)
        copylen = size;

    dwError = RTL_ALLOCATE(
                    &pszOutputString,
                    CHAR,
                    copylen+1);
    BAIL_ON_EFD_ERROR(dwError);

    memcpy(pszOutputString, pszInputString, copylen);
    pszOutputString[copylen] = 0;

    *ppszOutputString = pszOutputString;
    
cleanup:
    return dwError;
    
error:
    RtlCStringFree(&pszOutputString);
    goto cleanup;
}

DWORD
EfdStrDupOrNull(
    PCSTR pszInputString, 
    PSTR *ppszOutputString
    )
{
    if (pszInputString == NULL)
    {
        *ppszOutputString = NULL;
        return EFD_ERROR_SUCCESS;
    }
    else
    {
        return RtlCStringDuplicate(ppszOutputString, pszInputString);
    }
}


DWORD
EfdHexStrToByteArray(
    PCSTR   pszHexString,
    UCHAR** ppucByteArray,
    DWORD*  pdwByteArrayLength
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwHexChars = strlen(pszHexString);
    UCHAR* pucByteArray = NULL;
    DWORD dwByteArrayLength = dwHexChars / 2;
    
    if ((dwHexChars & 0x00000001) != 0) 
    {
       dwError = EFD_ERROR_INVALID_PARAMETER;
       BAIL_ON_EFD_ERROR(dwError);
    }
    
    dwError = RTL_ALLOCATE(
                  &pucByteArray,
                  UCHAR,
                  sizeof(UCHAR)*(dwByteArrayLength));
    BAIL_ON_EFD_ERROR(dwError);
    
    for (i = 0; i < dwByteArrayLength; i++)
    {
        CHAR hexHi = pszHexString[2*i];
        CHAR hexLow = pszHexString[2*i + 1];
        
        UCHAR ucHi = 0;
        UCHAR ucLow = 0;
      
        dwError = EfdHexCharToByte(hexHi, &ucHi);
        BAIL_ON_EFD_ERROR(dwError);
      
        dwError = EfdHexCharToByte(hexLow, &ucLow);
        BAIL_ON_EFD_ERROR(dwError);
      
        pucByteArray[i] = (ucHi * 16) + ucLow;
    }
    
    *ppucByteArray = pucByteArray;
    *pdwByteArrayLength = dwByteArrayLength;
    
cleanup:
    
    return dwError;

error:
    
    EFD_SAFE_FREE_MEMORY(pucByteArray);
    *ppucByteArray = NULL;
    *pdwByteArrayLength = 0;
    
    goto cleanup;
}

DWORD
EfdByteArrayToHexStr(
    UCHAR* pucByteArray,
    DWORD dwByteArrayLength,
    PSTR* ppszHexString
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR pszHexString = NULL;
    
    dwError = RTL_ALLOCATE(
                &pszHexString,
                CHAR,
                (dwByteArrayLength*2 + 1) * sizeof(CHAR));
    BAIL_ON_EFD_ERROR(dwError);
    
    for (i = 0; i < dwByteArrayLength; i++)
    {
        sprintf((char*)pszHexString+(2*i), "%.2X", pucByteArray[i]);
    }
    
    *ppszHexString = pszHexString;
    
cleanup:

    return dwError;

error:

    *ppszHexString = NULL;
    goto cleanup;
}


DWORD
EfdHexCharToByte(
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
       dwError = EFD_ERROR_INVALID_PARAMETER;
       BAIL_ON_EFD_ERROR(dwError);
    }
    
    *pucByte = ucByte;
    
cleanup:

    return dwError;

error:
    
    *pucByte = 0;

    goto cleanup;
}

