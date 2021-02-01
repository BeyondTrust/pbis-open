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

#ifndef __CT_HASH_H__
#define __CT_HASH_H__

LW_BEGIN_EXTERN_C

typedef struct __CT_HASH_ENTRY CT_HASH_ENTRY;

typedef int (*CT_HASH_KEY_COMPARE)(PCVOID, PCVOID);
typedef size_t (*CT_HASH_KEY)(PCVOID);
typedef void (*CT_HASH_FREE_ENTRY)(const CT_HASH_ENTRY *);
typedef DWORD (*CT_HASH_COPY_ENTRY)(const CT_HASH_ENTRY *, CT_HASH_ENTRY *);

struct __CT_HASH_ENTRY
{
    PVOID pKey;
    PVOID pValue;
    struct __CT_HASH_ENTRY* pNext;
};

typedef struct __CT_HASH_TABLE
{
    size_t sTableSize;
    size_t sCount;
    CT_HASH_ENTRY **ppEntries;
    CT_HASH_KEY_COMPARE fnComparator;
    CT_HASH_KEY fnHash;
    CT_HASH_FREE_ENTRY fnFree;
    CT_HASH_COPY_ENTRY fnCopy;
} CT_HASH_TABLE, *PCT_HASH_TABLE;

typedef struct __CT_HASH_ITERATOR
{
    CT_HASH_TABLE *pTable;
    size_t sEntryIndex;
    CT_HASH_ENTRY *pEntryPos;
} CT_HASH_ITERATOR;

DWORD
CtHashCreate(
    size_t sTableSize,
    CT_HASH_KEY_COMPARE fnComparator,
    CT_HASH_KEY fnHash,
    CT_HASH_FREE_ENTRY fnFree, //optional
    CT_HASH_COPY_ENTRY fnCopy, //optional
    CT_HASH_TABLE** ppResult
    );

size_t
CtHashGetKeyCount(
    PCT_HASH_TABLE pTable
    );

void
CtHashRemoveAll(
        CT_HASH_TABLE* pResult);

void
CtHashSafeFree(
    CT_HASH_TABLE** ppResult
    );

DWORD
CtHashSetValue(
    CT_HASH_TABLE *pTable,
    PVOID  pKey,
    PVOID  pValue
    );

//Returns ERROR_NOT_FOUND if pKey is not in the table
DWORD
CtHashGetValue(
    CT_HASH_TABLE *pTable,
    PCVOID  pKey,
    PVOID* ppValue
    );

BOOLEAN
CtHashExists(
    IN PCT_HASH_TABLE pTable,
    IN PCVOID pKey
    );

DWORD
CtHashCopy(
    IN  CT_HASH_TABLE *pTable,
    OUT CT_HASH_TABLE **ppResult
    );

//Invalidates all iterators
DWORD
CtHashResize(
    CT_HASH_TABLE *pTable,
    size_t sTableSize
    );

DWORD
CtHashGetIterator(
    CT_HASH_TABLE *pTable,
    CT_HASH_ITERATOR *pIterator
    );

// returns NULL after passing the last entry
CT_HASH_ENTRY *
CtHashNext(
    CT_HASH_ITERATOR *pIterator
    );

DWORD
CtHashRemoveKey(
    CT_HASH_TABLE *pTable,
    PVOID  pKey
    );


DWORD
CtHashRemoveKey(
    CT_HASH_TABLE *pTable,
    PVOID  pKey
    );

int
CtHashStringCompare(
    PCVOID str1,
    PCVOID str2
    );

size_t
CtHashStringHash(
    PCVOID str
    );

int
CtHashPVoidCompare(
    IN PCVOID pvData1,
    IN PCVOID pvData2
    );

size_t
CtHashPVoidHash(
    IN PCVOID pvData
    );

LW_END_EXTERN_C

#endif
