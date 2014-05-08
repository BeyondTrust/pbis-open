/*
 * 
 * (c) Copyright 1993 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1993 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1993 DIGITAL EQUIPMENT CORPORATION
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
**  NAME:
**
**      ctxeecli.c
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**      Maintain callee stub's table of clients
**
**
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


/* The ordering of the following 3 includes should NOT be changed! */
#include <dce/rpc.h>
#include <dce/stubbase.h>
#include <lsysdep.h>

#include <ctxeertl.h>

#   include <stdio.h>

#ifdef PERFMON
#include <dce/idl_log.h>
#endif

#ifndef DEBUG_VERBOSE
#   define NDEBUG
#endif
#include <assert.h>

#ifdef CTXEETEST
#   define DPRINT(ARGS) printf ARGS
#else
#   define DPRINT(ARGS) /* ARGS */
#endif

#include <rpcsvc.h>

/*
 *  Data structure for element of list of rundown actions
 */
typedef struct rpc_ss_rundown_list_elt {
    ctx_rundown_fn_p_t rundown; /* Pointer to rundown routine for context */
    rpc_ss_context_t user_context;
    struct rpc_ss_rundown_list_elt *next;
} rpc_ss_rundown_list_elt;

/*  Number of client slots in hash table.
*/
#define CALLEE_CLIENT_TABLE_SIZE 256

/*  Macro to hash client IDs.  ID is actually a pointer to memory,
**  probably allocated by malloc and 16-byte aligned.
*/
#define HASH_CLIENT_ID(ID) (((unsigned long)(ID) >> 4) % CALLEE_CLIENT_TABLE_SIZE)

static callee_client_entry_t *client_table = NULL;

/*  Allocate callee client lookup table and initialize pointers to NULL.
*/
void rpc_ss_init_callee_client_table(
    void
)
{
#ifdef PERFMON
    RPC_SS_INIT_CALLEE_CLNT_TABLE_N;
#endif

    assert(!client_table);      /* Must not be called more than once. */

    client_table = (callee_client_entry_t *)malloc(
        CALLEE_CLIENT_TABLE_SIZE * sizeof(callee_client_entry_t)
    );

    if (!client_table)
        DCETHREAD_RAISE(rpc_x_no_memory);

    memset(client_table, 0, CALLEE_CLIENT_TABLE_SIZE * sizeof(callee_client_entry_t));

#ifdef PERFMON
    RPC_SS_INIT_CALLEE_CLNT_TABLE_X;
#endif

}

