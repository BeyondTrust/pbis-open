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
**  NAME
**
**      ASTP_SIM.C
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Builds the Abstract Syntax Tree (AST) representation for
**      the interface.  Contains building of all exported items
**      except for the complex types structures and unions.
**
**  VERSION: DCE 1.0
**
*/

#include <nidl.h>
#include <nametbl.h>
#include <errors.h>
#include <astp.h>
#include <nidlmsg.h>
#include <nidl_y.h>

extern int nidl_yylineno;

/*
 * Prototypes
 */
char *KEYWORDS_lookup_text (
    long    token
);


static void AST_synthesize_param_to_oper_attr (
    AST_parameter_n_t *parameter_node,
    AST_operation_n_t *operation_node,
    ASTP_parameter_count_t *param_count
);

static void AST_set_oper_has_ins_outs (
    AST_operation_n_t *operation_node,
    ASTP_parameter_count_t *param_count
);



/*
 *
 *  A S T _ a r r a y _ b o u n d _ i n f o
 *  =======================================
 */

ASTP_type_attr_n_t *AST_array_bound_info
(
    NAMETABLE_id_t name,
    ASTP_attr_k_t  kind,
    boolean is_pointer

)
{
    ASTP_type_attr_n_t *attr_node_p;

    attr_node_p = NEW (ASTP_type_attr_n_t);

    attr_node_p->is_expr = false;
	 
    attr_node_p->kind = kind ;
    attr_node_p->b.simple.name = name;
    attr_node_p->b.simple.pointer = is_pointer;

    attr_node_p->source_line = nidl_yylineno ;

    return attr_node_p ;
}

ASTP_type_attr_n_t * AST_array_bound_from_expr(
		  AST_exp_n_t * exp,
		  ASTP_attr_k_t kind
)
{
    ASTP_type_attr_n_t *attr_node_p;
	 
    attr_node_p = NEW (ASTP_type_attr_n_t);

    attr_node_p->is_expr = true;

    attr_node_p->kind = kind ;
    attr_node_p->b.expr = exp;

    attr_node_p->source_line = nidl_yylineno ;

    return attr_node_p ;
}

ASTP_type_attr_n_t * AST_range_from_expr(
		  AST_exp_n_t * exp_low_val,
		  AST_exp_n_t * exp_high_val
)
{
    ASTP_type_attr_n_t *attr_node_p;
	 
    attr_node_p = NEW (ASTP_type_attr_n_t);

    attr_node_p->is_expr = true;

    attr_node_p->kind = range_k;
    attr_node_p->b.expr = AST_expression(AST_EXP_TUPLE,
                                         exp_low_val,
                                         exp_high_val,
                                         NULL);

    attr_node_p->source_line = nidl_yylineno;

    return attr_node_p;
}

/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/


/*
 *
 *  A S T _ b o o l e a n _ c o n s t a n t
 *  =======================================
 */

AST_constant_n_t *AST_boolean_constant
(
    boolean value
)
{
    AST_constant_n_t  *const_node_ptr;

    const_node_ptr = AST_constant_node(AST_boolean_const_k);
    const_node_ptr->value.boolean_val = value;

    return const_node_ptr;
}

/*---------------------------------------------------------------------*/

/*
 *
 *  A S T _ c h a r  _ c o n s t a n t
 *  ==================================
 *
 */

AST_constant_n_t *AST_char_constant
(
    char value

)
{
    AST_constant_n_t  *const_node_ptr;

    const_node_ptr = AST_constant_node(AST_char_const_k);
    const_node_ptr->value.char_val = value;

    return const_node_ptr;
}

/*---------------------------------------------------------------------*/


/*
 *  A S T _ c o n s t a n t _ n o d e
 *  =================================
 *
 *  Create and initialize a constant node.
 *
 */

AST_constant_n_t *AST_constant_node
(
    AST_constant_k_t kind
)
{
    AST_constant_n_t *constant_node_p;

    constant_node_p = NEW (AST_constant_n_t);

    constant_node_p->name = NAMETABLE_NIL_ID;
    constant_node_p->kind = kind;

    ASTP_set_fe_info (&constant_node_p->fe_info, fe_constant_n_k);

    return constant_node_p;
}

/*---------------------------------------------------------------------*/


/*
 *  A S T _ c r e a t e _ o p e r a t i o n _ n o d e
 *  =================================================
 *
 *  Allocate and initialize an operation node
 *
 */
static AST_operation_n_t *AST_create_operation_node
(
    NAMETABLE_id_t op_name,
    AST_parameter_n_t *parameters
)
{
    AST_operation_n_t *operation_node_p;

    operation_node_p = NEW (AST_operation_n_t);
    ASTP_set_fe_info (&operation_node_p->fe_info, fe_operation_n_k);

    operation_node_p->name = op_name;
    operation_node_p->parameters = parameters;

    return(operation_node_p);
}

/*---------------------------------------------------------------------*/

/*
 *  A S T _ d e c l a r a t o r _ n o d e
 *  =====================================
 *
 *
 *  Create and initialize a private declarator node which is later collapsed to
 *  the appropriate type node.  The declarator node represents only a named
 *  declarator.  Any complexities (arrays, pointers, etc) are linked in a list
 *  of ASTP_declarator_op_n_t nodes.  These are in the order that they are to
 *  be applied to the base type.
 *
 *  Inputs:
 *      name -- Name for the declarator.
 */

ASTP_declarator_n_t *AST_declarator_node
(
    NAMETABLE_id_t name
)
{
    ASTP_declarator_n_t  *declarator_node_ptr;

    declarator_node_ptr = NEW (ASTP_declarator_n_t );

    declarator_node_ptr->name = name;

    return declarator_node_ptr;

}

/*---------------------------------------------------------------------*/


/*
 *  A S T _ d e c l a r a t o r _ o p e r a t i o n
 *  ===============================================
 *
 *
 *  Create and initialize a private declarator op
 *  node and push it on the operations list of the
 *  specified declarator.
 *
 *  Inputs:
 *      declarator -- Declarator that is modified by the operations.
 *      op_kind -- on of: AST_array_k, AST_pointer_k, or AST_function_k
 *                  which specifies the meaning of this declarator.
 *      op_node -- ASTP_node_t for arrays or function ptrs.
 *      pointer_count -- Number of levels of indirection (number of *'s)
 */


void AST_declarator_operation
(
    ASTP_declarator_n_t     *declarator,
    AST_type_k_t            op_kind,
    ASTP_node_t             *op_info,
    int                     pointer_count
)
{
    ASTP_declarator_op_n_t  *declarator_op;

    /*
     * Concatenate multiple array indices into one AST_array_k declarator op
     * and return instead of making seperator declarator operator node.
     */
     if ((op_kind == AST_array_k) && (declarator->next_op != NULL) &&
         (declarator->next_op->op_kind == AST_array_k))
     {
        declarator->next_op->op_info.indices = (ASTP_array_index_n_t *)
            AST_concat_element ((ASTP_node_t *)declarator->next_op->op_info.indices,
                                (ASTP_node_t *)op_info);
        return;
     }


    /*
     * Create and initialize the new declarator operation
     */
    declarator_op = NEW (ASTP_declarator_op_n_t);
    declarator_op->op_kind = op_kind;

    /*
     * fill in the op info based upon op kind
     */
    if (op_kind == AST_pointer_k)
        declarator_op->op_info.pointer_count = pointer_count;
    else
        declarator_op->op_info.node = op_info;


    /*
     * Link it into the operation list for the specified declarator
     */
     declarator_op->next_op = declarator->next_op;
     declarator->next_op = declarator_op;

    /*
     * Set the last_op pointer if not yet set.
     */
    if (declarator->last_op == NULL)
        declarator->last_op = declarator->next_op;

    return;
}

