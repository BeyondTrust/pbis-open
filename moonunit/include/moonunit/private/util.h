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

#ifndef __MU_UTIL_H__
#define __MU_UTIL_H__

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <moonunit/internal/boilerplate.h>

C_BEGIN_DECLS

bool match_path (const char* path, const char* pattern);
bool ends_with (const char* haystack, const char* needle);
char* format(const char* format, ...);
char* formatv(const char* format, va_list ap);
const char* basename_pure(const char* filename);
void* mu_dlopen(const char* path, int flags);
char* safe_strdup(const char* in);

/* Dynamic array */

typedef void* array;

array* array_new(void);
size_t array_size(array* a);
array* array_append(array* a, void* e);
void array_free(array* a);
array* array_dup(array* a);
array* array_from_generic(void** g);

/* Hash table */

typedef struct _hashtable hashtable;
typedef bool (*hashequal) (const void* a, const void *b, void* data);
typedef size_t (*hashfunc) (const void* a, void* data);
typedef void (*hashfree) (void* key, void* value, void* data);

hashtable* hashtable_new(size_t size, hashfunc hash, hashequal equal, hashfree free, void* data);
void hashtable_set(hashtable* table, void* key, void* value);
void* hashtable_get(hashtable* table, const void* key);
bool hashtable_present(hashtable* table, const void* key);
void hashtable_remove(hashtable* table, void* key);
void hashtable_free(hashtable* table);

/* Useful standard hash functions */
bool string_hashequal(const void* a, const void* b, void* unused);
size_t string_hashfunc(const void* key, void* unused);

/* Ini file reading */

typedef void (*inievent) (const char* section, const char* key, const char* value, void* data);

void ini_read(FILE* file, inievent cb, void* data);

C_END_DECLS

#endif
