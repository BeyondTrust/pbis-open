/*
 *
 * (c) Copyright 1989 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1989 DIGITAL EQUIPMENT CORPORATION
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
**      dutils.h
**
**  FACILITY:
**
**      IDL Compiler Backend
**
**  ABSTRACT:
**
**  Header file for dutils.c
**
**  VERSION: DCE 1.0
*/

#ifndef DUTILS_H
#define DUTILS_H

NAMETABLE_id_t BE_new_local_var_name
(
#ifdef PROTO
    char *root
#endif
);

char const *BE_get_name
(
    NAMETABLE_id_t id
);

int BE_required_alignment
(
#ifdef PROTO
    AST_parameter_n_t *param
#endif
);

int BE_resulting_alignment
(
#ifdef PROTO
    AST_parameter_n_t *param
#endif
);

struct BE_ptr_init_t *BE_new_ptr_init
(
#ifdef PROTO
    NAMETABLE_id_t pointer_name,
    AST_type_n_t *pointer_type,
    NAMETABLE_id_t pointee_name,
    AST_type_n_t *pointee_type,
    boolean heap
#endif
);

AST_type_n_t *BE_get_type_node
(
#ifdef PROTO
    AST_type_k_t kind
#endif
);

AST_type_n_t *BE_pointer_type_node
(
#ifdef PROTO
    AST_type_n_t *type
#endif
);

AST_type_n_t *BE_slice_type_node
(
#ifdef PROTO
    AST_type_n_t *type
#endif
);

char *BE_first_element_expression
(
#ifdef PROTO
    AST_parameter_n_t *param
#endif
);

char *BE_count_expression
(
#ifdef PROTO
    AST_parameter_n_t *param
#endif
);

char *BE_size_expression
(
#ifdef PROTO
    AST_parameter_n_t *param
#endif
);

void BE_declare_surrogates
(
#ifdef PROTO
    AST_operation_n_t *oper,
    AST_parameter_n_t *param
#endif
);

void BE_declare_server_surrogates
(
#ifdef PROTO
    AST_operation_n_t *oper
#endif
);

int BE_num_elts
(
#ifdef PROTO
    AST_parameter_n_t *param
#endif
);

char *BE_A_expression
(
#ifdef PROTO
    AST_parameter_n_t *param,
    int dim
#endif
);

char *BE_B_expression
(
#ifdef PROTO
    AST_parameter_n_t *param,
    int dim
#endif
);

char *BE_Z_expression
(
#ifdef PROTO
    AST_parameter_n_t *param,
    int dim
#endif
);

AST_parameter_n_t *BE_create_recs
(
#ifdef PROTO
    AST_parameter_n_t *params,
    BE_side_t side
#endif
);

#ifdef DEBUG_VERBOSE

void traverse(
#ifdef PROTO
    AST_parameter_n_t *list,
    int indent
#endif
);

void traverse_blocks(
#ifdef PROTO
BE_param_blk_t *block
#endif
);

#endif


#endif
