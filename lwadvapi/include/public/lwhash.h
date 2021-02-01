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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwhash.h
 *
 * Abstract:
 *
 *        BeyondTrust Advanced API (lwadvapi) Hash Functions
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *          
 */
#ifndef __LWHASH_H__
#define __LWHASH_H__

#include <lw/types.h>
#include <lw/attrs.h>

LW_BEGIN_EXTERN_C

typedef struct __LW_HASH_ENTRY LW_HASH_ENTRY;

typedef int (*LW_HASH_KEY_COMPARE)(PCVOID, PCVOID);
typedef size_t (*LW_HASH_KEY)(PCVOID);
typedef void (*LW_HASH_FREE_ENTRY)(const LW_HASH_ENTRY *);
typedef DWORD (*LW_HASH_COPY_ENTRY)(const LW_HASH_ENTRY *, LW_HASH_ENTRY *);

struct __LW_HASH_ENTRY
{
    PVOID pKey;
    PVOID pValue;
    //Next value in chain
    struct __LW_HASH_ENTRY *pNext;
};

typedef struct __LW_HASH_TABLE
{
    size_t sTableSize;
    size_t sCount;
    LW_HASH_ENTRY **ppEntries;
    LW_HASH_KEY_COMPARE fnComparator;
    LW_HASH_KEY fnHash;
    LW_HASH_FREE_ENTRY fnFree;
    LW_HASH_COPY_ENTRY fnCopy;
} LW_HASH_TABLE, *PLW_HASH_TABLE;

typedef struct __LW_HASH_ITERATOR
{
    LW_HASH_TABLE *pTable;
    size_t sEntryIndex;
    LW_HASH_ENTRY *pEntryPos;
} LW_HASH_ITERATOR;

DWORD
LwHashCreate(
    size_t sTableSize,
    LW_HASH_KEY_COMPARE fnComparator,
    LW_HASH_KEY fnHash,
    LW_HASH_FREE_ENTRY fnFree, //optional
    LW_HASH_COPY_ENTRY fnCopy, //optional
    LW_HASH_TABLE** ppResult
    );

size_t
LwHashGetKeyCount(
    PLW_HASH_TABLE pTable
    );

void
LwHashRemoveAll(
        LW_HASH_TABLE* pResult);

void
LwHashSafeFree(
    LW_HASH_TABLE** ppResult
    );

DWORD
LwHashSetValue(
    LW_HASH_TABLE *pTable,
    PVOID  pKey,
    PVOID  pValue
    );

//Returns ERROR_NOT_FOUND if pKey is not in the table
DWORD
LwHashGetValue(
    LW_HASH_TABLE *pTable,
    PCVOID  pKey,
    PVOID* ppValue
    );

BOOLEAN
LwHashExists(
    IN PLW_HASH_TABLE pTable,
    IN PCVOID pKey
    );

DWORD
LwHashCopy(
    IN  LW_HASH_TABLE *pTable,
    OUT LW_HASH_TABLE **ppResult
    );

//Invalidates all iterators
DWORD
LwHashResize(
    LW_HASH_TABLE *pTable,
    size_t sTableSize
    );

DWORD
LwHashGetIterator(
    LW_HASH_TABLE *pTable,
    LW_HASH_ITERATOR *pIterator
    );

// returns NULL after passing the last entry
LW_HASH_ENTRY *
LwHashNext(
    LW_HASH_ITERATOR *pIterator
    );

DWORD
LwHashRemoveKey(
    LW_HASH_TABLE *pTable,
    PVOID  pKey
    );

int
LwHashStringCompare(
    PCVOID str1,
    PCVOID str2
    );

size_t
LwHashStringHash(
    PCVOID str
    );

int
LwHashCaselessStringCompare(
    PCVOID str1,
    PCVOID str2
    );

size_t
LwHashCaselessStringHash(
    PCVOID str
    );

int
LwHashPVoidCompare(
    IN PCVOID pvData1,
    IN PCVOID pvData2
    );

size_t
LwHashPVoidHash(
    IN PCVOID pvData
    );

VOID
LwHashFreeStringKey(
    IN OUT const LW_HASH_ENTRY *pEntry
    );

/**
 * @brief Free a string hash entry, i.e. where both the key and value are strings.
 */
VOID
LwHashFreeStringEntry(
    IN OUT const LW_HASH_ENTRY *pEntry
    );

LW_END_EXTERN_C

#endif /* __LWHASH_H__ */
