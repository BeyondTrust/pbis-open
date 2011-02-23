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
**      user_exc.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Code generation for user exceptions
**
**
*/

#include <ast.h>
#include <be_pvt.h>
#include <cspell.h>
#include <user_exc.h>

/******************************************************************************/
/*                                                                            */
/*  Declare user exceptions                                                   */
/*                                                                            */
/******************************************************************************/
static void DDBE_list_exceptions
(
    FILE *fid,                      /* [in] Handle for emitted C text */
    AST_interface_n_t *p_interface, /* [in] Pointer to AST interface node */
    int *p_num_declared_exceptions, /* [out] Number of declared exceptions */
    int *p_num_extern_exceptions    /* [out] Number of external exceptions */
)
{
    AST_exception_n_t *p_exception;

    *p_num_declared_exceptions = 0;
    *p_num_extern_exceptions = 0;

    for (p_exception = p_interface->exceptions;
         p_exception != NULL;
         p_exception = p_exception->next)
    {
        if (AST_EXTERN_SET(p_exception))
        {
            fprintf(fid, "extern ");
            (*p_num_extern_exceptions)++;
        }
        else
            (*p_num_declared_exceptions)++;
        fprintf(fid, "EXCEPTION ");
        spell_name(fid, p_exception->name);
        fprintf(fid, ";\n");
    }
}

/******************************************************************************/
/*                                                                            */
/*  Spell code to initialize declared exceptions                              */
/*                                                                            */
/******************************************************************************/
static void DDBE_init_exceptions
(
    FILE *fid,                      /* [in] Handle for emitted C text */
    AST_interface_n_t *p_interface  /* [in] Pointer to AST interface node */
)
{
    AST_exception_n_t *p_exception;

    fprintf(fid, "static void IDL_exceptions_init()\n{\n");
    for (p_exception = p_interface->exceptions;
         p_exception != NULL;
         p_exception = p_exception->next)
    {
        if ( ! AST_EXTERN_SET(p_exception))
        {
            fprintf(fid, "EXCEPTION_INIT(");
            spell_name(fid, p_exception->name);
            fprintf(fid, ");\n");
        }
    }
    fprintf(fid, "}\n");
    fprintf( fid,
"static RPC_SS_THREADS_ONCE_T IDL_exception_once = RPC_SS_THREADS_ONCE_INIT;\n"
             );
}

/******************************************************************************/
/*                                                                            */
/*  Spell an array of pointers to the user exceptions                         */
/*                                                                            */
/******************************************************************************/
static void DDBE_ref_exception_array
(
    FILE *fid,                      /* [in] Handle for emitted C text */
    AST_interface_n_t *p_interface  /* [in] Pointer to AST interface node */
)
{
    AST_exception_n_t *p_exception;
    boolean first = true;

    fprintf(fid, "static EXCEPTION *IDL_exception_addresses[] = {\n");
    for (p_exception = p_interface->exceptions;
         p_exception != NULL;
         p_exception = p_exception->next)
    {
        if (first)
            first = false;
        else
            fprintf(fid, ",\n");
        fprintf(fid, "&");
        spell_name(fid, p_exception->name);
    }
    fprintf(fid, "};\n");
}

/******************************************************************************/
/*                                                                            */
/*  Declare user exception machinery at start of stub                         */
/*                                                                            */
/******************************************************************************/
void DDBE_user_exceptions
(
    FILE *fid,                      /* [in] Handle for emitted C text */
    AST_interface_n_t *p_interface, /* [in] Pointer to AST interface node */
    int *p_num_declared_exceptions, /* [out] Number of declared exceptions */
    int *p_num_extern_exceptions    /* [out] Number of external exceptions */
)
{
    DDBE_list_exceptions(fid, p_interface, p_num_declared_exceptions,
                         p_num_extern_exceptions);
    if (*p_num_declared_exceptions != 0)
        DDBE_init_exceptions(fid, p_interface);
    else if (*p_num_extern_exceptions == 0)
    {
        /* No exception machinery to set up */
        return;
    }
    DDBE_ref_exception_array(fid, p_interface);
}
