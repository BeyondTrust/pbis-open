/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        evtcfg_p.h
 *
 * Abstract:
 *
 *        Likewise Eventlog
 * 
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#ifndef __LWMGMTCFG_P_H__
#define __LWMGMTCFG_P_H__

#define LWMGMT_CFG_TOKEN_DEFAULT_LENGTH 128

typedef struct __LWMGMT_CONFIG_PARSE_STATE
{
    PSTR  pszFilePath;
    
    FILE* fp;
    
    DWORD dwLine;
    DWORD dwCol;
    
    BOOLEAN bSkipSection;
    
    PSTR pszSectionName;
    
    PLWMGMTSTACK pLexerTokenStack; //only for lexer
    
    PFNCONFIG_START_SECTION   pfnStartSectionHandler;
    PFNCONFIG_COMMENT         pfnCommentHandler;
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler;
    PFNCONFIG_END_SECTION     pfnEndSectionHandler;
    
} LWMGMT_CONFIG_PARSE_STATE, *PLWMGMT_CONFIG_PARSE_STATE;

typedef enum
{
    Consume,
    Pushback,
    Skip
} LWMGMTLexAction;

typedef enum
{
    LWMGMTLexBegin = 0,
    LWMGMTLexChar,
    LWMGMTLexLSqBrace,
    LWMGMTLexRSqBrace,
    LWMGMTLexEquals,
    LWMGMTLexHash,
    LWMGMTLexNewline,
    LWMGMTLexOther,
    LWMGMTLexEOF,
    LWMGMTLexEnd
} LWMGMTCfgLexState;

typedef enum
{
    LWMGMTCfgNone = 0,
    LWMGMTCfgString,
    LWMGMTCfgHash,
    LWMGMTCfgNewline,
    LWMGMTCfgEquals,
    LWMGMTCfgRightSquareBrace,
    LWMGMTCfgLeftSquareBrace,
    LWMGMTCfgOther,
    LWMGMTCfgEOF
} LWMGMTCfgTokenType;

typedef enum {
    PARSE_STATE_COMMENT       = 0,
    PARSE_STATE_START_SECTION,
    PARSE_STATE_SECTION_NAME,
    PARSE_STATE_SECTION,
    PARSE_STATE_NAME,
    PARSE_STATE_EQUALS,
    PARSE_STATE_VALUE
} LWMGMTCfgParseState;

typedef struct __LWMGMT_CFG_ELEMENT
{
    LWMGMTCfgParseState parseState;
    PSTR             pszToken;
    DWORD            dwMaxLen;
    DWORD            dwLen;
} LWMGMT_CFG_ELEMENT, *PLWMGMT_CFG_ELEMENT;

typedef struct __LWMGMT_CFG_TOKEN
{
    LWMGMTCfgTokenType tokenType;
    PSTR            pszToken;
    DWORD           dwMaxLen;
    DWORD           dwLen;
} LWMGMT_CFG_TOKEN, *PLWMGMT_CFG_TOKEN;

typedef struct __LWMGMT_CFG_LEXER_STATE
{
    LWMGMTCfgLexState  nextState;
    LWMGMTLexAction action;
    LWMGMTCfgTokenType tokenId;
} LWMGMT_CFG_LEXER_STATE, *PLWMGMT_CFG_LEXER_STATE;

DWORD
LWMGMTCfgInitParseState(
    PCSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler,
    PLWMGMT_CONFIG_PARSE_STATE*  ppParseState
    );

VOID
LWMGMTCfgFreeParseState(
    PLWMGMT_CONFIG_PARSE_STATE pParseState
    );

DWORD
LWMGMTCfgParse(
    PLWMGMT_CONFIG_PARSE_STATE pParseState
    );

DWORD
LWMGMTCfgParseSections(
    PLWMGMT_CONFIG_PARSE_STATE pParseState
    );

DWORD
LWMGMTCfgParseComment(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    );

DWORD
LWMGMTCfgParseSectionHeader(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
LWMGMTAssertWhitespaceOnly(
    PLWMGMT_CONFIG_PARSE_STATE pParseState
    );

DWORD
LWMGMTCfgParseNameValuePair(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
LWMGMTCfgProcessComment(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PLWMGMTSTACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
LWMGMTCfgProcessBeginSection(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PLWMGMTSTACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
LWMGMTCfgProcessNameValuePair(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PLWMGMTSTACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
LWMGMTCfgProcessEndSection(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    );

DWORD
LWMGMTCfgDetermineTokenLength(
    PLWMGMTSTACK pStack
    );

//this will consume the token stack
DWORD
LWMGMTCfgProcessTokenStackIntoString(
    PLWMGMTSTACK* ppTokenStack,
    PSTR* ppszConcatenated
    );

DWORD
LWMGMTCfgAllocateToken(
    DWORD           dwSize,
    PLWMGMT_CFG_TOKEN* ppToken
    );

DWORD
LWMGMTCfgReallocToken(
    PLWMGMT_CFG_TOKEN pToken,
    DWORD          dwNewSize
    );

VOID
LWMGMTCfgResetToken(
    PLWMGMT_CFG_TOKEN pToken
    );

DWORD
LWMGMTCfgCopyToken(
    PLWMGMT_CFG_TOKEN pTokenSrc,
    PLWMGMT_CFG_TOKEN pTokenDst
    );

DWORD
LWMGMTCfgFreeTokenStack(
    PLWMGMTSTACK* ppTokenStack
    );

DWORD
LWMGMTCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    );

VOID
LWMGMTCfgFreeToken(
    PLWMGMT_CFG_TOKEN pToken
    );

DWORD
LWMGMTCfgGetNextToken(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PLWMGMT_CFG_TOKEN*         ppToken
    );

DWORD
LWMGMTCfgGetCharacter(
    PLWMGMT_CONFIG_PARSE_STATE pParseState
    );

LWMGMTCfgLexState
LWMGMTCfgGetLexClass(
    DWORD ch
    );

DWORD
LWMGMTCfgPushBackCharacter(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    );

LWMGMTCfgLexState
LWMGMTCfgGetNextLexState(
    LWMGMTCfgLexState currentState,
    DWORD chId
    );

LWMGMTLexAction
LWMGMTCfgGetLexAction(
    LWMGMTCfgLexState currentState,
    DWORD chId
    );

LWMGMTCfgTokenType
LWMGMTCfgGetTokenType(
    LWMGMTCfgLexState currentState,
    DWORD chId
    );



#endif /* __LWMGMTCFG_P_H__ */
