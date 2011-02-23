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
**      propagat.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Propagates attributes that were not propagated by earlier compiler
**  phases throughout the Abstract Syntax Tree.
**
*/

#include <nidl.h>               /* Standard IDL defs */
#include <propagat.h>           /* Functions exported by this module */

#include <ast.h>                /* Abstract Syntax Tree defs */
#include <astp.h>               /* AST processing defs */
#include <checker.h>            /* Type and constant macros */
#include <command.h>            /* Command options */
#include <errors.h>             /* Error reporting defs */
#include <nidlmsg.h>            /* Error codes */
#include <ddbe.h>               /* Backend macros */

/*
 * Local storage
 */
static AST_type_p_n_t *PROP_up_types_list;

/* Local copy of pointers to command line information. */

static boolean      *cmd_opt;   /* Array of command option flags */
static void         **cmd_val;  /* Array of command option values */

/*
 * Macro to identify types that can define a context handle attribute.
 */
#define type_can_contain_context(type_p) \
    (   (type_p)->kind == AST_array_k \
    ||  (type_p)->kind == AST_pointer_k )

/*
 * Macro to set the first argument to the larger of the two numbers
 */
#define COPY_IF_LARGER(a,b) {if ((b) > (a)) (a) = (b);}


/*
 * Macro to identify types that can contain mutable pointers.
 */
#define type_can_contain_pointer(type_p) \
    (   (type_p)->kind == AST_array_k \
    ||  (type_p)->kind == AST_structure_k \
    ||  ((type_p)->kind == AST_pointer_k \
      && (type_p)->type_structure.pointer->pointee_type->kind != AST_void_k \
      && (type_p)->type_structure.pointer->pointee_type->kind != AST_interface_k \
	) \
    ||  (type_p)->kind == AST_disc_union_k)


/*
 * Structure to allow us to keep track of type nodes we've visited.
 */
typedef struct visit_t
{
    struct visit_t  *next;
    AST_type_n_t    *type;
} visit_t;

static visit_t *visited_list;   /* List of visited types */


/*
 * Structure to pass context which will allow mutli-purpose propagation
 * routines.
 */
typedef struct
{
      AST_interface_n_t     *int_p;           /* interface node */

      ASTP_node_t           *instance_p;      /* instance node that can     */
                                              /* have type flags            */

      AST_type_n_t          *parent_type_p;   /* parent type of instance    */
                                              /* node, e.g. struct type     */
                                              /* node, or NULL              */

      AST_parameter_n_t     *param_p;         /* parameter node that can    */
                                              /* for findind in/out flags   */

      boolean               toplevel_ref_param; /* true if the type being   */
                                                /* checked is a toplevel    */
                                                /* with [ref]               */

      boolean               toplevel_param;   /* true if the type being     */
                                              /* checked is a toplevel      */

      boolean               typedef_dcl;      /* true if the type being     */
                                              /* processed is a typedef as  */
                                              /* opposed to a parameter     */

      boolean               has_rep_as;       /* true if the type processed */
                                              /* is or contains a type with */
                                              /* the represent_as attribute */

      boolean               ct_cs_char;       /* true if the type processed */
                                              /* contains, not merely is, a */
                                              /* type with cs_char attribute*/

      boolean               has_xmit_as;      /* true if the type processed */
                                              /* is or contains a type with */
                                              /* the transmit_as attribute  */

      boolean               in_xmitted;       /* true if the type processed */
                                              /* is within a transmitted    */
                                              /* type specified in a        */
                                              /* transmit_as attribute      */

      boolean               has_v1_attr;      /* true if type has or        */
                                              /* contains a V1-specific     */
                                              /* attribute                  */

      boolean               has_v2_attr;      /* true if type has or        */
                                              /* contains a V2-specific     */
                                              /* attribute                  */

      boolean               in_aux;           /* true if type is enclosed   */
                                              /* by a self-pointing or ool  */
                                              /* type and thus is in aux    */

      boolean               under_ptr;        /* true if type is enclosed   */
                                              /* by a non-ref pointer and   */
                                              /* thus the pa routine is not */
                                              /* needed just to allocate    */
                                              /* the ref pointee instance   */

      /* for types below, "has" implies "is or contains" */

      boolean               has_enum;         /* T => type has enum         */
      boolean               has_union;        /* T => type has union        */
      boolean               has_vary_array;   /* T => type has varying array*/
      boolean               has_char;         /* T => type has character    */
      boolean               has_float;        /* T => type has float|double */
      boolean               has_int;          /* T => type has integer      */
      boolean               has_error_status; /* T => type has error_status_t */
      boolean               has_v1_struct;    /* T => type has v1_struct    */
		boolean					 has_interface;

} prop_ctx_t;


/*
 * Necessary forward function declarations.
 */
static void PROP_type_info(
    AST_type_n_t        *type_p,         /* [in] Ptr to AST type node */
    prop_ctx_t          *ctx             /* [in,out] ptr prop context */
);

/*
**  t y p e _ v i s i t
**
**  Marks a type as "visited" by saving the type node address in a linked list.
**
**  Implicit Inputs:    visited_list - a list of type nodes that we've visited.
*/

static void type_visit
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    visit_t             *visit_p;

    visit_p = NEW (visit_t);

    visit_p->next = visited_list;
    visit_p->type = type_p;

    visited_list = visit_p;
}


/*
**  t y p e _ u n v i s i t
**
**  "Unvisits" a type by removing its entry from a linked list.
**
**  Implicit Inputs:    visited_list - a list of type nodes that we've visited.
*/

static void type_unvisit
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    visit_t             *visit_p, *prev_p;

    prev_p = NULL;

    for (visit_p = visited_list ; visit_p != NULL ; visit_p = visit_p->next)
    {
        if (visit_p->type == type_p)
        {
            if (prev_p == NULL)
                /* Remove first entry */
                visited_list = visit_p->next;
            else
                /* Remove subsequent entry */
                prev_p->next = visit_p->next;

            /* Free removed entry */
            FREE(visit_p);
            return;
        }

        prev_p = visit_p;
    }
}


/*
**  t y p e _ v i s i t e d
**
**  Returns TRUE if a type node has already been visited.
**
**  Implicit Inputs:    visited_list - a list of type nodes that we've visited.
*/

static boolean type_visited
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    visit_t             *visit_p;

    for (visit_p = visited_list ; visit_p != NULL ; visit_p = visit_p->next)
        if (visit_p->type == type_p)
            return TRUE;

    return FALSE;
}


/*
**  t y p e _ v i s i t _ f r e e
**
**  Frees allocated storage on the visited list.
**
**  Implicit Inputs:    visited_list - a list of type nodes that we've visited.
*/

static void type_visit_free
(void)

{
    visit_t             *visit_p;
    visit_t             *t_visit_p;

    visit_p = visited_list;

    while (visit_p != NULL)
    {
        t_visit_p = visit_p;
        visit_p = visit_p->next;
        FREE(t_visit_p);
    }
}


/*
**  P R O P _ s e t _ n f _ c s _ c h a r _ p a r a m
**  
**  Given a parameter that is a non-fixed array of [cs_char] base type:
**   a) sets a flag on the first [in] and first [out] parameter(s) in the
**      operation.
**   b) sets a flag on the last [in] and last [out] parameters(s) in the
**      operation.
**  The flags indicate that an operation contains an [in] and/or [out] non-fixed
**  array of [cs_char], and placing it on the relevant parameter(s) puts it in
**  the most convenient place for the backend.
**
**  Also,
**   c) If the parameter is [out]-only, and references a parameter 'sz' in a
**      [size_is(sz)] attribute, and 'sz' is [in], bump a special reference
**      count on 'sz' to count the occurence.
*/

static void PROP_set_nf_cs_char_param
(
    AST_parameter_n_t   *cs_param_p     /* [in] Ptr to AST parameter node */
)

{
    AST_operation_n_t   *op_p;
    AST_parameter_n_t   *param_p, *last_param_p;
    boolean             done_first, got_out_allocate;

    op_p = cs_param_p->uplink;

    /*
     * Some parameters don't have any type vector representation, so we need to
     * assure we don't set a flag on such a parameter.  The cases should agree
     * with similar logic near the start of DDBE_gen_param_reps in ddbe.c.
     * Any parameter that is [out]-only but requires server side allocation as
     * part of [in]s processing (DDBE_ALLOCATE) effectively becomes the last
     * [in] parameter.  Need to consider this as we process the [in]s flags.
     */
    if (AST_IN_SET(cs_param_p) || DDBE_ALLOCATE(cs_param_p))
    {
        done_first = FALSE;
        got_out_allocate = FALSE;
        last_param_p = NULL;
        for (param_p = op_p->parameters; param_p; param_p = param_p->next)
        {
            /*
             * A binding handle_t parameter by value or ref has no vector rep.
             * Note: A binding handle_t parameter with [represent_as] also has
             * no vector rep (for this case, the necessary work is done in stubs
             * rather than in Interpreter).
             */
            if (param_p == op_p->parameters /* first parameter */
                && (param_p->type->kind == AST_handle_k
                    || (param_p->type->kind == AST_pointer_k
                        && param_p->type->type_structure.pointer->pointee_type
                           ->kind == AST_handle_k)))
                continue;
                
            if (AST_IN_SET(param_p))
            {
                if (!done_first)
                {
                    FE_SET(param_p->fe_info->flags, FE_FIRST_IN_NF_CS_ARR);
                    done_first = TRUE;
                }
            }
            if (DDBE_ALLOCATE(param_p))
            {
                last_param_p = param_p;
                got_out_allocate = TRUE;
            }
            else if (AST_IN_SET(param_p) && !got_out_allocate)
                last_param_p = param_p;
        }
        if (last_param_p != NULL)
            FE_SET(last_param_p->fe_info->flags, FE_LAST_IN_NF_CS_ARR);
    }

    if (AST_OUT_SET(cs_param_p))
    {
        done_first = FALSE;
        last_param_p = NULL;
        for (param_p = op_p->parameters; param_p; param_p = param_p->next)
        {
            /*
             * An ACF-added [comm_status] or [fault_status] parameter
             * has no vector rep.
             */
            if (AST_ADD_COMM_STATUS_SET(param_p)
                || AST_ADD_FAULT_STATUS_SET(param_p))
                continue;

            if (AST_OUT_SET(param_p))
            {
                if (!done_first)
                {
                    FE_SET(param_p->fe_info->flags, FE_FIRST_OUT_NF_CS_ARR);
                    done_first = TRUE;
                }
                last_param_p = param_p;
            }
        }
        if (last_param_p != NULL)
            FE_SET(last_param_p->fe_info->flags, FE_LAST_OUT_NF_CS_ARR);
    }

    /* Cond. bump special refcount for case explained in header comment */
    if (DDBE_ALLOCATE(cs_param_p)
        && cs_param_p->field_attrs != NULL
        && cs_param_p->field_attrs->size_is_vec != NULL
        && cs_param_p->field_attrs->size_is_vec[0].valid
        && cs_param_p->field_attrs->size_is_vec[0].constant == false)
    {
        (cs_param_p->field_attrs->size_is_vec[0].ref.p_ref->
         fe_info->ref_count_a)++;
    }
}

/*
**  P R O P _ s e t _ u s e d _ a s _ r e g _ f l d _ a t t r
**
**  Given a parameter or field instance with field attributes, sets flags on
**  any parameters/fields that are referenced as field attributes.
**  Assumptions: Correspondence of fields in field, param, and fe_info nodes.
*/

static void PROP_set_used_as_reg_fld_attr
(
    AST_instance_n_t    *inst_p         /* [in] Ptr to instance node */
)

{
    AST_field_attr_n_t  *fattr_p;
    unsigned short      max_dim;
    unsigned short      dim;

    fattr_p = inst_p->field_attrs;

    if (inst_p->type->kind == AST_array_k)
        max_dim = inst_p->type->type_structure.array->index_count;
    else
        max_dim = 1;

    for (dim = 0; dim < max_dim; dim++)
    {
        if (fattr_p->min_is_vec != NULL && fattr_p->min_is_vec[dim].valid &&
            fattr_p->min_is_vec[dim].constant == false)
            FE_SET(fattr_p->min_is_vec[dim].ref.p_ref->fe_info->flags,
                   FE_USED_AS_REG_FLD_ATTR);
        if (fattr_p->max_is_vec != NULL && fattr_p->max_is_vec[dim].valid &&
            fattr_p->max_is_vec[dim].constant == false)
            FE_SET(fattr_p->max_is_vec[dim].ref.p_ref->fe_info->flags,
                   FE_USED_AS_REG_FLD_ATTR);
        if (fattr_p->size_is_vec != NULL && fattr_p->size_is_vec[dim].valid &&
            fattr_p->size_is_vec[dim].constant == false)
            FE_SET(fattr_p->size_is_vec[dim].ref.p_ref->fe_info->flags,
                   FE_USED_AS_REG_FLD_ATTR);
        if (fattr_p->first_is_vec != NULL && fattr_p->first_is_vec[dim].valid &&
            fattr_p->first_is_vec[dim].constant == false)
            FE_SET(fattr_p->first_is_vec[dim].ref.p_ref->fe_info->flags,
                   FE_USED_AS_REG_FLD_ATTR);
        if (fattr_p->last_is_vec != NULL && fattr_p->last_is_vec[dim].valid &&
            fattr_p->last_is_vec[dim].constant == false)
            FE_SET(fattr_p->last_is_vec[dim].ref.p_ref->fe_info->flags,
                   FE_USED_AS_REG_FLD_ATTR);
        if (fattr_p->length_is_vec != NULL && fattr_p->length_is_vec[dim].valid &&
            fattr_p->length_is_vec[dim].constant == false)
            FE_SET(fattr_p->length_is_vec[dim].ref.p_ref->fe_info->flags,
                   FE_USED_AS_REG_FLD_ATTR);
    }
}

/*
**  P R O P _ s e t _ t y p e _ a t t r
**  =======================================
**
**  This routine accepts a type and an attribute mask value and
**  sets the attribute on the type and any related types if they
**  exist.  Specifically, if this is a structure/union it will
**  set the attribute on the associate def-as-tag node.
**
**  Inputs:
**      type_node_ptr -- The type node to set an attribute on.
**      type_attr -- Attribute value to set
*/

void PROP_set_type_attr
(
    AST_type_n_t *type_node_ptr,
    AST_flags_t  type_attr
)
{

      /* Set the attribute on the type */
      type_node_ptr->flags |= type_attr;

      /* Check for associate def-as-tag node */
      if (type_node_ptr->fe_info->tag_ptr != NULL)
          type_node_ptr->fe_info->tag_ptr->flags |= type_attr;

      /* Check for original associated with the def-as-tag node */
      if (type_node_ptr->fe_info->original != NULL)
          type_node_ptr->fe_info->original->flags |= type_attr;
}


/*
**  P R O P _ s e t _ t y p e _ u s a g e _ a t t r
**  ===============================================
**
**  This routine accepts a type and an attribute mask value and
**  sets the attribute on the type and any related types if they
**  exist.  Specifically, if this is a structure/union it will
**  set the attribute on the associate def-as-tag node, and it
**  will go up to the original of this type and apply it there
**  also.
**
**  Inputs:
**      type_node_ptr -- The type node to set an attribute on.
**      type_attr -- Attribute value to set
*/

static void PROP_set_type_usage_attr
(
    AST_type_n_t *type_node_ptr,
    AST_flags_t  type_attr
)
{
      if ((type_node_ptr->kind == AST_pointer_k) &&
         (type_node_ptr->type_structure.pointer->pointee_type->array_rep_type != NULL))
              type_node_ptr->type_structure.pointer->pointee_type->array_rep_type->flags |= type_attr;


      /* Call other routine to do the rest */
      PROP_set_type_attr(type_node_ptr, type_attr);
}


/*
**  P R O P _ p r o c e s s _ p a _ t y p e
**  =======================================
**
**  This routine provides the processing needed for types that are pointed at.
**  If not done already, it sets the pointed_at flag in the fe_info block on
**  the specified type node so that it is not entered on the pointed_at list
**  multiple times, and by creating a type pointer node, links it on a
**  the ASTP_pa_types_list.  Since we have no access to the interace node, the
**  pointed_at types list must be global until parsing of the interface is
**  complete and upon creation of the interface node, the list is filled in and
**  the global pointer set to NULL.
**
**  Inputs:
**      type_node_ptr -- The newly found pointed_at type.
**      pa_types -- pa type list to add type to
*/

