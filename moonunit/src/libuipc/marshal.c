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

#include <uipc/marshal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define REDUCE(lvalue, delta) (reduce(&lvalue, delta))

static inline void
reduce(unsigned long* size, unsigned long delta)
{
    if (*size > delta)
        (*size) -= delta;
    else
        *size = 0;
}

static unsigned long
marshal_string(void* buffer, unsigned long size, const void* payload)
{
    if (payload)
    {
        unsigned long length = strlen((const char*) payload);
        
        if (size >= length + 1)
            memcpy(buffer, payload, length+1);
        return (length + 1);
    }
    else
    {
        return 0;
    }
}

unsigned long
uipc_marshal_payload(void* buffer, unsigned long size, const void* payload, uipc_typeinfo* type)
{
	int i;
    unsigned long delta;
    unsigned long written = 0;
    void* base = NULL;

    if (payload == NULL)
    {
        return 0;
    }

    if (size >= type->size)
    {
        base = buffer;
        memcpy(buffer, payload, type->size);
    }

    buffer += type->size;
    written += type->size;

    REDUCE(size, type->size);

    for (i = 0; type->members[i].kind != UIPC_KIND_NONE; i++)
    {
        switch (type->members[i].kind)
        {
        case UIPC_KIND_STRING:
            delta = marshal_string(buffer, size, *(void **)(payload + type->members[i].offset));
            if (base)
                memset(base + type->members[i].offset, delta ? 0xFF : 0x0, sizeof(char*));
            buffer += delta;
            written += delta;
            REDUCE(size, delta);
            break;
        case UIPC_KIND_POINTER:
            delta = uipc_marshal_payload(buffer, size, 
                                         *(void **)(payload + type->members[i].offset), 
                                         type->members[i].pointee_type);
            if (base)
                memset(base + type->members[i].offset, delta ? 0xFF : 0x0, sizeof(void*));
            buffer += delta;
            written += delta;
            REDUCE(size, delta);
            break;
        default:
            ;
        }
    }
    
    return written;
}

static
unsigned long
unmarshal_string(void** out, const void* payload)
{
    if (payload)
    {
        *out = strdup((const char*) payload);    
        return strlen((const char*) payload) + 1;
    }
    else
    {
        *out = NULL;
        return 0;
    }
    
}

unsigned long
uipc_unmarshal_payload(void** out, const void* payload, uipc_typeinfo* type)
{
    int i;
    void* object;
    unsigned long delta;
    unsigned long read = 0;
    void* member;
    const void* base = payload;

    object = malloc(type->size);
    memcpy(object, payload, type->size);

    payload += type->size;
    read += type->size;

    for (i = 0; type->members[i].kind != UIPC_KIND_NONE; i++)
    {
        switch (type->members[i].kind)
        {
        case UIPC_KIND_STRING:
            /* Structures in payload may be unaligned, so access with memcpy */
            memcpy(&member, base + type->members[i].offset, sizeof(member));
            if (member)
            {
                delta = unmarshal_string(object + type->members[i].offset, payload);
                payload += delta;
                read += delta;
            }
            else
            {
                *(void**) (object + type->members[i].offset) = NULL;
            }
            break;
        case UIPC_KIND_POINTER:
            memcpy(&member, base + type->members[i].offset, sizeof(member));
            if (member)
            {
                delta = uipc_unmarshal_payload(object + type->members[i].offset, payload, 
                                               type->members[i].pointee_type);
                payload += delta;
                read += delta;
            }
            else
            {
                *(void**) (object + type->members[i].offset) = NULL;
            }
            break;
        default:
            ;
        }
    }
    
    *out = object;

    return read;
}

void
uipc_free_object(void* object, uipc_typeinfo* type)
{
    int i;
    void* member;

    if (!object)
        return;

    for (i = 0; type->members[i].kind != UIPC_KIND_NONE; i++)
    {
        switch (type->members[i].kind)
        {
        case UIPC_KIND_STRING:
            member = *(void**) (object + type->members[i].offset);
            free(member);
            break;
        case UIPC_KIND_POINTER:
            member = *(void**) (object + type->members[i].offset);
            uipc_free_object(member, type->members[i].pointee_type);
            break;
        default:
            ;
        }
    }

    free(object);
}
