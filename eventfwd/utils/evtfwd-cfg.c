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
 *        evtfwd-cfg.c
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
#include "includes.h"

static EFD_CFG_LEXER_STATE gEfdLexStateTable[][9] =
{
  /* EfdLexBegin    := 0 */
  {
    {   EfdLexBegin,  Consume,             EfdCfgNone }, /* EfdLexBegin    */
    {    EfdLexChar,  Consume,             EfdCfgNone }, /* EfdLexChar     */
    {     EfdLexEnd,  Consume,  EfdCfgLeftSquareBrace }, /* EfdLexLSqBrace */
    {     EfdLexEnd,  Consume, EfdCfgRightSquareBrace }, /* EfdLexRSqBrace */
    {     EfdLexEnd,  Consume,           EfdCfgEquals }, /* EfdLexEquals   */
    {     EfdLexEnd,  Consume,             EfdCfgHash }, /* EfdLexHash     */
    {     EfdLexEnd,  Consume,          EfdCfgNewline }, /* EfdLexNewline  */
    {   EfdLexBegin,     Skip,             EfdCfgNone }, /* EfdLexOther    */
    {     EfdLexEnd,  Consume,              EfdCfgEOF }  /* EfdLexEOF      */
  },
  /* EfdLexChar     := 1 */
  {
    {   EfdLexBegin,  Consume,             EfdCfgNone }, /* EfdLexBegin    */ 
    {    EfdLexChar,  Consume,             EfdCfgNone }, /* EfdLexChar     */
    {     EfdLexEnd, Pushback,           EfdCfgString }, /* EfdLexLSqBrace */
    {     EfdLexEnd, Pushback,           EfdCfgString }, /* EfdLexRSqBrace */
    {     EfdLexEnd, Pushback,           EfdCfgString }, /* EfdLexEquals   */
    {     EfdLexEnd, Pushback,           EfdCfgString }, /* EfdLexHash     */
    {     EfdLexEnd, Pushback,           EfdCfgString }, /* EfdLexNewline  */
    {     EfdLexEnd,     Skip,           EfdCfgString }, /* EfdLexOther    */
    {     EfdLexEnd,     Skip,           EfdCfgString }  /* EfdLexEOF      */
  },
  /* EfdLexLSqBrace := 2 */
  {
    {   EfdLexBegin,  Consume,             EfdCfgNone }, /* EfdLexBegin    */ 
    {     EfdLexEnd, Pushback,  EfdCfgLeftSquareBrace }, /* EfdLexChar     */ 
    {     EfdLexEnd, Pushback,  EfdCfgLeftSquareBrace }, /* EfdLexLSqBrace */
    {     EfdLexEnd, Pushback,  EfdCfgLeftSquareBrace }, /* EfdLexRSqBrace */
    {     EfdLexEnd, Pushback,  EfdCfgLeftSquareBrace }, /* EfdLexEquals   */
    {     EfdLexEnd, Pushback,  EfdCfgLeftSquareBrace }, /* EfdLexHash     */
    {     EfdLexEnd, Pushback,  EfdCfgLeftSquareBrace }, /* EfdLexNewline  */
    {     EfdLexEnd,     Skip,  EfdCfgLeftSquareBrace }, /* EfdLexOther    */
    {     EfdLexEnd,     Skip,  EfdCfgLeftSquareBrace }  /* EfdLexEOF      */
  },
  /* EfdLexRSqBrace := 3 */
  {
    {   EfdLexBegin,  Consume,             EfdCfgNone }, /* EfdLexBegin    */
    {     EfdLexEnd, Pushback, EfdCfgRightSquareBrace }, /* EfdLexChar     */
    {     EfdLexEnd, Pushback, EfdCfgRightSquareBrace }, /* EfdLexLSqBrace */
    {     EfdLexEnd, Pushback, EfdCfgRightSquareBrace }, /* EfdLexRSqBrace */
    {     EfdLexEnd, Pushback, EfdCfgRightSquareBrace }, /* EfdLexEquals   */
    {     EfdLexEnd, Pushback, EfdCfgRightSquareBrace }, /* EfdLexHash     */
    {     EfdLexEnd, Pushback, EfdCfgRightSquareBrace }, /* EfdLexNewline  */
    {     EfdLexEnd,     Skip, EfdCfgRightSquareBrace }, /* EfdLexOther    */
    {     EfdLexEnd,     Skip, EfdCfgRightSquareBrace }  /* EfdLexEOF      */
  },
  /* EfdLexEquals   := 4 */
  {
    {   EfdLexBegin,  Consume,             EfdCfgNone }, /* EfdLexBegin    */
    {     EfdLexEnd, Pushback,           EfdCfgEquals }, /* EfdLexChar     */
    {     EfdLexEnd, Pushback,           EfdCfgEquals }, /* EfdLexLSqBrace */
    {     EfdLexEnd, Pushback,           EfdCfgEquals }, /* EfdLexRSqBrace */
    {     EfdLexEnd, Pushback,           EfdCfgEquals }, /* EfdLexEquals   */
    {     EfdLexEnd, Pushback,           EfdCfgEquals }, /* EfdLexHash     */
    {     EfdLexEnd, Pushback,           EfdCfgEquals }, /* EfdLexNewline  */
    {     EfdLexEnd,     Skip,           EfdCfgEquals }, /* EfdLexOther    */
    {     EfdLexEnd,     Skip,           EfdCfgEquals }  /* EfdLexEOF      */
  },
  /* EfdLexHash     := 5 */
  {
    {   EfdLexBegin,  Consume,             EfdCfgNone }, /* EfdLexBegin    */
    {     EfdLexEnd, Pushback,             EfdCfgHash }, /* EfdLexChar     */
    {     EfdLexEnd, Pushback,             EfdCfgHash }, /* EfdLexLSqBrace */
    {     EfdLexEnd, Pushback,             EfdCfgHash }, /* EfdLexRSqBrace */
    {     EfdLexEnd, Pushback,             EfdCfgHash }, /* EfdLexEquals   */
    {     EfdLexEnd, Pushback,             EfdCfgHash }, /* EfdLexHash     */
    {     EfdLexEnd, Pushback,             EfdCfgHash }, /* EfdLexNewline  */
    {     EfdLexEnd,     Skip,             EfdCfgHash }, /* EfdLexOther    */
    {     EfdLexEnd,     Skip,             EfdCfgHash }  /* EfdLexEOF      */
  },
  /* EfdLexNewline  := 6 */
  {
    {   EfdLexBegin,  Consume,             EfdCfgNone }, /* EfdLexBegin    */
    {     EfdLexEnd, Pushback,          EfdCfgNewline }, /* EfdLexChar     */
    {     EfdLexEnd, Pushback,          EfdCfgNewline }, /* EfdLexLSqBrace */
    {     EfdLexEnd, Pushback,          EfdCfgNewline }, /* EfdLexRSqBrace */
    {     EfdLexEnd, Pushback,          EfdCfgNewline }, /* EfdLexEquals   */
    {     EfdLexEnd, Pushback,          EfdCfgNewline }, /* EfdLexHash     */
    {     EfdLexEnd, Pushback,          EfdCfgNewline }, /* EfdLexNewline  */
    {     EfdLexEnd,     Skip,          EfdCfgNewline }, /* EfdLexOther    */
    {     EfdLexEnd,     Skip,          EfdCfgNewline }  /* EfdLexEOF      */
  },
  /* EfdLexOther    := 7 */
  {
    {   EfdLexBegin,  Consume,             EfdCfgNone }, /* EfdLexBegin    */
    {    EfdLexChar,  Consume,             EfdCfgNone }, /* EfdLexChar     */
    {EfdLexLSqBrace,  Consume,             EfdCfgNone }, /* EfdLexLSqBrace */
    {EfdLexRSqBrace,  Consume,             EfdCfgNone }, /* EfdLexRSqBrace */
    {  EfdLexEquals,  Consume,             EfdCfgNone }, /* EfdLexEquals   */
    {    EfdLexHash,  Consume,             EfdCfgNone }, /* EfdLexHash     */
    { EfdLexNewline,  Consume,             EfdCfgNone }, /* EfdLexNewline  */
    {   EfdLexOther,  Consume,             EfdCfgNone }, /* EfdLexOther    */
    {     EfdLexEOF,  Consume,             EfdCfgNone }  /* EfdLexEOF      */
  },
  /* EfdLexEOF      := 8 */
  {
    {   EfdLexBegin,  Consume,             EfdCfgNone }, /* EfdLexBegin    */
    {     EfdLexEnd,  Consume,             EfdCfgNone }, /* EfdLexChar     */
    {     EfdLexEnd,  Consume,             EfdCfgNone }, /* EfdLexLSqBrace */
    {     EfdLexEnd,  Consume,             EfdCfgNone }, /* EfdLexRSqBrace */
    {     EfdLexEnd,  Consume,             EfdCfgNone }, /* EfdLexEquals   */
    {     EfdLexEnd,  Consume,             EfdCfgNone }, /* EfdLexHash     */
    { EfdLexNewline,  Consume,             EfdCfgNone }, /* EfdLexNewline  */
    {     EfdLexEnd,  Consume,             EfdCfgNone }, /* EfdLexOther    */
    {     EfdLexEnd,  Consume,             EfdCfgNone }  /* EfdLexEOF      */
  }
};

