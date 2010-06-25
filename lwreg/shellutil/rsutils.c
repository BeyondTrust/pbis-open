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
 *        export.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry utility functions for regshell
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *          Adam Bernstein (abernstein@likewise.com)
 *
 */
#include "includes.h"

DWORD
RegShellCanonicalizePath(
    PSTR pszInDefaultKey,
    PSTR pszInKeyName,
    PSTR *ppszFullPath,
    PSTR *ppszParentPath,
    PSTR *ppszSubKey)
{
    DWORD dwError = 0;
    DWORD dwFullPathLen = 0;
    PSTR pszNewPath = NULL;
    PSTR pszToken = NULL;
    PSTR pszStrtokState = NULL;
    PSTR pszTmp = NULL;
    PSTR pszStrtokValue = NULL;

    /*
     * Path is already fully qualified, so just return KeyName as the full path
     */
    if (pszInKeyName && (pszInKeyName[0] == '\\'))
    {
        dwError = RegCStringDuplicate(&pszNewPath, pszInKeyName);
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (pszInKeyName || pszInDefaultKey)
    {
        if (pszInDefaultKey)
        {
            dwFullPathLen += strlen(pszInDefaultKey);
        }
        if (pszInKeyName)
        {
            dwFullPathLen += strlen(pszInKeyName);
        }

        /* Leading and separating \ and \0 characters */
        dwFullPathLen += 3;

        dwError = RegAllocateMemory(sizeof(*pszNewPath) * dwFullPathLen, (PVOID*)&pszNewPath);
        BAIL_ON_REG_ERROR(dwError);

        if (pszInDefaultKey)
        {
            strcat(pszNewPath, "\\");
            strcat(pszNewPath, pszInDefaultKey);
        }
        if (pszInKeyName)
        {
            /* Copy of pszInKeyName because strtok_r modifies this value */
            dwError = RegCStringDuplicate(&pszStrtokValue, pszInKeyName);
            BAIL_ON_REG_ERROR(dwError);

            pszToken = strtok_r(pszStrtokValue, "\\", &pszStrtokState);
            while (pszToken)
            {
                if (strcmp(pszToken, ".") == 0)
                {
                    /* Do nothing */
                }
                else if (strcmp(pszToken, "..") == 0)
                {
                    /*
                     * Subtract one directory off the NewPath being
                     * constructed
                     */
                    pszTmp = strrchr(pszNewPath, '\\');
                    if (pszTmp)
                    {
                        *pszTmp = '\0';
                    }
                    else if (strlen(pszNewPath) > 0)
                    {
                        *pszNewPath = '\0';
                    }
                }
                else if (strncmp(pszToken, "...", 3) == 0)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                    BAIL_ON_REG_ERROR(dwError);
                }
                else
                {
                    /*
                     * Append token to end of newPath
                     */
                    strcat(pszNewPath, "\\");
                    strcat(pszNewPath, pszToken);
                }
                pszToken = strtok_r(NULL, "\\/", &pszStrtokState);
            }
        }
    }
    else if (!pszInDefaultKey)
    {
        dwError = RegCStringDuplicate(&pszNewPath, "\\");
        BAIL_ON_REG_ERROR(dwError);
    }

    if (strlen(pszNewPath) == 0)
    {
        strcpy(pszNewPath, "\\");
    }
#ifdef _LW_DEBUG
    printf("CanonicalizePath: pszNewPath='%s'\n", pszNewPath);
#endif
    if (ppszParentPath)
    {
        dwError = RegCStringDuplicate(ppszParentPath, pszNewPath);
        BAIL_ON_REG_ERROR(dwError);

        pszTmp = strrchr(*ppszParentPath, '\\');
        if (pszTmp)
        {
            if (pszTmp == *ppszParentPath)
            {
                pszTmp[1] = '\0';
            }
            else
            {
                pszTmp[0] = '\0';
            }
        }
    }

    if (ppszSubKey)
    {
        pszTmp = strrchr(pszNewPath, '\\');
        if (pszTmp)
        {
            dwError = RegCStringDuplicate(ppszSubKey, pszTmp+1);
            BAIL_ON_REG_ERROR(dwError);
        }
        else
        {
            dwError = RegCStringDuplicate(ppszSubKey, "");
            BAIL_ON_REG_ERROR(dwError);
        }
    }

    if (ppszFullPath)
    {
        *ppszFullPath = pszNewPath;
        pszNewPath = NULL;
    }

cleanup:
    LWREG_SAFE_FREE_STRING(pszNewPath);
    LWREG_SAFE_FREE_MEMORY(pszStrtokValue);
    LWREG_SAFE_FREE_STRING(pszNewPath);
    return dwError;

error:
    goto cleanup;
}



