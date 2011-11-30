/*
* Copyright (c) Likewise Software.  All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at
* your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
* for more details.  You should have received a copy of the GNU General
* Public License along with this program.  If not, see
* <http://www.gnu.org/licenses/>.
*
* LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
* TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
* WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
* TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
* GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
* HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
* TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
* license@likewise.com
*/

#include "includes.h"

DWORD
UtilAllocateMultistring(
    PCSTR *ppszValues,
    DWORD dwValues,
    PSTR *ppszValue
    )
{
    DWORD dwError = 0;
    DWORD dwLength = 0;
    PSTR pszValue = NULL;
    size_t i;

    if (dwValues)
    {
        for (i = 0; i < dwValues; i++)
        {
            dwLength += strlen(ppszValues[i]) + 1;
        }
        dwLength++;


        dwError = LwAllocateMemory(dwLength, (PVOID*)&pszValue);
        BAIL_ON_ERROR(dwError);

        dwLength = 0;
        for (i = 0; i < dwValues; i++)
        {
            strcpy(pszValue + dwLength, ppszValues[i]);
            dwLength += strlen(ppszValues[i]) + 1;
        }
        pszValue[dwLength++] = '\0';
    }

    *ppszValue = pszValue;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pszValue);
    goto cleanup;
}

DWORD
UtilMultistringLength(
    PCSTR pszValue
    )
{
    PCSTR pszEnd = NULL;

    pszEnd = pszValue;
    while (*pszEnd)
    {
        pszEnd = pszEnd + strlen(pszEnd) + 1;
    }

    return (pszEnd - pszValue) + 1;
}

DWORD
UtilDuplicateMultistring(
    PCSTR pszValue,
    PSTR *ppszValue
    )
{
    DWORD dwError = 0;
    DWORD dwLength = 0;
    PSTR pszOut = NULL;

    dwLength = UtilMultistringLength(pszValue);

    dwError = LwAllocateMemory(dwLength, (PVOID*)&pszOut);
    BAIL_ON_ERROR(dwError);

    memcpy(pszOut, pszValue, dwLength);

    *ppszValue = pszOut;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pszOut);
    goto cleanup;
}

BOOLEAN
UtilMultistringAreEqual(
    PCSTR pszValue1,
    PCSTR pszValue2
    )
{
    if (!pszValue1 && !pszValue2)
    {
        /* Neither exists */
        return TRUE;
    }

    if (!pszValue1 || !pszValue2)
    {
        /* Only one exists */
        return FALSE;
    }

    while (*pszValue1 && *pszValue2)
    {
        if (strcmp(pszValue1, pszValue2))
        {
            return FALSE;
        }

        pszValue1 = pszValue1 + strlen(pszValue1) + 1;
        pszValue2 = pszValue2 + strlen(pszValue2) + 1;
    }

    if (!*pszValue1 && !*pszValue2)
    {
        /* Both are empty -- so each piece up to this was the same.*/
        return TRUE;
    }
    else
    {
        /* Only one is empty -- one string is shorter than the other*/
        return FALSE;
    }

    return FALSE;
}

DWORD
UtilParseRegName(
    PCSTR pszPath,
    PSTR *ppszRoot,
    PSTR *ppszKey,
    PSTR *ppszName
    )
{
    DWORD dwError = 0;
    PSTR pszRoot = NULL;
    PSTR pszKey = NULL;
    PSTR pszName = NULL;
    PCSTR pszFirst = NULL;
    PCSTR pszLast = NULL;
    PCSTR pszEnd = NULL;

    if (!pszPath)
    {
        dwError = APP_ERROR_BAD_REGISTRY_PATH;
        BAIL_ON_ERROR(dwError);
    }

    // Skip past a leading /
    if (pszPath[0] == '\\')
        pszPath++;

    // Find the end of the string (before the terminator).
    pszEnd = pszPath + strlen(pszPath) - 1;

    pszFirst = strchr(pszPath, '\\');
    if (!pszFirst)
    {
        dwError = APP_ERROR_BAD_REGISTRY_PATH;
        BAIL_ON_ERROR(dwError);
    }

    pszLast = strrchr(pszPath, '\\');

    dwError = LwAllocateMemory(pszFirst - pszPath + 1, (PVOID*) &pszRoot);
    BAIL_ON_ERROR(dwError);

    memcpy(pszRoot, pszPath, pszFirst - pszPath);
    pszRoot[pszFirst - pszPath] = '\0';

    if (pszFirst != pszLast)
    {
        dwError = LwAllocateMemory(pszLast - pszFirst + 1, (PVOID*) &pszKey);
        BAIL_ON_ERROR(dwError);

        memcpy(pszKey, pszFirst + 1, (pszLast - pszFirst) - 1);
        pszKey[pszLast - pszFirst] = '\0';
    }

    dwError = LwAllocateMemory(pszEnd - pszLast + 1, (PVOID*) &pszName);
    BAIL_ON_ERROR(dwError);

    memcpy(pszName, pszLast + 1, pszEnd - pszLast);
    pszName[pszEnd - pszLast] = '\0';

    *ppszRoot = pszRoot;
    *ppszKey = pszKey;
    *ppszName = pszName;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszRoot);
    LW_SAFE_FREE_STRING(pszKey);
    LW_SAFE_FREE_STRING(pszName);
    goto cleanup;
}