/*---------------------------------------------------------------------*/


/*
 *  A S T _ d e c l a r a t o r _ t o _ p a r a m s
 *  ===============================================
 *
 *  Collapse a declarator node to a parameter node.
 *  This involves setting the appropriate parameter flags,
 *  field attribute flags, and type.
 *
 *  Inputs:
 *      array_bounds    Pointer to the temporary array bounds list
 *                      describing first_is, and friends.
 *      type            Pointer to the type node of the parameter.
 *      declarator      Pointer to the temporary declarator structure
 *                      for the parameter.
 *
 *  Outputs:
 *      None.
 *
 *  Function Value:
 *      Pointer to the new parameter node.
 */

AST_parameter_n_t  *AST_declarator_to_param
(
    ASTP_attributes_t   *attributes,
    AST_type_n_t            *type,
    ASTP_declarator_n_t *declarator
)
{
    AST_parameter_n_t   *new_parameter;
    AST_type_n_t        *new_type;

    /*
     * Check that the base type is not an anonymous struct/union
     */
    if (!AST_DEF_AS_TAG_SET(type) &&
        ((type->kind == AST_disc_union_k) ||
         (type->kind == AST_structure_k)) &&
        (type->name == NAMETABLE_NIL_ID))
    {
        char const *identifier;
        NAMETABLE_id_to_string(declarator->name, &identifier);
        log_error(nidl_yylineno, NIDL_ANONTYPE, identifier, NULL);
    }

    /*
     * Propagate the parameter declarator to the type.
     * Note, for complex declarators, a new type will be created.
     * For simple declarators, the new type is the type passed.
     */
    new_parameter = AST_parameter_node (declarator->name);
    new_type = AST_propagate_type(type, declarator, attributes,
                                  (ASTP_node_t *)new_parameter);
    new_parameter->type = new_type;
    ASTP_validate_forward_ref(new_type);


    /* Set field attributes, if specified. */
    new_parameter->field_attrs =
        AST_set_field_attrs(attributes, (ASTP_node_t *)new_parameter,
                            new_parameter->type);


    /*
     * Set the boolean attributes parsed in the source.
     */
    AST_set_flags(&new_parameter->flags, (ASTP_node_t *)new_parameter, attributes);

	/* set the iid_is name if specified */
	 if (attributes)
		  new_type->iid_is_name = attributes->iid_is_name;
	 
    /* Now bind the new parameter name to the parameter node */
    ASTP_add_name_binding (new_parameter->name, new_parameter);

    /* Free the declarator node */
    ASTP_free_declarators(declarator);

    return new_parameter;
}

/*-------------------------------------------------------------------*/


/*
 *  A S T _ e n u m e r a t o r _ n o d e
 *  =======================================
 *
 *  This routine returns an enumerator node
 *  containing the list of constants defined
 *  by this enumerator.
 *
 *  Inputs:
 *      constant_list -- list of constants in this enum
 *      size -- integer size for this enum
 *
 *  Returns:
 *      Type node of appropriately sized enum
 */

AST_type_n_t *AST_enumerator_node
(
    AST_constant_n_t *constant_list,
    AST_type_k_t size ATTRIBUTE_UNUSED
)
{
    AST_enumeration_n_t *enum_node_ptr;
    AST_type_n_t *type_node_ptr;
    unsigned int N = 0;
    AST_constant_n_t *cp;
    int overflow = FALSE;

    /*
     * Loop through the constant nodes and assign them integer values from
     * 0 through N.
     */
    for (cp = constant_list; cp; cp = cp->next)
    {
        /*
        ** Set the value of the enum element, if we overflow set a flag
        */
		  if (cp->value.int_val != 0)
				N = cp->value.int_val + 1;
		  else
				cp->value.int_val = N++;
        if ((unsigned int)N >= ASTP_C_SHORT_MAX) overflow = TRUE;
    }

    /*
     * If there was an overflow, log the error message
     */
    if (overflow)
        log_error(nidl_yylineno, NIDL_TOOMANYELEM, "enum", NULL) ;


    /*
     * Allocate and initialize the enumeration node
     */
    enum_node_ptr = NEW (AST_enumeration_n_t);
    type_node_ptr = AST_type_node(AST_enum_k);


    /*
     * Initialize enum node contents
     */
     enum_node_ptr->enum_constants = constant_list;


    /*
     * Initialize type node contents
     */
    type_node_ptr->type_structure.enumeration = enum_node_ptr;


    /*
     * Set the source information
     */
    ASTP_set_fe_info (&enum_node_ptr->fe_info, fe_enumeration_n_k);


    /*
     * Return the type node
     */
    return type_node_ptr;
}


/*---------------------------------------------------------------------*/

/*
 *  A S T _ e x p o r t _ n o d e
 *  =============================
 *
 *  Creates and initializes a single export node
 *  given a single constant, operation, or type.
 *
 */

AST_export_n_t *AST_export_node
(
    ASTP_node_t *export_ptr,
    AST_export_k_t kind
)
{
    AST_export_n_t *export_node_ptr;

    /* If no export, return NULL */
    if (export_ptr == NULL) return NULL;

    /* Build export node */
    export_node_ptr = NEW (AST_export_n_t);

    export_node_ptr->kind = kind;

    switch (kind)
    {
        case AST_cpp_quote_k:
            export_node_ptr->thing_p.exported_cpp_quote =
                (AST_cpp_quote_n_t *) export_ptr;
            break;
        case AST_constant_k:
            export_node_ptr->thing_p.exported_constant =
                                        (AST_constant_n_t *) export_ptr;
            break;

        case AST_operation_k:
            export_node_ptr->thing_p.exported_operation  =
                                        (AST_operation_n_t *) export_ptr;
            break;

        case AST_type_k:
            export_node_ptr->thing_p.exported_type =
                                        (AST_type_n_t *) export_ptr;
            break;

        default:
            break;
    }

    ASTP_set_fe_info (&export_node_ptr->fe_info, fe_export_n_k);

    return export_node_ptr;
}

/*---------------------------------------------------------------------*/

/*
 *  A S T _ c p p _ q u o t e _ n o d e
 *  =============================
 *
 *  Creates and initializes a single cpp_quote node
 *  given a block of text
 *
 */

AST_cpp_quote_n_t *AST_cpp_quote_node
(
    STRTAB_str_t text
)
{
    AST_cpp_quote_n_t *cpp_quote_node_ptr;

    /* Build cpp_quote node */
    cpp_quote_node_ptr = NEW (AST_cpp_quote_n_t);
    cpp_quote_node_ptr->text = text;
    
    ASTP_set_fe_info (&cpp_quote_node_ptr->fe_info, fe_cpp_quote_n_k);

    return cpp_quote_node_ptr;
}

