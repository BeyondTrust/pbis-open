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
 *        regparse.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */

/*
 * Parser implementation:
 *
 *                REG_KEY
 *                   |
 *          +--------+---------+
 *          |                  |
 *        Empty              KeyValue
 *
 *
 *               KeyValue
 *                  +
 *                  |
 *       +----------+--------+-------+
 *       |          |        |       |
 *  NameDefault  ValueName  "="  TypeValue
 *     ("@")      (REG_SZ)
 *
 *
 *                          TypeValue
 *                              |
 *      +----------+------------+------------+-------------+-----...
 *      |          |            |            |             |
 *  ValueNone  ValueDword  ValueBinary  ValueString ValueMultiString
 *  (REG_NONE) (REG_DWORD) (REG_BINARY) (REG_SZ)    (REG_MULTI_SZ)
 *    [hex(0)]                [hex]                   [hex(7)]
 *
 * ...---+-----------------+-----------------------+----------...
 *       |                 |                       |
 *   ValueExpandSz    ValueQuaword   ValueResourceRequirementsList
 *   (REG_EXPAND_SZ)  (REG_QUADWORD) (REG_RESOURCE_REQUIREMENTS_LIST)
 *      [hex(2)]         [hex(b)]         [hex(a)]
 *
 *
 * ...-------+-----------------------+-------------------------+
 *           |                       |                         |
 *   ValueResourceList    FullResourceDescriptor       ValueStringArray
 *   (REG_RESOURCE_LIST)  (REG_FULL_RESOURCE_LIST)    (REG_MULTI_SZ)
 *         [hex(8)]             [hex(9)]                    [sza]
 *
 * Note: All "hex()" types are TypeBinary, a sequence of binary
 *       hex values.
 *
 *
 *            TypeBinary
 *               |
 *        +-->(HexPair)---->(HexPairEnd)
 *        |      |
 *        |      |
 *        +------+
 */

#include "includes.h"

DWORD 
RegParseAttributes(PREGPARSE_HANDLE parseHandle);


static void 
RegParseExternDataType(
    REGLEX_TOKEN valueType,
    PREG_DATA_TYPE externValueType
    )
{

    if (!externValueType)
    {
        return;
    }
    switch (valueType)
    {
        case REGLEX_REG_DWORD:
            *externValueType = REG_DWORD;
            break;
        case REGLEX_REG_SZ:
            *externValueType = REG_SZ;
            break;
        case REGLEX_REG_BINARY:
            *externValueType = REG_BINARY;
            break;
        case REGLEX_REG_NONE:
            *externValueType = REG_NONE;
            break;
        case REGLEX_REG_EXPAND_SZ:
            *externValueType = REG_EXPAND_SZ;
            break;
        case REGLEX_REG_MULTI_SZ:
            *externValueType = REG_MULTI_SZ;
            break;
        case REGLEX_REG_RESOURCE_LIST:
            *externValueType = REG_RESOURCE_LIST;
            break;
        case REGLEX_REG_FULL_RESOURCE_DESCRIPTOR:
            *externValueType = REG_FULL_RESOURCE_DESCRIPTOR;
            break;
        case REGLEX_REG_RESOURCE_REQUIREMENTS_LIST:
            *externValueType = REG_RESOURCE_REQUIREMENTS_LIST;
            break;
        case REGLEX_REG_QUADWORD:
            *externValueType = REG_QWORD;
            break;
        case REGLEX_REG_KEY:
            *externValueType = REG_KEY;
            break;
        case REGLEX_KEY_NAME_DEFAULT:
            *externValueType = REG_KEY_DEFAULT;
            break;
        case REGLEX_REG_ATTRIBUTES:
            *externValueType = REG_ATTRIBUTES;
            break;
        case REGLEX_PLAIN_TEXT:
        default:
            *externValueType = REG_PLAIN_TEXT;
            break;
    }
}


/*
 * Does same job as RegParseExternDataType(), but is aware of
 * regAttr data type, and when set, returns the type of the attribute.
 */
static void
RegParseAttributesExternDataType(
    PREGPARSE_HANDLE parseHandle,
    PREG_DATA_TYPE externValueType
    )
{

    if (parseHandle->registryEntry.type == REG_ATTRIBUTES)
    {
        *externValueType = parseHandle->registryEntry.regAttr.ValueType;
    }
    else
    {
        RegParseExternDataType(
            parseHandle->registryEntry.type,
            externValueType);
    }
}



DWORD
RegParseTypeNone(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD dwError = 0;

    parseHandle->dataType = REGLEX_REG_NONE;
    RegParseBinaryData(parseHandle);

    return dwError;
}

static DWORD
RegParseReAllocateData(
    PREGPARSE_HANDLE parseHandle,
    DWORD dwAppendLen
    )
{
    DWORD dwError = 0;
    DWORD newValueSize = 0;
    PVOID pNewMemory = NULL;

    BAIL_ON_INVALID_POINTER(parseHandle);

    if ((parseHandle->binaryDataLen + dwAppendLen) >= 
         parseHandle->binaryDataAllocLen)
    {
        newValueSize = parseHandle->binaryDataAllocLen * 2 + dwAppendLen;
        dwError = RegReallocMemory(
                      parseHandle->binaryData,
                      &pNewMemory,
                      newValueSize);
        BAIL_ON_REG_ERROR(dwError);

        parseHandle->binaryData = pNewMemory;
        parseHandle->binaryDataAllocLen = newValueSize;
    }

cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(pNewMemory);
    goto cleanup;
}


DWORD
RegParseIsValidAttribute(
    PSTR pszAttr,
    PBOOLEAN pbIsAttribute)
{
    static PSTR pszAttrArray[] =
        { "value",
          "default",
          "doc",
          "range",
          "hint",
          NULL };

    DWORD dwI = 0;
    DWORD dwError = 0;
    BOOLEAN bFound = FALSE;

    BAIL_ON_INVALID_POINTER(pszAttr);

    for (dwI=0; pszAttrArray[dwI]; dwI++)
    {
        if (!strcmp(pszAttrArray[dwI], pszAttr))
        {
            bFound = TRUE;
            break;
        }
    }
    
    *pbIsAttribute = bFound;

cleanup:
    return dwError;
  
error:
     goto cleanup;
}


