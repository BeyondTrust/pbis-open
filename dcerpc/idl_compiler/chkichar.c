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
**      chkichar.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  IDL Compiler Semantic Checking for international character support.
**
*/

#include <nidl.h>       /* Standard IDL defs */
#include <checker.h>    /* Type and constant macros */
#include <chkichar.h>   /* Protos for I-char checking */

#include <acf_y.h>  /* ACF token values */
#include <ast.h>        /* Abstract Syntax Tree defs */
#include <astp.h>       /* AST processing defs */
#include <command.h>    /* Command option defs */
#include <errors.h>     /* Error reporting functions */
#include <message.h>    /* reporting functions */


/*
**  C H K _ p a r a m _ c s
**
**  Checks a parameter node's I-char ([cs_*]) attributes.
**  The passed type node is the dereferenced parameter type if the
**  parameter is passed by reference.
*/
void CHK_param_cs
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *type_p         /* [in] Parameter type */
)
{
    AST_parameter_n_t   *pp;
    AST_field_attr_n_t  *fattr_p;

    fattr_p = param_p->field_attrs;

    /* Operation with [in] [cs_char] data requires [cs_stag] parameter */
    if (AST_IN_SET(param_p)
        && (type_p->cs_char_type != NULL
            || FE_TEST(type_p->fe_info->flags, FE_CT_CS_CHAR)))
    {
        for (pp = param_p->uplink->parameters; pp != NULL; pp = pp->next)
        {
            if (AST_CS_STAG_SET(pp))
                break;
        }
        if (pp == NULL)
            CHECKER_acf_error(param_p, NIDL_OPINCSCHAR);
    }

    /* Operation with [out] [cs_char] data requires [cs_rtag] parameter */
    if (AST_OUT_SET(param_p)
        && (type_p->cs_char_type != NULL
            || FE_TEST(type_p->fe_info->flags, FE_CT_CS_CHAR)))
    {
        for (pp = param_p->uplink->parameters; pp != NULL; pp = pp->next)
        {
            if (AST_CS_RTAG_SET(pp))
                break;
        }
        if (pp == NULL)
            CHECKER_acf_error(param_p, NIDL_OPOUTCSCHAR);
    }

    /* A [cs_stag] parameter must have the [in] attribute */
    if (AST_CS_STAG_SET(param_p) && !AST_IN_SET(param_p))
    {
        ASTP_attr_flag_t attr2 = ASTP_IN;
        CHECKER_error(param_p, NIDL_PRMDEPATTR, "cs_stag", 
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr2)));
    }

    /* A [cs_drtag] parameter must have the [in] attribute */
    if (AST_CS_DRTAG_SET(param_p) && !AST_IN_SET(param_p))
    {
        ASTP_attr_flag_t attr2 = ASTP_IN;
        CHECKER_error(param_p, NIDL_PRMDEPATTR, "cs_drtag", 
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr2)));
    }

    /* A [cs_rtag] parameter must have the [out] attribute */
    if (AST_CS_RTAG_SET(param_p) && !AST_OUT_SET(param_p))
    {
        ASTP_attr_flag_t attr2 = ASTP_OUT;
        CHECKER_error(param_p, NIDL_PRMDEPATTR, "cs_rtag", 
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr2)));
    }

    /* Array attr can't be used for both [cs_char] and non-[cs_char] arrays */
    if (FE_TEST(param_p->fe_info->flags, FE_USED_AS_CS_FLD_ATTR)
        && FE_TEST(param_p->fe_info->flags, FE_USED_AS_REG_FLD_ATTR))
        CHECKER_error(param_p, NIDL_ARRATTRSHR);

    /* A [size_is] or [max_is] attr can't be applied to ptr to [cs_char] type */
    if (type_p->kind == AST_pointer_k
        && type_p->type_structure.pointer->pointee_type->cs_char_type != NULL
        && fattr_p != NULL
        && (fattr_p->max_is_vec != NULL || fattr_p->size_is_vec != NULL))
        CHECKER_error(param_p, NIDL_CSARRSYN);

    /* An array with [cs_char] base type cannot have [ptr] or [unique] attrs */
    if (type_p->kind == AST_array_k
        && type_p->type_structure.array->element_type->cs_char_type != NULL
        && (AST_PTR_SET(param_p) || AST_UNIQUE_SET(param_p)))
        CHECKER_error(param_p, NIDL_ARRPTRUNIQ, "cs_char");

    /* A [cs_stag] param must precede any [in] [cs_char] data in a param list */
    if (AST_CS_STAG_SET(param_p))
    {
        for (pp = param_p->uplink->parameters; pp != param_p; pp = pp->next)
        {
            if (AST_IN_SET(pp)
                && (pp->type->cs_char_type != NULL
                    || FE_TEST(pp->type->fe_info->flags, FE_CT_CS_CHAR)))
                break;
        }
        if (pp != param_p)
            CHECKER_error(param_p, NIDL_TAGBEFDATA);
    }

    /* A [cs_rtag] param must precede any [out][cs_char] data in a param list */
    if (AST_CS_RTAG_SET(param_p))
    {
        for (pp = param_p->uplink->parameters; pp != param_p; pp = pp->next)
        {
            if (AST_OUT_SET(pp)
                && (pp->type->cs_char_type != NULL
                    || FE_TEST(pp->type->fe_info->flags, FE_CT_CS_CHAR)))
                break;
        }
        if (pp != param_p)
            CHECKER_error(param_p, NIDL_TAGAFTDATA);
    }

    /* A [handle] binding parameter cannot contain a [cs_char] type */
    if (AST_HANDLE_SET(type_p)
        && param_p == param_p->uplink->parameters   /* first param */
        && (type_p->cs_char_type != NULL
            || FE_TEST(type_p->fe_info->flags, FE_CT_CS_CHAR)))
        CHECKER_error(param_p, NIDL_HANCTYPE, "cs_char");

    /* Arrays of [cs_char] can only use the [size_is] and [length_is] attrs */
    if (type_p->kind == AST_array_k
        && type_p->type_structure.array->element_type->cs_char_type != NULL
        && (AST_STRING_SET(type_p) || AST_STRING_SET(param_p)
            || (fattr_p != NULL
                && (fattr_p->min_is_vec != NULL
                    || fattr_p->max_is_vec != NULL
                    || fattr_p->first_is_vec != NULL
                    || fattr_p->last_is_vec != NULL))))
        CHECKER_error(param_p, NIDL_ARRTYPATTR, "cs_char");

    /* Tag params must have type unsigned long int passed by value or ref */
    if (  (AST_CS_STAG_SET(param_p)
           || AST_CS_DRTAG_SET(param_p)
           || AST_CS_RTAG_SET(param_p))
        && type_p->kind != AST_long_unsigned_k )
        CHECKER_error(param_p, NIDL_TAGPRMTYPE);

    /* Use of [cs_stag] attribute requires -standard extended */
    if (AST_CS_STAG_SET(param_p)
        && (*(int *)CMD_vals[opt_standard] < opt_standard_dce_1_1))
        CHECKER_acf_warning(param_p, NIDL_NOPORTATTR, "cs_stag",
                            OPT_STD_EXTENDED);

    /* Use of [cs_drtag] attribute requires -standard extended */
    if (AST_CS_DRTAG_SET(param_p)
        && (*(int *)CMD_vals[opt_standard] < opt_standard_dce_1_1))
        CHECKER_acf_warning(param_p, NIDL_NOPORTATTR, "cs_drtag",
                            OPT_STD_EXTENDED);

    /* Use of [cs_rtag] attribute requires -standard extended */
    if (AST_CS_RTAG_SET(param_p)
        && (*(int *)CMD_vals[opt_standard] < opt_standard_dce_1_1))
        CHECKER_acf_warning(param_p, NIDL_NOPORTATTR, "cs_rtag",
                            OPT_STD_EXTENDED);
}


