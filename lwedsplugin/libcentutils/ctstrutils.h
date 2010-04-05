/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __CTSTRUTILS_H__
#define __CTSTRUTILS_H__


#define CTStrdup LWAllocateString


#define CT_SAFE_FREE_STRING(str) \
    do { if (str) { LWFreeString(str); (str) = NULL; } } while (0)

void
CTStripLeadingWhitespace(
    PSTR pszString
    );

void
CTStripTrailingWhitespace(
    PSTR pszString
    );

void
CTRemoveLeadingWhitespacesOnly(
    PSTR pszString
    );

void
CTRemoveTrailingWhitespacesOnly(
    PSTR pszString
    );

void
CTStripWhitespace(
    PSTR pszString
    );

void
CTStrToUpper(
    PSTR pszString
    );

void
CTStrToLower(
    PSTR pszString
    );

long
LWAllocateStringPrintfV(
    PSTR* result,
    PCSTR format,
    va_list args
    );

long
LWAllocateStringPrintf(
    PSTR* result,
    PCSTR format,
    ...
    );

/** Returns true if substr(str, 0, strlen(prefix)) == prefix
 */
BOOLEAN
CTStrStartsWith(
    PCSTR str,
    PCSTR prefix
    );

/** Returns true if substr(str, strlen(str) - strlen(suffix)) == suffix
 */
BOOLEAN
CTStrEndsWith(
    PCSTR str,
    PCSTR suffix
    );

BOOLEAN
CTIsAllDigit(
    PCSTR pszVal
    );


#endif /* __CTSTRUTILS_H__ */

