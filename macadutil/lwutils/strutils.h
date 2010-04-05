/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#ifndef __CTSTRUTILS_H__
#define __CTSTRUTILS_H__

void
LWStripLeadingWhitespace(
    PSTR pszString
    );

void
LWStripTrailingWhitespace(
    PSTR pszString
    );

void
LWRemoveLeadingWhitespacesOnly(
    PSTR pszString
    );

void
LWRemoveTrailingWhitespacesOnly(
    PSTR pszString
    );

void
LWStripWhitespace(
    PSTR pszString
    );

DWORD
LWAllocateStringPrintfV(
    PSTR* result,
    PCSTR format,
    va_list args
    );

void
LWStrToUpper(
    PSTR pszString
    );

void
LWStrToLower(
    PSTR pszString
    );

/** Returns true if substr(str, 0, strlen(prefix)) == prefix
 */
BOOLEAN
LWStrStartsWith(
    PCSTR str,
    PCSTR prefix
    );

/** Returns true if substr(str, strlen(str) - strlen(suffix)) == suffix
 */
BOOLEAN
LWStrEndsWith(
    PCSTR str,
    PCSTR suffix
    );

BOOLEAN
LWIsAllDigit(
    PCSTR pszVal
    );


#endif /* __CTSTRUTILS_H__ */

