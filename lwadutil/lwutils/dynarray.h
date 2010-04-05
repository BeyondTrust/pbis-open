/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

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

DWORD
LWArrayConstruct(
    DynamicArray* array,
    size_t        itemSize
    );

/**
 * Change the available space in the array
 *
 * @errcode
 * @canfail
 */
DWORD
LWSetCapacity(
    DynamicArray *array,
    size_t itemSize,
    size_t capacity
    );

/**
 * Insert one or more items into the array at any position.
 *
 * @errcode
 * @canfail
 */
DWORD
LWArrayInsert(
    DynamicArray* array,
    int           insertPos,
    int           itemSize,
    const void *  data,
    size_t        dataLen
    );

/**
 * Append one or more items to the end of the array.
 * dataLen is in number of elements, not number of bytes.
 *
 * @errcode
 * @canfail
 */
DWORD
LWArrayAppend(
    DynamicArray* array,
    int           itemSize,
    const void *  data,
    size_t        dataLen
    );

/**
 * Remove one or more items from the array at any position. This will not
 * shrink the allocated memory (capacity stays the same).
 *
 * @errcode
 * @canfail
 */
DWORD
LWArrayRemove(
    DynamicArray* array,
    int           removePos,
    int           itemSize,
    size_t        dataLen
    );

/**
 * Pop items off of the head of the list. They first get copied into the user
 * supplied buffer, then they are removed from the front of the array.
 *
 * @return the number of items removed
 */
size_t
LWArrayRemoveHead(
    DynamicArray * array,
    int            itemSize,
    void *         store,
    size_t         dataLen
    );

/**
 * Free the memory associated with a dynamic array, and zero out the pointers.
 * The dynamic array essentialy becomes a zero length array. The object can be
 * reused by appending new data after it has been freed.
 *
 * @wontfail
 */
void
LWArrayFree(
    DynamicArray *array
    );

void *
LWArrayGetItem(
    DynamicArray* array,
    size_t index,
    size_t itemSize
    );

// Only works when the DynamicArray holds (char *)
ssize_t
LWArrayFindString(
    DynamicArray* array,
    PCSTR find
    );

#endif
