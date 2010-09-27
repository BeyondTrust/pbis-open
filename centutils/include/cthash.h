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
