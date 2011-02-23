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
**      cstubmts.c
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
#include <command.h>
#include <cspell.h>
#include <cspeldcl.h>
#include <ddbe.h>
#include <ifspec.h>
#include <commstat.h>
#include <clihamts.h>
#include <cstubgen.h>
#include <mtsbacke.h>
#include <cstubmts.h>
#include <user_exc.h>
#include <icharsup.h>
#include <hdgen.h>

BE_handle_info_t BE_handle_info;

static AST_interface_n_t * the_interface = NULL;

/******************************************************************************/
/*                                                                            */
/*    Spell test of status after runtime call                                 */
/*                                                                            */
/******************************************************************************/
void CSPELL_test_status
(
    FILE *fid
)
{
    fprintf(fid,
             "if (IDL_ms.IDL_status != error_status_ok) goto IDL_closedown;\n");
}

/******************************************************************************/
/*                                                                            */
/*    Spell test of status after rpc_call_transceive                          */
/*                                                                            */
/******************************************************************************/
void CSPELL_test_transceive_status
(
    FILE *fid
)
{
    fprintf(fid, "if (IDL_ms.IDL_status != error_status_ok)\n{\n");
    fprintf(fid, "IDL_ms.IDL_elt_p = NULL;\n");
    if ( ( BE_handle_info.handle_type == BE_auto_handle_k )
          && ( ! BE_handle_info.auto_handle_idempotent_op ) )
    {
        fprintf(fid,
"IDL_ms.IDL_restartable=IDL_ms.IDL_restartable&&(!(rpc_call_did_mgr_execute\n");
        fprintf(fid,"  ((rpc_call_handle_t)IDL_ms.IDL_call_h,&IDL_st2)));\n");
    }
    fprintf(fid, "goto IDL_closedown;\n}\n");
}

/******************************************************************************/
/*                                                                            */
/*    Spell client stub routine header                                        */
/*                                                                            */
/******************************************************************************/
void CSPELL_csr_header
(
    FILE *fid,
    char const *p_interface_name,       /* Ptr to name of interface */
    AST_operation_n_t *p_operation, /* Ptr to operation node */
    boolean use_internal_name       /* use internal name if true */
)
{
	char op_internal_name[3 * MAX_ID];
	NAMETABLE_id_t emitted_name;
	AST_parameter_n_t * handle_param = NULL;
	
	if (use_internal_name) {
		sprintf(op_internal_name, "op%d_csr", p_operation->op_number);
		emitted_name = NAMETABLE_add_id(op_internal_name);
		fprintf(fid, "\nstatic ");
	}
	else if (AST_OBJECT_SET(the_interface))	{
		sprintf(op_internal_name, "%sProxy::%s", p_interface_name,
				BE_get_name(p_operation->name));
		emitted_name = NAMETABLE_add_id(op_internal_name);

		/* Skip handle params */
		if (BE_is_handle_param(p_operation->parameters))	{
			handle_param = p_operation->parameters;
			p_operation->parameters = p_operation->parameters->next;
		}
	} else {
            sprintf(op_internal_name, "%s%s", cstub_pref, BE_get_name(p_operation->name));
            emitted_name = NAMETABLE_add_id(op_internal_name);
            fprintf (fid, "\n");
	}

	CSPELL_function_def_header (fid, p_operation, emitted_name);

	/* restore skipped handle params */
	if (handle_param)
		p_operation->parameters->next = handle_param;
	
}


