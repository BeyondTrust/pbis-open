/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        registry.c
 *
 * Abstract:
 *
 *        Registry logic
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

struct SM_LOGGER_MAP
{
    PCSTR pszName;
    LW_SM_LOG_LEVEL eValue;
};

static struct SM_LOGGER_MAP gLoggerMap[] =
{
    { "default", LW_SM_LOGGER_DEFAULT },
    { "none", LW_SM_LOGGER_NONE },
    { "file", LW_SM_LOGGER_FILE },
    { "syslog", LW_SM_LOGGER_SYSLOG}
};

struct SM_LOGLEVEL_MAP
{
    PCSTR pszName;
    LW_SM_LOG_LEVEL eValue;
};

static struct SM_LOGLEVEL_MAP gLogLevelMap[] =
{
    { "default", LW_SM_LOG_LEVEL_DEFAULT },
    { "always", LW_RTL_LOG_LEVEL_ALWAYS },
    { "error", LW_RTL_LOG_LEVEL_ERROR },
    { "warning", LW_RTL_LOG_LEVEL_WARNING },
    { "info", LW_RTL_LOG_LEVEL_INFO },
    { "verbose", LW_RTL_LOG_LEVEL_VERBOSE },
    { "trace", LW_RTL_LOG_LEVEL_TRACE }
};

static
DWORD
LwSmRegistryReadDword(
    HANDLE hReg,
    HKEY pRootKey,
    PCWSTR pwszParentKey,
    PCWSTR pwszValueName,
    PDWORD pdwValue
    );

static
DWORD
LwSmRegistryReadString(
    HANDLE hReg,
    HKEY pRootKey,
    PCWSTR pwszParentKey,
    PCWSTR pwszValueName,
    PWSTR* ppwszValue
    );

static
DWORD
LwSmRegistryReadStringList(
    HANDLE hReg,
    HKEY pRootKey,
    PCWSTR pwszParentKey,
    PCWSTR pwszValueName,
    PWSTR** pppwszValues
    );

static
size_t
LwSmCountTokens(
    PWSTR pwszString
    );

DWORD
LwSmRegistryEnumServices(
    HANDLE hReg,
    PWSTR** pppwszNames
    )
{
    DWORD dwError = 0;
    DWORD dwKeyCount = 0;
    DWORD dwMaxKeyLen = 0;
    DWORD dwKeyLen = 0;
    HKEY pRootKey = NULL;
    HKEY pParentKey = NULL;
    PWSTR* ppwszNames = NULL;
    PWSTR pwszParentPath = NULL;
    DWORD i = 0;

    static const PSTR pszServicesKeyName = "\\Services";

    dwError = LwMbsToWc16s(pszServicesKeyName + 1, &pwszParentPath);

    dwError = RegOpenKeyExA(
            hReg,
            NULL,
            HKEY_THIS_MACHINE,
            0,
            KEY_READ,
            &pRootKey);
    BAIL_ON_ERROR(dwError);

    dwError = RegOpenKeyExW(hReg, pRootKey, pwszParentPath, 0, KEY_READ, &pParentKey);
    BAIL_ON_ERROR(dwError);

    dwError = RegQueryInfoKeyW(
        hReg,
        pParentKey,
        NULL,
        NULL,
        NULL,
        &dwKeyCount,
        &dwMaxKeyLen,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);
    BAIL_ON_ERROR(dwError);

    dwError = LwAllocateMemory(
        sizeof(*ppwszNames) * (dwKeyCount + 1),
        OUT_PPVOID(&ppwszNames));
    BAIL_ON_ERROR(dwError);

    for (i = 0; i < dwKeyCount; i++)
    {
        dwKeyLen = dwMaxKeyLen + 1;

        dwError = LwAllocateMemory(
            sizeof(**ppwszNames) * dwKeyLen,
            OUT_PPVOID(&ppwszNames[i]));
        BAIL_ON_ERROR(dwError);

        dwError = RegEnumKeyExW(hReg, pParentKey, i, ppwszNames[i], &dwKeyLen, NULL, NULL, NULL, NULL);
        BAIL_ON_ERROR(dwError);
    }

    *pppwszNames = ppwszNames;

cleanup:

    LW_SAFE_FREE_MEMORY(pwszParentPath);

    if (pParentKey)
    {
        RegCloseKey(hReg, pParentKey);
    }

    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }

    return dwError;

