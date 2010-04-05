/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Modified by Likewise Software Corporation 2007.
 */

#ifndef __GPA_HASHTABLE_H__
#define __GPA_HASHTABLE_H__

#include "includes.h"

typedef struct _GPAHashNode
{
  gpapointer   key;
  gpapointer   value;
  struct _GPAHashNode *next;
}GPAHashNode;

typedef struct _GPAHashTable
{
  gpaint             size;
  gpaint             nnodes;
  GPAHashNode      **nodes;
  GPAHashFunc        hash_func;
  GPAEqualFunc       key_equal_func;
  volatile gpaint    ref_count;
  GPADestroyNotify   key_destroy_func;
  GPADestroyNotify   value_destroy_func;
}GPAHashTable;


typedef gpaboolean  (*GPAHRFunc)  (gpapointer  key,
                               gpapointer  value,
                               gpapointer  user_data);

/* Hash tables
 */
GPAHashTable* gpa_hash_table_new		   (GPAHashFunc	    hash_func,
					    GPAEqualFunc	    key_equal_func);
GPAHashTable* gpa_hash_table_new_full      	   (GPAHashFunc	    hash_func,
					    GPAEqualFunc	    key_equal_func,
					    GPADestroyNotify  key_destroy_func,
					    GPADestroyNotify  value_destroy_func);
void	    gpa_hash_table_destroy	   (GPAHashTable	   *hash_table);
void	    gpa_hash_table_insert		   (GPAHashTable	   *hash_table,
					    gpapointer	    key,
					    gpapointer	    value);
void        gpa_hash_table_replace           (GPAHashTable     *hash_table,
					    gpapointer	    key,
					    gpapointer	    value);
gpaboolean    gpa_hash_table_remove		   (GPAHashTable	   *hash_table,
					    gpaconstpointer   key);
void        gpa_hash_table_remove_all        (GPAHashTable     *hash_table);
gpaboolean    gpa_hash_table_steal             (GPAHashTable     *hash_table,
					    gpaconstpointer   key);
void        gpa_hash_table_steal_all         (GPAHashTable     *hash_table);
gpapointer    gpa_hash_table_lookup		   (GPAHashTable	   *hash_table,
					    gpaconstpointer   key);
gpaboolean    gpa_hash_table_lookup_extended   (GPAHashTable	   *hash_table,
					    gpaconstpointer   lookup_key,
					    gpapointer	   *origpa_key,
					    gpapointer	   *value);
void	    gpa_hash_table_foreach	   (GPAHashTable	   *hash_table,
					    GPAHFunc	    func,
					    gpapointer	    user_data);
gpapointer    gpa_hash_table_find	           (GPAHashTable	   *hash_table,
					    GPAHRFunc	    predicate,
					    gpapointer	    user_data);
gpauint	    gpa_hash_table_foreach_remove	   (GPAHashTable	   *hash_table,
					    GPAHRFunc	    func,
					    gpapointer	    user_data);
gpauint	    gpa_hash_table_foreach_steal	   (GPAHashTable	   *hash_table,
					    GPAHRFunc	    func,
					    gpapointer	    user_data);
gpauint	    gpa_hash_table_size		   (GPAHashTable	   *hash_table);

/* keeping hash tables alive */
GPAHashTable* gpa_hash_table_ref   		   (GPAHashTable 	   *hash_table);
void        gpa_hash_table_unref             (GPAHashTable     *hash_table);

#ifndef GPA_DISABLE_DEPRECATED

/* The following two functions are deprecated and will be removed in
 * the next major release. They do no good. */
#define gpa_hash_table_freeze(hash_table) ((void)0)
#define gpa_hash_table_thaw(hash_table) ((void)0)

#endif /* GPA_DISABLE_DEPRECATED */

/* Hash Functions
 */
gpaboolean gpa_str_equal (gpaconstpointer  v1,
                      gpaconstpointer  v2);
gpauint    gpa_str_hash  (gpaconstpointer  v);

gpaboolean gpa_int_equal (gpaconstpointer  v1,
                      gpaconstpointer  v2);
gpauint    gpa_int_hash  (gpaconstpointer  v);

/* This "hash" function will just return the key's address as an
 * unsigned integer. Useful for hashing on plain addresses or
 * simple integer values.
 * Passing NULL into gpa_hash_table_new() as GPAHashFunc has the
 * same effect as passing gpa_direct_hash().
 */
gpauint    gpa_direct_hash  (gpaconstpointer  v) GPA_GNUC_CONST;
gpaboolean gpa_direct_equal (gpaconstpointer  v1,
                         gpaconstpointer  v2) GPA_GNUC_CONST;

#endif /* __GPA_HASH_H__ */

