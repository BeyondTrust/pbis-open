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

#ifndef __GMISSINLWG_H__
#define __GMISSINLWG_H__

#include <stdlib.h>

#define DISABLE_VISIBILITY
#define LWG_BEGIN_DECLS
#define LWG_END_DECLS
#define LWG_GNUC_CONST
#define LWG_GNUC_WARN_UNUSED_RESULT

#ifndef NULL
#define NULL (0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define LWG_STMT_START do
#define LWG_STMT_END while(0)

#define lwg_slice_new(type) malloc(sizeof(type))
#define lwg_slice_new0(type) calloc(1, sizeof(type))
#define lwg_slice_free(type, ptr) free(ptr)
#define lwg_slice_free_chain(type, _ptr, next_field) \
    LWG_STMT_START { \
        type *ptr = (_ptr), *next_ptr = NULL; \
        for(; ptr; ptr = next_ptr) \
        { \
            next_ptr = ptr->next_field; \
            free(ptr); \
        } \
    } LWG_STMT_END
                
#define lwg_new0(type, count) calloc(count, sizeof(type))
#define lwg_free(ptr) free(ptr)
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define lwg_return_if_fail(expr) \
    LWG_STMT_START { \
        if (expr) {} else { return; } \
    } LWG_STMT_END
#define lwg_return_val_if_fail(expr, val) \
    LWG_STMT_START { \
        if (expr) {} else { return (val); } \
    } LWG_STMT_END

/* -- WARNING WARNING WARNING --
   These stubs make ghash/glist NOT THREADSAFE
*/

#define lwg_atomic_int_exchange_and_add(atomic, val) (*(atomic) += val, *(atomic) - val)
#define lwg_atomic_int_add(atomic, val) (*(atomic) += val)

typedef void* lwgpointer;
typedef void const* lwgconstpointer;
typedef char lwgchar;
typedef int lwgint;
typedef unsigned int lwguint;
typedef lwgint lwgboolean;
typedef lwguint (*LWGHashFunc) (lwgconstpointer);
typedef lwgboolean (*LWGEqualFunc) (lwgconstpointer, lwgconstpointer);
typedef void (*LWGDestroyNotify) (lwgpointer);
typedef void (*LWGFreeFunc) (lwgpointer);
typedef void (*LWGHFunc) (lwgpointer key, lwgpointer value, lwgpointer user_data);
typedef lwgint (*LWGCompareFunc) (lwgconstpointer, lwgconstpointer);
typedef lwgint (*LWGCompareDataFunc) (lwgconstpointer, lwgconstpointer, lwgpointer user_data);
typedef void (*LWGFunc) (lwgpointer data, lwgpointer user_data);

lwguint lwg_str_hash (lwgconstpointer v);
lwgboolean lwg_str_equal (lwgconstpointer v1, lwgconstpointer v2);


#endif