static void PROP_process_pa_type
(
    AST_type_n_t *type_node_ptr,
    AST_type_p_n_t **pa_types,
    prop_ctx_t          *ctx             /* [in,out] ptr prop context */
)
{
    AST_type_p_n_t *tp_node; /* type pointer node to link on chain */

    /*
     * pa_types should be set only for types used in an operation.
     * If this is not an operation context, then return
     */
    if (ctx->typedef_dcl) return;

    /*
     *  If the type is already marked pointed-at or self_pointing, then don't
     *  do anything.  The only special case is if it is an anonymous self_pointing
     *  type and then we must put it on the pa list so we can generate a static
     *  copy of the pa routine in the stub.
     */
     if (AST_SELF_POINTER_SET(type_node_ptr) &&
         (type_node_ptr->name != NAMETABLE_NIL_ID))
        return;

    /* Context handle types are not really pointed at */
    if (AST_CONTEXT_RD_SET(type_node_ptr))
        return;

    /*
     *  We now know that the type is really pointed at in a parameter.  If the
     *  context indicates that we are pointed-at in the stub (or if the type is
     *  an anonymous & self-pointing) then mark it as being needed in the stub.
     */
    if (!ctx->in_aux ||
        !ctx->under_ptr ||
        (AST_SELF_POINTER_SET(type_node_ptr) &&
         (type_node_ptr->name == NAMETABLE_NIL_ID)))
    {
        /*
         * If parent is sp or ool, then in this instance the pa routine is
         * needed in the aux file.  Set the in/out_pa_aux flags.
         */
        if (AST_IN_SET(ctx->param_p)) PROP_set_type_usage_attr(type_node_ptr,AST_IN_PA_STUB);
        if (AST_OUT_SET(ctx->param_p)) PROP_set_type_usage_attr(type_node_ptr,AST_OUT_PA_STUB);
    }

    /*
     * Already on the PA list, so nothing more to do.
     */
    if (FE_TEST(type_node_ptr->fe_info->flags,FE_POINTED_AT)) return;

    /*
     *  Don't want both the def_as_tag node and the assoicated type node on the
     *  list.  Put only the original on the pa list, but mark the def_as_tag
     *  node as if it were.
     */
     if (AST_DEF_AS_TAG_SET(type_node_ptr) && type_node_ptr->fe_info->original)
     {
        FE_SET(type_node_ptr->fe_info->flags,FE_POINTED_AT);
        type_node_ptr = type_node_ptr->fe_info->original;
     }

    /*
     * Mark the specified node as pointed_at to prevent it from being processed
     * again.
     */
    FE_SET(type_node_ptr->fe_info->flags,FE_POINTED_AT);

    /*
     * If the pointee is a scalar type, void, or handle, no need to add it
     * to the pa_list unless it has a [transmit_as] type.
     */
    if (type_node_ptr->xmit_as_type == NULL
        && (type_is_scalar(type_node_ptr) || type_node_ptr == ASTP_void_ptr
            || type_node_ptr == ASTP_handle_ptr))
         return;

    /*
     * Create a new type pointer node and link it on the pa_types list
     * of the interface node.
     */
    tp_node = AST_type_ptr_node();
    tp_node->type = type_node_ptr;

    /* link it on the pa_types list of the interface node */
    *pa_types = (AST_type_p_n_t *)AST_concat_element(
                        (ASTP_node_t *)*pa_types,
                        (ASTP_node_t *)tp_node);

}

/*
**  P R O P _ p r o c e s s _ u p _ t y p e
**  =======================================
**
**  This routine provides the processing needed for union types that contain
**  pointers.
**
**  Inputs:
**      type_node_ptr -- The newly found up type.
**  Globals:
**      PROP_up_types_list -- The propagat global to hold list.
*/

void PROP_process_up_type
(
    AST_type_n_t *type_node_ptr
)
{
    AST_type_p_n_t *tp_node; /* type pointer node to link on chain */

    /*
     * Create a new type pointer node and link it on the unions with ptrs list.
     */
    tp_node = AST_type_ptr_node();
    tp_node->type = type_node_ptr;

    PROP_up_types_list = (AST_type_p_n_t *)AST_concat_element(
                        (ASTP_node_t *)PROP_up_types_list,
                        (ASTP_node_t *)tp_node);
}

/*
**  t y p e _ c o n t a i n s _ c o n t e x t
**
**  Recursive routine used to chase a type's node and
**  any types that it points to for the context_rd, context w/ rundown
**  attribute.  TRUE is returned at the first context found.
**
**  Implicit Inputs:    visited_list - a list of type nodes that we've
**                      already visited in the recursion stack
*/

static boolean type_contains_context
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{


    /* If type has context, return TRUE */
    if (AST_CONTEXT_RD_SET(type_p))
    {
        return TRUE;
    }

    /* If context attribute does not apply to this type, return FALSE */
    if (!type_can_contain_context(type_p))
    {
        return FALSE;
    }

    /*
     * If the type has already been visited, return FALSE.
     * This avoids user's error containing structures that
     * refer to each other.
     */
    if (type_visited(type_p))
        return FALSE;
    else
        type_visit(type_p);     /* Mark as visited */

    /* Process all valid types, returning at first sign of context handle */

    switch (type_p->kind)
    {
        case AST_pointer_k:
            /* Already know context handle not set; chase the pointer. */
            if (type_contains_context(type_p->type_structure.pointer->pointee_type))
            {
                return TRUE;
            }
            break;

        case AST_array_k:
            /* Chase the array element type. */
            if (AST_CONTEXT_RD_SET(type_p->type_structure.array->element_type)
                || type_contains_context(type_p->type_structure.array->element_type))
            {
                return TRUE;
            }
            break;

/*
 * The structure and union code will never be visited since we currently disallow
 * context handles in fields of structures and arms of unions
 */
        case AST_structure_k:
        {
            AST_field_n_t       *field_p;   /* A field in the structure */

            /*
             * If any field has the context attribute or its type
             * defines a context handle, return TRUE
             */
            for (field_p = type_p->type_structure.structure->fields;
                 field_p != NULL;
                 field_p = field_p->next)
            {
                if (AST_CONTEXT_SET(field_p) ||
                    type_contains_context(field_p->type))
                {
                    return TRUE;
                }
            }

            break;
        }

        case AST_disc_union_k:
        {
            AST_arm_n_t         *arm_p;     /* An arm in the union */

            /*
             * Chase all arms in the union.  Again, if any arm has the
             * context attribute, or its type defines a context handle,
             * return TRUE.
             */
            for (arm_p = type_p->type_structure.disc_union->arms;
                 arm_p != NULL;
                 arm_p = arm_p->next)
            {
                if (AST_CONTEXT_SET(arm_p)
                    ||  (arm_p->type != NULL
                        &&  type_contains_context(arm_p->type)))
                {
                    return TRUE;
                }
            }
            break;
        }

        default:
            break;
    }


    return FALSE;
}

/*
**  t y p e _ c o n t a i n s _ o o l
**
**  Recursive routine used to chase a type's node and
**  any types that contains for the [out_of_line] attribute.
**  TRUE is returned at the first ool found.
**
**  Implicit Inputs:    visited_list - a list of type nodes that we've
**                      already visited in the recursion stack
*/

static boolean type_contains_ool
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{


    /* If type has out-of-line, return TRUE */
    if (AST_OUT_OF_LINE_SET(type_p))
    {
        return TRUE;
    }


    /*
     * If type has a transmit_as type, check that for OOL instead of
     * the local type.
     */
    if (type_p->xmit_as_type != NULL)
        return type_contains_ool(type_p->xmit_as_type);


    /*
     * If the type has already been visited, return FALSE.
     * This avoids user's error containing structures that
     * refer to each other.
     */
    if (type_visited(type_p))
        return FALSE;
    else
        type_visit(type_p);     /* Mark as visited */

    /* Process all valid types, returning at first sign of out_of_line */

    switch (type_p->kind)
    {
        case AST_pointer_k:
            /* Already know out_of_line not set; chase the pointer. */
            if (type_contains_ool(type_p->type_structure.pointer->pointee_type))
            {
                return TRUE;
            }
            break;

        case AST_array_k:
            /* Chase the array element type. */
            if (AST_OUT_OF_LINE_SET(type_p->type_structure.array->element_type)
                || type_contains_ool(type_p->type_structure.array->element_type))
            {
                return TRUE;
            }
            break;

        case AST_structure_k:
        {
            AST_field_n_t       *field_p;   /* A field in the structure */

            /*
             * If any field's has the ool attribute or contains
             * a ool type, return TRUE
             */
            for (field_p = type_p->type_structure.structure->fields;
                 field_p != NULL;
                 field_p = field_p->next)
            {
                if (AST_OUT_OF_LINE_SET(field_p) ||
                    type_contains_ool(field_p->type))
                {
                    return TRUE;
                }
            }

            break;
        }

        case AST_disc_union_k:
        {
            AST_arm_n_t         *arm_p;     /* An arm in the union */

            /*
             * Chase all arms in the union.  Again, if any arm has the
             * ool return TRUE.
             */
            for (arm_p = type_p->type_structure.disc_union->arms;
                 arm_p != NULL;
                 arm_p = arm_p->next)
            {
                if (AST_OUT_OF_LINE_SET(arm_p)
                    ||  (arm_p->type != NULL
                        &&  type_contains_ool(arm_p->type)))
                {
                    return TRUE;
                }
            }
            break;
        }

        case AST_pipe_k:
            /* Chase the pipe base type. */
            if (AST_OUT_OF_LINE_SET(type_p->type_structure.pipe->base_type)
                || type_contains_ool(type_p->type_structure.pipe->base_type))
                return TRUE;
            break;

        default:
            break;
    }


    return FALSE;
}

/*
**  t y p e  _ c o n t a i n s _ c o n f o r m a n t
**
**  Recursive routine used to chase a type's structure node and
**  any nested structures for a field containing a conformant type.
**  If the [conformant] flag is set for a field type, the conformant
**  flag is set for all types down the recursion stack.
**
**  Implicit Inputs:    visited_list - a list of type nodes that we've
**                      already visited in the recursion stack
*/

static boolean type_contains_conformant
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    AST_field_n_t       *field_p;       /* A field in the structure */


    /* If type is not a structure, return, doesn't apply */
    if (!(type_p->kind == AST_structure_k))
    {
        return FALSE;
    }

    /*
     * If the type has already been visited, return FALSE.
     * This avoids user's error containing structures that
     * refer to each other.
     */
    if (type_visited(type_p))
        return FALSE;
    else
        type_visit(type_p);     /* Mark as visited */

    /*
     * If any field's type has a conformant array,
     * propagate it to structure node.  Note that a field
     * that is a pointer to a conformant array does NOT
     * make the structure conformant.
     */
    for (field_p = type_p->type_structure.structure->fields;
         field_p != NULL;
         field_p = field_p->next)
    {
        if ((AST_CONFORMANT_SET(field_p->type)
                && field_p->type->kind != AST_pointer_k)
            ||
            type_contains_conformant(field_p->type))
        {
            PROP_set_type_attr(type_p,AST_CONFORMANT);
            return TRUE;
        }

    }

    return FALSE;
}

/*
**  t y p e  _ p r o p _ p a r a m
**
**  Recursive propagation routine for propagating type attrs up to parameters.
**  See priming routine type_prop_up_to_param for further info.
**
**  Implicit Inputs:    visited_list - a list of type nodes that we've
**                      already visited in the recursion stack
*/

/*
 * Macro to return to caller if all relevant attributes are already set.
 */
#define TYPE_PROP_PARAM_EXIT_TEST \
    if (FE_TEST(param_p->fe_info->flags, FE_HAS_CFMT_ARR)) \
        return

static void type_prop_param
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *type_p,        /* [in] Ptr to AST type node */
    AST_field_attr_n_t  *fattr_p,       /* [in] Field attributes on instance */
    boolean             string_set,     /* [in] TRUE => [string] on instance */
    boolean             non_ref         /* [in] TRUE => not [ref] on instance */
)

{
    /*
     * If the type has already been visited, return.
     */
    if (type_visited(type_p))
        return;
    else
        type_visit(type_p);     /* Mark as visited */

    switch (type_p->kind)
    {
    case AST_pointer_k:
        /*
         * Don't recurse any further for a full pointer.  All of the FE flags
         * that can be set by this routine should NOT be set if they are
         * satisfied under a full pointer, since full pointers in [out]-only
         * parameters are written in manager code and thus the size of the
         * pointee needn't be known to the callee stub.  For further info,
         * see how these flags are used in checker code.
         */
        if (type_p == param_p->type)
        {
            if (AST_PTR_SET(param_p)) return;
        }
        else
            if (non_ref) return;

        /*
         * Pointer could satisfy attribute(s).  If not, chase pointer since
         * it could point to something that satisfies attribute(s).
         */
        if (type_p != param_p->type     /* Don't set for top-level type */
            &&  (string_set || AST_STRING_SET(type_p)))
            /* [string] arrayifies a pointer and makes type conformant. */
            FE_SET(param_p->fe_info->flags, FE_HAS_CFMT_ARR);

        if (type_p != param_p->type     /* Don't set for top-level type */
            &&  fattr_p != NULL
            &&  (fattr_p->min_is_vec != NULL
                ||  fattr_p->max_is_vec != NULL
                ||  fattr_p->size_is_vec != NULL))
            FE_SET(param_p->fe_info->flags, FE_HAS_CFMT_ARR);

        TYPE_PROP_PARAM_EXIT_TEST;

        type_prop_param(param_p, type_p->type_structure.pointer->pointee_type,
                        (AST_field_attr_n_t *)NULL, FALSE,
                        !AST_REF_SET(type_p->type_structure.pointer->pointee_type));
        break;

    case AST_array_k:
        /*
         * Array could be conformant.  If not, chase array element type since
         * it could be something that contains a conformant array.
         */
        if (type_p != param_p->type     /* Don't set for top-level type */
            &&  AST_CONFORMANT_SET(type_p))
            FE_SET(param_p->fe_info->flags, FE_HAS_CFMT_ARR);

        TYPE_PROP_PARAM_EXIT_TEST;

        type_prop_param(param_p, type_p->type_structure.array->element_type,
                        (AST_field_attr_n_t *)NULL, FALSE,
                        !AST_REF_SET(type_p->type_structure.array->element_type));
        break;

    case AST_structure_k:
    {
        /*
         * A field type in the structure could satisfy attribute(s).
         */
        AST_structure_n_t   *struct_p;  /* Ptr to structure node */
        AST_field_n_t       *field_p;   /* A field in the structure */

        struct_p = type_p->type_structure.structure;

        for (field_p = struct_p->fields
            ;   field_p != NULL
            ;   field_p = field_p->next)
        {
            type_prop_param(param_p, field_p->type, field_p->field_attrs,
                            AST_STRING_SET(field_p)!=0, !AST_REF_SET(field_p));
            TYPE_PROP_PARAM_EXIT_TEST;
        }
        break;
    }

    case AST_disc_union_k:
    {
        /*
         * An arm type in the union could satisfy attribute(s).
         */
        AST_disc_union_n_t  *union_p;   /* Ptr to union node */
        AST_arm_n_t         *arm_p;     /* An arm of the union */

        union_p = type_p->type_structure.disc_union;

        for (arm_p = union_p->arms
            ;   arm_p != NULL
            ;   arm_p = arm_p->next)
            if (arm_p->type != NULL)
            {
                type_prop_param(param_p, arm_p->type,
                                (AST_field_attr_n_t *)NULL,
                                AST_STRING_SET(arm_p)!=0, !AST_REF_SET(arm_p));
                TYPE_PROP_PARAM_EXIT_TEST;
            }
        break;
    }

    default:
        break;
    }

    return;
}

/*
**  t y p e _ c o n t a i n s _ p o i n t e r
**
**  Recursive routine used to chase a type node and any contained types
**  to conditionally set the synthesized pointer flags.
**
**  HAS_PTR         is set if the type has any pointers.
**  HAS_REF_PTR     is set if the type has any [ref] pointers.
**  HAS_UNIQUE_PTR  is set if the type has any [unique] pointers.
**  Both of these flags are used by the semantic checker.
**
**  Implicit Inputs:    visited_list - a list of type nodes that we've
**                      already visited in the recursion stack
*/

