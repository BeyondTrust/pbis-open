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

#include <moonunit/option.h>
#include <moonunit/private/util.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef void (*bool_setter) (void* object, bool value);
typedef void (*int_setter) (void* object, int value);
typedef void (*float_setter) (void* object, double value);
typedef void (*string_setter) (void* object, const char* value);
typedef void (*pointer_setter) (void* object, void* value);

typedef bool (*bool_getter) (void* object);
typedef int (*int_getter) (void* object);
typedef double (*double_getter) (void* object);
typedef const char* (*string_getter) (void* object);
typedef void* (*pointer_getter) (void* object);

static void
set_bool(MuOption* option, void* object, bool value)
{
    ((bool_setter) option->set)(object, value);
}

static void
set_integer(MuOption* option, void* object, int value)
{
    ((int_setter) option->set)(object, value);
}

static void
set_float(MuOption* option, void* object, double value)
{
    ((float_setter) option->set)(object, value);
}

static void
set_string(MuOption* option, void* object, const char* value)
{
    ((string_setter) option->set)(object, value);
}

static void
set_pointer(MuOption* option, void* object, void* value)
{
    ((pointer_setter) option->set)(object, value);
}

static bool
get_bool(MuOption* option, void* object)
{
    return ((bool_getter) option->get)(object);
}

static int
get_integer(MuOption* option, void* object)
{
    return ((int_getter) option->get)(object);
}

static double
get_float(MuOption* option, void* object)
{
    return ((double_getter) option->get)(object);
}

static const char*
get_string(MuOption* option, void* object)
{
    return ((string_getter) option->get)(object);
}

static void*
get_pointer(MuOption* option, void* object)
{
    return ((pointer_getter) option->get)(object);
}

static MuOption*
lookup(MuOption* table, const char* name)
{
    unsigned int i;

    for (i = 0; table[i].name; i++)
    {
        if (!strcmp(name, table[i].name))
            return &table[i];
    }

    return NULL;
}

void
Mu_Option_SetString(MuOption* table, void* object, const char *name, const char* value)
{
    MuOption* option = lookup(table, name);

    if (!option)
        return;

    switch (option->type)
    {
    case MU_TYPE_BOOLEAN:
        if (!strcmp(value, "yes") ||
            !strcmp(value, "y") ||
            !strcmp(value, "true"))
            set_bool(option, object, true);
        else
            set_bool(option, object, false);
        break;
    case MU_TYPE_INTEGER:
        set_integer(option, object, atoi(value));
        break;
    case MU_TYPE_FLOAT:
        set_float(option, object, atof(value));
        break;
    case MU_TYPE_STRING:
        set_string(option, object, value);
        break;
    case MU_TYPE_POINTER:
        set_pointer(option, object, (void*) value);
        break;
    case MU_TYPE_UNKNOWN:
        break;
    }

    return;
}

void
Mu_Option_Setv(MuOption* table, void* object, const char *name, va_list ap)
{
    MuOption* option = lookup(table, name);

    if (!option)
        return;

    switch (option->type)
    {
    case MU_TYPE_BOOLEAN:
        set_bool(option, object, (bool) va_arg(ap, int));
        break;
    case MU_TYPE_INTEGER:
        set_integer(option, object, va_arg(ap, int));
        break;
    case MU_TYPE_FLOAT:
        set_float(option, object, va_arg(ap, double));
        break;
    case MU_TYPE_STRING:
        set_string(option, object, va_arg(ap, const char*));
        break;
    case MU_TYPE_POINTER:
        set_pointer(option, object, va_arg(ap, void*));
        break;
    case MU_TYPE_UNKNOWN:
        break;
    }
}

void
Mu_Option_Set(MuOption* table, void* object, const char *name, ...)
{
    va_list ap;

    va_start(ap, name);

    Mu_Option_Setv(table, object, name, ap);

    va_end(ap);
}

void
Mu_Option_Get(MuOption* table, void* object, const char *name, void* res)
{
    MuOption* option = lookup(table, name);

    if (!option)
        return;
    
    switch (option->type)
    {
    case MU_TYPE_BOOLEAN:
        *(bool*) res = get_bool(option, object);
        break;
    case MU_TYPE_INTEGER:
        *(int*) res = get_integer(option, object);
        break;
    case MU_TYPE_FLOAT:
        *(double*) res = get_float(option, object);
        break;
    case MU_TYPE_STRING:
        *(const char**) res = get_string(option, object);
        break;
    case MU_TYPE_POINTER:
        *(void **) res = get_pointer(option, object);
        break;
    case MU_TYPE_UNKNOWN:
        break;
    }
}

char*
Mu_Option_GetString(MuOption* table, void* object, const char *name)
{
    MuOption* option = lookup(table, name);

    if (!option)
        return NULL;
    
    switch (option->type)
    {
    case MU_TYPE_BOOLEAN:
    {
        bool res = get_bool(option, object);
        if (res)
            return strdup("true");
        else
            return strdup("false");
    }
    case MU_TYPE_INTEGER:
        return format("%i", get_integer(option, object));
    case MU_TYPE_FLOAT:
        return format("%f", get_float(option, object));
    case MU_TYPE_STRING:
    {
        const char* str = get_string(option, object);
        if (!str)
            return strdup("<none>");
        else
            return strdup(str);
    }
    case MU_TYPE_POINTER:
        return format("0x%lx", (unsigned long) get_pointer(option, object));
    case MU_TYPE_UNKNOWN:
    default:
        return NULL;
    }
}

MuType
Mu_Option_Type(MuOption* table, const char* name)
{
    MuOption* option = lookup(table, name);

    if (!option)
        return MU_TYPE_UNKNOWN;
    else
        return option->type;
}

const char*
Mu_Option_Description(MuOption* table, const char *name)
{
    MuOption* option = lookup(table, name);

    if (!option)
        return NULL;
    else
        return option->name;
}
