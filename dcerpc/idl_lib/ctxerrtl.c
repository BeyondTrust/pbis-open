/*
 * 
 * (c) Copyright 1990 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1990 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1990 DIGITAL EQUIPMENT CORPORATION
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
**      ctxerrtl.c
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**      Runtime support for caller context
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

#ifdef DEBUGCTX
#  include <stdio.h>
#endif

#ifdef PERFMON
#include <dce/idl_log.h>
#endif

/* On the caller side a context handle is a pointer into the stub's data space.
    The object pointed to contains a 0 attributes word, a UUID and a handle_t */

/*****************************************************************************/
/*                                                                           */
/* Uses  malloc  to create a  context_element_t  containing the              */
/* wire rep of the context handle and the binding handle                     */
/*  of the association. Makes the caller's context handle point at           */
/* the created object. Starts liveness maintenance                           */
/*                                                                           */
/*****************************************************************************/
static void rpc_ss_create_caller_context
(
    ndr_context_handle *p_wire_context,
              /* Pointer to the wire representation of the context_handle */
    handle_t caller_handle,    /* Binding handle */
    rpc_ss_context_t *p_caller_context, /* Pointer to caller's context handle */
    error_status_t *p_st
)
{
    rpc_ss_caller_context_element_t *p_created_context;

#ifdef PERFMON
    RPC_SS_CREATE_CALLER_CONTEXT_N;
#endif

    p_created_context = (rpc_ss_caller_context_element_t *)
                       malloc(sizeof(rpc_ss_caller_context_element_t));
    if (p_created_context == NULL)
    {

#ifdef PERFMON
    RPC_SS_CREATE_CALLER_CONTEXT_X;
#endif

        DCETHREAD_RAISE( rpc_x_no_memory );
        return;
    }

    p_created_context->context_on_wire.context_handle_attributes
        = p_wire_context->context_handle_attributes;
    memcpy(
            (char *)&p_created_context->context_on_wire.context_handle_uuid,
            (char *)&p_wire_context->context_handle_uuid,
                        sizeof(dce_uuid_t));

    rpc_binding_copy(caller_handle, &p_created_context->using_handle, p_st);
    if (*p_st != error_status_ok) return;
    *p_caller_context = (rpc_ss_context_t)p_created_context;
    rpc_network_maintain_liveness(p_created_context->using_handle, p_st);
#ifdef PERFMON
    RPC_SS_CREATE_CALLER_CONTEXT_X;
#endif

}

/******************************************************************************/
/*                                                                            */
/*    Convert wire form of context handle to local form and update the stub's */
/*    internal tables                                                         */
/*                                                                            */
/******************************************************************************/
void rpc_ss_er_ctx_from_wire
(
    ndr_context_handle      *p_wire_context,
    rpc_ss_context_t        *p_caller_context, /* Pointer to application context */
    handle_t                caller_handle,     /* Binding handle */
    ndr_boolean             in_out,            /* TRUE for [in,out], FALSE for [out] */
    volatile error_status_t *p_st
)
{
#ifdef DEBUGCTX
    debug_context_uuid(&p_wire_context->context_handle_uuid, "N");
#endif

#ifdef PERFMON
    RPC_SS_ER_CTX_FROM_WIRE_N;
#endif

    if (in_out) {
        if (
            dce_uuid_is_nil(
                &p_wire_context->context_handle_uuid, (error_status_t *)p_st
            )
        ) {
            /* Context is now NIL */
            if (*p_caller_context != NULL) {
                /* If it wasn't NIL previously, stop monitoring it */
                rpc_network_stop_maintaining(
                    caller_handle, (error_status_t *)p_st
                );
                rpc_binding_free(
                     &((rpc_ss_caller_context_element_t *)*p_caller_context)
                        ->using_handle, (error_status_t *)p_st
                );
                /* Now release it */
                free((byte_p_t)*p_caller_context);
                *p_caller_context = NULL;

#ifdef PERFMON
                RPC_SS_ER_CTX_FROM_WIRE_X;
#endif

                return;
            }
        }
        else {  /* Returned context is not NIL */
            if (*p_caller_context != NULL) {
                /* And it wasn't NIL before the call */
                if (
                    ! dce_uuid_equal(
                        &p_wire_context->context_handle_uuid,
                        &((rpc_ss_caller_context_element_t *)*p_caller_context)
                            ->context_on_wire.context_handle_uuid,
                        (error_status_t *)p_st
                    )
                )
                    DCETHREAD_RAISE( rpc_x_ss_context_damaged );
            }
            else {
                /* This is a new context */
                rpc_ss_create_caller_context(
                    p_wire_context, caller_handle,
                    p_caller_context, (error_status_t *)p_st
                );
            }
        }
    }
    else {  /* Handling an OUT parameter */
        if (
            dce_uuid_is_nil(
                &p_wire_context->context_handle_uuid, (error_status_t *)p_st
            )
        )
            *p_caller_context = NULL; /* A NIL context was returned */
        else
            rpc_ss_create_caller_context(
                p_wire_context, caller_handle,
                p_caller_context, (error_status_t *)p_st
            );
    }
/*
  closedown:
*/
#ifdef PERFMON
    RPC_SS_ER_CTX_FROM_WIRE_X;
#endif

    return;
}