/*
 * Macro to recurse for a contained type and propagate return result
 * up to the parent type.
 */
#define C_PTR_TEST(c_type_p, early_exit) \
{ \
    prop_pointer_types(c_type_p, &c_ptr, &c_ref, &c_unique, &c_full); \
    if (c_ptr) \
    { \
        FE_SET(type_p->fe_info->flags, FE_HAS_PTR); \
        *ptr = TRUE; \
        if (c_ref || FE_TEST(c_type_p->fe_info->flags, FE_HAS_REF_PTR)) \
        { \
            FE_SET(type_p->fe_info->flags, FE_HAS_REF_PTR); \
            *ref = TRUE; \
        } \
        if (c_unique || FE_TEST(c_type_p->fe_info->flags, FE_HAS_UNIQUE_PTR)) \
        { \
            FE_SET(type_p->fe_info->flags, FE_HAS_UNIQUE_PTR); \
            *unique = TRUE; \
        } \
        if (c_full || FE_TEST(c_type_p->fe_info->flags, FE_HAS_FULL_PTR)) \
        { \
            FE_SET(type_p->fe_info->flags, FE_HAS_FULL_PTR); \
            *full = TRUE; \
        } \
        if (early_exit && *ref && *unique && *full) \
            return; \
    } \
}

static void prop_pointer_types
(
    AST_type_n_t        *type_p,        /* [in] Ptr to AST type node */
    boolean             *ptr,           /*[out] TRUE => contains ptr */
    boolean             *ref,           /*[out] TRUE => contains ref ptr */
    boolean             *unique,        /*[out] TRUE => contains unique ptr */
    boolean             *full           /*[out] TRUE => contains full ptr */
)

{
    boolean             c_ptr;          /* Contained type has ptr */
    boolean             c_ref;          /* Contained type has ref ptr */
    boolean             c_unique;       /* Contained type has unique ptr */
    boolean             c_full;         /* Contained type has full ptr */

    /* Set up default return values to already stored attributes. */
    *ptr    =  FE_TEST(type_p->fe_info->flags, FE_HAS_PTR);
    *ref    =  FE_TEST(type_p->fe_info->flags, FE_HAS_REF_PTR);
    *unique =  FE_TEST(type_p->fe_info->flags, FE_HAS_UNIQUE_PTR);
    *full   =  FE_TEST(type_p->fe_info->flags, FE_HAS_FULL_PTR);

    /*
     * If a type to which pointers can't apply, or if the type has already
     * been visited, return with the already stored attributes.
     */
    if (!type_can_contain_pointer(type_p) || type_visited(type_p) || *ptr)
        return;

    type_visit(type_p);         /* Mark type as visited */

    switch (type_p->kind)
    {
    case AST_pointer_k:
        FE_SET(type_p->fe_info->flags, FE_HAS_PTR);
        *ptr = TRUE;

        if (AST_REF_SET(type_p)
            ||  FE_TEST(type_p->fe_info->flags, FE_HAS_REF_PTR))
        {
            FE_SET(type_p->fe_info->flags, FE_HAS_REF_PTR);
            *ref = TRUE;
        }

        if (AST_UNIQUE_SET(type_p)
            ||  FE_TEST(type_p->fe_info->flags, FE_HAS_UNIQUE_PTR))
        {
            FE_SET(type_p->fe_info->flags, FE_HAS_UNIQUE_PTR);
            *unique = TRUE;
        }

        if (AST_PTR_SET(type_p)
            ||  FE_TEST(type_p->fe_info->flags, FE_HAS_FULL_PTR))
        {
            FE_SET(type_p->fe_info->flags, FE_HAS_FULL_PTR);
            *full = TRUE;
        }

        /*
         * If all of the pointer flags have been determined to be TRUE we can
         * stop the recursion.  Otherwise recurse with the pointee type.
         */
        if (*ref && *unique && *full)   /* *ptr known to be TRUE */
            return;

        C_PTR_TEST(type_p->type_structure.pointer->pointee_type, TRUE);
        break;

    case AST_structure_k:
    {
        AST_structure_n_t   *struct_p;  /* Ptr to structure node */
        AST_field_n_t       *field_p;   /* A field in the structure */

        struct_p = type_p->type_structure.structure;

        /* Chase all fields in the structure. */

        for (field_p = struct_p->fields
            ;   field_p != NULL
            ;   field_p = field_p->next)
        {
            if (AST_REF_SET(field_p))
                FE_SET(type_p->fe_info->flags, FE_HAS_REF_PTR);
            if (AST_PTR_SET(field_p))
                FE_SET(type_p->fe_info->flags, FE_HAS_PTR);
            if (AST_UNIQUE_SET(field_p))
                FE_SET(type_p->fe_info->flags, FE_HAS_UNIQUE_PTR);
            if (AST_PTR_SET(field_p))
                FE_SET(type_p->fe_info->flags, FE_HAS_FULL_PTR);
            C_PTR_TEST(field_p->type, TRUE);
        }
        break;
    }

    case AST_array_k:
        /* Chase the array element type. */
        C_PTR_TEST(type_p->type_structure.array->element_type, TRUE);
        break;

    case AST_disc_union_k:
    {
        AST_disc_union_n_t  *union_p;   /* Ptr to discriminated union node */
        AST_arm_n_t         *arm_p;     /* An arm in the union */

        union_p = type_p->type_structure.disc_union;

        /* Chase all arms in the union. */

        for (arm_p = union_p->arms
            ;   arm_p != NULL
            ;   arm_p = arm_p->next)
            if (arm_p->type != NULL)
            {
                if (AST_REF_SET(arm_p))
                    FE_SET(type_p->fe_info->flags, FE_HAS_REF_PTR);
                if (AST_PTR_SET(arm_p))
                    FE_SET(type_p->fe_info->flags, FE_HAS_PTR);
                if (AST_UNIQUE_SET(arm_p))
                    FE_SET(type_p->fe_info->flags, FE_HAS_UNIQUE_PTR);
                if (AST_PTR_SET(arm_p))
                    FE_SET(type_p->fe_info->flags, FE_HAS_FULL_PTR);
                C_PTR_TEST(arm_p->type, FALSE);
            }
        if (FE_TEST(type_p->fe_info->flags,FE_HAS_PTR))
            PROP_process_up_type(type_p);
        break;
    }

    default:
        break;
    }
}

/*
**  P R O P _ t y p e _ i n f o _ O R
**
**  Replaces destination propagation attributes with the logical OR of
**  its attributes and the source propagation attributes.
*/

static void PROP_type_info_OR
(
    prop_ctx_t  *dst_ctx,       /* [io] Destination propagation context */
    prop_ctx_t  *src_ctx        /* [in] Source propagation context */
)

{
    dst_ctx->toplevel_ref_param |= src_ctx->toplevel_ref_param;
    dst_ctx->toplevel_param     |= src_ctx->toplevel_param;
    dst_ctx->typedef_dcl        |= src_ctx->typedef_dcl;
    dst_ctx->has_rep_as         |= src_ctx->has_rep_as;
    dst_ctx->ct_cs_char         |= src_ctx->ct_cs_char;
    dst_ctx->has_xmit_as        |= src_ctx->has_xmit_as;
    dst_ctx->in_xmitted         |= src_ctx->in_xmitted;
    dst_ctx->has_v1_attr        |= src_ctx->has_v1_attr;
    dst_ctx->has_v2_attr        |= src_ctx->has_v2_attr;
    dst_ctx->in_aux             |= src_ctx->in_aux;
    dst_ctx->under_ptr          |= src_ctx->under_ptr;
    dst_ctx->has_enum           |= src_ctx->has_enum;
    dst_ctx->has_union          |= src_ctx->has_union;
    dst_ctx->has_vary_array     |= src_ctx->has_vary_array;
    dst_ctx->has_char           |= src_ctx->has_char;
    dst_ctx->has_float          |= src_ctx->has_float;
    dst_ctx->has_int            |= src_ctx->has_int;
    dst_ctx->has_error_status   |= src_ctx->has_error_status;
    dst_ctx->has_v1_struct      |= src_ctx->has_v1_struct;
    dst_ctx->has_interface      |= src_ctx->has_interface;
}

/*
**  P R O P _ t y p e _ u n i o n
**
**  Routine mutually recursive with PROP_type_info.  Processes each arm
**  of a union to set properties and propagate them to the union type.
*/

static void PROP_type_union
(
    AST_type_n_t        *type_p,         /* [in] Ptr to AST type node */
    prop_ctx_t          *ctx             /* [in,out] ptr prop context */
)

{
    AST_disc_union_n_t  *union_p;   /* Ptr to discriminated union node */
    AST_arm_n_t         *arm_p;     /* An arm in the union */
    prop_ctx_t  saved_parent_ctx;   /* Union type prop context */
    prop_ctx_t  member_ctx;         /* Member type prop context */

    union_p = type_p->type_structure.disc_union;

    /* Do size propagation */
    if (union_p->discrim_type != NULL) {
    COPY_IF_LARGER(type_p->ndr_size,union_p->discrim_type->ndr_size);
    COPY_IF_LARGER(type_p->alignment_size,union_p->discrim_type->alignment_size);
    }

    saved_parent_ctx = *ctx;

    /* Chase all arms in the union. */
    for (arm_p = union_p->arms
        ;   arm_p != NULL
        ;   arm_p = arm_p->next)
    {
        if (arm_p->type != NULL)
        {
            /*
             * If not under a ptr, and the arm is a pointer, set the
             * under_ptr flag if the arm doesn't have the [ref] attribute.
             */
            if (!ctx->under_ptr && (arm_p->type->kind == AST_pointer_k))
                ctx->under_ptr = !AST_REF_SET(arm_p);

            /* Recurse for member type */
            member_ctx = saved_parent_ctx;
            member_ctx.instance_p = (ASTP_node_t *)arm_p;
            member_ctx.parent_type_p = type_p;
            PROP_type_info(arm_p->type, &member_ctx);
            PROP_type_info_OR(ctx, &member_ctx);

            /* Do size propagation */
            COPY_IF_LARGER(type_p->alignment_size,arm_p->type->alignment_size);
        }
    }
}

/*
**  P R O P _ t y p e _ s t r u c t
**
**  Routine mutually recursive with PROP_type_info.  Processes each field
**  of a structure to set properties and propagate them to the structure type.
*/

static void PROP_type_struct
(
    AST_type_n_t        *type_p,         /* [in] Ptr to AST type node */
    prop_ctx_t          *ctx             /* [in,out] ptr prop context */
)

{
    AST_structure_n_t   *struct_p;  /* Ptr to structure node */
    AST_field_n_t       *field_p;   /* A field in the structure */
    prop_ctx_t  saved_parent_ctx;   /* Structure type prop context */
    prop_ctx_t  member_ctx;         /* Member type prop context */

    struct_p = type_p->type_structure.structure;
    type_p->ndr_size = 0;   /* re-init before recursing */

    saved_parent_ctx = *ctx;


    /* Chase all fields in the structure. */
    for (field_p = struct_p->fields
        ;   field_p != NULL
        ;   field_p = field_p->next)
    {
        /*
         * If not under a ptr, and the field is a pointer, set the
         * under_ptr flag if the field doesn't have the [ref] attribute.
         */
        if (!ctx->under_ptr && (field_p->type->kind == AST_pointer_k))
            ctx->under_ptr = !AST_REF_SET(field_p);

        /* Set flag if field is varying */
        if (AST_VARYING_SET(field_p))
            ctx->has_vary_array = TRUE;

        /* Recurse for member type */
        member_ctx = saved_parent_ctx;
        member_ctx.instance_p = (ASTP_node_t *)field_p;
        member_ctx.parent_type_p = type_p;
        PROP_type_info(field_p->type, &member_ctx);
        /*
         * If field is conformant or varying, set "depends on integer" flag to
         * account for the ABZ values on the wire.  Note this is only currently
         * done for field instances and not for parameter instances.  The
         * Interpreter currently only needs this information on struct types.
         */
        if (AST_CONFORMANT_SET(field_p->type) || AST_VARYING_SET(field_p))
            member_ctx.has_int = TRUE;
        PROP_type_info_OR(ctx, &member_ctx);

        /* Do size propagation */
        type_p->ndr_size += field_p->type->ndr_size;

        /*
         *  If the type has [v1_struct] (unalign) specified, set the
         *  alignment_size field to that of the first field, otherwise
         *  set it to the largest alignment of any field.
         */
        if (AST_UNALIGN_SET(type_p))
            type_p->alignment_size = struct_p->fields->type->alignment_size;
        else
            COPY_IF_LARGER(type_p->alignment_size,field_p->type->alignment_size);
    }
}

/*
**  P R O P _ t y p e _ i n f o
**
**  1) Recursive routine used to chase a type node and any contained types
**  to conditionally set the alignment_size and ndr_size fields to
**  the proper values.
**
**  Implicit Inputs:    visited_list - a list of type nodes that we've
**                      already visited in the recursion stack
*/

static void PROP_type_info
(
    AST_type_n_t        *type_p,         /* [in] Ptr to AST type node */
    prop_ctx_t          *ctx             /* [in,out] ptr prop context */
)