/******************************************************************************/
/*                                                                            */
/*    Add a context to the list being maintained for a client                 */
/*      or create a client table entry for a client which it is expected      */
/*      a context will be created                                             */
/*                                                                            */
/*  Assumes that the context table mutex is locked                            */
/*                                                                            */
/******************************************************************************/
void rpc_ss_add_to_callee_client
(
    /* [in] */ rpc_client_handle_t ctx_client,
                             /* Client for whom there is or will be a context */
    /* [in] */ callee_context_entry_t *p_context,  /* Pointer to the context,
                     NULL if we are creating a client table entry to which
                     we expect a context to be attached later */
    /* [out] */ndr_boolean *p_is_new_client,
                                          /* TRUE => first context for client */
    error_status_t *result         /* Function result */
)
{
    int slot;    /* Index of the slot in the lookup table the client
                    should be in */
    ndr_boolean creating_at_home;    /* TRUE if new client being created in slot
                                    in lookup table */
    callee_client_entry_t *this_client, *next_client, *new_client;


#ifdef PERFMON
    RPC_SS_ADD_TO_CALLEE_CLIENT_N;
#endif
    /* Now find which chain */
    slot = HASH_CLIENT_ID(ctx_client);

    /* Look for the client in a chain based on the slot.
        Note the following possibilities exist for the slot:
        1) Unoccupied. No chain.
        2) Occupied. No chain.
        3) Occupied. Chain exists.
        4) Unoccupied. Chain exists.
        This is because once a client record is
            created it is constrained by the presence of threads machinery
            not to be moved */
    this_client = &client_table[slot];
    while (ndr_true)
    {
        if ( ctx_client == this_client->client )
        {
            ++this_client->count;
            *p_is_new_client = (this_client->count == 1);
            /* Linkage of contexts within client */
            p_context->p_client_entry = this_client;
            p_context->prev_in_client = this_client->last_context;
            p_context->next_in_client = NULL;
            if (this_client->first_context == NULL)
            {
                /* Client table entry created before manager was entered */
                this_client->first_context = p_context;
            }
            else
            {
                (this_client->last_context)->next_in_client = p_context;
            }
            this_client->last_context = p_context;
            *result = error_status_ok;
#ifdef PERFMON
            RPC_SS_ADD_TO_CALLEE_CLIENT_X;
#endif
            return;
        }
        next_client = this_client->next_h_client;
        if (next_client == NULL) break;
        this_client = next_client;
    }

    /* Get here only if this is the first context for the client */
    creating_at_home = (client_table[slot].client == NULL);
    if (creating_at_home)
    {
        /* The slot in the table is unoccupied */
        new_client = &client_table[slot];
    }
    else
    {
        new_client = (callee_client_entry_t *)
                            malloc(sizeof(callee_client_entry_t));
        if (new_client == NULL)
        {
            RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
            DPRINT(("Released context tables\n"));
            DCETHREAD_RAISE( rpc_x_no_memory );
        }
        this_client->next_h_client = new_client;
        new_client->prev_h_client = this_client;
        new_client->next_h_client = NULL;
    }
    new_client->client = ctx_client;
    new_client->rundown_pending = idl_false;
    RPC_SS_THREADS_CONDITION_CREATE(&(new_client->cond_var));
    if (p_context == NULL)
    {
        /* New (locking) code. Client table entry created before manager
            entered */
        new_client->count = 0;
        new_client->first_context = NULL;
        new_client->last_context = NULL;
        new_client->ref_count = 1;
        *p_is_new_client = ndr_false;
    }
    else
    {
        /* Old (non-locking) code. Client table entry created when non-null
            context marshalled after return from manager */
        new_client->count = 1;
        new_client->first_context = p_context;
        new_client->last_context = p_context;
        new_client->ref_count = 0;
        p_context->p_client_entry = new_client;
        p_context->prev_in_client = NULL;
        p_context->next_in_client = NULL;
        *p_is_new_client = ndr_true;
    }
    *result = error_status_ok;


#ifdef PERFMON
    RPC_SS_ADD_TO_CALLEE_CLIENT_X;
#endif
}

/******************************************************************************/
/*                                                                            */
/*    Remove a client entry from the client hash table                        */
/*                                                                            */
/*  Assumes that the context table mutex is locked                            */
/*                                                                            */
/******************************************************************************/
static void rpc_ss_ctx_remove_client_entry
(
    /* [in] */callee_client_entry_t *this_client
)
{
    callee_client_entry_t *next_client, *prev_client;

    RPC_SS_THREADS_CONDITION_DELETE(&(this_client->cond_var));
    prev_client = this_client->prev_h_client;
    next_client = this_client->next_h_client;

    if (prev_client != NULL)
    {
        /* Not at head of chain, cut this client out */
        prev_client->next_h_client = next_client;
        if (next_client != NULL)
        {
            next_client->prev_h_client = prev_client;
        }
        free((char_p_t)this_client);
    }
    else    /* Head of chain */
    {
        this_client->client = NULL;
        /* Hash link fields left intact. Other fields will be initialized
            next time a context is created in this slot */
    }
}

