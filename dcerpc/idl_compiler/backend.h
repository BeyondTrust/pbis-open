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
**      backend.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Header for backend.c
**
**  VERSION: DCE 1.0
**
*/
boolean BE_main
(
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val,  /* [in] array of cmd option values */
    FILE                *h_fid,     /* [in] header file handle, or NULL */
    FILE                *caux_fid,  /* [in] client aux file handle, or NULL */
    FILE                *saux_fid,  /* [in] server aux file handle, or NULL */
    FILE                *cstub_fid, /* [in] cstub file handle, or NULL */
    FILE                *sstub_fid, /* [in] sstub file handle, or NULL */
    AST_interface_n_t   *int_p      /* [in] ptr to interface node */
);

void BE_push_malloc_ctx
(
      void
);

void BE_push_perm_malloc_ctx
(
      void
);

void BE_pop_malloc_ctx
(
      void
);

heap_mem *BE_ctx_malloc
(
      size_t size
);
