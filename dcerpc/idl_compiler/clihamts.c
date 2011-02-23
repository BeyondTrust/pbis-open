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
**  NAME:
**
**      clihamts.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Client binding handle stuff for MTS compiler
**
*/

#include <nidl.h>
#include <ast.h>
#include <bedeck.h>
#include <commstat.h>
#include <ifspec.h>
#include <clihamts.h>
#include <dutils.h>
#include <cspell.h>
#include <cstubgen.h>
#include <marshall.h>

static char assoc_handle_name[] = "IDL_assoc_handle";
char assoc_handle_ptr[] = "IDL_assoc_handle_p";

/******************************************************************************/
/*                                                                            */
/*    Spell declaration of assoc_handle                                       */
/*                                                                            */
/******************************************************************************/
static void CSPELL_decl_assoc_handle_vars
(
    FILE *fid,
    BE_handle_info_t *p_handle_info
)
{
    if ( (p_handle_info->handle_type == BE_impl_handle_t_k)
         || (p_handle_info->handle_type == BE_context_handle_k)
         || (p_handle_info->handle_type == BE_auto_handle_k) )
    {
        fprintf( fid, "error_status_t IDL_st2;\n" );
    }
    if ( (p_handle_info->handle_type == BE_rep_as_handle_t_k)
         || (p_handle_info->handle_type == BE_rep_as_handle_t_p_k) )
    {
        fprintf( fid, "volatile handle_t *%s;\n", assoc_handle_ptr );
    }
    else
    {
        fprintf (fid, "volatile handle_t %s;\n", assoc_handle_name);
    }
}

/******************************************************************************/
/*                                                                            */
/*    Analyze the type of handle to be used in the call, and emit any         */
/*    necessary declarations                                                  */
/*                                                                            */
/******************************************************************************/
void BE_setup_client_handle
(
    FILE *fid,
    AST_interface_n_t *p_interface,
    AST_operation_n_t *p_operation,
    BE_handle_info_t *p_handle_info
)
{
    AST_parameter_n_t *p_first_parameter;
    AST_type_n_t *p_type;

    p_handle_info->deref_assoc = ' ';
    p_handle_info->deref_generic = ' ';
    p_first_parameter = p_operation->parameters;
    if (p_first_parameter != NULL)
    {
        p_type = p_first_parameter->type;

        /* Look for [handle] before handle_t to deal correctly with
            [transmit_as(...),handle] handle_t */
        if ( AST_HANDLE_SET(p_type) )
        {
            /* Customized handle by value */
            p_handle_info->handle_type = BE_parm_user_handle_k;
            p_handle_info->assoc_name = assoc_handle_name;
            CSPELL_decl_assoc_handle_vars(fid,p_handle_info);
            NAMETABLE_id_to_string( p_type->name, &(p_handle_info->type_name) );
            NAMETABLE_id_to_string( p_first_parameter->name,
                    &(p_handle_info->user_handle_name) );
            return;
        }
        if ( (p_type->kind == AST_pointer_k)
             && AST_HANDLE_SET(p_type->type_structure.pointer->pointee_type) )
        {
            /* Customized handle by referemce */
            p_handle_info->handle_type = BE_parm_user_handle_k;
            p_handle_info->assoc_name = assoc_handle_name;
            CSPELL_decl_assoc_handle_vars(fid,p_handle_info);
            NAMETABLE_id_to_string(
                          p_type->type_structure.pointer->pointee_type->name,
                          &(p_handle_info->type_name) );
            NAMETABLE_id_to_string( p_first_parameter->name,
                    &(p_handle_info->user_handle_name) );
            p_handle_info->deref_generic = '*';
            return;
        }
        if (p_type->kind == AST_handle_k)
        {
            /* handle_t by value */
            if (p_type->rep_as_type != NULL)
            {
                p_handle_info->handle_type = BE_rep_as_handle_t_k;
                p_handle_info->rep_as_name = p_first_parameter->name; 
                p_handle_info->rep_as_type = p_type->name;
                p_handle_info->assoc_name = assoc_handle_ptr;
                p_handle_info->deref_assoc = '*';
                CSPELL_decl_assoc_handle_vars(fid,p_handle_info);
                return;
            }
            p_handle_info->handle_type = BE_parm_handle_t_k;
            NAMETABLE_id_to_string(
                    p_first_parameter->name,
                    &(p_handle_info->assoc_name) );
            return;
        }
        if ( (p_type->kind == AST_pointer_k)
              && (p_type->type_structure.pointer->pointee_type->kind
                    == AST_handle_k) )
        {
            /* handle_t by reference */
            if (p_type->type_structure.pointer->pointee_type->rep_as_type
                                                                != NULL)
            {
                p_handle_info->handle_type = BE_rep_as_handle_t_p_k;
                p_handle_info->rep_as_name = p_first_parameter->name; 
                p_handle_info->rep_as_type = p_type->type_structure.pointer
                                    ->pointee_type->name;
                p_handle_info->assoc_name = assoc_handle_ptr;
                p_handle_info->deref_assoc = '*';
                CSPELL_decl_assoc_handle_vars(fid,p_handle_info);
                return;
            }
            p_handle_info->handle_type = BE_parm_handle_t_k;
            NAMETABLE_id_to_string(
                    p_first_parameter->name,
                    &(p_handle_info->assoc_name) );
            p_handle_info->deref_assoc = '*';
            return;
        }
        if ( AST_HAS_IN_CTX_SET(p_operation) )
        {
            /* Operation has an [in] or [in, out] context handle */
            p_handle_info->handle_type = BE_context_handle_k;
            p_handle_info->assoc_name = assoc_handle_name;
            CSPELL_decl_assoc_handle_vars(fid,p_handle_info);
            return;
        }
    }
    if (p_interface->implicit_handle_name != NAMETABLE_NIL_ID)
    {
        p_handle_info->assoc_name = assoc_handle_name;
        if ( ! AST_IMPLICIT_HANDLE_G_SET(p_interface) )
        {
            p_handle_info->handle_type = BE_impl_handle_t_k;
            CSPELL_decl_assoc_handle_vars(fid,p_handle_info);
            return;
        }
        CSPELL_decl_assoc_handle_vars(fid,p_handle_info);
        /* Checker ensures we must have a [handle] type here */
        p_handle_info->handle_type = BE_impl_user_handle_k;
        NAMETABLE_id_to_string( p_interface->implicit_handle_type_name,
                               &(p_handle_info->type_name) );
        NAMETABLE_id_to_string( p_interface->implicit_handle_name,
                               &(p_handle_info->user_handle_name) );
        return;
    }
    /* If we get here, must be [auto_handle] */
    p_handle_info->handle_type = BE_auto_handle_k;
    p_handle_info->assoc_name = assoc_handle_name;
    p_handle_info->auto_handle_idempotent_op =
                    ( AST_IDEMPOTENT_SET(p_operation) ) ? true : false;
    CSPELL_decl_assoc_handle_vars(fid,p_handle_info);
    fprintf( fid, "idl_boolean IDL_timeout_was_set_low=idl_false;\n" );
    return;
}

