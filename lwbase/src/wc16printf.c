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

/*
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */

/* -*- mode: c; c-basic-offset: 4 -*- */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <wchar.h>
#include <wchar16.h>
#include <wc16str.h>
#include <wc16printf.h>
#include <limits.h>

#ifndef SOLARIS_11
#ifdef HAVE_SYS_VARARGS_H
#include <sys/varargs.h>
#endif
#endif

#ifdef _WIN32
#pragma warning( disable : 4996 )

#include <mbstring.h>

#define va_copy(a, b)   ((a) = (b))
#endif

// % <flag>* <field width>? <precision>? <length modifier>? <conversion specifier>
// flag may be one of:
#define FLAG_ALT_FORM           1
//  '#'
#define FLAG_ZERO_PAD           2
//  '0'
#define FLAG_LEFT_JUSTIFY       4
//  '-'
#define FLAG_SPACE_FOR_POSITIVE 8
//  ' '
#define FLAG_PLUS_FOR_POSITIVE  16
//  '+'
#define FLAG_COMMA_SEPARATE     32
//  '\''
#define FLAG_LOCALE_DIGITS      64
//  'I'
//
#define FLAG_UPPER_CASE         128
// This one isn't accessible from the format specifier, but X, E, G, etc.
// are converted to their lower case counterparts with this flag set.
//
// field width:
//  \* | [0-9]+
//
// precision:
//  \. ( \* | [0-9]* )
//
// length modifier:
#define LENGTH_UNSET            0
#define LENGTH_CHAR             1
//  hh
#define LENGTH_SHORT            2
//  h
#define LENGTH_LONG             3
//  l
#define LENGTH_LONG_LONG        4
//  ll,q
#define LENGTH_LONG_DOUBLE      5
//  L
#define LENGTH_INTMAX_T         6
//  j
#define LENGTH_SIZE_T           7
//  z
#define LENGTH_PTRDIFF_T        8
//  t
#define LENGTH_W16CHAR_T        9
//  w
//
// conversion specifier:
#define TYPE_INTEGER            1
//  d,i
#define TYPE_OCTAL              2
//  o
#define TYPE_UNSIGNED_INTEGER   3
//  u
#define TYPE_HEX          4
//  x,X
#define TYPE_SCIENTIFIC   6
//  e,E
#define TYPE_DOUBLE             8
//  f,F
#define TYPE_AUTO_DOUBLE  10
//  g,G
#define TYPE_HEX_DOUBLE   11
//  a,A
#define TYPE_CHAR               13
//  c
#define TYPE_STRING             14
//  s
#define TYPE_CHAR               13
//  C
#define TYPE_STRING             14
//  S
#define TYPE_POINTER            15
//  p
#define TYPE_WRITTEN_COUNT      16
//  n
#define TYPE_STRERROR           17
//  m
#define TYPE_PERCENT            18
//  %
#define TYPE_INVALID            19

typedef struct _PRINTF_BUFFER PRINTF_BUFFER, *PPRINTF_BUFFER;

typedef void (*PFNBUFFER_WRITE_WCS)(
    PPRINTF_BUFFER pBuffer,
    const wchar_t *pwszBuffer,
    size_t cchBuffer);
typedef void (*PFNBUFFER_WRITE_WC16S)(
    PPRINTF_BUFFER pBuffer,
    const wchar16_t *pwszBuffer,
    size_t cchBuffer);
typedef void (*PFNBUFFER_WRITE_MBS)(
    PPRINTF_BUFFER pBuffer,
    const char *pszBuffer,
    size_t cchBuffer);

struct _PRINTF_BUFFER
{
    size_t sWrittenCount;
    PFNBUFFER_WRITE_WCS pfnWriteWcs;
    PFNBUFFER_WRITE_WC16S pfnWriteWc16s;
    PFNBUFFER_WRITE_MBS pfnWriteMbs;
};

static
void
WriteSpaces(
    PPRINTF_BUFFER pBuffer,
    ssize_t sCount
    )
{
    wchar16_t space = ' ';

    while (sCount > 0)
    {
        pBuffer->pfnWriteWc16s(pBuffer, &space, 1);
        sCount--;
    }
}

static
unsigned int
ParseFlags(
    const wchar16_t** ppwszPos
    )
{
    const wchar16_t* pwszPos = *ppwszPos;
    int bFoundOption = 1;
    unsigned int dwFlags = 0;

    while (bFoundOption)
    {
        switch (*pwszPos++)
        {
            case '#':
                dwFlags |= FLAG_ALT_FORM;
                break;
            case '0':
                dwFlags |= FLAG_ZERO_PAD;
                break;
            case '-':
                dwFlags |= FLAG_LEFT_JUSTIFY;
                break;
            case ' ':
                dwFlags |= FLAG_SPACE_FOR_POSITIVE;
                break;
            case '+':
                dwFlags |= FLAG_PLUS_FOR_POSITIVE;
                break;
            case '\'':
                dwFlags |= FLAG_COMMA_SEPARATE;
                break;
            case 'I':
                dwFlags |= FLAG_LOCALE_DIGITS;
                break;
            default:
                bFoundOption = 0;
                pwszPos--;
                break;
        }
    }

    *ppwszPos = pwszPos;
    return dwFlags;
}

