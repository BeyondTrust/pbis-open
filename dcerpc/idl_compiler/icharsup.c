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
**      icharsup.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Code generation for international character support
**
**
*/

#include <ast.h>
#include <bedeck.h>
#include <be_pvt.h>
#include <clihandl.h>
#include <dutils.h>
#include <nametbl.h>
#include <icharsup.h>
#include <cspell.h>

/******************************************************************************/
/*                                                                            */
/*  Analyze the use of [cs_*tag] and [cs_tag_rtn] in an operation             */
/*      and spell any necessary stub local variables                          */
/*                                                                            */
/******************************************************************************/
void BE_cs_analyze_and_spell_vars
(
    FILE *fid,                      /* [in] Handle for emitted C text */
    AST_operation_n_t *p_operation, /* [in] Pointer to AST operation node */
    BE_side_t side,                 /* [in] client or server */
    BE_cs_info_t *p_cs_info         /* [out] Description of I-char machinery */
)
{
    AST_parameter_n_t *p_parameter;

    /* Look at the operation definition. If any of the parameters are [cs_*tag],
        then I-char machinery is in use, and an IDL_cs_tags_t will be needed. 
        Remember the names of the tags for later use */
    p_cs_info->cs_machinery = false;
    p_cs_info->stag = NAMETABLE_NIL_ID;
    p_cs_info->drtag = NAMETABLE_NIL_ID;
    p_cs_info->rtag = NAMETABLE_NIL_ID;
    for (p_parameter = p_operation->parameters;
         p_parameter != NULL;
         p_parameter = p_parameter->next)
    {
        if (AST_CS_STAG_SET(p_parameter))
        {
            p_cs_info->cs_machinery = true;
            p_cs_info->stag_by_ref = (p_parameter->type->kind == AST_pointer_k);
            p_cs_info->stag = p_parameter->name;
        }
        if (AST_CS_DRTAG_SET(p_parameter))
        {
            p_cs_info->cs_machinery = true;
            p_cs_info->drtag_by_ref=(p_parameter->type->kind == AST_pointer_k);
            p_cs_info->drtag = p_parameter->name;
        }
        if (AST_CS_RTAG_SET(p_parameter))
        {
            p_cs_info->cs_machinery = true;
            p_cs_info->rtag_by_ref = (p_parameter->type->kind == AST_pointer_k);
            p_cs_info->rtag = p_parameter->name;
        }
    }

    if ( ! p_cs_info->cs_machinery )
        return;

    fprintf(fid, "IDL_cs_tags_t IDL_cs_tags;\n");

    /*  If [cs_tag_rtn] is present, the [cs_*tag] parameters are hidden from the
        user. For the [cs_*tag]s that do not appear in the IDL
        parameter list, dummy variables are passed to the [cs_tag_rtn].
        For the server side, spell the dummies as they are identified */
    if (p_operation->cs_tag_rtn_name != NAMETABLE_NIL_ID)
    {
        if (p_cs_info->stag == NAMETABLE_NIL_ID)
        {
            p_cs_info->stag_by_ref = false;
            if (side == BE_server_side)
                fprintf(fid, "idl_ulong_int IDL_dummy_stag;\n");
            p_cs_info->stag = NAMETABLE_add_id("IDL_dummy_stag");
        }
        if (p_cs_info->drtag == NAMETABLE_NIL_ID)
        {
            p_cs_info->drtag_by_ref = false;
            if (side == BE_server_side)
                fprintf(fid, "idl_ulong_int IDL_dummy_drtag=0;\n");
            p_cs_info->drtag = NAMETABLE_add_id("IDL_dummy_drtag");
        }
        if (p_cs_info->rtag == NAMETABLE_NIL_ID)
        {
            p_cs_info->rtag_by_ref = false;
            if (side == BE_server_side)
                fprintf(fid, "idl_ulong_int IDL_dummy_rtag;\n");
            p_cs_info->rtag = NAMETABLE_add_id("IDL_dummy_rtag");
        }

        /* For the client side only we need to spell all the tag variables.
            On the server side the non-dummy tags are declared by the
            normal parameter surrogate mechanism */
        if (side == BE_server_side)
            return;
        if (p_cs_info->stag_by_ref)
        {
            fprintf(fid, "idl_ulong_int IDL_stag_value;\n");
            fprintf(fid, "idl_ulong_int *");
            spell_name(fid, p_cs_info->stag);
            fprintf(fid, "= &IDL_stag_value;\n");
        }
        else
        {
            fprintf(fid, "idl_ulong_int ");
            spell_name(fid, p_cs_info->stag);
            fprintf(fid, ";\n");
        }
        if (p_cs_info->drtag != p_cs_info->stag)
        {
            if (p_cs_info->drtag_by_ref)
            {
                fprintf(fid, "idl_ulong_int IDL_drtag_value;\n");
                fprintf(fid, "idl_ulong_int *");
                spell_name(fid, p_cs_info->drtag);
                fprintf(fid, "= &IDL_drtag_value;\n");
            }
            else
            {
                fprintf(fid, "idl_ulong_int ");
                spell_name(fid, p_cs_info->drtag);
                fprintf(fid, ";\n");
            }
        }
        if ((p_cs_info->rtag != p_cs_info->stag)
            && (p_cs_info->rtag != p_cs_info->drtag))
        {
            if (p_cs_info->rtag_by_ref)
            {
                fprintf(fid, "idl_ulong_int IDL_rtag_value;\n");
                fprintf(fid, "idl_ulong_int *");
                spell_name(fid, p_cs_info->rtag);
                fprintf(fid, "= &IDL_rtag_value;\n");
            }
            else
            {
                fprintf(fid, "idl_ulong_int ");
                spell_name(fid, p_cs_info->rtag);
                fprintf(fid, ";\n");
            }
        }
    }
}


