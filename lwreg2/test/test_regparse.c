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
#include <reg/lwreg.h>


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
    BOOLEAN bIsAttrData,
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
                             dwIndent,
                             pszIndentStr,
                             pItem->lineNumber, typeString, pItem->type,
                             pItem->keyName);
            break;
        case REG_SZ:
            ctx->pfn_fprintf(outStream, 
                             "%*s%5d: {%s/(%d)} \"%s\"=\"%*s\"\n",
                             dwIndent,
                             pszIndentStr,
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
                             dwIndent,
                             pszIndentStr,
                             pItem->lineNumber, 
                             typeString,
                             pItem->type,
                             pItem->valueName);
            /*
             * Possibly an inconsistency here. The registry schema
             * clearly defines pwszRangeEnumStrings to be a null terminated
             * array of pointers to wide character strings. The data 
             * passed in from the parser for MULTI_SZ is the marshalled 
             * (byte array) form of this data. That is why this call
             * is needed here.
             */
            if (!bIsAttrData)
            {
                RegByteArrayToMultiStrsW(
                    pItem->value,
                    pItem->valueLen,
                    &outMultiSz);
            }
            else
            {
                outMultiSz = pItem->regAttr.Range.ppwszRangeEnumStrings;
            }
            if (outMultiSz)
            {
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
    
                    ctx->pfn_fprintf(outStream,
                        "%*s    REG_MULTI_SZ[%d] = '%s'\n",
                        dwIndent,
                        pszIndentStr,
                        count,
                        pszString);
                    LWREG_SAFE_FREE_MEMORY(pszString);
                }
            }
            ctx->pfn_fprintf(outStream, "\n");
            if (!bIsAttrData)
            {
                RegFreeMultiStrsW(outMultiSz);
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
    PSTR pszHint = NULL;

    BAIL_ON_INVALID_HANDLE(userContext);
    BAIL_ON_INVALID_POINTER(outStream);
    pValue = pItem->value ?
                 pItem->value : pItem->regAttr.pDefaultValue;
    RegExportBinaryTypeToString(pItem->type, typeString, FALSE);
    switch (pItem->type)
    {
        case REG_ATTRIBUTES:
            ctx->pfn_fprintf(outStream, "%5d: {%s/(%d} \"%s\"= {\n", 
                pItem->lineNumber,
                typeString,
                pItem->type,
                pItem->valueName);
              
            if (pItem->regAttr.RangeType == LWREG_VALUE_RANGE_TYPE_INTEGER)
            {
                ctx->pfn_fprintf(outStream, 
                    "     %5d: {%s} \"range\"=integer:%d-%d\n", 
                    pItem->lineNumber,
                    "LWREG_VALUE_RANGE_TYPE_INTEGER",
                    pItem->regAttr.Range.RangeInteger.Min,
                    pItem->regAttr.Range.RangeInteger.Max);
            }
            else if (pItem->regAttr.RangeType == LWREG_VALUE_RANGE_TYPE_ENUM)
            {
                ctx->pfn_fprintf(outStream, 
                    "     %5d: {%s} \"range\"=string:\n", 
                    pItem->lineNumber,
                    "LWREG_VALUE_RANGE_TYPE_ENUM");
                /* 
                 * Synthesize a REG_MULTI for the "range" attribute for
                 * ENUM type.
                 */
                schemaItem = *pItem;
                schemaItem.type = REG_MULTI_SZ;
                schemaItem.valueName = "range";
                schemaItem.value = pItem->regAttr.Range.ppwszRangeEnumStrings;
                parseCallbackPrintData(ctx, &schemaItem, TRUE, 10);
            }
            else if (pItem->regAttr.RangeType ==
                     LWREG_VALUE_RANGE_TYPE_BOOLEAN &&
                     pValue)
            {
                dwValue = *((PDWORD) pValue);
                ctx->pfn_fprintf(outStream, 
                    "      %5d: {%s} \"range\"=boolean:%s (%d)\n", 
                    pItem->lineNumber,
                    "LWREG_VALUE_RANGE_TYPE_BOOLEAN",
                    dwValue ? "TRUE" : "FALSE",
                    dwValue ? 1 : 0);
            }

            /* Handle the "hint" attribute */
            pszHint = RegFindHintByValue(pItem->regAttr.Hint);
            if (pszHint)
            {
                ctx->pfn_fprintf(outStream, 
                    "     %5d: {%s} \"hint\"=%s (%d)\n", 
                    pItem->lineNumber,
                    "Hint",
                    pszHint,
                    pItem->regAttr.Hint);
            }
            
            /* Handle "doc" string attribute */
            if (pItem->regAttr.pwszDocString)
            {
                /* Synthesize a REG_SZ for the "doc" attribute */
                schemaItem = *pItem;
                schemaItem.type = REG_SZ;
                schemaItem.valueName = "doc";
                dwError = LwRtlCStringAllocateFromWC16String(
                              (PSTR *) &schemaItem.value,
                              pItem->regAttr.pwszDocString);
                parseCallbackPrintData(ctx, &schemaItem, TRUE, 5);
                LWREG_SAFE_FREE_MEMORY(schemaItem.value);
            }

            if (pItem->value)
            {
                /* Handle data value (non-attribute data */
                schemaItem = *pItem;
                schemaItem.type = pItem->regAttr.ValueType;
                schemaItem.value = pItem->value;
                schemaItem.valueLen = pItem->valueLen;
                schemaItem.valueName = "value";
                parseCallbackPrintData(ctx, &schemaItem, TRUE, 5);
            }
            if (pItem->regAttr.pDefaultValue)
            {
                /* Handle data value (non-attribute data */
                schemaItem = *pItem;
                schemaItem.type = pItem->regAttr.ValueType;
                schemaItem.value = pItem->regAttr.pDefaultValue;
                schemaItem.valueLen = pItem->regAttr.DefaultValueLen;
                schemaItem.valueName = "default";
                parseCallbackPrintData(ctx, &schemaItem, TRUE, 5);
            }
            ctx->pfn_fprintf(outStream, "    }\n\n");
            break;

        default:
            parseCallbackPrintData(ctx, pItem, FALSE, 0);
            break;
    }

cleanup:
    return dwError;

error:
    goto cleanup;

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

cleanup:
    RegIconvConvertClose(ctx.ivHandle);
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
}
