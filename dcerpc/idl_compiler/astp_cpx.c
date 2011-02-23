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
 */
/*
**
**  NAME
**
**      ASTP_CPX.C
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Builds the Abstract Syntax Tree (AST) representation
**      for complex types: currently structures and unions.
**
**  VERSION: DCE 1.0
**
*/
#include <nidl.h>
#include <nametbl.h>
#include <errors.h>
#include <astp.h>
#include <nidlmsg.h>

extern int nidl_yylineno;


/*
 * Prototypes for static routines and forward references
 */

static void ASTP_add_tag_binding (
    NAMETABLE_id_t      name,
    AST_type_n_t *type_node
);

static void ASTP_process_sp_type (
    AST_interface_n_t *interface_node_ptr,
    AST_type_n_t *type_node_ptr
);

static AST_field_n_t *AST_field_node (
    NAMETABLE_id_t field_name
);

static void AST_find_self_reference (
    AST_interface_n_t *interface_node_ptr,
    AST_type_p_n_t *active_type_chain,
    AST_type_n_t *current_type_node_ptr
);



/*
 * External References
 */
extern int error_count;     /* number of errors detected thus far */





/*
 *  A S T P _ a d d _ n a m e _ b i n d i n g
 *  ==========================================
 *
 * This routine add a binding between the tag name and
 * node specified.  If the name is already declared
 * the NIDL_NAMEALRDEC error message is displayed.
 */

static void ASTP_add_tag_binding
(
    NAMETABLE_id_t      name,
    AST_type_n_t *type_node
)
{
    /*
     * Add name to nametable and bind it to the specified node
     */
    if (!NAMETABLE_add_tag_binding (name, (char *)type_node))
    {
        char const *identifier; /* place to receive the identifier text pointer */
        char const *filename;   /* place to receive the filename text pointer */
        ASTP_node_t *binding;
        NAMETABLE_id_to_string (name, &identifier);
        binding = (ASTP_node_t *)NAMETABLE_lookup_binding(name);

        /* Select message and file information as present */
        if ((binding->fe_info != (fe_info_t *)NULL) &&
            (binding->fe_info->source_line != 0) &&
            (binding->fe_info->file != STRTAB_NULL_STR))
        {
            STRTAB_str_to_string(binding->fe_info->file, &filename);
            log_error (nidl_yylineno, NIDL_NAMEPREVDECLAT, identifier,
                    filename, binding->fe_info->source_line, NULL);
        }
        else
            log_error (nidl_yylineno, NIDL_NAMEALRDEC, identifier, NULL);
    }

    return;
}

/*---------------------------------------------------------------------*/

/*
 *  A S T P _ a r r a y _ i n d e x _ n o d e
 *  =========================================
 *
 *  Create the temporary array index structure
 *  to keep the array bound type and values for
 *  each dimension of an array.
 *
 */

ASTP_array_index_n_t *ASTP_array_index_node
(
    AST_constant_n_t *lower_bound,          /* Pointer to lower bound constant node */
    ASTP_bound_t     lower_bound_type,      /* Lower bound type */
    AST_constant_n_t *upper_bound,          /* Pointer to upper bound constant node */
    ASTP_bound_t     upper_bound_type       /* Upper bound type */
)
{
    ASTP_array_index_n_t *index_node_ptr;

    index_node_ptr = NEW (ASTP_array_index_n_t);

    /* Fill in array bound type for lower and upper bounds */
    index_node_ptr->lower_bound_type = lower_bound_type;
    index_node_ptr->upper_bound_type = upper_bound_type;

    /* If bounded with a constant, link in constant node */
    if (lower_bound_type == ASTP_constant_bound)
    {
        index_node_ptr->lower_bound = lower_bound;
    };

    /* If lower bound is the default, link in zero constant node */
    if (lower_bound_type == ASTP_default_bound)
    {
        index_node_ptr->lower_bound = zero_constant_p;
    };

    if (upper_bound_type == ASTP_constant_bound)
    {
        if (lower_bound_type == ASTP_default_bound)
        {
            index_node_ptr->upper_bound = AST_clone_constant(upper_bound);
            index_node_ptr->upper_bound->value.int_val--;
        }
        else
        {
            index_node_ptr->upper_bound = upper_bound;
        }

    };

    return index_node_ptr;
}

/*---------------------------------------------------------------------*/


/*
 *  A S T _ a r r a y _ i n d e x _ n o d e
 *  =======================================
 *
 *  Create the array vector to hold the
 *  the index information (array index node)
 *  for each dimension of an array.
 *
 */

AST_array_index_n_t *AST_array_index_node
(
    unsigned short array_size
)
{
    unsigned short      i;
    AST_array_index_n_t *index_vector,
                        *index_node;


    index_vector = NEW_VEC (AST_array_index_n_t, array_size);

    for (i = 0, index_node = index_vector;
         i<array_size;
         i++, index_node++)
    {
        ASTP_set_fe_info (&index_node->fe_info, fe_array_index_n_k);
    }

    return index_vector;
}

/*---------------------------------------------------------------------*/

/*
 *  A S T _ a r r a y _ n o d e
 *  ===========================
 *
 *  Create the array node and
 *  point to the element type.
 *
 */


AST_array_n_t *AST_array_node
(
    AST_type_n_t *element_type_ptr
)
{
    AST_array_n_t *array_node_ptr;

    array_node_ptr = NEW (AST_array_n_t);


    /* Link in the element type */
    array_node_ptr->element_type = element_type_ptr;

    ASTP_set_fe_info(&array_node_ptr->fe_info, fe_array_n_k);

    return array_node_ptr;
}

/*---------------------------------------------------------------------*/

