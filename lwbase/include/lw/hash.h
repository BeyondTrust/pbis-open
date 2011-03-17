/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        hash.h
 *
 * Abstract:
 *
 *        Hash table and hash map APIs
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LWBASE_HASH_H__
#define __LWBASE_HASH_H__

#include <lw/types.h>
#include <lw/attrs.h>

LW_BEGIN_EXTERN_C

/**
 * @file hash.h
 * @brief Hash APIs
 */

/**
 * @defgroup hashtable Hash tables
 * @brief Embedded-node hash tables
 *
 * The RTL hash table API provides a chained hash table implementation
 * with hash chain nodes which are embedded directly into the user's data
 * structures.  This allows insertions and removals without allocating
 * or freeing additional memory, which comes with several benefits:
 *
 * - Insertions are guaranteed to succeed (no out-of-memory errors)
 * - Better cache locality because hash chain nodes are contiguous
 *   in memory with inserted elements
 * - Reduced memory overhead due to fewer discrete allocations
 * - Better performance when repeatedly inserting and removing
 *   the same elements since no malloc/free calls are necessary
 *
 * Drawbacks:
 *
 * - Not as intuitive to use as a hash map
 * - The structure to insert must contain its own key
 * - A structure can only be inserted into X tables at a time,
 *   where X is the number of node fields in the structure.
 *
 * To use this API, add a #LW_HASHTABLE_NODE field to your structure
 * and insert a pointer to the field into your table.  When given a
 * node pointer, you can recover your original structure with
 * #LW_STRUCT_FROM_FIELD(pNode, StructName, NodeFieldName).  In addition
 * to the usual digest and equality callback functions, you will need
 * to provide a "get key" function that takes a node and returns the key
 * from your structure.
 *
 * If you need to insert your structure into multiple tables simultaneously,
 * you will need a separate node field for each table.
 *
 * If you don't care about the insertion guarantees or performance benefits and
 * want to use an easier API, use @ref hashmap.
 */

/*@{*/

/**
 * @brief Hash table node structure
 *
 * A node which can be inserted and retrieved from a hash table.
 * This structure should be treated as opaque.
 *
 * @hideinitializer
 */
typedef struct _LW_HASHTABLE_NODE
{
    struct _LW_HASHTABLE_NODE* pNext;
    LW_ULONG ulDigest;
} LW_HASHTABLE_NODE, *PLW_HASHTABLE_NODE;

/**
 * @brief Hash table iterator
 *
 * This structure allows you to iterate over a hash table
 * with #LwRtlHashTableIterate().  It should be initialized with
 * #LW_HASH_ITER_INIT.
 *
 * @hideinitializer
 */
typedef struct _LW_HASHTABLE_ITER
{
    PLW_HASHTABLE_NODE pNext;
    LW_ULONG ulIndex;
} LW_HASHTABLE_ITER, *PLW_HASHTABLE_ITER;

/**
 * @brief Hash table iterator initializer
 *
 * A suitable value for statically initializing #LW_HASH_ITER structures
 *
 * @hideinitializer
 */
#define LW_HASHTABLE_ITER_INIT {NULL, 0}

/**
 * @brief Hash table structure
 *
 * An opaque hash table structure
 */
typedef struct _LW_HASHTABLE *PLW_HASHTABLE;
typedef struct _LW_HASHTABLE const *PCLW_HASHTABLE;

/**
 * @brief Key fetch function
 *
 * A callback function that returns the key associated with
 * a hash table node.
 *
 * @param[in] pNode the hash node
 * @param[in] pUserData arbitrary user data
 * @return a constant pointer to the key
 */
typedef LW_PCVOID
(*LW_HASH_GET_KEY_FUNCTION)(
    PLW_HASHTABLE_NODE pNode,
    LW_PVOID pUserData
    );

/**
 * @brief Key digest function
 *
 * A callback function that returns a digested form of a key
 *
 * @param[in] pKey the key
 * @param[in] pUserData arbitrary user data
 * @return a suitable digest
 */
typedef LW_ULONG
(*LW_HASH_DIGEST_FUNCTION)(
    LW_PCVOID pKey,
    LW_PVOID pUserData
    );

/**
 * @brief Key equality function
 *
 * A callback function which determines if two keys are equal
 *
 * @param[in] pKey1 the first key
 * @param[in] pKey2 the second key
 * @param[in] pUserData arbitrary user data
 * @retval TRUE the keys are equal
 * @retval FALSE the keys are not equal
 */
