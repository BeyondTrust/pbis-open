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

#ifndef WCSTR16_H
#include <wchar16.h>
#include <stddef.h>

#ifdef __GNUC__
#define TEXT(str)  str
#endif

/* The WC16STR macro can be used to create wchar16_t string literals on Linux
 * and Windows. The macro takes a string literal as input. Only one instance
 * can occur per line.
 *
 * At the top of the .c file, after wchar16.h has been included, but before any
 * wchar16_t string literals are used, WC16STR_INSERT must occur on a line by
 * itself.
 *
 * Here is an example usage:
#include <wc16printf.h>

WC16STR_INSERT

int main()
{
    fw16printf(stdout,
        WC16STR("%ws\n"),
        WC16STR("sample string"));
    return 0;
}
*/

#ifdef _WIN32
#define WC16STR(x) 	L##x
#else
#define _WC16STR3(x) 	gwsz ## x
#define _WC16STR2(x) 	_WC16STR3(x)
#define WC16STR(x) 	_WC16STR2(__LINE__)
#endif

#define WC16STR_INSERT

#ifdef _WIN32
#define snprintf(dest, count, ...) _snprintf(dest, count, __VA_ARGS__)
#endif

LIBUNISTR_API
unsigned long long
_wc16stoull(
    const wchar16_t *input, 
    const wchar16_t **end, 
    int base
    );
#ifdef WCHAR16_IS_WCHAR
#define wc16stoull(input, end, base)  _wcstoui64(input, (wchar_t**)end, base)
#else
#define wc16stoull(input, end, base)  _wc16stoull(input, end, base)
#endif

#ifdef WCHAR16_IS_WCHAR
#define _w16toi(str)  _wtoi(str)
#else
#define _w16toi(str)  (int)wc16stoull((str), NULL, 10)
#endif

LIBUNISTR_API
size_t _wc16slen(const wchar16_t *str);
#ifdef __GNUC__
#define wc16slen(str)  _wc16slen(str)
#elif _WIN32
#define wc16slen(str)  wcslen(str)
#endif

LIBUNISTR_API
size_t _wc16snlen(const wchar16_t *str, size_t n);
#ifdef WCHAR16_IS_WCHAR
#define wc16snlen(str, n)  wcsnlen(str, n)
#else
#define wc16snlen(str, n)  _wc16snlen(str, n)
#endif

LIBUNISTR_API
wchar16_t *_wc16scpy(wchar16_t *dst, const wchar16_t *src);
#ifdef __GNUC__
#define wc16scpy(dst, src)  _wc16scpy(dst, src);
#elif _WIN32
#define wc16scpy(dst, src)  wcscpy(dst, src);
#endif

LIBUNISTR_API
wchar16_t* _wc16sdup(const wchar16_t *str);
#ifdef __GNUC__
#define wc16sdup(str)  _wc16sdup(str)
#elif _WIN32
#define wc16sdup(str)  _wcsdup(str)
#endif

LIBUNISTR_API
wchar16_t*  _wc16sndup(const wchar16_t *str, size_t max_characters);
#define wc16sdupn(str, max_characters)  _wc16sndup(str, max_characters)
#define wc16sndup(str, max_characters)  _wc16sndup(str, max_characters)

LIBUNISTR_API
wchar16_t* _wc16sncpy(wchar16_t *dest, const wchar16_t *src, size_t n);
#ifdef WCHAR16_IS_WCHAR
#define wc16sncpy(dest, src, n)  wcsncpy(dest, src, n)
#else
#define wc16sncpy(dest, src, n)  _wc16sncpy(dest, src, n)
#endif

LIBUNISTR_API
wchar16_t* _w16memcpy(wchar16_t *dest, const wchar16_t *src, size_t n);
#ifdef WCHAR16_IS_WCHAR
#define w16memcpy(dest, src, n)  wmemcpy(dest, src, n)
#else
#define w16memcpy(dest, src, n)  _w16memcpy(dest, src, n)
#endif

LIBUNISTR_API
wchar16_t* _w16memset(wchar16_t *dest, wchar16_t fill, size_t n);
#ifdef WCHAR16_IS_WCHAR
#define w16memset(dest, fill, n)  wmemset(dest, fill, n)
#else
#define w16memset(dest, fill, n)  _w16memset(dest, fill, n)
#endif

/* Copy upto n 16bit characters from one string to another
 *
 * n is the maximum number of characters to store in dest (including null).
 */
LIBUNISTR_API
wchar16_t* _wc16pncpy(wchar16_t *dest, const wchar16_t *src, size_t n);
#define wc16pncpy(dest, src, n)  _wc16pncpy(dest, src, n)

LIBUNISTR_API
int wc16scasecmp(const wchar16_t *s1, const wchar16_t *s2);

