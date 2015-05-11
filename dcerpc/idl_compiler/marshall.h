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
**      marshall.h
**
**  FACILITY:
**
**      IDL Compiler Backend
**
**  ABSTRACT:
**
**  Header file for marshall.c
**
**  VERSION: DCE 1.0
**
*/

#ifndef MARSHALL_H
#define MARSHALL_H

typedef unsigned long BE_mflags_t;

/*
 *  marshalling subsystem flags
 */
#define BE_M_TRANSCEIVE     0x000001  /* for the last client-side in parameter */
#define BE_M_NEW_SLOT       0x000002  /* start a new slot before marshalling */
#define BE_M_SYNC_MP        0x000004  /* synchronize the mp before marshalling */
#define BE_M_ALIGN_MP       0x000008  /* align the mp before this param */
#define BE_M_MAINTAIN_OP    0x000010  /* maintain the op as well as the mp */
#define BE_M_SLOTLESS       0x000020  /* marshall via "mp" rather than "pmp" */
#define BE_M_CHECK_BUFFER   0x000040  /* check recv buff before unmarshalling */
#define BE_M_FIRST_BLOCK    0x000080  /* this is the first block in the list */
#define BE_M_ADVANCE_MP     0x000100  /* advance the mp after this param */
#define BE_M_DEFER          0x000200  /* space out this pointer parameter */
#define BE_M_CONVERT        0x000400  /* use the costlier convert macros */
#define BE_M_SP_BLOCK       0x000800  /* this block contains only a pointer */
#define BE_M_CALLEE         0x001000  /* for sp pointer marshalling code */
#define BE_M_CALLER         0x002000  /* for sp pointer marshalling code */
#define BE_M_ENOUGH_ROOM    0x004000  /* unmarshall entire array from buffer */
#define BE_M_BUFF_BOOL      0x008000  /* set buffer_changed if it does */
#define BE_M_PIPE           0x010000  /* emitting pipe un/marshalling code */
#define BE_M_NODE           0x020000  /* emitting node un/marshalling code */
#define BE_M_BUFF_EXISTS    0x040000  /* don't start new pointee buffer */
#define BE_M_XMIT_CVT_DONE  0x080000  /* don't call to_xmit or from_local, */
                                      /* because already done by caller */
#define BE_M_ARRAY_BUFF     0x100000  /* using the stub's malloc'ed array
                                         marshalling storage */
#define BE_M_OOL_RTN        0x200000  /* emitting an ool routine */

/*
 * The following structure contains the names to be used by marshalling
 * routines for the parameters in question.  "mn" stands for "marshalling names"
 */
typedef struct
{
    char *mp;
    char *op;
    char *drep;
    char *elt;
    char *st;
    char *pmem_h;
    char *call_h;
    char *bad_st;    /* code to emit if st.all != status_ok */
} BE_mn_t;

#ifndef MIA
extern BE_mn_t BE_def_mn;
#endif

void BE_declare_stack_vars
(
    FILE *fid,
    AST_operation_n_t *oper,
    boolean *p_uses_packet
);

void BE_marshall
(
    FILE *fid,
    AST_operation_n_t *oper,
    BE_direction_t direction
);

void BE_unmarshall
(
    FILE *fid,
    AST_operation_n_t *oper,
    BE_direction_t direction
);

void BE_marshall_param
(
    FILE *fid,
    AST_parameter_n_t *param,
    BE_mflags_t flags,
    int slot_num,
    int *p_slots_used,
    boolean *p_routine_mode,
    int total_slots
);

void BE_unmarshall_param
(
    FILE *fid,
    AST_parameter_n_t *param,
    BE_mflags_t flags,
    BE_mn_t *mn,
    boolean *p_routine_mode
);

#endif
