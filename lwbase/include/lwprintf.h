/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-

 *  Editor Settings: expandtabs and use 4 spaces for indentation */
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

//
// This module deviates from the POSIX C library functions a bit.
// All functions return an error code, take their output as parameters which
// includes a place to write the formatted string to
// (buffer | allocated buffer) as well as an output parameter that will contain
// the number of bytes written.
//

#ifndef _LWPRINTF_H_
#define _LWPRINTF_H_

#include <wchar16.h>
#include <stddef.h>
#include <stdarg.h>
#include <lw/attrs.h>

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
    );

int
LwPrintfW16StringW(
    OUT wchar16_t* pOut,
    IN size_t MaxChars,
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN ...
    );

int
LwPrintfW16AllocateStringWV(
    OUT wchar16_t** pBuffer,
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN va_list Args
    );

int
LwPrintfW16AllocateStringW(
    OUT wchar16_t** pBuffer,
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN ...
    );


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
    );

int
LwPrintfStringW(
    OUT char* pOut,
    IN size_t MaxChars,
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    ...
    );

int
LwPrintfAllocateStringWV(
    OUT char** pBuffer,
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN va_list args
    );

int
LwPrintfAllocateStringW(
    OUT char** pBuffer,
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN ...
    );

int
LwPrintfStdoutWV(
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN va_list Args
    );

int
LwPrintfStdoutW(
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN ...
    );

int
LwPrintfStderrWV(
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN va_list Args
    );

int
LwPrintfStderrW(
    OUT size_t* pCharsOut,
    IN const wchar_t* Format,
    IN ...
    );

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
    );

int
LwPrintfW16String(
    OUT wchar16_t* pOut,
    IN size_t MaxChars,
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN ...
    );

int
LwPrintfW16AllocateStringV(
    OUT wchar16_t** pBuffer,
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN va_list Args
    );

int
LwPrintfW16AllocateString(
    OUT wchar16_t** pBuffer,
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN ...
    );
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
    );

int
LwPrintfString(
    OUT char* pOut,
    IN size_t MaxChars,
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN ...
    );

int
LwPrintfAllocateStringV(
    OUT char** pBuffer,
    IN size_t* pCharsOut,
    IN const char* Format,
    IN va_list Args
    );

int
LwPrintfAllocateString(
    OUT char** pBuffer,
    OUT size_t* pCharsOut,
    IN const char* format,
    IN ...
    );

int
LwPrintfStdoutV(
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN va_list Args
    );

int
LwPrintfStdout(
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN ...
    );

int
LwPrintfStderrV(
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN va_list Args
    );

int
LwPrintfStderr(
    OUT size_t* pCharsOut,
    IN const char* Format,
    IN ...
    );

#endif // _LWPRINTF_H_
