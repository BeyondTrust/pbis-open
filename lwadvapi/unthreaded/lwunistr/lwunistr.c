/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
LwMbsToWc16s(
    PCSTR pszInput,
    PWSTR* ppszOutput
    )
{
    DWORD dwError = 0;
    PWSTR pszOutput = NULL;

    if (!pszInput) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    pszOutput = ambstowc16s(pszInput);
    if (!pszOutput) {
        dwError = LW_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LW_ERROR(dwError);
    }

    *ppszOutput = pszOutput;

cleanup:

    return dwError;

error:

    *ppszOutput = NULL;

    goto cleanup;
}

DWORD
LwWc16snToMbs(
    PCWSTR pwszInput,
    PSTR*  ppszOutput,
    size_t sMaxChars
    )
{
    DWORD dwError = 0;
    PWSTR pwszTruncated = NULL;
    PSTR pszOutput = NULL;

    if (!pwszInput) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    pwszTruncated = _wc16sndup(pwszInput, sMaxChars);
    if (!pwszTruncated) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    pszOutput = awc16stombs(pwszTruncated);
    if (!pszOutput) {
        dwError = LW_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LW_ERROR(dwError);
    }

    *ppszOutput = pszOutput;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszTruncated);

    return dwError;

error:

    *ppszOutput = NULL;

    goto cleanup;
}

DWORD
LwWc16sToMbs(
    PCWSTR pwszInput,
    PSTR*  ppszOutput
    )
{
    DWORD dwError = 0;
    PSTR pszOutput = NULL;

    if (!pwszInput) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    pszOutput = awc16stombs(pwszInput);
    if (!pszOutput) {
        dwError = LW_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LW_ERROR(dwError);
    }

    *ppszOutput = pszOutput;

cleanup:

    return dwError;

error:

    *ppszOutput = NULL;

    goto cleanup;
}

DWORD
LwWc16sLen(
    PCWSTR  pwszInput,
    size_t* psLen
    )
{
    DWORD dwError = 0;
    size_t sLen = 0;

    if (!pwszInput) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    sLen = wc16slen(pwszInput);

    *psLen = sLen;

cleanup:

    return dwError;

error:

    *psLen = 0;

    goto cleanup;
}


