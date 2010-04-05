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

#include "includes.h"

/*
 * Locking macros
 */

#define PTR_LIST_LOCK(list)                       \
    do {                                          \
        int ret = 0;                              \
        ret = pthread_mutex_lock(&(list)->mutex); \
        if (ret) {                                \
            status = STATUS_UNSUCCESSFUL;         \
            goto error;                           \
                                                  \
        } else {                                  \
            locked = 1;                           \
        }                                         \
    } while (0);


#define PTR_LIST_UNLOCK(list)                       \
    do {                                            \
        int ret = 0;                                \
        if (!locked) break;                         \
        ret = pthread_mutex_unlock(&(list)->mutex); \
        if (ret && status == STATUS_SUCCESS) {      \
            status = STATUS_UNSUCCESSFUL;           \
                                                    \
        } else {                                    \
            locked = 0;                             \
        }                                           \
    } while (0);



static NTSTATUS MemPtrNodeAppend(PtrList *list, PtrNode *node)
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PtrNode *last = NULL;
    int locked = 0;

    BAIL_ON_INVALID_PTR(list, dwError);
    BAIL_ON_INVALID_PTR(node, dwError);

    PTR_LIST_LOCK(list);

    /* Find the last node */
    last = list->p;
    while (last && last->next) last = last->next;

    if (last == NULL) {
        /* This is the very first node */
        list->p = node;

    } else {
        /* Append the new node */
        last->next = node;
    }

    node->next = NULL;

cleanup:
    PTR_LIST_UNLOCK(list);

    if (status == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        status = LwWin32ErrorToNtStatus(dwError);
    }
    
    return status;

error:
    goto cleanup;
}


static NTSTATUS MemPtrNodeRemove(PtrList *list, PtrNode *node)
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PtrNode *prev = NULL;
    int locked = 0;

    BAIL_ON_INVALID_PTR(list, dwError);
    BAIL_ON_INVALID_PTR(node, dwError);

    PTR_LIST_LOCK(list);

    /* Simple case - this happens to be the first node */
    if (node == list->p) {
        list->p = node->next;
        goto cleanup;
    }

    /* Find node that is previous to the requested one */
    prev = list->p;
    while (prev && prev->next != node) {
        prev = prev->next;
    }

    if (prev == NULL) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    prev->next = node->next;

cleanup:
    PTR_LIST_UNLOCK(list);

    if (status == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        status = LwWin32ErrorToNtStatus(dwError);
    }
    
    return status;

error:
    goto cleanup;
}


NTSTATUS MemPtrListInit(PtrList **out)
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PtrList *list = NULL;

    BAIL_ON_INVALID_PTR(out, status);

    list = (PtrList*) malloc(sizeof(PtrList));
    BAIL_ON_NULL_PTR(list, dwError);

    list->p = NULL;

    /* According to pthread_mutex_init(3) documentation
       the function call always succeeds */
    pthread_mutex_init(&list->mutex, NULL);

    *out = list;

cleanup:
    if (status == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        status = LwWin32ErrorToNtStatus(dwError);
    }
    
    return status;

error:
    NFSSVC_SAFE_FREE(list);
    *out = NULL;
    goto cleanup;
}


NTSTATUS MemPtrListDestroy(PtrList **out)
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PtrList *list = NULL;
    PtrNode *node = NULL;
    int ret = 0;

    BAIL_ON_INVALID_PTR(out, dwError);

    list = *out;

    node = list->p;
    while (node) {
        PtrNode *rmnode = NULL;

        NFSSVC_SAFE_FREE(node->ptr);

        rmnode = node;
        node = node->next;

        NFSSVC_SAFE_FREE(rmnode);
    }

    ret = pthread_mutex_destroy(&list->mutex);
    if (ret) status = STATUS_UNSUCCESSFUL;

    NFSSVC_SAFE_FREE(list);
    *out = NULL;

cleanup:
    if (status == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        status = LwWin32ErrorToNtStatus(dwError);
    }
    
    return status;

error:
    goto cleanup;
}


NTSTATUS MemPtrAllocate(PtrList *list, void **out, size_t size, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PtrNode *node = NULL;

    BAIL_ON_INVALID_PTR(out, dwError);

    /* Allocate new node */
    node = (PtrNode*) malloc(sizeof(PtrNode));
    BAIL_ON_NULL_PTR(node, dwError);

    node->ptr  = NULL;
    node->dep  = dep;
    node->size = size;

    if (node->size)
    {
        /* Allocate the actual memory */
        node->ptr = malloc(node->size);
        BAIL_ON_NULL_PTR(node->ptr, dwError);

        /* ... and initialise it to zero */
        memset(node->ptr, 0, node->size);
    }

    status = MemPtrNodeAppend(list, node);
    BAIL_ON_NT_STATUS(status);

    *out = node->ptr;

cleanup:
    if (status == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        status = LwWin32ErrorToNtStatus(dwError);
    }
    
    return status;

error:
    if (node && node->ptr) {
        NFSSVC_SAFE_FREE(node->ptr);
    }

    NFSSVC_SAFE_FREE(node);
    *out = NULL;

    goto cleanup;
}


NTSTATUS MemPtrFree(PtrList *list, void *ptr)
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PtrNode *node = NULL;

    BAIL_ON_INVALID_PTR(ptr, dwError);

    /* Free the pointer and all pointer (nodes) depending on it */
    node = list->p;
    while (node) {
        if (node->dep == ptr || node->ptr == ptr) {
            PtrNode *rmnode = NULL;
            
            /* Move to the next node before removing the current one */
            rmnode = node;
            node   = node->next;

            status = MemPtrNodeRemove(list, rmnode);
            BAIL_ON_NT_STATUS(status);

            free(rmnode->ptr);
            free(rmnode);
            continue;
        }

        node = node->next;
    }

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS MemPtrAddDependant(PtrList *list, void *ptr, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PtrNode *node = NULL;

    BAIL_ON_INVALID_PTR(ptr, dwError);

    /* Allocate new node */
    node = (PtrNode*) malloc(sizeof(PtrNode));
    BAIL_ON_NULL_PTR(node, dwError);

    node->ptr  = ptr;
    node->dep  = dep;
    node->size = 0;    /* size is unknown when adding dependency */

    status = MemPtrNodeAppend(list, node);
    BAIL_ON_NT_STATUS(status);

cleanup:
    if (status == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        status = LwWin32ErrorToNtStatus(dwError);
    }
    
    return status;

error:
    /* Only the node should be freed here. node->ptr should
       be left intact */
    NFSSVC_SAFE_FREE(node);
    goto cleanup;
}




/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
