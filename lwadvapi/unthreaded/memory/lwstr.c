/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwstr.c
 *
 * Abstract:
 *
 *        BeyondTrust Advanced API (lwadvapi) String Utilities
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

/*
 * Spare input string array is supported.
 * Use LwFreeStringArray() to release new string array.
 */
DWORD
LwDuplicateStringArray(
    PSTR** ppszaNewStringArray,
    PDWORD pdwNewCount,
    PSTR* pszaStringArray,
    DWORD dwCount
    )
{
    DWORD dwError = 0;
    PSTR* pszaNewStringArray = NULL;
    DWORD dwNewCount = 0;

    if (dwCount)
    {
        DWORD i = 0;

        dwError = LwAllocateStringArray(&pszaNewStringArray, dwCount);
        BAIL_ON_LW_ERROR(dwError);

        dwNewCount = dwCount;

        for (i = 0; i < dwCount; i++)
        {
            if (pszaStringArray[i])
            {
               dwError = LwAllocateString(
                               pszaStringArray[i],
                               &pszaNewStringArray[i]);
               BAIL_ON_LW_ERROR(dwError);
            }
        }
    }

cleanup:

    *ppszaNewStringArray = pszaNewStringArray;
    if (pdwNewCount)
    {
        *pdwNewCount = dwNewCount;
    }

    return dwError;

error:

    LwFreeStringArray(pszaNewStringArray, dwNewCount);
    pszaNewStringArray = NULL;
    dwNewCount = 0;

    goto cleanup;
}

/* Merge the given string arrays into ArrayOut.
 * Sparse input string arrays are supported.
 * Use LwFreeStringArray() to release out string array.
 */
DWORD
LwMergeStringArray(
   IN  PSTR* ppStrArrayA,     IN  DWORD  dwStrArrayACount,
   IN  PSTR* ppStrArrayB,     IN  DWORD  dwStrArrayBCount,
   OUT PSTR** pppStrArrayOut, OUT DWORD* pdwStrArrayOutCount)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   DWORD dwStrArrayOutCount = 0;
   DWORD i = 0;
   DWORD dwIndexOut = 0;
   PSTR* ppStrArrayOut = NULL;

   if ((dwStrArrayACount) && (dwStrArrayBCount == 0))
   {
      dwError = LwDuplicateStringArray(pppStrArrayOut, pdwStrArrayOutCount,
                                       ppStrArrayA, dwStrArrayACount);
      BAIL_ON_LW_ERROR(dwError);
      goto cleanup;
   }

   if ((dwStrArrayBCount) && (dwStrArrayACount == 0))
   {
      dwError = LwDuplicateStringArray(pppStrArrayOut, pdwStrArrayOutCount,
                                       ppStrArrayB, dwStrArrayBCount);
      BAIL_ON_LW_ERROR(dwError);
      goto cleanup;
   }

   dwStrArrayOutCount = dwStrArrayBCount + dwStrArrayACount;

   dwError = LwAllocateStringArray(&ppStrArrayOut, dwStrArrayOutCount);
   BAIL_ON_LW_ERROR(dwError);

   for(i = 0; i < dwStrArrayACount; i++)
   {
      if (ppStrArrayA[i])
      {
         dwError = LwAllocateString(ppStrArrayA[i], &ppStrArrayOut[i]);
         BAIL_ON_LW_ERROR(dwError);
      }
   }

   dwIndexOut = i;
   for(i = 0; i < dwStrArrayBCount; i++)
   {
      if (ppStrArrayB[i])
      {
         dwError = LwAllocateString(ppStrArrayB[i], &ppStrArrayOut[dwIndexOut]);
         BAIL_ON_LW_ERROR(dwError);
      }
        
      dwIndexOut++;
   }

   *pppStrArrayOut = ppStrArrayOut;
   *pdwStrArrayOutCount = dwStrArrayOutCount;

cleanup:
    return dwError;

error:
    goto cleanup;

}