/******************************************************************************/
/*                                                                            */
/*    Spell "duplicate implicit handle_t handle"                              */
/*                                                                            */
/******************************************************************************/
static void CSPELL_dup_implicit_handle_t
(
    FILE *fid,
    AST_interface_n_t *p_interface
)
{
    fprintf(fid,"rpc_binding_handle_copy(");
    spell_name(fid, p_interface->implicit_handle_name);
    fprintf(fid, ",(rpc_binding_handle_t*)&IDL_assoc_handle");
    fprintf(fid, ",(error_status_t*)&IDL_ms.IDL_status);\n" );
    CSPELL_test_status(fid);
}

/******************************************************************************/
/*                                                                            */
/*    Spell "use a generic handle to get a binding"                           */
/*                                                                            */
/******************************************************************************/
static void CSPELL_bind_generic_handle
(
    FILE *fid,
    BE_handle_info_t *p_handle_info
)
{
        fprintf (fid,
                 "IDL_assoc_handle = (volatile handle_t)%s_bind(%c%s%s);\n",
                 p_handle_info->type_name, p_handle_info->deref_generic,
                 p_handle_info->user_handle_name,
                                                 " ");
}

/******************************************************************************/
/*                                                                            */
/*    Spell "use the [auto_handle] mechanism to get a binding"                */
/*                                                                            */
/******************************************************************************/
static void CSPELL_bind_auto_handle
(
    FILE *fid,
    AST_operation_n_t *p_operation ATTRIBUTE_UNUSED,
    BE_stat_info_t *p_comm_stat_info ATTRIBUTE_UNUSED,
    BE_stat_info_t *p_fault_stat_info ATTRIBUTE_UNUSED
)
{
        fprintf (fid,"rpc_ss_make_import_cursor_valid(&IDL_auto_handle_mutex,\n");
        fprintf (fid,    "&IDL_import_cursor,\n" );
        fprintf (fid,
                "   (rpc_if_handle_t)&IDL_ifspec,&IDL_auto_handle_status);\n" );
        fprintf( fid, "if(IDL_auto_handle_status!=error_status_ok)\n" );
        fprintf( fid, "{\n" );
        fprintf( fid, "IDL_ms.IDL_status=IDL_auto_handle_status;\n");
        fprintf( fid, "goto IDL_auto_binding_failure;\n");
        fprintf( fid, "}\n" );
        fprintf (fid, "IDL_find_server:\n");
#ifdef PERFMON
        fprintf (fid, "#ifdef PERFMON\n");
	fprintf (fid, "IDL_TRY_N;\n");
	fprintf (fid, "#endif\n");
#endif
        fprintf (fid, "DCETHREAD_TRY\n");
#ifdef PERFMON
        fprintf (fid, "#ifdef PERFMON\n");
	fprintf (fid, "IDL_TRY_X;\n");
	fprintf (fid, "#endif\n");
#endif
        fprintf (fid,"rpc_ss_import_cursor_advance(&IDL_auto_handle_mutex,\n");
        fprintf (fid,  "&IDL_timeout_was_set_low,&IDL_import_cursor,\n" );
        fprintf (fid,
                   "(rpc_if_handle_t)&IDL_ifspec,&IDL_error_using_binding,\n" );
        fprintf (fid,  "&IDL_interface_binding,(rpc_binding_handle_t*)&IDL_assoc_handle,\n");
        fprintf (fid,  "(error_status_t*)&IDL_auto_handle_status,(error_status_t*)&IDL_ms.IDL_status);\n");
        fprintf (fid, "if(IDL_ms.IDL_status!=error_status_ok)\n");
        fprintf (fid, "{\n");
        fprintf (fid, "IDL_ms.IDL_restartable=idl_false;\n");
        fprintf (fid, "goto IDL_closedown;\n");
        fprintf (fid, "}\n");
}