DWORD
EfdWrapperStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    PEFD_CONFIG_WRAPPER_STATE pState = (PEFD_CONFIG_WRAPPER_STATE)pData;

    pState->pszCurrentSection = pszSectionName;
    *pbSkipSection = FALSE;
    *pbContinue = TRUE;
    
    return 0;
}

DWORD
EfdWrapperReadValue(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PEFD_CONFIG_WRAPPER_STATE pState = (PEFD_CONFIG_WRAPPER_STATE)pData;
    DWORD dwIndex = 0;
    PEFD_CONFIG_SETTING pSetting = NULL;

    for (dwIndex = 0; dwIndex < pState->pSettings->dwCount; dwIndex++)
    {
        pSetting = &pState->pSettings->pSettings[dwIndex];
        if (strcmp(pSetting->pszSectionName, pState->pszCurrentSection) ||
            strcmp(pSetting->pszName, pszName))
        {
            continue;
        }
        if (pSetting->bParsed)
        {
            dwError = EEXIST;
            BAIL_ON_EFD_ERROR(dwError);
        }
        switch (pSetting->type)
        {
            case Dword:
            {
                PSTR pszEndPtr = NULL;
                unsigned long ulValue = strtoul(pszValue, &pszEndPtr, 10);

                if (pszEndPtr == NULL || pszEndPtr[0] != 0)
                {
                    // The conversion didn't finish
                    dwError = EINVAL;
                    BAIL_ON_EFD_ERROR(dwError);
                }

                if (ulValue < pSetting->dwMin || ulValue > pSetting->dwMax)
                {
                    dwError = ERANGE;
                    BAIL_ON_EFD_ERROR(dwError);
                }
                
                *(PDWORD)pSetting->pvDest = ulValue;
                pSetting->bParsed = TRUE;
                break;
            }
            case String:
            {
                size_t sLength = strlen(pszValue);
                
                if (sLength < pSetting->dwMin || sLength > pSetting->dwMax)
                {
                    dwError = ERANGE;
                    BAIL_ON_EFD_ERROR(dwError);
                }

                dwError = RtlCStringDuplicate(
                                (PSTR*)pSetting->pvDest,
                                pszValue);
                BAIL_ON_EFD_ERROR(dwError);

                pSetting->bParsed = TRUE;
                break;
            }
            case Enum:
            {
                DWORD dwEnumIndex = 0;

                for (dwEnumIndex = 0;
                     dwEnumIndex <= pSetting->dwMax - pSetting->dwMin;
                     dwEnumIndex++)
                {
                    if (!strcmp(pszValue, pSetting->ppszEnumNames[dwEnumIndex]))
                    {
                        *(int *)pSetting->pvDest = dwEnumIndex +
                            pSetting->dwMin;
                        pSetting->bParsed = TRUE;
                        break;
                    }

                    if (dwEnumIndex == pSetting->dwMax - pSetting->dwMin)
                    {
                        // Nothing matched
                        dwError = EINVAL;
                        BAIL_ON_EFD_ERROR(dwError);
                    }
                }
                break;
            }
            case Boolean:
            {
                BOOLEAN bValue = FALSE;

                if (!strcasecmp(pszValue, "true") ||
                    !strcasecmp(pszValue, "1") ||
                    !strcasecmp(pszValue, "yes"))
                {
                    bValue = TRUE;
                }
                else if (!strcasecmp(pszValue, "false") ||
                         !strcasecmp(pszValue, "0") ||
                         !strcasecmp(pszValue, "no"))
                {
                    bValue = FALSE;
                }
                else
                {
                    // Invalid value for a boolean
                    dwError = EINVAL;
                    BAIL_ON_EFD_ERROR(dwError);
                }
                *(PBOOLEAN)pSetting->pvDest = bValue;
                pSetting->bParsed = TRUE;
                break;
            }
            default:
                // Don't know how to parse this
                dwError = EINVAL;
                BAIL_ON_EFD_ERROR(dwError);
        }
    }
    *pbContinue = TRUE;
    
cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
EfdWrapperEndSection(
    PCSTR pszSectionName,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    PEFD_CONFIG_WRAPPER_STATE pState = (PEFD_CONFIG_WRAPPER_STATE)pData;

    pState->pszCurrentSection = NULL;
    *pbContinue = TRUE;
    
    return 0;
}

DWORD
EfdParseConfigFile(
    PCSTR pszFilePath,
    DWORD dwOptions,
    PEFD_CONFIG_SETTINGS pSettings
    )
{
    DWORD dwError = 0;
    EFD_CONFIG_WRAPPER_STATE state = {0};
    state.pSettings = pSettings;
    state.pszCurrentSection = 0;
    DWORD dwIndex = 0;
    PEFD_CONFIG_SETTING pSetting = NULL;

    dwError = EfdParseConfigFileEx(
                    pszFilePath,
                    dwOptions,
                    EfdWrapperStartSection,
                    NULL,
                    EfdWrapperReadValue,
                    EfdWrapperEndSection,
                    &state);
    BAIL_ON_EFD_ERROR(dwError);

cleanup:
    return dwError;

error:
    // Free any strings that were saved in the output variable before this
    // function failed.
    for (dwIndex = 0; dwIndex < pSettings->dwCount; dwIndex++)
    {
        pSetting = &pSettings->pSettings[dwIndex];
        if (pSetting->bParsed && pSetting->type == String)
        {
            RtlCStringFree((PSTR*)pSetting->pvDest);
        }
    }
    goto cleanup;
}

DWORD
EfdParseConfigFileEx(
    PCSTR                     pszFilePath,
    DWORD                     dwOptions,
    PFNEFD_CONFIG_START_SECTION   pfnStartSectionHandler,
    PFNEFD_CONFIG_COMMENT         pfnCommentHandler,
    PFNEFD_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNEFD_CONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                     pData
    )
{
    DWORD dwError = 0;
    PEFD_CONFIG_PARSE_STATE pParseState = NULL;
    
    dwError = EfdCfgInitParseState(
                    pszFilePath,
                    dwOptions, 
                    pfnStartSectionHandler,
                    pfnCommentHandler,
                    pfnNameValuePairHandler,
                    pfnEndSectionHandler,
                    pData,
                    &pParseState);
    BAIL_ON_EFD_ERROR(dwError);
    
    dwError = EfdCfgParse(pParseState);
    BAIL_ON_EFD_ERROR(dwError);
    
cleanup:
    
    if (pParseState) {
        EfdCfgFreeParseState(pParseState);
    }

    return dwError;
    
error:

    goto cleanup;
}

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
    )
{
    DWORD dwError = 0;
    PEFD_CONFIG_PARSE_STATE pParseState = NULL;
    PEFD_STACK pTokenStack = NULL;
    FILE* fp = NULL;

    if ((fp = fopen(pszFilePath, "r")) == NULL) {
        dwError = errno;
        BAIL_ON_EFD_ERROR(dwError);
    }
    
    
    dwError = RTL_ALLOCATE(
                    &pParseState,
                    EFD_CONFIG_PARSE_STATE,
                    sizeof(EFD_CONFIG_PARSE_STATE));
    BAIL_ON_EFD_ERROR(dwError);

    dwError = RTL_ALLOCATE(
                    &pTokenStack,
                    EFD_STACK,
                    sizeof(EFD_STACK));
    BAIL_ON_EFD_ERROR(dwError);

    pParseState->pLexerTokenStack = pTokenStack;

    dwError = RtlCStringDuplicate(
                    &(pParseState->pszFilePath),
                    pszFilePath);
    BAIL_ON_EFD_ERROR(dwError);
    
    pParseState->fp = fp;
    fp = NULL;
    
    pParseState->dwOptions = dwOptions;
    pParseState->pData = pData;
    
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
        EfdCfgFreeParseState(pParseState);
    }
    
    if (fp) {
        fclose(fp);
    }

    goto cleanup;
}

