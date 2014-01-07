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
**  NAME:
**
**      ctxeectx.c
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**      Maintain callee stub's table of contexts
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

#include <dce/uuid.h>

#include <ctxeertl.h>

#ifndef DEBUG_VERBOSE
#   define NDEBUG
#endif
#include <assert.h>

#ifdef PERFMON
#include <dce/idl_log.h>
#endif

#ifdef CTXEETEST
#   include <stdio.h>
#   define DPRINT(ARGS) printf ARGS
#else
#   define DPRINT(ARGS)
#endif

/******************************************************************************/
/*                                                                            */
/*    Set up CMA machinery required by context tables                         */
/*                                                                            */
/******************************************************************************/

#ifndef VMS
    ndr_boolean rpc_ss_context_is_set_up = ndr_false;
#endif

static RPC_SS_THREADS_ONCE_T context_once = RPC_SS_THREADS_ONCE_INIT;

RPC_SS_THREADS_MUTEX_T rpc_ss_context_table_mutex;

static void rpc_ss_init_context(
    void
)
{
    /* Create mutex for context handle tables */
    RPC_SS_THREADS_MUTEX_CREATE( &rpc_ss_context_table_mutex );
    /* And initialize the tables */
    rpc_ss_init_callee_ctx_tables();
}

void rpc_ss_init_context_once(
    void
)
{
    RPC_SS_THREADS_INIT;
    RPC_SS_THREADS_ONCE( &context_once, rpc_ss_init_context );
#ifndef VMS
    rpc_ss_context_is_set_up = ndr_true;
#endif

}

/*  Number of context slots in hash table.
*/
#define CALLEE_CONTEXT_TABLE_SIZE 256

static callee_context_entry_t *context_table = NULL;

/*  Allocate and initialize callee context and client lookup tables.
*/
void rpc_ss_init_callee_ctx_tables(
    void
)
{

#ifdef PERFMON
    RPC_SS_INIT_CALLEE_CTX_TABLES_N;
#endif

    assert(!context_table);     /* Must not be called more than once. */

    context_table = (callee_context_entry_t *)malloc(
        CALLEE_CONTEXT_TABLE_SIZE * sizeof(callee_context_entry_t)
    );

    if (!context_table)
        DCETHREAD_RAISE(rpc_x_no_memory);

/*****************
**
** The memset below has the same effect as this more descriptive loop.
** 
**     for (i = 0; i < CALLEE_CONTEXT_TABLE_SIZE; i++) {
**         dce_uuid_create_nil(&context_table[i].uuid, &status);
**         context_table[i].next_context = NULL;
**     }
** 
******************/

    memset (context_table, 0, 
            CALLEE_CONTEXT_TABLE_SIZE * sizeof(callee_context_entry_t));
    rpc_ss_init_callee_client_table();

#ifdef PERFMON
    RPC_SS_INIT_CALLEE_CTX_TABLES_X;
#endif

}

