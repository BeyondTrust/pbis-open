/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */
/*
 * Copyright Likewise Software    2004-2011
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
 *          David Leimbach <dleimbach@likewise.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <wchar16.h>
#include <wc16str.h>
#include "lwprintf.h"
#include <lw/rtlgoto.h>
#include <lw/types.h>
#include <lw/rtlmemory.h>
#include <limits.h>
#include <iconv.h>
#include <inttypes.h> // for PRIxPTR format string

#ifdef HAVE_SYS_VARARGS_H
#include <sys/varargs.h>
#endif

// % <flag>* <field width>? <precision>? <length modifier>? <conversion specifier>
// flag may be one of:
#define LW_FLAG_ALT_FORM           1
//  '#'
#define LW_FLAG_ZERO_PAD           2
//  '0'
#define LW_FLAG_LEFT_JUSTIFY       4
//  '-'
#define LW_FLAG_SPACE_FOR_POSITIVE 8
//  ' '
#define LW_FLAG_PLUS_FOR_POSITIVE  16
//  '+'
#define LW_FLAG_COMMA_SEPARATE     32
//  '\''
#define LW_FLAG_LOCALE_DIGITS      64
//  'I'
//
#define LW_FLAG_UPPER_CASE         128
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
#define LW_LENGTH_UNSET            0
#define LW_LENGTH_CHAR             1
//  hh
#define LW_LENGTH_SHORT            2
//  h
#define LW_LENGTH_LONG             3
//  l
#define LW_LENGTH_LONG_LONG        4
//  ll,q
#define LW_LENGTH_LONG_DOUBLE      5
//  L
#define LW_LENGTH_INTMAX_T         6
//  j
#define LW_LENGTH_SIZE_T           7
//  z
#define LW_LENGTH_PTRDIFF_T        8
//  t
#define LW_LENGTH_W16CHAR_T        9
//  w
//
// conversion specifier:
#define LW_TYPE_INTEGER            1
//  d,i
#define LW_TYPE_OCTAL              2
//  o
#define LW_TYPE_UNSIGNED_INTEGER   3
//  u
#define LW_TYPE_HEX          4
//  x,X
#define LW_TYPE_SCIENTIFIC   6
//  e,E
#define LW_TYPE_DOUBLE             8
//  f,F
#define LW_TYPE_AUTO_DOUBLE  10
//  g,G
#define LW_TYPE_HEX_DOUBLE   11
//  a,A
#define LW_TYPE_CHAR               13
//  c
#define LW_TYPE_STRING             14
//  s
#define LW_TYPE_POINTER            15
//  p
#define LW_TYPE_WRITTEN_COUNT      16
//  n
#define LW_TYPE_STRERROR           17
//  m
#define LW_TYPE_PERCENT            18
//  %
#define LW_TYPE_NTSTRING           19
//  Z
#define LW_TYPE_INVALID            20

/* UCS-2, little endian byte order
 * Note that UCS-2 without LE will
 * default to big endian on FreeBSD
 */
#if defined(LW_BIG_ENDIAN)
#define WINDOWS_ENCODING "UCS-2BE"
#else
#define WINDOWS_ENCODING "UCS-2LE"
#endif

#define LW_PRINTF_ALLOCATE(ppMemory, Type, Size) \
   LW_RTL_ALLOCATE(ppMemory, Type, Size) == LW_STATUS_SUCCESS ? 0 : ENOMEM

static
size_t
Wchar16sToUtf8s(
    OUT char* dest,
    IN const wchar16_t* src,
    IN size_t cbcopy)
{
    iconv_t handle = iconv_open("UTF-8", WINDOWS_ENCODING);
    char* inbuf = (char *)src;
    char* outbuf = (char *)dest;
    size_t cbin = wc16slen(src) * sizeof(src[0]);
    size_t cbout = cbcopy;
    size_t converted;
    size_t error = 0;

    converted = iconv(handle, (ICONV_IN_TYPE)&inbuf, &cbin, &outbuf, &cbout);

    if(cbout >= sizeof(dest[0]))
    {
        *(char*)outbuf = 0;
    }
    iconv_close(handle);
    if(converted == (size_t) - 1 && cbout !=0)
    {
        error = (size_t) - 1;
        GOTO_CLEANUP();
    }
    else
    {
        error = cbcopy - cbout / sizeof(src[0]);
    }

cleanup:
    return error;
}

static
size_t
WcharsToUtf8s(
    OUT char* pDest,
    IN const wchar_t* pSrc,
    IN size_t numSrcChars,
    IN size_t available)
{
    size_t converted = (size_t)-1;
    size_t amountLeft = available;
    size_t sIn =  numSrcChars * sizeof(wchar_t);
    char* inBuf = (char *) pSrc;
    char* outBuf = (char *) pDest;
    iconv_t cd = iconv_open("UTF-8", "WCHAR_T");

    if (cd == (iconv_t) -1)
    {
       GOTO_CLEANUP();
    }

    converted = iconv(cd, (ICONV_IN_TYPE)&inBuf, &sIn, &outBuf, &amountLeft);
    if (converted == (size_t) -1)
    {
        GOTO_CLEANUP();
    }

    if (amountLeft >= sizeof(char))
    {
        *outBuf = 0;
    }

    iconv_close(cd);

cleanup:
    return numSrcChars - sIn / sizeof(wchar_t);
}


//
// Must free the return value if not NULL
//
static
char*
AWcharsToUtf8s(
    IN const wchar_t* istr)
{
    size_t inSize = 0;
    size_t remainSize = 0;
    char* output = NULL;
    size_t allocSize = 80;  //Just a default amount to start with
    char* outputPtr = 0;
    char** inputPtr = 0;
    iconv_t cd = iconv_open("UTF-8", "WCHAR_T");
    int error = 0;

    if (cd == (iconv_t) -1)
    {
        GOTO_CLEANUP();
    }

    // string length plus the wchar_t NULL
    inSize = (wcslen(istr) * sizeof(wchar_t)) + sizeof(wchar_t);
    remainSize = inSize;

    do
    {
        size_t iconvOut = allocSize - inSize;
        size_t nonReversable = 0;

        output = realloc(output, allocSize);
        if (!output) {
            GOTO_CLEANUP();
        }
        outputPtr = output;
        outputPtr += inSize - remainSize;
        inputPtr = (char **) ((void *) &istr);

        nonReversable = iconv(cd, (ICONV_IN_TYPE)inputPtr, &remainSize, &outputPtr, &iconvOut);
        if (nonReversable == (size_t) -1)
        {
            if (errno != E2BIG)
            {
                error = errno;
                GOTO_CLEANUP();
            }
        }
        allocSize <<= 1;
    } while(remainSize > 0);

    GOTO_CLEANUP();

cleanup:
    if (error != 0)
    {
        RTL_FREE(&output);
        output = NULL;
    }

    if (cd != (iconv_t) -1)
    {
      iconv_close(cd);
    }

    return output;
}

