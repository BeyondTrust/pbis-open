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

#ifndef __GPASHELL_H__
#define __GPASHELL_H__

#include "includes.h"

typedef struct GPAShellVar
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
} GPAShellVar;

struct GPAShellVar __GPAVarInteger(const char* name, int value);
struct GPAShellVar __GPAVarString(const char* name, const char* value);
struct GPAShellVar __GPAVarArray(const char* name, char const * const * value);
struct GPAShellVar __GPAVarOut(const char* name, char** out);
struct GPAShellVar __GPAVarZero(const char* name);

#define GPASHELL_INTEGER(name, value) (__GPAVarInteger( #name , value))
#define GPASHELL_STRING(name, value) (__GPAVarString( #name , value))
#define GPASHELL_ARRAY(name, value) (__GPAVarArray( #name , (char const * const *) (value)))
#define GPASHELL_BUFFER(name, value) (__GPAVarOut( #name , value))
#define GPASHELL_ZERO(name) (__GPAVarZero( #name ))

CENTERROR
GPAShell(const char* format, ...);

CENTERROR
GPAShellEx(char * const envp[], const char* format, ...);

#endif
