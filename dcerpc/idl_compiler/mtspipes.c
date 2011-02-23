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
**
**  NAME:
**
**      mtspipes.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Pipes for MTS compiler
**
**  VERSION: DCE 1.0
**
*/

#include <nidl.h>
#include <ast.h>
#include <bedeck.h>
#include <cspell.h>
#include <ddbe.h>
#include <dutils.h>
#include <nametbl.h>
#include <mtspipes.h>

/******************************************************************************/
/*                                                                            */
/*    Find index of next [in] or [out] pipe                                   */
/*                                                                            */
/******************************************************************************/
static void BE_get_next_pipe_index
(
    AST_parameter_n_t *p_parameter,
    unsigned long ast_in_or_out,    /* AST_IN or AST_OUT */
    long curr_pipe_index,
    long *p_next_pipe_index      /* 0 if no more pipes inrequested direction */
)
{
    for ( p_parameter = p_parameter->next;
          p_parameter != NULL;
          p_parameter = p_parameter->next )
    {
        if ( (p_parameter->type->kind == AST_pipe_k)
             || ((p_parameter->type->kind == AST_pointer_k)
                 && (p_parameter->type->type_structure.pointer->pointee_type
                     ->kind == AST_pipe_k)) )
        {
            curr_pipe_index++;
            if (ast_in_or_out & (p_parameter->flags))
            {
                *p_next_pipe_index = curr_pipe_index;
                return;
            }
        }
    }
    *p_next_pipe_index = 0;
}

/******************************************************************************/
/*                                                                            */
/*    Get pipe type name for parameter                                        */
/*                                                                            */
/******************************************************************************/
static void BE_get_pipe_type_name
(
    AST_parameter_n_t *p_parameter,
    char const **p_p_name
)
{
    if (p_parameter->type->kind == AST_pipe_k)
    {
        NAMETABLE_id_to_string( p_parameter->type->name, p_p_name );
    }
    else /* parameter is reference pointer to pipe */
    {
        NAMETABLE_id_to_string( p_parameter->type->type_structure.pointer
                                 ->pointee_type->name, p_p_name );
    }
}

/******************************************************************************/
/*                                                                            */
/*    Initialization of server pipes                                          */
/*                                                                            */
/******************************************************************************/
void DDBE_init_server_pipes
(
    FILE *fid,
    AST_operation_n_t *p_operation,
    long *p_first_pipe      /* ptr to index and direction of first pipe */
)
{
    long first_in_pipe;     /* index of first [in] pipe */
    long first_out_pipe;    /* index of first [out] pipe */
    long curr_pipe_index;
    long next_in_pipe_index;
    long next_out_pipe_index;
    AST_parameter_n_t *p_parameter;
    char const *p_pipe_type_name;

    /* Establish indices of first pipes */
    first_in_pipe = 0;
    first_out_pipe = 0;
    curr_pipe_index = 0;
    for ( p_parameter = p_operation->parameters;
          p_parameter != NULL;
          p_parameter = p_parameter->next )
    {
        if ( (p_parameter->type->kind == AST_pipe_k)
             || ((p_parameter->type->kind == AST_pointer_k)
                 && (p_parameter->type->type_structure.pointer->pointee_type
                     ->kind == AST_pipe_k)) )
        {
            curr_pipe_index++;
            if ( AST_IN_SET(p_parameter) )
            {
                if (first_in_pipe == 0) first_in_pipe = curr_pipe_index;
            }
            if ( AST_OUT_SET(p_parameter) )
            {
                if (first_out_pipe == 0) first_out_pipe = curr_pipe_index;
            }
        }
    }
    if ( first_in_pipe != 0 ) *p_first_pipe = first_in_pipe;
    else *p_first_pipe = -first_out_pipe;

    /* Emit initialization code */
    curr_pipe_index = 0;
    for ( p_parameter = p_operation->parameters;
          p_parameter != NULL;
          p_parameter = p_parameter->next )
    {
        if ( (p_parameter->type->kind == AST_pipe_k)
             || ((p_parameter->type->kind == AST_pointer_k)
                 && (p_parameter->type->type_structure.pointer->pointee_type
                     ->kind == AST_pipe_k)) )
        {
	    AST_type_n_t *pipe_t = p_parameter->type;

	    /* Find pipe type, if passed by reference */
	    if (pipe_t->kind == AST_pointer_k)
		pipe_t = pipe_t->type_structure.pointer->pointee_type;

            curr_pipe_index++;
            BE_get_pipe_type_name( p_parameter, &p_pipe_type_name );

            /* Hook the push and pull routines */
            fprintf( fid, "%s.push=(",BE_get_name(p_parameter->name) );
	    CSPELL_pipe_struct_routine_decl(fid, pipe_t, BE_pipe_push_k, TRUE);
            fprintf( fid, ")rpc_ss_ndr_ee_marsh_pipe_chunk;\n");

            fprintf( fid, "%s.pull=(",BE_get_name(p_parameter->name) );
	    CSPELL_pipe_struct_routine_decl(fid, pipe_t, BE_pipe_pull_k, TRUE);
            fprintf( fid, ")rpc_ss_ndr_ee_unmar_pipe_chunk;\n");

            /* Initialize the state block */
            next_in_pipe_index = 0;
            next_out_pipe_index = 0;
            if ( AST_IN_SET(p_parameter) )
            {
                BE_get_next_pipe_index( p_parameter, AST_IN, curr_pipe_index,
                                                         &next_in_pipe_index );
                if (next_in_pipe_index == 0)
                {
                    /* Next pipe is [out] */
                    if (first_out_pipe != 0)
                         next_in_pipe_index = -first_out_pipe;
                    else next_in_pipe_index = BE_FINISHED_WITH_PIPES;
                }
            }
            if ( AST_OUT_SET(p_parameter) )
            {
                BE_get_next_pipe_index( p_parameter, AST_OUT, curr_pipe_index,
                                                         &next_out_pipe_index );
                if (next_out_pipe_index == 0 )
                     next_out_pipe_index = BE_FINISHED_WITH_PIPES;
                else
                     next_out_pipe_index = -next_out_pipe_index;
            }
            fprintf( fid,
        "rpc_ss_mts_init_callee_pipe(%ld,%ld,%ld,&IDL_current_pipe,&IDL_ms,\n",
             curr_pipe_index, next_in_pipe_index, next_out_pipe_index );
            fprintf( fid, "%ld,(rpc_ss_mts_ee_pipe_state_t**)&%s.state);\n",
                     (p_parameter->type->kind == AST_pipe_k)
                     ? p_parameter->type->be_info.dd_type->type_vec_p->index
                     : p_parameter->type->type_structure.pointer->pointee_type
                                           ->be_info.dd_type->type_vec_p->index,
                     BE_get_name(p_parameter->name) );
        }
    }

}