typedef LW_BOOLEAN
(*LW_HASH_EQUAL_FUNCTION)(
    LW_PCVOID pKey1,
    LW_PCVOID pKey2,
    LW_PVOID pUserData
    );

/**
 * @brief Node free function
 *
 * A callback function used by #LwRtlHashTableClear() to optionally
 * free any nodes cleared from the table.
 *
 * @param[in] pNode a node
 * @param[in] pUserData arbitrary user data
 */
typedef VOID
(*LW_HASHNODE_FREE_FUNCTION)(
    PLW_HASHTABLE_NODE pNode,
    LW_PVOID pUserData
    );

/**
 * @brief Create a hash table
 *
 * Creates a new hash table with the specified callback functions and
 * initial size.
 *
 * @param[out] ppTable the created table
 * @param[in] pfnGetKey the key fetch function
 * @param[in] pfnDigest the key digest function
 * @param[in] pfnEqual the key equality function
 * @param[in] pUserData arbitrary user data to pass to callback functions
 * @param[in] ulSize the initial size of the table
 * @retval LW_STATUS_SUCCESS success
 * @retval LW_STATUS_INSUFFICIENT_RESOURCES out of memory
 */
LW_NTSTATUS
LwRtlCreateHashTable(
    LW_OUT PLW_HASHTABLE* ppTable,
    LW_IN LW_HASH_GET_KEY_FUNCTION pfnGetKey,
    LW_IN LW_HASH_DIGEST_FUNCTION pfnDigest,
    LW_IN LW_HASH_EQUAL_FUNCTION pfnEqual,
    LW_IN LW_OPTIONAL LW_PVOID pUserData,
    LW_IN LW_ULONG ulSize
    );

/**
 * @brief Insert node into table
 *
 * Inserts a node into the hash table, potentially replacing
 * an existing node with the same key.
 *
 * @param[in,out] pTable the hash table
 * @param[in] pNode the node to insert
 * @param[out] ppPrevNode if provided, set to the node which was kicked
 * out of the table or NULL if no node was evicted
 */
VOID
LwRtlHashTableInsert(
    LW_IN LW_OUT PLW_HASHTABLE pTable,
    LW_IN PLW_HASHTABLE_NODE pNode,
    LW_OUT LW_OPTIONAL PLW_HASHTABLE_NODE* ppPrevNode
    );

/**
 * @brief Insert node into table with automatic resizing
 *
 * Identical to #LwRtlHashTableInsert(), but first attempts to
 * resize the table if the load factor exceeds some threshold
 * (currently hard-coded at 80%) by calling #LwRtlHashTableResize().
 * If the resize attempt fails, the node is inserted anyway.
 *
 * @param[in,out] pTable the hash table
 * @param[in] pNode the node to insert
 * @param[out] ppPrevNode if provided, set to the node which was kicked
 * out of the table or NULL if no node was evicted
 */
VOID
LwRtlHashTableResizeAndInsert(
    LW_IN LW_OUT PLW_HASHTABLE pTable,
    LW_IN PLW_HASHTABLE_NODE pNode,
    LW_OUT LW_OPTIONAL PLW_HASHTABLE_NODE* ppPrevNode
    );

/**
 * @brief Remove node from table
 *
 * Removes the specified node from the table.
 *
 * @param[in,out] pTable the hash table
 * @param[in] pNode the node to remove
 * @retval LW_STATUS_SUCCESS success
 * @retval LW_STATUS_NOT_FOUND the specified node was not present in the table
 */
LW_NTSTATUS
LwRtlHashTableRemove(
    LW_IN LW_OUT PLW_HASHTABLE pTable,
    LW_IN PLW_HASHTABLE_NODE pNode
    );

/**
 * @brief Find node by key
 *
 * Finds a node in a hash table by the specified key.
 *
 * @param[in] pTable the hash table
 * @param[out] ppNode set to the node which was found, or NULL on failure
 * @param[in] pKey the key to search for
 * @retval LW_STATUS_SUCCESS the node was found
 * @retval LW_STATUS_NOT_FOUND no node with the specified key was found
 */
LW_NTSTATUS
LwRtlHashTableFindKey(
    LW_IN PCLW_HASHTABLE pTable,
    LW_OUT LW_OPTIONAL PLW_HASHTABLE_NODE* ppNode,
    LW_IN LW_PCVOID pKey
    );

/**
 * @brief Reset hash table iterator
 *
 * Resets the specified hash table iterator to the start of the table.
 *
 * @param[out] pIter the iterator to reset
 */
VOID
LwRtlHashTableResetIter(
    LW_OUT PLW_HASHTABLE_ITER pIter
    );

