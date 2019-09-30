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
 *        parser.c
 *
 * Abstract:
 *
 *        Config (INF Style) parser
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *          Scott Salley <ssalley@likewise.com>
 * 
 */
#include "includes.h"

static
DWORD
UnquoteString(
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
                    (void) sscanf(pszInput + 2, "%2hhX", pszOutput);
                    pszInput += 4;
                    break;
                default:
                    dwError = LW_STATUS_UNMAPPABLE_CHARACTER;
                    BAIL_ON_UP_ERROR(dwError);
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

static
DWORD
ReadLine(
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
            BAIL_ON_UP_ERROR(dwError);
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
            BAIL_ON_UP_ERROR(dwError);
        }

        sSize += strlen(pszBuffer + sSize);
    }
    // While the whole buffer is used and it does not end in a newline
    while(sSize == sCapacity - 1 && pszBuffer[sSize-1] != '\n');

    if (sSize == 0)
    {
        dwError = LW_STATUS_END_OF_FILE;
        BAIL_ON_UP_ERROR(dwError);
    }

    *ppszLine = pszBuffer;

cleanup:
    return dwError;

error:
    LW_RTL_FREE(&pszBuffer);
    goto cleanup;
}

static
DWORD
RegComp(
    regex_t* pRegex,
    PBOOLEAN pbCompiled,
    PCSTR pszExpr
    )
{
    DWORD dwError = regcomp(pRegex, pszExpr, REG_EXTENDED);

    if (dwError)
    {
        dwError = LwMapErrnoToLwError(EINVAL);
        BAIL_ON_UP_ERROR(dwError);
    }

    *pbCompiled = TRUE;

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
UpParseConfigFile(
    PCSTR                           pszFilePath,
    UpParseConfigSectionHandler     pfnSectionHandler,
    UpParseConfigNameValueHandler   pfnNameValuePairHandler,
    PVOID                           pData
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
    
    DWORD dwLineNumber = 0;

    // A comment is started with a hash sign and goes to the end of the line.
    // White space may occur before the hash.
    // A comment may also be a blank line.
    dwError = RegComp(
                &commentExp,
                &bCommentExpCompiled,
                "^([[:space:]]*(#.*)?[[:space:]]*)$");
    BAIL_ON_UP_ERROR(dwError);
    
    // A section name is surrounded by []. It may have leading and trailing
    // white space, and another statement may appear afterwards.
    dwError = RegComp(
                &sectionExp,
                &bSectionExpCompiled,
                "^[[:space:]]*\\[([^]]+)\\][[:space:]]*");
    BAIL_ON_UP_ERROR(dwError);

    // A quoted name value pair follows the form 'name = "value"'. Escape
    // sequences are allowed inside of the quotes. Another statement may appear
    // afterwards.
    dwError = RegComp(
                &quotedNameValueExp,
                &bQuotedNameValueExpCompiled,
                "^[[:space:]]*([^[:space:]=]+)[[:space:]]*=[[:space:]]*\"((\\.|[^\"\\])*)\"[[:space:]]*");
    BAIL_ON_UP_ERROR(dwError);

    // An unquoted statement has the form 'name = value'. The value runs to the
    // end of the line, except that trailing spaces are removed. The value may
    // not contain a # character. If a # appears on the line, it is later
    // translated to a comment.
    dwError = RegComp(
                &nameValueExp,
                &bNameValueExpCompiled,
                "^[[:space:]]*([^[:space:]=]+)[[:space:]]*=[[:space:]]*(([^#]*[^[:space:]#])?)[[:space:]]*");
    BAIL_ON_UP_ERROR(dwError);

    if ((fp = fopen(pszFilePath, "r")) == NULL) {
        dwError = errno;
        BAIL_ON_UP_ERROR(dwError);
    }

    dwLineNumber = 0;
    while (bContinue)
    {
        dwLineNumber++;
        RtlCStringFree(&pszLine);
        //fprintf(stderr, "Reading line %lu\n", (unsigned long) dwLineNumber);
        dwError = ReadLine(&pszLine, fp);
        if (dwError == LW_STATUS_END_OF_FILE)
        {
            dwError = 0;
            break;
        }
        BAIL_ON_UP_ERROR(dwError);


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
                    if ( pfnSectionHandler )
                    {
                        dwError = pfnSectionHandler(
                                    0,
                                    pszCurrentSection,
                                    pData,
                                    &bContinue);
                        BAIL_ON_UP_ERROR(dwError);
                        RtlCStringFree(&pszCurrentSection);

                        if (!bContinue)
                        {
                            break;
                        }
                    }
                }

                dwError = RtlCStringDuplicate(
                                &pszCurrentSection,
                                pszLinePos + matches[1].rm_so);
                BAIL_ON_UP_ERROR(dwError);

                if ( pfnSectionHandler )
                {
                    dwError = pfnSectionHandler(
                                    1,
                                    pszCurrentSection,
                                    pData,
                                    &bContinue);
                    BAIL_ON_UP_ERROR(dwError);
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

                dwError = UnquoteString(
                                pszLinePos + matches[2].rm_so);
                BAIL_ON_UP_ERROR(dwError);

                if (pfnNameValuePairHandler)
                {
                    dwError = pfnNameValuePairHandler(
                                    pszCurrentSection,
                                    pszLinePos + matches[1].rm_so,
                                    pszLinePos + matches[2].rm_so,
                                    pData,
                                    &bContinue);
                    BAIL_ON_UP_ERROR(dwError);
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
                                    pszCurrentSection,
                                    pszLinePos + matches[1].rm_so,
                                    pszLinePos + matches[2].rm_so,
                                    pData,
                                    &bContinue);
                    BAIL_ON_UP_ERROR(dwError);
                }
            }
            else
            {
                //RSYS_LOG_ERROR ("Unable to parse line [%s]", pszLinePos);
                dwError = LW_ERROR_INVALID_CONFIG;
                BAIL_ON_UP_ERROR(dwError);
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

// Parses configuration files as found in Likewise Open 4.1
DWORD
UpParseSambaConfigFile(
    PCSTR                           pszFilePath,
    UpParseConfigSectionHandler     pfnSectionHandler,
    UpParseConfigNameValueHandler   pfnNameValuePairHandler,
    PVOID                           pData
    )
{
    DWORD dwError = 0;
    FILE* fp = NULL;
    PSTR pszLine = NULL;
    // Do not free.
    PSTR pszLinePos = NULL;

    regex_t commentExp, sectionExp, nameValueExp;
    BOOLEAN bCommentExpCompiled = FALSE;
    BOOLEAN bSectionExpCompiled = FALSE;
    BOOLEAN bNameValueExpCompiled = FALSE;

    BOOLEAN bContinue = TRUE;
    PSTR pszCurrentSection = NULL;
    regmatch_t matches[5];

    DWORD dwLineNumber = 0;

    // A comment is started with a hash sign or semicolon 
    // and goes to the end of the line.
    // White space may occur before the hash/semicolon.
    // A comment may also be a blank line.
    dwError = RegComp(
                &commentExp,
                &bCommentExpCompiled,
                "^([[:space:]]*([#;].*)?[[:space:]]*)$");
    BAIL_ON_UP_ERROR(dwError);

    // A section name is surrounded by []. It may have leading and trailing
    // white space, and another statement may appear afterwards.
    dwError = RegComp(
                &sectionExp,
                &bSectionExpCompiled,
                "^[[:space:]]*\\[([^]]+)\\][[:space:]]*");
    BAIL_ON_UP_ERROR(dwError);

    // An unquoted statement has the form 'name = value'. The value runs to the
    // end of the line, except that trailing spaces are removed. The value may
    // not contain a # character. If a # appears on the line, it is later
    // translated to a comment.
    dwError = RegComp(
                &nameValueExp,
                &bNameValueExpCompiled,
                "^[[:space:]]*([^=#;]*[^=#; ]+)[[:space:]]*=[[:space:]]*(([^#;]*[^[:space:]#])?)[[:space:]]*");
    BAIL_ON_UP_ERROR(dwError);
// Start of line            ^
// Spaces Before Name       [[:space:]]*
// Name                     ([^[:space:]=]+)    =>   ([^=#;]+)
// Spaces Just After Name   [[:space:]]*
// Equals                   =
// Spaces Before Value      [[:space:]]*
// Value                    (([^#;]*[^[:space:]#;])?)[[:space:]]*

    if ((fp = fopen(pszFilePath, "r")) == NULL) {
        dwError = errno;
        BAIL_ON_UP_ERROR(dwError);
    }

    dwLineNumber = 0;
    while (bContinue)
    {
        dwLineNumber++;
        RtlCStringFree(&pszLine);
        //fprintf(stderr, "Reading line %lu\n", (unsigned long) dwLineNumber);
        dwError = ReadLine(&pszLine, fp);
        if (dwError == LW_STATUS_END_OF_FILE)
        {
            dwError = 0;
            break;
        }
        BAIL_ON_UP_ERROR(dwError);


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
                    if ( pfnSectionHandler )
                    {
                        dwError = pfnSectionHandler(
                                    0,
                                    pszCurrentSection,
                                    pData,
                                    &bContinue);
                        BAIL_ON_UP_ERROR(dwError);
                        RtlCStringFree(&pszCurrentSection);

                        if (!bContinue)
                        {
                            break;
                        }
                    }
                }

                dwError = RtlCStringDuplicate(
                                &pszCurrentSection,
                                pszLinePos + matches[1].rm_so);
                BAIL_ON_UP_ERROR(dwError);

                if ( pfnSectionHandler )
                {
                    dwError = pfnSectionHandler(
                                    1,
                                    pszCurrentSection,
                                    pData,
                                    &bContinue);
                    BAIL_ON_UP_ERROR(dwError);
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
                                    pszCurrentSection,
                                    pszLinePos + matches[1].rm_so,
                                    pszLinePos + matches[2].rm_so,
                                    pData,
                                    &bContinue);
                    BAIL_ON_UP_ERROR(dwError);
                }
            }
            else
            {
                //RSYS_LOG_ERROR ("Unable to parse line [%s]", pszLinePos);
                dwError = LW_ERROR_INVALID_CONFIG;
                BAIL_ON_UP_ERROR(dwError);
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

    return dwError;

error:
    goto cleanup;
}
