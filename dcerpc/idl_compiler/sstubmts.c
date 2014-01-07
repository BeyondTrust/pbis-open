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
**  NAME:
**
**      sstubmts.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Generation of server stub file for MTS compiler
**
*/

#include <nidl.h>
#include <ast.h>
#include <bedeck.h>
#include <command.h>
#include <ddbe.h>
#include <ifspec.h>
#include <cspeldcl.h>
#include <cspell.h>
#include <mtsbacke.h>
#include <mtspipes.h>
#include <sstubmts.h>
#include <user_exc.h>
#include <clihandl.h>
#include <icharsup.h>
#include <cstubmts.h>

typedef struct param_node_link_t {
    AST_parameter_n_t *p_param;
    struct param_node_link_t *p_next;
} param_node_link_t;
/* Used to build a list of "interesting" parameters of an operation */

typedef enum {
                                /* handle_t parameter with [represent_as] */
    BE_no_rep_as_handle_t_k,  /* not present */
    BE_val_rep_as_handle_t_k, /* passed by value */
    BE_ref_rep_as_handle_t_k  /* passed by reference */
} BE_rep_as_handle_t_k_t;

static char rep_as_handle_name[] = "IDL_handle_rep_as";

/*
 * BE_server_binding_analyze
 *
 * Find whether operation can be called by [auto_handle] client, and
 * whether there is a first handle_t parameter with a [represent_as] attribute
 *
 */
static void BE_server_binding_analyze
(
    AST_operation_n_t *p_operation,
    boolean *server_binding_explicit, /* TRUE if no client can use [auto_handle]
                                         binding with this operation */
    BE_rep_as_handle_t_k_t *p_rep_as_handle_param,
    NAMETABLE_id_t *p_rep_as_type_name,  /* type of handle param */
    NAMETABLE_id_t *p_binding_handle_name
)
{
    AST_parameter_n_t *p_first_parameter;
    AST_type_n_t *p_type, *p_pointee_type;

    *p_rep_as_handle_param = BE_no_rep_as_handle_t_k;
    *p_binding_handle_name = NAMETABLE_add_id("IDL_binding_handle");
    p_first_parameter = p_operation->parameters;
    if ( p_first_parameter == NULL )
    {
        /* No parameters */
        *server_binding_explicit = false;
        return;
    }
    p_type = p_first_parameter->type;
    if ( p_type->kind == AST_handle_k )
    {
        /* handle_t by value */
        *server_binding_explicit = true;
        *p_binding_handle_name = p_first_parameter->name;
        if ( p_type->rep_as_type != NULL )
        {
            *p_rep_as_handle_param = BE_val_rep_as_handle_t_k;
            *p_rep_as_type_name = p_type->name;
        }
        return;
    }
    if ( AST_HANDLE_SET(p_type) )
    {
        /* Customized handle by value */
        *server_binding_explicit = true;
        return;
    }
    if ( p_type->kind == AST_pointer_k )
    {
        p_pointee_type = p_type->type_structure.pointer->pointee_type;
        if ( p_pointee_type->kind == AST_handle_k )
        {
            /* handle_t by reference */
            *server_binding_explicit = true;
            *p_binding_handle_name = p_first_parameter->name;
            if ( p_pointee_type->rep_as_type != NULL )
            {
                *p_rep_as_handle_param = BE_ref_rep_as_handle_t_k;
                *p_rep_as_type_name = p_pointee_type->name;
            }
            return;
        }
        if ( AST_HANDLE_SET(p_pointee_type) )
        {
            /* Customized handle by reference */
            *server_binding_explicit = true;
            return;
        }
    }
    if ( AST_HAS_IN_CTX_SET(p_operation) )
    {
        *server_binding_explicit = true;
        return;
    }
    if ( AST_EXPLICIT_HANDLE_SET(p_operation) )
    {
        /* [explicit_handle] in ACF */
        *server_binding_explicit = false;
        return;
    }
    *server_binding_explicit = false;
    return;
}


/*
 * DDBE_spell_stack_surrogates
 *
 * Spell server surrogates as stack variables
 */
