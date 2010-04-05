/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#include "includes.h"

#define HASH_TABLE_MIN_SIZE 11
#define HASH_TABLE_MAX_SIZE 13845163

#define GPA_HASH_TABLE_RESIZE(hash_table)				\
   GPA_STMT_START {						\
     if ((hash_table->size >= 3 * hash_table->nnodes &&	        \
	  hash_table->size > HASH_TABLE_MIN_SIZE) ||		\
	 (3 * hash_table->size <= hash_table->nnodes &&	        \
	  hash_table->size < HASH_TABLE_MAX_SIZE))		\
	   gpa_hash_table_resize (hash_table);			\
   } GPA_STMT_END

static void		gpa_hash_table_resize	  (GPAHashTable	  *hash_table);
static GPAHashNode**	gpa_hash_table_lookup_node  (GPAHashTable     *hash_table,
                                                   gpaconstpointer   key);
static GPAHashNode*	gpa_hash_node_new		  (gpapointer	   key,
                                                   gpapointer        value);
static void		gpa_hash_node_destroy	  (GPAHashNode	  *hash_node,
                                                   GPADestroyNotify  key_destroy_func,
                                                   GPADestroyNotify  value_destroy_func);
static void		gpa_hash_nodes_destroy	  (GPAHashNode	  *hash_node,
						  GPADestroyNotify   key_destroy_func,
						  GPADestroyNotify   value_destroy_func);
static gpauint gpa_hash_table_foreach_remove_or_steal (GPAHashTable     *hash_table,
                                                   GPAHRFunc	   func,
                                                   gpapointer	   user_data,
                                                   gpaboolean        notify);


static const gpauint gpa_primes[] =
{
  11,
  19,
  37,
  73,
  109,
  163,
  251,
  367,
  557,
  823,
  1237,
  1861,
  2777,
  4177,
  6247,
  9371,
  14057,
  21089,
  31627,
  47431,
  71143,
  106721,
  160073,
  240101,
  360163,
  540217,
  810343,
  1215497,
  1823231,
  2734867,
  4102283,
  6153409,
  9230113,
  13845163,
};

static const gpauint gpa_nprimes = sizeof (gpa_primes) / sizeof (gpa_primes[0]);

gpauint
gpa_spaced_primes_closest (gpauint num)
{
  gpaint i;

  for (i = 0; i < gpa_nprimes; i++)
    if (gpa_primes[i] > num)
      return gpa_primes[i];

  return gpa_primes[gpa_nprimes - 1];
}

/**
 * gpa_hash_table_new:
 * @hash_func: a function to create a hash value from a key.
 *   Hash values are used to determine where keys are stored within the
 *   #GPAHashTable data structure. The gpa_direct_hash(), gpa_int_hash() and 
 *   gpa_str_hash() functions are provided for some common types of keys. 
 *   If hash_func is %NULL, gpa_direct_hash() is used.
 * @key_equal_func: a function to check two keys for equality.  This is
 *   used when looking up keys in the #GPAHashTable.  The gpa_direct_equal(),
 *   gpa_int_equal() and gpa_str_equal() functions are provided for the most
 *   common types of keys. If @key_equal_func is %NULL, keys are compared
 *   directly in a similar fashion to gpa_direct_equal(), but without the
 *   overhead of a function call.
 *
 * Creates a new #GPAHashTable with a reference count of 1.
 * 
 * Return value: a new #GPAHashTable.
 **/
GPAHashTable*
gpa_hash_table_new (GPAHashFunc    hash_func,
		  GPAEqualFunc   key_equal_func)
{
  return gpa_hash_table_new_full (hash_func, key_equal_func, NULL, NULL);
}


/**
 * gpa_hash_table_new_full:
 * @hash_func: a function to create a hash value from a key.
 * @key_equal_func: a function to check two keys for equality.
 * @key_destroy_func: a function to free the memory allocated for the key 
 *   used when removing the entry from the #GPAHashTable or %NULL if you 
 *   don't want to supply such a function.
 * @value_destroy_func: a function to free the memory allocated for the 
 *   value used when removing the entry from the #GPAHashTable or %NULL if 
 *   you don't want to supply such a function.
 * 
 * Creates a new #GPAHashTable like gpa_hash_table_new() with a reference count
 * of 1 and allows to specify functions to free the memory allocated for the
 * key and value that get called when removing the entry from the #GPAHashTable.
 * 
 * Return value: a new #GPAHashTable.
 **/
