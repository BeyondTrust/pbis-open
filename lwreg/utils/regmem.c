/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        regmem.c
 *
 * Abstract:
 *
 *        Registry Memory Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */
#include "includes.h"

static
NTSTATUS
RegHexCharToByte(
    CHAR cHexChar,
    UCHAR* pucByte
    );

static
void
RegStripLeadingWhitespace(
    PSTR pszString
    );

static
void
RegStripTrailingWhitespace(
    PSTR pszString
    );

NTSTATUS
RegInitializeStringBuffer(
    REG_STRING_BUFFER *pBuffer,
    size_t sCapacity
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszBuffer = NULL;

    pBuffer->sLen = 0;
    pBuffer->sCapacity = 0;

    if (sCapacity > DWORD_MAX - 1)
    {
    	status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    status = LW_RTL_ALLOCATE((PVOID*)&pszBuffer, CHAR, sCapacity + 1);
    BAIL_ON_NT_STATUS(status);

    pBuffer->pszBuffer = pszBuffer;
    pBuffer->sCapacity = sCapacity;

cleanup:
    return status;

error:
    pBuffer->pszBuffer = NULL;

    goto cleanup;
}

NTSTATUS
RegAppendStringBuffer(
    REG_STRING_BUFFER *pBuffer,
    PCSTR pszAppend
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t sAppendLen = 0;
    size_t sNewCapacity = 0;

    if (pszAppend != NULL)
        sAppendLen = strlen(pszAppend);

    if (sAppendLen + pBuffer->sLen > pBuffer->sCapacity ||
            pBuffer->pszBuffer == NULL)
    {
        sNewCapacity = (pBuffer->sCapacity + sAppendLen) * 2;

        if (sNewCapacity > DWORD_MAX - 1)
        {
        	status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
        }

        if (sNewCapacity < pBuffer->sCapacity)
        {
        	status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
        }

        status = NtRegReallocMemory(
            pBuffer->pszBuffer,
            (PVOID *)&pBuffer->pszBuffer,
            sNewCapacity + 1);
        BAIL_ON_NT_STATUS(status);

        pBuffer->sCapacity = sNewCapacity;
    }

    if (pszAppend != NULL)
    {
        memcpy(
            pBuffer->pszBuffer + pBuffer->sLen,
            pszAppend,
            sAppendLen);
        pBuffer->sLen += sAppendLen;
    }
    pBuffer->pszBuffer[pBuffer->sLen] = '\0';

cleanup:
    return status;

error:
    goto cleanup;
}

void
RegFreeStringBufferContents(
    REG_STRING_BUFFER *pBuffer)
{
    LWREG_SAFE_FREE_MEMORY(pBuffer->pszBuffer);

    pBuffer->sLen = 0;
    pBuffer->sCapacity = 0;
}

VOID
RegFreeMemory(
    PVOID pMemory
    )
{
	LwRtlMemoryFree(pMemory);
}

void
RegSafeFreeValueAttributes(
    PLWREG_VALUE_ATTRIBUTES* ppValueAttrs
    )
{
    PLWREG_VALUE_ATTRIBUTES pValueAttrs = NULL;

    if ((ppValueAttrs == NULL) || (*ppValueAttrs == NULL)) {
        return;
    }

    pValueAttrs = *ppValueAttrs;
    RTL_FREE(&pValueAttrs->pDefaultValue);
    RTL_FREE(&pValueAttrs->pwszDocString);

    if (LWREG_VALUE_RANGE_TYPE_ENUM == pValueAttrs->RangeType)
    {
        if (pValueAttrs->Range.ppwszRangeEnumStrings)
        {
            RegFreeWC16StringArrayWithNullTerminator(pValueAttrs->Range.ppwszRangeEnumStrings);
        }
        pValueAttrs->Range.ppwszRangeEnumStrings = NULL;
    }

    RTL_FREE(&pValueAttrs);

    *ppValueAttrs = NULL;

    return;
}

void
RegSafeFreeCurrentValueInfo(
    PLWREG_CURRENT_VALUEINFO* ppValueInfo
    )
{
    PLWREG_CURRENT_VALUEINFO pValueInfo = NULL;

    if ((ppValueInfo == NULL) || (*ppValueInfo == NULL)) {
        return;
    }

    pValueInfo = *ppValueInfo;
    RTL_FREE(&pValueInfo->pvData);
    RTL_FREE(&pValueInfo);

    *ppValueInfo = NULL;

    return;
}

DWORD
RegReallocMemory(
    IN PVOID pMemory,
    OUT PVOID* ppNewMemory,
    IN DWORD dwSize
    )
{
    return RegNtStatusToWin32Error(
    		NtRegReallocMemory(pMemory,
    				         ppNewMemory,
    				         dwSize)
    				         );

}

NTSTATUS
NtRegReallocMemory(
    IN PVOID pMemory,
    OUT PVOID* ppNewMemory,
    IN DWORD dwSize
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PVOID pNewMemory = NULL;

    if (pMemory == NULL) {
       pNewMemory = malloc(dwSize);
       memset(pNewMemory, 0, dwSize);
    }else {
       pNewMemory = realloc(pMemory, dwSize);
    }
    if (!pNewMemory){
    	status = STATUS_INSUFFICIENT_RESOURCES;
       *ppNewMemory = NULL;
    }else {
       *ppNewMemory = pNewMemory;
    }

    return status;
}

void
RegFreeString(
    PSTR pszString
    )
{
	LwRtlMemoryFree(pszString);
}

void
RegFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    )
{
    DWORD i;

    if ( ppStringArray )
    {
        for(i = 0; i < dwCount; i++)
        {
            if (ppStringArray[i])
            {
            	LwRtlMemoryFree(ppStringArray[i]);
            }
        }

        LwRtlMemoryFree(ppStringArray);
    }

    return;
}