/******************************************************************************/
/*                                                                            */
/*    Spell "prepare for and make call to rpc_call_start"                     */
/*                                                                            */
/******************************************************************************/
void CSPELL_call_start
(
    FILE *fid,
    BE_handle_info_t *p_handle_info,
    AST_interface_n_t *p_interface,
    AST_operation_n_t *p_operation,
    unsigned long op_num,            /* Number of current operation */
    BE_stat_info_t *p_comm_stat_info,
    BE_stat_info_t *p_fault_stat_info
)
{
    AST_parameter_n_t *p_parameter;
    char const *parameter_name;
    char deref_context;
    boolean found_in_context_handle;

    if (p_handle_info->handle_type == BE_impl_handle_t_k)
    {
        CSPELL_dup_implicit_handle_t( fid, p_interface );
    }
    else if ((p_handle_info->handle_type == BE_parm_user_handle_k)
        || (p_handle_info->handle_type == BE_impl_user_handle_k))
    {
        CSPELL_bind_generic_handle( fid, p_handle_info );
    }
    else if (p_handle_info->handle_type == BE_context_handle_k)
    {
        fprintf (fid, "IDL_assoc_handle = NULL;\n");
        found_in_context_handle = false;
        for (p_parameter = p_operation->parameters;
             p_parameter != NULL;
             p_parameter = p_parameter->next)
        {
            if ( AST_CONTEXT_SET(p_parameter) && AST_IN_SET(p_parameter)
                    && !AST_OUT_SET(p_parameter) )
            {
                if (( p_parameter->type->type_structure.pointer->pointee_type
                     ->kind != AST_pointer_k )
                   )
                {
                    deref_context = ' ';
                }
                else deref_context = '*';  /* Dereference ptr to context */
                NAMETABLE_id_to_string (p_parameter->name, &parameter_name);
                fprintf (fid,
                    "if ( %c(rpc_ss_caller_context_element_t %c*)%s != NULL )\n",
                    deref_context, deref_context, parameter_name);
                fprintf (fid, "{\n");
                fprintf(fid, "rpc_binding_handle_copy(");
                fprintf(fid, "(%c(rpc_ss_caller_context_element_t %c*)",
                        deref_context, deref_context);
                fprintf(fid, "%s)->using_handle,\n", parameter_name);
                fprintf(fid, "(rpc_binding_handle_t*)&IDL_assoc_handle,");
                fprintf(fid, "(error_status_t*)&IDL_ms.IDL_status);\n");
                CSPELL_test_status (fid);
                fprintf (fid, "}\n");
                found_in_context_handle = true;
                break;
            }
        }
        if ( ! found_in_context_handle )
        {
            for (p_parameter = p_operation->parameters;
                 p_parameter != NULL;
                 p_parameter = p_parameter->next)
            {
                if ( AST_CONTEXT_SET(p_parameter) && AST_IN_SET(p_parameter)
                    && AST_OUT_SET(p_parameter) )
                {
                    if ( p_parameter->type->type_structure.pointer->pointee_type
                         ->kind == AST_void_k ) deref_context = ' ';
                    else deref_context = '*';  /* Dereference ptr to context */
                    NAMETABLE_id_to_string (p_parameter->name, &parameter_name);
                    fprintf (fid,
                        "if ( %c(rpc_ss_caller_context_element_t %c*)%s != NULL )\n",
                        deref_context, deref_context, parameter_name);
                    fprintf (fid, "{\n");
                    fprintf(fid, "rpc_binding_handle_copy(");
                    fprintf(fid, "(%c(rpc_ss_caller_context_element_t %c*)",
                            deref_context, deref_context);
                    fprintf(fid, "%s)->using_handle,\n", parameter_name);
                    fprintf(fid, "(rpc_binding_handle_t*)&IDL_assoc_handle, ");
                    fprintf(fid, "(error_status_t*)&IDL_ms.IDL_status);\n");
                    CSPELL_test_status (fid);
                    fprintf (fid, "goto IDL_make_assoc;\n");
                    fprintf (fid, "}\n");
                }
            }
            fprintf (fid, "IDL_make_assoc:\n  ");
        }
        /* If assoc_handle remains null, raise an exception */
        fprintf (fid, "if(IDL_assoc_handle == NULL)\n");
        fprintf (fid, "{\n");
        fprintf (fid, "DCETHREAD_RAISE(rpc_x_ss_in_null_context);\n");
        fprintf (fid, "}\n");
    }
    else if (p_handle_info->handle_type == BE_auto_handle_k)
    {
        fprintf (fid, "IDL_assoc_handle = NULL;\n");
        CSPELL_bind_auto_handle( fid, p_operation, p_comm_stat_info,
                                      p_fault_stat_info );
    }
    else if (p_handle_info->handle_type == BE_rep_as_handle_t_k)
    {
        fprintf( fid, "%s_from_local(&%s,(handle_t **)&%s);\n",
                    BE_get_name(p_handle_info->rep_as_type),
                    BE_get_name(p_handle_info->rep_as_name),
                    assoc_handle_ptr );
    }
    else if (p_handle_info->handle_type == BE_rep_as_handle_t_p_k)
    {
        fprintf( fid, "%s_from_local(%s,(handle_t **)&%s);\n",
                    BE_get_name(p_handle_info->rep_as_type),
                    BE_get_name(p_handle_info->rep_as_name),
                    assoc_handle_ptr );
    }

    /* Call user [binding_callout] routine if one was specified */
    if (p_interface->binding_callout_name != NAMETABLE_NIL_ID)
    {
        spell_name(fid, p_interface->binding_callout_name);
        fprintf(fid, "((rpc_binding_handle_t *)%c%s,\n",
                ((p_handle_info->deref_assoc == '*') ? ' ' : '&'),
                p_handle_info->assoc_name);
        fprintf(fid, "(rpc_if_handle_t)&IDL_ifspec, ");
        fprintf(fid, "(error_status_t *)&IDL_ms.IDL_status);\n");
        CSPELL_test_status(fid);
    }

    fprintf(fid, "rpc_call_start((rpc_binding_handle_t)%c%s, 0",
                    p_handle_info->deref_assoc, p_handle_info->assoc_name);
    if ( AST_BROADCAST_SET(p_operation) )
    {
        fprintf( fid, "|rpc_c_call_brdcst" );
    }
    if ( AST_MAYBE_SET(p_operation) )
    {
        fprintf( fid, "|rpc_c_call_maybe" );
    }
    if ( AST_IDEMPOTENT_SET(p_operation) )
    {
        fprintf( fid, "|rpc_c_call_idempotent" );
    }
    fprintf(fid,",\n (rpc_if_handle_t)&IDL_ifspec,%ld,", op_num);
    fprintf(fid,
"(rpc_call_handle_t*)&IDL_ms.IDL_call_h,&IDL_transfer_syntax,(unsigned32*)&IDL_ms.IDL_status);\n");
    CSPELL_test_status (fid);
}

