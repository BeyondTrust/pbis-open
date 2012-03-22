/*
 * Copyright Likewise Software    2004-2009
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