/*---------------------------------------------------------------------*/

/*
 *  A S T _ f i n i s h _ c o n s t a n t _ n o d e
 *  ===============================================
 *
 *  Propagates the declarator to the type node.
 *  Clones the constant if a named constant, replicating
 *  the value and the defined_as field.
 *  Finishes filling in the constant node with the name.
 *  And finally, checks that the constant type is
 *  equivalent to the type specifier before binding
 *  the name to the node.
 *
 */

AST_constant_n_t *AST_finish_constant_node
(
    AST_constant_n_t *constant_ptr,     /* Pointer to constant node */
    ASTP_declarator_n_t *declarator,    /* Constant identifier  */
    AST_type_n_t *type_ptr              /* Pointer to type node */
)
{
    boolean type_check_failed = false;  /* Boolean for type check */
    AST_type_n_t *result_type;          /* Result of applying declarator */
    AST_constant_n_t *return_constant;  /* Returned constant node */
    ASTP_attributes_t no_attrs;         /* No Attrributes set */

    /* Propagate the declarator to a type node */
    no_attrs.bounds = NULL;
    no_attrs.attr_flags = ASTP_PTR;
	 no_attrs.iid_is_name = NAMETABLE_NIL_ID;
    result_type = AST_propagate_type(type_ptr, declarator,
                                        &no_attrs,
                                        (ASTP_node_t *)constant_ptr);

    /* Clone the constant if a named type */
    if (constant_ptr->name != NAMETABLE_NIL_ID)
    {
        return_constant = AST_clone_constant(constant_ptr);
        return_constant->defined_as = constant_ptr;
    }
    else
    {
        return_constant = constant_ptr;
    }

    return_constant->name = declarator->name;

    /* Add constant name to nametable and bind to constant node */
    ASTP_add_name_binding (return_constant->name, return_constant);

    switch (return_constant->kind)
    {
        case AST_nil_const_k:
            if (!((result_type->kind == AST_pointer_k) &&
                (result_type->type_structure.pointer->pointee_type->kind ==
                    AST_void_k)))
            {
                type_check_failed = true;
            }
            break;

        case AST_int_const_k:
            switch (result_type->kind)
            {
                case AST_small_integer_k:
                    if ((constant_ptr->value.int_val > ASTP_C_SMALL_MAX) ||
                        (constant_ptr->value.int_val < ASTP_C_SMALL_MIN))
                    {
                        log_error(nidl_yylineno, NIDL_INTOVERFLOW,
                            KEYWORDS_lookup_text(SMALL_KW), NULL);
                    }
                    break;
                case AST_small_unsigned_k:
                    if ((constant_ptr->value.int_val > ASTP_C_USMALL_MAX) ||
                        (constant_ptr->value.int_val < ASTP_C_USMALL_MIN))
                    {
                        log_error(nidl_yylineno, NIDL_INTOVERFLOW,
                            KEYWORDS_lookup_text(SMALL_KW), NULL);
                    }
                    break;
                case AST_short_integer_k:
                    if ((constant_ptr->value.int_val > ASTP_C_SHORT_MAX) ||
                        (constant_ptr->value.int_val < ASTP_C_SHORT_MIN))
                    {
                        log_error(nidl_yylineno, NIDL_INTOVERFLOW,
                            KEYWORDS_lookup_text(SHORT_KW), NULL);
                    }
                    break;
                case AST_short_unsigned_k:
                    if ((constant_ptr->value.int_val > ASTP_C_USHORT_MAX) ||
                        (constant_ptr->value.int_val < ASTP_C_USHORT_MIN))
                    {
                        log_error(nidl_yylineno, NIDL_INTOVERFLOW,
                            KEYWORDS_lookup_text(SHORT_KW), NULL);
                    }
                    break;
                case AST_long_integer_k:
                    if ((constant_ptr->value.int_val > ASTP_C_LONG_MAX) ||
                        (constant_ptr->value.int_val < ASTP_C_LONG_MIN))
                    {
                        log_error(nidl_yylineno, NIDL_INTOVERFLOW,
                            KEYWORDS_lookup_text(LONG_KW), NULL);
                    }
                    break;
                case AST_long_unsigned_k:
                    if (((unsigned int)constant_ptr->value.int_val > ASTP_C_ULONG_MAX)
#if ASTP_C_ULONG_MIN > 0
								  || ((unsigned int)constant_ptr->value.int_val < ASTP_C_ULONG_MIN)
#endif
								)
                    {
                        log_error(nidl_yylineno, NIDL_INTOVERFLOW,
                            KEYWORDS_lookup_text(LONG_KW), NULL);
                    }
                    break;

                case AST_hyper_integer_k:
                case AST_hyper_unsigned_k:
                    /* Not currently supported */
                    log_error (nidl_yylineno, NIDL_HYPERCONST, NULL);
                    break;

                default:
                    type_check_failed = true;
                    break;
            }
            break;

        case AST_hyper_int_const_k:
            if (!(result_type->kind == AST_hyper_integer_k) ||
                 (result_type->kind == AST_hyper_unsigned_k))
            {
                type_check_failed = true;
            }

            break;

        case AST_char_const_k:
            if (!(result_type->kind == AST_character_k))
            {
                type_check_failed = true;
            }
            break;

        case AST_string_const_k:
            if (!((result_type->kind == AST_pointer_k) &&
                  (result_type->type_structure.pointer->pointee_type->kind ==
                    AST_character_k)))
            {
                type_check_failed = true;
            }
            break;

        case AST_boolean_const_k:
            if (!(result_type->kind == AST_boolean_k))
            {
                type_check_failed = true;
            }
            break;

        default:
            break;

    }

    if (type_check_failed == true)
    {
        log_error (nidl_yylineno, NIDL_CONSTTYPE, NULL);
    }

    /* Free the declarator node */
    ASTP_free_declarators(declarator);

    return return_constant;
}

/*---------------------------------------------------------------------*/

/*
 *  A S T _ f i n i s h _ o p e r a t i o n _ n o d e
 *  =================================================
 *
 *  Uplink the parameters to the operation node
 *  and set the operation's synthesized attributes.
 *
 */