typedef struct _LW_PRINTF_BUFFER LW_PRINTF_BUFFER, *PLW_PRINTF_BUFFER;

typedef int (*LW_PFNBUFFER_WRITE_WCS)(
    PLW_PRINTF_BUFFER pBuffer,
    const wchar_t *pwszBuffer,
    size_t cchBuffer);
typedef int (*LW_PFNBUFFER_WRITE_WC16S)(
    PLW_PRINTF_BUFFER pBuffer,
    const wchar16_t *pwszBuffer,
    size_t cchBuffer);
typedef int (*LW_PFNBUFFER_WRITE_MBS)(
    PLW_PRINTF_BUFFER pBuffer,
    const char *pszBuffer,
    size_t cchBuffer);
typedef int (*LW_PFNBUFFER_WRITE_UCS)(
    PLW_PRINTF_BUFFER pBuffer,
    PUNICODE_STRING Us);
typedef int (*LW_PFNBUFFER_WRITE_AS)(
    PLW_PRINTF_BUFFER pBUffer,
    PANSI_STRING As);

struct _LW_PRINTF_BUFFER
{
    size_t sWrittenCount;
    LW_PFNBUFFER_WRITE_WCS pfnWriteWcs;
    LW_PFNBUFFER_WRITE_WC16S pfnWriteWc16s;
    LW_PFNBUFFER_WRITE_MBS pfnWriteMbs;
    LW_PFNBUFFER_WRITE_UCS pfnWriteUcs;
    LW_PFNBUFFER_WRITE_AS pfnWriteAs;
};

static
int
WriteSpaces(
    IN OUT PLW_PRINTF_BUFFER pBuffer,
    IN ssize_t sCount
    )
{
    char space = ' ';
    int error = 0;

    while (sCount > 0)
    {
        error = pBuffer->pfnWriteMbs(pBuffer, &space, 1);

        GOTO_CLEANUP_ON_ERRNO(error);

        sCount--;
    }
cleanup:
    return error;
}

static
unsigned int
ParseFlags(
    IN const char** ppszPos
    )
{
    const char* pszPos = *ppszPos;
    int bFoundOption = 1;
    unsigned int dwFlags = 0;

    while (bFoundOption)
    {
        switch (*pszPos++)
        {
            case '#':
                dwFlags |= LW_FLAG_ALT_FORM;
                break;
            case '0':
                dwFlags |= LW_FLAG_ZERO_PAD;
                break;
            case '-':
                dwFlags |= LW_FLAG_LEFT_JUSTIFY;
                break;
            case ' ':
                dwFlags |= LW_FLAG_SPACE_FOR_POSITIVE;
                break;
            case '+':
                dwFlags |= LW_FLAG_PLUS_FOR_POSITIVE;
                break;
#if 0 // We are not supporting these yet, if ever (David Leimbach 3/3/2011)
            case '\'':
                dwFlags |= LW_FLAG_COMMA_SEPARATE;
                break;
            case 'I':
                dwFlags |= LW_FLAG_LOCALE_DIGITS;
                break;
#endif
            default:
                bFoundOption = 0;
                pszPos--;
                break;
        }
    }

    *ppszPos = pszPos;
    return dwFlags;
}

static
unsigned int
ParseLengthModifier(
    IN const char** ppszPos
    )
{
    const char* pszPos = *ppszPos;
    unsigned int dwLengthModifier = LW_LENGTH_UNSET;

    switch (*pszPos)
    {
        case 'h':
            if (pszPos[1] == 'h')
            {
                dwLengthModifier = LW_LENGTH_CHAR;
                pszPos += 2;
            }
            else
            {
                dwLengthModifier = LW_LENGTH_SHORT;
                pszPos++;
            }
            break;
        case 'l':
            if (pszPos[1] == 'l')
            {
                dwLengthModifier = LW_LENGTH_LONG_LONG;
                pszPos += 2;
            }
            else
            {
                dwLengthModifier = LW_LENGTH_LONG;
                pszPos++;
            }
            break;
        case 'q':
            dwLengthModifier = LW_LENGTH_LONG_LONG;
            pszPos++;
            break;
        case 'L':
            dwLengthModifier = LW_LENGTH_LONG_DOUBLE;
            pszPos++;
            break;
        case 'j':
            dwLengthModifier = LW_LENGTH_INTMAX_T;
            pszPos++;
            break;
        case 'z':
            dwLengthModifier = LW_LENGTH_SIZE_T;
            pszPos++;
            break;
        case 't':
            dwLengthModifier = LW_LENGTH_PTRDIFF_T;
            pszPos++;
            break;
        case 'w':
            dwLengthModifier = LW_LENGTH_W16CHAR_T;
            pszPos++;
            break;
    }

    *ppszPos = pszPos;
    return dwLengthModifier;
}

static
unsigned int
ParseType(
    IN const char** ppszPos,
    OUT unsigned int* pdwFlags
    )
{
    const char* pszPos = *ppszPos;
    unsigned int dwType = LW_TYPE_INVALID;

    switch (*pszPos++)
    {
        case 'd':
        case 'i':
            dwType = LW_TYPE_INTEGER;
            break;
        case 'o':
            dwType = LW_TYPE_OCTAL;
            break;
        case 'u':
            dwType = LW_TYPE_UNSIGNED_INTEGER;
            break;
        case 'X':
            *pdwFlags |= LW_FLAG_UPPER_CASE;
        case 'x':
            dwType = LW_TYPE_HEX;
            break;
        case 'E':
            *pdwFlags |= LW_FLAG_UPPER_CASE;
        case 'e':
            dwType = LW_TYPE_SCIENTIFIC;
            break;
#if 0
        case 'F':
#endif
            // Upper case F doesn't do anything special
        case 'f':
            dwType = LW_TYPE_DOUBLE;
            break;
        case 'G':
            *pdwFlags |= LW_FLAG_UPPER_CASE;
        case 'g':
            dwType = LW_TYPE_AUTO_DOUBLE;
            break;
#if 0
        case 'A':
            *pdwFlags |= LW_FLAG_UPPER_CASE;
        case 'a':
            dwType = LW_TYPE_HEX_DOUBLE;
            break;
#endif
        case 'c':
            dwType = LW_TYPE_CHAR;
            break;
        case 's':
            dwType = LW_TYPE_STRING;
            break;
        case 'p':
            dwType = LW_TYPE_POINTER;
            break;
        case 'n':
            dwType = LW_TYPE_WRITTEN_COUNT;
            break;
#if 0
        case 'm':
            dwType = LW_TYPE_STRERROR;
            break;
#endif
        case 'Z':
            dwType = LW_TYPE_NTSTRING;
            break;
        case '%':
            dwType = LW_TYPE_PERCENT;
            break;
        default:
            pszPos--;
            dwType = LW_TYPE_INVALID;
            break;
    }

    *ppszPos = pszPos;
    return dwType;
}

