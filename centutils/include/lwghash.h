/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

/*
 * Modified by Likewise Software Corporation 2007.
 */

#ifndef __LWG_HASH_H__
#define __LWG_HASH_H__

#include "lwgmissing.h"

LWG_BEGIN_DECLS

typedef struct _LWGHashTable  LWGHashTable;

typedef lwgboolean  (*LWGHRFunc)  (lwgpointer  key,
                               lwgpointer  value,
                               lwgpointer  user_data);

/* Hash tables
 */
LWGHashTable* lwg_hash_table_new		   (LWGHashFunc	    hash_func,
					    LWGEqualFunc	    key_equal_func);
LWGHashTable* lwg_hash_table_new_full      	   (LWGHashFunc	    hash_func,
					    LWGEqualFunc	    key_equal_func,
					    LWGDestroyNotify  key_destroy_func,
					    LWGDestroyNotify  value_destroy_func);
void	    lwg_hash_table_destroy	   (LWGHashTable	   *hash_table);
void	    lwg_hash_table_insert		   (LWGHashTable	   *hash_table,
					    lwgpointer	    key,
					    lwgpointer	    value);
void        lwg_hash_table_replace           (LWGHashTable     *hash_table,
					    lwgpointer	    key,
					    lwgpointer	    value);
lwgboolean    lwg_hash_table_remove		   (LWGHashTable	   *hash_table,
					    lwgconstpointer   key);
void        lwg_hash_table_remove_all        (LWGHashTable     *hash_table);
lwgboolean    lwg_hash_table_steal             (LWGHashTable     *hash_table,
					    lwgconstpointer   key);
void        lwg_hash_table_steal_all         (LWGHashTable     *hash_table);
lwgpointer    lwg_hash_table_lookup		   (LWGHashTable	   *hash_table,
					    lwgconstpointer   key);
lwgboolean    lwg_hash_table_lookup_extended   (LWGHashTable	   *hash_table,
					    lwgconstpointer   lookup_key,
					    lwgpointer	   *orilwg_key,
					    lwgpointer	   *value);
void	    lwg_hash_table_foreach	   (LWGHashTable	   *hash_table,
					    LWGHFunc	    func,
					    lwgpointer	    user_data);
lwgpointer    lwg_hash_table_find	           (LWGHashTable	   *hash_table,
					    LWGHRFunc	    predicate,
					    lwgpointer	    user_data);
lwguint	    lwg_hash_table_foreach_remove	   (LWGHashTable	   *hash_table,
					    LWGHRFunc	    func,
					    lwgpointer	    user_data);
lwguint	    lwg_hash_table_foreach_steal	   (LWGHashTable	   *hash_table,
					    LWGHRFunc	    func,
					    lwgpointer	    user_data);
lwguint	    lwg_hash_table_size		   (LWGHashTable	   *hash_table);

/* keeping hash tables alive */
LWGHashTable* lwg_hash_table_ref   		   (LWGHashTable 	   *hash_table);
void        lwg_hash_table_unref             (LWGHashTable     *hash_table);

#ifndef LWG_DISABLE_DEPRECATED

/* The following two functions are deprecated and will be removed in
 * the next major release. They do no good. */
#define lwg_hash_table_freeze(hash_table) ((void)0)
#define lwg_hash_table_thaw(hash_table) ((void)0)

#endif /* LWG_DISABLE_DEPRECATED */

/* Hash Functions
 */
lwgboolean lwg_str_equal (lwgconstpointer  v1,
                      lwgconstpointer  v2);
lwguint    lwg_str_hash  (lwgconstpointer  v);

lwgboolean lwg_int_equal (lwgconstpointer  v1,
                      lwgconstpointer  v2);
lwguint    lwg_int_hash  (lwgconstpointer  v);

/* This "hash" function will just return the key's address as an
 * unsigned integer. Useful for hashing on plain addresses or
 * simple integer values.
 * Passing NULL into lwg_hash_table_new() as LWGHashFunc has the
 * same effect as passing lwg_direct_hash().
 */
lwguint    lwg_direct_hash  (lwgconstpointer  v) LWG_GNUC_CONST;
lwgboolean lwg_direct_equal (lwgconstpointer  v1,
                         lwgconstpointer  v2) LWG_GNUC_CONST;

LWG_END_DECLS

#endif /* __LWG_HASH_H__ */

