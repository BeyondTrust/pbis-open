/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        evtcfg.c
 *
 * Abwcstract:
 *
 *        Likewise Security and Authentication Subsystem (CltrSS)
 *
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "cltr-base.h"
#include <ctype.h>
#include "cltr-cfg_p.h"


static CLTR_CFG_LEXER_STATE gCltrLexStateTable[][9] =
{
    /* CltrLexBegin    := 0 */
    {
    {   CltrLexBegin,  Consume,             CltrCfgNone }, /* CltrLexBegin    */
    {    CltrLexWCHAR,  Consume,             CltrCfgNone }, /* CltrLexWCHAR     */
    {     CltrLexEnd,  Consume,  CltrCfgLeftSquareBrace }, /* CltrLexLSqBrace */
    {     CltrLexEnd,  Consume, CltrCfgRightSquareBrace }, /* CltrLexRSqBrace */
    {     CltrLexEnd,  Consume,           CltrCfgEquals }, /* CltrLexEquals   */
    {     CltrLexEnd,  Consume,             CltrCfgHash }, /* CltrLexHash     */
    {     CltrLexEnd,  Consume,          CltrCfgNewline }, /* CltrLexNewline  */
    {   CltrLexBegin,     Skip,             CltrCfgNone }, /* CltrLexOther    */
    {     CltrLexEnd,  Consume,              CltrCfgEOF }  /* CltrLexEOF      */
    },
    /* CltrLexWCHAR     := 1 */
    {
    {   CltrLexBegin,  Consume,             CltrCfgNone }, /* CltrLexBegin    */
    {    CltrLexWCHAR,  Consume,             CltrCfgNone }, /* CltrLexWCHAR     */
    {     CltrLexEnd, Pushback,           CltrCfgString }, /* CltrLexLSqBrace */
    {     CltrLexEnd, Pushback,           CltrCfgString }, /* CltrLexRSqBrace */
    {     CltrLexEnd, Pushback,           CltrCfgString }, /* CltrLexEquals   */
    {     CltrLexEnd, Pushback,           CltrCfgString }, /* CltrLexHash     */
    {     CltrLexEnd, Pushback,           CltrCfgString }, /* CltrLexNewline  */
    {     CltrLexEnd,     Skip,           CltrCfgString }, /* CltrLexOther    */
    {     CltrLexEnd,     Skip,           CltrCfgString }  /* CltrLexEOF      */
    },
    /* CltrLexLSqBrace := 2 */
    {
    {   CltrLexBegin,  Consume,             CltrCfgNone }, /* CltrLexBegin    */
    {     CltrLexEnd, Pushback,  CltrCfgLeftSquareBrace }, /* CltrLexWCHAR     */
    {     CltrLexEnd, Pushback,  CltrCfgLeftSquareBrace }, /* CltrLexLSqBrace */
    {     CltrLexEnd, Pushback,  CltrCfgLeftSquareBrace }, /* CltrLexRSqBrace */
    {     CltrLexEnd, Pushback,  CltrCfgLeftSquareBrace }, /* CltrLexEquals   */
    {     CltrLexEnd, Pushback,  CltrCfgLeftSquareBrace }, /* CltrLexHash     */
    {     CltrLexEnd, Pushback,  CltrCfgLeftSquareBrace }, /* CltrLexNewline  */
    {     CltrLexEnd,     Skip,  CltrCfgLeftSquareBrace }, /* CltrLexOther    */
    {     CltrLexEnd,     Skip,  CltrCfgLeftSquareBrace }  /* CltrLexEOF      */
    },
    /* CltrLexRSqBrace := 3 */
    {
    {   CltrLexBegin,  Consume,             CltrCfgNone }, /* CltrLexBegin    */
    {     CltrLexEnd, Pushback, CltrCfgRightSquareBrace }, /* CltrLexWCHAR     */
    {     CltrLexEnd, Pushback, CltrCfgRightSquareBrace }, /* CltrLexLSqBrace */
    {     CltrLexEnd, Pushback, CltrCfgRightSquareBrace }, /* CltrLexRSqBrace */
    {     CltrLexEnd, Pushback, CltrCfgRightSquareBrace }, /* CltrLexEquals   */
    {     CltrLexEnd, Pushback, CltrCfgRightSquareBrace }, /* CltrLexHash     */
    {     CltrLexEnd, Pushback, CltrCfgRightSquareBrace }, /* CltrLexNewline  */
    {     CltrLexEnd,     Skip, CltrCfgRightSquareBrace }, /* CltrLexOther    */
    {     CltrLexEnd,     Skip, CltrCfgRightSquareBrace }  /* CltrLexEOF      */
    },
    /* CltrLexEquals   := 4 */
    {
    {   CltrLexBegin,  Consume,             CltrCfgNone }, /* CltrLexBegin    */
    {     CltrLexEnd, Pushback,           CltrCfgEquals }, /* CltrLexWCHAR     */
    {     CltrLexEnd, Pushback,           CltrCfgEquals }, /* CltrLexLSqBrace */
    {     CltrLexEnd, Pushback,           CltrCfgEquals }, /* CltrLexRSqBrace */
    {     CltrLexEnd, Pushback,           CltrCfgEquals }, /* CltrLexEquals   */
    {     CltrLexEnd, Pushback,           CltrCfgEquals }, /* CltrLexHash     */
    {     CltrLexEnd, Pushback,           CltrCfgEquals }, /* CltrLexNewline  */
    {     CltrLexEnd,     Skip,           CltrCfgEquals }, /* CltrLexOther    */
    {     CltrLexEnd,     Skip,           CltrCfgEquals }  /* CltrLexEOF      */
    },
    /* CltrLexHash     := 5 */
    {
    {   CltrLexBegin,  Consume,             CltrCfgNone }, /* CltrLexBegin    */
    {     CltrLexEnd, Pushback,             CltrCfgHash }, /* CltrLexWCHAR     */
    {     CltrLexEnd, Pushback,             CltrCfgHash }, /* CltrLexLSqBrace */
    {     CltrLexEnd, Pushback,             CltrCfgHash }, /* CltrLexRSqBrace */
    {     CltrLexEnd, Pushback,             CltrCfgHash }, /* CltrLexEquals   */
    {     CltrLexEnd, Pushback,             CltrCfgHash }, /* CltrLexHash     */
    {     CltrLexEnd, Pushback,             CltrCfgHash }, /* CltrLexNewline  */
    {     CltrLexEnd,     Skip,             CltrCfgHash }, /* CltrLexOther    */
    {     CltrLexEnd,     Skip,             CltrCfgHash }  /* CltrLexEOF      */
    },
    /* CltrLexNewline  := 6 */
    {
    {   CltrLexBegin,  Consume,             CltrCfgNone }, /* CltrLexBegin    */
    {     CltrLexEnd, Pushback,          CltrCfgNewline }, /* CltrLexWCHAR     */
    {     CltrLexEnd, Pushback,          CltrCfgNewline }, /* CltrLexLSqBrace */
    {     CltrLexEnd, Pushback,          CltrCfgNewline }, /* CltrLexRSqBrace */
    {     CltrLexEnd, Pushback,          CltrCfgNewline }, /* CltrLexEquals   */
    {     CltrLexEnd, Pushback,          CltrCfgNewline }, /* CltrLexHash     */
    {     CltrLexEnd, Pushback,          CltrCfgNewline }, /* CltrLexNewline  */
    {     CltrLexEnd,     Skip,          CltrCfgNewline }, /* CltrLexOther    */
    {     CltrLexEnd,     Skip,          CltrCfgNewline }  /* CltrLexEOF      */
    },
    /* CltrLexOther    := 7 */
    {
    {   CltrLexBegin,  Consume,             CltrCfgNone }, /* CltrLexBegin    */
    {    CltrLexWCHAR,  Consume,             CltrCfgNone }, /* CltrLexWCHAR     */
    {CltrLexLSqBrace,  Consume,             CltrCfgNone }, /* CltrLexLSqBrace */
    {CltrLexRSqBrace,  Consume,             CltrCfgNone }, /* CltrLexRSqBrace */
    {  CltrLexEquals,  Consume,             CltrCfgNone }, /* CltrLexEquals   */
    {    CltrLexHash,  Consume,             CltrCfgNone }, /* CltrLexHash     */
    { CltrLexNewline,  Consume,             CltrCfgNone }, /* CltrLexNewline  */
    {   CltrLexOther,  Consume,             CltrCfgNone }, /* CltrLexOther    */
    {     CltrLexEOF,  Consume,             CltrCfgNone }  /* CltrLexEOF      */
    },
    /* CltrLexEOF      := 8 */
    {
    {   CltrLexBegin,  Consume,             CltrCfgNone }, /* CltrLexBegin    */
    {     CltrLexEnd,  Consume,             CltrCfgNone }, /* CltrLexWCHAR     */
    {     CltrLexEnd,  Consume,             CltrCfgNone }, /* CltrLexLSqBrace */
    {     CltrLexEnd,  Consume,             CltrCfgNone }, /* CltrLexRSqBrace */
    {     CltrLexEnd,  Consume,             CltrCfgNone }, /* CltrLexEquals   */
    {     CltrLexEnd,  Consume,             CltrCfgNone }, /* CltrLexHash     */
    { CltrLexNewline,  Consume,             CltrCfgNone }, /* CltrLexNewline  */
    {     CltrLexEnd,  Consume,             CltrCfgNone }, /* CltrLexOther    */
    {     CltrLexEnd,  Consume,             CltrCfgNone }  /* CltrLexEOF      */
    }
};


