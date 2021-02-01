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