error:

    *pppwszNames = NULL;

    if (ppwszNames)
    {
        LwSmFreeStringList(ppwszNames);
    }

    goto cleanup;
}

DWORD
LwSmRegistryReadServiceInfo(
    HANDLE hReg,
    PCWSTR pwszName,
    PLW_SERVICE_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    PLW_SERVICE_INFO pInfo = NULL;
    HKEY pRootKey = NULL;
    PWSTR pwszParentKey = NULL;
    DWORD dwType = 0;
    PSTR pszName = NULL;
    PSTR pszParentKey = NULL;
    DWORD dwAutostart = 0;

    PWSTR pwszDefaultLogType = NULL;
    PSTR pszDefaultLogType = NULL;
    PWSTR pwszDefaultLogLevel = NULL;
    PSTR pszDefaultLogLevel = NULL;
    size_t i;

    static const WCHAR wszDescription[] =
        {'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', 0};
    static const WCHAR wszType[] =
        {'T', 'y', 'p', 'e', 0};
    static const WCHAR wszPath[] =
        {'P', 'a', 't', 'h', 0};
    static const WCHAR wszArguments[] =
        {'A', 'r', 'g', 'u', 'm', 'e', 'n', 't', 's', 0};
    static const WCHAR wszDependencies[] =
        {'D', 'e', 'p', 'e', 'n', 'd', 'e', 'n', 'c', 'i', 'e', 's', 0};
    static const WCHAR wszEnvironment[] =
        {'E', 'n', 'v', 'i', 'r', 'o', 'n', 'm', 'e', 'n', 't', 0};
    static const WCHAR wszGroup[] =
        {'S', 'e', 'r', 'v', 'i', 'c', 'e', 'G', 'r', 'o', 'u', 'p', 0};
    static const WCHAR wszAutostart[] =
        {'A', 'u', 't', 'o', 's', 't', 'a', 'r', 't', 0};
    static const WCHAR wszFdLimit[] =
        {'F', 'd', 'L', 'i', 'm', 'i', 't', 0};
    static const WCHAR wszDefaultLogType[] =
        {'D', 'e', 'f', 'a', 'u', 'l', 't', 'L', 'o', 'g', 'T', 'y', 'p', 'e', 0};
    static const WCHAR wszDefaultLogTarget[] =
            {'D', 'e', 'f', 'a', 'u', 'l', 't', 'L', 'o', 'g', 'T', 'a', 'r', 'g', 'e', 't', 0};
    static const WCHAR wszDefaultLogLevel[] =
            {'D', 'e', 'f', 'a', 'u', 'l', 't', 'L', 'o', 'g', 'L', 'e', 'v', 'e', 'l', 0};
    static const WCHAR wszCoreSize[] =
        {'C', 'o', 'r', 'e', 'S', 'i', 'z', 'e', 0};

    dwError = LwWc16sToMbs(pwszName, &pszName);
    BAIL_ON_ERROR(dwError);

    dwError = LwAllocateStringPrintf(&pszParentKey, "Services\\%s", pszName);
    BAIL_ON_ERROR(dwError);

    dwError = LwMbsToWc16s(pszParentKey, &pwszParentKey);
    BAIL_ON_ERROR(dwError);

    dwError = RegOpenKeyExA(
            hReg,
            NULL,
            HKEY_THIS_MACHINE,
            0,
            KEY_READ,
            &pRootKey);
    BAIL_ON_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pInfo), OUT_PPVOID(&pInfo));
    BAIL_ON_ERROR(dwError);

    dwError = LwSmRegistryReadDword(
        hReg,
        pRootKey,
        pwszParentKey,
        wszType,
        &dwType);
    BAIL_ON_ERROR(dwError);

    pInfo->type = dwType;

    dwError = LwAllocateWc16String(&pInfo->pwszName, pwszName);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmRegistryReadString(
        hReg,
        pRootKey,
        pwszParentKey,
        wszDescription,
        &pInfo->pwszDescription);
    BAIL_ON_ERROR(dwError);

   dwError = LwSmRegistryReadString(
        hReg,
        pRootKey,
        pwszParentKey,
        wszPath,
        &pInfo->pwszPath);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmRegistryReadStringList(
        hReg,
        pRootKey,
        pwszParentKey,
        wszArguments,
        &pInfo->ppwszArgs);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmRegistryReadStringList(
        hReg,
        pRootKey,
        pwszParentKey,
        wszEnvironment,
        &pInfo->ppwszEnv);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmRegistryReadStringList(
        hReg,
        pRootKey,
        pwszParentKey,
        wszDependencies,
        &pInfo->ppwszDependencies);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmRegistryReadDword(
        hReg,
        pRootKey,
        pwszParentKey,
        wszAutostart,
        &dwAutostart);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = 0;
        dwAutostart = FALSE;
    }
    BAIL_ON_ERROR(dwError);

    pInfo->bAutostart = dwAutostart ? TRUE : FALSE;

    dwError = LwSmRegistryReadDword(
        hReg,
        pRootKey,
        pwszParentKey,
        wszFdLimit,
        &pInfo->dwFdLimit);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = 0;
        pInfo->dwFdLimit = 0;
    }
    BAIL_ON_ERROR(dwError);

    dwError = LwSmRegistryReadDword(
        hReg,
        pRootKey,
        pwszParentKey,
        wszCoreSize,
        &pInfo->dwCoreSize);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = 0;
        pInfo->dwCoreSize = 0;
    }
    BAIL_ON_ERROR(dwError);

    dwError = LwSmRegistryReadString(
        hReg,
        pRootKey,
        pwszParentKey,
        wszGroup,
        &pInfo->pwszGroup);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
