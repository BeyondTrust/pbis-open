/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lwstr.h
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
#ifndef __LWSTR_H__
#define __LWSTR_H__

#include <stdarg.h>

#include <lw/types.h>
#include <lw/attrs.h>

#define LW_IS_NULL_OR_EMPTY_STR(str) (!(str) || !(*(str)))

#define LW_IS_EMPTY_STR(str) ((str)[0] == 0)

#define LW_SECURE_FREE_STRING(str) \
    do { \
       if (str) \
       { \
           if (*(str)) \
           { \
               memset(str, 0, strlen(str)); \
           } \
           LwFreeString(str); \
           (str) = NULL; \
       } \
    } while(0)

#define LW_SECURE_FREE_WSTRING(str) \
    do { \
       if (str) \
       { \
           if (*(str)) \
           { \
               memset(str, 0, wc16slen(str) * sizeof(str[0])); \
           } \
           LwFreeMemory(str); \
           (str) = NULL; \
       } \
    } while(0)

#define LW_SAFE_FREE_STRING(str) \
    do { \
        if (str) \
        { \
            LwFreeString(str); \
            (str) = NULL; \
        } \
    } while (0)

#define LW_SAFE_FREE_STRING_ARRAY(ppszArray)               \
        do {                                                \
           if (ppszArray) {                                 \
               LwFreeNullTerminatedStringArray(ppszArray); \
               (ppszArray) = NULL;                          \
           }                                                \
        } while (0);


LW_BEGIN_EXTERN_C

LW_DWORD
LwAllocateString(
    LW_PCSTR pszInputString,
    LW_PSTR* ppszOutputString
    );

LW_VOID
LwFreeString(
    LW_PSTR pszString
    );

/*
 * Spare input string array is supported.
 * Use LwFreeStringArray() to release new string array.
 */
LW_DWORD
LwDuplicateStringArray(
    OUT PSTR** pppNewStringArray,
    OUT PDWORD pdwNewCount,
    IN PSTR* ppStringArray,
    IN DWORD dwCount
    );

DWORD
LwConvertMultiStringToStringArray(
    IN PCSTR pszMultiString,
    OUT PSTR** pppszStringArray,
    OUT PDWORD pdwCount
    );

/* Merge the given string arrays into ArrayOut.
 * Sparse input string arrays are supported.
 * Use LwFreeStringArray() to release out string array.
 */
DWORD
LwMergeStringArray(
   IN  PSTR* ppStrArrayA,     IN  DWORD  dwStrArrayACount,
   IN  PSTR* ppStrArrayB,     IN  DWORD  dwStrArrayBCount,
   OUT PSTR** pppStrArrayOut, OUT DWORD* pdwStrArrayOutCount);

DWORD
LwAllocateStringArray(
    OUT PSTR** pppNewStringArray,
    IN DWORD dwCount
    );

DWORD
LwConvertListToStringArray(
    OUT PSTR** pppNewStringArray,
    OUT PDWORD pdwNewCount,
    IN PCSTR pszMultiString,
    IN PCSTR pszDelim
    );

BOOLEAN
LwStringArrayContains(
        IN PSTR* pszaList, 
        IN DWORD dwCount, 
        IN PSTR pszStr, 
        IN BOOLEAN bCaseSensitive);

BOOLEAN
LwStringArrayCompare(
        IN PSTR* pszaList1, 
        IN DWORD dwCount1, 
        IN PSTR* pszaList2, 
        IN DWORD dwCount2);

DWORD
LwSortStringArray(
    IN PSTR* pszaList, 
    IN DWORD dwCount,
    BOOLEAN bAscend
    );

void
LwFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    );

LW_DWORD
LwAllocateStringPrintf(
    LW_PSTR* ppszOutputString,
    LW_PCSTR pszFormat,
    ...
    );

LW_DWORD
LwAllocateStringPrintfV(
    LW_PSTR* ppszOutputString,
    LW_PCSTR pszFormat,
    va_list args
    );

LW_VOID
LwStripLeadingWhitespace(
    LW_PSTR pszString
    );

LW_DWORD
LwStrIsAllSpace(
    LW_PCSTR pszString,
    LW_PBOOLEAN pbIsAllSpace
    );

LW_VOID
LwStripTrailingWhitespace(
    LW_PSTR pszString
    );

LW_VOID
LwStripWhitespace(
    LW_IN LW_OUT LW_PSTR pszString,
    LW_IN LW_BOOLEAN bLeading,
    LW_IN LW_BOOLEAN bTrailing
    );

LW_VOID
LwStripLeadingCharacters(
    PSTR pszString,
    CHAR character
    );

LW_VOID
LwStripTrailingCharacters(
    PSTR pszString,
    CHAR character
    );

