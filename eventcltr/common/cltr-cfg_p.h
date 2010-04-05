/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        evtcfg_p.h
 *
 * Abwcstract:
 *
 *        Likewise Eventlog
 * 
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#ifndef __EVTCFG_P_H__
#define __EVTCFG_P_H__

#define CLTR_CFG_TOKEN_DEFAULT_LENGTH 128

typedef struct __CLTR_CONFIG_PARSE_STATE
{
    PWSTR  pszFilePath;
    
    FILE* fp;
    
    DWORD dwLine;
    DWORD dwCol;
    
    BOOLEAN bSkipSection;
    
    PWSTR pszSectionName;
    
    PCLTR_STACK pLexerTokenStack; //only for lexer
    
    PFNCONFIG_START_SECTION   pfnStartSectionHandler;
    PFNCONFIG_COMMENT         pfnCommentHandler;
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler;
    PFNCONFIG_END_SECTION     pfnEndSectionHandler;
    
} CLTR_CONFIG_PARSE_STATE, *PCLTR_CONFIG_PARSE_STATE;

typedef enum
{
    Consume,
    Pushback,
    Skip
} CltrCfgLexAction;

typedef enum
{
    CltrLexBegin = 0,
    CltrLexWCHAR,
    CltrLexLSqBrace,
    CltrLexRSqBrace,
    CltrLexEquals,
    CltrLexHash,
    CltrLexNewline,
    CltrLexOther,
    CltrLexEOF,
    CltrLexEnd
} CltrCfgLexState;

typedef enum
{
    CltrCfgNone = 0,
    CltrCfgString,
    CltrCfgHash,
    CltrCfgNewline,
    CltrCfgEquals,
    CltrCfgRightSquareBrace,
    CltrCfgLeftSquareBrace,
    CltrCfgOther,
    CltrCfgEOF
} CltrCfgTokenType;

typedef enum {
    PARSE_STATE_COMMENT       = 0,
    PARSE_STATE_START_SECTION,
    PARSE_STATE_SECTION_NAME,
    PARSE_STATE_SECTION,
    PARSE_STATE_NAME,
    PARSE_STATE_EQUALS,
    PARSE_STATE_VALUE
} CltrCfgParseState;

typedef struct __CLTR_CFG_ELEMENT
{
    CltrCfgParseState parseState;
    PWSTR             pszToken;
    DWORD            dwMaxLen;
    DWORD            dwLen;
} CLTR_CFG_ELEMENT, *PCLTR_CFG_ELEMENT;

typedef struct __CLTR_CFG_TOKEN
{
    CltrCfgTokenType tokenType;
    PWSTR            pszToken;
    DWORD           dwMaxLen;
    DWORD           dwLen;
} CLTR_CFG_TOKEN, *PCLTR_CFG_TOKEN;

typedef struct __CLTR_CFG_LEXER_STATE
{
    CltrCfgLexState  nextState;
    CltrCfgLexAction action;
    CltrCfgTokenType tokenId;
} CLTR_CFG_LEXER_STATE, *PCLTR_CFG_LEXER_STATE;

DWORD
CltrCfgInitParseState(
    PCWSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler,
    PCLTR_CONFIG_PARSE_STATE*  ppParseState
    );

VOID
CltrCfgFreeParseState(
    PCLTR_CONFIG_PARSE_STATE pParseState
    );

DWORD
CltrCfgParse(
    PCLTR_CONFIG_PARSE_STATE pParseState
    );

DWORD
CltrCfgParseSections(
    PCLTR_CONFIG_PARSE_STATE pParseState
    );

DWORD
CltrCfgParseComment(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    );

DWORD
CltrCfgParseSectionHeader(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
CltrAssertWhitespaceOnly(
    PCLTR_CONFIG_PARSE_STATE pParseState
    );

DWORD
CltrCfgParseNameValuePair(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    );

DWORD
CltrCfgProcessComment(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PCLTR_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
CltrCfgProcessBeginSection(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PCLTR_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
CltrCfgProcessNameValuePair(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PCLTR_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    );

DWORD
CltrCfgProcessEndSection(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    );

DWORD
CltrCfgDetermineTokenLength(
    PCLTR_STACK pStack
    );

//this will consume the token stack
DWORD
CltrCfgProcessTokenStackIntoString(
    PCLTR_STACK* ppTokenStack,
    PWSTR* ppszConcatenated
    );

DWORD
CltrCfgAllocateToken(
    DWORD           dwSize,
    PCLTR_CFG_TOKEN* ppToken
    );

DWORD
CltrCfgReallocToken(
    PCLTR_CFG_TOKEN pToken,
    DWORD          dwNewSize
    );

VOID
CltrCfgResetToken(
    PCLTR_CFG_TOKEN pToken
    );

DWORD
CltrCfgCopyToken(
    PCLTR_CFG_TOKEN pTokenSrc,
    PCLTR_CFG_TOKEN pTokenDst
    );

DWORD
CltrCfgFreeTokenStack(
    PCLTR_STACK* ppTokenStack
    );

DWORD
CltrCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    );

VOID
CltrCfgFreeToken(
    PCLTR_CFG_TOKEN pToken
    );

DWORD
CltrCfgGetNextToken(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PCLTR_CFG_TOKEN*         ppToken
    );

DWORD
CltrCfgGetWCHARacter(
    PCLTR_CONFIG_PARSE_STATE pParseState
    );

CltrCfgLexState
CltrCfgGetLexClass(
    DWORD ch
    );

DWORD
CltrCfgPushBackWCHARacter(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    );

CltrCfgLexState
CltrCfgGetNextLexState(
    CltrCfgLexState currentState,
    DWORD chId
    );

CltrCfgLexAction
CltrCfgGetLexAction(
    CltrCfgLexState currentState,
    DWORD chId
    );

CltrCfgTokenType
CltrCfgGetTokenType(
    CltrCfgLexState currentState,
    DWORD chId
    );



#endif /* __EVTCFG_P_H__ */