DWORD
UtilSetValueExA(
    PCSTR pszRoot,
    PCSTR pszKey,
    PCSTR pszValueName,
    DWORD dwType,
    const BYTE *pData,
    DWORD cbData
    )
{
    DWORD dwError = 0;
    HANDLE hReg = NULL;
    HKEY hRootKey = NULL;
    HKEY hKeyKey = NULL;

    dwError = LwRegOpenServer(&hReg);
    BAIL_ON_ERROR(dwError);

    if (!strcmp(pszRoot, "HKEY_THIS_MACHINE"))
    {
        dwError = LwRegOpenKeyExA(hReg, NULL, HKEY_THIS_MACHINE, 0, KEY_WRITE, &hRootKey);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    if (pszKey && pszKey[0])
    {
        dwError = LwRegOpenKeyExA(hReg, hRootKey, pszKey, 0, KEY_WRITE,
                &hKeyKey);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        hKeyKey = hRootKey;
        hRootKey = NULL;
    }

    dwError =  LwRegSetValueExA(hReg, hKeyKey, pszValueName, 0, dwType, pData, cbData);
    BAIL_ON_ERROR(dwError);

cleanup:
    if (hKeyKey)
    {
        LwRegCloseKey(hReg, hKeyKey);
        hKeyKey = NULL;
    }

    if (hRootKey)
    {
        LwRegCloseKey(hReg, hRootKey);
        hRootKey = NULL;
    }

    if (hReg)
    {
        LwRegCloseServer(hReg);
        hReg = NULL;
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
UtilGetValueExA(
    PCSTR pszRoot,
    PCSTR pszKey,
    PCSTR pszValueName,
    DWORD dwType,
    PVOID *ppvData,
    PDWORD pcbData
    )
{
    DWORD dwError = 0;
    DWORD dwActualType = 0;
    PSTR pszValue = NULL;
    char szValue[MAX_VALUE_LENGTH];
    DWORD cbData = sizeof(szValue);
    HANDLE hReg = NULL;
    HKEY hRootKey = NULL;
    HKEY hKeyKey = NULL;

    dwError = LwRegOpenServer(&hReg);
    BAIL_ON_ERROR(dwError);

    if (!strcmp(pszRoot, "HKEY_THIS_MACHINE"))
    {
        dwError = LwRegOpenKeyExA(hReg, NULL, HKEY_THIS_MACHINE, 0, KEY_READ, &hRootKey);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    if (pszKey && pszKey[0])
    {
        dwError = LwRegOpenKeyExA(hReg, hRootKey, pszKey, 0, KEY_READ,
                &hKeyKey);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        hKeyKey = hRootKey;
        hRootKey = NULL;
    }

    dwError =  LwRegQueryValueExA(hReg, hKeyKey, pszValueName, 0, &dwActualType,
            (PBYTE)szValue, &cbData);
    BAIL_ON_ERROR(dwError);

    if (dwActualType != dwType)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    if (dwType == REG_DWORD && cbData == sizeof(REG_DWORD))
    {
        memcpy(ppvData, szValue, sizeof(REG_DWORD));
        *pcbData = sizeof(REG_DWORD);
    }
    else if (dwType == REG_SZ)
    {
       dwError = LwAllocateString(szValue, &pszValue);
       BAIL_ON_ERROR(dwError);

       *ppvData = pszValue;
       *pcbData = cbData;
    }
    else if (dwType == REG_MULTI_SZ)
    {
        dwError = LwAllocateMemory(cbData, (PVOID*)&pszValue);
        BAIL_ON_ERROR(dwError);

        memcpy(pszValue, szValue, cbData);

        *ppvData = pszValue;
        *pcbData = cbData;
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }


cleanup:
    if (hKeyKey)
    {
        LwRegCloseKey(hReg, hKeyKey);
        hKeyKey = NULL;
    }

    if (hRootKey)
    {
        LwRegCloseKey(hReg, hRootKey);
        hRootKey = NULL;
    }

    if (hReg)
    {
        LwRegCloseServer(hReg);
        hReg = NULL;
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pszValue);
    goto cleanup;
}

static
DWORD
AppendCharacter(int c, PSTR *ppszLine, PDWORD pdwMaxSize)
{
    DWORD dwError = 0;
    PSTR pszLine = NULL;
    DWORD dwMaxSize = 0;
    size_t i = 0;

    if (*ppszLine)
        for (i = 0; (*ppszLine)[i]; i++)
            ;

    if (!*ppszLine || i + 1 == *pdwMaxSize)
    {
        dwMaxSize = *pdwMaxSize + 16; // Not doing exponential growth
        dwError = LwAllocateMemory(sizeof(char) * dwMaxSize, (PVOID*)&pszLine);
        BAIL_ON_ERROR(dwError);

        memcpy(pszLine, *ppszLine, *pdwMaxSize);

        LW_SAFE_FREE_STRING(*ppszLine);
        *ppszLine = pszLine;
        pszLine = NULL;
        *pdwMaxSize = dwMaxSize;
    }
    (*ppszLine)[i] = c;
    (*ppszLine)[i+1] = '\0';

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszLine);
    goto cleanup;
}

static
DWORD
AppendArgument(PSTR pszLine, PSTR **pppszArgs, PDWORD pdwArgs)
{
    DWORD dwError = 0;
    PSTR pszNewLine = NULL;
    PSTR *ppszArgs = NULL;
    DWORD dwArgs = 0;

    dwError = LwAllocateString(pszLine, &pszNewLine);
    BAIL_ON_ERROR(dwError);

    dwArgs = *pdwArgs + 1;
    dwError = LwAllocateMemory(sizeof(PSTR) * dwArgs, (PVOID*)&ppszArgs);
    BAIL_ON_ERROR(dwError);

    memcpy(ppszArgs, *pppszArgs, *pdwArgs * sizeof(*ppszArgs));
    ppszArgs[*pdwArgs] = pszNewLine;
    pszNewLine = NULL;

    LW_SAFE_FREE_MEMORY(*pppszArgs);
    *pppszArgs = ppszArgs;
    ppszArgs = NULL;

    *pdwArgs = dwArgs;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszNewLine);
    goto cleanup;
}

DWORD
UtilParseLine(
    PCSTR pszLine,
    PSTR **pppszArgs,
    PDWORD pdwArgs
    )
{
    DWORD dwError = 0;
    PSTR pszArg = NULL;
    DWORD dwArgMaxSize = 0;
    PSTR *ppszArgs = NULL;
    DWORD dwArgs = 0;
    BOOLEAN bArgument = FALSE;
    BOOLEAN bQuoted = FALSE;
    BOOLEAN bEscaped = FALSE;
    size_t i = 0;

    dwArgMaxSize = 64;
    dwError = LwAllocateMemory(sizeof(char) * dwArgMaxSize, (PVOID*)&pszArg);
    BAIL_ON_ERROR(dwError);

    for (i = 0; pszLine[i] && pszLine[i] != '\n' && pszLine[i] != '\r'; i++)
    {
        int c = pszLine[i];

        if (!bArgument)
        {
            if (c == ' ' || c == '\t')
                continue;
        }
        bArgument = TRUE;

        if (bEscaped)
        {
            dwError = AppendCharacter(c, &pszArg, &dwArgMaxSize);
            BAIL_ON_ERROR(dwError);
            bEscaped = FALSE;
        }
        else if (c == '\\')
        {
            bEscaped = TRUE;
        }
        else if (bQuoted)
        {
            if (c == '"')
            {
                bQuoted = FALSE;
                bArgument = FALSE;

                // Terminate argument
                dwError = AppendArgument(pszArg, &ppszArgs, &dwArgs);
                BAIL_ON_ERROR(dwError);
                pszArg[0] = '\0';
            }
            else
            {
                dwError = AppendCharacter(c, &pszArg, &dwArgMaxSize);
                BAIL_ON_ERROR(dwError);
            }
        }
        else if (c == '"')
        {
            bQuoted = TRUE;

            // Terminate previous argument if there was one.
            if (pszArg && pszArg[0])
            {
                dwError = AppendArgument(pszArg, &ppszArgs, &dwArgs);
                BAIL_ON_ERROR(dwError);
                pszArg[0] = '\0';
            }
        }
        else if (c == ' ' || c == '\t')
        {
            bArgument = FALSE;

            // Terminate previous argument
            dwError = AppendArgument(pszArg, &ppszArgs, &dwArgs);
            BAIL_ON_ERROR(dwError);
            pszArg[0] = '\0';
        }
        else
        {
            dwError = AppendCharacter(c, &pszArg, &dwArgMaxSize);
            BAIL_ON_ERROR(dwError);
        }
    }

    if (bEscaped)
    {
        dwError = APP_ERROR_INVALID_ESCAPE_SEQUENCE;
        BAIL_ON_ERROR(dwError);
    }
    if (bQuoted)
    {
        dwError = APP_ERROR_UNTERMINATED_QUOTE;
        BAIL_ON_ERROR(dwError);
    }
    if (pszArg[0])
    {
        dwError = AppendArgument(pszArg, &ppszArgs, &dwArgs);
        BAIL_ON_ERROR(dwError);
    }

    *pppszArgs = ppszArgs;
    *pdwArgs = dwArgs;

cleanup:
    LW_SAFE_FREE_STRING(pszArg);
    return dwError;

error:

    LW_SAFE_FREE_STRING(pszArg);
    for (i = 0; i < dwArgs; i++)
    {
        LW_SAFE_FREE_STRING(ppszArgs[i]);
    }
    LW_SAFE_FREE_MEMORY(ppszArgs);
    goto cleanup;
}

DWORD
UtilReadLine(
    FILE* pStream,
    PSTR* ppszLine
    )
{
    DWORD dwError = 0;
    ssize_t sSize = 0, sCapacity = 0;
    PSTR pszBuffer = NULL;
    // Do not free
    PSTR pszNewBuffer = NULL;

    do
    {
        // There is not enough space. Allocate a larger buffer
        sCapacity = sSize*2 + 10;
        dwError = LwReallocMemory(
                            pszBuffer,
                            OUT_PPVOID(&pszNewBuffer),
                            sCapacity + 1);
        BAIL_ON_ERROR(dwError);
        pszBuffer = pszNewBuffer;

        // Read as much as the stream will give us up to the space in the
        // buffer.
        errno = 0;
        if (fgets(pszBuffer + sSize, sCapacity - sSize, pStream) == NULL)
        {
            dwError = LwMapErrnoToLwError(errno);
            if (dwError == 0)
            {
                dwError = ERROR_HANDLE_EOF;
            }
            BAIL_ON_ERROR(dwError);
        }

        sSize += strlen(pszBuffer + sSize);
    }
    // While the whole buffer is used and it does not end in a newline
    while(sSize == sCapacity - 1 && pszBuffer[sSize-1] != '\n');

    if (sSize == 0)
    {
        dwError = ERROR_HANDLE_EOF;
        BAIL_ON_ERROR(dwError);
    }
    if (pszBuffer[sSize-1] == '\n')
        pszBuffer[sSize-1] = '\0';

    *ppszLine = pszBuffer;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszBuffer);
    goto cleanup;
}

DWORD
UtilAllocateEscapedString(
    PCSTR pszStr,
    PSTR *ppszEscapedStr
    )
{
    DWORD dwError = 0;
    const PCSTR pszEscapeChars = "\\\"";
    DWORD i, j;
    PSTR pszEscapedStr = NULL;
    DWORD dwCount = 0;

    for (i = 0; pszStr[i]; i++)
    {
        if (strchr(pszEscapeChars, pszStr[i]) != NULL)
            dwCount++;
        dwCount++;
    }
    dwCount++;

    dwError = LwAllocateMemory(dwCount, (PVOID)&pszEscapedStr);
    BAIL_ON_ERROR(dwError);

    for (i = 0, j = 0; pszStr[i]; i++)
    {
        if (strchr(pszEscapeChars, pszStr[i]) != NULL)
        {
            pszEscapedStr[j++] = '\\';
        }
        pszEscapedStr[j++] = pszStr[i];
    }
    pszEscapedStr[j] = '\0';

    *ppszEscapedStr = pszEscapedStr;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pszEscapedStr);
    goto cleanup;
}
