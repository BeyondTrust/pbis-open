/*
 * 
 * (c) Copyright 1992 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1992 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1992 DIGITAL EQUIPMENT CORPORATION
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 *                 permission to use, copy, modify, and distribute this
 * file for any purpose is hereby granted without fee, provided that
 * the above copyright notices and this notice appears in all source
 * code copies, and that none of the names of Open Software
 * Foundation, Inc., Hewlett-Packard Company, or Digital Equipment
 * Corporation be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Neither Open Software Foundation, Inc., Hewlett-
 * Packard Company, nor Digital Equipment Corporation makes any
 * representations about the suitability of this software for any
 * purpose.
 * 
 */
/*
 */
/*
**
**
**  NAME:
**
**      allocate.c
**
**  FACILITY:
**
**      IDL Stub Support Routines
**
**  ABSTRACT:
**
**  Stub memory allocation and free routines to keep track of all allocated
**  memory so that it can readily be freed
**
**  VERSION: DCE 1.0
**
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


#include <dce/rpc.h>
#include <dce/stubbase.h>
#include <lsysdep.h>

#ifdef DEBUG_VERBOSE
#   include <stdio.h>
#endif

#ifdef PERFMON
#include <dce/idl_log.h>
#endif


typedef struct memlink
{
    rpc_void_p_t obj;
    struct memlink *next;
} memlink;

byte_p_t 
rpc_ss_mem_alloc(rpc_ss_mem_handle *handle, unsigned bytes)
{

    error_status_t status = 0;
    byte_p_t result;

    result = rpc_sm_mem_alloc(handle, bytes, &status);

    if (status == rpc_s_no_memory)
        DCETHREAD_RAISE( rpc_x_no_memory );

    return result;
}

byte_p_t
rpc_sm_mem_alloc (rpc_ss_mem_handle *handle, unsigned bytes, error_status_t *st)
{
    memlink* l = (memlink*) handle->alloc(sizeof(memlink));

#ifdef PERFMON
    RPC_SM_MEM_ALLOC_N;
#endif

    if (l == NULL)
    {
        *st = rpc_s_no_memory;
        return NULL;
    }

    l->obj = handle->alloc(bytes);

    if (l->obj == NULL)
    {
        *st = rpc_s_no_memory;
        handle->free(l);
        return NULL;
    }

    l->next = (memlink*) handle->memory;
    handle->memory = l;

#ifdef PERFMON
    RPC_SM_MEM_ALLOC_X;
#endif

    return l->obj;
}

void 
rpc_ss_mem_free (rpc_ss_mem_handle *handle)
{
    memlink* lp, *next;
#ifdef PERFMON
    RPC_SS_MEM_FREE_N;
#endif

    for (lp = (memlink*) handle->memory; lp; lp = next)
    {
        next = lp->next;
        handle->free(lp->obj);
        handle->free(lp);
    }

#ifdef PERFMON
    RPC_SS_MEM_FREE_X;
#endif

}

void 
rpc_ss_mem_release (rpc_ss_mem_handle *handle, byte_p_t data_addr, int freeit)
{
    memlink** lp, **next, *memory;

#ifdef PERFMON
    RPC_SS_MEM_RELEASE_N;
#endif

    memory = (memlink*) handle->memory;
    for (lp = &memory; *lp; lp = next)
    {
        next = &(*lp)->next;
        
        if ((*lp)->obj == data_addr)
        {
            memlink* realnext = *next;
            if (freeit)
                handle->free((*lp)->obj);
            handle->free(*lp);
            *lp = realnext;
            break;
        }
    }
    handle->memory = (idl_void_p_t) memory;

#ifdef PERFMON
    RPC_SS_MEM_RELEASE_X;
#endif

}

#ifdef MIA
void
rpc_ss_mem_item_free (rpc_ss_mem_handle *handle, byte_p_t data_addr)
{
#ifdef PERFMON
    RPC_SS_MEM_ITEM_FREE_N;
#endif

    rpc_ss_mem_release(handle, data_addr, 1);

#ifdef PERFMON
    RPC_SS_MEM_ITEM_FREE_X;
#endif

}
#endif


#if 0
void
rpc_ss_mem_dealloc (byte_p_t data_addr)
{
#ifdef PERFMON
    RPC_SS_MEM_DEALLOC_N;
#endif

    printf("BADNESS: dealloc reached\n");

#ifdef PERFMON
    RPC_SS_MEM_DEALLOC_X;
#endif
}
#endif

#if 0
void traverse_list(rpc_ss_mem_handle handle)
{
    printf("List contains:");
    while (handle)
    {
        printf(" %d", handle);
        handle = ((header *)handle)->next;
    }
    printf(" (done)\n");
}

void main()
{
    char buf[100];
    byte_p_t tmp, *buff_addr;
    rpc_ss_mem_handle handle = NULL;

    do
    {
        printf("q/a bytes/f addr/d addr:");
        gets(buf);
        if (*buf == 'q')
        {
            rpc_ss_mem_free(&handle);
            exit();
        }
        if (*buf == 'a')
            if ((tmp = rpc_ss_mem_alloc(&handle, atoi(buf+2))) == NULL)
                printf("\tCouldn't get memory\n");
                else printf("\tGot %d\n", tmp);
        if (*buf == 'f')
            rpc_ss_mem_release(&handle, (byte_p_t)atoi(buf+2), 1);
        if (*buf == 'd')
            rpc_ss_mem_dealloc((byte_p_t)atoi(buf+2));
        traverse_list(handle);
    } while (*buf != 'q');
}
#endif
