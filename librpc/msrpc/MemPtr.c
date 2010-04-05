/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <lwrpc/types.h>
#include <lw/ntstatus.h>
#include <lwrpc/memptr.h>
#include <lwrpc/allocate.h>

#include "macros.h"

/*
 * Status check macros
 */

#define goto_if_invalid_param_ntstatus(p, lbl) \
    if ((p) == NULL) {                       \
        status = STATUS_INVALID_PARAMETER;   \
        goto lbl;                            \
    }

#define goto_if_no_memory_ntstatus(p, lbl)   \
    if ((p) == NULL) {                       \
        status = STATUS_NO_MEMORY;           \
        goto lbl;                            \
    }

#define goto_if_ntstatus_not_success(s, lbl) \
    if ((s) != STATUS_SUCCESS) {             \
        status = s;                          \
        goto lbl;                            \
    }


static NTSTATUS MemPtrNodeAppend(PtrList *list, PtrNode *node)
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;    

    goto_if_invalid_param_ntstatus(list, error);
    goto_if_invalid_param_ntstatus(node, error);

    LIBRPC_LOCK_MUTEX(bLocked, &list->mutex);

    node->next = node->prev = NULL;
    
    if (list->head == NULL) {
        /* This is the very first node */
        list->head = node;
        list->tail = node;
    } else {
        /* Append the new node */
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;        
    }
    list->count++;    

done:
    LIBRPC_UNLOCK_MUTEX(bLocked, &list->mutex);
    return status;

error:
    goto done;
}


static NTSTATUS MemPtrNodeRemove(PtrList *list, PtrNode *node)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(list, error);
    goto_if_invalid_param_ntstatus(node, error);

    /* We're assuming the list has already been locked */

    if ((node == list->head) && (node == list->tail))
    {
        list->head = list->tail = NULL;
    }
    else if (node == list->head)
    {
        list->head = node->next;
        list->head->prev = NULL;        
    }
    else if (node == list->tail)
    {
        list->tail = node->prev;
        list->tail->next = NULL;        
    }
    else
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    list->count--;    
    

done:
    return status;

error:
    goto done;
}


NTSTATUS MemPtrListInit(PtrList **out)
{
    NTSTATUS status = STATUS_SUCCESS;
    PtrList *list = NULL;

    goto_if_invalid_param_ntstatus(out, error);

    list = (PtrList*) malloc(sizeof(PtrList));
    goto_if_no_memory_ntstatus(list, error);

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;    

    /* According to pthread_mutex_init(3) documentation
       the function call always succeeds */
    pthread_mutex_init(&list->mutex, NULL);

    *out = list;

done:
    return status;

error:
    SAFE_FREE(list);
    *out = NULL;
    goto done;
}


NTSTATUS MemPtrListDestroy(PtrList **out)
{
    NTSTATUS status = STATUS_SUCCESS;
    PtrList *list = NULL;
    PtrNode *node = NULL;
    int ret = 0;

    goto_if_invalid_param_ntstatus(out, done);

    list = *out;

    node = list->head;
    while (node) {
        PtrNode *rmnode = NULL;

        SAFE_FREE(node->ptr);

        rmnode = node;
        node = node->next;

        SAFE_FREE(rmnode);
    }

    ret = pthread_mutex_destroy(&list->mutex);
    if (ret) status = STATUS_UNSUCCESSFUL;

    SAFE_FREE(list);
    *out = NULL;

done:
    return status;
}


NTSTATUS MemPtrAllocate(PtrList *list, void **out, size_t size, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;
    PtrNode *node = NULL;

    goto_if_invalid_param_ntstatus(out, error);

    /* Allocate new node */
    node = (PtrNode*) malloc(sizeof(PtrNode));
    goto_if_no_memory_ntstatus(node, done);

    node->ptr  = NULL;
    node->dep  = dep;
    node->size = size;

    if (node->size)
    {
        /* Allocate the actual memory */
        node->ptr = malloc(node->size);
        goto_if_no_memory_ntstatus(node->ptr, error);

        /* ... and initialise it to zero */
        memset(node->ptr, 0, node->size);
    }

    status = MemPtrNodeAppend(list, node);
    goto_if_ntstatus_not_success(status, error);

    *out = node->ptr;

done:
    return status;

error:

    if (node && node->ptr) {
        SAFE_FREE(node->ptr);
    }

    SAFE_FREE(node);
    *out = NULL;

    goto done;
}


NTSTATUS MemPtrFree(PtrList *list, void *ptr)
{
    NTSTATUS status = STATUS_SUCCESS;
    PtrNode *node = NULL;
    BOOLEAN bLocked = FALSE;

    goto_if_invalid_param_ntstatus(ptr, error);

    LIBRPC_LOCK_MUTEX(bLocked, &list->mutex);

    /* Free the pointer and all pointers (nodes) depending on it */
    node = list->head;
    while (node) {
        if (node->dep == ptr || node->ptr == ptr) {
            PtrNode *rmnode = NULL;
            
            /* Move to the next node before removing the current one */
            rmnode = node;
            node   = node->next;

            status = MemPtrNodeRemove(list, rmnode);
            goto_if_ntstatus_not_success(status, error);

            free(rmnode->ptr);
            free(rmnode);
            continue;
        }

        node = node->next;
    }

done:
    LIBRPC_UNLOCK_MUTEX(bLocked, &list->mutex);
    return status;

error:
    goto done;
}


NTSTATUS MemPtrAddDependant(PtrList *list, void *ptr, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;
    PtrNode *node = NULL;

    goto_if_invalid_param_ntstatus(ptr, error);

    /* Allocate new node */
    node = (PtrNode*) malloc(sizeof(PtrNode));
    goto_if_no_memory_ntstatus(node, done);

    node->ptr  = ptr;
    node->dep  = dep;
    node->size = 0;    /* size is unknown when adding dependency */

    status = MemPtrNodeAppend(list, node);
    goto_if_ntstatus_not_success(status, error);

done:
    return status;

error:
    /* Only the node should be freed here. node->ptr should
       be left intact */
    SAFE_FREE(node);
    goto done;
}




/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
