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
 *        reglex_test.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser lexical analzyer test harness
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */
#include <parse/includes.h>

DWORD
runLexInput(
    void *ioH,
    PREGLEX_ITEM pLexH)
{
    DWORD dwError;
    CHAR pszTokenStr[128] = {0};
    DWORD lineNum = 0;
    BOOLEAN eof = FALSE;
    REGLEX_TOKEN token = 0;
    DWORD attrSize = 0;
    PSTR pszAttr = 0;
    DWORD count = 0;

    do
    {
        if (count && (count % 1000) == 0)
        {
            dwError = RegLexGetToken(ioH, pLexH, &token, &eof);
            RegLexTokenToString(token, pszTokenStr, sizeof(pszTokenStr));
            RegLexGetLineNumber(pLexH, &lineNum);
            RegLexGetAttribute(pLexH, &attrSize, &pszAttr);

            printf("Got NEXT token:  (%d) %s line=%d\n",
                   count, pszTokenStr, lineNum);
            printf("Got NEXT attrib: (%d) '%s'\n\n",
                   count, pszAttr);
            RegLexUnGetToken(pLexH);
        }
        count++;

        dwError = RegLexGetToken(ioH, pLexH, &token, &eof);
        if (dwError == ERROR_SUCCESS && !eof)
        {
            RegLexTokenToString(token, pszTokenStr, sizeof(pszTokenStr));
            RegLexGetLineNumber(pLexH, &lineNum);
            RegLexGetAttribute(pLexH, &attrSize, &pszAttr);

            printf("Got token:  %s line=%d\n", pszTokenStr, lineNum);
            if (attrSize > 0)
            {
                printf("Got attrib: '%s'\n\n", pszAttr);
            }
        }
    }
    while (!eof && dwError == ERROR_SUCCESS);

    if (!eof)
    {
        if (dwError == LWREG_ERROR_UNEXPECTED_TOKEN)
        {
            RegLexGetLineNumber(pLexH, &lineNum);
            printf("ERROR: Syntax error! line=%d\n", lineNum);
        }
        else
        {
            printf("ERROR: %0x\n", dwError);
        }
    }
    return dwError;
}


int main(int argc, char *argv[])
{
    DWORD dwError;
    void *ioH;
    PREGLEX_ITEM pLexH;
    PSTR ptr = NULL;
    CHAR inBuf[1024];
    DWORD inBufLen = 0;


    if (argc == 1)
    {
        printf("usage: %s regfile.reg\n", argv[0]);
        return 0;
    }

    dwError = RegLexOpen(&pLexH);
    if (dwError)
    {
        fprintf(stderr, "RegLexInit: failed %d\n", dwError);
        return 1;
    }

    if (strcmp(argv[1], "--shell") == 0)
    {
        dwError = RegIOBufferOpen(&ioH);
        if (dwError)
        {
            fprintf(stderr, "RegIOBufferOpen: failed %d\n", dwError);
            return 1;
        }

        do
        {
            printf("> ");
            fflush(stdout);
            ptr = fgets(inBuf, sizeof(inBuf)-1, stdin);
            if (ptr)
            {
                inBufLen = strlen(ptr);
                dwError = RegIOBufferSetData(ioH, inBuf, inBufLen);
                if (dwError) 
                {
                    return 1;
                }
                runLexInput(ioH, pLexH);
            }
        } while (!feof(stdin));
    }
    else
    {
        dwError = RegIOOpen(argv[1], &ioH);
        if (dwError)
        {
            fprintf(stderr, "RegIOOpen: failed %d\n", dwError);
            return 1;
        }
        runLexInput(ioH, pLexH);
    }


    
    RegLexClose(pLexH);
    RegIOClose(ioH);
    return 0;
}
