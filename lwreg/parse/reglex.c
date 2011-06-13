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
 *        reglex.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser lexical analyzer
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */

#include "includes.h"

DWORD
RegLexOpen(
    PREGLEX_ITEM *ppLexHandle)
{
    DWORD dwError = 0;
    DWORD indx = 0;
    PREGLEX_ITEM pLexHandle;

    dwError = RegAllocateMemory(sizeof(*pLexHandle), (LW_PVOID*)&pLexHandle);
    BAIL_ON_REG_ERROR(dwError);

    memset(pLexHandle, 0, sizeof(REGLEX_ITEM));

    /* Initialize table of function pointers to parsing routines */
    for (indx=0; indx<256; indx++)
    {
        pLexHandle->parseFuncArray[indx] = RegLexParseDefaultState;
    }

    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX('[')]  =
        RegLexParseOpenBracket;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX(']')]  =
        RegLexParseCloseBracket;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX('{')]  = RegLexParseOpenBrace;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX('}')]  = RegLexParseCloseBrace;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX('"')]  = RegLexParseQuote;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX('-')]  = RegLexParseDash;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX('@')]  = RegLexParseAt;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX('=')]  = RegLexParseEquals;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX(',')]  = RegLexParseComma;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX('\\')] = RegLexParseBackslash;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX(':')]  = RegLexParseColon;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX('\r')] = RegLexParseNewline;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX('\n')] = RegLexParseNewline;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX(' ')]  = RegLexParseWhitespace;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX('\t')] = RegLexParseWhitespace;
    pLexHandle->parseFuncArray[REGLEX_CHAR_INDEX('#')]  = RegLexParseComment;

    *ppLexHandle = pLexHandle;
cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(pLexHandle);
    goto cleanup;
}


void
RegLexClose(
    PREGLEX_ITEM pLexHandle)
{
    if (pLexHandle)
    {
        if (pLexHandle->curToken.pszValue)
        {
            RegMemoryFree(pLexHandle->curToken.pszValue);
            pLexHandle->curToken.pszValue = NULL;
        }
        RegMemoryFree(pLexHandle);
    }
}


DWORD RegLexTokenToString(
    REGLEX_TOKEN token,
    PSTR tokenStr,
    DWORD dwTokenStrLen)
{
    static char *tokenStrs[] = {
        "REGLEX_FIRST",
        "REGLEX_QUOTE_BEGIN",
        "REGLEX_QUOTE_END",
        "REGLEX_KEY_PREFIX",
        "REGLEX_KEY_SUFFIX",
        "REGLEX_KEY_NAME",
        "REGLEX_EQUALS",
        "REGLEX_COMMA",
        "REGLEX_HEXPAIR",
        "REGLEX_HEXPAIR_END",
        "REGLEX_PLAIN_TEXT",

        "REGLEX_REG_DWORD",                              /* dword:    */
        "REGLEX_REG_SZ",                                 /* ="REG_SZ" */
        "REGLEX_REG_BINARY",                             /* hex:      */
        "REGLEX_REG_NONE",                               /* hex(0):   */
        "REGLEX_REG_EXPAND_SZ",                          /* hex(2):   */
        "REGLEX_REG_MULTI_SZ",                           /* hex(7):   */
        "REGLEX_REG_RESOURCE_LIST",                      /* hex(8):   */
        "REGLEX_REG_FULL_RESOURCE_DESCRIPTOR",           /* hex(9):   */
        "REGLEX_REG_RESOURCE_REQUIREMENTS_LIST",         /* hex(a):   */
        "REGLEX_REG_QUADWORD",                           /* hex(b):   */
        "REGLEX_REG_KEY",
        "REGLEX_REG_NAME_DEFAULT",
        "REGLEX_REG_STRING_ARRAY",                       /* sza:      */
        "REGLEX_ATTRIBUTES",                             /* { attributes } */
        "REGLEX_ATTRIBUTES_START",                       /* { attributes start */
        "REGLEX_ATTRIBUTES_END",                         /* attributes end } */
        "REGLEX_REG_INTEGER_RANGE",                      /* integer:M - N */
        "REGLEX_DASH",                                   /* Integer range values separator */
    };
    if (token < (sizeof(tokenStrs)/sizeof(char *)))
    {
        tokenStr[0] = '\0';
        strncat(tokenStr, tokenStrs[token], dwTokenStrLen-1);
    }
    else
    {
        snprintf(tokenStr, dwTokenStrLen, "ERROR: No Such Token %d", token);
    }

    return 0;
}