static
int
_LwSafeStringCompare(PSTR pszA, PSTR pszB)
{
    int cmp = 0;
    
    if (pszA)
    {
        if (pszB)
        {
            cmp = strcmp(pszA, pszB);
        }
        else
        {
            cmp = 1;
        }
    }
    else
    {
        if (pszB)
        {
            cmp = -1;
        }
    }
    
    return cmp;
}

static
void
_LwSortMergeStringArray(PSTR* pszaA, PSTR* pszaB, DWORD dwLeft, DWORD dwRight, DWORD dwEnd, BOOLEAN bAscend)
{
    DWORD dwL = dwLeft;
    DWORD dwR = dwRight;
    DWORD i = 0;
    
    for (i = dwLeft; i < dwEnd; i++)
    {
        if (bAscend)
        {
            if (dwL < dwRight && (dwR >= dwEnd || _LwSafeStringCompare(pszaA[dwL], pszaA[dwR]) <= 0))
            {
                pszaB[i] = pszaA[dwL++];
            }
            else
            {
                pszaB[i] = pszaA[dwR++];
            }
        }
        else
        {
            if (dwL < dwRight && (dwR >= dwEnd || _LwSafeStringCompare(pszaA[dwL], pszaA[dwR]) >= 0))
            {
                pszaB[i] = pszaA[dwL++];
            }
            else
            {
                pszaB[i] = pszaA[dwR++];
            }
        }
    }
}

DWORD
LwSortStringArray(
    IN PSTR* pszaList, 
    IN DWORD dwCount,
    BOOLEAN bAscend
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR* pszaTmp = NULL;
    PSTR* pszaA[2] = {0};
    PSTR* pszaA1 = NULL;
    DWORD dwWidth = 0;
    DWORD i = 0;
    
    if (dwCount > 1)
    {
        dwError = LwAllocateStringArray(&pszaTmp, dwCount);
        BAIL_ON_LW_ERROR(dwError);
        
        pszaA[0] = pszaList;
        pszaA[1] = pszaTmp;
                
        for (dwWidth = 1; dwWidth < dwCount; dwWidth = 2 * dwWidth)
        {
            for (i = 0; i < dwCount; i = i + 2 * dwWidth)
            {
                _LwSortMergeStringArray(pszaA[0], pszaA[1], i, LW_MIN(i + dwWidth, dwCount), LW_MIN(i + (2 * dwWidth), dwCount), bAscend);
            }
            
            pszaA1 = pszaA[1];
            pszaA[1] = pszaA[0];
            pszaA[0] = pszaA1;
        }
        
        if (pszaA[0] != pszaList)
        {
            for (i = 0; i < dwCount; i++)
            {
                pszaList[i] = pszaTmp[i];
            }
        }
    }

cleanup:
    // Don't use LwFreeStringArray as we share the strings with pszaList
    LwFreeMemory(pszaTmp);

    return dwError;

error:
    goto cleanup;

}

DWORD
LwConvertListToStringArray(
    OUT PSTR** pppNewStringArray,
    OUT PDWORD pdwNewCount,
    IN PCSTR pszMultiString,
    IN PCSTR pszDelim
    )
{

    DWORD dwError = ERROR_SUCCESS;
    PSTR pszToken = NULL;
    PSTR pszValues = NULL;
    PSTR pszStrtokState = NULL;
    PSTR* ppNewStringArray = NULL;
    PCSTR pszD, pszV;
    DWORD dwNewCount = 0;
    DWORD dwCount = 0;
    
    if (pppNewStringArray == NULL || pdwNewCount == NULL || pszMultiString == NULL || pszDelim == NULL) 
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    // Count the number of values
    for (pszD = pszDelim; *pszD != '\0'; pszD++) 
    {
        for (pszV = pszMultiString; *pszV != '\0'; pszV++) {
            if (*pszV == *pszD) dwNewCount++;
        }
    }
    // Allow for final entry if list is not blank
    if (*pszMultiString) dwNewCount++;

    dwError = LwAllocateStringArray(&ppNewStringArray, dwNewCount);
    BAIL_ON_LW_ERROR(dwError);
    
    pszStrtokState = NULL;
    
    dwError = LwAllocateString(pszMultiString, &pszValues);
    BAIL_ON_LW_ERROR(dwError);

    pszToken = strtok_r(pszValues, pszDelim, &pszStrtokState);

    while (pszToken && *pszToken && dwCount < dwNewCount)
    {
        dwError = LwAllocateString(pszToken, &ppNewStringArray[dwCount++]);
        BAIL_ON_LW_ERROR(dwError);

        pszToken = strtok_r(NULL, pszDelim, &pszStrtokState);
    }

cleanup:
    LW_SAFE_FREE_STRING(pszValues);

    *pppNewStringArray = ppNewStringArray;
    *pdwNewCount = dwCount;

    return dwError;

error:

    LwFreeStringArray(ppNewStringArray, dwNewCount);
    ppNewStringArray = NULL;
    dwNewCount = 0;

    goto cleanup;
}