DWORD
RegParseAssignAttrData(
    PREGPARSE_HANDLE parseHandle,
    PVOID pData,
    DWORD dwDataLen
    )
{
    DWORD dwError = 0;
    PVOID pvData = NULL;
    PWSTR pwszDocString = NULL;
    PWSTR *ppwszEnumString = NULL;
    BOOLEAN bIsAttr = FALSE;

    /* regAttr contains memory that must be freed by caller */

    if (parseHandle->lexHandle->eValueNameType == REGLEX_VALUENAME_ATTRIBUTES)
    {
        dwError = RegParseIsValidAttribute(
                      parseHandle->attrName,
                      &bIsAttr);
        BAIL_ON_REG_ERROR(dwError);
        if (!bIsAttr)
        {
            dwError = LWREG_ERROR_PARSE;
            BAIL_ON_REG_ERROR(dwError);
        }
    }

    if (parseHandle->lexHandle->eValueNameType ==
        REGLEX_VALUENAME_ATTRIBUTES && pData)
    {
        if (!strcmp(parseHandle->attrName, "value"))
        {
            dwError = RegAllocateMemory(dwDataLen + 1, (LW_PVOID) &pvData);
            BAIL_ON_REG_ERROR(dwError);
            memcpy(pvData, pData, dwDataLen);

            LWREG_SAFE_FREE_MEMORY(
                parseHandle->pCurrentAttrValue);
            parseHandle->pCurrentAttrValue = pvData;
            parseHandle->registryEntry.value = pvData;
            parseHandle->dwCurrentAttrValueLen = dwDataLen;
            parseHandle->registryEntry.valueLen = dwDataLen;
            RegParseExternDataType(
                parseHandle->dataType, 
                (PREG_DATA_TYPE) &parseHandle->registryEntry.regAttr.ValueType);
            parseHandle->bTypeSet = TRUE;
        }
        else if (!strcmp(parseHandle->attrName, "default"))
        {
            dwError = RegAllocateMemory(dwDataLen + 1, (LW_PVOID) &pvData);
            BAIL_ON_REG_ERROR(dwError);
            memcpy(pvData, pData, dwDataLen);

            LWREG_SAFE_FREE_MEMORY(
                parseHandle->registryEntry.regAttr.pDefaultValue);
            parseHandle->registryEntry.regAttr.pDefaultValue = pvData;
            parseHandle->registryEntry.regAttr.DefaultValueLen = dwDataLen;
            RegParseExternDataType(
                parseHandle->dataType, 
                (PREG_DATA_TYPE) &parseHandle->registryEntry.regAttr.ValueType);
            parseHandle->bTypeSet = TRUE;
        }
        else if (!strcmp(parseHandle->attrName, "doc"))
        {
            dwError = LwRtlWC16StringAllocateFromCString(
                          &pwszDocString,
                          pData);
            BAIL_ON_REG_ERROR(dwError);

            LWREG_SAFE_FREE_MEMORY(
                parseHandle->registryEntry.regAttr.pwszDocString);
            parseHandle->registryEntry.regAttr.pwszDocString = pwszDocString;
        }
        else if (!strcmp(parseHandle->attrName, "range"))
        {
            if (parseHandle->registryEntry.regAttr.RangeType)
            {
                /*
                 * Range type is already set and not enum, so this is an error.
                 */
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                BAIL_ON_REG_ERROR(dwError);
            }

            if (parseHandle->registryEntry.type == REG_MULTI_SZ)
            {
                RegByteArrayToMultiStrsW(
                    pData,
                    dwDataLen,
                    &ppwszEnumString);

                if (parseHandle->registryEntry.regAttr.Range.ppwszRangeEnumStrings)
                {
                    RegFreeMultiStrsW(parseHandle->registryEntry.
                                      regAttr.Range.ppwszRangeEnumStrings);
                    parseHandle->registryEntry.
                        regAttr.Range.ppwszRangeEnumStrings = NULL;
                }
                parseHandle->registryEntry.regAttr.Range.ppwszRangeEnumStrings =
                    ppwszEnumString;
                parseHandle->registryEntry.regAttr.RangeType =
                    LWREG_VALUE_RANGE_TYPE_ENUM;
            }
        }
        else if (!strcmp(parseHandle->attrName, "hint"))
        {
            parseHandle->registryEntry.regAttr.Hint =
                RegFindHintByName((PSTR) pData);
        }
        else if (parseHandle->bTypeSet)
        {
            RegParseExternDataType(
                parseHandle->dataType, 
                (PREG_DATA_TYPE) &parseHandle->registryEntry.regAttr.ValueType);
        }
    }

cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(pvData);
    LWREG_SAFE_FREE_MEMORY(pwszDocString);
    LWREG_SAFE_FREE_MEMORY(ppwszEnumString);
    goto cleanup;
}


DWORD 
RegParseAppendData(
    PREGPARSE_HANDLE parseHandle,
    PSTR pszHexValue
    )
{
    DWORD dwError = 0;
    DWORD attrSize = 0;
    PSTR pszAttr = NULL;
    DWORD binaryValue = 0;
    REG_DATA_TYPE eDataType = 0;

    BAIL_ON_INVALID_POINTER(parseHandle);

    RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);

    /* Realloc internal buffer if worst case size (4 bytes) isn't available */
    dwError = RegParseReAllocateData(parseHandle, 4);
    BAIL_ON_REG_ERROR(dwError);

    switch(parseHandle->dataType)
    {
        case REGLEX_REG_DWORD:
            binaryValue = strtoul(pszHexValue,
                                  NULL,
                                  16);
            memcpy(&parseHandle->binaryData[parseHandle->binaryDataLen],
                   &binaryValue,
                   sizeof(binaryValue));
            parseHandle->binaryDataLen += sizeof(binaryValue);
            eDataType = REG_DWORD;
            break;

        case REGLEX_REG_MULTI_SZ:
        case REGLEX_REG_BINARY:
        case REGLEX_REG_EXPAND_SZ:
        case REGLEX_REG_RESOURCE_REQUIREMENTS_LIST:
        case REGLEX_REG_RESOURCE_LIST:
        case REGLEX_REG_FULL_RESOURCE_DESCRIPTOR:
        case REGLEX_REG_QUADWORD:
        case REGLEX_REG_NONE:
            /* Need to enforce only 2 bytes are being converted */
            binaryValue = strtoul(pszHexValue,
                                  NULL,
                                  16);
            parseHandle->binaryData[parseHandle->binaryDataLen] = binaryValue;
            parseHandle->binaryDataLen++;
            break;

        default:
            break;
    }

    dwError = RegParseAssignAttrData(
        parseHandle,
        (PVOID) parseHandle->binaryData,
        parseHandle->binaryDataLen);
    
cleanup:
    return dwError;

