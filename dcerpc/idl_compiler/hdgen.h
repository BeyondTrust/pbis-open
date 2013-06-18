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
#ifndef HDGEN_H
#define HDGEN_H
/*
**
**  NAME:
**
**      hdgen.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Function prototypes for hdgen.c
**
**  VERSION: DCE 1.0
**
*/

void BE_gen_c_header(
    FILE *fid,              /* Handle for emitted C text */
    AST_interface_n_t *ifp, /* Ptr to AST interface node */
    boolean bugs[],         /* List of backward compatibility "bugs" */
    boolean *cmd_opt
);

void CSPELL_type_def
(
    FILE *fid,
    AST_type_n_t *tp,
    boolean spell_tag
);

char *mapchar
(
    AST_constant_n_t *cp,   /* Constant node with kind == AST_char_const_k */
    boolean warning_flag    /* TRUE => log warning on nonportable escape char */
);
int BE_is_handle_param(AST_parameter_n_t * p);
enum orpc_class_def_type { class_def, proxy_def, stub_def };
void BE_gen_orpc_defs(FILE * fid, AST_interface_n_t * ifp, enum orpc_class_def_type deftype);

#endif