/*
 *  A S T _ f i e l d _ a t t r _ n o d e
 *  =====================================
 *
 *  Create a field attribute node to hold
 *  the field attributes for a structure
 *  field or parameter.
 *
 */

AST_field_attr_n_t *AST_field_attr_node
(void)
{
    AST_field_attr_n_t *field_attr_node_ptr;

    field_attr_node_ptr = NEW (AST_field_attr_n_t);
    ASTP_set_fe_info(&field_attr_node_ptr->fe_info, fe_field_attr_n_k);

    return field_attr_node_ptr;
}

/*---------------------------------------------------------------------*/

/*
 *  A S T _ f i e l d _ r e f _ n o d e
 *  =====================================
 *
 *  Create a field reference vector to hold
 *  the field attribute references for each
 *  dimension of an array.
 *
 */

AST_field_ref_n_t *AST_field_ref_node
(
    unsigned short dimension
)
{
    unsigned short i;
    AST_field_ref_n_t *field_ref_vector,
                      *field_ref_node;

    field_ref_vector = NEW_VEC (AST_field_ref_n_t, dimension);

    for (i = 0, field_ref_node = field_ref_vector;
         i<dimension;
         i++, field_ref_node++)
    {
        field_ref_node->valid = FALSE;
        ASTP_set_fe_info(&field_ref_node->fe_info, fe_field_ref_n_k);
    }

    return field_ref_vector;
}

/*---------------------------------------------------------------------*/

/*
 *  A S T _ s t r u c t u r e _ n o d e
 *  =====================================
 *
 *
 *  Create and initialize a private structure
 *  node which is which points to the fields of
 *  the structure.
 *
 */
AST_type_n_t *AST_structure_node
(
    AST_field_n_t *field_list,
    NAMETABLE_id_t identifier
)
{
    AST_structure_n_t   *structure_node_ptr;    /* Structure */
    AST_type_n_t        *type_node_ptr;         /* Type node for structure */
    AST_type_n_t        *tag_type_node_ptr;     /* [def_as_tag] Type node for structure */

    /*
     * Allocate and initialize the structure node which contains the
     * tag name and pointer to field list.
     */
    structure_node_ptr = NEW (AST_structure_n_t);
    structure_node_ptr->fields = field_list;
    structure_node_ptr->tag_name = identifier;
    ASTP_set_fe_info(&structure_node_ptr->fe_info, fe_structure_n_k);


    /*
     * If this struct has a tag, check and see if we have already had a
     * reference to this tag name.
     */
    tag_type_node_ptr = NULL;
    if (identifier != NAMETABLE_NIL_ID)
    {
        /*
         * Check for a binding for this tag
         */
        tag_type_node_ptr = (AST_type_n_t *) NAMETABLE_lookup_tag_binding(identifier);


        /*
         * If we didn't find a [def_as_tag] type node above, allocate a new one,
         * Otherwise, verify the binding is consistant with any previous references.
         */
        if (tag_type_node_ptr == NULL)
        {
            /*
             * First reference, so allocate a new type.
             */
            tag_type_node_ptr = AST_type_node(AST_structure_k);

            /*
             * Set up a name binding such that we can reference this structure by
             * tag name if a tag was specified.  Also if it has a tag name the
             * defined_as_tag flag should be set to true.
             */
            AST_SET_DEF_AS_TAG(tag_type_node_ptr);
            ASTP_add_tag_binding(identifier, tag_type_node_ptr);
            ASTP_save_tag_ref(identifier, AST_structure_k, tag_type_node_ptr);
            tag_type_node_ptr->type_structure.structure = structure_node_ptr;
        }
        else
        {
            /* Make sure we have the correct kind of node */
            if (tag_type_node_ptr->kind != AST_structure_k)
            {
                char const *identifier_text; /* place to receive the identifier text */
                NAMETABLE_id_to_string (identifier, &identifier_text);
                log_error (nidl_yylineno, NIDL_BADTAGREF, identifier_text,
			   NULL);

                /* State where the name was previously declared, if known */
                if ((tag_type_node_ptr->fe_info->source_line != 0) &&
                    (tag_type_node_ptr->fe_info->file != STRTAB_NULL_STR))
                {
                    char const *filename; /* place to receive the filename text pointer */
                    STRTAB_str_to_string(tag_type_node_ptr->fe_info->file, &filename);
		    log_error (nidl_yylineno, NIDL_NAMEPREVDECLAT,
			       identifier_text, filename,
			       tag_type_node_ptr->fe_info->source_line, NULL);
                }

                /* recovery is to return a bogus type node */
                tag_type_node_ptr = AST_type_node(AST_structure_k);
                AST_SET_DEF_AS_TAG(tag_type_node_ptr);
            }
            else if (tag_type_node_ptr->type_structure.structure != NULL)
            {
                char const *identifier_text; /* place to receive the identifier text */
                NAMETABLE_id_to_string (identifier, &identifier_text);

                /* State where the name was previously declared, if known */
                if ((tag_type_node_ptr->fe_info->source_line != 0) &&
                    (tag_type_node_ptr->fe_info->file != STRTAB_NULL_STR))
                {
                    char const *filename; /* place to receive the filename text pointer */
                    STRTAB_str_to_string(tag_type_node_ptr->fe_info->file, &filename);
		    log_error (nidl_yylineno, NIDL_NAMEPREVDECLAT,
			       identifier_text, filename,
			       tag_type_node_ptr->fe_info->source_line, NULL);
                }
                else
                    log_error (nidl_yylineno, NIDL_NAMEALRDEC,
			       identifier_text, NULL);

                /* recovery is to return a bogus type node */
                tag_type_node_ptr = AST_type_node(AST_structure_k);
                AST_SET_DEF_AS_TAG(tag_type_node_ptr);
            }
        }
    }


    /*
     * Create and fill in fields of type node for the struct.  Because this is
     * the type node for a struct, the name will be filled in by the typedef
     * code.  We cannot mark this type as complete because then copies of this
     * tag would not be check for self_pointing types.
     */
    type_node_ptr = AST_type_node(AST_structure_k);
    type_node_ptr->type_structure.structure = structure_node_ptr;

    /* Fill in the FE info so we can find the [def_as_tag] node from the type node */
    type_node_ptr->fe_info->tag_ptr = tag_type_node_ptr;
    type_node_ptr->fe_info->tag_name = identifier;

    /* TBS -- For now always assume incomplete to find all self-pointers */
    if (tag_type_node_ptr != NULL)
    {
        ASTP_save_tag_ref(identifier, AST_structure_k, type_node_ptr);
        tag_type_node_ptr->fe_info->original = type_node_ptr;
        tag_type_node_ptr->fe_info->tag_ptr = tag_type_node_ptr;
        tag_type_node_ptr->fe_info->tag_name = identifier;
        tag_type_node_ptr->type_structure.structure = structure_node_ptr;

        /*
         *  If the pointee was an arrayified pointer to a structure, then
         *  create an array of the original type instead of the def-as-tag
         *  type.
         */
        if (tag_type_node_ptr->array_rep_type != NULL)
            tag_type_node_ptr->array_rep_type->type_structure.array->element_type = type_node_ptr;
    }


    /*
     * Return the resulting type node
     */
    return type_node_ptr;
}

