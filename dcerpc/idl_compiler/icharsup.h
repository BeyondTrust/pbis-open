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
**      icharsup.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Function prototypes and type definitions for international
**          character support
**
**
*/

#ifndef ICHARSUP_H
#define ICHARSUP_H

/* Description of an operation's use of I-char machinery */
typedef struct BE_cs_info_t {
    boolean cs_machinery;   /* TRUE if operation has I-char machinery */
    boolean stag_by_ref;    /* TRUE if passed by ref according to IDL */
    NAMETABLE_id_t  stag;
    boolean drtag_by_ref;    /* TRUE if passed by ref according to IDL */
    NAMETABLE_id_t  drtag;
    boolean rtag_by_ref;    /* TRUE if passed by ref according to IDL */
    NAMETABLE_id_t  rtag;
} BE_cs_info_t;

void BE_cs_analyze_and_spell_vars
(
    FILE *fid,                      /* [in] Handle for emitted C text */
    AST_operation_n_t *p_operation, /* [in] Pointer to AST operation node */
    BE_side_t side,                 /* [in] client or server */
    BE_cs_info_t *p_cs_info         /* [out] Description of I-char machinery */
);

void BE_spell_cs_state
(
    FILE *fid,                      /* [in] Handle for emitted C text */
    char *state_access,             /* [in] "IDL_ms." or "IDL_msp->" */
    BE_side_t side,                 /* [in] client or server */
    BE_cs_info_t *p_cs_info         /* [in] Description of I-char machinery */
);

void BE_spell_cs_tag_rtn_call
(
    FILE *fid,                      /* [in] Handle for emitted C text */
    char *state_access,             /* [in] "IDL_ms." or "IDL_msp->" */
    AST_operation_n_t *p_operation, /* [in] Pointer to AST operation node */
    BE_side_t side,                 /* [in] client or server */
    BE_handle_info_t *p_handle_info,/* [in] How to spell binding handle name */
    BE_cs_info_t *p_cs_info,        /* [in] Description of I-char machinery */
    boolean pickling
);

#endif
