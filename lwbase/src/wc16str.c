/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

/* boo */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <locale.h>
#include <wc16str.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_WCTYPE_H
#    include <wctype.h>
#endif
#ifndef _WIN32
#include <iconv.h>
#include <inttypes.h>
#endif
#include <wchar.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include "wc16printf.h"

#ifdef _WIN32
#pragma warning( disable : 4996 )
#endif

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

#if SIZEOF_WCHAR_T == 2
#define WCHAR16_IS_WCHAR 1
#endif

/* UCS-2, little endian byte order
 * Note that UCS-2 without LE will
 * default to big endian on FreeBSD
 */
#if defined(WORDS_BIGENDIAN)
#define WINDOWS_ENCODING "UCS-2BE"
#define UTF32_ENCODING "UTF-32BE"
#else
#define WINDOWS_ENCODING "UCS-2LE"
#define UTF32_ENCODING "UTF-32LE"
#endif

#ifndef ULLONG_MAX
#define ULLONG_MAX 0xFFFFFFFFFFFFFFFFull
#endif

typedef int (*caseconv)(int c);
typedef wint_t (*wcaseconv)(wint_t c);

// Returns the integer value of a digit character for a given base. If the
// character is not a valid digit, -1 is returned.
static
int
get_digit_in_base(
    wchar16_t c,
    int base
    )
{
    int digit = -1;
    if (c >= '0' && c <= '9')
    {
        digit = c - '0';
    }
    else if (c >= 'a' && c <= 'z')
    {
        digit = c - 'a' + 10;
    }
    else if (c >= 'A' && c <= 'Z')
    {
        digit = c - 'A' + 10;
    }

    if (digit >= base)
    {
        // This digit is not valid for the given base
        digit = -1;
    }
    return digit;
}

unsigned long long
_wc16stoull(
    const wchar16_t *input, 
    const wchar16_t **end, 
    int base
    )
{
    unsigned long long result = 0;
    int bOverflowed = 0;
    int bNegate = 0;

    if (base < 0 || base == 1 || base > 36)
    {
        /* This matches the behavior of glibc's strtoull. *end is not changed,
         * zero is returned, and errno is set to EINVAL.
         */
        errno = EINVAL;
        return 0;
    }
    // Don't treat the - as part of a number unless it is followed by number
    // that can be parsed (meaning it has a digit afterwards).
    if (*input == '-' && get_digit_in_base(input[1], base? base : 10) >= 0)
    {
        bNegate = 1;
        input++;
    }
    if (base == 0)
    {
        const wchar16_t wszHexPrefix[] = {'0', 'x', 0};
        const wchar16_t wszOctalPrefix[] = {'0', 0};
        // gotta figure out what the real base is
        if (!wc16sncmp(input, wszHexPrefix, 2) &&
                get_digit_in_base(input[2], 16) >= 0)
        {
            base = 16;
            input += 2;
        }
        else if (!wc16sncmp(input, wszOctalPrefix, 1) &&
                get_digit_in_base(input[1], 8) >= 0)
        {
            base = 8;
            input += 1;
        }
        else
        {
            base = 10;
        }
    }

    while (1)
    {
        unsigned long long old_result = result;
        int digit;

        digit = get_digit_in_base(*input, base);
        if (digit < 0)
        {
            /* done with conversion
             * If 0 digits are parsed, it is not considered an error by glibc
             * and 0 is returned as the result.
             */
            break;
        }

        result *= base;
        result += digit;
        if (result < old_result)
        {
            /* Match glibc's behavior by continuing to parse the integer, but
             * finally return with an overflow error after the last digit is
             * parsed. This affects what the end pointer will be set to.
             */
            bOverflowed = 1;
        }

        input++;
    }

    if (end != NULL)
    {
        *end = input;
    }

    if (bOverflowed)
    {
        errno = ERANGE;
        return ULLONG_MAX;
    }

    /* Match glibc's behavior with regards to negating. It is not considered an
     * error if negating the result overflows (which means numbers lower than
     * LLONG_MIN can be entered). The lowest acceptable input is -ULLONG_MAX,
     * although anything lower than LLONG_MIN will wrap around.
     */
    if (bNegate)
    {
        return (unsigned long long)-(long long)result;
    }
    else
    {
        return result;
    }
}