/******************************************************************************/
/*                                                                            */
/*  If I-char machinery is in use, spell the code necessary to attach tag     */
/*  information to the state block                                            */
/*                                                                            */
/******************************************************************************/
void BE_spell_cs_state
(
    FILE *fid,                      /* [in] Handle for emitted C text */
    char *state_access,             /* [in] "IDL_ms." or "IDL_msp->" */
    BE_side_t side,                 /* [in] client or server */
    BE_cs_info_t *p_cs_info         /* [in] Description of I-char machinery */
)
{
    if ( ! p_cs_info->cs_machinery )
        return;
    if (side == BE_client_side)
    {
        if (p_cs_info->stag != NAMETABLE_NIL_ID)
        {
            fprintf(fid, "IDL_cs_tags.p_marsh_tag = %c%s;\n",
                            p_cs_info->stag_by_ref ? ' ' : '&',
                            BE_get_name(p_cs_info->stag));
        }
    }
    else    /* server side */
    {
        if (p_cs_info->rtag != NAMETABLE_NIL_ID)
        {
            fprintf(fid, "IDL_cs_tags.p_marsh_tag = &%s;\n",
                            BE_get_name(p_cs_info->rtag));
        }
    }
    if (side == BE_client_side)
    {
        if (p_cs_info->rtag != NAMETABLE_NIL_ID)
        {
            fprintf(fid, "IDL_cs_tags.p_unmar_tag = %c%s;\n",
                            p_cs_info->rtag_by_ref ? ' ' : '&',
                            BE_get_name(p_cs_info->rtag));
        }
    }
    else    /* server side */
    {
        if (p_cs_info->stag != NAMETABLE_NIL_ID)
        {
            fprintf(fid, "IDL_cs_tags.p_unmar_tag = &%s;\n",
                            BE_get_name(p_cs_info->stag));
        }
    }
    fprintf(fid, "%sIDL_cs_tags_p = &IDL_cs_tags;\n", state_access);
}

/******************************************************************************/
/*                                                                            */
/* If the operation has [cs_tag_rtn] and at least one [cs_*tag] parameter,    */
/*  spell the [cs_tag_rtn] call                                               */
/*                                                                            */
/******************************************************************************/
void BE_spell_cs_tag_rtn_call
(
    FILE *fid,                      /* [in] Handle for emitted C text */
    char *state_access,             /* [in] "IDL_ms." or "IDL_msp->" */
    AST_operation_n_t *p_operation, /* [in] Pointer to AST operation node */
    BE_side_t side,                 /* [in] client or server */
    BE_handle_info_t *p_handle_info,/* [in] How to spell binding handle name */
    BE_cs_info_t *p_cs_info,        /* [in] Description of I-char machinery */
    boolean pickling                /* [in] TRUE => called from pickling stub */
)
{
    if ((p_operation->cs_tag_rtn_name == NAMETABLE_NIL_ID)
        || ( ! p_cs_info->cs_machinery ) )
    {
        /* No I-char machinery */
        return;
    }

    spell_name(fid, p_operation->cs_tag_rtn_name);
    if (pickling)
        fprintf(fid, "(NULL,");
    else
        fprintf(fid, "(%c%s,",
                p_handle_info->deref_assoc, p_handle_info->assoc_name);
    if (side == BE_client_side)
    {
        fprintf(fid, "idl_false,%c%s,%c%s,%c%s",
                    p_cs_info->stag_by_ref ? ' ' : '&',
                    BE_get_name(p_cs_info->stag),
                    p_cs_info->drtag_by_ref ? ' ' : '&',
                    BE_get_name(p_cs_info->drtag),
                    p_cs_info->rtag_by_ref ? ' ' : '&',
                    BE_get_name(p_cs_info->rtag));
    }
    else    /* server */
    {
        fprintf(fid, "idl_true,&%s,&%s,&%s",
                    BE_get_name(p_cs_info->stag),
                    BE_get_name(p_cs_info->drtag),
                    BE_get_name(p_cs_info->rtag));
    }
    fprintf(fid, ",(error_status_t *)&%sIDL_status);\n", state_access);
    fprintf(fid, "if (%sIDL_status!=error_status_ok)\n{\n", state_access);
    fprintf(fid, "DCETHREAD_RAISE(rpc_x_ss_pipe_comm_error);\n}\n");
}

