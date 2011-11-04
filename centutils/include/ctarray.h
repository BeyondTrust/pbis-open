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

#ifndef __CT_ARRAY_H__
#define __CT_ARRAY_H__

/**
 * @brief A dynamically resizeable array
 *
 * This encapsulates a dynamically resizeable array of a user defined type. The
 * data pointer requires casting to the user's preferred type.
 */

typedef struct
{
    void *data;
    /**
     * The number of items in the array in terms of the type this array holds,
     * not in terms of bytes.
     */
    size_t size;
    /**
     * The number of items that can be stored without having to reallocate
     * memory. This is in items, not bytes
     */
    size_t capacity;
} DynamicArray;



LW_BEGIN_EXTERN_C

DWORD CTArrayConstruct(DynamicArray* array, size_t itemSize);

/**
 * Change the available space in the array
 *
 * @errcode
 * @canfail
 */
DWORD CTSetCapacity(DynamicArray *array, size_t itemSize, size_t capacity);

/**
 * Insert one or more items into the array at any position.
 *
 * @errcode
 * @canfail
 */
DWORD CTArrayInsert(DynamicArray *array, int insertPos, int itemSize, const void *data, size_t dataLen);

/**
 * Append one or more items to the end of the array.
 * dataLen is in number of elements, not number of bytes.
 *
 * @errcode
 * @canfail
 */
DWORD CTArrayAppend(DynamicArray *array, int itemSize, const void *data, size_t dataLen);

/**
 * Remove one or more items from the array at any position. This will not
 * shrink the allocated memory (capacity stays the same).
 *
 * @errcode
 * @canfail
 */
DWORD CTArrayRemove(DynamicArray *array, int removePos, int itemSize, size_t dataLen);

/**
 * Pop items off of the head of the list. They first get copied into the user
 * supplied buffer, then they are removed from the front of the array.
 *
 * @return the number of items removed
 */
size_t CTArrayRemoveHead(DynamicArray *array, int itemSize, void *store, size_t dataLen);

/**
 * Free the memory associated with a dynamic array, and zero out the pointers.
 * The dynamic array essentialy becomes a zero length array. The object can be
 * reused by appending new data after it has been freed.
 *
 * @wontfail
 */
void CTArrayFree(DynamicArray *array);

void * CTArrayGetItem(DynamicArray* array, size_t index, size_t itemSize);

// Only works when the DynamicArray holds (char *)
ssize_t CTArrayFindString(DynamicArray* array, PCSTR find);

LW_END_EXTERN_C


#endif