DWORD
LwWc16sCpy(
    PWSTR  pwszOutputString,
    PCWSTR pwszInputString
    )
{
    DWORD dwError = 0;

    if (!pwszInputString ||
        !pwszOutputString)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    wc16scpy(pwszOutputString, pwszInputString);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LwWc16snCpy(
    PWSTR  pwszOutputString,
    PCWSTR pwszInputString,
    DWORD  dwLen
    )
{
    DWORD dwError = 0;
    size_t sLen = (size_t)dwLen;

    if (!pwszInputString ||
        !pwszOutputString)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    if (sLen)
    {
        wc16sncpy(pwszOutputString, pwszInputString, sLen);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LwSW16printf(
    PWSTR*  ppwszStrOutput,
    PCSTR   pszFormat,
    ...)
{
    DWORD dwError = 0;
    INT ret = 0;
    PWSTR pwszStrOutput = NULL;
    va_list args;

    va_start(args, pszFormat);

    ret = sw16printf(
                  pwszStrOutput,
                  pszFormat,
                  args);

    if (ret == -1){
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    *ppwszStrOutput = pwszStrOutput;

cleanup:

    va_end(args);

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszStrOutput);

    goto cleanup;

}

DWORD
LwAllocateWc16sPrintfW(
    PWSTR* ppszString,
    const wchar_t* pszFormat,
    ...
    )
{
    DWORD dwError = 0;
    PWSTR pszNewString = NULL;
    va_list args;

    va_start(args, pszFormat);

    pszNewString = asw16printfwv(pszFormat, args);
    if (pszNewString == NULL)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    *ppszString = pszNewString;

    va_end(args);

    return dwError;
}

DWORD
LwWc16sToUpper(
    IN OUT PWSTR pwszString
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    wc16supper(pwszString);

    return dwError;
}

DWORD
LwWc16sToLower(
    IN OUT PWSTR pwszString
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    wc16slower(pwszString);

    return dwError;
}

DWORD
LwAllocateWc16String(
    PWSTR *ppwszOutputString,
    PCWSTR pwszInputString
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwLen = 0;
    PWSTR pwszOutputString = NULL;

    if (!pwszInputString)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwLen = wc16slen(pwszInputString);

    dwError = LwAllocateMemory(sizeof(pwszOutputString[0]) * (dwLen + 1),
                               OUT_PPVOID(&pwszOutputString));
    BAIL_ON_LW_ERROR(dwError);

    if (dwLen)
    {
        wc16sncpy(pwszOutputString, pwszInputString, dwLen);
    }

    *ppwszOutputString = pwszOutputString;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszOutputString);

    *ppwszOutputString = NULL;
    goto cleanup;
}


DWORD
LwAllocateUnicodeStringFromWc16String(
    PUNICODE_STRING   pOutputString,
    PCWSTR            pwszInputString
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;
    PWSTR pwszBuffer = NULL;

    if (!pOutputString)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pwszInputString)
    {
        dwLen  = wc16slen(pwszInputString);
    }

    dwSize = dwLen + 1;
    dwError = LwAllocateMemory(sizeof(pwszBuffer[0]) * dwSize,
                               OUT_PPVOID(&pwszBuffer));
    BAIL_ON_LW_ERROR(dwError);

    if (dwLen)
    {
        wc16sncpy(pwszBuffer, pwszInputString, dwLen);
    }

    pOutputString->Length        = sizeof(pwszBuffer[0]) * dwLen;
    pOutputString->MaximumLength = sizeof(pwszBuffer[0]) * dwSize;
    pOutputString->Buffer        = pwszBuffer;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszBuffer);

    pOutputString->Length        = 0;
    pOutputString->MaximumLength = 0;
    pOutputString->Buffer = NULL;

    goto cleanup;
}


DWORD
LwAllocateUnicodeStringExFromWc16String(
    PUNICODE_STRING   pOutputString,
    PCWSTR            pwszInputString
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;
    PWSTR pwszBuffer = NULL;

    if (!pOutputString)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pwszInputString)
    {
        dwLen  = wc16slen(pwszInputString);
    }

    dwSize = dwLen + 1;
    dwError = LwAllocateMemory(sizeof(pwszBuffer[0]) * dwSize,
                               OUT_PPVOID(&pwszBuffer));
    BAIL_ON_LW_ERROR(dwError);

    if (dwLen)
    {
        wc16sncpy(pwszBuffer, pwszInputString, dwLen);
    }

    pOutputString->Length        = sizeof(pwszBuffer[0]) * dwLen;
    pOutputString->MaximumLength = sizeof(pwszBuffer[0]) * dwSize;
    pOutputString->Buffer        = pwszBuffer;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszBuffer);

    pOutputString->Length        = 0;
    pOutputString->MaximumLength = 0;
    pOutputString->Buffer = NULL;

    goto cleanup;
}


DWORD
LwAllocateWc16StringFromUnicodeString(
    PWSTR            *ppOutputString,
    PUNICODE_STRING   pInputString
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszString = NULL;
    DWORD dwSize = 0;

    if (!ppOutputString ||
        !pInputString ||
        !pInputString->Buffer)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    /*
     * Correctly handle the case where windows (incorrectly) sets
     * max length to the same value as length
     */
    if (pInputString->MaximumLength > pInputString->Length)
    {
        dwSize = pInputString->MaximumLength;
    }
    else if (pInputString->MaximumLength == pInputString->Length)
    {
        dwSize = pInputString->Length + sizeof(WCHAR);
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwAllocateMemory(dwSize,
                               OUT_PPVOID(&pwszString));
    BAIL_ON_LW_ERROR(dwError);

    wc16sncpy(pwszString, pInputString->Buffer, pInputString->Length / 2);

    *ppOutputString = pwszString;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszString);
    *ppOutputString = NULL;

    goto cleanup;
}


