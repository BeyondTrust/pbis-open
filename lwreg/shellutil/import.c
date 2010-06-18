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
 *        import.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry import utilities (import a win32 compatible registry file)
 *        Originally implemented for regimport (regimport.c)
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *          Adam Bernstein (abernstein@likewise.com)
 *
 */
#include "includes.h"


static
DWORD
ProcessImportedKeyName(
    HANDLE hReg,
    PCSTR pszKeyName
    )
{
    DWORD dwError = 0;

    PCSTR pszDelim = "\\";
    PWSTR pSubKey = NULL;
    HKEY pNewKey = NULL;
    PSTR pszCurrKeyName = NULL;
    PSTR pszPrevKeyName = NULL;
    PSTR pszKeyToken = NULL;
    PSTR pszStrTokSav = NULL;
    HKEY hCurrKey = NULL;
    PSTR pszKeyNameCopy = NULL;


    dwError = RegCStringDuplicate(&pszKeyNameCopy, pszKeyName);
    BAIL_ON_REG_ERROR(dwError);

    pszKeyToken = strtok_r (pszKeyNameCopy, pszDelim, &pszStrTokSav);
    if (!LW_IS_NULL_OR_EMPTY_STR(pszKeyToken))
    {
        if (!strcmp(pszKeyToken, HKEY_THIS_MACHINE))
        {
            dwError = RegOpenKeyExA(
                           hReg,
                           NULL,
                           HKEY_THIS_MACHINE,
                           0,
                           KEY_ALL_ACCESS,
                           &hCurrKey);
                BAIL_ON_REG_ERROR(dwError);
        }
        else
        {
            REG_LOG_DEBUG("Found invalid Key Path in REG file that is imported");
            goto cleanup;
        }
    }

    pszKeyToken = strtok_r (NULL, pszDelim, &pszStrTokSav);
    while (!LW_IS_NULL_OR_EMPTY_STR(pszKeyToken))
    {
    	dwError = RegWC16StringAllocateFromCString(&pSubKey,
    			                                     pszKeyToken);
    	BAIL_ON_REG_ERROR(dwError);

        dwError = RegCreateKeyExW(
            hReg,
            hCurrKey,
            pSubKey,
            0,
            NULL,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &pNewKey,
            NULL
            );
        if (LWREG_ERROR_KEYNAME_EXIST == dwError)
        {
        	// Key has already been created (open it).
        	dwError = RegOpenKeyExW(hReg,
        		                    hCurrKey,
        	                        pSubKey,
        	                        0,
        	                        KEY_ALL_ACCESS,
        	                        &pNewKey);
        	BAIL_ON_REG_ERROR(dwError);
        }
        BAIL_ON_REG_ERROR(dwError);

        LWREG_SAFE_FREE_MEMORY(pSubKey);

        if (hCurrKey)
        {
            dwError = RegCloseKey(hReg, hCurrKey);
            BAIL_ON_REG_ERROR(dwError);
            hCurrKey = NULL;
        }

        hCurrKey = pNewKey;
        pNewKey = NULL;

        pszKeyToken = strtok_r (NULL, pszDelim, &pszStrTokSav);
    }


cleanup:
    LWREG_SAFE_FREE_STRING(pszCurrKeyName);
    LWREG_SAFE_FREE_STRING(pszPrevKeyName);
    LWREG_SAFE_FREE_MEMORY(pSubKey);
    LWREG_SAFE_FREE_STRING(pszKeyNameCopy);

    if (pNewKey)
    {
        dwError = RegCloseKey(hReg,pNewKey);
    }
 
    if (hCurrKey)
    {
        dwError = RegCloseKey(hReg,hCurrKey);
    }

    return dwError;

error:
    goto cleanup;
}

