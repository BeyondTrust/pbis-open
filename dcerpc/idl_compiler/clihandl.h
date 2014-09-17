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
**      clihandl.h
**
**  FACILITY:
**
**      IDL Compiler Backend
**
**  ABSTRACT:
**
**  Header file for clihandl.c
**
**  VERSION: DCE 1.0
**
*/
#ifndef CLIHANDL_H
#define CLIHANDL_H

#include <commstat.h>

typedef enum {
    BE_parm_handle_t_k,    /* Operation has first parameter of type handle_t */
    BE_parm_user_handle_k, /* Operation has first parm with [handle] attrib */
    BE_context_handle_k,   /* Operation has an [in] context handle parameter */
    BE_impl_handle_t_k,    /* No handle parm. Implicit handle_t handle */
    BE_impl_user_handle_k, /* No handle parm. Implicit [handle] handle */
    BE_auto_handle_k,      /* No handle parm. [auto_handle] interface */
    BE_rep_as_handle_t_k,  /* First parm handle_t with [rep_as] passed by val */
    BE_rep_as_handle_t_p_k /* First parm handle_t with [rep_as] passed by ref */
} BE_handle_type_k_t;

typedef struct {
    BE_handle_type_k_t handle_type;  /* Type of handle for operation */
    char const *assoc_name;       /* Ptr to name to be used for assoc handle */
    char const *type_name;        /* Ptr to name of [handle] type */
    char const *user_handle_name; /* Ptr to name of [handle] object */
    char deref_assoc;         /* '*' if handle must be dereferenced, else ' ' */
    char deref_generic;      /* '*' if handle must be dereferenced, else ' ' */
    boolean auto_handle_idempotent_op;  /* Only used if op is [auto_handle]
                                           TRUE if op is [idempotent] */
    NAMETABLE_id_t rep_as_name; /* Name of handle_t param to which [rep_as]
                                        is attached */
    NAMETABLE_id_t rep_as_type; /* Type of handle param which has [rep_as] */
} BE_handle_info_t;

extern char assoc_handle_ptr[];

void BE_setup_client_handle(
    FILE *fid,
    AST_interface_n_t *p_interface,
    AST_operation_n_t *p_operation,
    BE_handle_info_t *p_handle_info
);

void CSPELL_call_start(
    FILE *fid,
    BE_handle_info_t *p_handle_info,
    AST_interface_n_t *p_interface,
    AST_operation_n_t *p_operation,
    unsigned long op_num,            /* Number of current operation */
    BE_stat_info_t *p_comm_stat_info,
    BE_stat_info_t *p_fault_stat_info
);

void CSPELL_auto_handle_statics
(
    FILE * fid
);

void CSPELL_restart_logic
(
    FILE * fid,
    AST_operation_n_t *p_operation,
    boolean uses_packet
);

void CSPELL_binding_free_if_needed
(
    FILE *fid,
    BE_handle_info_t *p_handle_info
);

#endif