{
    boolean toplevel_ref_param;           /* Local copy of ctx->toplevel_ref_param */
    boolean toplevel_param;               /* Local copy of ctx->toplevel_param */
    boolean in_aux;                       /* Local copy of ctx->in_aux */
    boolean under_ptr;                    /* Local copy of ctx->under_ptr */
    ASTP_node_t *instance_p;              /* Local copy of instance_p     */
    AST_type_n_t *parent_p;               /* Local copy of parent_type_p  */

    /*
     * If the type has already been visited through a parent, return.
     */
    if (type_visited(type_p))
        return;
    else
        type_visit(type_p);     /* Mark as visited */

    /*
     * If the type is a [ptr] array and not a toplevel param, set type flag.
     * Further propagation is not needed since this flag only applies to
     * named types which are guaranteed to be processed by users of the flag.
     */
    if (type_p->kind == AST_array_k
        &&  type_p->name != NAMETABLE_NIL_ID
        &&  !ctx->toplevel_param
        &&  !ctx->typedef_dcl
        &&  AST_PTR_SET(type_p))
        FE_SET(type_p->fe_info->flags, FE_HAS_PTR_ARRAY);

    /*
     * Handle some properties that propagate downwards.
     */
    if (ctx->in_xmitted)
        FE_SET(type_p->fe_info->flags, FE_USED_IN_TRANSMITTED);

    /*
     * If the type has a V1-only or V2-only attribute, set a temporary flag.
     */
    if (AST_STRING0_SET(type_p)         /* [v1_string]  */
        ||  (AST_SMALL_SET(type_p) &&   /* [v1_array]   */
             (type_p->kind != AST_array_k || AST_CONFORMANT_SET(type_p)))
        ||  AST_V1_ENUM_SET(type_p)     /* [v1_enum]    */
        ||  AST_UNALIGN_SET(type_p))    /* [v1_struct]  */
        FE_SET(type_p->fe_info->flags, FE_HAS_V1_ATTR);

    if (AST_STRING_SET(type_p)
        ||  type_p->kind == AST_pipe_k
        ||  (type_p->kind == AST_pointer_k &&
              !ctx->typedef_dcl && !ctx->toplevel_ref_param)
        ||  (type_p->kind == AST_enum_k && !AST_V1_ENUM_SET(type_p))
        ||  (type_p->kind == AST_structure_k && !AST_UNALIGN_SET(type_p))
        ||  (type_is_anonymous(type_p) && ctx->instance_p == NULL &&
              type_p->kind == AST_array_k && AST_CONFORMANT_SET(type_p) &&
              !AST_SMALL_SET(type_p))
        ||  type_p->cs_char_type != NULL)
        FE_SET(type_p->fe_info->flags, FE_HAS_V2_ATTR);

    /*
     * If the current instance of this type has a V1-only or V2-only attribute,
     * set a flag in the instance node.  This is necessary because instances
     * can have attributes that affect (only this instance of) the type.
     */
    instance_p = ctx->instance_p;
    parent_p   = ctx->parent_type_p;
    ctx->instance_p    = NULL;
    ctx->parent_type_p = NULL;

    if (instance_p != NULL)
    {
        AST_instance_n_t *inst_p = (AST_instance_n_t *)instance_p;
        /*
         * For a conformant and/or varying array of [cs_char] type, set a flag
         * on the referenced [size_is] and/or [length_is] fields/parameters.
         * Also bump a reference count for these cases.
         * Assumptions: Such arrays limited to 1 dim and the above field attrs.
         * Correspondence of fields in field, param, and fe_info nodes.
         */
        if (type_p->kind == AST_array_k
            && type_p->type_structure.array->element_type->cs_char_type != NULL
            && inst_p->field_attrs != NULL)
        {
            AST_field_attr_n_t  *fattr_p = inst_p->field_attrs;

            if (fattr_p->size_is_vec != NULL)
            {
                FE_SET(fattr_p->size_is_vec->ref.p_ref->fe_info->flags,
                       FE_USED_AS_CS_FLD_ATTR);
                (fattr_p->size_is_vec->ref.p_ref->fe_info->ref_count)++;
            }
            if (fattr_p->length_is_vec != NULL)
            {
                FE_SET(fattr_p->length_is_vec->ref.p_ref->fe_info->flags,
                       FE_USED_AS_CS_FLD_ATTR);
                (fattr_p->length_is_vec->ref.p_ref->fe_info->ref_count)++;
            }

            /*
             * Set flags on parameters(s) or the structure type to indicate
             * the presence of non-fixed array of [cs_char] type.
             */
            switch(inst_p->fe_info->node_kind)
            {
            case fe_parameter_n_k:
                PROP_set_nf_cs_char_param((AST_parameter_n_t *)inst_p);
                break;

            case fe_field_n_k:
                FE_SET(parent_p->fe_info->flags, FE_HAS_NF_CS_ARRAY);
                break;
            default:
		/* do nothing */
                break;
            }
        }
        else if (inst_p->field_attrs != NULL)
        {
            /*
             * A conformant and/or varying array of other than [cs_char] type.
             * Set flags on any fields/params referenced as field attributes.
             */
            PROP_set_used_as_reg_fld_attr(inst_p);
        }

        switch(instance_p->fe_info->node_kind)
        {
            case fe_parameter_n_k:
            {
                AST_parameter_n_t *param_p = (AST_parameter_n_t *)instance_p;
                if (FE_TEST(type_p->fe_info->flags, FE_HAS_V1_ATTR)
                    ||  AST_STRING0_SET(param_p)    /* [v1_string]  */
                    ||  (AST_SMALL_SET(param_p) &&  /* [v1_array]   */
                         (type_p->kind != AST_array_k ||
                          AST_CONFORMANT_SET(type_p) || AST_VARYING_SET(param_p))))
                    FE_SET(param_p->fe_info->flags, FE_HAS_V1_ATTR);
                if (FE_TEST(type_p->fe_info->flags, FE_HAS_V2_ATTR)
                    ||  AST_STRING_SET(param_p)
                    ||  AST_PTR_SET(param_p) || AST_UNIQUE_SET(param_p)
                    ||  (type_p->kind == AST_array_k &&
                        (AST_CONFORMANT_SET(type_p) || AST_VARYING_SET(param_p))
                        && !AST_SMALL_SET(type_p) && !AST_SMALL_SET(param_p)))
                    FE_SET(param_p->fe_info->flags, FE_HAS_V2_ATTR);
                break;
            }
            case fe_field_n_k:
            {
                AST_field_n_t *field_p = (AST_field_n_t *)instance_p;
                if (FE_TEST(type_p->fe_info->flags, FE_HAS_V1_ATTR)
                    ||  AST_STRING0_SET(field_p)    /* [v1_string]  */
                    ||  (AST_SMALL_SET(field_p) &&  /* [v1_array]   */
                         (type_p->kind != AST_array_k ||
                          AST_CONFORMANT_SET(type_p) || AST_VARYING_SET(field_p))))
                {
                    FE_SET(field_p->fe_info->flags, FE_HAS_V1_ATTR);
                    FE_SET(parent_p->fe_info->flags, FE_HAS_V1_ATTR);
                }
                if (FE_TEST(type_p->fe_info->flags, FE_HAS_V2_ATTR)
                    ||  AST_STRING_SET(field_p)
                    ||  AST_PTR_SET(field_p) || AST_UNIQUE_SET(field_p)
                    ||  (type_p->kind == AST_array_k &&
                        (AST_CONFORMANT_SET(type_p) || AST_VARYING_SET(field_p))
                        && !AST_SMALL_SET(type_p) && !AST_SMALL_SET(field_p)))
                {
                    FE_SET(field_p->fe_info->flags, FE_HAS_V2_ATTR);
                    FE_SET(parent_p->fe_info->flags, FE_HAS_V2_ATTR);
                }
                break;
            }
            case fe_arm_n_k:
            {
                AST_arm_n_t *arm_p = (AST_arm_n_t *)instance_p;
                if (FE_TEST(type_p->fe_info->flags, FE_HAS_V1_ATTR)
                    ||  AST_STRING0_SET(arm_p)      /* [v1_string]  */
                    ||  (AST_SMALL_SET(arm_p) &&    /* [v1_array]   */
                         (type_p->kind != AST_array_k ||
                          AST_CONFORMANT_SET(type_p) || AST_VARYING_SET(arm_p))))
                {
                    FE_SET(arm_p->fe_info->flags, FE_HAS_V1_ATTR);
                    FE_SET(parent_p->fe_info->flags, FE_HAS_V1_ATTR);
                }
                if (FE_TEST(type_p->fe_info->flags, FE_HAS_V2_ATTR)
                    ||  AST_STRING_SET(arm_p)
                    ||  AST_PTR_SET(arm_p) || AST_UNIQUE_SET(arm_p)
                    ||  (type_p->kind == AST_array_k &&
                        (AST_CONFORMANT_SET(type_p) || AST_VARYING_SET(arm_p))
                        && !AST_SMALL_SET(type_p) && !AST_SMALL_SET(arm_p)))
                {
                    FE_SET(arm_p->fe_info->flags, FE_HAS_V2_ATTR);
                    FE_SET(parent_p->fe_info->flags, FE_HAS_V2_ATTR);
                }
                break;
            }
            default:
                break;
        }
    }

    /*
     * Save the context-sensitive flags and update the ctx to reflect
     * the new state.  This is necessary for propagating any contained
     * types.
     */
    toplevel_param = ctx->toplevel_param;
    toplevel_ref_param = ctx->toplevel_ref_param;
    under_ptr = ctx->under_ptr;
    ctx->toplevel_ref_param = false;
    ctx->toplevel_param = false;

    /* if the type has already been visited then nothing to do. */
    if (ctx->typedef_dcl)
    {
        if (FE_TEST(type_p->fe_info->flags,FE_PROP_TYPE_DONE))
        {
            /*
             * Even though the type has been visited its has_v1_attr and
             * has_v2_attr flags should be bubbled up to parent types.
             */
            if (FE_TEST(type_p->fe_info->flags, FE_HAS_V1_ATTR))
                ctx->has_v1_attr = true;
            if (FE_TEST(type_p->fe_info->flags, FE_HAS_V2_ATTR))
                ctx->has_v2_attr = true;
        }
        else FE_SET(type_p->fe_info->flags,FE_PROP_TYPE_DONE);
    }
    /* if the param has already been visited then nothing to do. */
    else
    {
        /*
         *  If it is not the case that  the param is [in], and we haven't done
         *  [in] param propagation or the param is [out], and we haven't done
         *  [out] param propagation then we have completed param propagation
         *  and we can just update the context for v1/v2 attributes and return.
         */
        if (!toplevel_param && !( (AST_IN_SET(ctx->param_p) &&
                !FE_TEST(type_p->fe_info->flags,FE_PROP_IN_PARAM_DONE)) ||
               (AST_OUT_SET(ctx->param_p) &&
                !FE_TEST(type_p->fe_info->flags,FE_PROP_OUT_PARAM_DONE))))
        {
            /*
             * Even though the type has been visited its has_v1_attr and
             * has_v2_attr flags should be bubbled up to parent types.
             */
            if (FE_TEST(type_p->fe_info->flags, FE_HAS_V1_ATTR))
                ctx->has_v1_attr = true;
            if (FE_TEST(type_p->fe_info->flags, FE_HAS_V2_ATTR))
                ctx->has_v2_attr = true;
        }

       if (AST_IN_SET(ctx->param_p)) FE_SET(type_p->fe_info->flags,FE_PROP_IN_PARAM_DONE);
       if (AST_OUT_SET(ctx->param_p)) FE_SET(type_p->fe_info->flags,FE_PROP_OUT_PARAM_DONE);
    }

    /*
     *  If the type is a non-anonymous type and either self-pointing type or
     *  ool, set the in_aux flags such that we know if contained pointers
     *  should be generated into the aux or not.
     */
    in_aux = ctx->in_aux;
    if (((type_p->name != NAMETABLE_NIL_ID) &&
        (((type_p->kind == AST_pointer_k) && AST_SELF_POINTER_SET(type_p)) ||
         (AST_OUT_OF_LINE_SET(type_p)))))
        ctx->in_aux = true;

    /*
     * If the type has an xmit as, don't propagate through the contents because
     * it essentially doesn't exist.  Instead do only the propagation on the
     * xmit as type itself.
     */
    if (type_p->xmit_as_type != NULL)
    {
        /*
         *  Do propagation as if the xmit_as type was specified instead, for
         *  alignment info, and rep_as determination.  Pointed-at xmit_as
         *  types are handled below in the pointer processing in the next
         *  clause.
         */
        /* Before recursing, set property to propagate downwards. */
        ctx->in_xmitted = TRUE;

        PROP_type_info(type_p->xmit_as_type, ctx);

        /* After recursing, set property to propagate upwards. */
        ctx->has_xmit_as = TRUE;
    }
    else
    {
        /* Depending upon type set the size fields as appropriate */
        switch (type_p->kind)
        {
        case AST_structure_k:
            PROP_type_struct(type_p, ctx);

            /* After recursing, set property to propagate upwards if applies. */
            if (AST_UNALIGN_SET(type_p))
                ctx->has_v1_struct = TRUE;
            break;

        case AST_array_k:
            PROP_type_info(type_p->type_structure.array->element_type, ctx);

            /* Do size propagation */
            COPY_IF_LARGER(type_p->alignment_size,type_p->type_structure.array->element_type->alignment_size);
            type_p->ndr_size = type_p->type_structure.array->element_type->ndr_size;
            if (!AST_CONFORMANT_SET(type_p))
            { 
                int i;
                for (i = type_p->type_structure.array->index_count - 1; i >= 0; i--)
                {
                    AST_array_index_n_t *index_p;       /* Array index node for a dim */
                    index_p = &type_p->type_structure.array->index_vec[i];
                    type_p->ndr_size *= (
                        index_p->upper_bound->value.int_val - index_p->lower_bound->value.int_val);
                }
            }

            /*
             * Propagation for pa types:  If a toplevel parameter array without
             * [ref] attribute put on the pa types list.
             */
            if (toplevel_param && !toplevel_ref_param)
            {
                PROP_process_pa_type(type_p, &ctx->int_p->pa_types, ctx);
                if (type_p->xmit_as_type != NULL)
                    PROP_process_pa_type(type_p->xmit_as_type, &ctx->int_p->pa_types, ctx);
            }
            break;

        case AST_disc_union_k:
            PROP_type_union(type_p, ctx);

            /* After recursing, set property to propagate upwards. */
            ctx->has_union = TRUE;
            break;

        case AST_pointer_k:
        {
            /*
             * Save the pointer node address in the pointee node's fe_info.
             */
            AST_type_n_t *ptee_type_p;  /* Pointee type */

            ptee_type_p = type_p->type_structure.pointer->pointee_type;

				if (ptee_type_p->kind == AST_interface_k)
					ctx->has_interface = TRUE;
				
            if (ptee_type_p->fe_info->fe_type_id != fe_ptr_info)
            {
                ptee_type_p->fe_info->fe_type_id = fe_ptr_info;
                ptee_type_p->fe_info->type_specific.pointer_type = type_p;
            }

            /*
             *  If this is type without an instance attribute (no field,
             *  parameter, or arm node) then ndetermine if it is ptr by the
             *  looking for the [ref] attribute on the type.
             */
            if ((instance_p == NULL) && !ctx->under_ptr)
                ctx->under_ptr = !AST_REF_SET(type_p);

            PROP_type_info(ptee_type_p, ctx);
            if (ptee_type_p->array_rep_type != NULL
                && ptee_type_p->cs_char_type == NULL)
            {      /* [cs_char] type cannot be arrayified */
                if (!in_aux && ctx->in_aux)
                {
                    ctx->in_aux = false;
                    PROP_type_info(ptee_type_p->array_rep_type,ctx);
                    ctx->in_aux = true;
                }
                else
                    PROP_type_info(ptee_type_p->array_rep_type,ctx);
            }


            /*
             *  Propagation for pa types:  If not a toplevel parameter with the
             *  [ref] attribute it will need a marshalling routine, so put on
             *  the pa types list.
             */
            if (!toplevel_ref_param)
            {
                ctx->parent_type_p = type_p;
                PROP_process_pa_type(ptee_type_p, &ctx->int_p->pa_types, ctx);

                /* Do the xmit as type */
                if (ptee_type_p->xmit_as_type != NULL)
                    PROP_process_pa_type(ptee_type_p->xmit_as_type,
                          &ctx->int_p->pa_types, ctx);

                /*
                 *  If this is a valid arrayified full pointer (the array_rep
                 *  field is not null and not [ref]), put the array_rep type on
                 *  the pa types list.
                 */
                if (ptee_type_p->array_rep_type != NULL
                    && ptee_type_p->cs_char_type == NULL)
                {      /* [cs_char] type cannot be arrayified */

                    PROP_process_pa_type(
                          ptee_type_p->array_rep_type,
                          &ctx->int_p->pa_types,
                          ctx);

                    /* Do the xmit as type */
                    if (ptee_type_p->array_rep_type->xmit_as_type != NULL)
                        PROP_process_pa_type(
                              ptee_type_p->array_rep_type->xmit_as_type,
                              &ctx->int_p->pa_types,
                              ctx);
                }
            }
            break;
        }

        case AST_pipe_k:
            PROP_type_info(type_p->type_structure.pipe->base_type, ctx);
            break;

        default:
            /* Alignment already set for simple types */
            break;
        }
    }

    /* After recursing, set any applicable properties to propagate upwards */
    if (type_p->rep_as_type != NULL)
        ctx->has_rep_as = TRUE;
    if (type_p->xmit_as_type == NULL)
    {
        char const   *type_name;
        AST_type_n_t *base_type_p = type_p;

        while (base_type_p->defined_as != NULL)
            base_type_p = base_type_p->defined_as;
        NAMETABLE_id_to_string(base_type_p->name, &type_name);
        if (strcmp(type_name, "error_status_t") == 0)
            ctx->has_error_status = TRUE;

        if (type_p->kind == AST_character_k)
            ctx->has_char = TRUE;
        else if (type_is_float(type_p))
            ctx->has_float = TRUE;
        else if (type_is_multibyte_integer(type_p))
            ctx->has_int = TRUE;
        else if (type_is_enum(type_p))
            ctx->has_enum = TRUE;
        else if (AST_PTR_SET(type_p))
        {
            /*
             * Type is a full pointer, set "depends on integer" flag since the
             * wire representation of a full pointer is an integer.  Unique
             * pointers do not depend on integer since the Interpreter only
             * evaluates them for NULL vs. non-NULL.
             */
            ctx->has_int = TRUE;
        }
    }

    /*
     * We are now returning up a level in the recursion so reset the
     * any variables that do not propagate upward such as contained in
     * an ool type.  Also "unvisit" the type so that, for example, two
     * fields of the same type within a struct cause the type to be
     * processed both times.  This is necessary since OOL references
     * to types can have separate requirements from other references.
     */
    ctx->in_aux = in_aux;
    ctx->under_ptr = under_ptr;
    type_unvisit(type_p);

    /*
     * Handle properties that propagate only if the type contains, not merely
     * is, a type with the property.
     */
    if (ctx->ct_cs_char
        || (type_p->defined_as != NULL
            && type_p->defined_as->cs_char_type != NULL))
        FE_SET(type_p->fe_info->flags, FE_CT_CS_CHAR);
    if (type_p->cs_char_type != NULL)
        ctx->ct_cs_char = TRUE;

    /*
     * Propagate conformant flag and synthesized type attributes.
     * This is here for the special case of a declarator that
     * contains anonoymous types.  Since the type is anonymous
     * it is not processes during propagation of the exports list,
     * but because it is contained by a named type, propagation
     * doesn't happen when it is used as parameter either.  
     */
    if ((type_p->name == NAMETABLE_NIL_ID) &&
	(type_contains_conformant(type_p)))
	    PROP_set_type_attr(type_p,AST_CONFORMANT);


    /*
     * For any properties that propagate upwards and are set in the context
     * block, set the corresponding flag on the type.
     */
    if (ctx->has_rep_as)
        FE_SET(type_p->fe_info->flags, FE_HAS_REP_AS);
    if (ctx->has_xmit_as)
        FE_SET(type_p->fe_info->flags, FE_HAS_XMIT_AS);
    if (ctx->has_char)
        FE_SET(type_p->fe_info->flags, FE_HAS_CHAR);
    if (ctx->has_float)
        FE_SET(type_p->fe_info->flags, FE_HAS_FLOAT);
    if (ctx->has_int)
        FE_SET(type_p->fe_info->flags, FE_HAS_INT);
    if (type_p->kind == AST_structure_k
        && !ctx->has_enum
        && !ctx->has_union
        && !ctx->has_vary_array
        && !ctx->has_xmit_as
        && !ctx->has_rep_as
        && !FE_TEST(type_p->fe_info->flags, FE_HAS_PTR)
        && !ctx->has_error_status
		  && !ctx->has_interface
        && !ctx->has_v1_struct)
        FE_SET(type_p->fe_info->flags, FE_MAYBE_WIRE_ALIGNED);

    /*
     * If we found any containing types with V1- or V2-specific attributes,
     * set the corresponding flag on the type and the type instance, if any.
     */
    if (FE_TEST(type_p->fe_info->flags, FE_HAS_V1_ATTR))
        ctx->has_v1_attr = true;
    if (FE_TEST(type_p->fe_info->flags, FE_HAS_V2_ATTR))
        ctx->has_v2_attr = true;
    if (type_p->kind == AST_pointer_k       /* If type poss. not V1/V2 compat */
        ||  type_p->kind == AST_structure_k
        ||  type_p->kind == AST_array_k
        ||  type_p->kind == AST_enum_k
        ||  type_p->kind == AST_pipe_k)
    {
        if (ctx->has_v1_attr)
            FE_SET(type_p->fe_info->flags, FE_HAS_V1_ATTR);
        if (ctx->has_v2_attr)
            FE_SET(type_p->fe_info->flags, FE_HAS_V2_ATTR);

        if (instance_p != NULL)
        {
            switch(instance_p->fe_info->node_kind)
            {
                case fe_parameter_n_k:
                {
                    AST_parameter_n_t *param_p = (AST_parameter_n_t *)instance_p;
                    if (ctx->has_v1_attr)
                        FE_SET(param_p->fe_info->flags, FE_HAS_V1_ATTR);
                    if (ctx->has_v2_attr)
                        FE_SET(param_p->fe_info->flags, FE_HAS_V2_ATTR);
                    break;
                }
                case fe_field_n_k:
                {
                    AST_field_n_t *field_p = (AST_field_n_t *)instance_p;
                    if (ctx->has_v1_attr)
                        FE_SET(field_p->fe_info->flags, FE_HAS_V1_ATTR);
                    if (ctx->has_v2_attr)
                        FE_SET(field_p->fe_info->flags, FE_HAS_V2_ATTR);
                    break;
                }
                case fe_arm_n_k:
                {
                    AST_arm_n_t *arm_p = (AST_arm_n_t *)instance_p;
                    if (ctx->has_v1_attr)
                        FE_SET(arm_p->fe_info->flags, FE_HAS_V1_ATTR);
                    if (ctx->has_v2_attr)
                        FE_SET(arm_p->fe_info->flags, FE_HAS_V2_ATTR);
                    break;
                }
                default:
                    break;
            }
        }
    }
}