#ifdef SERVICE_DIRECT
        dwError = LwMbsToWc16s("direct", &pInfo->pwszGroup);
#else
        dwError = LwAllocateWc16String(&pInfo->pwszGroup, pInfo->pwszName);
#endif
    }
    BAIL_ON_ERROR(dwError);

    dwError = LwSmRegistryReadString(
        hReg,
        pRootKey,
        pwszParentKey,
        wszDefaultLogType,
        &pwszDefaultLogType);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = 0;
        pInfo->DefaultLogType = LW_SM_LOGGER_DEFAULT;
    }
    else if (dwError == 0)
    {
        dwError = LwWc16sToMbs(pwszDefaultLogType, &pszDefaultLogType);
        BAIL_ON_ERROR(dwError);

        for (i = 0; i < sizeof(gLoggerMap)/sizeof(gLoggerMap[0]); i++)
        {
            if (!strcmp(gLoggerMap[i].pszName, pszDefaultLogType))
            {
                pInfo->DefaultLogType = gLoggerMap[i].eValue;
                break;
            }
        }
        if (i == sizeof(gLoggerMap)/sizeof(gLoggerMap[0]))
        {
            SM_LOG_WARNING("Invalid value in registry for LogType");
            pInfo->DefaultLogType = LW_SM_LOGGER_DEFAULT;
        }
    }
    BAIL_ON_ERROR(dwError);

    dwError = LwSmRegistryReadString(
        hReg,
        pRootKey,
        pwszParentKey,
        wszDefaultLogLevel,
        &pwszDefaultLogLevel);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = 0;
        pInfo->DefaultLogLevel = 0;
    }
    else if (dwError == 0)
    {
        dwError = LwWc16sToMbs(pwszDefaultLogLevel, &pszDefaultLogLevel);
        BAIL_ON_ERROR(dwError);

        for (i = 0; i < sizeof(gLogLevelMap)/sizeof(gLogLevelMap[0]); i++)
        {
            if (!strcmp(gLogLevelMap[i].pszName, pszDefaultLogType))
            {
                pInfo->DefaultLogType = gLogLevelMap[i].eValue;
                break;
            }
        }
        if (i == sizeof(gLogLevelMap)/sizeof(gLogLevelMap[0]))
        {
            SM_LOG_WARNING("Invalid value in registry for LogType");
            pInfo->DefaultLogType = LW_SM_LOGGER_DEFAULT;
        }
    }
    BAIL_ON_ERROR(dwError);


    dwError = LwSmRegistryReadString(
        hReg,
        pRootKey,
        pwszParentKey,
        wszDefaultLogTarget,
        &pInfo->pDefaultLogTarget);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        switch (pInfo->DefaultLogType)
        {
        case LW_SM_LOGGER_SYSLOG:
            dwError = LwMbsToWc16s("LOG_DAEMON", &pInfo->pDefaultLogTarget);
            break;
        default:
            dwError = 0;
            break;
        }
    }
    BAIL_ON_ERROR(dwError);

    *ppInfo = pInfo;

cleanup:

    LW_SAFE_FREE_MEMORY(pszName);
    LW_SAFE_FREE_MEMORY(pszParentKey);
    LW_SAFE_FREE_MEMORY(pwszParentKey);

    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }

    return dwError;

