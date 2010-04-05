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

#ifdef HAVE_WCHAR16_T

DWORD
LsaMbsToWc16s(
    PCSTR pszInput,
    PWSTR* ppszOutput
    )
{
    DWORD dwError = 0;
    PWSTR pszOutput = NULL;
    
    if (!pszInput) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    pszOutput = ambstowc16s(pszInput);
    if (!pszOutput) {
        dwError = LW_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppszOutput = pszOutput;
    
cleanup:

    return dwError;
    
error:
    
    *ppszOutput = NULL;

    goto cleanup;
}

DWORD
LsaWc16snToMbs(
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
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    pwszTruncated = _wc16sndup(pwszInput, sMaxChars);
    if (!pwszTruncated) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pszOutput = awc16stombs(pwszTruncated);
    if (!pszOutput) {
        dwError = LW_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
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
LsaWc16sToMbs(
    PCWSTR pwszInput,
    PSTR*  ppszOutput
    )
{
    DWORD dwError = 0;
    PSTR pszOutput = NULL;
    
    if (!pwszInput) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    pszOutput = awc16stombs(pwszInput);
    if (!pszOutput) {
        dwError = LW_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppszOutput = pszOutput;
    
cleanup:

    return dwError;
    
error:

    *ppszOutput = NULL;

    goto cleanup;
}

DWORD
LsaWc16sLen(
    PCWSTR  pwszInput,
    size_t* psLen
    )
{
    DWORD dwError = 0;
    size_t sLen = 0;
    
    if (!pwszInput) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
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
LsaSW16printf(
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
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppwszStrOutput = pwszStrOutput;
    
cleanup:

    va_end(args); 
     
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszStrOutput);
    
    goto cleanup;
    
}

//
// LSA_STRING Helpers
//

void
LsaFreeLsaString(
    PLSA_STRING lsaString
)
{
    LW_SAFE_FREE_MEMORY(lsaString->buffer);
    lsaString->buffer = NULL;
}

DWORD
LsaCopyLsaStringBase(
    PLSA_STRING dest,
    const PLSA_STRING src,
    BOOLEAN nullTerm
    )
{

    DWORD dwError;
    DWORD newLength = src->max + (nullTerm ? sizeof(wchar16_t) : 0);

    dwError = LwAllocateMemory(newLength, 
        (PVOID*)&dest->buffer);

    BAIL_ON_LSA_ERROR(dwError);

    dest->length = src->length;
    dest->max = newLength;  
    memcpy(dest->buffer, src->buffer, src->max);

    if (nullTerm)
        dest->buffer[((dest->max/sizeof(wchar16_t)) - 1)] = '\0';

error:

    return dwError;
}


/*
 * @brief LsaCopyLsaStringNullTerm
 *
 * For lsa_strings, it is assumed that maxLength > length indicates
 * the string is null terminated.  If they are equal, it
 * is just a raw wchar16t array.
 *
 * This routine pads raw array's with null-term, or leaves it mostly
 * as is.  To be safe, it sets buffer[max] to NULL.
 *
 * @param dest - destination - free buffer with LsaFreeLsaString().
 * @param src - src
 *
 * @returns error on alloc failure.
 */ 
DWORD
LsaCopyLsaStringNullTerm(
    PLSA_STRING dest,
    const PLSA_STRING src
    )
{
    DWORD dwError;

    dwError = LsaCopyLsaStringBase(
                dest,
                src,
                (src->length == src->max)
                );

    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
LsaCopyLsaString(
    PLSA_STRING dest,
    const PLSA_STRING src
    )
{
    return LsaCopyLsaStringBase(dest,src,false);
}


VOID
LsaUpperCaseLsaString(
    PLSA_STRING s
)
{
    int i;

    for (i = 0;i < LSASTR_WCHAR_COUNT(s->length);i++)
    {
        s->buffer[i] = towupper(s->buffer[i]);
    }
}

BOOLEAN
LsaEqualLsaStringBase(
    PLSA_STRING s1,
    PLSA_STRING s2,
    BOOLEAN caseSensitive
    )
{

    int i;

    if (s1->length != s2->length)
        return false;

    /* do we care about max? */
    if (!caseSensitive) {
        for (i=0; i<LSASTR_WCHAR_COUNT(s1->length);i++) {
            if (towupper(s1->buffer[i]) != towupper(s2->buffer[i]))
                return false;
        }
    }
    else
    {
        if (memcmp(s1->buffer, s2->buffer, s1->length))
            return false;
    }

    return true;
}

BOOLEAN
LsaEqualLsaString(
    PLSA_STRING s1,
    PLSA_STRING s2
    )
{
    return LsaEqualLsaStringBase(s1,s2,true);
}


BOOLEAN
LsaEqualLsaStringNoCase(
    PLSA_STRING s1,
    PLSA_STRING s2
    )
{
    return LsaEqualLsaStringBase(s1,s2,false);
}


DWORD 
LsaInitializeLsaStringW(
    wchar16_t *input, 
    BOOLEAN copy,
    PLSA_STRING lsaStringOut
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    lsaStringOut->length = LSASTR_BYTECOUNT(input);
    lsaStringOut->max = lsaStringOut->length + sizeof(wchar16_t);

    if (copy) {
        dwError = LwAllocateMemory(lsaStringOut->max,
            (PVOID*) &lsaStringOut->buffer);

        BAIL_ON_LSA_ERROR(dwError);
        _wc16scpy(lsaStringOut->buffer, input);
    } else {
        lsaStringOut->buffer = input;
    }

    return dwError;

error:
    if (copy)
        LsaFreeLsaString(lsaStringOut);

    return dwError;
}

DWORD
LsaStringToAnsi(
    PLSA_STRING lsaString,
    PSTR *ansiString
)
{
    return LsaWc16sToMbs(lsaString->buffer, ansiString);
}

DWORD
LsaInitializeLsaStringA(
    PCSTR wcString, 
    PLSA_STRING lsaStringOut
    )
{
    DWORD dwError;
    wchar16_t* pwszConverted = NULL;
    size_t cGuess;
    size_t cConverted;

    if (wcString == NULL)
    {
        lsaStringOut->length = 0;
        lsaStringOut->max = 0;
        lsaStringOut->buffer = NULL;
        return LW_ERROR_SUCCESS;
    }

    cGuess = (strlen(wcString) + 1);

    do {

        dwError = LwReallocMemory((PVOID)pwszConverted, 
            (PVOID*)&pwszConverted,
            LSASTR_CCH_BYTECOUNT(cGuess));

        BAIL_ON_LSA_ERROR(dwError);

        cConverted = mbstowc16s(pwszConverted,
            wcString,
            cGuess);

        if (cConverted == -1) {
            /* @todo - LsaConvertPosixError() */
            if ( errno != E2BIG) {
                dwError = LW_ERROR_INSUFFICIENT_BUFFER;
                BAIL_ON_LSA_ERROR(cGuess);
            }

            cGuess += LSASTR_BUFF_PROBE;
        }

    } while (cConverted < 0 && cGuess < LSASTR_BUFF_MAXCONVERTED);


    dwError = LsaInitializeLsaStringW(
        pwszConverted,
        false,
        lsaStringOut
    );

    BAIL_ON_LSA_ERROR(dwError);

    /* this is assigned to the string now */
    pwszConverted = NULL;

error:

    LW_SAFE_FREE_MEMORY(pwszConverted);
    return dwError;
}

DWORD
LsaWc16ToUpper(
    IN OUT PWSTR pwszString
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    
    wc16supper(pwszString);
    
    return dwError;    
}



#endif


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
