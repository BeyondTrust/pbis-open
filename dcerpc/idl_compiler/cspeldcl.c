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
**  NAME:
**
**      cspeldcl.c
**
**  FACILITY:
**
**      IDL Compiler Backend
**
**  ABSTRACT:
**
**  Routines to spell declaration related material to C source
**
**  VERSION: DCE 1.0
*/

#include <nidl.h>
#include <ast.h>
#include <bedeck.h>
#include <cspell.h>
#include <cspeldcl.h>
#include <hdgen.h>

/******************************************************************************/
/*                                                                            */
/*    Build text string for constant value                                    */
/*                                                                            */
/******************************************************************************/
void CSPELL_constant_val_to_string
(
    AST_constant_n_t *cp,
    char *str
)
{
    char const *str2;

    switch (cp->kind) {
        case AST_nil_const_k:
            sprintf (str, "NULL");
            break;
        case AST_boolean_const_k:
            if (cp->value.boolean_val)
                sprintf (str, "ndr_true");
            else
                sprintf (str, "ndr_false");
            break;
        case AST_int_const_k:
            sprintf (str, "%ld", cp->value.int_val);
            break;
        case AST_string_const_k:
            STRTAB_str_to_string (cp->value.string_val, &str2);
            sprintf (str, "\"%s\"", str2);
            break;
        case AST_char_const_k:
            sprintf (str, "'%s'", mapchar(cp, FALSE));
            break;
        default:
            INTERNAL_ERROR("Unsupported tag in CSPELL_constant_val_to_string");
            break;
        }
}

/******************************************************************************/
/*                                                                            */
/*    Routine to spell constant to C source                                   */
/*                                                                            */
/******************************************************************************/
void CSPELL_constant_val
(
    FILE *fid,
    AST_constant_n_t *cp
)
{
    char str[max_string_len];

    CSPELL_constant_val_to_string (cp, str);
    fprintf (fid, "%s", str);
}

/******************************************************************************/
/*                                                                            */
/*    Routine to spell union case label comment to C source                   */
/*                                                                            */
/******************************************************************************/
void CSPELL_labels
(
    FILE *fid,
    AST_case_label_n_t *clp
)
{
    boolean first = true;

    fprintf (fid, "/* case(s): ");
    for (; clp; clp = clp->next) {
        if (first)
            first = false;
        else
            fprintf (fid, ", ");
        if (clp->default_label)
            fprintf (fid, "default");
        else
            CSPELL_constant_val (fid, clp->value);
        };
    fprintf (fid, " */\n");
}

/******************************************************************************/
/*                                                                            */
/*    Routine to spell function parameter list to C source                    */
/*                                                                            */
/******************************************************************************/
void CSPELL_parameter_list
(
    FILE *fid,
    AST_parameter_n_t *pp,
    boolean encoding_services   /* TRUE => [encode] or [decode] on operation */
)
{
    boolean            first = true;

    if (pp)
    {
        for (; pp; pp = pp->next)
        {
            if (AST_HIDDEN_SET(pp))
            {
                /* Parameter does not appear in signature delivered to user */
                continue;
            }
            if (first)
            {
                first = false;
                if (encoding_services)
                {
                    /* First parameter is a pickling handle */
                    fprintf(fid, "    /* [in] */ idl_es_handle_t ");
                    if (pp->type->kind == AST_pointer_k)
                    {
                        /* Passed by reference */
                        fprintf(fid, "*");
                    }
                    spell_name(fid, pp->name);
                    continue;
                }
            }
            else
                fprintf (fid, ",\n");
            fprintf (fid, "    /* [");
            if (AST_IN_SET(pp))
            {
                fprintf(fid, "in");
                if (AST_OUT_SET(pp)) fprintf (fid, ", out");
            }
            else fprintf  (fid, "out");
            fprintf (fid, "] */ ");
#ifndef MIA
            if (pp->be_info.param)
                CSPELL_typed_name (fid, pp->type,
                    pp->be_info.param->name, NULL, false, true, false);
            else
#endif
                 CSPELL_typed_name (fid, pp->type, pp->name, NULL, false, true,
                                    false);
        }
    }
    else
        fprintf (fid, "    void");
}