VOID
EfdCfgFreeParseState(
    PEFD_CONFIG_PARSE_STATE pParseState
    )
{
    RtlCStringFree(&pParseState->pszFilePath);
    RtlCStringFree(&pParseState->pszSectionName);
    if (pParseState->pLexerTokenStack) {
        EfdCfgFreeTokenStack(&pParseState->pLexerTokenStack);
    }
    
    if (pParseState->fp) {
        fclose(pParseState->fp);
    }
    RtlMemoryFree(pParseState);
}
  

DWORD
EfdCfgParse(
    PEFD_CONFIG_PARSE_STATE pParseState
    )
{
    
    DWORD dwError = 0;
    PEFD_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PEFD_STACK pTokenStack = NULL;
    
    do 
    {
    
        dwError = EfdCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_EFD_ERROR(dwError);
        
        switch (pToken->tokenType)
        {
            case EfdCfgHash:
            {
                dwError = EfdCfgParseComment(
                                pParseState,
                                &bContinue);
                BAIL_ON_EFD_ERROR(dwError);
                
                break;
            }
            case EfdCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = EfdCfgProcessComment(
                                pParseState, 
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_EFD_ERROR(dwError);
                
                break;
            }
            case EfdCfgLeftSquareBrace:
            {
                
                dwError = EfdCfgParseSections(
                                pParseState);
                BAIL_ON_EFD_ERROR(dwError);
                
                break;
            }
            case EfdCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = EFD_ERROR_INVALID_CONFIG;
                BAIL_ON_EFD_ERROR(dwError);
            }
        }
        
    } while (bContinue);
    