/******************************************************************************/
/*                                                                            */
/*    Spell interface-wide declarations needed for [auto_handle]              */
/*                                                                            */
/******************************************************************************/
void CSPELL_auto_handle_statics
(
    FILE * fid
)
{
    /* Declare the variables */
    fprintf( fid,
"static RPC_SS_THREADS_ONCE_T IDL_interface_client_once = RPC_SS_THREADS_ONCE_INIT;\n" );
    fprintf( fid, "static RPC_SS_THREADS_MUTEX_T IDL_auto_handle_mutex;\n" );
    fprintf( fid, "static rpc_ns_handle_t IDL_import_cursor;\n" );
    fprintf( fid,
              "static rpc_binding_handle_t IDL_interface_binding = NULL;\n" );
    fprintf( fid, "static idl_boolean IDL_error_using_binding = idl_false;\n");
    fprintf( fid,
  "static error_status_t IDL_auto_handle_status=rpc_s_ss_no_import_cursor;\n" );

    /* And a procedure that will be called once to initialize them */
    fprintf( fid, "static void IDL_auto_handle_init()\n" );
    fprintf( fid, "{\n" );
    fprintf( fid,
            "RPC_SS_THREADS_MUTEX_CREATE( &IDL_auto_handle_mutex );\n" );
    fprintf( fid, "}\n" );
}

