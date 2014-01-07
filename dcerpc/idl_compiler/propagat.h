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
**      propagat.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Declaration of functions exported by propagat.c.
**
**  VERSION: DCE 1.0
**
*/

#ifndef PROPAGATH_INCL
#define PROPAGATH_INCL

#include <ast.h>                /* Abstract Syntax Tree defs */

/*
**  P R O P _ m a i n
**
**  Main routine for propagation component of IDL compiler.
**  Propagates attributes and other information throughout the AST.
**  Much of propagation is done by earlier compiler phases - this
**  routine handles any propagation that is easier to do if saved
**  until after the parsing/AST-building phases are complete.
*/

extern boolean PROP_main(       /* Returns true on success */
    boolean     *cmd_opt_arr,   /* [in] Array of command option flags */
    void        **cmd_val_arr,  /* [in] Array of command option values */
    AST_interface_n_t *int_p    /* [in] Ptr to AST interface node */
);

#ifndef mips
void PROP_set_type_attr
(
    AST_type_n_t *type_node_ptr,
    AST_flags_t  type_attr
);
#endif


#endif