size_t _wc16slen(const wchar16_t *str)
{
    size_t i = 0;
    if (str == NULL) return i;

    while (str[i] != 0) i++;

    return i;
}

size_t _wc16snlen(const wchar16_t *str, size_t n)
{
    size_t i = 0;
    while (n > 0)
    {
        if (str[i] == 0)
            break;
        n--;
        i++;
    }

    return i;
}

wchar16_t* _wc16scpy(wchar16_t *dst, const wchar16_t *src)
{
    size_t size;

    if (dst == NULL || src == NULL) return NULL;

    size = (_wc16slen(src) + 1) * sizeof(wchar16_t);
    memcpy(dst, src, size);

    return dst;
}


wchar16_t* _wc16sdup(const wchar16_t *str)
{
    size_t size;
    wchar16_t *out;

    if (str == NULL) return NULL;

    size = (_wc16slen(str) + 1) * sizeof(wchar16_t);
    out = (wchar16_t*) malloc(size);
    if (out == NULL) return NULL;
    memcpy(out, str, size);

    return out;
}


wchar16_t* _wc16sndup(const wchar16_t *str, size_t max_characters)
{
    size_t len;
    wchar16_t *out;

    if (str == NULL) return NULL;

    //Find the length of str, up to max_characters
    for(len = 0; len < max_characters && str[len] != 0; len++);

    out = (wchar16_t*) malloc((len + 1) * sizeof(wchar16_t));
    if (out == NULL) return NULL;

    //Copy everything up to the NULL terminator from str
    memcpy(out, str, len * sizeof(wchar16_t));
    //Add the NULL
    out[len] = 0;

    return out;
}

/**
 * @fixme: According to the manpage, wcsncpy() returns dest, not the
 * end of the string pointed to by dest.  Do we want to diverge?
 */
wchar16_t* _wc16sncpy(wchar16_t *dest, const wchar16_t *src, size_t n)
{
    while(n > 0)
    {
        *dest = *src;
        if(*src == 0)
            break;
        dest++;
        src++;
        n--;
    }
    return dest;
}

wchar16_t* _w16memcpy(wchar16_t *dest, const wchar16_t *src, size_t n)
{
    return memcpy(dest, src, n * sizeof(wchar16_t));
}

wchar16_t* _w16memset(wchar16_t *dest, wchar16_t fill, size_t n)
{
    wchar16_t* pwszPos = dest;
    while(n > 0)
    {
        *pwszPos = fill;
        pwszPos++;
        n--;
    }
    return dest;
}

/*
 * For consistency and performance, we break the wcpncpy() contract and don't
 * NUL pad dest when src is short.
 */
wchar16_t* _wc16pncpy(wchar16_t *dest, const wchar16_t *src, size_t n)
{
    /* Return pointer can't point to written memory when n == 0 */
    /* assert() instead? */
    if (!n)
        return dest;

    while(n > 0)
    {
        n--;
        *dest = *src;
        if(*src == 0 || n == 0)
            break;
        src++;
        dest++;
    }
    return dest;
}

