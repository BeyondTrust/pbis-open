/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        cfg.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LWMGMTSS)
 *
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "includes.h"


static LWMGMT_CFG_LEXER_STATE gLWMGMTLexStateTable[][9] =
{
    /* LWMGMTLexBegin    := 0 */
    {
    {   LWMGMTLexBegin,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexBegin    */
    {    LWMGMTLexChar,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexChar     */
    {     LWMGMTLexEnd,  Consume,  LWMGMTCfgLeftSquareBrace }, /* LWMGMTLexLSqBrace */
    {     LWMGMTLexEnd,  Consume, LWMGMTCfgRightSquareBrace }, /* LWMGMTLexRSqBrace */
    {     LWMGMTLexEnd,  Consume,           LWMGMTCfgEquals }, /* LWMGMTLexEquals   */
    {     LWMGMTLexEnd,  Consume,             LWMGMTCfgHash }, /* LWMGMTLexHash     */
    {     LWMGMTLexEnd,  Consume,          LWMGMTCfgNewline }, /* LWMGMTLexNewline  */
    {   LWMGMTLexBegin,     Skip,             LWMGMTCfgNone }, /* LWMGMTLexOther    */
    {     LWMGMTLexEnd,  Consume,              LWMGMTCfgEOF }  /* LWMGMTLexEOF      */
    },
    /* LWMGMTLexChar     := 1 */
    {
    {   LWMGMTLexBegin,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexBegin    */
    {    LWMGMTLexChar,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexChar     */
    {     LWMGMTLexEnd, Pushback,           LWMGMTCfgString }, /* LWMGMTLexLSqBrace */
    {     LWMGMTLexEnd, Pushback,           LWMGMTCfgString }, /* LWMGMTLexRSqBrace */
    {     LWMGMTLexEnd, Pushback,           LWMGMTCfgString }, /* LWMGMTLexEquals   */
    {     LWMGMTLexEnd, Pushback,           LWMGMTCfgString }, /* LWMGMTLexHash     */
    {     LWMGMTLexEnd, Pushback,           LWMGMTCfgString }, /* LWMGMTLexNewline  */
    {     LWMGMTLexEnd,     Skip,           LWMGMTCfgString }, /* LWMGMTLexOther    */
    {     LWMGMTLexEnd,     Skip,           LWMGMTCfgString }  /* LWMGMTLexEOF      */
    },
    /* LWMGMTLexLSqBrace := 2 */
    {
    {   LWMGMTLexBegin,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexBegin    */
    {     LWMGMTLexEnd, Pushback,  LWMGMTCfgLeftSquareBrace }, /* LWMGMTLexChar     */
    {     LWMGMTLexEnd, Pushback,  LWMGMTCfgLeftSquareBrace }, /* LWMGMTLexLSqBrace */
    {     LWMGMTLexEnd, Pushback,  LWMGMTCfgLeftSquareBrace }, /* LWMGMTLexRSqBrace */
    {     LWMGMTLexEnd, Pushback,  LWMGMTCfgLeftSquareBrace }, /* LWMGMTLexEquals   */
    {     LWMGMTLexEnd, Pushback,  LWMGMTCfgLeftSquareBrace }, /* LWMGMTLexHash     */
    {     LWMGMTLexEnd, Pushback,  LWMGMTCfgLeftSquareBrace }, /* LWMGMTLexNewline  */
    {     LWMGMTLexEnd,     Skip,  LWMGMTCfgLeftSquareBrace }, /* LWMGMTLexOther    */
    {     LWMGMTLexEnd,     Skip,  LWMGMTCfgLeftSquareBrace }  /* LWMGMTLexEOF      */
    },
    /* LWMGMTLexRSqBrace := 3 */
    {
    {   LWMGMTLexBegin,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexBegin    */
    {     LWMGMTLexEnd, Pushback, LWMGMTCfgRightSquareBrace }, /* LWMGMTLexChar     */
    {     LWMGMTLexEnd, Pushback, LWMGMTCfgRightSquareBrace }, /* LWMGMTLexLSqBrace */
    {     LWMGMTLexEnd, Pushback, LWMGMTCfgRightSquareBrace }, /* LWMGMTLexRSqBrace */
    {     LWMGMTLexEnd, Pushback, LWMGMTCfgRightSquareBrace }, /* LWMGMTLexEquals   */
    {     LWMGMTLexEnd, Pushback, LWMGMTCfgRightSquareBrace }, /* LWMGMTLexHash     */
    {     LWMGMTLexEnd, Pushback, LWMGMTCfgRightSquareBrace }, /* LWMGMTLexNewline  */
    {     LWMGMTLexEnd,     Skip, LWMGMTCfgRightSquareBrace }, /* LWMGMTLexOther    */
    {     LWMGMTLexEnd,     Skip, LWMGMTCfgRightSquareBrace }  /* LWMGMTLexEOF      */
    },
    /* LWMGMTLexEquals   := 4 */
    {
    {   LWMGMTLexBegin,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexBegin    */
    {     LWMGMTLexEnd, Pushback,           LWMGMTCfgEquals }, /* LWMGMTLexChar     */
    {     LWMGMTLexEnd, Pushback,           LWMGMTCfgEquals }, /* LWMGMTLexLSqBrace */
    {     LWMGMTLexEnd, Pushback,           LWMGMTCfgEquals }, /* LWMGMTLexRSqBrace */
    {     LWMGMTLexEnd, Pushback,           LWMGMTCfgEquals }, /* LWMGMTLexEquals   */
    {     LWMGMTLexEnd, Pushback,           LWMGMTCfgEquals }, /* LWMGMTLexHash     */
    {     LWMGMTLexEnd, Pushback,           LWMGMTCfgEquals }, /* LWMGMTLexNewline  */
    {     LWMGMTLexEnd,     Skip,           LWMGMTCfgEquals }, /* LWMGMTLexOther    */
    {     LWMGMTLexEnd,     Skip,           LWMGMTCfgEquals }  /* LWMGMTLexEOF      */
    },
    /* LWMGMTLexHash     := 5 */
    {
    {   LWMGMTLexBegin,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexBegin    */
    {     LWMGMTLexEnd, Pushback,             LWMGMTCfgHash }, /* LWMGMTLexChar     */
    {     LWMGMTLexEnd, Pushback,             LWMGMTCfgHash }, /* LWMGMTLexLSqBrace */
    {     LWMGMTLexEnd, Pushback,             LWMGMTCfgHash }, /* LWMGMTLexRSqBrace */
    {     LWMGMTLexEnd, Pushback,             LWMGMTCfgHash }, /* LWMGMTLexEquals   */
    {     LWMGMTLexEnd, Pushback,             LWMGMTCfgHash }, /* LWMGMTLexHash     */
    {     LWMGMTLexEnd, Pushback,             LWMGMTCfgHash }, /* LWMGMTLexNewline  */
    {     LWMGMTLexEnd,     Skip,             LWMGMTCfgHash }, /* LWMGMTLexOther    */
    {     LWMGMTLexEnd,     Skip,             LWMGMTCfgHash }  /* LWMGMTLexEOF      */
    },
    /* LWMGMTLexNewline  := 6 */
    {
    {   LWMGMTLexBegin,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexBegin    */
    {     LWMGMTLexEnd, Pushback,          LWMGMTCfgNewline }, /* LWMGMTLexChar     */
    {     LWMGMTLexEnd, Pushback,          LWMGMTCfgNewline }, /* LWMGMTLexLSqBrace */
    {     LWMGMTLexEnd, Pushback,          LWMGMTCfgNewline }, /* LWMGMTLexRSqBrace */
    {     LWMGMTLexEnd, Pushback,          LWMGMTCfgNewline }, /* LWMGMTLexEquals   */
    {     LWMGMTLexEnd, Pushback,          LWMGMTCfgNewline }, /* LWMGMTLexHash     */
    {     LWMGMTLexEnd, Pushback,          LWMGMTCfgNewline }, /* LWMGMTLexNewline  */
    {     LWMGMTLexEnd,     Skip,          LWMGMTCfgNewline }, /* LWMGMTLexOther    */
    {     LWMGMTLexEnd,     Skip,          LWMGMTCfgNewline }  /* LWMGMTLexEOF      */
    },
    /* LWMGMTLexOther    := 7 */
    {
    {   LWMGMTLexBegin,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexBegin    */
    {    LWMGMTLexChar,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexChar     */
    {LWMGMTLexLSqBrace,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexLSqBrace */
    {LWMGMTLexRSqBrace,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexRSqBrace */
    {  LWMGMTLexEquals,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexEquals   */
    {    LWMGMTLexHash,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexHash     */
    { LWMGMTLexNewline,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexNewline  */
    {   LWMGMTLexOther,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexOther    */
    {     LWMGMTLexEOF,  Consume,             LWMGMTCfgNone }  /* LWMGMTLexEOF      */
    },
    /* LWMGMTLexEOF      := 8 */
    {
    {   LWMGMTLexBegin,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexBegin    */
    {     LWMGMTLexEnd,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexChar     */
    {     LWMGMTLexEnd,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexLSqBrace */
    {     LWMGMTLexEnd,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexRSqBrace */
    {     LWMGMTLexEnd,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexEquals   */
    {     LWMGMTLexEnd,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexHash     */
    { LWMGMTLexNewline,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexNewline  */
    {     LWMGMTLexEnd,  Consume,             LWMGMTCfgNone }, /* LWMGMTLexOther    */
    {     LWMGMTLexEnd,  Consume,             LWMGMTCfgNone }  /* LWMGMTLexEOF      */
    }
};

DWORD
LWMGMTParseConfigFile(
    PCSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler
    )
{
    DWORD dwError = 0;
    PLWMGMT_CONFIG_PARSE_STATE pParseState = NULL;

    dwError = LWMGMTCfgInitParseState(
                    pszFilePath,
                    pfnStartSectionHandler,
                    pfnCommentHandler,
                    pfnNameValuePairHandler,
                    pfnEndSectionHandler,
                    &pParseState);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LWMGMTCfgParse(pParseState);
    BAIL_ON_LWMGMT_ERROR(dwError);

cleanup:

    if (pParseState) {
        LWMGMTCfgFreeParseState(pParseState);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LWMGMTCfgInitParseState(
    PCSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler,
    PLWMGMT_CONFIG_PARSE_STATE*  ppParseState
    )
{
    DWORD dwError = 0;
    PLWMGMT_CONFIG_PARSE_STATE pParseState = NULL;
    PLWMGMTSTACK pTokenStack = NULL;
    FILE* fp = NULL;

    if ((fp = fopen(pszFilePath, "r")) == NULL) {
        dwError = errno;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }


    dwError = LWMGMTAllocateMemory(
                    sizeof(LWMGMT_CONFIG_PARSE_STATE),
                    (PVOID*)&pParseState);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LWMGMTAllocateMemory(
                    sizeof(LWMGMTSTACK),
                    (PVOID*)&pTokenStack);
    BAIL_ON_LWMGMT_ERROR(dwError);

    pParseState->pLexerTokenStack = pTokenStack;

    dwError = LWMGMTAllocateString(
                    pszFilePath,
                    &(pParseState->pszFilePath));
    BAIL_ON_LWMGMT_ERROR(dwError);

    pParseState->fp = fp;
    fp = NULL;

    pParseState->pfnStartSectionHandler =
        pfnStartSectionHandler;
    pParseState->pfnCommentHandler =
        pfnCommentHandler;
    pParseState->pfnNameValuePairHandler =
        pfnNameValuePairHandler;
    pParseState->pfnEndSectionHandler =
        pfnEndSectionHandler;

    pParseState->dwLine = 1;

    *ppParseState = pParseState;

cleanup:

    return dwError;

error:

    *ppParseState = NULL;

    if (pParseState) {
        LWMGMTCfgFreeParseState(pParseState);
    }

    if (fp) {
        fclose(fp);
    }

    goto cleanup;
}

VOID
LWMGMTCfgFreeParseState(
    PLWMGMT_CONFIG_PARSE_STATE pParseState
    )
{
    LWMGMT_SAFE_FREE_STRING(pParseState->pszFilePath);
    if (pParseState->fp) {
        fclose(pParseState->fp);
    }
    LWMGMTFreeMemory(pParseState);
}


DWORD
LWMGMTCfgParse(
    PLWMGMT_CONFIG_PARSE_STATE pParseState
    )
{

    DWORD dwError = 0;
    PLWMGMT_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PLWMGMTSTACK pTokenStack = NULL;

    do
    {

        dwError = LWMGMTCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LWMGMT_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case LWMGMTCfgHash:
            {
                dwError = LWMGMTCfgParseComment(
                                pParseState,
                                &bContinue);
                BAIL_ON_LWMGMT_ERROR(dwError);

                break;
            }
            case LWMGMTCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = LWMGMTCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LWMGMT_ERROR(dwError);

                break;
            }
            case LWMGMTCfgLeftSquareBrace:
            {

                dwError = LWMGMTCfgParseSections(
                                pParseState);
                BAIL_ON_LWMGMT_ERROR(dwError);

                break;
            }
            case LWMGMTCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = LWMGMT_ERROR_INVALID_CONFIG;
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
        }

    } while (bContinue);

cleanup:

    if (pToken) {
        LWMGMTCfgFreeToken(pToken);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LWMGMTCfgParseSections(
    PLWMGMT_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PLWMGMT_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PLWMGMTSTACK pTokenStack = NULL;

    dwError = LWMGMTCfgParseSectionHeader(
                    pParseState,
                    &bContinue);
    BAIL_ON_LWMGMT_ERROR(dwError);

    while (bContinue)
    {
        dwError = LWMGMTCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LWMGMT_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case LWMGMTCfgString:
            {

	      LWMGMTStripWhitespace(pToken->pszToken, TRUE, TRUE);

                if(!IsNullOrEmptyString(pToken->pszToken)) 
		{

                    dwError = LWMGMTStackPush(pToken, &(pParseState->pLexerTokenStack));
                    BAIL_ON_LWMGMT_ERROR(dwError);

                    pToken = NULL;

                    dwError = LWMGMTCfgParseNameValuePair(
                                    pParseState,
                                    &bContinue);
                    BAIL_ON_LWMGMT_ERROR(dwError);
		}
                break;
            }

            case LWMGMTCfgHash:
            {
                dwError = LWMGMTCfgParseComment(
                                pParseState,
                                &bContinue);
                BAIL_ON_LWMGMT_ERROR(dwError);
                break;
            }
            case LWMGMTCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = LWMGMTCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LWMGMT_ERROR(dwError);
                break;
            }
            case LWMGMTCfgLeftSquareBrace:
            {
                dwError = LWMGMTCfgParseSectionHeader(
                                pParseState,
                                &bContinue);
                BAIL_ON_LWMGMT_ERROR(dwError);

                break;
            }
            case LWMGMTCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = LWMGMT_ERROR_INVALID_CONFIG;
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
        }
    }

    if (bContinue && !IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = LWMGMTCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

cleanup:

    if (pToken) {
        LWMGMTCfgFreeToken(pToken);
    }

    return dwError;

error:

    goto cleanup;
}



DWORD
LWMGMTCfgParseComment(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PLWMGMT_CFG_TOKEN pToken = NULL;
    PLWMGMTSTACK pTokenStack = NULL;

    do
    {
        dwError = LWMGMTCfgGetNextToken(
                    pParseState,
                    &pToken);
        BAIL_ON_LWMGMT_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case LWMGMTCfgEOF:
            {
                dwError = LWMGMTCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LWMGMT_ERROR(dwError);

                bContinue = FALSE;

                break;
            }
            case LWMGMTCfgNewline:
            {
                dwError = LWMGMTCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_LWMGMT_ERROR(dwError);

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = LWMGMTStackPush(pToken, &pTokenStack);
                BAIL_ON_LWMGMT_ERROR(dwError);

                pToken = NULL;

            }
        }

    } while (bContinue && bKeepParsing);

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        LWMGMTCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LWMGMTCfgParseSectionHeader(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bKeepParsing = TRUE;
    BOOLEAN bContinue = TRUE;
    PLWMGMT_CFG_TOKEN pToken = NULL;
    PLWMGMTSTACK pTokenStack = NULL;

    if (!IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = LWMGMTCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_LWMGMT_ERROR(dwError);

    }

    if (!bContinue)
    {
        goto done;
    }

    pParseState->bSkipSection = FALSE;

    do
    {
        dwError = LWMGMTCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LWMGMT_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case LWMGMTCfgString:
            case LWMGMTCfgEquals:
            case LWMGMTCfgOther:
            {
                dwError = LWMGMTStackPush(pToken, &pTokenStack);
                BAIL_ON_LWMGMT_ERROR(dwError);

                pToken = NULL;
                break;
            }
            case LWMGMTCfgRightSquareBrace:
            {
                dwError = LWMGMTAssertWhitespaceOnly(pParseState);
                BAIL_ON_LWMGMT_ERROR(dwError);

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = LWMGMT_ERROR_INVALID_CONFIG;
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
        }

    } while (bKeepParsing);

    dwError = LWMGMTCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_LWMGMT_ERROR(dwError);

    switch(pToken->tokenType)
    {

        case LWMGMTCfgNewline:
        {
            dwError = LWMGMTCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_LWMGMT_ERROR(dwError);

            break;
        }
        case LWMGMTCfgEOF:
        {
            dwError = LWMGMTCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_LWMGMT_ERROR(dwError);

            if (bContinue) {

                dwError = LWMGMTCfgProcessEndSection(
                                pParseState,
                                &bContinue);
                BAIL_ON_LWMGMT_ERROR(dwError);
            }

            bContinue = FALSE;

            break;
        }
        default:
        {
            dwError = LWMGMT_ERROR_INVALID_CONFIG;
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
    }

done:

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        LWMGMTCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LWMGMTAssertWhitespaceOnly(
    PLWMGMT_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PLWMGMT_CFG_TOKEN pToken = NULL;
    BOOLEAN bKeepParsing = TRUE;

    do
    {
        dwError = LWMGMTCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LWMGMT_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case LWMGMTCfgString:
            case LWMGMTCfgOther:
            {
                DWORD i = 0;
                for (; i < pToken->dwLen; i++) {
                    if (!isspace((int)pToken->pszToken[i])) {
                        dwError = LWMGMT_ERROR_INVALID_CONFIG;
                        BAIL_ON_LWMGMT_ERROR(dwError);
                    }
                }
                break;
            }
            case LWMGMTCfgEOF:
            case LWMGMTCfgNewline:
            {
                dwError = LWMGMTStackPush(pToken, &pParseState->pLexerTokenStack);
                BAIL_ON_LWMGMT_ERROR(dwError);

                pToken = NULL;

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = LWMGMT_ERROR_INVALID_CONFIG;
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
        }
    } while (bKeepParsing);

cleanup:

    if (pToken) {
        LWMGMTCfgFreeToken(pToken);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LWMGMTCfgParseNameValuePair(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PLWMGMT_CFG_TOKEN pToken = NULL;
    PLWMGMTSTACK pTokenStack = NULL;

    //format is <str><equals><token1><token2>...<newline>

    //get initial <str>
    dwError = LWMGMTCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_LWMGMT_ERROR(dwError);

    if (pToken->tokenType == LWMGMTCfgString)
    {
        dwError = LWMGMTStackPush(pToken, &pTokenStack);
        BAIL_ON_LWMGMT_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = LWMGMT_ERROR_INVALID_CONFIG;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    //get <equals>
    dwError = LWMGMTCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_LWMGMT_ERROR(dwError);

    if (pToken->tokenType == LWMGMTCfgEquals)
    {
        dwError = LWMGMTStackPush(pToken, &pTokenStack);
        BAIL_ON_LWMGMT_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = LWMGMT_ERROR_INVALID_CONFIG;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }


    do
    {
        dwError = LWMGMTCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_LWMGMT_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case LWMGMTCfgString:
            case LWMGMTCfgEquals:
            case LWMGMTCfgOther:
            {

                dwError = LWMGMTStackPush(pToken, &pTokenStack);
                BAIL_ON_LWMGMT_ERROR(dwError);
                pToken = NULL;

                break;

            }
            case LWMGMTCfgNewline:
            {
                dwError = LWMGMTCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_LWMGMT_ERROR(dwError);
                bKeepParsing = FALSE;
                break;
            }
            case LWMGMTCfgEOF:
            {
                dwError = LWMGMTCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_LWMGMT_ERROR(dwError);
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = LWMGMT_ERROR_INVALID_CONFIG;
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
        }
    } while (bContinue && bKeepParsing);


    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        LWMGMTCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LWMGMTCfgProcessComment(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PLWMGMTSTACK* ppTokenStack,
    PBOOLEAN    pbContinue
    )
{
    DWORD   dwError = 0;
    BOOLEAN bContinue = TRUE;
    PSTR    pszComment = NULL;

    dwError = LWMGMTCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszComment);
    BAIL_ON_LWMGMT_ERROR(dwError);

    if (pParseState->pfnCommentHandler &&
        !pParseState->bSkipSection) {

        dwError = pParseState->pfnCommentHandler(
                        pszComment,
                        &bContinue);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    *pbContinue = bContinue;

cleanup:

    LWMGMT_SAFE_FREE_STRING(pszComment);

    return dwError;

error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
LWMGMTCfgProcessBeginSection(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PLWMGMTSTACK* ppTokenStack,
    PBOOLEAN    pbContinue)
{
    DWORD   dwError = 0;
    PSTR    pszSectionName = NULL;
    BOOLEAN bSkipSection = FALSE;
    BOOLEAN bContinue = TRUE;

    dwError = LWMGMTCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszSectionName);
    BAIL_ON_LWMGMT_ERROR(dwError);

    if (IsNullOrEmptyString(pszSectionName)) {
        dwError = LWMGMT_ERROR_INVALID_CONFIG;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    if (pParseState->pfnStartSectionHandler) {
        dwError = pParseState->pfnStartSectionHandler(
                        pszSectionName,
                        &bSkipSection,
                        &bContinue);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    pParseState->pszSectionName = pszSectionName;
    pParseState->bSkipSection = bSkipSection;
    *pbContinue = bContinue;

cleanup:

    return dwError;

error:

    LWMGMT_SAFE_FREE_STRING(pszSectionName);
    pParseState->pszSectionName = NULL;
    pParseState->bSkipSection = FALSE;
    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LWMGMTCfgProcessNameValuePair(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PLWMGMTSTACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    )
{
    DWORD      dwError = 0;
    BOOLEAN    bContinue = TRUE;
    PSTR       pszName = NULL;
    PSTR       pszValue = NULL;
    PLWMGMT_CFG_TOKEN pToken = NULL;

    *ppTokenStack = LWMGMTStackReverse(*ppTokenStack);
    pToken = (PLWMGMT_CFG_TOKEN)LWMGMTStackPop(ppTokenStack);
    if (pToken && pToken->dwLen) {
        dwError = LWMGMTStrndup(
                    pToken->pszToken,
                    pToken->dwLen,
                    &pszName);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    if (IsNullOrEmptyString(pszName)) {
        dwError = LWMGMT_ERROR_INVALID_CONFIG;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    LWMGMTCfgFreeToken(pToken);
    pToken = NULL;

    pToken = (PLWMGMT_CFG_TOKEN)LWMGMTStackPop(ppTokenStack);
    if (!pToken || pToken->tokenType != LWMGMTCfgEquals)
    {
        dwError = LWMGMT_ERROR_INVALID_CONFIG;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    LWMGMTCfgFreeToken(pToken);
    pToken = NULL;

    //this will consume the token stack
    dwError = LWMGMTCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszValue);
    BAIL_ON_LWMGMT_ERROR(dwError);

    if (pParseState->pfnNameValuePairHandler  &&
        !pParseState->bSkipSection) {

        dwError = pParseState->pfnNameValuePairHandler(
                        pszName,
                        pszValue,
                        &bContinue);
        BAIL_ON_LWMGMT_ERROR(dwError);

    }

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        LWMGMTCfgFreeToken(pToken);
        pToken = NULL;
    }

    if (ppTokenStack && *ppTokenStack)
    {
        dwError = LWMGMTCfgFreeTokenStack(ppTokenStack);
    }

    LWMGMT_SAFE_FREE_STRING(pszName);
    LWMGMT_SAFE_FREE_STRING(pszValue);

    return dwError;

error:

    goto cleanup;
}

DWORD
LWMGMTCfgProcessEndSection(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;

    if (pParseState->pfnEndSectionHandler &&
        !pParseState->bSkipSection) {
        dwError = pParseState->pfnEndSectionHandler(
                        pParseState->pszSectionName,
                        &bContinue);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    *pbContinue = bContinue;

cleanup:

    LWMGMT_SAFE_FREE_STRING(pParseState->pszSectionName);

    return dwError;

error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
LWMGMTCfgDetermineTokenLength(
    PLWMGMTSTACK pStack
    )
{
    DWORD dwLen = 0;
    PLWMGMTSTACK pIter = pStack;

    for (; pIter; pIter = pIter->pNext)
    {
        PLWMGMT_CFG_TOKEN pToken = (PLWMGMT_CFG_TOKEN)pIter->pItem;
        if (pToken) {
            dwLen += pToken->dwLen;
        }
    }

    return dwLen;
}

//this will consume the token stack
DWORD
LWMGMTCfgProcessTokenStackIntoString(
    PLWMGMTSTACK* ppTokenStack,
    PSTR* ppszConcatenated
    )
{
    DWORD   dwError = 0;
    DWORD   dwRequiredTokenLen = 0;
    PSTR    pszConcatenated = NULL;

    dwRequiredTokenLen = LWMGMTCfgDetermineTokenLength(*ppTokenStack);

    if (dwRequiredTokenLen) {

        PSTR pszPos = NULL;
        PLWMGMT_CFG_TOKEN pToken = NULL;

        *ppTokenStack = LWMGMTStackReverse(*ppTokenStack);


        dwError = LWMGMTAllocateMemory(
                        (dwRequiredTokenLen + 1) * sizeof(CHAR),
                        (PVOID*)&pszConcatenated);
        BAIL_ON_LWMGMT_ERROR(dwError);


        pszPos = pszConcatenated;
        while (*ppTokenStack)
        {
            pToken = LWMGMTStackPop(ppTokenStack);
            if (pToken && pToken->dwLen) {

                strncpy(pszPos, pToken->pszToken, pToken->dwLen);
                pszPos += pToken->dwLen;

                LWMGMTCfgFreeToken(pToken);
                pToken = NULL;
            }
        }
    }

    *ppszConcatenated = pszConcatenated;

cleanup:

    return dwError;

error:

    LWMGMT_SAFE_FREE_STRING(pszConcatenated);

    *ppszConcatenated = NULL;

    goto cleanup;
}


DWORD
LWMGMTCfgAllocateToken(
    DWORD           dwSize,
    PLWMGMT_CFG_TOKEN* ppToken
    )
{
    DWORD dwError = 0;
    PLWMGMT_CFG_TOKEN pToken = NULL;
    DWORD dwMaxLen = (dwSize > 0 ? dwSize : LWMGMT_CFG_TOKEN_DEFAULT_LENGTH);


    dwError = LWMGMTAllocateMemory(
                    sizeof(LWMGMT_CFG_TOKEN),
                    (PVOID*)&pToken);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LWMGMTAllocateMemory(
                    dwMaxLen * sizeof(CHAR),
                    (PVOID*)&pToken->pszToken);
    BAIL_ON_LWMGMT_ERROR(dwError);


    pToken->tokenType = LWMGMTCfgNone;
    pToken->dwMaxLen = dwMaxLen;

    *ppToken = pToken;

cleanup:

    return dwError;

error:

    *ppToken = NULL;

    if (pToken) {
        LWMGMTCfgFreeToken(pToken);
    }

    goto cleanup;
}

DWORD
LWMGMTCfgReallocToken(
    PLWMGMT_CFG_TOKEN pToken,
    DWORD          dwNewSize
    )
{
    DWORD dwError = 0;

    dwError = LWMGMTReallocMemory(
                    pToken->pszToken,
                    (PVOID*)&pToken->pszToken,
                    dwNewSize);
    BAIL_ON_LWMGMT_ERROR(dwError);

    pToken->dwMaxLen = dwNewSize;

cleanup:

    return dwError;

error:

    goto cleanup;
}

VOID
LWMGMTCfgResetToken(
    PLWMGMT_CFG_TOKEN pToken
    )
{
    if (pToken && pToken->pszToken && pToken->dwMaxLen)
    {
        memset(pToken->pszToken, 0, pToken->dwMaxLen);
        pToken->dwLen = 0;
    }
}

DWORD
LWMGMTCfgCopyToken(
    PLWMGMT_CFG_TOKEN pTokenSrc,
    PLWMGMT_CFG_TOKEN pTokenDst
    )
{
    DWORD dwError = 0;

    pTokenDst->tokenType = pTokenSrc->tokenType;

    if (pTokenSrc->dwLen > pTokenDst->dwLen) {
        dwError = LWMGMTReallocMemory(
                        (PVOID)  pTokenDst->pszToken,
                        (PVOID*) &pTokenDst->pszToken,
                        (DWORD)  pTokenSrc->dwLen);
        BAIL_ON_LWMGMT_ERROR(dwError);

        pTokenDst->dwLen = pTokenSrc->dwLen;
        pTokenDst->dwMaxLen = pTokenDst->dwLen;
    }

    memset(pTokenDst->pszToken, 0, pTokenDst->dwLen);
    memcpy(pTokenDst->pszToken, pTokenSrc->pszToken, pTokenSrc->dwLen);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LWMGMTCfgFreeTokenStack(
    PLWMGMTSTACK* ppTokenStack
    )
{

    DWORD dwError = 0;

    PLWMGMTSTACK pTokenStack = *ppTokenStack;

    dwError = LWMGMTStackForeach(
            pTokenStack,
            &LWMGMTCfgFreeTokenInStack,
            NULL);
    BAIL_ON_LWMGMT_ERROR(dwError);

    LWMGMTStackFree(pTokenStack);

    *ppTokenStack = NULL;

error:

    return dwError;
}

DWORD
LWMGMTCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    )
{
    if (pToken)
    {
        LWMGMTCfgFreeToken((PLWMGMT_CFG_TOKEN)pToken);
    }

    return 0;
}

VOID
LWMGMTCfgFreeToken(
    PLWMGMT_CFG_TOKEN pToken
    )
{
    LWMGMT_SAFE_FREE_MEMORY(pToken->pszToken);
    LWMGMTFreeMemory(pToken);
}

DWORD
LWMGMTCfgGetNextToken(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    PLWMGMT_CFG_TOKEN*         ppToken
    )
{
    DWORD dwError = 0;
    LWMGMTCfgTokenType tokenType = LWMGMTCfgNone;
    LWMGMTCfgLexState  curLexState = LWMGMTLexBegin;
    PLWMGMT_CFG_TOKEN  pToken = NULL;
    BOOLEAN         bOwnToken = FALSE;

    if (LWMGMTStackPeek(pParseState->pLexerTokenStack) != NULL)
    {
        PLWMGMT_CFG_TOKEN pToken_input = *ppToken;

        pToken = (PLWMGMT_CFG_TOKEN)LWMGMTStackPop(&pParseState->pLexerTokenStack);
        bOwnToken = TRUE;

        if (pToken_input) {

            dwError = LWMGMTCfgCopyToken(pToken, pToken_input);
            BAIL_ON_LWMGMT_ERROR(dwError);

            LWMGMTCfgFreeToken(pToken);
            pToken = NULL;
            bOwnToken = FALSE;

        }

        goto done;
    }

    pToken = *ppToken;

    if (!pToken) {
        dwError = LWMGMTCfgAllocateToken(
                        0,
                        &pToken);
        BAIL_ON_LWMGMT_ERROR(dwError);

        bOwnToken = TRUE;
    }
    else
    {
        LWMGMTCfgResetToken(pToken);
    }

    while (curLexState != LWMGMTLexEnd)
    {
        DWORD ch = LWMGMTCfgGetCharacter(pParseState);
        LWMGMTCfgLexState lexClass = LWMGMTCfgGetLexClass(ch);

        if (lexClass != LWMGMTLexEOF) {
            pParseState->dwCol++;
        }

        if (ch == (DWORD)'\n') {
            pParseState->dwLine++;
            pParseState->dwCol = 0;
        }

        tokenType = LWMGMTCfgGetTokenType(curLexState, lexClass);

        switch(LWMGMTCfgGetLexAction(curLexState, lexClass))
        {
            case Skip:

                break;

            case Consume:

                if (pToken->dwLen >= pToken->dwMaxLen) {
                    dwError = LWMGMTCfgReallocToken(
                                    pToken,
                                    pToken->dwMaxLen + LWMGMT_CFG_TOKEN_DEFAULT_LENGTH);
                    BAIL_ON_LWMGMT_ERROR(dwError);
                }

                pToken->pszToken[pToken->dwLen++] = (BYTE)ch;

                break;

            case Pushback:

                pParseState->dwCol--;
                dwError = LWMGMTCfgPushBackCharacter(
                                pParseState,
                                (BYTE)ch);
                BAIL_ON_LWMGMT_ERROR(dwError);

                break;
        }

        curLexState = LWMGMTCfgGetNextLexState(curLexState, lexClass);
    }

    pToken->tokenType = tokenType;

done:

    if (bOwnToken) {
        *ppToken = pToken;
    }

cleanup:

    return dwError;

error:

    if (bOwnToken && pToken) {
        LWMGMTCfgFreeToken(pToken);
        *ppToken = NULL;
    }

    goto cleanup;
}

DWORD
LWMGMTCfgGetCharacter(
    PLWMGMT_CONFIG_PARSE_STATE pParseState
    )
{
    return getc(pParseState->fp);
}

LWMGMTCfgLexState
LWMGMTCfgGetLexClass(
    DWORD ch
    )
{

    if (ch == EOF) {
        return LWMGMTLexEOF;
    }

    if (ch == '\n') {
        return LWMGMTLexNewline;
    }

    if (ch == '[') {
        return LWMGMTLexLSqBrace;
    }

    if (ch == ']') {
        return LWMGMTLexRSqBrace;
    }

    if (ch == '=') {
        return LWMGMTLexEquals;
    }

    if (ch == '#') {
        return LWMGMTLexHash;
    }

    if (isalnum(ch) ||
        ispunct(ch) ||
        isblank(ch)) {
        return LWMGMTLexChar;
    }

    return LWMGMTLexOther;
}

DWORD
LWMGMTCfgPushBackCharacter(
    PLWMGMT_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    )
{
    return ((EOF == ungetc(ch, pParseState->fp)) ? errno : 0);
}

LWMGMTCfgLexState
LWMGMTCfgGetNextLexState(
    LWMGMTCfgLexState currentState,
    DWORD chId
    )
{
    return (gLWMGMTLexStateTable[currentState][chId].nextState);
}

LWMGMTLexAction
LWMGMTCfgGetLexAction(
    LWMGMTCfgLexState currentState,
    DWORD chId
    )
{
    return (gLWMGMTLexStateTable[currentState][chId].action);
}

LWMGMTCfgTokenType
LWMGMTCfgGetTokenType(
    LWMGMTCfgLexState currentState,
    DWORD chId
    )
{
    return (gLWMGMTLexStateTable[currentState][chId].tokenId);
}
