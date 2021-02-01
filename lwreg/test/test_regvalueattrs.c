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

DWORD dwDefaultVal = 8;

CHAR* ppszRangeEnumStrings[] = {"enum1", "enum2", "enum3", NULL};

LWREG_VALUE_ATTRIBUTES_A ValueAttribute = {
        REG_SZ,
        szDefaultVal,
        sizeof(szDefaultVal),
        szDocString,
        LWREG_VALUE_RANGE_TYPE_ENUM,
        0};

LWREG_VALUE_ATTRIBUTES_A ValueAttribute_int = {
        REG_DWORD,
        &dwDefaultVal,
        sizeof(DWORD),
        szDocString,
        LWREG_VALUE_RANGE_TYPE_INTEGER,
        0};


int main(int argc, char *argv[])
{
    DWORD dwError;
    HANDLE hReg = NULL;
    PLWREG_VALUE_ATTRIBUTES pAttr = NULL;
    PLWREG_VALUE_ATTRIBUTES pAttr_int = NULL;
    PWSTR* ppwszRootKeyNames = NULL;
    DWORD dwNumRootKeys = 0;
    wchar16_t szSubKey[] = {'a', 0};
    wchar16_t szSubKey1[] = {'b', 0};
    wchar16_t szValueName[] = {'a','t','t','r',0};
    wchar16_t szValueName1[] = {'a','t','t','r','1',0};
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    HKEY hSubSubKey = NULL;

    PLWREG_CURRENT_VALUEINFO pCurrentValue = NULL;
    PLWREG_VALUE_ATTRIBUTES pValueAttributes = NULL;
    PLWREG_VALUE_ATTRIBUTES pValueAttributes_int = NULL;

    ValueAttribute.Range.ppszRangeEnumStrings = ppszRangeEnumStrings;
    ValueAttribute_int.Range.RangeInteger.Max = 100;
    ValueAttribute_int.Range.RangeInteger.Min = 1;

    DWORD dwType = REG_NONE;
    PBYTE pData[MAX_VALUE_LENGTH] = {0};
    DWORD cbData = MAX_VALUE_LENGTH;

    wchar16_t pCurrData[] = {'C', 'u', 'r', 'r', 'e', 'n', 't', 0};


    dwError = RegConvertValueAttributesAToW(ValueAttribute,
                                            &pAttr);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegConvertValueAttributesAToW(ValueAttribute_int,
                                            &pAttr_int);
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
                KEY_ALL_ACCESS | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | DELETE,
                &hKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegCreateKeyExW(
                hReg,
                hKey,
                szSubKey,
                0,
                NULL,
                0,
                KEY_ALL_ACCESS,
                NULL,
                &hSubKey,
                NULL);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegCreateKeyExW(
                hReg,
                hSubKey,
                szSubKey1,
                0,
                NULL,
                0,
                KEY_ALL_ACCESS,
                NULL,
                &hSubSubKey,
                NULL);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSetValueAttributesW(
                   hReg,
                   hKey,
                   NULL,
                   szValueName,
                   pAttr);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSetValueAttributesW(
                   hReg,
                   hKey,
                   NULL,
                   szValueName1,
                   pAttr_int);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSetValueAttributesW(
                   hReg,
                   hKey,
                   szSubKey,
                   szValueName,
                   pAttr);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSetValueAttributesW(
                   hReg,
                   hKey,
                   szSubKey,
                   szValueName1,
                   pAttr);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSetValueAttributesW(
                   hReg,
                   hSubSubKey,
                   NULL,
                   szValueName1,
                   pAttr_int);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSetValueAttributesW(
                   hReg,
                   hSubSubKey,
                   NULL,
                   szValueName,
                   pAttr);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegGetValueAttributesW(
                   hReg,
                   hKey,
                   NULL,
                   szValueName,
                   &pCurrentValue,
                   &pValueAttributes);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegGetValueAttributesW(
                   hReg,
                   hKey,
                   NULL,
                   szValueName1,
                   NULL,
                   &pValueAttributes_int);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSetValueExW(
                  hReg,
                  hSubKey,
                  szValueName,
                  0,
                  REG_SZ,
                  (const BYTE*)pCurrData,
                  (wc16slen(pCurrData)+1)*sizeof(*pCurrData));
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegGetValueW(
                   hReg,
                   hKey,
                   szSubKey,
                   szValueName,
                   RRF_RT_REG_NONE,
                   &dwType,
                   pData,
                   &cbData);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegDeleteValueAttributesW(
                   hReg,
                   hKey,
                   NULL,
                   szValueName);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegDeleteTreeW(
                   hReg,
                   hKey,
                   szSubKey);
    if (LWREG_ERROR_KEY_IS_ACTIVE == dwError)
    {
        if (hSubKey)
        {
            RegCloseKey(hReg, hSubKey);
        }
        if (hSubSubKey)
        {
            RegCloseKey(hReg, hSubSubKey);
        }
        dwError = RegDeleteTreeW(
                hReg,
                hKey,
                szSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }
    BAIL_ON_REG_ERROR(dwError);

cleanup:

    if (hKey)
    {
        RegCloseKey(hReg, hKey);
    }
    if (hSubKey)
    {
        RegCloseKey(hReg, hSubKey);
    }
    if (hSubSubKey)
    {
        RegCloseKey(hReg, hSubSubKey);
    }


    if (hReg)
    {
        RegCloseServer(hReg);
    }

    RegFreeWC16StringArray(ppwszRootKeyNames,
                           dwNumRootKeys);

    RegSafeFreeValueAttributes(&pAttr);
    RegSafeFreeCurrentValueInfo(&pCurrentValue);
    RegSafeFreeValueAttributes(&pValueAttributes);


    return dwError;

error:

    goto cleanup;
}
