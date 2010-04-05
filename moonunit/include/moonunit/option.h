/*
 * Copyright (c) 2007, Brian Koropoff
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Moonunit project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRIAN KOROPOFF ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL BRIAN KOROPOFF BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MU_OPTION_H__
#define __MU_OPTION_H__

#include <stdarg.h>
#include <moonunit/type.h>
#include <moonunit/internal/boilerplate.h>

typedef struct MuOption
{
    const char* name;
    MuType type;
    /* Function pointers (prototype varies by type) */
    void *set, *get;
    const char* description;
} MuOption;

#define MU_OPTION(_name, _type, _get, _set, _description)   \
    {                                                       \
        FIELD(name, (_name)),                               \
        FIELD(type, (_type)),                               \
        FIELD(set, (_set)),                                 \
        FIELD(get, (_get)),                                 \
        FIELD(description, (_description))                  \
    }

#define MU_OPTION_END                                       \
    {                                                       \
        FIELD(name, NULL),                                  \
        FIELD(type, MU_TYPE_UNKNOWN),                       \
        FIELD(set, NULL),                                   \
        FIELD(get, NULL),                                   \
        FIELD(description, NULL)                            \
    }                                                       \

C_BEGIN_DECLS

void Mu_Option_SetString(MuOption* table, void* object, const char *name, const char* value);
void Mu_Option_Setv(MuOption* table, void* object, const char *name, va_list ap);
void Mu_Option_Set(MuOption* table, void* object, const char *name, ...);
void Mu_Option_Get(MuOption* table, void* object, const char *name, void* res);
char* Mu_Option_GetString(MuOption* table, void* object, const char *name);
MuType Mu_Option_Type(MuOption* table, const char* name);
const char* Mu_Option_Description(MuOption* table, const char *name);

C_END_DECLS

#endif
