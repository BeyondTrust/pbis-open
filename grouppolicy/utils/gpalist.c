/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Modified by Likewise Software Corporation 2007.
 */

/* 
 * MT safe
 */

#include "includes.h"

void gpa_list_push_allocator (gpapointer dummy) { /* present for binary compat only */ }
void gpa_list_pop_allocator  (void)           { /* present for binary compat only */ }

#define _gpa_list_alloc()         gpa_slice_new (GPAList)
#define _gpa_list_alloc0()        gpa_slice_new0 (GPAList)
#define _gpa_list_free1(list)     gpa_slice_free (GPAList, list)

GPAList*
gpa_list_alloc (void)
{
  return _gpa_list_alloc0 ();
}

void
gpa_list_free (GPAList *list)
{
  gpa_slice_free_chain (GPAList, list, next);
}

void
gpa_list_free_1 (GPAList *list)
{
  _gpa_list_free1 (list);
}

GPAList*
gpa_list_append (GPAList	*list,
	       gpapointer	 data)
{
  GPAList *new_list;
  GPAList *last;
  
  new_list = _gpa_list_alloc ();
  new_list->data = data;
  new_list->next = NULL;
  
  if (list)
    {
      last = gpa_list_last (list);
      /* gpa_assert (last != NULL); */
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

GPAList*
gpa_list_prepend (GPAList	 *list,
		gpapointer  data)
{
  GPAList *new_list;
  
  new_list = _gpa_list_alloc ();
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

GPAList*
gpa_list_insert (GPAList	*list,
	       gpapointer	 data,
	       gpaint	 position)
{
  GPAList *new_list;
  GPAList *tmp_list;
  
  if (position < 0)
    return gpa_list_append (list, data);
  else if (position == 0)
    return gpa_list_prepend (list, data);
  
  tmp_list = gpa_list_nth (list, position);
  if (!tmp_list)
    return gpa_list_append (list, data);
  
  new_list = _gpa_list_alloc ();
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

GPAList*
gpa_list_insert_before (GPAList   *list,
		      GPAList   *sibling,
		      gpapointer data)
{
  if (!list)
    {
      list = gpa_list_alloc ();
      list->data = data;
      gpa_return_val_if_fail (sibling == NULL, list);
      return list;
    }
  else if (sibling)
    {
      GPAList *node;

      node = _gpa_list_alloc ();
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
	  gpa_return_val_if_fail (sibling == list, node);
	  return node;
	}
    }
  else
    {
      GPAList *last;

      last = list;
      while (last->next)
	last = last->next;

      last->next = _gpa_list_alloc ();
      last->next->data = data;
      last->next->prev = last;
      last->next->next = NULL;

      return list;
    }
}

GPAList *
gpa_list_concat (GPAList *list1, GPAList *list2)
{
  GPAList *tmp_list;
  
  if (list2)
    {
      tmp_list = gpa_list_last (list1);
      if (tmp_list)
	tmp_list->next = list2;
      else
	list1 = list2;
      list2->prev = tmp_list;
    }
  
  return list1;
}

GPAList*
gpa_list_remove (GPAList	     *list,
	       gpaconstpointer  data)
{
  GPAList *tmp;
  
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
	  
	  _gpa_list_free1 (tmp);
	  
	  break;
	}
    }
  return list;
}

GPAList*
gpa_list_remove_all (GPAList	*list,
		   gpaconstpointer data)
{
  GPAList *tmp = list;

  while (tmp)
    {
      if (tmp->data != data)
	tmp = tmp->next;
      else
	{
	  GPAList *next = tmp->next;

	  if (tmp->prev)
	    tmp->prev->next = next;
	  else
	    list = next;
	  if (next)
	    next->prev = tmp->prev;

	  _gpa_list_free1 (tmp);
	  tmp = next;
	}
    }
  return list;
}

static /*inline*/ GPAList*
_gpa_list_remove_link (GPAList *list,
		     GPAList *link)
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

GPAList*
gpa_list_remove_link (GPAList *list,
		    GPAList *link)
{
  return _gpa_list_remove_link (list, link);
}

GPAList*
gpa_list_delete_link (GPAList *list,
		    GPAList *link)
{
  list = _gpa_list_remove_link (list, link);
  _gpa_list_free1 (link);

  return list;
}

GPAList*
gpa_list_copy (GPAList *list)
{
  GPAList *new_list = NULL;

  if (list)
    {
      GPAList *last;

      new_list = _gpa_list_alloc ();
      new_list->data = list->data;
      new_list->prev = NULL;
      last = new_list;
      list = list->next;
      while (list)
	{
	  last->next = _gpa_list_alloc ();
	  last->next->prev = last;
	  last = last->next;
	  last->data = list->data;
	  list = list->next;
	}
      last->next = NULL;
    }

  return new_list;
}

GPAList*
gpa_list_reverse (GPAList *list)
{
  GPAList *last;
  
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

GPAList*
gpa_list_nth (GPAList *list,
	    gpauint  n)
{
  while ((n-- > 0) && list)
    list = list->next;
  
  return list;
}

GPAList*
gpa_list_nth_prev (GPAList *list,
		 gpauint  n)
{
  while ((n-- > 0) && list)
    list = list->prev;
  
  return list;
}

gpapointer
gpa_list_nth_data (GPAList     *list,
		 gpauint      n)
{
  while ((n-- > 0) && list)
    list = list->next;
  
  return list ? list->data : NULL;
}

GPAList*
gpa_list_find (GPAList         *list,
	     gpaconstpointer  data)
{
  while (list)
    {
      if (list->data == data)
	break;
      list = list->next;
    }
  
  return list;
}

GPAList*
gpa_list_find_custom (GPAList         *list,
		    gpaconstpointer  data,
		    GPACompareFunc   func)
{
  gpa_return_val_if_fail (func != NULL, list);

  while (list)
    {
      if (! func (list->data, data))
	return list;
      list = list->next;
    }

  return NULL;
}


gpaint
gpa_list_position (GPAList *list,
		 GPAList *link)
{
  gpaint i;

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

gpaint
gpa_list_index (GPAList         *list,
	      gpaconstpointer  data)
{
  gpaint i;

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

GPAList*
gpa_list_last (GPAList *list)
{
  if (list)
    {
      while (list->next)
	list = list->next;
    }
  
  return list;
}

GPAList*
gpa_list_first (GPAList *list)
{
  if (list)
    {
      while (list->prev)
	list = list->prev;
    }
  
  return list;
}

gpauint
gpa_list_length (GPAList *list)
{
  gpauint length;
  
  length = 0;
  while (list)
    {
      length++;
      list = list->next;
    }
  
  return length;
}

void
gpa_list_foreach (GPAList	 *list,
		GPAFunc	  func,
		gpapointer  user_data)
{
  while (list)
    {
      GPAList *next = list->next;
      (*func) (list->data, user_data);
      list = next;
    }
}

static GPAList*
gpa_list_insert_sorted_real (GPAList    *list,
			   gpapointer  data,
			   GPAFunc     func,
			   gpapointer  user_data)
{
  GPAList *tmp_list = list;
  GPAList *new_list;
  gpaint cmp;

  gpa_return_val_if_fail (func != NULL, list);
  
  if (!list) 
    {
      new_list = _gpa_list_alloc0 ();
      new_list->data = data;
      return new_list;
    }
  
  cmp = ((GPACompareDataFunc) func) (data, tmp_list->data, user_data);

  while ((tmp_list->next) && (cmp > 0))
    {
      tmp_list = tmp_list->next;

      cmp = ((GPACompareDataFunc) func) (data, tmp_list->data, user_data);
    }

  new_list = _gpa_list_alloc0 ();
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

GPAList*
gpa_list_insert_sorted (GPAList        *list,
		      gpapointer      data,
		      GPACompareFunc  func)
{
  return gpa_list_insert_sorted_real (list, data, (GPAFunc) func, NULL);
}

GPAList*
gpa_list_insert_sorted_with_data (GPAList            *list,
				gpapointer          data,
				GPACompareDataFunc  func,
				gpapointer          user_data)
{
  return gpa_list_insert_sorted_real (list, data, (GPAFunc) func, user_data);
}

static GPAList *
gpa_list_sort_merge (GPAList     *l1, 
		   GPAList     *l2,
		   GPAFunc     compare_func,
		   gpapointer  user_data)
{
  GPAList list, *l, *lprev;
  gpaint cmp;

  l = &list; 
  lprev = NULL;

  while (l1 && l2)
    {
      cmp = ((GPACompareDataFunc) compare_func) (l1->data, l2->data, user_data);

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

static GPAList* 
gpa_list_sort_real (GPAList    *list,
		  GPAFunc     compare_func,
		  gpapointer  user_data)
{
  GPAList *l1, *l2;
  
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

  return gpa_list_sort_merge (gpa_list_sort_real (list, compare_func, user_data),
			    gpa_list_sort_real (l2, compare_func, user_data),
			    compare_func,
			    user_data);
}

GPAList *
gpa_list_sort (GPAList        *list,
	     GPACompareFunc  compare_func)
{
  return gpa_list_sort_real (list, (GPAFunc) compare_func, NULL);
			    
}

GPAList *
gpa_list_sort_with_data (GPAList            *list,
		       GPACompareDataFunc  compare_func,
		       gpapointer          user_data)
{
  return gpa_list_sort_real (list, (GPAFunc) compare_func, user_data);
}

/*
#define __GPA_LIST_C__
#include "galiasdef.c"
*/
