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

#ifndef __CTSHELL_H__
#define __CTSHELL_H__

#include <ctbase.h>

typedef struct CTShellVar
{
    enum
    {
        SVAR_INT,
        SVAR_STR,
        SVAR_ARR,
	SVAR_ZERO,
        SVAR_OUT
    } type;
    const char* name;
    union
    {
        int integer;
        const char* string;
        char const * const * array;
        char** out;
    } value;
} CTShellVar;

struct CTShellVar __CTVarInteger(const char* name, int value);
struct CTShellVar __CTVarString(const char* name, const char* value);
struct CTShellVar __CTVarArray(const char* name, char const * const * value);
struct CTShellVar __CTVarOut(const char* name, char** out);
struct CTShellVar __CTVarZero(const char* name);

#define CTSHELL_INTEGER(name, value) (__CTVarInteger( #name , value))
#define CTSHELL_STRING(name, value) (__CTVarString( #name , value))
#define CTSHELL_ARRAY(name, value) (__CTVarArray( #name , (char const * const *) (value)))
#define CTSHELL_BUFFER(name, value) (__CTVarOut( #name , value))
#define CTSHELL_ZERO(name) (__CTVarZero( #name ))


LW_BEGIN_EXTERN_C

DWORD
CTShell(const char* format, ...);

DWORD
CTShellEx(char * const envp[], const char* format, ...);

LW_END_EXTERN_C


#endif
