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
 *        test_regdump.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser iconv character encoding translation
 *            test harness
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */

#include "includes.h"

#include "parse/regio.h"
#include "parse/reglex.h"
#include "parse/regiconv.h"
#include "parse/regparse.h"

BYTE testbuf[] = {
  0x49, 0x00, 0x4e, 0x00, 0x54, 0x00, 0x45,
  0x00, 0x4c, 0x00, 0x20, 0x00, 0x20, 0x00,
  0x2d, 0x00, 0x20, 0x00, 0x36, 0x00, 0x30,
  0x00, 0x34, 0x00, 0x30, 0x00, 0x30, 0x00,
  0x30, 0x00, 0x30, 0x00, 0x00, 0x00, 0x50,
  0x00, 0x68, 0x00, 0x6f, 0x00, 0x65, 0x00,
  0x6e, 0x00, 0x69, 0x00, 0x78, 0x00, 0x42,
  0x00, 0x49, 0x00, 0x4f, 0x00, 0x53, 0x00,
  0x20, 0x00, 0x34, 0x00, 0x2e, 0x00, 0x30,
  0x00, 0x20, 0x00, 0x52, 0x00, 0x65, 0x00,
  0x6c, 0x00, 0x65, 0x00, 0x61, 0x00, 0x73,
  0x00, 0x65, 0x00, 0x20, 0x00, 0x36, 0x00,
  0x2e, 0x00, 0x30, 0x00, 0x20, 0x00, 0x20,
  0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
  0x00, 0x00, 0x00, 0x00 };


void PrintASCII(char *buf, int buflen)
{
    int i;
    char c;
    for (i=0; i<buflen; i++)
    {
        c = buf[i];
        if (isprint((int)c))
        {
            putchar(c);
        }
    }
    printf("\n");
}


int main(int argc, char *argv[])
{
    FILE *fp = NULL;
    PIV_CONVERT_CTX iconvCtx = NULL;
    DWORD dwError;
    BYTE buf[9000];
    ssize_t bufLen;
    CHAR utf8String[BUFSIZ];
    PCHAR pszUTF8 = NULL;
    ssize_t utf8StringLen = 0;
    ssize_t inBytesUsed = 0;
    ssize_t offset = 0;



    if (argc == 1)
    {
        printf("usage: %s regfile\n", argv[0]);
        return 1;
    }

    dwError = RegIconvConvertOpen(&iconvCtx,
                                  REGICONV_ENCODING_UTF8,
                                  REGICONV_ENCODING_UCS2);
    if (dwError)
    {
        printf("RegIconvConvertOpen: failed %d\n", dwError);
        return 1;
    }

    inBytesUsed = 0;
    offset = 0;
    RegIconvConvertBuffer(iconvCtx,
                          testbuf,
                          sizeof(testbuf),
                          utf8String,
                          &inBytesUsed,
                          &utf8StringLen);
    pszUTF8 = utf8String;
    while (*pszUTF8)
    {
        printf("\nbytes2: left=%zd buf from file='%s'\n", inBytesUsed, pszUTF8);
        offset = strlen(pszUTF8) + 1;
        pszUTF8 += offset;
    }

    fp = fopen(argv[1], "r");
    if (!fp)
    {
        perror("fopen failed");
        return 1;
    }

    do {
        bufLen = fread(buf, 1, sizeof(buf), fp);
        if (bufLen > 0)
        {
            inBytesUsed = 0;
            offset = 0;
            do {
                RegIconvConvertBuffer(iconvCtx,
                                      buf + offset,
                                      bufLen,
                                      utf8String,
                                      &inBytesUsed,
                                      &utf8StringLen);
                printf("\n1: bytes %zd left=%zd buf from file='%s'\n", inBytesUsed, utf8StringLen, utf8String);
                offset += inBytesUsed;
                bufLen -= inBytesUsed;
            } while (inBytesUsed > 0);
        }
    } while (!feof(fp));


    RegIconvConvertClose(iconvCtx);
    dwError = RegIconvConvertOpen(&iconvCtx,
                                  REGICONV_ENCODING_UTF8,
                                  REGICONV_ENCODING_UCS2);
    RegIconvConvertClose(iconvCtx);
    return 0;
}
