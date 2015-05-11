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
    PCSTR pszInDefaultKey,
    PCSTR pszInKeyName,
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
    PCSTR pszRootKeyName,
    PCSTR pszKey)
{
    DWORD dwError = 0;
    HKEY pFullKey = NULL;
    HKEY pRootKey = NULL;
    PWSTR pSubKey = NULL;

    BAIL_ON_INVALID_HANDLE(hReg);

    if (!pszRootKeyName)
    {
        pszRootKeyName = HKEY_THIS_MACHINE;
    }
    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, KEY_READ, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    if (pszKey)
    {
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
    }

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
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR pszKeyName,
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
    if (pCurrentKey)
    {
        RegCloseKey(hReg, pCurrentKey);
    }
    if (pNextKey)
    {
        RegCloseKey(hReg, pNextKey);
    }
    RegCloseServer(hRegLocal);
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
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR pszKeyName,
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
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR keyName)
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
    if (pCurrentKey)
    {
        RegCloseKey(hReg, pCurrentKey);
    }
    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }
    RegCloseServer(hRegLocal);
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
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR keyName)
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
    if (pCurrentKey)
    {
        RegCloseKey(hReg, pCurrentKey);
    }
    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }
    RegCloseServer(hRegLocal);
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
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR keyName,
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
    	dwError = RegWC16StringAllocateFromCString(
                      &pwszSubKey,
                      pszParentPath+1);
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

        dwError = RegWC16StringAllocateFromCString(
                      &pwszParentKeyName,
                      pszRootKeyName);
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

    dwError = RegAllocateMemory(
                  sizeof(*subKeys) * dwSubKeyCount,
                  (PVOID*)&subKeys);
    BAIL_ON_REG_ERROR(dwError);

#ifdef _LW_DEBUG
    printf( "\nNumber of subkeys: %d\n", dwSubKeyCount);
#endif

    for (i = 0; i < dwSubKeyCount; i++)
    {
        dwSubKeyLen = dwMaxSubKeyLen+1;

        dwError = RegAllocateMemory(
                      sizeof(*pszKeyName) * dwSubKeyLen,
                      (PVOID*)&pszKeyName);
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
    if (pFullKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }
    RegCloseServer(hRegLocal);
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
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR keyName,
    PCSTR valueName,
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
    if (pFullKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }
    RegCloseServer(hRegLocal);
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
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR keyName,
    PREGSHELL_UTIL_VALUE *valueArray,
    PDWORD pdwValueArrayLen
    )
{
    HANDLE hRegLocal = NULL;
    DWORD dwError = 0;
    DWORD indx = 0;
    PSTR pszValueName = NULL;
    PWSTR pwszValueName = NULL;
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
    PLWREG_CURRENT_VALUEINFO pCurrentValue = NULL;

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
    	dwError = RegWC16StringAllocateFromCString(
                      &pwszParentPath,
                      pszParentPath+1);
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

    dwError = RegAllocateMemory(
                  sizeof(*pValueArray) * dwValuesCount,
                  (PVOID*)&pValueArray);
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

        dwError = RegAllocateMemory(
                      sizeof(*pszValueName) * dwValueNameLen,
                      (PVOID*)&pszValueName);
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

    	dwError = RegWC16StringAllocateFromCString(
                      &pwszValueName,
                      pszValueName);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwRegGetValueAttributesW(
                      hReg,
                      pFullKey,
                      NULL,
                      pwszValueName,
                      &pCurrentValue,
                      NULL);
        /*
         * Don't care if this fails.
         * Manually added new values don't have attributes to back them.
         */
        if (pCurrentValue)
        {
            pValueArray[indx].bValueSet = TRUE;
        }

    	dwError = RegWC16StringAllocateFromCString(
                      &pValueArray[indx].pValueName,
                      pszValueName);
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
    if (pFullKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }
    RegCloseServer(hRegLocal);
    LWREG_SAFE_FREE_STRING(pszValueName);
    LWREG_SAFE_FREE_STRING(pszParentPath);
    LWREG_SAFE_FREE_MEMORY(pSubKey);
    LWREG_SAFE_FREE_MEMORY(pwszValueName);
    LWREG_SAFE_FREE_MEMORY(pData);
    RegSafeFreeCurrentValueInfo(&pCurrentValue);

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
RegShellUtilGetKeyObjectCounts(
    HANDLE hReg,
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR keyName,
    PDWORD pdwSubKeysCount,
    PDWORD pdwValuesCount
    )
{
    HANDLE hRegLocal = NULL;
    DWORD dwError = 0;
    HKEY pRootKey = NULL;
    HKEY pFullKey = NULL;
    PSTR pszParentPath = NULL;
    PWSTR pwszParentPath = NULL;
    DWORD dwSubKeysCount = 0;
    DWORD dwValuesCount = 0;

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
                  &dwSubKeysCount,
                  NULL,
                  NULL,
                  &dwValuesCount,
                  NULL,
                  NULL,
                  NULL,
                  NULL);
    BAIL_ON_REG_ERROR(dwError);

    if (pdwSubKeysCount)
    {
        *pdwSubKeysCount = dwSubKeysCount;
    }
    if (pdwValuesCount)
    {
        *pdwValuesCount = dwValuesCount;
    }

cleanup:
    if (pFullKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }
    RegCloseServer(hRegLocal);
    LWREG_SAFE_FREE_STRING(pszParentPath);
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(pwszParentPath);
    goto cleanup;
}