static void DDBE_spell_stack_surrogates
(
    FILE *fid,
    param_node_link_t **p_fixed_char_array_list ATTRIBUTE_UNUSED,
                /* Pointer to list of fixed size character array parameters */
    AST_operation_n_t *p_operation
)
{
    unsigned long param_index;
    AST_parameter_n_t *pp;  /* Pointer down list of parameters */
    AST_type_n_t array_elt_ptr_type_node;
    AST_pointer_n_t array_elt_ptr_pointer_node;

    param_index = 0;
    for (pp = p_operation->parameters; pp != NULL; pp = pp->next) 
    {
        param_index++;
        if (param_index == 1)
        {
            if (pp->type->kind == AST_handle_k)
            {
                continue;
            }
            else if ( (pp->type->kind == AST_pointer_k)
                      && (pp->type->type_structure.pointer->pointee_type->kind
                                                     == AST_handle_k) )
            {
                continue;
            }
        }
        if (AST_REF_SET(pp) && (pp->type->rep_as_type == NULL)
            && (AST_CONFORMANT_SET(pp->type)
                      || (DDBE_ARRAYIFIED(pp)
                          && AST_CONFORMANT_SET(pp->type->type_structure.
                             pointer->pointee_type->array_rep_type))))
        {
            /* [ref] arrayified pointer or array without [ptr] or [unique]
                 Storage allocated by interpreter */
            continue;
        }
        if ((pp->type->kind == AST_pointer_k) && AST_REF_SET(pp)
                 && AST_CONFORMANT_SET(pp->type->type_structure.pointer
                                        ->pointee_type)
                 && (pp->type->type_structure.pointer->pointee_type
                        ->rep_as_type == NULL))
        {
            /* [ref] pointer to conformant object
                 Storage allocated by interpreter */
            continue;
        }
        /*
         * No surrogate is needed for a [heap] parameter unless it has a
         * non-[ref] pointer attribute in which case a surrogate for the
         * pointer is needed and the pointee is allocated in the Interpreter.
         */
        if (AST_HEAP_SET(pp) && !AST_PTR_SET(pp) && !AST_UNIQUE_SET(pp))
        {
            /* No stack surrogate. Storage allocated by interpreter */
            continue;
        }
        if (AST_CONTEXT_SET(pp))
        {
            /* Special data structure for context handle */
            fprintf(fid,"IDL_ee_context_t %s;\n", BE_get_name(pp->name));
            continue;
        }
        if (AST_CONTEXT_SET(pp))
        {
            /* Special data structure for context handle */
            fprintf(fid,"IDL_ee_context_t %s;\n", BE_get_name(pp->name));
            continue;
        }
        if (pp->type->kind == AST_pointer_k)
        {
            CSPELL_typed_name(fid,
                              (AST_REF_SET(pp)
                               ? pp->type->type_structure.pointer->pointee_type
                               : pp->type),
                              pp->name, NULL, false, true, false);
        }
        else if ( (pp->type->kind == AST_array_k)
                    && (AST_UNIQUE_SET(pp) || AST_PTR_SET(pp)) )
        {
            array_elt_ptr_pointer_node.pointee_type
                = pp->type->type_structure.array->element_type;
            array_elt_ptr_type_node.name = NAMETABLE_NIL_ID;
            array_elt_ptr_type_node.kind = AST_pointer_k;
            array_elt_ptr_type_node.flags = 0;
            array_elt_ptr_type_node.xmit_as_type = NULL;
            array_elt_ptr_type_node.rep_as_type = NULL;
            array_elt_ptr_type_node.cs_char_type = NULL;
            array_elt_ptr_type_node.array_rep_type = NULL;
            array_elt_ptr_type_node.type_structure.pointer
                = &array_elt_ptr_pointer_node;
            CSPELL_typed_name(fid, &array_elt_ptr_type_node,
                              pp->name, NULL, false, true, false);
        }
        else
            CSPELL_typed_name(fid, pp->type, pp->name, NULL, false, true, 
                                false);
        fprintf(fid, ";\n");
    }
}

/*
 * CSPELL_manager_call
 *
 * Emit a call to a manager operation
 */
