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

#ifndef WCHAR16_H
#define WCHAR16_H

#ifndef WCHAR16_T_DEFINED
#define WCHAR16_T_DEFINED 1

#ifdef __GNUC__
typedef unsigned short int  wchar16_t;  /* 16-bit unsigned */

#elif defined(_WIN32)
#include <wchar.h>
typedef wchar_t             wchar16_t;
#define WCHAR16_IS_WCHAR
#endif

#endif /* WCHAR16_T_DEFINED */

#ifdef _WIN32
#ifdef LIBUNISTR_EXPORTS
#define LIBUNISTR_API __declspec(dllexport)
#else
#define LIBUNISTR_API __declspec(dllimport)
#endif
#else
#define LIBUNISTR_API
#endif

#endif /* WCHAR16_H */
