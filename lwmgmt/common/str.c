/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog String Utilities
 *
 */

#include "includes.h"

DWORD
LWMGMTStrndup(
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
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    copylen = strlen(pszInputString);
    if (copylen > size)
        copylen = size;

    dwError = LWMGMTAllocateMemory(copylen+1, (PVOID *)&pszOutputString);
    BAIL_ON_LWMGMT_ERROR(dwError);

    memcpy(pszOutputString, pszInputString, copylen);
    pszOutputString[copylen] = 0;

error:

    *ppszOutputString = pszOutputString;

    return(dwError);
}

/* return a string description of the TableCategory type */
PCSTR
TableCategoryToStr(
    DWORD tableCategory
    )
{

    switch(tableCategory) {
    case 0:
        return "Application";
    case 1:
        return "WebBrowser";
    case 2:
        return "Security";
    case 3:
        return "System";
    default:
        return "Unknown";
    }

    return "";

}



BOOLEAN
LWMGMTIsWhiteSpace(
    char c
    )
{
    switch(c) {
    case '\n':
    case '\r':
    case '\t':
    case ' ':
        return true;
    default:
        return false;
    }
}


/* modify PSTR in-place to conver sequences of whitespace characters into single spaces (0x20) */
DWORD
LWMGMTCompressWhitespace(
    PSTR pszString
    )
{
    DWORD pszStringLen = 0;
    DWORD i = 0;
    DWORD j = 0;
    BOOLEAN whitespace = false;

    if (pszString == NULL) {
        return -1;
    }

    pszStringLen = strlen(pszString);

    for (i = 0; i < pszStringLen; i++) {

    if (LWMGMTIsWhiteSpace(pszString[i])) {
        if (!whitespace) {
        whitespace = true;
        pszString[j++] = ' ';
        }
    }
    else {
        whitespace = false;
        pszString[j++] = pszString[i];
    }

    }
    pszString[j] = 0;
    return 0;
}

/* convert a 16-bit string to an 8-bit string, allocating new memory in the process */
DWORD
LWMGMTLpwStrToLpStr(
    PCWSTR pszwString,
    PSTR* ppszString
    )
{

    DWORD dwError = 0;
    DWORD pszwStringLen = 0;
    PSTR pszString = NULL;

    DWORD i = 0;

    if (ppszString == NULL || pszwString == NULL)
    {
        return -1;
    }

    pszwStringLen = wc16slen(pszwString);

    dwError = LWMGMTAllocateMemory(pszwStringLen+1, (PVOID*)ppszString);
    BAIL_ON_LWMGMT_ERROR(dwError);

    pszString = *ppszString;

    for (i = 0; i < pszwStringLen; i++) {
        pszString[i] = (char)((WORD)pszwString[i]);
    }

    pszString[pszwStringLen] = 0;

 error:

    return dwError;



}

void
LWMGMTStripLeadingWhitespace(
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

void
LWMGMTStripTrailingWhitespace(
    PSTR pszString
    )
{
    PSTR pszLastSpace = NULL;
    PSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0') {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        pszLastSpace = (isspace((int)*pszTmp) ? (pszLastSpace ? pszLastSpace : pszTmp) : NULL);
        pszTmp++;
    }

    if (pszLastSpace != NULL) {
        *pszLastSpace = '\0';
    }
}

void
LWMGMTStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    )
{
    if (pszString == NULL || *pszString == '\0') {
        return;
    }

    if (bLeading) {
        LWMGMTStripLeadingWhitespace(pszString);
    }

    if (bTrailing) {
        LWMGMTStripTrailingWhitespace(pszString);
    }
}

void
LWMGMTStrToUpper(
    PSTR pszString
    )
{

    if (pszString != NULL) {
         while (*pszString != '\0') {
             *pszString = toupper(*pszString);
             pszString++;
         }
    }
}

void
LWMGMTStrToLower(
    PSTR pszString
    )
{

    if (pszString != NULL) {
         while (*pszString != '\0') {
             *pszString = tolower(*pszString);
             pszString++;
         }
    }
}

DWORD
LWMGMTEscapeString(
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
         BAIL_ON_LWMGMT_ERROR(dwError);
    }

    while (pszTmp && *pszTmp)
    {
         if (*pszTmp=='\'') {
             nQuotes++;
         }
         pszTmp++;
    }

    if (!nQuotes) {
         dwError = LWMGMTAllocateString(pszOrig, &pszNew);
         BAIL_ON_LWMGMT_ERROR(dwError);
    } else {
         /*
            * We are going to escape each single quote and enclose it in two other
            * single-quotes
            */
         dwError = LWMGMTAllocateMemory( strlen(pszOrig)+3*nQuotes+1, (PVOID*)&pszNew );
         BAIL_ON_LWMGMT_ERROR(dwError);

         pszTmp = pszOrig;
         pszNewTmp = pszNew;

         while (pszTmp && *pszTmp)
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
    return dwError;

error:

    if (pszNew) {
        LWMGMTFreeMemory(pszNew);
    }

    return dwError;
}

DWORD
LWMGMTAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    )
{
    DWORD dwError = 0;
    va_list args;
    
    va_start(args, pszFormat);
    
    dwError = LWMGMTAllocateStringPrintfV(
                      ppszOutputString,
                      pszFormat,
                      args);
    
    va_end(args);

    return dwError;
}

DWORD
LWMGMTAllocateStringPrintfV(
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
        dwError = LWMGMTAllocateMemory(
                        dwBufsize, 
                        (PVOID*) &pszSmallBuffer);
        BAIL_ON_LWMGMT_ERROR(dwError);
        
        requiredLength = vsnprintf(
                              pszSmallBuffer,
                              dwBufsize,
                              pszFormat,
                              args);
        if (requiredLength < 0)
        {
            dwBufsize *= 2;
        }
        LWMGMTFreeMemory(pszSmallBuffer);
        pszSmallBuffer = NULL;
        
    } while (requiredLength < 0);

    if (requiredLength >= (UINT32_MAX - 1))
    {
        dwError = ENOMEM;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    dwError = LWMGMTAllocateMemory(
                    requiredLength + 2,
                    (PVOID*)&pszOutputString);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwNewRequiredLength = vsnprintf(
                            pszOutputString,
                            requiredLength + 1,
                            pszFormat,
                            args2);
    if (dwNewRequiredLength < 0)
    {
        dwError = errno;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    else if (dwNewRequiredLength > requiredLength)
    {
        /* unexpected, ideally should log something, or use better error code */
        dwError = ENOMEM;
        BAIL_ON_LWMGMT_ERROR(dwError);
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

    LWMGMT_SAFE_FREE_MEMORY(pszOutputString);
    
    *ppszOutputString = NULL;

    goto cleanup;
}