int wc16sncmp(const wchar16_t *s1, const wchar16_t *s2, size_t n)
{
    size_t s1_len, s2_len, len;

    if (s1 == NULL || s2 == NULL) return -1;

    s1_len = wc16slen(s1);
    s2_len = wc16slen(s2);

    if (s1_len > n)
    {
        s1_len = n;
    }
    if (s2_len > n)
    {
        s2_len = n;
    }

    if (s1_len != s2_len)
    {
        ssize_t sLenDiff = (ssize_t)(s1_len - s2_len);
        if (sLenDiff > INT_MAX || sLenDiff < INT_MIN)
        {
            if (sLenDiff > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
        }
        return (int)sLenDiff;
    }

    len = s1_len * sizeof(wchar16_t);
    return memcmp((void*)s1, (void*)s2, len);
}

int wc16scmp(const wchar16_t *s1, const wchar16_t *s2)
{
    return wc16sncmp(s1, s2, SIZE_MAX);
}

#ifndef HAVE_WCSCASECMP

int wcscasecmp(const wchar_t *w1, const wchar_t *w2);

int wcscasecmp(const wchar_t *w1, const wchar_t *w2)
{
    int index;
    wchar_t c1 = 0, c2 = 0;

    for (index = 0, c1 = towlower(w1[0]), c2 = towlower(w2[0]);
         c1 && c2 && c1 == c2;
         index++, c1 = towlower(w1[index]), c2 = towlower(w2[index]));

    return (c1 == c2) ? 0 : ((c1 < c2) ? -1 : 1);
}

#else

extern int wcscasecmp(const wchar_t *w1, const wchar_t *w2);

#endif

int wc16scasecmp(const wchar16_t *s1, const wchar16_t *s2)
{
    int need_free = 0;
    wchar_t* w1, *w2;
    int result;

    w1 = awc16stowcs(s1, &need_free);
    w2 = awc16stowcs(s2, &need_free);

    result = wcscasecmp(w1, w2);

    if (need_free)
    {
        free(w1);
        free(w2);
    }

    return result;
}

/*Optimistically try to wc16sncpy()
 *
 * Returns the length of dest needed for a successful copy including
 * the NUL character.  If the copy was complete, the value will be
 * <= wc16slen(src) + 1
 */
size_t wc16oncpy(wchar16_t *dest, const wchar16_t *src, size_t n)
{
    ptrdiff_t diff = 0;

    if (n != 0)
    {
        wchar16_t *cursor = wc16pncpy(dest, src, n);
        diff = cursor - dest + 1;
        if (!*cursor)
        {
            /* Success */
            return diff;
        }
    }

    return diff + wc16slen(src + diff) + 1;
}

wchar16_t *awcstowc16s(const wchar_t *input, int *free_required)
{
#ifdef WCHAR16_IS_WCHAR
    *free_required = 0;
    return (wchar16_t*) input;
#else
    size_t cchlen;
    wchar16_t *buffer;
    if(input == NULL)
        return NULL;

    cchlen = wcslen(input);
    buffer = malloc((cchlen + 1) * sizeof(wchar16_t));
    if(buffer == NULL)
        return NULL;
    if(wcstowc16s(buffer, input, cchlen + 1) != cchlen)
    {
        free(buffer);
        return NULL;
    }
    *free_required = 1;
    return buffer;
#endif
}

size_t wcstowc16s(wchar16_t *dest, const wchar_t *src, size_t cchcopy)
{
#ifdef WCHAR16_IS_WCHAR
    size_t input_len = wcslen(src);
    if(input_len >= cchcopy)
        input_len = cchcopy;
    else
        dest[input_len] = 0;

    memcpy(dest, src, input_len * sizeof(wchar16_t));
    return input_len;
#else
    iconv_t handle = iconv_open(WINDOWS_ENCODING, "WCHAR_T");
    char *inbuf = (char *)src;
    char *outbuf = (char *)dest;
    size_t cbin = wcslen(src) * sizeof(src[0]);
    size_t cbout = cchcopy * sizeof(dest[0]);
    size_t converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);

    if(cbout >= sizeof(dest[0]))
        *(wchar16_t *)outbuf = 0;
    iconv_close(handle);
    if(converted == (size_t)-1 && cbout != 0)
        return (size_t)-1;
    else
        return cchcopy - cbout/sizeof(dest[0]);
#endif
}