DWORD
RegLexGetLineNumber(
    PREGLEX_ITEM pLexHandle,
    PDWORD pLineNum)
{
    DWORD dwError = 0;
    BAIL_ON_INVALID_HANDLE(pLexHandle);


    *pLineNum = pLexHandle->curToken.lineNum + 1;
cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegLexGetAttribute(
    PREGLEX_ITEM pLexHandle,
    PDWORD pValueSize,
    PSTR *ppszTokenValue)
{
    DWORD dwError = 0;
    BAIL_ON_INVALID_HANDLE(pLexHandle);
    BAIL_ON_INVALID_HANDLE(pValueSize);
    BAIL_ON_INVALID_HANDLE(ppszTokenValue);

    *ppszTokenValue = pLexHandle->curToken.pszValue;
    *pValueSize     = pLexHandle->curToken.valueCursor;
cleanup:
    return dwError;

error:
    goto cleanup;
}



DWORD RegLexAppendChar(
    PREGLEX_ITEM lexHandle,
    CHAR inC)
{
    DWORD dwError = 0;
    PVOID pNewMemory = NULL;
    DWORD newValueSize = 0;

    BAIL_ON_INVALID_HANDLE(lexHandle);
    if (lexHandle->curToken.valueCursor >= lexHandle->curToken.valueSize)
    {
        newValueSize = lexHandle->curToken.valueSize * 2;
        dwError = RegReallocMemory(
                      lexHandle->curToken.pszValue,
                      &pNewMemory,
                      newValueSize + 1); /* Extra byte for string termination */
        BAIL_ON_REG_ERROR(dwError);

        lexHandle->curToken.pszValue = pNewMemory;
        lexHandle->curToken.valueSize = newValueSize;
    }

    /* Append current character to string */
    lexHandle->curToken.pszValue[lexHandle->curToken.valueCursor] = inC;
    lexHandle->curToken.valueCursor++;
    lexHandle->curToken.pszValue[lexHandle->curToken.valueCursor] = '\0';

cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(pNewMemory);
    goto cleanup;
}


DWORD
RegLexParseQuote(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;

    if (lexHandle->curToken.token == REGLEX_PLAIN_TEXT)
    {
        lexHandle->isToken = TRUE;
        lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        dwError = RegIOUnGetChar(ioHandle, NULL);
        return dwError;
    }
    /*
     * Track state of "in quote" and "out quote" since
     * the same character begins/ends a string literal.
     * This is further complicated by escaped '\"' sequences
     * which are a literal quote in a quoted string.
     */
    if (lexHandle->state == REGLEX_STATE_IN_KEY)
    {
        RegLexAppendChar(lexHandle, inC);
    }
    else if (lexHandle->state != REGLEX_STATE_IN_QUOTE)
    {
        /* Close quote found, done with current string */
        lexHandle->curToken.token = REGLEX_QUOTE_BEGIN;
        lexHandle->state = REGLEX_STATE_IN_QUOTE;
        lexHandle->curToken.valueCursor = 0;
        lexHandle->curToken.pszValue[lexHandle->curToken.valueCursor] = '\0';
    }
    else
    {
        lexHandle->isToken = TRUE;
        lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        lexHandle->curToken.token = REGLEX_REG_SZ;
        lexHandle->state = REGLEX_STATE_INIT;
        return dwError;
    }

    return dwError;
}


DWORD
RegLexParseOpenBracket(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;

    if (lexHandle->curToken.token == REGLEX_PLAIN_TEXT)
    {
        lexHandle->isToken = TRUE;
        lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        dwError = RegIOUnGetChar(ioHandle, NULL);
        return dwError;
    }
    if (lexHandle->state != REGLEX_STATE_IN_QUOTE)
    {
        lexHandle->curToken.token = REGLEX_KEY_PREFIX;
        if (lexHandle->state == REGLEX_STATE_IN_KEY)
        {
            /* This is a problem, can't have [ then another [ */
            dwError = LWREG_ERROR_UNEXPECTED_TOKEN;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        }
        else
        {
            lexHandle->curToken.token = REGLEX_KEY_PREFIX;
            lexHandle->state = REGLEX_STATE_IN_KEY;
            lexHandle->curToken.valueCursor = 0;
        }
    }
    else
    {
        RegLexAppendChar(lexHandle, inC);
    }
    return dwError;
}


DWORD
RegLexParseCloseBracket(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;

    if (lexHandle->state != REGLEX_STATE_IN_QUOTE)
    {
        lexHandle->curToken.token = REGLEX_KEY_SUFFIX;
        if (lexHandle->state != REGLEX_STATE_IN_KEY)
        {
            /* This is a problem, can't have ] without a previous [ */
            dwError = LWREG_ERROR_UNEXPECTED_TOKEN;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        }
        else
        {
            lexHandle->isToken = TRUE;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
            lexHandle->curToken.token = REGLEX_REG_KEY;
            lexHandle->state = REGLEX_STATE_INIT;
        }
    }
    else
    {
        RegLexAppendChar(lexHandle, inC);
    }
    return dwError;
}

DWORD
RegLexParseOpenBrace(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;

    if (lexHandle->state != REGLEX_STATE_IN_QUOTE &&
        lexHandle->state != REGLEX_STATE_IN_KEY)
    {
        lexHandle->curToken.token = REGLEX_ATTRIBUTES_BEGIN;
        if (lexHandle->eValueNameType == REGLEX_VALUENAME_ATTRIBUTES)
        {
            /* This is a problem, can't have { then another { */
            dwError = LWREG_ERROR_UNEXPECTED_TOKEN;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        }
        else if (!lexHandle->curToken.pszValue ||
                 !lexHandle->curToken.pszValue[0])
        {
            dwError = LWREG_ERROR_UNEXPECTED_TOKEN;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        }
        else
        {
            lexHandle->curToken.token = REGLEX_ATTRIBUTES_BEGIN;
            lexHandle->eValueNameType = REGLEX_VALUENAME_ATTRIBUTES;
            lexHandle->isToken = TRUE;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
            lexHandle->curToken.valueCursor = 0;
        }
    }
    else
    {
        RegLexAppendChar(lexHandle, inC);
    }
    return dwError;
}

DWORD
RegLexParseCloseBrace(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;

    if (lexHandle->state != REGLEX_STATE_IN_QUOTE &&
        lexHandle->state != REGLEX_STATE_IN_KEY)
    {
        if (lexHandle->eValueNameType != REGLEX_VALUENAME_ATTRIBUTES)
        {
            /* This is a problem, can't have } without a previous { */
            dwError = LWREG_ERROR_UNEXPECTED_TOKEN;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        }
        else
        {
            lexHandle->isToken = TRUE;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
            lexHandle->curToken.token = REGLEX_ATTRIBUTES_END;
            lexHandle->state = REGLEX_STATE_INIT;
        }
    }
    else
    {
        RegLexAppendChar(lexHandle, inC);
    }
    return dwError;
}

DWORD
RegLexParseAt(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;
    BOOLEAN eof = FALSE;
    BOOLEAN bHasSecurity = FALSE;

    if (lexHandle->state != REGLEX_STATE_IN_QUOTE &&
        lexHandle->state != REGLEX_STATE_IN_KEY)
    {
        /* Default value for a registry key */
        lexHandle->isToken = TRUE;
        lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        lexHandle->curToken.token = REGLEX_KEY_NAME_DEFAULT;
        lexHandle->state = REGLEX_STATE_INIT;
        lexHandle->curToken.valueCursor = 0;
        RegLexAppendChar(lexHandle, inC);

        /* Handle the case of @frob, i.e. "@security" */
        dwError = RegIOGetChar(ioHandle, &inC, &eof);
        while (dwError == 0 && !eof && isalpha((int) inC))
        {
            RegLexAppendChar(lexHandle, inC);
            dwError = RegIOGetChar(ioHandle, &inC, &eof);
            bHasSecurity = TRUE;
        }
        if (eof)
        {
            return dwError;
        }
        dwError = RegIOUnGetChar(ioHandle, NULL);
        if (bHasSecurity)
        {
            if (strcmp(lexHandle->curToken.pszValue, "@security") == 0)
            {
                lexHandle->eValueNameType = REGLEX_VALUENAME_SECURITY;
            }
            else
            {
                dwError = LWREG_ERROR_UNEXPECTED_TOKEN;
            }
        }
    }
    return dwError;
}


DWORD
RegLexParseEquals(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;

    if (lexHandle->state != REGLEX_STATE_IN_QUOTE &&
        lexHandle->state != REGLEX_STATE_IN_KEY)
    {
        if (lexHandle->curToken.valueCursor > 0)
        {
            dwError = RegIOUnGetChar(ioHandle, NULL);
            lexHandle->isToken = TRUE;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        }
        else
        {
            lexHandle->isToken = TRUE;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
            lexHandle->curToken.token = REGLEX_EQUALS;
            lexHandle->curToken.valueCursor = 0;
        }
    }
    else
    {
        RegLexAppendChar(lexHandle, inC);
    }
    return dwError;
}


DWORD
RegLexParseDash(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;
    BOOLEAN eof = FALSE;
    BOOLEAN isDash = FALSE;

    /* Ignore dash when any of these contexts */
    if (lexHandle->state == REGLEX_STATE_IN_QUOTE ||
        lexHandle->state == REGLEX_STATE_IN_KEY ||
        lexHandle->eValueNameType == REGLEX_VALUENAME_SECURITY)
    {
        RegLexAppendChar(lexHandle, '-');
        return dwError;
    }

    if (lexHandle->curToken.pszValue[lexHandle->curToken.valueCursor] == '-')
    {
        isDash = TRUE;
    }
    else
    {
        dwError = RegIOGetChar(ioHandle, &inC, &eof);
        if (inC == '\r' || inC == '\n')
        {
            isDash = TRUE;
        }
        else if (inC != '-')
        {
            RegLexAppendChar(lexHandle, '-');
            RegLexAppendChar(lexHandle, inC);
            return dwError;
        }
        else
        {
            dwError = RegIOUnGetChar(ioHandle, NULL);
            isDash = TRUE;
        }
    }
    if (isDash && lexHandle->state != REGLEX_STATE_IN_QUOTE &&
        lexHandle->state != REGLEX_STATE_IN_KEY &&
        lexHandle->eValueNameType != REGLEX_VALUENAME_SECURITY)
    {
        if (lexHandle->curToken.valueCursor > 0)
        {
            dwError = RegIOUnGetChar(ioHandle, NULL);
            lexHandle->isToken = TRUE;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        }
        else
        {
            lexHandle->isToken = TRUE;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
            lexHandle->curToken.token = REGLEX_DASH;
            lexHandle->curToken.valueCursor = 0;
            RegLexAppendChar(lexHandle, inC);
        }
    }
    else
    {
        RegLexAppendChar(lexHandle, inC);
    }
    return dwError;
}


DWORD
RegLexParseComma(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;

    if (lexHandle->state != REGLEX_STATE_IN_QUOTE &&
        lexHandle->state != REGLEX_STATE_IN_KEY)
    {
        if (lexHandle->curToken.valueCursor > 0 && lexHandle->curToken.valueCursor <=2)
        {
            lexHandle->isToken = TRUE;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
            lexHandle->curToken.token = REGLEX_HEXPAIR;
        }
        else
        {
            /* Syntax error: Hex pair is only 2 characters */
            dwError = LWREG_ERROR_UNEXPECTED_TOKEN;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        }
    }
    else
    {
        RegLexAppendChar(lexHandle, inC);
    }
    return dwError;
}

DWORD
RegLexParseBackslash(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;
    BOOLEAN eof = FALSE;

    if (lexHandle->state == REGLEX_STATE_BINHEX_STR ||
        lexHandle->tokenDataType == REGLEX_REG_STRING_ARRAY)
    {
        /* Eat line continuation character when in BINHEX state */
        dwError = RegIOGetChar(ioHandle, &inC, &eof);
        if (eof)
        {
            return dwError;
        }
        if (inC == '\r' || inC == '\n')
        {
            lexHandle->parseLineNum++;
            dwError = RegIOGetChar(ioHandle, &inC, &eof);
            if (eof)
            {
                return dwError;
            }
            if (inC != '\r' && inC != '\n')
            {
                dwError = RegIOUnGetChar(ioHandle, NULL);
            }
        }
        else
        {
            dwError = RegIOUnGetChar(ioHandle, NULL);
        }
    }

    if (lexHandle->state == REGLEX_STATE_IN_QUOTE)
    {
        /*
         * Treat sequence '\C'  (C=any character) as
         * the literal escaped character. Only handle backslashes
         * in this way when in quoted strings. When processing a
         * registry key value, this is a "bad thing" to do.
         * [HKEY_LOCAL_MACHINE\HARDWARE]
         * This processing in this case would be an escaped 'H', not
         * what is intended.
         */
        dwError = RegIOGetChar(ioHandle, &inC, &eof);
        if (!eof)
        {
            switch(inC)
            {
              case 'n':
                RegLexAppendChar(lexHandle, '\n');
                break;

              case 'r':
                RegLexAppendChar(lexHandle, '\r');
                break;

              case 't':
                RegLexAppendChar(lexHandle, '\t');
                break;

              case 'a':
                RegLexAppendChar(lexHandle, '\a');
                break;

              case 'v':
                RegLexAppendChar(lexHandle, '\v');
                break;

              case 'f':
                RegLexAppendChar(lexHandle, '\f');
                break;

              case '\\':
                RegLexAppendChar(lexHandle, '\\');
                break;

              case '"':
                RegLexAppendChar(lexHandle, '"');
                break;

              default:
                RegLexAppendChar(lexHandle, '\\');
                RegLexAppendChar(lexHandle, inC);
                break;
            }
        }
    }
    else if (lexHandle->state == REGLEX_STATE_IN_KEY)
    {
        RegLexAppendChar(lexHandle, '\\');
    }
    else if (lexHandle->state != REGLEX_STATE_BINHEX_STR) {
        RegLexAppendChar(lexHandle, '\\');
    }
    lexHandle->curToken.lineNum = lexHandle->parseLineNum;
    return dwError;
}


DWORD
RegLexParseBinary(
    PREGLEX_ITEM lexHandle)
{
    DWORD dwError = 0;

    /* Test for type prefix */
    if (lexHandle->curToken.valueCursor > 0)
    {
        if (strcasecmp(lexHandle->curToken.pszValue, "dword") == 0 ||
            strcasecmp(lexHandle->curToken.pszValue, "REG_DWORD") == 0)
        {
            lexHandle->curToken.token = REGLEX_REG_DWORD;
            lexHandle->state = REGLEX_STATE_DWORD;
            lexHandle->curToken.valueCursor = 0;
        }
        else if (strcasecmp(lexHandle->curToken.pszValue, "hex") == 0 ||
                 strcasecmp(lexHandle->curToken.pszValue, "REG_BINARY") == 0)
        {
            lexHandle->isToken = TRUE;
            lexHandle->curToken.token = REGLEX_REG_BINARY;
            lexHandle->state = REGLEX_STATE_BINHEX_STR;
            lexHandle->curToken.valueCursor = 0;
        }
        else if (strcasecmp(lexHandle->curToken.pszValue, "hex(0)") == 0 ||
                 strcasecmp(lexHandle->curToken.pszValue, "REG_NONE") == 0)
        {
            /* REG_NONE */
            lexHandle->isToken = TRUE;
            lexHandle->curToken.token = REGLEX_REG_NONE;
            lexHandle->state = REGLEX_STATE_BINHEX_STR;
            lexHandle->curToken.valueCursor = 0;
        }
        else if (strcasecmp(lexHandle->curToken.pszValue, "hex(2)") == 0 ||
                 strcasecmp(lexHandle->curToken.pszValue, "REG_EXPAND_SZ") == 0)
        {
            /* REG_EXPAND_SZ */
            lexHandle->isToken = TRUE;
            lexHandle->curToken.token = REGLEX_REG_EXPAND_SZ;
            lexHandle->state = REGLEX_STATE_BINHEX_STR;
            lexHandle->curToken.valueCursor = 0;

        }
        else if (strcasecmp(lexHandle->curToken.pszValue, "hex(7)") == 0 ||
                 strcasecmp(lexHandle->curToken.pszValue, "REG_MULTI_SZ") == 0)
        {
            /* REG_MULTI_SZ */
            lexHandle->isToken = TRUE;
            lexHandle->curToken.token = REGLEX_REG_MULTI_SZ;
            lexHandle->state = REGLEX_STATE_BINHEX_STR;
            lexHandle->curToken.valueCursor = 0;
        }
        else if (strcasecmp(lexHandle->curToken.pszValue, "sza") == 0 ||
                 strcasecmp(lexHandle->curToken.pszValue, "REG_STRING_ARRAY") == 0 ||
                 (strcasecmp(lexHandle->curToken.pszValue, "string") == 0 &&
                  lexHandle->eValueNameType == REGLEX_VALUENAME_ATTRIBUTES))
        {
            /* REG_STRING_ARRAY
             * Similar to REG_MULTI_SZ (token type returned will be MULTI_SZ).
             * However, the parse format is:
             * "ValueName"=sza:"String 1" "String 2" \
             *              "String 3" "Last string"
             */

            lexHandle->isToken = TRUE;
            lexHandle->curToken.token = REGLEX_REG_STRING_ARRAY;
            lexHandle->tokenDataType = REGLEX_REG_STRING_ARRAY;
            lexHandle->state = REGLEX_STATE_INIT;
            lexHandle->curToken.valueCursor = 0;
        }
        else if (strcasecmp(lexHandle->curToken.pszValue, "hex(8)") == 0 ||
                 strcasecmp(lexHandle->curToken.pszValue, "REG_RESOURCE_LIST") == 0)
        {
            /* REG_RESOURCE_LIST */
            lexHandle->isToken = TRUE;
            lexHandle->curToken.token = REGLEX_REG_RESOURCE_LIST;
            lexHandle->state = REGLEX_STATE_BINHEX_STR;
            lexHandle->curToken.valueCursor = 0;
        }
        else if (strcasecmp(lexHandle->curToken.pszValue, "hex(9)") == 0 ||
                 strcasecmp(lexHandle->curToken.pszValue, "REG_FULL_RESOURCE_DESCRIPTOR") == 0)
        {
            /* REG_FULL_RESOURCE_DESCRIPTOR */
            lexHandle->isToken = TRUE;
            lexHandle->curToken.token = REGLEX_REG_FULL_RESOURCE_DESCRIPTOR;
            lexHandle->state = REGLEX_STATE_BINHEX_STR;
            lexHandle->curToken.valueCursor = 0;
        }
        else if (strcasecmp(lexHandle->curToken.pszValue, "hex(a)") == 0 ||
                 strcasecmp(lexHandle->curToken.pszValue, "REG_RESOURCE_REQUIREMENTS_LIST") == 0)
        {
            /* REG_RESOURCE_REQUIREMENTS_LIST */
            lexHandle->isToken = TRUE;
            lexHandle->curToken.token = REGLEX_REG_RESOURCE_REQUIREMENTS_LIST;
            lexHandle->state = REGLEX_STATE_BINHEX_STR;
            lexHandle->curToken.valueCursor = 0;
        }
        else if (strcasecmp(lexHandle->curToken.pszValue, "hex(b)") == 0 ||
                 strcasecmp(lexHandle->curToken.pszValue, "REG_QUADWORD") == 0)
        {
            /* REG_QUADWORD */
            lexHandle->isToken = TRUE;
            lexHandle->curToken.token = REGLEX_REG_QUADWORD;
            lexHandle->state = REGLEX_STATE_BINHEX_STR;
            lexHandle->curToken.valueCursor = 0;
        }
        else if (strcasecmp(lexHandle->curToken.pszValue, "integer") == 0)
        {
            /* integer:m-n range  */
            lexHandle->isToken = TRUE;
            lexHandle->curToken.token = REGLEX_REG_INTEGER_RANGE;
            lexHandle->state = REGLEX_STATE_INTEGER_RANGE;
            lexHandle->curToken.valueCursor = 0;
        }
    }

    if (lexHandle->isToken)
    {
        lexHandle->curToken.lineNum = lexHandle->parseLineNum;
    }
    return dwError;
}


DWORD
RegLexParseColon(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;

    if (lexHandle->state == REGLEX_STATE_IN_QUOTE ||
        lexHandle->state == REGLEX_STATE_IN_KEY || 
        lexHandle->eValueNameType == REGLEX_VALUENAME_SECURITY)
    {
        RegLexAppendChar(lexHandle, inC);
        return dwError;
    }

    return RegLexParseBinary(lexHandle);
}


DWORD
RegLexParseNewline(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;
    CHAR prevC = '\0';
    BOOLEAN eof = 0;

    if (lexHandle->state != REGLEX_STATE_IN_QUOTE)
    {
        /*
         * Throw out CR when not in a quoted string. CR/LF
         * is still handled correctly by this treatment.
         */
        if (inC == '\r')
        {
            return dwError;
        }
    }
    else
    {
        /*
         * Count CR/LF sequence in a quoted string as a single newline.
         * A bare CR is maintained, but not counted as a newline.
         */
        if (inC == '\r')
        {
            RegLexAppendChar(lexHandle, inC);
            dwError = RegIOGetChar(ioHandle, &inC, &eof);
            if (eof)
            {
                return dwError;
            }
            if (inC == '\n')
            {
                RegLexAppendChar(lexHandle, inC);
                lexHandle->parseLineNum++;
            }
            else
            {
                dwError = RegIOUnGetChar(ioHandle, &inC);
            }
        }
    }

    if (lexHandle->state != REGLEX_STATE_IN_QUOTE)
    {
        lexHandle->parseLineNum++;
    }

    /* Emit the final token in a BINHEX_STR sequence */
    if (lexHandle->state == REGLEX_STATE_BINHEX_STR)
    {
        /* Don't care about "\\n sequence */
        dwError = RegIOGetPrevChar(ioHandle, &prevC);
        if (dwError == ERROR_SUCCESS && prevC == '\\')
        {
            return dwError;
        }

        lexHandle->isToken = TRUE;
        lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        lexHandle->curToken.token = REGLEX_HEXPAIR_END;
        lexHandle->state = REGLEX_STATE_INIT;
        return dwError;
    }
    else if (lexHandle->tokenDataType == REGLEX_REG_STRING_ARRAY)
    {
        lexHandle->isToken = TRUE;
        lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        lexHandle->tokenDataType = REGLEX_FIRST;
        return dwError;
    }
    else if (lexHandle->state == REGLEX_STATE_DWORD)
    {
        if (lexHandle->curToken.valueCursor == 8)
        {
            lexHandle->isToken = TRUE;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
            lexHandle->curToken.token = REGLEX_REG_DWORD;
            return dwError;
        }
        else
        {
            dwError = LWREG_ERROR_UNEXPECTED_TOKEN;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
            return dwError;
        }
    }
    else if ((lexHandle->state == REGLEX_STATE_INIT ||
              lexHandle->state == REGLEX_STATE_INTEGER_RANGE) &&
             lexHandle->curToken.valueCursor > 0)
    {
        /*
         * This is "junk" accumulated so far, since no valid token start
         * has been found before the current end-of-line.
         */
        lexHandle->isToken = TRUE;
        lexHandle->curToken.lineNum = lexHandle->parseLineNum - 1;
        lexHandle->curToken.token = REGLEX_PLAIN_TEXT;
        return dwError;
    }
    else
    {
        /* Syntax error case... */
        /* Error needed */
    }
    return dwError;
}


DWORD
RegLexParseComment(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;
    BOOLEAN eof = FALSE;

    if (lexHandle->state == REGLEX_STATE_IN_QUOTE ||
        lexHandle->state == REGLEX_STATE_IN_KEY)
    {
        RegLexAppendChar(lexHandle, inC);
    }
    else
    {
        do
        {
            dwError = RegIOGetChar(ioHandle, &inC, &eof);
        } while (dwError == 0 && !eof && inC != '\n' && inC != '\r');

        if (!eof && (inC == '\n' || inC == '\r'))
        {
            dwError = RegIOUnGetChar(ioHandle, NULL);
        }
    }
    return dwError;
}


DWORD
RegLexParseWhitespace(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;

    if (lexHandle->state == REGLEX_STATE_IN_QUOTE ||
        lexHandle->state == REGLEX_STATE_IN_KEY)
    {
        RegLexAppendChar(lexHandle, inC);
    }
    else if (lexHandle->state == REGLEX_STATE_BINHEX_STR)
    {
        if (lexHandle->curToken.valueCursor > 0 &&
            lexHandle->curToken.valueCursor <= 2)
        {
            lexHandle->isToken = TRUE;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
            lexHandle->curToken.token = REGLEX_HEXPAIR;
        }
    }
    else
    {
        dwError = RegLexParseBinary(lexHandle);
        if (dwError || lexHandle->isToken)
        {
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
            return dwError;
        }
        if (lexHandle->curToken.token != REGLEX_FIRST)
        {
            lexHandle->isToken = TRUE;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
        }
    }
    return dwError;
}


DWORD
RegLexParseDefaultState(
    PREGLEX_ITEM lexHandle,
    HANDLE ioHandle,
    CHAR inC)
{
    DWORD dwError = 0;

    if (lexHandle->state == REGLEX_STATE_IN_QUOTE ||
        lexHandle->state == REGLEX_STATE_IN_KEY)
    {
        /* Append current character to string */
        RegLexAppendChar(lexHandle, inC);
    }
    else if (lexHandle->state == REGLEX_STATE_BINHEX_STR)
    {
        if (inC == ' ' || inC == ',')
        {
            /* Eat white spaces in binhex strings */
            lexHandle->isToken = TRUE;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
            lexHandle->curToken.token = REGLEX_HEXPAIR;
            return dwError;
        }

        if (!isxdigit((int)inC))
        {
            dwError = LWREG_ERROR_UNEXPECTED_TOKEN;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
            return dwError;
        }
        if (lexHandle->curToken.valueCursor == 2)
        {
            dwError = RegIOUnGetChar(ioHandle, &inC);
            lexHandle->isToken = TRUE;
            lexHandle->curToken.lineNum = lexHandle->parseLineNum;
            lexHandle->curToken.token = REGLEX_HEXPAIR;
            return dwError;
        }
        RegLexAppendChar(lexHandle, inC);

    }
    else
    {
        RegLexAppendChar(lexHandle, inC);
        if (lexHandle->state != REGLEX_STATE_DWORD)
        {
            lexHandle->curToken.token = REGLEX_PLAIN_TEXT;
        }
#ifdef _LW_DEBUG
        if (lexHandle->state != REGLEX_STATE_IN_QUOTE &&
            lexHandle->state != REGLEX_STATE_IN_KEY &&
            lexHandle->state != REGLEX_STATE_BINHEX_STR)
        {
            printf("<%c> <%02x>\n", inC, inC);
        }
#endif
    }
    return dwError;
}


DWORD
RegLexUnGetToken(PREGLEX_ITEM lexHandle)
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(lexHandle);

    if (lexHandle->prevToken.token)
    {
        if (lexHandle->prevToken.pszValue)
        {
            LWREG_SAFE_FREE_MEMORY(lexHandle->prevToken.pszValue);
        }
    }
    lexHandle->prevToken = lexHandle->curToken;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegLexResetToken(
    PREGLEX_ITEM lexHandle)
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(lexHandle);

    lexHandle->state = REGLEX_FIRST;
    lexHandle->tokenDataType = REGLEX_FIRST;
    lexHandle->isToken = FALSE;
    LWREG_SAFE_FREE_MEMORY(lexHandle->curToken.pszValue);
    memset(&lexHandle->curToken, 0, sizeof(lexHandle->curToken));
    lexHandle->prevToken.pszValue = NULL;

cleanup:
    return dwError;

error:
    goto cleanup;
}