/*
**  t y p e _ p r o c e s s _ i n _ o u t _ a t t r s
**
**  Recursive routine used to chase a type node and any contained types
**  to propagate the IN, OUT, OUT_PA_REF type attributes.
**
**  Implicit Inputs:    visited_list - a list of type nodes that we've
**                      already visited in the recursion stack
*/

static void type_process_in_out_attrs
(
    AST_type_n_t        *type_p,        /* [in] Ptr to AST type node */
    boolean             set_in,         /* [in] TRUE => set [in] */
    boolean             set_out,        /* [in] TRUE => set [out] */
    boolean             set_out_pa_ref, /* [in] TRUE => set [out_pa_ref] */
    boolean             varying         /* [in] TRUE => set varying flags */
)

{
    /*
     * If the type has already been visited, return.
     */
    if (type_visited(type_p))
        return;

    type_visit(type_p);         /* Mark type as visited */

    /* Propagate attributes. */

    if (set_in)
    {
        PROP_set_type_usage_attr(type_p,AST_IN);
        if (type_p->xmit_as_type != NULL)
              PROP_set_type_usage_attr(type_p->xmit_as_type,AST_IN);
    }

    if (set_out)
    {
        PROP_set_type_usage_attr(type_p,AST_OUT);
        if (type_p->xmit_as_type != NULL)
              PROP_set_type_usage_attr(type_p->xmit_as_type,AST_OUT);
    }

    if (set_out_pa_ref)
    {
        PROP_set_type_usage_attr(type_p,AST_OUT_PA_REF);
        if (type_p->xmit_as_type != NULL)
              PROP_set_type_usage_attr(type_p->xmit_as_type,AST_OUT_PA_REF);
    }

    /* Chase any contained types. */

    switch (type_p->kind)
    {
    case AST_pointer_k:
        type_process_in_out_attrs(type_p->type_structure.pointer->pointee_type,
                                  set_in, set_out, set_out_pa_ref,false);
        if (type_p->type_structure.pointer->pointee_type->array_rep_type != NULL)
        {
            type_process_in_out_attrs(
                      type_p->type_structure.pointer->pointee_type->array_rep_type,
                      set_in, set_out, set_out_pa_ref,varying || AST_STRING_SET(type_p));
        }
        break;

    case AST_structure_k:
    {
        AST_structure_n_t   *struct_p;  /* Ptr to structure node */
        AST_field_n_t       *field_p;   /* A field in the structure */

        struct_p = type_p->type_structure.structure;

        /* Chase all fields in the structure. */

        for (field_p = struct_p->fields
            ;   field_p != NULL
            ;   field_p = field_p->next)
        {
            /* Set the in/out varying or fixed flag on the field type */
            if (AST_VARYING_SET(field_p))
            {
                if (set_in) PROP_set_type_usage_attr(field_p->type,AST_IN_VARYING);
                if (set_out) PROP_set_type_usage_attr(field_p->type,AST_OUT_VARYING);
            /* fixed usage is only interesting on arrays and arrayified pointers */
            } else if ((field_p->type->kind == AST_pointer_k) ||
                       (field_p->type->kind == AST_array_k))
            {
                if (set_in) PROP_set_type_usage_attr(field_p->type,AST_IN_FIXED);
                if (set_out) PROP_set_type_usage_attr(field_p->type,AST_OUT_FIXED);
            }


            type_process_in_out_attrs(field_p->type,
                          set_in, set_out, set_out_pa_ref, AST_VARYING_SET(field_p)!=0);
        }
        break;
    }

    case AST_array_k:

        if (varying)
        {
            if (set_in) PROP_set_type_usage_attr(type_p,AST_IN_VARYING);
            if (set_out) PROP_set_type_usage_attr(type_p,AST_OUT_VARYING);
        }
        else {
            if (set_in) PROP_set_type_usage_attr(type_p,AST_IN_FIXED);
            if (set_out) PROP_set_type_usage_attr(type_p,AST_OUT_FIXED);
        }

        /* Chase the array element type. */
        type_process_in_out_attrs(type_p->type_structure.array->element_type,
                                  set_in, set_out, set_out_pa_ref, false);
        break;

    case AST_disc_union_k:
    {
        AST_disc_union_n_t  *union_p;   /* Ptr to discriminated union node */
        AST_arm_n_t         *arm_p;     /* An arm in the union */

        union_p = type_p->type_structure.disc_union;

        /* Chase all arms in the union. */

        for (arm_p = union_p->arms
            ;   arm_p != NULL
            ;   arm_p = arm_p->next)
            if (arm_p->type != NULL)
            {
                /* Set the in/out varying or fixed flag on the field type */
                if (AST_VARYING_SET(arm_p))
                {
                    if (set_in) PROP_set_type_usage_attr(arm_p->type,AST_IN_VARYING);
                    if (set_out) PROP_set_type_usage_attr(arm_p->type,AST_OUT_VARYING);
                /* fixed usage is only interesting on arrays and arrayified pointers */
                } else if ((arm_p->type->kind == AST_pointer_k) ||
                           (arm_p->type->kind == AST_array_k))
                {
                    if (set_in) PROP_set_type_usage_attr(arm_p->type,AST_IN_FIXED);
                    if (set_out) PROP_set_type_usage_attr(arm_p->type,AST_OUT_FIXED);
                }

                type_process_in_out_attrs(arm_p->type,
                      set_in, set_out, set_out_pa_ref, AST_VARYING_SET(arm_p)!=0);
            }
        break;
    }

    case AST_pipe_k:
        /* Chase the pipe base type. */
        type_process_in_out_attrs(type_p->type_structure.pipe->base_type,
                                  set_in, set_out, set_out_pa_ref, false);
        break;

    default:
        break;
    }
}

#if 0
/*
**  t y p e  _ c o n t a i n s _ m u t a b l e
**
**  Recursive routine used to chase a type's node and
**  any types that it points to for a mutable pointer.
**  TRUE is returned at the first mutable pointer found.
**
**  Implicit Inputs:    visited_list - a list of type nodes that we've
**                      already visited in the recursion stack
*/

static boolean type_contains_mutable
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{


    /* If mutable pointers do not apply to this type, return FALSE */
    if (!type_can_contain_pointer(type_p))
    {
        return FALSE;
    }

    /*
     * If the type has already been visited, return FALSE.
     * This avoids user's error containing structures that
     * refer to each other.
     */
    if (type_visited(type_p))
        return FALSE;
    else
        type_visit(type_p);     /* Mark as visited */


    /* Process all valid types, returning at first mutable pointer */

    switch (type_p->kind)
    {
        case AST_pointer_k:
            if AST_MUTABLE_SET(type_p)
            {
                return TRUE;
            }
            else
            {
                /* Check the pointee type */
                if (type_contains_mutable(
                                type_p->type_structure.pointer->pointee_type))
                {
                    return TRUE;
                }
            }

            break;

        case AST_structure_k:
        {
            AST_field_n_t       *field_p;   /* A field in the structure */

            /*
             * If any field has a mutable pointer, return TRUE
             */
            for (field_p = type_p->type_structure.structure->fields;
                 field_p != NULL;
                 field_p = field_p->next)
            {
                if (type_contains_mutable(field_p->type))
                {
                    return TRUE;
                }
            }

            break;
        }

        case AST_array_k:
            /* Chase the array element type. */
            if (type_contains_mutable(
                                type_p->type_structure.array->element_type))
            {
                return TRUE;
            }
            break;

        case AST_disc_union_k:
        {
            AST_arm_n_t         *arm_p;     /* An arm in the union */

            /*
             * Chase all arms in the union.  Again, if any arm has a
             * mutable pointer type, return TRUE.
             */
            for (arm_p = type_p->type_structure.disc_union->arms;
                 arm_p != NULL;
                 arm_p = arm_p->next)
            {
                if (arm_p->type != NULL && type_contains_mutable(arm_p->type))
                {
                    return TRUE;
                }
            }
            break;
        }

        default:
            break;
    }


    return FALSE;
}
#endif

/*
**  t y p e _ c o n t a i n s _ p o i n t e r
**
**  Recursive routine used to chase a type's node and
**  any types that it points to for a pointer.
**  TRUE is returned at the first pointer found.
**
**  Implicit Inputs:    visited_list - a list of type nodes that we've
**                      already visited in the recursion stack
*/

static boolean type_contains_pointer
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{


    /* If pointers do not apply to this type, return FALSE */
    if (!type_can_contain_pointer(type_p))
    {
        return FALSE;
    }

    /*
     * If the type has already been visited, return FALSE.
     * This avoids user's error containing structures that
     * refer to each other.
     */
    if (type_visited(type_p))
        return FALSE;
    else
        type_visit(type_p);     /* Mark as visited */


    /*
     * If the type has the [transmit_as] attr, process its transmissible type.
     */
    if (type_p->xmit_as_type != NULL)
        return type_contains_pointer(type_p->xmit_as_type);

    /* Process all valid types, returning at first pointer */

    switch (type_p->kind)
    {
        case AST_pointer_k:
            return TRUE;
            break;

        case AST_structure_k:
        {
            AST_field_n_t       *field_p;   /* A field in the structure */

            /*
             * If any field has a pointer, return TRUE
             */
            for (field_p = type_p->type_structure.structure->fields;
                 field_p != NULL;
                 field_p = field_p->next)
            {
                if (type_contains_pointer(field_p->type))
                {
                    return TRUE;
                }
            }

            break;
        }

        case AST_array_k:
            /* Chase the array element type. */
            if (type_contains_pointer(
                                type_p->type_structure.array->element_type))
            {
                return TRUE;
            }
            break;

        case AST_disc_union_k:
        {
            AST_arm_n_t         *arm_p;     /* An arm in the union */

            /*
             * Chase all arms in the union.  Again, if any arm has a
             * pointer type, return TRUE.
             */
            for (arm_p = type_p->type_structure.disc_union->arms;
                 arm_p != NULL;
                 arm_p = arm_p->next)
            {
                if (arm_p->type != NULL && type_contains_pointer(arm_p->type))
                {
                    return TRUE;
                }
            }
            break;
        }

        default:
            break;
    }


    return FALSE;
}

/*
**  t y p e _ p r o p _ c o n f o r m a n t
**
**  Sets the conformant flag for a type if it is a structure that contains
**  any conformant types.
**
**  Implicit Inputs:    visited_list - listhead for visited type nodes
*/

static void type_prop_conformant
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    visited_list = NULL;                /* Init the visited list */

    /* Conditionally sets the conformant flag. */
    type_contains_conformant(type_p);

    type_visit_free();                  /* Frees entries on visited list */
}

/*
**  t y p e _ p r o p _ u p _ t o _ p a r a m
**
**  Does propagation from type nodes under a parameter node
**  back up to the parameter node.
**
**  Sets the "has conformant array" flag for a parameter if it contains a
**  non-top-level conformant array that is not under a full pointer.
**
**  Implicit Inputs:    visited_list - listhead for visited type nodes
*/

static void type_prop_up_to_param
(
    AST_parameter_n_t   *param_p        /* [in] Ptr to AST parameter node */
)

{
    AST_type_n_t        *type_p;        /* Parameter data type */
    boolean             non_ref;        /* TRUE if [ref] not set */

    visited_list = NULL;                /* Init the visited list */

    type_p = param_follow_ref_ptr(param_p, CHK_follow_ref_arr_siz);

    /* For toplevel, look at param attributes, otherwise check type */
    if (param_p->type == type_p)
        non_ref = !AST_REF_SET(param_p);
    else
        non_ref = !AST_REF_SET(type_p);

    type_prop_param(param_p, type_p, param_p->field_attrs,
                    AST_STRING_SET(param_p)!=0, non_ref);

    type_visit_free();                  /* Frees entries on visited list */
}

#if 0
/*
**  t y p e _ h a s _ m u t a b l e
**
**  This routine searches down the data structure for any
**  types that contain mutable pointers such that an
**  operation's HAS_IN/OUT_PTRS can be set.
**
**  Implicit Inputs:    visited_list - listhead for visited type nodes
*/

static boolean type_has_mutable
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    boolean has_mutable;

    visited_list = NULL;                /* Init the visited list */

    /* Searches for a mutable pointer */
    has_mutable = type_contains_mutable(type_p);

    type_visit_free();                  /* Frees entries on visited list */

    return has_mutable;
}
#endif