/******************************************************************************/
/*                                                                            */
/*    Add an entry to the callee context lookup table                         */
/*                                                                            */
/******************************************************************************/
void rpc_ss_create_callee_context
(
    rpc_ss_context_t callee_context,/* The user's local form of the context */
    dce_uuid_t    *p_uuid,              /* Pointer to the equivalent UUID */
    handle_t h,                     /* Binding handle */
    ctx_rundown_fn_p_t ctx_rundown, /* Pointer to context rundown routine */
    error_status_t *result     /* Function result */
)
{
    rpc_client_handle_t  ctx_client;         /* ID of client owning context */
    callee_context_entry_t *this_link, *next_link, * volatile new_link;
    ndr_boolean is_new_client;

    //DO_NOT_CLOBBER(new_link);

#ifdef PERFMON
    RPC_SS_CREATE_CALLEE_CONTEXT_N;
#endif

    /* If this is the first context to be created, initialization is needed */
    RPC_SS_INIT_CONTEXT

    rpc_binding_inq_client(h, &ctx_client, (error_status_t *) result);
    if (*result != error_status_ok) return;

    RPC_SS_THREADS_MUTEX_LOCK(&rpc_ss_context_table_mutex);
    DPRINT(("Seized context tables\n"));
    this_link = &context_table[dce_uuid_hash(p_uuid,(error_status_t *)result)
                                               % CALLEE_CONTEXT_TABLE_SIZE];
    if ( dce_uuid_is_nil(&this_link->uuid, (error_status_t *)result) )
    {
        /* Home slot in the hash table is empty */
        new_link = this_link;
        next_link = NULL;
    }
    else
    {
        /* Put the new item at the head of the overflow chain */
        new_link = (callee_context_entry_t *)
                             malloc(sizeof(callee_context_entry_t));
        if (new_link == NULL)
        {
            RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
            DPRINT(("Released context tables\n"));
            DCETHREAD_RAISE( rpc_x_no_memory );
        }
        next_link = this_link->next_context;
        this_link->next_context = new_link;
    }

    /* Fill in fields of context entry */
    memcpy(
        (char *)&new_link->uuid,
        (char *)p_uuid,
        sizeof(dce_uuid_t)
    );
    new_link->user_context = callee_context;
    new_link->rundown = ctx_rundown;
    new_link->next_context = next_link;

    DCETHREAD_TRY
    rpc_ss_add_to_callee_client(ctx_client,new_link,&is_new_client,
                                result);
    DCETHREAD_FINALLY
    RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
    DPRINT(("Released context tables\n"));
    DCETHREAD_ENDTRY
    if ((*result == error_status_ok) && is_new_client)
    {
        rpc_network_monitor_liveness( h, ctx_client,
                             rpc_ss_rundown_client,
                             (error_status_t *) result );
#ifdef PERFMON
    RPC_SS_CREATE_CALLEE_CONTEXT_X;
#endif
    }
}

/******************************************************************************/
/*                                                                            */
/*  Update an entry in the callee context lookup table                        */
/*  *result is error_status_ok unless the UUID is not in the lookup table.    */
/*                                                                            */
/******************************************************************************/
void rpc_ss_update_callee_context
(
    rpc_ss_context_t    callee_context, /* The user's local form of the context */
    dce_uuid_t              *p_uuid,        /* Pointer to the equivalent UUID */
    error_status_t      *result         /* Function result */
)
{
    callee_context_entry_t *this_link;

#ifdef PERFMON
    RPC_SS_UPDATE_CALLEE_CONTEXT_N;
#endif

    RPC_SS_THREADS_MUTEX_LOCK(&rpc_ss_context_table_mutex);
    DPRINT(("Seized context tables\n"));
    this_link = &context_table[dce_uuid_hash(p_uuid,result)
                                               % CALLEE_CONTEXT_TABLE_SIZE];
    while ( ! dce_uuid_equal(p_uuid,&this_link->uuid,result) )
    {
        this_link = this_link->next_context;
        if (this_link == NULL)
        {
            RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
            DPRINT(("Released context tables\n"));
            DCETHREAD_RAISE( rpc_x_ss_context_mismatch);
        }
    }
    this_link->user_context = callee_context;
    RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
    DPRINT(("Released context tables\n"));
    *result = error_status_ok;

#ifdef PERFMON
    RPC_SS_UPDATE_CALLEE_CONTEXT_X;
#endif

}

