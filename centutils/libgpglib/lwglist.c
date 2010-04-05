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

/* 
 * MT safe
 */

/*
#include "config.h"

#include "glib.h"
#include "galias.h"
*/
#include "lwglist.h"

void lwg_list_push_allocator (lwgpointer dummy) { /* present for binary compat only */ }
void lwg_list_pop_allocator  (void)           { /* present for binary compat only */ }

#define _lwg_list_alloc()         lwg_slice_new (LWGList)
#define _lwg_list_alloc0()        lwg_slice_new0 (LWGList)
#define _lwg_list_free1(list)     lwg_slice_free (LWGList, list)

LWGList*
lwg_list_alloc (void)
{
  return _lwg_list_alloc0 ();
}

void
lwg_list_free (LWGList *list)
{
  lwg_slice_free_chain (LWGList, list, next);
}

void
lwg_list_free_1 (LWGList *list)
{
  _lwg_list_free1 (list);
}

LWGList*
lwg_list_append (LWGList	*list,
	       lwgpointer	 data)
{
  LWGList *new_list;
  LWGList *last;
  
  new_list = _lwg_list_alloc ();
  new_list->data = data;
  new_list->next = NULL;
  
  if (list)
    {
      last = lwg_list_last (list);
      /* lwg_assert (last != NULL); */
      last->next = new_list;
      new_list->prev = last;

      return list;
    }
  else
    {
      new_list->prev = NULL;
      return new_list;
    }
}

LWGList*
lwg_list_prepend (LWGList	 *list,
		lwgpointer  data)
{
  LWGList *new_list;
  
  new_list = _lwg_list_alloc ();
  new_list->data = data;
  new_list->next = list;
  
  if (list)
    {
      new_list->prev = list->prev;
      if (list->prev)
	list->prev->next = new_list;
      list->prev = new_list;
    }
  else
    new_list->prev = NULL;
  
  return new_list;
}

LWGList*
lwg_list_insert (LWGList	*list,
	       lwgpointer	 data,
	       lwgint	 position)
{
  LWGList *new_list;
  LWGList *tmp_list;
  
  if (position < 0)
    return lwg_list_append (list, data);
  else if (position == 0)
    return lwg_list_prepend (list, data);
  
  tmp_list = lwg_list_nth (list, position);
  if (!tmp_list)
    return lwg_list_append (list, data);
  
  new_list = _lwg_list_alloc ();
  new_list->data = data;
  new_list->prev = tmp_list->prev;
  if (tmp_list->prev)
    tmp_list->prev->next = new_list;
  new_list->next = tmp_list;
  tmp_list->prev = new_list;
  
  if (tmp_list == list)
    return new_list;
  else
    return list;
}

LWGList*
lwg_list_insert_before (LWGList   *list,
		      LWGList   *sibling,
		      lwgpointer data)
{
  if (!list)
    {
      list = lwg_list_alloc ();
      list->data = data;
      lwg_return_val_if_fail (sibling == NULL, list);
      return list;
    }
  else if (sibling)
    {
      LWGList *node;

      node = _lwg_list_alloc ();
      node->data = data;
      node->prev = sibling->prev;
      node->next = sibling;
      sibling->prev = node;
      if (node->prev)
	{
	  node->prev->next = node;
	  return list;
	}
      else
	{
	  lwg_return_val_if_fail (sibling == list, node);
	  return node;
	}
    }
  else
    {
      LWGList *last;

      last = list;
      while (last->next)
	last = last->next;

      last->next = _lwg_list_alloc ();
      last->next->data = data;
      last->next->prev = last;
      last->next->next = NULL;

      return list;
    }
}

LWGList *
lwg_list_concat (LWGList *list1, LWGList *list2)
{
  LWGList *tmp_list;
  
  if (list2)
    {
      tmp_list = lwg_list_last (list1);
      if (tmp_list)
	tmp_list->next = list2;
      else
	list1 = list2;
      list2->prev = tmp_list;
    }
  
  return list1;
}

