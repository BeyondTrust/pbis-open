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
**      frontend.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Functions exported by frontend.c.
**
**  VERSION: DCE 1.0
**
*/

#include <nidl.h>               /* IDL common defs */
#include <ast.h>                /* Abstract Syntax Tree defs */
#include <nametbl.h>            /* Nametable defs */

#define DCEIDL_DEF   "_DCE_IDL_"

boolean FE_main(
    boolean             *cmd_opt,
    void                **cmd_val,
    STRTAB_str_t        idl_sid,
    AST_interface_n_t   **int_p
);


AST_interface_n_t   *FE_parse_import(
    STRTAB_str_t new_input
);