void
RegFreeWC16StringArrayWithNullTerminator(
    PWSTR * ppwStringArray
    )
{
    DWORD i = 0;

    if ( ppwStringArray )
    {
        while (ppwStringArray[i])
        {
            LwRtlMemoryFree(ppwStringArray[i++]);
        }

        LwRtlMemoryFree(ppwStringArray);
    }

    return;
}

void
RegFreeWC16StringArray(
    PWSTR * ppwStringArray,
    DWORD dwCount
    )
{
    DWORD i;

    if ( ppwStringArray )
    {
        for(i = 0; i < dwCount; i++)
        {
            if (ppwStringArray[i])
            {
                LwRtlMemoryFree(ppwStringArray[i]);
            }
        }

        LwRtlMemoryFree(ppwStringArray);
    }

    return;
}

void
RegFreeValueByteArray(
    PBYTE* ppValues,
    DWORD dwCount
    )
{
    DWORD i;

    if ( ppValues )
    {
        for(i = 0; i < dwCount; i++)
        {
            if (ppValues[i])
            {
            	LwRtlMemoryFree(ppValues[i]);
            }
        }

        LwRtlMemoryFree(ppValues);
    }

    return;
}

DWORD
RegStrndup(
    PCSTR pszInputString,
    size_t size,
    PSTR * ppszOutputString
    )
{
    DWORD dwError = 0;
    size_t copylen = 0;
    PSTR pszOutputString = NULL;

    if (!pszInputString || !ppszOutputString){
        dwError = EINVAL;
        BAIL_ON_REG_ERROR(dwError);
    }

    for (copylen = 0; copylen < size && pszInputString[copylen]; copylen++);

    dwError = RegAllocateMemory(copylen+1, (PVOID*)&pszOutputString);
    BAIL_ON_REG_ERROR(dwError);

    memcpy(pszOutputString, pszInputString, copylen);
    pszOutputString[copylen] = 0;

    *ppszOutputString = pszOutputString;

cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_STRING(pszOutputString);
    goto cleanup;
}