static
unsigned int
ParseLengthModifier(
    const wchar16_t** ppwszPos
    )
{
    const wchar16_t* pwszPos = *ppwszPos;
    unsigned int dwLengthModifier = LENGTH_UNSET;

    switch (*pwszPos)
    {
        case 'h':
            if (pwszPos[1] == 'h')
            {
                dwLengthModifier = LENGTH_CHAR;
                pwszPos += 2;
            }
            else
            {
                dwLengthModifier = LENGTH_SHORT;
                pwszPos++;
            }
            break;
        case 'l':
            if (pwszPos[1] == 'l')
            {
                dwLengthModifier = LENGTH_LONG_LONG;
                pwszPos += 2;
            }
            else
            {
                dwLengthModifier = LENGTH_LONG;
                pwszPos++;
            }
            break;
        case 'q':
            dwLengthModifier = LENGTH_LONG_LONG;
            pwszPos++;
            break;
        case 'L':
            dwLengthModifier = LENGTH_LONG_DOUBLE;
            pwszPos++;
            break;
        case 'j':
            dwLengthModifier = LENGTH_INTMAX_T;
            pwszPos++;
            break;
        case 'z':
            dwLengthModifier = LENGTH_SIZE_T;
            pwszPos++;
            break;
        case 't':
            dwLengthModifier = LENGTH_PTRDIFF_T;
            pwszPos++;
            break;
        case 'w':
            dwLengthModifier = LENGTH_W16CHAR_T;
            pwszPos++;
            break;
    }

    *ppwszPos = pwszPos;
    return dwLengthModifier;
}

static
unsigned int
ParseType(
    const wchar16_t** ppwszPos,
    unsigned int *pdwFlags
    )
{
    const wchar16_t* pwszPos = *ppwszPos;
    unsigned int dwType = TYPE_INVALID;

    switch (*pwszPos++)
    {
        case 'd':
        case 'i':
            dwType = TYPE_INTEGER;
            break;
        case 'o':
            dwType = TYPE_OCTAL;
            break;
        case 'u':
            dwType = TYPE_UNSIGNED_INTEGER;
            break;
        case 'X':
            *pdwFlags |= FLAG_UPPER_CASE;
        case 'x':
            dwType = TYPE_HEX;
            break;
        case 'E':
            *pdwFlags |= FLAG_UPPER_CASE;
        case 'e':
            dwType = TYPE_SCIENTIFIC;
            break;
        case 'F':
            // Upper case F doesn't do anything special
        case 'f':
            dwType = TYPE_DOUBLE;
            break;
        case 'G':
            *pdwFlags |= FLAG_UPPER_CASE;
        case 'g':
            dwType = TYPE_AUTO_DOUBLE;
            break;
        case 'A':
            *pdwFlags |= FLAG_UPPER_CASE;
        case 'a':
            dwType = TYPE_HEX_DOUBLE;
            break;
        case 'C':
            *pdwFlags |= FLAG_UPPER_CASE;
        case 'c':
            dwType = TYPE_CHAR;
            break;
        case 'S':
            *pdwFlags |= FLAG_UPPER_CASE;
        case 's':
            dwType = TYPE_STRING;
            break;
        case 'p':
            dwType = TYPE_POINTER;
            break;
        case 'n':
            dwType = TYPE_WRITTEN_COUNT;
            break;
        case 'm':
            dwType = TYPE_STRERROR;
            break;
        case '%':
            dwType = TYPE_PERCENT;
            break;
        default:
            pwszPos--;
            dwType = TYPE_INVALID;
            break;
    }

    *ppwszPos = pwszPos;
    return dwType;
}