DWORD
RegShellIsValidKey(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszKey)
{
    DWORD dwError = 0;
    HKEY pFullKey = NULL;
    HKEY pRootKey = NULL;
    PWSTR pSubKey = NULL;

    BAIL_ON_INVALID_HANDLE(hReg);
    BAIL_ON_INVALID_POINTER(pszKey);

    if (!pszRootKeyName)
    {
        pszRootKeyName = HKEY_THIS_MACHINE;
    }
    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, KEY_READ, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

	dwError = RegWC16StringAllocateFromCString(&pSubKey, pszKey);
	BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExW(
        hReg,
        pRootKey,
        pSubKey,
        0,
        KEY_READ,
        &pFullKey);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    
    LWREG_SAFE_FREE_MEMORY(pSubKey);
    if (pFullKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellUtilAddKeySecDesc(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR pszKeyName,
    BOOLEAN bDoBail,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor
    )
{
    DWORD dwError = 0;
    HANDLE hRegLocal = NULL;
    HKEY pNextKey = NULL;
    HKEY pCurrentKey = NULL; //key that to be performed more operations on
    PWSTR pwszSubKey = NULL;
    PSTR pszToken = NULL;
    PSTR pszStrtokState = NULL;
    PSTR pszDelim = "\\";
    PSTR pszFullPath = NULL;
    PSTR pszSubKey = NULL;
    DWORD dwDisposition = 0;


    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }

    if (!pszRootKeyName)
    {
        pszRootKeyName = HKEY_THIS_MACHINE;
    }
    dwError = RegShellCanonicalizePath(pszDefaultKey,
                                       pszKeyName,
                                       &pszFullPath,
                                       NULL,
                                       &pszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, KEY_ALL_ACCESS, &pCurrentKey);
    BAIL_ON_REG_ERROR(dwError);

    pszToken = strtok_r(pszFullPath, pszDelim, &pszStrtokState);
    while (!LW_IS_NULL_OR_EMPTY_STR(pszToken))
    {
        dwError = RegWC16StringAllocateFromCString(&pwszSubKey, pszToken);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegCreateKeyExW(
                      hReg,
                      pCurrentKey,
                      pwszSubKey,
                      0,
                      NULL,
                      0,
                      AccessDesired,
                      pSecurityDescriptor,
                      &pNextKey,
                      &dwDisposition);
        BAIL_ON_REG_ERROR(dwError);

        LWREG_SAFE_FREE_MEMORY(pwszSubKey);

        if (pCurrentKey)
        {
            dwError = RegCloseKey(hReg, pCurrentKey);
            BAIL_ON_REG_ERROR(dwError);
            pCurrentKey = NULL;
        }
        pCurrentKey = pNextKey;
        pNextKey = NULL;

        pszToken = strtok_r (NULL, pszDelim, &pszStrtokState);

        if (LW_IS_NULL_OR_EMPTY_STR(pszToken) &&
        	REG_OPENED_EXISTING_KEY == dwDisposition && bDoBail)
        {
            dwError = LWREG_ERROR_KEYNAME_EXIST;
            BAIL_ON_REG_ERROR(dwError);
        }
    }

cleanup:
    RegCloseServer(hRegLocal);
    if (pCurrentKey)
    {
        RegCloseKey(hReg, pCurrentKey);
    }
    if (pNextKey)
    {
        RegCloseKey(hReg, pNextKey);
    }
    LWREG_SAFE_FREE_STRING(pszFullPath);
    LWREG_SAFE_FREE_STRING(pszSubKey);
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);

    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellUtilAddKey(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR pszKeyName,
    BOOLEAN bDoBail
    )
{
    return RegShellUtilAddKeySecDesc(
               hReg,
               pszRootKeyName,
               pszDefaultKey,
               pszKeyName,
               bDoBail,
               KEY_ALL_ACCESS,
               NULL);
}


DWORD
RegShellUtilDeleteKey(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName)
{
    HANDLE hRegLocal = NULL;
    PWSTR pwszSubKey = NULL;
    HKEY pCurrentKey = NULL;
    HKEY pRootKey = NULL;
    DWORD dwError = 0;
    PSTR pszFullPath = NULL;
    PSTR pszParentPath = NULL;
    PSTR pszSubKey = NULL;

    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }


    if (!pszRootKeyName)
    {
        pszRootKeyName = HKEY_THIS_MACHINE;
    }
    dwError = RegShellCanonicalizePath(pszDefaultKey,
                                       keyName,
                                       &pszFullPath,
                                       &pszParentPath,
                                       &pszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegShellIsValidKey(hReg, pszRootKeyName, pszFullPath+1);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, KEY_ALL_ACCESS, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    if (pszParentPath && pszParentPath[1])
    {
        dwError = RegWC16StringAllocateFromCString(
                      &pwszSubKey,
                      pszParentPath+1);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegOpenKeyExW(
                      hReg,
                      pRootKey,
                      pwszSubKey,
                      0,
                      KEY_ALL_ACCESS | DELETE,
                      &pCurrentKey);
        BAIL_ON_REG_ERROR(dwError);
        LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    }
    else 
    {
        pCurrentKey = pRootKey;
        pRootKey = NULL;
    }

    dwError = RegWC16StringAllocateFromCString(&pwszSubKey, pszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegDeleteKeyW(hReg, pCurrentKey, pwszSubKey);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    RegCloseServer(hRegLocal);
    if (pCurrentKey)
    {
        RegCloseKey(hReg, pCurrentKey);
    }
    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
                                       
    LWREG_SAFE_FREE_STRING(pszFullPath);
    LWREG_SAFE_FREE_STRING(pszParentPath);
    LWREG_SAFE_FREE_STRING(pszSubKey);
                                       
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellUtilDeleteTree(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName)
{
    HANDLE hRegLocal = NULL;
    PWSTR pwszSubKey = NULL;
    HKEY pCurrentKey = NULL;
    HKEY pRootKey = NULL;
    DWORD dwError = 0;
    PSTR pszFullPath = NULL;
    PSTR pszParentPath = NULL;
    PSTR pszSubKey = NULL;

    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }


    if (!pszRootKeyName)
    {
        pszRootKeyName = HKEY_THIS_MACHINE;
    }
    dwError = RegShellCanonicalizePath(pszDefaultKey,
                                       keyName,
                                       &pszFullPath,
                                       &pszParentPath,
                                       &pszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegShellIsValidKey(hReg, pszRootKeyName, pszFullPath+1);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, KEY_ALL_ACCESS, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    if (pszParentPath && pszParentPath[1])
    {
    	dwError = RegWC16StringAllocateFromCString(&pwszSubKey, pszParentPath+1);
    	BAIL_ON_REG_ERROR(dwError);

        dwError = RegOpenKeyExW(
                      hReg,
                      pRootKey,
                      pwszSubKey,
                      0,
                      KEY_ALL_ACCESS | DELETE,
                      &pCurrentKey);
        BAIL_ON_REG_ERROR(dwError);
        LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    }
    else
    {
        pCurrentKey = pRootKey;
        pRootKey = NULL;
    }

    dwError = RegWC16StringAllocateFromCString(&pwszSubKey, pszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegDeleteTreeW(hReg, pCurrentKey, pwszSubKey);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    RegCloseServer(hRegLocal);
    if (pCurrentKey)
    {
        RegCloseKey(hReg, pCurrentKey);
    }
    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    LWREG_SAFE_FREE_STRING(pszFullPath);
    LWREG_SAFE_FREE_STRING(pszParentPath);
    LWREG_SAFE_FREE_STRING(pszSubKey);
    return dwError;

error:
    goto cleanup;
}

DWORD
RegShellUtilGetKeys(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName,
    LW_WCHAR ***pppRetSubKeys,
    PDWORD pdwRetSubKeyCount)
{
    HANDLE hRegLocal = NULL;
    HKEY pRootKey = NULL;
    HKEY pFullKey = NULL;
    DWORD dwError = 0;
    DWORD dwSubKeyCount = 0;
    DWORD dwValuesCount = 0;
    DWORD dwMaxSubKeyLen = 0;
    DWORD i = 0;
    DWORD dwSubKeyLen = MAX_KEY_LENGTH;
    LW_WCHAR **subKeys = NULL;
    PSTR pszParentPath = NULL;
    PWSTR pwszSubKey = NULL;
    PSTR pszKeyName = NULL;
    PWSTR pwszParentKeyName = NULL;


    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }


    if (!pszRootKeyName)
    {
        return RegEnumRootKeysW(hReg,
                                pppRetSubKeys,
                                pdwRetSubKeyCount);
    }

    dwError = RegShellCanonicalizePath(pszDefaultKey,
                                       keyName,
                                       &pszParentPath,
                                       NULL,
                                       NULL);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, KEY_READ, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    if (pszParentPath && pszParentPath[1])
    {
    	dwError = RegWC16StringAllocateFromCString(&pwszSubKey, pszParentPath+1);
    	BAIL_ON_REG_ERROR(dwError);

        dwError = RegOpenKeyExW(
                      hReg,
                      pRootKey,
                      pwszSubKey,
                      0,
                      KEY_READ,
                      &pFullKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegWC16StringAllocatePrintfW(
                        &pwszParentKeyName,
                        L"%s\\%ws",
                        pszRootKeyName,
                        pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);

        LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    }
    else
    {
        pFullKey = pRootKey;
        pRootKey = NULL;

        dwError = RegWC16StringAllocateFromCString(&pwszParentKeyName, pszRootKeyName);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegQueryInfoKeyA(
        hReg,
        pFullKey,
        NULL,
        NULL,
        NULL,
        &dwSubKeyCount,
        &dwMaxSubKeyLen,
        NULL,
        &dwValuesCount,
        NULL,
        NULL,
        NULL,
        NULL);
    BAIL_ON_REG_ERROR(dwError);

    if (!dwSubKeyCount)
    {
    	goto done;
    }

    dwError = RegAllocateMemory(sizeof(*subKeys) * dwSubKeyCount, (PVOID*)&subKeys);
    BAIL_ON_REG_ERROR(dwError);

#ifdef _LW_DEBUG
    printf( "\nNumber of subkeys: %d\n", dwSubKeyCount);
#endif

    for (i = 0; i < dwSubKeyCount; i++)
    {
        dwSubKeyLen = dwMaxSubKeyLen+1;

        dwError = RegAllocateMemory(sizeof(*pszKeyName) * dwSubKeyLen, (PVOID*)&pszKeyName);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegEnumKeyExA((HANDLE)hReg,
                                pFullKey,
                                i,
                                pszKeyName,
                                &dwSubKeyLen,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegWC16StringAllocatePrintfW(
                        &subKeys[i],
                        L"%ws\\%s",
                        pwszParentKeyName,
                        pszKeyName);
        BAIL_ON_REG_ERROR(dwError);

        LWREG_SAFE_FREE_STRING(pszKeyName);
        pszKeyName = NULL;
    }

done:

    *pppRetSubKeys = subKeys;
    *pdwRetSubKeyCount = dwSubKeyCount;

cleanup:
    RegCloseServer(hRegLocal);
    if (pFullKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }
    LWREG_SAFE_FREE_STRING(pszParentPath);
    LWREG_SAFE_FREE_STRING(pszKeyName);
    LWREG_SAFE_FREE_MEMORY(pwszParentKeyName);

    return dwError;

error:
    for (i = 0; subKeys && i<dwSubKeyCount; i++)
    {
        LWREG_SAFE_FREE_MEMORY(subKeys[i]);
    }
    LWREG_SAFE_FREE_MEMORY(subKeys);
    goto cleanup;
}

DWORD
RegShellUtilSetValue(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName,
    PSTR valueName,
    REG_DATA_TYPE type,
    LW_PVOID data,
    DWORD dataLen
    )
{
    HANDLE hRegLocal = NULL;
    DWORD dwError = 0;
    PBYTE pData = NULL;
    SSIZE_T dwDataLen = 0;
    HKEY pFullKey = NULL;
    HKEY pRootKey = NULL;
    PSTR pszParentPath = NULL;
    DWORD dwOffset = 0;
    PBYTE pMultiStr = NULL;

    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }
    if (!pszRootKeyName)
    {
        pszRootKeyName = HKEY_THIS_MACHINE;
    }

    /*
     *  Key specified with leading \ is a fully-qualified path, so
     * ignore the DefaultKey (pwd) and use only this path.
     */
    if (keyName && keyName[0] == '\\')
    {
        pszDefaultKey = NULL;
        keyName++;
    }
    dwError = RegShellCanonicalizePath(pszDefaultKey,
                                       keyName,
                                       &pszParentPath,
                                       NULL,
                                       NULL);
    BAIL_ON_REG_ERROR(dwError);
    if (pszParentPath[0] == '\\')
    {
        dwOffset++;
    }

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, KEY_SET_VALUE, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    if (pszParentPath && strcmp(pszParentPath, "\\") != 0)
    {

        dwError = RegOpenKeyExA(
                      hReg,
                      pRootKey,
                      &pszParentPath[dwOffset],
                      0,
                      KEY_SET_VALUE,
                      &pFullKey);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        pFullKey = pRootKey;
        pRootKey = NULL;
    }

    switch (type)
    {
        case REG_MULTI_SZ:
            if (!data)
            {
                data = "";
            }
            dwError = RegMultiStrsToByteArrayA(
                          data,
                          &pMultiStr,
                          &dwDataLen);
            BAIL_ON_REG_ERROR(dwError);

            pData = (PBYTE) pMultiStr;
            dwDataLen = dwDataLen * sizeof(CHAR);
            break;

        case REG_SZ:
            dwError = RegCStringDuplicate((LW_PVOID) &pData, data?data:"");
            BAIL_ON_REG_ERROR(dwError);
            dwDataLen = strlen((PSTR) pData)+1;
            break;

        case REG_DWORD:
            dwError = RegAllocateMemory(
                          sizeof(*pData) * sizeof(DWORD),
                          (PVOID*)&pData);
            BAIL_ON_REG_ERROR(dwError);

            dwDataLen = (SSIZE_T) dataLen;
            memcpy(pData, data, dwDataLen);
            break;

        case REG_BINARY:
            pData = data;
            dwDataLen = (SSIZE_T) dataLen;
            break;

        default:
            printf("RegShellSetValue: unknown type %d\n", type);
            break;
    }

    dwError = RegSetValueExA(
        hReg,
        pFullKey,
        valueName,
        0,
        type,
        pData,
        dwDataLen);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    RegCloseServer(hRegLocal);
    if (pFullKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }
    if (type != REG_BINARY)
    {
        LWREG_SAFE_FREE_MEMORY(pData);
    }
    LWREG_SAFE_FREE_STRING(pszParentPath);
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellUtilValueArrayFree(
    PREGSHELL_UTIL_VALUE pValueArray,
    DWORD dwValueArrayLen)
{
    DWORD dwError = 0;
    DWORD i = 0;

    BAIL_ON_INVALID_POINTER(pValueArray);

    for (i=0; i<dwValueArrayLen; i++)
    {
        LWREG_SAFE_FREE_MEMORY(pValueArray[i].pValueName);
        LWREG_SAFE_FREE_MEMORY(pValueArray[i].pData);
    }
    
    LWREG_SAFE_FREE_MEMORY(pValueArray);
cleanup:
    return dwError;
error:
    goto cleanup;

}


DWORD
RegShellUtilGetValues(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName,
    PREGSHELL_UTIL_VALUE *valueArray,
    PDWORD pdwValueArrayLen
    )
{
    HANDLE hRegLocal = NULL;
    DWORD dwError = 0;
    DWORD indx = 0;
    PSTR pszValueName = NULL;
    DWORD dwValueNameLen = 0;
    DWORD regType = 0;
    PBYTE pData = NULL;
    DWORD dwDataLen = 0;
    PWSTR pSubKey = NULL;
    HKEY pRootKey = NULL;
    HKEY pFullKey = NULL;
    PSTR pszParentPath = NULL;
    PWSTR pwszParentPath = NULL;
    DWORD dwValuesCount = 0;
    PREGSHELL_UTIL_VALUE pValueArray = NULL;
    DWORD dwMaxValueNameLen = 0;
    DWORD dwMaxValueLen = 0;

    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }


    if (!pszRootKeyName)
    {
        return 0;
    }
    dwError = RegShellCanonicalizePath(pszDefaultKey,
                                       keyName,
                                       &pszParentPath,
                                       NULL,
                                       NULL);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, KEY_READ, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    if (pszParentPath && strcmp(pszParentPath, "\\") != 0)
    {
    	dwError = RegWC16StringAllocateFromCString(&pwszParentPath, pszParentPath+1);
    	BAIL_ON_REG_ERROR(dwError);

        dwError = RegOpenKeyExW(
                      hReg,
                      pRootKey,
                      pwszParentPath,
                      0,
                      KEY_READ,
                      &pFullKey);
        BAIL_ON_REG_ERROR(dwError);
        LWREG_SAFE_FREE_MEMORY(pwszParentPath);
    }
    else
    {
        pFullKey = pRootKey;
        pRootKey = NULL;
    }

    dwError = RegQueryInfoKeyA(
                  hReg,
                  pFullKey,
                  NULL,
                  NULL,
                  NULL,
                  NULL,
                  NULL,
                  NULL,
                  &dwValuesCount,
                  &dwMaxValueNameLen,
                  &dwMaxValueLen,
                  NULL,
                  NULL);
    BAIL_ON_REG_ERROR(dwError);

    if (!dwValuesCount)
    {
    	goto done;
    }

    dwError = RegAllocateMemory(sizeof(*pValueArray) * dwValuesCount, (PVOID*)&pValueArray);
    BAIL_ON_REG_ERROR(dwError);

    /*
     * Apparently the data size is not returned in bytes for REG_SZ
     * which is why this adjustment is needed.
     */
    dwMaxValueNameLen +=1;

    for (indx = 0; indx < dwValuesCount; indx++)
    {
        /*
         * TBD/adam
         * Add wide character NULL size here; bug in RegEnumValueA()?
         */
        dwValueNameLen = dwMaxValueNameLen;

        dwError = RegAllocateMemory(sizeof(*pszValueName) * dwValueNameLen, (PVOID*)&pszValueName);
        BAIL_ON_REG_ERROR(dwError);

        dwDataLen = dwMaxValueLen;

        if (dwDataLen)
        {
            dwError = RegAllocateMemory(dwDataLen, (PVOID*)&pData);
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = RegEnumValueA(
                      hReg,
                      pFullKey,
                      indx,
                      pszValueName,
                      &dwValueNameLen,
                      NULL,
                      &regType,
                      pData,
                      &dwDataLen);
        BAIL_ON_REG_ERROR(dwError);

    	dwError = RegWC16StringAllocateFromCString(&pValueArray[indx].pValueName, pszValueName);
    	BAIL_ON_REG_ERROR(dwError);
        LWREG_SAFE_FREE_STRING(pszValueName);

        pValueArray[indx].type = regType;
        pValueArray[indx].pData = pData;
        pData = NULL;
        pValueArray[indx].dwDataLen = dwDataLen;
    }

 done:

    *valueArray = pValueArray;
    *pdwValueArrayLen = indx;

cleanup:
    RegCloseServer(hRegLocal);
    if (pFullKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }
    LWREG_SAFE_FREE_STRING(pszValueName);
    LWREG_SAFE_FREE_STRING(pszParentPath);
    LWREG_SAFE_FREE_MEMORY(pSubKey);
    LWREG_SAFE_FREE_MEMORY(pData);
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(pwszParentPath);
    for (indx=0; indx<dwValuesCount; indx++)
    {
        LWREG_SAFE_FREE_MEMORY(pValueArray[indx].pValueName);
        LWREG_SAFE_FREE_MEMORY(pValueArray[indx].pData);
    }
    LWREG_SAFE_FREE_MEMORY(pValueArray);
    goto cleanup;
}


DWORD
RegShellUtilDeleteValue(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName,
    PSTR valueName)
{
    HANDLE hRegLocal = NULL;
    HKEY hRootKey = NULL;
    HKEY hDefaultKey = NULL;
    PSTR pszParentPath = NULL;
    PSTR pszSubKey = NULL;
    DWORD dwError = 0;
    DWORD dwOffset = 0;

    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }

    if (!pszRootKeyName)
    {
        pszRootKeyName = HKEY_THIS_MACHINE;
    }

    /*
     *  Key specified with leading \ is a fully-qualified path, so
     * ignore the DefaultKey (pwd) and use only this path.
     */
    if (keyName && keyName[0] == '\\')
    {
        pszDefaultKey = NULL;
        keyName++;
    }
    dwError = RegShellCanonicalizePath(pszDefaultKey,
                                       keyName,
                                       &pszParentPath,
                                       NULL,
                                       NULL);
    BAIL_ON_REG_ERROR(dwError);
    if (pszParentPath[0] == '\\')
    {
        dwOffset++;
    }

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, KEY_SET_VALUE, &hRootKey);
    BAIL_ON_REG_ERROR(dwError);
    if (pszParentPath && strcmp(pszParentPath, "\\") != 0)
    {
        dwError = RegOpenKeyExA(
                      hReg,
                      hRootKey,
                      &pszParentPath[dwOffset],
                      0,
                      KEY_SET_VALUE,
                      &hDefaultKey);
	BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        hDefaultKey = hRootKey;
        hRootKey = NULL;
    }

    dwError = RegDeleteKeyValueA(hReg, hDefaultKey, NULL, valueName);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    if (hDefaultKey)
    {
        RegCloseKey(hReg, hDefaultKey);
    }
    if (hRootKey)
    {
        RegCloseKey(hReg, hRootKey);
    }
    RegCloseServer(hRegLocal);
    LWREG_SAFE_FREE_STRING(pszParentPath);
    LWREG_SAFE_FREE_STRING(pszSubKey);
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellUtilAllocateMemory(
    HANDLE hReg,
    HKEY hKey,
    REG_DATA_TYPE regType,
    PSTR pszValueName,
    PVOID *ppRetBuf,
    PDWORD pdwRetBufLen)
{
    PBYTE pBuf = NULL;
    DWORD dwError = 0;
    DWORD dwValueLen = 0;

    switch (regType)
    {
        case REG_SZ:
        case REG_MULTI_SZ:
        case REG_BINARY:
            dwError = RegGetValueA(
                          hReg,
                          hKey,
                          NULL,
                          pszValueName,
                          regType,
                          NULL,
                          NULL,
                          &dwValueLen);
            BAIL_ON_REG_ERROR(dwError);
            break;

        default:
            break;
    }
    if (dwError == 0 && dwValueLen > 0)
    {
        dwError = RegAllocateMemory((dwValueLen+1), (PVOID*)&pBuf);
        BAIL_ON_REG_ERROR(dwError);
    }

    memset(pBuf, 0, dwValueLen);
    *ppRetBuf = (PVOID) pBuf;
    *pdwRetBufLen = dwValueLen;

cleanup:
    return dwError;
error:
    LWREG_SAFE_FREE_MEMORY(pBuf);
    goto cleanup;
}


DWORD
RegShellUtilGetValue(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR pszKeyName,
    PSTR pszValueName,
    PREG_DATA_TYPE pRegType,
    PVOID *ppValue,
    PDWORD pdwValueLen)
{
    HANDLE hRegLocal = NULL;
    DWORD dwError = 0;
    DWORD dwValueLen = 0;
    PDWORD pdwValue = (PDWORD) ppValue;
    DWORD dwIndex = 0;
    DWORD dwDataType = 0;
    PVOID pRetData = NULL;
    PBYTE *ppData = (PBYTE *) ppValue;
    PSTR *ppszValue = (PSTR *) ppValue;
    PSTR **pppszMultiValue = (PSTR **) ppValue;
    PSTR *ppszMultiStrArray = NULL;

    HKEY hRootKey = NULL;
    HKEY hDefaultKey = NULL;
    HKEY hFullKeyName = NULL;

    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }

    if (!pszRootKeyName)
    {
        pszRootKeyName = HKEY_THIS_MACHINE;
    }

    /* Open the root key */
    dwError = RegOpenKeyExA(
                      hReg,
                      NULL,
                      pszRootKeyName,
                      0,
                      KEY_READ,
                      &hRootKey);
    BAIL_ON_REG_ERROR(dwError);

    if (pszDefaultKey && *pszDefaultKey)
    {
        /* Open the default key */
        dwError = RegOpenKeyExA(
                          hReg,
                          hRootKey,
                          pszDefaultKey,
                          0,
                          KEY_READ,
                          &hDefaultKey);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        hDefaultKey = hRootKey;
        hRootKey = NULL;
    }

    if (pszKeyName && *pszKeyName)
    {
        /* Open the sub key */
        dwError = RegOpenKeyExA(
                          hReg,
                          hDefaultKey,
                          pszKeyName,
                          0,
                          KEY_READ,
                          &hFullKeyName);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        hFullKeyName = hDefaultKey;
        hDefaultKey = NULL;
    }

    /* Get the type for the valueName data */
    dwError = RegGetValueA(
                  hReg,
                  hFullKeyName,
                  NULL,
                  pszValueName,
                  0,
                  &dwDataType,
                  NULL,
                  NULL);
    BAIL_ON_REG_ERROR(dwError);
    if (pRegType)
    {
        *pRegType = dwDataType;
    }

    dwError = RegShellUtilAllocateMemory(
                  hReg,
                  hFullKeyName,
                  dwDataType,
                  pszValueName,
                  &pRetData,
                  &dwValueLen);
    BAIL_ON_REG_ERROR(dwError);

    switch (dwDataType)
    {
        case REG_SZ:
            dwError = RegGetValueA(
                          hReg,
                          hFullKeyName,
                          NULL,
                          pszValueName,
                          REG_SZ,
                          NULL,
                          pRetData,
                          &dwValueLen);
            BAIL_ON_REG_ERROR(dwError);
            if (ppszValue)
            {
                *ppszValue = pRetData;
                pRetData = NULL;
            }
            if (pdwValueLen)
            {
                *pdwValueLen = dwValueLen;
            }
            break;

        case REG_DWORD:
            dwValueLen = sizeof(DWORD);
            dwError = RegGetValueA(
                          hReg,
                          hFullKeyName,
                          NULL,
                          pszValueName,
                          REG_DWORD,
                          NULL,
                          pdwValue,
                          &dwValueLen);
            BAIL_ON_REG_ERROR(dwError);
            if (pdwValueLen)
            {
                *pdwValueLen = dwValueLen;
            }
            break;

        case REG_BINARY:
            dwError = RegGetValueA(
                          hReg,
                          hFullKeyName,
                          NULL,
                          pszValueName,
                          REG_BINARY,
                          NULL,
                          pRetData,
                          &dwValueLen);
            BAIL_ON_REG_ERROR(dwError);
            if (ppData)
            {
                *ppData = pRetData;
                pRetData = NULL;
            }
            if (pdwValueLen)
            {
                *pdwValueLen = dwValueLen;
            }
            break;

        case REG_MULTI_SZ:
            if (!pppszMultiValue)
            {
                goto cleanup;
            }
            dwError = RegGetValueA(
                          hReg,
                          hFullKeyName,
                          NULL,
                          pszValueName,
                          REG_MULTI_SZ,
                          NULL,
                          pRetData,
                          &dwValueLen);
            BAIL_ON_REG_ERROR(dwError);

            dwError = RegByteArrayToMultiStrsA(
                          pRetData,
                          dwValueLen,
                          &ppszMultiStrArray);
            LWREG_SAFE_FREE_MEMORY(pRetData);
            BAIL_ON_REG_ERROR(dwError);

            /* Return number of entries in multi-string array, not byte count */
            for (dwIndex=0; ppszMultiStrArray[dwIndex]; dwIndex++)
                ;
            *pdwValueLen = dwIndex;
            *pppszMultiValue = ppszMultiStrArray;
            
            break;
    }

cleanup:
    LWREG_SAFE_FREE_MEMORY(pRetData);
    RegCloseServer(hRegLocal);
    if (hFullKeyName)
    {
        RegCloseKey(hReg, hFullKeyName);
    }
    if (hDefaultKey)
    {
        RegCloseKey(hReg, hDefaultKey);
    }
    if (hRootKey)
    {
        RegCloseKey(hReg, hRootKey);
    }
    return dwError;
error:
    goto cleanup;
}


DWORD
RegShellUtilEscapeString(
    PSTR pszValue,
    PSTR *ppszRetValue,
    PDWORD pdwEscapeValueLen)
{
    DWORD i = 0;
    DWORD dwError = 0;
    DWORD dwLen = 0;
    DWORD dwEscapeValueLen = 0;
    PSTR pszRetValue = NULL;

    BAIL_ON_INVALID_POINTER(pszValue);
    BAIL_ON_INVALID_POINTER(ppszRetValue);
    BAIL_ON_INVALID_POINTER(pdwEscapeValueLen);

    /* Count number of \ found in string to escape */
    for (i=0; pszValue[i]; i++)
    {
        if (pszValue[i] == '\\' || pszValue[i] == '\n' || 
            pszValue[i] == '\r' || pszValue[i] == '"' || 
            pszValue[i] == '\t' || pszValue[i] == '\a' || 
            pszValue[i] == '\v' || pszValue[i] == '\f')
        {
            dwEscapeValueLen++;
        }
        dwEscapeValueLen++;
    }
    dwEscapeValueLen++;

    dwError = RegAllocateMemory(sizeof(*pszRetValue)* dwEscapeValueLen, (PVOID*)&pszRetValue);
    BAIL_ON_REG_ERROR(dwError);

    for (i=0; pszValue[i]; i++)
    {
        if (pszValue[i] == '\n')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = 'n';
        }
        if (pszValue[i] == '\r')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = 'r';
        }
        else if (pszValue[i] == '"')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = '"';
        }
        else if (pszValue[i] == '\t')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = 't';
        }
        else if (pszValue[i] == '\a')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = 'a';
        }
        else if (pszValue[i] == '\v')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = 'v';
        }
        else if (pszValue[i] == '\f')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = 'f';
        }
        else if (pszValue[i] == '\\')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = '\\';
        }
        else
        {
            pszRetValue[dwLen++] = pszValue[i];
        }
    }
    pszRetValue[dwLen] = '\0';
    *ppszRetValue = pszRetValue;
    *pdwEscapeValueLen = dwLen;

cleanup:
    return dwError;

error:
    goto cleanup;
}