/******************************************************************************/
/*                                                                            */
/*    Find the local context that corresponds to a supplied UUID              */
/*                                                                            */
/*  ENTRY POINT INTO LIBIDL FROM STUB                                         */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ee_ctx_from_wire
(
    ndr_context_handle      *p_wire_context,
    rpc_ss_context_t        *p_context,         /* The application context */
    volatile error_status_t *p_st
)
{
    dce_uuid_t *p_uuid;    /* Pointer to the UUID that has come off the wire */
    callee_context_entry_t *this_link;

#ifdef PERFMON
    RPC_SS_EE_CTX_FROM_WIRE_N;
#endif

    p_uuid = &p_wire_context->context_handle_uuid;

#ifdef DEBUGCTX
    debug_context_lookup(p_uuid);
    debug_context_table();
#endif

    *p_st = error_status_ok;
    if ( dce_uuid_is_nil(p_uuid, (error_status_t *)p_st) )
    {
        *p_context = NULL;

#ifdef PERFMON
        RPC_SS_EE_CTX_FROM_WIRE_X;
#endif

        return;
    }
    RPC_SS_THREADS_MUTEX_LOCK(&rpc_ss_context_table_mutex);
    DPRINT(("Seized context tables\n"));
    this_link = &context_table[dce_uuid_hash(p_uuid, (error_status_t *)p_st)
                                               % CALLEE_CONTEXT_TABLE_SIZE];
    while ( ! dce_uuid_equal(p_uuid,&this_link->uuid, (error_status_t *)p_st) )
    {
        this_link = this_link->next_context;
        if (this_link == NULL)
        {
            RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
            DPRINT(("Released context tables\n"));

#ifdef PERFMON
            RPC_SS_EE_CTX_FROM_WIRE_X;
#endif

            DCETHREAD_RAISE( rpc_x_ss_context_mismatch );
            return;
        }
    }
    *p_context = this_link->user_context;
    RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
    DPRINT(("Released context tables\n"));

#ifdef PERFMON
    RPC_SS_EE_CTX_FROM_WIRE_X;
#endif

    return;
}

/******************************************************************************/
/*                                                                            */
/*    Take a lock on the context tables and destroy a context entry           */
/*                                                                            */
/******************************************************************************/
void rpc_ss_destroy_callee_context
(
    dce_uuid_t *p_uuid,             /* Pointer to UUID of context to be destroyed */
    handle_t  h,                /* Binding handle */
    error_status_t *result /* Function result */
)    /* Returns error_status_ok unless the UUID is not in the lookup table */
{
    rpc_client_handle_t close_client;   /* NULL or client to stop monitoring */

#ifdef PERFMON
    RPC_SS_DESTROY_CALLEE_CONTEXT_N;
#endif

    RPC_SS_THREADS_MUTEX_LOCK(&rpc_ss_context_table_mutex);
    DPRINT(("Seized context tables\n"));
    rpc_ss_lkddest_callee_context(p_uuid,&close_client,result);
    RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
    DPRINT(("Released context tables\n"));
    if ((*result == error_status_ok) && (close_client != NULL))
    {
        rpc_network_stop_monitoring(h, close_client, (error_status_t *) result);
    }
#ifdef PERFMON
    RPC_SS_DESTROY_CALLEE_CONTEXT_X;
#endif
}