/*
**  C H K _ o p _ c s
**
**  Checks an operation node's [cs_tag_rtn] attribute and other attributes
**  relating to I-char support.
*/
void CHK_op_cs
(
    AST_operation_n_t   *op_p           /* [in] Ptr to AST operation node */
)
{
    AST_parameter_n_t   *param_p;
    int s=0, d=0, r=0;

    /* Operation with [cs_tag_rtn] contains no codeset tag parameters */
    if (op_p->cs_tag_rtn_name != NAMETABLE_NIL_ID)
    {
        for (param_p = op_p->parameters; param_p; param_p = param_p->next)
        {
            if (AST_CS_STAG_SET(param_p) || AST_CS_DRTAG_SET(param_p)
                || AST_CS_RTAG_SET(param_p))
                break;
        }
        if (param_p == NULL)
            CHECKER_acf_warning(op_p, NIDL_OPNOTAGS);
    }

    /* The [name] attribute cannot be duplicated in the same parameter list */
    for (param_p = op_p->parameters; param_p; param_p = param_p->next)
    {
        if (AST_CS_STAG_SET(param_p)) s++;
        if (AST_CS_DRTAG_SET(param_p)) d++;
        if (AST_CS_RTAG_SET(param_p)) r++;
    }
    if (s > 1)
        CHECKER_acf_error(op_p, NIDL_DUPPRMATTR, "cs_stag");
    if (d > 1)
        CHECKER_acf_error(op_p, NIDL_DUPPRMATTR, "cs_drtag");
    if (r > 1)
        CHECKER_acf_error(op_p, NIDL_DUPPRMATTR, "cs_rtag");

    /* Use of [cs_tag_rtn] attribute requires -standard extended */
    if (op_p->cs_tag_rtn_name != NAMETABLE_NIL_ID
        && (*(int *)CMD_vals[opt_standard] < opt_standard_dce_1_1))
        CHECKER_acf_warning(op_p, NIDL_NOPORTATTR, "cs_tag_rtn",
                            OPT_STD_EXTENDED);
}