static
int
PrintfCore(
    IN OUT PLW_PRINTF_BUFFER pBuffer,
    IN const char* pszFormat,
    IN va_list args
    )
{
    size_t sLen = 0;
    const char* pszPos = pszFormat;
    const char* pszTokenStart = pszFormat;
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
    char szFormatBuffer[LW_TYPE_INVALID + 1];
    char *pszFormatBufferPos = NULL;
    int error = 0;

    while (*pszPos)
    {
        if (*pszPos == '%')
        {
            error = pBuffer->pfnWriteMbs(
                        pBuffer,
                        pszTokenStart,
                        pszPos - pszTokenStart);

            GOTO_CLEANUP_ON_ERRNO(error);

            pszPos++;

            // Put the format specifier (% included) into a multibyte
            // string in case regular printf needs to be called
            pszFormatBufferPos = szFormatBuffer;
            *pszFormatBufferPos++ = '%';

            pszTokenStart = pszPos;
            dwFlags = ParseFlags(&pszPos);
            // update multibyte format
            // subtract for % * . * null (=5)
            if (pszPos - pszTokenStart > sizeof(szFormatBuffer) - 5)
            {
                error = ENOMEM;
                GOTO_CLEANUP();
            }

            sLen = pszPos - pszTokenStart;
            memcpy((void *)pszFormatBufferPos,
                   (void *)pszTokenStart,
                   sLen);

            pszFormatBufferPos += sLen;

            if (*pszPos == '*')
            {
                pszPos++;
                sWidth = va_arg(args, size_t);
            }
            else
            {
              char** pszEnd = (char **)&pszPos;
              sWidth = (size_t)strtoul(pszPos, pszEnd, 10);
            }
            *pszFormatBufferPos++ = '*';

            iPrecision = -1;
            if (*pszPos == '.')
            {
                pszPos++;
                if (*pszPos == '*')
                {
                    pszPos++;
                    iPrecision = va_arg(args, ssize_t);
                }
                else if (isdigit((int)*pszPos))
                {
                    char ** pszEnd = (char **)&pszPos;
                    iPrecision = (ssize_t)strtoul(pszPos, pszEnd, 10);
                }
            }
            *pszFormatBufferPos++ = '.';
            *pszFormatBufferPos++ = '*';

            pszTokenStart = pszPos;

            dwLengthModifier = ParseLengthModifier(&pszPos);
            dwType = ParseType(&pszPos, &dwType);

            if (pszFormatBufferPos - szFormatBuffer +
                    pszPos - pszTokenStart > sizeof(szFormatBuffer) - 1)
            {
                error = ENOMEM;
                GOTO_CLEANUP();
            }

            sLen = pszPos - pszTokenStart;
            memcpy((void *)pszFormatBufferPos,
                   (void *)pszTokenStart,
                   sLen);

            pszFormatBufferPos += sLen;
            *pszFormatBufferPos = 0;

            switch (dwType)
            {
                case LW_TYPE_OCTAL:
                case LW_TYPE_UNSIGNED_INTEGER:
                case LW_TYPE_HEX:
                case LW_TYPE_INTEGER:
                    if (dwLengthModifier == LW_LENGTH_LONG_LONG)
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
                        error = ENOMEM;
                        GOTO_CLEANUP();
                    }

                    error = pBuffer->pfnWriteMbs(pBuffer, szArgBuffer, sLen);

                    GOTO_CLEANUP_ON_ERRNO(error);

                    break;

                case LW_TYPE_DOUBLE:
                case LW_TYPE_AUTO_DOUBLE:
#if 0
                case LW_TYPE_HEX_DOUBLE:
#endif
                case LW_TYPE_SCIENTIFIC:
                    if (dwLengthModifier == LW_LENGTH_LONG_DOUBLE)
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
                        error = ENOMEM;
                        GOTO_CLEANUP();
                    }
                    error = pBuffer->pfnWriteMbs(pBuffer, szArgBuffer, sLen);

                    GOTO_CLEANUP_ON_ERRNO(error);
                    break;

                case LW_TYPE_CHAR:
                    {
                        wint_t iArg = va_arg(args, wint_t);
                        if (!(dwFlags & LW_FLAG_LEFT_JUSTIFY))
                        {
                            error = WriteSpaces(pBuffer, (ssize_t)sWidth - 1);

                            GOTO_CLEANUP_ON_ERRNO(error);
                        }
                        switch (dwLengthModifier)
                        {
                            case LW_LENGTH_UNSET:
                            case LW_LENGTH_SHORT:
                            case LW_LENGTH_CHAR:
                                szArgBuffer[0] = (char)iArg;
                                error = pBuffer->pfnWriteMbs(
                                            pBuffer,
                                            szArgBuffer,
                                            1);

                                GOTO_CLEANUP_ON_ERRNO(error);
                                break;
                            case LW_LENGTH_W16CHAR_T:
                                w16szArgBuffer[0] = iArg;
                                error = pBuffer->pfnWriteWc16s(
                                            pBuffer,
                                            w16szArgBuffer,
                                            1);

                                GOTO_CLEANUP_ON_ERRNO(error);
                                break;
                            case LW_LENGTH_LONG:
                                wszArgBuffer[0] = iArg;
                                error = pBuffer->pfnWriteWcs(
                                            pBuffer,
                                            wszArgBuffer,
                                            1);

                                GOTO_CLEANUP_ON_ERRNO(error);
                                break;
                            case LW_LENGTH_LONG_LONG:
                            case LW_LENGTH_LONG_DOUBLE:
                            case LW_LENGTH_INTMAX_T:
                            case LW_LENGTH_SIZE_T:
                            case LW_LENGTH_PTRDIFF_T:
                            default:
                                error = EINVAL;
                                GOTO_CLEANUP();
                        }
                        if (dwFlags & LW_FLAG_LEFT_JUSTIFY)
                        {
                            error = WriteSpaces(pBuffer, (ssize_t)sWidth - 1);

                            GOTO_CLEANUP_ON_ERRNO(error);
                        }
                    }
                    break;
                case LW_TYPE_STRING:
                    {
                        void* pvArg = va_arg(args, void *);
                        if (pvArg == NULL)
                        {
                            pvArg = "(null)";

                            dwLengthModifier = LW_LENGTH_CHAR;
                        }
                        switch (dwLengthModifier)
                        {
                            case LW_LENGTH_UNSET:
                            case LW_LENGTH_SHORT:
                            case LW_LENGTH_CHAR:
                                sLen = strlen((char *)pvArg);
                                break;
                            case LW_LENGTH_W16CHAR_T:
                                sLen = wc16slen((wchar16_t *)pvArg);
                                break;
                            case LW_LENGTH_LONG:
                                sLen = wcslen((wchar_t *)pvArg);
                                break;
                            case LW_LENGTH_LONG_LONG:
                            case LW_LENGTH_LONG_DOUBLE:
                            case LW_LENGTH_INTMAX_T:
                            case LW_LENGTH_SIZE_T:
                            case LW_LENGTH_PTRDIFF_T:
                            default:
                                error = EINVAL;
                                GOTO_CLEANUP();
                        }
                        if (iPrecision >= 0 && sLen > (size_t)iPrecision)
                        {
                            sLen = iPrecision;
                        }
                        if (!(dwFlags & LW_FLAG_LEFT_JUSTIFY))
                        {
                            error = WriteSpaces(pBuffer, (ssize_t)sWidth - (ssize_t)sLen);

                            GOTO_CLEANUP_ON_ERRNO(error);
                        }
                        switch (dwLengthModifier)
                        {
                            case LW_LENGTH_UNSET:
                            case LW_LENGTH_CHAR:
                                error = pBuffer->pfnWriteMbs(
                                            pBuffer,
                                            (char *)pvArg,
                                            sLen);

                                GOTO_CLEANUP_ON_ERRNO(error);
                                break;
                            case LW_LENGTH_SHORT:
                            case LW_LENGTH_W16CHAR_T:
                                error = pBuffer->pfnWriteWc16s(
                                            pBuffer,
                                            (wchar16_t *)pvArg,
                                            sLen);

                                GOTO_CLEANUP_ON_ERRNO(error);
                                break;
                            case LW_LENGTH_LONG:
                                error = pBuffer->pfnWriteWcs(
                                            pBuffer,
                                            (wchar_t *)pvArg,
                                            sLen);

                                GOTO_CLEANUP_ON_ERRNO(error);
                                break;
                            //Invalid cases have already been checked for
                        }
                        if (dwFlags & LW_FLAG_LEFT_JUSTIFY)
                        {
                            error = WriteSpaces(pBuffer, (ssize_t)(sWidth - sLen));

                            GOTO_CLEANUP_ON_ERRNO(error);
                        }
                    }
                    break;
                case LW_TYPE_POINTER:
                    {
                        void *pvArg = va_arg(args, void *);
#if 0
    // OLD version, behaves like standard %p
                        sLen = snprintf(
                                    szArgBuffer,
                                    sizeof(szArgBuffer),
                                    szFormatBuffer,
                                    sWidth,
                                    iPrecision,
                                    pvArg);
#endif
    // New version, prints out a pointer as an unsigned hex,
    // 0 padded to the right amount for the 32bit or 64bit platform in question
                        iPrecision = sWidth = (sizeof(void *) == 4 ? 8 : 16);
                        sLen = snprintf(
                                      szArgBuffer,
                                      sizeof(szArgBuffer),
                                      "%*.*lx",
                                      (int)sWidth,
                                      (int)iPrecision,
                                      (unsigned long int)pvArg);
                        if (sLen > sizeof(szArgBuffer) || (ssize_t) sLen < 0)
                        {
                            error = ENOMEM;
                            GOTO_CLEANUP();
                        }
                        error = pBuffer->pfnWriteMbs(pBuffer, szArgBuffer, sLen);

                        GOTO_CLEANUP_ON_ERRNO(error);
                    }
                    break;
                case LW_TYPE_WRITTEN_COUNT:
                    {
                        size_t *psArg = va_arg(args, size_t *);
                        if (psArg == NULL)
                        {
                            error = EINVAL;
                            GOTO_CLEANUP();
                        }
                        *psArg = pBuffer->sWrittenCount;
                    }
                    break;
#if 0
                case LW_TYPE_STRERROR:
                    {
                        sLen = snprintf(
                                    szArgBuffer,
                                    sizeof(szArgBuffer),
                                    szFormatBuffer,
                                    sWidth,
                                    iPrecision);
                        if (sLen > sizeof(szArgBuffer) || (ssize_t) sLen < 0)
                        {
                            error = ENOMEM;
                            GOTO_CLEANUP();
                        }
                        error = pBuffer->pfnWriteMbs(pBuffer, szArgBuffer, sLen);

                        GOTO_CLEANUP_ON_ERRNO(error);
                    }
                    break;
#endif
                case LW_TYPE_PERCENT:
                    w16szArgBuffer[0] = '%';
                    error = pBuffer->pfnWriteWc16s(pBuffer, w16szArgBuffer, 1);

                    GOTO_CLEANUP_ON_ERRNO(error);
                    break;
                case LW_TYPE_NTSTRING:
                    switch (dwLengthModifier)
                    {
                            // ANSI_STRING
                            case LW_LENGTH_UNSET:
                            case LW_LENGTH_CHAR:
                            {
                                PANSI_STRING as = va_arg(args, PANSI_STRING);
                                if (as == NULL)
                                {
                                    error = pBuffer->pfnWriteMbs(pBuffer, "(null)", sizeof("(null)"));

                                    GOTO_CLEANUP_ON_ERRNO(error);
                                }
                                else
                                {
                                    error = pBuffer->pfnWriteAs(pBuffer, as);

                                    GOTO_CLEANUP_ON_ERRNO(error);
                                }
                            }
                            break;
                            // UNICODE_STRING
                            case LW_LENGTH_SHORT:
                            case LW_LENGTH_LONG:
                            case LW_LENGTH_W16CHAR_T:
                            {
                                PUNICODE_STRING us = va_arg(args, PUNICODE_STRING);
                                if (us == NULL)
                                {
                                    error = pBuffer->pfnWriteMbs(pBuffer, "(null)", sizeof("(null)"));

                                    GOTO_CLEANUP_ON_ERRNO(error);
                                }
                                else
                                {
                                    error = pBuffer->pfnWriteUcs(pBuffer, us);

                                    GOTO_CLEANUP_ON_ERRNO(error);
                                }
                            }
                            break;
                            // Invalid cases have already been checked for
                        }
                    break;
                case LW_TYPE_INVALID:
                    error = EINVAL;
                    GOTO_CLEANUP();
            }
            pszTokenStart = pszPos;
        }
        else
        {
            pszPos++;
        }
    }

    error = pBuffer->pfnWriteMbs(pBuffer, pszTokenStart, pszPos - pszTokenStart);

    GOTO_CLEANUP_ON_ERRNO(error);