//including it temporarily
#ifndef iswhitespace
int iswhitespace(int c)
{
    return c == '\t' || c == ' ';
}
#endif

DWORD
CltrParseConfigFile(
    PCWSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler
    )
{
    DWORD dwError = 0;
    PCLTR_CONFIG_PARSE_STATE pParseState = NULL;

    dwError = CltrCfgInitParseState(
                    pszFilePath,
                    pfnStartSectionHandler,
                    pfnCommentHandler,
                    pfnNameValuePairHandler,
                    pfnEndSectionHandler,
                    &pParseState);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrCfgParse(pParseState);
    BAIL_ON_CLTR_ERROR(dwError);

cleanup:

    if (pParseState) {
        CltrCfgFreeParseState(pParseState);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
CltrCfgInitParseState(
    PCWSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler,
    PCLTR_CONFIG_PARSE_STATE*  ppParseState
    )
{
    DWORD dwError = 0;
    PCLTR_CONFIG_PARSE_STATE pParseState = NULL;
    PCLTR_STACK pTokenStack = NULL;
    FILE* fp = NULL;

    if ((fp = fopen((const char*)pszFilePath, "r")) == NULL) {
        dwError = errno;
        BAIL_ON_CLTR_ERROR(dwError);
    }


    dwError = CltrAllocateMemory(
                    sizeof(CLTR_CONFIG_PARSE_STATE),
                    (PVOID*)&pParseState);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrAllocateMemory(
                    sizeof(CLTR_STACK),
                    (PVOID*)&pTokenStack);
    BAIL_ON_CLTR_ERROR(dwError);

    pParseState->pLexerTokenStack = pTokenStack;

    dwError = CltrAllocateString(
                    pszFilePath,
                    (PWCHAR*)(pParseState->pszFilePath));
    BAIL_ON_CLTR_ERROR(dwError);

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
        CltrCfgFreeParseState(pParseState);
    }

    if (fp) {
        fclose(fp);
    }

    goto cleanup;
}

VOID
CltrCfgFreeParseState(
    PCLTR_CONFIG_PARSE_STATE pParseState
    )
{
    CLTR_SAFE_FREE_STRING(pParseState->pszFilePath);
    if (pParseState->fp) {
        fclose(pParseState->fp);
    }
    CltrFreeMemory(pParseState);
}


DWORD
CltrCfgParse(
    PCLTR_CONFIG_PARSE_STATE pParseState
    )
{

    DWORD dwError = 0;
    PCLTR_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PCLTR_STACK pTokenStack = NULL;

    do
    {

        dwError = CltrCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_CLTR_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case CltrCfgHash:
            {
                dwError = CltrCfgParseComment(
                                pParseState,
                                &bContinue);
                BAIL_ON_CLTR_ERROR(dwError);

                break;
            }
            case CltrCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = CltrCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_CLTR_ERROR(dwError);

                break;
            }
            case CltrCfgLeftSquareBrace:
            {

                dwError = CltrCfgParseSections(
                                pParseState);
                BAIL_ON_CLTR_ERROR(dwError);

                break;
            }
            case CltrCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = CLTR_ERROR_INVALID_CONFIG;
                BAIL_ON_CLTR_ERROR(dwError);
            }
        }

    } while (bContinue);

