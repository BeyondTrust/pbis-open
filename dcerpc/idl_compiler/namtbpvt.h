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
/*
**
**  NAME:
**
**      namtbpvt.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  This header file contains the private definitions necessary for the
**  nametable modules.
**
**  VERSION: DCE 1.0
**
*/
/********************************************************************/
/*                                                                  */
/*   NAMTBPVT.H                                                     */
/*                                                                  */
/*              Data types private to the nametable routines.       */
/*                                                                  */
/********************************************************************/



typedef struct NAMETABLE_binding_n_t {
        int                              bindingLevel;
        const void                      *theBinding;
        struct NAMETABLE_binding_n_t    *nextBindingThisLevel;
        struct NAMETABLE_binding_n_t    *oldBinding;
        NAMETABLE_id_t                   boundBy;
}
NAMETABLE_binding_n_t;


typedef struct NAMETABLE_n_t {
        struct NAMETABLE_n_t    *left;  /* Subtree with names less          */
        struct NAMETABLE_n_t    *right; /* Subtree with names greater       */
        struct NAMETABLE_n_t    *parent;/* Parent in the tree               */
                                        /* NULL if this is the root         */
        const char              *id;    /* The identifier string            */
        NAMETABLE_binding_n_t   *bindings;      /* The list of bindings known       */
                                                /* by this name at this time.       */
        NAMETABLE_binding_n_t   *tagBinding;    /* The structure known by this tag. */
}
NAMETABLE_n_t;

typedef struct NAMETABLE_temp_name_t {
        struct NAMETABLE_temp_name_t * next;  /* Next temp name chain block */
        NAMETABLE_id_t   node;                /* The temp name tree node    */
}
NAMETABLE_temp_name_t;