/******************************************************************************/
/*                                                                            */
/*    Remove a context from the list being maintained for a client            */
/*                                                                            */
/*  Assumes that the context table mutex is locked                            */
/*                                                                            */
/******************************************************************************/
void rpc_ss_take_from_callee_client
(
    /* [in] */ callee_context_entry_t *p_context,  /* Pointer to the context */
    /* [out] */ rpc_client_handle_t *p_close_client,
                                /* Ptr to NULL or client to stop monitoring */
    /* [out] */ error_status_t *result /* Function result */
)
{
    callee_client_entry_t *this_client;

#ifdef PERFMON
    RPC_SS_TAKE_FROM_CALLEE_CLNT_N;
#endif


    *result = error_status_ok;
    *p_close_client = NULL;
    this_client = p_context->p_client_entry;
    --this_client->count;
    if ( (this_client->count != 0) || (this_client->rundown_pending) )
    {
        /* We will not be destroying the client entry */
        /* Remove this context from the chain */
        if (this_client->first_context == p_context)
        {
            /* Context to be removed is first in client chain */
            this_client->first_context = p_context->next_in_client;
        }
        else
        {
            (p_context->prev_in_client)->next_in_client
                = p_context->next_in_client;
        }
        if (this_client->last_context == p_context)
        {
            /* Context to be removed is last in client chain */
            this_client->last_context = p_context->prev_in_client;
        }
        else
        {
            (p_context->next_in_client)->prev_in_client
                = p_context->prev_in_client;
        }
        if (this_client->count != 0)
        {
#ifdef PERFMON
            RPC_SS_TAKE_FROM_CALLEE_CLNT_X;
#endif
            return;
        }
    }

    /* Was last context for client */
    *p_close_client = this_client->client;
    if ( ! this_client->rundown_pending )
    {
        rpc_ss_ctx_remove_client_entry(this_client);
    }

#ifdef PERFMON
    RPC_SS_TAKE_FROM_CALLEE_CLNT_X;
#endif

}

/******************************************************************************/
/*                                                                            */
/*    Run down all the contexts a client owns                                 */
/*                                                                            */
/******************************************************************************/
void rpc_ss_rundown_client
(
    /* [in] */ rpc_client_handle_t failed_client
)
{
    error_status_t result;
    callee_client_entry_t *this_client;
    callee_context_entry_t *this_context;
    rpc_client_handle_t close_client = NULL;
                                       /* NULL or client to stop monitoring */
    /* FIXME: is the volatility set correctly here? */
    rpc_ss_rundown_list_elt * volatile rundown_list;
    rpc_ss_rundown_list_elt * volatile rundown_elt;
	 
    rundown_list = NULL;

#ifdef PERFMON
    RPC_SS_RUNDOWN_CLIENT_N;
#endif

    RPC_SS_THREADS_MUTEX_LOCK(&rpc_ss_context_table_mutex);
    DPRINT(("Seized context tables\n"));
    for( this_client = &client_table[HASH_CLIENT_ID(failed_client)];
         (this_client != NULL) && (close_client == NULL);
         this_client = this_client->next_h_client )
    {
        if (this_client->client == failed_client)
        {
            while (this_client->ref_count != 0)
            {
                this_client->rundown_pending = idl_true;
                RPC_SS_THREADS_CONDITION_WAIT(&this_client->cond_var,
                                              &rpc_ss_context_table_mutex);
                                        /* Mutex has been released */
                RPC_SS_THREADS_MUTEX_LOCK(&rpc_ss_context_table_mutex);
            }
            if (this_client->count == 0)
            {
                /* The manager closed the contexts while a rundown was
                        pending */
                rpc_ss_ctx_remove_client_entry(this_client);
                RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
                DPRINT(("Released context tables\n"));
#ifdef PERFMON
                RPC_SS_RUNDOWN_CLIENT_X;
#endif
                return;
            }
            /* Need to clear the rundown pending flag so that the client
                entry can be deleted */
            this_client->rundown_pending = idl_false;
            while (close_client == NULL)
            {
                /* Loop until all contexts for this client have been removed
                    from the context table. Note that each iteration brings
                    a different context to first_context position */
                this_context = this_client->first_context;
                rundown_elt = (rpc_ss_rundown_list_elt *)
                                malloc(sizeof(rpc_ss_rundown_list_elt));
                if (rundown_elt == NULL)
                {
                    RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
                    DPRINT(("Released context tables\n"));
		    /*
		     * rpc_m_ctxrundown_nomem
		     * "Out of memory while trying to run down contexts of client %x"
		     */
                    RPC_DCE_SVC_PRINTF ((
		        DCE_SVC(RPC__SVC_HANDLE, "%x"),
		        rpc_svc_libidl,
		        svc_c_sev_error,
		        rpc_m_ctxrundown_nomem,
		        this_client ));
                    return;
                }
                rundown_elt->rundown = this_context->rundown;
                rundown_elt->user_context = this_context->user_context;
                rundown_elt->next = rundown_list;
                rundown_list = rundown_elt;
                rpc_ss_lkddest_callee_context
                                    (&this_context->uuid,&close_client,&result);
            }
        }
    }
    RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
    DPRINT(("Released context tables\n"));
    while (rundown_list != NULL)
    {
        if (rundown_list->rundown != NULL)
        {
            DCETHREAD_TRY
            (*(rundown_list->rundown))(rundown_list->user_context);
            DCETHREAD_CATCH_ALL(caught)
		/*
		 * rpc_m_ctxrundown_exc
		 * "Exception in routine at %x, running down context %x of client %x"
		 */
                RPC_DCE_SVC_PRINTF ((
		    DCE_SVC(RPC__SVC_HANDLE, "%x%x%x"),
		    rpc_svc_libidl,
		    svc_c_sev_error,
		    rpc_m_ctxrundown_exc,
                    rundown_list->rundown,
		    rundown_list->user_context,
		    this_client ));
            DCETHREAD_ENDTRY
        }
        rundown_elt = rundown_list;
        rundown_list = rundown_list->next;
        free(rundown_elt);
    }

#ifdef PERFMON
    RPC_SS_RUNDOWN_CLIENT_X;
#endif

    return;
}