/*
**  t y p e _ h a s _ p o i n t e r
**
**  This routine searches down the data structure for any types that contain
**  pointers such that an operation's HAS_IN/OUT_PTRS can be set.  It
**  does not consider the toplevel pointer in its pointer search as it
**  my be a [ref] parameter pointer which is treated differently than
**  contained [ref] pointers.
**
**  Implicit Inputs:    visited_list - listhead for visited type nodes
*/

static boolean type_has_pointer
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    boolean has_pointer;

    visited_list = NULL;                /* Init the visited list */

    has_pointer = type_contains_pointer(type_p);

    type_visit_free();                  /* Frees entries on visited list */

    return has_pointer;
}

/*
**  t y p e _ p r o p _ p t r _ a t t r s
**
**  Sets the HAS_PTR flag if the type contains any pointers.
**  Sets the HAS_UNIQUE_PTR flag if the type contains any [unique] pointers.
**
**  Implicit Inputs:    visited_list - listhead for visited type nodes
*/

static void type_prop_ptr_attrs
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    boolean             ptr;            /* Type has pointer */
    boolean             ref;            /* Type has ref pointer */
    boolean             unique;         /* Type has unique pointer */
    boolean             full;           /* Type has full pointer */

    visited_list = NULL;                /* Init the visited list */

    prop_pointer_types(type_p, &ptr, &ref, &unique, &full);

    type_visit_free();                  /* Frees entries on visited list */
}


/*
**  t y p e _ p r o p _ i n _ o u t _ a t t r s
**
**  If the IN or OUT attributes are set on the type, propagates these
**  attributes to any contained pointer types.  The in and out booleans
**  are needed because the type_process_in_out_attrs routine now
**  also set in/out fixed/varying and it is sensitive to the whether an instance
**  is in or out.
**
**  Implicit Inputs:    visited_list - listhead for visited type nodes
*/


static void type_prop_in_out_attrs
(
    AST_type_n_t        *type_p,        /* [in] Ptr to AST type node */
    boolean             in,             /* [in] true if in */
    boolean             out,            /* [in] true if out */
    boolean             varying         /* [in] true if varying */
)

{
    if (!in && !out && !AST_OUT_PA_REF_SET(type_p))
        return;

    visited_list = NULL;                /* Init the visited list */

    type_process_in_out_attrs(type_p,
                              in,
                              out,
                              (AST_OUT_PA_REF_SET(type_p) != 0),
                              varying
                              );

    type_visit_free();                  /* Frees entries on visited list */
}

/*
**  t y p e _ h a s _ c o n t e x t
**
**  This routine doesn't propagate the context, but searches down
**  the type to see if it defines a context handle such that an
**  operation's HAS_IN_CTX or HAS_OUT_CTX can be set.
**
**  Implicit Inputs:    visited_list - listhead for visited type nodes
*/

static boolean type_has_context
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    boolean has_context;

    visited_list = NULL;                /* Init the visited list */

    /* Searches for a context handle */
    has_context = type_contains_context(type_p);

    type_visit_free();                  /* Frees entries on visited list */

    return has_context;
}

/*
**  t y p e _ h a s _ o o l
**
**  This routine doesn't propagate, but searches down
**  the type to see if it contains an ool type such that an
**  operation's HAS_IN_OOLS or HAS_OUT_OOLS can be set.
**
**  Implicit Inputs:    visited_list - listhead for visited type nodes
*/

static boolean type_has_ool
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    boolean has_ool;

    visited_list = NULL;                /* Init the visited list */

    /* Searches for a contained [out_of_line] type. */
    has_ool = type_contains_ool(type_p);

    type_visit_free();                  /* Frees entries on visited list */

    return has_ool;
}

/*
**  t y p e _ g e t _ t a g _ n a m e _ a d d r
**
**  Given a type node, returns the address of a tag name field if the
**  type is a structure or union, or if the type is an array whose element
**  type is a structure or union.  Otherwise, returns NULL.
*/

static NAMETABLE_id_t *type_get_tag_name_addr
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    switch (type_p->kind)
    {
    case AST_structure_k:
        return &(type_p->type_structure.structure->tag_name);

    case AST_disc_union_k:
        return &(type_p->type_structure.disc_union->tag_name);

    case AST_array_k:
        return type_get_tag_name_addr
                (type_p->type_structure.array->element_type);
	 default:
		  return NULL;
    }
}

/*
**  t y p e _ p r o p _ t o _ i n s t a n c e
**
*/

static void type_prop_to_instance
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    switch (type_p->kind)
    {
    case AST_structure_k:
    {
        AST_field_n_t   *field_p;       /* A field of the structure */

        for (field_p = type_p->type_structure.structure->fields
             ;  field_p != NULL
             ;  field_p = field_p->next)
        {
            if (AST_STRING_SET(field_p->type) || AST_STRING_SET(field_p))
            {
                AST_SET_STRING(field_p);
                AST_SET_VARYING(field_p);
            }
        }
        return;
    }

    case AST_disc_union_k:
    {
        AST_arm_n_t     *arm_p;         /* An arm of the union */

        for (arm_p = type_p->type_structure.disc_union->arms
             ;  arm_p != NULL
             ;  arm_p = arm_p->next)
            if (arm_p->type != NULL
                &&  (AST_STRING_SET(arm_p) || AST_STRING_SET(arm_p->type)))
            {
                AST_SET_STRING(arm_p);
                AST_SET_VARYING(arm_p);
            }
        return;
    }
    default:
        /* do nothing */
        break;
    }
}

/*
**  t y p e _ p r o p
**
**  Processes a type node for any propagation issues.
**
**  Notes:  Currently, this routine is called only for named types, i.e.
**          those that are exported by either the main interface or an
**          imported interface.  Type propagation for anonymous types in
**          parameters must be handled separately.  Anonymous types
**          elsewhere, e.g. fields of structures, need not be processed,
**          since they can appear only as children of named types and
**          thus will be processed in the recursive type propagation.
*/

static void type_prop
(
    AST_type_n_t        *type_p,        /* [in] Ptr to AST type node */
    AST_interface_n_t   *int_p,         /* [in] Ptr to interface node */
    AST_interface_n_t   *parent_int_p   /* [in] Parent interface node */
)

{
    prop_ctx_t          ctx;           /* context during propagation */

    /*
     * Initialize the propagation context block
     */
    ctx.toplevel_ref_param = false;
    ctx.toplevel_param = false;
    ctx.int_p = int_p;
    ctx.instance_p = NULL;
    ctx.param_p = NULL;
    ctx.parent_type_p = NULL;
    ctx.typedef_dcl = true;
    ctx.has_rep_as = false;
    ctx.ct_cs_char = false;
    ctx.has_xmit_as = false;
    ctx.in_xmitted = false;
    ctx.has_v1_attr = false;
    ctx.has_v2_attr = false;
    ctx.in_aux = false;
    ctx.under_ptr = false;
    ctx.has_enum = false;
    ctx.has_union = false;
    ctx.has_vary_array = false;
    ctx.has_char = false;
    ctx.has_float = false;
    ctx.has_int = false;
    ctx.has_error_status = false;
    ctx.has_v1_struct = false;

    /*
     * Propagate the interface's [in_line] attribute (if any) to the type node
     * if it is a non-scalar type, unless the type node has the [out_of_line]
     * attribute set.
     */
    if (AST_IN_LINE_SET(int_p)
        &&  !AST_OUT_OF_LINE_SET(type_p)
        &&  !type_is_scalar(type_p))
    {
        PROP_set_type_usage_attr(type_p,AST_IN_LINE);
    }


    /*
     * Propagate the interface's [out_of_line] attribute (if any) to the type
     * node if it is a non-scalar type, unless the type node has the [in_line]
     * attribute set.
     */
    if (AST_OUT_OF_LINE_SET(int_p)
        &&  !AST_IN_LINE_SET(type_p)
        &&  !type_is_scalar(type_p)
        &&  type_p->xmit_as_type == NULL
        &&  type_p->kind != AST_pointer_k)
    {
        PROP_set_type_usage_attr(type_p,AST_OUT_OF_LINE);
    }


    /*
     * If the type's [out_of_line] attribute is set, add the type node
     * to the list of out_of_line types off the interface node.
     */
    if (parent_int_p == NULL    /* main interface */
        &&  AST_OUT_OF_LINE_SET(type_p)
        &&  !FE_TEST(type_p->fe_info->flags,FE_OOL))
    {
        AST_type_p_n_t  *typep_p;       /* Used to link type nodes */

        typep_p = AST_type_ptr_node();
        typep_p->type = type_p;

        FE_SET(type_p->fe_info->flags,FE_OOL);
        int_p->ool_types = (AST_type_p_n_t *)AST_concat_element(
                                (ASTP_node_t *)int_p->ool_types,
                                (ASTP_node_t *)typep_p);

        /* If pointer is arrayified, put arrayified type on ool list also */
        if ((type_p->kind == AST_pointer_k) &&
            (type_p->type_structure.pointer->pointee_type->array_rep_type != NULL) &&
             !FE_TEST(type_p->type_structure.pointer->pointee_type->array_rep_type->fe_info->flags,FE_OOL))
        {
            typep_p = AST_type_ptr_node();
            typep_p->type = type_p;

            FE_SET(type_p->type_structure.pointer->pointee_type->array_rep_type->fe_info->flags,FE_OOL);
            int_p->ool_types = (AST_type_p_n_t *)AST_concat_element(
                                    (ASTP_node_t *)int_p->ool_types,
                                    (ASTP_node_t *)type_p->type_structure.pointer->pointee_type->array_rep_type);
        }
    }

    /*
     * Do any type-specific propagation from type to instance.  Note that
     * this should only be required for named types.  For example, when
     * '[string] char *foo' (which has an anonymous type) is parsed, the
     * attribute will be put on the instance, not the anonymous type.
     */
    type_prop_to_instance(type_p);

    /*
     * To this point, all propagation applies to named types only.
     * Propagation below this point also needs to be done for
     * anonymous types in parameters.
     */

    /*
     * Propagate conformant flag and synthesized type attributes.
     */
    type_prop_conformant(type_p);
    type_prop_ptr_attrs(type_p);

    /*
     * If the type is an array whose element type is an anonymous
     * structure or union, then construct a tag name if necessary.
     * A tag name is needed by the spellers, since ANSI-C does not recognize
     * two identical structs without tags as being equivalent.
     */
    if (type_p->kind == AST_array_k
        &&  type_is_anonymous(type_p->type_structure.array->element_type))
    {
        NAMETABLE_id_t  *tag_name_p;    /* Ptr to tag name field to fill in */

        tag_name_p = type_get_tag_name_addr
                        (type_p->type_structure.array->element_type);

        if (tag_name_p != NULL && *tag_name_p == NAMETABLE_NIL_ID)
            *tag_name_p = AST_generate_name(int_p,"_tag");
    }

    /*
     * Propagate the size (ndr and alignment) based on any contained types.
     * The ndr_size for a array/struct/union is the fixed size of the item. 
     * The alignment_size is the max of any contained type or the current
     * value alignment_size (which may have been set via the align attribute).
     *
     * Note: This is only done for main-interface types, and those used
     *    in the main interface (reference from an import).
     */
    if (parent_int_p == NULL && !AST_LOCAL_SET(int_p))
    {
        visited_list = NULL;            /* Init the visited list */
        PROP_type_info(type_p, &ctx);
        type_visit_free();              /* Frees entries on visited list */
    }
}

/*
**  p a r a m _ p r o p
**
**  Processes a parameter node for any propagation issues.
*/

