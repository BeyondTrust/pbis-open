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
 *        rsys-cfg.c
 *
 * Abstract:
 *
 *        Reaper for syslog
 * 
 *        Config (INF Style) parser
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 * 
 */
#include "includes.h"

DWORD
RSysUnquoteString(
    PSTR pszInput
    )
{
    DWORD dwError = 0;
    PSTR pszOutput = pszInput;

    while (pszInput && *pszInput != 0)
    {
        if (*pszInput == '\\')
        {
            switch(pszInput[1])
            {
                case 'n':
                    *pszOutput = '\n';
                    pszInput += 2;
                    break;
                case 'r':
                    *pszOutput = '\r';
                    pszInput += 2;
                    break;
                case '\\':
                    *pszOutput = '\\';
                    pszInput += 2;
                    break;
                case '\"':
                    *pszOutput = '\"';
                    pszInput += 2;
                    break;
                case 'x':
                    sscanf(pszInput + 2, "%2hhX", pszOutput);
                    pszInput += 4;
                    break;
                default:
                    dwError = LW_STATUS_UNMAPPABLE_CHARACTER;
                    BAIL_ON_RSYS_ERROR(dwError);
                    break;
            }
        }
        else
        {
            *pszOutput = *pszInput;
            pszInput++;
        }
        pszOutput++;
    }

    if (pszOutput)
    {
        *pszOutput = 0;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
RSysWrapperStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PRSYS_CONFIG_SETTINGS pSettings = (PRSYS_CONFIG_SETTINGS)pData;

    pSettings->pszCurrentSection = pszSectionName;
    *pbContinue = TRUE;

    if (pSettings->pfnStartSectionHandler)
    {
        dwError = pSettings->pfnStartSectionHandler(
                        pSettings,
                        pszSectionName);
        BAIL_ON_RSYS_ERROR(dwError);
    }
    
cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
RSysWrapperReadValue(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PRSYS_CONFIG_SETTINGS pSettings = (PRSYS_CONFIG_SETTINGS)pData;
    DWORD dwIndex = 0;
    PRSYS_CONFIG_SETTING pSetting = NULL;
    BOOLEAN bMatched = FALSE;

    for (dwIndex = 0; dwIndex < pSettings->dwCount; dwIndex++)
    {
        pSetting = &pSettings->pSettings[dwIndex];
        if (strcmp(pSetting->pszSectionName, pSettings->pszCurrentSection) ||
            strcmp(pSetting->pszName, pszName))
        {
            continue;
        }
        if (pSetting->bParsed)
        {
            dwError = EEXIST;
            BAIL_ON_RSYS_ERROR(dwError);
        }
        bMatched = TRUE;
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
                    BAIL_ON_RSYS_ERROR(dwError);
                }

                if (ulValue < pSetting->dwMin || ulValue > pSetting->dwMax)
                {
                    dwError = ERANGE;
                    BAIL_ON_RSYS_ERROR(dwError);
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
                    BAIL_ON_RSYS_ERROR(dwError);
                }

                dwError = RtlCStringDuplicate(
                                (PSTR*)pSetting->pvDest,
                                pszValue);
                BAIL_ON_RSYS_ERROR(dwError);

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
                        BAIL_ON_RSYS_ERROR(dwError);
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
                    BAIL_ON_RSYS_ERROR(dwError);
                }
                *(PBOOLEAN)pSetting->pvDest = bValue;
                pSetting->bParsed = TRUE;
                break;
            }
            default:
                // Don't know how to parse this
                dwError = EINVAL;
                BAIL_ON_RSYS_ERROR(dwError);
        }
    }
    if (!bMatched && pSettings->pfnUnknownSetting)
    {
        dwError = pSettings->pfnUnknownSetting(
                                pSettings,
                                pszName,
                                pszValue);
        BAIL_ON_RSYS_ERROR(dwError);
    }
    *pbContinue = TRUE;
    
cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
RSysWrapperEndSection(
    PCSTR pszSectionName,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    PRSYS_CONFIG_SETTINGS pSettings = (PRSYS_CONFIG_SETTINGS)pData;

    pSettings->pszCurrentSection = NULL;
    *pbContinue = TRUE;
    
    return 0;
}

