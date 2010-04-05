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


#include "cltr-base.h"

LW_LONG
CltrWC16StringCompareNum(
    LW_IN LW_PCWSTR pString1,
    LW_IN LW_PCWSTR pString2,
    size_t maxCompare,
    LW_IN LW_BOOLEAN bIsCaseSensitive
    )
{
    size_t pos = 0;
    wchar16_t c1[2] = { 0, 0 };
    wchar16_t c2[2] = { 0, 0 };
    size_t remaining = 0;

    while(maxCompare)
    {
        c1[0] = pString1[pos];
        c2[0] = pString2[pos];
        if (!bIsCaseSensitive)
        {
            wc16supper(c1);
            wc16supper(c2);
        }

        if (c1[0] != c2[0])
        {
            if (c1[0] == 0)
            {
                // string1 is shorter
                remaining = wc16slen(pString2 + pos);
                return -(LW_LONG)LW_MIN(remaining, LW_MAXLONG);
            }
            if (c2[0] == 0)
            {
                // string2 is shorter
                remaining = wc16slen(pString1 + pos);
                return (LW_LONG)LW_MIN(remaining, LW_MAXLONG);
            }
            // a character is different
            return c1[0] - c2[0];
        }

        if (c1[0] == 0)
        {
            // This is the end of both strings
            return 0;
        }

        pos++;
        maxCompare--;
    }
    // The strings were equal up to the max comparison length
    return 0;
}

LW_LONG
CltrWC16StringCompareWCNString(
    LW_IN LW_PCWSTR pString1,
    LW_IN const wchar_t* pString2,
    LW_IN LW_BOOLEAN bIsCaseSensitive
    )
{
#ifdef WCHAR16_IS_WCHAR
    if (bIsCaseSensitive)
    { 
        return wcscmp(pString1, pString2);
    }
    else
    {
        return wcsicmp(pString1, pString2);
    }
#else
    WCHAR string2Buffer[256];
    size_t comparisonOffset = 0;
    size_t converted = 0;
    LW_LONG result = 0;

    while (1)
    {
        converted = wcstowc16s(
                        string2Buffer,
                        pString2 + comparisonOffset,
                        sizeof(string2Buffer)/sizeof(string2Buffer[0]) - 1);
        if (converted == -1)
        {
            // The second string has an invalid encoding. We'll define invalid
            // strings as always shorter than valid strings.
            return -1;
        }
        string2Buffer[converted] = 0;

        if (converted == 0)
        {
            // This is the end of string2.
            // If string1 is longer, return a positive value, otherwise return 0.
            converted = wc16slen(pString1 + comparisonOffset);
            return (LW_LONG)LW_MIN(LW_MAXLONG, converted);
        }

        result = CltrWC16StringCompareNum(
                        pString1 + comparisonOffset,
                        string2Buffer,
                        converted,
                        bIsCaseSensitive);
        if (result != 0)
        {
            return result;
        }
        comparisonOffset += converted;
    }
#endif
}

