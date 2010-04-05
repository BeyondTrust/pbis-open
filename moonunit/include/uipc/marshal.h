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

#ifndef __UIPC_MARSHAL_H__
#define __UIPC_MARSHAL_H__

typedef enum
{
    UIPC_KIND_NONE,
    UIPC_KIND_STRING,
    UIPC_KIND_POINTER
} uipc_kind;

typedef struct __uipc_typeinfo
{
    unsigned long size;
    const char* name;
    struct
    {
        unsigned long offset;
        uipc_kind kind;
        struct __uipc_typeinfo* pointee_type;
    } members[];
} uipc_typeinfo;

#define UIPC_OFFSET(type, field) ((unsigned long) &((type*)0)->field)
#define UIPC_POINTER(type, field, info)     \
    {                                       \
        .offset = UIPC_OFFSET(type, field), \
        .kind = UIPC_KIND_POINTER,          \
        .pointee_type = info                \
    }                                       \

#define UIPC_STRING(type, field)                \
    {                                           \
        .offset = UIPC_OFFSET(type, field),     \
        .kind = UIPC_KIND_STRING,               \
    }                                           \

#define UIPC_END { .kind = UIPC_KIND_NONE }

unsigned long uipc_marshal_payload(void* buffer, unsigned long size, const void* payload, uipc_typeinfo* type);
unsigned long uipc_unmarshal_payload(void** out, const void* payload, uipc_typeinfo* type);
void uipc_free_object(void* object, uipc_typeinfo* type);

#endif
