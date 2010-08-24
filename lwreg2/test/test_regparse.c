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



DWORD
parseCallbackPrintData(
    USER_CONTEXT *ctx,
    PREG_PARSE_ITEM pItem,
    DWORD dwIndent)
{
    CHAR typeString[128];
    PWSTR  *outMultiSz = NULL;
    PSTR   pszString = NULL;
    FILE *outStream = stdout;
    DWORD count = 0;
    DWORD dwError = 0;
    PSTR pszIndentStr = NULL;

    pszIndentStr = dwIndent ? " " : "";

    RegExportBinaryTypeToString(pItem->type, typeString, FALSE);
    switch (pItem->type)
    {
        case REG_KEY:
            ctx->pfn_fprintf(outStream, 
                             "%*s%5d: {%s/(%d)} [%s]\n", 
                             dwIndent, pszIndentStr,
                             pItem->lineNumber, typeString, pItem->type,
                             pItem->keyName);
            break;
        case REG_SZ:
            ctx->pfn_fprintf(outStream, 
                             "%*s%5d: {%s/(%d)} \"%s\"=\"%*s\"\n",
                             dwIndent, pszIndentStr,
                             pItem->lineNumber,
                             typeString,
                             pItem->type,
                             pItem->valueName,
                             pItem->valueLen,
                             (PCHAR) pItem->value);
            break;

        case REG_MULTI_SZ:
            ctx->pfn_fprintf(outStream,
                             "%*s%5d: {%s/(%d)} \"%s\"=\n",
                             dwIndent, pszIndentStr,
                             pItem->lineNumber, 
                             typeString,
                             pItem->type,
                             pItem->valueName);
            RegByteArrayToMultiStrsW(
                pItem->value,
                pItem->valueLen,
                &outMultiSz);
            for (count=0; outMultiSz[count]; count++)
            {
                if (pszString)
                {
                    RegMemoryFree(pszString);
                    pszString = NULL;
                }

                dwError = RegCStringAllocateFromWC16String(
                              &pszString,
                              outMultiSz[count]);
                BAIL_ON_REG_ERROR(dwError);

                printf("    REG_MULTI_SZ[%d] = '%s'\n", count, pszString);
            }
            printf("\n");

            if (outMultiSz)
            {
                RegFreeMultiStrsW(outMultiSz);
                outMultiSz = NULL;
            }

            break;

        case REG_DWORD:
            ctx->pfn_fprintf(outStream, "%*s%5d: {%s/(%d)} \"%s\"=0x%08x\n", 
                             dwIndent, pszIndentStr,
                             pItem->lineNumber,
                             typeString,
                             pItem->type,
                             pItem->valueName, 
                             *((unsigned int *) pItem->value));
            break;

        case REG_QWORD:
            ctx->pfn_fprintf(outStream, 
                             "%*s%5d: {%s\(%d)} \"%s\"=0x%016llx\n",
                             dwIndent, pszIndentStr,
                             pItem->lineNumber,
                             typeString,
                             pItem->type,
                             pItem->valueName, 
                             *((ULONG64 *) pItem->value));
            break;

        case REG_BINARY:
        case REG_EXPAND_SZ:
        case REG_RESOURCE_REQUIREMENTS_LIST:
        case REG_RESOURCE_LIST:
        case REG_FULL_RESOURCE_DESCRIPTOR:
        case REG_NONE:
            ctx->pfn_fprintf(outStream, 
                             "%*s%5d: {%s\(%d)} \"%s\"=",
                             dwIndent, pszIndentStr,
                             pItem->lineNumber,
                             typeString,
                             pItem->type,
                             pItem->valueName);
            RegParsePrintBinaryData(pItem->value, pItem->valueLen);
            break;


        default:
            break;
    }
cleanup:
    return dwError;

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
    goto cleanup;
}


DWORD parseCallback(PREG_PARSE_ITEM pItem, HANDLE userContext)
{
    CHAR typeString[128];
    USER_CONTEXT *ctx = (USER_CONTEXT *) userContext;
    FILE *outStream = stdout;
    DWORD dwError = 0;
    REG_PARSE_ITEM schemaItem = {0};
    PVOID pValue = NULL;
    DWORD dwValue = 0;
    PSTR pszDocString = NULL;

    pValue = pItem->regAttr.CurrentValue ?
                 pItem->regAttr.CurrentValue : pItem->regAttr.DefaultValue;
    RegExportBinaryTypeToString(pItem->type, typeString, FALSE);
    switch (pItem->type)
    {
        case REG_ATTRIBUTES:
            ctx->pfn_fprintf(outStream, "%5d: {%s/(%d)} \"%s\"= {\n", 
                pItem->lineNumber,
                typeString,
                pItem->type,
                pItem->valueName);
              
            if (pItem->regAttr.RangeType == LWREG_VALUE_RANGE_TYPE_INTEGER)
            {
                ctx->pfn_fprintf(outStream, 
                    "     %5d: {%s/(%d)} \"range\"=%d-%d\n", 
                    pItem->lineNumber,
                    "REG_RANGE",
                    101,
                    pItem->regAttr.Range.RangeInteger.Min,
                    pItem->regAttr.Range.RangeInteger.Max);
            }
            else if (pItem->regAttr.RangeType == LWREG_VALUE_RANGE_TYPE_ENUM)
            {
                printf("RegAttrEnum...\n");
            }
            else if (pItem->regAttr.RangeType ==
                     LWREG_VALUE_RANGE_TYPE_BOOLEAN &&
                     pValue)
            {
                dwValue = *((PDWORD) pValue);
                ctx->pfn_fprintf(outStream, 
                    "      %5d: {%s/(%d)} \"boolean\"=%d\n", 
                    pItem->lineNumber,
                    "REG_BOOLEAN",
                    100,
                    dwValue ? 1 : 0);
            }

            /* Handle "doc" string attribute */
            if (pItem->regAttr.DocString)
            {
                dwError = LwRtlCStringAllocateFromWC16String(
                              &pszDocString,
                              pItem->regAttr.DocString);
                BAIL_ON_REG_ERROR(dwError);

                /* Synthesize a REG_SZ for the "doc" attribute */
                schemaItem = *pItem;
                schemaItem.type = REG_SZ;
                schemaItem.valueName = "doc";
                schemaItem.value = pszDocString;
                parseCallbackPrintData(ctx, &schemaItem, 5);
                LWREG_SAFE_FREE_STRING(pszDocString);
            }

            if (pItem->value && *((PSTR)pItem->value))
            {
                /* Handle data value (non-attribute data */
                schemaItem = *pItem;
                schemaItem.type = pItem->regAttr.ValueType;
                schemaItem.value = pValue;
                parseCallbackPrintData(ctx, &schemaItem, 5);
            }
            ctx->pfn_fprintf(outStream, "    }\n\n");
            break;

        default:
            parseCallbackPrintData(ctx, pItem, 0);
            break;
    }

cleanup:
    return dwError;

error:
    goto cleanup;

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
