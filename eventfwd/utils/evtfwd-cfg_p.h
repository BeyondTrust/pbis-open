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
 *        evtfwd-cfg_p.h
 *
 * Abstract:
 *
 *        Event forwarder from eventlogd to collector service
 * 
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#ifndef __EFD_CFG_P_H__
#define __EFD_CFG_P_H__

#define EFD_CFG_TOKEN_DEFAULT_LENGTH 128


typedef struct __EFD_CONFIG_WRAPPER_STATE
{
    PEFD_CONFIG_SETTINGS pSettings;
    PCSTR pszCurrentSection;
} EFD_CONFIG_WRAPPER_STATE, *PEFD_CONFIG_WRAPPER_STATE;

typedef struct __EFD_CONFIG_PARSE_STATE
{
    PSTR  pszFilePath;
    
    PVOID pData;
    
    DWORD dwOptions;
    
    FILE* fp;
    
    DWORD dwLine;
    DWORD dwCol;

    BOOLEAN bSkipSection;
    
    PSTR pszSectionName;
    
    PEFD_STACK pLexerTokenStack; //only for lexer
    
    PFNEFD_CONFIG_START_SECTION   pfnStartSectionHandler;
    PFNEFD_CONFIG_COMMENT         pfnCommentHandler;
    PFNEFD_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler;
    PFNEFD_CONFIG_END_SECTION     pfnEndSectionHandler;
    
} EFD_CONFIG_PARSE_STATE, *PEFD_CONFIG_PARSE_STATE;

typedef enum
{
    Consume,
    Pushback,
    Skip
} EfdCfgLexAction;

typedef enum
{
    EfdLexBegin = 0,
    EfdLexChar,
    EfdLexLSqBrace,
    EfdLexRSqBrace,
    EfdLexEquals,
    EfdLexHash,
    EfdLexNewline,
    EfdLexOther,
    EfdLexEOF,
    EfdLexEnd
} EfdCfgLexState;

typedef enum
{
    EfdCfgNone = 0,
    EfdCfgString,
    EfdCfgHash,
    EfdCfgNewline,
    EfdCfgEquals,
    EfdCfgRightSquareBrace,
    EfdCfgLeftSquareBrace,
    EfdCfgOther,
    EfdCfgEOF
} EfdCfgTokenType;

typedef struct __EFD_CFG_TOKEN
{
    EfdCfgTokenType tokenType;
    PSTR            pszToken;
    DWORD           dwMaxLen;
    DWORD           dwLen;
} EFD_CFG_TOKEN, *PEFD_CFG_TOKEN;

typedef struct __EFD_CFG_LEXER_STATE
{
    EfdCfgLexState  nextState;
    EfdCfgLexAction action;
    EfdCfgTokenType tokenId;
} EFD_CFG_LEXER_STATE, *PEFD_CFG_LEXER_STATE;

DWORD
EfdCfgInitParseState(
    PCSTR                            pszFilePath,
    DWORD                            dwOptions,
    PFNEFD_CONFIG_START_SECTION   pfnStartSectionHandler,
    PFNEFD_CONFIG_COMMENT         pfnCommentHandler,
    PFNEFD_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNEFD_CONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                            pData,
    PEFD_CONFIG_PARSE_STATE*      ppParseState
    );

VOID
EfdCfgFreeParseState(
    PEFD_CONFIG_PARSE_STATE pParseState
    );

DWORD
EfdCfgParse(
    PEFD_CONFIG_PARSE_STATE pParseState
    );

DWORD
EfdCfgParseSections(
    PEFD_CONFIG_PARSE_STATE pParseState
    );

DWORD
EfdCfgParseComment(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    );

DWORD
EfdCfgParseSectionHeader(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
EfdAssertWhitespaceOnly(
    PEFD_CONFIG_PARSE_STATE pParseState
    );

DWORD
EfdCfgParseNameValuePair(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
EfdCfgProcessComment(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PEFD_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
EfdCfgProcessBeginSection(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PEFD_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
EfdCfgProcessNameValuePair(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PEFD_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
EfdCfgProcessEndSection(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    );

DWORD
EfdCfgDetermineTokenLength(
    PEFD_STACK pStack
    );

//this will consume the token stack
DWORD
EfdCfgProcessTokenStackIntoString(
    PEFD_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    );

DWORD
EfdCfgAllocateToken(
    DWORD           dwSize,
    PEFD_CFG_TOKEN* ppToken
    );

DWORD
EfdCfgReallocToken(
    PEFD_CFG_TOKEN pToken,
    DWORD          dwNewSize
    );

VOID
EfdCfgResetToken(
    PEFD_CFG_TOKEN pToken
    );

DWORD
EfdCfgCopyToken(
    PEFD_CFG_TOKEN pTokenSrc,
    PEFD_CFG_TOKEN pTokenDst
    );

DWORD
EfdCfgFreeTokenStack(
    PEFD_STACK* ppTokenStack
    );

DWORD
EfdCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    );

VOID
EfdCfgFreeToken(
    PEFD_CFG_TOKEN pToken
    );

DWORD
EfdCfgGetNextToken(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PEFD_CFG_TOKEN*         ppToken
    );

DWORD
EfdCfgGetCharacter(
    PEFD_CONFIG_PARSE_STATE pParseState
    );

EfdCfgLexState
EfdCfgGetLexClass(
    DWORD ch
    );

DWORD
EfdCfgPushBackCharacter(
    PEFD_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    );

EfdCfgLexState
EfdCfgGetNextLexState(
    EfdCfgLexState currentState,
    DWORD chId
    );

EfdCfgLexAction
EfdCfgGetLexAction(
    EfdCfgLexState currentState,
    DWORD chId
    );

EfdCfgTokenType
EfdCfgGetTokenType(
    EfdCfgLexState currentState,
    DWORD chId
    );



#endif /* __EFD_CFG_P_H__ */