static void AST_finish_operation_node
(
    AST_operation_n_t *operation_node_p
)
{
    AST_parameter_n_t *param_p;
    ASTP_parameter_count_t param_count;

    /*
     *  Operation results always indicate new storage, so if a result is a
     *  pointer then set the PTR attribute on the parameter node for the return
     *  type if neigher REF or UNIQUE attributes were not specified.  Also
     *  don't set it if it is a "void *" because either it has [context_handle] or
     *  checker will issue an error anyway.
	  *  Don't set it for "interface*" types
     */
    if ((operation_node_p->result->type->kind == AST_pointer_k) &&
        (operation_node_p->result->type->type_structure.pointer->pointee_type->kind != AST_void_k) &&
        (operation_node_p->result->type->type_structure.pointer->pointee_type->kind != AST_interface_k) &&
        !AST_REF_SET(operation_node_p->result) &&
        !AST_UNIQUE_SET(operation_node_p->result))
          AST_SET_PTR (operation_node_p->result);


    /*
     * Traverse through list of parameter nodes filling
     * in uplink field to the operation node
     * (Uplink from result parameter was done above.)
     */
    for (param_p = operation_node_p->parameters,
         param_count.in_params= 0, param_count.out_params=0;
         param_p != (AST_parameter_n_t *) NULL;
         param_p = param_p->next)
    {
        param_p->uplink = operation_node_p;
        AST_synthesize_param_to_oper_attr(param_p, operation_node_p,
                                            &param_count);
    }

    /* Now further adjust HAS_INS and HAS_OUTS */
    AST_set_oper_has_ins_outs(operation_node_p, &param_count);
    return;
}

/*
 *  A S T _ f u n c t i o n _ p t r _ n o d e
 *  =========================================
 *
 *  A function ptr type points to an operation
 *  node.  This routine is very similar to
 *  AST_operation_node() but differs
 *  primarily due to the fact that a function
 *  pointer declaration does not count
 *  as an exported operation.  Also,
 *  the attributes are somewhat different
 *  although not apparent at the current time.
 *
 *  Called from AST_propagate_type and AST_propagate_typedefs.
 */

AST_operation_n_t *AST_function_node
(
    AST_type_n_t          *result_type,
    NAMETABLE_id_t        op_name,
    AST_parameter_n_t *parameters
)
{
    AST_operation_n_t *operation_node_p;

    /* Create and initialize the operation node */
    operation_node_p = AST_create_operation_node(op_name, parameters);
    operation_node_p->result = AST_parameter_node (operation_node_p->name);
    (operation_node_p->result)->type = result_type;
    (operation_node_p->result)->uplink = operation_node_p;

    /*
     * Propagate type attributes to parameter node for op result
     */
    if (AST_REF_SET(result_type)) AST_SET_REF(operation_node_p->result);
    if (AST_PTR_SET(result_type)) AST_SET_PTR(operation_node_p->result);
    if (AST_UNIQUE_SET(result_type)) AST_SET_UNIQUE(operation_node_p->result);

    /*
     * Link parameters to operation node
     * Merge in any synthesized attributes (has_ins, has_outs, has_in_ctx, etc.).
     */
    AST_finish_operation_node(operation_node_p);

    return operation_node_p;
}

/*---------------------------------------------------------------------*/

/*
 *  A S T _ i n c l u d e _ n o d e
 *  =============================
 *
 *  Creates and initializes an include node.
 *
 */

AST_include_n_t *AST_include_node
(
    STRTAB_str_t include_file,
    STRTAB_str_t include_file_name
)
{
    AST_include_n_t *include_node_ptr;

    include_node_ptr = NEW (AST_include_n_t);
    include_node_ptr->file_name = include_file;
    include_node_ptr->simple_file_name = include_file_name;
    ASTP_set_fe_info (&include_node_ptr->fe_info, fe_include_n_k);

    return include_node_ptr;
}

/*--------------------------------------------------------------------*/

/*
 *  A S T _ i m p o r t _ n o d e
 *  =============================
 *
 *  Creates and initializes an import node.
 *
 */

AST_import_n_t *AST_import_node
(
    STRTAB_str_t imported_file
)
{
    AST_import_n_t *import_node_ptr;

    import_node_ptr = NEW (AST_import_n_t);

    import_node_ptr->file_name = imported_file;

    ASTP_set_fe_info (&import_node_ptr->fe_info, fe_import_n_k);

    return import_node_ptr;
}

/*--------------------------------------------------------------------*/

/*
 *  A S T _ e x c e p t i o n _ n o d e
 *  ===================================
 *
 *  Creates and initializes an exception node.
 *
 */

AST_exception_n_t *AST_exception_node
(
    NAMETABLE_id_t  excep_name
)
{
    ASTP_node_t         *binding;
    AST_exception_n_t   *excep_node_ptr;

    binding = (ASTP_node_t *)NAMETABLE_lookup_binding(excep_name);
    if (binding != NULL)
    {
        char const *identifier;
        NAMETABLE_id_to_string(excep_name, &identifier);
        if ((binding->fe_info != (fe_info_t *)NULL) &&
            (binding->fe_info->source_line != 0) &&
            (binding->fe_info->file != STRTAB_NULL_STR))
        {
            char const *filename;
            STRTAB_str_to_string(binding->fe_info->file, &filename);
            log_error(nidl_yylineno, NIDL_NAMEPREVDECLAT, identifier,
                      filename, binding->fe_info->source_line, NULL);
        }
        else
            log_error(nidl_yylineno, NIDL_NAMEALRDEC, identifier, NULL);
        return NULL;
    }

    excep_node_ptr = NEW (AST_exception_n_t);
    excep_node_ptr->name = excep_name;
    ASTP_set_fe_info (&excep_node_ptr->fe_info, fe_exception_n_k);

    NAMETABLE_add_binding(excep_name, (char *)excep_node_ptr);

    return excep_node_ptr;
}

/*---------------------------------------------------------------------*/

/*
 *  A S T _ n a m e _ n o d e
 *  =============================
 *
 *  Creates and initializes an AST_name_n_t node.
 *
 */

AST_name_n_t *AST_name_node
(
    NAMETABLE_id_t  name
)
{
    AST_name_n_t *name_node_ptr;

    name_node_ptr = NEW (AST_name_n_t);
    name_node_ptr->name = name;
    /* This node doesn't carry any fe_info */

    return name_node_ptr;
}


/*
 *  A S T _ i n i t
 *  ===============
 *
 *  Abstract Syntax Tree (AST) Initialization routine.
 *
 */

void AST_init
(void)
{
    /* Initialize a 0 constants */
    zero_constant_p = AST_integer_constant (0L);

    /* Add default union tag to name table */
    ASTP_tagged_union_id = NAMETABLE_add_id ("tagged_union");

    /* Initialize nodes for each of the base types */
    ASTP_char_ptr = AST_type_node(AST_character_k);
    ASTP_boolean_ptr = AST_type_node(AST_boolean_k);
    ASTP_byte_ptr = AST_type_node(AST_byte_k);
    ASTP_void_ptr = AST_type_node(AST_void_k);
    ASTP_handle_ptr = AST_type_node(AST_handle_k);
    ASTP_short_float_ptr = AST_type_node(AST_short_float_k);
    ASTP_long_float_ptr = AST_type_node(AST_long_float_k);
    ASTP_small_int_ptr = AST_type_node(AST_small_integer_k);
    ASTP_short_int_ptr = AST_type_node(AST_short_integer_k);
    ASTP_long_int_ptr =  AST_type_node(AST_long_integer_k);
    ASTP_hyper_int_ptr = AST_type_node(AST_hyper_integer_k);
    ASTP_small_unsigned_ptr = AST_type_node(AST_small_unsigned_k);
    ASTP_short_unsigned_ptr = AST_type_node(AST_short_unsigned_k);
    ASTP_long_unsigned_ptr =  AST_type_node(AST_long_unsigned_k);
    ASTP_hyper_unsigned_ptr = AST_type_node(AST_hyper_unsigned_k);

    return;
}