/******************************************************************************/
/*                                                                            */
/*    Destroy an entry in the context table                                   */
/*                                                                            */
/*  Assumes that the context table mutex is locked                            */
/*                                                                            */
/******************************************************************************/
void rpc_ss_lkddest_callee_context
(
    dce_uuid_t *p_uuid,    /* Pointer to UUID of context to be destroyed */
    rpc_client_handle_t *p_close_client,
                                /* Ptr to NULL or client to stop monitoring */
    error_status_t *result /* Function result */
)    /* Returns error_status_ok unless the UUID is not in the lookup table */
{
    callee_context_entry_t *this_link, *next_link, *last_link;

#ifdef PERFMON
    RPC_SS_LKDDEST_CALLEE_CONTEXT_N;
#endif

    this_link = &context_table[dce_uuid_hash(p_uuid,(error_status_t *) result)
                                               % CALLEE_CONTEXT_TABLE_SIZE];
    next_link = this_link->next_context;
    if ( dce_uuid_equal(p_uuid,&this_link->uuid, (error_status_t *) result) )
    {
        /* Context to be destroyed is in home slot */
        rpc_ss_take_from_callee_client(this_link,p_close_client,result);
        if (next_link == NULL)
        {
            /* There is no chain from the home slot */
            dce_uuid_create_nil(&this_link->uuid, (error_status_t *) result);
        }
        else
        {
            /* Move the second item in the chain to the home slot */
            memcpy(
                (char *)&this_link->uuid,
                (char *)&next_link->uuid,
                sizeof(dce_uuid_t)
            );
            this_link->user_context = next_link->user_context;
            this_link->rundown = next_link->rundown;
            this_link->p_client_entry = next_link->p_client_entry;
            this_link->prev_in_client = next_link->prev_in_client;
            if (this_link->prev_in_client == NULL)
            {
                (this_link->p_client_entry)->first_context = this_link;
            }
            else
            {
                (this_link->prev_in_client)->next_in_client = this_link;
            }
            this_link->next_in_client = next_link->next_in_client;
            if (this_link->next_in_client == NULL)
            {
                (this_link->p_client_entry)->last_context = this_link;
            }
            else
            {
                (this_link->next_in_client)->prev_in_client = this_link;
            }
            this_link->next_context = next_link->next_context;
            /* And release the memory it was in */
            free((char_p_t)next_link);
        }

#ifdef PERFMON
        RPC_SS_LKDDEST_CALLEE_CONTEXT_X;
#endif

        return;
    }
    else    /* Context is further down chain */
    {
        while (next_link != NULL)
        {
            last_link = this_link;
            this_link = next_link;
            next_link = this_link->next_context;
            if ( dce_uuid_equal(p_uuid,&this_link->uuid,(error_status_t *)result) )
            {
                rpc_ss_take_from_callee_client(this_link,p_close_client,result);
                /* Relink chain to omit found entry */
                last_link->next_context = next_link;
                /* And free the memory it occupied */
                free((char_p_t)this_link);

#ifdef PERFMON
                RPC_SS_LKDDEST_CALLEE_CONTEXT_X;
#endif

                return;
            }
        }
        RPC_SS_THREADS_MUTEX_UNLOCK(&rpc_ss_context_table_mutex);
        DPRINT(("Released context tables\n"));
        DCETHREAD_RAISE( rpc_x_ss_context_mismatch);
    }
}


#ifdef CTXEETEST
void dump_context_table()
{
    int i;
    callee_context_entry_t *this_link;
    callee_client_entry_t *this_client;
    error_status_t status;

    for (i=0; i<CALLEE_CONTEXT_TABLE_SIZE; i++)
    {
        if ( ! dce_uuid_is_nil(&context_table[i].uuid, &status) )
        {
            this_link = &context_table[i];
            printf("Context chain for context_slot %d\n",i);
            while (this_link != NULL)
            {
                printf("\t %s %lx ",
                        &this_link->uuid,
                        this_link->user_context);
                this_client = this_link->p_client_entry;
                printf("Client %lx %d\n",
                        this_client->client,
                        this_client->count);
                this_link = this_link->next_context;
            }
        }
    }
}
#endif

#ifdef DEBUGCTX
static ndr_boolean debug_file_open = ndr_false;
static char *debug_file = "ctxee.dmp";
static FILE *debug_fid;

static int debug_context_lookup(uuid_p)
    unsigned char *uuid_p;
{
    int j;
    unsigned long k;

    if (!debug_file_open)
    {
        debug_fid = fopen(debug_file, "w");
        debug_file_open = ndr_true;
    }

    fprintf(debug_fid, "L");
    for (j=0; j<sizeof(dce_uuid_t); j++)
    {
        k = *uuid_p++;
        fprintf(debug_fid, " %02x", k);
    }
    fprintf(debug_fid, "\n");
}

static int debug_context_add(user_context)
    long user_context;
{
    if (!debug_file_open)
    {
        debug_fid = fopen(debug_file, "w");
        debug_file_open = ndr_true;
    }

    fprintf(debug_fid, "N %lx\n", user_context);
}

