/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

/*
 * Modified by Likewise Software Corporation 2007.
 */

#ifndef __GPA_LIST_H__
#define __GPA_LIST_H__

#include "includes.h"

typedef struct _GPAList GPAList;

struct _GPAList
{
  gpapointer data;
  GPAList *next;
  GPAList *prev;
};

/* Doubly linked lists
 */
GPAList*   gpa_list_alloc                   (void) GPA_GNUC_WARN_UNUSED_RESULT;
void     gpa_list_free                    (GPAList            *list);
void     gpa_list_free_1                  (GPAList            *list);
#define  gpa_list_free1                   gpa_list_free_1
GPAList*   gpa_list_append                  (GPAList            *list,
					 gpapointer          data) GPA_GNUC_WARN_UNUSED_RESULT;
GPAList*   gpa_list_prepend                 (GPAList            *list,
					 gpapointer          data) GPA_GNUC_WARN_UNUSED_RESULT;
GPAList*   gpa_list_insert                  (GPAList            *list,
					 gpapointer          data,
					 gpaint              position) GPA_GNUC_WARN_UNUSED_RESULT;
GPAList*   gpa_list_insert_sorted           (GPAList            *list,
					 gpapointer          data,
					 GPACompareFunc      func) GPA_GNUC_WARN_UNUSED_RESULT;
GPAList*   gpa_list_insert_sorted_with_data (GPAList            *list,
					 gpapointer          data,
					 GPACompareDataFunc  func,
					 gpapointer          user_data) GPA_GNUC_WARN_UNUSED_RESULT;
GPAList*   gpa_list_insert_before           (GPAList            *list,
					 GPAList            *sibling,
					 gpapointer          data) GPA_GNUC_WARN_UNUSED_RESULT;
GPAList*   gpa_list_concat                  (GPAList            *list1,
					 GPAList            *list2) GPA_GNUC_WARN_UNUSED_RESULT;
GPAList*   gpa_list_remove                  (GPAList            *list,
					 gpaconstpointer     data) GPA_GNUC_WARN_UNUSED_RESULT;
GPAList*   gpa_list_remove_all              (GPAList            *list,
					 gpaconstpointer     data) GPA_GNUC_WARN_UNUSED_RESULT;
GPAList*   gpa_list_remove_link             (GPAList            *list,
					 GPAList            *llink) GPA_GNUC_WARN_UNUSED_RESULT;
GPAList*   gpa_list_delete_link             (GPAList            *list,
					 GPAList            *link_) GPA_GNUC_WARN_UNUSED_RESULT;
GPAList*   gpa_list_reverse                 (GPAList            *list) GPA_GNUC_WARN_UNUSED_RESULT;
GPAList*   gpa_list_copy                    (GPAList            *list) GPA_GNUC_WARN_UNUSED_RESULT;
GPAList*   gpa_list_nth                     (GPAList            *list,
					 gpauint             n);
GPAList*   gpa_list_nth_prev                (GPAList            *list,
					 gpauint             n);
GPAList*   gpa_list_find                    (GPAList            *list,
					 gpaconstpointer     data);
GPAList*   gpa_list_find_custom             (GPAList            *list,
					 gpaconstpointer     data,
					 GPACompareFunc      func);
gpaint     gpa_list_position                (GPAList            *list,
					 GPAList            *llink);
gpaint     gpa_list_index                   (GPAList            *list,
					 gpaconstpointer     data);
GPAList*   gpa_list_last                    (GPAList            *list);
GPAList*   gpa_list_first                   (GPAList            *list);
gpauint    gpa_list_length                  (GPAList            *list);
void     gpa_list_foreach                 (GPAList            *list,
					 GPAFunc             func,
					 gpapointer          user_data);
GPAList*   gpa_list_sort                    (GPAList            *list,
					 GPACompareFunc      compare_func) GPA_GNUC_WARN_UNUSED_RESULT;
GPAList*   gpa_list_sort_with_data          (GPAList            *list,
					 GPACompareDataFunc  compare_func,
					 gpapointer          user_data)  GPA_GNUC_WARN_UNUSED_RESULT;
gpapointer gpa_list_nth_data                (GPAList            *list,
					 gpauint             n);


#define gpa_list_previous(list)	        ((list) ? (((GPAList *)(list))->prev) : NULL)
#define gpa_list_next(list)	        ((list) ? (((GPAList *)(list))->next) : NULL)

#ifndef GPA_DISABLE_DEPRECATED
void     gpa_list_push_allocator          (gpapointer          allocator);
void     gpa_list_pop_allocator           (void);
#endif

#endif /* __GPA_LIST_H__ */