/*---------------------------------------------------------------------*/



/*
 *  A S T _ l a b e l _ a r m
 *  =========================
 *
 *  Set the list of labels for an arm of a union.
 */

AST_arm_n_t *AST_label_arm
(
    AST_arm_n_t *member,
    AST_case_label_n_t *case_labels
)
{
    /*
     * Fill in the fields of the arm node which identifies the list of values
     * for which this arm of a disc union is valid.
     */
    member->labels = case_labels;

    /*
     * Return the arm node
     */
    return member;
}

/*---------------------------------------------------------------------*/


/*
 *  A S T _ c a s e _ l a b e l _ n o d e
 *  =====================================
 *
 *  Create and initialize a node representing a member
 *  of the list of labels for one arm of a union.
 */

AST_case_label_n_t *AST_case_label_node
(
    AST_constant_n_t *case_label
)
{
    AST_case_label_n_t *case_label_node;

    /*
     * Allocate and fill fields of case label node
     */
    case_label_node = NEW (AST_case_label_n_t);
    case_label_node->default_label = FALSE;
    case_label_node->value = case_label;


    /*
     * Set source information
     */
    ASTP_set_fe_info (&case_label_node->fe_info, fe_case_label_n_k);

    /*
     * Return the case node
     */
    return case_label_node;
}

/*---------------------------------------------------------------------*/


/*
 *  A S T _ d e f a u l t _ c a s e _ l a b e l _ n o d e
 *  =====================================================
 *
 *  Create and initialize a label node identifying this
 *  as the DEFAULT arm of a union.
 */

AST_case_label_n_t *AST_default_case_label_node
(void)
{
    AST_case_label_n_t *case_label_node;

    /*
     * Use the normal case label node routine, but specifify a NULL
     * value and set the default_label flag to identify this as a
     * default case label.
     */
    case_label_node = AST_case_label_node(NULL);
    case_label_node->default_label = TRUE;
    return case_label_node;
}

/*---------------------------------------------------------------------*/


/*
 *  A S T _ d i s c _ u n i o n _ n o d e
 *  =====================================
 *
 *
 *  Create and initialize a private union
 *  node which is which points to the arms of
 *  the union.
 *
 */
