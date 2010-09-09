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


int main(int argc, char *argv[])
{
    DWORD dwError;
    HANDLE hReg = NULL;
    PLWREG_VALUE_ATTRIBUTES pAttr = NULL;
    PWSTR* ppwszRootKeyNames = NULL;
    DWORD dwNumRootKeys = 0;
    wchar16_t szValueName[] = {'a','t','t','r',0};
    HKEY hKey = NULL;

    PLWREG_CURRENT_VALUEINFO pCurrentValue = NULL;
    PLWREG_VALUE_ATTRIBUTES pValueAttributes = NULL;

    ValueAttribute.Range.ppszRangeEnumStrings = ppszRangeEnumStrings;

    dwError = RegConvertAttrAToAttrW(ValueAttribute,
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

    dwError = RegGetValueAttributesW(
                   hReg,
                   hKey,
                   NULL,
                   szValueName,
                   &pCurrentValue,
                   &pValueAttributes);
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
    RegSafeFreeCurrentValueInfo(&pCurrentValue);
    RegSafeFreeValueAttributes(&pValueAttributes);

    return dwError;

error:

    goto cleanup;
}
