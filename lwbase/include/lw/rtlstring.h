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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        rtlstring.h
 *
 * Abstract:
 *
 *        Base String API
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *          David Leimbach (dleimbach@likewise.com)
 */

#ifndef __RTL_STRING_H__
#define __RTL_STRING_H__

#include <lw/types.h>
#include <lw/attrs.h>
#include <lw/ntstatus.h>
#include <stdarg.h>

LW_BEGIN_EXTERN_C

// c-style (null-terminated) strings

size_t
LwRtlCStringNumChars(
    LW_IN LW_PCSTR pszString
    );

LW_NTSTATUS
LwRtlCStringAllocateFromWC16String(
    LW_OUT LW_PSTR* ppszNewString,
    LW_IN LW_PCWSTR pszOriginalString
    );

LW_NTSTATUS
LwRtlCStringAllocateFromUnicodeString(
    LW_OUT LW_PSTR* ppszNewString,
    LW_IN LW_PUNICODE_STRING pOriginalString
    );

LW_NTSTATUS
LwRtlCStringDuplicate(
    LW_OUT LW_PSTR* ppszNewString,
    LW_IN LW_PCSTR pszOriginalString
    );

LW_VOID
LwRtlCStringFree(
    LW_IN LW_OUT LW_PSTR* ppszString
    );

LW_LONG
LwRtlCStringCompare(
    LW_IN LW_PCSTR pString1,
    LW_IN LW_PCSTR pString2,
    LW_IN LW_BOOLEAN bIsCaseSensitive
    );

LW_BOOLEAN
LwRtlCStringIsEqual(
    LW_IN LW_PCSTR pString1,
    LW_IN LW_PCSTR pString2,
    LW_IN LW_BOOLEAN bIsCaseSensitive
    );

BOOLEAN
LwRtlCStringFindSubstring(
    LW_IN LW_PCSTR pHaystack,
    LW_IN LW_PCSTR pNeedle,
    LW_IN LW_BOOLEAN isCaseSensitive,
    LW_OUT LW_PCSTR *ppSubstring
    );

LW_NTSTATUS
LwRtlCStringAllocatePrintf(
    LW_OUT LW_PSTR* pNewString,
    LW_IN LW_PCSTR Format,
    LW_IN ...
    );

LW_NTSTATUS
LwRtlCStringAllocatePrintfV(
    LW_OUT LW_PSTR* pNewString,
    LW_IN LW_PCSTR Format,
    LW_IN va_list Args
    );

LW_NTSTATUS
LwRtlCStringAllocateAppendPrintf(
    LW_IN LW_OUT LW_PSTR* pString,
    LW_IN LW_PCSTR Format,
    ...
    );

// wc16-style (null-terminated) strings

size_t
LwRtlWC16StringNumChars(
    LW_IN LW_PCWSTR pszString
    );

LW_NTSTATUS
LwRtlWC16StringAllocateFromCString(
    LW_OUT LW_PWSTR* ppszNewString,
    LW_IN LW_PCSTR pszOriginalString
    );

LW_NTSTATUS
LwRtlWC16StringAllocateFromUnicodeString(
    LW_OUT LW_PWSTR* ppszNewString,
    LW_IN LW_PUNICODE_STRING pOriginalString
    );

LW_NTSTATUS
LwRtlWC16StringDuplicate(
    LW_OUT LW_PWSTR* ppszNewString,
    LW_IN LW_PCWSTR pszOriginalString
    );

LW_VOID
LwRtlWC16StringFree(
    LW_OUT LW_PWSTR* ppszString
    );

LW_BOOLEAN
LwRtlWC16StringIsEqual(
    LW_IN LW_PCWSTR pString1,
    LW_IN LW_PCWSTR pString2,
    LW_IN LW_BOOLEAN bIsCaseSensitive
    );

INT16
LwRtlWC16StringCompare(
    LW_IN LW_PCWSTR pString1,
    LW_IN LW_PCWSTR pString2
    );