AST_type_n_t *AST_disc_union_node
(
    NAMETABLE_id_t identifier,
    NAMETABLE_id_t union_name,
    NAMETABLE_id_t disc_name,
    AST_type_n_t *disc_type,
    AST_arm_n_t *arm_list
)
{
    AST_disc_union_n_t  *disc_union_node_ptr;   /* disc_union */
    AST_type_n_t        *type_node_ptr;         /* Type node for disc_union */
    AST_type_n_t        *tag_type_node_ptr;     /* [def_as_tag] Type node for union */

    /*
     * Allocate and initiailze the disc_union node
     */
    disc_union_node_ptr = NEW (AST_disc_union_n_t);
    disc_union_node_ptr->tag_name = identifier;
    disc_union_node_ptr->union_name = union_name;
    disc_union_node_ptr->discrim_name = disc_name;
    disc_union_node_ptr->discrim_type = disc_type;
    disc_union_node_ptr->arms = arm_list;
    ASTP_set_fe_info (&disc_union_node_ptr->fe_info, fe_disc_union_n_k);


    /*
     * If this union has a tag, check and see if we have already had a
     * reference to this tag name.
     */
    tag_type_node_ptr = NULL;
    if (identifier != NAMETABLE_NIL_ID)
    {
        /*
         * Check for a binding for this tag
         */
        tag_type_node_ptr = (AST_type_n_t *) NAMETABLE_lookup_tag_binding(identifier);


        /*
         * If we didn't find a [def_as_tag] type node above, allocate a new one,
         * Otherwise, verify the binding is consistant with any previous references.
         */
        if (tag_type_node_ptr == NULL)
        {
            /*
             * First reference, so allocate a new type.
             */
            tag_type_node_ptr = AST_type_node(AST_disc_union_k);

            /*
             * Set up a name binding such that we can reference this union by
             * tag name if a tag was specified.  Also if it has a tag name the
             * defined_as_tag flag should be set to true.
             */
            AST_SET_DEF_AS_TAG(tag_type_node_ptr);
            ASTP_add_tag_binding(identifier, tag_type_node_ptr);
            ASTP_save_tag_ref(identifier, AST_disc_union_k, tag_type_node_ptr);
            tag_type_node_ptr->type_structure.disc_union = disc_union_node_ptr;
        }
        else
        {
            /* Make sure we have the correct kind of node */
            if (tag_type_node_ptr->kind != AST_disc_union_k)
            {
                char const *identifier_text; /* place to receive the identifier text */
                NAMETABLE_id_to_string (identifier, &identifier_text);
                log_error (nidl_yylineno, NIDL_BADTAGREF, identifier_text,
			   NULL);

                /* State where the name was previously declared, if known */
                if ((tag_type_node_ptr->fe_info->source_line != 0) &&
                    (tag_type_node_ptr->fe_info->file != STRTAB_NULL_STR))
                {
                    char const *filename;             /* place to receive the filename text pointer */
                    STRTAB_str_to_string(tag_type_node_ptr->fe_info->file, &filename);
		    log_error (nidl_yylineno, NIDL_NAMEPREVDECLAT,
			       identifier_text, filename,
			       tag_type_node_ptr->fe_info->source_line, NULL);
                }

                /* recovery is to return a bogus type node */
                tag_type_node_ptr = AST_type_node(AST_disc_union_k);
                AST_SET_DEF_AS_TAG(tag_type_node_ptr);
            }
            else if (tag_type_node_ptr->type_structure.disc_union != NULL)
            {
                char const *identifier_text; /* place to receive the identifier text */
                NAMETABLE_id_to_string (identifier, &identifier_text);

                /* State where the name was previously declared, if known */
                if ((tag_type_node_ptr->fe_info->source_line != 0) &&
                    (tag_type_node_ptr->fe_info->file != STRTAB_NULL_STR))
                {
                    char const *filename; /* place to receive the filename text pointer */
                    STRTAB_str_to_string(tag_type_node_ptr->fe_info->file, &filename);
		    log_error (nidl_yylineno, NIDL_NAMEPREVDECLAT,
			       identifier_text, filename,
			       tag_type_node_ptr->fe_info->source_line, NULL);
                }
                else
                    log_error (nidl_yylineno, NIDL_NAMEALRDEC,
			       identifier_text, NULL);

                /* recovery is to return a bogus type node */
                tag_type_node_ptr = AST_type_node(AST_disc_union_k);
                AST_SET_DEF_AS_TAG(tag_type_node_ptr);
            }
        }
    }


    /*
     * Create and fill in fields of type node for the union.  Because this is
     * the type node for a union, the name will be filled in by the typedef
     * code.  We cannot mark this type as complete because then copies of this
     * tag would not be check for self_pointing types.
     */
    type_node_ptr = AST_type_node(AST_disc_union_k);
    type_node_ptr->type_structure.disc_union = disc_union_node_ptr;

    /* Fill in the FE info so we can find the [def_as_tag] node from the type node */
    type_node_ptr->fe_info->tag_ptr = tag_type_node_ptr;
    type_node_ptr->fe_info->tag_name = identifier;

    /* TBS -- For now always assume incomplete to find all self-pointers */
    if (tag_type_node_ptr != NULL)
    {
        ASTP_save_tag_ref(identifier, AST_disc_union_k, type_node_ptr);
        tag_type_node_ptr->fe_info->original = type_node_ptr;
        tag_type_node_ptr->fe_info->tag_ptr = tag_type_node_ptr;
        tag_type_node_ptr->fe_info->tag_name = identifier;
        tag_type_node_ptr->type_structure.disc_union = disc_union_node_ptr;

        /*
         *  If the pointee was an arrayified pointer to a structure, then
         *  create an array of the original type instead of the def-as-tag
         *  type.
         */
        if (tag_type_node_ptr->array_rep_type != NULL)
            tag_type_node_ptr->array_rep_type->type_structure.array->element_type = type_node_ptr;
    }

    /*
     * Return the new type node
     */
    return type_node_ptr;

}

/*---------------------------------------------------------------------*/



/*
 *
 *  A S T _ a r m _ n o d e
 *  ===================================
 *
 *  Create the node describing the arm of a union.
 *
 */

AST_arm_n_t *AST_arm_node
(
    NAMETABLE_id_t name,
    AST_case_label_n_t *label,
    AST_type_n_t *type
)
{
    AST_arm_n_t * arm_node_ptr;

    /*
     * Allocate and initialize a node representing one case of this disc union.
     */
    arm_node_ptr = NEW (AST_arm_n_t);
    arm_node_ptr->labels = label;
    arm_node_ptr->name = name;
    arm_node_ptr->type = type;


    /*
     * Set source information
     */
    ASTP_set_fe_info (&arm_node_ptr->fe_info, fe_arm_n_k);


    /*
     * Bind arm name (if not null) to the arm node, give an error if it
     * is already declared in this scope.
     */
    if (name != NAMETABLE_NIL_ID)
        ASTP_add_name_binding (name, arm_node_ptr);


    /*
     * Return the new arm node
     */
    return arm_node_ptr;
}


/*
 *  A S T _ f i e l d _ n o d e
 *  ===========================
 *
 *  Create and initialize a node representing a field in
 *  a structure.
 */

static AST_field_n_t * AST_field_node
(
    NAMETABLE_id_t field_name
)
{
    AST_field_n_t * field_node_ptr;


    /*
     * Create and initialize a node to contain a field of a structure.
     */
    field_node_ptr = NEW (AST_field_n_t);
    field_node_ptr->name = field_name;

    /*
     * Set source information
     */
    ASTP_set_fe_info (&field_node_ptr->fe_info, fe_field_n_k);


    /*
     * Bind field name to the field node, give an error if it
     * is already declared in this scope
     */
    ASTP_add_name_binding (field_name, field_node_ptr);

    /*
     * Return the new field node
     */
    return field_node_ptr;
}