LIBUNISTR_API
int wc16scmp(const wchar16_t *s1, const wchar16_t *s2);

LIBUNISTR_API
int wc16sncmp(const wchar16_t *s1, const wchar16_t *s2, size_t n);

#ifndef HAVE_MBSTRLEN
#ifdef _WIN32
#define mbstrlen(x) _mbstrlen(x)
#else
#define mbstrlen(x) mbstowcs(NULL, (x), 0)
#endif
#endif

/* Returns the number of bytes in src that form the next cchFind multi-byte
 * characters.
 *
 * If fewer than cchFind characters are in src, then the size of the rest of
 * the string in bytes is returned.
 *
 * Basically this converts from a count of multi-byte characters into a count
 * of bytes.
 */
LIBUNISTR_API
size_t __mbsnbcnt(const char *src, size_t cchFind);

#ifndef HAVE_MBSNBCNT
#ifdef _WIN32
#define mbsnbcnt(src, cchFind)  _mbsnbcnt(src, cchFind)
#else
#define mbsnbcnt(src, cchFind)  __mbsnbcnt(src, cchFind)
#endif
#endif

/*Optimistically try to wc16sncpy()
 *
 * Returns the length of dest needed for a successful copy including
 * the NUL character.  If the copy was complete, the value will be
 * <= wc16slen(src) + 1
 */
LIBUNISTR_API
size_t wc16oncpy(wchar16_t *dest, const wchar16_t *src, size_t n);

/*Convert a wchar_t string to a wchar16_t string and return the result.
 *
 * If the result needs to be freed, *free_required will be set to 1.
 */
LIBUNISTR_API
wchar16_t * awcstowc16s(const wchar_t *input, int *free_required);

/*Convert a wchar_t string to a wchar16_t string and return the number of characters converted.
 *
 * cchn is the maximum number of characters to store in dest (including null).
 */
LIBUNISTR_API
size_t wcstowc16s(wchar16_t *dest, const wchar_t *src, size_t cchn);

/*Convert a wchar16_t string to a wchar_t string and return the result.
 *
 * If the result needs to be freed, *free_required will be set to 1.
 */
LIBUNISTR_API
wchar_t * awc16stowcs(const wchar16_t *input, int *free_required);

/*Convert a wchar16_t string to a wchar_t string and return the number of characters converted.
 *
 * cchn is the maximum number of characters to store in dest (including null).
 */
LIBUNISTR_API
size_t wc16stowcs(wchar_t *dest, const wchar16_t *src, size_t cchn);

/* Convert a wchar16_t string to a wchar16_t string in little-endian byte order */
LIBUNISTR_API
size_t wc16stowc16les(wchar16_t *dest, const wchar16_t *src, size_t cchcopy);

/* Convert a wchar16_t string in little-endian byte order to a wchar16_t string */
LIBUNISTR_API
size_t wc16lestowc16s(wchar16_t *dest, const wchar16_t *src, size_t cchcopy);

/*Convert a multibyte character string to a wchar16_t string and return the result.
 */
LIBUNISTR_API
wchar16_t * ambstowc16s(const char *input);

/*Convert a multibyte character string to a wchar16_t string and return the number of characters converted.
 *
 * cchn is the maximum number of characters to store in dest (including null).
 */
LIBUNISTR_API
size_t mbstowc16s(wchar16_t *dest, const char *src, size_t cchn);

/*Convert a multibyte character string to a little endian wchar16_t string and return the number of characters converted.
 *
 * cchn is the maximum number of characters to store in dest (including null).
 */
LIBUNISTR_API
size_t mbstowc16les(wchar16_t *dest, const char *src, size_t cchn);

/*Convert a wchar16_t string to a multibyte character string and return the result.
 */
LIBUNISTR_API
char * awc16stombs(const wchar16_t *input);

/*Convert a wchar16_t string to a multicharacter string and return the number of characters converted.
 *
 * cbn is the maximum number of bytes to store in dest (including null).
 */
LIBUNISTR_API
size_t wc16stombs(char *dest, const wchar16_t *src, size_t cbn);


/* Convert a wchar16_t string to upper case */
LIBUNISTR_API
void wc16supper(wchar16_t *s);

/* Convert a wchar16_t string to lower case */
LIBUNISTR_API
void wc16slower(wchar16_t *s);


/* Convert a single-byte string to upper case */
LIBUNISTR_API
void strlower(char *s);

/* Convert a single-byte string to lower case */
LIBUNISTR_API
void strupper(char *s);

LIBUNISTR_API
void printwc16s(const wchar16_t* pwszPrint);

#ifdef _WIN32
#define strcasecmp(a, b) stricmp(a, b)
#endif

#endif /* WCSTR16_H */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