/******************************************************************************/
/*                                                                            */
/*    This routine converts a caller's context handle to wire format          */
/*    This routine is only called for an IN or IN OUT  context_t   parameter  */
/*                                                                            */
/******************************************************************************/
void rpc_ss_er_ctx_to_wire
(
    rpc_ss_context_t   caller_context,  /* The context handle the caller is using */
    ndr_context_handle *p_wire_context, /* Where to put data to be marshalled */
    handle_t           assoc_handle ATTRIBUTE_UNUSED,    /* Handle on which the call will be made */
    ndr_boolean        in_out,          /* TRUE for [in,out] param, FALSE for [in] */
    volatile error_status_t     *p_st
)
{

#ifdef PERFMON
    RPC_SS_ER_CTX_TO_WIRE_N;
#endif

    *p_st = error_status_ok;
    if (caller_context != NULL)
    {
        memcpy(
         (char *)p_wire_context,
         (char *)
          &((rpc_ss_caller_context_element_t *)caller_context)->context_on_wire,
             sizeof(ndr_context_handle) );
    }
    else
    {
        if ( in_out )
        {
            /* No active context. Send callee a NIL UUID */
            p_wire_context->context_handle_attributes = 0;
            dce_uuid_create_nil(&p_wire_context->context_handle_uuid,(unsigned32*)p_st);
        }
        else
        {
            DCETHREAD_RAISE( rpc_x_ss_in_null_context );
        }
    }

#ifdef DEBUGCTX
    debug_context_uuid(&p_wire_context->context_handle_uuid, "L");
#endif

#ifdef PERFMON
    RPC_SS_ER_CTX_TO_WIRE_X;
#endif

}

#ifdef DEBUGCTX
static ndr_boolean debug_file_open = ndr_false;
static char *debug_file = "ctxer.dmp";
static FILE *debug_fid;

static int debug_context_uuid(uuid_p, prefix)
    unsigned char *uuid_p;
    char *prefix;
{
    int j;
    unsigned long k;

    if (!debug_file_open)
    {
        debug_fid = fopen(debug_file, "w");
        debug_file_open = ndr_true;
    }

    fprintf(debug_fid, prefix);
    for (j=0; j<sizeof(dce_uuid_t); j++)
    {
        k = *uuid_p++;
        fprintf(debug_fid, " %02x", k);
    }
    fprintf(debug_fid, "\n");
}
#endif

/******************************************************************************/
/*                                                                            */
/* Function to be called by user to release unusable context_handle           */
/*                                                                            */
/******************************************************************************/
void rpc_ss_destroy_client_context
(
    rpc_ss_context_t *p_unusable_context_handle
)
{

#ifdef PERFMON
    RPC_SS_DESTROY_CLIENT_CONTEXT_N;
#endif

    free( *p_unusable_context_handle );
    *p_unusable_context_handle = NULL;

#ifdef PERFMON
    RPC_SS_DESTROY_CLIENT_CONTEXT_X;
#endif

}