/*---------------------------------------------------------------------*/


/*
 *  A S T _ i n t e g e r _ c o n s t a n t
 *  =====================================
 */

AST_constant_n_t *AST_integer_constant
(
    long int    value
)
{
    AST_constant_n_t *constant_node_p;

    constant_node_p = AST_constant_node (AST_int_const_k);

    constant_node_p->value.int_val = value;

    /*
     * Qualify the constant type for Checker
     */
    constant_node_p->fe_info->fe_type_id = fe_const_info;
    constant_node_p->fe_info->type_specific.const_kind = fe_int_const_k;

    return constant_node_p;
}


/*---------------------------------------------------------------------*/



/*
 *  A S T _ i n t e r f a c e _ n o d e
 *  ===================================
 *
 *  Create and initialize an interface node
 *
 *  Implicit Inputs:
 *      ASTP_pa_types_list -- The list of pointed at types for this interface.
 *
 */

AST_interface_n_t *AST_interface_node
(
      void
)
{
    AST_interface_n_t *interface_node_p;

    interface_node_p = NEW (AST_interface_n_t);

    ASTP_set_fe_info (&interface_node_p->fe_info, fe_interface_n_k);
    ASTP_CLR_IF_AF(interface_node_p);

    /*
     *  Default interface attributes.
     *  These will be filled in later when we process
     *  the interface attributes.
     */
    interface_node_p->name = NAMETABLE_NIL_ID;
    interface_node_p->implicit_handle_name = NAMETABLE_NIL_ID;

    return interface_node_p;
}

/*---------------------------------------------------------------------*/

/*
 *  A S T _ l o o k u p _ i n t e g e r _ t y p e _ n o d e
 *  =======================================================
 *
 *  Lookup up the base integer type after first
 *  figuring out the type of integer using the
 *  global variables int_size and int_signed set
 *  by the grammer.
 *
 */

AST_type_n_t *AST_lookup_integer_type_node
(
      AST_type_k_t    int_size,
      int             int_signed
)
{
    AST_type_n_t *type_node_p = NULL;

    /* Allocate a type node according to the integer type modifiers */
    if (int_signed)
       type_node_p = AST_lookup_type_node ((AST_type_k_t)int_size);
    else
        switch (int_size)
        {
        case AST_small_integer_k:
            type_node_p = AST_lookup_type_node (AST_small_unsigned_k);
            break;
        case AST_short_integer_k:
            type_node_p = AST_lookup_type_node (AST_short_unsigned_k);
            break;
        case AST_long_integer_k:
            type_node_p = AST_lookup_type_node (AST_long_unsigned_k);
            break;
        case AST_hyper_integer_k:
            type_node_p = AST_lookup_type_node (AST_hyper_unsigned_k);
            break;
        default:
            printf("Unexpected type!!!!!\n");
        }

    return type_node_p;
}

/*---------------------------------------------------------------------*/

/*
 *  A S T _ l o o k u p _ n a m e d _ t y p e
 *  =========================================
 *
 *  Look up and existing named type
 *  and return the type node bound to it.
 *
 */

AST_type_n_t *AST_lookup_named_type
(
    NAMETABLE_id_t type_name
)
{
    AST_type_n_t *type_node_ptr;


    type_node_ptr =
                (AST_type_n_t *) ASTP_lookup_binding(type_name,
                                                     fe_type_n_k, TRUE);

    /* If no binding or invliad type... */
    if (type_node_ptr ==  NULL)
    {
        /* Return a pointer to a type node, so parse/checker won't barf */
        type_node_ptr  = ASTP_long_int_ptr;
    }

    return type_node_ptr;
}

/*
 *  A S T _ l o o k u p _t y p e _ n o d e
 *  ======================================
 *
 *  Look up and existing type node.
 *
 */

AST_type_n_t *AST_lookup_type_node
(
    AST_type_k_t kind
)
{
    AST_type_n_t *type_node_ptr = NULL;

    /* Return the node pointer to the base type */
    switch (kind)
    {
        case AST_boolean_k:
            type_node_ptr = ASTP_boolean_ptr;
            break;

        case AST_byte_k:
            type_node_ptr  = ASTP_byte_ptr;
            break;

        case AST_character_k:
            type_node_ptr  = ASTP_char_ptr;
            break;

        case AST_small_integer_k:
            type_node_ptr  = ASTP_small_int_ptr;
            break;

        case AST_small_unsigned_k:
            type_node_ptr  = ASTP_small_unsigned_ptr;
            break;

        case AST_short_integer_k:
            type_node_ptr  = ASTP_short_int_ptr;
            break;

        case AST_short_unsigned_k:
            type_node_ptr  = ASTP_short_unsigned_ptr;
            break;

        case AST_long_integer_k:
            type_node_ptr  = ASTP_long_int_ptr;
            break;

        case AST_long_unsigned_k:
            type_node_ptr  = ASTP_long_unsigned_ptr;
            break;

        case AST_hyper_integer_k:
            type_node_ptr  = ASTP_hyper_int_ptr;
            break;

        case AST_hyper_unsigned_k:
            type_node_ptr  = ASTP_hyper_unsigned_ptr;
            break;

        case AST_short_float_k:
            type_node_ptr  = ASTP_short_float_ptr;
            break;

        case AST_long_float_k:
            type_node_ptr  = ASTP_long_float_ptr;
				if (the_interface)
					 AST_SET_DOUBLE_USED(the_interface);
            break;

        case AST_void_k:
            type_node_ptr  = ASTP_void_ptr;
            break;

        case AST_handle_k:
            type_node_ptr  = ASTP_handle_ptr;
            break;

        default:
            break;
    }


    return type_node_ptr;
}


/*---------------------------------------------------------------------*/

/*
 *  A S T _ n a m e d _ c o n s t a n t
 *  ===================================
 *
 *  Look up the referenced constant and return
 *  the binding to the caller.
 *
 */

AST_constant_n_t *AST_named_constant
(
    NAMETABLE_id_t const_name
)
{
    AST_constant_n_t *named_const_node_p;

    named_const_node_p= (AST_constant_n_t *)
                            ASTP_lookup_binding(const_name,
                                                fe_constant_n_k, TRUE);

    /* If we did not find the binding or an invalid type... */
    if (named_const_node_p ==  NULL)
    {
        /* Return a pointer to a zero constant, so parse can continue */
        named_const_node_p = zero_constant_p;
    }


    return named_const_node_p;

}

/*---------------------------------------------------------------------*/


/*
 *
 *  A S T _ n u l l _ c o n s t a n t
 *  ==================================
 *
 *  The NULL keyword is used to specify
 *  a Null pointer.  It can only be used
 *  with a void * type.
 */

AST_constant_n_t *AST_null_constant
(void)
{
    AST_constant_n_t  *const_node_ptr;

    const_node_ptr = AST_constant_node(AST_nil_const_k);
    const_node_ptr->value.int_val = 0;

    return const_node_ptr;
}

/*---------------------------------------------------------------------*/

