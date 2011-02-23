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
**      cspeldcl.h
**
**  FACILITY:
**
**      IDL Compiler Backend
**
**  ABSTRACT:
**
**  Header file for cspeldcl.c
**
**  VERSION: DCE 1.0
**
*/

#ifndef CSPELDCL_H
#define CSPELDCL_H

extern void CSPELL_constant_val (
    FILE *fid, AST_constant_n_t *cp
);

extern void CSPELL_labels (
    FILE *fid, AST_case_label_n_t *tgp
);

extern void CSPELL_parameter_list (
    FILE        *fid,
    AST_parameter_n_t *pp,
    boolean encoding_services
);

#endif
