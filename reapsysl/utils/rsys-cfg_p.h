/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        rsys-cfg_p.h
 *
 * Abstract:
 *
 *        Reaper for syslog
 * 
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#ifndef __RSYS_CFG_P_H__
#define __RSYS_CFG_P_H__

#define RSYS_CFG_TOKEN_DEFAULT_LENGTH 128

typedef struct __RSYS_CONFIG_PARSE_STATE
{
    PSTR  pszFilePath;
    
    PVOID pData;
    
    DWORD dwOptions;
    
    FILE* fp;
    
    DWORD dwLine;
    DWORD dwCol;

    BOOLEAN bSkipSection;
    
    PSTR pszSectionName;
    
    PRSYS_STACK pLexerTokenStack; //only for lexer
    
    PFNRSYS_CONFIG_START_SECTION   pfnStartSectionHandler;
    PFNRSYS_CONFIG_COMMENT         pfnCommentHandler;
    PFNRSYS_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler;
    PFNRSYS_CONFIG_END_SECTION     pfnEndSectionHandler;
    
} RSYS_CONFIG_PARSE_STATE, *PRSYS_CONFIG_PARSE_STATE;

typedef enum
{
    Consume,
    Pushback,
    Skip
} RSysCfgLexAction;

typedef enum
{
    RSysLexBegin = 0,
    RSysLexChar,
    RSysLexLSqBrace,
    RSysLexRSqBrace,
    RSysLexEquals,
    RSysLexHash,
    RSysLexNewline,
    RSysLexOther,
    RSysLexEOF,
    RSysLexEnd
} RSysCfgLexState;

typedef enum
{
    RSysCfgNone = 0,
    RSysCfgString,
    RSysCfgHash,
    RSysCfgNewline,
    RSysCfgEquals,
    RSysCfgRightSquareBrace,
    RSysCfgLeftSquareBrace,
    RSysCfgOther,
    RSysCfgEOF
} RSysCfgTokenType;

typedef struct __RSYS_CFG_TOKEN
{
    RSysCfgTokenType tokenType;
    PSTR            pszToken;
    DWORD           dwMaxLen;
    DWORD           dwLen;
} RSYS_CFG_TOKEN, *PRSYS_CFG_TOKEN;

typedef struct __RSYS_CFG_LEXER_STATE
{
    RSysCfgLexState  nextState;
    RSysCfgLexAction action;
    RSysCfgTokenType tokenId;
} RSYS_CFG_LEXER_STATE, *PRSYS_CFG_LEXER_STATE;

DWORD
RSysUnquoteString(
    PSTR pszInput
    );

DWORD
RSysCfgInitParseState(
    PCSTR                            pszFilePath,
    DWORD                            dwOptions,
    PFNRSYS_CONFIG_START_SECTION   pfnStartSectionHandler,
    PFNRSYS_CONFIG_COMMENT         pfnCommentHandler,
    PFNRSYS_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNRSYS_CONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                            pData,
    PRSYS_CONFIG_PARSE_STATE*      ppParseState
    );

VOID
RSysCfgFreeParseState(
    PRSYS_CONFIG_PARSE_STATE pParseState
    );

DWORD
RSysCfgParse(
    PRSYS_CONFIG_PARSE_STATE pParseState
    );

DWORD
RSysCfgParseSections(
    PRSYS_CONFIG_PARSE_STATE pParseState
    );

DWORD
RSysCfgParseComment(
    PRSYS_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    );

DWORD
RSysCfgParseSectionHeader(
    PRSYS_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
RSysAssertWhitespaceOnly(
    PRSYS_CONFIG_PARSE_STATE pParseState
    );

DWORD
RSysCfgParseNameValuePair(
    PRSYS_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
RSysCfgProcessComment(
    PRSYS_CONFIG_PARSE_STATE pParseState,
    PRSYS_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
RSysCfgProcessBeginSection(
    PRSYS_CONFIG_PARSE_STATE pParseState,
    PRSYS_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
RSysCfgProcessNameValuePair(
    PRSYS_CONFIG_PARSE_STATE pParseState,
    PRSYS_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
RSysCfgProcessEndSection(
    PRSYS_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    );

DWORD
RSysCfgDetermineTokenLength(
    PRSYS_STACK pStack
    );

//this will consume the token stack
DWORD
RSysCfgProcessTokenStackIntoString(
    PRSYS_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    );

DWORD
RSysCfgAllocateToken(
    DWORD           dwSize,
    PRSYS_CFG_TOKEN* ppToken
    );

DWORD
RSysCfgReallocToken(
    PRSYS_CFG_TOKEN pToken,
    DWORD          dwNewSize
    );

VOID
RSysCfgResetToken(
    PRSYS_CFG_TOKEN pToken
    );

DWORD
RSysCfgCopyToken(
    PRSYS_CFG_TOKEN pTokenSrc,
    PRSYS_CFG_TOKEN pTokenDst
    );

DWORD
RSysCfgFreeTokenStack(
    PRSYS_STACK* ppTokenStack
    );

DWORD
RSysCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    );

VOID
RSysCfgFreeToken(
    PRSYS_CFG_TOKEN pToken
    );

DWORD
RSysCfgGetNextToken(
    PRSYS_CONFIG_PARSE_STATE pParseState,
    PRSYS_CFG_TOKEN*         ppToken
    );

DWORD
RSysCfgGetCharacter(
    PRSYS_CONFIG_PARSE_STATE pParseState
    );

RSysCfgLexState
RSysCfgGetLexClass(
    DWORD ch
    );

DWORD
RSysCfgPushBackCharacter(
    PRSYS_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    );

RSysCfgLexState
RSysCfgGetNextLexState(
    RSysCfgLexState currentState,
    DWORD chId
    );

RSysCfgLexAction
RSysCfgGetLexAction(
    RSysCfgLexState currentState,
    DWORD chId
    );

RSysCfgTokenType
RSysCfgGetTokenType(
    RSysCfgLexState currentState,
    DWORD chId
    );

DWORD
RSysReadLine(
    PSTR* ppszLine,
    FILE *stream
    );

DWORD
RSysRegComp(
    regex_t* pRegex,
    PBOOLEAN pbCompiled,
    PCSTR pszExpr
    );

#endif /* __RSYS_CFG_P_H__ */