DWORD
LwConvertMultiStringToStringArray(
    IN PCSTR pszMultiString,
    OUT PSTR** pppszStringArray,
    OUT PDWORD pdwCount
    )
{
    DWORD dwError = 0;
    PSTR* ppszStringArray = NULL;
    DWORD dwCount = 0;
    PCSTR pszIter = NULL;
    DWORD dwIndex = 0;

    dwCount = 0;
    for (pszIter = pszMultiString;
         pszIter && *pszIter;
         pszIter += strlen(pszIter) + 1)
    {
        dwCount++;
    }

    dwError = LwAllocateStringArray(&ppszStringArray, dwCount);
    BAIL_ON_LW_ERROR(dwError);

    if (dwCount)
    {
        dwIndex = 0;
        for (pszIter = pszMultiString;
             pszIter && *pszIter;
             pszIter += strlen(pszIter) + 1)
        {
            dwError = LwAllocateString(pszIter, &ppszStringArray[dwIndex]);
            BAIL_ON_LW_ERROR(dwError);

            dwIndex++;
        }
    }

    LW_ASSERT(dwIndex == dwCount);

cleanup:

    *pppszStringArray = ppszStringArray;
    *pdwCount = dwCount;

    return dwError;

error:

    LwFreeStringArray(ppszStringArray, dwCount);
    ppszStringArray = NULL;
    dwCount = 0;

    goto cleanup;
}

DWORD
LwAllocateStringArray(
    OUT PSTR** pppNewStringArray,
    IN DWORD dwCount
    )
{

    DWORD dwError = ERROR_SUCCESS;
    PSTR* ppNewStringArray = NULL;

    // Allocate space for dwNewCount entries plus a NULL terminator
    dwError = LwAllocateMemory(
                    (dwCount + 1) * sizeof(*ppNewStringArray),
                    OUT_PPVOID(&ppNewStringArray));
    BAIL_ON_LW_ERROR(dwError);
    
cleanup:
    *pppNewStringArray = ppNewStringArray;

    return dwError;

error:

    LwFreeStringArray(ppNewStringArray, dwCount);
    ppNewStringArray = NULL;

    goto cleanup;
}

BOOLEAN
LwStringArrayContains(
        IN PSTR* pszaList, 
        IN DWORD dwCount, 
        IN PSTR pszStr, 
        IN BOOLEAN bCaseSensitive)
{
    DWORD dwIndex;
    
    if (pszaList == NULL) return FALSE;
    
    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        if (pszaList[dwIndex])
        {
            if (bCaseSensitive)
            {
                if (strcmp(pszaList[dwIndex], pszStr) ==  0)
                {
                    return TRUE;                
                } 
            }
            else if (strcasecmp(pszaList[dwIndex], pszStr) ==  0)
            {
                return TRUE;                                
            }
        }
        else if (pszStr == NULL)
        {
            return TRUE;
        }
    }
    
    return FALSE;
}