/*
 *  A S T _ o p e r a t i o n _ n o d e
 *  ===================================
 *
 *  This function creates a function node for the specified declarator, type and
 *  attributes.  If the toplevel declarator is not a function then an error
 *  is issued since IDL does not support variable declarations.
 *
 */

AST_operation_n_t *AST_operation_node
(
    AST_type_n_t      *base_type,
    ASTP_declarator_n_t *declarator,
    ASTP_attributes_t   *attributes
)
{
    AST_operation_n_t *operation_node_p;
    AST_type_n_t      *result_type;
    ASTP_declarator_op_n_t *function_op, *new_last_op;
    AST_parameter_n_t *parameters;
    NAMETABLE_id_t    op_name;

    /*
     * The top level declarator should be AST_function_k.  If not
     * the declaration was for a variable and is not supported so issue a
     * message and return NULL.
     */
    if ((declarator->last_op == NULL) ||
       (declarator->last_op->op_kind != AST_function_k))
    {
        char const *var_name;
        NAMETABLE_id_to_string(declarator->name, &var_name);
        log_error(nidl_yylineno, NIDL_VARDECLNOSUP, var_name, NULL);
        return NULL;
    }

    /*
     * Save the function op node for later use
     */
    function_op = declarator->last_op;

    /*
     *  Find the operation immediately before the function op
     */
    for (new_last_op = declarator->next_op;
        ((new_last_op != NULL) && (new_last_op->next_op != function_op));
        new_last_op = new_last_op->next_op);

    /*
     * Remove the function op from the list, leaving a list of declarators that
     * describe the return type of the function.
     */
    declarator->last_op = new_last_op;

    /*
     * If there are no other ops in the list, NULL out the next_op field of the
     * declarator, otherwise cause the list to end with the new_last_op
     */
    if (new_last_op == NULL)
        declarator->next_op = NULL;
    else
        new_last_op->next_op = NULL;

    /*
     * Extract the operation name and parameters from the function_op node.
     */
    parameters = function_op->op_info.routine_params;
    op_name = declarator->name;

    /*
     * Propagate the declarator to a type node.  Temporaily add in [ptr]
     * attribute because operation results can only really be [ptr].
     */
    operation_node_p = AST_create_operation_node(op_name, parameters);
    result_type = AST_propagate_type(base_type, declarator,
                                        attributes,
                                        (ASTP_node_t *)operation_node_p);

    /* Initialize the operation node */
    operation_node_p->result = AST_parameter_node (operation_node_p->name);
    (operation_node_p->result)->type = result_type;
    (operation_node_p->result)->uplink = operation_node_p;


    /*
     *  Propagate operation attributes that belong on the parameter to
     *  parameter node for op result
     */
    if (ASTP_TEST_ATTR(attributes,ASTP_REF))
        AST_SET_REF(operation_node_p->result);
    if (ASTP_TEST_ATTR(attributes,ASTP_UNIQUE))
        AST_SET_UNIQUE(operation_node_p->result);
    if (ASTP_TEST_ATTR(attributes,ASTP_PTR))
        AST_SET_PTR(operation_node_p->result);
    if (ASTP_TEST_ATTR(attributes,ASTP_STRING))
        AST_SET_STRING(operation_node_p->result);
    if (ASTP_TEST_ATTR(attributes,ASTP_CONTEXT))
        AST_SET_CONTEXT(operation_node_p->result);

    ASTP_CLR_ATTR(attributes,
          (ASTP_STRING|ASTP_CONTEXT|ASTP_REF|ASTP_UNIQUE|ASTP_PTR));


    /*
     * Propagate type attributes to parameter node for op result
     */
    if (AST_REF_SET(result_type)) AST_SET_REF(operation_node_p->result);
    if (AST_PTR_SET(result_type)) AST_SET_PTR(operation_node_p->result);
    if (AST_UNIQUE_SET(result_type)) AST_SET_UNIQUE(operation_node_p->result);

    /*
     * Check that the base type is not an anonymous struct/union
     */
    if (!AST_DEF_AS_TAG_SET(result_type) &&
        ((result_type->kind == AST_disc_union_k) ||
         (result_type->kind == AST_structure_k)) &&
        (result_type->name == NAMETABLE_NIL_ID))
    {
        char const *identifier;
        NAMETABLE_id_to_string(operation_node_p->name, &identifier);
        log_error(nidl_yylineno, NIDL_ANONTYPE, identifier, NULL);
    }


    /* Update the operation count of the interface node */
    operation_node_p->op_number = the_interface->op_count++;

    /*
     * Set flags field from the attributes that were parsed.
     */
    AST_set_flags(&operation_node_p->flags, (ASTP_node_t *)operation_node_p,
                    attributes);

    /*
     * Link parameters to operation node.
     * Merge in any synthesized attributes (has_ins, has_outs, has_in_ctx, etc.).
     */
    AST_finish_operation_node(operation_node_p);


    /*
     * Bind the operation node to the operation name.
     */
    ASTP_add_name_binding (op_name, operation_node_p);


    /*
     * Free the list of declarators, and the function operation.
     */
    FREE(function_op);
    ASTP_free_declarators(declarator);

    return operation_node_p;
}

/*---------------------------------------------------------------------*/


/*
 *  A S T _ p a r a m e t e r _ n o d e
 *  ===================================
 */

AST_parameter_n_t * AST_parameter_node
(
    NAMETABLE_id_t identifier
)
{
    AST_parameter_n_t  * parameter_node_ptr;

    parameter_node_ptr = NEW (AST_parameter_n_t);

    ASTP_set_fe_info (&parameter_node_ptr->fe_info, fe_parameter_n_k);

    parameter_node_ptr->name = identifier;

    return parameter_node_ptr;
}

/*---------------------------------------------------------------------*/


/*
 *  A S T _ f i n i s h _ i n t e r f a c e _ n o d e
 *  =================================================
 */

void AST_finish_interface_node
(
    AST_interface_n_t *interface_node_p
)
{
    if (interface_node_p == NULL) interface_node_p = AST_interface_node();

    /* Patch Tag references */
    ASTP_patch_tag_references(interface_node_p);
}

/*---------------------------------------------------------------------*/


AST_rep_as_n_t *AST_represent_as_node
(
    NAMETABLE_id_t name
)
{
    AST_rep_as_n_t *represent_as_node;

    represent_as_node = NEW (AST_rep_as_n_t);

    represent_as_node->type_name = name;
    represent_as_node->file_name = STRTAB_NULL_STR;

    ASTP_set_fe_info (&represent_as_node->fe_info, fe_rep_as_n_k);

    return represent_as_node;

}

/*---------------------------------------------------------------------*/


AST_cs_char_n_t *AST_cs_char_node
(
    NAMETABLE_id_t name
)
{
    AST_cs_char_n_t *cs_char_node;

    cs_char_node = NEW (AST_cs_char_n_t);

    cs_char_node->type_name = name;

    ASTP_set_fe_info (&cs_char_node->fe_info, fe_cs_char_n_k);

    return cs_char_node;

}

/*---------------------------------------------------------------------*/