cleanup:
    return error;
}

//
// Generic string output buffer for all types of sprintf style functions
//
typedef struct _LW_STRING_PRINTF_BUFFER
{
    LW_PRINTF_BUFFER parent;
    void* pBuffer;
    size_t sAvailable;
} LW_STRING_PRINTF_BUFFER, *PLW_STRING_PRINTF_BUFFER;

//
// These output wchar16_t buffers
//
static
int
StringPrintfWriteWcsWc16s(
    IN OUT PLW_STRING_PRINTF_BUFFER pBuffer,
    IN const wchar_t* pwszWrite,
    IN size_t cchWrite)
{
    size_t sConverted;
    int error = 0;
    wchar16_t * wc16pBuffer = (wchar16_t *)pBuffer->pBuffer;

    if (wc16pBuffer)
    {
        if (cchWrite > pBuffer->sAvailable)
        {
            error = ENOMEM;
            GOTO_CLEANUP();
        }
        sConverted = wcstowc16s(wc16pBuffer, pwszWrite, cchWrite);
        if (sConverted != cchWrite)
        {
            error = errno;
            GOTO_CLEANUP();
        }
        wc16pBuffer += sConverted;
        pBuffer->pBuffer = (char *) wc16pBuffer;
        pBuffer->sAvailable -= sConverted;
    }
    pBuffer->parent.sWrittenCount += cchWrite;
cleanup:

    return error;
}