NTSTATUS
RegHexStrToByteArray(
    IN PCSTR pszHexString,
    IN OPTIONAL DWORD* pdwHexStringLength,
    OUT UCHAR** ppucByteArray,
    OUT DWORD*  pdwByteArrayLength
    )
{
    NTSTATUS status = 0;
    DWORD i = 0;
    DWORD dwHexChars = 0;
    UCHAR* pucByteArray = NULL;
    DWORD dwByteArrayLength = 0;

    BAIL_ON_NT_INVALID_POINTER(pszHexString);

    if (pdwHexStringLength)
    {
        dwHexChars = *pdwHexStringLength;
    }
    else
    {
        dwHexChars = strlen(pszHexString);
    }
    dwByteArrayLength = dwHexChars / 2;

    if ((dwHexChars & 0x00000001) != 0)
    {
       status = STATUS_INVALID_PARAMETER;
       BAIL_ON_NT_STATUS(status);
    }

    status = LW_RTL_ALLOCATE((PVOID*)&pucByteArray, UCHAR,
            		          sizeof(*pucByteArray)* dwByteArrayLength);
    BAIL_ON_NT_STATUS(status);

    for (i = 0; i < dwByteArrayLength; i++)
    {
        CHAR hexHi = pszHexString[2*i];
        CHAR hexLow = pszHexString[2*i + 1];

        UCHAR ucHi = 0;
        UCHAR ucLow = 0;

        status = RegHexCharToByte(hexHi, &ucHi);
        BAIL_ON_NT_STATUS(status);

        status = RegHexCharToByte(hexLow, &ucLow);
        BAIL_ON_NT_STATUS(status);

        pucByteArray[i] = (ucHi * 16) + ucLow;
    }

    *ppucByteArray = pucByteArray;
    *pdwByteArrayLength = dwByteArrayLength;

cleanup:

    return status;

error:

    LWREG_SAFE_FREE_MEMORY(pucByteArray);
    *ppucByteArray = NULL;
    *pdwByteArrayLength = 0;

    goto cleanup;
}

NTSTATUS
RegByteArrayToHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    )
{
    NTSTATUS status = 0;
    DWORD i = 0;
    PSTR pszHexString = NULL;

    status = LW_RTL_ALLOCATE((PVOID*)&pszHexString, CHAR,
            		          sizeof(*pszHexString)* (dwByteArrayLength*2 + 1));
    BAIL_ON_NT_STATUS(status);

    for (i = 0; i < dwByteArrayLength; i++)
    {
        sprintf(pszHexString+(2*i), "%.2X", pucByteArray[i]);
    }

    *ppszHexString = pszHexString;

cleanup:

    return status;

error:
    LWREG_SAFE_FREE_STRING(pszHexString);

    *ppszHexString = NULL;
    goto cleanup;
}

NTSTATUS
RegStrDupOrNull(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    )
{
    if (pszInputString == NULL)
    {
        *ppszOutputString = NULL;
        return STATUS_SUCCESS;
    }
    else
    {
        return LwRtlCStringDuplicate(ppszOutputString, pszInputString);
    }
}

NTSTATUS
RegWcStrDupOrNull(
    PCWSTR pwszInputString,
    PWSTR *ppwszOutputString
    )
{
    if (pwszInputString == NULL)
    {
        *ppwszOutputString = NULL;
        return STATUS_SUCCESS;
    }
    else
    {
        return LwRtlWC16StringDuplicate(ppwszOutputString, pwszInputString);
    }
}

void
RegStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    )
{
    if (LW_IS_NULL_OR_EMPTY_STR(pszString))
        return;

    if (bLeading) {
        RegStripLeadingWhitespace(pszString);
    }

    if (bTrailing) {
        RegStripTrailingWhitespace(pszString);
    }
}

DWORD
RegAllocateMemory(
    size_t Size,
    LW_PVOID * ppMemory
    )
{
    DWORD dwError = 0;

	if (Size)
    {
		dwError = RegNtStatusToWin32Error(
		    		RTL_ALLOCATE(ppMemory, VOID, Size)
		    		);
    }
	else
	{
		dwError = RegNtStatusToWin32Error(LW_STATUS_INSUFFICIENT_RESOURCES);
	}

	return dwError;
}

DWORD
RegCStringDuplicate(
    OUT PSTR* ppszNewString,
    IN PCSTR pszOriginalString
    )
{
	return RegNtStatusToWin32Error(
			LwRtlCStringDuplicate(ppszNewString, pszOriginalString)
			);
}