DWORD
CltrStrndup(
    PCWSTR pszInputString,
    size_t size,
    PWSTR * ppszOutputString
)
{
    DWORD dwError = 0;
    size_t copylen = 0;
    PWSTR pszOutputString = NULL;

    if (!pszInputString || !ppszOutputString){
        dwError = EINVAL;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    copylen = wc16slen(pszInputString);
    if (copylen > size)
        copylen = size;

    dwError = CltrAllocateMemory((DWORD)copylen+1, (PVOID *)&pszOutputString);
    BAIL_ON_CLTR_ERROR(dwError);

    memcpy(pszOutputString, pszInputString, copylen);
    pszOutputString[copylen] = 0;

error:

    *ppszOutputString = pszOutputString;

    return(dwError);
}

/* return a wcstring description of the TableCategory type */
PCWSTR
CltrTableCategoryToStr(
    DWORD tableCategory
    )
{

    switch(tableCategory) {
    case 0:
        return ((PCWSTR)"Application");
    case 1:
        return ((PCWSTR)"WebBrowser");
    case 2:
        return ((PCWSTR)"Security");
    case 3:
        return ((PCWSTR)"System");
    default:
        return ((PCWSTR)"Unknown");
    }

    return ((PCWSTR)"");

}



BOOLEAN
CltrIsWhiteSpace(
    WCHAR c
    )
{
    switch(c) {
    case '\n':
    case '\r':
    case '\t':
    case ' ':
        return TRUE;
    default:
        return FALSE;
    }
}


/* modify PWSTR in-place to conver sequences of whitespace WCHARacters into single spaces (0x20) */
DWORD
CltrCompressWhitespace(
    PWSTR pszString
    )
{
    DWORD pszStringLen = 0;
    DWORD i = 0;
    DWORD j = 0;
    BOOLEAN whitespace = FALSE;

    if (pszString == NULL) {
        return -1;
    }

    pszStringLen = (DWORD)wc16slen(pszString);

    for (i = 0; i < pszStringLen; i++) {

    if (CltrIsWhiteSpace(pszString[i])) {
        if (!whitespace) {
        whitespace = TRUE;
        pszString[j++] = ' ';
        }
    }
    else {
        whitespace = FALSE;
        pszString[j++] = pszString[i];
    }

    }
    pszString[j] = 0;
    return 0;
}

/* convert a 16-bit wcstring to an 8-bit wcstring, allocating new memory in the process */
DWORD
CltrLWCHARToLpStr(
    PCWSTR pszwString,
    PWSTR* ppszString
    )
{

    DWORD dwError = 0;
    DWORD cChar = 0;
    PWSTR pszString = NULL;

    DWORD i = 0;

    if (ppszString == NULL || pszwString == NULL)
    {
        return -1;
    }

    cChar  = (DWORD)wc16slen(pszwString);

    dwError = CltrAllocateMemory((cChar+1) * 2, (PVOID*)ppszString);
    BAIL_ON_CLTR_ERROR(dwError);

    pszString = *ppszString;

    for (i = 0; i < cChar; i++) {
        pszString[i] = (WCHAR)((WORD)pszwString[i]);
    }

    pszString[cChar] = 0;

 error:

    return dwError;



}

void
CltrStripLeadingWhitespace(
    PWSTR pszString
    )
{
    PWSTR pszNew = pszString;
    PWSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0' || !isspace(*pszString)) {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0' && isspace(*pszTmp)) {
        pszTmp++;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        *pszNew++ = *pszTmp++;
    }
    *pszNew = '\0';
}

void
CltrStripTrailingWhitespace(
    PWSTR pszString
    )
{
    PWSTR pszLastSpace = NULL;
    PWSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0') {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        pszLastSpace = (isspace(*pszTmp) ? (pszLastSpace ? pszLastSpace : pszTmp) : NULL);
        pszTmp++;
    }

    if (pszLastSpace != NULL) {
        *pszLastSpace = '\0';
    }
}

void
CltrStripWhitespace(
    PWSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    )
{
    if (pszString == NULL || *pszString == '\0') {
        return;
    }

    if (bLeading) {
        CltrStripLeadingWhitespace(pszString);
    }

    if (bTrailing) {
        CltrStripTrailingWhitespace(pszString);
    }
}

void
CltrStrToUpper(
    PWSTR pszString
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
CltrStrToLower(
    PWSTR pszString
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
CltrEscapeString(
    PWSTR pszOrig,
    PWSTR * ppszEscapedString
    )
{
    DWORD dwError = 0;
    int nQuotes = 0;
    PWSTR pszTmp = pszOrig;
    PWSTR pszNew = NULL;
    PWSTR pszNewTmp = NULL;

    if ( !ppszEscapedString || !pszOrig ) {
         dwError = EINVAL;
         BAIL_ON_CLTR_ERROR(dwError);
    }

    while (pszTmp && *pszTmp)
    {
         if (*pszTmp=='\'') {
             nQuotes++;
         }
         pszTmp++;
    }

    if (!nQuotes) {
         dwError = CltrAllocateString(pszOrig, &pszNew);
         BAIL_ON_CLTR_ERROR(dwError);
    } else {
         /*
            * We are going to escape each single quote and enclose it in two other
            * single-quotes
            */
         dwError = CltrAllocateMemory( (DWORD)wc16slen(pszOrig)+3*nQuotes+1, (PVOID*)&pszNew );
         BAIL_ON_CLTR_ERROR(dwError);

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
        CltrFreeMemory(pszNew);
    }

    return dwError;
}