wchar_t *awc16stowcs(const wchar16_t *input, int *free_required)
{
#ifdef WCHAR16_IS_WCHAR
    *free_required = 0;
    return (wchar_t*) input;
#else
    size_t cchlen;
    wchar_t *buffer;
    if(input == NULL)
        return NULL;
    cchlen = wc16slen(input);
    buffer = malloc((cchlen + 1) * sizeof(wchar_t));
    if(buffer == NULL)
        return NULL;
    if(wc16stowcs(buffer, input, cchlen + 1) != cchlen)
    {
        free(buffer);
        return NULL;
    }
    *free_required = 1;
    return buffer;
#endif
}

size_t wc16stowcs(wchar_t *dest, const wchar16_t *src, size_t cchcopy)
{
#ifdef WCHAR16_IS_WCHAR
    size_t input_len = wcslen(src);
    if(input_len >= cchcopy)
        input_len = cchcopy;
    else
        dest[input_len] = 0;

    memcpy(dest, src, input_len * sizeof(wchar16_t));
    return input_len;
#else
    iconv_t handle = iconv_open("WCHAR_T", WINDOWS_ENCODING);
    char *inbuf = (char *)src;
    char *outbuf = (char *)dest;
    size_t cbin = wc16slen(src) * sizeof(src[0]);
    size_t cbout = cchcopy * sizeof(dest[0]);
    size_t converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);

    if(cbout >= sizeof(dest[0]))
        *(wchar_t *)outbuf = 0;
    iconv_close(handle);
    if(converted == (size_t)-1 && cbout != 0)
        return (size_t)-1;
    else
        return cchcopy - cbout/sizeof(dest[0]);
#endif
}

size_t wc16stowc16les(wchar16_t *dest, const wchar16_t *src, size_t cchcopy)
{
    size_t cbout = wc16snlen(src, cchcopy);

    if (cbout < cchcopy)
    {
        cbout++;
    }

#if defined(WORDS_BIGENDIAN)
    swab(src, dest, cbout * sizeof(src[0]));
#else
    memcpy(dest, src, cbout * sizeof(src[0]));
#endif

    if (cbout > 0 && src[cbout - 1] == 0)
    {
        cbout--;
    }

    return cbout;
}

size_t wc16lestowc16s(wchar16_t *dest, const wchar16_t *src, size_t cchcopy)
{
    size_t cbout = wc16snlen(src, cchcopy);

    if (cbout < cchcopy)
    {
        cbout++;
    }

#if defined(WORDS_BIGENDIAN)
    swab(src, dest, cbout * sizeof(src[0]));
#else
    memcpy(dest, src, cbout * sizeof(src[0]));
#endif

    if (cbout > 0 && src[cbout - 1] == 0)
    {
        cbout--;
    }

    return cbout;
}

wchar16_t *ambstowc16s(const char *input)
{
    size_t cchlen;
    wchar16_t *buffer;
    if(input == NULL)
        return NULL;
    cchlen = mbstrlen(input);
    if(cchlen == (size_t)-1)
        return NULL;
    buffer = malloc((cchlen + 1) * sizeof(wchar16_t));
    if(buffer == NULL)
        return NULL;
    if(mbstowc16s(buffer, input, cchlen + 1) != cchlen)
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}

/*Convert a multibyte character string to a wchar16_t string and return the number of characters converted.
 *
 * cchn is the maximum number of characters to store in dest (including null).
 */