void
RegMemoryFree(
	IN OUT LW_PVOID pMemory
	)
{
	return LwRtlMemoryFree(pMemory);
}

DWORD
RegWC16StringDuplicate(
    PWSTR* ppwszNewString,
    PCWSTR pwszOriginalString
    )
{
	return RegNtStatusToWin32Error(
	        LwRtlWC16StringDuplicate(ppwszNewString, pwszOriginalString)
	        );
}

DWORD
RegWC16StringAllocateFromCString(
    OUT PWSTR* ppszNewString,
    IN PCSTR pszOriginalString
    )
{
	return RegNtStatusToWin32Error(
			LwRtlWC16StringAllocateFromCString(ppszNewString, pszOriginalString)
			);
}

DWORD
RegCStringAllocateFromWC16String(
    OUT PSTR* ppszNewString,
    IN PCWSTR pszOriginalString
    )
{
	return RegNtStatusToWin32Error(
			LwRtlCStringAllocateFromWC16String(ppszNewString, pszOriginalString)
			    );
}

DWORD
RegWC16StringAllocatePrintfW(
    LW_OUT LW_PWSTR* ppszString,
    LW_IN const wchar_t* pszFormat,
    LW_IN ...
    )
{
    DWORD dwError = 0;
    va_list args;

    va_start(args, pszFormat);
    dwError = RegNtStatusToWin32Error(LwRtlWC16StringAllocatePrintfWV(ppszString, pszFormat, args));
    va_end(args);

    return dwError;
}

DWORD
RegCStringAllocatePrintf(
    OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    IN ...
    )
{
    DWORD dwError = 0;
    va_list args;

    va_start(args, pszFormat);
    dwError = RegNtStatusToWin32Error(LwRtlCStringAllocatePrintfV(ppszString, pszFormat, args));
    va_end(args);

    return dwError;
}

PCWSTR
RegStrrchr(
    PCWSTR pwszStr,
    wchar16_t wch
    )
{
	int iIndex = RtlWC16StringNumChars(pwszStr);

	for (; iIndex >= 0; iIndex--)
	{
		if (pwszStr[iIndex] == wch)
		{
			return &pwszStr[iIndex];
		}
	}

	return NULL;
}

PCWSTR
RegStrchr(
    PCWSTR pwszStr,
    wchar16_t wch
    )
{
	int iIndex = 0;

	for (; pwszStr[iIndex] != (wchar16_t)'\0'; iIndex++)
	{
		if (pwszStr[iIndex] == wch)
		{
			return &pwszStr[iIndex];
		}
	}

	return NULL;
}


DWORD
RegWC16StringArraysAllocateFromCStringArraysWithNullTerminator(
    IN PSTR* ppszStrings,
    OUT PWSTR** pppwszStrings
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszStrings = NULL;
    int i = 0;

    if (!ppszStrings || !ppszStrings[0])
    {
        goto cleanup;
    }

    while (ppszStrings[i++]);

    dwError = RegAllocateMemory(sizeof(*ppwszStrings)*i, (PVOID*)&ppwszStrings);
    BAIL_ON_REG_ERROR(dwError);

    i = 0;

    while (ppszStrings[i])
    {
        dwError = RegWC16StringAllocateFromCString(
                               &ppwszStrings[i],
                               ppszStrings[i]);
        BAIL_ON_REG_ERROR(dwError);

        i++;
    }

    *pppwszStrings = ppwszStrings;

cleanup:
    return dwError;

error:
    if (ppwszStrings)
    {
        RegFreeWC16StringArrayWithNullTerminator(ppwszStrings);
    }
    *pppwszStrings = NULL;

    goto cleanup;
}