LW_VOID
LwStrToUpper(
    LW_IN LW_OUT LW_PSTR pszString
    );

VOID
LwStrnToUpper(
    PSTR  pszString,
    DWORD dwLen
    );

VOID
LwStrToLower(
    PSTR pszString
    );

VOID
LwStrnToLower(
    PSTR  pszString,
    DWORD dwLen
    );

DWORD
LwEscapeString(
    PSTR pszOrig,
    PSTR * ppszEscapedString
    );

DWORD
LwStrndup(
    PCSTR pszInputString,
    size_t size,
    PSTR * ppszOutputString
    );

VOID
LwStrCharReplace(
    PSTR pszStr,
    CHAR oldCh,
    CHAR newCh
    );

DWORD
LwStrDupOrNull(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    );

PCSTR
LwEmptyStrForNull(
    PCSTR pszInputString
    );

VOID
LwStrChr(
    PCSTR pszInputString,
    CHAR c,
    PSTR *ppszOutputString
    );

void
LwStrStr(
    PCSTR  pszInputString,
    PCSTR  pszSubstr,
    PSTR  *ppszOutputString
    );

void
LwFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    );

DWORD
LwHexCharToByte(
    CHAR cHexChar,
    UCHAR* pucByte
    );

DWORD
LwHexStrToByteArray(
    IN PCSTR pszHexString,
    IN OPTIONAL DWORD* pdwHexStringLength,
    OUT UCHAR** ppucByteArray,
    OUT DWORD*  pdwByteArrayLength
    );

DWORD
LwByteArrayToHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    );

DWORD
LwWc16sToMbs(
    PCWSTR pwszInput,
    PSTR*  ppszOutput
    );

DWORD
LwMbsToWc16s(
    PCSTR pszInput,
    PWSTR* ppszOutput
    );

DWORD
LwWc16snToMbs(
    PCWSTR pwszInput,
    PSTR*  ppszOutput,
    size_t sMaxChars
    );

DWORD
LwWc16sLen(
    PCWSTR  pwszInput,
    size_t  *psLen
    );

DWORD
LwWc16sCpy(
    PWSTR   pwszOutput,
    PCWSTR  pwszInput
    );

DWORD
LwWc16snCpy(
    PWSTR   pwszOutput,
    PCWSTR  pwszInput,
    DWORD   dwLen
    );

DWORD
LwSW16printf(
    PWSTR*  ppwszStrOutput,
    PCSTR   pszFormat,
    ...);

DWORD
LwAllocateWc16sPrintfW(
    PWSTR* ppszString,
    const wchar_t* pszFormat,
    ...
    );

DWORD
LwWc16sToUpper(
    IN OUT PWSTR pwszString
    );

DWORD
LwWc16sToLower(
    IN OUT PWSTR pwszString
    );

DWORD
LwAllocateWc16String(
    OUT PWSTR *ppwszOutputString,
    IN  PCWSTR pwszInputString
    );

PSTR
LwCaselessStringSearch(
    PCSTR pszHaystack,
    PCSTR pszNeedle
    );

DWORD
LwAllocateUnicodeStringFromWc16String(
    PUNICODE_STRING  pOutputString,
    PCWSTR           pwszInputString
    );


DWORD
LwAllocateUnicodeStringExFromWc16String(
    PUNICODE_STRING   pOutputString,
    PCWSTR            pwszInputString
    );


DWORD
LwAllocateWc16StringFromUnicodeString(
    PWSTR            *ppOutputString,
    PUNICODE_STRING   pInputString
    );


DWORD
LwAllocateUnicodeStringFromCString(
    PUNICODE_STRING   pOutputString,
    PCSTR             pszInputString
    );


DWORD
LwAllocateCStringFromUnicodeString(
    PSTR             *ppszOutputString,
    PUNICODE_STRING   pInputString
    );


DWORD
LwAllocateUnicodeStringExFromCString(
    PUNICODE_STRING   pOutputString,
    PCSTR             pszInputString
    );


VOID
LwFreeUnicodeString(
    PUNICODE_STRING pString
    );

DWORD
LwURLEncodeString(
    PCSTR pIn,
    PSTR *ppOut
    );

DWORD
LwURLDecodeString(
    PCSTR pIn,
    PSTR *ppOut
    );

long long int
LwStrtoll(
    const char* nptr,
    char**      endptr,
    int         base
    );

unsigned long long int
LwStrtoull(
    const char* nptr,
    char**      endptr,
    int         base
    );


DWORD
LwURILdapDecode(
    PCSTR pszURI,
    PSTR *ppszScheme,
    PSTR *ppszAuthority,
    PSTR *ppszPath
    );

LW_END_EXTERN_C


#endif

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