GPAHashTable*
gpa_hash_table_new_full (GPAHashFunc       hash_func,
		       GPAEqualFunc      key_equal_func,
		       GPADestroyNotify  key_destroy_func,
		       GPADestroyNotify  value_destroy_func)
{
  GPAHashTable *hash_table;
  
  hash_table = gpa_slice_new (GPAHashTable);
  hash_table->size               = HASH_TABLE_MIN_SIZE;
  hash_table->nnodes             = 0;
  hash_table->hash_func          = hash_func ? hash_func : gpa_direct_hash;
  hash_table->key_equal_func     = key_equal_func;
  hash_table->ref_count          = 1;
  hash_table->key_destroy_func   = key_destroy_func;
  hash_table->value_destroy_func = value_destroy_func;
  hash_table->nodes              = gpa_new0 (GPAHashNode*, hash_table->size);
  
  return hash_table;
}


/**
 * gpa_hash_table_ref:
 * @hash_table: a valid #GPAHashTable.
 * 
 * Atomically increments the reference count of @hash_table by one.
 * This function is MT-safe and may be called from any thread.
 * 
 * Return value: the passed in #GPAHashTable.
 * 
 * Since: 2.10
 **/
GPAHashTable*
gpa_hash_table_ref (GPAHashTable *hash_table)
{
  gpa_return_val_if_fail (hash_table != NULL, NULL);
  gpa_return_val_if_fail (hash_table->ref_count > 0, hash_table);

  gpa_atomic_int_add (&hash_table->ref_count, 1);
  return hash_table;
}

/**
 * gpa_hash_table_unref:
 * @hash_table: a valid #GPAHashTable.
 * 
 * Atomically decrements the reference count of @hash_table by one.
 * If the reference count drops to 0, all keys and values will be
 * destroyed, and all memory allocated by the hash table is released.
 * This function is MT-safe and may be called from any thread.
 * 
 * Since: 2.10
 **/
void
gpa_hash_table_unref (GPAHashTable *hash_table)
{
  gpa_return_if_fail (hash_table != NULL);
  gpa_return_if_fail (hash_table->ref_count > 0);

  if (gpa_atomic_int_exchange_and_add (&hash_table->ref_count, -1) - 1 == 0)
    {
      gpaint i;

      for (i = 0; i < hash_table->size; i++)
        gpa_hash_nodes_destroy (hash_table->nodes[i], 
                              hash_table->key_destroy_func,
                              hash_table->value_destroy_func);
      gpa_free (hash_table->nodes);
      gpa_slice_free (GPAHashTable, hash_table);
    }
}

/**
 * gpa_hash_table_destroy:
 * @hash_table: a #GPAHashTable.
 * 
 * Destroys all keys and values in the #GPAHashTable and decrements its
 * reference count by 1. If keys and/or values are dynamically allocated,
 * you should either free them first or create the #GPAHashTable with destroy
 * notifiers using gpa_hash_table_new_full(). In the latter case the destroy
 * functions you supplied will be called on all keys and values during the
 * destruction phase.
 **/
void
gpa_hash_table_destroy (GPAHashTable *hash_table)
{
  gpa_return_if_fail (hash_table != NULL);
  gpa_return_if_fail (hash_table->ref_count > 0);
  
  gpa_hash_table_remove_all (hash_table);
  gpa_hash_table_unref (hash_table);
}

static /*inline*/ GPAHashNode**
gpa_hash_table_lookup_node (GPAHashTable	*hash_table,
			  gpaconstpointer	 key)
{
  GPAHashNode **node;
  
  node = &hash_table->nodes
    [(* hash_table->hash_func) (key) % hash_table->size];
  
  /* Hash table lookup needs to be fast.
   *  We therefore remove the extra conditional of testing
   *  whether to call the key_equal_func or not from
   *  the inner loop.
   */
  if (hash_table->key_equal_func)
    while (*node && !(*hash_table->key_equal_func) ((*node)->key, key))
      node = &(*node)->next;
  else
    while (*node && (*node)->key != key)
      node = &(*node)->next;
  
  return node;
}