static
int
W16PrintfCore(
    PPRINTF_BUFFER pBuffer,
    int bMSCompat,
    const wchar16_t* pwszFormat,
    va_list args
    )
{
    size_t sLen = 0;
    const wchar16_t* pwszPos = pwszFormat;
    const wchar16_t* pwszTokenStart = pwszFormat;
    unsigned int dwFlags = 0;
    unsigned int dwLengthModifier = 0;
    unsigned int dwType = 0;
    size_t sWidth = 0;
    ssize_t iPrecision = -1;
    // Enough room for all supported fixed sized types
    char szArgBuffer[100];
    wchar_t wszArgBuffer[1];
    wchar16_t w16szArgBuffer[100];
    // Enough room for all supported format specifiers
    char szFormatBuffer[20];
    char *pszFormatBufferPos = NULL;

    while (*pwszPos)
    {
        if (*pwszPos == '%')
        {
            pBuffer->pfnWriteWc16s(
                    pBuffer,
                    pwszTokenStart,
                    pwszPos - pwszTokenStart);

            pwszPos++;

            // Put the format specifier (% included) into a multibyte
            // string incase regular printf needs to be called
            pszFormatBufferPos = szFormatBuffer;
            *pszFormatBufferPos++ = '%';

            pwszTokenStart = pwszPos;
            dwFlags = ParseFlags(&pwszPos);
            // update multibyte format
            // subtract for % * . * null
            if (pwszPos - pwszTokenStart > sizeof(szFormatBuffer) - 5)
            {
                errno = ENOMEM;
                return -1;
            }
            sLen = wc16stombs(
                    pszFormatBufferPos,
                    pwszTokenStart,
                    pwszPos - pwszTokenStart);
            if (sLen == (size_t)-1)
            {
                return -1;
            }
            pszFormatBufferPos += sLen;

            if (*pwszPos == '*')
            {
                pwszPos++;
                sWidth = va_arg(args, size_t);
            }
            else
            {
                sWidth = (size_t)wc16stoull(pwszPos, &pwszPos, 10);
            }
            *pszFormatBufferPos++ = '*';

            iPrecision = -1;
            if (*pwszPos == '.')
            {
                pwszPos++;
                if (*pwszPos == '*')
                {
                    pwszPos++;
                    iPrecision = va_arg(args, ssize_t);
                }
                else if (*pwszPos != '-')
                {
                    iPrecision = (ssize_t)wc16stoull(pwszPos, &pwszPos, 10);
                }
            }
            *pszFormatBufferPos++ = '.';
            *pszFormatBufferPos++ = '*';

            pwszTokenStart = pwszPos;

            dwLengthModifier = ParseLengthModifier(&pwszPos);
            dwType = ParseType(&pwszPos, &dwType);

            if (pszFormatBufferPos - szFormatBuffer +
                    pwszPos - pwszTokenStart > sizeof(szFormatBuffer) - 1)
            {
                errno = ENOMEM;
                return -1;
            }
            sLen = wc16stombs(
                    pszFormatBufferPos,
                    pwszTokenStart,
                    pwszPos - pwszTokenStart);
            if (sLen == (size_t)-1)
            {
                return -1;
            }
            pszFormatBufferPos += sLen;
            *pszFormatBufferPos = 0;

            if (bMSCompat && (dwType == TYPE_STRING || dwType == TYPE_CHAR))
            {
                if (dwLengthModifier == LENGTH_LONG)
                {
                    dwLengthModifier = LENGTH_W16CHAR_T;
                }
                else if (dwLengthModifier == LENGTH_SHORT)
                {
                    dwLengthModifier = LENGTH_CHAR;
                }
                else
                {
                    if (dwFlags & FLAG_UPPER_CASE)
                    {
                        dwLengthModifier = LENGTH_CHAR;
                    }
                    else
                    {
                        dwLengthModifier = LENGTH_W16CHAR_T;
                    }
                }
            }

            switch (dwType)
            {
                case TYPE_OCTAL:
                case TYPE_UNSIGNED_INTEGER:
                case TYPE_HEX:
                case TYPE_INTEGER:
                    if (dwLengthModifier == LENGTH_LONG_LONG)
                    {
                        long long llArg = va_arg(args, long long);
                        sLen = snprintf(
                                    szArgBuffer,
                                    sizeof(szArgBuffer),
                                    szFormatBuffer,
                                    sWidth,
                                    iPrecision,
                                    llArg);
                    }
                    else
                    {
                        long lArg = va_arg(args, long);
                        sLen = snprintf(
                                    szArgBuffer,
                                    sizeof(szArgBuffer),
                                    szFormatBuffer,
                                    sWidth,
                                    iPrecision,
                                    lArg);
                    }
                    if (sLen > sizeof(szArgBuffer) || (ssize_t) sLen < 0)
                    {
                        errno = ENOMEM;
                        return -1;
                    }
                    pBuffer->pfnWriteMbs(pBuffer, szArgBuffer, sLen);
                    break;

                case TYPE_DOUBLE:
                case TYPE_AUTO_DOUBLE:
                case TYPE_HEX_DOUBLE:
                case TYPE_SCIENTIFIC:
                    if (dwLengthModifier == LENGTH_LONG_DOUBLE)
                    {
                        long double ldArg = va_arg(args, long double);
                        sLen = snprintf(
                                    szArgBuffer,
                                    sizeof(szArgBuffer),
                                    szFormatBuffer,
                                    sWidth,
                                    iPrecision,
                                    ldArg);
                    }
                    else
                    {
                        double dArg = va_arg(args, double);
                        sLen = snprintf(
                                    szArgBuffer,
                                    sizeof(szArgBuffer),
                                    szFormatBuffer,
                                    sWidth,
                                    iPrecision,
                                    dArg);
                    }
                    if (sLen > sizeof(szArgBuffer) || (ssize_t) sLen < 0)
                    {
                        errno = ENOMEM;
                        return -1;
                    }
                    pBuffer->pfnWriteMbs(pBuffer, szArgBuffer, sLen);
                    break;

                case TYPE_CHAR:
                    {
                        wint_t iArg = va_arg(args, wint_t);
                        if (!(dwFlags & FLAG_LEFT_JUSTIFY))
                        {
                            WriteSpaces(pBuffer, (ssize_t)sWidth - 1);
                        }
                        switch (dwLengthModifier)
                        {
                            case LENGTH_UNSET:
                            case LENGTH_CHAR:
                                szArgBuffer[0] = (char)iArg;
                                pBuffer->pfnWriteMbs(
                                        pBuffer,
                                        szArgBuffer,
                                        1);
                                break;
                            case LENGTH_SHORT:
                            case LENGTH_W16CHAR_T:
                                w16szArgBuffer[0] = iArg;
                                pBuffer->pfnWriteWc16s(
                                        pBuffer,
                                        w16szArgBuffer,
                                        1);
                                break;
                            case LENGTH_LONG:
                                wszArgBuffer[0] = iArg;
                                pBuffer->pfnWriteWcs(
                                        pBuffer,
                                        wszArgBuffer,
                                        1);
                                break;
                            case LENGTH_LONG_LONG:
                            case LENGTH_LONG_DOUBLE:
                            case LENGTH_INTMAX_T:
                            case LENGTH_SIZE_T:
                            case LENGTH_PTRDIFF_T:
                            default:
                                errno = EINVAL;
                                return -1;
                        }
                        if (dwFlags & FLAG_LEFT_JUSTIFY)
                        {
                            WriteSpaces(pBuffer, (ssize_t)sWidth - 1);
                        }
                    }
                    break;
                case TYPE_STRING:
                    {
                        void* pvArg = va_arg(args, void *);
                        if (pvArg == NULL)
                        {
                            pvArg = "(null)";
                            dwLengthModifier = LENGTH_CHAR;
                        }
                        switch (dwLengthModifier)
                        {
                            case LENGTH_UNSET:
                            case LENGTH_CHAR:
                                sLen = strlen((char *)pvArg);
                                break;
                            case LENGTH_SHORT:
                            case LENGTH_W16CHAR_T:
                                sLen = wc16slen((wchar16_t *)pvArg);
                                break;
                            case LENGTH_LONG:
                                sLen = wcslen((wchar_t *)pvArg);
                                break;
                            case LENGTH_LONG_LONG:
                            case LENGTH_LONG_DOUBLE:
                            case LENGTH_INTMAX_T:
                            case LENGTH_SIZE_T:
                            case LENGTH_PTRDIFF_T:
                            default:
                                errno = EINVAL;
                                return -1;
                        }
                        if (iPrecision >= 0 && sLen > (size_t)iPrecision)
                        {
                            sLen = iPrecision;
                        }
                        if (!(dwFlags & FLAG_LEFT_JUSTIFY))
                        {
                            WriteSpaces(pBuffer, (ssize_t)sWidth - (ssize_t)sLen);
                        }
                        switch (dwLengthModifier)
                        {
                            case LENGTH_UNSET:
                            case LENGTH_CHAR:
                                pBuffer->pfnWriteMbs(
                                        pBuffer,
                                        (char *)pvArg,
                                        sLen);
                                break;
                            case LENGTH_SHORT:
                            case LENGTH_W16CHAR_T:
                                pBuffer->pfnWriteWc16s(
                                        pBuffer,
                                        (wchar16_t *)pvArg,
                                        sLen);
                                break;
                            case LENGTH_LONG:
                                pBuffer->pfnWriteWcs(
                                        pBuffer,
                                        (wchar_t *)pvArg,
                                        sLen);
                                break;
                            //Invalid cases have already been checked for
                        }
                        if (dwFlags & FLAG_LEFT_JUSTIFY)
                        {
                            WriteSpaces(pBuffer, (ssize_t)(sWidth - sLen));
                        }
                    }
                    break;
                case TYPE_POINTER:
                    {
                        void *pvArg = va_arg(args, void *);
                        sLen = snprintf(
                                    szArgBuffer,
                                    sizeof(szArgBuffer),
                                    szFormatBuffer,
                                    sWidth,
                                    iPrecision,
                                    pvArg);
                        if (sLen > sizeof(szArgBuffer) || (ssize_t) sLen < 0)
                        {
                            errno = ENOMEM;
                            return -1;
                        }
                        pBuffer->pfnWriteMbs(pBuffer, szArgBuffer, sLen);
                    }
                    break;
                case TYPE_WRITTEN_COUNT:
                    {
                        size_t *psArg = va_arg(args, size_t *);
                        if (psArg == NULL)
                        {
                            errno = EINVAL;
                            return -1;
                        }
                        *psArg = pBuffer->sWrittenCount;
                    }
                    break;
                case TYPE_STRERROR:
                    {
                        sLen = snprintf(
                                    szArgBuffer,
                                    sizeof(szArgBuffer),
                                    szFormatBuffer,
                                    sWidth,
                                    iPrecision);
                        if (sLen > sizeof(szArgBuffer) || (ssize_t) sLen < 0)
                        {
                            errno = ENOMEM;
                            return -1;
                        }
                        pBuffer->pfnWriteMbs(pBuffer, szArgBuffer, sLen);
                    }
                    break;
                case TYPE_PERCENT:
                    w16szArgBuffer[0] = '%';
                    pBuffer->pfnWriteWc16s(pBuffer, w16szArgBuffer, 1);
                    break;
                case TYPE_INVALID:
                    errno = EINVAL;
                    return -1;
            }
            pwszTokenStart = pwszPos;
        }
        else
        {
            pwszPos++;
        }
    }

    pBuffer->pfnWriteWc16s(pBuffer, pwszTokenStart, pwszPos - pwszTokenStart);
    return 0;
}