size_t mbstowc16s(wchar16_t *dest, const char *src, size_t cchcopy)
{
#ifdef WCHAR16_IS_WCHAR
    return mbstowcs(dest, src, cchcopy);
#else
    iconv_t handle = iconv_open(WINDOWS_ENCODING, "");
    char *inbuf;
    char *outbuf;
    size_t cbin;
    size_t cbout;
    size_t converted;
    if(handle == (iconv_t)-1)
        return (size_t)-1;
    inbuf = (char *)src;
    outbuf = (char *)dest;
    cbin = src ? strlen(src) * sizeof(src[0]) : 0;
    cbout = cchcopy * sizeof(dest[0]);
    converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);

    if(cbout >= sizeof(dest[0]))
        *(wchar16_t *)outbuf = 0;
    iconv_close(handle);
    if(converted == (size_t)-1 && cbout != 0)
        return (size_t)-1;
    else
        return cchcopy - cbout/sizeof(dest[0]);
#endif
}

size_t mbstowc16les(wchar16_t *dest, const char *src, size_t cchcopy)
{
#ifdef WCHAR16_IS_WCHAR
    cchcopy = mbstowc16s(dest, src, cchcopy);
    cchcopy = wc16stowc16les(dest, dest, cchcopy);
    return cchcopy;
#else
    iconv_t handle = iconv_open("UTF-16LE", "");
    char *inbuf;
    char *outbuf;
    size_t cbin;
    size_t cbout;
    size_t converted;
    if(handle == (iconv_t)-1)
        return (size_t)-1;
    inbuf = (char *)src;
    outbuf = (char *)dest;
    cbin = strlen(src) * sizeof(src[0]);
    cbout = cchcopy * sizeof(dest[0]);
    converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);

    if(cbout >= sizeof(dest[0]))
        *(wchar16_t *)outbuf = 0;
    iconv_close(handle);
    if(converted == (size_t)-1 && cbout != 0)
        return (size_t)-1;
    else
        return cchcopy - cbout/sizeof(dest[0]);
#endif
}

char *awc16stombs(const wchar16_t *input)
{
    size_t cblen;
    char *buffer;
    if(input == NULL)
        return NULL;
    cblen = wc16stombs(NULL, input, 0);
    buffer = malloc((cblen + 1) * sizeof(char));
    if(buffer == NULL)
        return NULL;
    if(wc16stombs(buffer, input, cblen + 1) != cblen)
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}

#ifndef _WIN32
// Calculates how many bytes it would take to convert *insize bytes of *inbuf,
// given unlimited output buffer to iconv.
static
size_t
iconv_count(
    iconv_t handle,
    ICONV_IN_TYPE inbuf,
    size_t *insize,
    size_t *outsize
    )
{
    char buffer[100];
    char *outbuf = NULL;
    size_t cbout = 0;
    size_t cNonreversible = 0;

    //iconv does not allow the output buffer to be NULL. To emulate this
    //functionality, we'll have to actually convert the string, but only
    //a few characters at a time.

    *outsize = 0;
    while (*insize > 0)
    {
        outbuf = buffer;
        cbout = sizeof(buffer);
        cNonreversible = iconv(handle, inbuf, insize, &outbuf, &cbout);
        if (cNonreversible == (size_t)-1)
        {
            if (errno != E2BIG)
            {
                return -1;
            }
        }
        *outsize += outbuf - buffer;
    }
    return cNonreversible;
}

size_t __mbsnbcnt(const char *src, size_t cchFind)
{
    iconv_t handle = iconv_open("UCS-4", "");
    size_t cbFind = strlen(src);
    char *srcPos = (char *)src;
    size_t retValue = -1;

    if (iconv_count(handle, (ICONV_IN_TYPE) &srcPos, &cbFind, &cchFind) == (size_t) -1)
    {
        iconv_close(handle);
        goto cleanup;
    }
    retValue = srcPos - src;

cleanup:
    iconv_close(handle);
    return retValue;
}
#endif