error:
    goto cleanup;
}


void RegParsePrintBinaryData(
    PUCHAR binaryData,
    DWORD binaryDataLen
    )
{
    int i;
    for (i=0; i<binaryDataLen; i++)
    {
        printf("%02X ", binaryData[i]);
    }
    printf("\n");
}


DWORD
RegParseBinaryData(
    PREGPARSE_HANDLE parseHandle
    )
{
    PSTR dataName = NULL;
    BOOLEAN eof = FALSE;
    REGLEX_TOKEN token = 0;
    DWORD attrSize = 0;
    PSTR pszAttr = NULL;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    CHAR tokenName[256];

    BAIL_ON_INVALID_POINTER(parseHandle);

    parseHandle->binaryDataLen = 0;
    RegParseExternDataType(parseHandle->dataType,
                           &parseHandle->registryEntry.type);

    switch (parseHandle->dataType)
    {
        case REGLEX_REG_MULTI_SZ:
            dataName = "MultiStringValue";
            break;

        case REGLEX_REG_BINARY:
            dataName = "Binary";
            break;

        case REGLEX_REG_EXPAND_SZ:
            dataName = "ExpandString";
            break;

        case REGLEX_REG_RESOURCE_REQUIREMENTS_LIST:
            dataName = "ResourceRequirementsList";
            break;

        case REGLEX_REG_RESOURCE_LIST:
            dataName = "ResourceList";
            break;

        case REGLEX_REG_FULL_RESOURCE_DESCRIPTOR:
            dataName = "FullResourceDescriptor";
            break;

        case REGLEX_REG_QUADWORD:
            dataName = "Quadword";
            break;

        case REGLEX_REG_NONE:
            dataName = "REG_NONE";
            break;

        default:
            break;
    }

    do {
        dwError = RegLexGetToken(parseHandle->ioHandle,
                                 parseHandle->lexHandle,
                                 &token,
                                 &eof);
        if (eof)
        {
            return dwError;
        }

        RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
        RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);
        RegLexTokenToString(token, tokenName, sizeof(tokenName));
        if (token == REGLEX_HEXPAIR)
        {
            dwError = RegParseAppendData(
                          parseHandle,
                          parseHandle->lexHandle->curToken.pszValue);
            BAIL_ON_REG_ERROR(dwError);
        }
        else if (token == REGLEX_HEXPAIR_END)
        {
            if (parseHandle->lexHandle->curToken.valueCursor == 0)
            {
                return dwError;
            }

            dwError = RegParseAppendData(
                          parseHandle,
                          parseHandle->lexHandle->curToken.pszValue);
            BAIL_ON_REG_ERROR(dwError);
            if (parseHandle->lexHandle->eValueNameType !=
                REGLEX_VALUENAME_ATTRIBUTES)
            {
                parseHandle->registryEntry.valueLen =
                    parseHandle->binaryDataLen;
                parseHandle->registryEntry.value =
                    parseHandle->binaryData;
            }

            return dwError;
        }
        else
        {
            dwError = LWREG_ERROR_SYNTAX;
            return dwError;
        }
    } while (!eof);


cleanup:
    return dwError;

error:

    goto cleanup;
}


DWORD
RegParseTypeBinary(
    PREGPARSE_HANDLE parseHandle
    )
{
    DWORD dwError = 0;

    RegParseBinaryData(parseHandle);
    return dwError;
}


DWORD
RegParseTypeExpandString(
    PREGPARSE_HANDLE parseHandle
    )
{
    DWORD dwError = 0;

    RegParseBinaryData(parseHandle);
    return dwError;
}


DWORD
RegParseTypeQuadWord(
    PREGPARSE_HANDLE parseHandle
    )
{
    DWORD dwError = 0;

    RegParseBinaryData(parseHandle);
    return dwError;
}


DWORD
RegParseTypeFullResDescriptor(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD dwError = 0;

    RegParseBinaryData(parseHandle);
    return dwError;
}


DWORD
RegParseTypeResourceReqList(
    PREGPARSE_HANDLE parseHandle
    )
{
    DWORD dwError = 0;

    RegParseBinaryData(parseHandle);
    return dwError;
}


DWORD
RegParseTypeResourceList(
    PREGPARSE_HANDLE parseHandle
    )
{
    DWORD dwError = 0;

    RegParseBinaryData(parseHandle);
    return dwError;
}



DWORD
RegParseTypeDword(
    PREGPARSE_HANDLE parseHandle
    )
{
    DWORD attrSize = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    PSTR pszAttr = NULL;

    if (parseHandle->dataType == REGLEX_REG_DWORD)
    {
        RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
        RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);

        parseHandle->binaryDataLen = 0;
        dwError = RegParseAppendData(parseHandle, pszAttr);
    }
    else
    {
        dwError = LWREG_ERROR_SYNTAX;
    }
    return dwError;
}


DWORD
RegParseTypeMultiStringValue(
    PREGPARSE_HANDLE parseHandle
    )
{
    DWORD attrSize = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    PSTR pszAttr = NULL;

    RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
    RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);

    RegParseBinaryData(parseHandle);
    dwError = RegParseAssignAttrData(
        parseHandle,
        (PVOID) parseHandle->registryEntry.value,
        parseHandle->registryEntry.valueLen);
    return dwError;
}


DWORD
RegParseTypeStringArrayValue(
    PREGPARSE_HANDLE parseHandle
    )
{
    DWORD attrSize = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    DWORD dwStrLen = 0;
    PSTR pszAttr = NULL;
    REGLEX_TOKEN token = 0;
    BOOLEAN eof = FALSE;
    NTSTATUS ntStatus = 0;
    PWSTR pwszString = NULL;

    RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
    RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);

    dwError = RegLexGetToken(parseHandle->ioHandle,
                             parseHandle->lexHandle,
                             &token,
                             &eof);
    if (eof)
    {
        return eof;
    }
    while (token == REGLEX_REG_SZ ||
           (token == REGLEX_PLAIN_TEXT && strcmp(pszAttr, "\\")==0))
    {
        RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
        RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);
        if (token == REGLEX_REG_SZ)
        {
            LWREG_SAFE_FREE_MEMORY(pwszString);
            ntStatus = LwRtlWC16StringAllocateFromCString(&pwszString, pszAttr);
            if (ntStatus)
            {
                goto error;
            }

            dwStrLen = wc16slen(pwszString) * 2 + 2;
            /* 
             * This tests if current buffer is big enough and allocates more 
             * space if needed.
             */
            dwError = RegParseReAllocateData(parseHandle, dwStrLen);
            BAIL_ON_REG_ERROR(dwError);
            memcpy(&parseHandle->binaryData[parseHandle->binaryDataLen],
                   pwszString,
                   dwStrLen);
            parseHandle->binaryDataLen += dwStrLen;
        }

        dwError = RegLexGetToken(parseHandle->ioHandle,
                                 parseHandle->lexHandle,
                                 &token,
                                 &eof);
        RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
    }
    parseHandle->binaryData[parseHandle->binaryDataLen++] = '\0';
    parseHandle->binaryData[parseHandle->binaryDataLen++] = '\0';
    if (token != REGLEX_FIRST)
    {
        RegLexUnGetToken(parseHandle->lexHandle);
    }

    parseHandle->dataType = REGLEX_REG_MULTI_SZ;
    parseHandle->lexHandle->isToken = TRUE;
    RegParseExternDataType(parseHandle->dataType,
                           &parseHandle->registryEntry.type);
    dwError = RegParseAssignAttrData(
                  parseHandle,
                  (PVOID) parseHandle->binaryData,
                  parseHandle->binaryDataLen);

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszString);
    return dwError;