/**
 * gpa_hash_table_lookup:
 * @hash_table: a #GPAHashTable.
 * @key: the key to look up.
 * 
 * Looks up a key in a #GPAHashTable. Note that this function cannot
 * distinguish between a key that is not present and one which is present
 * and has the value %NULL. If you need this distinction, use
 * gpa_hash_table_lookup_extended().
 * 
 * Return value: the associated value, or %NULL if the key is not found.
 **/
gpapointer
gpa_hash_table_lookup (GPAHashTable	  *hash_table,
		     gpaconstpointer key)
{
  GPAHashNode *node;
  
  gpa_return_val_if_fail (hash_table != NULL, NULL);
  
  node = *gpa_hash_table_lookup_node (hash_table, key);
  
  return node ? node->value : NULL;
}

/**
 * gpa_hash_table_lookup_extended:
 * @hash_table: a #GPAHashTable.
 * @lookup_key: the key to look up.
 * @origpa_key: returns the original key.
 * @value: returns the value associated with the key.
 * 
 * Looks up a key in the #GPAHashTable, returning the original key and the
 * associated value and a #gpaboolean which is %TRUE if the key was found. This 
 * is useful if you need to free the memory allocated for the original key, 
 * for example before calling gpa_hash_table_remove().
 * 
 * Return value: %TRUE if the key was found in the #GPAHashTable.
 **/
