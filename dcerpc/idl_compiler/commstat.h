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
**      commstat.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Data types and function prototypes for commstat.c
**
**  VERSION: DCE 1.0
**
*/

#ifndef COMMSTAT_H
#define COMMSTAT_H

typedef enum {BE_stat_param_k, BE_stat_result_k, BE_stat_addl_k,
              BE_stat_except_k} BE_stat_k_t;

typedef struct BE_stat_info_t {
    BE_stat_k_t type;
    NAMETABLE_id_t name;
} BE_stat_info_t;

void BE_get_comm_stat_info
(
    AST_operation_n_t *p_operation,
    BE_stat_info_t *p_comm_stat_info
);

void BE_get_fault_stat_info
(
    AST_operation_n_t *p_operation,
    BE_stat_info_t *p_fault_stat_info
);

void CSPELL_receive_fault
(
    FILE *fid
);

void CSPELL_return_status
(
    FILE *fid,
    BE_stat_info_t *p_comm_stat_info,
    BE_stat_info_t *p_fault_stat_info,
    char *status_var_name,
    char *result_param_name,
    int num_user_exceptions,
    char *IDL_msp_name
);

#endif