cleanup:
    
    if (pToken) {
        EfdCfgFreeToken(pToken);
    }
    
    return dwError;
    
error: 

    if (dwError == EFD_ERROR_INVALID_CONFIG)
    {
        if (pParseState) {                                                             
            EFD_LOG_ERROR ("Parse error at line=%d, column=%d of file [%s]",  
                          pParseState->dwLine, 
                          pParseState->dwCol, 
                          IsNullOrEmptyString(pParseState->pszFilePath) ? 
                              "" : pParseState->pszFilePath);                  
        }
    }

    goto cleanup;
}

DWORD
EfdCfgParseSections(
    PEFD_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PEFD_CFG_TOKEN pToken = NULL;
    BOOLEAN bContinue = TRUE;
    PEFD_STACK pTokenStack = NULL;
    
    dwError = EfdCfgParseSectionHeader(
                    pParseState, 
                    &bContinue);
    BAIL_ON_EFD_ERROR(dwError);
    
    while (bContinue)
    {
        dwError = EfdCfgGetNextToken(
                        pParseState,
                        &pToken
                        );
        BAIL_ON_EFD_ERROR(dwError);
        
        switch (pToken->tokenType)
        {
            case EfdCfgString:
            {
                BOOLEAN bIsAllSpace = FALSE;
                
                dwError = EfdStrIsAllSpace(
                                pToken->pszToken,
                                &bIsAllSpace
                                );
                BAIL_ON_EFD_ERROR(dwError);
                                
                if (bIsAllSpace)
                {
                    continue;
                }
                            
                dwError = EfdStackPush(pToken, &(pParseState->pLexerTokenStack));
                BAIL_ON_EFD_ERROR(dwError);
                
                pToken = NULL;
                
                dwError = EfdCfgParseNameValuePair(
                                pParseState,
                                &bContinue
                                );
                BAIL_ON_EFD_ERROR(dwError);
                break;                   
            }
        
            case EfdCfgHash:
            {
                dwError = EfdCfgParseComment(
                                pParseState,
                                &bContinue
                                );
                BAIL_ON_EFD_ERROR(dwError);
                break;
            }
            case EfdCfgNewline:
            {
                //in case of a blank line (newline),
                //interpret this as an empty comment.
                dwError = EfdCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_EFD_ERROR(dwError);
                break;
            }
            case EfdCfgLeftSquareBrace:
            {
                dwError = EfdCfgParseSectionHeader(
                                pParseState, 
                                &bContinue);
                BAIL_ON_EFD_ERROR(dwError);

                break;
            }
            case EfdCfgEOF:
            {
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = EFD_ERROR_INVALID_CONFIG;
                BAIL_ON_EFD_ERROR(dwError);
            }
        }
    } 
    
    if (bContinue && !IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = EfdCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_EFD_ERROR(dwError);
    }
    
cleanup:
    
    if (pToken) {
        EfdCfgFreeToken(pToken);
    }
    
    return dwError;
    
error:

    goto cleanup;
}



DWORD
EfdCfgParseComment(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN    pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PEFD_CFG_TOKEN pToken = NULL;
    PEFD_STACK pTokenStack = NULL;
    
    do
    {
        dwError = EfdCfgGetNextToken(
                    pParseState,
                    &pToken);
        BAIL_ON_EFD_ERROR(dwError);
        
        switch (pToken->tokenType)
        {
            case EfdCfgEOF:
            {
                dwError = EfdCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_EFD_ERROR(dwError);
                
                bContinue = FALSE;
                
                break;
            }
            case EfdCfgNewline:
            {
                dwError = EfdCfgProcessComment(
                                pParseState,
                                &pTokenStack,
                                &bContinue);
                BAIL_ON_EFD_ERROR(dwError);
                
                bKeepParsing = FALSE;
                
                break;
            }
            default:
            {
                dwError = EfdStackPush(pToken, &pTokenStack);
                BAIL_ON_EFD_ERROR(dwError);
                
                pToken = NULL;
                
            }
        }
        
    } while (bContinue && bKeepParsing);
    
    *pbContinue = bContinue;
    
cleanup:

    if (pToken) {
        EfdCfgFreeToken(pToken);
    }

    return dwError;
    
error:

    *pbContinue = FALSE;
    
    goto cleanup;
}