static
int
StringPrintfWriteMbsWc16s(
    IN OUT PLW_STRING_PRINTF_BUFFER pBuffer,
    IN const char* pszWrite,
    IN size_t cchWrite)
{
    size_t sConverted;
    int error = 0;
    wchar16_t * wc16pBuffer = (wchar16_t *)pBuffer->pBuffer;

    if (wc16pBuffer)
    {
        if (cchWrite > pBuffer->sAvailable)
        {
            error = ENOMEM;
            GOTO_CLEANUP();
        }

        sConverted = mbstowc16s(wc16pBuffer, pszWrite, cchWrite);
        if (sConverted == -1)
        {
            error = errno;
            GOTO_CLEANUP();
        }
        wc16pBuffer += sConverted;
        pBuffer->pBuffer = (char *) wc16pBuffer;
        pBuffer->sAvailable -= sConverted;
    }
    pBuffer->parent.sWrittenCount += cchWrite;
cleanup:
    return error;
}

static
int
StringPrintfWriteWc16sWc16s(
    IN OUT PLW_STRING_PRINTF_BUFFER pBuffer,
    IN const wchar16_t* pwszWrite,
    IN size_t cchWrite)
{
    ssize_t sConverted;
    int error = 0;
    wchar16_t* wc16pBuffer = pBuffer->pBuffer;


    if (wc16pBuffer && cchWrite)
    {
        if (cchWrite > pBuffer->sAvailable)
        {
            error = ENOMEM;
            GOTO_CLEANUP();
        }

        sConverted = wc16pncpy(wc16pBuffer, pwszWrite, cchWrite) -
                              wc16pBuffer + 1;
        if (sConverted != cchWrite)
        {
            error = errno;
            GOTO_CLEANUP();
        }
        wc16pBuffer += sConverted;
        pBuffer->pBuffer = (char *) wc16pBuffer;
        pBuffer->sAvailable -= sConverted;
    }
    pBuffer->parent.sWrittenCount += cchWrite;
cleanup:
    return error;
}

static
int
StringPrintfWriteUcsWc16s(
    IN OUT PLW_STRING_PRINTF_BUFFER pBuffer,
    IN PUNICODE_STRING pUs)
{
    // UNICODE_STRING length is actually bytes, not a true string length in chars
    return StringPrintfWriteWc16sWc16s(pBuffer, pUs->Buffer, (size_t)pUs->Length/2);
}

static
int
StringPrintfWriteAsWc16s(
    IN OUT PLW_STRING_PRINTF_BUFFER pBuffer,
    IN PANSI_STRING pAs)
{
    return StringPrintfWriteMbsWc16s(pBuffer, pAs->Buffer, (size_t)pAs->Length);
}

//
// These output char based buffers
//
static
int
StringPrintfWriteWcsMbs(
    IN OUT PLW_STRING_PRINTF_BUFFER pBuffer,
    IN const wchar_t* pwszWrite,
    IN size_t cchWrite)
{
    size_t sConverted;
    int error = 0;
    char * utf8pBuffer = pBuffer->pBuffer;

    if (utf8pBuffer)
    {
        if (cchWrite > pBuffer->sAvailable)
        {
            error = ENOMEM;
            GOTO_CLEANUP();
        }

        // write at most the available bytes.
        sConverted = WcharsToUtf8s(utf8pBuffer, pwszWrite, cchWrite, pBuffer->sAvailable);
        if (sConverted != cchWrite)
        {
            error = errno;
            GOTO_CLEANUP();
        }

        utf8pBuffer += strlen(utf8pBuffer);
        pBuffer->pBuffer = utf8pBuffer;
        pBuffer->sAvailable -= sConverted;
    }
    pBuffer->parent.sWrittenCount += cchWrite;
cleanup:
    return error;
}

static
int
StringPrintfWriteMbsMbs(
    IN OUT PLW_STRING_PRINTF_BUFFER pBuffer,
    IN const char* pszWrite,
    IN size_t cchWrite)
{
    char* utf8pBuffer = pBuffer->pBuffer;
    int error = 0;

    if (utf8pBuffer)
    {
        if (cchWrite > pBuffer->sAvailable)
        {
            error = ENOMEM;
            GOTO_CLEANUP();
        }
        strncpy(utf8pBuffer, pszWrite, cchWrite);

        utf8pBuffer += cchWrite;
        pBuffer->pBuffer = utf8pBuffer;
        pBuffer->sAvailable -= cchWrite;
    }
    pBuffer->parent.sWrittenCount += cchWrite;
cleanup:
    return error;
}