/*---------------------------------------------------------------------*/



/*
 *  A S T _ d e c l a r a t o r s _ t o _ f i e l d s
 *  =================================================
 *
 *  Create and return a list of fields from a list of
 *  declarators and type information.
 */

AST_field_n_t *AST_declarators_to_fields
(
    ASTP_declarator_n_t *declarators_ptr,
    AST_type_n_t        *type_ptr,
    ASTP_attributes_t   *attributes
)
{
    AST_field_n_t * field_list = NULL;
    AST_field_n_t * new_field;
    ASTP_declarator_n_t *dp;
    ASTP_type_attr_n_t  *attr_ptr;

    /*
     * Loop through the declarators creating fields
     * and appending them to the list of fields to be returned.
     */
    for (dp = declarators_ptr; dp; dp = dp->next)
    {
        new_field = AST_field_node (dp->name);
        new_field->type = AST_propagate_type(type_ptr, declarators_ptr,
                                             attributes,
                                             (ASTP_node_t *)new_field);

        /* Disallow indirection [attribute(*field)] */
        if (attributes != NULL)
        {
            for (attr_ptr = attributes->bounds;
                 attr_ptr != NULL;
                 attr_ptr = attr_ptr->next)
            {
                if (attr_ptr->is_expr && attr_ptr->b.simple.pointer)
                    log_error(attr_ptr->source_line, NIDL_ATTRVALIND, NULL);
            }
        }

        new_field->field_attrs = AST_set_field_attrs(attributes,
                (ASTP_node_t *)new_field, new_field->type);

        AST_set_flags(&new_field->flags, (ASTP_node_t *)new_field, attributes);
        field_list = (AST_field_n_t *)AST_concat_element(
                (ASTP_node_t *)field_list, (ASTP_node_t *)new_field);

        ASTP_validate_forward_ref(new_field->type);
    }


    /* Free declarator list */
    ASTP_free_declarators(declarators_ptr);


    /* Return the list of fields */
    return field_list;

}

/*---------------------------------------------------------------------*/



/*
 *  A S T _ d e c l a r a t o r _ t o _ a r m s
 *  =================================================
 *
 *  Create and return a list of arms from a list of
 *  declarators and type information.
 */

AST_arm_n_t *AST_declarator_to_arm
(
    AST_type_n_t *type_ptr,
    ASTP_declarator_n_t *declarator,
    ASTP_attributes_t   *attributes
)
{
    AST_arm_n_t * arm_list = NULL;
    AST_arm_n_t * new_arm;
    ASTP_declarator_n_t * dp;


    if (declarator == NULL)
    {
        arm_list = AST_arm_node(NAMETABLE_NIL_ID, NULL, NULL);
        if (ASTP_TEST_ATTR(attributes, ASTP_DEFAULT))
        {
            if (ASTP_TEST_ATTR(attributes, ASTP_CASE))
            {
                ASTP_attr_flag_t attr1 = ASTP_CASE;
                ASTP_attr_flag_t attr2 = ASTP_DEFAULT;
                log_error(nidl_yylineno, NIDL_CONFLICTATTR,
                      KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)),
                      KEYWORDS_lookup_text(AST_attribute_to_token(&attr2)),
		      NULL);
            }
            else
                arm_list->labels = AST_default_case_label_node();
        }
        if (ASTP_TEST_ATTR(attributes, ASTP_CASE))
        {
            arm_list->labels = ASTP_case;
            ASTP_case = NULL;
        }
        /* Clear the attributes since they do not appear in the AST. */
        ASTP_CLR_ATTR(attributes, ASTP_CASE | ASTP_DEFAULT);
    }
    /*
     * Now loop through the declarators, creating fields and
     * appending them to the list of arm nodes.
     */
    for (dp = declarator; dp; dp = dp->next) {
        new_arm = AST_arm_node (dp->name,NULL,NULL);
        /*
         * Process the union arm attributes for non-encapsulated unions.
         */
        if (ASTP_TEST_ATTR(attributes, ASTP_DEFAULT))
        {
            if (ASTP_TEST_ATTR(attributes, ASTP_CASE))
            {
                ASTP_attr_flag_t attr1 = ASTP_CASE;
                ASTP_attr_flag_t attr2 = ASTP_DEFAULT;
                log_error(nidl_yylineno, NIDL_CONFLICTATTR,
                      KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)),
                      KEYWORDS_lookup_text(AST_attribute_to_token(&attr2)),
		      NULL);
            }
            else
                new_arm->labels = AST_default_case_label_node();
        }
        if (ASTP_TEST_ATTR(attributes, ASTP_CASE))
        {
            new_arm->labels = ASTP_case;
            ASTP_case = NULL;
        }
        /* Clear the attributes since they do not appear in the AST. */
        ASTP_CLR_ATTR(attributes, ASTP_CASE | ASTP_DEFAULT);
        new_arm->type = AST_propagate_type(type_ptr, declarator,
                                             attributes,
                                            (ASTP_node_t *)new_arm);
        AST_set_flags(&new_arm->flags, (ASTP_node_t *)new_arm, attributes);

        /*
         *  If the union arm is a pointer with the string attribute, generated
         *  the array_rep_type for the pointer.
         */
        if (AST_STRING_SET(new_arm) && new_arm->type &&
            new_arm->type->kind == AST_pointer_k)
        {
            ASTP_set_array_rep_type(new_arm->type,
                new_arm->type->type_structure.pointer->pointee_type,
                TRUE);
        }

        arm_list = (AST_arm_n_t *)AST_concat_element(
                (ASTP_node_t *)arm_list, (ASTP_node_t *)new_arm);
        ASTP_validate_forward_ref(new_arm->type);
    }


    /* Free declarator list */
    ASTP_free_declarators(declarator);

    return arm_list;
}