error:
    goto cleanup;
}


DWORD
RegParseInstallCallback(
    PREGPARSE_HANDLE parseHandle,
    PFN_REG_CALLBACK parseCallback,
    HANDLE userContext,
    PDWORD indexCallback
    )
{
    DWORD dwError = 0;
    DWORD i = 0;

    BAIL_ON_INVALID_POINTER(parseHandle);
    BAIL_ON_INVALID_POINTER(parseCallback);

    for (i=0;
         i < sizeof(parseHandle->parseCallback.callbacks) /
             sizeof(REGPARSE_CALLBACK_ENTRY);
         i++)
    {
        if (!parseHandle->parseCallback.callbacks[i].used)
        {
            parseHandle->parseCallback.callbacks[i].pfnCallback = parseCallback;
            parseHandle->parseCallback.callbacks[i].userContext = userContext;
            parseHandle->parseCallback.callbacks[i].used = TRUE;
            parseHandle->parseCallback.entries++;
            if (indexCallback)
            {
                *indexCallback = i;
            }
            break;
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegParseRemoveCallback(
    PREGPARSE_HANDLE parseHandle,
    DWORD indexCallback
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(parseHandle);

    if (indexCallback < sizeof(parseHandle->parseCallback.callbacks) /
                        sizeof(REGPARSE_CALLBACK_ENTRY) &&
         parseHandle->parseCallback.callbacks[indexCallback].used)
    {
        parseHandle->parseCallback.callbacks[indexCallback].used = FALSE;
        parseHandle->parseCallback.callbacks[indexCallback].pfnCallback = NULL;
        parseHandle->parseCallback.callbacks[indexCallback].userContext = NULL;
        parseHandle->parseCallback.entries--;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegParseRunCallbacks(
    PREGPARSE_HANDLE parseHandle
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD calledCallback = 0;

    BAIL_ON_INVALID_POINTER(parseHandle);

    if (parseHandle->parseCallback.entries == 0)
    {
        return dwError;
    }

    for (i=0; calledCallback < parseHandle->parseCallback.entries; i++)
    {
        if (parseHandle->parseCallback.callbacks[i].used)
        {
            parseHandle->parseCallback.callbacks[i].pfnCallback(
                &parseHandle->registryEntry,
                parseHandle->parseCallback.callbacks[i].userContext);
            calledCallback++;
        }
    }


cleanup:
    return dwError;

error:

    goto cleanup;
}


DWORD
RegParseTypeStringValue(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD attrSize = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    PSTR pszAttr = NULL;

    RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
    RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);

    if (parseHandle->parseCallback.entries > 0)
    {
        parseHandle->registryEntry.value = pszAttr;
        parseHandle->registryEntry.valueLen = attrSize;
    }
    dwError = RegParseAssignAttrData(
        parseHandle,
        (PVOID) pszAttr,
        attrSize);
    if (parseHandle->lexHandle->eValueNameType == REGLEX_VALUENAME_ATTRIBUTES)
    {
        parseHandle->registryEntry.value = parseHandle->pCurrentAttrValue;
        parseHandle->registryEntry.valueLen =
            parseHandle->dwCurrentAttrValueLen;
    }
    return dwError;
}


void
RegParsePrintASCII(
    UCHAR *buf,
    DWORD buflen)
{
    int i;
    char c;
    printf("PrintASCII: '");
    for (i=0; i<buflen; i++)
    {
        c = (char) buf[i];
        if (isprint((int)c))
        {
            putchar(c);
        }
    }
    printf("'\n");
}


DWORD
RegParseTypeValue(
    PREGPARSE_HANDLE parseHandle
    )
{
    DWORD attrSize = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    BOOLEAN eof = FALSE;
    BOOLEAN bIsAttr = FALSE;
    PSTR pszAttr = NULL;
    PSTR pszTmp = 0;
    CHAR tokenName[256];
    REGLEX_TOKEN token = 0;
    CHAR *pNumLastChar = NULL;

    dwError = RegLexGetToken(parseHandle->ioHandle,
                             parseHandle->lexHandle,
                             &token,
                             &eof);
    if (eof)
    {
        return dwError;
    }
    RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
    RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);
    parseHandle->registryEntry.lineNumber = lineNum;

    parseHandle->dataType = REGLEX_FIRST;
    parseHandle->binaryDataLen = 0;
    switch(token)
    {
        case REGLEX_KEY_NAME_DEFAULT:
            parseHandle->valueType = REGLEX_KEY_NAME_DEFAULT;
            parseHandle->dataType = REGLEX_REG_SZ;
            dwError = RegParseTypeStringValue(parseHandle);
            break;

        case REGLEX_REG_SZ:
            parseHandle->dataType = REGLEX_REG_SZ;
            dwError = RegParseTypeStringValue(parseHandle);
            break;

        case REGLEX_REG_MULTI_SZ:
            parseHandle->dataType = REGLEX_REG_MULTI_SZ;
            dwError = RegParseTypeMultiStringValue(parseHandle);
            if (parseHandle->lexHandle->eValueNameType ==
                REGLEX_VALUENAME_ATTRIBUTES)
            {
                parseHandle->registryEntry.regAttr.RangeType =
                        LWREG_VALUE_RANGE_TYPE_ENUM;
            }
            break;

        case REGLEX_REG_STRING_ARRAY:
            parseHandle->dataType = REGLEX_REG_STRING_ARRAY;
            dwError = RegParseTypeStringArrayValue(parseHandle);
            break;

        case REGLEX_REG_DWORD:
            parseHandle->dataType = REGLEX_REG_DWORD;
            dwError = RegParseTypeDword(parseHandle);
            break;

        case REGLEX_REG_BINARY:
            parseHandle->dataType = REGLEX_REG_BINARY;
            dwError = RegParseTypeBinary(parseHandle);
            break;

        case REGLEX_REG_EXPAND_SZ:
            parseHandle->dataType = REGLEX_REG_EXPAND_SZ;
            dwError = RegParseTypeExpandString(parseHandle);
            break;

        case REGLEX_REG_QUADWORD:
            parseHandle->dataType = REGLEX_REG_QUADWORD;
            dwError = RegParseTypeQuadWord(parseHandle);
            break;

        case REGLEX_REG_RESOURCE_REQUIREMENTS_LIST:
            parseHandle->dataType = REGLEX_REG_RESOURCE_REQUIREMENTS_LIST;
            dwError = RegParseTypeResourceReqList(parseHandle);
            break;

        case REGLEX_REG_RESOURCE_LIST:
            parseHandle->dataType = REGLEX_REG_RESOURCE_LIST;
            dwError = RegParseTypeResourceList(parseHandle);
            break;

        case REGLEX_REG_FULL_RESOURCE_DESCRIPTOR:
            parseHandle->dataType = REGLEX_REG_FULL_RESOURCE_DESCRIPTOR;
            dwError = RegParseTypeFullResDescriptor(parseHandle);
            break;

        case REGLEX_REG_NONE:
            parseHandle->dataType = REGLEX_REG_NONE;
            dwError = RegParseTypeNone(parseHandle);
            break;

        case REGLEX_ATTRIBUTES_BEGIN:
            RegParseFreeRegAttrData(parseHandle);
            parseHandle->dataType = REGLEX_REG_ATTRIBUTES;
            dwError = RegParseAttributes(parseHandle);
            BAIL_ON_REG_ERROR(dwError);
            break;

        case REGLEX_PLAIN_TEXT:
            /*
             * Supported unquoted data values when in a security context:
             * @security = frob
             *
             * Supported unquoted data values when in an attribute context:
             * "range" = boolean
             * "range" = integer:m - n
             * "range" = string:"s1" "s2"
             */
            if (parseHandle->lexHandle->eValueNameType ==
                REGLEX_VALUENAME_SECURITY)
            {
                parseHandle->dataType = REGLEX_REG_SZ;
                dwError = RegParseTypeStringValue(parseHandle);
                BAIL_ON_REG_ERROR(dwError);
                parseHandle->lexHandle->eValueNameType = 0;
            }
            else if ((parseHandle->lexHandle->eValueNameType ==
                     REGLEX_VALUENAME_ATTRIBUTES &&
                     pszAttr &&
                     RegParseIsValidAttribute(parseHandle->attrName,
                                              &bIsAttr) == 0 &&
                     bIsAttr) ||
                     RegFindHintByName(pszAttr))
            {
                if (!strcmp(parseHandle->attrName, "range"))
                {
                    if (!strcmp(pszAttr, "boolean"))
                    {
                        parseHandle->registryEntry.regAttr.RangeType = 
                            LWREG_VALUE_RANGE_TYPE_BOOLEAN;
                    }
                    else if (!strcmp(pszAttr, "string"))
                    {
                        parseHandle->registryEntry.regAttr.RangeType = 
                            LWREG_VALUE_RANGE_TYPE_ENUM;
                    }
                    else
                    {
                        dwError = LWREG_ERROR_PARSE;
                        BAIL_ON_REG_ERROR(dwError);
                    }
                }
                else
                {
                    parseHandle->dataType = REGLEX_REG_SZ;
                    dwError = RegParseTypeStringValue(parseHandle);
                }
            }
            else
            {
                RegLexTokenToString(token, tokenName, sizeof(tokenName));
                dwError = LWREG_ERROR_SYNTAX;
                return dwError;
            }
            break;

        case REGLEX_REG_INTEGER_RANGE:
            if (strcmp(pszAttr, "integer") != 0)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
            }
            else
            {
                parseHandle->registryEntry.regAttr.RangeType =
                    LWREG_VALUE_RANGE_TYPE_INTEGER;
                dwError = RegLexGetToken(parseHandle->ioHandle,
                                         parseHandle->lexHandle,
                                         &token,
                                         &eof);
                if (eof)
                {
                    return dwError;
                }
                if (token != REGLEX_PLAIN_TEXT)
                {
                    dwError = LWREG_ERROR_PARSE;
                    return dwError;
                }

                RegLexGetAttribute(
                    parseHandle->lexHandle,
                    &attrSize,
                    &pszAttr);
                pszTmp = pszAttr;
                parseHandle->registryEntry.regAttr.Range.RangeInteger.Min =
                        strtoul(pszTmp, &pNumLastChar, 0);
                pszTmp = pNumLastChar;
                while (*pszTmp && isspace((int) *pszTmp))
                {
                    pszTmp++;
                }
                if (*pNumLastChar == '\0' || *pNumLastChar != '-')
                {
                    dwError = LWREG_ERROR_PARSE;
                    return dwError;
                }

                /* Skip over dash separator */
                pszTmp = ++pNumLastChar;
                while (*pszTmp && isspace((int) *pszTmp))
                {
                    pszTmp++;
                }
                parseHandle->registryEntry.regAttr.Range.RangeInteger.Max =
                    strtoul(pszTmp, NULL, 0);
            }

            break;

        default:
            if (parseHandle->valueType == REGLEX_KEY_NAME_DEFAULT)
            {
                /* Handle @security name default */
                dwError = RegParseTypeStringValue(parseHandle);
            }
            else
            {
                RegLexTokenToString(token, tokenName, sizeof(tokenName));
                dwError = LWREG_ERROR_SYNTAX;
                return dwError;
                break;
            }
    }

cleanup:
    return dwError;

error: 
    goto cleanup;
}


/*
 * Function that parses "valueName" = datatype:data
 */
DWORD
RegParseKeyValue(
    PREGPARSE_HANDLE parseHandle
    )
{
    DWORD attrSize = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    PSTR pszAttr = NULL;
    REGLEX_TOKEN token = 0;
    BOOLEAN eof = FALSE;


    /* Parse valueName component of data entry */
    dwError = RegLexGetToken(
                  parseHandle->ioHandle,
                  parseHandle->lexHandle,
                  &token,
                  &eof);
    if (eof)
    {
        return dwError;
    }
    RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
    RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);
    parseHandle->registryEntry.lineNumber = lineNum;

    if (parseHandle->valueType == REGLEX_KEY_NAME_DEFAULT &&
        token == REGLEX_PLAIN_TEXT)
    {
        /* Look for something of the format @security = binary_string */
        if (strcmp(pszAttr, "@security") == 0)
        {
            dwError = RegCStringDuplicate(
                          &parseHandle->registryEntry.valueName, "@security");
            BAIL_ON_REG_ERROR(dwError);
        }
        else
        {
            dwError = LWREG_ERROR_INVALID_CONTEXT;
            BAIL_ON_REG_ERROR(dwError);
        }
        dwError = RegLexGetToken(
                      parseHandle->ioHandle,
                      parseHandle->lexHandle,
                      &token,
                      &eof);
        if (eof)
        {
            return dwError;
        }
    }

    /* '=' between valueName and data */
    if (token == REGLEX_EQUALS)
    {
        if ((parseHandle->lexHandle->eValueNameType !=
             REGLEX_VALUENAME_ATTRIBUTES) &&
            pszAttr)
        {
            if (parseHandle->registryEntry.valueName)
            {
                RegMemoryFree(parseHandle->registryEntry.valueName);
                parseHandle->registryEntry.valueName = NULL;
            }
            LWREG_SAFE_FREE_STRING(parseHandle->registryEntry.valueName);
            dwError = RegCStringDuplicate(
                          &parseHandle->registryEntry.valueName,
                          pszAttr);
            BAIL_ON_INVALID_POINTER(parseHandle->registryEntry.valueName);
        }
        else
        {
            /* 
             * This name is the registry attribute field to be populated:
             * value | default | doc | range | hint
             */
            LWREG_SAFE_FREE_STRING(parseHandle->attrName);
            dwError = RegCStringDuplicate(
                          &parseHandle->attrName,
                          pszAttr);
            BAIL_ON_INVALID_POINTER(parseHandle->attrName);
        }
    }
    else
    {
        dwError = LWREG_ERROR_SYNTAX;
        return dwError;
    }

    /* Parse data value */
    dwError = RegParseTypeValue(parseHandle);
    BAIL_ON_REG_ERROR(dwError);
    if (dwError == 0)
    {
        RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);
        RegParseExternDataType(parseHandle->dataType,
                               &parseHandle->registryEntry.type);
        RegParseExternDataType(parseHandle->valueType,
                               &parseHandle->registryEntry.valueType);
        if (parseHandle->dataType != REGLEX_REG_SZ &&
            parseHandle->lexHandle->eValueNameType != REGLEX_VALUENAME_ATTRIBUTES)
        {
            parseHandle->registryEntry.valueLen = parseHandle->binaryDataLen;
            parseHandle->registryEntry.value = parseHandle->binaryData;
        }
    }

    if (parseHandle->lexHandle->eValueNameType !=
            REGLEX_VALUENAME_ATTRIBUTES &&
            parseHandle->lexHandle->eValueNameType !=
            REGLEX_VALUENAME_ATTRIBUTES_RESET)
    {
        RegParseRunCallbacks(parseHandle); 
    }
    if (parseHandle->lexHandle->eValueNameType ==
            REGLEX_VALUENAME_ATTRIBUTES_RESET)
    {
        parseHandle->lexHandle->eValueNameType = 0;
        RegParseFreeRegAttrData(parseHandle);
    }
    RegLexResetToken(parseHandle->lexHandle);

cleanup:
    return dwError;

error:

    goto cleanup;
}