/******************************************************************************/
/*                                                                            */
/*    Generation of a stub for a client operation call                        */
/*                                                                            */
/******************************************************************************/
static void CSPELL_client_stub_routine
(
    FILE *fid,                      /* Handle for emitted C text */
    AST_interface_n_t *p_interface, /* Ptr to AST interface node */
    language_k_t language ATTRIBUTE_UNUSED,          /* Language stub is to interface to */
    AST_operation_n_t *p_operation, /* Ptr to AST operation node */
    char const *p_interface_name,   /* Ptr to name of interface */
    unsigned long op_num,           /* Number of current operation */
    boolean *cmd_opt,               /* Command line options */
    int num_declared_exceptions,    /* Count of user declared exceptions */
    int num_extern_exceptions       /* Count of user extern_exceptions */
)
{
    BE_stat_info_t comm_stat_info;
    BE_stat_info_t fault_stat_info;
    BE_cs_info_t cs_info;           /* I-char machinery description */
    boolean use_internal_name = cmd_opt[opt_cepv];
    boolean midl_mode = cmd_opt[opt_midl];

    /* What sort of status reporting? */
    BE_get_comm_stat_info( p_operation, &comm_stat_info );
    BE_get_fault_stat_info( p_operation, &fault_stat_info );

	 
    /* Routine header */
    CSPELL_csr_header(fid, p_interface_name, p_operation,
        use_internal_name);

    fprintf (fid, "{\n");

	 if (AST_OBJECT_SET(p_interface))	{
		 /* If we skipped a handle param, add it as a local var */
		 if (BE_is_handle_param(p_operation->parameters))
			 CSPELL_var_decl(fid, p_operation->parameters->type,
					 p_operation->parameters->name);

	 }

    /*
     * Analyze the association handle the call is being made on;
     * declare assoc_handle if necessary
     */
    BE_setup_client_handle (fid, p_interface, p_operation, &BE_handle_info);

    /*
     * Does operation use I-char machinery? If so, declare any needed variables
     */
    BE_cs_analyze_and_spell_vars(fid, p_operation, BE_client_side, &cs_info);

    /*
     * Standard local variables
     */
    fprintf(fid, "rpc_transfer_syntax_t\t\tIDL_transfer_syntax;\n");
    fprintf(fid, "rpc_iovector_elt_t\t\tIDL_outs;\n");
    fprintf(fid, "volatile ndr_ulong_int\t\tIDL_fault_code=error_status_ok;\n");
    fprintf(fid, "volatile ndr_ulong_int\t\tIDL_user_fault_id=0;\n");
    fprintf(fid,
           "volatile RPC_SS_THREADS_CANCEL_STATE_T IDL_async_cancel_state;\n");
    fprintf(fid, "IDL_ms_t\t\tIDL_ms;\n");
    fprintf(fid, "idl_byte\t\tIDL_stack_packet[IDL_STACK_PACKET_SIZE];\n");

    DDBE_spell_param_vec_def( fid, p_operation, BE_client_side,
                              BE_cmd_opt, BE_cmd_val );

    /* If there is a function result, we need somewhere to put it */
    if ( (p_operation->result->type->kind != AST_void_k)
       )
    {
        CSPELL_typed_name(fid, p_operation->result->type,
                            NAMETABLE_add_id("IDL_function_result"),
                            NULL, false, true, false);
        fprintf(fid, ";\n");
    }

    /*
     * Start of executable code
     */
    fprintf(fid, "\nRPC_SS_INIT_CLIENT\n");
    fprintf(fid, "RPC_SS_THREADS_DISABLE_ASYNC(IDL_async_cancel_state);\n");
    if ( BE_handle_info.handle_type == BE_auto_handle_k )
    {
        fprintf( fid, "IDL_ms.IDL_restartable=idl_true;\n" );
        fprintf(fid,
     "RPC_SS_THREADS_ONCE(&IDL_interface_client_once,IDL_auto_handle_init);\n");
    }
    else
    {
	/* 
	 *  To support those platforms which do not allow both a CATCH and
	 *  FINALLY clause on the same TRY, we generate one TRY block with a
	 *  catch clause nested inside another TRY block with the FINALLY
	 *  clause.  We conditionally generate the extra TRY statement only for
	 *  non-auto handle case because for auto-handle we already have an
	 *  nest TRY block.
	 */
#ifdef NO_TRY_CATCH_FINALLY
	fprintf(fid, "DCETHREAD_TRY\n");
#endif
    }

    /* If there are user exceptions which are not external, initialize them */
    if (num_declared_exceptions != 0)
    {
        fprintf(fid,
             "RPC_SS_THREADS_ONCE(&IDL_exception_once,IDL_exceptions_init);\n");
    }

    fprintf(fid, "rpc_ss_init_marsh_state(IDL_type_vec, &IDL_ms);\n");

    /* Centeris: set alloc/free function pointers if in MIDL mode */

    if (midl_mode)
    {
        fprintf(fid, "IDL_ms.IDL_mem_handle.alloc = midl_user_allocate;\n");
        fprintf(fid, "IDL_ms.IDL_mem_handle.free = midl_user_free;\n");
        fprintf(fid, "rpc_ss_set_client_alloc_free(midl_user_allocate, midl_user_free);\n");
    }

    fprintf(fid,
             "IDL_ms.IDL_stack_packet_status = IDL_stack_packet_unused_k;\n");
    fprintf(fid, "IDL_ms.IDL_stack_packet_addr = IDL_stack_packet;\n");
    fprintf(fid, "DCETHREAD_TRY\n");
    fprintf(fid, "IDL_ms.IDL_call_h = 0;\n");
    fprintf(fid, "IDL_ms.IDL_elt_p = NULL;\n");
    fprintf(fid, "IDL_ms.IDL_offset_vec = IDL_offset_vec;\n");
    fprintf(fid, "IDL_ms.IDL_rtn_vec = IDL_rtn_vec;\n");
    DDBE_spell_param_vec_init( fid, p_operation, BE_client_side,
                               BE_cmd_opt, BE_cmd_val );
    fprintf(fid, "IDL_ms.IDL_param_vec = IDL_param_vec;\n");
    fprintf(fid, "IDL_ms.IDL_side = IDL_client_side_k;\n");
    fprintf(fid, "IDL_ms.IDL_language = ");
        fprintf(fid, "IDL_lang_c_k");
    fprintf(fid, ";\n");



    if (AST_HAS_FULL_PTRS_SET(p_operation) &&
        (AST_HAS_IN_PTRS_SET(p_operation) || AST_HAS_OUT_PTRS_SET(p_operation)))
    {
        fprintf(fid,
"rpc_ss_init_node_table(&IDL_ms.IDL_node_table,&IDL_ms.IDL_mem_handle);\n");
    }

    if (AST_HAS_OUT_PTRS_SET(p_operation) )
    {
        /* The following call is done here to enable PRT testing */
        fprintf(fid, "rpc_ss_mts_client_estab_alloc(&IDL_ms);\n");
    }

    /* Does operation use I-char machinery? If so, set up needed state */
    BE_spell_cs_state(fid, "IDL_ms.", BE_client_side, &cs_info);
    /*          And call the [cs_tag_rtn] if there is one */
    BE_spell_cs_tag_rtn_call(fid, "IDL_ms.", p_operation, BE_client_side,
                             &BE_handle_info, &cs_info, false);

/* WEZ:setup the handle here ? */
	 

    CSPELL_call_start(fid, &BE_handle_info, p_interface, p_operation, op_num,
                        &comm_stat_info, &fault_stat_info);

    if (AST_HAS_IN_CTX_SET(p_operation)
        || AST_HAS_OUT_CTX_SET(p_operation)
        || cs_info.cs_machinery)
    {
        fprintf(fid, "IDL_ms.IDL_h=(handle_t)%c%s;\n",
                     BE_handle_info.deref_assoc, BE_handle_info.assoc_name);
    }

    /* Marshall the ins */
        DDBE_spell_marsh_or_unmar( fid, p_operation, "rpc_ss_ndr_marsh_interp",
                                "&IDL_ms", BE_client_side, BE_marshalling_k );

    fprintf(fid,"IDL_ms.IDL_elt_p = &IDL_outs;\n");
    fprintf(fid,
"rpc_call_transceive((rpc_call_handle_t)IDL_ms.IDL_call_h,(rpc_iovector_p_t)&IDL_ms.IDL_iovec,\n");
        fprintf(fid,
"  IDL_ms.IDL_elt_p,&IDL_ms.IDL_drep,(unsigned32*)&IDL_ms.IDL_status);\n");

    /* !!WORKAROUND!! for problem with buff_dealloc for [maybe] operation
                      with CN runtime */
    if (AST_MAYBE_SET(p_operation))
        fprintf(fid, "IDL_outs.buff_dealloc=NULL;\n");

    CSPELL_test_transceive_status(fid);


    /* Unmarshall the outs */
        DDBE_spell_marsh_or_unmar( fid, p_operation, "rpc_ss_ndr_unmar_interp",
                                "&IDL_ms", BE_client_side, BE_unmarshalling_k );

    fprintf(fid, "IDL_closedown: __IDL_UNUSED_LABEL__;\n");

    /*
     * Catch the error that indicates that st has be set to a failing status
     * that should be reported, and then do normal cleanup processing.  If
     * for some reason the status is not set, then set it.
     */
    fprintf(fid, "DCETHREAD_CATCH(rpc_x_ss_pipe_comm_error)\n");
    if ( BE_handle_info.handle_type == BE_auto_handle_k )
    {
        DDBE_spell_restart_logic( fid, p_operation );

        /*
         * This label is used when no valid auto-handle binding can be
         * found.  The call to this is generated in CSPELL_bind_auto_handle
         * when no valid binding can be found.
         */
        fprintf(fid, "IDL_auto_binding_failure:;\n");
    }
    else {
	/* 
	 *  Add the matching ENDTRY for the nested TRY/CATCH block, if
	 *  necessary, as decribed above.
	 */
#ifdef NO_TRY_CATCH_FINALLY
	fprintf(fid, "DCETHREAD_ENDTRY\n");
#endif
    }

    /*
     * Normal cleanup processing to free up resources and end the call and
     * and report any faults or failing statuses.
     */
    fprintf(fid, "DCETHREAD_FINALLY\n");
        fprintf(fid, "rpc_ss_ndr_clean_up(&IDL_ms);\n");

    /* End the call, but only if we have one to end for auto handle */
    if ( BE_handle_info.handle_type == BE_auto_handle_k )
        fprintf(fid, "if(IDL_ms.IDL_call_h!=NULL)");
    fprintf(fid,
"rpc_ss_call_end_2(&IDL_ms.IDL_call_h,&IDL_fault_code,&IDL_user_fault_id,&IDL_ms.IDL_status);\n");
    CSPELL_binding_free_if_needed( fid, &BE_handle_info );

    /* Must free user binding after ending the call */
    if ((BE_handle_info.handle_type == BE_parm_user_handle_k)
        || (BE_handle_info.handle_type == BE_impl_user_handle_k))
    {
        /* There is a user handle to unbind. As we are inside an exception
           handler, we don't want any exception the unbind causes */
        fprintf (fid, "DCETHREAD_TRY\n");
        fprintf (fid, "%s_unbind(%c%s%s, (handle_t)IDL_assoc_handle);\n",
            BE_handle_info.type_name, BE_handle_info.deref_generic,
            BE_handle_info.user_handle_name,
                                            "");
        fprintf (fid, "DCETHREAD_FINALLY\n");
    }

    /* If [represent_as] on handle_t parameter, release the handle_t */
    if ((BE_handle_info.handle_type == BE_rep_as_handle_t_k)
        || (BE_handle_info.handle_type == BE_rep_as_handle_t_p_k))
    {
        fprintf (fid, "DCETHREAD_TRY\n");
        fprintf( fid, "%s_free_inst((handle_t *)%s);\n",
                    BE_get_name(BE_handle_info.rep_as_type),
                    assoc_handle_ptr );
        fprintf (fid, "DCETHREAD_FINALLY\n");
    }

    /* Release memory allocated by stub code */
    fprintf(fid, "if (IDL_ms.IDL_mem_handle.memory)\n{\n");
    fprintf(fid, " rpc_ss_mem_free(&IDL_ms.IDL_mem_handle);\n}\n");

    /* Give status information to client, or raise the appropriate exception */
    CSPELL_return_status( fid, &comm_stat_info, &fault_stat_info,
        "IDL_ms.IDL_status",
        ( (comm_stat_info.type == BE_stat_result_k)
            || (fault_stat_info.type == BE_stat_result_k) )
                ? "IDL_function_result" : (char *)NULL,
        num_declared_exceptions + num_extern_exceptions, "&IDL_ms" );

    fprintf(fid, "RPC_SS_THREADS_RESTORE_ASYNC(IDL_async_cancel_state);\n");
    if ((BE_handle_info.handle_type == BE_parm_user_handle_k)
        || (BE_handle_info.handle_type == BE_impl_user_handle_k)
        || (BE_handle_info.handle_type == BE_rep_as_handle_t_k)
        || (BE_handle_info.handle_type == BE_rep_as_handle_t_p_k))
    {
        fprintf(fid, "DCETHREAD_ENDTRY\n");
    }
    fprintf(fid, "DCETHREAD_ENDTRY\n");


    /* Set the return value */
    if ( (p_operation->result->type->kind != AST_void_k)
       )
        fprintf(fid, "return IDL_function_result;\n");
    fprintf (fid, "}\n");
/* WEZ:we could spell out c -> c++ mappings here */
}