/******************************************************************************/
/*                                                                            */
/*    Increment the count of managers currently using contexts of a client    */
/*                                                                            */
/*  ENTRY POINT INTO LIBIDL FROM STUB                                         */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ctx_client_ref_count_inc
(
    handle_t h,
    error_status_t *p_st
)
{
    rpc_client_handle_t client_id;     /* ID of client */
    callee_client_entry_t *this_client;
    ndr_boolean is_new_client;      /* Dummy procedure parameter */

    /* This could be the first access to the context tables */
    RPC_SS_INIT_CONTEXT

    rpc_binding_inq_client(h, &client_id, p_st);
    if (*p_st != error_status_ok)
        return;
    RPC_SS_THREADS_MUTEX_LOCK(&rpc_ss_context_table_mutex);
    this_client = &client_table[HASH_CLIENT_ID(client_id)];
    while (this_client != NULL)
    {
        if (this_client->client == client_id)
        {
            (this_client->ref_count)++;
            break;
        }
        this_client = this_client->next_h_client;
    }
    if (this_client == NULL)
    {
        /* Client does not yet have any active contexts.
                                 Current manager is expected to create one */
        rpc_ss_add_to_callee_client(client_id, NULL, &is_new_client, p_st);
    }
    RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
}

/******************************************************************************/
/*                                                                            */
/*    Decrement the count of managers currently using contexts of a client    */
/*                                                                            */
/*  ENTRY POINT INTO LIBIDL FROM STUB                                         */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ctx_client_ref_count_dec
(
    handle_t h,
    error_status_t *p_st
)
{
    rpc_client_handle_t client_id;     /* ID of client */
    callee_client_entry_t *this_client;

    rpc_binding_inq_client(h, &client_id, p_st);
    if (*p_st != error_status_ok)
        return;
    RPC_SS_THREADS_MUTEX_LOCK(&rpc_ss_context_table_mutex);
    this_client = &client_table[HASH_CLIENT_ID(client_id)];
    while (this_client != NULL)
    {
        if (this_client->client == client_id)
        {
            (this_client->ref_count)--;
            if (this_client->rundown_pending)
            {
                RPC_SS_THREADS_CONDITION_SIGNAL(&this_client->cond_var);
            }
            else if ((this_client->count == 0) && (this_client->ref_count == 0))
            {
                /* This thread expected to create a context for the client,
                    but didn't */
                rpc_ss_ctx_remove_client_entry(this_client);
            }
            break;
        }
        this_client = this_client->next_h_client;
    }
    /* Can reach here with this_client == NULL if last context of client
        was destroyed by manager */
    RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
}

/*
 *  OT_8069 - context reference counts incorrectly maintained
 *  Duplicate routines so existing stubs still work
 */