DWORD
RegParseCheckAttributes(
    PREGPARSE_HANDLE parseHandle
    )
{
    DWORD dwError = 0;
    DWORD dwValue = 0;
    DWORD dwMin = 0;
    DWORD dwMax = 0;
    REG_DATA_TYPE regDataType = 0;
    
           
    BAIL_ON_INVALID_HANDLE(parseHandle);
    parseHandle->registryEntry.status = 0;
    /*
     * Perform semantic consistency check of the data present in the
     * LWREG_VALUE_ATTRIBUTES structure. Only individual valueName=dataValue
     * entries are checked for syntactic correctness. The combination of
     * attributes is not checked for semantic correctness. This function does
     * this, and either fixes up the data, or throws a syntax error
     * because an invalid combination of data was presented.
     * Such an example would be:
     *  "someValue" = {
     *     default = "theValue"
     *     range = 1-1024
     *     hint = seconds
     *  }
     *
     * Applying a numeric range on a string value is inconistent. 
     *
     * Another example:
     *  "someValue" = {
     *     default = dword:0000ffff
     *     range = 1-1024
     *     hint = seconds
     *  }
     *
     * Here the range hint matches the data type, but the default value is
     * out of range, so this is also inconsistent.
     */

    /*
     * Handle degenerate case where there is only a value set, and no
     * attributes set. If so, convert to value=data type.
     */
    if (!parseHandle->registryEntry.regAttr.pDefaultValue &&
        !parseHandle->registryEntry.regAttr.RangeType &&
        !parseHandle->registryEntry.regAttr.Hint)
    {
        parseHandle->registryEntry.type =
            parseHandle->registryEntry.regAttr.ValueType;
        parseHandle->registryEntry.regAttr.ValueType = 0;
        dwError = 0;
        goto cleanup;
    }

    /* value/default data types must be the same */
    RegParseAttributesExternDataType(
        parseHandle,
        &regDataType);
    if (parseHandle->registryEntry.regAttr.pDefaultValue &&
        regDataType != parseHandle->registryEntry.regAttr.ValueType)
    {
        parseHandle->registryEntry.status = LWREG_ERROR_INVALID_CONTEXT;
    }
    
    /*
     * Perform various range/type consistency checks. But
     * only perform this check if there is a value assigned.
     */
    if  (parseHandle->registryEntry.value ||
         parseHandle->registryEntry.regAttr.pDefaultValue)
    {
        if (parseHandle->registryEntry.regAttr.RangeType ==
            LWREG_VALUE_RANGE_TYPE_INTEGER)
        {
            if (parseHandle->registryEntry.regAttr.ValueType != REG_DWORD)
            {
                parseHandle->registryEntry.status = LWREG_ERROR_INVALID_CONTEXT;
            }
    
            if (parseHandle->registryEntry.value)
            {
                dwValue = *(PDWORD) parseHandle->registryEntry.value;
            }
            else
            {
                dwValue = *(PDWORD)
                    parseHandle->registryEntry.regAttr.pDefaultValue;
            }
    
            /* Perform range check on integer value */
            dwMin = parseHandle->registryEntry.regAttr.Range.RangeInteger.Min;
            dwMax = parseHandle->registryEntry.regAttr.Range.RangeInteger.Max;
            if (dwValue < dwMin || dwValue > dwMax)
            {
                parseHandle->registryEntry.status = LWREG_ERROR_INVALID_CONTEXT;
            }
        }
        if (parseHandle->registryEntry.regAttr.RangeType ==
            LWREG_VALUE_RANGE_TYPE_BOOLEAN)
        {
            if (parseHandle->registryEntry.value)
            {
                dwValue = *(PDWORD) parseHandle->registryEntry.value;
            }
            else
            {
                dwValue = *(PDWORD)
                    parseHandle->registryEntry.regAttr.pDefaultValue;
            }
            if (dwValue != 0 && dwValue != 1)
            {
                parseHandle->registryEntry.status = LWREG_ERROR_INVALID_CONTEXT;
            }
        }
    }
    if (parseHandle->registryEntry.regAttr.RangeType ==
             LWREG_VALUE_RANGE_TYPE_ENUM)
    {
        if (!parseHandle->registryEntry.regAttr.Range.ppwszRangeEnumStrings)
        {
            parseHandle->registryEntry.status = LWREG_ERROR_INVALID_CONTEXT;
        }
        if ((parseHandle->registryEntry.regAttr.ValueType &&
             parseHandle->registryEntry.regAttr.ValueType != REG_SZ))
        {
            parseHandle->registryEntry.status = LWREG_ERROR_INVALID_CONTEXT;
        }
    }

    switch (parseHandle->registryEntry.regAttr.Hint)
    {
      case LWREG_VALUE_HINT_SECONDS:
        if (parseHandle->registryEntry.regAttr.ValueType != REG_DWORD)
        {
            parseHandle->registryEntry.status = LWREG_ERROR_INVALID_CONTEXT;
        }
        break;

      case LWREG_VALUE_HINT_PATH:
      case LWREG_VALUE_HINT_ACCOUNT:
        if (parseHandle->registryEntry.regAttr.ValueType != REG_SZ)
        {
            parseHandle->registryEntry.status = LWREG_ERROR_INVALID_CONTEXT;
        }
        break;

      default:
          goto cleanup;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegParseKey(
    PREGPARSE_HANDLE parseHandle,
    REGLEX_TOKEN token
    )
{
    DWORD attrSize = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    BOOLEAN eof = FALSE;
    PSTR pszAttr = 0;

    for (;;)
    {
        RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
        RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);

        /*
         * Test for empty registry subkey case first
         */
        if (token == REGLEX_REG_KEY)
        {
            /* Empty subkey, with no value, free any previous value */
            if (parseHandle->registryEntry.valueName)
            {
                RegMemoryFree(parseHandle->registryEntry.valueName);
                parseHandle->registryEntry.valueName = NULL;
            }
            parseHandle->dataType = REGLEX_REG_KEY;
            parseHandle->valueType = REGLEX_REG_NONE;
            if (pszAttr)
            {
                if (parseHandle->registryEntry.keyName)
                {
                    RegMemoryFree(parseHandle->registryEntry.keyName);
                }
                dwError = RegCStringDuplicate(
                		&parseHandle->registryEntry.keyName, pszAttr);
                BAIL_ON_INVALID_POINTER(parseHandle->registryEntry.keyName);
            }
            if (parseHandle->parseCallback.entries > 0)
            {
                parseHandle->registryEntry.lineNumber = lineNum;
                RegParseExternDataType(parseHandle->dataType,
                                       &parseHandle->registryEntry.type);
                RegParseExternDataType(parseHandle->valueType,
                                       &parseHandle->registryEntry.valueType);
                parseHandle->registryEntry.valueLen = 0;
                parseHandle->registryEntry.value = NULL;
                if (parseHandle->lexHandle->eValueNameType !=
                    REGLEX_VALUENAME_ATTRIBUTES)
                {
                    RegParseRunCallbacks(parseHandle);
                }
            }
            return dwError;
        }
        else if (token == REGLEX_REG_SZ ||
                 token == REGLEX_KEY_NAME_DEFAULT ||
                 (token == REGLEX_PLAIN_TEXT &&
                  parseHandle->lexHandle->eValueNameType ==
                          REGLEX_VALUENAME_ATTRIBUTES
                 )
                )
        {
            /* 
             * Value names can be "valueName", @, or valueName when in the
             * registry attribute context.
             */
            parseHandle->valueType = token;
            dwError = RegParseKeyValue(parseHandle);
            BAIL_ON_REG_ERROR(dwError);
   
        }
        else if (token == REGLEX_PLAIN_TEXT)
        {
            parseHandle->dataType = REGLEX_PLAIN_TEXT;
            parseHandle->valueType = REGLEX_REG_SZ;
            if (parseHandle->parseCallback.entries > 0)
            {
                parseHandle->registryEntry.lineNumber = lineNum;
                RegParseExternDataType(parseHandle->dataType,
                                       &parseHandle->registryEntry.type);
                RegParseExternDataType(parseHandle->valueType,
                                       &parseHandle->registryEntry.valueType);
                parseHandle->registryEntry.value = pszAttr;
                parseHandle->registryEntry.valueLen = attrSize;
                if (parseHandle->lexHandle->eValueNameType !=
                    REGLEX_VALUENAME_ATTRIBUTES) 
                {
                    RegParseRunCallbacks(parseHandle);
                }
            }
        }
        else if (token == REGLEX_ATTRIBUTES_END)
        {
            RegLexUnGetToken(parseHandle->lexHandle);
            parseHandle->registryEntry.type = REG_ATTRIBUTES;

            /* Perform semantic checks on attributes */
            dwError = RegParseCheckAttributes(parseHandle);
            BAIL_ON_REG_ERROR(dwError);

            RegParseRunCallbacks(parseHandle); 
            parseHandle->lexHandle->eValueNameType =
                REGLEX_VALUENAME_ATTRIBUTES_RESET;
            return 0;
        }
        else
        {
            /*
             * Maybe treat as a syntax error here. May be in infinite loop
             * for poorly formed entries.
             */
            parseHandle->valueType = token;
            RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
            dwError = RegParseKeyValue(parseHandle);
            dwError = LWREG_ERROR_PARSE;
            return dwError;
        }
        dwError = RegLexGetToken(parseHandle->ioHandle,
                                 parseHandle->lexHandle,
                                 &token,
                                 &eof);
        BAIL_ON_REG_ERROR(dwError);
        if (eof)
        {
            return dwError;
        }
    }

cleanup:
    return dwError;

error:

    goto cleanup;
}


DWORD
RegParseAttributes(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD dwError = 0;
    BOOLEAN eof = FALSE;
    REGLEX_TOKEN token = 0;
    CHAR tokenName[256];
    PSTR pszAttr = NULL;
    DWORD attrSize = 0;

    RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
    if (!pszAttr  || !pszAttr[0])
    {
        dwError = LWREG_ERROR_INVALID_CONTEXT;
        return dwError;
    }
    do {
        dwError = RegLexGetToken(parseHandle->ioHandle,
                                 parseHandle->lexHandle,
                                 &token,
                                 &eof);
        if (eof)
        {
            return dwError;
        }
        RegLexTokenToString(token, tokenName, sizeof(tokenName));
        RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
        if (token != REGLEX_ATTRIBUTES_END)
        {
            /* Scary recursive call to RegParseKey() here */
            dwError = RegParseKey(parseHandle, token);
            if (dwError)
            {
                break;
            }
            dwError = RegLexGetToken(parseHandle->ioHandle,
                                     parseHandle->lexHandle,
                                     &token,
                                     &eof);
            if (eof)
            {
                if (parseHandle->lexHandle->eValueNameType ==
                    REGLEX_VALUENAME_ATTRIBUTES)
                {
                    /* Syntax error, imbalanced { } and EOF found */
                    dwError = LWREG_ERROR_SYNTAX;
                }
                return dwError;
            }
        }
    }
    while (token != REGLEX_ATTRIBUTES_END);
    return dwError;
}

DWORD
RegParseOpen(
    PSTR pszRegFileName,
    PFN_REG_CALLBACK parseCallback,
    HANDLE userContext,
    HANDLE *ppNewHandle)
{
    HANDLE ioHandle = NULL;
    PREGLEX_ITEM lexHandle = NULL;
    PREGPARSE_HANDLE newHandle = NULL;
    DWORD dwError = 0;
    void *binaryData = NULL;

    dwError = RegIOOpen(pszRegFileName, &ioHandle);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegLexOpen(&lexHandle);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegAllocateMemory(sizeof(REGPARSE_HANDLE),
                                (LW_PVOID) &newHandle);
    BAIL_ON_INVALID_POINTER(newHandle);

    dwError = RegAllocateMemory(REGPARSE_BUFSIZ,
                                &binaryData);
    BAIL_ON_INVALID_POINTER(binaryData);

    newHandle->ioHandle = ioHandle;
    newHandle->lexHandle = lexHandle;
    if (parseCallback)
    {
        RegParseInstallCallback(newHandle,
                                parseCallback,
                                userContext,
                                NULL);
    }
    newHandle->binaryData = binaryData;
    newHandle->binaryDataAllocLen = REGPARSE_BUFSIZ;
    newHandle->binaryDataLen = 0;

    *ppNewHandle = (HANDLE) newHandle;

cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(newHandle);
    LWREG_SAFE_FREE_MEMORY(binaryData);
    RegLexClose(lexHandle);
    RegIOClose(ioHandle);
    goto cleanup;
}


void 
RegParseFreeRegAttrData(
    HANDLE pHandle)
{
    PREGPARSE_HANDLE pParseHandle = (PREGPARSE_HANDLE) pHandle;

    /* Cleanup memory related to registry attributes */
    LWREG_SAFE_FREE_MEMORY(
        pParseHandle->pCurrentAttrValue);
    LWREG_SAFE_FREE_MEMORY(
        pParseHandle->registryEntry.regAttr.pDefaultValue);
    LWREG_SAFE_FREE_MEMORY(pParseHandle->registryEntry.regAttr.pwszDocString);
    pParseHandle->registryEntry.value = NULL;
    pParseHandle->registryEntry.valueType = 0;
    if (pParseHandle->registryEntry.regAttr.RangeType ==
            LWREG_VALUE_RANGE_TYPE_ENUM &&
        pParseHandle->registryEntry.regAttr.Range.ppwszRangeEnumStrings)
    {
        RegFreeMultiStrsW(
            pParseHandle->registryEntry.regAttr.Range.ppwszRangeEnumStrings);
        pParseHandle->registryEntry.regAttr.Range.ppwszRangeEnumStrings = NULL;
    }
    memset(&pParseHandle->registryEntry.regAttr, 
           0,
           sizeof(pParseHandle->registryEntry.regAttr));
}


DWORD
RegParseGetLineNumber(
    HANDLE pHandle,
    PDWORD pdwLineNum)
{
    PREGPARSE_HANDLE pParseHandle = (PREGPARSE_HANDLE) pHandle;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pHandle);
    BAIL_ON_INVALID_POINTER(pdwLineNum);

    *pdwLineNum = pParseHandle->registryEntry.lineNumber;
cleanup:
    return dwError;
error:
    goto cleanup;
}
 