/*---------------------------------------------------------------------*/



/*
 *  A S T _ t y p e _ w i t h _ a t t r s _ f r o m _ t a g
 *  =======================================================
 *
 *  Create and initialize a type node from the specified
 *  tag (struct or union).  If this is a forward
 *  reference, then the type_structure field is not yet
 *  filled in, and the node is instead linked onto the
 *  forward reference chain for processing at the end of the
 *  parse.
 */

AST_type_n_t *AST_type_from_tag
(
    AST_type_k_t kind,
    NAMETABLE_id_t identifier
)
{
    AST_type_n_t        *tag_type_node_ptr;     /* Type node for tag */

    /*
     * Lookup the identifier to see if we can find the structure definition
     */
    tag_type_node_ptr = (AST_type_n_t *) NAMETABLE_lookup_tag_binding(identifier);
    if (tag_type_node_ptr != NULL)
    {
        /* Make sure we have the correct kind of node */
        if (tag_type_node_ptr->kind != kind)
        {
            char const *identifier_text; /* place to receive the identifier text */
            NAMETABLE_id_to_string (identifier, &identifier_text);
            log_error (nidl_yylineno, NIDL_BADTAGREF, identifier_text, NULL);

            /* State where the name was previously declared, if known */
            if ((tag_type_node_ptr->fe_info->source_line != 0) &&
                (tag_type_node_ptr->fe_info->file != STRTAB_NULL_STR))
            {
                char const *filename;         /* place to receive the filename text pointer */
                STRTAB_str_to_string(tag_type_node_ptr->fe_info->file, &filename);
                log_error (nidl_yylineno, NIDL_NAMEPREVDECLAT, identifier_text,
                        filename, tag_type_node_ptr->fe_info->source_line,
			NULL);
            }

            /* recovery is to return a bogus type node */
            tag_type_node_ptr = AST_type_node(kind);
            AST_SET_DEF_AS_TAG(tag_type_node_ptr);
        }
    }
    else {
        /*
         * Allocate the type node for this tag
         */
        tag_type_node_ptr = AST_type_node (kind);
        AST_SET_DEF_AS_TAG(tag_type_node_ptr);
        tag_type_node_ptr->type_structure.structure = NULL;

        /*
         * Mark this type node as incomplete, because we either must patch
         * it or atleast check if it should have the [self_pointer] attribute.
         */
        tag_type_node_ptr->fe_info->tag_ptr = tag_type_node_ptr;
        tag_type_node_ptr->fe_info->tag_name = identifier;
        ASTP_save_tag_ref(identifier, kind, tag_type_node_ptr);


        /*
         * Bind this type node to the tag so further references to it will
         * resolve to this instance of the type node.
         */
        ASTP_add_tag_binding(identifier,tag_type_node_ptr);
     }


    /*
     * Return the new type
     */
    return tag_type_node_ptr;
}


/*---------------------------------------------------------------------*/


/*
 * A S T P _ p a t c h _ t a g _ r e f e r e n c e s
 * ===================================================
 *
 * Loop through the list of tag references and patches the
 * type nodes that reference them which at the time the
 * the tag reference was parsed, was not yet defined.
 *
 * Inputs:
 *  None.
 *
 * Outputs:
 *  None.
 *
 * Implicit Inputs:
 *  ASTP_tag_ref_list
 *
 * Function value:
 *  None.
 *
 */