typedef struct _STRING_PRINTF_BUFFER
{
    PRINTF_BUFFER parent;
    wchar16_t* pwszBuffer;
    size_t sAvailable;
    unsigned int dwError;
} STRING_PRINTF_BUFFER, *PSTRING_PRINTF_BUFFER;

static
void
StringPrintfWriteWcs(
    PSTRING_PRINTF_BUFFER pBuffer,
    const wchar_t *pwszWrite,
    size_t cchWrite)
{
    size_t sConverted;
   
    if (pBuffer->dwError)
    {
        goto error;
    }
    if (pBuffer->pwszBuffer)
    {
        if (cchWrite > pBuffer->sAvailable)
        {
            pBuffer->dwError = ENOMEM;
            goto error;
        }

        sConverted = wcstowc16s(pBuffer->pwszBuffer, pwszWrite, cchWrite);
        if (sConverted != cchWrite)
        {
            pBuffer->dwError = errno;
            goto error;
        }
        pBuffer->pwszBuffer += sConverted;
        pBuffer->sAvailable -= sConverted;
    }
    pBuffer->parent.sWrittenCount += cchWrite;
error:
    ;
}

static
void
StringPrintfWriteMbs(
    PSTRING_PRINTF_BUFFER pBuffer,
    const char *pszWrite,
    size_t cchWrite)
{
    size_t sConverted;
   
    if (pBuffer->dwError)
    {
        goto error;
    }
    if (pBuffer->pwszBuffer)
    {
        if (cchWrite > pBuffer->sAvailable)
        {
            pBuffer->dwError = ENOMEM;
            goto error;
        }

        sConverted = mbstowc16s(pBuffer->pwszBuffer, pszWrite, cchWrite);
        if (sConverted == -1)
        {
            pBuffer->dwError = errno;
            goto error;
        }
        pBuffer->pwszBuffer += sConverted;
        pBuffer->sAvailable -= sConverted;
    }
    pBuffer->parent.sWrittenCount += cchWrite;
error:
    ;
}

