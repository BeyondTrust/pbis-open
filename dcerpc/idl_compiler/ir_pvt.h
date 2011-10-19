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
**      ir_pvt.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Header file containing defining intermediate-rep private data structures
**  for data that is kept in the IR_info_t field of some AST nodes.
**
**  %a%private_begin
**
**
**  %a%private_end  
*/

#ifndef IR_PVTH_INCL
#define IR_PVTH_INCL

typedef struct IR_info_n_t {
    /*
     * Pointer to last created tuple in a parameter or type node's tuple list.
     */
    struct IR_tup_n_t   *cur_tup_p;
    /*
     * For a field, field number.  For other nodes, available for any use.
     */
    long                id_num;
    /*
     * On a param, T => requires server side preallocation of [ref] pointee(s).
     * On a type, T => same if reference is not under a full or unique pointer.
     */
    boolean             allocate_ref;
} IR_info_n_t;

typedef IR_info_n_t *IR_info_t;

/*
 * Data structure used to help sort the case labels of a union.
 */
typedef struct IR_case_info_n_t {
    struct AST_arm_n_t          *arm_p;     /* Ptr to union arm node */
    struct AST_case_label_n_t   *label_p;   /* Ptr to case label node */
    unsigned long               value;      /* Normallized case label value */
} IR_case_info_n_t;

#endif