gpaboolean
gpa_hash_table_lookup_extended (GPAHashTable    *hash_table,
			      gpaconstpointer  lookup_key,
			      gpapointer	    *origpa_key,
			      gpapointer	    *value)
{
  GPAHashNode *node;
  
  gpa_return_val_if_fail (hash_table != NULL, FALSE);
  
  node = *gpa_hash_table_lookup_node (hash_table, lookup_key);
  
  if (node)
    {
      if (origpa_key)
	*origpa_key = node->key;
      if (value)
	*value = node->value;
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * gpa_hash_table_insert:
 * @hash_table: a #GPAHashTable.
 * @key: a key to insert.
 * @value: the value to associate with the key.
 * 
 * Inserts a new key and value into a #GPAHashTable.
 * 
 * If the key already exists in the #GPAHashTable its current value is replaced
 * with the new value. If you supplied a @value_destroy_func when creating the 
 * #GPAHashTable, the old value is freed using that function. If you supplied
 * a @key_destroy_func when creating the #GPAHashTable, the passed key is freed 
 * using that function.
 **/
void
gpa_hash_table_insert (GPAHashTable *hash_table,
		     gpapointer	 key,
		     gpapointer	 value)
{
  GPAHashNode **node;
  
  gpa_return_if_fail (hash_table != NULL);
  gpa_return_if_fail (hash_table->ref_count > 0);
  
  node = gpa_hash_table_lookup_node (hash_table, key);
  
  if (*node)
    {
      /* do not reset node->key in this place, keeping
       * the old key is the intended behaviour. 
       * gpa_hash_table_replace() can be used instead.
       */

      /* free the passed key */
      if (hash_table->key_destroy_func)
	hash_table->key_destroy_func (key);
      
      if (hash_table->value_destroy_func)
	hash_table->value_destroy_func ((*node)->value);

      (*node)->value = value;
    }
  else
    {
      *node = gpa_hash_node_new (key, value);
      hash_table->nnodes++;
      GPA_HASH_TABLE_RESIZE (hash_table);
    }
}

/**
 * gpa_hash_table_replace:
 * @hash_table: a #GPAHashTable.
 * @key: a key to insert.
 * @value: the value to associate with the key.
 * 
 * Inserts a new key and value into a #GPAHashTable similar to 
 * gpa_hash_table_insert(). The difference is that if the key already exists 
 * in the #GPAHashTable, it gets replaced by the new key. If you supplied a 
 * @value_destroy_func when creating the #GPAHashTable, the old value is freed 
 * using that function. If you supplied a @key_destroy_func when creating the 
 * #GPAHashTable, the old key is freed using that function. 
 **/
void
gpa_hash_table_replace (GPAHashTable *hash_table,
		      gpapointer	  key,
		      gpapointer	  value)
{
  GPAHashNode **node;
  
  gpa_return_if_fail (hash_table != NULL);
  gpa_return_if_fail (hash_table->ref_count > 0);
  
  node = gpa_hash_table_lookup_node (hash_table, key);
  
  if (*node)
    {
      if (hash_table->key_destroy_func)
	hash_table->key_destroy_func ((*node)->key);
      
      if (hash_table->value_destroy_func)
	hash_table->value_destroy_func ((*node)->value);

      (*node)->key   = key;
      (*node)->value = value;
    }
  else
    {
      *node = gpa_hash_node_new (key, value);
      hash_table->nnodes++;
      GPA_HASH_TABLE_RESIZE (hash_table);
    }
}

/**
 * gpa_hash_table_remove:
 * @hash_table: a #GPAHashTable.
 * @key: the key to remove.
 * 
 * Removes a key and its associated value from a #GPAHashTable.
 *
 * If the #GPAHashTable was created using gpa_hash_table_new_full(), the
 * key and value are freed using the supplied destroy functions, otherwise
 * you have to make sure that any dynamically allocated values are freed 
 * yourself.
 * 
 * Return value: %TRUE if the key was found and removed from the #GPAHashTable.
 **/
gpaboolean
gpa_hash_table_remove (GPAHashTable	   *hash_table,
		     gpaconstpointer  key)
{
  GPAHashNode **node, *dest;
  
  gpa_return_val_if_fail (hash_table != NULL, FALSE);
  
  node = gpa_hash_table_lookup_node (hash_table, key);
  if (*node)
    {
      dest = *node;
      (*node) = dest->next;
      gpa_hash_node_destroy (dest, 
			   hash_table->key_destroy_func,
			   hash_table->value_destroy_func);
      hash_table->nnodes--;
  
      GPA_HASH_TABLE_RESIZE (hash_table);

      return TRUE;
    }

  return FALSE;
}

/**
 * gpa_hash_table_remove_all:
 * @hash_table: a #GPAHashTable
 *
 * Removes all keys and their associated values from a #GPAHashTable.
 *
 * If the #GPAHashTable was created using gpa_hash_table_new_full(), the keys
 * and values are freed using the supplied destroy functions, otherwise you
 * have to make sure that any dynamically allocated values are freed
 * yourself.
 *
 * Since: 2.12
 **/
void
gpa_hash_table_remove_all (GPAHashTable *hash_table)
{
  gpauint i;

  gpa_return_if_fail (hash_table != NULL);

  for (i = 0; i < hash_table->size; i++)
    {
      gpa_hash_nodes_destroy (hash_table->nodes[i],
                            hash_table->key_destroy_func,
                            hash_table->value_destroy_func);
      hash_table->nodes[i] = NULL;
    }
  hash_table->nnodes = 0;
  
  GPA_HASH_TABLE_RESIZE (hash_table);
}

/**
 * gpa_hash_table_steal:
 * @hash_table: a #GPAHashTable.
 * @key: the key to remove.
 * 
 * Removes a key and its associated value from a #GPAHashTable without
 * calling the key and value destroy functions.
 *
 * Return value: %TRUE if the key was found and removed from the #GPAHashTable.
 **/
gpaboolean
gpa_hash_table_steal (GPAHashTable    *hash_table,
                    gpaconstpointer  key)
{
  GPAHashNode **node, *dest;
  
  gpa_return_val_if_fail (hash_table != NULL, FALSE);
  
  node = gpa_hash_table_lookup_node (hash_table, key);
  if (*node)
    {
      dest = *node;
      (*node) = dest->next;
      gpa_hash_node_destroy (dest, NULL, NULL);
      hash_table->nnodes--;
  
      GPA_HASH_TABLE_RESIZE (hash_table);

      return TRUE;
    }

  return FALSE;
}

/**
 * gpa_hash_table_steal_all:
 * @hash_table: a #GPAHashTable.
 *
 * Removes all keys and their associated values from a #GPAHashTable 
 * without calling the key and value destroy functions.
 *
 * Since: 2.12
 **/
void
gpa_hash_table_steal_all (GPAHashTable *hash_table)
{
  gpauint i;

  gpa_return_if_fail (hash_table != NULL);

  for (i = 0; i < hash_table->size; i++)
    {
      gpa_hash_nodes_destroy (hash_table->nodes[i], NULL, NULL);
      hash_table->nodes[i] = NULL;
    }

  hash_table->nnodes = 0;

  GPA_HASH_TABLE_RESIZE (hash_table);
}

/**
 * gpa_hash_table_foreach_remove:
 * @hash_table: a #GPAHashTable.
 * @func: the function to call for each key/value pair.
 * @user_data: user data to pass to the function.
 * 
 * Calls the given function for each key/value pair in the #GPAHashTable.
 * If the function returns %TRUE, then the key/value pair is removed from the
 * #GPAHashTable. If you supplied key or value destroy functions when creating
 * the #GPAHashTable, they are used to free the memory allocated for the removed
 * keys and values.
 * 
 * Return value: the number of key/value pairs removed.
 **/
gpauint
gpa_hash_table_foreach_remove (GPAHashTable	*hash_table,
			     GPAHRFunc	 func,
			     gpapointer	 user_data)
{
  gpa_return_val_if_fail (hash_table != NULL, 0);
  gpa_return_val_if_fail (func != NULL, 0);
  
  return gpa_hash_table_foreach_remove_or_steal (hash_table, func, user_data, TRUE);
}

/**
 * gpa_hash_table_foreach_steal:
 * @hash_table: a #GPAHashTable.
 * @func: the function to call for each key/value pair.
 * @user_data: user data to pass to the function.
 * 
 * Calls the given function for each key/value pair in the #GPAHashTable.
 * If the function returns %TRUE, then the key/value pair is removed from the
 * #GPAHashTable, but no key or value destroy functions are called.
 * 
 * Return value: the number of key/value pairs removed.
 **/
gpauint
gpa_hash_table_foreach_steal (GPAHashTable *hash_table,
                            GPAHRFunc	func,
                            gpapointer	user_data)
{
  gpa_return_val_if_fail (hash_table != NULL, 0);
  gpa_return_val_if_fail (func != NULL, 0);
  
  return gpa_hash_table_foreach_remove_or_steal (hash_table, func, user_data, FALSE);
}

static gpauint
gpa_hash_table_foreach_remove_or_steal (GPAHashTable *hash_table,
                                      GPAHRFunc	  func,
                                      gpapointer	  user_data,
                                      gpaboolean    notify)
{
  GPAHashNode *node, *prev;
  gpaint i;
  gpauint deleted = 0;
  
  for (i = 0; i < hash_table->size; i++)
    {
    restart:
      
      prev = NULL;
      
      for (node = hash_table->nodes[i]; node; prev = node, node = node->next)
	{
	  if ((* func) (node->key, node->value, user_data))
	    {
	      deleted += 1;
	      
	      hash_table->nnodes -= 1;
	      
	      if (prev)
		{
		  prev->next = node->next;
		  gpa_hash_node_destroy (node,
				       notify ? hash_table->key_destroy_func : NULL,
				       notify ? hash_table->value_destroy_func : NULL);
		  node = prev;
		}
	      else
		{
		  hash_table->nodes[i] = node->next;
		  gpa_hash_node_destroy (node,
				       notify ? hash_table->key_destroy_func : NULL,
				       notify ? hash_table->value_destroy_func : NULL);
		  goto restart;
		}
	    }
	}
    }
  
  GPA_HASH_TABLE_RESIZE (hash_table);
  
  return deleted;
}

/**
 * gpa_hash_table_foreach:
 * @hash_table: a #GPAHashTable.
 * @func: the function to call for each key/value pair.
 * @user_data: user data to pass to the function.
 * 
 * Calls the given function for each of the key/value pairs in the
 * #GPAHashTable.  The function is passed the key and value of each
 * pair, and the given @user_data parameter.  The hash table may not
 * be modified while iterating over it (you can't add/remove
 * items). To remove all items matching a predicate, use
 * gpa_hash_table_foreach_remove().
 **/
void
gpa_hash_table_foreach (GPAHashTable *hash_table,
		      GPAHFunc	  func,
		      gpapointer	  user_data)
{
  GPAHashNode *node;
  gpaint i;
  
  gpa_return_if_fail (hash_table != NULL);
  gpa_return_if_fail (func != NULL);
  
  for (i = 0; i < hash_table->size; i++)
    for (node = hash_table->nodes[i]; node; node = node->next)
      (* func) (node->key, node->value, user_data);
}

/**
 * gpa_hash_table_find:
 * @hash_table: a #GPAHashTable.
 * @predicate:  function to test the key/value pairs for a certain property.
 * @user_data:  user data to pass to the function.
 * 
 * Calls the given function for key/value pairs in the #GPAHashTable until 
 * @predicate returns %TRUE.  The function is passed the key and value of 
 * each pair, and the given @user_data parameter. The hash table may not
 * be modified while iterating over it (you can't add/remove items). 
 *
 * Return value: The value of the first key/value pair is returned, for which 
 * func evaluates to %TRUE. If no pair with the requested property is found, 
 * %NULL is returned.
 *
 * Since: 2.4
 **/
gpapointer
gpa_hash_table_find (GPAHashTable	   *hash_table,
                   GPAHRFunc	    predicate,
                   gpapointer	    user_data)
{
  GPAHashNode *node;
  gpaint i;
  
  gpa_return_val_if_fail (hash_table != NULL, NULL);
  gpa_return_val_if_fail (predicate != NULL, NULL);
  
  for (i = 0; i < hash_table->size; i++)
    for (node = hash_table->nodes[i]; node; node = node->next)
      if (predicate (node->key, node->value, user_data))
        return node->value;       
  return NULL;
}

/**
 * gpa_hash_table_size:
 * @hash_table: a #GPAHashTable.
 * 
 * Returns the number of elements contained in the #GPAHashTable.
 * 
 * Return value: the number of key/value pairs in the #GPAHashTable.
 **/
gpauint
gpa_hash_table_size (GPAHashTable *hash_table)
{
  gpa_return_val_if_fail (hash_table != NULL, 0);
  
  return hash_table->nnodes;
}

static void
gpa_hash_table_resize (GPAHashTable *hash_table)
{
  GPAHashNode **new_nodes;
  GPAHashNode *node;
  GPAHashNode *next;
  gpauint hash_val;
  gpaint new_size;
  gpaint i;

  new_size = gpa_spaced_primes_closest (hash_table->nnodes);
  new_size = CLAMP (new_size, HASH_TABLE_MIN_SIZE, HASH_TABLE_MAX_SIZE);
 
  new_nodes = gpa_new0 (GPAHashNode*, new_size);
  
  for (i = 0; i < hash_table->size; i++)
    for (node = hash_table->nodes[i]; node; node = next)
      {
	next = node->next;

	hash_val = (* hash_table->hash_func) (node->key) % new_size;

	node->next = new_nodes[hash_val];
	new_nodes[hash_val] = node;
      }
  
  gpa_free (hash_table->nodes);
  hash_table->nodes = new_nodes;
  hash_table->size = new_size;
}

static GPAHashNode*
gpa_hash_node_new (gpapointer key,
		 gpapointer value)
{
  GPAHashNode *hash_node = gpa_slice_new (GPAHashNode);
  
  hash_node->key = key;
  hash_node->value = value;
  hash_node->next = NULL;
  
  return hash_node;
}

static void
gpa_hash_node_destroy (GPAHashNode      *hash_node,
		     GPADestroyNotify  key_destroy_func,
		     GPADestroyNotify  value_destroy_func)
{
  if (key_destroy_func)
    key_destroy_func (hash_node->key);
  if (value_destroy_func)
    value_destroy_func (hash_node->value);
  gpa_slice_free (GPAHashNode, hash_node);
}

static void
gpa_hash_nodes_destroy (GPAHashNode *hash_node,
		      GPAFreeFunc  key_destroy_func,
		      GPAFreeFunc  value_destroy_func)
{
  while (hash_node)
    {
      GPAHashNode *next = hash_node->next;
      if (key_destroy_func)
	key_destroy_func (hash_node->key);
      if (value_destroy_func)
	value_destroy_func (hash_node->value);
      gpa_slice_free (GPAHashNode, hash_node);
      hash_node = next;
    }
}

gpauint
gpa_direct_hash(gpaconstpointer ptr)
{
    union { gpaconstpointer ptr; gpauint ptrval; } tmp;
    tmp.ptr = ptr;
    return tmp.ptrval;
}

/*
#define __GPA_HASH_C__
#include "galiasdef.c"
*/