DWORD
EfdCfgParseSectionHeader(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bKeepParsing = TRUE;
    BOOLEAN bContinue = TRUE;
    PEFD_CFG_TOKEN pToken = NULL;
    PEFD_STACK pTokenStack = NULL;
    
    if(!IsNullOrEmptyString(pParseState->pszSectionName))
    {
        dwError = EfdCfgProcessEndSection(
                        pParseState,
                        &bContinue);
        BAIL_ON_EFD_ERROR(dwError);
        
    }
    
    if (!bContinue)
    {
        goto done;
    }
    
    pParseState->bSkipSection = FALSE;
    
    do
    {
        dwError = EfdCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_EFD_ERROR(dwError);
        
        switch(pToken->tokenType)
        {
            case EfdCfgString:
            case EfdCfgEquals:
            case EfdCfgOther:
            { 
                dwError = EfdStackPush(pToken, &pTokenStack);
                BAIL_ON_EFD_ERROR(dwError);
                
                pToken = NULL;
                break;
            }
            case EfdCfgRightSquareBrace:
            {
                dwError = EfdAssertWhitespaceOnly(pParseState);
                BAIL_ON_EFD_ERROR(dwError);
                
                bKeepParsing = FALSE;
                
                break;
            }
            default:
            {
                dwError = EFD_ERROR_INVALID_CONFIG;
                BAIL_ON_EFD_ERROR(dwError);
            }
        }
        
    } while (bKeepParsing);
    
    dwError = EfdCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_EFD_ERROR(dwError);
    
    switch(pToken->tokenType)
    {
    
        case EfdCfgNewline:
        {
            dwError = EfdCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_EFD_ERROR(dwError);

            break;
        }
        case EfdCfgEOF:
        {
            dwError = EfdCfgProcessBeginSection(
                            pParseState,
                            &pTokenStack,
                            &bContinue);
            BAIL_ON_EFD_ERROR(dwError);
            
            if (bContinue) {
                
                dwError = EfdCfgProcessEndSection(
                                pParseState,
                                &bContinue);
                BAIL_ON_EFD_ERROR(dwError);
            }
            
            bContinue = FALSE;
            
            break;     
        }
        default:
        {
            dwError = EFD_ERROR_INVALID_CONFIG;
            BAIL_ON_EFD_ERROR(dwError);
        }
    }
    
done:

    *pbContinue = bContinue;
    
cleanup:

    if (pToken) {
        EfdCfgFreeToken(pToken);
    }

    return dwError;
    
error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
EfdAssertWhitespaceOnly(
    PEFD_CONFIG_PARSE_STATE pParseState
    )
{
    DWORD dwError = 0;
    PEFD_CFG_TOKEN pToken = NULL;
    BOOLEAN bKeepParsing = TRUE;
    
    do
    {
        dwError = EfdCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_EFD_ERROR(dwError);
        
        switch(pToken->tokenType)
        {
            case EfdCfgString:
            case EfdCfgOther:
            {
                DWORD i = 0;
                for (; i < pToken->dwLen; i++) {
                    if (!isspace((int)pToken->pszToken[i])) {
                        dwError = EFD_ERROR_INVALID_CONFIG;
                        BAIL_ON_EFD_ERROR(dwError);
                    }
                }
                break;
            }
            case EfdCfgEOF:
            case EfdCfgNewline:
            {
                dwError = EfdStackPush(pToken, &pParseState->pLexerTokenStack);
                BAIL_ON_EFD_ERROR(dwError);
                
                pToken = NULL;
                
                bKeepParsing = FALSE;
                
                break;
            }
            default:
            {
                dwError = EFD_ERROR_INVALID_CONFIG;
                BAIL_ON_EFD_ERROR(dwError);
            }
        }
    } while (bKeepParsing);
    
cleanup:

    if (pToken) {
        EfdCfgFreeToken(pToken);
    }

    return dwError;
    
error:

    goto cleanup;
}

DWORD
EfdCfgParseNameValuePair(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bKeepParsing = TRUE;
    PEFD_CFG_TOKEN pToken = NULL;
    PEFD_STACK pTokenStack = NULL;
    
    //format is <str><equals><token1><token2>...<newline>
    
    //get initial <str>
    dwError = EfdCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_EFD_ERROR(dwError);
    
    if(pToken->tokenType == EfdCfgString) 
    {
        dwError = EfdStackPush(pToken, &pTokenStack);
        BAIL_ON_EFD_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = EFD_ERROR_INVALID_CONFIG;
        BAIL_ON_EFD_ERROR(dwError);
    }

    //get <equals>
    dwError = EfdCfgGetNextToken(
                    pParseState,
                    &pToken);
    BAIL_ON_EFD_ERROR(dwError);
   
    if(pToken->tokenType == EfdCfgEquals) 
    {
        dwError = EfdStackPush(pToken, &pTokenStack);
        BAIL_ON_EFD_ERROR(dwError);
        pToken = NULL;
    }
    else
    {
        dwError = EFD_ERROR_INVALID_CONFIG;
        BAIL_ON_EFD_ERROR(dwError);
    }


    do 
    {
        dwError = EfdCfgGetNextToken(
                        pParseState,
                        &pToken);
        BAIL_ON_EFD_ERROR(dwError);
        
        switch(pToken->tokenType)
        {
            case EfdCfgString:
            case EfdCfgEquals:
            case EfdCfgOther:
            {
                
                dwError = EfdStackPush(pToken, &pTokenStack);
                BAIL_ON_EFD_ERROR(dwError);
                pToken = NULL;
                
                break;

            }
            case EfdCfgNewline:
            {
                dwError = EfdCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_EFD_ERROR(dwError);
                bKeepParsing = FALSE; 
                break;
            }
            case EfdCfgEOF: 
            {
                dwError = EfdCfgProcessNameValuePair(
                    pParseState,
                    &pTokenStack,
                    &bContinue);
                BAIL_ON_EFD_ERROR(dwError);
                bContinue = FALSE;
                break;
            }
            default:
            {
                dwError = EFD_ERROR_INVALID_CONFIG;
                BAIL_ON_EFD_ERROR(dwError);
            }
        }
    } while(bContinue && bKeepParsing);
    
    
    *pbContinue = bContinue;
    
cleanup:

    if (pToken) {
        EfdCfgFreeToken(pToken);
    }

    return dwError;
    
error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
EfdCfgProcessComment(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PEFD_STACK* ppTokenStack,
    PBOOLEAN    pbContinue
    )
{
    DWORD   dwError = 0;
    BOOLEAN bContinue = TRUE;
    PSTR    pszComment = NULL;
    
    dwError = EfdCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszComment);
    BAIL_ON_EFD_ERROR(dwError);
    
    if (pParseState->pfnCommentHandler &&
        !pParseState->bSkipSection) {
            
        dwError = pParseState->pfnCommentHandler(
                        pszComment,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_EFD_ERROR(dwError);
    }
    
    *pbContinue = bContinue;
    
cleanup:

    RtlCStringFree(&pszComment);

    return dwError;
    
error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
EfdCfgProcessBeginSection(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PEFD_STACK* ppTokenStack,
    PBOOLEAN    pbContinue)
{
    DWORD   dwError = 0;
    PSTR    pszSectionName = NULL;
    BOOLEAN bSkipSection = FALSE;
    BOOLEAN bContinue = TRUE;
    
    dwError = EfdCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszSectionName);
    BAIL_ON_EFD_ERROR(dwError);
    
    if (IsNullOrEmptyString(pszSectionName)) {
        dwError = EFD_ERROR_INVALID_CONFIG;
        BAIL_ON_EFD_ERROR(dwError);
    }
    
    if (pParseState->pfnStartSectionHandler) {
        
        if(pParseState->dwOptions & EFD_CFG_OPTION_STRIP_SECTION) 
        {
            EfdStripWhitespace(pszSectionName, TRUE, TRUE); 
        }
        
        dwError = pParseState->pfnStartSectionHandler(
                        pszSectionName,
                        pParseState->pData,
                        &bSkipSection,
                        &bContinue);
        BAIL_ON_EFD_ERROR(dwError);
    }
    
    pParseState->pszSectionName = pszSectionName;
    pParseState->bSkipSection = bSkipSection;
    *pbContinue = bContinue;
    
cleanup:

    return dwError;
    
error:

    RtlCStringFree(&pszSectionName);
    pParseState->pszSectionName = NULL;
    pParseState->bSkipSection = FALSE;
    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
EfdCfgProcessNameValuePair(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PEFD_STACK*             ppTokenStack,
    PBOOLEAN                pbContinue
    )
{
    DWORD      dwError = 0;
    BOOLEAN    bContinue = TRUE;
    PSTR       pszName = NULL;
    PSTR       pszValue = NULL;
    PEFD_CFG_TOKEN pToken = NULL;
    
    *ppTokenStack = EfdStackReverse(*ppTokenStack);
    pToken = (PEFD_CFG_TOKEN)EfdStackPop(ppTokenStack);
    if (pToken && pToken->dwLen) {
        dwError = EfdStrndup(
                    pToken->pszToken,
                    pToken->dwLen,
                    &pszName);
        BAIL_ON_EFD_ERROR(dwError);
    }
    
    if (IsNullOrEmptyString(pszName)) {
        dwError = EFD_ERROR_INVALID_CONFIG;
        BAIL_ON_EFD_ERROR(dwError);
    }
    
    EfdCfgFreeToken(pToken);
    pToken = NULL;
    
    pToken = (PEFD_CFG_TOKEN)EfdStackPop(ppTokenStack);
    if (!pToken || pToken->tokenType != EfdCfgEquals) 
    {
        dwError = EFD_ERROR_INVALID_CONFIG;
        BAIL_ON_EFD_ERROR(dwError);
    }
    
    EfdCfgFreeToken(pToken);
    pToken = NULL;
    
    //this will consume the token stack
    dwError = EfdCfgProcessTokenStackIntoString(
        ppTokenStack,
        &pszValue);
    BAIL_ON_EFD_ERROR(dwError);
    
    if (pParseState->pfnNameValuePairHandler  &&
        !pParseState->bSkipSection) {
        
        if(pParseState->dwOptions & EFD_CFG_OPTION_STRIP_NAME_VALUE_PAIR) 
        {
            EfdStripWhitespace(pszName, TRUE, TRUE); 
            EfdStripWhitespace(pszValue, TRUE, TRUE); 
        }
        
        dwError = pParseState->pfnNameValuePairHandler( 
                        pszName,
                        pszValue,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_EFD_ERROR(dwError);
        
    }
    
    *pbContinue = bContinue;
    
cleanup:

    if (pToken) {
        EfdCfgFreeToken(pToken);
        pToken = NULL;
    }
    
    if(ppTokenStack && *ppTokenStack)
    {
        dwError = EfdCfgFreeTokenStack(ppTokenStack);
    }
    
    RtlCStringFree(&pszName);
    RtlCStringFree(&pszValue);

    return dwError;
    
error:

    goto cleanup;
}

DWORD
EfdCfgProcessEndSection(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PBOOLEAN                pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    
    if (pParseState->pfnEndSectionHandler &&
        !pParseState->bSkipSection) {
        
        if(pParseState->dwOptions & EFD_CFG_OPTION_STRIP_SECTION) 
        {
            EfdStripWhitespace(pParseState->pszSectionName, TRUE, TRUE); 
        }
        
        
        dwError = pParseState->pfnEndSectionHandler(
                        pParseState->pszSectionName,
                        pParseState->pData,
                        &bContinue);
        BAIL_ON_EFD_ERROR(dwError);
    }
    
    *pbContinue = bContinue;
    
cleanup:

    RtlCStringFree(&pParseState->pszSectionName);

    return dwError;
    
error:

    *pbContinue = TRUE;

    goto cleanup;
}

DWORD
EfdCfgDetermineTokenLength(
    PEFD_STACK pStack
    )
{
    DWORD dwLen = 0;
    PEFD_STACK pIter = pStack;
    
    for (; pIter; pIter = pIter->pNext)
    {
        PEFD_CFG_TOKEN pToken = (PEFD_CFG_TOKEN)pIter->pItem;
        if (pToken) {
            dwLen += pToken->dwLen;
        }
    }
    
    return dwLen;
}

//this will consume the token stack
DWORD
EfdCfgProcessTokenStackIntoString(
    PEFD_STACK* ppTokenStack,
    PSTR* ppszConcatenated
    )
{
    DWORD   dwError = 0;
    DWORD   dwRequiredTokenLen = 0;
    PSTR    pszConcatenated = NULL;
    
    dwRequiredTokenLen = EfdCfgDetermineTokenLength(*ppTokenStack);
    
    if (dwRequiredTokenLen) {
        
        PSTR pszPos = NULL;
        PEFD_CFG_TOKEN pToken = NULL;
        
        *ppTokenStack = EfdStackReverse(*ppTokenStack);
        
    
        dwError = RTL_ALLOCATE(
                        &pszConcatenated,
                        CHAR,
                        (dwRequiredTokenLen + 1) * sizeof(CHAR));
        BAIL_ON_EFD_ERROR(dwError);
            

        pszPos = pszConcatenated;
        while (*ppTokenStack)
        {
            pToken = EfdStackPop(ppTokenStack);
            if (pToken && pToken->dwLen) {
                    
                strncpy(pszPos, pToken->pszToken, pToken->dwLen);
                pszPos += pToken->dwLen;
                    
                EfdCfgFreeToken(pToken);
                pToken = NULL;
            }
        }
    }
    
    *ppszConcatenated = pszConcatenated;
    
cleanup:

    return dwError;
    
error:

    RtlCStringFree(&pszConcatenated);
    
    *ppszConcatenated = NULL;

    goto cleanup;
}


DWORD
EfdCfgAllocateToken(
    DWORD           dwSize,
    PEFD_CFG_TOKEN* ppToken
    )
{
    DWORD dwError = 0;
    PEFD_CFG_TOKEN pToken = NULL;
    DWORD dwMaxLen = (dwSize > 0 ? dwSize : EFD_CFG_TOKEN_DEFAULT_LENGTH);
    
    
    dwError = RTL_ALLOCATE(
                    &pToken,
                    EFD_CFG_TOKEN,
                    sizeof(EFD_CFG_TOKEN));
    BAIL_ON_EFD_ERROR(dwError);
    
    dwError = RTL_ALLOCATE(
                    &pToken->pszToken,
                    CHAR,
                    dwMaxLen * sizeof(CHAR));
    BAIL_ON_EFD_ERROR(dwError);
        

    pToken->tokenType = EfdCfgNone;
    pToken->dwMaxLen = dwMaxLen;
    
    *ppToken = pToken;
    
cleanup:
    
    return dwError;
    
error:

    *ppToken = NULL;
    
    if (pToken) {
        EfdCfgFreeToken(pToken);
    }

    goto cleanup;
}

DWORD
EfdCfgReallocToken(
    PEFD_CFG_TOKEN pToken,
    DWORD          dwNewSize
    )
{
    DWORD dwError = 0;
    PVOID pvNewMem = NULL;
    
    pvNewMem = RtlMemoryRealloc(
                    pToken->pszToken,
                    dwNewSize);
    if (pvNewMem == NULL)
    {
        dwError = ENOMEM;
        BAIL_ON_EFD_ERROR(dwError);
    }
    pToken->pszToken = pvNewMem;
    pToken->dwMaxLen = dwNewSize;
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

VOID
EfdCfgResetToken(
    PEFD_CFG_TOKEN pToken
    )
{
    if (pToken && pToken->pszToken && pToken->dwMaxLen)
    {
        memset(pToken->pszToken, 0, pToken->dwMaxLen);
        pToken->dwLen = 0;
    }
}

DWORD
EfdCfgCopyToken(
    PEFD_CFG_TOKEN pTokenSrc,
    PEFD_CFG_TOKEN pTokenDst
    )
{
    DWORD dwError = 0;
    
    pTokenDst->tokenType = pTokenSrc->tokenType;
    
    if (pTokenSrc->dwLen > pTokenDst->dwLen)
    {
        dwError = EfdCfgReallocToken(
                        pTokenDst,
                        pTokenSrc->dwLen);
        BAIL_ON_EFD_ERROR(dwError);
    }
    
    memset(pTokenDst->pszToken, 0, pTokenDst->dwLen);
    pTokenDst->dwLen = pTokenSrc->dwLen;
    memcpy(pTokenDst->pszToken, pTokenSrc->pszToken, pTokenSrc->dwLen);
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
EfdCfgFreeTokenStack(
    PEFD_STACK* ppTokenStack
    )
{
    
    DWORD dwError = 0;
    
    PEFD_STACK pTokenStack = *ppTokenStack;
    
    dwError = EfdStackForeach(
            pTokenStack,
            &EfdCfgFreeTokenInStack,
            NULL);
    BAIL_ON_EFD_ERROR(dwError);
    
    EfdStackFree(pTokenStack);
    
    *ppTokenStack = NULL; 
    
error:

    return dwError;
}

DWORD
EfdCfgFreeTokenInStack(
    PVOID pToken,
    PVOID pUserData
    )
{
    if (pToken)
    {
        EfdCfgFreeToken((PEFD_CFG_TOKEN)pToken);
    }
    
    return 0;
}

VOID
EfdCfgFreeToken(
    PEFD_CFG_TOKEN pToken
    )
{
    EFD_SAFE_FREE_MEMORY(pToken->pszToken);
    RtlMemoryFree(pToken);
}

DWORD
EfdCfgGetNextToken(
    PEFD_CONFIG_PARSE_STATE pParseState,
    PEFD_CFG_TOKEN*         ppToken
    )
{
    DWORD dwError = 0;
    EfdCfgTokenType tokenType = EfdCfgNone;
    EfdCfgLexState  curLexState = EfdLexBegin;
    PEFD_CFG_TOKEN  pToken = NULL;
    BOOLEAN         bOwnToken = FALSE;
    
    if (EfdStackPeek(pParseState->pLexerTokenStack) != NULL) 
    {
        PEFD_CFG_TOKEN pToken_input = *ppToken;
        
        pToken = (PEFD_CFG_TOKEN)EfdStackPop(&pParseState->pLexerTokenStack);
        bOwnToken = TRUE;
        
        if (pToken_input) {
            
            dwError = EfdCfgCopyToken(pToken, pToken_input);
            BAIL_ON_EFD_ERROR(dwError);
            
            EfdCfgFreeToken(pToken);
            pToken = NULL;
            bOwnToken = FALSE;
            
        }
        
        goto done;
    }
    
    pToken = *ppToken;
    
    if (!pToken) {
        dwError = EfdCfgAllocateToken(
                        0,
                        &pToken);
        BAIL_ON_EFD_ERROR(dwError);
        
        bOwnToken = TRUE;
    }
    else
    {
        EfdCfgResetToken(pToken);
    }

    while (curLexState != EfdLexEnd)
    {
        DWORD ch = EfdCfgGetCharacter(pParseState);
        EfdCfgLexState lexClass = EfdCfgGetLexClass(ch);

        if (lexClass != EfdLexEOF) {
            pParseState->dwCol++;
        }

        if (lexClass == EfdLexNewline) {
            pParseState->dwLine++;
            pParseState->dwCol = 0;
        }

        tokenType = EfdCfgGetTokenType(curLexState, lexClass);

        switch(EfdCfgGetLexAction(curLexState, lexClass))
        {
            case Skip:
                
                break;
                
            case Consume:

                if (pToken->dwLen >= pToken->dwMaxLen) {
                    dwError = EfdCfgReallocToken(
                                    pToken,
                                    pToken->dwMaxLen + EFD_CFG_TOKEN_DEFAULT_LENGTH);
                    BAIL_ON_EFD_ERROR(dwError);
                }
                
                pToken->pszToken[pToken->dwLen++] = (BYTE)ch;

                break;
                
            case Pushback:

            if (lexClass == EfdLexNewline)
            {
                    pParseState->dwLine--;
            }
                pParseState->dwCol--;
                dwError = EfdCfgPushBackCharacter(
                                pParseState,
                                (BYTE)ch);
                BAIL_ON_EFD_ERROR(dwError);
 
                break;
        }

        curLexState = EfdCfgGetNextLexState(curLexState, lexClass);
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
        EfdCfgFreeToken(pToken);
        *ppToken = NULL;
    }

    goto cleanup;
}

DWORD
EfdCfgGetCharacter(
    PEFD_CONFIG_PARSE_STATE pParseState
    )
{
    return getc(pParseState->fp);
}

EfdCfgLexState
EfdCfgGetLexClass(
    DWORD ch
    )
{
    
    if (ch == EOF) {
        return EfdLexEOF;
    }
    
    if (ch == '\n') {
        return EfdLexNewline;
    }

    if (ch == '[') {
        return EfdLexLSqBrace;
    }

    if (ch == ']') {
        return EfdLexRSqBrace;
    }

    if (ch == '=') {
        return EfdLexEquals;
    }
    
    if (ch == '#') {
        return EfdLexHash;
    }

    if (isalnum(ch) ||
        ispunct(ch) ||
        isblank(ch)) {
        return EfdLexChar;
    }

    return EfdLexOther;
}

DWORD
EfdCfgPushBackCharacter(
    PEFD_CONFIG_PARSE_STATE pParseState,
    BYTE ch
    )
{
    return ((EOF == ungetc(ch, pParseState->fp)) ? errno : 0);
}

EfdCfgLexState
EfdCfgGetNextLexState(
    EfdCfgLexState currentState,
    DWORD chId
    )
{
    return (gEfdLexStateTable[currentState][chId].nextState);
}

EfdCfgLexAction
EfdCfgGetLexAction(
    EfdCfgLexState currentState,
    DWORD chId
    )
{
    return (gEfdLexStateTable[currentState][chId].action);
}

EfdCfgTokenType
EfdCfgGetTokenType(
    EfdCfgLexState currentState,
    DWORD chId
    )
{
    return (gEfdLexStateTable[currentState][chId].tokenId);
}