static
void
StringPrintfWriteWc16s(
    PSTRING_PRINTF_BUFFER pBuffer,
    const wchar16_t *pwszWrite,
    size_t cchWrite)
{
    ssize_t sConverted;
   
    if (pBuffer->dwError)
    {
        goto error;
    }
    if (pBuffer->pwszBuffer && cchWrite)
    {
        if (cchWrite > pBuffer->sAvailable)
        {
            pBuffer->dwError = ENOMEM;
            goto error;
        }

        sConverted = wc16pncpy(pBuffer->pwszBuffer, pwszWrite, cchWrite) -
                        pBuffer->pwszBuffer + 1;
        if (sConverted != cchWrite)
        {
            pBuffer->dwError = errno;
            goto error;
        }
        pBuffer->pwszBuffer += sConverted;
        pBuffer->sAvailable -= sConverted;
    }
    pBuffer->parent.sWrittenCount += cchWrite;
error:
    ;
}

ssize_t
_vsw16printf(
    wchar16_t *out,
    size_t maxchars,
    const wchar16_t *format,
    va_list args
    )
{
    int error = 0;
    STRING_PRINTF_BUFFER output;
   
    memset(&output, 0, sizeof(output));

    output.parent.pfnWriteWcs = (PFNBUFFER_WRITE_WCS)StringPrintfWriteWcs;
    output.parent.pfnWriteWc16s = (PFNBUFFER_WRITE_WC16S)StringPrintfWriteWc16s;
    output.parent.pfnWriteMbs = (PFNBUFFER_WRITE_MBS)StringPrintfWriteMbs;
    output.pwszBuffer = out;
    output.sAvailable = maxchars;

    if (W16PrintfCore(
            &output.parent,
            0,
            format,
            args) < 0)
    {
        error = errno;
    }
    if (output.dwError)
    {
        // This error takes precedence over an error from W16PrintfCore
        error = output.dwError;
    }
    if (error)
    {
        goto error;
    }

    if (!output.pwszBuffer)
    {
        /* This is only calculating the maximum size. Do not include the null
         * in this calculation, and do not write anything.
         */
    }
    else if (output.sAvailable)
    {
        /* terminate the resulting string */
        *output.pwszBuffer = 0;
    }
    else
    {
        error = ENOMEM;
        goto error;
    }

cleanup:
    if (error)
    {
        return -1;
    }
    else
    {
        return output.parent.sWrittenCount;
    }

error:
    goto cleanup;
}

//TODO: rename this once the deprecated sw16printf is removed
ssize_t
_sw16printf_new(wchar16_t *out, size_t maxchars, const wchar16_t *format, ...)
{
    ssize_t sResult = 0;
    va_list ap;

    va_start(ap, format);
    
    sResult = _vsw16printf(
                    out,
                    maxchars,
                    format,
                    ap);
    if (sResult < 0)
    {
        goto error;
    }

cleanup:
    va_end(ap);

    return sResult;

error:
    goto cleanup;
}

