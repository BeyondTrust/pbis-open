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