DWORD
RegCopyValueAToW(
    IN REG_DATA_TYPE dwType,
    IN PVOID pData,
    IN DWORD cbData,
    OUT PVOID* ppOutData,
    OUT PDWORD pcbOutDataLen
    )
{
    DWORD dwError = 0;
    PVOID pOutData = NULL;
    DWORD cbOutDataLen = 0;
    BOOLEAN bIsStrType = FALSE;

    if (dwType == REG_MULTI_SZ)
    {
        if (!pData)
        {
            pData = (PBYTE) "";
        }
        if (cbData == 0)
        {
            cbData = 1;
        }
    }

    if (pData)
    {
        if (REG_MULTI_SZ == dwType)
        {
            dwError = RegConvertByteStreamA2W((PBYTE)pData,
                                              cbData,
                                              (PBYTE*)&pOutData,
                                              &cbOutDataLen);
            BAIL_ON_REG_ERROR(dwError);

            bIsStrType = TRUE;
        }
        else if (REG_SZ == dwType)
        {
            /* Verify correct null termination of input data */
            if (strlen((char *) pData) != (cbData-1))
            {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_REG_ERROR(dwError);
            }

            dwError = RegWC16StringAllocateFromCString((PWSTR*)&pOutData, pData);
            BAIL_ON_REG_ERROR(dwError);

            cbOutDataLen = (mbstrlen((const char*) pData)+1) * sizeof(WCHAR);
            bIsStrType = TRUE;
        }
    }

    if (!bIsStrType && cbData)
    {
        dwError = RegAllocateMemory(cbData, &pOutData);
        BAIL_ON_REG_ERROR(dwError);

        if (pData)
        {
            memcpy(pOutData, pData, cbData);
            cbOutDataLen = cbData;
        }
    }

    *ppOutData = pOutData;
    *pcbOutDataLen = cbOutDataLen;

cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(pOutData);

    *ppOutData = NULL;
    *pcbOutDataLen = 0;

    goto cleanup;
}

DWORD
RegConvertValueAttributesAToW(
    LWREG_VALUE_ATTRIBUTES_A attrA,
    PLWREG_VALUE_ATTRIBUTES* ppAttrW
    )
{
    DWORD dwError = 0;
    PLWREG_VALUE_ATTRIBUTES pAttr = NULL;

    dwError = RegAllocateMemory(sizeof(*pAttr),(PVOID*)&pAttr);
    BAIL_ON_REG_ERROR(dwError);

    pAttr->DefaultValueLen = attrA.DefaultValueLen;
    pAttr->Hint = attrA.Hint;
    pAttr->RangeType = attrA.RangeType;

    switch (pAttr->RangeType)
    {
        case LWREG_VALUE_RANGE_TYPE_ENUM:

            dwError = RegWC16StringArraysAllocateFromCStringArraysWithNullTerminator(
                            attrA.Range.ppszRangeEnumStrings,
                            &pAttr->Range.ppwszRangeEnumStrings);
            BAIL_ON_REG_ERROR(dwError);

            break;

        case LWREG_VALUE_RANGE_TYPE_INTEGER:
             pAttr->Range.RangeInteger.Min = attrA.Range.RangeInteger.Min;
             pAttr->Range.RangeInteger.Max = attrA.Range.RangeInteger.Max;

             break;

        case LWREG_VALUE_RANGE_TYPE_BOOLEAN:
             pAttr->Range.RangeInteger.Min = 0;
             pAttr->Range.RangeInteger.Max = 1;

             break;

        default:
             dwError = ERROR_INVALID_PARAMETER;
             BAIL_ON_REG_ERROR(dwError);
      }

    if (attrA.pszDocString)
    {
        dwError = RegWC16StringAllocateFromCString(
                             &pAttr->pwszDocString,
                             attrA.pszDocString
                               );
        BAIL_ON_REG_ERROR(dwError);
    }

    pAttr->ValueType = attrA.ValueType;

    dwError = RegCopyValueAToW(pAttr->ValueType,
                               attrA.pDefaultValue,
                               attrA.DefaultValueLen,
                               &pAttr->pDefaultValue,
                               &pAttr->DefaultValueLen);
    BAIL_ON_REG_ERROR(dwError);

    *ppAttrW = pAttr;

cleanup:
    return dwError;

error:

    RegSafeFreeValueAttributes(&pAttr);

    goto cleanup;
}