LWGList*
lwg_list_remove (LWGList	     *list,
	       lwgconstpointer  data)
{
  LWGList *tmp;
  
  tmp = list;
  while (tmp)
    {
      if (tmp->data != data)
	tmp = tmp->next;
      else
	{
	  if (tmp->prev)
	    tmp->prev->next = tmp->next;
	  if (tmp->next)
	    tmp->next->prev = tmp->prev;
	  
	  if (list == tmp)
	    list = list->next;
	  
	  _lwg_list_free1 (tmp);
	  
	  break;
	}
    }
  return list;
}

LWGList*
lwg_list_remove_all (LWGList	*list,
		   lwgconstpointer data)
{
  LWGList *tmp = list;

  while (tmp)
    {
      if (tmp->data != data)
	tmp = tmp->next;
      else
	{
	  LWGList *next = tmp->next;

	  if (tmp->prev)
	    tmp->prev->next = next;
	  else
	    list = next;
	  if (next)
	    next->prev = tmp->prev;

	  _lwg_list_free1 (tmp);
	  tmp = next;
	}
    }
  return list;
}

static /*inline*/ LWGList*
_lwg_list_remove_link (LWGList *list,
		     LWGList *link)
{
  if (link)
    {
      if (link->prev)
	link->prev->next = link->next;
      if (link->next)
	link->next->prev = link->prev;
      
      if (link == list)
	list = list->next;
      
      link->next = NULL;
      link->prev = NULL;
    }
  
  return list;
}

LWGList*
lwg_list_remove_link (LWGList *list,
		    LWGList *link)
{
  return _lwg_list_remove_link (list, link);
}

LWGList*
lwg_list_delete_link (LWGList *list,
		    LWGList *link)
{
  list = _lwg_list_remove_link (list, link);
  _lwg_list_free1 (link);

  return list;
}

LWGList*
lwg_list_copy (LWGList *list)
{
  LWGList *new_list = NULL;

  if (list)
    {
      LWGList *last;

      new_list = _lwg_list_alloc ();
      new_list->data = list->data;
      new_list->prev = NULL;
      last = new_list;
      list = list->next;
      while (list)
	{
	  last->next = _lwg_list_alloc ();
	  last->next->prev = last;
	  last = last->next;
	  last->data = list->data;
	  list = list->next;
	}
      last->next = NULL;
    }

  return new_list;
}

LWGList*
lwg_list_reverse (LWGList *list)
{
  LWGList *last;
  
  last = NULL;
  while (list)
    {
      last = list;
      list = last->next;
      last->next = last->prev;
      last->prev = list;
    }
  
  return last;
}

LWGList*
lwg_list_nth (LWGList *list,
	    lwguint  n)
{
  while ((n-- > 0) && list)
    list = list->next;
  
  return list;
}

LWGList*
lwg_list_nth_prev (LWGList *list,
		 lwguint  n)
{
  while ((n-- > 0) && list)
    list = list->prev;
  
  return list;
}

lwgpointer
lwg_list_nth_data (LWGList     *list,
		 lwguint      n)
{
  while ((n-- > 0) && list)
    list = list->next;
  
  return list ? list->data : NULL;
}

LWGList*
lwg_list_find (LWGList         *list,
	     lwgconstpointer  data)
{
  while (list)
    {
      if (list->data == data)
	break;
      list = list->next;
    }
  
  return list;
}

LWGList*
lwg_list_find_custom (LWGList         *list,
		    lwgconstpointer  data,
		    LWGCompareFunc   func)
{
  lwg_return_val_if_fail (func != NULL, list);

  while (list)
    {
      if (! func (list->data, data))
	return list;
      list = list->next;
    }

  return NULL;
}


lwgint
lwg_list_position (LWGList *list,
		 LWGList *link)
{
  lwgint i;

  i = 0;
  while (list)
    {
      if (list == link)
	return i;
      i++;
      list = list->next;
    }

  return -1;
}

lwgint
lwg_list_index (LWGList         *list,
	      lwgconstpointer  data)
{
  lwgint i;

  i = 0;
  while (list)
    {
      if (list->data == data)
	return i;
      i++;
      list = list->next;
    }

  return -1;
}

LWGList*
lwg_list_last (LWGList *list)
{
  if (list)
    {
      while (list->next)
	list = list->next;
    }
  
  return list;
}

LWGList*
lwg_list_first (LWGList *list)
{
  if (list)
    {
      while (list->prev)
	list = list->prev;
    }
  
  return list;
}