void ASTP_patch_tag_references
(
    AST_interface_n_t *interface_node_ptr
)
{
    AST_type_n_t *type_node_ptr;
    ASTP_tag_ref_n_t *tag_ref_node_ptr;

    /*
     * Loop through all forward references of tags, and patch the references.
     */
    for (tag_ref_node_ptr = ASTP_tag_ref_list; tag_ref_node_ptr;
         tag_ref_node_ptr= tag_ref_node_ptr->next)
    {
        /* Check if the tag has been defined */
        type_node_ptr = (AST_type_n_t *)
                        NAMETABLE_lookup_tag_binding(tag_ref_node_ptr->name);
        if ((type_node_ptr != NULL) && (type_node_ptr->type_structure.structure != NULL))
        {
            /* It has been defined, so make sure the type is correct */
            if (tag_ref_node_ptr->ref_kind != type_node_ptr->kind)
            {
                char const *identifier; /* place to receive the identifier text */
                NAMETABLE_id_to_string (tag_ref_node_ptr->name, &identifier);
		log_source_error
		    (tag_ref_node_ptr->type_node_ptr->fe_info->file,
		     tag_ref_node_ptr->type_node_ptr->fe_info->source_line,
		     NIDL_BADTAGREF, identifier, NULL);

                /* State where the name was previously declared, if known */
                if ((type_node_ptr->fe_info->source_line != 0) &&
                    (type_node_ptr->fe_info->file != STRTAB_NULL_STR))
                {
                    char const *filename; /* place to receive the filename text pointer */
                    STRTAB_str_to_string(type_node_ptr->fe_info->file, &filename);
		    log_source_error
			(tag_ref_node_ptr->type_node_ptr->fe_info->file,
			 tag_ref_node_ptr->type_node_ptr->fe_info->source_line,
			 NIDL_NAMEPREVDECLAT, identifier, filename,
			 type_node_ptr->fe_info->source_line, NULL);
                }
            }


            /* Update the type node that references this tag */
            tag_ref_node_ptr->type_node_ptr->type_structure = type_node_ptr->type_structure;
            FE_CLEAR(tag_ref_node_ptr->type_node_ptr->fe_info->flags,FE_INCOMPLETE);

            /* TBS -- flag propagation here? */
        }
        else
        {
            char const  *identifier;
            /*
             * Allow undeclared name only if it is a struct tag which is part
             * of a type definition with the [context_handle] attribute.  This
             * allows applications to define opaquely typed context handles.
             */
            if (type_node_ptr != NULL
                && tag_ref_node_ptr->ref_kind == AST_structure_k
                && AST_CONTEXT_RD_SET(type_node_ptr))
            {
                /* Construct a minimal struct - not needed for its content */
                AST_structure_n_t *struct_p;
                struct_p = type_node_ptr->type_structure.structure =
                    NEW (AST_structure_n_t);
                ASTP_set_fe_info (&struct_p->fe_info, fe_structure_n_k);
                struct_p->tag_name = tag_ref_node_ptr->name;
                continue;
            }
            NAMETABLE_id_to_string (tag_ref_node_ptr->name, &identifier);
	    log_source_error
		(tag_ref_node_ptr->type_node_ptr->fe_info->file,
		 tag_ref_node_ptr->type_node_ptr->fe_info->source_line,
		 NIDL_NAMENOTFND, identifier, NULL);
        }
    }


    /*
     * If there have been no errors, check if each of the types that
     * were forward referenced in case they were self pointing
     */
    if (error_count == 0)
    {
        /*
         * On the first pass resolve forward references on nodes with the
         * DEF_AS_TAG attribute.
         */
        for (tag_ref_node_ptr = ASTP_tag_ref_list; tag_ref_node_ptr;
            tag_ref_node_ptr = tag_ref_node_ptr->next)
        {
            if (AST_DEF_AS_TAG_SET(tag_ref_node_ptr->type_node_ptr)
                   /* allow for opaque type as above */
                && tag_ref_node_ptr->type_node_ptr->type_structure.structure)
            {
                AST_find_self_reference(interface_node_ptr,
                        NULL, tag_ref_node_ptr->type_node_ptr);
            }
        }

        /*
         * Go through the list again so we can make sure that types that
         * are defined_as struct/union tag refs can be set self_pointer too.
         */
        while (ASTP_tag_ref_list != NULL)
        {
            AST_type_n_t  *tp;

            if (!AST_DEF_AS_TAG_SET(ASTP_tag_ref_list->type_node_ptr) &&
                !AST_SELF_POINTER_SET(ASTP_tag_ref_list->type_node_ptr))
            {
                AST_find_self_reference(interface_node_ptr,
                        NULL, ASTP_tag_ref_list->type_node_ptr);

                /*
                 * check to see if the type node from which this type was
                 * derived is a self_pointer node.  If so, this node is
                 * self-pointing also.
                 */
                tp = ASTP_tag_ref_list->type_node_ptr->fe_info->tag_ptr;
                if ((tp != NULL) && AST_SELF_POINTER_SET(tp))
                {
                    ASTP_process_sp_type(interface_node_ptr, ASTP_tag_ref_list->type_node_ptr);
                }
            }

            /* Free the tag ref node */
            tag_ref_node_ptr = ASTP_tag_ref_list;
            ASTP_tag_ref_list = ASTP_tag_ref_list->next;
            FREE(tag_ref_node_ptr);
        }
    }

    return;
}

/*---------------------------------------------------------------------*/


/*
 *  A S T P _ c h e c k _ c h a i n
 *  ===============================
 *  Check the specified chain for the current type node.
 *  If found, set it and all that follow it on the chain
 *  to have the [self_pointer] attribute, and link it on the
 *  sp_types list in the interface node.
 *
 *  If the current_type node is already self_pointing, then
 *  mark all ancestors in the chain self_pointing also.
 *
 */

static int ASTP_check_chain
(
    AST_interface_n_t *interface_node_ptr,
    AST_type_p_n_t *active_type_chain,
    AST_type_n_t *current_type_node_ptr
)
{
    AST_type_p_n_t  *tp;                /* pointer into current chain */
    int             found;              /* current node found on active chain */

    /*
     * If the specified type is already self pointing, then mark all of the
     * active type chain as self_pointing as they contain a self pointing type.
     */
    if (AST_SELF_POINTER_SET(current_type_node_ptr))
        found = TRUE;
    else
        found = FALSE;

    /*
     * Loop the active type chain.  If the current type is found then
     * we know all type on the active list are self pointing.
     */
    for (tp = active_type_chain; tp; tp = tp->next)
    {
        /* If the current type is in the active chain, found a self_pointer */
        if (tp->type == current_type_node_ptr) found = TRUE;
    }

    /*
     *  If the chain is self pointing, then loop over the active type chain
     *  again marked the types as self pointing.
     */
    if (found)
    {
        for (tp = active_type_chain; tp; tp = tp->next)
            ASTP_process_sp_type(interface_node_ptr, tp->type);
    }

    return found;
}

/*---------------------------------------------------------------------*/


/*
 *  A S T _ f i n d _ s e l f _ r e f e r e n c e
 *  ==================================================
 *  Set the [self_point] attribute of the target type
 *  node if can (directly or indirectly) point to
 *  itself.
 *
 */

