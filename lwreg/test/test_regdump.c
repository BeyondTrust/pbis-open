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
 *        Registry .REG parser export functionality test harness
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
    int userValue;
    int count;
    DWORD refCB;
    HANDLE parseH;
    REG_DATA_TYPE prevType;
} USER_CONTEXT;


DWORD parseCallback(PREG_PARSE_ITEM pItem, HANDLE userContext)
{
    PSTR dumpString = NULL;
    DWORD dumpStringLen = 0;
    USER_CONTEXT *ctx = (USER_CONTEXT *) userContext;

    RegExportAttributes(
        pItem,
        &dumpString,
        &dumpStringLen);

    if (dumpStringLen > 0 && dumpString)
    {
        switch (pItem->type)
        {
            case REG_KEY:
                printf("\r\n%.*s\r\n", dumpStringLen, dumpString);
                break;

            case REG_PLAIN_TEXT:
                if (ctx->prevType && ctx->prevType != pItem->type)
                {
                    printf("\n");
                }
                printf("%*s ", pItem->valueLen, (PCHAR) pItem->value);
                break;

            case REG_ATTRIBUTES:
                printf("%.*s\r\n\r\n", dumpStringLen, dumpString);
                break;

            default:
                printf("%.*s\r\n", dumpStringLen, dumpString);
                break;
        }
    }
    fflush(stdout);
    ctx->prevType = pItem->type;

    if (dumpString)
    {
    	RegMemoryFree(dumpString);
        dumpString = NULL;
    }
    return 0;
}


DWORD parseCallbackDebug(PREG_PARSE_ITEM pItem, HANDLE userContext)
{
    CHAR tokenName[128];
    CHAR valueName[128];
    USER_CONTEXT *ctx = (USER_CONTEXT *) userContext;
    FILE *outStream = stderr;

    ctx->count++;

    ctx->pfn_fprintf(outStream, "parseCallback: <<< %d\n", ctx->userValue);
    ctx->pfn_fprintf(outStream, "parseCallback: Line number = %d\n", pItem->lineNumber);
    ctx->pfn_fprintf(outStream, "parseCallback: Key Name    = %s\n", pItem->keyName);
    if (pItem->valueName)
    {
        ctx->pfn_fprintf(outStream, "parseCallback: Value name  = '%s'\n", pItem->valueName);
        ctx->pfn_fprintf(outStream, "parseCallback: Value length= %d\n", pItem->valueLen);
    }
    else
    {
        ctx->pfn_fprintf(outStream, "parseCallback: Value name  = (EMPTY)\n");
    }

    RegLexTokenToString(pItem->type, tokenName, sizeof(tokenName));
    RegLexTokenToString(pItem->valueType, valueName, sizeof(tokenName));
    ctx->pfn_fprintf(outStream, "parseCallback: Value type   = %d (%s)\n",
           pItem->valueType, valueName);
    ctx->pfn_fprintf(outStream, "parseCallback: Data type   = %d (%s) - ", pItem->type, tokenName);
    switch (pItem->type)
    {
        case REG_SZ:
        case REG_PLAIN_TEXT:
            ctx->pfn_fprintf(outStream, "'%*s'\n", pItem->valueLen, (PCHAR) pItem->value);
            break;

        case REG_MULTI_SZ:
            RegParsePrintASCII(pItem->value, pItem->valueLen);
            break;

        case REG_DWORD:
            ctx->pfn_fprintf(outStream, "0x%08x\n", *((unsigned int *) pItem->value));
            break;

        case REG_QWORD:
            ctx->pfn_fprintf(outStream, "0x%016llx\n", *((unsigned long long *) pItem->value));
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

    return 0;
}


int main(int argc, char *argv[])
{
    DWORD dwError = 0;
    DWORD dwLineNum = 0;
    HANDLE parseH = NULL;
    USER_CONTEXT ctx = {0};
    CHAR cErrorBuf[1024] = {0};


    if (argc == 1)
    {
        printf("usage: %s regfile.reg\n", argv[0]);
        return 0;
    }

    setlocale(LC_ALL, "");
    dwError = RegParseOpen(argv[1], NULL, NULL, &parseH);
    if (dwError)
    {
        fprintf(stderr, "RegParseOpen: failed %d\n", dwError);
        return 1;
    }

    ctx.pfn_fprintf = (int (*)(FILE *, const char *, ...)) fprintf;
    ctx.userValue = 314159;

#ifdef _DEBUG
    RegParseInstallCallback(parseH, parseCallbackDebug, &ctx, &ctx.refCB);
#endif
    ctx.parseH = parseH;
    printf("Installed callback; handle='%d'\n", ctx.refCB);

    RegParseInstallCallback(parseH, parseCallback, &ctx, NULL);

    dwError = RegParseRegistry(parseH);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    RegParseClose(parseH);
    return dwError ? 1 : 0;

error:
    RegParseGetLineNumber(parseH, &dwLineNum);
    LwRegGetErrorString(
        dwError,
        cErrorBuf,
        sizeof(cErrorBuf) - 1);

    printf("RegParseRegistry: '%s' (%d) line=%d\n", 
           cErrorBuf, dwError, dwLineNum);

    goto cleanup;

    return 0;
}