/*
 *  A S T _ p a r s e _ p o r t
 *  ===========================
 *
 * Break the string specified by the user as the port name, into
 * the protocol and endpoint portions for easier use by the backend.
 *
 * Globals used:  interface_protocol_count, interface_protocols,
 *           and interface_endpoints
 */

void ASTP_parse_port
(
    AST_interface_n_t       *interface_p,
    STRTAB_str_t        port_string
)
{
    STRTAB_str_t protocol_id;
    STRTAB_str_t endpoint_id;
    char protocol_buf[256];
    char endpoint_buf[256];
    char const *protocol_start;
    char *protocol_end;
    char *endpoint_start;
    char *endpoint_end;
    int  i;                     /* loop through previous port names */
    boolean found = false;      /* An entry for this protocol already exists */

    /*
     * Get the port string as a char *
     */
    STRTAB_str_to_string(port_string,&protocol_start);

    /*
     * Find the colon denoting the end of the protocol portion,
     * if there is none, issue a warning and assume there is no
     * endpoint specification.
     */
    protocol_end = strchr(protocol_start,':');
    if (protocol_end == NULL)
    {
        /* warn that no endpoint was found */
        log_warning(1,NIDL_NOENDPOINT,protocol_start, NULL);
        protocol_id = STRTAB_add_string(protocol_start);
        endpoint_id = STRTAB_NULL_STR;
    }
    else {
        /*
         * Copy the protocol that we found into a buffer so it can
         * have a NULL added to terminate it and add it to the STRTAB.
         */
        strncpy(protocol_buf,protocol_start,protocol_end - protocol_start);
        protocol_buf[protocol_end - protocol_start] = '\0';
        protocol_id = STRTAB_add_string(protocol_buf);


        /*
         * After the protocol should be the endpoint.  It should
         * be enclosed in brackets.  If not issue a warning, but
         * use the rest of the string as the endpoint anyway.
         */
        endpoint_start = protocol_end + 1;
        if (*endpoint_start == '[')
        {
            endpoint_start++;
            endpoint_end = strchr(endpoint_start,']');  /* Find the close bracket on endpoint field */
            if (endpoint_end == NULL)
            {
                log_warning(1,NIDL_ENDPOINTSYNTAX,endpoint_start, NULL);
                protocol_id = STRTAB_add_string(protocol_start);
                endpoint_id = STRTAB_NULL_STR;
            }
            else
            {
                strncpy(endpoint_buf,endpoint_start,endpoint_end - endpoint_start);
                endpoint_buf[endpoint_end - endpoint_start] = '\0';
                endpoint_id = STRTAB_add_string(endpoint_buf);
                /* warn that no endpoint was found */
                if (endpoint_buf[0] == '\0')
                {
                    log_warning(1,NIDL_NOENDPOINT,protocol_start, NULL);
                }
            }
        }
        else {
            endpoint_id = STRTAB_add_string(endpoint_start);
            log_warning(1,NIDL_ENDPOINTSYNTAX,endpoint_start, NULL);
        }
    }

    /*
     * Make sure that only one endpoint is specified for each protocol
     */
    for (i = 0; i < interface_p->number_of_ports; i++)
    {
        if (interface_p->protocol[i] == protocol_id)
        {
            found = true;
            log_warning(1,NIDL_DUPPROTOCOL,protocol_start, NULL);
        }
    }

    /*
     * If not already there add a new entry
     */
    if (!found)
    {
        interface_p->number_of_ports++;
        if (interface_p->number_of_ports == 1)
        {
            interface_p->protocol  = NEW (STRTAB_str_t);
            interface_p->endpoints = NEW (STRTAB_str_t);
        }
        else {
	    interface_p->protocol =
		RENEW (interface_p->protocol, interface_p->number_of_ports);
	    interface_p->endpoints =
		RENEW (interface_p->endpoints, interface_p->number_of_ports);
        }
        (interface_p->protocol)[i] = protocol_id;
        (interface_p->endpoints)[i] = endpoint_id;
    }


}
/*---------------------------------------------------------------------*/


/*
 *  A S T _ s t r i n g _ c o n s t a n t
 *  =====================================
 */
AST_constant_n_t *AST_string_constant
(
    STRTAB_str_t value
)
{
    AST_constant_n_t *const_node_ptr;

    const_node_ptr = AST_constant_node (AST_string_const_k);

    const_node_ptr->value.string_val = value;

    return const_node_ptr;
}

/*---------------------------------------------------------------------*/


/*
 *  A S T _ t y p e _ n o d e
 *  =========================
 *
 *  Create and initialize a type node.
 *
 */

AST_type_n_t *AST_type_node
(
    AST_type_k_t kind
)
{
    AST_type_n_t *type_node_ptr;

    type_node_ptr = NEW (AST_type_n_t);

    type_node_ptr->name = NAMETABLE_NIL_ID;
    type_node_ptr->kind = kind;

    ASTP_set_fe_info (&type_node_ptr->fe_info, fe_type_n_k);
    type_node_ptr->fe_info->type_specific.clone = NULL; /* No clones yet! */

    /* Now get the ndr size for if this is a scalar and pointer type */
    switch (kind)
    {
        case AST_boolean_k:
            type_node_ptr->ndr_size = NDR_C_BOOLEAN_SIZE;
            break;
        case AST_byte_k:
            type_node_ptr->ndr_size = NDR_C_BYTE_SIZE;
            break;
        case AST_character_k:
            type_node_ptr->ndr_size = NDR_C_CHARACTER_SIZE;
            break;
        case AST_small_integer_k:
        case AST_small_unsigned_k:
            type_node_ptr->ndr_size = NDR_C_SMALL_INT_SIZE;
            break;

        case AST_short_integer_k:
        case AST_short_unsigned_k:
        case AST_enum_k:
            type_node_ptr->ndr_size = NDR_C_SHORT_INT_SIZE;
            break;

        case AST_long_integer_k:
        case AST_long_unsigned_k:
            type_node_ptr->ndr_size = NDR_C_LONG_INT_SIZE;
            break;

        case AST_hyper_integer_k:
        case AST_hyper_unsigned_k:
            type_node_ptr->ndr_size = NDR_C_HYPER_INT_SIZE;
            break;

        case AST_short_float_k:
            type_node_ptr->ndr_size = NDR_C_SHORT_FLOAT_SIZE;
            break;

        case AST_long_float_k:
            type_node_ptr->ndr_size = NDR_C_LONG_FLOAT_SIZE;
            break;

        case AST_pointer_k:
            type_node_ptr->ndr_size = NDR_C_POINTER_SIZE;
            break;

        default:
            break;
    }

    /* Set the alignment to the type's NDR size */
    type_node_ptr->alignment_size = type_node_ptr->ndr_size;
    return type_node_ptr;
}


/*---------------------------------------------------------------------*/

/*
 *  A S T _ t y p e _ p t r _ n o d e
 *  =================================
 *
 *  Create and initialize a type pointer node.
 *  A type pointer node contains a pointer to a type node
 *  and is used to link type nodes together.
 *
 */

AST_type_p_n_t *AST_type_ptr_node
(void)
{
    AST_type_p_n_t *type_p_node;

    type_p_node = NEW (AST_type_p_n_t );

    ASTP_set_fe_info (&type_p_node->fe_info, fe_type_p_n_k);

    return type_p_node;
}


