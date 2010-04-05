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

#ifndef __ALLOCATE_H__
#define __ALLOCATE_H__

#include <stdlib.h>

void *talloc(
    void* ctx, /* Parent memory block (or NULL) */
    size_t size,
    void (*destruct)(void*));

void* trealloc(
    void *obj,
    size_t newsize);

void tfree(
    void* obj);

void
tfree_children(
    void* obj);

void *tdup(
    void* ctx,
    void* src,
    size_t size,
    void (*destruct)(void*));

void tlink(
    void* parent,
    void* child);

void* tattach(
    const void* parent,
    const void** ptr,
    size_t size,
    void (*destruct)(void*));

void tunlink(
    void* child);

void tunlink_children(
    void* obj);

#define TNEW(ctx, type)  ((type*) talloc(ctx, sizeof(type), NULL))
#define TNEWDUP(ctx, src, type)  ((type*) tdup(ctx, src, sizeof(type), NULL))

#define SAFE_FREE(ptr)  do { if (ptr) free(ptr); (ptr) = NULL; } while (0)

#endif