static void CSPELL_manager_call
(
    FILE *fid,
    AST_interface_n_t *p_interface,
    AST_operation_n_t *p_operation,
    BE_rep_as_handle_t_k_t rep_as_handle_param,
    NAMETABLE_id_t rep_as_type_name,
    NAMETABLE_id_t binding_handle_name
)
{
    AST_parameter_n_t *pp;  /* Pointer down list of parameters */
    int param_index;        /* Index of parameter in param list */
    int visible_param_count;    /* Number of parameters spelt into call */

    if ( rep_as_handle_param != BE_no_rep_as_handle_t_k )
    {
        fprintf( fid, "%s_to_local(&%s,&%s);\n",
                BE_get_name(rep_as_type_name),
                BE_get_name(binding_handle_name),
                rep_as_handle_name );
    }

    fprintf (fid, "\n/* manager call */\n");
    fprintf( fid, "IDL_manager_entered = ndr_true;\n" );
    fprintf( fid, "RPC_SS_THREADS_DISABLE_ASYNC(IDL_async_cancel_state);\n");

    if (AST_NO_CANCEL_SET(p_operation))
        fprintf( fid, "RPC_SS_THREADS_DISABLE_GENERAL(IDL_general_cancel_state);\n" );
    else
        fprintf( fid, "RPC_SS_THREADS_ENABLE_GENERAL(IDL_general_cancel_state);\n" );

    if (p_operation->result->type->kind != AST_void_k)
    {
        if (AST_CONTEXT_SET(p_operation->result))
            fprintf(fid, "IDL_function_result.local = ");
        else
            fprintf(fid, "IDL_function_result = ");
    }

    fprintf(fid, "(*((%s_v%ld_%ld_epv_t *)IDL_mgr_epv)->",
            BE_get_name(p_interface->name), (p_interface->version%65536),
            (p_interface->version/65536));
            spell_name(fid, p_operation->name);

    fprintf(fid,")(");
    visible_param_count = 0;
    param_index = 1;
    for (pp = p_operation->parameters; pp != NULL; pp = pp->next)
    {
        if (AST_HIDDEN_SET(pp))
        {
            /* Parameter does not appear in signature delivered to user */
            /* Note that hidden parameter cannot appear before binding handle */
            param_index++;
            continue;
        }
        else
        {
            visible_param_count++;
            if (visible_param_count > 1)
                fprintf (fid, ",\n ");
        }
        if (param_index == 1)
        {
            if ( rep_as_handle_param != BE_no_rep_as_handle_t_k )
            {
                fprintf( fid, "%c%s",
                  ((rep_as_handle_param == BE_ref_rep_as_handle_t_k)
                                                                        )
                                                                    ? '&' : ' ',
                  rep_as_handle_name );
                param_index++;
                continue;
            }
            else if (pp->type->kind == AST_handle_k)
            {
                spell_name(fid, binding_handle_name);
                param_index++;
                continue;
            }
            else if ( (pp->type->kind == AST_pointer_k)
                      && (pp->type->type_structure.pointer->pointee_type->kind
                                                     == AST_handle_k) )
            {
                fprintf(fid, "&%s", BE_get_name(binding_handle_name));
                param_index++;
                continue;
            }
        }

        if ((AST_HEAP_SET(pp)
            || (AST_CONFORMANT_SET(pp->type)
                 && !AST_UNIQUE_SET(pp) && !AST_PTR_SET(pp))
            || ((pp->type->kind == AST_pointer_k) && AST_REF_SET(pp)
                    && (BE_Is_Arrayified(pp,pp->type)
                        || AST_CONFORMANT_SET(pp->type->type_structure.pointer
                                                ->pointee_type))))
           )
        {
            /*
             * Cast needed on (void *) pointer to dynamically allocated store.
             * A different cast is needed if we need to dereference the ptr.
             */
            boolean deref;
            if ((pp->type->kind == AST_array_k)
                && (AST_UNIQUE_SET(pp) || AST_PTR_SET(pp)))
            {
                DDBE_spell_manager_param_cast( fid, pp->type );
                fprintf(fid, "*(rpc_void_p_t *)");
            }
            else
            {
                deref = ((pp->type->kind != AST_pointer_k)
                     && (pp->type->kind != AST_array_k)
                    );
                if (!deref)
                    DDBE_spell_manager_param_cast( fid, pp->type );
                fprintf( fid, "%c", ((deref) ? '*' : ' ') );
                if (deref)
                    CSPELL_ptr_cast_exp( fid, pp->type );
            }
            fprintf( fid, "(IDL_param_vec[%d])", param_index );
        }
        else if (AST_CONTEXT_SET(pp))
        {
            /* Opaque context handle type requires cast */
            if (pp->type->type_structure.pointer->pointee_type->kind
                == AST_pointer_k
                &&  pp->type->type_structure.pointer->pointee_type->
                    type_structure.pointer->pointee_type->kind
                    == AST_structure_k)
                CSPELL_cast_exp(fid, pp->type);
            /* Context handle by value is pointer to void,
                              by reference is pointer to pointer to void */ 
            fprintf( fid, "%c%s.local", 
                     ((pp->type->type_structure.pointer->pointee_type->kind
                                                            == AST_pointer_k)
                                                                            )
                                                                    ? '&' : ' ',
                     BE_get_name(pp->name) );
        }
        else
        {
            if (
                   ((pp->type->kind == AST_pointer_k) && AST_REF_SET(pp)
                    && !(BE_Is_Arrayified(pp,pp->type))) )
            {
                /* non-arrayified parameter passed by reference, but not for interfaces */
					if (!(pp->type->kind == AST_pointer_k && pp->type->type_structure.pointer->pointee_type->kind == AST_interface_k))
                fprintf(fid, "&");
            }
            spell_name(fid, pp->name);
        }
        
        param_index++;
    }

    fprintf(fid, ");\n");
    fprintf( fid,
              "RPC_SS_THREADS_RESTORE_GENERAL(IDL_general_cancel_state);\n" );
    fprintf( fid, "RPC_SS_THREADS_RESTORE_ASYNC(IDL_async_cancel_state);\n");

    if ( rep_as_handle_param != BE_no_rep_as_handle_t_k )
    {
        fprintf( fid, "%s_free_local(&%s);\n",
                BE_get_name(rep_as_type_name),
                rep_as_handle_name );
    }
}