ssize_t
_sw16printfw(wchar16_t *out, size_t maxchars, const wchar_t *format, ...)
{
    int bFreeFormat = 0;
    wchar16_t *pwszFormat = NULL;
    ssize_t sResult = 0;
    va_list ap;

    va_start(ap, format);
    
    pwszFormat = awcstowc16s(
                        format,
                        &bFreeFormat);
    if (pwszFormat == NULL)
    {
        errno = ENOMEM;
        goto error;
    }

    sResult = _vsw16printf(
                    out,
                    maxchars,
                    pwszFormat,
                    ap);
    if (sResult < 0)
    {
        goto error;
    }

cleanup:
    if (bFreeFormat)
    {
        free(pwszFormat);
    }
    va_end(ap);

    return sResult;

error:
    goto cleanup;
}

wchar16_t *
asw16printfwv(const wchar_t *format, va_list args)
{
    int bFreeFormat = 0;
    wchar16_t *pwszFormat = NULL;
    wchar16_t *pwszOut = NULL;
    ssize_t sLen = 0;
    va_list args2;

    va_copy(args2, args);
    
    pwszFormat = awcstowc16s(
                        format,
                        &bFreeFormat);
    if (pwszFormat == NULL)
    {
        errno = ENOMEM;
        goto error;
    }

    sLen = vsw16printf(
                    NULL,
                    0,
                    pwszFormat,
                    args);
    if (sLen < 0)
    {
        goto error;
    }

    pwszOut = malloc((sLen + 1) * sizeof(*pwszOut));

    sLen = vsw16printf(
                    pwszOut,
                    sLen + 1,
                    pwszFormat,
                    args2);
    if (sLen < 0)
    {
        goto error;
    }

cleanup:
    if (bFreeFormat)
    {
        free(pwszFormat);
    }
    va_end(args2);

    return pwszOut;

error:
    if (pwszOut != NULL)
    {
        free(pwszOut);
        pwszOut = NULL;
    }
    goto cleanup;
}

wchar16_t *
asw16printfw(const wchar_t *format, ...)
{
    va_list ap;
    wchar16_t *result = NULL;

    va_start(ap, format);
    result = asw16printfwv(format, ap);
    va_end(ap);

    return result;
}

typedef struct _FILE_PRINTF_BUFFER
{
    PRINTF_BUFFER parent;
    FILE* pFile;
    unsigned int dwError;
} FILE_PRINTF_BUFFER, *PFILE_PRINTF_BUFFER;

static
void
FilePrintfWriteWcs(
    PFILE_PRINTF_BUFFER pBuffer,
    const wchar_t *pwszWrite,
    size_t cchWrite)
{
    wchar_t *pwszWrite0 = NULL; // Null-terminated equivalent to pwszWrite

    if (pBuffer->dwError)
    {
        // Something already went wrong. Don't try to write anything else
        return;
    }
    if (cchWrite > INT_MAX)
    {
        pBuffer->dwError = ERANGE;
        return;
    }
   
    pwszWrite0 = calloc(cchWrite + 1, sizeof(*pwszWrite));
    if (!pwszWrite0)
    {
        return;
    }
    
    memcpy(pwszWrite0, pwszWrite, cchWrite);

    // This is the number of characters that would be written if the output
    // string had unlimited space.
    pBuffer->parent.sWrittenCount += cchWrite;

    if (fprintf(pBuffer->pFile, "%ls", pwszWrite0) < 0)
    {
        pBuffer->dwError = errno;
    }
    free(pwszWrite0);
}

static
void
FilePrintfWriteWc16s(
    PFILE_PRINTF_BUFFER pBuffer,
    const wchar16_t *pw16szWrite,
    size_t cch16Write)
{
    wchar_t* pwszWrite = NULL;
    size_t cchWrite = 0;

    if (pBuffer->dwError)
    {
        // Something already went wrong. Don't try to write anything else
        goto error;
    }

    pwszWrite = calloc(cch16Write + 1, sizeof(*pwszWrite));
    if (pwszWrite == NULL)
    {
        goto error;
    }

    cchWrite = wc16stowcs(pwszWrite, pw16szWrite, cch16Write);

    if (cchWrite == (size_t)-1)
    {
        goto error;
    }
    if (cchWrite > INT_MAX)
    {
        pBuffer->dwError = ERANGE;
        goto error;
    }

    // This is the number of characters that would be written if the output
    // string had unlimited space.
    pBuffer->parent.sWrittenCount += cchWrite;

    if (fprintf(pBuffer->pFile, "%ls", pwszWrite) < 0)
    {
        pBuffer->dwError = errno;
    }

cleanup:
    if (pwszWrite)
    {
        free(pwszWrite);
    }
    return;

error:
    goto cleanup;
}