/*
**  C H K _ f i e l d _ c s
**
**  Checks a structure field node's I-char ([cs_*]) attributes.
*/
void CHK_field_cs
(
    AST_field_n_t       *field_p        /* [in] Ptr to AST field node */
)
{
    AST_type_n_t        *type_p;
    AST_field_attr_n_t  *fattr_p;

    type_p = field_p->type;
    fattr_p = field_p->field_attrs;

    /* Array attr cannot be used for both [cs_char] and non-[cs_char] arrays */
    if (FE_TEST(field_p->fe_info->flags, FE_USED_AS_CS_FLD_ATTR)
        && FE_TEST(field_p->fe_info->flags, FE_USED_AS_REG_FLD_ATTR))
        CHECKER_error(field_p, NIDL_ARRATTRSHR);

    /* A [size_is] or [max_is] attr can't be applied to ptr to [cs_char] type */
    if (type_p->kind == AST_pointer_k
        && type_p->type_structure.pointer->pointee_type->cs_char_type != NULL
        && fattr_p != NULL
        && (fattr_p->max_is_vec != NULL || fattr_p->size_is_vec != NULL))
        CHECKER_error(field_p, NIDL_CSARRSYN);

    /* Arrays of [cs_char] can only use the [size_is] and [length_is] attrs */
    if (type_p->kind == AST_array_k
        && type_p->type_structure.array->element_type->cs_char_type != NULL
        && (AST_STRING_SET(type_p) || AST_STRING_SET(field_p)
            || (fattr_p != NULL
                && (fattr_p->min_is_vec != NULL
                    || fattr_p->max_is_vec != NULL
                    || fattr_p->first_is_vec != NULL
                    || fattr_p->last_is_vec != NULL))))
        CHECKER_error(field_p, NIDL_ARRTYPATTR, "cs_char");
}


/*
**  C H K _ p i p e _ b a s e _ t y p e _ c s
**
**  Checks a pipe node's base data type for [cs_char] attribute.
*/
void CHK_pipe_base_type_cs
(
    AST_pipe_n_t        *pipe_p,        /* [in] Ptr to AST pipe node */
    AST_interface_n_t   *int_p ATTRIBUTE_UNUSED         /* [in] Ptr to interface node */
)
{
    AST_type_n_t        *type_p;        /* Pipe base data type node */

    type_p = pipe_p->base_type;

    /* The base type of a pipe cannot be or contain a [cs_char] type */
    if (type_p->cs_char_type != NULL
        || FE_TEST(type_p->fe_info->flags, FE_CT_CS_CHAR))
        CHECKER_error(type_p, NIDL_PIPECTYPE, "cs_char");
}