cleanup:

    if (pToken) {
        CltrCfgFreeToken(pToken);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
CltrCfgParseSections(
    PCLTR_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PCLTR_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PCLTR_STACK pTokenStack = NULL;

    dwError = CltrCfgParseSectionHeader(
                    pParseState,
                    &bContinue);
    BAIL_ON_CLTR_ERROR(dwError);

    while (bContinue)
    {
        dwError = CltrCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_CLTR_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case CltrCfgString:
            {

          CltrStripWhitespace(pToken->pszToken, TRUE, TRUE);

                if(!IsNullOrEmptyString(pToken->pszToken)) 
        {

                    dwError = CltrStackPush(pToken, &(pParseState->pLexerTokenStack));
                    BAIL_ON_CLTR_ERROR(dwError);

                    pToken = NULL;

                    dwError = CltrCfgParseNameValuePair(
                                    pParseState,
                                    &bContinue);
                    BAIL_ON_CLTR_ERROR(dwError);
        }
                break;
            }

            case CltrCfgHash:
            {
                dwError = CltrCfgParseComment(
                                pParseState,
                                &bContinue);
                BAIL_ON_CLTR_ERROR(dwError);
                break;
            }
            case CltrCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = CltrCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_CLTR_ERROR(dwError);
                break;
            }
            case CltrCfgLeftSquareBrace:
            {
                dwError = CltrCfgParseSectionHeader(
                                pParseState,
                                &bContinue);
                BAIL_ON_CLTR_ERROR(dwError);

                break;
            }
            case CltrCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = CLTR_ERROR_INVALID_CONFIG;
                BAIL_ON_CLTR_ERROR(dwError);
            }
        }
    }

    if (bContinue && !IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = CltrCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_CLTR_ERROR(dwError);
    }