static
int
StringPrintfWriteWc16sMbs(
    IN OUT PLW_STRING_PRINTF_BUFFER pBuffer,
    IN const wchar16_t* pwszWrite,
    IN size_t cchWrite)
{
    ssize_t sConverted;
    int error = 0;
    char* utf8pBuffer = pBuffer->pBuffer;

    if (utf8pBuffer && cchWrite)
    {
        if (cchWrite > pBuffer->sAvailable)
        {
            error = ENOMEM;
            GOTO_CLEANUP();
        }

        sConverted = Wchar16sToUtf8s(utf8pBuffer, pwszWrite, cchWrite);
        if (sConverted != cchWrite)
        {
            error = errno;
            GOTO_CLEANUP();
        }
        utf8pBuffer += sConverted;
        pBuffer->pBuffer = utf8pBuffer;
        pBuffer->sAvailable -= sConverted;
    }
    pBuffer->parent.sWrittenCount += cchWrite;
cleanup:
    return error;
}

static
int
StringPrintfWriteUcsMbs(
    IN OUT PLW_STRING_PRINTF_BUFFER pBuffer,
    IN PUNICODE_STRING pUs)
{
    return StringPrintfWriteWc16sMbs(pBuffer, pUs->Buffer, (size_t)pUs->Length/2);
}


static
int
StringPrintfWriteAsMbs(
    IN OUT PLW_STRING_PRINTF_BUFFER pBuffer,
    IN PANSI_STRING pAs)
{
    return StringPrintfWriteMbsMbs(pBuffer, pAs->Buffer, (size_t)pAs->Length);
}


//
// Output Element: wchar16_t
// Format String element: wchar_t
//
int
LwPrintfW16StringWV(
    OUT wchar16_t* pOut,
    IN size_t MaxChars,
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN va_list Args
    )
{
    int error = 0;
    LW_STRING_PRINTF_BUFFER pOutput;
    char* utf8Format;

    memset(&pOutput, 0, sizeof(pOutput));

    pOutput.parent.pfnWriteWcs = (LW_PFNBUFFER_WRITE_WCS)StringPrintfWriteWcsWc16s;
    pOutput.parent.pfnWriteWc16s = (LW_PFNBUFFER_WRITE_WC16S)StringPrintfWriteWc16sWc16s;
    pOutput.parent.pfnWriteMbs = (LW_PFNBUFFER_WRITE_MBS)StringPrintfWriteMbsWc16s;
    pOutput.parent.pfnWriteUcs = (LW_PFNBUFFER_WRITE_UCS)StringPrintfWriteUcsWc16s;
    pOutput.parent.pfnWriteAs = (LW_PFNBUFFER_WRITE_AS)StringPrintfWriteAsWc16s;

    pOutput.pBuffer = (char *)pOut;
    pOutput.sAvailable = MaxChars;

    utf8Format = AWcharsToUtf8s(Format);

    if (utf8Format == NULL)
    {
        error = EINVAL;
        GOTO_CLEANUP();
    }
    error = PrintfCore(
                &pOutput.parent,
                utf8Format,
                Args);

    GOTO_CLEANUP_ON_ERRNO(error);

    if (!pOutput.pBuffer)
    {
        /* This is only calculating the maximum size. Do not include the null
         * in this calculation, and do not write anything.
         */
    }
    else if (pOutput.sAvailable)
    {
      /* terminate the resulting string */
      *(wchar16_t *)pOutput.pBuffer = 0;
    }
    else
    {
        error = ENOMEM;
        GOTO_CLEANUP();
    }

cleanup:
    if (utf8Format)
    {
        RTL_FREE(&utf8Format);
    }
    if (!error)
    {
        *pCharsOut = pOutput.parent.sWrittenCount;
    }
    return error;
}

int
LwPrintfW16StringW(
    OUT wchar16_t* pOut,
    IN size_t MaxChars,
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN ...
    )
{
    int error = 0;
    va_list ap;

    va_start(ap, Format);

    error = LwPrintfW16StringWV(
                    pOut,
                    MaxChars,
                    pCharsOut,
                    Format,
                    ap);
    va_end(ap);

    return error;
}

int
LwPrintfW16AllocateStringWV(
    OUT wchar16_t** pBuffer,
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN va_list Args
    )
{
    wchar16_t* pwszOut = NULL;
    int error = 0;
    size_t sLen = 0;
    va_list args2;

    va_copy(args2, Args);

    error = LwPrintfW16StringWV(
                    NULL,
                    0,
                    &sLen,
                    Format,
                    Args);

    GOTO_CLEANUP_ON_ERRNO(error);
    
    error = LW_PRINTF_ALLOCATE(&pwszOut, wchar16_t, (sLen + 1) * sizeof(wchar16_t));

    GOTO_CLEANUP_ON_ERRNO(error);

    error = LwPrintfW16StringWV(
                    pwszOut,
                    sLen + 1,
                    pCharsOut,
                    Format,
                    args2);

    GOTO_CLEANUP_ON_ERRNO(error);

cleanup:

    if (error)
    {
        RTL_FREE(&pwszOut);
    }

    va_end(args2);

    *pBuffer = pwszOut;

    return error;
}

int
LwPrintfW16AllocateStringW(
    OUT wchar16_t** pBuffer,
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN ...
    )
{
    va_list ap;
    int error = 0;

    va_start(ap, Format);
    error = LwPrintfW16AllocateStringWV(pBuffer, pCharsOut, Format, ap);
    va_end(ap);

    return error;
}


