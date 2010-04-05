/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#include "includes.h"

DWORD
LWArrayConstruct(
    DynamicArray* array,
    size_t itemSize
    )
{
    DWORD dwError = 0;
    array->size = 0;
    array->capacity = 32;

    dwError = LWAllocateMemory(array->capacity*itemSize, (PVOID*)&array->data);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWSetCapacity(
    DynamicArray *array,
    size_t itemSize,
    size_t capacity
    )
{
    DWORD dwError = 0;

    /* Resize the array */
    dwError = LWReallocMemory(array->data, &array->data, capacity * itemSize);
    BAIL_ON_LWUTIL_ERROR(dwError);
    
    array->capacity = capacity;
    if (array->size > capacity)
    {
        array->size = capacity;
    }

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWArrayInsert(
    DynamicArray *array,
    int insertPos,
    int itemSize,
    const void *data,
    size_t dataLen
    )
{
    DWORD dwError = 0;

    if (array->size + dataLen > array->capacity)
    {
        /* Resize the array */
        dwError = LWSetCapacity(array, itemSize, array->capacity + dataLen + array->capacity);
        BAIL_ON_LWUTIL_ERROR(dwError);
    }
    
    /* Make room for the new value */
    memmove((char *)array->data + (insertPos + dataLen)*itemSize,
            (char *)array->data + insertPos*itemSize,
            (array->size - insertPos)*itemSize);
    
    memcpy((char *)array->data + insertPos*itemSize, data, dataLen*itemSize);
    
    array->size += dataLen;
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWArrayAppend(
    DynamicArray* array,
    int           itemSize,
    const void *  data,
    size_t        dataLen
    )
{
    return LWArrayInsert(
                array,
                array->size,
                itemSize,
                data,
                dataLen);
}

DWORD
LWArrayRemove(
    DynamicArray *array,
    int removePos,
    int itemSize,
    size_t dataLen
    )
{
    DWORD dwError = 0;
    
    if (dataLen + removePos > array->size)
    {
        dataLen = array->size - removePos;
    }
    
    if (dataLen < 0)
    {
        dwError = EINVAL;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }
    
    memmove((char *)array->data + removePos*itemSize,
            (char *)array->data + (removePos + dataLen)*itemSize,
            (array->size - removePos - dataLen)*itemSize);
    array->size -= dataLen;
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

size_t
LWArrayRemoveHead(
    DynamicArray *array,
    int    itemSize,
    void * store,
    size_t dataLen
    )
{
    if (dataLen > array->size)
    {
        dataLen = array->size;
    }
    
    if (store != NULL)
    {
        memcpy(store, array->data, dataLen * itemSize);
    }
    
    LWArrayRemove(array, 0, itemSize, dataLen);
    
    return dataLen;
}

VOID
LWArrayFree(
    DynamicArray* array
    )
{
    if (array->data)
    {
        LWFreeMemory(array->data);
    }
    
    array->size = 0;
    array->capacity = 0;
}

PVOID
LWArrayGetItem(
    DynamicArray *array,
    size_t index,
    size_t itemSize
    )
{
    PVOID pItem = NULL;
    
    if (index < array->size)
    {
        pItem = (char *)array->data + index * itemSize;
    }
    
    return pItem;
}

ssize_t
LWArrayFindString(
    DynamicArray* array,
    PCSTR find
    )
{
    ssize_t match = -1;
    size_t i;
    
    for(i = 0; i < array->size; i++)
    {
        PVOID pItem = LWArrayGetItem(array, i, sizeof(PCSTR));
        
        if (!strcmp(*(PCSTR *)pItem, find))
        {
            match = i;
            break;
        }
    }
    
    return match;
}