DWORD
RSysParseConfigFile(
    PCSTR pszFilePath,
    DWORD dwOptions,
    PRSYS_CONFIG_SETTINGS pSettings
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    PRSYS_CONFIG_SETTING pSetting = NULL;

    pSettings->pszCurrentSection = NULL;
    dwError = RSysParseConfigFileEx(
                    pszFilePath,
                    dwOptions,
                    RSysWrapperStartSection,
                    NULL,
                    RSysWrapperReadValue,
                    RSysWrapperEndSection,
                    pSettings);
    BAIL_ON_RSYS_ERROR(dwError);

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
RSysReadLine(
    PSTR* ppszLine,
    FILE *pStream
    )
{
    DWORD dwError = 0;
    ssize_t sSize = 0, sCapacity = 0;
    PSTR pszBuffer = NULL;
    // Do not free
    PSTR pszNewBuffer = NULL;

    do
    {
        // There is not enough space. Allocate a larger buffer
        sCapacity = sSize*2 + 10;
        pszNewBuffer = LwRtlMemoryRealloc(
                            pszBuffer,
                            sCapacity + 1);
        if (pszNewBuffer == NULL)
        {
            dwError = ENOMEM;
            BAIL_ON_RSYS_ERROR(dwError);
        }
        pszBuffer = pszNewBuffer;

        // Read as much as the stream will give us up to the space in the
        // buffer.
        errno = 0;
        if (fgets(pszBuffer + sSize, sCapacity - sSize, pStream) == NULL)
        {
            dwError = errno;
            if (dwError == 0)
            {
                dwError = LW_STATUS_END_OF_FILE;
            }
            BAIL_ON_RSYS_ERROR(dwError);
        }

        sSize += strlen(pszBuffer + sSize);
    }
    // While the whole buffer is used and it does not end in a newline
    while(sSize == sCapacity - 1 && pszBuffer[sSize-1] != '\n');

    if (sSize == 0)
    {
        dwError = LW_STATUS_END_OF_FILE;
        BAIL_ON_RSYS_ERROR(dwError);
    }

    *ppszLine = pszBuffer;

cleanup:
    return dwError;

error:
    LW_RTL_FREE(&pszBuffer);
    goto cleanup;
}

DWORD
RSysRegComp(
    regex_t* pRegex,
    PBOOLEAN pbCompiled,
    PCSTR pszExpr
    )
{
    DWORD dwError = regcomp(pRegex, pszExpr, REG_EXTENDED);
    
    if (dwError)
    {
        dwError = EINVAL;
        BAIL_ON_RSYS_ERROR(dwError);
    }

    *pbCompiled = TRUE;

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
RSysParseConfigFileEx(
    PCSTR                     pszFilePath,
    DWORD                     dwOptions,
    PFNRSYS_CONFIG_START_SECTION   pfnStartSectionHandler,
    PFNRSYS_CONFIG_COMMENT         pfnCommentHandler,
    PFNRSYS_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNRSYS_CONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                     pData
    )
{
    DWORD dwError = 0;
    FILE* fp = NULL;
    PSTR pszLine = NULL;
    // Do not free.
    PSTR pszLinePos = NULL;
    regex_t commentExp, sectionExp, nameValueExp, quotedNameValueExp;
    BOOLEAN bCommentExpCompiled = FALSE;
    BOOLEAN bSectionExpCompiled = FALSE;
    BOOLEAN bNameValueExpCompiled = FALSE;
    BOOLEAN bQuotedNameValueExpCompiled = FALSE;
    BOOLEAN bContinue = TRUE;
    PSTR pszCurrentSection = NULL;
    regmatch_t matches[5];

    // A comment is started with a hash sign and goes to the end of the line.
    // White space may occur before the hash.
    // A comment may also be a blank line.
    dwError = RSysRegComp(
                    &commentExp,
                    &bCommentExpCompiled,
                    "^([[:space:]]*(#.*)?[[:space:]]*)$");
    BAIL_ON_RSYS_ERROR(dwError);
    
    // A section name is surrounded by []. It may have leading and trailing
    // white space, and another statement may appear afterwards.
    dwError = RSysRegComp(
                    &sectionExp,
                    &bSectionExpCompiled,
                    "^[[:space:]]*\\[([^][:space:]]+)\\][[:space:]]*");
    BAIL_ON_RSYS_ERROR(dwError);

    // A quoted name value pair follows the form 'name = "value"'. Escape
    // sequences are allowed inside of the quotes. Another statement may appear
    // afterwards.
    dwError = RSysRegComp(
                    &quotedNameValueExp,
                    &bQuotedNameValueExpCompiled,
                    "^[[:space:]]*([^[:space:]=]+)[[:space:]]*=[[:space:]]*\"((\\.|[^\"\\])*)\"[[:space:]]*");
    BAIL_ON_RSYS_ERROR(dwError);

    // An unquoted statement has the form 'name = value'. The value runs to the
    // end of the line, except that trailing spaces are removed. The value may
    // not contain a # character. If a # appears on the line, it is later
    // translated to a comment.
    dwError = RSysRegComp(
                    &nameValueExp,
                    &bNameValueExpCompiled,
                    "^[[:space:]]*([^[:space:]=]+)[[:space:]]*=[[:space:]]*(([^#]*[^[:space:]#])?)[[:space:]]*");
    BAIL_ON_RSYS_ERROR(dwError);

    if ((fp = fopen(pszFilePath, "r")) == NULL) {
        dwError = errno;
        BAIL_ON_RSYS_ERROR(dwError);
    }

    while (bContinue)
    {
        RtlCStringFree(&pszLine);
        dwError = RSysReadLine(
                        &pszLine,
                        fp);
        if (dwError == LW_STATUS_END_OF_FILE)
        {
            dwError = 0;
            break;
        }
        BAIL_ON_RSYS_ERROR(dwError);
        
        pszLinePos = pszLine;
        while (bContinue && *pszLinePos != 0)
        {
            if (!regexec(&commentExp,
                        pszLinePos,
                        sizeof(matches)/sizeof(matches[0]),
                        matches,
                        0))
            {
                // Null terminate the contents of the comment
                pszLinePos[matches[1].rm_eo] = 0;
                if (pfnCommentHandler)
                {
                    dwError = pfnCommentHandler(
                                    pszLinePos + matches[1].rm_so,
                                    pData,
                                    &bContinue);
                }
            }
            else if (!regexec(&sectionExp,
                        pszLinePos,
                        sizeof(matches)/sizeof(matches[0]),
                        matches,
                        0))
            {
                // Null terminate the name of the section
                pszLinePos[matches[1].rm_eo] = 0;

                if (pszCurrentSection)
                {
                    dwError = pfnEndSectionHandler(
                                        pszCurrentSection,
                                        pData,
                                        &bContinue);
                    BAIL_ON_RSYS_ERROR(dwError);
                    RtlCStringFree(&pszCurrentSection);

                    if (!bContinue)
                    {
                        break;
                    }
                }

                dwError = RtlCStringDuplicate(
                                &pszCurrentSection,
                                pszLinePos + matches[1].rm_so);
                BAIL_ON_RSYS_ERROR(dwError);

                if (pfnStartSectionHandler)
                {
                    dwError = pfnStartSectionHandler(
                                    pszCurrentSection,
                                    pData,
                                    &bContinue);
                    BAIL_ON_RSYS_ERROR(dwError);
                }
            }
            else if (!regexec(&quotedNameValueExp,
                        pszLinePos,
                        sizeof(matches)/sizeof(matches[0]),
                        matches,
                        0))
            {
                // Null terminate the name of the pair
                pszLinePos[matches[1].rm_eo] = 0;
                // Null terminate the value of the pair
                pszLinePos[matches[2].rm_eo] = 0;

                dwError = RSysUnquoteString(
                                pszLinePos + matches[2].rm_so);
                BAIL_ON_RSYS_ERROR(dwError);

                if (pfnNameValuePairHandler)
                {
                    dwError = pfnNameValuePairHandler(
                                    pszLinePos + matches[1].rm_so,
                                    pszLinePos + matches[2].rm_so,
                                    pData,
                                    &bContinue);
                    BAIL_ON_RSYS_ERROR(dwError);
                }
            }
            else if (!regexec(&nameValueExp,
                        pszLinePos,
                        sizeof(matches)/sizeof(matches[0]),
                        matches,
                        0))
            {
                // Null terminate the name of the pair
                pszLinePos[matches[1].rm_eo] = 0;
                // Null terminate the value of the pair
                pszLinePos[matches[2].rm_eo] = 0;
                if (pfnNameValuePairHandler)
                {
                    dwError = pfnNameValuePairHandler(
                                    pszLinePos + matches[1].rm_so,
                                    pszLinePos + matches[2].rm_so,
                                    pData,
                                    &bContinue);
                    BAIL_ON_RSYS_ERROR(dwError);
                }
            }
            else
            {
                RSYS_LOG_ERROR ("Unable to parse line [%s]", pszLinePos);
                dwError = RSYS_ERROR_INVALID_CONFIG;
                BAIL_ON_RSYS_ERROR(dwError);
            }
            // Go to the first character after the match
            pszLinePos += matches[0].rm_eo;
        }
    }

cleanup:
    if (fp) {
        fclose(fp);
    }
    
    RtlCStringFree(&pszLine);
    RtlCStringFree(&pszCurrentSection);

    if (bCommentExpCompiled)
    {
        regfree(&commentExp);
    }
    if (bSectionExpCompiled)
    {
        regfree(&sectionExp);
    }
    if (bNameValueExpCompiled)
    {
        regfree(&nameValueExp);
    }
    if (bQuotedNameValueExpCompiled)
    {
        regfree(&quotedNameValueExp);
    }

    return dwError;
    
error:
    goto cleanup;
}