static
void
FilePrintfWriteMbs(
    PFILE_PRINTF_BUFFER pBuffer,
    const char *pszWrite,
    size_t cchWrite)
{
    size_t cbWrite;

    if (pBuffer->dwError)
    {
        // Something already went wrong. Don't try to write anything else
        return;
    }
   
    // This is the number of characters that would be written if the output
    // string had unlimited space.
    pBuffer->parent.sWrittenCount += cchWrite;

    cbWrite = mbsnbcnt(pszWrite, cchWrite);
    if (cbWrite > INT_MAX)
    {
        pBuffer->dwError = ERANGE;
        return;
    }

    if (fprintf(pBuffer->pFile, "%.*s", (int)cbWrite, pszWrite) < 0)
    {
        pBuffer->dwError = errno;
    }
}

ssize_t
_vfw16printf(
    FILE *pFile,
    const wchar16_t *format,
    va_list args
    )
{
    int error = 0;
    FILE_PRINTF_BUFFER output;

    memset(&output, 0, sizeof(output));

    output.parent.pfnWriteWcs = (PFNBUFFER_WRITE_WCS)FilePrintfWriteWcs;
    output.parent.pfnWriteWc16s = (PFNBUFFER_WRITE_WC16S)FilePrintfWriteWc16s;
    output.parent.pfnWriteMbs = (PFNBUFFER_WRITE_MBS)FilePrintfWriteMbs;
    output.pFile = pFile;

    if (W16PrintfCore(
            &output.parent,
            0,
            format,
            args) < 0)
    {
        error = errno;
    }
    if (output.dwError)
    {
        // This error takes precedence over an error from W16PrintfCore
        error = output.dwError;
    }
    if (error)
    {
        goto error;
    }

cleanup:
    if (error)
    {
        return -1;
    }
    else
    {
        return output.parent.sWrittenCount;
    }

error:
    goto cleanup;
}

ssize_t
_fw16printf(FILE *pFile, const wchar16_t *format, ...)
{
    ssize_t sResult = 0;
    va_list ap;

    va_start(ap, format);
    
    sResult = _vfw16printf(
                    pFile,
                    format,
                    ap);
    if (sResult < 0)
    {
        goto error;
    }

cleanup:
    va_end(ap);

    return sResult;

error:
    goto cleanup;
}

ssize_t
_fw16printfw(FILE *pFile, const wchar_t *format, ...)
{
    int bFreeFormat = 0;
    wchar16_t *pwszFormat = NULL;
    ssize_t sResult = 0;
    va_list ap;

    va_start(ap, format);
    
    pwszFormat = awcstowc16s(
                        format,
                        &bFreeFormat);
    if (pwszFormat == NULL)
    {
        errno = ENOMEM;
        goto error;
    }

    sResult = _vfw16printf(
                    pFile,
                    pwszFormat,
                    ap);
    if (sResult < 0)
    {
        goto error;
    }

cleanup:
    if (bFreeFormat)
    {
        free(pwszFormat);
    }
    va_end(ap);

    return sResult;

error:
    goto cleanup;
}

ssize_t
_w16printfw(const wchar_t *format, ...)
{
    int bFreeFormat = 0;
    wchar16_t *pwszFormat = NULL;
    ssize_t sResult = 0;
    va_list ap;

    va_start(ap, format);
    
    pwszFormat = awcstowc16s(
                        format,
                        &bFreeFormat);
    if (pwszFormat == NULL)
    {
        errno = ENOMEM;
        goto error;
    }

    sResult = _vfw16printf(
                    stdout,
                    pwszFormat,
                    ap);
    if (sResult < 0)
    {
        goto error;
    }

cleanup:
    if (bFreeFormat)
    {
        free(pwszFormat);
    }
    va_end(ap);

    return sResult;

error:
    goto cleanup;
}