//
// Output Element: char
// Format String Element: wchar_t
//
int
LwPrintfStringWV(
    OUT char* pOut,
    IN size_t MaxChars,
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN va_list Args
    )
{
    int error = 0;
    LW_STRING_PRINTF_BUFFER pOutput;
    char * utf8Format;

    memset(&pOutput, 0, sizeof(pOutput));

    pOutput.parent.pfnWriteWcs = (LW_PFNBUFFER_WRITE_WCS)StringPrintfWriteWcsMbs;
    pOutput.parent.pfnWriteWc16s = (LW_PFNBUFFER_WRITE_WC16S)StringPrintfWriteWc16sMbs;
    pOutput.parent.pfnWriteMbs = (LW_PFNBUFFER_WRITE_MBS)StringPrintfWriteMbsMbs;
    pOutput.parent.pfnWriteUcs = (LW_PFNBUFFER_WRITE_UCS)StringPrintfWriteUcsMbs;
    pOutput.parent.pfnWriteAs = (LW_PFNBUFFER_WRITE_AS)StringPrintfWriteAsMbs;

    pOutput.pBuffer = pOut;
    pOutput.sAvailable = MaxChars;

    utf8Format = AWcharsToUtf8s(Format);

    if (utf8Format == NULL)
    {
        error = EINVAL;
        GOTO_CLEANUP();
    }
    error = PrintfCore(
                &pOutput.parent,
                utf8Format,
                Args);

    GOTO_CLEANUP_ON_ERRNO(error);

    if (!pOutput.pBuffer)
    {
        /* This is only calculating the maximum size. Do not include the null
         * in this calculation, and do not write anything.
         */
    }
    else if (pOutput.sAvailable)
    {
        /* terminate the resulting string */
        *(char *)pOutput.pBuffer = 0;
    }
    else
    {
        error = ENOMEM;
        GOTO_CLEANUP();
    }

cleanup:
    if (utf8Format)
    {
        RTL_FREE(&utf8Format);
    }
    if (!error)
    {
        *pCharsOut = pOutput.parent.sWrittenCount;
    }

    return error;
}

int
LwPrintfStringW(
    OUT char* pOut,
    IN size_t MaxChars,
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    ...
    )
{
    int error = 0;
    va_list ap;

    va_start(ap, Format);

    error = LwPrintfStringWV(
                    pOut,
                    MaxChars,
                    pCharsOut,
                    Format,
                    ap);

    va_end(ap);

    return error;
}

int
LwPrintfAllocateStringWV(
    OUT char** pBuffer,
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN va_list args
    )
{
    int error = 0;
    char* pszOut = NULL;
    size_t sLen = 0;
    va_list args2;

    va_copy(args2, args);

    error = LwPrintfStringWV(
                    NULL,
                    0,
                    &sLen,
                    Format,
                    args);

    GOTO_CLEANUP_ON_ERRNO(error);

    error = LW_PRINTF_ALLOCATE(&pszOut, char, (sLen + 1) * sizeof(char));

    GOTO_CLEANUP_ON_ERRNO(error);

    error = LwPrintfStringWV(
                    pszOut,
                    sLen + 1,
                    pCharsOut,
                    Format,
                    args2);

    GOTO_CLEANUP_ON_ERRNO(error);

cleanup:
    if (error)
    {
        RTL_FREE(&pszOut);
    }

    va_end(args2);

    *pBuffer = pszOut;

    return error;
}

int
LwPrintfAllocateStringW(
    OUT char** pBuffer,
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN ...
    )
{
    va_list ap;
    int error = 0;

    va_start(ap, Format);
    error = LwPrintfAllocateStringWV(pBuffer, pCharsOut, Format, ap);
    va_end(ap);

    return error;
}


// For STDERR and STDOUT and wchar_t formats
static
int
GenericPrintfWV(
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN va_list Args,
    IN FILE* OutputFile
    )
{
    int error = 0;
    char * pBuffer = NULL;
    size_t bufferSize = 0;

    error = LwPrintfAllocateStringWV(
        &pBuffer,
        &bufferSize,
        Format,
        Args);

    GOTO_CLEANUP_ON_ERRNO(error);

    fprintf(OutputFile, "%s", pBuffer);

cleanup:
    if (!error)
    {
        *pCharsOut = mbstowcs(NULL, pBuffer, 0);
    }
    else
    {
        RTL_FREE(&pBuffer);
    }

    return error;
}

// For STDERR and STDOUT and char formats
static
int
GenericPrintfV(
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN va_list Args,
    IN FILE* OutputFile
    )
{
    int error = 0;
    char*  pBuffer = NULL;
    size_t bufferSize = 0;

    error = LwPrintfAllocateStringV(
        &pBuffer,
        &bufferSize,
        Format,
        Args);

    GOTO_CLEANUP_ON_ERRNO(error);

    fprintf(OutputFile, "%s", pBuffer);

cleanup:

    if (!error)
    {
        *pCharsOut = mbstowcs(NULL, pBuffer, 0);
    }
    else
    {
        RTL_FREE(&pBuffer);
    }

    return error;
}

int
LwPrintfStdoutWV(
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN va_list Args
    )
{
    return GenericPrintfWV(pCharsOut, Format, Args, stdout);
}

int
LwPrintfStdoutW(
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN ...
    )
{
    int error = 0;
    va_list ap;

    va_start(ap, Format);

    error = LwPrintfStdoutWV(
                    pCharsOut,
                    Format,
                    ap);

    va_end(ap);

    return error;
}

int
LwPrintfStderrWV(
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN va_list Args
    )
{
    return GenericPrintfWV(pCharsOut, Format, Args, stderr);
}

int
LwPrintfStderrW(
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN ...
    )
{
    int error = 0;
    va_list ap;

    va_start(ap, Format);

    error = LwPrintfStderrWV(
                    pCharsOut,
                    Format,
                    ap);

    va_end(ap);

    return error;
}

//
// Output Element: wchar16_t
// Format String Element: char (UTF-8)
//
int
LwPrintfW16StringV(
    OUT wchar16_t* pOut,
    IN size_t MaxChars,
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN va_list Args
    )
{
    int error = 0;
    LW_STRING_PRINTF_BUFFER pOutput;

    memset(&pOutput, 0, sizeof(pOutput));

    pOutput.parent.pfnWriteWcs = (LW_PFNBUFFER_WRITE_WCS)StringPrintfWriteWcsWc16s;
    pOutput.parent.pfnWriteWc16s = (LW_PFNBUFFER_WRITE_WC16S)StringPrintfWriteWc16sWc16s;
    pOutput.parent.pfnWriteMbs = (LW_PFNBUFFER_WRITE_MBS)StringPrintfWriteMbsWc16s;
    pOutput.parent.pfnWriteUcs = (LW_PFNBUFFER_WRITE_UCS)StringPrintfWriteUcsWc16s;
    pOutput.parent.pfnWriteAs = (LW_PFNBUFFER_WRITE_AS)StringPrintfWriteAsWc16s;

    pOutput.pBuffer = pOut;
    pOutput.sAvailable = MaxChars;

    error = PrintfCore(
                &pOutput.parent,
                Format,
                Args);

    GOTO_CLEANUP_ON_ERRNO(error);

    if (!pOutput.pBuffer)
    {
        /* This is only calculating the maximum size. Do not include the null
         * in this calculation, and do not write anything.
         */
    }
    else if (pOutput.sAvailable)
    {
      // terminate the resulting string
      *(wchar16_t *)pOutput.pBuffer = 0;
    }
    else
    {
        error = ENOMEM;
        GOTO_CLEANUP();
    }

cleanup:
    if (!error)
    {
        *pCharsOut = pOutput.parent.sWrittenCount;
    }
    return error;
}