BOOLEAN
LwStringArrayCompare(
        IN PSTR* pszaList1, 
        IN DWORD dwCount1, 
        IN PSTR* pszaList2, 
        IN DWORD dwCount2)
{
    DWORD dwIndex;
    int cmp = 0;
    
    cmp = dwCount1 - dwCount2;

    if (cmp != 0) return cmp;
    if (pszaList1 == NULL && pszaList2 == NULL) return 0;
    if (pszaList1 == NULL) return -1;
    if (pszaList2 == NULL) return 1;
    
    for (dwIndex = 0; dwIndex < dwCount1 && cmp == 0; dwIndex++)
    {
        if (pszaList1[dwIndex])
        {
            if (pszaList2[dwIndex])
            {
                cmp = strcmp(pszaList1[dwIndex], pszaList2[dwIndex]);
            }
            else 
            {
                cmp = 1;
            }

        }
        else if (pszaList2[dwIndex])
        {
            cmp = -1;
        }
    }
    
    return cmp;
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
    ssize_t dwNewRequiredLength = 0;
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
LwStripLeadingCharacters(
    PSTR pszString,
    CHAR character
)
{
    PSTR pszNew = pszString;
    PSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0' || *pszString != character) {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0' && *pszTmp == character) {
        pszTmp++;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        *pszNew++ = *pszTmp++;
    }
    *pszNew = '\0';
}

