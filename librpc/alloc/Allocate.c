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

#include <lwrpc/allocate.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct Link
{
    struct Link *prev, *next;
} Link;

typedef struct MemHeader
{
    Link sibling;
    Link child;
    void (*destruct) (void* data);
} MemHeader;

#define OFFSET(type, field) ((char*) &((type*)0)->field - (char*) 0)
#define LINK_STRUCT(link, type, field) ((void*) ((char*) link - OFFSET(type, field)))

static void
LinkInit(Link* link)
{
    link->prev = link->next = link;
}

static void
LinkAppend(Link* list, Link* link)
{
    list->prev->next = link;
    link->prev = list->prev;
    list->prev = link;
    link->next = list;
}

static void
LinkRemove(Link* link)
{
    Link* prev = link->prev;
    Link* next = link->next;
   
    link->prev->next = next;
    link->next->prev = prev;

    LinkInit(link);
}

#define ALIGNMENT 8
#define ALIGNMENT_GAP ((ALIGNMENT - sizeof(MemHeader) % ALIGNMENT) % ALIGNMENT)
#define MEM_HEADER(obj) ((MemHeader*) ((char*) obj - ALIGNMENT_GAP - sizeof(MemHeader)))
#define MEM_CONTENT(hdr) ((void*) ((char*) hdr + ALIGNMENT_GAP + sizeof(MemHeader)))

void*
talloc (void* ctx, size_t size, void (*destruct)(void*))
{
    size_t gapsize = ALIGNMENT_GAP;
    size_t addedsize = size + sizeof(MemHeader) + gapsize;
    
    MemHeader* context = ctx ? MEM_HEADER(ctx) : NULL;
    MemHeader* obj = malloc(addedsize);

    if (!obj)
    {
        return NULL;
    }

    LinkInit(&obj->child);
    LinkInit(&obj->sibling);
    obj->destruct = destruct;

    if (!obj->destruct)
        obj->destruct = tfree_children;

    if (context)
    {
        LinkAppend(&context->child, &obj->sibling);      
    }

    return MEM_CONTENT(obj);
}

void*
trealloc (void *obj, size_t newsize)
{
    size_t gapsize = ALIGNMENT_GAP;
    size_t addedsize = newsize + sizeof(MemHeader) + gapsize;

    MemHeader* hdr = MEM_HEADER(obj);

    hdr = realloc(hdr, addedsize);
    if (!hdr)
    {
        return NULL;
    }

    return MEM_CONTENT(hdr);
}

void
tfree (void* obj)
{
    MemHeader* hdr = MEM_HEADER(obj);

    LinkRemove(&hdr->sibling);

    if (hdr->destruct)
        hdr->destruct(obj);

    memset(hdr, 0, sizeof(*hdr));

    free(hdr);
}

void
tfree_children(void* obj)
{
    MemHeader* hdr = MEM_HEADER(obj);
    
    while (hdr->child.next != &hdr->child)
        tfree(MEM_CONTENT(LINK_STRUCT(hdr->child.next, MemHeader, sibling)));
}

void
tunlink_children(void* obj)
{
    MemHeader* hdr = MEM_HEADER(obj);
    Link* child, *next;

    for (child = hdr->child.next; child != &hdr->child; child = next)
    {
        next = child->next;
        LinkInit(child);
    }

    LinkInit(&hdr->child);
}

void *
tdup (void* ctx, void *src, size_t size, void (*destruct)(void*))
{
    void* result = talloc(ctx, size, destruct);

    if (!result)
    {
        return NULL;
    }

    memcpy(result, src, size);

    return result;
}

void
tlink(void* parent, void* child)
{
    MemHeader* phdr = MEM_HEADER(parent);
    MemHeader* chdr = MEM_HEADER(child);

    LinkRemove(&chdr->sibling);
    LinkAppend(&phdr->child, &chdr->sibling);
}

void
tunlink(void* child)
{
    MemHeader* chdr = MEM_HEADER(child);

    LinkRemove(&chdr->child);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
