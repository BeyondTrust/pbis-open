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

#include "includes.h"

CENTERROR GPAArrayConstruct(GPADynamicArray* array, size_t itemSize)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    array->size = 0;
    array->capacity = 32;

    ceError = LwAllocateMemory(array->capacity*itemSize, 
                               (PVOID*)&array->data);
    BAIL_ON_GPA_ERROR(ceError);
error:
    return ceError;
}

CENTERROR GPASetCapacity(GPADynamicArray *array, size_t itemSize, size_t capacity)
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* Resize the array */
    ceError = LwReallocMemory(array->data, &array->data, capacity * itemSize);
    BAIL_ON_GPA_ERROR(ceError);
    array->capacity = capacity;
    if(array->size > capacity)
        array->size = capacity;

error:
    return ceError;
}

CENTERROR GPAArrayInsert(GPADynamicArray *array, int insertPos, int itemSize, const void *data, size_t dataLen)
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if(array->size + dataLen > array->capacity)
    {
        /* Resize the array */
        ceError = GPASetCapacity(array, itemSize, array->capacity + dataLen + array->capacity);
        BAIL_ON_GPA_ERROR(ceError);
    }
    /* Make room for the new value */
    memmove((char *)array->data + (insertPos + dataLen)*itemSize,
            (char *)array->data + insertPos*itemSize,
            (array->size - insertPos)*itemSize);
    memcpy((char *)array->data + insertPos*itemSize, data, dataLen*itemSize);
    array->size += dataLen;
error:
    return ceError;
}

CENTERROR GPAArrayAppend(GPADynamicArray *array, int itemSize, const void *data, size_t dataLen)
{
    return GPAArrayInsert(array, array->size, itemSize, data, dataLen);
}

CENTERROR GPAArrayRemove(GPADynamicArray *array, int removePos, int itemSize, size_t dataLen)
{
    if(dataLen + removePos > array->size)
        dataLen = array->size - removePos;
    if(dataLen < 0)
        return CENTERROR_INVALID_PARAMETER;
    memmove((char *)array->data + removePos*itemSize,
            (char *)array->data + (removePos + dataLen)*itemSize,
            (array->size - removePos - dataLen)*itemSize);
    array->size -= dataLen;
    return CENTERROR_SUCCESS;
}

size_t GPAArrayRemoveHead(GPADynamicArray *array, int itemSize, void *store, size_t dataLen)
{
    if(dataLen > array->size)
        dataLen = array->size;
    if(store != NULL)
    {
        memcpy(store, array->data, dataLen * itemSize);
    }
    GPAArrayRemove(array, 0, itemSize, dataLen);
    return dataLen;
}

void GPAArrayFree(GPADynamicArray *array)
{
    LW_SAFE_FREE_MEMORY(array->data);
    array->size = 0;
    array->capacity = 0;
}

void * GPAArrayGetItem(GPADynamicArray *array, size_t index, size_t itemSize)
{
    if(index >= array->size)
        return NULL;
    return (char *)array->data + index*itemSize;
}

ssize_t GPAArrayFindString(GPADynamicArray* array, PCSTR find)
{
    size_t i;
    for(i = 0; i < array->size; i++)
    {
        if(!strcmp(*(PCSTR *)GPAArrayGetItem(array, i, sizeof(PCSTR)), find))
            return i;
    }
    return -1;
}