error:

    *ppInfo = NULL;

    if (pInfo)
    {
        LwSmCommonFreeServiceInfo(pInfo);
    }

    goto cleanup;
}

static
DWORD
LwSmRegistryReadDword(
    HANDLE hReg,
    HKEY pRootKey,
    PCWSTR pwszParentKey,
    PCWSTR pwszValueName,
    PDWORD pdwValue
    )
{
    DWORD dwError = 0;
    DWORD dwSize = sizeof(*pdwValue) + 1;
    DWORD dwType = 0;

    dwError = RegGetValueW(
        hReg,
        pRootKey,
        pwszParentKey,
        pwszValueName,
        RRF_RT_REG_DWORD,
        &dwType,
        (PBYTE) pdwValue,
        &dwSize);
    BAIL_ON_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmRegistryReadString(
    HANDLE hReg,
    HKEY pRootKey,
    PCWSTR pwszParentKey,
    PCWSTR pwszValueName,
    PWSTR* ppwszValue
    )
{
    DWORD dwError = 0;
    WCHAR wszValue[MAX_VALUE_LENGTH];
    DWORD dwSize = 0;
    DWORD dwType = 0;
    PWSTR pwszValue = NULL;

    memset(wszValue, 0, sizeof(wszValue));
    dwSize = sizeof(wszValue);

    dwError = RegGetValueW(
        hReg,
        pRootKey,
        pwszParentKey,
        pwszValueName,
        RRF_RT_REG_SZ,
        &dwType,
        wszValue,
        &dwSize);
    BAIL_ON_ERROR(dwError);

    dwError = LwAllocateMemory(dwSize + sizeof(WCHAR), OUT_PPVOID(&pwszValue));
    BAIL_ON_ERROR(dwError);

    memcpy(pwszValue, wszValue, dwSize);
    *ppwszValue = pwszValue;

cleanup:

    return dwError;

error:

    *ppwszValue = NULL;

    goto cleanup;
}

static
DWORD
LwSmRegistryReadStringList(
    HANDLE hReg,
    HKEY pRootKey,
    PCWSTR pwszParentKey,
    PCWSTR pwszValueName,
    PWSTR** pppwszValues
    )
{
    DWORD dwError = 0;
    PWSTR pwszValuesString = NULL;
    PWSTR pwszCursor = NULL;
    PWSTR pwszSpace = NULL;
    PWSTR* ppwszValues = NULL;
    size_t count = 0;
    size_t i = 0;

    dwError = LwSmRegistryReadString(hReg, pRootKey, pwszParentKey, pwszValueName, &pwszValuesString);
    BAIL_ON_ERROR(dwError);

    if (*pwszValuesString)
    {
        count = LwSmCountTokens(pwszValuesString);
    }
    else
    {
        count = 0;
    }

    dwError = LwAllocateMemory((count + 1) * sizeof(*ppwszValues), OUT_PPVOID(&ppwszValues));
    BAIL_ON_ERROR(dwError);

    for (pwszCursor = pwszValuesString, i = 0; i < count; i++)
    {
        for (pwszSpace = pwszCursor; *pwszSpace && *pwszSpace != ' '; pwszSpace++);

        *pwszSpace = (WCHAR) '\0';

        dwError = LwAllocateWc16String(&ppwszValues[i], pwszCursor);
        BAIL_ON_ERROR(dwError);

        for (pwszCursor = pwszSpace + 1; *pwszCursor == ' '; pwszCursor++);
    }

    *pppwszValues = ppwszValues;

cleanup:

    LW_SAFE_FREE_MEMORY(pwszValuesString);

    return dwError;

error:

    *pppwszValues = NULL;

    if (ppwszValues)
    {
        LwSmFreeStringList(ppwszValues);
    }

    goto cleanup;
}

static
size_t
LwSmCountTokens(
    PWSTR pwszString
    )
{
    PWSTR pwszCursor = NULL;
    size_t count = 0;
    BOOLEAN inSpace = TRUE;

    for (pwszCursor = pwszString; *pwszCursor; pwszCursor++)
    {
        if (*pwszCursor == (WCHAR) ' ')
        {
            inSpace = TRUE;
        }
        else
        {
            if (inSpace)
            {
                count++;
            }
            inSpace = FALSE;
        }
    }

    return count;
}