/******************************************************************************/
/*                                                                            */
/*    Increment the count of managers currently using contexts of a client    */
/*                                                                            */
/*  ENTRY POINT INTO LIBIDL FROM STUB                                         */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ctx_client_ref_count_i_2
(
    handle_t h,
    rpc_client_handle_t *p_client_id,     /* [out] ID of client, or NULL */
    error_status_t *p_st
)
{
    rpc_client_handle_t client_id;     /* ID of client */
    callee_client_entry_t *this_client;
    ndr_boolean is_new_client;      /* Dummy procedure parameter */

    /* This could be the first access to the context tables */
    RPC_SS_INIT_CONTEXT

    rpc_binding_inq_client(h, p_client_id, p_st);
    if (*p_st != error_status_ok)
    {
        *p_client_id = NULL;
        return;
    }
    client_id = *p_client_id;
    RPC_SS_THREADS_MUTEX_LOCK(&rpc_ss_context_table_mutex);
    this_client = &client_table[HASH_CLIENT_ID(client_id)];
    while (this_client != NULL)
    {
        if (this_client->client == client_id)
        {
            (this_client->ref_count)++;
            break;
        }
        this_client = this_client->next_h_client;
    }
    if (this_client == NULL)
    {
        /* Client does not yet have any active contexts.
                                 Current manager is expected to create one */
        rpc_ss_add_to_callee_client(client_id, NULL, &is_new_client, p_st);
    }
    RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
}

/******************************************************************************/
/*                                                                            */
/*    Decrement the count of managers currently using contexts of a client    */
/*                                                                            */
/*  ENTRY POINT INTO LIBIDL FROM STUB                                         */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ctx_client_ref_count_d_2
(
    handle_t h ATTRIBUTE_UNUSED,
    rpc_client_handle_t client_id     /* [in] ID of client, or NULL */
)
{
    callee_client_entry_t *this_client;

    if (client_id == NULL)
        return;
    RPC_SS_THREADS_MUTEX_LOCK(&rpc_ss_context_table_mutex);
    this_client = &client_table[HASH_CLIENT_ID(client_id)];
    while (this_client != NULL)
    {
        if (this_client->client == client_id)
        {
            (this_client->ref_count)--;
            if (this_client->rundown_pending)
            {
                RPC_SS_THREADS_CONDITION_SIGNAL(&this_client->cond_var);
            }
            else if ((this_client->count == 0) && (this_client->ref_count == 0))
            {
                /* This thread expected to create a context for the client,
                    but didn't */
                rpc_ss_ctx_remove_client_entry(this_client);
            }
            break;
        }
        this_client = this_client->next_h_client;
    }
    /* Can reach here with this_client == NULL if last context of client
        was destroyed by manager */
    RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
}

#ifdef CTXEETEST
void show_client_context_chain
(
    callee_client_entry_t *p_client_entry
)
{
    callee_context_entry_t *this_context;

    this_context = p_client_entry->first_context;
    printf("\t\tForward context chain\n");
    while (this_context != NULL)
    {
        printf("\t\t\t %s %lx\n",
                &this_context->uuid,
                this_context->user_context);
        this_context = this_context->next_in_client;
    }

    this_context = p_client_entry->last_context;
    printf("\t\tBackward context chain\n");
    while (this_context != NULL)
    {
        printf("\t\t\t %s %lx\n",
                &this_context->uuid,
                this_context->user_context);
        this_context = this_context->prev_in_client;
    }

}

void dump_client_table(
    void
)
{
    int i;
    callee_client_entry_t *this_client;

    for (i=0; i<CALLEE_CLIENT_TABLE_SIZE; i++)
    {
        if  ( (client_table[i].client != NULL)
             || (client_table[i].next_h_client != NULL) )
        {
            printf("Forward client chain for client slot %d\n",i);
            this_client = &client_table[i];
            while (ndr_true)
            {
                if (this_client->client != NULL)
                {
                    printf("\t %lx %d\n",this_client->client,this_client->count);
                    show_client_context_chain(this_client);
                }
                if (this_client->next_h_client == NULL) break;
                this_client = this_client->next_h_client;
            }
            printf("Backward chain to same slot\n");
            while (ndr_true)
            {
                if (this_client->client != NULL)
                {
                    printf("\t %lx %d\n",this_client->client,this_client->count);
                    show_client_context_chain(this_client);
                }
                if (this_client->prev_h_client == NULL) break;
                this_client = this_client->prev_h_client;
            }
        }
    }
}
#endif