int
LwPrintfW16String(
    OUT wchar16_t* pOut,
    IN size_t MaxChars,
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN ...
    )
{
    int error = 0;
    va_list ap;

    va_start(ap, Format);

    error = LwPrintfW16StringV(
                    pOut,
                    MaxChars,
                    pCharsOut,
                    Format,
                    ap);
    va_end(ap);

    return error;
}

int
LwPrintfW16AllocateStringV(
    OUT wchar16_t** pBuffer,
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN va_list Args
    )
{
    int error = 0;
    wchar16_t *pwszOut = NULL;
    size_t sLen = 0;
    va_list args2;

    va_copy(args2, Args);

    error = LwPrintfW16StringV(
        NULL,
        0,
        &sLen,
        Format,
        Args);

    GOTO_CLEANUP_ON_ERRNO(error);

    error = LW_PRINTF_ALLOCATE(&pwszOut, wchar16_t, (sLen + 1) * sizeof(wchar16_t));

    GOTO_CLEANUP_ON_ERRNO(error);

    error = LwPrintfW16StringV(
        pwszOut,
        sLen + 1,
        pCharsOut,
        Format,
        args2);

    GOTO_CLEANUP_ON_ERRNO(error);

cleanup:
    if (error)
    {
        RTL_FREE(&pwszOut);
    }

    va_end(args2);

    *pBuffer = pwszOut;

    return error;

}

int
LwPrintfW16AllocateString(
    OUT wchar16_t** pBuffer,
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN ...
    )
{
    int error;
    va_list ap;

    va_start(ap, Format);
    error = LwPrintfW16AllocateStringV(pBuffer, pCharsOut, Format, ap);
    va_end(ap);

    return error;
}

int
LwPrintfStdoutV(
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN va_list Args
    )
{
    return GenericPrintfV(pCharsOut, Format, Args, stdout);
}

int
LwPrintfStdout(
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN ...
    )
{
    int error = 0;
    va_list ap;

    va_start(ap, Format);

    error = LwPrintfStdoutV(
                    pCharsOut,
                    Format,
                    ap);

    va_end(ap);

    return error;
}

int
LwPrintfStderrV(
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN va_list Args
    )
{
    return GenericPrintfV(pCharsOut, Format, Args, stderr);
}

int
LwPrintfStderr(
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN ...
    )
{
    int error = 0;
    va_list ap;

    va_start(ap, Format);

    error = LwPrintfStderrV(
                    pCharsOut,
                    Format,
                    ap);

    va_end(ap);

    return error;
}


//
// Output Element: char
// Format String Element: char (UTF-8)
//
int
LwPrintfStringV(
    OUT char* pOut,
    IN size_t MaxChars,
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN va_list Args
    )
{
    int error = 0;
    LW_STRING_PRINTF_BUFFER pOutput;

    memset(&pOutput, 0, sizeof(pOutput));

    pOutput.parent.pfnWriteWcs = (LW_PFNBUFFER_WRITE_WCS)StringPrintfWriteWcsMbs;
    pOutput.parent.pfnWriteWc16s = (LW_PFNBUFFER_WRITE_WC16S)StringPrintfWriteWc16sMbs;
    pOutput.parent.pfnWriteMbs = (LW_PFNBUFFER_WRITE_MBS)StringPrintfWriteMbsMbs;
    pOutput.parent.pfnWriteUcs = (LW_PFNBUFFER_WRITE_UCS)StringPrintfWriteUcsMbs;
    pOutput.parent.pfnWriteAs = (LW_PFNBUFFER_WRITE_AS)StringPrintfWriteAsMbs;

    pOutput.pBuffer = pOut;
    pOutput.sAvailable = MaxChars;

    error = PrintfCore(
                &pOutput.parent,
                Format,
                Args);

    GOTO_CLEANUP_ON_ERRNO(error);

    if (!pOutput.pBuffer)
    {
        /* This is only calculating the maximum size. Do not include the null
         * in this calculation, and do not write anything.
         */
    }
    else if (pOutput.sAvailable)
    {
        /* terminate the resulting string */
        *(char *)pOutput.pBuffer = 0;
    }
    else
    {
        error = ENOMEM;
        GOTO_CLEANUP();
    }

cleanup:
    if (!error)
    {
        *pCharsOut = pOutput.parent.sWrittenCount;
    }

    return error;
}

int
LwPrintfString(
    OUT char* pOut,
    IN size_t MaxChars,
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN ...
    )
{
    int error = 0;
    va_list ap;

    va_start(ap, Format);

    error = LwPrintfStringV(
                    pOut,
                    MaxChars,
                    pCharsOut,
                    Format,
                    ap);

    va_end(ap);

    return error;

}

int
LwPrintfAllocateStringV(
    OUT char** pBuffer,
    IN size_t* pCharsOut,
    IN const char* Format,
    IN va_list Args
    )
{
    char* pszOut = NULL;
    size_t sLen = 0;
    va_list args2;
    int error = 0;

    va_copy(args2, Args);

    error = LwPrintfStringV(
        NULL,
        0,
        &sLen,
        Format,
        Args);

    GOTO_CLEANUP_ON_ERRNO(error);

    error = LW_PRINTF_ALLOCATE(&pszOut, char, (sLen + 1) * sizeof(char));

    GOTO_CLEANUP_ON_ERRNO(error);

    error = LwPrintfStringV(
        pszOut,
        sLen + 1,
        pCharsOut,
        Format,
        args2);

    GOTO_CLEANUP_ON_ERRNO(error);

cleanup:
    if (error)
    {
        RTL_FREE(&pszOut);
    }

    va_end(args2);

    *pBuffer = pszOut;

    return error;
}

int
LwPrintfAllocateString(
    OUT char** pBuffer,
    OUT size_t* pCharsOut,
    IN const char* format,
    IN ...
    )
{
    va_list ap;
    int error = 0;

    va_start(ap, format);
    error = LwPrintfAllocateStringV(pBuffer, pCharsOut, format, ap);
    va_end(ap);

    return error;
}