BOOLEAN
LwRtlWC16StringFindSubstring(
    LW_IN LW_PCWSTR pHaystack,
    LW_IN LW_PCWSTR pNeedle,
    LW_IN LW_BOOLEAN isCaseSensitive,
    LW_OUT LW_OPTIONAL LW_PCWSTR *ppSubString
    );

LW_NTSTATUS
LwRtlWC16StringAllocatePrintfWV(
    LW_OUT LW_PWSTR* ppszString,
    LW_IN const wchar_t* pszFormat,
    LW_IN va_list Args
    );

LW_NTSTATUS
LwRtlWC16StringAllocatePrintfW(
    LW_OUT LW_PWSTR* ppszString,
    LW_IN const wchar_t* pszFormat,
    LW_IN ...
    );

LW_NTSTATUS
LwRtlWC16StringAllocatePrintf(
    LW_OUT LW_PWSTR* pNewString,
    LW_IN LW_PCSTR Format,
    LW_IN ...
    );

LW_NTSTATUS
LwRtlWC16StringAllocatePrintfV(
    LW_OUT LW_PWSTR* pNewString,
    LW_IN LW_PCSTR Format,
    LW_IN va_list Args
    );

LW_NTSTATUS
LwRtlWC16StringAllocateAppendPrintf(
    LW_IN LW_OUT LW_PWSTR* pString,
    LW_IN LW_PCSTR Format,
    ...
    );

// UNICODE_STRING strings

// TODO: Deprecate in favor of Ex version
LW_VOID
LwRtlUnicodeStringInit(
    LW_OUT LW_PUNICODE_STRING DestinationString,
    LW_IN LW_PCWSTR SourceString
    );

LW_NTSTATUS
LwRtlUnicodeStringInitEx(
    LW_OUT LW_PUNICODE_STRING DestinationString,
    LW_IN LW_PCWSTR SourceString
    );

LW_NTSTATUS
LwRtlUnicodeStringAllocateFromAnsiString(
    LW_OUT LW_PUNICODE_STRING pNewString,
    LW_IN LW_PANSI_STRING pOriginalString
    );

LW_NTSTATUS
LwRtlUnicodeStringAllocateFromWC16String(
    LW_OUT LW_PUNICODE_STRING pString,
    LW_IN LW_PCWSTR pszString
    );

LW_NTSTATUS
LwRtlUnicodeStringAllocateFromCString(
    LW_OUT LW_PUNICODE_STRING pString,
    LW_IN LW_PCSTR pszString
    );

LW_NTSTATUS
LwRtlUnicodeStringDuplicate(
    LW_OUT LW_PUNICODE_STRING pNewString,
    LW_IN LW_PUNICODE_STRING pOriginalString
    );

LW_VOID
LwRtlUnicodeStringFree(
    LW_IN LW_OUT LW_PUNICODE_STRING pString
    );

LW_BOOLEAN
LwRtlUnicodeStringIsEqual(
    LW_IN LW_PUNICODE_STRING pString1,
    LW_IN LW_PUNICODE_STRING pString2,
    LW_IN LW_BOOLEAN bIsCaseSensitive
    );

LW_BOOLEAN
LwRtlUnicodeStringIsPrefix(
    LW_IN LW_PUNICODE_STRING pPrefix,
    LW_IN LW_PUNICODE_STRING pString,
    LW_IN LW_BOOLEAN bIsCaseSensitive
    );

LW_NTSTATUS
LwRtlUnicodeStringParseULONG(
    LW_OUT LW_PULONG pResult,
    LW_IN LW_PUNICODE_STRING pString,
    LW_OUT LW_PUNICODE_STRING pRemainingString
    );

LW_NTSTATUS
LwRtlUnicodeStringAllocatePrintfWV(
    LW_OUT LW_PUNICODE_STRING pString,
    LW_IN const wchar_t* pszFormat,
    LW_IN va_list Args
    );

LW_NTSTATUS
LwRtlUnicodeStringAllocatePrintfW(
    LW_OUT LW_PUNICODE_STRING pString,
    LW_IN const wchar_t* pszFormat,
    LW_IN ...
    );

LW_NTSTATUS
LwRtlUnicodeStringAllocatePrintf(
    LW_OUT LW_PUNICODE_STRING pNewString,
    LW_IN LW_PCSTR Format,
    LW_IN ...
    );