/******************************************************************************/
/*                                                                            */
/*    Stub for an operation with [encode] or [decode] attribute               */
/*                                                                            */
/******************************************************************************/
void DDBE_spell_pickling_stub
(
    FILE *fid,
    AST_interface_n_t *p_interface, /* Ptr to AST interface node */
    char const *p_interface_name,   /* Ptr to name of interface */
    AST_operation_n_t *p_operation, /* Ptr to operation node */
    boolean use_internal_name       /* use internal name if true */
)
{
    boolean encode_decode;  /* True if operation has [encode] and [decode] */
    char *action_type;
    BE_stat_info_t comm_stat_info;
    BE_stat_info_t fault_stat_info;
    BE_cs_info_t cs_info;           /* I-char machinery description */

    /* What sort of status reporting? */
    BE_get_comm_stat_info( p_operation, &comm_stat_info );
    BE_get_fault_stat_info( p_operation, &fault_stat_info );


    BE_setup_client_handle (fid, p_interface, p_operation, &BE_handle_info);
    encode_decode = (AST_ENCODE_SET(p_operation) 
                                            && AST_DECODE_SET(p_operation));

    CSPELL_csr_header(fid, p_interface_name, p_operation, use_internal_name);

    fprintf(fid, "{\n");
    /*
     * Standard local variables
     */
    fprintf(fid, "volatile ndr_ulong_int IDL_fault_code=error_status_ok;\n");
    fprintf(fid, "volatile ndr_ulong_int IDL_user_fault_id=0;\n");
    fprintf(fid,
           "volatile RPC_SS_THREADS_CANCEL_STATE_T IDL_async_cancel_state;\n");
    fprintf(fid, "IDL_es_state_t *IDL_es_state_p;\n");
    fprintf(fid, "volatile IDL_ms_t *IDL_msp;\n");
    fprintf(fid, "idl_es_transfer_syntax_t IDL_es_transfer_syntax;\n");
    DDBE_spell_param_vec_def( fid, p_operation, BE_client_side,
                              BE_cmd_opt, BE_cmd_val );

    /* If there is a function result, we need somewhere to put it */
    if ( (p_operation->result->type->kind != AST_void_k)
       )
    {
        CSPELL_typed_name(fid, p_operation->result->type,
                            NAMETABLE_add_id("IDL_function_result"),
                            NULL, false, true, false);
        fprintf(fid, ";\n");
    }

    /*
     * Does operation use I-char machinery? If so, declare any needed variables
     */
    BE_cs_analyze_and_spell_vars(fid, p_operation, BE_client_side, &cs_info);


    /*
     * Start of executable code
     */
    fprintf(fid, "RPC_SS_INIT_CLIENT\n");
    fprintf(fid, "RPC_SS_THREADS_DISABLE_ASYNC(IDL_async_cancel_state);\n");
    fprintf(fid, "IDL_es_state_p = (IDL_es_state_t *)%c%s;\n",
                BE_handle_info.deref_assoc, BE_handle_info.assoc_name);
    fprintf(fid, "IDL_msp = (volatile IDL_ms_t *)IDL_es_state_p->IDL_msp;\n");
    fprintf(fid, "IDL_msp->IDL_offset_vec = IDL_offset_vec;\n");
    fprintf(fid, "IDL_msp->IDL_rtn_vec = IDL_rtn_vec;\n");
    fprintf(fid, "DCETHREAD_TRY\n");
    DDBE_spell_param_vec_init( fid, p_operation, BE_client_side,
                               BE_cmd_opt, BE_cmd_val );
    fprintf(fid, "IDL_msp->IDL_param_vec = IDL_param_vec;\n");
    fprintf(fid, "IDL_msp->IDL_side = IDL_client_side_k;\n");
    fprintf(fid, "IDL_msp->IDL_language = ");
        fprintf(fid, "IDL_lang_c_k");
    fprintf(fid, ";\n");

    if (AST_HAS_FULL_PTRS_SET(p_operation) &&
        (AST_HAS_IN_PTRS_SET(p_operation) || AST_HAS_OUT_PTRS_SET(p_operation)))
    {
        fprintf(fid,
"rpc_ss_init_node_table(&IDL_msp->IDL_node_table,&IDL_msp->IDL_mem_handle);\n");
    }

    if (AST_HAS_OUT_PTRS_SET(p_operation) )
    {
        fprintf(fid, "rpc_ss_mts_client_estab_alloc(IDL_msp);\n");
    }

    /* Does operation use I-char machinery? If so, set up needed state */
    BE_spell_cs_state(fid, "IDL_msp->", BE_client_side, &cs_info);
    if (cs_info.cs_machinery)
        fprintf(fid, "IDL_msp->IDL_h=NULL;\n");

    if (encode_decode)
        action_type = "IDL_both_k";
    else if (AST_ENCODE_SET(p_operation))
        action_type = "IDL_encoding_k";
    else
        action_type = "IDL_decoding_k";
    fprintf(fid, 
"idl_es_before_interp_call(%c%s,(rpc_if_handle_t)&IDL_ifspec,\n",
                BE_handle_info.deref_assoc, BE_handle_info.assoc_name);
    fprintf(fid, 
"  IDL_type_vec,%d,%s,&IDL_es_transfer_syntax,(IDL_msp_t)IDL_msp);\n",
                p_operation->op_number, action_type);

    /* If there is I-char machinery,  call the [cs_tag_rtn] if there is one */
    BE_spell_cs_tag_rtn_call(fid, "IDL_msp->", p_operation, BE_client_side,
                             &BE_handle_info, &cs_info, true);

        if (encode_decode)
            fprintf(fid, 
                    "if (IDL_es_state_p->IDL_action == IDL_encoding_k)\n{\n");
        if (AST_ENCODE_SET(p_operation))
            DDBE_spell_marsh_or_unmar( fid, p_operation,
                                       "rpc_ss_ndr_marsh_interp",
                                       "(IDL_msp_t)IDL_msp",
                                       BE_client_side, BE_marshalling_k );
        if (encode_decode)
            fprintf(fid, "}\nelse\n{\n");
        if (AST_DECODE_SET(p_operation))
            DDBE_spell_marsh_or_unmar( fid, p_operation,
                                       "rpc_ss_ndr_unmar_interp",
                                       "(IDL_msp_t)IDL_msp",
                                       BE_client_side, BE_unmarshalling_k );
        if (encode_decode)
            fprintf(fid, "}\n");
    fprintf(fid, "idl_es_after_interp_call((IDL_msp_t)IDL_msp);\n");
    fprintf(fid, "DCETHREAD_CATCH(rpc_x_ss_pipe_comm_error)\n");
    fprintf(fid, "DCETHREAD_FINALLY\n");

    /* Clean-up code */
    fprintf(fid, "idl_es_clean_up((IDL_msp_t)IDL_msp);\n");

    /* Give status information to client, or raise the appropriate exception */
    CSPELL_return_status( fid, &comm_stat_info, &fault_stat_info,
        "IDL_msp->IDL_status",
        ( (comm_stat_info.type == BE_stat_result_k)
            || (fault_stat_info.type == BE_stat_result_k) )
                ? "IDL_function_result" : (char *)NULL,
        0, "(IDL_msp_t)IDL_msp" );
    fprintf(fid, "RPC_SS_THREADS_RESTORE_ASYNC(IDL_async_cancel_state);\n");

    fprintf(fid, "DCETHREAD_ENDTRY\n");

    /* Set the return value */
    if ( (p_operation->result->type->kind != AST_void_k)
       )
    {
        fprintf(fid, "return IDL_function_result;\n");
    }
    fprintf (fid, "}\n");
}

