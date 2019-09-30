/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

#include <wchar16.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef _WIN32
#ifdef _WIN64
typedef __int64 __w64 ssize_t;
#else
typedef int __w64 ssize_t;
#endif
#endif

LIBUNISTR_API
ssize_t
_vsw16printf(
    wchar16_t *out,
    size_t maxchars,
    const wchar16_t *format,
    va_list args
    );

#ifdef __GNUC__
#define vsw16printf(out, maxchars, format, args) \
            _vsw16printf(out, maxchars, format, args)
#elif _WIN32
#define vsw16printf(out, maxchars, format, args) \
            vswprintf(out, maxchars, format, args)
#endif

//TODO: rename this once the deprecated sw16printf is removed
LIBUNISTR_API
ssize_t
_sw16printf_new(wchar16_t *out, size_t maxchars, const wchar16_t *format, ...);

#ifdef __GNUC__
#define sw16printf_new(out, maxchars, ...) \
    _sw16printf_new(out, maxchars, __VA_ARGS__)
#elif _WIN32
#define sw16printf_new(out, maxchars, ...) \
    swprintf(out, maxchars, __VA_ARGS__)
#endif

LIBUNISTR_API
ssize_t
_sw16printfw(wchar16_t *out, size_t maxchars, const wchar_t *format, ...);

#ifdef __GNUC__
#define sw16printfw(out, maxchars, ...) \
    _sw16printfw(out, maxchars, __VA_ARGS__)
#elif _WIN32
#define sw16printfw(out, maxchars, ...) \
    swprintf(out, maxchars, __VA_ARGS__)
#endif

LIBUNISTR_API
wchar16_t *
asw16printfwv(const wchar_t *format, va_list args);

LIBUNISTR_API
wchar16_t *
asw16printfw(const wchar_t *format, ...);

LIBUNISTR_API
ssize_t
_vfw16printf(
    FILE *pFile,
    const wchar16_t *format,
    va_list args
    );

#ifdef __GNUC__
#define vfw16printf(pFile, format, args) \
    _vfw16printf(pFile, format, args)
#elif _WIN32
#define vfw16printf(pFile, format, args) \
    vfwprintf(pFile, format, args)
#endif

LIBUNISTR_API
ssize_t
_fw16printf(FILE *pFile, const wchar16_t *format, ...);

#ifdef __GNUC__
#define fw16printf(pFile, ...) \
    _fw16printf(pFile, __VA_ARGS__)
#elif _WIN32
#define fw16printf(pFile, ...) \
    fwprintf(pFile, __VA_ARGS__)
#endif

LIBUNISTR_API
ssize_t
_fw16printfw(FILE *pFile, const wchar_t *format, ...);

#ifdef __GNUC__
#define fw16printfw(pFile, ...) \
    _fw16printfw(pFile, __VA_ARGS__)
#elif _WIN32
#define fw16printfw(pFile, ...) \
    fwprintf(pFile, __VA_ARGS__)
#endif

LIBUNISTR_API
ssize_t
_w16printfw(const wchar_t *format, ...);

#ifdef __GNUC__
#define w16printfw(pFile, ...) \
    _w16printfw(pFile, __VA_ARGS__)
#elif _WIN32
#define w16printfw(pFile, ...) \
    wprintf(pFile, __VA_ARGS__)
#endif

//Deprecated
int printfw16(const char *fmt, ...);
//Deprecated
int sw16printf(wchar16_t *out, const char *fmt, ...);