//Deprecated
int sw16printf(wchar16_t *out, const char *format, ...)
{
    va_list ap;
    char c;
    size_t len;
    char *s;
    wchar16_t *S;
    wchar_t *W;
    wchar16_t *buf = out;
    char *fmt, *f;
    size_t spec_len;
    char *spec_end, spec[64];
    char spec_out[64];    /* larger than any number "printf-ed" */
    char conv_string[2];

    len = 0;

    fmt = strdup(format);
    if (fmt == NULL) return -1;

    f = fmt;
    va_start(ap, format);

    while (*fmt) {
	if ((*fmt) != '%') {
            conv_string[0] = *fmt;
            conv_string[1] = 0;
	    mbstowc16s(buf++, conv_string, 1);
	    fmt++;
	    continue;
	}

	/* get the next character */
	c = *(fmt + 1);

	switch (c) {
	case 's': /* byte character string */
	    s = va_arg(ap, char *);
	    len = strlen(s);
	    mbstowc16s(buf, s, len);
	    fmt += 2;
	    break;
	    
	case 'S': /* 16-bit character string */
	    S = va_arg(ap, wchar16_t *);
	    len = wc16slen(S);
	    wc16scpy(buf, S);
	    fmt += 2;
	    break;
	
	case 'W': /* 32-bit character string */
	    W = va_arg(ap, wchar_t *);
	    len = wcslen(W);
	    wcstowc16s(buf, W, len);
	    fmt += 2;
	    break;

	default:  /* all other format strings */
	    /* extract single specifier (space ends it) */
	    spec_end = strchr(fmt, ' ');
	    if (spec_end == NULL) {
		spec_end = strchr(fmt + 1, '%');
	    }

	    if (spec_end == NULL) {
		spec_len = strlen(fmt);
	    } else {
		spec_len = spec_end - fmt;
	    }

	    memset(spec, 0, sizeof(spec));
	    strncpy(spec, fmt, spec_len);

	    if (strchr(spec, 'd') ||
		strchr(spec, 'i') ||
		strchr(spec, 'c')) {
		int arg = va_arg(ap, int);
		snprintf(spec_out, sizeof(spec_out), spec, arg);

	    } else if (strchr(spec, 'u') ||
		       strchr(spec, 'x') ||
		       strchr(spec, 'X') ||
		       strchr(spec, 'o') ) {
		unsigned int arg = va_arg(ap, unsigned int);
		snprintf(spec_out, sizeof(spec_out), spec, arg);

	    } else if (strchr(spec, 'f') ||
		       strchr(spec, 'F') ||
		       strchr(spec, 'e') ||
		       strchr(spec, 'E') ||
		       strchr(spec, 'g') ||
		       strchr(spec, 'G') ||
		       strchr(spec, 'a') ||
		       strchr(spec, 'A')) {
		double arg = va_arg(ap, double);
		snprintf(spec_out, sizeof(spec_out), spec, arg);

	    }

	    len = strlen(spec_out);
	    mbstowc16s(buf, spec_out, len);
	    fmt += spec_len;
	}

	buf += len;
    }
    
    va_end(ap);

    /* terminate the resulting string */
    *buf = (wchar16_t) 0;

    /* free what's been duplicated as fmt */
    if (f) free(f);
    return (int)(buf - out);
}

//Deprecated
int printfw16(const char *format, ...)
{
    va_list ap;
    char c;
    size_t len, total;
    char *s;
    wchar16_t *S;
    wchar_t *W;
    char *fmt, *f, *out;
    size_t spec_len;
    char *spec_end, spec[64];
    char spec_out[64];    /* larger than any number "printf-ed" */

    len   = 0;
    total = 0;

    fmt = strdup(format);
    if (fmt == NULL) return -1;

    f = fmt;
    va_start(ap, format);

    while (*fmt) {
	if ((*fmt) != '%') {
	    c = *fmt;
	    printf("%c", c);
	    fmt++;
	    continue;
	}

	/* get the next character */
	c = *(fmt + 1);

	switch (c) {
	case 's': /* byte character string */
	    s = va_arg(ap, char *);
	    len = strlen(s);
	    printf("%s", s);
	    fmt += 2;
	    break;
	    
	case 'S': /* 16-bit character string */
	    S = va_arg(ap, wchar16_t *);
	    len = wc16slen(S);
	    out = (char*) malloc(len + 1);
	    wc16stombs(out, S, len + 1);
	    printf("%s", out);
	    free(out);
	    fmt += 2;
	    break;
	
	case 'W': /* 32-bit character string */
	    W = va_arg(ap, wchar_t *);
	    len = wcslen(W);
	    out = (char*) malloc(len + 1);
	    wcstombs(out, W, len + 1);
	    printf("%s", out);
	    free(out);
	    fmt += 2;
	    break;

	default:  /* all other format strings */
	    /* extract single specifier (space ends it) */
	    spec_end = strchr(fmt, ' ');
	    if (spec_end == NULL) {
		spec_end = strchr(fmt + 1, '%');
	    }

	    if (spec_end == NULL) {
		spec_len = strlen(fmt);
	    } else {
		spec_len = spec_end - fmt;
	    }

	    memset(spec, 0, sizeof(spec));
	    strncpy(spec, fmt, spec_len);

	    if (strchr(spec, 'd') ||
		strchr(spec, 'i') ||
		strchr(spec, 'c')) {
		int arg = va_arg(ap, int);
		snprintf(spec_out, sizeof(spec_out), spec, arg);
		printf("%s", spec_out);
		len = strlen(spec_out);

	    } else if (strchr(spec, 'u') ||
		       strchr(spec, 'x') ||
		       strchr(spec, 'X') ||
		       strchr(spec, 'o') ) {
		unsigned int arg = va_arg(ap, unsigned int);
		snprintf(spec_out, sizeof(spec_out), spec, arg);
		printf("%s", spec_out);
		len = strlen(spec_out);

	    } else if (strchr(spec, 'f') ||
		       strchr(spec, 'F') ||
		       strchr(spec, 'e') ||
		       strchr(spec, 'E') ||
		       strchr(spec, 'g') ||
		       strchr(spec, 'G') ||
		       strchr(spec, 'a') ||
		       strchr(spec, 'A')) {
		double arg = va_arg(ap, double);
		snprintf(spec_out, sizeof(spec_out), spec, arg);
		printf("%s", spec_out);
		len = strlen(spec_out);

	    }

	    fmt += spec_len;
	}

	total += len;
    }
    
    va_end(ap);

    /* free what's been duplicated as fmt */
    if (f) free(f);
    return (int)total;
}
