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

#include <lwglib.h>

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
    lwgpointer key;
    /**
     * Pointer to value of user-defined type
     */
    lwgpointer value;
} SHASH_PAIR, *PSHASH_PAIR;

/**
 * @brief Stable hash table
 *
 * Wraps the glib LWGHashTable and LWGList structures to
 * allow iteration of keys in the order of their insertion
 */
typedef struct SHASH_TABLE
{
    /**
     * The underlying LWGHashTable
     */
    LWGHashTable* table;
    /**
     * A LWGList containing elements of type SHASH_PAIR.
     * This list may be iterated to visit all keys/values
     * in their insertion order
     */
    LWGList* list;
    /**
     *@cond PRIVATE_FIELDS
     */
    LWGDestroyNotify key_free, value_free;
    BOOLEAN dynamic;
    /**
     *@endcond
     */
} SHASH_TABLE, *PSHASH_TABLE;


LW_BEGIN_EXTERN_C

/**
 * @brief Initialize a table
 *
 * Initializes a stable hash table with the specified parameters.  This
 * function is intended to initialize a SHASH_TABLE which is allocated
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
CTStableHashTableInit(
    PSHASH_TABLE tab,
    LWGHashFunc hash_func,
    LWGEqualFunc equal_func,
    LWGDestroyNotify key_free,
    LWGDestroyNotify value_free
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
CTStableHashTableNew(
    PSHASH_TABLE* tab,
    LWGHashFunc hash_func,
    LWGEqualFunc equal_func,
    LWGDestroyNotify key_free,
    LWGDestroyNotify value_free
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
CTStableHashTableInsert(
    PSHASH_TABLE tab,
    lwgpointer key,
    lwgpointer value
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
CTStableHashTableRemove(
    PSHASH_TABLE tab,
    lwgconstpointer key
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
lwgpointer
CTStableHashTableLookup(
    PSHASH_TABLE tab,
    lwgconstpointer key
    );

/**
 * @brief Deallocate a table
 *
 * Deallocates a table and all associated memory; this includes all
 * keys and values present in the table if the table was created with
 * non-null key_free and value_free parameters.  If the table was
 * allocated with CTStableHashTableNew, the SHASH_TABLE structure
 * itself will also be freed.
 * @param tab @in the table to deallocate
 * @wontfail
 */
void
CTStableHashTableFree(
    PSHASH_TABLE tab
    );

LW_END_EXTERN_C


/*@}*/

#endif