/**
 * @brief Iterate over nodes
 *
 * Returns the next node in the table, or NULL if the end
 * of the table has been reached for the given iterator.
 *
 * @warning This function has undefined behavior if the table is modified
 * in any way during iteration, with the following exception: a node
 * just returned by this function may be safely removed with
 * #LwRtlHashTableRemove() as long as no other iterator is in active
 * use for the given hash table.
 *
 * @param[in] pTable the hash table
 * @param[in,out] pIter an iterator which tracks the current position in the table
 * @return the next node
 */
PLW_HASHTABLE_NODE
LwRtlHashTableIterate(
    LW_IN PCLW_HASHTABLE pTable,
    LW_IN LW_OUT PLW_HASHTABLE_ITER pIter
    );

/**
 * @brief Clear hash table
 *
 * Removes all nodes from the given hash table.  If provided,
 * a free function is called on each removed node.
 *
 * @param[in,out] pTable the hash table
 * @param[in] pFree an optional callback to invoke on each removed node
 * @param[in] pUserData arbitrary user data to pass to pFree
 */
VOID
LwRtlHashTableClear(
    LW_IN LW_OUT PLW_HASHTABLE pTable,
    LW_IN LW_HASHNODE_FREE_FUNCTION pFree,
    LW_IN LW_PVOID pUserData
    );

/**
 * @brief Query hash table size
 *
 * Returns the current size of the given hash table.
 *
 * @param[in] pTable the hash table
 * @return the current size of the table
 */
ULONG
LwRtlHashTableGetSize(
    LW_IN PCLW_HASHTABLE pTable
    );

/**
 * @brief Query hash table node countsize
 *
 * Returns the current count of nodes in the the given hash table.
 *
 * @param[in] pTable the hash table
 * @return the current number of nodes in the table
 */
ULONG
LwRtlHashTableGetCount(
    LW_IN PCLW_HASHTABLE pTable
    );

/**
 * @brief Resize hash table
 *
 * Resizes the specified table, rehashing all present nodes.  This
 * can be an expensive operation for a large table.
 *
 * @param[in,out] pTable the hash table
 * @param[in] ulSize the new size of the table
 * @retval LW_STATUS_SUCCESS success
 * @retval LW_STATUS_INSUFFICIENT_RESOURCES out of memory
 */
LW_NTSTATUS
LwRtlHashTableResize(
    LW_IN LW_OUT PLW_HASHTABLE pTable,
    LW_ULONG ulSize
    );

/**
 * @brief Free hash table
 *
 * Frees the given hash table and sets the pointer to NULL.
 * If *ppTable is already NULL, no action is taken.
 *
 * @param[in,out] ppTable hash table pointer to free and set to NULL
 */
VOID
LwRtlFreeHashTable(
    LW_IN LW_OUT PLW_HASHTABLE* ppTable
    );

/*@}*/

/**
 * @defgroup hashmap Hash maps
 * @brief Key-value hash maps
 *
 * The RTL hash map API provides an associative array of key-value pairs
 * backed by a hash table.  Unlike the lower-level hash table API, insertions
 * are not guaranteed to succeed.
 */

/*@{*/

/**
 * @brief Hash map structure
 *
 * An opaque hash map structure
 */
typedef struct _LW_HASHMAP *PLW_HASHMAP;
typedef struct _LW_HASHMAP const *PCLW_HASHMAP;

/**
 * @brief Hash map iterator
 *
 * An iterator over the pairs in a hash map
 * @hideinitializer
 */
typedef struct _LW_HASHMAP_ITER
{
    LW_HASHTABLE_ITER Inner;
} LW_HASHMAP_ITER, *PLW_HASHMAP_ITER;

/**
 * @brief Hash pair
 *
 * A structure representing a key-value pair in a hash table
 */
typedef struct _LW_HASHMAP_PAIR
{
    LW_PVOID pKey;
    LW_PVOID pValue;
} LW_HASHMAP_PAIR, *PLW_HASHMAP_PAIR;

/**
 * @brief Pair free function
 *
 * A callback function used by #LwRtlHashMapClear() to optionally
 * free any key-value pairs cleared from the table.
 *
 * @param[in] pPair the pair to free
 * @param[in] pUserData arbitrary user data
 */
typedef LW_VOID
(*LW_HASHPAIR_FREE_FUNCTION)(
    PLW_HASHMAP_PAIR pPair,
    LW_PVOID pUserData
    );