lwguint
lwg_list_length (LWGList *list)
{
  lwguint length;
  
  length = 0;
  while (list)
    {
      length++;
      list = list->next;
    }
  
  return length;
}

void
lwg_list_foreach (LWGList	 *list,
		LWGFunc	  func,
		lwgpointer  user_data)
{
  while (list)
    {
      LWGList *next = list->next;
      (*func) (list->data, user_data);
      list = next;
    }
}

static LWGList*
lwg_list_insert_sorted_real (LWGList    *list,
			   lwgpointer  data,
			   LWGFunc     func,
			   lwgpointer  user_data)
{
  LWGList *tmp_list = list;
  LWGList *new_list;
  lwgint cmp;

  lwg_return_val_if_fail (func != NULL, list);
  
  if (!list) 
    {
      new_list = _lwg_list_alloc0 ();
      new_list->data = data;
      return new_list;
    }
  
  cmp = ((LWGCompareDataFunc) func) (data, tmp_list->data, user_data);

  while ((tmp_list->next) && (cmp > 0))
    {
      tmp_list = tmp_list->next;

      cmp = ((LWGCompareDataFunc) func) (data, tmp_list->data, user_data);
    }

  new_list = _lwg_list_alloc0 ();
  new_list->data = data;

  if ((!tmp_list->next) && (cmp > 0))
    {
      tmp_list->next = new_list;
      new_list->prev = tmp_list;
      return list;
    }
   
  if (tmp_list->prev)
    {
      tmp_list->prev->next = new_list;
      new_list->prev = tmp_list->prev;
    }
  new_list->next = tmp_list;
  tmp_list->prev = new_list;
 
  if (tmp_list == list)
    return new_list;
  else
    return list;
}

LWGList*
lwg_list_insert_sorted (LWGList        *list,
		      lwgpointer      data,
		      LWGCompareFunc  func)
{
  return lwg_list_insert_sorted_real (list, data, (LWGFunc) func, NULL);
}

LWGList*
lwg_list_insert_sorted_with_data (LWGList            *list,
				lwgpointer          data,
				LWGCompareDataFunc  func,
				lwgpointer          user_data)
{
  return lwg_list_insert_sorted_real (list, data, (LWGFunc) func, user_data);
}

static LWGList *
lwg_list_sort_merge (LWGList     *l1, 
		   LWGList     *l2,
		   LWGFunc     compare_func,
		   lwgpointer  user_data)
{
  LWGList list, *l, *lprev;
  lwgint cmp;

  l = &list; 
  lprev = NULL;

  while (l1 && l2)
    {
      cmp = ((LWGCompareDataFunc) compare_func) (l1->data, l2->data, user_data);

      if (cmp <= 0)
        {
	  l->next = l1;
	  l1 = l1->next;
        } 
      else 
	{
	  l->next = l2;
	  l2 = l2->next;
        }
      l = l->next;
      l->prev = lprev; 
      lprev = l;
    }
  l->next = l1 ? l1 : l2;
  l->next->prev = l;

  return list.next;
}

static LWGList* 
lwg_list_sort_real (LWGList    *list,
		  LWGFunc     compare_func,
		  lwgpointer  user_data)
{
  LWGList *l1, *l2;
  
  if (!list) 
    return NULL;
  if (!list->next) 
    return list;
  
  l1 = list; 
  l2 = list->next;

  while ((l2 = l2->next) != NULL)
    {
      if ((l2 = l2->next) == NULL) 
	break;
      l1 = l1->next;
    }
  l2 = l1->next; 
  l1->next = NULL; 

  return lwg_list_sort_merge (lwg_list_sort_real (list, compare_func, user_data),
			    lwg_list_sort_real (l2, compare_func, user_data),
			    compare_func,
			    user_data);
}

LWGList *
lwg_list_sort (LWGList        *list,
	     LWGCompareFunc  compare_func)
{
  return lwg_list_sort_real (list, (LWGFunc) compare_func, NULL);
			    
}

LWGList *
lwg_list_sort_with_data (LWGList            *list,
		       LWGCompareDataFunc  compare_func,
		       lwgpointer          user_data)
{
  return lwg_list_sort_real (list, (LWGFunc) compare_func, user_data);
}

/*
#define __LWG_LIST_C__
#include "galiasdef.c"
*/