/*---------------------------------------------------------------------*/

/*
 *  A S T _ t y p e s _ t o _ e x p o r t s
 *  =======================================
 *
 *  Creates and initializes a single export node
 *  for each type provided in the type pointer list.
 *
 * Inputs:
 *  type_p_list     A list of type pointer nodes containing the
 *                  type nodes to be exported.
 * Outputs:
 *  None
 *
 * Function Value:
 *  Pointer to the resultant list of export nodes.
 *
 */

AST_export_n_t *AST_types_to_exports
(
    AST_type_p_n_t *type_p_list
)
{
    AST_export_n_t *export_list = NULL,
                   *export_node_ptr;

    AST_type_p_n_t *type_p;             /* For list traversing */


    for (type_p = type_p_list; type_p != (AST_type_p_n_t *) NULL;
        type_p = type_p->next)
    {
        export_node_ptr =
                    AST_export_node((ASTP_node_t *)type_p->type, AST_type_k);
        export_list = (AST_export_n_t *)
                        AST_concat_element((ASTP_node_t *)export_list,
                                           (ASTP_node_t *)export_node_ptr);
    }

    /* Free the no longer needed linked list of type pointer nodes */
    ASTP_free_simple_list((ASTP_node_t *)type_p_list);


    return export_list;
}

/*-----------------------------------------------------------------*/




/*---------------------------------------------------------------------*/



/*
 *  A S T _ s e t _ o p e r _ h a s _ i n s _ o u t s
 *  =================================================
 *
 * Local routine to set the operation's has_ins, has_outs
 * synthesized attributes, and propagate operation
 * attributes that apply to the result parameter.
 *
 */
static void AST_set_oper_has_ins_outs
(
    AST_operation_n_t *operation_node,
    ASTP_parameter_count_t *param_count
)
{
    if (param_count->in_params > 0 )
    {
        /*
         * If a handle type (handle_t) is the first
         * and only parameter do not set the HAS_INS flag
         * since handle_t types do not need marshalling.
         */
        if (!((param_count->in_params == 1) &&
            (((operation_node->parameters->type->kind == AST_handle_k)
              && !AST_HANDLE_SET(operation_node->parameters->type))
             || ((operation_node->parameters->type->kind == AST_pointer_k)
              && (operation_node->parameters->type->type_structure.pointer
                    ->pointee_type->kind == AST_handle_k)
              && !AST_HANDLE_SET(operation_node->parameters->type->
                                 type_structure.pointer->pointee_type)))))
        {
            AST_SET_HAS_INS(operation_node);
        }
    }

    if (param_count->out_params > 0)
    {
        AST_SET_HAS_OUTS(operation_node);
    }

    /*
     * If a result is there, flag it as an OUT
     * parameter, and be sure that the operation
     * flag is set.
     */
    if (operation_node->result->type->kind != AST_void_k)
    {
        AST_SET_OUT(operation_node->result);

        /* Make sure it gets reflected on the operation */
        AST_SET_HAS_OUTS(operation_node);
    }


    return;
}


/*---------------------------------------------------------------------*/



/*
 *  A S T _ s y n t h e s i z e _ p a r a m _ t o _ o p e r_  a t t r s
 *  ===================================================================
 *
 * Local routine to set a operation's attributes
 * synthesized from certain parameter attributes.
 *
 */
static void AST_synthesize_param_to_oper_attr
(
    AST_parameter_n_t *parameter_node,
    AST_operation_n_t *operation_node,
    ASTP_parameter_count_t *param_count
)
{

    AST_type_n_t *param_type;

    /*
     * Get the real data being used as the parameter, i.e.
     * Ignore passing data (by reference) mechanism
     */
    if ((parameter_node->type->kind == AST_pointer_k) &&
        (parameter_node->type->name == NAMETABLE_NIL_ID))
    {
        param_type = parameter_node->type->type_structure.pointer->pointee_type;
    }
    else
    {
        param_type = parameter_node->type;
    }

    if (AST_IN_SET(parameter_node))
    {
        /* IN pipes count towards HAS_IN_PIPES but not HAS_INS */
        if (param_type->kind == AST_pipe_k)
        {
            AST_SET_HAS_IN_PIPES(operation_node);
        }
        else
        {
            param_count->in_params++;
        }
    }

    if (AST_OUT_SET(parameter_node))
    {
        /* OUT pipes count towards HAS_OUT_PIPES, but not HAS_OUTS */
        if (param_type->kind == AST_pipe_k)
        {
            AST_SET_HAS_OUT_PIPES(operation_node);
        }
        else
        {
            param_count->out_params++;
        }
    }


    return;
}


/*---------------------------------------------------------------------*/



/*
 *  A S T _ e n u m _ c o n s t a n t
 *  =================================
 *
 *  This routine returns a constant node
 *  representing an identifier observed in the
 *  grammar.
 */

AST_constant_n_t *AST_enum_constant
(
    NAMETABLE_id_t identifier,
	 AST_exp_n_t * exp
)
{
	 AST_constant_n_t    *constant_node_ptr;

	 /*
	  * Allocate and initialize a constant node.
	  */
	 constant_node_ptr = AST_constant_node(AST_int_const_k);
	 constant_node_ptr->name = identifier;

	 /*
	  * Qualify the constant type for Checker
	  */
	 constant_node_ptr->fe_info->fe_type_id = fe_const_info;
	 constant_node_ptr->fe_info->type_specific.const_kind = fe_enum_const_k;

	 /*
	  * Bind the name to the constant value
	  */
	 ASTP_add_name_binding (identifier, constant_node_ptr);


	 /* set the value */

	 constant_node_ptr->value.int_val = exp->exp.constant.val.integer;
	 constant_node_ptr->int_signed = exp->exp.constant.int_signed;
	 
	 /*
	  * Return the new constant node
	  */
	 return constant_node_ptr;
}


/*---------------------------------------------------------------------*/



/*
 *  A S T _ p i p e _ n o d e
 *  =================================
 *
 *  This routine allocates and initializes a pipe node
 *  the newly create pipe type is returned.
 */

AST_type_n_t *AST_pipe_node
(
    AST_type_n_t *pipe_type
)
{
    AST_pipe_n_t *pipe_node_ptr;        /* pipe node */
    AST_type_n_t *type_node_ptr;        /* type node pointing to the pipe */


    /*
     * Allocate and initialize the pipe node
     */
    pipe_node_ptr = NEW (AST_pipe_n_t);
    type_node_ptr = AST_type_node(AST_pipe_k);


    /*
     * Initialize pipe node contents
     */
     pipe_node_ptr->base_type = pipe_type;


    /*
     * Initialize type node contents
     */
    type_node_ptr->type_structure.pipe = pipe_node_ptr;


    /*
     * Set the source information
     */
    ASTP_set_fe_info(&pipe_node_ptr->fe_info, fe_pipe_n_k);

    /*
     * Return the type node
     */
    return type_node_ptr;
}


/*---------------------------------------------------------------------*/
/* preserve coding style vim: set tw=78 sw=4 : */