DWORD
LwAllocateUnicodeStringFromCString(
    PUNICODE_STRING   pOutputString,
    PCSTR             pszInputString
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;
    PWSTR pwszBuffer = NULL;

    if (!pOutputString)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pszInputString)
    {
        dwLen  = strlen(pszInputString);
    }

    dwSize = dwLen + 1;
    dwError = LwAllocateMemory(sizeof(pwszBuffer[0]) * dwSize,
                               OUT_PPVOID(&pwszBuffer));
    BAIL_ON_LW_ERROR(dwError);

    if (dwLen)
    {
        mbstowc16s(pwszBuffer, pszInputString, dwLen);
    }

    pOutputString->Length        = sizeof(pwszBuffer[0]) * dwLen;
    pOutputString->MaximumLength = sizeof(pwszBuffer[0]) * dwSize;
    pOutputString->Buffer        = pwszBuffer;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszBuffer);

    pOutputString->Length        = 0;
    pOutputString->MaximumLength = 0;
    pOutputString->Buffer = NULL;

    goto cleanup;
}


DWORD
LwAllocateUnicodeStringExFromCString(
    PUNICODE_STRING   pOutputString,
    PCSTR             pszInputString
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwSize = 0;
    PWSTR pwszBuffer = NULL;

    if (!pOutputString)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pszInputString)
    {
        dwLen  = strlen(pszInputString);
    }

    dwSize = dwLen + 1;
    dwError = LwAllocateMemory(sizeof(pwszBuffer[0]) * dwSize,
                               OUT_PPVOID(&pwszBuffer));
    BAIL_ON_LW_ERROR(dwError);

    if (dwLen)
    {
        mbstowc16s(pwszBuffer, pszInputString, dwLen);
    }

    pOutputString->Length        = sizeof(pwszBuffer[0]) * dwLen;
    pOutputString->MaximumLength = sizeof(pwszBuffer[0]) * dwSize;
    pOutputString->Buffer        = pwszBuffer;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszBuffer);

    pOutputString->Length        = 0;
    pOutputString->MaximumLength = 0;
    pOutputString->Buffer = NULL;

    goto cleanup;
}


DWORD
LwAllocateCStringFromUnicodeString(
    PSTR             *ppszOutputString,
    PUNICODE_STRING   pInputString
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszString = NULL;
    DWORD dwSize = 0;

    if (!ppszOutputString ||
        !pInputString ||
        !pInputString->Buffer)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    /*
     * Correctly handle the case where windows (incorrectly) sets
     * max length to the same value as length
     */
    if (pInputString->MaximumLength > pInputString->Length)
    {
        dwSize = (pInputString->MaximumLength / sizeof(WCHAR));
    }
    else if (pInputString->MaximumLength == pInputString->Length)
    {
        dwSize = (pInputString->Length / sizeof(WCHAR)) + sizeof(CHAR);
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwAllocateMemory(dwSize,
                               OUT_PPVOID(&pszString));
    BAIL_ON_LW_ERROR(dwError);

    wc16stombs(pszString, pInputString->Buffer,
               pInputString->Length / sizeof(WCHAR));

    *ppszOutputString = pszString;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pszString);
    *ppszOutputString = NULL;

    goto cleanup;
}


VOID
LwFreeUnicodeString(
    PUNICODE_STRING pString
    )
{
    if (!pString ||
        pString->MaximumLength == 0 ||
        !pString->Buffer)
    {
        return;
    }

    LW_SAFE_FREE_MEMORY(pString->Buffer);
    pString->Buffer = NULL;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