/*
 * DDBE_convert_out_contexts
 *
 * Convert user [out] context handles from local format to wire format
 * and change the param vector entries to point at the wire formats
 *
 */
static void DDBE_convert_out_contexts
(
    FILE *fid,
    AST_operation_n_t *p_operation,
    NAMETABLE_id_t binding_handle_name
)
{
    AST_parameter_n_t *pp;

    if ( AST_CONTEXT_SET(p_operation->result) )
    {
        fprintf(fid,
"rpc_ss_ee_ctx_to_wire(IDL_function_result.local,&IDL_function_result.wire, %s,",
        BE_get_name(binding_handle_name));
        if ( AST_CONTEXT_RD_SET(p_operation->result->type) )
        {
            spell_name(fid, p_operation->result->type->name);
            fprintf(fid, "_rundown, ");
        }
        else
            fprintf(fid, "(void (*)())NULL, ");
        fprintf(fid, "idl_false, &IDL_ms.IDL_status);\n");
        CSPELL_test_status(fid);
    }

    for (pp = p_operation->parameters; pp != NULL; pp = pp->next) 
    {
        if ( AST_OUT_SET(pp) && AST_CONTEXT_SET(pp) )
        {
            fprintf(fid, "rpc_ss_ee_ctx_to_wire(%s.local, &%s.wire, %s,",
                    BE_get_name(pp->name), BE_get_name(pp->name),
                    BE_get_name(binding_handle_name));
            /* [out] context must be passed by reference.
                    Does it require rundown? */
            if ( AST_CONTEXT_RD_SET(pp->type->type_structure.pointer
                                                            ->pointee_type) )
            {
                spell_name(fid, pp->type->type_structure.pointer
                                                          ->pointee_type->name);
                fprintf(fid, "_rundown, ");
            }
            else
                fprintf(fid, "(void (*)())NULL, ");
            fprintf(fid, "%s, &IDL_ms.IDL_status);\n",
                         AST_IN_SET(pp) ? "idl_true" : "idl_false");
            CSPELL_test_status(fid);
        }
    }
}

/*
 *  CSPELL_server_stub_routine
 *
 *  Generate a server stub routine for an operation
 */
