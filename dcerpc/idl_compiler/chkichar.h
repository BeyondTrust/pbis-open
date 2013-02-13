/*
 * 
 * (c) Copyright 1991 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1991 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1991 DIGITAL EQUIPMENT CORPORATION
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
**      chkichar.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  IDL Compiler Semantic Checking for international character support.
**
*/

extern void CHK_param_cs(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *type_p         /* [in] Parameter type */
);

extern void CHK_op_cs(
    AST_operation_n_t   *op_p           /* [in] Ptr to AST operation node */
);

extern void CHK_field_cs(
    AST_field_n_t       *field_p        /* [in] Ptr to AST field node */
);

extern void CHK_pipe_base_type_cs(
    AST_pipe_n_t        *pipe_p,        /* [in] Ptr to AST pipe node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
);

extern void CHK_type_cs(
    AST_type_n_t        *top_type_p,    /* [in] Top-level presented type */
    AST_type_n_t        *type_p,        /* [in] Ptr to AST type node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
);