LW_NTSTATUS
LwRtlUnicodeStringAllocatePrintfV(
    LW_OUT LW_PUNICODE_STRING pNewString,
    LW_IN LW_PCSTR Format,
    LW_IN va_list Args
    );

LW_NTSTATUS
LwRtlUnicodeStringAllocateAppendPrintf(
    LW_IN LW_OUT LW_PUNICODE_STRING pString,
    LW_IN LW_PCSTR Format,
    ...
    );

// ANSI strings

// TODO: Deprecate in favor of Ex version
LW_VOID
LwRtlAnsiStringInit(
    LW_OUT LW_PANSI_STRING DestinationString,
    LW_IN LW_PCSTR SourceString
    );

LW_NTSTATUS
LwRtlAnsiStringInitEx(
    LW_OUT LW_PANSI_STRING DestinationString,
    LW_IN LW_PCSTR SourceString
    );

LW_NTSTATUS
LwRtlAnsiStringAllocateFromCString(
    LW_OUT LW_PANSI_STRING pNewString,
    LW_IN LW_PCSTR pszString
    );

LW_NTSTATUS
LwRtlAnsiStringDuplicate(
    LW_OUT LW_PANSI_STRING pNewString,
    LW_IN LW_PANSI_STRING pOriginalString
    );

LW_VOID
LwRtlAnsiStringFree(
    LW_IN LW_OUT LW_PANSI_STRING pString
    );

LW_BOOLEAN
LwRtlAnsiStringIsEqual(
    LW_IN LW_PANSI_STRING pString1,
    LW_IN LW_PANSI_STRING pString2,
    LW_IN LW_BOOLEAN bIsCaseSensitive
    );

LW_BOOLEAN
LwRtlAnsiStringIsPrefix(
    LW_IN LW_PANSI_STRING pPrefix,
    LW_IN LW_PANSI_STRING pString,
    LW_IN LW_BOOLEAN bIsCaseSensitive
    );

LW_NTSTATUS
LwRtlAnsiStringParseULONG(
    LW_OUT LW_PULONG pResult,
    LW_IN LW_PANSI_STRING pString,
    LW_OUT LW_PANSI_STRING pRemainingString
    );

LW_NTSTATUS
LwRtlAnsiStringAllocatePrintf(
    LW_OUT LW_PANSI_STRING pNewString,
    LW_IN LW_PCSTR Format,
    LW_IN ...
    );

LW_NTSTATUS
LwRtlAnsiStringAllocatePrintfV(
    LW_OUT LW_PANSI_STRING pNewString,
    LW_IN LW_PCSTR Format,
    LW_IN va_list Args
    );

LW_NTSTATUS
LwRtlAnsiStringAllocateAppendPrintf(
    LW_IN LW_OUT LW_PANSI_STRING pString,
    LW_IN LW_PCSTR Format,
    ...
    );

// Free helpers

#define LW_RTL_UNICODE_STRING_FREE(String) \
    do { \
        if ((String)->Buffer) \
        { \
            LwRtlUnicodeStringFree(String); \
        } \
    } while (0)

#define LW_RTL_ANSI_STRING_FREE(String) \
    do { \
        if ((String)->Buffer) \
        { \
            LwRtlAnsiStringFree(String); \
        } \
    } while (0)

#ifndef LW_STRICT_NAMESPACE

#define RtlCStringNumChars(String) \
    LwRtlCStringNumChars(String)
#define RtlCStringAllocateFromWC16String(NewString, OriginalString) \
    LwRtlCStringAllocateFromWC16String(NewString, OriginalString)
#define RtlCStringAllocateFromUnicodeString(NewString, OriginalString) \
    LwRtlCStringAllocateFromUnicodeString(NewString, OriginalString)
#define RtlCStringDuplicate(NewString, OriginalString) \
    LwRtlCStringDuplicate(NewString, OriginalString)
#define RtlCStringFree(String) \
    LwRtlCStringFree(String)