cleanup:

    if (pToken) {
        CltrCfgFreeToken(pToken);
    }

    return dwError;

error:

    goto cleanup;
}



DWORD
CltrCfgParseComment(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PCLTR_CFG_TOKEN pToken = NULL;
    PCLTR_STACK pTokenStack = NULL;

    do
    {
        dwError = CltrCfgGetNextToken(
                    pParseState,
                    &pToken);
        BAIL_ON_CLTR_ERROR(dwError);

        switch (pToken->tokenType)
        {
            case CltrCfgEOF:
            {
                dwError = CltrCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_CLTR_ERROR(dwError);

                bContinue = FALSE;

                break;
            }
            case CltrCfgNewline:
            {
                dwError = CltrCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_CLTR_ERROR(dwError);

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = CltrStackPush(pToken, &pTokenStack);
                BAIL_ON_CLTR_ERROR(dwError);

                pToken = NULL;

            }
        }

    } while (bContinue && bKeepParsing);

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        CltrCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
CltrCfgParseSectionHeader(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bKeepParsing = TRUE;
    BOOLEAN bContinue = TRUE;
    PCLTR_CFG_TOKEN pToken = NULL;
    PCLTR_STACK pTokenStack = NULL;

    if (!IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = CltrCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_CLTR_ERROR(dwError);

    }

    if (!bContinue)
    {
        goto done;
    }

    pParseState->bSkipSection = FALSE;

    do
    {
        dwError = CltrCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_CLTR_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case CltrCfgString:
            case CltrCfgEquals:
            case CltrCfgOther:
            {
                dwError = CltrStackPush(pToken, &pTokenStack);
                BAIL_ON_CLTR_ERROR(dwError);

                pToken = NULL;
                break;
            }
            case CltrCfgRightSquareBrace:
            {
                dwError = CltrAssertWhitespaceOnly(pParseState);
                BAIL_ON_CLTR_ERROR(dwError);

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = CLTR_ERROR_INVALID_CONFIG;
                BAIL_ON_CLTR_ERROR(dwError);
            }
        }

    } while (bKeepParsing);

    dwError = CltrCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_CLTR_ERROR(dwError);

    switch(pToken->tokenType)
    {

        case CltrCfgNewline:
        {
            dwError = CltrCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_CLTR_ERROR(dwError);

            break;
        }
        case CltrCfgEOF:
        {
            dwError = CltrCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_CLTR_ERROR(dwError);

            if (bContinue) {

                dwError = CltrCfgProcessEndSection(
                                pParseState,
                                &bContinue);
                BAIL_ON_CLTR_ERROR(dwError);
            }

            bContinue = FALSE;

            break;
        }
        default:
        {
            dwError = CLTR_ERROR_INVALID_CONFIG;
            BAIL_ON_CLTR_ERROR(dwError);
        }
    }

done:

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        CltrCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
CltrAssertWhitespaceOnly(
    PCLTR_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PCLTR_CFG_TOKEN pToken = NULL;
    BOOLEAN bKeepParsing = TRUE;

    do
    {
        dwError = CltrCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_CLTR_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case CltrCfgString:
            case CltrCfgOther:
            {
                DWORD i = 0;
                for (; i < pToken->dwLen; i++) {
                    if (!isspace((int)pToken->pszToken[i])) {
                        dwError = CLTR_ERROR_INVALID_CONFIG;
                        BAIL_ON_CLTR_ERROR(dwError);
                    }
                }
                break;
            }
            case CltrCfgEOF:
            case CltrCfgNewline:
            {
                dwError = CltrStackPush(pToken, &pParseState->pLexerTokenStack);
                BAIL_ON_CLTR_ERROR(dwError);

                pToken = NULL;

                bKeepParsing = FALSE;

                break;
            }
            default:
            {
                dwError = CLTR_ERROR_INVALID_CONFIG;
                BAIL_ON_CLTR_ERROR(dwError);
            }
        }
    } while (bKeepParsing);

cleanup:

    if (pToken) {
        CltrCfgFreeToken(pToken);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
CltrCfgParseNameValuePair(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PCLTR_CFG_TOKEN pToken = NULL;
    PCLTR_STACK pTokenStack = NULL;

    //format is <wcstr><equals><token1><token2>...<newline>

    //get initial <wcstr>
    dwError = CltrCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_CLTR_ERROR(dwError);

    if (pToken->tokenType == CltrCfgString)
    {
        dwError = CltrStackPush(pToken, &pTokenStack);
        BAIL_ON_CLTR_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = CLTR_ERROR_INVALID_CONFIG;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    //get <equals>
    dwError = CltrCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_CLTR_ERROR(dwError);

    if (pToken->tokenType == CltrCfgEquals)
    {
        dwError = CltrStackPush(pToken, &pTokenStack);
        BAIL_ON_CLTR_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = CLTR_ERROR_INVALID_CONFIG;
        BAIL_ON_CLTR_ERROR(dwError);
    }


    do
    {
        dwError = CltrCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_CLTR_ERROR(dwError);

        switch(pToken->tokenType)
        {
            case CltrCfgString:
            case CltrCfgEquals:
            case CltrCfgOther:
            {

                dwError = CltrStackPush(pToken, &pTokenStack);
                BAIL_ON_CLTR_ERROR(dwError);
                pToken = NULL;

                break;

            }
            case CltrCfgNewline:
            {
                dwError = CltrCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_CLTR_ERROR(dwError);
                bKeepParsing = FALSE;
                break;
            }
            case CltrCfgEOF:
            {
                dwError = CltrCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_CLTR_ERROR(dwError);
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = CLTR_ERROR_INVALID_CONFIG;
                BAIL_ON_CLTR_ERROR(dwError);
            }
        }
    } while (bContinue && bKeepParsing);


    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        CltrCfgFreeToken(pToken);
    }

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
CltrCfgProcessComment(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PCLTR_STACK* ppTokenStack,
    PBOOLEAN    pbContinue
    )
{
    DWORD   dwError = 0;
    BOOLEAN bContinue = TRUE;
    PWSTR    pszComment = NULL;

    dwError = CltrCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszComment);
    BAIL_ON_CLTR_ERROR(dwError);

    if (pParseState->pfnCommentHandler &&
        !pParseState->bSkipSection) {

        dwError = pParseState->pfnCommentHandler(
                        pszComment,
                        &bContinue);
        BAIL_ON_CLTR_ERROR(dwError);
    }

    *pbContinue = bContinue;

cleanup:

    CLTR_SAFE_FREE_STRING(pszComment);

    return dwError;

error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
CltrCfgProcessBeginSection(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PCLTR_STACK* ppTokenStack,
    PBOOLEAN    pbContinue)
{
    DWORD   dwError = 0;
    PWSTR    pszSectionName = NULL;
    BOOLEAN bSkipSection = FALSE;
    BOOLEAN bContinue = TRUE;

    dwError = CltrCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszSectionName);
    BAIL_ON_CLTR_ERROR(dwError);

    if (IsNullOrEmptyString(pszSectionName)) {
        dwError = CLTR_ERROR_INVALID_CONFIG;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    if (pParseState->pfnStartSectionHandler) {
        dwError = pParseState->pfnStartSectionHandler(
                        pszSectionName,
                        &bSkipSection,
                        &bContinue);
        BAIL_ON_CLTR_ERROR(dwError);
    }

    pParseState->pszSectionName = pszSectionName;
    pParseState->bSkipSection = bSkipSection;
    *pbContinue = bContinue;

cleanup:

    return dwError;

error:

    CLTR_SAFE_FREE_STRING(pszSectionName);
    pParseState->pszSectionName = NULL;
    pParseState->bSkipSection = FALSE;
    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
CltrCfgProcessNameValuePair(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PCLTR_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    )
{
    DWORD      dwError = 0;
    BOOLEAN    bContinue = TRUE;
    PWSTR       pszName = NULL;
    PWSTR       pszValue = NULL;
    PCLTR_CFG_TOKEN pToken = NULL;

    *ppTokenStack = CltrStackReverse(*ppTokenStack);
    pToken = (PCLTR_CFG_TOKEN)CltrStackPop(ppTokenStack);
    if (pToken && pToken->dwLen) {
        dwError = CltrStrndup(
                    pToken->pszToken,
                    pToken->dwLen,
                    &pszName);
        BAIL_ON_CLTR_ERROR(dwError);
    }

    if (IsNullOrEmptyString(pszName)) {
        dwError = CLTR_ERROR_INVALID_CONFIG;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    CltrCfgFreeToken(pToken);
    pToken = NULL;

    pToken = (PCLTR_CFG_TOKEN)CltrStackPop(ppTokenStack);
    if (!pToken || pToken->tokenType != CltrCfgEquals)
    {
        dwError = CLTR_ERROR_INVALID_CONFIG;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    CltrCfgFreeToken(pToken);
    pToken = NULL;

    //this will consume the token stack
    dwError = CltrCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszValue);
    BAIL_ON_CLTR_ERROR(dwError);

    if (pParseState->pfnNameValuePairHandler  &&
        !pParseState->bSkipSection) {

        dwError = pParseState->pfnNameValuePairHandler(
                        pszName,
                        pszValue,
                        &bContinue);
        BAIL_ON_CLTR_ERROR(dwError);

    }

    *pbContinue = bContinue;

cleanup:

    if (pToken) {
        CltrCfgFreeToken(pToken);
        pToken = NULL;
    }

    if (ppTokenStack && *ppTokenStack)
    {
        dwError = CltrCfgFreeTokenStack(ppTokenStack);
    }

    CLTR_SAFE_FREE_STRING(pszName);
    CLTR_SAFE_FREE_STRING(pszValue);

    return dwError;

error:

    goto cleanup;
}

DWORD
CltrCfgProcessEndSection(
    PCLTR_CONFIG_PARSE_STATE pParseState,
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
        BAIL_ON_CLTR_ERROR(dwError);
    }

    *pbContinue = bContinue;

cleanup:

    CLTR_SAFE_FREE_STRING(pParseState->pszSectionName);

    return dwError;

error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
CltrCfgDetermineTokenLength(
    PCLTR_STACK pStack
    )
{
    DWORD dwLen = 0;
    PCLTR_STACK pIter = pStack;

    for (; pIter; pIter = pIter->pNext)
    {
        PCLTR_CFG_TOKEN pToken = (PCLTR_CFG_TOKEN)pIter->pItem;
        if (pToken) {
            dwLen += pToken->dwLen;
        }
    }

    return dwLen;
}

//this will consume the token stack
DWORD
CltrCfgProcessTokenStackIntoString(
    PCLTR_STACK* ppTokenStack,
    PWSTR* ppszConcatenated
    )
{
    DWORD   dwError = 0;
    DWORD   dwRequiredTokenLen = 0;
    PWSTR    pszConcatenated = NULL;

    dwRequiredTokenLen = CltrCfgDetermineTokenLength(*ppTokenStack);

    if (dwRequiredTokenLen) {

        PWSTR pszPos = NULL;
        PCLTR_CFG_TOKEN pToken = NULL;

        *ppTokenStack = CltrStackReverse(*ppTokenStack);


        dwError = CltrAllocateMemory(
                        (dwRequiredTokenLen + 1) * sizeof(WCHAR),
                        (PVOID*)&pszConcatenated);
        BAIL_ON_CLTR_ERROR(dwError);


        pszPos = pszConcatenated;
        while (*ppTokenStack)
        {
            pToken = CltrStackPop(ppTokenStack);
            if (pToken && pToken->dwLen) {

                wc16sncpy(pszPos, pToken->pszToken, pToken->dwLen);
                pszPos += pToken->dwLen;

                CltrCfgFreeToken(pToken);
                pToken = NULL;
            }
        }
    }

    *ppszConcatenated = pszConcatenated;

cleanup:

    return dwError;

error:

    CLTR_SAFE_FREE_STRING(pszConcatenated);

    *ppszConcatenated = NULL;

    goto cleanup;
}


DWORD
CltrCfgAllocateToken(
    DWORD           dwSize,
    PCLTR_CFG_TOKEN* ppToken
    )
{
    DWORD dwError = 0;
    PCLTR_CFG_TOKEN pToken = NULL;
    DWORD dwMaxLen = (dwSize > 0 ? dwSize : CLTR_CFG_TOKEN_DEFAULT_LENGTH);


    dwError = CltrAllocateMemory(
                    sizeof(CLTR_CFG_TOKEN),
                    (PVOID*)&pToken);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrAllocateMemory(
                    dwMaxLen * sizeof(WCHAR),
                    (PVOID*)&pToken->pszToken);
    BAIL_ON_CLTR_ERROR(dwError);


    pToken->tokenType = CltrCfgNone;
    pToken->dwMaxLen = dwMaxLen;

    *ppToken = pToken;

cleanup:

    return dwError;

error:

    *ppToken = NULL;

    if (pToken) {
        CltrCfgFreeToken(pToken);
    }

    goto cleanup;
}

DWORD
CltrCfgReallocToken(
    PCLTR_CFG_TOKEN pToken,
    DWORD          dwNewSize
    )
{
    DWORD dwError = 0;

    dwError = CltrReallocMemory(
                    pToken->pszToken,
                    (PVOID*)&pToken->pszToken,
                    dwNewSize);
    BAIL_ON_CLTR_ERROR(dwError);

    pToken->dwMaxLen = dwNewSize;

cleanup:

    return dwError;

error:

    goto cleanup;
}

VOID
CltrCfgResetToken(
    PCLTR_CFG_TOKEN pToken
    )
{
    if (pToken && pToken->pszToken && pToken->dwMaxLen)
    {
        w16memset(pToken->pszToken, 0, pToken->dwMaxLen);
        pToken->dwLen = 0;
    }
}

DWORD
CltrCfgCopyToken(
    PCLTR_CFG_TOKEN pTokenSrc,
    PCLTR_CFG_TOKEN pTokenDst
    )
{
    DWORD dwError = 0;

    pTokenDst->tokenType = pTokenSrc->tokenType;

    if (pTokenSrc->dwLen > pTokenDst->dwLen) {
        dwError = CltrReallocMemory(
                        (PVOID)  pTokenDst->pszToken,
                        (PVOID*) &pTokenDst->pszToken,
                        (DWORD)  pTokenSrc->dwLen);
        BAIL_ON_CLTR_ERROR(dwError);

        pTokenDst->dwLen = pTokenSrc->dwLen;
        pTokenDst->dwMaxLen = pTokenDst->dwLen;
    }

    w16memset(pTokenDst->pszToken, 0, pTokenDst->dwLen);
    w16memcpy(pTokenDst->pszToken, pTokenSrc->pszToken, pTokenSrc->dwLen);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
CltrCfgFreeTokenStack(
    PCLTR_STACK* ppTokenStack
    )
{

    DWORD dwError = 0;

    PCLTR_STACK pTokenStack = *ppTokenStack;

    dwError = CltrStackForeach(
            pTokenStack,
            &CltrCfgFreeTokenInStack,
            NULL);
    BAIL_ON_CLTR_ERROR(dwError);

    CltrStackFree(pTokenStack);

    *ppTokenStack = NULL;

error:

    return dwError;
}

DWORD
CltrCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    )
{
    if (pToken)
    {
        CltrCfgFreeToken((PCLTR_CFG_TOKEN)pToken);
    }

    return 0;
}

VOID
CltrCfgFreeToken(
    PCLTR_CFG_TOKEN pToken
    )
{
    CLTR_SAFE_FREE_MEMORY(pToken->pszToken);
    CltrFreeMemory(pToken);
}

DWORD
CltrCfgGetNextToken(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    PCLTR_CFG_TOKEN*         ppToken
    )
{
    DWORD dwError = 0;
    CltrCfgTokenType tokenType = CltrCfgNone;
    CltrCfgLexState  curLexState = CltrLexBegin;
    PCLTR_CFG_TOKEN  pToken = NULL;
    BOOLEAN         bOwnToken = FALSE;

    if (CltrStackPeek(pParseState->pLexerTokenStack) != NULL)
    {
        PCLTR_CFG_TOKEN pToken_input = *ppToken;

        pToken = (PCLTR_CFG_TOKEN)CltrStackPop(&pParseState->pLexerTokenStack);
        bOwnToken = TRUE;

        if (pToken_input) {

            dwError = CltrCfgCopyToken(pToken, pToken_input);
            BAIL_ON_CLTR_ERROR(dwError);

            CltrCfgFreeToken(pToken);
            pToken = NULL;
            bOwnToken = FALSE;

        }

        goto done;
    }

    pToken = *ppToken;

    if (!pToken) {
        dwError = CltrCfgAllocateToken(
                        0,
                        &pToken);
        BAIL_ON_CLTR_ERROR(dwError);

        bOwnToken = TRUE;
    }
    else
    {
        CltrCfgResetToken(pToken);
    }

    while (curLexState != CltrLexEnd)
    {
        DWORD ch = CltrCfgGetWCHARacter(pParseState);
        CltrCfgLexState lexClass = CltrCfgGetLexClass(ch);

        if (lexClass != CltrLexEOF) {
            pParseState->dwCol++;
        }

        if (ch == (DWORD)'\n') {
            pParseState->dwLine++;
            pParseState->dwCol = 0;
        }

        tokenType = CltrCfgGetTokenType(curLexState, lexClass);

        switch(CltrCfgGetLexAction(curLexState, lexClass))
        {
            case Skip:

                break;

            case Consume:

                if (pToken->dwLen >= pToken->dwMaxLen) {
                    dwError = CltrCfgReallocToken(
                                    pToken,
                                    pToken->dwMaxLen + CLTR_CFG_TOKEN_DEFAULT_LENGTH);
                    BAIL_ON_CLTR_ERROR(dwError);
                }

                pToken->pszToken[pToken->dwLen++] = (BYTE)ch;

                break;

            case Pushback:

                pParseState->dwCol--;
                dwError = CltrCfgPushBackWCHARacter(
                                pParseState,
                                (BYTE)ch);
                BAIL_ON_CLTR_ERROR(dwError);

                break;
        }

        curLexState = CltrCfgGetNextLexState(curLexState, lexClass);
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
        CltrCfgFreeToken(pToken);
        *ppToken = NULL;
    }

    goto cleanup;
}

DWORD
CltrCfgGetWCHARacter(
    PCLTR_CONFIG_PARSE_STATE pParseState
    )
{
    return getc(pParseState->fp);
}

CltrCfgLexState
CltrCfgGetLexClass(
    DWORD ch
    )
{

    if (ch == EOF) {
        return CltrLexEOF;
    }

    if (ch == '\n') {
        return CltrLexNewline;
    }

    if (ch == '[') {
        return CltrLexLSqBrace;
    }

    if (ch == ']') {
        return CltrLexRSqBrace;
    }

    if (ch == '=') {
        return CltrLexEquals;
    }

    if (ch == '#') {
        return CltrLexHash;
    }

    if (isalnum(ch) ||
        ispunct(ch) ||
        iswhitespace(ch)) {
        return CltrLexWCHAR;
    }

    return CltrLexOther;
}

DWORD
CltrCfgPushBackWCHARacter(
    PCLTR_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    )
{
    return ((EOF == ungetc(ch, pParseState->fp)) ? errno : 0);
}

CltrCfgLexState
CltrCfgGetNextLexState(
    CltrCfgLexState currentState,
    DWORD chId
    )
{
    return (gCltrLexStateTable[currentState][chId].nextState);
}

CltrCfgLexAction
CltrCfgGetLexAction(
    CltrCfgLexState currentState,
    DWORD chId
    )
{
    return (gCltrLexStateTable[currentState][chId].action);
}

CltrCfgTokenType
CltrCfgGetTokenType(
    CltrCfgLexState currentState,
    DWORD chId
    )
{
    return (gCltrLexStateTable[currentState][chId].tokenId);
}
