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

#ifndef __LWG_LIST_H__
#define __LWG_LIST_H__

#include "lwgmissing.h"

LWG_BEGIN_DECLS

typedef struct _LWGList LWGList;

struct _LWGList
{
  lwgpointer data;
  LWGList *next;
  LWGList *prev;
};

/* Doubly linked lists
 */
LWGList*   lwg_list_alloc                   (void) LWG_GNUC_WARN_UNUSED_RESULT;
void     lwg_list_free                    (LWGList            *list);
void     lwg_list_free_1                  (LWGList            *list);
#define  lwg_list_free1                   lwg_list_free_1
LWGList*   lwg_list_append                  (LWGList            *list,
					 lwgpointer          data) LWG_GNUC_WARN_UNUSED_RESULT;
LWGList*   lwg_list_prepend                 (LWGList            *list,
					 lwgpointer          data) LWG_GNUC_WARN_UNUSED_RESULT;
LWGList*   lwg_list_insert                  (LWGList            *list,
					 lwgpointer          data,
					 lwgint              position) LWG_GNUC_WARN_UNUSED_RESULT;
LWGList*   lwg_list_insert_sorted           (LWGList            *list,
					 lwgpointer          data,
					 LWGCompareFunc      func) LWG_GNUC_WARN_UNUSED_RESULT;
LWGList*   lwg_list_insert_sorted_with_data (LWGList            *list,
					 lwgpointer          data,
					 LWGCompareDataFunc  func,
					 lwgpointer          user_data) LWG_GNUC_WARN_UNUSED_RESULT;
LWGList*   lwg_list_insert_before           (LWGList            *list,
					 LWGList            *sibling,
					 lwgpointer          data) LWG_GNUC_WARN_UNUSED_RESULT;
LWGList*   lwg_list_concat                  (LWGList            *list1,
					 LWGList            *list2) LWG_GNUC_WARN_UNUSED_RESULT;
LWGList*   lwg_list_remove                  (LWGList            *list,
					 lwgconstpointer     data) LWG_GNUC_WARN_UNUSED_RESULT;
LWGList*   lwg_list_remove_all              (LWGList            *list,
					 lwgconstpointer     data) LWG_GNUC_WARN_UNUSED_RESULT;
LWGList*   lwg_list_remove_link             (LWGList            *list,
					 LWGList            *llink) LWG_GNUC_WARN_UNUSED_RESULT;
LWGList*   lwg_list_delete_link             (LWGList            *list,
					 LWGList            *link_) LWG_GNUC_WARN_UNUSED_RESULT;
LWGList*   lwg_list_reverse                 (LWGList            *list) LWG_GNUC_WARN_UNUSED_RESULT;
LWGList*   lwg_list_copy                    (LWGList            *list) LWG_GNUC_WARN_UNUSED_RESULT;
LWGList*   lwg_list_nth                     (LWGList            *list,
					 lwguint             n);
LWGList*   lwg_list_nth_prev                (LWGList            *list,
					 lwguint             n);
LWGList*   lwg_list_find                    (LWGList            *list,
					 lwgconstpointer     data);
LWGList*   lwg_list_find_custom             (LWGList            *list,
					 lwgconstpointer     data,
					 LWGCompareFunc      func);
lwgint     lwg_list_position                (LWGList            *list,
					 LWGList            *llink);
lwgint     lwg_list_index                   (LWGList            *list,
					 lwgconstpointer     data);
LWGList*   lwg_list_last                    (LWGList            *list);
LWGList*   lwg_list_first                   (LWGList            *list);
lwguint    lwg_list_length                  (LWGList            *list);
void     lwg_list_foreach                 (LWGList            *list,
					 LWGFunc             func,
					 lwgpointer          user_data);
LWGList*   lwg_list_sort                    (LWGList            *list,
					 LWGCompareFunc      compare_func) LWG_GNUC_WARN_UNUSED_RESULT;
LWGList*   lwg_list_sort_with_data          (LWGList            *list,
					 LWGCompareDataFunc  compare_func,
					 lwgpointer          user_data)  LWG_GNUC_WARN_UNUSED_RESULT;
lwgpointer lwg_list_nth_data                (LWGList            *list,
					 lwguint             n);


#define lwg_list_previous(list)	        ((list) ? (((LWGList *)(list))->prev) : NULL)
#define lwg_list_next(list)	        ((list) ? (((LWGList *)(list))->next) : NULL)

#ifndef LWG_DISABLE_DEPRECATED
void     lwg_list_push_allocator          (lwgpointer          allocator);
void     lwg_list_pop_allocator           (void);
#endif
LWG_END_DECLS

#endif /* __LWG_LIST_H__ */

