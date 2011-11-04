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

#include "includes.h"

DWORD
SMBAllocateStringW(
    PWSTR  pwszInputString,
    PWSTR* ppwszOutputString
    )
{
    DWORD dwError = 0;
    size_t sLen = 0;
    PWSTR  pwszOutputString = NULL;

    if (!pwszInputString) {
        dwError = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    sLen = wc16slen(pwszInputString);

    dwError = LwIoAllocateMemory(
                    (sLen + 1 ) * sizeof(wchar16_t),
                    (PVOID *)&pwszOutputString);
    BAIL_ON_LWIO_ERROR(dwError);

    if (sLen)
    {
       memcpy((PBYTE)pwszOutputString, (PBYTE)pwszInputString, sLen * sizeof(wchar16_t));
    }

    *ppwszOutputString = pwszOutputString;

cleanup:

    return dwError;

error:

    LWIO_SAFE_FREE_MEMORY(pwszOutputString);

    *ppwszOutputString = NULL;

    goto cleanup;
}

DWORD
SMBMbsToWc16s(
    PCSTR     pszInput,
    PWSTR* ppwszOutput
    )
{
    DWORD    dwError = 0;
    PWSTR pwszOutput = NULL;

    if (!pszInput) {
        dwError = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    pwszOutput = ambstowc16s(pszInput);
    if (!pwszOutput) {
        dwError = LWIO_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    *ppwszOutput = pwszOutput;

cleanup:

    return dwError;

error:

    *ppwszOutput = NULL;

    goto cleanup;
}

DWORD
SMBWc16snToMbs(
    PCWSTR pwszInput,
    size_t    sMaxChars,
    PSTR*     ppszOutput
    )
{
    DWORD dwError = 0;
    PWSTR pwszTruncated = NULL;
    PSTR pszOutput = NULL;

    if (!pwszInput) {
        dwError = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    pwszTruncated = _wc16sndup(pwszInput, sMaxChars);
    if (!pwszTruncated) {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    pszOutput = awc16stombs(pwszTruncated);
    if (!pszOutput) {
        dwError = LWIO_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    *ppszOutput = pszOutput;

cleanup:

    LWIO_SAFE_FREE_MEMORY(pwszTruncated);

    return dwError;

error:

    *ppszOutput = NULL;

    goto cleanup;
}

DWORD
SMBWc16sToMbs(
    PCWSTR pwszInput,
    PSTR*     ppszOutput
    )
{
    DWORD dwError = 0;
    PSTR pszOutput = NULL;

    if (!pwszInput) {
        dwError = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    pszOutput = awc16stombs(pwszInput);
    if (!pszOutput) {
        dwError = LWIO_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    *ppszOutput = pszOutput;

cleanup:

    return dwError;

error:

    *ppszOutput = NULL;

    goto cleanup;
}

DWORD
SMBWc16sLen(
    PCWSTR  pwszInput,
    size_t*    psLen
    )
{
    DWORD dwError = 0;
    size_t sLen = 0;

    if (!pwszInput) {
        dwError = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    sLen = wc16slen(pwszInput);

    *psLen = sLen;

cleanup:

    return dwError;

error:

    *psLen = 0;

    goto cleanup;
}

int
SMBWc16sCmp(
    PCWSTR  pwszFirst,
    PCWSTR  pwszSecond
    )
{
    return wc16scmp(pwszFirst, pwszSecond);
}

int
SMBWc16snCmp(
    PCWSTR pwszFirst,
    PCWSTR pwszSecond,
    size_t sLen
    )
{
    return wc16sncmp(pwszFirst, pwszSecond, sLen);
}

int
SMBWc16sCaseCmp(
    PCWSTR  pwszFirst,
    PCWSTR  pwszSecond
    )
{
    return wc16scasecmp(pwszFirst, pwszSecond);
}

DWORD
SMBWc16sDup(
    PCWSTR pwszInput,
    PWSTR* pwszOutput
    )
{
    *pwszOutput = wc16sdup(pwszInput);

    if (!*pwszOutput)
    {
        return LWIO_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        return 0;
    }
}