static
DWORD
ProcessImportedValue(
    HANDLE hReg,
    PREG_PARSE_ITEM pItem,
    REGSHELL_UTIL_IMPORT_MODE eMode
    )
{
    DWORD dwError = 0;
    CHAR c = '\\';
    PCSTR pszDelim = "\\";
    PSTR pszKeyToken = NULL;
    PSTR pszStrTokSav = NULL;
    //Do not free
    PSTR pszSubKeyName = NULL;
    PWSTR pSubKey = NULL;
    //Do not close
    HKEY hKey = NULL;
    //Do not close
    HKEY hCurrRootKey = NULL;
    HKEY hRootKey = NULL;
    HKEY hSubKey = NULL;
    PSTR pszKeyName = NULL;
    PBYTE pData = NULL;
    DWORD cbData = 0;
    DWORD dwDataType = 0;
    BOOLEAN bSetValue = TRUE;
    PWSTR pwszValueName = NULL;

    BAIL_ON_INVALID_HANDLE(hReg);

    dwError = RegCStringDuplicate(&pszKeyName, (PCSTR)pItem->keyName);
    BAIL_ON_REG_ERROR(dwError);

    pszKeyToken = strtok_r (pszKeyName, pszDelim, &pszStrTokSav);

    if (LW_IS_NULL_OR_EMPTY_STR(pszKeyToken))
    {
        pszKeyToken = pszKeyName;
    }

    if (!strcmp(pszKeyToken, HKEY_THIS_MACHINE))
    {
        if (!hRootKey)
        {
            dwError = RegOpenKeyExA(
                          hReg,
                          NULL,
                          HKEY_THIS_MACHINE,
                          0,
                          KEY_ALL_ACCESS,
                          &hRootKey);
            BAIL_ON_REG_ERROR(dwError);
        }

        hCurrRootKey = hRootKey;
    }
    else
    {
        REG_LOG_DEBUG("Found invalid Key Path in REG file that is imported");
        goto cleanup;
    }

    pszSubKeyName = strchr(pItem->keyName, c);

    if (pszSubKeyName && !LW_IS_NULL_OR_EMPTY_STR(pszSubKeyName+1))
    {
        //Open the subkey
    	dwError = RegWC16StringAllocateFromCString(&pSubKey,
    			                                     pszSubKeyName+1);
    	BAIL_ON_REG_ERROR(dwError);

        dwError = RegOpenKeyExW(
            hReg,
            hCurrRootKey,
            pSubKey,
            0,
            KEY_ALL_ACCESS,
            &hSubKey);
        BAIL_ON_REG_ERROR(dwError);

        hKey = hSubKey;
    }
    else
    {
        hKey = hCurrRootKey;
    }

    cbData = pItem->type == REG_SZ ? pItem->valueLen+1 : pItem->valueLen;

    dwError = RegAllocateMemory(sizeof(*pData) * cbData, (PVOID*)&pData);
    BAIL_ON_REG_ERROR(dwError);

    memcpy(pData, (PBYTE)pItem->value, cbData);

    if (eMode == REGSHELL_UTIL_IMPORT_UPGRADE)
    {
        dwError = RegGetValueA(
                      hReg,
                      hKey,
                      NULL,
                      pItem->valueName,
                      0,
                      &dwDataType,
                      NULL,
                      NULL);
        if (dwError == 0)
        {
            bSetValue = FALSE;
        }
        else
        {
            printf("[%s%s]\n",
                   HKEY_THIS_MACHINE,
                   pszSubKeyName ? pszSubKeyName : "");
            printf("    '%s' Merged.\n", pItem->valueName);
        }
    }

    if (bSetValue)
    {
        if (pItem->type == REG_MULTI_SZ)
        {
            dwError = RegWC16StringAllocateFromCString(
                          &pwszValueName,
                          pItem->valueName);
            BAIL_ON_REG_ERROR(dwError);
   
            dwError = RegSetValueExW(
                          hReg,
                          hKey,
                          pwszValueName,
                          0,
                          pItem->type,
                          pData,
                          cbData);
        }
        else 
        {
            dwError = RegSetValueExA(
                          hReg,
                          hKey,
                          pItem->valueName,
                          0,
                          pItem->type,
                          pData,
                          cbData);
        }
        BAIL_ON_REG_ERROR(dwError);
    }


cleanup:
    LWREG_SAFE_FREE_MEMORY(pSubKey);
    LWREG_SAFE_FREE_STRING(pszKeyName);
    LWREG_SAFE_FREE_MEMORY(pData);
    LWREG_SAFE_FREE_MEMORY(pwszValueName);

    if (hSubKey)
    {
        dwError = RegCloseKey(hReg, hSubKey);
    }
    if (hRootKey)
    {
        dwError = RegCloseKey(hReg,hRootKey);
    }

    return dwError;

error:
    goto cleanup;
}


DWORD 
RegShellUtilImportCallback(
    PREG_PARSE_ITEM pItem,
    HANDLE hUserCtx
    )
{
    DWORD dwError = 0;
    PREGSHELL_UTIL_IMPORT_CONTEXT pImportCtx =
        (PREGSHELL_UTIL_IMPORT_CONTEXT ) hUserCtx;

    if (pItem->type == REG_KEY)
    {
        dwError = ProcessImportedKeyName(
                pImportCtx->hReg,
                (PCSTR)pItem->keyName);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        dwError = ProcessImportedValue(
                 pImportCtx->hReg,
                 pItem,
                 pImportCtx->eImportMode);
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:

    return dwError;

error:
    goto cleanup;
}