#define RtlCStringCompare(String1, String2, IsCaseSensitive) \
    LwRtlCStringCompare(String1, String2, IsCaseSensitive)
#define RtlCStringIsEqual(String1, String2, IsCaseSensitive) \
    LwRtlCStringIsEqual(String1, String2, IsCaseSensitive)
#define RtlCStringFindSubstring(Needle, Haystack, IsCaseSensitive, Substring) \
    LwRtlCStringFindSubstring(Needle, Haystack, IsCaseSensitive, Substring)
#define RtlCStringAllocatePrintf(String, Format, ...) \
    LwRtlCStringAllocatePrintf(String, Format, ## __VA_ARGS__)
#define RtlCStringAllocatePrintfV(String, Format, args) \
    LwRtlCStringAllocatePrintfV(String, Format, args)
#define RtlCStringAllocateAppendPrintf(String, Format, ...) \
    LwRtlCStringAllocateAppendPrintf(String, Format, ## __VA_ARGS__)

#define RtlWC16StringNumChars(String) \
    LwRtlWC16StringNumChars(String)
#define RtlWC16StringAllocateFromCString(NewString, OriginalString) \
    LwRtlWC16StringAllocateFromCString(NewString, OriginalString)
#define RtlWC16StringAllocateFromUnicodeString(NewString, OriginalString) \
    LwRtlWC16StringAllocateFromUnicodeString(NewString, OriginalString)
#define RtlWC16StringDuplicate(NewString, OriginalString) \
    LwRtlWC16StringDuplicate(NewString, OriginalString)
#define RtlWC16StringFree(String) \
    LwRtlWC16StringFree(String)
#define RtlWC16StringIsEqual(String1, String2, IsCaseSensitive) \
    LwRtlWC16StringIsEqual(String1, String2, IsCaseSensitive)
#define RtlWC16StringFindSubstring(Haystack, Needle, IsCaseSensitive, SubStr) \
    LwRtlWC16StringFindSubstring(Haystack, Needle, IsCaseSensitive, SubStr
#define RtlWC16StringAllocatePrintfWV(Result, Format, Args) \
    LwRtlWC16StringAllocatePrintfWV(Result, Format, Args)
#define RtlWC16StringAllocatePrintfW(Result, Format, ...) \
    LwRtlWC16StringAllocatePrintfW(Result, Format, ##  __VA_ARGS__)
#define RtlWC16StringAllocatePrintf(Result, Format, ...) \
    LwRtlWC16StringAllocatePrintf(Result, Format, ## __VA_ARGS__)
#define RtlWC16StringAllocatePrintfV(Result, Format, Args) \
    LwRtlWC16StringAllocatePrintfV(Result, Format, Args)
#define RtlWC16StringAllocateAppendPrintf(Result, Format, ...) \
  LwRtlWC16StringAllocateAppendPrintf(Result, Format, ## __VA_ARGS__)

#define RtlUnicodeStringInit(DestinationString, SourceString) \
    LwRtlUnicodeStringInit(DestinationString, SourceString)
#define RtlUnicodeStringInitEx(DestinationString, SourceString) \
    LwRtlUnicodeStringInitEx(DestinationString, SourceString)
#define RtlUnicodeStringAllocateFromAnsiString(NewString, OriginalString) \
    LwRtlUnicodeStringAllocateFromAnsiString(NewString, OriginalString)
#define RtlUnicodeStringAllocateFromWC16String(NewString, OriginalString) \
    LwRtlUnicodeStringAllocateFromWC16String(NewString, OriginalString)
#define RtlUnicodeStringAllocateFromCString(NewString, OriginalString) \
    LwRtlUnicodeStringAllocateFromCString(NewString, OriginalString)
#define RtlUnicodeStringDuplicate(NewString, OriginalString) \
    LwRtlUnicodeStringDuplicate(NewString, OriginalString)
#define RtlUnicodeStringFree(String) \
    LwRtlUnicodeStringFree(String)
#define RtlUnicodeStringIsEqual(String1, String2, IsCaseSensitive) \
    LwRtlUnicodeStringIsEqual(String1, String2, IsCaseSensitive)
#define RtlUnicodeStringIsPrefix(Prefix, String, IsCaseSensitive) \
    LwRtlUnicodeStringIsPrefix(Prefix, String, IsCaseSensitive)
#define RtlUnicdeStringParseULONG(Result, String, RemainingString) \
    LwRtlUnicodeStringParseULONG(Result, String, RemainingString)
#define RtlUnicodeStringAllocatePrintfW(Result, Format,  ...) \
  LwRtlUnicodeStringAllocatePrintfW(Result, Format, ## __VA_ARGS__)
#define RtlUnicodeStringAllocatePrintf(Result, Format, ...) \
  LwRtlUnicodeStringAllocatePrintf(Result, Format, ## __VA_ARGS__)
#define RtlUnicodeStringAllocatePrintfV(Result, Format, ...) \
  LwRtlUnicodeStringAllocatePrintfV(Result, Format, ## __VA_ARGS__)
#define RtlUnicodeStringAllocateAppendPrintf(Result, Format, ...) \
  LwRtlUnicodeStringAllocateAppendPrintf(Result, Format, ## __VA_ARGS__)

#define RtlAnsiStringInit(DestinationString, SourceString) \
    LwRtlAnsiStringInit(DestinationString, SourceString)
#define RtlAnsiStringInitEx(DestinationString, SourceString) \
    LwRtlAnsiStringInitEx(DestinationString, SourceString)
#define RtlAnsiStringAllocateFromCString(NewString, OriginalString) \
    LwRtlAnsiStringAllocateFromCString(NewString, OriginalString)
#define RtlAnsiStringDuplicate(NewString, OriginalString) \
    LwRtlAnsiStringDuplicate(NewString, OriginalString)
#define RtlAnsiStringFree(String) \
    LwRtlAnsiStringFree(String)
#define RtlAnsiStringIsEqual(String1, String2, IsCaseSensitive) \
    LwRtlAnsiStringIsEqual(String1, String2, IsCaseSensitive)
#define RtlAnsiStringIsPrefix(Prefix, String, IsCaseSensitive) \
    LwRtlAnsiStringIsPrefix(Prefix, String, IsCaseSensitive)
#define RtlAnsiStringParseULONG(Result, String, RemainingString) \
    LwRtlAnsiStringParseULONG(Result, String, RemainingString)
#define RtlAnsiStringAllocatePrintf(Result, Format, ...) \
    LwRtlAnsiStringAllocatePrintf(Result, Format, ## __VA_ARGS__)
#define RtlAnsiStringAllocatePrintfV(Result, Format, Args) \
    LwRtlAnsiStringAllocatePrintfV(Result, Format, Args)
#define RtlAnsiStringAllocateAppendPrintf(Result, Format, ...) \
    LwRtlAnsiStringAllocateAppendPrintf(Result, Format, ## __VA_ARGS__)


#define RTL_UNICODE_STRING_FREE(String) \
    LW_RTL_UNICODE_STRING_FREE(String)
#define RTL_ANSI_STRING_FREE(String) \
    LW_RTL_ANSI_STRING_FREE(String)

#endif /* LW_STRICT_NAMESPAE */

#define USE_RTL_STRING_LOG_HACK 1

#ifdef USE_RTL_STRING_LOG_HACK

LW_PCSTR
LwRtlUnicodeStringToLog(
    LW_IN LW_PUNICODE_STRING pString
    );

LW_PCSTR
LwRtlAnsiStringToLog(
    LW_IN LW_PANSI_STRING pString
    );

LW_PCSTR
LwRtlWC16StringToLog(
    LW_IN LW_PCWSTR pszString
    );

#ifndef LW_STRICT_NAMESPACE
#define RtlUnicodeStringToLog(String) LwRtlUnicodeStringToLog(String)
#define RtlAnsiStringToLog(String)    LwRtlAnsiStringToLog(String)
#define RtlWC16StringToLog(String)    LwRtlWC16StringToLog(String)
#endif /* LW_STRICT_NAMESPAE */

#endif /* USE_RTL_STRING_LOG_HACK */

LW_END_EXTERN_C

#endif /* __RTL_STRING_H__ */
