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
 * This file was pieced together from various parts of glib to 
 * provide misc. glib code necessary to use ghash and glist
 */

#ifndef __GPA_MISSINLWG_H__
#define __GPA_MISSINLWG_H__

#include <stdlib.h>

#define DISABLE_VISIBILITY
#define GPA_BEGIN_DECLS
#define GPA_END_DECLS
#define GPA_GNUC_CONST
#define GPA_GNUC_WARN_UNUSED_RESULT

#ifndef NULL
#define NULL (0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define GPA_STMT_START do
#define GPA_STMT_END while(0)

#define gpa_slice_new(type) malloc(sizeof(type))
#define gpa_slice_new0(type) calloc(1, sizeof(type))
#define gpa_slice_free(type, ptr) free(ptr)
#define gpa_slice_free_chain(type, _ptr, next_field) \
    GPA_STMT_START { \
        type *ptr = (_ptr), *next_ptr = NULL; \
        for(; ptr; ptr = next_ptr) \
        { \
            next_ptr = ptr->next_field; \
            free(ptr); \
        } \
    } GPA_STMT_END
                
#define gpa_new0(type, count) calloc(count, sizeof(type))
#define gpa_free(ptr) free(ptr)
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define gpa_return_if_fail(expr) \
    GPA_STMT_START { \
        if (expr) {} else { return; } \
    } GPA_STMT_END
#define gpa_return_val_if_fail(expr, val) \
    GPA_STMT_START { \
        if (expr) {} else { return (val); } \
    } GPA_STMT_END

/* -- WARNING WARNING WARNING --
   These stubs make ghash/glist NOT THREADSAFE
*/

#define gpa_atomic_int_exchange_and_add(atomic, val) (*(atomic) += val, *(atomic) - val)
#define gpa_atomic_int_add(atomic, val) (*(atomic) += val)

typedef void* gpapointer;
typedef void const* gpaconstpointer;
typedef char gpachar;
typedef int gpaint;
typedef unsigned int gpauint;
typedef gpaint gpaboolean;
typedef gpauint (*GPAHashFunc) (gpaconstpointer);
typedef gpaboolean (*GPAEqualFunc) (gpaconstpointer, gpaconstpointer);
typedef void (*GPADestroyNotify) (gpapointer);
typedef void (*GPAFreeFunc) (gpapointer);
typedef void (*GPAHFunc) (gpapointer key, gpapointer value, gpapointer user_data);
typedef gpaint (*GPACompareFunc) (gpaconstpointer, gpaconstpointer);
typedef gpaint (*GPACompareDataFunc) (gpaconstpointer, gpaconstpointer, gpapointer user_data);
typedef void (*GPAFunc) (gpapointer data, gpapointer user_data);

gpauint gpa_str_hash (gpaconstpointer v);
gpaboolean gpa_str_equal (gpaconstpointer v1, gpaconstpointer v2);


#endif