/******************************************************************************/
/*                                                                            */
/*    Spell logic for whether to restart an [auto_handle] operation           */
/*                                                                            */
/******************************************************************************/
void DDBE_spell_restart_logic
(
    FILE * fid,
    AST_operation_n_t *p_operation ATTRIBUTE_UNUSED
)
{
#ifdef PERFMON
        fprintf (fid, "#ifdef PERFMON\n");
	fprintf (fid, "IDL_ENDTRY_N;\n");
	fprintf (fid, "#endif\n");
#endif
    fprintf( fid, "DCETHREAD_ENDTRY\n" );
#ifdef PERFMON
        fprintf (fid, "#ifdef PERFMON\n");
	fprintf (fid, "IDL_ENDTRY_X;\n");
	fprintf (fid, "#endif\n");
#endif
    fprintf( fid,
 "if ((IDL_ms.IDL_status != error_status_ok) && (IDL_ms.IDL_status != rpc_s_no_more_bindings))\n" );
    fprintf( fid, "{\n" );
    fprintf( fid, "if (IDL_ms.IDL_restartable)\n" );
    fprintf( fid, "{\n" );
    fprintf( fid, "if (IDL_ms.IDL_call_h!=NULL) rpc_call_end((rpc_call_handle_t*)&IDL_ms.IDL_call_h");
    fprintf( fid,     ",(unsigned32*)&IDL_ms.IDL_status);\n" );
    fprintf( fid, "IDL_ms.IDL_call_h=NULL;\n" );
    fprintf( fid, "rpc_ss_ndr_clean_up(&IDL_ms);\n" );
    fprintf( fid, "IDL_ms.IDL_elt_p=NULL;\n" );
    fprintf( fid, "goto IDL_find_server;\n" );
    fprintf( fid, "}\n" );
    fprintf( fid, 
"else rpc_ss_flag_error_on_binding(&IDL_auto_handle_mutex,\n");
    fprintf (fid, "   &IDL_error_using_binding,\n" );
    fprintf (fid, 
"   &IDL_interface_binding,(rpc_binding_handle_t*)&IDL_assoc_handle);\n}\n");

}


/******************************************************************************/
/*                                                                            */
/*  If the call was made on a binding handle made by rpc_binding_handle_copy, */
/*    emit a call to rpc_binding_free                                         */
/*                                                                            */
/******************************************************************************/
void CSPELL_binding_free_if_needed
(
    FILE *fid,
    BE_handle_info_t *p_handle_info
)
{
    if (p_handle_info->handle_type == BE_impl_handle_t_k)
    {
        fprintf( fid,
     "rpc_binding_free((rpc_binding_handle_t*)&IDL_assoc_handle,&IDL_st2);\n" );
    }
    else if ( (p_handle_info->handle_type == BE_auto_handle_k)
               || (p_handle_info->handle_type == BE_context_handle_k) )
    {
        fprintf( fid,
"if(IDL_assoc_handle!=NULL)rpc_binding_free((rpc_binding_handle_t*)&IDL_assoc_handle,&IDL_st2);\n" );
    }
}


