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
 *        test_regparse.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser test harness
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */
#include <parse/includes.h>
#include <shellutil/rsutils.h>
#include <locale.h>


/* Just to demonstrate user context handle use. Not very useful otherwise */
typedef struct _USER_CONTEXT
{
    int (*pfn_fprintf)(FILE *stream, const char *fmt, ...);
    PIV_CONVERT_CTX ivHandle;
} USER_CONTEXT;


DWORD parseCallback(PREG_PARSE_ITEM pItem, HANDLE userContext)
{
    DWORD dwError = 0;
    CHAR tokenName[128];
    USER_CONTEXT *ctx = (USER_CONTEXT *) userContext;
    FILE *outStream = stdout;
    PWSTR *outMultiSz = NULL;
    PSTR   pszString = NULL;
    DWORD count = 0;

    ctx->pfn_fprintf(outStream, "parseCallback: Line number = %d\n", pItem->lineNumber);
    ctx->pfn_fprintf(outStream, "parseCallback: Key Name    = %s\n", pItem->keyName);
    if (pItem->valueName && pItem->valueLen)
    {
        ctx->pfn_fprintf(outStream, "parseCallback: Value name  = '%s'\n", pItem->valueName);
        ctx->pfn_fprintf(outStream, "parseCallback: Value length= %d\n", pItem->valueLen);
    }
    else
    {
        ctx->pfn_fprintf(outStream, "parseCallback: Value name  = (EMPTY)\n");
    }

    RegExportBinaryTypeToString(pItem->type, tokenName, FALSE);
    ctx->pfn_fprintf(outStream, "parseCallback: Value type   = %d (%s)\n",
           pItem->valueType, tokenName);
    ctx->pfn_fprintf(outStream, "parseCallback: Data type   = %d (%s) - ", pItem->type, tokenName);
    switch (pItem->type)
    {
        case REG_SZ:
            ctx->pfn_fprintf(outStream, "'%*s'\n", pItem->valueLen, (PCHAR) pItem->value);
            break;

        case REG_MULTI_SZ:
            RegByteArrayToMultiStrsW(
                pItem->value,
                pItem->valueLen,
                &outMultiSz);
            printf("\n");
            for (count=0; outMultiSz[count]; count++)
            {
                if (pszString)
                {
                	RegMemoryFree(pszString);
                    pszString = NULL;
                }

            	dwError = RegCStringAllocateFromWC16String(&pszString, outMultiSz[count]);
                BAIL_ON_REG_ERROR(dwError);

                printf("outMultiSz[%d] = '%s'\n", count, pszString);
            }
            if (outMultiSz)
            {
                RegFreeMultiStrsW(outMultiSz);
                outMultiSz = NULL;
            }

            break;

        case REG_DWORD:
            ctx->pfn_fprintf(outStream, "0x%08x\n", *((unsigned int *) pItem->value));
            break;

        case REG_QWORD:
            ctx->pfn_fprintf(outStream, "0x%016llx\n", *((ULONG64 *) pItem->value));
            break;

        case REG_BINARY:
        case REG_EXPAND_SZ:
        case REG_RESOURCE_REQUIREMENTS_LIST:
        case REG_RESOURCE_LIST:
        case REG_FULL_RESOURCE_DESCRIPTOR:
        case REG_NONE:
            RegParsePrintBinaryData(pItem->value, pItem->valueLen);
            break;

        default:
            break;
    }
    ctx->pfn_fprintf(outStream, "parseCallback: >>>\n\n");

error:

    if (pszString)
    {
    	RegMemoryFree(pszString);
    }
    if (outMultiSz)
    {
        RegFreeMultiStrsW(outMultiSz);
        outMultiSz = NULL;
    }

    return dwError;
}


int main(int argc, char *argv[])
{
    DWORD dwError;
    HANDLE parseH = NULL;
    USER_CONTEXT ctx = {0};


    if (argc == 1)
    {
        printf("usage: %s regfile.reg\n", argv[0]);
        return 0;
    }
    setlocale(LC_ALL, "");
    dwError = RegIconvConvertOpen(&ctx.ivHandle,
                                  REGICONV_ENCODING_UTF8,
                                  REGICONV_ENCODING_UCS2);
    BAIL_ON_REG_ERROR(dwError);


    ctx.pfn_fprintf = (int (*)(FILE *, const char *, ...)) fprintf;
    dwError = RegParseOpen(argv[1], parseCallback, &ctx, &parseH);
    BAIL_ON_REG_ERROR(dwError);
    if (dwError)
    {
        fprintf(stderr, "RegParseOpen: failed %d\n", dwError);
        return 1;
    }

    dwError = RegParseRegistry(parseH);
    BAIL_ON_REG_ERROR(dwError);

    RegIconvConvertClose(ctx.ivHandle);
    RegParseClose(parseH);

cleanup:
    return dwError;

error:
    goto cleanup;
}