static void AST_find_self_reference
(
    AST_interface_n_t *interface_node_ptr,
    AST_type_p_n_t *active_type_chain,
    AST_type_n_t *current_type_node_ptr
)
{
    AST_type_p_n_t  link_node;
    AST_field_n_t   *fp;
    AST_arm_n_t     *ap;


    /*
     * If the current type node exists in the active type chain, then
     * it and all types that follow it in the active type chain are self
     * pointing.
     */
     if (ASTP_check_chain(interface_node_ptr, active_type_chain,
                        current_type_node_ptr)) return;


    /*
     * Link the current node into the active chain
     */
     link_node.next = active_type_chain;
     link_node.type = current_type_node_ptr;


    /*
     * Otherwise, check subtypes of this type
     */
    switch (current_type_node_ptr->kind)
    {
        case AST_pointer_k:
            AST_find_self_reference(interface_node_ptr, &link_node,
                current_type_node_ptr->type_structure.pointer->pointee_type);
            break;
        case AST_structure_k:
            {
            /*
             * For a struct also have to link in the DEF_AS_TAG type node
             * because a any references to that cause self references too.
             */
            AST_type_p_n_t  tag_link_node;
            AST_type_p_n_t  *tag_link_node_ptr = &tag_link_node;
            tag_link_node.next = &link_node;
            tag_link_node.type = current_type_node_ptr->fe_info->tag_ptr;

            /*
             * If tag_ptr is not filled-in then there is no DEF_AS_TAG node
             * to check, so don't link in the tag_link_node for the tag.
             */
            if (tag_link_node.type == NULL)
                  tag_link_node_ptr = &link_node;

            /* Loop through each field, checking type for self references */
            for (fp = current_type_node_ptr->type_structure.structure->fields;
                fp; fp = fp->next)
                AST_find_self_reference(interface_node_ptr,
                                        tag_link_node_ptr, fp->type);
            break;
            }
        case AST_disc_union_k:
            {
            /*
             * For a struct also have to link in the DEF_AS_TAG type node
             * because a any references to that cause self references too.
             */
            AST_type_p_n_t  tag_link_node;
            AST_type_p_n_t  *tag_link_node_ptr = &tag_link_node;
            tag_link_node.next = &link_node;
            tag_link_node.type = current_type_node_ptr->fe_info->tag_ptr;

            /*
             * If tag_ptr is not filled-in then there is no DEF_AS_TAG node
             * to check, so don't link in the tag_link_node for the tag.
             */
            if (tag_link_node.type == NULL)
                  tag_link_node_ptr = &link_node;

            /* Loop through each arm, checking type for self references */
            for (ap = current_type_node_ptr->type_structure.disc_union->arms;
                ap; ap = ap->next)
            {
                if (ap->type != NULL)
                {
                    AST_find_self_reference(interface_node_ptr,
                                    tag_link_node_ptr, ap->type);
                }
            }
            break;
            }
        case AST_array_k:
            AST_find_self_reference(interface_node_ptr, &link_node,
                current_type_node_ptr->type_structure.array->element_type);
            break;
        default:
            break;
    }
}

/*---------------------------------------------------------------------*/



/*
 *  A S T P _ p r o c e s s _ s p _ t y p e
 *  =======================================
 *
 *  This routine provides the processing needed for types with the
 *  [self_pointer] attribute.  If not done already, It sets the attribute on
 *  the specified type node, and by creating a type pointer node, links it on
 *  the sp_types list of the specified interface.  If the type is a pointer and
 *  it has an array_rep_type, process that as self-pointing as well.
 *
 *  Inputs:
 *      interface_node_ptr -- node to which the type is added to the sp_types
 *      type_node_ptr -- The newly found self_pointer type.
 */

static void ASTP_process_sp_type
(
    AST_interface_n_t *interface_node_ptr,
    AST_type_n_t *type_node_ptr
)
{
    AST_type_p_n_t *tp_node; /* type pointer node to link on chain */

    /*
     * If the type is already on the list, then don't do anything.
     */
     if (FE_TEST(type_node_ptr->fe_info->flags,FE_SELF_POINTING)) return;


    /*
     * Mark the specified node as self-pointing
     */
    AST_SET_SELF_POINTER(type_node_ptr);

    /*
     * If this is an arrayified pointer, then it is self-pointing because of
     * what it points at so process the array rep type as a self pointing
     * type as well.
     */
    if ((type_node_ptr->kind == AST_pointer_k) &&
        (type_node_ptr->type_structure.pointer->pointee_type->array_rep_type != NULL))
            ASTP_process_sp_type(interface_node_ptr,
                    type_node_ptr->type_structure.pointer->pointee_type->array_rep_type);


    /*
     *  Don't want both the def_as_tag node and the assoicated type node on the
     *  list.  Put only the original on the sp list, but mark the def_as_tag
     *  node as if it were.
     */
     if (AST_DEF_AS_TAG_SET(type_node_ptr) &&
         (type_node_ptr->fe_info->original != NULL))
     {
        FE_SET(type_node_ptr->fe_info->flags,FE_SELF_POINTING);
        type_node_ptr = type_node_ptr->fe_info->original;
        AST_SET_SELF_POINTER(type_node_ptr);
        FE_SET(type_node_ptr->fe_info->flags,FE_SELF_POINTING);
     }


    /*
     * If the node is an anonymous pointer then it does not need to be
     * on the sp list unless it is pointed at.  Routines are generated only
     * for the pa type not the pointer that points at it, so for now do
     * not put the pointer on the list.  If the pointer itself needs to
     * be on the list, it will be added during propagation when pa types
     * contained in sp types are processed.
     */
    if ((type_node_ptr->kind == AST_pointer_k) &&
        (type_node_ptr->name == NAMETABLE_NIL_ID)) return;

    /*
     * Create a new type pointer node and link it on the sp_types list
     * of the interface node.
     */
    tp_node = AST_type_ptr_node();
    tp_node->type = type_node_ptr;
    FE_SET(type_node_ptr->fe_info->flags,FE_SELF_POINTING);

    /* link it on the sp_types list of the interface node */
    interface_node_ptr->sp_types = (AST_type_p_n_t *)AST_concat_element(
                        (ASTP_node_t *)interface_node_ptr->sp_types,
                        (ASTP_node_t *)tp_node);
}

/*---------------------------------------------------------------------*/
/* preserve coding style vim: set tw=78 sw=4 : */