/**
 * @brief Hash map iterator initializer
 *
 * A suitable value for statically initializing #LW_HASHMAP_ITER structures
 *
 * @hideinitializer
 */
#define LW_HASHMAP_ITER_INIT {{NULL, 0}}

/**
 * @brief Create a hash map
 *
 * Creates a new hash map with the specified callback functions.
 *
 * @param[out] ppMap the created map
 * @param[in] pfnDigest the key digest function
 * @param[in] pfnEqual the key equality function
 * @param[in] pUserData arbitrary user data to pass to callback functions
 * @retval LW_STATUS_SUCCESS success
 * @retval LW_STATUS_INSUFFICIENT_RESOURCES out of memory
 */
LW_NTSTATUS
LwRtlCreateHashMap(
    LW_OUT PLW_HASHMAP* ppMap,
    LW_IN LW_HASH_DIGEST_FUNCTION pfnDigest,
    LW_IN LW_HASH_EQUAL_FUNCTION pfnEqual,
    LW_IN LW_OPTIONAL LW_PVOID pUserData
    );

/**
 * @brief Insert pair into map
 *
 * Inserts a key-value pair into the hash map,
 * potentially replacing an existing pair with the same key.
 *
 * @param[in,out] pMap the hash map
 * @param[in] pKey the key to insert
 * @param[in] pValue the value to insert
 * @param[out] pPrevPair if provided, filled in with the
 * previous pair which was kicked out of the table, or NULL
 * if no such pair existed
 * @retval LW_STATUS_SUCCESS success
 * @retval LW_STATUS_INSUFFICIENT_RESOURCES out of memory
 */
LW_NTSTATUS
LwRtlHashMapInsert(
    LW_IN LW_OUT PLW_HASHMAP pMap,
    LW_IN LW_PVOID pKey,
    LW_IN LW_PVOID pValue,
    LW_OUT LW_OPTIONAL PLW_HASHMAP_PAIR pPrevPair
    );

/**
 * @brief Remove pair from map
 *
 * Removes the key-value pair with the specified key from the map.
 *
 * @param[in,out] pMap the hash map
 * @param[in] pKey the key to remove
 * @param[out] pPair filled in with the removed pair if provided
 * @retval LW_STATUS_SUCCESS success
 * @retval LW_STATUS_NOT_FOUND the specified key was not present in the map
 */
LW_NTSTATUS
LwRtlHashMapRemove(
    LW_IN LW_OUT PLW_HASHMAP pMap,
    LW_IN LW_PCVOID pKey,
    LW_OUT LW_OPTIONAL PLW_HASHMAP_PAIR pPair
    );

/**
 * @brief Find value by key
 *
 * Finds a value in a hash map by the specified key.
 *
 * @param[in] pMap the hash map
 * @param[out] ppValue set to the value which was found, or NULL on failure
 * @param[in] pKey the key to search for
 * @retval LW_STATUS_SUCCESS the node was found
 * @retval LW_STATUS_NOT_FOUND no node with the specified key was found
 */
LW_NTSTATUS
LwRtlHashMapFindKey(
    LW_IN PCLW_HASHMAP pMap,
    LW_OUT LW_OPTIONAL LW_PVOID* ppValue,
    LW_IN LW_PCVOID pKey
    );

/**
 * @brief Reset hash map iterator
 *
 * Resets the specified hash map iterator to the start of the map.
 *
 * @param[out] pIter the iterator to reset
 */
VOID
LwRtlHashMapResetIter(
    LW_OUT PLW_HASHMAP_ITER pIter
    );

/**
 * @brief Iterate over key-value pairs
 *
 * Fetches the next key-value pair in the map, returning FALSE
 * if the end of the map has been reached for the given iterator.
 *
 * @warning This function has undefined behavior if the map is modified
 * in any way during iteration, with the following exception: a pair
 * just returned by this function may be safely removed with
 * #LwRtlHashMapRemove() as long as no other iterator is in active
 * use for the given hash map.
 *
 * @param[in] pMap the hash map
 * @param[in,out] pIter an iterator which tracks the current position in the map
 * @param[out] pPair the next pair
 * @retval TRUE another pair was placed in pPair
 * @retval FALSE the end of the map was reached
 */
BOOLEAN
LwRtlHashMapIterate(
    LW_IN PCLW_HASHMAP pMap,
    LW_IN LW_OUT PLW_HASHMAP_ITER pIter,
    LW_OUT PLW_HASHMAP_PAIR pPair
    );