static void param_prop
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_operation_n_t   *op_p,          /* [in] Ptr to operation node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_type_n_t        *top_type_p;    /* Top-level parameter type */
    AST_type_n_t        *type_p;        /* Param type (deref'd if necess.) */
    prop_ctx_t          ctx;            /* propagation ctx block */
    boolean             has_in_ctx = false;     /* Local for determining has_in_out_ctx */

    /*
     * If the parameter type has a top-level '*' which indicates passing
     * mechanism only, follow the pointer to the data of interest.
     *      top_type_p  = top-level parameter type
     *      type_p      = possibly dereferenced parameter type
     */
    top_type_p = param_p->type;
    type_p = param_follow_ref_ptr(param_p, CHK_follow_ref_arr_siz);

    /*
     * Set the [hidden] parameter attribute if:
     *  o Any of the [cs_*tag] attributes are set and the operation has
     *    the [cs_tag_rtn] attribute.
     */
    if (param_p->uplink->cs_tag_rtn_name != NAMETABLE_NIL_ID
        && (AST_CS_STAG_SET(param_p) || AST_CS_DRTAG_SET(param_p)
            || AST_CS_RTAG_SET(param_p)))
        AST_SET_HIDDEN(param_p);

    /*
     * Set special flag on [out]-only ref pointers as they need the
     * typename_us routine generated on the server to do the allocation
     * of the surrogate.
     */
     if ((top_type_p->kind == AST_pointer_k) &&
         AST_REF_SET(param_p) &&
         AST_OUT_SET(param_p) && !AST_IN_SET(param_p))
              PROP_set_type_usage_attr(type_p,AST_OUT_PA_REF);


    /*
     * Initialize the propagation context block
     */
    ctx.toplevel_param = true;
    ctx.toplevel_ref_param = (AST_REF_SET(param_p) != 0);
    ctx.int_p = int_p;
    ctx.instance_p = (ASTP_node_t *)param_p;
    ctx.param_p = param_p;
    ctx.parent_type_p = NULL;
    ctx.typedef_dcl = false;
    ctx.has_rep_as = false;
    ctx.ct_cs_char = false;
    ctx.has_xmit_as = false;
    ctx.in_xmitted = false;
    ctx.has_v1_attr = false;
    ctx.has_v2_attr = false;
    ctx.in_aux = false;
    ctx.under_ptr = false;
    ctx.has_enum = false;
    ctx.has_union = false;
    ctx.has_vary_array = false;
    ctx.has_char = false;
    ctx.has_float = false;
    ctx.has_int = false;
    ctx.has_error_status = false;
    ctx.has_v1_struct = false;

    /*
     * Do type propagation on the type of the parameter, in case it hasn't
     * been previously done.
     */
    if (!AST_LOCAL_SET(int_p))
    {
        visited_list = NULL;            /* Init the visited list */
        PROP_type_info(top_type_p, &ctx);
        type_visit_free();              /* Frees entries on visited list */
    }

    /*
     * If param is float passed by value, set operation FE_HAS_VAL_FLOAT flag.
     */
    if (top_type_p->kind == AST_short_float_k
        &&  param_p != op_p->result)    /* Not the result param */
        FE_SET(op_p->fe_info->flags, FE_HAS_VAL_FLOAT);

    /*
     * Propagate the [string] and [heap] attributes from type to parameter.
     * Propagate the [context_rd] type attr to [context] parameter attr.
     */
    if (AST_HEAP_SET(type_p))
        AST_SET_HEAP(param_p);
    if (AST_STRING_SET(top_type_p) || AST_STRING_SET(param_p))
    {
        AST_SET_STRING(param_p);
        AST_SET_VARYING(param_p);
    }
    if (AST_CONTEXT_RD_SET(top_type_p)
        ||  AST_CONTEXT_RD_SET(type_p))
        AST_SET_CONTEXT(param_p);

    /*
     * If an [in] parameter, propagate the [in] attribute to the parameter's
     * type node.  Also, set the HAS_IN_CTX operation flag if any IN parameter
     * contains a context handle.
     */
    if (AST_IN_SET(param_p))
    {
        PROP_set_type_usage_attr(type_p,AST_IN);
        if (type_p->xmit_as_type != NULL)
              PROP_set_type_usage_attr(type_p->xmit_as_type,AST_IN);

        /*
         * If parameter has context attribute, or it's defined in its type,
         * then set the operation's AST_HAS_IN_CTX flag.
         */
        if (AST_CONTEXT_SET(param_p) ||
            type_has_context(top_type_p))
        {
            AST_SET_HAS_IN_CTX(op_p);
            has_in_ctx = true;
        }

        /*
         *  If type has ool set then set the operation's AST_HAS_IN_OOLS flag.
         */
        if (type_has_ool(top_type_p))
        {
            AST_SET_HAS_IN_OOLS(op_p);
        }

        /*
         * If a parameter type does not have the [ref] attribute (it is
         * a real pointer) or contains pointers, set the operation's
         * AST_HAS_IN_PTRS flag.  Note that a special case check for [string]
         * is needed because the param_follow_ref_ptr call above does not follow
         * a [ref,string] pointer.
         */
        if (AST_PTR_SET(param_p)
            ||  AST_UNIQUE_SET(param_p)
            ||  (!AST_STRING_SET(param_p)
                 && !AST_CONTEXT_RD_SET(type_p) && type_has_pointer(type_p)))
            AST_SET_HAS_IN_PTRS(op_p);

        /*
         * Propagate [used_varying] or [used_fixed] to type of param.
         */
        if (AST_VARYING_SET(param_p))
            PROP_set_type_usage_attr(type_p,AST_IN_VARYING);
        else if ((type_p->kind == AST_pointer_k) ||
                 (type_p->kind == AST_array_k))
            PROP_set_type_usage_attr(type_p,AST_IN_FIXED);
    }

    /*
     * If an [out] parameter, propagate the [out] attribute to the parameter's
     * type node.  Also, set the HAS_OUT_CTX operation flag if any OUT parameter
     * contains a context handle.
     */
    if (AST_OUT_SET(param_p))
    {
        PROP_set_type_usage_attr(type_p,AST_OUT);
        if (type_p->xmit_as_type != NULL)
              PROP_set_type_usage_attr(type_p->xmit_as_type,AST_OUT);

        /*
         * If parameter has context attribute, or it's defined in its type,
         * then set the operation's AST_HAS_OUT_CTX flag.
         */
        if (AST_CONTEXT_SET(param_p) ||
            type_has_context(top_type_p))
        {
            AST_SET_HAS_OUT_CTX(op_p);
            if (has_in_ctx) AST_SET_HAS_IN_OUT_CTX(op_p);
        }

        /*
         *  If type has ool set then set the operation's AST_HAS_OUT_OOLS flag.
         */
        if (type_has_ool(top_type_p))
        {
            AST_SET_HAS_OUT_OOLS(op_p);
        }

        /*
         * If a parameter type does not have the [ref] attribute (it is
         * a real pointer) or contains pointers, set the operation's
         * AST_HAS_OUT_PTRS flag.  Note that a special case check for [string]
         * is needed because the param_follow_ref_ptr call above does not follow
         * a [ref,string] pointer.

         */
        if (AST_PTR_SET(param_p)
            ||  AST_UNIQUE_SET(param_p)
            ||  (!AST_STRING_SET(param_p)
                 && !AST_CONTEXT_RD_SET(type_p) && type_has_pointer(type_p)))
            AST_SET_HAS_OUT_PTRS(op_p);

        /*
         * Propagate [used_varying] or [used_fixed] to type of param.
         */
        if (AST_VARYING_SET(param_p))
            PROP_set_type_usage_attr(type_p,AST_OUT_VARYING);
        else if ((type_p->kind == AST_pointer_k) ||
                 (type_p->kind == AST_array_k))
            PROP_set_type_usage_attr(type_p,AST_OUT_FIXED);
    }

    /*
     * Do any propagation of attributes from type nodes under the
     * parameter node back up to the parameter node.
     */
    type_prop_up_to_param(param_p);

    /*
     * Recursively propagate the [in] and [out] attrs to any contained types.
     */
    type_prop_in_out_attrs(type_p,
          AST_IN_SET(param_p)!=0,
          AST_OUT_SET(param_p)!=0,
          AST_VARYING_SET(param_p)!=0);

    /*
     * If we found a [rep_as] type, then force memory management to be setup
     * on the server, because the rep_as type may be a pointer.
     */
    if (ctx.has_rep_as)
        AST_SET_ENABLE_ALLOCATE(op_p);
    if (ctx.has_xmit_as)
        AST_SET_HAS_XMIT_AS(op_p);

    /*
     * If the parameter type is an anonymous type, do any type propagation
     * that can apply to anonymous types.
     */
    if (type_is_anonymous(top_type_p))
    {
        type_prop_conformant(top_type_p);
        type_prop_ptr_attrs(top_type_p);
    }

    /*
     *  If a parameter type has the [ptr] attribute or contains a [ptr]
     *  pointers, set the operation's AST_HAS_FULL_PTRS flag to cause the node
     *  table to be initialized.
     */
    if (AST_PTR_SET(param_p) || FE_TEST(param_p->type->fe_info->flags, FE_HAS_FULL_PTR))
    {
        AST_SET_HAS_FULL_PTRS(op_p);
        if (AST_IN_SET(param_p))
            FE_SET(op_p->fe_info->flags, FE_HAS_IN_FULL_PTR);
    }

    /*
     * If the parameter type is an anonymous structure or union, or an array of
     * anonymous structures or unions, then construct a tag name if necessary.
     * A tag name is needed by the spellers, since ANSI-C does not recognize
     * two identical structs without tags as being equivalent.
     */
    if (type_is_anonymous(top_type_p))
    {
        NAMETABLE_id_t  *tag_name_p;    /* Ptr to tag name field to fill in */

        tag_name_p = type_get_tag_name_addr(top_type_p);

        if (tag_name_p != NULL && *tag_name_p == NAMETABLE_NIL_ID)
            *tag_name_p = AST_generate_name(int_p,"_tag");
    }
}

/*
**  o p _ a d d _ b i n d i n g _ h a n d l e _ p a r a m
**
**  Inserts a binding handle parameter before the existing first parameter
**  in an operation.
*/

static void op_add_binding_handle_param
(
    AST_operation_n_t   *op_p           /* [in] Ptr to AST operation node */
)

{
    NAMETABLE_id_t      new_param_id;   /* Nametable id of new parameter name */
    AST_parameter_n_t   *new_param_p;   /* Ptr to new parameter node */

    /* Have to create an '[in] handle_t IDL_handle' parameter. */

    new_param_id = NAMETABLE_add_id("IDL_handle");
    new_param_p = AST_parameter_node(new_param_id); /* Get inited param node */

    new_param_p->type = ASTP_handle_ptr;
    new_param_p->uplink = op_p;
    AST_SET_IN(new_param_p);

    new_param_p->next = op_p->parameters;
    if (new_param_p->next != NULL)
    {
        new_param_p->last = new_param_p->next->last;
        new_param_p->next->last = NULL;
    }

    op_p->parameters = new_param_p;
}


/*
**  P R O P _  a u t o _ h e a p
**
**  Processes an operation parameters, verifying that the total stack
**  requirements for surrogates are a reasonable size.  Due to threading, the
**  stack available to the server stub may be relatively small.  This code
**  loops over the parameters, placing the surrogates in heap, until the total
**  estimated stack size is less than AUTO_HEAP_STACK_THRESHOLD.
*/

static void PROP_auto_heap
(
    AST_operation_n_t   *op_p          /* [in] Ptr to AST operation node */
)

{
    AST_parameter_n_t   *param_p;       /* A parameter in the operation */

    int stack_size;                     /* An estimate of size requirements */
                                        /* for stack surrogates             */
                                        /* representing operation           */
                                        /* parameters                       */

    int auto_heap_size;                 /* Current threshold for moving any */
                                        /* particular object to heap from   */
                                        /* the stack                        */

    /*
     *  Loop until the stack usage threshold is met, or we've put most
     *  everything on the heap already.
     */    
    auto_heap_size = AUTO_HEAP_STACK_THRESHOLD;
    do {
        stack_size = 0;

        /* Process each parameter in the operation. */
        for (param_p = op_p->parameters ; param_p != NULL ; param_p = param_p->next)
        {
            AST_type_n_t *surrogate_type = param_p->type;
            if (surrogate_type->kind == AST_pointer_k && !AST_PTR_SET(surrogate_type))
                surrogate_type = surrogate_type->type_structure.pointer->pointee_type;
            if (surrogate_type->ndr_size >= auto_heap_size) 
                AST_SET_HEAP(param_p);
            if (!AST_HEAP_SET(param_p)) 
                stack_size += surrogate_type->ndr_size;
        }

        /* Process the operation result, if present. */
        if (op_p->result != NULL) 
        {
            AST_type_n_t *surrogate_type = op_p->result->type;
            if (surrogate_type->kind == AST_pointer_k && !AST_PTR_SET(surrogate_type))
                surrogate_type = surrogate_type->type_structure.pointer->pointee_type;
            if (surrogate_type->ndr_size >= auto_heap_size) 
                AST_SET_HEAP(op_p->result);
            if (!AST_HEAP_SET(op_p->result)) 
                stack_size += surrogate_type->ndr_size;
        }

        /* 
         *  Half the size at which we move items to heap for the next pass
         *  through the loop.
         */
        auto_heap_size = auto_heap_size >> 2;
    } while ((stack_size > AUTO_HEAP_STACK_THRESHOLD) && (auto_heap_size > 4));
}



/*
**  o p e r a t i o n _ p r o p
**
**  Processes an operation node for any propagation issues.
*/

static void operation_prop
(
    AST_operation_n_t   *op_p,          /* [in] Ptr to AST operation node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_parameter_n_t   *param_p;       /* A parameter in the operation */

    /*
     * Propagate the interface's [code] attribute (if any) to the operation
     * node, unless the operation node has the [nocode] attribute set.
     */
    if (AST_CODE_SET(int_p)
        &&  !AST_NO_CODE_SET(op_p))
        AST_SET_CODE(op_p);

    /*
     * Propagate the interface's [nocode] attribute (if any) to the oper-
     * ation node, unless the operation node has the [code] attribute set.
     */
    if (AST_NO_CODE_SET(int_p)
        &&  !AST_CODE_SET(op_p))
        AST_SET_NO_CODE(op_p);

    /*
     * Propagate the interface's [encode] and [decode] attributes (if any)
     * to the operation node.  Also do upward propagation to interface node.
     */
    if (AST_ENCODE_SET(int_p))
        AST_SET_ENCODE(op_p);
    if (AST_DECODE_SET(int_p))
        AST_SET_DECODE(op_p);
    if ((AST_ENCODE_SET(op_p) || AST_DECODE_SET(op_p))
        && !AST_NO_CODE_SET(op_p))
        AST_SET_HAS_ENCODE_OPS(int_p);

    /*
     * Propagate the interface's [explicit_handle] attribute (if any) to the
     * operation node under certain conditions.
     */
    if (AST_EXPLICIT_HANDLE_SET(int_p)
        &&  (op_p->parameters == NULL
            ||  !type_is_handle(op_p->parameters->type)))
        AST_SET_EXPLICIT_HANDLE(op_p);

    /*
     * If the operation has the [explicit_handle] attribute but already has an
     * IDL-defined binding handle, silently remove the [explicit_handle] attr.
     */
    if (AST_EXPLICIT_HANDLE_SET(op_p)
        &&  op_p->parameters != NULL
        &&  param_is_handle(op_p->parameters))
        AST_CLR_EXPLICIT_HANDLE(op_p);

    /*
     * If the operation still has the [explicit_handle] attribute, add a new
     * parameter of type handle_t before the existing first parameter.
     */
    if (AST_EXPLICIT_HANDLE_SET(op_p))
        op_add_binding_handle_param(op_p);

    /*
     * Propagate the interface's [nocancel] attribute (if any) to the oper-
     * ation node.
     */
    if (AST_NO_CANCEL_SET(int_p))
        AST_SET_NO_CANCEL(op_p);

    /*
     * Propagate the interface's [cs_tag_rtn] attribute (if any) to the
     * operation node if it has no explicit [cs_tag_rtn] attribute and
     * has any parameters with a [cs_*tag] attribute.
     */
    if (int_p->cs_tag_rtn_name != NAMETABLE_NIL_ID
        && op_p->cs_tag_rtn_name == NAMETABLE_NIL_ID)
    {
        /* Don't have to check fn result which can't have [cs_*tag] attrs */
        for (param_p = op_p->parameters;
             param_p != NULL ;
             param_p = param_p->next)
        {
            if (AST_CS_STAG_SET(param_p) || AST_CS_DRTAG_SET(param_p)
                || AST_CS_RTAG_SET(param_p))
            {
                op_p->cs_tag_rtn_name = int_p->cs_tag_rtn_name;
                break;
            }
        }
    }

    /*
     * If the operation has a [cs_tag_rtn], add the name, if it hasn't been
     * added already, to the list of [cs_tag_rtn] names off the interface node.
     */
    if (op_p->cs_tag_rtn_name != NAMETABLE_NIL_ID)
    {
        if (NAMETABLE_add_binding(op_p->cs_tag_rtn_name, (char *)int_p))
        {
            AST_name_n_t *name_p;
            name_p = AST_name_node(op_p->cs_tag_rtn_name);
            int_p->cs_tag_rtns = (AST_name_n_t *)AST_concat_element(
                (ASTP_node_t *)int_p->cs_tag_rtns, (ASTP_node_t *)name_p);
        }
        /*** {TBS} else clause with error if we failed to add the name but the
            cs_tag_rtns list is empty, implying the global name already existed
            in a different context ***/
    }

    /* Process each parameter in the operation. */

    for (param_p = op_p->parameters ; param_p != NULL ; param_p = param_p->next)
        param_prop(param_p, op_p, int_p);


    /* Process the operation result, if any. */

    if (op_p->result != NULL)
        param_prop(op_p->result, op_p, int_p);

    /* Make sure surrogates are likely to fit on the stack */

    PROP_auto_heap(op_p);
}

/*
**  e x p o r t _ p r o p
**
**  Processes an export node for any propagation issues.
*/

static void export_prop
(
    AST_export_n_t      *export_p,      /* [in] Ptr to AST export node */
    AST_interface_n_t   *int_p,         /* [in] Ptr to interface node */
    AST_interface_n_t   *parent_int_p   /* [in] Parent interface node */
)

{
    switch (export_p->kind)
    {
    case AST_cpp_quote_k:
    case AST_constant_k:
        break;      /* No propagation needed for constants */

    case AST_operation_k:
        if (parent_int_p == NULL)   /* main interface */
            operation_prop(export_p->thing_p.exported_operation, int_p);
        break;

    case AST_type_k:
        type_prop(export_p->thing_p.exported_type, int_p, parent_int_p);
        break;

    default:
        error(NIDL_INTERNAL_ERROR, __FILE__, __LINE__);
    }
}

/*
**  i n t e r f a c e _ p r o p
**
**  Propagate attributes throughout the interface.
*/

static void interface_prop
(
    AST_interface_n_t   *int_p,         /* [in] Ptr to interface node */
    AST_interface_n_t   *parent_int_p   /* [in] Parent interface node */
)

{
    AST_export_n_t      *export_p;      /* Ptr to export node */
    AST_import_n_t      *import_p;      /* Ptr to import node */

    /* Some pairs of interface attributes need default values established. */

    if (parent_int_p == NULL    /* main interface */
        &&  !AST_CODE_SET(int_p)
        &&  !AST_NO_CODE_SET(int_p))
        AST_SET_CODE(int_p);

    if (!AST_IN_LINE_SET(int_p)
        &&  !AST_OUT_OF_LINE_SET(int_p))
    {
#ifdef DUMPERS
        if (cmd_opt[opt_ool])
            /* The -ool command option reverses the default. */
            AST_SET_OUT_OF_LINE(int_p);
        else
#endif
            AST_SET_IN_LINE(int_p);
    }

    /* Process any interfaces that this interface imports. */

    import_p = int_p->imports;
    while (import_p != NULL)
    {
        if (import_p->interface != NULL)
            interface_prop(import_p->interface, int_p);
        import_p = import_p->next;
    }

    /* Process any previous sibiling interfaces */
    
    if (int_p->prev)
    {
        interface_prop(int_p->prev, parent_int_p);
    }

    /*
     * The contents of the up_types list is determined during propagation, so
     * clear the list before processing the interface.  After propagation we
     * can retrieve the contents of the list.
     */
    PROP_up_types_list = NULL;

    /* Process each item that is exported by the interface. */

    for (export_p = int_p->exports
        ;   export_p != NULL
        ;   export_p = export_p->next)
        export_prop(export_p, int_p, parent_int_p);

    /*
     * We have now done all propagation for this interface, so
     * copy the contents of the PROP_up_types_list into the
     * up_types field of the interface node.
     */
    int_p->up_types = PROP_up_types_list;
}