void
RegParseClose(
    HANDLE pHandle)
{
    PREGPARSE_HANDLE pParseHandle = (PREGPARSE_HANDLE) pHandle;

    if (pParseHandle)
    {

        LWREG_SAFE_FREE_STRING(pParseHandle->registryEntry.keyName);
        LWREG_SAFE_FREE_STRING(pParseHandle->registryEntry.valueName);
        LWREG_SAFE_FREE_MEMORY(pParseHandle->binaryData);
        LWREG_SAFE_FREE_STRING(pParseHandle->attrName);

        RegParseFreeRegAttrData(pParseHandle);
        RegLexClose(pParseHandle->lexHandle);
        RegIOClose(pParseHandle->ioHandle);
        RegMemoryFree(pParseHandle);
    }
}


DWORD
RegParseRegistry(
    HANDLE pHandle)
{
    DWORD dwError = 0;
    REGLEX_TOKEN token = 0;
    BOOLEAN eof = 0;
    PREGPARSE_HANDLE parseHandle = (PREGPARSE_HANDLE) pHandle;

    do {
        dwError = RegLexGetToken(parseHandle->ioHandle,
                                 parseHandle->lexHandle,
                                 &token,
                                 &eof);
        
        BAIL_ON_REG_ERROR(dwError);
        if (!eof)
        {
            dwError = RegParseKey(parseHandle, token);
            if (dwError)
            {
                break;
            }
        }
    } while (!eof);

cleanup:
    return dwError;

error:
    goto cleanup;
}