/**
 * @brief Clear hash map
 *
 * Removes all nodes from the given hash table.  If provided,
 * a free function is called on each removed node.
 *
 * @param[in,out] pMap the hash map
 * @param[in] pFree an optional callback to invoke on each removed node
 * @param[in] pUserData arbitrary user data to pass to pFree
 */
VOID
LwRtlHashMapClear(
    LW_IN LW_OUT PLW_HASHMAP pMap,
    LW_IN LW_OPTIONAL LW_HASHPAIR_FREE_FUNCTION pFree,
    LW_IN LW_OPTIONAL LW_PVOID pUserData
    );

/**
 * @brief Query hash table pair count
 *
 * Returns the current count of pairs in the the given hash map.
 *
 * @param[in] pMap the hash map
 * @return the current number of nodes in the table
 */
ULONG
LwRtlHashMapGetCount(
    LW_IN PCLW_HASHMAP pMap
    );

/**
 * @brief Free hash map
 *
 * Frees the given hash map and sets the pointer to NULL.
 * If *ppTable is already NULL, no action is taken.
 *
 * @param[in,out] ppMap hash map pointer to free and set to NULL
 */
VOID
LwRtlFreeHashMap(
    LW_IN LW_OUT PLW_HASHMAP* ppMap
    );

/*@}*/

/**
 * @defgroup hashutil Hash convenience functions
 * @brief Hash convenience functions
 *
 * A set of ready-to-use callback functions for hash tables and hash maps
 * for common data types.
 */

/*@{*/

/**
 * @brief Digest function for multi-byte strings (case sensitive)
 */
LW_ULONG
LwRtlHashDigestPstr(
    LW_PCVOID pKey,
    LW_PVOID pUnused
    );

/**
 * @brief Equality function for multi-byte strings (case sensitive)
 */
LW_BOOLEAN
LwRtlHashEqualPstr(
    LW_PCVOID pKey1,
    LW_PCVOID pKey2,
    LW_PVOID pUnused
    );

/**
 * @brief Digest function for multi-byte strings (case insensitive)
 */
LW_ULONG
LwRtlHashDigestPstrCaseless(
    LW_PCVOID pKey,
    LW_PVOID pUnused
    );

/**
 * @brief Equality function for multi-byte strings (case insensitive)
 */
LW_BOOLEAN
LwRtlHashEqualPstrCaseless(
    LW_PCVOID pKey1,
    LW_PCVOID pKey2,
    LW_PVOID pUnused
    );

/**
 * @brief Digest function for UTF-16 strings (case sensitive)
 */
LW_ULONG
LwRtlHashDigestPwstr(
    LW_PCVOID pKey,
    LW_PVOID pUnused
    );

/**
 * @brief Equality function for UTF-16 strings (case sensitive)
 */
LW_BOOLEAN
LwRtlHashEqualPwstr(
    LW_PCVOID pKey1,
    LW_PCVOID pKey2,
    LW_PVOID pUnused
    );

/**
 * @brief Digest function for UTF-16 strings (case insensitive)
 */
LW_ULONG
LwRtlHashDigestPwstrCaseless(
    LW_PCVOID pKey,
    LW_PVOID pUnused
    );

/**
 * @brief Equality function for UTF-16 strings (case insensitive)
 */
LW_BOOLEAN
LwRtlHashEqualPwstrCaseless(
    LW_PCVOID pKey1,
    LW_PCVOID pKey2,
    LW_PVOID pUnused
    );

/**
 * @brief Digest function for generic pointers (address equality)
 */
LW_ULONG
LwRtlHashDigestPointer(
    LW_PCVOID pKey,
    LW_PVOID pUnused
    );

/**
 * @brief Equality function for generic pointers (address equality)
 */
LW_BOOLEAN
LwRtlHashEqualPointer(
    LW_PCVOID pKey1,
    LW_PCVOID pKey2,
    LW_PVOID pUnused
    );

#define LwRtlHashDigestCString LwRtlHashDigestPstr
#define LwRtlHashDigestWC16String LwRtlHashDigestPwstr
#define LwRtlHashDigestCStringCaseless LwRtlHashDigestPstrCaseless
#define LwRtlHashDigestWC16StringCaseless LwRtlHashDigestPwstrCaseless
#define LwRtlHashEqualCString LwRtlHashEqualPstr
#define LwRtlHashEqualWC16String LwRtlHashEqualPwstr
#define LwRtlHashEqualCStringCaseless LwRtlHashEqualPstrCaseless
#define LwRtlHashEqualWC16StringCaseless LwRtlHashEqualPwstrCaseless

/*@}*/

LW_END_EXTERN_C

#endif