/******************************************************************************/
/*                                                                            */
/*    Main control flow for generating a client stub                          */
/*                                                                            */
/******************************************************************************/
void DDBE_gen_cstub
(
    FILE *fid,                      /* Handle for emitted C text */
    AST_interface_n_t *p_interface, /* Ptr to AST interface node */
    language_k_t language,          /* Language stub is to interface to */
    char header_name[],         /* Name of header file to be included in stub */
    boolean *cmd_opt,
    void **cmd_val,
    DDBE_vectors_t *dd_vip    /* Data driven BE vector information ptr */
)
{
    AST_export_n_t *p_export;
    AST_operation_n_t *p_operation;
    char const *p_interface_name;
    boolean first;
    /* Exceptions may be declared or external. We need a count of both */
    int num_declared_exceptions;
    int num_extern_exceptions;

	the_interface = p_interface;
    NAMETABLE_id_to_string(p_interface->name, &p_interface_name);
    /*
     * Emit a #pragma nostandard to suppress warnings on non-standard C usage
     */
    fprintf(fid, "#ifdef VMS\n#pragma nostandard\n#endif\n");

    /*
     * Emit #defines and #includes
     */
    CSPELL_mts_includes(fid, header_name);

    /*
     * Emit if_spec definition
     */
    CSPELL_interface_def(fid, p_interface, BE_client_stub_k, false);

    /* If necessary, emit statics needed for [auto_handle] */
    if ( AST_AUTO_HANDLE_SET(p_interface) || AST_OBJECT_SET(p_interface))
    {
        CSPELL_auto_handle_statics( fid );
    }

	 /* Declare the Proxy class for ORPC */
	 if (AST_OBJECT_SET(p_interface))	{
		 BE_gen_orpc_defs(fid, p_interface, proxy_def);
	 }

    /* If there is an implicit handle, declare it */
    if (p_interface->implicit_handle_name != NAMETABLE_NIL_ID)
    {
            fprintf( fid, "globaldef " );
        if ( ! AST_IMPLICIT_HANDLE_G_SET(p_interface) )
        {
            fprintf(fid, "handle_t ");
        }
        else
        {
            spell_name( fid, p_interface->implicit_handle_type_name);
            fprintf( fid, " " );
        }
        spell_name( fid, p_interface->implicit_handle_name);
        fprintf( fid, ";\n" );
    }

    /* If there are any user exceptions, emit the necessary declarations */
    DDBE_user_exceptions(fid, p_interface,
                         &num_declared_exceptions, &num_extern_exceptions);

    /*
     *  Set up interpreter data structures
     */
    DDBE_spell_offset_vec( fid, dd_vip, cmd_opt, cmd_val );
    DDBE_spell_rtn_vec( fid, dd_vip, cmd_opt, cmd_val, TRUE );
    DDBE_spell_type_vec( fid, dd_vip, cmd_opt, cmd_val );


    /*
     * Emit operation definitions
     */
    for (p_export = p_interface->exports; p_export; p_export = p_export->next)
    {
        if (p_export->kind == AST_operation_k)
        {
            BE_push_malloc_ctx();
            NAMETABLE_set_temp_name_mode();
            p_operation = p_export->thing_p.exported_operation;
            if (!AST_NO_CODE_SET(p_operation))
            {
                if (AST_ENCODE_SET(p_operation) || AST_DECODE_SET(p_operation))
                    DDBE_spell_pickling_stub(fid, p_interface,
                        p_interface_name, p_operation, cmd_opt[opt_cepv]);
                else
                    CSPELL_client_stub_routine(
                        fid, p_interface, language,
                        p_operation, p_interface_name, p_operation->op_number,
                        /* cmd_opt[opt_cepv], */ cmd_opt, num_declared_exceptions,
                        num_extern_exceptions);
            }
            NAMETABLE_clear_temp_name_mode();
            BE_pop_malloc_ctx();
        }
    }



    if (cmd_opt[opt_cepv]) {
        /*
         * Emit EPV declarations
         */
        fprintf(fid, "/* global */ %s_v%ld_%ld_epv_t %s_v%ld_%ld_c_epv = {\n",
            p_interface_name,
            (p_interface->version%65536), (p_interface->version/65536),
            p_interface_name,
            (p_interface->version%65536), (p_interface->version/65536));

        first = true;

        for (p_export = p_interface->exports; p_export;
                p_export = p_export->next)
            if (p_export->kind == AST_operation_k)
        {
            if (first) first = false;
            else fprintf (fid, ",\n");
            p_operation = p_export->thing_p.exported_operation;
            if (!AST_NO_CODE_SET(p_operation))
            {
                fprintf(fid, " op%d_csr", p_operation->op_number);
            }
            else fprintf(fid, " NULL");
        }
        fprintf (fid, "\n};\n");
    }

    /*
     * Emit a closing #pragma standard to match the nostandard pragma above
     */
    fprintf(fid, "#ifdef VMS\n#pragma standard\n#endif\n");

	 the_interface = NULL;
}