static void CSPELL_server_stub_routine
(
    FILE *fid,
    language_k_t language ATTRIBUTE_UNUSED,
    AST_interface_n_t *p_interface,
    AST_operation_n_t *p_operation,
    int num_declared_exceptions,    /* Count of user declared exceptions */
    int num_extern_exceptions,       /* Count of user extern_exceptions */
    boolean *cmd_opt
)
{
    long first_pipe;        /* Index of first pipe to be processed */
    boolean explicit_binding;
    BE_rep_as_handle_t_k_t rep_as_handle_param;
    NAMETABLE_id_t rep_as_type_name = NULL;
    NAMETABLE_id_t binding_handle_name = NULL;
    param_node_link_t *fixed_char_array_list = NULL;
                /* List of fixed size character array parameters */
    BE_cs_info_t cs_info;           /* I-char machinery description */
    BE_handle_info_t handle_info;
    boolean midl_mode = cmd_opt[opt_midl];

    BE_server_binding_analyze(p_operation, &explicit_binding,
                 &rep_as_handle_param, &rep_as_type_name, &binding_handle_name);
    handle_info.deref_assoc = ' ';
    NAMETABLE_id_to_string(binding_handle_name, &handle_info.assoc_name);

    fprintf (fid, "\nstatic void op%d_ssr", p_operation->op_number);

    fprintf (fid, "(\n");
    fprintf (fid, " handle_t %s,\n", handle_info.assoc_name);
    fprintf (fid, " rpc_call_handle_t IDL_call_h,\n");
    fprintf (fid, " rpc_iovector_elt_p_t IDL_elt_p,\n");
    fprintf (fid, " ndr_format_p_t IDL_drep_p,\n");
    fprintf (fid, " __IDL_UNUSED__ rpc_transfer_syntax_p_t IDL_transfer_syntax_p,\n");
    fprintf (fid, " rpc_mgr_epv_t IDL_mgr_epv,\n");
    fprintf (fid, " error_status_t *IDL_status_p\n)\n");

    fprintf (fid, "{\n");
    fprintf(fid, "IDL_ms_t IDL_ms;\n");
    fprintf(fid, "volatile ndr_boolean IDL_manager_entered = ndr_false;\n");
    fprintf(fid,
         "volatile RPC_SS_THREADS_CANCEL_STATE_T IDL_async_cancel_state=RPC_SS_THREADS_CANCEL_STATE_T_INITIALIZER;\n");
    fprintf(fid,
         "volatile RPC_SS_THREADS_CANCEL_STATE_T IDL_general_cancel_state=RPC_SS_THREADS_CANCEL_STATE_T_INITIALIZER;\n");
    fprintf(fid, "idl_byte IDL_stack_packet[IDL_STACK_PACKET_SIZE];\n");

    if (AST_HAS_IN_PIPES_SET(p_operation)
    || AST_HAS_OUT_PIPES_SET(p_operation))
    {
        fprintf(fid, "long IDL_current_pipe = 0;\n");
    }

    DDBE_spell_param_vec_def( fid, p_operation, BE_server_side,
                              BE_cmd_opt, BE_cmd_val );
    DDBE_spell_stack_surrogates( fid,
                                 &fixed_char_array_list,
                                 p_operation );

    /* Does operation use I-char machinery? If so, declare needed variables */
    BE_cs_analyze_and_spell_vars(fid, p_operation, BE_server_side, &cs_info);

    if (AST_HAS_IN_PTRS_SET(p_operation)
    || AST_HAS_OUT_PTRS_SET(p_operation)
    || AST_ENABLE_ALLOCATE_SET(p_operation))
    {
        fprintf(fid, "rpc_ss_thread_support_ptrs_t IDL_support_ptrs;\n");
    }

    /* If there is a function result, we need somewhere to put it */
    if (p_operation->result->type->kind != AST_void_k)
    {
        if ( AST_CONTEXT_SET(p_operation->result) )
        {
            /* Declare stack workspace for the wire form of context handle
                    function result */
            fprintf(fid, "IDL_ee_context_t IDL_function_result;\n");
        }
        else
        {
            CSPELL_typed_name(fid, p_operation->result->type,
                            NAMETABLE_add_id("IDL_function_result"),
                            NULL, false, true, false);
            fprintf(fid, ";\n");
        }
    }

    if ( rep_as_handle_param != BE_no_rep_as_handle_t_k )
    {
        fprintf( fid, "%s %s;\n",
                      BE_get_name(rep_as_type_name), rep_as_handle_name );
    }

    if (AST_HAS_IN_CTX_SET(p_operation)
        || AST_HAS_OUT_CTX_SET(p_operation))
    {
        fprintf(fid, "rpc_client_handle_t IDL_client_id=NULL;\n");
    }

    /*
     *  Start of executable code
     */
    fprintf(fid, "RPC_SS_INIT_SERVER\n");
    fprintf(fid, "rpc_ss_init_marsh_state(IDL_type_vec, &IDL_ms);\n");

    /* Centeris: set memory management callbacks if in midl mode */
    if (midl_mode)
    {
        fprintf(fid, "IDL_ms.IDL_mem_handle.alloc = midl_user_allocate;\n");
        fprintf(fid, "IDL_ms.IDL_mem_handle.free = midl_user_free;\n");
    }

    fprintf(fid,
             "IDL_ms.IDL_stack_packet_status = IDL_stack_packet_unused_k;\n");
    fprintf(fid, "IDL_ms.IDL_stack_packet_addr = IDL_stack_packet;\n");
    fprintf(fid, "DCETHREAD_TRY\n");
    fprintf(fid, "IDL_ms.IDL_offset_vec = IDL_offset_vec;\n");
    fprintf(fid, "IDL_ms.IDL_rtn_vec = IDL_rtn_vec;\n");
    fprintf(fid, "IDL_ms.IDL_call_h = (volatile rpc_call_handle_t)IDL_call_h;\n");
    fprintf(fid, "IDL_ms.IDL_drep = *IDL_drep_p;\n");

    fprintf(fid, "IDL_ms.IDL_elt_p = IDL_elt_p;\n");
    DDBE_spell_param_vec_init( fid, p_operation, BE_server_side,
                               BE_cmd_opt, BE_cmd_val );
    fprintf(fid, "IDL_ms.IDL_param_vec = IDL_param_vec;\n");
    fprintf(fid, "IDL_ms.IDL_side = IDL_server_side_k;\n");
    fprintf(fid, "IDL_ms.IDL_language = ");
        fprintf(fid, "IDL_lang_c_k");
    fprintf(fid, ";\n");

    /* Does operation use I-char machinery? If so, set up needed state */
    BE_spell_cs_state(fid, "IDL_ms.", BE_server_side, &cs_info);
    if (cs_info.cs_machinery)
        fprintf(fid, "IDL_ms.IDL_h=%s;\n", handle_info.assoc_name);


    /* If there are user exceptions which are not external, initialize them */
    if (num_declared_exceptions != 0)
    {
        fprintf(fid,
             "RPC_SS_THREADS_ONCE(&IDL_exception_once,IDL_exceptions_init);\n");
    }

    /*
     *  Is there a reference from this client to a context?
     */
    if (AST_HAS_IN_CTX_SET(p_operation)
        || AST_HAS_OUT_CTX_SET(p_operation))
    {
        fprintf(fid,
"rpc_ss_ctx_client_ref_count_i_2(%s,&IDL_client_id,(error_status_t*)&IDL_ms.IDL_status);\n",
              handle_info.assoc_name);
        CSPELL_test_status(fid);
    }

    /*
     * Node initializations
     */
    if (AST_HAS_IN_PTRS_SET(p_operation)
    || AST_HAS_OUT_PTRS_SET(p_operation)
    || AST_ENABLE_ALLOCATE_SET(p_operation))
    {
        fprintf( fid,
"rpc_ss_create_support_ptrs( &IDL_support_ptrs,&IDL_ms.IDL_mem_handle);\n" );
        if (AST_HAS_FULL_PTRS_SET(p_operation))
        {
            fprintf(fid,
"rpc_ss_init_node_table(&IDL_ms.IDL_node_table,&IDL_ms.IDL_mem_handle);\n");
            if (AST_REFLECT_DELETIONS_SET(p_operation))
            {
                fprintf(fid,
                     "rpc_ss_enable_reflect_deletes(IDL_ms.IDL_node_table);\n");
            }
        }
    }

    /*
     * Pipe initializations
     */
    if (AST_HAS_IN_PIPES_SET(p_operation) || AST_HAS_OUT_PIPES_SET(p_operation))
    {
        DDBE_init_server_pipes( fid, p_operation, &first_pipe );
        fprintf( fid, "IDL_current_pipe=(%ld);\n", first_pipe );
    }

    /* Unmarshall the ins */
        DDBE_spell_marsh_or_unmar( fid, p_operation, "rpc_ss_ndr_unmar_interp",
                                "&IDL_ms", BE_server_side, BE_unmarshalling_k );

    /* If there is I-char machinery,  call the [cs_tag_rtn] if there is one */
    BE_spell_cs_tag_rtn_call(fid, "IDL_ms.", p_operation, BE_server_side,
                             &handle_info, &cs_info, false);

    CSPELL_manager_call(fid, p_interface, p_operation,
                    rep_as_handle_param, rep_as_type_name, binding_handle_name);

    DDBE_convert_out_contexts(fid, p_operation, binding_handle_name);

    if (AST_HAS_IN_PIPES_SET(p_operation) || AST_HAS_OUT_PIPES_SET(p_operation))
    {
        fprintf( fid, "if (IDL_current_pipe != %d)\n", BE_FINISHED_WITH_PIPES );
        fprintf( fid, "{\n" );
        fprintf( fid, "DCETHREAD_RAISE(rpc_x_ss_pipe_discipline_error);\n" );
        fprintf( fid, "}\n" );
    }

    /* Marshall the outs */
        fprintf(fid, "{\n");
        DDBE_spell_marsh_or_unmar( fid, p_operation, "rpc_ss_ndr_marsh_interp",
                                "&IDL_ms", BE_server_side, BE_marshalling_k );
        fprintf(fid, "if (IDL_ms.IDL_iovec.num_elt != 0)\n");
        fprintf(fid,
"  rpc_call_transmit((rpc_call_handle_t)IDL_ms.IDL_call_h,(rpc_iovector_p_t)&IDL_ms.IDL_iovec,\n");
        fprintf(fid,
            "  (unsigned32*)&IDL_ms.IDL_status);  /* Send remaining outs */\n");
        fprintf(fid, "\n}");

    fprintf(fid, "\nIDL_closedown: __IDL_UNUSED_LABEL__;\n");
    fprintf(fid, "DCETHREAD_CATCH_ALL(THIS_CATCH)\n");

    fprintf(fid, "if ( IDL_manager_entered )\n{\n");
    fprintf(fid,
              "RPC_SS_THREADS_RESTORE_GENERAL(IDL_general_cancel_state);\n");
    fprintf(fid, "RPC_SS_THREADS_RESTORE_ASYNC(IDL_async_cancel_state);\n");
    fprintf(fid, "}\n");

    /*
     * For all exceptions other than report status, send the exception to the
     * client.  For the report status exception, just fall through and
     * perform the normal failing status reporting.
     */
        fprintf(fid, "rpc_ss_ndr_clean_up(&IDL_ms);\n"); 
    fprintf(fid,
         "if (!RPC_SS_EXC_MATCHES(THIS_CATCH,&rpc_x_ss_pipe_comm_error))\n{\n");
    fprintf(fid, "if ( ! IDL_manager_entered )\n{\n");
    if ( ! explicit_binding )
    {
        fprintf(fid, "IDL_ms.IDL_status = rpc_s_manager_not_entered;\n");
    }
    fprintf(fid, "}\n");
    if ( ! explicit_binding )
    {
        fprintf(fid, "else\n");
    }
    fprintf(fid,"{\n");
        fprintf(fid,"rpc_ss_send_server_exception_2(IDL_call_h,THIS_CATCH,%d,%s,&IDL_ms);\n",
            num_declared_exceptions + num_extern_exceptions,
            (num_declared_exceptions + num_extern_exceptions) ? "IDL_exception_addresses" : "NULL"
	    );
    fprintf(fid, "IDL_ms.IDL_status = error_status_ok;\n}\n");
    fprintf(fid, "}\n");
    fprintf(fid, "DCETHREAD_ENDTRY\n");

    if (AST_HAS_IN_CTX_SET(p_operation)
        || AST_HAS_OUT_CTX_SET(p_operation))
    {
        fprintf(fid,
             "rpc_ss_ctx_client_ref_count_d_2(%s, IDL_client_id);\n",
              handle_info.assoc_name);
    }

    if (AST_HAS_IN_PTRS_SET(p_operation)
    || AST_HAS_OUT_PTRS_SET(p_operation)
    || AST_ENABLE_ALLOCATE_SET(p_operation))
    {
        fprintf( fid, "rpc_ss_destroy_support_ptrs();\n" );
    }
    fprintf(fid, "if (IDL_ms.IDL_mem_handle.memory)\n{\n");
    fprintf(fid, " rpc_ss_mem_free(&IDL_ms.IDL_mem_handle);\n}\n");
    fprintf(fid, "if (IDL_ms.IDL_status != error_status_ok)\n{\n");
    fprintf(fid, "if (IDL_ms.IDL_status == rpc_s_call_cancelled)\n{\n");
    fprintf(fid,   "rpc_ss_send_server_exception(");
    fprintf(fid,      "IDL_call_h,&RPC_SS_THREADS_X_CANCELLED);\n");
    fprintf(fid,   "IDL_ms.IDL_status = error_status_ok;\n");
    fprintf(fid,  "}\nelse\n{\n");
    if ( ! explicit_binding )
    {
        fprintf(fid, "if (IDL_manager_entered)\n");
    }
    fprintf(fid,   "{\nrpc_ss_send_server_exception(");
    fprintf(fid,      "IDL_call_h,&rpc_x_ss_remote_comm_failure);\n");
    fprintf(fid,   "IDL_ms.IDL_status = error_status_ok;\n");
    fprintf(fid, "}\n}\n}\n");

    /* When reached here IDL_ms.IDL_status is either error_status_ok
     * or rpc_s_manager_not_entered
     */
    fprintf(fid, "*IDL_status_p = IDL_ms.IDL_status;\n");


    fprintf(fid, "}\n");
}

