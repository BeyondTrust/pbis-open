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
**
**  NAME:
**
**      genpipes.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Function prototypes for genpipes.c
**
**  VERSION: DCE 1.0
**
*/

#ifndef GENPIPES_H
#define GENPIPES_H

#define BE_FINISHED_WITH_PIPES -32767

void BE_spell_pipe_struct_name
(
    AST_parameter_n_t *p_parameter,
    char pipe_struct_name[]
);

void CSPELL_init_server_pipes
(
    FILE *fid,
    AST_operation_n_t *p_operation,
    long *p_first_pipe      /* ptr to index and direction of first pipe */
);

void CSPELL_pipe_support_header
(
    FILE *fid,
    AST_type_n_t *p_pipe_type,
    BE_pipe_routine_k_t push_or_pull,
    boolean in_header
);

void BE_gen_pipe_routines
(
    FILE *fid,
    AST_interface_n_t *p_interface
);

void BE_gen_pipe_routine_decls
(
    FILE *fid,
    AST_interface_n_t *p_interface
);

void CSPELL_pipe_base_cast_exp
(
    FILE *fid,
    AST_type_n_t *p_type
);

void CSPELL_pipe_base_type_exp
(
    FILE *fid,
    AST_type_n_t *p_type
);

void BE_undec_piped_arrays
(
    AST_parameter_n_t *flat
);

#endif