/*
**  C H K _ t y p e _ c s
**
**  Checks a type node's [cs_char] attribute.
*/
void CHK_type_cs
(
    AST_type_n_t        *top_type_p,    /* [in] Top-level presented type */
    AST_type_n_t        *type_p,        /* [in] Ptr to AST type node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)
{
    char const          *type_name;     /* Type name */
    int                 type_len;       /* Length of type name */
    int                 max_len;        /* Maximum identifier length */

    /* Types with the [cs_char] attribute cannot be nested */
    if (type_p->cs_char_type != NULL
        && FE_TEST(type_p->fe_info->flags, FE_CT_CS_CHAR))
        CHECKER_acf_error(type_p, NIDL_TYPENEST, "cs_char");

    /* A [transmit_as] transmitted type cannot contain a [cs_char] type */
    if (type_p->cs_char_type != NULL
        && FE_TEST(type_p->fe_info->flags, FE_USED_IN_TRANSMITTED))
        CHECKER_error(type_p, NIDL_XMITCTYPE, "cs_char");

    /* Arrays of [cs_char] type cannot be multidimensional */
    if (type_p->kind == AST_array_k
        && type_p->type_structure.array->element_type->cs_char_type != NULL
        && type_p->type_structure.array->index_count > 1)
        CHECKER_error(type_p, NIDL_ARRMULTID, "cs_char");

    /* Type with [cs_char] cannot be or contain type with [transmit_as] */
    if (top_type_p->cs_char_type != NULL
        && (top_type_p->xmit_as_type != NULL
            || FE_TEST(top_type_p->fe_info->flags, FE_HAS_XMIT_AS)) )
    {
        ASTP_attr_flag_t attr2 = ASTP_TRANSMIT_AS;
        CHECKER_error(top_type_p, NIDL_TYPECTYPE, "cs_char",
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr2)));
    }

    /* Type with [cs_char] cannot be or contain type with [transmit_as] */
    if (type_p->cs_char_type != NULL
        && (type_p->rep_as_type != NULL
            || FE_TEST(type_p->fe_info->flags, FE_HAS_REP_AS)) )
        CHECKER_error(type_p, NIDL_TYPECTYPE, "cs_char", "represent_as");

    if (type_p->cs_char_type != NULL
        && type_p->kind != AST_byte_k
        && !(type_p->kind == AST_structure_k
             && CHK_struct_is_all_byte_fields(type_p->type_structure.structure)))
        CHECKER_error(type_p, NIDL_TYPEOFBYTES, "cs_char");

    /* ACF 'include' statement advised for definition of type 'name' */
    if (int_p->includes == NULL
        && type_p->cs_char_type != NULL
        && ASTP_lookup_binding(type_p->cs_char_type->type_name,
                               fe_type_n_k, FALSE) == NULL)
    {
        char const *id_name;
        NAMETABLE_id_to_string(type_p->cs_char_type->type_name, &id_name);
        CHECKER_acf_warning(type_p, NIDL_INCLTYPE, id_name);
    }

    /* Maximum identifier length for [cs_char] type is 'n' characters */
    if (type_p->cs_char_type != NULL
        && type_p->name != NAMETABLE_NIL_ID)
    {
        NAMETABLE_id_to_string(type_p->name, &type_name);
        type_len = strlen(type_name);
        max_len = MAX_ID - strlen("_from_netcs");
        if (type_len > max_len)
            CHECKER_error(type_p, NIDL_MAXIDTYPE, "cs_char", max_len);
    }

    /* Use of [cs_char] attribute requires -standard extended */
    if (type_p->cs_char_type != NULL
        && (*(int *)CMD_vals[opt_standard] < opt_standard_dce_1_1))
        CHECKER_acf_warning(type_p, NIDL_NOPORTATTR, "cs_char",
                            OPT_STD_EXTENDED);
}
