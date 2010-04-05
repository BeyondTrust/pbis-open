/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

#ifndef __GPA_HASH_H__
#define __GPA_HASH_H__

#include "includes.h"
/**
 * @defgroup SHashTable Stable hash tables
 * @brief Hash tables which guarantee iteration order
 */
/*@{*/

/** 
 * @brief Key/value pair
 *
 * Represents a key/value pair in a stable hash table
 */
typedef struct
{
    /**
     * Pointer to key of user-defined type
     */
    gpapointer key;
    /**
     * Pointer to value of user-defined type
     */
    gpapointer value;
} GPASHASH_PAIR, *PGPASHASH_PAIR;

/**
 * @brief Stable hash table
 *
 * Wraps the glib GPAHashTable and GPAList structures to
 * allow iteration of keys in the order of their insertion
 */
typedef struct GPASHASH_TABLE
{
    /**
     * The underlying GPAHashTable
     */
    GPAHashTable* table;
    /**
     * A GPAList containing elements of type GPASHASH_PAIR.
     * This list may be iterated to visit all keys/values
     * in their insertion order
     */
    GPAList* list;
    /**
     *@cond PRIVATE_FIELDS
     */
    GPADestroyNotify key_free, value_free;
    BOOLEAN dynamic;
    /**
     *@endcond
     */
} GPASHASH_TABLE, *PGPASHASH_TABLE;

/**
 * @brief Initialize a table
 *
 * Initializes a stable hash table with the specified parameters.  This
 * function is intended to initialize a GPASHASH_TABLE which is allocated
 * on the stack or as part of a structure.
 *
 * @param tab @in a pointer to the table structure
 * @param hash_func @in the key hash function
 * @param equal_func @in the key comparison function
 * @param key_free @in a function that will be called to free keys evicted from the table
 * @param value_free @in a function that will be called to free values evicted from the table
 * @errcode
 * @canfail
 */
CENTERROR
GPAStableHashTableInit(
    PGPASHASH_TABLE tab,
    GPAHashFunc hash_func,
    GPAEqualFunc equal_func,
    GPADestroyNotify key_free,
    GPADestroyNotify value_free
    );

/**
 * @brief Allocate and initialize a table
 *
 * Allocates and initializes a stable hash table with the specified parameters.
 *
 * @param tab @out a pointer where the address of the new table will be stored
 * @param hash_func @in the key hash function
 * @param equal_func @in the key comparison function
 * @param key_free @in a function that will be called to free keys evicted from the table
 * @param value_free @in a function that will be called to free values evicted from the table
 * @errcode
 * @canfail
 */
CENTERROR
GPAStableHashTableNew(
    PGPASHASH_TABLE* tab,
    GPAHashFunc hash_func,
    GPAEqualFunc equal_func,
    GPADestroyNotify key_free,
    GPADestroyNotify value_free
    );

/**
 * @brief Insert a key
 * 
 * Inserts a key/value pair into the table.  If the key was not
 * previously present in the table, it will added to the end
 * of the iteration order.  If it was already present, the
 * previous pair will be replaced and the iteration order will
 * be unchanged.
 * @param tab @in the table in which to insert
 * @param key @in a key of user-defined type to insert into the table
 * @param value @in a value of user-defined associated with key
 * @errcode
 * @canfail
 */
CENTERROR
GPAStableHashTableInsert(
    PGPASHASH_TABLE tab,
    gpapointer key,
    gpapointer value
    );

/**
 * @brief Remove a key
 *
 * Removes a key and its associated value from the table.  If the
 * key was not present, no action is taken.  If key_free and
 * value_free were specified in the creation of this table, the
 * removed items will automatically be freed.
 * @param tab @in the table from which to remove the key
 * @param key @in the key to remove
 * @wontfail
 */
   
void
GPAStableHashTableRemove(
    PGPASHASH_TABLE tab,
    gpaconstpointer key
    );

/**
 * @brief Look up a key
 *
 * Looks up a key in a table and returns the associated value.
 * @param tab @in the table in which to perform the lookup
 * @param key @in a key of user-defined type to look up
 * @return the value of user-defined type associated with the key, or
 * NULL if the key was not present in the table
 * @wontfail
 */
gpapointer
GPAStableHashTableLookup(
    PGPASHASH_TABLE tab,
    gpaconstpointer key
    );

/**
 * @brief Deallocate a table
 *
 * Deallocates a table and all associated memory; this includes all
 * keys and values present in the table if the table was created with
 * non-null key_free and value_free parameters.  If the table was
 * allocated with GPAStableHashTableNew, the GPASHASH_TABLE structure
 * itself will also be freed.
 * @param tab @in the table to deallocate
 * @wontfail
 */
void
GPAStableHashTableFree(
    PGPASHASH_TABLE tab
    );

/*@}*/

#endif