static size_t wc16stombs_slow(char *dest, const wchar16_t *src, size_t cbcopy)
{
#ifdef WCHAR16_IS_WCHAR
    return wcstombs(dest, src, cbcopy);
#else
    iconv_t handle = iconv_open("", WINDOWS_ENCODING);
    char *inbuf = (char *)src;
    char *outbuf = (char *)dest;
    size_t cbin = wc16slen(src) * sizeof(src[0]);
    size_t cbout = cbcopy;
    size_t converted;

    if(outbuf == NULL)
    {
        //wcstombs allows dest to be NULL. In this case, cbcopy is ignored
        //and the total number of bytes it would take to store src is returned.
        //

        size_t cblen = 0;
        if (iconv_count(
                    handle,
                    (ICONV_IN_TYPE)&inbuf,
                    &cbin,
                    &cblen) == (size_t)-1)
        {
            cblen = -1;
        }
        iconv_close(handle);
        return cblen;
    }
    converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);

    if(cbout >= sizeof(dest[0]))
        *(char *)outbuf = 0;
    iconv_close(handle);
    if(converted == (size_t)-1 && cbout != 0)
        return (size_t)-1;
    else
        return cbcopy - cbout/sizeof(dest[0]);
#endif
}

static size_t wc16stombs_fast(char* dest, const wchar16_t *src, size_t cbcopy)
{
    size_t i = 0;
    size_t res = 0;

    for (i = 0; !dest || i < cbcopy; i++)
    {
        wchar16_t wc = src[i];
        wchar16_t upper = wc & 0xFF00;
        wchar16_t lower = wc & 0x00FF;

        if (upper == 0 && lower <= 127)
        {
            if (dest)
            {
                dest[i] = (char) lower;
            }

            if (lower == 0)
            {
                break;
            }
        }
        else
        {
            /* We encountered a character we couldn't handle, so fall back
               on slow but accurate conversion path */
            res = wc16stombs_slow((dest ? dest + i : NULL),
                                  src + i,
                                  (i > cbcopy ? 0 : cbcopy - i));
            return res == (size_t) -1 ? res : i + res;
        }
    }

    return i;
}

size_t wc16stombs(char *dest, const wchar16_t *src, size_t cbcopy)
{
    char* lcname = setlocale(LC_CTYPE, NULL);

    if (strstr (lcname, ".UTF-8") ||
        !strcmp (lcname, "C") ||
        !strcmp (lcname, "POSIX"))
    {
        return wc16stombs_fast(dest, src, cbcopy);
    }
    else
    {
        return wc16stombs_slow(dest, src, cbcopy);
    }
}

/*
  We probably ought to be doing this for multi-byte
  strings also.  Note that the case conversion for
  some characters depends on context which this
  doesn't handle.
*/
static void wc16scaseconv(wcaseconv fconv, wchar16_t *s)
{
    size_t len;
    size_t i;

    if (fconv == NULL || s == NULL) return;

    len = wc16slen(s);

    for (i = 0; i < len; i++)
    {
        wint_t c = s[i];
        s[i] = (wchar16_t)fconv(c);
    }
}


void
wc16supper(
    wchar16_t * pwszStr
    )
{
    wc16scaseconv(towupper, pwszStr);
}


void
wc16slower(
    wchar16_t * pwszStr
    )
{
    wc16scaseconv(towlower, pwszStr);
}


/*
  These case conversions aren't exactly right, because toupper
  and tolower functions depend on locale settingsand not on
  unicode maps.
  TODO: Find better case conversion function for unicode
*/
static void strcaseconv(caseconv fconv, char *s)
{
    size_t len;
    size_t i;

    if (fconv == NULL || s == NULL) return;
    len = strlen(s);

    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        s[i] = (char) fconv(c);
    }
}


void strupper(char *s)
{
    strcaseconv(toupper, s);
}


void strlower(char *s)
{
    strcaseconv(tolower, s);
}

void printwc16s(const wchar16_t* pwszPrint)
{
    FILE *tty = fopen("/dev/tty", "w");
    if (tty)
    {
        fw16printfw(tty, L"[%ws]\n", pwszPrint);
        fclose(tty);
    }
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
