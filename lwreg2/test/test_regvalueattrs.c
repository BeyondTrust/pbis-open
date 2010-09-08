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
 *        test_regvalueattrs.c
 *
 * Abstract:
 *
 *        New Registry Value Attributes Related API Tests
 *
 *
 * Authors: Wei Fu (wfu@likewise.com)
 */
#include "includes.h"


CHAR szCurrentVal[] = "Current value";
CHAR szDefaultVal[] = "Default value";
CHAR szDocString[] = "Document String";

CHAR* ppszRangeEnumStrings[] = {"enum1", "enum2", "enum3", NULL};

LWREG_VALUE_ATTRIBUTES_A ValueAttribute = {
        REG_SZ,
        szCurrentVal,
        sizeof(szCurrentVal),
        szDefaultVal,
        sizeof(szDefaultVal),
        szDocString,
        LWREG_VALUE_RANGE_TYPE_ENUM,
        0};


static
DWORD
TestAllocateWC16StringArraysFromCStringArrays(
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
    RegFreeWC16StringArrayWithNullTerminator(ppwszStrings);
    *ppwszStrings = NULL;

    goto cleanup;
}

static
DWORD
TestCopyValueAToValueW(
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

        memcpy(pOutData, pData, cbData);
        cbOutDataLen = cbData;
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

static
DWORD
TestConvertAttrAToAttrW(
    LWREG_VALUE_ATTRIBUTES_A attrA,
    PLWREG_VALUE_ATTRIBUTES* ppAttrW
    )
{
    DWORD dwError = 0;
    PLWREG_VALUE_ATTRIBUTES pAttr = NULL;

    dwError = RegAllocateMemory(sizeof(*pAttr),(PVOID*)&pAttr);
    BAIL_ON_REG_ERROR(dwError);

    pAttr->CurrentValueLen = attrA.CurrentValueLen;
    pAttr->DefaultValueLen = attrA.DefaultValueLen;
    pAttr->Hint = attrA.Hint;
    pAttr->RangeType = attrA.RangeType;

    switch (pAttr->RangeType)
    {
        case LWREG_VALUE_RANGE_TYPE_ENUM:

            dwError = TestAllocateWC16StringArraysFromCStringArrays(
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

    dwError = TestCopyValueAToValueW(pAttr->ValueType,
                                     attrA.pCurrentValue,
                                     attrA.CurrentValueLen,
                                     &pAttr->pCurrentValue,
                                     &pAttr->CurrentValueLen);
    BAIL_ON_REG_ERROR(dwError);

    dwError = TestCopyValueAToValueW(pAttr->ValueType,
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


int main(int argc, char *argv[])
{
    DWORD dwError;
    HANDLE hReg = NULL;
    PLWREG_VALUE_ATTRIBUTES pAttr = NULL;
    PWSTR* ppwszRootKeyNames = NULL;
    DWORD dwNumRootKeys = 0;
    wchar16_t szValueName[] = {'a','t','t','r',0};
    HKEY hKey = NULL;

    ValueAttribute.Range.ppszRangeEnumStrings = ppszRangeEnumStrings;

    dwError = TestConvertAttrAToAttrW(ValueAttribute,
                                      &pAttr);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenServer(&hReg);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegEnumRootKeysW(hReg,
                               &ppwszRootKeyNames,
                               &dwNumRootKeys);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExW(
                hReg,
                NULL,
                ppwszRootKeyNames[0],
                0,
                KEY_READ,
                &hKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSetValueAttributesW(
                   hReg,
                   hKey,
                   NULL,
                   szValueName,
                   pAttr);
    BAIL_ON_REG_ERROR(dwError);

cleanup:

    if (hKey)
    {
        RegCloseKey(hReg, hKey);
    }

    if (hReg)
    {
        RegCloseServer(hReg);
    }

    RegFreeWC16StringArray(ppwszRootKeyNames,
                           dwNumRootKeys);

    RegSafeFreeValueAttributes(&pAttr);

    return dwError;

error:

    goto cleanup;
}