/*
 * BE_gen_sstub
 *
 * Public entry point for server stub file generation
 */
void BE_gen_sstub
(
    FILE *fid,              /* Handle for emitted C text */
    AST_interface_n_t *p_interface,     /* Ptr to AST interface node */
    language_k_t language,  /* Language stub is to interface to */
    char header_name[],     /* Name of header file to be included in stub */
    boolean *cmd_opt,
    void **cmd_val,
    DDBE_vectors_t *dd_vip    /* Data driven BE vector information ptr */
)
{
    AST_export_n_t *p_export;
    AST_operation_n_t *p_operation;
    boolean first;
    /* Exceptions may be declared or external. We need a count of both */
    int num_declared_exceptions;
    int num_extern_exceptions;

    /*
     * Emit a #pragma nostandard to suppress warnings on non-standard C usage
     */
    fprintf(fid, "#ifdef VMS\n#pragma nostandard\n#endif\n");

    /*
     * Emit #defines and #includes
     */
    CSPELL_mts_includes(fid, header_name);

    /*
     *  Set up interpreter data structures
     */
    DDBE_spell_offset_vec( fid, dd_vip, cmd_opt, cmd_val );
    DDBE_spell_rtn_vec( fid, dd_vip, cmd_opt, cmd_val, FALSE );
    DDBE_spell_type_vec( fid, dd_vip, cmd_opt, cmd_val );


    /* If there are any user exceptions, emit the necessary declarations */
    DDBE_user_exceptions(fid, p_interface,
                         &num_declared_exceptions, &num_extern_exceptions);

    /*
     *  Emit manager entry point vector, if requested
     */
    if (cmd_opt[opt_mepv])
	CSPELL_manager_epv(fid, p_interface);

    /*
     * Emit operation definitions
     */
    for (p_export = p_interface->exports; p_export; p_export = p_export->next)
        if ((p_export->kind == AST_operation_k)
                && ( ! AST_ENCODE_SET(p_export->thing_p.exported_operation))
                && ( ! AST_DECODE_SET(p_export->thing_p.exported_operation)))
        {
            BE_push_malloc_ctx();
            NAMETABLE_set_temp_name_mode();
            p_operation = p_export->thing_p.exported_operation;
            CSPELL_server_stub_routine(fid, language, p_interface, p_operation,
                                       num_declared_exceptions, num_extern_exceptions, cmd_opt);
            NAMETABLE_clear_temp_name_mode();
            BE_pop_malloc_ctx();
        }

    /*
     * Emit server epv
     */
    fprintf (fid, "\nstatic rpc_v2_server_stub_proc_t IDL_epva[] = \n{\n");
    first = true;
    for (p_export = p_interface->exports; p_export; p_export = p_export->next)
        if (p_export->kind == AST_operation_k)
        {
            if (first)
                first = false;
            else
                fprintf(fid, ",\n");
            if (AST_ENCODE_SET(p_export->thing_p.exported_operation)
                || AST_DECODE_SET(p_export->thing_p.exported_operation))
            {
                fprintf(fid, "NULL");
            }
            else
                fprintf(
                    fid, " (rpc_v2_server_stub_proc_t)op%d_ssr",
                    p_export->thing_p.exported_operation->op_number
                    );
        }
    fprintf (fid, "\n};\n");

    /*
     * Emit static if_spec definition and global exported pointer
     */
    CSPELL_interface_def(fid, p_interface, BE_server_stub_k, cmd_opt[opt_mepv]);

    /*
     * Emit a closing #pragma standard to match the nostandard pragma above
     */
    fprintf(fid, "#ifdef VMS\n#pragma standard\n#endif\n");
}