static int debug_context_table()
{
    int i, j;
    unsigned long k;
    ndr_boolean at_home;
    unsigned char *uuid_p;
    callee_context_entry_t *this_link;
    callee_client_entry_t *this_client;
    error_status_t status;

    if (!debug_file_open)
    {
        debug_fid = fopen(debug_file, "w");
        debug_file_open = ndr_true;
    }

    for (i=0; i<CALLEE_CONTEXT_TABLE_SIZE; i++)
    {
        if ( ! dce_uuid_is_nil(&context_table[i].uuid, &status) )
        {
            at_home = ndr_true;
            this_link = &context_table[i];
            while (this_link != NULL)
            {
                if (at_home)
                {
                    at_home = ndr_false;
                    fprintf(debug_fid, "%4d:", i);
                }
                else
                    fprintf(debug_fid, "   C:", i);
                uuid_p = (unsigned char *)&this_link->uuid;
                for (j=0; j<sizeof(this_link->uuid); j++)
                {
                    k = *uuid_p++;
                    fprintf(debug_fid, " %02x", k);
                }
                this_client = this_link->p_client_entry;
                fprintf(debug_fid, " client %lx %d\n",
                        this_client->client,
                        this_client->count);
                this_link = this_link->next_context;
            }
        }
    }
}
#endif

/******************************************************************************/
/*                                                                            */
/*    Routine to be called when a callee argument of type  context_t  is to   */
/*    be marshalled                                                           */
/*                                                                            */
/*  ENTRY POINT INTO LIBIDL FROM STUB                                         */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ee_ctx_to_wire
(
    rpc_ss_context_t        callee_context,   /* The application context */
    ndr_context_handle      *p_wire_context,  /* Pointer to wire form of context */
    handle_t                h,                /* Binding handle */
    ctx_rundown_fn_p_t      ctx_rundown,      /* Pointer to context rundown routine */
    ndr_boolean             in_out,           /* TRUE for [in,out], FALSE for [out] */
    volatile error_status_t *p_st
)
{
#ifdef DEBUGCTX
    debug_context_add(callee_context);
#endif

    int wire, context;
    p_wire_context->context_handle_attributes = 0;  /* Only defined value. */

    /*  Boolean conditions are
     *      wire:    UUID from wire is valid (not nil) -- in_out must be true.
     *      context: callee context from manager is valid (not NULL).
     *      in_out:  context handle parameter is [in,out] (not just [out]).
     *  Mapping of conditions onto actions implemented in the following switch.
     *      wire  context  in_out  condition  Action
     *       0       0       0         0      create nil UUID
     *       0       0       1         1      do nothing -- use old (nil) UUID
     *       0       1       0         2      create new UUID and new context
     *       0       1       1         3      create new UUID and new context
     *       1       0       0         4      impossible
     *       1       0       1         5      destroy existing context, make UUID nil
     *       1       1       0         6      impossible
     *       1       1       1         7      update existing context
     */

#ifdef PERFMON
    RPC_SS_EE_CTX_TO_WIRE_N;
#endif

    wire    = in_out && !dce_uuid_is_nil(
                &p_wire_context->context_handle_uuid, (error_status_t *)p_st
            )? 4: 0;
    context = callee_context? 2: 0;
    in_out  = in_out? 1: 0;

    switch (wire | context | in_out) {
    case 0:
        dce_uuid_create_nil(
            &p_wire_context->context_handle_uuid, (error_status_t *)p_st
        );
        break;
    case 1:
        *p_st = error_status_ok;
        break;
    case 2:
    case 3:
        dce_uuid_create(
            &p_wire_context->context_handle_uuid, (error_status_t *)p_st
        );
        rpc_ss_create_callee_context(
            callee_context, &p_wire_context->context_handle_uuid, h,
            ctx_rundown, (error_status_t *)p_st
        );
        break;
    case 5:
        rpc_ss_destroy_callee_context(
            &p_wire_context->context_handle_uuid, h, (error_status_t *)p_st
        );
        if (*p_st != error_status_ok) break;
        dce_uuid_create_nil(
            &p_wire_context->context_handle_uuid, (error_status_t *)p_st
        );
        break;
    case 7:
        rpc_ss_update_callee_context(
            callee_context, &p_wire_context->context_handle_uuid,
            (error_status_t *)p_st
        );
        break;
    default:    /* All invalid conditions evaluate true. */
        assert(!(wire | context | in_out));
        break;
    }

#ifdef DEBUGCTX
    debug_context_table();
#endif

#ifdef PERFMON
    RPC_SS_EE_CTX_TO_WIRE_X;
#endif

}