#if 0
DWORD
RegDuplicateValueAttributes(
    LWREG_VALUE_ATTRIBUTES pAttr,
    PLWREG_VALUE_ATTRIBUTES* ppAttr
    )
{
    DWORD dwError = 0;
    PLWREG_VALUE_ATTRIBUTES pAttr = NULL;

    dwError = RegAllocateMemory(sizeof(*pAttr),(PVOID*)&pAttr);
    BAIL_ON_REG_ERROR(dwError);

    pAttr->DefaultValueLen = pAttr.DefaultValueLen;
    pAttr->Hint = attrA.Hint;
    pAttr->RangeType = attrA.RangeType;

    switch (pAttr->RangeType)
    {
        case LWREG_VALUE_RANGE_TYPE_ENUM:

            dwError = RegWC16StringArraysAllocateFromCStringArraysWithNullTerminator(
                            attrA.Range.ppszRangeEnumStrings,
                            &pAttr->Range.ppwszRangeEnumStrings);
            BAIL_ON_REG_ERROR(dwError);

            break;

        case LWREG_VALUE_RANGE_TYPE_INTEGER:
             pAttr->Range.RangeInteger.Min = attrA.Range.RangeInteger.Min;
             pAttr->Range.RangeInteger.Max = attrA.Range.RangeInteger.Max;

             break;

        case LWREG_VALUE_RANGE_TYPE_BOOLEAN:
             pAttr->Range.RangeInteger.Min = 0;
             pAttr->Range.RangeInteger.Max = 1;

             break;

        default:
             dwError = ERROR_INVALID_PARAMETER;
             BAIL_ON_REG_ERROR(dwError);
      }

    if (attrA.pszDocString)
    {
        dwError = RegWC16StringAllocateFromCString(
                             &pAttr->pwszDocString,
                             attrA.pszDocString
                               );
        BAIL_ON_REG_ERROR(dwError);
    }

    pAttr->ValueType = attrA.ValueType;

    dwError = RegCopyValueAToW(pAttr->ValueType,
                               attrA.pDefaultValue,
                               attrA.DefaultValueLen,
                               &pAttr->pDefaultValue,
                               &pAttr->DefaultValueLen);
    BAIL_ON_REG_ERROR(dwError);

    *ppAttrW = pAttr;

cleanup:
    return dwError;

error:

    RegSafeFreeValueAttributes(&pAttr);

    goto cleanup;
}
#endif


static
NTSTATUS
RegHexCharToByte(
    CHAR cHexChar,
    UCHAR* pucByte
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR ucByte = 0;

    if (cHexChar >= '0' && cHexChar <= '9')
    {
       ucByte = (UCHAR)(cHexChar - '0');
    }
    else if (cHexChar >= 'a' && cHexChar <= 'f')
    {
       ucByte = 10 + (UCHAR)(cHexChar - 'a');
    }
    else if (cHexChar >= 'A' && cHexChar <= 'F')
    {
       ucByte = 10 + (UCHAR)(cHexChar - 'A');
    }
    else
    {
    	status = STATUS_INVALID_PARAMETER;
       BAIL_ON_NT_STATUS(status);
    }

    *pucByte = ucByte;

cleanup:

    return status;

error:

    *pucByte = 0;

    goto cleanup;
}

static
void
RegStripLeadingWhitespace(
    PSTR pszString
    )
{
    PSTR pszNew = pszString;
    PSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0' || !isspace((int)*pszString)) {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0' && isspace((int)*pszTmp)) {
        pszTmp++;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        *pszNew++ = *pszTmp++;
    }
    *pszNew = '\0';
}

static
void
RegStripTrailingWhitespace(
    PSTR pszString
    )
{
    PSTR pszLastSpace = NULL;
    PSTR pszTmp = pszString;

    if (LW_IS_NULL_OR_EMPTY_STR(pszString))
        return;

    while (pszTmp != NULL && *pszTmp != '\0') {
        pszLastSpace = (isspace((int)*pszTmp) ? (pszLastSpace ? pszLastSpace : pszTmp) : NULL);
        pszTmp++;
    }

    if (pszLastSpace != NULL) {
        *pszLastSpace = '\0';
    }
}

