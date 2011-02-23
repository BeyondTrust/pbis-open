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
 */ 
/*
**
**  NAME
**      ASTP_DMP.H
**
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**      Header file for the AST Builder Dumper module, ASTP_DMP.C
**
**  VERSION: DCE 1.0
**
*/

#ifndef ASTP_DMP_H
#define ASTP_DMP_H
#ifdef DUMPERS
#include <nidl.h>
#include <ast.h>

/*
 * Exported dump routines
 */

void AST_dump_interface
(
    AST_interface_n_t *if_n_p
);

void AST_dump_operation
(
    AST_operation_n_t *operation_node_ptr,
    int indentation
);

void AST_dump_parameter
(
    AST_parameter_n_t *parameter_node_ptr,
    int indentation
);

void AST_dump_nametable_id
(
    char   *format_string,
    NAMETABLE_id_t id
);

void AST_dump_parameter
(
    AST_parameter_n_t *param_node_ptr,
    int     indentation
);

void AST_dump_type(
    AST_type_n_t *type_n_p,
    char *format,
    int indentation
);


void AST_dump_constant
(
    AST_constant_n_t *constant_node_ptr,
    int indentation
);

void AST_enable_hex_dump();


#endif /* Dumpers */
#endif /* ASTP_DMP_H */
