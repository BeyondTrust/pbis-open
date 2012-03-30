/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lwstr.h
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

LW_DWORD
LwDuplicateStringArray(
    OUT PSTR** pppNewStringArray,
    OUT PDWORD pdwNewCount,
    IN PSTR* ppStringArray,
    IN DWORD dwCount
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