void
LwStripTrailingCharacters(
    PSTR pszString,
    CHAR character
)
{
    PSTR pszLastChar = NULL;
    PSTR pszTmp = pszString;

    if (LW_IS_NULL_OR_EMPTY_STR(pszString))
        return;

    while (pszTmp != NULL && *pszTmp != '\0') {
        pszLastChar = ((*pszTmp == character) ? (pszLastChar ? pszLastChar : pszTmp) : NULL);
        pszTmp++;
    }

    if (pszLastChar != NULL) {
        *pszLastChar = '\0';
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

DWORD
LwURLEncodeString(
    PCSTR pIn,
    PSTR *ppOut
    )
{
    DWORD dwError = 0;
    const char *pRequireEscape = "$&+,/:;=?@ \"'<>#%{}|\\^~[]`";
    size_t outputPos = 0;
    size_t i = 0;
    PSTR pOut = NULL;

    for (i = 0; pIn[i]; i++)
    {
        if (pIn[i] < 0x20 || pIn[i] >= 0x7F ||
                strchr(pRequireEscape, pIn[i]) != NULL)
        {
            outputPos+=3;
        }
        else
        {
            outputPos++;
        }
    }

    // Space for the NULL
    outputPos++;

    dwError = LwAllocateMemory(
                    outputPos,
                    OUT_PPVOID(&pOut));
    BAIL_ON_LW_ERROR(dwError);

    for (outputPos = 0, i = 0; pIn[i]; i++)
    {
        if (pIn[i] < 0x20 || pIn[i] >= 0x7F ||
                strchr(pRequireEscape, pIn[i]) != NULL)
        {
            sprintf(pOut + outputPos, "%%%.2X", (BYTE)pIn[i]);
            outputPos+=3;
        }
        else
        {
            pOut[outputPos] = pIn[i];
            outputPos++;
        }
    }

    *ppOut = pOut;

cleanup:
    return dwError;

error:
    *ppOut = NULL;
    LW_SAFE_FREE_STRING(pOut);
    goto cleanup;
}

DWORD
LwURLDecodeString(
    PCSTR pIn,
    PSTR *ppOut
    )
{
    DWORD dwError = 0;
    size_t outputPos = 0;
    size_t i = 0;
    PSTR pOut = NULL;
    BYTE digit1 = 0;
    BYTE digit2 = 0;

    for (i = 0; pIn[i]; i++)
    {
        if (pIn[i] == '%')
        {
            if (!isxdigit((int)(pIn[i+1])) || !isxdigit((int)(pIn[i+2])))
            {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_LW_ERROR(dwError);
            }
            i += 2;
        }
        outputPos++;
    }

    // Space for the NULL
    outputPos++;

    dwError = LwAllocateMemory(
                    outputPos,
                    OUT_PPVOID(&pOut));
    BAIL_ON_LW_ERROR(dwError);

    for (outputPos = 0, i = 0; pIn[i]; i++, outputPos++)
    {
        if (pIn[i] == '%')
        {
            i++;
            dwError = LwHexCharToByte(
                            pIn[i],
                            &digit1);
            BAIL_ON_LW_ERROR(dwError);

            i++;
            dwError = LwHexCharToByte(
                            pIn[i],
                            &digit2);
            BAIL_ON_LW_ERROR(dwError);

            pOut[outputPos] = (digit1 << 4) | digit2;
        }
        else
        {
            pOut[outputPos] = pIn[i];
        }
    }

    *ppOut = pOut;

cleanup:
    return dwError;

error:
    *ppOut = NULL;
    LW_SAFE_FREE_STRING(pOut);
    goto cleanup;
}

/*
      The following are two example URIs and their component parts:

       foo://example.com:8042/over/there?name=ferret#nose
       \_/   \______________/\_________/ \_________/ \__/
        |           |            |            |        |
     scheme     authority       path        query   fragment
        |   _____________________|__
       / \ /                        \
       urn:example:animal:ferret:nose
*/
DWORD
LwURILdapDecode(
    PCSTR pszURI,
    PSTR *ppszScheme,
    PSTR *ppszAuthority,
    PSTR *ppszPath
    )
{
    DWORD dwError = 0;
    PCSTR pszSchemeStart = NULL;
    PCSTR pszAuthorityStart = NULL;
    PCSTR pszPathStart = NULL;
    PCSTR pszIter = NULL;
    PSTR pszScheme = NULL;
    PSTR pszAuthority = NULL;
    PSTR pszPath = NULL;
    PSTR pszPathURLDecode = NULL;

    pszSchemeStart = pszIter = pszURI;
    if (('a' <= *pszIter && *pszIter <= 'z') ||
        ('A' <= *pszIter && *pszIter <= 'Z'))
    {
        while (('a' <= *pszIter && *pszIter <= 'z') ||
           ('A' <= *pszIter && *pszIter <= 'Z') ||
           ('0' <= *pszIter && *pszIter <= '0') ||
           *pszIter == '+' ||
           *pszIter == '-' ||
           *pszIter == '.' )
        {
            pszIter++;
        }

        dwError = LwStrndup(
                    pszSchemeStart,
                    pszIter - pszSchemeStart,
                    &pszScheme);
        BAIL_ON_LW_ERROR(dwError);
    }
    else
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pszIter[0] != ':' ||
        pszIter[1] != '/' ||
        pszIter[2] != '/')
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LW_ERROR(dwError);
    }
    pszIter += 3;

    pszAuthorityStart = pszIter;
    while (*pszIter != '\0' && *pszIter != '/')
    {
        pszIter++;
    }
    if (pszAuthorityStart != pszIter)
    {
        dwError = LwStrndup(
                    pszAuthorityStart,
                    pszIter - pszAuthorityStart,
                    &pszAuthority);
        BAIL_ON_LW_ERROR(dwError);
    }
    else
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LW_ERROR(dwError);
    }

    if (*pszIter)
    {
        pszIter++; // Move past separator

        pszPathStart = pszIter;
        while (*pszIter)
        {
            pszIter++;
        }

        if (pszPathStart != pszIter)
        {
            dwError = LwStrndup(
                        pszPathStart,
                        pszIter - pszPathStart,
                        &pszPath);
            BAIL_ON_LW_ERROR(dwError);

            dwError = LwURLDecodeString(pszPath, &pszPathURLDecode);
            BAIL_ON_LW_ERROR(dwError);
        }
        else
        {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LW_ERROR(dwError);
        }
    }

    if (ppszScheme)
    {
        *ppszScheme = pszScheme;
        pszScheme = NULL;
    }

    if (ppszAuthority)
    {
        *ppszAuthority = pszAuthority;
        pszAuthority = NULL;
    }

    if (ppszPath)
    {
        *ppszPath = pszPathURLDecode;
        pszPathURLDecode = NULL;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszScheme);
    LW_SAFE_FREE_STRING(pszAuthority);
    LW_SAFE_FREE_STRING(pszPath);
    LW_SAFE_FREE_STRING(pszPathURLDecode);

    return dwError;

error:
    goto cleanup;
}