static AST_type_p_n_t *PROP_remove_type_p(
AST_type_p_n_t **list_root, AST_type_p_n_t *type_p, AST_type_p_n_t *prev_type_p
);
/*
**
**  P R O P _ r e m o v e _ t y p e _ p
**  =======================================================
**  This routine removes the specified type pointer from
**  the specified types list, and returns the node previous
**  to the one removed.
*/
static AST_type_p_n_t *PROP_remove_type_p
(AST_type_p_n_t **list_root, AST_type_p_n_t *type_p, AST_type_p_n_t *prev_type_p)
{
    /* If removing the head of the list */
    if (*list_root == type_p)
    {
        *list_root = (*list_root)->next;
        FREE(type_p);
        return *list_root;
    }
    /* Remove from within the list */
    else if (prev_type_p != NULL)
    {
        /* Unlink the node out of the list */
        prev_type_p->next = type_p->next;

        /*  If the last field of the list header pointes to */
        /*  the node being deleted, update the list header  */
        if ((*list_root)->last == type_p) (*list_root)->last = prev_type_p;

        /* free node, and continue search from the previous */
        /* type node                                        */
        FREE(type_p)
        return prev_type_p;
    }
    /* Otherwise find the previous type_p */
    else
    {
        AST_type_p_n_t *cp,*pp;
        pp = NULL;
        /* Find the node */
        for (cp = *list_root; ((cp != NULL) && (cp != type_p)); cp = cp->next) pp = cp;
        /* If we didn't find the previous, error */
        if ((cp != type_p) || (pp == NULL)) error(NIDL_INTERNAL_ERROR, __FILE__, __LINE__);
        /* Call ourself to remove the node */
        return PROP_remove_type_p(list_root,type_p,pp);
    }
}


/*
**
**  P R O P _ p o s t _ f i l t e r _ t y p e s _ l i s t
**  =======================================================
** This routine passes over the specified list and removes
** duplicate entries caused by tag references to named types.
** The backend requests that either the type node created via
** typedef or the type node created via tag reference be on the
** types list, but not both.  This routine removes the DEF_AS_TAG
** version of the type leaving the type node.  This accomplishes
** two things: first we need the multiple pa type routines for
** each named type even if they are derived from the same type,
** and second the tag nodes may have generated names that are
** less under the users control than if we use the named type for
** the type.
**
** If the filter_xmit_as flag is set, types with the xmit_as attribute
** set on them are removed from the list as well.
**
*/

static void     PROP_post_filter_types_list
(AST_type_p_n_t **list_root, boolean filter_xmit_as ATTRIBUTE_UNUSED)
{
    AST_type_p_n_t    *cp;      /* Current type being processed */
    AST_type_p_n_t    *pcp;     /* pointer to type previous current being compared */
    AST_type_p_n_t    *tp;      /* pointer to type being compared */
    AST_type_p_n_t    *ptp;     /* pointer to type previous to type being compared */

    /*
     * Make one major pass over the list looking at each type
     */
    pcp = (AST_type_p_n_t *)NULL;
    for (cp = *list_root; cp; cp = cp->next)
    {
restart: /* If the current node is removed from the list, recheck new current */

        /*
         * Currently, we only need to process struct or union types.
         */
        switch (cp->type->kind)
        {
            /*
             * Check if this struct has an associated entry
             */
            case AST_structure_k:
            {
                AST_structure_n_t  *sp; /* pointer to current structure node */

                /* Save the structure type pointer for the current type */
                sp = cp->type->type_structure.structure;

                /* Loop through the rest of the list to see if there is a match */
                ptp = cp;
                for (tp = cp->next; tp; tp = tp->next)
                {
                    boolean counterparts = false;

                    if (AST_DEF_AS_TAG_SET(tp->type) &&
                        (cp->type->fe_info->tag_ptr == tp->type)) counterparts = true;
                    if (AST_DEF_AS_TAG_SET(cp->type) &&
                        (tp->type->fe_info->tag_ptr == cp->type)) counterparts = true;

                    /* If we find another type node with the same structure */
                    /* node and they are def-as-tag/type counterparts.      */
                    if ((tp->type->kind == AST_structure_k) &&
                        (tp->type->type_structure.structure == sp) &&
                        counterparts)
                    {
                        /* Remove the node with def-as-tag set on it */
                        if (AST_DEF_AS_TAG_SET(tp->type))
                            tp = PROP_remove_type_p(list_root,tp,ptp);
                        else {
                            cp = PROP_remove_type_p(list_root,cp,pcp);
                            pcp = NULL;
                            goto restart;
                        }
                    }

                    /* Update pointer to previous type node */
                    ptp = tp;
                }

                break;
            }

            /*
             * Check if this union has an associated entry
             */
            case AST_disc_union_k:
            {
                AST_disc_union_n_t   *up;       /* pointer to current union node */

                /* Save the union type pointer for the current type */
                up = cp->type->type_structure.disc_union;

                /* Loop through the rest of the list to see if there is a match */
                ptp = cp;
                for (tp = cp->next; tp; tp = tp->next)
                {
                    boolean counterparts = false;

                    if (AST_DEF_AS_TAG_SET(tp->type) &&
                        (cp->type->fe_info->tag_ptr == tp->type)) counterparts = true;
                    if (AST_DEF_AS_TAG_SET(cp->type) &&
                        (tp->type->fe_info->tag_ptr == cp->type)) counterparts = true;

                    /* If we find another type node with the same structure */
                    /* node we need to remove it from the list.             */
                    if ((tp->type->kind == AST_disc_union_k) &&
                        (tp->type->type_structure.disc_union == up) &&
                        counterparts)
                    {
                        /* Remove the node with def-as-tag set on it */
                        if (AST_DEF_AS_TAG_SET(tp->type))
                            tp = PROP_remove_type_p(list_root,tp,ptp);
                        else {
                            cp = PROP_remove_type_p(list_root,cp,pcp);
                            pcp = NULL;
                            goto restart;
                        }
                    }

                    /* Update pointer to previous type node */
                    ptp = tp;
                }

                break;
            }
            default:
                break;
        } /* switch */

        /*
         * Save the current type, if we need to remove any xmit_as_type nodes
         */
         pcp = cp;
    }

}


/*
**  t y p e _ a d d _ t y p e _ t o _ s p _ l i s t
**  -----------------------------------------
**  If the type specified is a pa type in terms of how the backend
**  sees them, and it is not already on the sp list specified, then add a
**  new type pointer node to the list.  The reason for this is that ool
**  and sp types that have routines generated for them in the aux file
**  will need to call the pa routine for any type that they contain.  Thus
**  these types are put on the sp list such that marshalling routines will
**  be generated for them in the aux file.
**
*/

static void type_add_type_to_sp_list
(
    AST_type_n_t        *type_node_ptr, /* [in] Ptr to AST type node */
    AST_type_p_n_t      **types_list,   /* [in,out] Ptr to AST type list */
    AST_type_n_t        *parent_type_ptr ATTRIBUTE_UNUSED    /* [in] Ptr to parent type node */
)

{
    AST_type_p_n_t    *tp;      /* Current type being processed */

    /*
     * If the type has a [transmit_as] clause, then the actual pointed-at
     * type is its transmit_as type.
     */
    if (type_node_ptr->xmit_as_type != NULL)
        type_node_ptr = type_node_ptr->xmit_as_type;


    /*
     * If the pointee is already on sp list then return
     */
    if (FE_TEST(type_node_ptr->fe_info->flags,FE_SELF_POINTING)) return;

    /*
     *  Don't want both the def_as_tag node and the assoicated type node on the
     *  list.  Put only the original on the sp list, but mark the def_as_tag
     *  node as if it were.
     */
     if (AST_DEF_AS_TAG_SET(type_node_ptr) && type_node_ptr->fe_info->original)
     {
        FE_SET(type_node_ptr->fe_info->flags,FE_SELF_POINTING);
        type_node_ptr = type_node_ptr->fe_info->original;
     }

    /*
     * If the pointee is a base type, no need to add it to the sp_list.
     */
    if (type_is_scalar(type_node_ptr) || type_node_ptr == ASTP_void_ptr
        ||  type_node_ptr == ASTP_handle_ptr)
        return;

    /*
     * Make one major pass over the list looking for the target
     * type in the list.  If it is there return
     */
    for (tp = *types_list; tp; tp = tp->next)
        if (tp->type == type_node_ptr) return;

    /*
     *  Create a new type pointer node and link it on the sp_types list of the
     *  interface node.  Set the FE flag indicating that it is on the sp list.
     */
    tp = AST_type_ptr_node();
    tp->type = type_node_ptr;
    FE_SET(type_node_ptr->fe_info->flags,FE_SELF_POINTING);

    /* link it on the sp_types list of the interface node */
    *types_list = (AST_type_p_n_t *)AST_concat_element(
                        (ASTP_node_t *)*types_list,
                        (ASTP_node_t *)tp);
}


/*
**  t y p e _ f i n d _ p a _ t y p e s
**
**  Recursive routine used to chase a type node and any contained types
**  to return a list of contained pa types below this type, but not
**  below any others
**
**  Implicit Inputs:    visited_list - a list of type nodes that we've
**                      already visited in the recursion stack
*/

static void type_find_pa_types
(
    AST_type_n_t        *type_p,         /* [in] Ptr to AST type node */
    AST_type_p_n_t      **types_list    /* [in] Ptr to AST type list */
)

{
    /* if the type has already been visited then nothing to do. */
    if (type_visited(type_p)) return;

    /* Mark type as visited */
    type_visit(type_p);

    /* Depending upon type set the size fields as appropriate */
    switch (type_p->kind)
    {
    case AST_structure_k:
    {
        AST_structure_n_t   *struct_p;  /* Ptr to structure node */
        AST_field_n_t       *field_p;   /* A field in the structure */

        struct_p = type_p->type_structure.structure;

        /* Chase all fields in the structure. */
        for (field_p = struct_p->fields
            ;   field_p != NULL
            ;   field_p = field_p->next)
        {
            type_find_pa_types(field_p->type, types_list);
        }
        break;
    }

    case AST_array_k:
        type_find_pa_types(type_p->type_structure.array->element_type,
                  types_list);
        break;

    case AST_disc_union_k:
    {
        AST_disc_union_n_t  *union_p;   /* Ptr to discriminated union node */
        AST_arm_n_t         *arm_p;     /* An arm in the union */

        union_p = type_p->type_structure.disc_union;

        /* Chase all arms in the union. */
         for (arm_p = union_p->arms
            ;   arm_p != NULL
            ;   arm_p = arm_p->next)
        {
            if (arm_p->type != NULL)
            {
                type_find_pa_types(arm_p->type,types_list);
            }
        }
        break;
    }

    case AST_pointer_k:
        /*
         * If the pointer is not sp, then continue looking for contained
         * pointers and add it to the types_list that we return to caller.
         */
        if (!FE_TEST(type_p->fe_info->flags,FE_SELF_POINTING))
        {
            type_find_pa_types(type_p->type_structure.pointer->pointee_type,
                types_list);

            type_add_type_to_sp_list(type_p->type_structure.pointer->pointee_type,
                  types_list, type_p);

            /* Put the array rep type on the list also */
            if (type_p->type_structure.pointer->pointee_type->array_rep_type != NULL)
            {
                type_add_type_to_sp_list(type_p->type_structure.pointer->
                      pointee_type->array_rep_type,
                      types_list, type_p);

                /*
                 * Since the array rep type now will get an pa routine in the aux file, we
                 * need to make sure that it is generated in its varying form if it
                 * is referenced in a varying context.  That is becuase the pa
                 * routines are generated from the type and the varying attribute is
                 * an instance flag.
                 */
                if (AST_VARYING_SET(type_p->type_structure.pointer->pointee_type->array_rep_type))
                {
                    AST_SET_IN_VARYING(type_p->type_structure.pointer->pointee_type->array_rep_type);
                    AST_SET_OUT_VARYING(type_p->type_structure.pointer->pointee_type->array_rep_type);
                }
            }
        }
        break;

    default:
        /* Do nothing for simple types */
        break;
    }
}

/*
**
**  P R O P _ c o n t a i n e d _ p a _ t o _ l i s t
**  =======================================================
**  This routine passes over the specified types list.  It checks
**  each type for contained non-sp pointed-at types.  If
**  found, the pa type is added to the dest_types list.  This
**  causes the marshalling routine to be emitted into the aux
**  file because it is needed by the sp-type marshalling code.
**
**  Normally, the pa-type marshalling routine is static in the
**  stub, so this will result in two static copies of the same
**  routine (one in the aux and one in the stub).  Without generating
**  a multitude of files, this is how we are eliminating name
**  conflicts.
**
**  Outputs:
**     Additional types added to the specified dest_types list specified
**
**  Implicit Outputs:
**     visited_list - gets reset to null
*/

static void     PROP_contained_pa_to_sp_list
(AST_type_p_n_t **types_list,AST_type_p_n_t **dest_list)
{
    AST_type_p_n_t    *cp;      /* Current type being processed */
    AST_type_p_n_t    *contained_pa_types = NULL;

    /*
     * Make one major pass over the list looking at contained
     * pointers in each type on list.
     */
    for (cp = *types_list; cp; cp = cp->next)
    {
        visited_list = NULL;                /* Init the visited list */
        type_find_pa_types(cp->type,&contained_pa_types);
        type_visit_free();                  /* Frees entries on visited list */
    }

    /* link it on the specified types list */
    *dest_list = (AST_type_p_n_t *)AST_concat_element(
                        (ASTP_node_t *)*dest_list,
                        (ASTP_node_t *)contained_pa_types);

}

/*
**  t y p e s _ l i s t _ p r o p
**
**  Processes a list of data types, generating tag names for any anonymous
**  struct or union types that do not have a tag.
*/

static void types_list_prop
(
    AST_type_p_n_t      *typep_p,       /* [in] Listhead for types list */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_type_n_t        *type_p;        /* Ptr to a type node */
    NAMETABLE_id_t      *tag_name_p;    /* Ptr to tag name field to fill in */


    for ( ; typep_p != NULL ; typep_p = typep_p->next)
    {
        type_p = typep_p->type;

        /*
         * If the type is an anonymous structure or union, or an array of
         * anonymous structs or unions, then construct a tag name if necessary.
         * A tag name is needed by the spellers, since ANSI-C does not
         * recognize two identical structs without tags as being equivalent.
         */
        if (!type_is_anonymous(type_p))
            continue;

        tag_name_p = type_get_tag_name_addr(type_p);

        if (tag_name_p == NULL || *tag_name_p != NAMETABLE_NIL_ID)
            continue;

        *tag_name_p = AST_generate_name(int_p,"_tag");
    }
}

/*
**  P R O P _ m a i n
**
**  Main routine for propagation component of IDL compiler.
**  Propagates attributes and other information throughout the AST.
**  Much of propagation is done by earlier compiler phases - this
**  routine handles any propagation that is easier to do if saved
**  until after the parsing/AST-building phases are complete.
*/

boolean PROP_main               /* Returns TRUE on success */
(
    boolean     *cmd_opt_arr,   /* [in] Array of command option flags */
    void        **cmd_val_arr,  /* [in] Array of command option values */
    AST_interface_n_t *int_p    /* [in] Ptr to AST interface node */
)

{
    /* Save passed command array addresses in static storage. */
    cmd_opt = cmd_opt_arr;
    cmd_val = cmd_val_arr;

    /* Propagate attributes throughout the interface. */

    if (int_p != NULL)
        interface_prop(int_p, NULL);

    /* Update the sp types list to get all contained pa types into aux file */
    PROP_contained_pa_to_sp_list(&int_p->sp_types,&int_p->sp_types);

    /* Update the sp types list to get all ool contained pa types into aux file */
    PROP_contained_pa_to_sp_list(&int_p->ool_types,&int_p->sp_types);

    /* Go over types lists making them acceptable to the backend */
    PROP_post_filter_types_list(&int_p->ool_types, FALSE);
    PROP_post_filter_types_list(&int_p->ra_types, FALSE);
    PROP_post_filter_types_list(&int_p->sp_types, FALSE);
    PROP_post_filter_types_list(&int_p->pa_types, FALSE);
    PROP_post_filter_types_list(&int_p->up_types, FALSE);

    /* Propagate stuff in the pointed-at types list. */

    types_list_prop(int_p->pa_types, int_p);

    /* Return success. */

    return TRUE;
}
