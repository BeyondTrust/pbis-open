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
**
**  NAME:
**
**      cspell.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Definitions of routines declared in cspell.c
**
**  VERSION: DCE 1.0
**
*/

#ifndef CSPELL_H
#define CSPELL_H

void CSPELL_std_include(
    FILE *fid,
    char header_name[],
    BE_output_k_t filetype,
    int op_count
);

void spell_name(
    FILE *fid,
    NAMETABLE_id_t name
);

void CSPELL_var_decl(
    FILE *fid,
    AST_type_n_t *type,
    NAMETABLE_id_t name
);

void CSPELL_typed_name(
    FILE *fid,
    AST_type_n_t *type,
    NAMETABLE_id_t name,
    AST_type_n_t *in_typedef,
    boolean in_struct,
    boolean spell_tag,
    boolean encoding_services
);

void CSPELL_function_def_header(
    FILE *fid,
    AST_operation_n_t *oper,
    NAMETABLE_id_t name
);

void CSPELL_cast_exp(
    FILE *fid,
    AST_type_n_t *tp
);

void CSPELL_ptr_cast_exp(
    FILE *fid,
    AST_type_n_t *tp
);

void CSPELL_type_exp_simple(
    FILE *fid,
    AST_type_n_t *tp
);

boolean CSPELL_scalar_type_suffix(
    FILE *fid,
    AST_type_n_t *tp
);

void CSPELL_pipe_struct_routine_decl
(
    FILE *fid,
    AST_type_n_t *p_pipe_type,
    BE_pipe_routine_k_t routine_kind,
    boolean cast
);

void DDBE_spell_manager_param_cast
(
    FILE *fid,
    AST_type_n_t *tp
);

#endif