DWORD
RegLexGetToken(
    HANDLE ioHandle,
    PREGLEX_ITEM lexHandle,
    PREGLEX_TOKEN pRetToken,
    PBOOLEAN pEof)
{
    DWORD dwError = 0;
    CHAR inC = 0;
    BOOLEAN eof = FALSE;

    BAIL_ON_INVALID_HANDLE(ioHandle);
    BAIL_ON_INVALID_HANDLE(lexHandle);
    BAIL_ON_INVALID_HANDLE(pRetToken);
    *pRetToken = REGLEX_FIRST;

    if (lexHandle->isToken &&
        lexHandle->curToken.token != REGLEX_HEXPAIR &&
        lexHandle->curToken.token != REGLEX_REG_BINARY)
    {
        if ( lexHandle->curToken.token == REGLEX_REG_DWORD)
        {
            lexHandle->state = REGLEX_FIRST;
        }
        lexHandle->isToken = FALSE;
        lexHandle->curToken.token = REGLEX_FIRST;
        lexHandle->curToken.valueCursor = 0;
    }

    /* Return pushed back token, if one is present */
    if (lexHandle->prevToken.token != REGLEX_FIRST)
    {
        lexHandle->curToken = lexHandle->prevToken;
        lexHandle->prevToken.token = REGLEX_FIRST;
        *pRetToken = lexHandle->curToken.token;
        return dwError;
    }

    if (lexHandle->state == REGLEX_STATE_INIT ||
        lexHandle->curToken.token == REGLEX_HEXPAIR ||
        lexHandle->curToken.token == REGLEX_HEXPAIR_END)
    {
        lexHandle->curToken.valueCursor = 0;
    }

    if (!lexHandle->curToken.pszValue)
    {   /* Extra byte for string termination */
        LWREG_SAFE_FREE_MEMORY(lexHandle->curToken.pszValue);
        dwError = RegAllocateMemory(REGLEX_DEFAULT_SZ_LEN + 1,
                                    (LW_PVOID) &lexHandle->curToken.pszValue);
        BAIL_ON_REG_ERROR(dwError);

        lexHandle->curToken.valueCursor = 0;
        lexHandle->curToken.valueSize = REGLEX_DEFAULT_SZ_LEN;
    }

    do
    {
        lexHandle->isToken = FALSE;
        dwError = RegIOGetChar(ioHandle, &inC, &eof);
        if (eof)
        {
            if (lexHandle->curToken.token != REGLEX_FIRST && 
                lexHandle->curToken.valueCursor > 0)
            {
                lexHandle->isToken = TRUE;
                *pRetToken = lexHandle->curToken.token;
            }
            else
            {
                if (lexHandle->state == REGLEX_STATE_IN_QUOTE)
                {
                    dwError = LWREG_ERROR_UNEXPECTED_TOKEN;
                }
                else if (lexHandle->state == REGLEX_STATE_IN_KEY)
                {
                    lexHandle->isToken = TRUE;
                    lexHandle->curToken.token = REGLEX_REG_KEY;
                    lexHandle->state = REGLEX_STATE_INIT;
                    *pRetToken = lexHandle->curToken.token;
                    *pEof = 0;
                    break;
                }
                *pEof = eof;
            }
            break;
        }

        dwError = lexHandle->parseFuncArray[REGLEX_CHAR_INDEX(inC)](
                        lexHandle,
                        ioHandle,
                        inC);
        BAIL_ON_REG_ERROR(dwError);

        if (lexHandle->isToken)
        {
            *pRetToken = lexHandle->curToken.token;
            break;
        }
    }
    while (dwError == ERROR_SUCCESS);

cleanup:
    return dwError;

error:
    goto cleanup;
}
