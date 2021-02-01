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
 *        regio_test.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser file I/O test harness
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */
#include <parse/includes.h>
#include <stdio.h>

void
testInteractive(void)
{
    DWORD dwError = 0;
    CHAR inBuf[1024];
    PSTR ptr = NULL;
    DWORD inBufLen = 0;
    HANDLE ioH = NULL;
    CHAR inC = '\0';
    BOOLEAN eof = FALSE;

    dwError = RegIOBufferOpen(&ioH);
    if (dwError)
    {
        fprintf(stderr, "RegIOBufferOpen: failed %d\n", dwError);
        return;
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
            if (dwError) return;
        }
    
        do
        {
            dwError = RegIOGetChar(ioH, &inC, &eof);
            if (!eof)
            {
                putchar(inC);
            }
            else
            {
                printf("<EOF>\n");
            }
        }
        while (!eof);
    } while (!feof(stdin));

    RegIOClose(ioH);
}


int main(int argc, char *argv[])
{
    DWORD dwError;
    HANDLE ioH;
    CHAR inC;
    BOOLEAN eof = FALSE;

    if (argc == 1)
    {
        printf("usage: %s regfile.reg | %s --shell\n", argv[0], argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "--shell") == 0)
    {
        testInteractive();
        return 0;
    }

    dwError = RegIOOpen(argv[1], &ioH);
    if (dwError)
    {
        fprintf(stderr, "RegIOOpen: failed %d\n", dwError);
        return 1;
    }

    dwError = RegIOGetChar(ioH, &inC, &eof);
    if (eof || dwError != ERROR_SUCCESS)
    {
        fprintf(stderr, "RegIOGetChar: failed %d\n", dwError);
    }
    dwError = RegIOUnGetChar(ioH, NULL);
    if (dwError != ERROR_SUCCESS)
    {
        printf("RegIOUnGetChar: 1 UnGetChar failed!\n");
    }

    do
    {
        dwError = RegIOGetChar(ioH, &inC, &eof);
        if (!eof)
        {
            putchar(inC);
        }
        else
        {
            printf("<EOF>\n");
        }
    }
    while (!eof);

    RegIOClose(ioH);
    return 0;
}

