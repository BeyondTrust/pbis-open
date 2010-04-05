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
#ifndef __EVTSTR_H__
#define __EVTSTR_H__

#define IsNullOrEmptyString(pszStr)     \
    (pszStr == NULL || *pszStr == '\0')

#define CLTR_SAFE_FREE_STRING(wcstr) \
    do { if (wcstr) { CltrFreeString(wcstr); (wcstr) = NULL; } } while(0)

LW_LONG
CltrWC16StringCompareWCNString(
    LW_IN LW_PCWSTR pString1,
    LW_IN const wchar_t* pString2,
    LW_IN LW_BOOLEAN bIsCaseSensitive
    );

int sw16scanfw(LW_PCWSTR pFrom, const wchar_t *pFormat, ...);

DWORD
CltrStrndup(
    PCWSTR pszInputString,
    size_t size,
    PWSTR * ppszOutputString
    );

PCWSTR 
CltrTableCategoryToStr(
    DWORD tableCategory
    );

BOOLEAN
CltrIsWhiteSpace(
    WCHAR c
    );


/* modify PWSTR in-place to conver sequences of whitespace WCHARacters into single spaces (0x20) */
DWORD
CltrCompressWhitespace(
    PWSTR pszString
    );


/* convert a 16-bit wcstring to an 8-bit wcstring, allocating new memory in the process */
DWORD
CltrLWCHARToLpStr(
    PCWSTR pszwString,
    PWSTR* ppszString
    );

void
CltrStripWhitespace(
    PWSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    );

void
CltrStrToUpper(
    PWSTR pszString
    );

void
CltrStrToLower(
    PWSTR pszString
    );

DWORD
CltrEscapeString(
    PWSTR pszOrig,
    PWSTR * ppszEscapedString
    );

#endif /* __EVTSTR_H__ */
