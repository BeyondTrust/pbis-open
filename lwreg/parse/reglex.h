/*
 * Copyright Likewise Software    2004-2009
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
 *        reglex.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser lexical analyzer header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */
#ifndef REGLEX_H
#define REGLEX_H

#define REGLEX_DEFAULT_SZ_LEN 1024 
#define REGLEX_CHAR_INDEX(c) ((unsigned char) (c))


typedef enum __REGLEX_TOKEN
{
    REGLEX_FIRST = 0,
    REGLEX_QUOTE_BEGIN,
    REGLEX_QUOTE_END,
    REGLEX_KEY_PREFIX,
    REGLEX_KEY_SUFFIX,
    REGLEX_KEY_NAME,
    REGLEX_EQUALS,
    REGLEX_COMMA,
    REGLEX_HEXPAIR,
    REGLEX_HEXPAIR_END,
    REGLEX_PLAIN_TEXT,

    /* Registry data types */
    REGLEX_REG_DWORD,                      /* dword:    */
    REGLEX_REG_SZ,                         /* ="REG_SZ" */
    REGLEX_REG_BINARY,                     /* hex:      */
    REGLEX_REG_NONE,                       /* hex(0):   */
    REGLEX_REG_EXPAND_SZ,                  /* hex(2):   */
    REGLEX_REG_MULTI_SZ,                   /* hex(7):   */
    REGLEX_REG_RESOURCE_LIST,              /* hex(8):   */
    REGLEX_REG_FULL_RESOURCE_DESCRIPTOR,   /* hex(9):   */
    REGLEX_REG_RESOURCE_REQUIREMENTS_LIST, /* hex(a):   */
    REGLEX_REG_QUADWORD,                   /* hex(b):   */
    REGLEX_REG_KEY,                        /* [ HIVE\subkey] */
    REGLEX_KEY_NAME_DEFAULT,               /* @ token   */
    REGLEX_REG_STRING_ARRAY,               /* sza:      */
    REGLEX_REG_ATTRIBUTES,                 /* hex(aa):  */

    REGLEX_ATTRIBUTES_BEGIN,               /* "{" attibutes */
    REGLEX_ATTRIBUTES_END,                 /* attributes "}" */
    REGLEX_REG_INTEGER_RANGE,              /* integer:M - N */
    REGLEX_DASH,                           /* Integer range values separator */
} REGLEX_TOKEN, *PREGLEX_TOKEN;


typedef enum __REGLEX_STATE
{
    REGLEX_STATE_INIT = 0,
    REGLEX_STATE_IN_QUOTE,   /* Found '"', looking for close '"' */
    REGLEX_STATE_IN_KEY,     /* Found '[', looking for ']'       */
    REGLEX_STATE_IN_ATTRIBUTES, /* Found '{', looking for '}'       */
    REGLEX_STATE_BINHEX_STR,
    REGLEX_STATE_DWORD,
    REGLEX_STATE_QUADWORD,
    REGLEX_STATE_INTEGER_RANGE,
} REGLEX_STATE;

typedef enum __REGLEX_VALUENAME_TYPE
{
    REGLEX_VALUENAME_NAME = 0,
    REGLEX_VALUENAME_SECURITY,
    REGLEX_VALUENAME_ATTRIBUTES,
    REGLEX_VALUENAME_ATTRIBUTES_RESET
} REGLEX_VALUENAME_TYPE;

typedef struct __REGLEX_ITEM *PREGLEX_ITEM;
typedef DWORD (*REGLEX_PARSE_FUNC_T)(PREGLEX_ITEM lexH, HANDLE ioH, CHAR inC);


typedef struct _REGLEX_TOKEN_ITEM
{
    REGLEX_TOKEN token;
    PSTR pszValue;
    DWORD valueSize;
    DWORD valueCursor;
    DWORD lineNum;
} REGLEX_TOKEN_ITEM, *PREGLEX_TOKEN_ITEM;


typedef struct __REGLEX_ITEM
{
    REGLEX_TOKEN_ITEM curToken;
    REGLEX_TOKEN_ITEM prevToken;
    REGLEX_TOKEN tokenDataType;
    DWORD parseLineNum;
    REGLEX_STATE state;
    BOOLEAN isToken;
    REGLEX_PARSE_FUNC_T parseFuncArray[256];
    REGLEX_VALUENAME_TYPE eValueNameType;
} REGLEX_ITEM;

DWORD
RegLexGetToken(
    HANDLE ioHandle,
    PREGLEX_ITEM lexHandle,
    PREGLEX_TOKEN pRetToken,
    PBOOLEAN pEof);

DWORD
RegLexUnGetToken(PREGLEX_ITEM lexHandle);

DWORD
RegLexTokenToString(
    REGLEX_TOKEN token,
    PSTR tokenStr,
    DWORD dwTokenStrLen);

DWORD
RegLexGetLineNumber(
    PREGLEX_ITEM pLexHandle,
    PDWORD pLineNum);

DWORD
RegLexGetAttribute(
    PREGLEX_ITEM pLexHandle,
    PDWORD valueSize,
    PSTR *ppszTokenValue);

DWORD
RegLexOpen(
    PREGLEX_ITEM *ppLexHandle);

void
RegLexClose(
    PREGLEX_ITEM pLexHandle);

DWORD
RegLexResetToken(
    PREGLEX_ITEM lexHandle);

DWORD
RegLexParseQuote(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

DWORD
RegLexParseOpenBracket(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

DWORD
RegLexParseCloseBracket(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

DWORD
RegLexParseOpenBrace(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

DWORD
RegLexParseCloseBrace(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

DWORD
RegLexParseAt(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

DWORD
RegLexParseEquals(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

DWORD
RegLexParseDash(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

DWORD
RegLexParseComma(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

DWORD
RegLexParseBackslash(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

DWORD
RegLexParseColon(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

DWORD
RegLexParseNewline(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

DWORD
RegLexParseWhitespace(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

DWORD
RegLexParseComment(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

DWORD
RegLexParseDefaultState(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC);

#endif