DWORD
RegShellUtilDeleteValue(
    HANDLE hReg,
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR keyName,
    PCSTR valueName)
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
    PCSTR pszValueName,
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
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR pszKeyName,
    PCSTR pszValueName,
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
    RegCloseServer(hRegLocal);
    return dwError;
error:
    goto cleanup;
}


DWORD
RegShellUtilEscapeString(
    PCSTR pszValue,
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

    dwError = RegAllocateMemory(
                  sizeof(*pszRetValue)* dwEscapeValueLen,
                  (PVOID*)&pszRetValue);
    BAIL_ON_REG_ERROR(dwError);

    for (i=0; pszValue[i]; i++)
    {
        if (pszValue[i] == '\n')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = 'n';
        }
        else if (pszValue[i] == '\r')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = 'r';
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
        else if (pszValue[i] == '"')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = '"';
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

DWORD
RegExportBinaryTypeToString(
    REG_DATA_TYPE token,
    PSTR tokenStr, BOOLEAN dumpFormat)
{
    DWORD dwError = 0;
    static char *typeStrs[][2] = {
        { "hex(0):", "REG_NONE" },
        { "REG_SZ", "REG_SZ"},
        { "hex(2):", "REG_EXPAND_SZ" },
        { "hex:", "REG_BINARY" },
        { "dword:", "REG_DWORD" },
        { "dwordbe:", "REG_DWORD_BIG_ENDIAN" },
        { "link:", "REG_LINK" },
        { "hex(7):", "REG_MULTI_SZ" },
        { "hex(8):", "REG_RESOURCE_LIST" },
        { "hex(9):", "REG_FULL_RESOURCE_DESCRIPTOR" },
        { "hex(a):", "REG_RESOURCE_REQUIREMENTS_LIST" },
        { "hex(b):", "REG_QUADWORD" },
        { "unknown12:", "REG_UNKNOWN12" },
        { "unknown13:", "REG_UNKNOWN13" },
        { "unknown14:", "REG_UNKNOWN14" },
        { "unknown15:", "REG_UNKNOWN15" },
        { "unknown16:", "REG_UNKNOWN16" },
        { "unknown17:", "REG_UNKNOWN17" },
        { "unknown18:", "REG_UNKNOWN18" },
        { "unknown19:", "REG_UNKNOWN19" },
        { "unknown20:", "REG_UNKNOWN20" },
        { "REG_KEY", "REG_KEY" },
        { "REG_KEY_DEFAULT", "REG_KEY_DEFAULT" },
        { "REG_PLAIN_TEXT", "REG_PLAIN_TEXT" },
        { "REG_UNKNOWN", "REG_UNKNOWN" },
        { "sza:", "REG_STRING_ARRAY" }, /* Maps to REG_MULTI_SZ */
        { "REG_ATTRIBUTES", "REG_ATTRIBUTES" },
    };

    BAIL_ON_INVALID_POINTER(tokenStr);

    if (token < ((sizeof(typeStrs)/sizeof(char *))/2))
    {
        if (dumpFormat)
        {
            strcpy(tokenStr, typeStrs[token][0]);
        }
        else
        {
            strcpy(tokenStr, typeStrs[token][1]);
        }
    }
    else
    {
        sprintf(tokenStr, "ERROR: No Such Token %d", token);
    }


cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
RegExportAttributeEntries(
    PREG_PARSE_ITEM pItem,
    PSTR *ppszDumpString,
    PDWORD pdwDumpStringLen
    )
{
    DWORD dwError = 0;
    PSTR pszDumpString = NULL;
    PSTR pszString = NULL;
    DWORD dwAttrStringLen = 0;
    PSTR pszIndentChar = " ";
    PSTR pszValueNameEsc = NULL;
    DWORD dwValueNameEscLen = 0;
    PWSTR *ppwszRangeEnumStrings = NULL;
    DWORD dwIndentLevel = 4;

    BAIL_ON_INVALID_POINTER(pItem);
    BAIL_ON_INVALID_POINTER(ppszDumpString);
    BAIL_ON_INVALID_POINTER(pdwDumpStringLen);

    dwError = RegShellUtilEscapeString(
                  pItem->valueName,
                  &pszValueNameEsc,
                  &dwValueNameEscLen);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RtlCStringAllocateAppendPrintf(
                  &pszDumpString, "\"%s\" = {\n",
                  pItem->valueName);
    BAIL_ON_REG_ERROR(dwError);

    if (pItem->value)
    {
        dwError = RegExportEntry(
                      NULL,
                      NULL,
                      REG_SZ,    /* Type of value name "value" */
                      "value",
                      pItem->regAttr.ValueType,
                      pItem->value,
                      pItem->valueLen,
                      &pszString,
                      &dwAttrStringLen);
        BAIL_ON_REG_ERROR(dwError);

        if (pszString && dwAttrStringLen)
        {
            dwError = RtlCStringAllocateAppendPrintf(
                          &pszDumpString, "%*s%s\n",
                          dwIndentLevel,
                          pszIndentChar,
                          pszString);
            BAIL_ON_REG_ERROR(dwError);
            LWREG_SAFE_FREE_STRING(pszString);
        }
    }

    if (pItem->regAttr.pDefaultValue)
    {
        dwError = RegExportEntry(
                      NULL,
                      NULL,
                      REG_SZ,    /* Type of value name "value" */
                      "default",
                      pItem->regAttr.ValueType,
                      pItem->regAttr.pDefaultValue,
                      pItem->regAttr.DefaultValueLen,
                      &pszString,
                      &dwAttrStringLen);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RtlCStringAllocateAppendPrintf(
                      &pszDumpString, "%*s%s\n",
                      dwIndentLevel,
                      pszIndentChar,
                      pszString);
        BAIL_ON_REG_ERROR(dwError);
        LWREG_SAFE_FREE_STRING(pszString);
    }

    if (pItem->regAttr.pwszDocString)
    {
        dwError = RegCStringAllocateFromWC16String(
                      &pszString,
                      pItem->regAttr.pwszDocString);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RtlCStringAllocateAppendPrintf(
                      &pszDumpString, "%*sdoc=\"%s\"\n",
                      dwIndentLevel,
                      pszIndentChar,
                      pszString);
        BAIL_ON_REG_ERROR(dwError);
        LWREG_SAFE_FREE_STRING(pszString);
    }

    switch (pItem->regAttr.RangeType)
    {
        case LWREG_VALUE_RANGE_TYPE_BOOLEAN:
            dwError = RtlCStringAllocateAppendPrintf(
                          &pszDumpString, "%*srange=boolean\n",
                          dwIndentLevel,
                          pszIndentChar);
            BAIL_ON_REG_ERROR(dwError);

            break;
        case LWREG_VALUE_RANGE_TYPE_ENUM:
            dwError = RtlCStringAllocateAppendPrintf(
                          &pszDumpString, "%*srange=string:",
                          dwIndentLevel,
                          pszIndentChar);
            BAIL_ON_REG_ERROR(dwError);
            for (ppwszRangeEnumStrings =
                     pItem->regAttr.Range.ppwszRangeEnumStrings;
                 *ppwszRangeEnumStrings;
                 ppwszRangeEnumStrings++)
            {
                dwError = RegCStringAllocateFromWC16String(
                              &pszString,
                              *ppwszRangeEnumStrings);
                BAIL_ON_REG_ERROR(dwError);

                dwError = RtlCStringAllocateAppendPrintf(
                              &pszDumpString, "\"%s\"",
                              pszString);
                BAIL_ON_REG_ERROR(dwError);
                LWREG_SAFE_FREE_STRING(pszString);
                if (ppwszRangeEnumStrings[1])
                {
                    dwError = RtlCStringAllocateAppendPrintf(
                                  &pszDumpString, " ");
                    BAIL_ON_REG_ERROR(dwError);
                }
            }
            dwError = RtlCStringAllocateAppendPrintf(
                          &pszDumpString, "\n");
            BAIL_ON_REG_ERROR(dwError);
            break;
        case LWREG_VALUE_RANGE_TYPE_INTEGER:
            dwError = RtlCStringAllocateAppendPrintf(
                          &pszDumpString, "%*srange=integer:%d-%d\n",
                          dwIndentLevel,
                          pszIndentChar,
                          pItem->regAttr.Range.RangeInteger.Min,
                          pItem->regAttr.Range.RangeInteger.Max);
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            if (pItem->regAttr.RangeType > 0)
            {
                dwError = RtlCStringAllocateAppendPrintf(
                              &pszDumpString, "%*srange=invalid value (%d)\n",
                              dwIndentLevel,
                              pszIndentChar,
                              pItem->regAttr.RangeType);
                BAIL_ON_REG_ERROR(dwError);
            }
            break;
    }

    switch (pItem->regAttr.Hint)
    {
        case LWREG_VALUE_HINT_SECONDS:
            dwError = RtlCStringAllocateAppendPrintf(
                          &pszDumpString, "%*shint=seconds\n",
                          dwIndentLevel,
                          pszIndentChar);
            BAIL_ON_REG_ERROR(dwError);
            break;

        case LWREG_VALUE_HINT_PATH:
            dwError = RtlCStringAllocateAppendPrintf(
                          &pszDumpString, "%*shint=path\n",
                          dwIndentLevel,
                          pszIndentChar);
            BAIL_ON_REG_ERROR(dwError);
            break;

        case LWREG_VALUE_HINT_ACCOUNT:
            dwError = RtlCStringAllocateAppendPrintf(
                          &pszDumpString, "%*shint=account\n",
                          dwIndentLevel,
                          pszIndentChar);
            BAIL_ON_REG_ERROR(dwError);
            break;

        default:
            if (pItem->regAttr.Hint > 0)
            {

                dwError = RtlCStringAllocateAppendPrintf(
                              &pszDumpString, "%*shint=invalid hint (%d)\n",
                              dwIndentLevel,
                              pszIndentChar,
                              pItem->regAttr.Hint);
                BAIL_ON_REG_ERROR(dwError);
            }
    }


    dwError = RtlCStringAllocateAppendPrintf(
                  &pszDumpString, "}");
cleanup:
    *ppszDumpString = pszDumpString;
    *pdwDumpStringLen = strlen(pszDumpString);
    return dwError;

error:
    LWREG_SAFE_FREE_STRING(pszString);
    LWREG_SAFE_FREE_STRING(pszDumpString);
    LWREG_SAFE_FREE_STRING(pszValueNameEsc);
    goto cleanup;
}


DWORD
RegExportAttributes(
    PREG_PARSE_ITEM pItem,
    PSTR *ppszDumpString,
    PDWORD pdwDumpStringLen
    )
{
    DWORD dwError = 0;
    BAIL_ON_INVALID_POINTER(pItem);
    BAIL_ON_INVALID_POINTER(ppszDumpString);
    BAIL_ON_INVALID_POINTER(pdwDumpStringLen);


    if (pItem->type == REG_ATTRIBUTES)
    {
        dwError = RegExportAttributeEntries(
                      pItem,
                      ppszDumpString,
                      pdwDumpStringLen);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        dwError = RegExportEntry(
                      pItem->keyName,
                      NULL,
                      pItem->valueType,
                      pItem->valueName,
                      pItem->type,
                      pItem->value,
                      pItem->valueLen,
                      ppszDumpString,
                      pdwDumpStringLen);
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}


/* Dump multi_sz in sza:"plain text" " format" */
DWORD
RegExportMultiStringArray(
    PCSTR pszValueName,
    PVOID pValue,
    DWORD dwValueLen,
    PSTR *ppszDumpString,
    PDWORD pdwDumpStringLen
    )
{
    DWORD dwError = 0;
    DWORD dwPwszLen= 0;
    PWSTR pwszValue = NULL;
    PSTR pszString = NULL;
    PSTR pszDumpString = NULL;
    PSTR pszEscapedValue = NULL;
    PSTR pszEscapedValueName = NULL;
    DWORD dwEscapedValueLen = 0;
    DWORD dwEscapedValueNameLen = 0;

    BAIL_ON_INVALID_POINTER(pszValueName);
    BAIL_ON_INVALID_POINTER(pValue);

    dwError = RegShellUtilEscapeString(
                  pszValueName,
                  &pszEscapedValueName,
                  &dwEscapedValueNameLen);
    BAIL_ON_REG_ERROR(dwError);

    pwszValue = (PWSTR) pValue;
    dwError = RegCStringAllocatePrintf(
                  &pszDumpString,
                  "\"%s\"=sza:",
                  pszEscapedValueName);
    BAIL_ON_REG_ERROR(dwError);

    while (*pwszValue)
    {
        dwError = RegCStringAllocateFromWC16String(
                      &pszString,
                      pwszValue);
        BAIL_ON_REG_ERROR(dwError);

        LWREG_SAFE_FREE_STRING(pszEscapedValue);
        dwError = RegShellUtilEscapeString(
                      pszString,
                      &pszEscapedValue,
                      &dwEscapedValueLen);
        BAIL_ON_REG_ERROR(dwError);


        dwError = RtlCStringAllocateAppendPrintf(
                      &pszDumpString,
                      "\"%s\"",
                      pszEscapedValue);
        BAIL_ON_REG_ERROR(dwError);
        LWREG_SAFE_FREE_STRING(pszString);

        dwPwszLen = wc16slen(pwszValue);
        pwszValue += dwPwszLen + 1;
        if (*pwszValue)
        {
            dwError = RtlCStringAllocateAppendPrintf(
                          &pszDumpString,
                          " \\\n    ");
            BAIL_ON_REG_ERROR(dwError);
        }
    }

cleanup:
    LWREG_SAFE_FREE_STRING(pszEscapedValue);
    LWREG_SAFE_FREE_STRING(pszEscapedValueName);
    *ppszDumpString = pszDumpString;
    *pdwDumpStringLen = strlen(pszDumpString);
    return dwError;
error:
    LWREG_SAFE_FREE_STRING(pszString);
    LWREG_SAFE_FREE_STRING(pszDumpString);
    goto cleanup;
}



DWORD
RegExportEntry(
    PCSTR keyName,
    PCSTR pszSddlCString,
    REG_DATA_TYPE valueType,
    PCSTR valueName,
    REG_DATA_TYPE type,
    LW_PVOID value,
    DWORD valueLen,
    PSTR *dumpString,
    PDWORD dumpStringLen
    )
{
    DWORD dwError = 0;
    PSTR keyString = NULL;
    PSTR valueNameString = NULL;
    PSTR valueString = NULL;

    if (valueType == REG_WSZ)
    {
        dwError = LwRtlCStringAllocateFromWC16String(
                      &valueNameString,
                      (PWSTR) valueName);
        if (dwError)
        {
            goto cleanup;
        }
        valueName = valueNameString;
    }

    switch (type)
    {
        case REG_BINARY:
        case REG_NONE:
        case REG_EXPAND_SZ:
        case REG_RESOURCE_LIST:
        case REG_FULL_RESOURCE_DESCRIPTOR:
        case REG_RESOURCE_REQUIREMENTS_LIST:
        case REG_QWORD:
            dwError = RegExportBinaryData(valueType,
                                          valueName,
                                          type,
                                          value,
                                          valueLen,
                                          dumpString,
                                          dumpStringLen);
            break;
        case REG_MULTI_SZ:
            dwError = RegExportMultiStringArray(
                          valueName,
                          value,
                          valueLen,
                          dumpString,
                          dumpStringLen);
            break;
        case REG_DWORD:
            dwError = RegExportDword(valueType,
                                     valueName,
                                     *((PDWORD) value),
                                     dumpString,
                                     dumpStringLen);
            break;

        case REG_KEY:
            dwError = RegExportRegKey(keyName,
                                      pszSddlCString,
                                      dumpString,
                                      dumpStringLen);
            break;

        /* Input values are PSTR, so convert to C string first */
        case REG_WKEY:
            dwError = LwRtlCStringAllocateFromWC16String(
                           &keyString,
                           (PWSTR) keyName);
            
            if (dwError)
            {
                break;
            }
            dwError = RegExportRegKey(
                          keyString,
                          pszSddlCString,
                          dumpString,
                          dumpStringLen);
            break;

        case REG_SZ:
            dwError = RegExportString(valueType,
                                      valueName,
                                      value,
                                      dumpString,
                                      dumpStringLen);
            break;

        case REG_WSZ:
            /* Input values are PSTR, so convert to C string first */
            dwError = LwRtlCStringAllocateFromWC16String(
                          &valueString,
                          value);
            if (dwError)
            {
                break;
            }
            dwError = RegExportString(valueType,
                                      valueName,
                                      valueString,
                                      dumpString,
                                      dumpStringLen);
            break;

        case REG_PLAIN_TEXT:
        default:
            dwError = RegExportPlainText((PCHAR) value,
                                        dumpString,
                                        dumpStringLen);
    }

cleanup:
    LWREG_SAFE_FREE_MEMORY(keyString);
    LWREG_SAFE_FREE_MEMORY(valueNameString);
    LWREG_SAFE_FREE_MEMORY(valueString);
    return dwError;
}


DWORD
RegExportDword(
    REG_DATA_TYPE valueType,
    PCSTR valueName,
    DWORD value,
    PSTR *dumpString,
    PDWORD dumpStringLen
    )
{
    DWORD bufLen = 0;
    PSTR dumpBuf = NULL;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(valueName);
    BAIL_ON_INVALID_POINTER(dumpString);
    BAIL_ON_INVALID_POINTER(dumpStringLen);

    /*
     *  "name"=1234ABCD\r\n\0
     *  14: *  ""=1234ABCD\r\n\0
     */
    bufLen = strlen(valueName) + 20;

    dwError = RegAllocateMemory(sizeof(*dumpBuf) * bufLen, (PVOID*)&dumpBuf);
    BAIL_ON_REG_ERROR(dwError);

    if (valueType == REG_KEY_DEFAULT)
    {
        *dumpStringLen = sprintf(dumpBuf, "@=dword:%08x",
                                 value);
    }
    else
    {
        *dumpStringLen = sprintf(dumpBuf, "\"%s\"=dword:%08x",
                                 valueName,
                                 value);
    }

    *dumpString = dumpBuf;
cleanup:
    return dwError;

error:
    goto cleanup;

}


DWORD
RegExportRegKey(
    PCSTR keyName,
    PCSTR pszSddlCString,
    PSTR *dumpString,
    PDWORD dumpStringLen
    )
{
    DWORD bufLen = 0;
    PSTR dumpBuf = NULL;
    DWORD dwError = 0;
    char szTmpSec[] = "\r\n@security = ";

    BAIL_ON_INVALID_POINTER(keyName);
    BAIL_ON_INVALID_POINTER(dumpString);
    BAIL_ON_INVALID_POINTER(dumpStringLen);

    /*
     *  [key_name]\r\n\0
     *  5:  []\r\n\0
     */
    bufLen = strlen(keyName) + 5 +(LwRtlCStringIsNullOrEmpty(pszSddlCString) ? 0 : (strlen(szTmpSec) + strlen(pszSddlCString)));


    dwError = RegAllocateMemory(sizeof(*dumpBuf) * bufLen, (PVOID*)&dumpBuf);
    BAIL_ON_REG_ERROR(dwError);

    *dumpStringLen = LwRtlCStringIsNullOrEmpty(pszSddlCString)
                   ? sprintf(dumpBuf, "[%s]", keyName)
                   : sprintf(dumpBuf, "[%s]\r\n@security = %s", keyName, pszSddlCString);
    *dumpString = dumpBuf;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegExportString(
    REG_DATA_TYPE valueType,
    PCSTR valueName,
    PCSTR value,
    PSTR *dumpString,
    PDWORD retDumpStringLen
    )
{
    DWORD bufLen = 0;
    PSTR dumpBuf = NULL;
    PSTR valueEsc = NULL;
    PSTR valueNameEsc = NULL;
    DWORD dwError = 0;
    DWORD dumpStringLen = 0;
    DWORD dwEscapeStringLen = 0;

    BAIL_ON_INVALID_POINTER(valueName);
    BAIL_ON_INVALID_POINTER(dumpString);
    BAIL_ON_INVALID_POINTER(retDumpStringLen);

    dwError = RegShellUtilEscapeString(
                  valueName,
                  &valueNameEsc,
                  &dwEscapeStringLen);
    BAIL_ON_REG_ERROR(dwError);
    dwError = RegShellUtilEscapeString(
                  value,
                  &valueEsc,
                  &dwEscapeStringLen);
    BAIL_ON_REG_ERROR(dwError);

    bufLen = strlen(valueName) + dwEscapeStringLen + 8;

    dwError = RegAllocateMemory(sizeof(*dumpBuf) * bufLen, (PVOID*)&dumpBuf);
    BAIL_ON_REG_ERROR(dwError);

    if (valueType == REG_KEY_DEFAULT)
    {
        dumpStringLen = sprintf(dumpBuf, "%s=\"%s\"",
                            valueNameEsc,
                            valueEsc);
    }
    else
    {
        dumpStringLen = sprintf(dumpBuf, "\"%s\"=\"%s\"",
                            valueName,
                            valueEsc);
    }
    LWREG_SAFE_FREE_MEMORY(valueEsc);
    LWREG_SAFE_FREE_MEMORY(valueNameEsc);
    *retDumpStringLen = dumpStringLen;
    *dumpString = dumpBuf;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegExportPlainText(
    PCHAR value,
    PSTR *dumpString,
    PDWORD dumpStringLen
    )
{
    DWORD bufLen = 0;
    PSTR dumpBuf = NULL;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(dumpString);
    BAIL_ON_INVALID_POINTER(dumpStringLen);

    bufLen = strlen(value) + 8;

    dwError = RegAllocateMemory(sizeof(*dumpBuf) * bufLen, (PVOID*)&dumpBuf);
    BAIL_ON_REG_ERROR(dwError);

    *dumpStringLen = sprintf(dumpBuf, "%s", (PCHAR) value);
    *dumpString = dumpBuf;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegExportBinaryData(
    REG_DATA_TYPE valueType,
    PCSTR valueName,
    REG_DATA_TYPE type,
    UCHAR *value,
    DWORD valueLen,
    PSTR *dumpString,
    PDWORD dumpStringLen
    )
{
    DWORD bufLen = 0;
    DWORD formatLines = 0;
    DWORD indx = 0;
    DWORD dwError = 0;
    DWORD linePos = 0;
    PSTR dumpBuf = NULL;
    PSTR fmtCursor = NULL;
    UCHAR *pValue = NULL;
    BOOLEAN firstHex = FALSE;

    CHAR typeName[128];

    if (type == 0 && valueLen == 0)
    {
        *dumpString = NULL;
        *dumpStringLen = 0;
        return 0;
    }

    RegExportBinaryTypeToString(type, typeName, TRUE);
    /* 5 extra for " "= \\n characters on first line */
    bufLen = strlen(valueName) + strlen(typeName) + 6;

    /* 4 extra characters per line: Prefix "  " spaces, suffix \ and \r\n */
    formatLines = valueLen / 25 + 1;
    bufLen += valueLen * 3 + (formatLines * 5) + 1;

    dwError = RegAllocateMemory(sizeof(*dumpBuf) * bufLen, (PVOID*)&dumpBuf);
    BAIL_ON_REG_ERROR(dwError);

    /* Format binary prefix */
    fmtCursor = dumpBuf;
    if (valueType == REG_KEY_DEFAULT)
    {
        fmtCursor += sprintf(fmtCursor, "@=%s", typeName);
    }
    else
    {
        fmtCursor += sprintf(fmtCursor, "\"%s\"=%s",
                             valueName, typeName);
    }

    pValue = (UCHAR *) value;
    linePos = fmtCursor - dumpBuf;
    indx = 0;
    while (indx < valueLen)
    {
        while(((linePos + 3)<REGEXPORT_LINE_WIDTH && indx<valueLen) ||
               !firstHex)
        {
            firstHex = TRUE;
            fmtCursor += sprintf(fmtCursor, "%02x,", pValue[indx]);
            linePos += 3;
            indx++;
        }
        if (indx < valueLen)
        {
            fmtCursor += sprintf(fmtCursor, "\\\r\n  ");
            linePos = 2;
        }
        else
        {
            fmtCursor[-1] = '\0';
            linePos = 0;
        }

    }

    *dumpString = dumpBuf;
    *dumpStringLen = fmtCursor - dumpBuf;

cleanup:
    return dwError;

error:
    goto cleanup;
}
