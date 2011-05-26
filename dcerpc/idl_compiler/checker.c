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
**
**  NAME:
**
**      checker.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  IDL Compiler Semantic Checking.
**
**  VERSION: DCE 1.0
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
#include <stdarg.h>

extern char *acf_keyword_lookup(
    int token_value     /* Numeric value of keyword token */
);

extern int  error_count;        /* Count of semantic errors */

/* Local copy of pointers to command line information. */

static boolean *cmd_opt;   /* Array of command option flags */
static void    **cmd_val;  /* Array of command option values */

/* Necessary forward function declarations. */

static void type_check(
    AST_type_n_t        *type_p,        /* [in] Ptr to AST type node */
    ASTP_node_t         *node_p,        /* [in] Parent node of type node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
);

/*
**  s t r u c t u r e   u t i l i t y   r o u t i n e s
**
**  Structure "properties" used by other checks.
*/

/*
**  C H K _ s t r u c t _ i s _ a l l _ b y t e _ f i e l d s
**
**  Returns TRUE if a struct consists only of scalar byte fields.
*/

boolean CHK_struct_is_all_byte_fields
(
    AST_structure_n_t   *struct_p       /* [in] Ptr to AST structure node */
)

{
    AST_field_n_t       *field_p;       /* A field in the structure */

    for (field_p = struct_p->fields ; field_p != NULL ; field_p = field_p->next)
        if (field_p->type->kind != AST_byte_k)
            return FALSE;

    return TRUE;
}

/*
**  t y p e   u t i l i t y   r o u t i n e s
**
**  Type "properties" used by other checks.
*/

/*
**  t y p e _ i s _ s t r i n g
**
**  Returns TRUE if a type can be used as a valid [string] data type:
**  A 1-dim array of char or byte, or a pointer to a char or byte, OR
**  a (1-dim array of) or (pointer to) (struct containing only byte fields).
**  OR, a {1-dim array of | pointer to} unsigned short or unsigned long.
*/

static boolean type_is_string
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    unsigned short      index_count;    /* Number of array dimensions */
    AST_type_n_t        *base_type_p;   /* Base type of array */

    if (type_p->kind == AST_array_k)
    {
        index_count = type_p->type_structure.array->index_count;
        base_type_p = type_p->type_structure.array->element_type;
    }
    else if (type_p->kind == AST_pointer_k)
    {
        index_count = 1;
        base_type_p = type_p->type_structure.pointer->pointee_type;
    }
    else
        return FALSE;

    if (index_count == 1
        &&  (base_type_p->kind == AST_character_k
            ||  base_type_p->kind == AST_byte_k
            ||  base_type_p->kind == AST_short_unsigned_k
            ||  base_type_p->kind == AST_long_unsigned_k
            ||  (base_type_p->kind == AST_structure_k
                &&  CHK_struct_is_all_byte_fields
                        (base_type_p->type_structure.structure))))
        return TRUE;

    return FALSE;
}


/*
**  t y p e _ i s _ v 1 _ s t r i n g
**
**  Returns TRUE if a type can be used as a valid [v1_string] data type:
**  An array of char, in array syntax, with lower bound 0 and fixed upper
**  bound.  If the array is multidimensional, the minor dimension must meet
**  these criteria.
*/

static boolean type_is_v1_string
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    AST_array_n_t       *array_p;       /* Ptr to array node */
    AST_array_index_n_t *index_p;       /* Array index node for minor dim */

    if (type_p->kind != AST_array_k)
        return FALSE;

    array_p = type_p->type_structure.array;

    if (array_p->element_type->kind != AST_character_k)
        return FALSE;

    index_p = &array_p->index_vec[array_p->index_count - 1];

    if (!AST_CONFORMANT_SET(type_p)
        &&  index_p->lower_bound != NULL
        &&  index_p->lower_bound->kind == AST_int_const_k
        &&  index_p->lower_bound->value.int_val == 0
        &&  index_p->upper_bound != NULL
        &&  index_p->upper_bound->kind == AST_int_const_k)
        return TRUE;

    return FALSE;
}


/*
**  t y p e _ x m i t _ t y p e
**
**  Given a (presented) type, returns the transmissible type.  These two types
**  are only different when the type has a [transmit_as] attribute.
*/

static AST_type_n_t * type_xmit_type
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    while (type_p->xmit_as_type != NULL)
        type_p = type_p->xmit_as_type;

    return type_p;
}

/*
**  a r r a y   u t i l i t y   r o u t i n e s
**
**  Array "properties" used by other checks.
*/

/*
**  a r r a y _ i s _ c o n f o r m a n t _ u p p e r
**
**  Returns TRUE if an array is conformant in an upper dimension
**  (i.e., a dimension other than the first dimension).
*/

static boolean array_is_conformant_upper
(
    AST_array_n_t       *array_p        /* [in] Ptr to AST array node */
)

{
    AST_array_index_n_t *index_p;       /* Ptr to array index node */
    int                 i;              /* Integer array dimension 0..N-1 */

    index_p = array_p->index_vec;       /* Point at index vector */

    /* Check dimensions 1..N-1 for conformance (1st dim is index 0). */

    for (i = 1, index_p++ ; i < array_p->index_count ; i++, index_p++)
    {
        if (!AST_FIXED_LOWER_SET(index_p)
            ||  !AST_FIXED_UPPER_SET(index_p))
            return TRUE;    /* Conformant in this dimension */
    }

    return FALSE;
}


/*
**  a r r a y _ i s _ l a r g e
**
**  Returns TRUE if an array has a total (all dimensions) of >65535 elements.
*/

static boolean array_is_large
(
    AST_array_n_t       *array_p        /* [in] Ptr to AST array node */
)

{
    AST_array_index_n_t *index_p;       /* Ptr to array index node */
    int                 i;              /* Integer array dimension 0..N-1 */
    long                dim_size;       /* Size of one array dimension */
    long                elem_cnt;       /* Running total of array elements */

    index_p = array_p->index_vec;       /* Point at index vector */
    elem_cnt = 1;

    /*
     * Go through each dimension of the array computing the size of the
     * array in that dimension, and multiplying it into the running total
     * of elements.  If the total becomes >65535, the array is "large".
     * For conformant dimensions, we don't know the eventual
     * size of the dimension.  All we can really do is assume the smallest
     * size for that dimension (1) and continue with our check, as the
     * array still might be large from the fixed dimensions alone.
     */
    for (i = 0 ; i < array_p->index_count ; i++, index_p++)
    {
        if (AST_FIXED_LOWER_SET(index_p)
            &&  AST_FIXED_UPPER_SET(index_p))
        {
            dim_size =  index_p->upper_bound->value.int_val -
                        index_p->lower_bound->value.int_val + 1;
            /*
             * If the dimension itself has >65535 elements, the array is large.
             * This special case check is needed to avoid the possibility of
             * integer overflow on the running product.
             */
            if (dim_size > 65535)
                return TRUE;    /* Large in this dimension */

            /* If the total number of elements is >65535, the array is large. */
            elem_cnt = elem_cnt * dim_size;
            if (elem_cnt > 65535)
                return TRUE;    /* Large in overall size */
        }
    }

    return FALSE;
}


/*
**  a r r a y _ h a s _ o p e n _ l b
**
**  Returns TRUE if an array has an open lower bound in any dimension.
*/

static boolean array_has_open_lb
(
    AST_array_n_t       *array_p        /* [in] Ptr to AST array node */
)

{
    AST_array_index_n_t *index_p;       /* Ptr to array index node */
    int                 i;              /* Integer array dimension 0..N-1 */

    index_p = array_p->index_vec;       /* Point at index vector */

    /* Go through each dimension of the array checking for open lower bound. */

    for (i = 0 ; i < array_p->index_count ; i++, index_p++)
        if (!AST_FIXED_LOWER_SET(index_p))
            return TRUE;

    return FALSE;
}

/*
**  p a r a m e t e r   u t i l i t y   r o u t i n e s
**
**  Utility routines for parameter nodes, used by other checks.
*/

/*
**  p a r a m _ f o l l o w _ r e f _ p t r
**
**  Dereferences a parameter's type if it is a pointer and it meets certain
**  conditions.  Some of the conditions apply always; others are dependent
**  on the 'mode' argument (see below, and checker.h).
**
**  The general intent of this function is to follow reference pointers.
**  However, there are ambiguities as to what that really means, and
**  different dereferencing behavior is required depending on the need.
**  Thus, the 'mode' argument allows one to tweak what this routine does.
**
**  Returns address of pointee type node if the type was dereferenced.
**  Returns address of parameter's type node if the type was NOT dereferenced.
*/

AST_type_n_t * param_follow_ref_ptr     /* Returns ptr to type node */
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    CHK_follow_t        mode            /* [in] Follow mode */
)

{
    AST_type_n_t        *type_p;        /* Ptr to AST type node */
    AST_type_n_t        *ptee_type_p;   /* Pointee type */

    type_p = param_p->type;

    if (type_p->kind != AST_pointer_k)
        return type_p;

    ptee_type_p = type_p->type_structure.pointer->pointee_type;

    /*
     * The pointer is followed, with these EXCEPTIONS:
     *  - on a function result parameter, which by def can't be a ref ptr
     *  - void *, since "void passed-by-reference" makes no sense
     *  - function pointer
     *  - arrayified [string] pointers (but not pointer to string array)
     *
     * These additional EXCEPTIONS apply when the indicated mode(s) are selected
     * (see checker.h):
     *  - a named pointer                (mode == CHK_follow_any)
     *  - a [unique] or [ptr] pointer    (mode != CHK_follow_any)
     *  - arrayified via size attributes (mode != CHK_follow_ref_arr_siz)
     */
                                                /* EXCEPTIONS to following:  */
    if (param_p == param_p->uplink->result      /* The result param          */
        ||  ptee_type_p->kind == AST_void_k     /* (void *)                  */
        ||  ptee_type_p->kind == AST_function_k /* Function pointer          */
        ||  (type_p->name != NAMETABLE_NIL_ID   /* Not anonymous             */
             && mode == CHK_follow_any)         /*   AND follow any mode     */
        ||  (!AST_REF_SET(param_p)            /* [unique] attribute        */
             && mode != CHK_follow_any)         /*  AND not follow any mode  */
        ||  (param_p->field_attrs != NULL       /* Arrayified via size attrs */
             && instance_has_array_attr(param_p)/*   array not switch attr   */
             && mode != CHK_follow_ref_arr_siz) /*   BUT mode = don't follow */
        ||  ((AST_STRING_SET(type_p)            /* Arrayified via [string]   */
              || AST_STRING_SET(param_p))       /*   on type or param, AND   */
             && type_is_string(type_p)))        /*  not superfluous pointer  */
        return type_p;

    return ptee_type_p;
}


/*
**  d e f a u l t _ t o _ a u t o _ h a n d l e
**
**  Issues message that auto_handle is assumed for a particular operation.
**
**  Implicit Outputs:   def_auto_handle
*/

static int def_auto_handle = 0;

static void default_to_auto_handle
(
    AST_operation_n_t   *op_p,          /* [in] Ptr to AST operation node */
    int                 message_id      /* [in] message it display */
)

{
    char const *id_name;   /* Operation name */

    NAMETABLE_id_to_string(op_p->name, &id_name);
    message_print(message_id, id_name);
    def_auto_handle++;
}


/*
**  i n s t a n c e   u t i l i t y   r o u t i n e s
**
**  Instance "properties" used by other checks.
**  An "instance" is an instance (arm, field, parameter) of some data type.
*/

/*
**  i n s t a n c e _ i s _ v a r y i n g _ u p p e r
**
**  Returns TRUE if an instance of an array is varying in an upper dimension
**  (i.e., a dimension other than the first dimension).
*/

static boolean instance_is_varying_upper
(
    AST_array_n_t       *array_p,       /* [in] Ptr to AST array node */
    AST_field_attr_n_t  *fattr_p        /* [in] Ptr to AST field attr. node */
)

{
    AST_field_ref_n_t   *first_p;       /* first_is ref for a dimension */
    AST_field_ref_n_t   *last_p;        /* last_is ref for a dimension */
    AST_field_ref_n_t   *length_p;      /* length_is ref for a dimension */
    int                 i;              /* Integer array dimension 0..N-1 */

    if (fattr_p == NULL)
        return FALSE;

    first_p  = fattr_p->first_is_vec;
    last_p   = fattr_p->last_is_vec;
    length_p = fattr_p->length_is_vec;

    /* Check dimensions 1..N-1 for varying attribute (1st dim is index 0). */

    for (i = 1 ; i < array_p->index_count ; i++)
    {
        if (first_p != NULL)
        {
            first_p++;
            if (first_p->valid)
                return TRUE;    /* Varying in this dimension */
        }

        if (last_p != NULL)
        {
            last_p++;
            if (last_p->valid)
                return TRUE;    /* Varying in this dimension */
        }

        if (length_p != NULL)
        {
            length_p++;
            if (length_p->valid)
                return TRUE;    /* Varying in this dimension */
        }
    }

    return FALSE;
}

/*
**  f a t t r _ s w i t c h _ i s
**
**  Checks a field attribute node's [switch_is] attribute.
*/

static void fattr_switch_is
(
    AST_field_attr_n_t  *fattr_p,       /* [in] Ptr to AST field attr. node */
    ASTP_node_t         *node_p,        /* [in] Ptr to field or param node */
    AST_type_n_t        *type_p         /* [in] Ptr to field/param data type */
)

{
    AST_type_n_t        *ref_type_p;    /* Ptr to size info field/param type */
    AST_type_n_t        *deref_type_p;  /* Dereferenced field/param type */
    AST_type_n_t        *sw_type_p;     /* [switch_type] type */
    NAMETABLE_id_t      ref_name;       /* Field/param name */
    boolean             is_ref_ptr;     /* TRUE => ref_type_p is [ref] ptr */

    /* Checks assume presence of switch_is attr and union data type. */

    if (fattr_p->switch_is == NULL)
        return;

    type_p = ASTP_chase_ptr_to_kind(type_p, AST_disc_union_k);
    if (type_p == NULL)
        /* Parser supposed to catch this. */
        error(NIDL_INTERNAL_ERROR, __FILE__, __LINE__);

    sw_type_p = type_p->type_structure.disc_union->discrim_type;
    if (sw_type_p == NULL) return;

    /* Pick up [switch_is] field or parameter reference, if any. */

    if (node_p->fe_info->node_kind == fe_field_n_k)
    {
        ref_type_p = fattr_p->switch_is->ref.f_ref->type;
        ref_name   = fattr_p->switch_is->ref.f_ref->name;
        is_ref_ptr = (AST_REF_SET(fattr_p->switch_is->ref.f_ref));
    }
    else
    {
        ref_type_p = fattr_p->switch_is->ref.p_ref->type;
        ref_name   = fattr_p->switch_is->ref.p_ref->name;
        is_ref_ptr = (AST_REF_SET(fattr_p->switch_is->ref.p_ref));
    }

    /*
     * Param/field referenced in [switch_is] might be a ptr.  Chase ptr
     * to its base type.  This sets deref_type_p->fe_info->pointer_count
     * to the number of dereferences needed to get to the base type.
     */
    deref_type_p = ASTP_chase_ptr_to_type(ref_type_p);

    /* The union switch variable 'name' cannot have [ptr] or [unique] attr */

    if (deref_type_p->fe_info->pointer_count > 0
        &&  !is_ref_ptr)
    {
        char const *id_name;       /* [switch_is] variable name */
        NAMETABLE_id_to_string(ref_name, &id_name);
        CHECKER_error(fattr_p, NIDL_NEUSWPTR, id_name);
    }

    /* Data type of [switch_is] variable does not agree with [switch_type] */

    if (deref_type_p->kind != sw_type_p->kind)
    {
        char const *id_name;       /* [switch_is] variable name */
        char const *sw_type_name;  /* [switch_type] name, if any */

        NAMETABLE_id_to_string(ref_name, &id_name);
        NAMETABLE_id_to_string(sw_type_p->name, &sw_type_name);
        if (sw_type_name == NULL) sw_type_name = "";

        CHECKER_error(fattr_p, NIDL_SWDATATYPE, id_name, sw_type_name);
    }

    /* A [switch_is] variable must not have a represent_as type */

    if (deref_type_p->rep_as_type != NULL)
        CHECKER_error(fattr_p, NIDL_DISCRIMREPAS);

    /* A [switch_is] variable must not have a transmit_as type */

    if (deref_type_p->xmit_as_type != NULL)
        CHECKER_error(fattr_p, NIDL_DISCRIMXMITAS);
}
/*
**  f a t t r _ c h e c k _ s i z e
**
**  Checks a field attribute node for missing or inconsistent size attributes.
*/

static void fattr_check_size
(
    AST_field_attr_n_t  *fattr_p,       /* [in] Ptr to AST field attr. node */
    ASTP_node_t         *node_p,        /* [in] Ptr to field or param node */
    AST_type_n_t        *type_p,        /* [in] Ptr to field data type node */
    AST_interface_n_t   *int_p,         /* [in] Ptr to interface node */
    unsigned short      dim             /* [in] Array dimension to check */
)

{
    AST_array_n_t       *array_p;       /* Ptr to array node */
    AST_array_index_n_t *index_p = NULL;       /* Ptr to array index node */
    boolean             str_attr_set;   /* T => [string] on type or instance */

    /* Determine if [string] attribute set on type or instance. */

    if (AST_STRING_SET(type_p)
        ||  (node_p->fe_info->node_kind == fe_field_n_k
            &&  AST_STRING_SET((AST_field_n_t *)node_p))
        ||  (node_p->fe_info->node_kind == fe_parameter_n_k
            &&  AST_STRING_SET((AST_parameter_n_t *)node_p)))
        str_attr_set = TRUE;
    else
        str_attr_set = FALSE;

    /*
     * Assume parser won't put field attribute node on non-arrays.
     * An array can be represented either in normal array syntax,
     * or as a pointer that has any of the array attributes.
     */
    if (type_p->kind == AST_array_k)
    {
        array_p = type_p->type_structure.array;
        index_p = &array_p->index_vec[dim];
    }
    else
        array_p = NULL;

    /* Can't have both [max_is] and [size_is] attributes */

    if (fattr_p->max_is_vec != NULL
        &&  fattr_p->max_is_vec[dim].valid
        &&  fattr_p->size_is_vec != NULL
        &&  fattr_p->size_is_vec[dim].valid)
        CHECKER_error(fattr_p, NIDL_MAXSIZECONF);

    /* Can't have constant [size_is] and varying [min_is] */

    if (fattr_p->min_is_vec != NULL
        &&  fattr_p->min_is_vec[dim].valid
        &&  fattr_p->min_is_vec[dim].constant == false
        &&  fattr_p->size_is_vec != NULL
        &&  fattr_p->size_is_vec[dim].valid
        &&  fattr_p->size_is_vec[dim].constant == true)
	CHECKER_error(fattr_p, NIDL_CONSTREQ);

    /* Can't have both [last_is] and [length_is] attributes */

    if (fattr_p->last_is_vec != NULL
        &&  fattr_p->last_is_vec[dim].valid
        &&  fattr_p->length_is_vec != NULL
        &&  fattr_p->length_is_vec[dim].valid)
        CHECKER_error(fattr_p, NIDL_LASTLENCONF);

    /* Can't have constant [length_is] and varying [first_is] */

    if (fattr_p->first_is_vec != NULL
        &&  fattr_p->first_is_vec[dim].valid
        &&  fattr_p->first_is_vec[dim].constant == false
        &&  fattr_p->length_is_vec != NULL
        &&  fattr_p->length_is_vec[dim].valid
        &&  fattr_p->length_is_vec[dim].constant == true)
	CHECKER_error(fattr_p, NIDL_CONSTREQ);

    /*
     * Forgo the remaining checks if the field or parameter type is a pointer
     * to an array, since mixed pointer and array declarations with array size
     * attributes are disallowed by other checks.
     */
    if (type_p->kind == AST_pointer_k)
    {
        AST_type_n_t    *ref_type_p;    /* Pointee data type */

        ref_type_p = ASTP_chase_ptr_to_type(type_p);
        if (ref_type_p->kind == AST_array_k)
            return;
    }
    else if (fattr_p->range != NULL && !type_is_rangeable(type_p))
    {
        CHECKER_error(fattr_p, NIDL_RANGEATTR);
    }

    /* [max_is] or [size_is] attribute required */

    if (array_p != NULL     /* Array in array syntax */
        &&  !AST_LOCAL_SET(int_p)
        &&  type_p->xmit_as_type == NULL
        &&  !AST_FIXED_UPPER_SET(index_p)
        &&  (fattr_p->max_is_vec == NULL
            ||  !fattr_p->max_is_vec[dim].valid)
        &&  (fattr_p->size_is_vec == NULL
            ||  !fattr_p->size_is_vec[dim].valid))
        CHECKER_error(fattr_p, NIDL_MAXSIZEATTR);

    if (array_p == NULL     /* Non-[string] array in pointer syntax */
        &&  !str_attr_set
        &&  (fattr_p->first_is_vec != NULL
            ||  fattr_p->last_is_vec != NULL
            ||  fattr_p->length_is_vec != NULL
            ||  fattr_p->min_is_vec != NULL)
        &&  fattr_p->max_is_vec == NULL
        &&  fattr_p->size_is_vec == NULL)
        CHECKER_error(fattr_p, NIDL_MAXSIZEATTR);

    /* [min_is] attribute required */

    if (array_p != NULL
        &&  !AST_LOCAL_SET(int_p)
        &&  type_p->xmit_as_type == NULL
        &&  !AST_FIXED_LOWER_SET(index_p)
        &&  (fattr_p->min_is_vec == NULL
            ||  !fattr_p->min_is_vec[dim].valid))
        CHECKER_error(fattr_p, NIDL_MINATTREQ);

    if (array_p != NULL && fattr_p->range != NULL)
        CHECKER_error(fattr_p, NIDL_RANGEATTR);
}


/*
**  f a t t r _ f i r s t _ i s
**
**  Checks a field attribute node's [first_is] attribute.
*/

static void fattr_first_is
(
    AST_field_attr_n_t  *fattr_p,       /* [in] Ptr to AST field attr. node */
    ASTP_node_t         *node_p,        /* [in] Ptr to field or param node */
    unsigned short      dim             /* [in] Array dimension to check */
)

{
    AST_type_n_t        *ref_type_p;    /* Ptr to size info field/param type */
    AST_type_n_t        *deref_type_p = NULL;  /* Dereferenced field/param type */
    NAMETABLE_id_t      ref_name = 0;       /* Field/param name */
    boolean             is_ref_ptr = false;     /* TRUE => ref_type_p is [ref] ptr */

    /* Pick up [first_is] field or parameter reference, if any. */

    if (fattr_p->first_is_vec != NULL
        &&  fattr_p->first_is_vec[dim].valid)
    {
        if (fattr_p->first_is_vec[dim].constant)
	    return;

        if (node_p->fe_info->node_kind == fe_field_n_k)
        {
            ref_type_p = fattr_p->first_is_vec[dim].ref.f_ref->type;
            ref_name   = fattr_p->first_is_vec[dim].ref.f_ref->name;
            is_ref_ptr = (AST_REF_SET(fattr_p->first_is_vec[dim].ref.f_ref));
        }
        else
        {
            ref_type_p = fattr_p->first_is_vec[dim].ref.p_ref->type;
            ref_name   = fattr_p->first_is_vec[dim].ref.p_ref->name;
            is_ref_ptr = (AST_REF_SET(fattr_p->first_is_vec[dim].ref.p_ref));
        }

        /*
         * Param/field referenced in [first_is] might be a ptr.  Chase ptr
         * to its base type.  This sets deref_type_p->fe_info->pointer_count
         * to the number of dereferences needed to get to the base type.
         */
        deref_type_p = ASTP_chase_ptr_to_type(ref_type_p);
    }

    /* [first_is] variable must be of type integer */

    if (fattr_p->first_is_vec != NULL
        &&  fattr_p->first_is_vec[dim].valid
        &&  (!type_is_index(deref_type_p)
            ||  deref_type_p->fe_info->pointer_count
                != fattr_p->first_is_vec[dim].fe_info->pointer_count))
        CHECKER_error(fattr_p, NIDL_FIRSTYPEINT);

    /* A size attribute variable must not have a represent_as type */

    if (fattr_p->first_is_vec != NULL
        &&  fattr_p->first_is_vec[dim].valid
        &&  deref_type_p->rep_as_type != NULL)
        CHECKER_error(fattr_p, NIDL_SIZEVARREPAS);

    /* A size attribute variable must not have a transmit_as type */

    if (fattr_p->first_is_vec != NULL
        &&  fattr_p->first_is_vec[dim].valid
        &&  deref_type_p->xmit_as_type != NULL)
        CHECKER_error(fattr_p, NIDL_SIZEVARXMITAS);

    /* The array size attr var 'name' can not have [ptr] or [unique] attr */

    if (fattr_p->first_is_vec != NULL
        &&  fattr_p->first_is_vec[dim].valid
        &&  deref_type_p->fe_info->pointer_count > 0
        &&  !is_ref_ptr)
    {
        char const *id_name;       /* [first_is] variable name */

        NAMETABLE_id_to_string(ref_name, &id_name);

        CHECKER_error(fattr_p, NIDL_SIZEPRMPTR, id_name);
    }
}


/*
**  f a t t r _ l a s t _ i s
**
**  Checks a field attribute node's [last_is] attribute.
*/

static void fattr_last_is
(
    AST_field_attr_n_t  *fattr_p,       /* [in] Ptr to AST field attr. node */
    ASTP_node_t         *node_p,        /* [in] Ptr to field or param node */
    unsigned short      dim             /* [in] Array dimension to check */
)

{
    AST_type_n_t        *ref_type_p;    /* Ptr to size info field/param type */
    AST_type_n_t        *deref_type_p = NULL;  /* Dereferenced field/param type */
    NAMETABLE_id_t      ref_name = 0;       /* Field/param name */
    boolean             is_ref_ptr = false;     /* TRUE => ref_type_p is [ref] ptr */

    /* Pick up [last_is] field or parameter reference, if any. */

    if (fattr_p->last_is_vec != NULL
        &&  fattr_p->last_is_vec[dim].valid)
    {
        if (fattr_p->last_is_vec[dim].constant)
	    return;

        if (node_p->fe_info->node_kind == fe_field_n_k)
        {
            ref_type_p = fattr_p->last_is_vec[dim].ref.f_ref->type;
            ref_name   = fattr_p->last_is_vec[dim].ref.f_ref->name;
            is_ref_ptr = (AST_REF_SET(fattr_p->last_is_vec[dim].ref.f_ref));
        }
        else
        {
            ref_type_p = fattr_p->last_is_vec[dim].ref.p_ref->type;
            ref_name   = fattr_p->last_is_vec[dim].ref.p_ref->name;
            is_ref_ptr = (AST_REF_SET(fattr_p->last_is_vec[dim].ref.p_ref));
        }

        /*
         * Param/field referenced in [last_is] might be a ptr.  Chase ptr
         * to its base type.  This sets deref_type_p->fe_info->pointer_count
         * to the number of dereferences needed to get to the base type.
         */
        deref_type_p = ASTP_chase_ptr_to_type(ref_type_p);
    }

    /* [last_is] variable must be of type integer */

    if (fattr_p->last_is_vec != NULL
        &&  fattr_p->last_is_vec[dim].valid
        &&  (!type_is_index(deref_type_p)
            ||  deref_type_p->fe_info->pointer_count
                != fattr_p->last_is_vec[dim].fe_info->pointer_count))
        CHECKER_error(fattr_p, NIDL_LASTYPEINT);

    /* A size attribute variable must not have a represent_as type */

    if (fattr_p->last_is_vec != NULL
        &&  fattr_p->last_is_vec[dim].valid
        &&  deref_type_p->rep_as_type != NULL)
        CHECKER_error(fattr_p, NIDL_SIZEVARREPAS);

    /* A size attribute variable must not have a transmit_as type */

    if (fattr_p->last_is_vec != NULL
        &&  fattr_p->last_is_vec[dim].valid
        &&  deref_type_p->xmit_as_type != NULL)
        CHECKER_error(fattr_p, NIDL_SIZEVARXMITAS);

    /* The array size attr var 'name' can not have [ptr] or [unique] attr */

    if (fattr_p->last_is_vec != NULL
        &&  fattr_p->last_is_vec[dim].valid
        &&  deref_type_p->fe_info->pointer_count > 0
        &&  !is_ref_ptr)
    {
        char const *id_name;       /* [last_is] variable name */

        NAMETABLE_id_to_string(ref_name, &id_name);

        CHECKER_error(fattr_p, NIDL_SIZEPRMPTR, id_name);
    }
}


/*
**  f a t t r _ l e n g t h _ i s
**
**  Checks a field attribute node's [length_is] attribute.
*/

static void fattr_length_is
(
    AST_field_attr_n_t  *fattr_p,       /* [in] Ptr to AST field attr. node */
    ASTP_node_t         *node_p,        /* [in] Ptr to field or param node */
    unsigned short      dim             /* [in] Array dimension to check */
)

{
    AST_type_n_t        *ref_type_p;    /* Ptr to size info field/param type */
    AST_type_n_t        *deref_type_p = NULL;  /* Dereferenced field/param type */
    NAMETABLE_id_t      ref_name = 0;       /* Field/param name */
    boolean             is_ref_ptr = false;     /* TRUE => ref_type_p is [ref] ptr */

    /* Pick up [length_is] field or parameter reference, if any. */

    if (fattr_p->length_is_vec != NULL
        &&  fattr_p->length_is_vec[dim].valid)
    {
        if (fattr_p->length_is_vec[dim].constant)
	    return;

        if (node_p->fe_info->node_kind == fe_field_n_k)
        {
            ref_type_p = fattr_p->length_is_vec[dim].ref.f_ref->type;
            ref_name   = fattr_p->length_is_vec[dim].ref.f_ref->name;
            is_ref_ptr = (AST_REF_SET(fattr_p->length_is_vec[dim].ref.f_ref));
        }
        else
        {
            ref_type_p = fattr_p->length_is_vec[dim].ref.p_ref->type;
            ref_name   = fattr_p->length_is_vec[dim].ref.p_ref->name;
            is_ref_ptr = (AST_REF_SET(fattr_p->length_is_vec[dim].ref.p_ref));
        }

        /*
         * Param/field referenced in [length_is] might be a ptr.  Chase ptr
         * to its base type.  This sets deref_type_p->fe_info->pointer_count
         * to the number of dereferences needed to get to the base type.
         */
        deref_type_p = ASTP_chase_ptr_to_type(ref_type_p);
    }

    /* [length_is] variable must be of type integer */

    if (fattr_p->length_is_vec != NULL
        &&  fattr_p->length_is_vec[dim].valid
        &&  (!type_is_index(deref_type_p)
            ||  deref_type_p->fe_info->pointer_count
                != fattr_p->length_is_vec[dim].fe_info->pointer_count))
        CHECKER_error(fattr_p, NIDL_LENTYPEINT);

    /* A size attribute variable must not have a represent_as type */

    if (fattr_p->length_is_vec != NULL
        &&  fattr_p->length_is_vec[dim].valid
        &&  deref_type_p->rep_as_type != NULL)
        CHECKER_error(fattr_p, NIDL_SIZEVARREPAS);

    /* A size attribute variable must not have a transmit_as type */

    if (fattr_p->length_is_vec != NULL
        &&  fattr_p->length_is_vec[dim].valid
        &&  deref_type_p->xmit_as_type != NULL)
        CHECKER_error(fattr_p, NIDL_SIZEVARXMITAS);

    /* The array size attr var 'name' can not have [ptr] or [unique] attr */

    if (fattr_p->length_is_vec != NULL
        &&  fattr_p->length_is_vec[dim].valid
        &&  deref_type_p->fe_info->pointer_count > 0
        &&  !is_ref_ptr)
    {
        char const *id_name;       /* [length_is] variable name */

        NAMETABLE_id_to_string(ref_name, &id_name);

        CHECKER_error(fattr_p, NIDL_SIZEPRMPTR, id_name);
    }
}


/*
**  f a t t r _ m i n _ i s
**
**  Checks a field attribute node's [min_is] attribute.
*/

static void fattr_min_is
(
    AST_field_attr_n_t  *fattr_p,       /* [in] Ptr to AST field attr. node */
    ASTP_node_t         *node_p,        /* [in] Ptr to field or param node */
    AST_type_n_t        *type_p,        /* [in] Ptr to field/param data type */
    unsigned short      dim             /* [in] Array dimension to check */
)

{
    AST_array_n_t       *array_p;       /* Ptr to array node */
    AST_array_index_n_t *index_p = NULL;       /* Ptr to array index node */
    AST_type_n_t        *ref_type_p;    /* Ptr to size info field/param type */
    AST_type_n_t        *deref_type_p = NULL;  /* Dereferenced field/param type */
    NAMETABLE_id_t      ref_name = 0;       /* Field/param name */
    boolean             is_ref_ptr = false;     /* TRUE => ref_type_p is [ref] ptr */

    /*
     * Assume parser won't put field attribute node on non-arrays.
     * An array can be represented either in normal array syntax,
     * or as a pointer that has any of the array attributes.
     */
    if (type_p->kind == AST_array_k)
    {
        array_p = type_p->type_structure.array;
        index_p = &array_p->index_vec[dim];
    }
    else
        array_p = NULL;

    /* Pick up [min_is] field or parameter reference, if any. */

    if (fattr_p->min_is_vec != NULL
        &&  fattr_p->min_is_vec[dim].valid
        &&  fattr_p->min_is_vec[dim].constant == false)
    {
        if (node_p->fe_info->node_kind == fe_field_n_k)
        {
            ref_type_p = fattr_p->min_is_vec[dim].ref.f_ref->type;
            ref_name   = fattr_p->min_is_vec[dim].ref.f_ref->name;
            is_ref_ptr = (AST_REF_SET(fattr_p->min_is_vec[dim].ref.f_ref));
        }
        else
        {
            ref_type_p = fattr_p->min_is_vec[dim].ref.p_ref->type;
            ref_name   = fattr_p->min_is_vec[dim].ref.p_ref->name;
            is_ref_ptr = (AST_REF_SET(fattr_p->min_is_vec[dim].ref.p_ref));
        }

        /*
         * Param/field referenced in [min_is] might be a ptr.  Chase ptr
         * to its base type.  This sets deref_type_p->fe_info->pointer_count
         * to the number of dereferences needed to get to the base type.
         */
        deref_type_p = ASTP_chase_ptr_to_type(ref_type_p);
    }

    /* Arrays with a nonzero lower bound require -standard extended */

    /*
     * Anonymous type checking will pick this up for anonymous array types in
     * array syntax.  Normal type checking will pick it up for named types at
     * the typedef site.  Therefore, we only need to handle the pointer case.
     */
    if (type_p->kind == AST_pointer_k
        &&  fattr_p->min_is_vec != NULL
        &&  (*(int *)cmd_val[opt_standard] <= opt_standard_dce_1_0))
        CHECKER_warning(fattr_p, NIDL_NOPORTNZLB, OPT_STD_EXTENDED);

    /* [min_is] variable <var> invalid: lower bound fixed in dimension <n> */

    if (array_p != NULL
        &&  fattr_p->min_is_vec != NULL
        &&  (!type_is_array(type_p)
            ||  (type_is_array(type_p)
                &&  fattr_p->min_is_vec[dim].valid
                &&  AST_FIXED_LOWER_SET(index_p))))
        CHECKER_error(fattr_p, NIDL_MINCFMTYPE, dim+1);

    /* Constants are excused from further checks */

    if (fattr_p->min_is_vec != NULL
        &&  fattr_p->min_is_vec[dim].valid
        &&  fattr_p->min_is_vec[dim].constant)
        return;

    /* [min_is] variable must be of type integer */

    if (fattr_p->min_is_vec != NULL
        &&  fattr_p->min_is_vec[dim].valid
        &&  (!type_is_index(deref_type_p)
            ||  deref_type_p->fe_info->pointer_count
                != fattr_p->min_is_vec[dim].fe_info->pointer_count))
        CHECKER_error(fattr_p, NIDL_MINTYPEINT);

    /* A size attribute variable must not have a represent_as type */

    if (fattr_p->min_is_vec != NULL
        &&  fattr_p->min_is_vec[dim].valid
        &&  deref_type_p->rep_as_type != NULL)
        CHECKER_error(fattr_p, NIDL_SIZEVARREPAS);

    /* A size attribute variable must not have a transmit_as type */

    if (fattr_p->min_is_vec != NULL
        &&  fattr_p->min_is_vec[dim].valid
        &&  deref_type_p->xmit_as_type != NULL)
        CHECKER_error(fattr_p, NIDL_SIZEVARXMITAS);

    /* The array size attr var 'name' can not have [ptr] or [unique] attr */

    if (fattr_p->min_is_vec != NULL
        &&  fattr_p->min_is_vec[dim].valid
        &&  deref_type_p->fe_info->pointer_count > 0
        &&  !is_ref_ptr)
    {
        char const *id_name;       /* [min_is] variable name */

        NAMETABLE_id_to_string(ref_name, &id_name);

        CHECKER_error(fattr_p, NIDL_SIZEPRMPTR, id_name);
    }
}


/*
**  f a t t r _ m a x _ i s
**
**  Checks a field attribute node's [max_is] attribute.
*/

static void fattr_max_is
(
    AST_field_attr_n_t  *fattr_p,       /* [in] Ptr to AST field attr. node */
    ASTP_node_t         *node_p,        /* [in] Ptr to field or param node */
    AST_type_n_t        *type_p,        /* [in] Ptr to field/param data type */
    unsigned short      dim             /* [in] Array dimension to check */
)

{
    AST_array_n_t       *array_p;       /* Ptr to array node */
    AST_array_index_n_t *index_p = NULL;       /* Ptr to array index node */
    AST_type_n_t        *ref_type_p;    /* Ptr to size info field/param type */
    AST_type_n_t        *deref_type_p = NULL;  /* Dereferenced field/param type */
    NAMETABLE_id_t      ref_name = 0;       /* Field/param name */
    boolean             is_ref_ptr = false;     /* TRUE => ref_type_p is [ref] ptr */

    /*
     * Assume parser won't put field attribute node on non-arrays.
     * An array can be represented either in normal array syntax,
     * or as a pointer that has any of the array attributes.
     */
    if (type_p->kind == AST_array_k)
    {
        array_p = type_p->type_structure.array;
        index_p = &array_p->index_vec[dim];
    }
    else
        array_p = NULL;

    /* Pick up [max_is] field or parameter reference, if any. */

    if (fattr_p->max_is_vec != NULL
        &&  fattr_p->max_is_vec[dim].valid
        &&  fattr_p->max_is_vec[dim].constant == false)
    {
        if (node_p->fe_info->node_kind == fe_field_n_k)
        {
            ref_type_p = fattr_p->max_is_vec[dim].ref.f_ref->type;
            ref_name   = fattr_p->max_is_vec[dim].ref.f_ref->name;
            is_ref_ptr = (AST_REF_SET(fattr_p->max_is_vec[dim].ref.f_ref));
        }
        else
        {
            ref_type_p = fattr_p->max_is_vec[dim].ref.p_ref->type;
            ref_name   = fattr_p->max_is_vec[dim].ref.p_ref->name;
            is_ref_ptr = (AST_REF_SET(fattr_p->max_is_vec[dim].ref.p_ref));
        }

        /*
         * Param/field referenced in [max_is] might be a ptr.  Chase ptr
         * to its base type.  This sets deref_type_p->fe_info->pointer_count
         * to the number of dereferences needed to get to the base type.
         */
        deref_type_p = ASTP_chase_ptr_to_type(ref_type_p);
    }

    /* [max_is] variable <var> invalid: upper bound fixed in dimension <n> */

    if (array_p != NULL
        &&  fattr_p->max_is_vec != NULL
        &&  (!type_is_array(type_p)
            ||  (type_is_array(type_p)
                &&  fattr_p->max_is_vec[dim].valid
                &&  AST_FIXED_UPPER_SET(index_p))))
        CHECKER_error(fattr_p, NIDL_MAXCFMTYPE, dim+1);

    /* Constants are excused from further checks */

    if (fattr_p->max_is_vec != NULL
        &&  fattr_p->max_is_vec[dim].valid
        &&  fattr_p->max_is_vec[dim].constant == true)
        return;

    /* [max_is] variable must be of type integer */

    if (fattr_p->max_is_vec != NULL
        &&  fattr_p->max_is_vec[dim].valid
        &&  (!type_is_index(deref_type_p)
            ||  deref_type_p->fe_info->pointer_count
                != fattr_p->max_is_vec[dim].fe_info->pointer_count))
        CHECKER_error(fattr_p, NIDL_MAXTYPEINT);

    /* A size attribute variable must not have a represent_as type */

    if (fattr_p->max_is_vec != NULL
        &&  fattr_p->max_is_vec[dim].valid
        &&  deref_type_p->rep_as_type != NULL)
        CHECKER_error(fattr_p, NIDL_SIZEVARREPAS);

    /* A size attribute variable must not have a transmit_as type */

    if (fattr_p->max_is_vec != NULL
        &&  fattr_p->max_is_vec[dim].valid
        &&  deref_type_p->xmit_as_type != NULL)
        CHECKER_error(fattr_p, NIDL_SIZEVARXMITAS);

    /* The array size attr var 'name' can not have [ptr] or [unique] attr */

    if (fattr_p->max_is_vec != NULL
        &&  fattr_p->max_is_vec[dim].valid
        &&  deref_type_p->fe_info->pointer_count > 0
        &&  !is_ref_ptr)
    {
        char const *id_name;       /* [max_is] variable name */

        NAMETABLE_id_to_string(ref_name, &id_name);

        CHECKER_error(fattr_p, NIDL_SIZEPRMPTR, id_name);
    }
}


/*
**  f a t t r _ s i z e _ i s
**
**  Checks a field attribute node's [size_is] attribute.
*/

static void fattr_size_is
(
    AST_field_attr_n_t  *fattr_p,       /* [in] Ptr to AST field attr. node */
    ASTP_node_t         *node_p,        /* [in] Ptr to field or param node */
    AST_type_n_t        *type_p,        /* [in] Ptr to field/param data type */
    unsigned short      dim             /* [in] Array dimension to check */
)

{
    AST_array_n_t       *array_p;       /* Ptr to array node */
    AST_array_index_n_t *index_p = NULL;       /* Ptr to array index node */
    AST_type_n_t        *ref_type_p;    /* Ptr to size info field/param type */
    AST_type_n_t        *deref_type_p = NULL;  /* Dereferenced field/param type */
    NAMETABLE_id_t      ref_name = 0;       /* Field/param name */
    boolean             is_ref_ptr = false;     /* TRUE => ref_type_p is [ref] ptr */

    /*
     * Assume parser won't put field attribute node on non-arrays.
     * An array can be represented either in normal array syntax,
     * or as a pointer that has any of the array attributes.
     */
    if (type_p->kind == AST_array_k)
    {
        array_p = type_p->type_structure.array;
        index_p = &array_p->index_vec[dim];
    }
    else
        array_p = NULL;

    /* Pick up [size_is] field or parameter reference, if any. */

    if (fattr_p->size_is_vec != NULL
        &&  fattr_p->size_is_vec[dim].valid
        &&  fattr_p->size_is_vec[dim].constant == false)
    {
        if (node_p->fe_info->node_kind == fe_field_n_k)
        {
            ref_type_p = fattr_p->size_is_vec[dim].ref.f_ref->type;
            ref_name   = fattr_p->size_is_vec[dim].ref.f_ref->name;
            is_ref_ptr = (AST_REF_SET(fattr_p->size_is_vec[dim].ref.f_ref));
        }
        else
        {
            ref_type_p = fattr_p->size_is_vec[dim].ref.p_ref->type;
            ref_name   = fattr_p->size_is_vec[dim].ref.p_ref->name;
            is_ref_ptr = (AST_REF_SET(fattr_p->size_is_vec[dim].ref.p_ref));
        }

        /*
         * Param/field referenced in [size_is] might be a ptr.  Chase ptr
         * to its base type.  This sets deref_type_p->fe_info->pointer_count
         * to the number of dereferences needed to get to the base type.
         */
        deref_type_p = ASTP_chase_ptr_to_type(ref_type_p);
    }

    /* [size_is] variable <var> invalid: upper bound fixed in dimension <n> */

    if (array_p != NULL
        &&  fattr_p->size_is_vec != NULL
        &&  (!type_is_array(type_p)
            ||  (type_is_array(type_p)
                &&  fattr_p->size_is_vec[dim].valid
                &&  AST_FIXED_UPPER_SET(index_p))))
        CHECKER_error(fattr_p, NIDL_SIZECFMTYPE, dim+1);
  
    /* Constants are excused from further checks */
 
    if (fattr_p->size_is_vec != NULL
        &&  fattr_p->size_is_vec[dim].valid
        &&  fattr_p->size_is_vec[dim].constant == true)
        return;

    /* [size_is] variable must be of type integer */

    if (fattr_p->size_is_vec != NULL
        &&  fattr_p->size_is_vec[dim].valid
        &&  (!type_is_index(deref_type_p)
            ||  deref_type_p->fe_info->pointer_count
                != fattr_p->size_is_vec[dim].fe_info->pointer_count))
        CHECKER_error(fattr_p, NIDL_SIZETYPEINT);

    /* A size attribute variable must not have a represent_as type */

    if (fattr_p->size_is_vec != NULL
        &&  fattr_p->size_is_vec[dim].valid
        &&  deref_type_p->rep_as_type != NULL)
        CHECKER_error(fattr_p, NIDL_SIZEVARREPAS);

    /* A size attribute variable must not have a transmit_as type */

    if (fattr_p->size_is_vec != NULL
        &&  fattr_p->size_is_vec[dim].valid
        &&  deref_type_p->xmit_as_type != NULL)
        CHECKER_error(fattr_p, NIDL_SIZEVARXMITAS);

    /* The array size attr var 'name' can not have [ptr] or [unique] attr */

    if (fattr_p->size_is_vec != NULL
        &&  fattr_p->size_is_vec[dim].valid
        &&  deref_type_p->fe_info->pointer_count > 0
        &&  !is_ref_ptr)
    {
        char const *id_name;       /* [size_is] variable name */

        NAMETABLE_id_to_string(ref_name, &id_name);

        CHECKER_error(fattr_p, NIDL_SIZEPRMPTR, id_name);
    }
}


/*
**  f a t t r _ p a r a m _ c o n f o r m a n t
**
**  Checks an field attribute node associated with a parameter node to make
**  sure that a conformant array's size information parameters have the [in]
**  parameter attribute.
*/

static void fattr_param_conformant
(
    AST_field_attr_n_t  *fattr_p,       /* [in] Ptr to AST field attr. node */
    AST_parameter_n_t   *param_p,       /* [in] Ptr to associated param node */
    AST_interface_n_t   *int_p,         /* [in] Ptr to interface node */
    unsigned short      dim             /* [in] Array dimension */
)

{
    /* [min_is] parameter must have [in] attribute */

    if (!AST_LOCAL_SET(int_p)
        &&  param_p->type->xmit_as_type == NULL
        &&  fattr_p->min_is_vec != NULL
        &&  fattr_p->min_is_vec[dim].valid
        &&  fattr_p->min_is_vec[dim].constant == false
        &&  AST_IN_SET(param_p)
        &&  !AST_IN_SET(fattr_p->min_is_vec[dim].ref.p_ref))
        CHECKER_error(fattr_p, NIDL_MININATTR);

    /* [max_is] parameter must have [in] attribute */

    if (!AST_LOCAL_SET(int_p)
        &&  param_p->type->xmit_as_type == NULL
        &&  fattr_p->max_is_vec != NULL
        &&  fattr_p->max_is_vec[dim].valid
        &&  fattr_p->max_is_vec[dim].constant == false
        &&  AST_IN_SET(param_p)
        &&  !AST_IN_SET(fattr_p->max_is_vec[dim].ref.p_ref))
        CHECKER_error(fattr_p, NIDL_MAXINATTR);

    /* [size_is] parameter must have [in] attribute */

    if (!AST_LOCAL_SET(int_p)
        &&  param_p->type->xmit_as_type == NULL
        &&  fattr_p->size_is_vec != NULL
        &&  fattr_p->size_is_vec[dim].valid
        &&  fattr_p->size_is_vec[dim].constant == false
        &&  AST_IN_SET(param_p)
        &&  !AST_IN_SET(fattr_p->size_is_vec[dim].ref.p_ref))
        CHECKER_error(fattr_p, NIDL_SIZEINATTR);
}


/*
**  f a t t r _ p a r a m _ v a r y i n g
**
**  Checks an field attribute node associated with a parameter node to make
**  sure that a varying array's directional attributes are consistent with
**  the corresponding size information parameter's directional attributes.
*/

static void fattr_param_varying
(
    AST_field_attr_n_t  *fattr_p,       /* [in] Ptr to AST field attr. node */
    AST_parameter_n_t   *param_p,       /* [in] Ptr to associated param node */
    AST_interface_n_t   *int_p,         /* [in] Ptr to interface node */
    unsigned short      dim             /* [in] Array dimension */
)

{
    /* [first_is] parameter must have [in] attribute */

    if (!AST_LOCAL_SET(int_p)
        &&  param_p->type->xmit_as_type == NULL
        &&  fattr_p->first_is_vec != NULL
        &&  fattr_p->first_is_vec[dim].valid
        &&  fattr_p->first_is_vec[dim].constant == false
        &&  AST_IN_SET(param_p)
        &&  !AST_IN_SET(fattr_p->first_is_vec[dim].ref.p_ref))
        CHECKER_error(fattr_p, NIDL_FIRSTINATTR);

    /* [last_is] parameter must have [in] attribute */

    if (!AST_LOCAL_SET(int_p)
        &&  param_p->type->xmit_as_type == NULL
        &&  fattr_p->last_is_vec != NULL
        &&  fattr_p->last_is_vec[dim].valid
        &&  fattr_p->last_is_vec[dim].constant == false
        &&  AST_IN_SET(param_p)
        &&  !AST_IN_SET(fattr_p->last_is_vec[dim].ref.p_ref))
        CHECKER_error(fattr_p, NIDL_LASTINATTR);

    /* [length_is] parameter must have [in] attribute */

    if (!AST_LOCAL_SET(int_p)
        &&  param_p->type->xmit_as_type == NULL
        &&  fattr_p->length_is_vec != NULL
        &&  fattr_p->length_is_vec[dim].valid
        &&  fattr_p->length_is_vec[dim].constant == false
        &&  AST_IN_SET(param_p)
        &&  !AST_IN_SET(fattr_p->length_is_vec[dim].ref.p_ref))
        CHECKER_error(fattr_p, NIDL_LENINATTR);
}


/*
**  f a t t r _ p a r a m _ c h e c k
**
**  Checks an AST field attribute node.  This set of checks is specific to
**  field attribute nodes associated with parameters.  (Field attribute nodes
**  can also occur in field nodes).
*/

static void fattr_param_check
(
    AST_field_attr_n_t  *fattr_p,       /* [in] Ptr to AST field attr. node */
    AST_parameter_n_t   *param_p,       /* [in] Ptr to associated param node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    unsigned short      dim;            /* Array dimension */
    unsigned short      max_dim;        /* Maximum dimension */
    AST_type_n_t        *type_p;        /* Parameter data type */

    type_p = param_p->type;

    /*
     * Assume parser won't put field attribute node on non-arrays.
     * An array can be represented either in normal array syntax,
     * or as a pointer that has any of the array attributes.
     */
    if (type_p->kind == AST_array_k)
        max_dim = type_p->type_structure.array->index_count;
    else
        max_dim = 1;    /* Arrays in pointer form can only be 1-dimensional */

    /* Check field attributes in each dimension of the array. */

    for (dim = 0 ; dim < max_dim ; dim++)
    {
        fattr_param_conformant(fattr_p, param_p, int_p, dim);
        fattr_param_varying(fattr_p, param_p, int_p, dim);
    }
}


/*
**  f a t t r _ c h e c k
**
**  Checks an AST field attribute node.
*/

static void fattr_check
(
    AST_field_attr_n_t  *fattr_p,       /* [in] Ptr to AST field attr. node */
    ASTP_node_t         *node_p,        /* [in] Ptr to field or param node */
    AST_type_n_t        *type_p,        /* [in] Ptr to field/param data type */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    unsigned short      dim;            /* Array dimension */
    unsigned short      max_dim;        /* Maximum dimension */

    /*
     * Assume parser won't put field attribute node on non-arrays.
     * An array can be represented either in normal array syntax,
     * or as a pointer that has any of the array attributes.
     */
    if (type_p->kind == AST_array_k)
        max_dim = type_p->type_structure.array->index_count;
    else
        max_dim = 1;    /* Arrays in pointer form can only be 1-dimensional */

    /* [max_is,length_is] or [size_is,last_is] used together */

    if (    ((fattr_p->max_is_vec != NULL) && (fattr_p->length_is_vec != NULL))
        ||  ((fattr_p->size_is_vec != NULL) && (fattr_p->last_is_vec != NULL)))
        CHECKER_warning(fattr_p, NIDL_MIXEDARRATTR);

    fattr_switch_is(fattr_p, node_p, type_p);
    /* Check field attributes in each dimension of the array. */

    for (dim = 0 ; dim < max_dim ; dim++)
    {
        fattr_check_size(fattr_p, node_p, type_p, int_p, dim);
        fattr_first_is(fattr_p, node_p, dim);
        fattr_last_is(fattr_p, node_p, dim);
        fattr_length_is(fattr_p, node_p, dim);
        fattr_min_is(fattr_p, node_p, type_p, dim);
        fattr_max_is(fattr_p, node_p, type_p, dim);
        fattr_size_is(fattr_p, node_p, type_p, dim);
    }
}

/*
**  i n d e x _ c o n s t _ t y p e
**
**  Checks an array index node's constant type.
*/

static void index_const_type
(
    AST_array_index_n_t *index_p        /* [in] Ptr to AST array index node */
)

{
    /* Invalid array index type */

    if ((AST_FIXED_LOWER_SET(index_p)
            && !const_is_integer(index_p->lower_bound))
        ||
        (AST_FIXED_UPPER_SET(index_p)
            && !const_is_integer(index_p->upper_bound)))
        CHECKER_error(index_p, NIDL_INVARRIND);
}


/*
**  i n d e x _ b o u n d s
**
**  Checks an array index node's bounds.
*/

static void index_bounds
(
    AST_array_index_n_t *index_p        /* [in] Ptr to AST array index node */
)

{
    /* Lower bound must not be greater than upper bound */

    if (AST_FIXED_LOWER_SET(index_p)
        &&  AST_FIXED_UPPER_SET(index_p)
        &&  index_p->lower_bound->value.int_val
            > index_p->upper_bound->value.int_val)
        CHECKER_error(index_p, NIDL_LBLESSUB);

    /* Arrays with a nonzero lower bound require -standard extended */

    if (   (!AST_FIXED_LOWER_SET(index_p)
            || index_p->lower_bound->value.int_val != 0)
        && (*(int *)cmd_val[opt_standard] <= opt_standard_dce_1_0) )
        CHECKER_warning(index_p, NIDL_NOPORTNZLB, OPT_STD_EXTENDED);
}


/*
**  i n d e x _ c h e c k
**
**  Checks an AST array index node.
*/

static void index_check
(
    AST_array_index_n_t *index_p        /* [in] Ptr to AST array index node */
)

{
    index_const_type(index_p);
    index_bounds(index_p);
}

/*
**  a r r a y _ e l e m e n t _ t y p e
**
**  Checks the data type of the elements of an array.
*/

static void array_element_type
(
    ASTP_node_t         *node_p,        /* [in] Ptr to array or pointer node */
    AST_type_n_t        *type_p,        /* [in] Ptr to array elem type node */
    AST_type_n_t        *arr_type_p,    /* [in] Ptr to array | ptr type node */
    AST_interface_n_t   *int_p,          /* [in] Ptr to interface node */
    boolean             arrayified      /* [in] true if arrayified pointer */
)

{
	AST_type_n_t        *etype_p;       /* Array element presented type */

	/* If the array element type is anonymous, it must undergo type checks. */

	if (type_is_anonymous(type_p))
		type_check(type_p, (ASTP_node_t *)node_p, int_p);

	etype_p = type_p;
	type_p = type_xmit_type(type_p);    /* Pick up transmissible type */

	/*
	 * Array elements cannot be conformant arrays or structures
	 * Exceptions: Array elements types that are the target of a [transmit_as]
	 * and have determinable size (i.e. either [string] or a struct).
	 */
	if (AST_CONFORMANT_SET(type_p)
			&& !(etype_p != type_p && AST_STRING_SET(type_p))
			&& !(etype_p != type_p && type_p->kind == AST_structure_k) )
		CHECKER_error(node_p, NIDL_ARRELEMCFMT);

	/* Array elements cannot be pipes */

	if (type_p->kind == AST_pipe_k)
		CHECKER_error(node_p, NIDL_ARRELEMPIPE);

	/* Array elements cannot be context handles */

	if (AST_CONTEXT_RD_SET(type_p))
		CHECKER_error(node_p, NIDL_ARRELEMCTX);

	/* Function pointers are not valid as elements of conformant arrays */

	if (!AST_LOCAL_SET(int_p)
			&&  type_is_function(type_p)
			&&  (AST_CONFORMANT_SET(arr_type_p) || arrayified))
		CHECKER_error(node_p, NIDL_FPCFMTARR);

	/* Array elements cannot be of type handle_t */

	if (!AST_LOCAL_SET(int_p)
			&&  type_p->kind == AST_handle_k)
#if 0   /** Obsolete **/
		&&  type_p->xmit_as_type == NULL)
#endif
			CHECKER_error(node_p, NIDL_HANARRELEM);

	/* void is valid only in an operation or pointer declaration */

	if (type_p->kind == AST_void_k)
		CHECKER_error(node_p, NIDL_VOIDOPPTR);

	/* void * must be used in conjunction with the [context_handle] attribute */

	if (!AST_LOCAL_SET(int_p)
#if 0   /** Obsolete **/
			&&  type_p->xmit_as_type == NULL
#endif
			&&  type_p->kind == AST_pointer_k
			&&  type_p->type_structure.pointer->pointee_type->kind == AST_void_k
			&&  !AST_CONTEXT_RD_SET(type_p)
			)
	{
		CHECKER_error(node_p, NIDL_PTRVOIDCTX);
	}

	/* The [ignore] attribute is not allowed on array elements */

	if (AST_IGNORE_SET(type_p))
		CHECKER_error(node_p, NIDL_IGNARRELEM);
}


/*
**  a r r a y _ c h e c k
**
**  Checks an array.  Note that both array and pointer nodes can
**  represent arrays.
*/

static void array_check
(
    ASTP_node_t         *node_p,        /* [in] Ptr to array or pointer node */
    AST_type_n_t        *arr_type_p,    /* [in] Array or ptr type node */
    ASTP_node_t         *parent_p,      /* [in] Parent of array or ptr type */
    AST_type_n_t        *type_p,        /* [in] Ptr to array elem type node */
    AST_interface_n_t   *int_p,         /* [in] Ptr to interface node */
    boolean             arrayified      /* [in] true if arrayified pointer */
)

{
    unsigned short      dim;            /* Array dimension */

    /* Arrays with [transmit_as] can't be conformant or varying */

    if (arr_type_p->xmit_as_type != NULL
        &&  (AST_CONFORMANT_SET(arr_type_p)
            ||  (parent_p->fe_info->node_kind == fe_parameter_n_k
                &&  ((AST_parameter_n_t *)parent_p)->field_attrs != NULL)
            ||  (parent_p->fe_info->node_kind == fe_field_n_k
                &&  ((AST_field_n_t *)parent_p)->field_attrs != NULL)))
        CHECKER_error(parent_p, NIDL_ARRXMITOPEN);

    array_element_type(node_p, type_p, arr_type_p, int_p, arrayified);

    /* Arrays of non-encapsulated unions are not allowed */

    if (type_p->kind == AST_disc_union_k
        &&  type_p->type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID)
        CHECKER_error(arr_type_p, NIDL_NEUARRAY);

	/* Don't allow arrays of interfaces */
	 if (type_p->kind == AST_interface_k)	{
		 char const * id_name;
		 NAMETABLE_id_to_string(type_p->name, &id_name);
		 CHECKER_error(arr_type_p, NIDL_INTREFNOTALO, id_name);
	 }
	 
    /*
     * If the array is represented in array (as opposed to pointer) syntax,
     * then check the indices of each dimension of the array.
     */
    if (node_p->fe_info->node_kind == fe_array_n_k)
    {
        AST_array_n_t       *array_p;   /* Ptr to array node */

        array_p = (AST_array_n_t *)node_p;

        for (dim = 0
            ;   dim < array_p->index_count
            ;   dim++)
            index_check(&array_p->index_vec[dim]);
    }
}

/*
**  p a r a m _ t y p e
**
**  Checks an parameter node's data type.
*/

static void param_type
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *top_type_p,    /* [in] Top-level parameter type */
    AST_type_n_t        *type_p,        /* [in] Parameter type */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
#if 0
    AST_type_n_t        *btype_p;       /* Base type */

    /*
     * First, special case checks that do want to look at a toplevel [ref]
     * pointer (most look through this pointer).
     */

    /* Cannot have more than one level of indirection to a ne union */

    btype_p = ASTP_chase_ptr_to_kind(top_type_p, AST_disc_union_k);

    if (btype_p != NULL
        &&  btype_p->kind == AST_disc_union_k
        &&  btype_p->type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID
        &&  btype_p->fe_info->pointer_count > 1)
        CHECKER_error(top_type_p, NIDL_PTRNEUNION);
#endif

    /*
     * If the parameter type is anonymous, it must undergo type checks.
     * Note that the passed type_p is the dereferenced parameter type if
     * pass-by-reference mechanism was implied; i.e. top-level *'s that
     * indicate pass-by-reference and are not arrays are not type-checked.
     */
    if (type_is_anonymous(type_p))
        type_check(type_p, (ASTP_node_t *)param_p, int_p);

    type_p = type_xmit_type(type_p);    /* Pick up transmissible type */

    /* void is valid only in an operation or pointer declaration */

    if (type_p->kind == AST_void_k
        &&  param_p != param_p->uplink->result) /* Not the result param */
        CHECKER_error(param_p, NIDL_VOIDOPPTR);

	 
    /* void * must be used in conjunction with the [context_handle] attr */

    if (!AST_LOCAL_SET(int_p)
        &&  type_p->xmit_as_type == NULL /* Allow if void* is NOT transmitted */
        &&  type_p->kind == AST_pointer_k
        &&  type_p->type_structure.pointer->pointee_type->kind == AST_void_k
        &&  !AST_CONTEXT_RD_SET(type_p)
        &&  !AST_CONTEXT_SET(param_p)
		&& !AST_LOCAL_SET(param_p->uplink)
		  )
        CHECKER_error(param_p, NIDL_PTRVOIDCTX);

    /* A type with [transmit_as] may not have other type attributes */

    if (top_type_p->xmit_as_type != NULL
        &&
        (AST_STRING_SET(param_p)
         || AST_STRING0_SET(param_p)
         || AST_UNIQUE_SET(param_p)
         || AST_SMALL_SET(param_p)
         || AST_CONTEXT_SET(param_p)
         ))
        CHECKER_error(param_p, NIDL_XMITTYPEATTRS);

    /*
     * V1 attributes are incompatible with this type
     * Note: Issue error only if the same error doesn't fire for the type.
     */
    if (FE_TEST(param_p->fe_info->flags, FE_HAS_V1_ATTR)
        &&  FE_TEST(param_p->fe_info->flags, FE_HAS_V2_ATTR)
        &&  ! ( FE_TEST(type_p->fe_info->flags, FE_HAS_V1_ATTR)
                &&  FE_TEST(type_p->fe_info->flags, FE_HAS_V2_ATTR) ))
        CHECKER_warning(param_p, NIDL_INCOMPATV1);

    /* Array function results are not allowed */

    if (type_p->kind == AST_array_k
        && param_p == param_p->uplink->result)
        CHECKER_error(param_p, NIDL_ARRFUNRES);
}


/*
**  p a r a m _ s i z e
**
**  Checks an parameter node to see if array size information is required.
*/

static void param_size
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *top_type_p,    /* [in] Top-level parameter type */
    AST_type_n_t        *type_p,        /* [in] Parameter type */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_field_attr_n_t  *fattr_p;       /* Field attributes */

    fattr_p = param_p->field_attrs;

    /* Array size information required */

    if (!AST_LOCAL_SET(int_p)
        &&  type_p->xmit_as_type == NULL
        &&  fattr_p == NULL
        &&  type_is_array(type_p)
        &&  (AST_CONFORMANT_SET(type_p)
            ||  AST_VARYING_SET(param_p))
        &&  !AST_STRING_SET(type_p)     /* [string] cases handled below */
        &&  !AST_STRING_SET(param_p))
    {
        char const *id_name;

        NAMETABLE_id_to_string(param_p->name, &id_name);
        CHECKER_error(param_p, NIDL_ARRSIZEINFO, id_name);
    }

    /*
     * Array size information required on conformant [string] arrays only
     * if an [out]-only parameter; otherwise, size is implicitly determined
     * by the length of the string.  Size info is not required for a [ptr]
     * char * string (which includes an operation result char * string)
     * or a [unique] char * string.
     */
    if (!AST_LOCAL_SET(int_p)
        &&  type_p->xmit_as_type == NULL
        &&  fattr_p == NULL
        &&  (   (type_p->kind == AST_array_k
                &&  AST_CONFORMANT_SET(type_p))
             || (type_p->kind == AST_pointer_k
                &&  param_p->uplink->result != param_p  /* not op result */
                &&  !AST_PTR_SET(type_p) && !AST_UNIQUE_SET(type_p)) )
        &&  (AST_STRING_SET(type_p) || AST_STRING_SET(param_p))
        &&  !AST_IN_SET(param_p))
    {
        char const *id_name;

        NAMETABLE_id_to_string(param_p->name, &id_name);
        CHECKER_error(param_p, NIDL_ARRSIZEINFO, id_name);
    }

    /* A [string] array can not have varying array attributes */

    if (!AST_LOCAL_SET(int_p)
        &&  type_p->xmit_as_type == NULL
        &&  AST_STRING_SET(param_p)
        &&  fattr_p != NULL
        &&  (fattr_p->first_is_vec != NULL
            ||  fattr_p->last_is_vec != NULL
            ||  fattr_p->length_is_vec != NULL))
        CHECKER_error(param_p, NIDL_STRVARY);

    /* Arrays with [transmit_as] can't be conformant or varying */

    if (type_p->xmit_as_type != NULL
        &&  fattr_p != NULL)
        CHECKER_error(param_p, NIDL_ARRXMITOPEN);

    /*
     * Current rules allow mixed open array [] and pointer * syntax ONLY
     * of the form *..*param[][N]..[N], i.e. when param is a [multidimensional]
     * array of pointers (that, currently, can be conformant or varying in the
     * first dimension only).  Pointers to open arrays are not allowed due to
     * the ambiguity as to which * or [] the array size attributes apply to.
     *
     * Builder only allows one open array dimension when pointer and array
     * syntax is mixed.  It will detect errors on such examples as:
     *      [in, size_is(s1,s2)] long (*param)[]
     *
     * A corollary is that in declarations with more than one * and no [] or
     * [N], array size information applies to the rightmost * only.
     * Builder will detect errors on such examples as:
     *      [in, size_is(s1,s2)] long **param
     *
     * Checker detects these 3 cases separately:
     *      [in, size_is(s)] long (*..*param)[N]..[N]       ARRSYNTAX
     *      [in, size_is(s)] long (*..*param)[][N]..[N]     PTRCFMTARR
     *      [in, length_is(l)] long (*..*param)[N]..[N]     PTRVARYARR
     */
    if (top_type_p->kind == AST_pointer_k)
    {
        AST_type_n_t    *ref_type_p;    /* Pointee data type */

        ref_type_p = ASTP_chase_ptr_to_type(top_type_p);

        /* Use array syntax to declare multidimensional arrays */

        if (ref_type_p->kind == AST_array_k
            &&  !AST_CONFORMANT_SET(ref_type_p)
            &&  fattr_p != NULL
            &&  (fattr_p->min_is_vec != NULL
                ||  fattr_p->max_is_vec != NULL
                ||  fattr_p->size_is_vec != NULL))
            CHECKER_error(param_p, NIDL_ARRSYNTAX);

        /* Pointers to conformant arrays are not allowed */

        /*
         * Can't just check for conformant set since a string can be conformant
         * and not require size attributes; a ptr to such a string is legal.
         */
        if (ref_type_p->kind == AST_array_k
            &&  AST_CONFORMANT_SET(ref_type_p)
            &&  (!AST_STRING_SET(ref_type_p)
                ||  (fattr_p != NULL
                    &&  (fattr_p->min_is_vec != NULL
                        ||  fattr_p->max_is_vec != NULL
                        ||  fattr_p->size_is_vec != NULL))))
        {
            CHECKER_error(param_p, NIDL_PTRCFMTARR);
            return;     /* Return with no further checking */
        }

        /* Pointers to varying arrays are not allowed */

        /*
         * Can't just check for varying set since a string is implicitly vary-
         * ing and cannot have length attrs; a ptr to such a string is legal.
         */
        if (ref_type_p->kind == AST_array_k
            &&  fattr_p != NULL
            &&  (fattr_p->first_is_vec != NULL
                ||  fattr_p->last_is_vec != NULL
                ||  fattr_p->length_is_vec != NULL))
        {
            CHECKER_error(param_p, NIDL_PTRVARYARR);
            return;     /* Return with no further checking */
        }
    }

    /* An [out] conformant array must be a top-level param or under a full pointer */

    /*
     * Note that this check covers any [out] conformant array below top-level
     * that is not under a full pointer.  Top-level [out] conformant arrays
     * are handled by the ARRSIZEINFO and *INATTR checks.
     */
    if (!AST_LOCAL_SET(int_p)
        &&  param_p->type->xmit_as_type == NULL
        &&  !AST_IN_SET(param_p)
        &&  FE_TEST(param_p->fe_info->flags, FE_HAS_CFMT_ARR))
        CHECKER_error(param_p, NIDL_OUTCFMTARR);
}


/*
**  p a r a m _ s t r u c t
**
**  Checks a parameter node - structure specific checks.
*/

static void param_struct
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *top_type_p,    /* [in] Top-level parameter type */
    AST_type_n_t        *type_p         /* [in] Parameter type */
)
{
    type_p = type_xmit_type(type_p);    /* Pick up transmissible type */

    /* All checks below assume that parameter data type is structure. */

    if (type_p->kind != AST_structure_k)
        return;

    /* Structures containing conformant arrays must be passed by reference */

    if (!AST_OUT_SET(param_p)
        &&  top_type_p->kind != AST_pointer_k
        &&  AST_CONFORMANT_SET(type_p))
        CHECKER_error(param_p, NIDL_CFMTARRREF);
}


/*
**  p a r a m _ p i p e
**
**  Checks a parameter node - pipe specific checks.
*/

static void param_pipe
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *top_type_p ATTRIBUTE_UNUSED,    /* [in] Top-level parameter type */
    AST_type_n_t        *type_p         /* [in] Parameter type */
)

{
    type_p = type_xmit_type(type_p);    /* Pick up transmissible type */

    /* [ptr] attribute not valid on pipe parameters */

    if (type_p->kind == AST_pipe_k
        &&  AST_PTR_SET(param_p))
        CHECKER_error(param_p, NIDL_PTRPIPE);

    /* Operation result may not be a pipe */

    if (param_p == param_p->uplink->result
        &&  type_p->kind == AST_pipe_k)
        CHECKER_error(param_p, NIDL_OPRESPIPE);
}


/*
**  p a r a m _ i n _ l i n e
**
**  Checks an parameter node's [in_line] and [out_of_line] attributes.
*/

static void param_in_line
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *type_p         /* [in] Parameter type */
)

{
    /* Can't have both [in_line] and [out_of_line] parameter attributes */

    if (AST_IN_LINE_SET(param_p)
        &&  AST_OUT_OF_LINE_SET(param_p))
        CHECKER_acf_error(param_p, NIDL_PRMLINEATTR);

    /* [in_line] and [out_of_line] attributes apply only to non-scalar types */

    if ((AST_IN_LINE_SET(param_p) || AST_OUT_OF_LINE_SET(param_p))
        &&  type_is_scalar(type_p))
    {
        char const *file_name;     /* Related file name */
        char const *type_name;     /* Data type name */

        STRTAB_str_to_string(type_p->fe_info->file, &file_name);
        NAMETABLE_id_to_string(type_p->name, &type_name);

        CHECKER_acf_warning(param_p, NIDL_LINENONSCAL);
        CHECKER_acf_warning(param_p, NIDL_NAMEDECLAT, type_name, file_name,
                          type_p->fe_info->source_line);
    }
}


/*
**  p a r a m _ s t r i n g
**
**  Checks an parameter node's [string] and [v1_string] attributes.
*/

static void param_string
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *type_p         /* [in] Parameter type */
)

{
    /* A [v1_string] must be an array of char with fixed bounds */

    if (AST_STRING0_SET(param_p)
        &&  !type_is_v1_string(type_p))
        CHECKER_error(param_p, NIDL_STRV1FIXED);

    /* The [v1_string] attribute can only be applied to a [v1_array] */

    if ((AST_STRING0_SET(param_p) || AST_STRING0_SET(type_p))
        &&  !(AST_SMALL_SET(param_p) || AST_SMALL_SET(type_p)))
        CHECKER_error(param_p, NIDL_STRV1ARRAY);

    /* The [string] attribute cannot be applied to a [v1_array] */

    if ((AST_STRING_SET(param_p) && AST_SMALL_SET(param_p))
        ||  (AST_STRING_SET(param_p) && AST_SMALL_SET(type_p))
        ||  (AST_STRING_SET(type_p) && AST_SMALL_SET(param_p)))
        CHECKER_error(param_p, NIDL_STRARRAYV1);

    /* Cannot have both [string] and [v1_string] attributes */

    if (    (AST_STRING_SET(param_p) || AST_STRING_SET(type_p))
        &&  (AST_STRING0_SET(param_p) || AST_STRING0_SET(type_p))   )
    {
        ASTP_attr_flag_t attr1 = ASTP_STRING;
        ASTP_attr_flag_t attr2 = ASTP_STRING0;

        CHECKER_error(param_p, NIDL_CONFLICTATTR,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)),
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr2)));
    }

    /*
     * Forgo last check if the declaration is a pointer to an array and either
     * (the declaration has size attributes) or (the array is conformant and
     * not a string), since mixed pointer and array declarations that either
     * have or require array size attributes are disallowed by other checks.
     */
    if (type_p->kind == AST_pointer_k)
    {
        AST_type_n_t    *ref_type_p;    /* Pointee data type */

        ref_type_p = ASTP_chase_ptr_to_type(type_p);
        if (ref_type_p->kind == AST_array_k
            &&  (param_p->field_attrs != NULL
                ||  (AST_CONFORMANT_SET(ref_type_p)
                    &&  !AST_STRING_SET(ref_type_p))))
            return;
    }

    /* [string] attribute valid only for one-dim arrays of char or byte */

    if (AST_STRING_SET(param_p)
        &&  !type_is_string(param_p->type))
        CHECKER_error(param_p, NIDL_STRCHARBYTE);
}


/*
**  p a r a m _ p o i n t e r
**
**  Checks an parameter node's attributes related to pointers.
*/

static void param_pointer
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *top_type_p,    /* [in] Top-level parameter type */
    AST_type_n_t        *type_p,        /* [in] Parameter type */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    boolean pointer_attr_valid = FALSE;

    /*
     *  The AST Builder catches these errors on param ptr attributes:
     *      [ref] parameter requires explicit top-level '*'
     *      [ptr] parameter requires explicit top-level '*'
     *      [unique] parameter requires explicit top-level '*'
     */

    /* [out,ptr] parameters are not allowed */

    if (AST_OUT_SET(param_p)
        &&  !AST_IN_SET(param_p)
        &&  AST_PTR_SET(param_p)
        &&  param_p->uplink->result != param_p) /* Not the result param */
        CHECKER_error(param_p, NIDL_OUTPTRPRM);

    /* [out,unique] parameters are not allowed */

    if (AST_OUT_SET(param_p)
        &&  !AST_IN_SET(param_p)
        &&  AST_UNIQUE_SET(param_p)
        &&  param_p->uplink->result != param_p) /* Not the result param */
        CHECKER_error(param_p, NIDL_OUTUNIQPRM);

    /* [ref] function results are not valid */

    if (param_p->uplink->result == param_p      /* IS the result param */
        &&  AST_REF_SET(param_p))
        CHECKER_error(param_p, NIDL_REFFUNRES);

    /* [unique] function results are not valid */

    if (param_p->uplink->result == param_p      /* IS the result param */
        &&  AST_UNIQUE_SET(param_p))
        CHECKER_error(param_p, NIDL_UNIQFUNRES);

    if (top_type_p->kind == AST_array_k
        ||  (top_type_p->kind == AST_pointer_k
            &&  top_type_p->type_structure.pointer->pointee_type->kind
                != AST_void_k))
        pointer_attr_valid = TRUE;

    /* [ref] attribute valid only for pointer or array types */

    if (AST_REF_SET(param_p) && !pointer_attr_valid)
        CHECKER_error(param_p, NIDL_REFATTRPTR);

    /* [unique] attribute valid only for pointer or array types */

    if (AST_UNIQUE_SET(param_p) && !pointer_attr_valid)
        CHECKER_error(param_p, NIDL_UNIQATTRPTR);

    /* [ptr] attribute valid only for pointer or array types */

    if (AST_PTR_SET(param_p) && !pointer_attr_valid)
        CHECKER_error(param_p, NIDL_PTRATTRPTR);

    /* [unique] attribute requires -standard extended */

    if (AST_UNIQUE_SET(param_p)
        &&  (*(int *)cmd_val[opt_standard] <= opt_standard_dce_1_0))
        CHECKER_warning(param_p, NIDL_NOPORTUNIQUE, OPT_STD_EXTENDED);

#if 0
	/* if the parameter is a pointer to an interface, then it should ignore
	 * the pointer attributes REF, UNIQUE or PTR */
	 if (top_type_p->kind == AST_pointer_k && type_p->kind == AST_interface_k
			 && AST_REF_SET(param_p))
		 CHECKER_warning(param_p, NIDL_PTRATTBIGN);
	 if (type_p->kind == AST_pointer_k && type_p->type_structure.pointer->pointee_type->kind == AST_interface_k
			 && (AST_UNIQUE_SET(param_p) || AST_PTR_SET(param_p)))
		 CHECKER_warning(param_p, NIDL_PTRATTBIGN);
#endif
	 
    /*
     * If the parameter is a pointer, and it is not a pointer to an array,
     * and it has any of the array attributes, then it is an arrayified
     * pointer and it must undergo the checks for arrays.
     */
    if (type_p->kind == AST_pointer_k
        &&  type_p->type_structure.pointer->pointee_type->kind != AST_array_k
#if 0
        &&  (param_p->field_attrs != NULL
#endif
        &&  (instance_has_array_attr(param_p)
            ||  AST_STRING_SET(type_p)
            ||  AST_STRING_SET(param_p)))
    {
        array_check((ASTP_node_t *)type_p->type_structure.pointer,
                    type_p,
                    (ASTP_node_t *)param_p,
                    type_p->type_structure.pointer->pointee_type,
                    int_p,
                    true);
    }
}


/*
**  p a r a m _ s m a l l
**
**  Checks an parameter node's [v1_array] attribute.
*/

static void param_small
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *top_type_p,    /* [in] Top-level parameter type */
    AST_type_n_t        *type_p         /* [in] Parameter type */
)

{
    AST_array_n_t       *array_p;       /* Ptr to array node */
    AST_field_attr_n_t  *fattr_p;       /* Ptr to field attribute node */

    /* A [v1_array] must be in array, not pointer, syntax */

    if (AST_SMALL_SET(param_p)
        &&  (type_p->kind == AST_pointer_k
            ||  top_type_p->kind == AST_pointer_k))
        CHECKER_error(param_p, NIDL_SMALLARRSYN);

    /*
     * Remaining checks only apply to array types in array syntax.
     * V1 did not allow arrays in pointer syntax.
     */
    if (!type_is_array_np(type_p))
        return;

    fattr_p = param_p->field_attrs;
    array_p = type_p->type_structure.array;

    /* A [v1_array] can be conformant or varying in the first dimension only */

    if (AST_SMALL_SET(param_p)
        &&  ((AST_CONFORMANT_SET(type_p)
                && array_is_conformant_upper(array_p))
            ||
            (AST_VARYING_SET(param_p)
                && instance_is_varying_upper(array_p, param_p->field_attrs))))
        CHECKER_error(param_p, NIDL_SMALLMULTID);

    /* [v1_array] attribute invalid for array with more than 65535 elements */

    if (AST_SMALL_SET(param_p)
        &&  array_is_large(array_p))
        CHECKER_error(param_p, NIDL_SMALLINV);

    /* A conformant [v1_array] must also be varying */

    if ((AST_SMALL_SET(type_p) || AST_SMALL_SET(param_p))
        &&  AST_CONFORMANT_SET(type_p)
        &&  !AST_VARYING_SET(param_p))
        CHECKER_error(param_p, NIDL_SMALLCFMT);

    /* A [v1_array] can not have the [min_is] or [first_is] attributes */

    if ((AST_SMALL_SET(type_p) || AST_SMALL_SET(param_p))
        &&  fattr_p != NULL
        &&  (fattr_p->min_is_vec != NULL
            ||  fattr_p->first_is_vec != NULL))
        CHECKER_error(param_p, NIDL_SMALLMINFIRST);
}


/*
**  p a r a m _ c o n t e x t
**
**  Checks an parameter node's [context_handle] attribute.
*/

static void param_context
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *top_type_p,    /* [in] Top-level parameter type */
    AST_type_n_t        *type_p         /* [in] Parameter type */
)

{
    AST_type_n_t        *deref_type_p;  /* Explicit pointer's pointee type */
    boolean             type_is_pointer;/* Type is real pointer, not void* */

    deref_type_p = param_follow_ref_ptr(param_p, CHK_follow_any);
    /*
     * Special case code to follow an operation result pointer.  This allows
     * us to catch and disallow pointer to context handle operation results.
     */
    if (param_p->uplink->result == param_p
        &&  type_p->kind == AST_pointer_k
        &&  type_p->type_structure.pointer->pointee_type->kind != AST_void_k)
        deref_type_p = type_p->type_structure.pointer->pointee_type;

    /* [context_handle] attribute only applies to void * types */

    if (AST_CONTEXT_SET(param_p)
        &&  !AST_CONTEXT_RD_SET(type_p) /* no error if already done for type */
        &&  (deref_type_p->kind != AST_pointer_k
            ||  (deref_type_p->kind == AST_pointer_k
                &&  deref_type_p->type_structure.pointer->pointee_type->kind
                    != AST_void_k)))
            CHECKER_error(param_p, NIDL_CTXPTRVOID);

    type_is_pointer = (deref_type_p != top_type_p);

    /* [unique] pointers to context handles are not allowed */

    if (type_is_pointer
        &&  (AST_CONTEXT_SET(param_p) || AST_CONTEXT_RD_SET(deref_type_p))
        &&  AST_UNIQUE_SET(param_p))
        CHECKER_error(param_p, NIDL_UNIQCTXHAN);

    /* Full pointers to context handles are not allowed */

    if (type_is_pointer
        &&  (AST_CONTEXT_SET(param_p) || AST_CONTEXT_RD_SET(deref_type_p))
        &&  AST_PTR_SET(param_p)
        &&  deref_type_p->kind != AST_structure_k)  /* opaque ctx handle */
        CHECKER_error(param_p, NIDL_PTRCTXHAN);
}


/*
**  p a r a m _ v a r y i n g
**
**  Checks an parameter node's "varying" (synthesized) attribute.
*/

static void param_varying
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *type_p,        /* [in] Parameter type */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    /*
     * Assume that if the varying attribute is set, the parameter is an
     * array type and the field_attrs field is valid.  An array can be
     * represented either in normal array syntax, or as a pointer that
     * has any of the array attributes.
     */
    if (type_p->kind != AST_array_k)
        return;     /* All checks below assume normal array syntax */

    /* Arrays varying in other than the first dim require -standard extended */

    if (!AST_LOCAL_SET(int_p)
        &&  type_p->xmit_as_type == NULL
        &&  AST_VARYING_SET(param_p)
        &&  instance_is_varying_upper(type_p->type_structure.array,
                                      param_p->field_attrs)
        &&  (*(int *)cmd_val[opt_standard] <= opt_standard_dce_1_0))
        CHECKER_warning(param_p, NIDL_NOPORTVARY, OPT_STD_EXTENDED);
}


/*
**  p a r a m _ d i r e c t i o n
**
**  Checks an parameter node's [in] and [out] attributes.
*/

static void param_direction
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *top_type_p,    /* [in] Top-level parameter type */
    AST_type_n_t        *type_p,        /* [in] Parameter type */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    type_p = type_xmit_type(type_p);    /* Pick up transmissible type */

    /* Parameter must have a least one of [in] and [out] attributes */

    if (type_p->kind != AST_void_k
		  &&  !AST_LOCAL_SET(int_p)
        &&  !AST_IN_SET(param_p)
        &&  !AST_OUT_SET(param_p))
        CHECKER_error(param_p, NIDL_PRMINOROUT);

    /* [out] parameters must be passed by reference */

    if (AST_OUT_SET(param_p)
        &&  top_type_p->kind != AST_array_k /* Arrays are implicitly by ref */
        &&  param_p->uplink->result != param_p      /* Not the result param */
        &&  ((top_type_p->kind != AST_pointer_k
             && top_type_p->kind != AST_pipe_k)     /* Pipes an exception */
          || (top_type_p->kind == AST_pointer_k     /* void * is special */
             && top_type_p->type_structure.pointer->pointee_type->kind
                == AST_void_k)
			 || (top_type_p->kind == AST_pointer_k
				 && top_type_p->type_structure.pointer->pointee_type->kind == AST_interface_k)
			 ))
        CHECKER_error(param_p, NIDL_OUTPRMREF);

    /* [out] parameter requires explicit top-level '*' */

    if (!AST_LOCAL_SET(int_p)
        &&  AST_OUT_SET(param_p)
        &&  param_p->uplink->result != param_p  /* Not the result param */
        &&  top_type_p->kind == AST_pointer_k
        &&  top_type_p->type_structure.pointer->pointee_type->kind
            != AST_function_k               /* Func ptrs an exception */
        &&  top_type_p->name != NAMETABLE_NIL_ID)
        CHECKER_error(param_p, NIDL_OUTSTAR);

    /* Function pointer parameters can only be [in] parameters */

    if (AST_OUT_SET(param_p)
        &&  type_p->kind == AST_pointer_k
        &&  type_p->type_structure.pointer->pointee_type->kind
            == AST_function_k)
        CHECKER_error(param_p, NIDL_FPINPRM);

    /* [in(shape)] is not yet supported */

    if (AST_IN_SHAPE_SET(param_p))
        CHECKER_error(param_p, NIDL_NYSINSHAPE);

    /* [out(shape)] is not yet supported */

    if (AST_OUT_SHAPE_SET(param_p))
        CHECKER_error(param_p, NIDL_NYSOUTSHAPE);

    if (type_p->kind == AST_pointer_k)
        type_p = ASTP_chase_ptr_to_kind(type_p, AST_disc_union_k);

    /* An [in] or [in,out] union must have an [in] discriminator */

    if (AST_IN_SET(param_p)
        &&  type_p != NULL
        &&  type_p->kind == AST_disc_union_k
        &&  type_p->type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID
        &&  param_p->field_attrs != NULL
        &&  param_p->field_attrs->switch_is != NULL
        &&  !AST_IN_SET(param_p->field_attrs->switch_is->ref.p_ref))
        CHECKER_error(param_p, NIDL_DISCRIMIN);

    /* An [in,out] or [out] union must have an [out] discriminator */

#if 0
    if (AST_OUT_SET(param_p)
        &&  type_p != NULL
        &&  type_p->kind == AST_disc_union_k
        &&  type_p->type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID
        &&  param_p->field_attrs != NULL
        &&  param_p->field_attrs->switch_is != NULL
        &&  !AST_OUT_SET(param_p->field_attrs->switch_is->ref.p_ref))
        CHECKER_error(param_p, NIDL_DISCRIMOUT);
#endif

}


/*
**  p a r a m _ c o m m _ s t a t u s
**
**  Checks an parameter node's [comm_status] attribute.
*/

static void param_comm_status
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *type_p         /* [in] Parameter type */
)

{
    char const *type_name;     /* Data type name */

    /* [comm_status] parameter must be an [out] parameter */

    if (AST_COMM_STATUS_SET(param_p)
        &&  param_p != param_p->uplink->result  /* Op result special cased */
        &&  !AST_OUT_SET(param_p))
        CHECKER_error(param_p, NIDL_STSPRMOUT,
            acf_keyword_lookup(COMM_STATUS_KW));

    /* [fault_status] parameter must be an [out] parameter */

    if (AST_FAULT_STATUS_SET(param_p)
        &&  param_p != param_p->uplink->result  /* Op result special cased */
        &&  !AST_OUT_SET(param_p))
        CHECKER_error(param_p, NIDL_STSPRMOUT,
            acf_keyword_lookup(FAULT_STATUS_KW));

    /* Chase type down to base named type. */

    while (type_p->defined_as != NULL)
        type_p = type_p->defined_as;
    NAMETABLE_id_to_string(type_p->name, &type_name);

    /* [comm_status] parameter must be of type error_status_t */

    if (AST_COMM_STATUS_SET(param_p)
        &&  param_p != param_p->uplink->result  /* Op result special cased */
        &&  strcmp(type_name, "error_status_t") != 0)
        CHECKER_error(param_p, NIDL_STSVARTYPE,
            acf_keyword_lookup(COMM_STATUS_KW));

    /* [fault_status] parameter must be of type error_status_t */

    if (AST_FAULT_STATUS_SET(param_p)
        &&  param_p != param_p->uplink->result  /* Op result special cased */
        &&  strcmp(type_name, "error_status_t") != 0)
        CHECKER_error(param_p, NIDL_STSVARTYPE,
            acf_keyword_lookup(FAULT_STATUS_KW));

    /* A [comm_status] operation must return a value of type error_status_t */

    if (AST_COMM_STATUS_SET(param_p)
        &&  param_p == param_p->uplink->result  /* The operation result */
        &&  strcmp(type_name, "error_status_t") != 0)
        CHECKER_error(param_p->uplink, NIDL_STSRETVAL,
            acf_keyword_lookup(COMM_STATUS_KW));

    /* A [fault_status] operation must return a value of type error_status_t */

    if (AST_FAULT_STATUS_SET(param_p)
        &&  param_p == param_p->uplink->result  /* The operation result */
        &&  strcmp(type_name, "error_status_t") != 0)
        CHECKER_error(param_p->uplink, NIDL_STSRETVAL,
            acf_keyword_lookup(FAULT_STATUS_KW));
}
static void param_switch_is
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *type_p         /* [in] Parameter type */
)

{
    if (type_p->kind == AST_pointer_k)
        type_p = ASTP_chase_ptr_to_kind(type_p, AST_disc_union_k);

    /* A non-encapsulated union declaration must have a [switch_is] attribute */

    if (type_p != NULL
        &&  type_p->kind == AST_disc_union_k
        &&  type_p->type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID
        &&  (param_p->field_attrs == NULL
            ||  param_p->field_attrs->switch_is == NULL))
        CHECKER_error(param_p, NIDL_NEUSWATTR);
}


/*
**  p a r a m _ f i r s t _ h a n d l e
**
**  Checks a parameter node's first parameter to make sure it is a
**  handle if explicit handles are in use.
*/

static void param_first_handle
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_operation_n_t   *op_p;          /* Operation containing the parameter */
#if 0
    AST_type_n_t        *top_type_p;    /* Top-level parameter type */
#endif
    AST_type_n_t        *type_p;        /* Param type (deref'd if necess.) */

    op_p = param_p->uplink;

#if 0
    /*
     * If the parameter type has a top-level '*' which indicates passing
     * mechanism only, follow the pointer to the data of interest.
     *      top_type_p  = top-level parameter type
     *      type_p      = possibly dereferenced parameter type
     */
    top_type_p = param_p->type;
#endif
    type_p = param_follow_ref_ptr(param_p, CHK_follow_any);

    /* No binding handle parameter for 'operation' - auto_handle assumed. */

    if (int_p->implicit_handle_name == NAMETABLE_NIL_ID
			 && !AST_OBJECT_SET(int_p)
        &&  !AST_AUTO_HANDLE_SET(int_p)
        &&  !AST_LOCAL_SET(int_p)
        &&  !type_is_handle(type_p)
        &&  !AST_HAS_IN_CTX_SET(op_p))
        default_to_auto_handle(op_p, NIDL_DEFAUTOHAN);
#if 0   /** Don't default to auto_handle for op ct. [in,out] context handles **/
    else if (int_p->implicit_handle_name == NAMETABLE_NIL_ID
        &&  !AST_AUTO_HANDLE_SET(int_p)
        &&  !AST_LOCAL_SET(int_p)
        &&  !type_is_handle(type_p)
        &&  AST_HAS_IN_OUT_CTX_SET(op_p))
        default_to_auto_handle(op_p, NIDL_POSSAUTOHAN);
#endif

    /* handle_t binding parameter must be an [in] parameter */

    if (!AST_LOCAL_SET(int_p)
        &&  type_p->kind == AST_handle_k
        &&  (!AST_IN_SET(param_p) || AST_OUT_SET(param_p)))
        CHECKER_error(param_p, NIDL_HANPRMIN);

    /* [handle] binding parameter must be an in parameter */

    if (!AST_LOCAL_SET(int_p)
        &&  AST_HANDLE_SET(type_p)
        &&  !AST_IN_SET(param_p))
        CHECKER_error(param_p, NIDL_HANDLEIN);

    /* [ptr] attribute invalid on binding handle parameter */

    if (!AST_LOCAL_SET(int_p)
        &&  type_is_handle(type_p)
        &&  AST_PTR_SET(param_p))
        CHECKER_error(param_p, NIDL_PTRATTRHAN);

    /* [unique] attribute invalid on binding handle parameter */

    if (!AST_LOCAL_SET(int_p)
        &&  type_is_handle(type_p)
        &&  AST_UNIQUE_SET(param_p))
        CHECKER_error(param_p, NIDL_UNIQATTRHAN);

    /* handle_t first parameter must not have [transmit_as] type */

    if (!AST_LOCAL_SET(int_p)
        &&  type_p->kind == AST_handle_k
        &&  !AST_HANDLE_SET(type_p)
        &&  type_p->xmit_as_type != NULL)
        CHECKER_error(param_p, NIDL_HANXMITAS);

    /* Type with [transmit_as] cannot be used in defn of type with [handle] */

    if (AST_HANDLE_SET(type_p)
        &&  type_p->defined_as != NULL
        &&  FE_TEST(type_p->defined_as->fe_info->flags, FE_HAS_XMIT_AS))
    {
        ASTP_attr_flag_t attr1 = ASTP_TRANSMIT_AS;
        ASTP_attr_flag_t attr2 = ASTP_HANDLE;
        CHECKER_error(param_p, NIDL_TYPEATTRUSE,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)),
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr2)));
    }

    /* Type with [handle] cannot be used in defn of type with [transmit_as] */

    if (type_p->xmit_as_type != NULL
        &&  type_p->defined_as != NULL
        &&  AST_HANDLE_SET(type_p->defined_as))
    {
        ASTP_attr_flag_t attr1 = ASTP_HANDLE;
        ASTP_attr_flag_t attr2 = ASTP_TRANSMIT_AS;
        CHECKER_error(param_p, NIDL_TYPEATTRUSE,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)),
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr2)));
    }
}


/*
**  p a r a m _ c h e c k _ f i r s t
**
**  Checks an AST parameter node.  This set of checks applies only to
**  the first parameter in an operation.
*/

static void param_check_first
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    param_first_handle(param_p, int_p);
}


/*
**  p a r a m _ c h e c k _ n o n _ h a n d l e
**
**  Checks an AST parameter node.  This check applies only to parameters
**  in an operation that can not be handles.
*/

static void param_check_non_handle
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_type_n_t        *type_p;        /* Param type (deref'd if necess.) */

    /*
     * If the parameter type has a top-level '*' which indicates passing
     * mechanism only, follow the pointer to the data of interest.
     *      top_type_p  = top-level parameter type
     *      type_p      = possibly dereferenced parameter type
     */
    type_p = param_follow_ref_ptr(param_p, CHK_follow_ref);

    /* handle_t as other than first parameter requires [transmit_as] */

    if (!AST_LOCAL_SET(int_p)
        &&  type_p->kind == AST_handle_k
        &&  type_p->xmit_as_type == NULL)
        CHECKER_error(param_p, NIDL_HANFIRSTPRM);
}


/*
**  p a r a m _ c h e c k
**
**  Checks an AST parameter node.
*/

static void param_check
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_field_attr_n_t  *fattr_p;       /* Field attributes assoc. with param */
    AST_type_n_t        *top_type_p;    /* Top-level parameter type */
    AST_type_n_t        *type_p;        /* Param type (deref'd if necess.) */
    AST_type_n_t        *deref_type_p;  /* Param type (deref'd if necess.) */
	
    /*
     * If the parameter type has a top-level '*' which indicates passing
     * mechanism only, follow the pointer to the data of interest.
     *      top_type_p   = top-level parameter type
     *      type_p       = possibly dereferenced parameter type
     *      deref_type_p = possibly dereferenced parameter type
     */
    top_type_p   = param_p->type;
    type_p       = param_follow_ref_ptr(param_p, CHK_follow_ref);
    deref_type_p = param_follow_ref_ptr(param_p, CHK_follow_any);

    param_type(param_p, top_type_p, type_p, int_p);

    /* Check parameter's field attributes, if any. */

    fattr_p = param_p->field_attrs;
    if (fattr_p != NULL)
    {
        fattr_check(fattr_p, (ASTP_node_t *)param_p, type_p, int_p);
        fattr_param_check(fattr_p, param_p, int_p);
    }

	/* Interface must have a * */
	 if (type_p->kind == AST_interface_k)	{
		char const * id_name;
		 NAMETABLE_id_to_string(type_p->name, &id_name);
		 CHECKER_error(param_p, NIDL_INTREFNOTALO, id_name);
	 }
	 
    param_size(param_p, top_type_p, type_p, int_p);
    param_struct(param_p, top_type_p, type_p);
    param_pipe(param_p, top_type_p, deref_type_p);

    param_in_line(param_p, type_p);
    param_string(param_p, type_p);
    param_pointer(param_p, top_type_p, type_p, int_p);
    param_small(param_p, top_type_p, type_p);
    param_context(param_p, top_type_p, deref_type_p);
    param_varying(param_p, type_p, int_p);

    param_direction(param_p, top_type_p, type_p, int_p);
    param_comm_status(param_p, type_p);
    param_switch_is(param_p, type_p);

    CHK_param_cs(param_p, type_p);
}

/*
**  o p _ h a n d l e
**
**  Checks an operation node to see if a handle parameter is required.
*/

static void op_handle
(
    AST_operation_n_t   *op_p,          /* [in] Ptr to AST operation node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    /* No binding handle parameter for 'operation' - auto_handle assumed. */

    if (int_p->implicit_handle_name == NAMETABLE_NIL_ID
        &&  !AST_AUTO_HANDLE_SET(int_p)
        &&  !AST_LOCAL_SET(int_p)
		  &&  !AST_OBJECT_SET(int_p)
        &&  op_p->parameters == NULL)
        default_to_auto_handle(op_p, NIDL_DEFAUTOHAN);
}


/*
**  o p _ c o m m _ s t a t u s
**
**  Checks an operation node to make sure there is at most 1 comm_status param.
*/

static void op_comm_status
(
    AST_operation_n_t   *op_p           /* [in] Ptr to AST operation node */
)

{
    AST_parameter_n_t   *param_p;       /* A parameter in the operation */
    int     comm_status_count;          /* Number [comm_status] parameters */
    int     fault_status_count;         /* Number [fault_status] parameters */

    comm_status_count = 0;
    fault_status_count = 0;

    if (AST_COMM_STATUS_SET(op_p->result))
        comm_status_count++;
    if (AST_FAULT_STATUS_SET(op_p->result))
        fault_status_count++;

    for (param_p = op_p->parameters; param_p != NULL; param_p = param_p->next)
    {
        if (AST_COMM_STATUS_SET(param_p) || AST_ADD_COMM_STATUS_SET(param_p))
            comm_status_count++;
        if (AST_FAULT_STATUS_SET(param_p) || AST_ADD_FAULT_STATUS_SET(param_p))
            fault_status_count++;
    }

    /* Operation can have at most one [comm_status] parameter */

    if (comm_status_count > 1)
        CHECKER_acf_error(op_p, NIDL_STSATTRONCE,
            acf_keyword_lookup(COMM_STATUS_KW));

    /* Operation can have at most one [fault_status] parameter */

    if (fault_status_count > 1)
        CHECKER_acf_error(op_p, NIDL_STSATTRONCE,
            acf_keyword_lookup(FAULT_STATUS_KW));
}


/*
**  o p _ b r o a d c a s t
**
**  Checks an operation node's [broadcast] attribute.
*/

static void op_broadcast
(
    AST_operation_n_t   *op_p           /* [in] Ptr to AST operation node */
)

{
    /* Can't have [broadcast] attribute on operation with pipes */

    if (AST_BROADCAST_SET(op_p)
        &&  (AST_HAS_IN_PIPES_SET(op_p) || AST_HAS_OUT_PIPES_SET(op_p)))
        CHECKER_error(op_p, NIDL_BROADPIPE);
}


/*
**  o p _ m a y b e
**
**  Checks an operation node's [maybe] attribute.
*/

static void op_maybe
(
    AST_operation_n_t   *op_p           /* [in] Ptr to AST operation node */
)

{
    /* [maybe] operations cannot have [out] parameters */

    if (AST_MAYBE_SET(op_p)
        &&  (AST_HAS_OUTS_SET(op_p) || AST_HAS_OUT_PIPES_SET(op_p)))
        CHECKER_error(op_p, NIDL_MAYBEOUTPRM);
}


/*
**  o p _ c o d e
**
**  Checks an operation node's [code] and [nocode] attributes.
*/

static void op_code
(
    AST_operation_n_t   *op_p           /* [in] Ptr to AST operation node */
)

{
    /* Can't have both [code] and [nocode] operation attributes */

    if (AST_CODE_SET(op_p)
        &&  AST_NO_CODE_SET(op_p))
        CHECKER_acf_error(op_p, NIDL_OPCODEATTR);

    /* [nocode] attribute does not apply to server stub */

    if (AST_NO_CODE_SET(op_p)
        &&  cmd_opt[opt_emit_sstub]
        &&  !cmd_opt[opt_emit_cstub])
        CHECKER_acf_warning(op_p, NIDL_SRVNOCODE);
}


/*
**  o p _ i d e m p o t e n t
**
**  Checks an operation node's [idempotent] attribute.
*/

static void op_idempotent
(
    AST_operation_n_t   *op_p           /* [in] Ptr to AST operation node */
)

{
    /* Can't have [idempotent] attribute on operation with pipes */

    if (AST_IDEMPOTENT_SET(op_p)
        &&  !AST_BROADCAST_SET(op_p)    /* broadcast gens its own error */
        &&  (AST_HAS_IN_PIPES_SET(op_p) || AST_HAS_OUT_PIPES_SET(op_p)))
        CHECKER_error(op_p, NIDL_IDEMPIPE);
}


/*
**  o p _ e n c o d e
**
**  Checks an operation node's [encode] and [decode] attributes.
*/

static void op_encode
(
    AST_operation_n_t   *op_p           /* [in] Ptr to AST operation node */
)

{
    AST_parameter_n_t   *param_p;       /* A parameter in the operation */
    AST_parameter_n_t   *p1;            /* First parameter in the operation */
    boolean             all_in_out;     /* T => all parameters [in,out] */
    boolean             all_in;         /* T => all parameters [in]-only */
    boolean             all_out;        /* T => all parameters [out]-only */
    boolean             p1_is_handle_t; /* T => p1 non-NULL, type handle_t */

    if (AST_NO_CODE_SET(op_p))
        return;
    if (!AST_ENCODE_SET(op_p) && !AST_DECODE_SET(op_p))
        return;

    all_in_out = TRUE;
    all_in     = TRUE;
    all_out    = TRUE;
    p1 = op_p->parameters;
    if (p1 != NULL
        && (p1->type->kind == AST_handle_k
            || (p1->type->kind == AST_pointer_k
                && p1->type->type_structure.pointer->pointee_type->kind
                   == AST_handle_k)))
        p1_is_handle_t = TRUE;
    else
        p1_is_handle_t = FALSE;
    param_p = ((p1 != NULL) ? p1 : op_p->result);

    while (TRUE)
    {
        if (AST_IN_SET(param_p) && !AST_OUT_SET(param_p))
        {
            /* [in]-only (binding handle parameter excluded) */
            if ( !(param_p == p1 && p1_is_handle_t) )
            {
                all_out = FALSE;
                all_in_out = FALSE;
            }
        }
        else if (!AST_IN_SET(param_p) && AST_OUT_SET(param_p))
        {
            /* [out]-only (ACF-added [*_status] parameter excluded) */
            if (!AST_ADD_COMM_STATUS_SET(param_p)
                && !AST_ADD_FAULT_STATUS_SET(param_p))
            {
                all_in = FALSE;
                all_in_out = FALSE;
            }
        }
        else if (AST_IN_SET(param_p) && AST_OUT_SET(param_p))
        {
            /* [in,out] */
            all_in = FALSE;
            all_out = FALSE;
        }

        /* Operation with [encode] has [out]-only parameter P */
        /* Note an ACF-added [*_status] parameter is exception to this rule */
        if (AST_ENCODE_SET(op_p) && !AST_DECODE_SET(op_p)
            &&  AST_OUT_SET(param_p) && !AST_IN_SET(param_p)
            &&  !AST_ADD_COMM_STATUS_SET(param_p)
            &&  !AST_ADD_FAULT_STATUS_SET(param_p))
        {
            char const *id_name;
            NAMETABLE_id_to_string(param_p->name, &id_name);
            CHECKER_warning(param_p, NIDL_ENCOUTONLY, id_name);
        }

        /* Operation with [decode] has [in]-only parameter P */
        /* Note a handle_t first parameter is an exception to this rule */
        if (AST_DECODE_SET(op_p) && !AST_ENCODE_SET(op_p)
            &&  AST_IN_SET(param_p) && !AST_OUT_SET(param_p)
            &&  !(param_p == p1 && p1_is_handle_t) )
        {
            char const *id_name;
            NAMETABLE_id_to_string(param_p->name, &id_name);
            CHECKER_warning(param_p, NIDL_DECINONLY, id_name);
        }

        /* A [fault_status] parameter has no utility in this operation */
        if (AST_FAULT_STATUS_SET(param_p) || AST_ADD_FAULT_STATUS_SET(param_p))
            CHECKER_acf_warning(op_p, NIDL_NOFLTPARAM);

        /* Update to next parameter or operation result */
        if (param_p == op_p->result)
            break;
        param_p = param_p->next;
        if (param_p == NULL)
            param_p = op_p->result;
    }

    /* Operation with [encode] must have at least one [in] or [in,out] param */

    if (AST_ENCODE_SET(op_p) && !AST_DECODE_SET(op_p) && all_out)
        CHECKER_error(op_p, NIDL_ENCNOPARAMS);

    /* Operation with [decode] must have at least one [out] or [in,out] param */

    if (AST_DECODE_SET(op_p) && !AST_ENCODE_SET(op_p) && all_in)
        CHECKER_error(op_p, NIDL_DECNOPARAMS);

    /* Operation with [encode,decode] must have all [in,out] parameters */

    if (AST_ENCODE_SET(op_p) && AST_DECODE_SET(op_p) && !all_in_out)
        CHECKER_error(op_p, NIDL_ENCDECDIR);

    /* Operation with [encode] or [decode] may not contain pipe parameters */

    if (   (AST_ENCODE_SET(op_p) || AST_DECODE_SET(op_p))
        && (AST_HAS_IN_PIPES_SET(op_p) || AST_HAS_OUT_PIPES_SET(op_p)) )
        CHECKER_error(op_p, NIDL_ENCDECPIPE);

    /* Operation with [encode] or [decode] must use explicit binding */

    /*
     * Note: if explicit handle added via ACF attribute, propagator has
     * already added a handle_t parameter to the operation.
     */
    if (   (AST_ENCODE_SET(op_p) || AST_DECODE_SET(op_p))
        && !p1_is_handle_t )
        CHECKER_error(op_p, NIDL_ENCDECBIND);

    /* Use of [encode] attribute requires -standard extended */
    if (AST_ENCODE_SET(op_p)
        && (*(int *)cmd_val[opt_standard] < opt_standard_dce_1_1))
        CHECKER_acf_warning(op_p, NIDL_NOPORTATTR, "encode", OPT_STD_EXTENDED);

    /* Use of [decode] attribute requires -standard extended */
    if (AST_DECODE_SET(op_p)
        && (*(int *)cmd_val[opt_standard] < opt_standard_dce_1_1))
        CHECKER_acf_warning(op_p, NIDL_NOPORTATTR, "decode", OPT_STD_EXTENDED);
}


/*
**  o p e r a t i o n _ c h e c k
**
**  Checks an AST operation node.
*/

static void operation_check
(
    AST_operation_n_t   *op_p,          /* [in] Ptr to AST operation node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_parameter_n_t   *param_p;       /* A parameter in the operation */

    op_handle(op_p, int_p);
    op_comm_status(op_p);

    op_broadcast(op_p);
    op_maybe(op_p);
    op_code(op_p);
    op_idempotent(op_p);
    op_encode(op_p);
    CHK_op_cs(op_p);

    /* Must compile stubs with ANSI C to avoid promotion of float to double */

    if (FE_TEST(op_p->fe_info->flags, FE_HAS_VAL_FLOAT)
       )
    {
        char const *id_name;   /* Operation name */

        NAMETABLE_id_to_string(op_p->name, &id_name);
        CHECKER_warning(op_p, NIDL_FLOATPROM, id_name);
    }

    /* Operation with [reflect_deletions] has no [in] or [in,out] full ptrs */

    if (AST_REFLECT_DELETIONS_SET(op_p)
        &&  !FE_TEST(op_p->fe_info->flags, FE_HAS_IN_FULL_PTR))
        CHECKER_warning(op_p, NIDL_OPREFDELIN);

    /* Use of [reflect_deletions] attribute requires -standard extended */
    if (AST_REFLECT_DELETIONS_SET(op_p)
        && (*(int *)cmd_val[opt_standard] < opt_standard_dce_1_1))
    {
        ASTP_attr_flag_t attr1 = ASTP_REFLECT_DELETIONS;
        CHECKER_warning(op_p, NIDL_NOPORTATTR,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)),
            OPT_STD_EXTENDED);
    }

    /* Check each parameter in the operation. */

    param_p = op_p->parameters;
    if (param_p != NULL)
    {
        /* The first parameter in an operation is handled separately. */

        param_check_first(param_p, int_p);
        param_check(param_p, int_p);

        /* Check the remaining parameters, if any. */

        while ((param_p = param_p->next) != NULL)
        {
            param_check_non_handle(param_p, int_p);
            param_check(param_p, int_p);
        }
    }

    /* Check the operation result, if any. */

    if (op_p->result != NULL)
    {
        param_check_non_handle(op_p->result, int_p);
        param_check(op_p->result, int_p);
    }
}

/*
**  f i e l d _ t y p e
**
**  Checks a field node's data type.
*/

static void field_type
(
    AST_field_n_t       *field_p,       /* [in] Ptr to AST field node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_type_n_t        *type_p;        /* Data type of the field */

    type_p = field_p->type;

    /* If the field type is anonymous, it must undergo type checks. */

    if (type_is_anonymous(type_p))
        type_check(type_p, (ASTP_node_t *)field_p, int_p);

    /* A type with [transmit_as] may not have other type attributes */

    if (type_p->xmit_as_type != NULL
        &&
        (AST_STRING_SET(field_p)
         || AST_STRING0_SET(field_p)
         || AST_UNIQUE_SET(field_p)
         || AST_REF_SET(field_p)
         || AST_IGNORE_SET(field_p)
         || AST_SMALL_SET(field_p)
         || AST_CONTEXT_SET(field_p)
         || AST_PTR_SET(field_p)))
        CHECKER_error(field_p, NIDL_XMITTYPEATTRS);

    /* A field cannot be of presented type for which xmit type is conformant */

    if (type_p->xmit_as_type != NULL
        &&  AST_CONFORMANT_SET(type_p->xmit_as_type))
        CHECKER_error(field_p, NIDL_FLDXMITCFMT);

    type_p = type_xmit_type(type_p);    /* Pick up transmissible type */

    /* A conformant field must be the last field in structure */

    if (!AST_LOCAL_SET(int_p)
        &&  field_p->next != NULL
        &&  AST_CONFORMANT_SET(type_p)
        &&  type_p->kind != AST_pointer_k)  /* ptr does not make struct cfmt */
        CHECKER_error(field_p, NIDL_CFMTFLDLAST);

    /* Pipes not valid as structure fields */

    if (type_p->kind == AST_pipe_k)
        CHECKER_error(field_p, NIDL_PIPESTRFLD);

	/* interface must have a * */
	 if (type_p->kind == AST_interface_k)	{
		 char const * id_name;
		 NAMETABLE_id_to_string(type_p->name, &id_name);
		 CHECKER_error(field_p, NIDL_INTREFNOTALO, id_name);
	 }
	 
    /* Context handles not valid as structure fields */

    if (AST_CONTEXT_RD_SET(type_p)
        ||  AST_CONTEXT_SET(field_p))
        CHECKER_error(field_p, NIDL_CTXSTRFLD);

    /* Function pointers not valid as structure fields */

    if (!AST_LOCAL_SET(int_p)
        &&  type_is_function(type_p)
        &&  type_p->xmit_as_type == NULL /* allowed if void* is not xmited */
		  )
        CHECKER_error(field_p, NIDL_FPSTRFLD);

    /* Structure fields cannot be of type handle_t */

    if (type_p->kind == AST_handle_k)
#if 0   /** Obsolete **/
        &&  type_p->xmit_as_type == NULL)
#endif
        CHECKER_error(field_p, NIDL_HANSTRFLD);

    /* void is valid only in an operation or pointer declaration */

    if (type_p->kind == AST_void_k)
        CHECKER_error(field_p, NIDL_VOIDOPPTR);

    /* void * must be used in conjunction with the [context_handle] attr */

    if (!AST_LOCAL_SET(int_p)
        &&  type_p->kind == AST_pointer_k
        &&  type_p->type_structure.pointer->pointee_type->kind == AST_void_k
        &&  !AST_CONTEXT_RD_SET(type_p)
        &&  !AST_CONTEXT_SET(field_p)
		  )
        CHECKER_error(field_p, NIDL_PTRVOIDCTX);

    /*
     * V1 attributes are incompatible with this type
     * Note: Issue error only if the same error doesn't fire for the type.
     */
    if (FE_TEST(field_p->fe_info->flags, FE_HAS_V1_ATTR)
        &&  FE_TEST(field_p->fe_info->flags, FE_HAS_V2_ATTR)
        &&  ! ( FE_TEST(type_p->fe_info->flags, FE_HAS_V1_ATTR)
                &&  FE_TEST(type_p->fe_info->flags, FE_HAS_V2_ATTR) ))
        CHECKER_warning(field_p, NIDL_INCOMPATV1);
}


/*
**  f i e l d _ s i z e
**
**  Checks a field node to see if array size information is required.
*/

static void field_size
(
    AST_field_n_t       *field_p,       /* [in] Ptr to AST field node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_type_n_t        *type_p;        /* Data type of the field */
    AST_field_attr_n_t  *fattr_p;       /* Field attributes */

    type_p = field_p->type;
    fattr_p = field_p->field_attrs;

    /* Array size information required */

    if (!AST_LOCAL_SET(int_p)
        &&  type_p->kind != AST_structure_k /* Struct has its own size info */
        &&  type_p->xmit_as_type == NULL
        &&  fattr_p == NULL
        &&  (AST_CONFORMANT_SET(type_p)
            ||  AST_VARYING_SET(field_p))
        &&  !AST_STRING_SET(type_p)         /* [string] a possible exception */
        &&  !AST_STRING_SET(field_p))       /* if used in an [in] param      */
    {
        char const *id_name;

        NAMETABLE_id_to_string(field_p->name, &id_name);
        CHECKER_error(field_p, NIDL_ARRSIZEINFO, id_name);
    }

    /* A [string] array can not have varying array attributes */

    if (!AST_LOCAL_SET(int_p)
        &&  type_p->xmit_as_type == NULL
        &&  AST_STRING_SET(field_p)
        &&  fattr_p != NULL
        &&  (fattr_p->first_is_vec != NULL
            ||  fattr_p->last_is_vec != NULL
            ||  fattr_p->length_is_vec != NULL))
        CHECKER_error(field_p, NIDL_STRVARY);

    /* Arrays with [transmit_as] can't be conformant or varying */

    /*
     * Note that pointers that represent arrays are excluded from the test.
     * Since the instance of the pointer turns it into an array,
     * field_pointer calls array_check, which will detect the error.
     */
    if (type_p->xmit_as_type != NULL
        &&  fattr_p != NULL
        &&  type_p->kind != AST_pointer_k)
        CHECKER_error(field_p, NIDL_ARRXMITOPEN);

    /*
     * Current rules allow mixed open array [] and pointer * syntax ONLY
     * of the form *..*param[][N]..[N], i.e. when param is a [multidimensional]
     * array of pointers (that, currently, can be conformant or varying in the
     * first dimension only).  Pointers to open arrays are not allowed due to
     * the ambiguity as to which * or [] the array size attributes apply to.
     *
     * Builder only allows one open array dimension when pointer and array
     * syntax is mixed.  It will detect errors on such examples as:
     *      [in, size_is(s1,s2)] long (*param)[]
     *
     * A corollary is that in declarations with more than one * and no [] or
     * [N], array size information applies to the rightmost * only.
     * Builder will detect errors on such examples as:
     *      [in, size_is(s1,s2)] long **param
     *
     * Checker detects these 3 cases separately:
     *      [in, size_is(s)] long (*..*param)[N]..[N]       ARRSYNTAX
     *      [in, size_is(s)] long (*..*param)[][N]..[N]     PTRCFMTARR
     *      [in, length_is(l)] long (*..*param)[N]..[N]     PTRVARYARR
     */
    if (type_p->kind == AST_pointer_k)
    {
        AST_type_n_t    *ref_type_p;    /* Pointee data type */

        ref_type_p = ASTP_chase_ptr_to_type(type_p);

        /* Use array syntax to declare multidimensional arrays */

        if (ref_type_p->kind == AST_array_k
            &&  !AST_CONFORMANT_SET(ref_type_p)
            &&  fattr_p != NULL
            &&  (fattr_p->min_is_vec != NULL
                ||  fattr_p->max_is_vec != NULL
                ||  fattr_p->size_is_vec != NULL))
            CHECKER_error(field_p, NIDL_ARRSYNTAX);

        /* Pointers to conformant arrays are not allowed */

        /*
         * Can't just check for conformant set since a string can be conformant
         * and not require size attributes; a ptr to such a string is legal.
         */
        if (ref_type_p->kind == AST_array_k
            &&  AST_CONFORMANT_SET(ref_type_p)
            &&  (!AST_STRING_SET(ref_type_p)
                ||  (fattr_p != NULL
                    &&  (fattr_p->min_is_vec != NULL
                        ||  fattr_p->max_is_vec != NULL
                        ||  fattr_p->size_is_vec != NULL))))
        {
            CHECKER_error(field_p, NIDL_PTRCFMTARR);
            return;     /* Return with no further checking */
        }

        /* Pointers to varying arrays are not allowed */

        /*
         * Can't just check for varying set since a string is implicitly vary-
         * ing and cannot have length attrs; a ptr to such a string is legal.
         */
        if (ref_type_p->kind == AST_array_k
            &&  fattr_p != NULL
            &&  (fattr_p->first_is_vec != NULL
                ||  fattr_p->last_is_vec != NULL
                ||  fattr_p->length_is_vec != NULL))
        {
            CHECKER_error(field_p, NIDL_PTRVARYARR);
            return;     /* Return with no further checking */
        }
    }
}

/*
**  f i e l d _ i n _ l i n e
**
**  Checks a field node's [in_line] and [out_of_line] attributes.
*/

static void field_in_line
(
    AST_field_n_t       *field_p        /* [in] Ptr to AST field node */
)

{
    AST_type_n_t        *type_p;        /* Data type of the field */

    type_p = field_p->type;

    /* Can't have both [in_line] and [out_of_line] attributes */

    if (AST_IN_LINE_SET(field_p)
        &&  AST_OUT_OF_LINE_SET(field_p))
        CHECKER_acf_error(field_p, NIDL_CONFLINEATTR);

    /* [in_line] and [out_of_line] attributes apply only to non-scalar types */

    if ((AST_IN_LINE_SET(field_p) || AST_OUT_OF_LINE_SET(field_p))
        &&  type_is_scalar(type_p))
    {
        char const *file_name;     /* Related file name */
        char const *type_name;     /* Data type name */

        STRTAB_str_to_string(type_p->fe_info->file, &file_name);
        NAMETABLE_id_to_string(type_p->name, &type_name);

        CHECKER_acf_warning(field_p, NIDL_LINENONSCAL);
        CHECKER_acf_warning(field_p, NIDL_NAMEDECLAT, type_name, file_name,
                          type_p->fe_info->source_line);
    }
}


/*
**  f i e l d _ s t r i n g
**
**  Checks a field node's [string] and [v1_string] attributes.
*/

static void field_string
(
    AST_field_n_t       *field_p        /* [in] Ptr to AST field node */
)

{
    AST_type_n_t        *type_p;        /* Data type of the field */

    type_p = field_p->type;

    /* A [v1_string] must be an array of char with fixed bounds */

    if (AST_STRING0_SET(field_p)
        &&  !type_is_v1_string(type_p))
        CHECKER_error(field_p, NIDL_STRV1FIXED);

    /* The [v1_string] attribute can only be applied to a [v1_array] */

    if ((AST_STRING0_SET(field_p) || AST_STRING0_SET(type_p))
        &&  !(AST_SMALL_SET(field_p) || AST_SMALL_SET(type_p)))
        CHECKER_error(field_p, NIDL_STRV1ARRAY);

    /* The [string] attribute cannot be applied to a [v1_array] */

    if ((AST_STRING_SET(field_p) && AST_SMALL_SET(field_p))
        ||  (AST_STRING_SET(field_p) && AST_SMALL_SET(type_p))
        ||  (AST_STRING_SET(type_p) && AST_SMALL_SET(field_p)))
        CHECKER_error(field_p, NIDL_STRARRAYV1);

    /* Cannot have both [string] and [v1_string] attributes */

    if (    (AST_STRING_SET(field_p) || AST_STRING_SET(type_p))
        &&  (AST_STRING0_SET(field_p) || AST_STRING0_SET(type_p))   )
    {
        ASTP_attr_flag_t attr1 = ASTP_STRING;
        ASTP_attr_flag_t attr2 = ASTP_STRING0;

        CHECKER_error(field_p, NIDL_CONFLICTATTR,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)),
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr2)));
    }

    /*
     * Forgo last check if the declaration is a pointer to an array and either
     * (the declaration has size attributes) or (the array is conformant and
     * not a string), since mixed pointer and array declarations that either
     * have or require array size attributes are disallowed by other checks.
     */
    if (type_p->kind == AST_pointer_k)
    {
        AST_type_n_t    *ref_type_p;    /* Pointee data type */

        ref_type_p = ASTP_chase_ptr_to_type(type_p);
        if (ref_type_p->kind == AST_array_k
            &&  (field_p->field_attrs != NULL
                ||  (AST_CONFORMANT_SET(ref_type_p)
                    &&  !AST_STRING_SET(ref_type_p))))
            return;
    }

    /* [string] attribute valid only for one-dim arrays of char or byte */

    if (AST_STRING_SET(field_p)
        &&  !type_is_string(type_p))
        CHECKER_error(field_p, NIDL_STRCHARBYTE);
}


/*
**  f i e l d _ p o i n t e r
**
**  Checks a field node's attributes related to pointers.
*/

static void field_pointer
(
    AST_field_n_t       *field_p,       /* [in] Ptr to AST field node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_type_n_t        *type_p;        /* Data type of the field */
    boolean pointer_attr_valid = FALSE;

    type_p = field_p->type;

    if (type_p->kind == AST_pointer_k
        &&  type_p->type_structure.pointer->pointee_type->kind != AST_void_k)
        pointer_attr_valid = TRUE;

    /* [ref] attribute valid only for pointer or array types */

    if (AST_REF_SET(field_p) && !pointer_attr_valid)
        CHECKER_error(field_p, NIDL_REFATTRPTR);

    /* [unique] attribute valid only for pointer or array types */

    if (AST_UNIQUE_SET(field_p) && !pointer_attr_valid)
        CHECKER_error(field_p, NIDL_UNIQATTRPTR);

    /* [ptr] attribute valid only for pointer or array types */

    if (AST_PTR_SET(field_p) && !pointer_attr_valid)
        CHECKER_error(field_p, NIDL_PTRATTRPTR);

    /* [unique] attribute requires -standard extended */

    if (AST_UNIQUE_SET(field_p)
        &&  (*(int *)cmd_val[opt_standard] <= opt_standard_dce_1_0))
        CHECKER_warning(field_p, NIDL_NOPORTUNIQUE, OPT_STD_EXTENDED);

    /* An array with a pointer attribute is valid only as a parameter. */

    if (type_p->kind == AST_array_k &&
        (AST_REF_SET(field_p) || AST_PTR_SET(field_p) || AST_UNIQUE_SET(field_p)))
        CHECKER_error(field_p, NIDL_ARRPTRPRM);

	/* ignore REF, UNIQUE, PTR attributes on pointers to interfaces */
#if 0
	 if (type_p->kind == AST_pointer_k && type_p->type_structure.pointer->pointee_type->kind == AST_interface_k
			 && (AST_UNIQUE_SET(field_p) || AST_REF_SET(field_p) || AST_PTR_SET(field_p)))
		 CHECKER_warning(field_p, NIDL_PTRATTBIGN);
#endif
    /*
     * If the field is a pointer, and it is not a pointer to an array,
     * and it has any of the array attributes, then it is an arrayified
     * pointer and it must undergo the checks for arrays.
     */
    if (type_p->kind == AST_pointer_k
        &&  type_p->type_structure.pointer->pointee_type->kind != AST_array_k
#if 0
        &&  (field_p->field_attrs != NULL
#endif
        &&  (instance_has_array_attr(field_p)
            ||  AST_STRING_SET(type_p)
            ||  AST_STRING_SET(field_p)))
    {
        array_check((ASTP_node_t *)type_p->type_structure.pointer,
                    type_p,
                    (ASTP_node_t *)field_p,
                    type_p->type_structure.pointer->pointee_type,
                    int_p,
                    true);
    }
}


/*
**  f i e l d _ s m a l l
**
**  Checks a field node's [v1_array] attribute.
*/

static void field_small
(
    AST_field_n_t       *field_p        /* [in] Ptr to AST field node */
)

{
    AST_type_n_t        *type_p;        /* Data type of the field */
    AST_array_n_t       *array_p;       /* Ptr to array node */
    AST_field_attr_n_t  *fattr_p;       /* Ptr to field attribute node */

    type_p = field_p->type;

    /* A [v1_array] must be in array, not pointer, syntax */

    if (AST_SMALL_SET(field_p)
        &&  type_p->kind == AST_pointer_k)
        CHECKER_error(field_p, NIDL_SMALLARRSYN);

    /*
     * Remaining checks only apply to array types in array syntax.
     * V1 did not allow arrays in pointer syntax.
     */
    if (!type_is_array_np(type_p))
        return;

    fattr_p = field_p->field_attrs;
    array_p = type_p->type_structure.array;

    /* A [v1_array] can be conformant or varying in the first dimension only */

    if (AST_SMALL_SET(field_p)
        &&  ((AST_CONFORMANT_SET(type_p)
                && array_is_conformant_upper(array_p))
            ||
            (AST_VARYING_SET(field_p)
                && instance_is_varying_upper(array_p, field_p->field_attrs))))
        CHECKER_error(field_p, NIDL_SMALLMULTID);

    /* [v1_array] attribute invalid for array with more than 65535 elements */

    if (AST_SMALL_SET(field_p)
        &&  array_is_large(array_p))
        CHECKER_error(field_p, NIDL_SMALLINV);

    /* A conformant [v1_array] must also be varying */

    if ((AST_SMALL_SET(type_p) || AST_SMALL_SET(field_p))
        &&  AST_CONFORMANT_SET(type_p)
        &&  !AST_VARYING_SET(field_p))
        CHECKER_error(field_p, NIDL_SMALLCFMT);

    /* A [v1_array] can not have the [min_is] or [first_is] attributes */

    if ((AST_SMALL_SET(type_p) || AST_SMALL_SET(field_p))
        &&  fattr_p != NULL
        &&  (fattr_p->min_is_vec != NULL
            ||  fattr_p->first_is_vec != NULL))
        CHECKER_error(field_p, NIDL_SMALLMINFIRST);
}


/*
**  f i e l d _ c o n t e x t
**
**  Checks a field node's [context_handle] attribute.
*/

static void field_context
(
    AST_field_n_t       *field_p        /* [in] Ptr to AST field node */
)

{
    AST_type_n_t        *type_p;        /* Data type of the field */

    type_p = field_p->type;

    /* [context_handle] attribute only applies to void * types */

    if (AST_CONTEXT_SET(field_p)
        &&  (type_p->kind != AST_pointer_k
            ||  (type_p->kind == AST_pointer_k
                &&  type_p->type_structure.pointer->pointee_type->kind
                    != AST_void_k)))
        CHECKER_error(field_p, NIDL_CTXPTRVOID);

    /* [unique] pointers to context handles are not allowed */

    if (AST_UNIQUE_SET(field_p)
        &&  type_p->kind == AST_pointer_k
        &&  AST_CONTEXT_RD_SET(type_p->type_structure.pointer->pointee_type))
        CHECKER_error(field_p, NIDL_UNIQCTXHAN);

    /* Full pointers to context handles are not allowed */

    if (AST_PTR_SET(field_p)
        &&  type_p->kind == AST_pointer_k
        &&  AST_CONTEXT_RD_SET(type_p->type_structure.pointer->pointee_type))
        CHECKER_error(field_p, NIDL_PTRCTXHAN);
}


/*
**  f i e l d _ v a r y i n g
**
**  Checks an field node's "varying" (synthesized) attribute.
*/

static void field_varying
(
    AST_field_n_t       *field_p,       /* [in] Ptr to AST field node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_type_n_t        *type_p;        /* Field data type */

    type_p = field_p->type;

    /*
     * Assume that if the varying attribute is set, the field is an
     * array type and the field_attrs field is valid.  An array can be
     * represented either in normal array syntax, or as a pointer that
     * has any of the array attributes.
     */
    if (type_p->kind != AST_array_k)
        return;     /* All checks below assume normal array syntax */

    /* Arrays varying in other than the first dim require -standard extended */

    if (!AST_LOCAL_SET(int_p)
        &&  type_p->xmit_as_type == NULL
        &&  AST_VARYING_SET(field_p)
        &&  instance_is_varying_upper(type_p->type_structure.array,
                                      field_p->field_attrs)
        &&  (*(int *)cmd_val[opt_standard] <= opt_standard_dce_1_0))
        CHECKER_warning(field_p, NIDL_NOPORTVARY, OPT_STD_EXTENDED);
}


/*
**  f i e l d _ i g n o r e
**
**  Checks an field node's [ignore] attribute.
*/

static void field_ignore
(
    AST_field_n_t       *field_p        /* [in] Ptr to AST field node */
)

{
    AST_type_n_t        *type_p;        /* Field data type */

    type_p = field_p->type;

    /* The [ignore] attribute is valid only for pointers */

    if (AST_IGNORE_SET(field_p)
        &&  type_p->kind != AST_pointer_k)
        CHECKER_error(field_p, NIDL_IGNATTRPTR);
}
static void field_switch_is
(
    AST_field_n_t       *field_p        /* [in] Ptr to AST field node */
)

{
    AST_type_n_t        *type_p;        /* Field data type */

    type_p = field_p->type;

    if (type_p->kind == AST_pointer_k)
        type_p = ASTP_chase_ptr_to_kind(type_p, AST_disc_union_k);

    /* A non-encapsulated union declaration must have a [switch_is] attribute */

    if (type_p != NULL
        &&  type_p->kind == AST_disc_union_k
        &&  type_p->type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID
        &&  (field_p->field_attrs == NULL
            ||  field_p->field_attrs->switch_is == NULL))
        CHECKER_error(field_p, NIDL_NEUSWATTR);
}


/*
**  f i e l d _ c h e c k
**
**  Checks an AST field node.
*/

static void field_check
(
    AST_field_n_t       *field_p,       /* [in] Ptr to AST field node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_field_attr_n_t  *fattr_p;       /* Ptr to field attribute node */

    field_type(field_p, int_p);
    field_size(field_p, int_p);

    field_in_line(field_p);
    field_string(field_p);
    field_pointer(field_p, int_p);
    field_small(field_p);
    field_context(field_p);
    field_varying(field_p, int_p);
    field_ignore(field_p);
    field_switch_is(field_p);

    CHK_field_cs(field_p);

    /* If there is a field attribute node, check it. */

    fattr_p = field_p->field_attrs;
    if (fattr_p != NULL)
        fattr_check(fattr_p, (ASTP_node_t *)field_p, field_p->type, int_p);
}

/*
**  s t r u c t _ c h e c k
**
**  Checks an AST structure node.
*/

static void struct_check
(
    AST_structure_n_t   *struct_p,      /* [in] Ptr to AST structure node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_field_n_t       *field_p;       /* A field of the structure */

    /* Check each field node in the structure. */

    field_p = struct_p->fields;
    while (field_p != NULL)
    {
        field_check(field_p, int_p);
        field_p = field_p->next;
    }
}

/*
**  f p _ p a r a m _ h a n d l e
**
**  Checks a parameter node that is part of a function pointer operation
**  for any issues around handles.
*/

static void fp_param_handle
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_type_n_t        *type_p,        /* [in] Parameter type */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    type_p = type_xmit_type(type_p);    /* Pick up transmissible type */

    /* Function pointer parameters cannot be of type handle_t */

    if (!AST_LOCAL_SET(int_p)
        &&  type_p->kind == AST_handle_k)
        CHECKER_error(param_p, NIDL_FPHANPRM);

#if 0   /** This warning is disabled. **/
    /* [handle] attribute of function pointer parameter ignored */

    if (AST_HANDLE_SET(type_p))
        CHECKER_warning(param_p, NIDL_FPHANATTR);
#endif
}


/*
**  f p _ p a r a m _ c h e c k
**
**  Checks a parameter node that is part of a function pointer operation.
**  Many of these checks are identical to those for interface operations.
**  Compare this routine to param_check to see the differences.
*/

static void fp_param_check
(
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_field_attr_n_t  *fattr_p;       /* Field attributes assoc. with param */
    AST_type_n_t        *top_type_p;    /* Top-level parameter type */
    AST_type_n_t        *type_p;        /* Param type (deref'd if necess.) */
    AST_type_n_t        *deref_type_p;  /* Param type (deref'd if necess.) */

    /*
     * If the parameter type has a top-level '*' which indicates passing
     * mechanism only, follow the pointer to the data of interest.
     *      top_type_p   = top-level parameter type
     *      type_p       = possibly dereferenced parameter type
     *      deref_type_p = possibly dereferenced parameter type
     */
    top_type_p   = param_p->type;
    type_p       = param_follow_ref_ptr(param_p, CHK_follow_ref);
    deref_type_p = param_follow_ref_ptr(param_p, CHK_follow_any);

    param_type(param_p, top_type_p, type_p, int_p);

    /* Check parameter's field attributes, if any. */

    fattr_p = param_p->field_attrs;
    if (fattr_p != NULL)
        fattr_check(fattr_p, (ASTP_node_t *)param_p, param_p->type, int_p);

    param_size(param_p, top_type_p, type_p, int_p);
    param_struct(param_p, top_type_p, type_p);
    param_pipe(param_p, top_type_p, deref_type_p);

    param_string(param_p, type_p);
    param_pointer(param_p, top_type_p, type_p, int_p);
    param_small(param_p, top_type_p, type_p);
    param_context(param_p, top_type_p, deref_type_p);
    param_varying(param_p, type_p, int_p);

    fp_param_handle(param_p, type_p, int_p);
}


/*
**  f u n c t i o n _ p t r _ c h e c k
**
**  Checks an AST function pointer node.
*/

static void function_ptr_check
(
    AST_operation_n_t   *op_p,          /* [in] Ptr to operation node */
    AST_type_n_t        *type_p,        /* [in] Ptr to type node of pointer */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_parameter_n_t   *param_p;       /* A parameter in the operation */

/*
 * For now, assume that none of the operation attributes are allowed
 * on function pointer operations.
 */

    /* Function pointers are currently allowed only in local interfaces */

    if (!AST_LOCAL_SET(int_p)
        &&  type_p->xmit_as_type == NULL)
        CHECKER_error(op_p, NIDL_FPLOCINT);

    /* Check each parameter in the function. */

    for (param_p = op_p->parameters; param_p != NULL; param_p = param_p->next)
        fp_param_check(param_p, int_p);

    /* Check the function result, if any. */

    if (op_p->result != NULL)
        fp_param_check(op_p->result, int_p);
}

/*
**  p t r _ p o i n t e e _ t y p e
**
**  Checks a pointer node's pointee type.
*/

static void ptr_pointee_type
(
    AST_pointer_n_t     *ptr_p,         /* [in] Ptr to AST pointer node */
    AST_type_n_t        *ptr_type_p,    /* [in] Pointer type node */
    ASTP_node_t         *node_p,        /* [in] Parent node of ptr type node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_type_n_t        *type_p;        /* Pointee data type node */

    /*
     * Note: No need to pick up transmissible type - if a pointed-at type is
     * used in [transmit_as] it must be a named type, so type checking will do.
     */
    type_p = ptr_p->pointee_type;

    /*
     * If the pointer's parent has any array size attributes, then this
     * pointer represents an array - ignore the normal pointer checking.
     */
    if (    (node_p->fe_info->node_kind == fe_parameter_n_k
            &&  ((AST_parameter_n_t *)node_p)->field_attrs != NULL)
        ||  (node_p->fe_info->node_kind == fe_field_n_k
            &&  ((AST_field_n_t *)node_p)->field_attrs != NULL))
        return;

    /* Pointers to pipes valid only in parameter declarations */

    if (type_p->kind == AST_pipe_k
        &&  node_p->fe_info->node_kind != fe_parameter_n_k)
        CHECKER_error(ptr_p, NIDL_INVPTRPIPE);

    /* Pointers to context handles valid only in parameter declarations */

    if (AST_CONTEXT_RD_SET(type_p)
        &&  node_p->fe_info->node_kind != fe_parameter_n_k
            /* OK in opaque context handle typedef */
        &&  !(node_p->fe_info->node_kind == fe_export_n_k
              && type_p->kind == AST_structure_k
              && AST_DEF_AS_TAG_SET(type_p)))
        CHECKER_error(ptr_p, NIDL_INVPTRCTX);

    /* Pointers to type handle_t valid only in parameter declarations */

    if (type_p->kind == AST_handle_k
        &&  node_p->fe_info->node_kind != fe_parameter_n_k)
        CHECKER_error(ptr_p, NIDL_HANDLEPTR);

    /* void * must be used in conjunction with the [context_handle] attribute */
    /*
     * (1) Don't report error if parent node is parameter and [context_handle]
     * is set on the parameter.  This is one case where an attribute can apply
     * to other than the top-level * (e.g. in [out,context_handle] void **p,
     * the [context_handle] parameter attribute applies to the second-level *
     * which is an anonymous type without the [context_handle] attribute).
     */
    if (!AST_LOCAL_SET(int_p)
        &&  type_p->kind == AST_pointer_k
        &&  type_p->type_structure.pointer->pointee_type->kind == AST_void_k
        &&  !AST_CONTEXT_RD_SET(type_p)
        &&  !(node_p->fe_info->node_kind == fe_parameter_n_k /* (1) */
              && AST_CONTEXT_SET((AST_parameter_n_t *)node_p))
		  )
        CHECKER_error(node_p, NIDL_PTRVOIDCTX);

    /* If this is a function pointer, call routine to check it. */

    if (type_p->kind == AST_function_k)
        function_ptr_check(type_p->type_structure.function, ptr_type_p, int_p);
}


/*
**  p t r _ c h e c k
**
**  Checks an AST pointer node.
*/

static void ptr_check
(
    AST_pointer_n_t     *ptr_p,         /* [in] Ptr to AST pointer node */
    AST_type_n_t        *ptr_type_p,    /* [in] Pointer type node */
    ASTP_node_t         *node_p,        /* [in] Parent node of ptr type node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    ptr_pointee_type(ptr_p, ptr_type_p, node_p, int_p);
}

/*
**  p i p e _ b a s e _ t y p e
**
**  Checks a pipe node's base data type.
*/

static void pipe_base_type
(
    AST_pipe_n_t        *pipe_p,        /* [in] Ptr to AST pipe node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_type_n_t        *type_p;        /* Pipe base data type node */

    type_p = pipe_p->base_type;

    /* If the pipe base type is anonymous, it must undergo type checks. */

    if (type_is_anonymous(type_p))
        type_check(type_p, (ASTP_node_t *)pipe_p, int_p);

    /* Base type of a pipe can't be a pipe type */

    if (type_p->kind == AST_pipe_k)
        CHECKER_error(pipe_p, NIDL_PIPEBASETYP);

    /* Base type of a pipe can't be a [context_handle] type */

    if (AST_CONTEXT_RD_SET(type_p))
        CHECKER_error(pipe_p, NIDL_CTXBASETYP);

    /* Base type of a pipe can't be a conformant type */

    if (AST_CONFORMANT_SET(type_p))
        CHECKER_error(pipe_p, NIDL_CFMTBASETYP);

    /* Base type of a pipe cannot be handle_t */

    if (type_p->kind == AST_handle_k)
        CHECKER_error(pipe_p, NIDL_HANPIPEBASE);

    /* Base type of a pipe cannot be a function pointer */

    if (type_is_function(type_p))
        CHECKER_error(pipe_p, NIDL_FPPIPEBASE);

	/* Cant be an interface or interface reference */
	 if (type_p->kind == AST_interface_k)
		 CHECKER_error(pipe_p, NIDL_PIPECTYPE, "interface");
	if (type_p->kind == AST_pointer_k && type_p->type_structure.pointer->pointee_type->kind == AST_interface_k)
		 CHECKER_error(pipe_p, NIDL_PIPECTYPE, "interface reference");

	 
	 
    /* Base type of a pipe can't be or contain a pointer */

    if (!type_is_function(type_p)
        &&  FE_TEST(type_p->fe_info->flags, FE_HAS_PTR))
        CHECKER_error(pipe_p, NIDL_PTRBASETYP);

    /* Base type of a pipe may not have a [transmit_as] type */

    if (type_p->xmit_as_type != NULL)
        CHECKER_error(pipe_p, NIDL_XMITPIPEBASE);

    /* void is valid only in an operation or pointer declaration */

    if (type_p->kind == AST_void_k)
        CHECKER_error(pipe_p, NIDL_VOIDOPPTR);

    /* void * must be used in conjunction with the [context_handle] attribute */

    if (!AST_LOCAL_SET(int_p)
        &&  type_p->xmit_as_type == NULL
        &&  type_p->kind == AST_pointer_k
        &&  type_p->type_structure.pointer->pointee_type->kind == AST_void_k
        &&  !AST_CONTEXT_RD_SET(type_p)
		  )
        CHECKER_error(pipe_p, NIDL_PTRVOIDCTX);

    CHK_pipe_base_type_cs(pipe_p, int_p);
}


/*
**  p i p e _ c h e c k
**
**  Checks an AST pipe node.
*/

static void pipe_check
(
    AST_pipe_n_t        *pipe_p,        /* [in] Ptr to AST pipe node */
    AST_type_n_t        *type_p,        /* [in] Ptr to pipe data type node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    /* A pipe may not have a [transmit_as] type */

    if (type_p->xmit_as_type != NULL)
        CHECKER_error(pipe_p, NIDL_PIPEXMITAS);

    /* Pipes must be defined with typedef */

    if (type_p->name == NAMETABLE_NIL_ID)
        CHECKER_error(pipe_p, NIDL_ANONPIPE);

    pipe_base_type(pipe_p, int_p);
}

/*
**  e n u m _ c h e c k
**
**  Checks an AST enumeration node.
*/

static void enum_check
(
    AST_enumeration_n_t *enum_p,        /* [in] Ptr to AST enumeration node */
    AST_type_n_t        *type_p         /* [in] Ptr to enum data type node */
)

{
    /* Use of anonymous enum may not be portable across C compilers */
    if (type_is_anonymous(type_p))
        CHECKER_warning(enum_p, NIDL_NOPORTANON, "enum");
}

/*
**  c l a b e l _ v a l u e
**
**  Checks a case label node's constant value.
*/

static void clabel_value
(
    AST_case_label_n_t  *clabel_p,      /* [in] Ptr to AST case label node */
    AST_type_n_t        *type_p         /* [in] Union discriminator data type */
)

{
    AST_constant_n_t    *const_p;       /* Constant value in case label */

    /* Ignore if default case in union, which does not have a value. */

    if (clabel_p->default_label)
        return;

    const_p = clabel_p->value;

    /* Invalid case label type */

    if (!const_is_integer(const_p)
        &&  !const_is_enum(const_p)
        &&  const_p->kind != AST_boolean_const_k
        &&  const_p->kind != AST_char_const_k)
    {
        CHECKER_error(clabel_p, NIDL_INVCASETYP);
        return;
    }

    /* Case label type does not agree with discriminator type */

    if ((type_is_integer(type_p)
            &&  !const_is_integer(const_p))
        ||  (type_is_enum(type_p)
            &&  !const_is_enum(const_p))
        ||  (type_p->kind == AST_boolean_k
            &&  const_p->kind != AST_boolean_const_k)
        ||  (type_p->kind == AST_character_k
            &&  const_p->kind != AST_char_const_k))
    {
        CHECKER_error(clabel_p, NIDL_CASEDISCTYPE);
        return;
    }

    /* Case label must be a constant from discriminator's enumeration type */

    if (type_is_enum(type_p))
    {
        char const      *clabel_name;   /* Name of case label constant */
        char const      *econst_name;   /* Name of a constant in enumeration */
        AST_enumeration_n_t *enum_p;    /* Ptr to enumeration node */
        AST_constant_n_t    *econst_p;  /* Ptr to a constant node in the enum */

        NAMETABLE_id_to_string(const_p->name, &clabel_name);
        if (clabel_name == NULL)
            return;             /* Shouldn't happen */

        enum_p = type_p->type_structure.enumeration;

        econst_p = enum_p->enum_constants;
        while (econst_p != NULL)
        {
            NAMETABLE_id_to_string(econst_p->name, &econst_name);
            if (econst_name == NULL)
                continue;       /* Shouldn't happen */

            if (strcmp(clabel_name, econst_name) == 0)
                return;         /* Matching constant in right enum; return */

            econst_p = econst_p->next;
        }

        /* Matching constant name not found in the relevant enumeration. */

        CHECKER_error(clabel_p, NIDL_CASECONENUM);
    }
}


/*
**  c l a b e l _ c h e c k
**
**  Checks an AST case label node.
*/

static void clabel_check
(
    AST_case_label_n_t  *clabel_p,      /* [in] Ptr to AST case label node */
    AST_type_n_t        *type_p         /* [in] Union discriminator data type */
)

{
    clabel_value(clabel_p, type_p);
}

/*
**  a r m _ t y p e
**
**  Check an arm node's data type.
*/

static void arm_type
(
    AST_arm_n_t         *arm_p,         /* [in] Ptr to AST arm node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_type_n_t        *type_p;        /* Data type of the arm */

    type_p = arm_p->type;
    if (type_p == NULL)
        return;

    /* If the arm type is anonymous, it must undergo type checks. */

    if (type_is_anonymous(type_p))
        type_check(type_p, (ASTP_node_t *)arm_p, int_p);

    /* A type with [transmit_as] may not have other type attributes */

    if (type_p->xmit_as_type != NULL
        &&
        (AST_STRING_SET(arm_p)
         || AST_STRING0_SET(arm_p)
         || AST_UNIQUE_SET(arm_p)
         || AST_REF_SET(arm_p)
         || AST_SMALL_SET(arm_p)
         || AST_CONTEXT_SET(arm_p)
         || AST_PTR_SET(arm_p)))
        CHECKER_error(arm_p, NIDL_XMITTYPEATTRS);

    type_p = type_xmit_type(type_p);    /* Pick up transmissible type */

    /* Conformant arrays or structures are invalid within unions */
	/* WEZ:FIXME we need them for ORPC! */
#ifdef notdef
    if (AST_CONFORMANT_SET(type_p))
        CHECKER_error(arm_p, NIDL_CFMTUNION);
#endif

	/* interface must have a * */
	 if (type_p->kind == AST_interface_k)	{
		 char const * id_name;
		 NAMETABLE_id_to_string(type_p->name, &id_name);
		 CHECKER_error(arm_p, NIDL_INTREFNOTALO, id_name);
	 }
	 
    /* Pipes not valid as members of unions */

    if (type_p->kind == AST_pipe_k)
        CHECKER_error(arm_p, NIDL_PIPEUNIMEM);

    /* Context handles not valid as members of unions */

    if (AST_CONTEXT_RD_SET(type_p)
        ||  AST_CONTEXT_SET(arm_p))
        CHECKER_error(arm_p, NIDL_CTXUNIMEM);

    /* Function pointers not valid as members of unions */

    if (!AST_LOCAL_SET(int_p)
        &&  type_is_function(type_p))
#if 0   /** Obsolete **/
        &&  type_p->xmit_as_type == NULL)
#endif
        CHECKER_error(arm_p, NIDL_FPUNIMEM);

    /* Members of unions cannot be of type handle_t */

    if (type_p->kind == AST_handle_k)
#if 0   /** Obsolete **/
        &&  type_p->xmit_as_type == NULL)
#endif
        CHECKER_error(arm_p, NIDL_HANUNIMEM);

    /* void is valid only in an operation or pointer declaration */

    if (type_p->kind == AST_void_k)
        CHECKER_error(arm_p, NIDL_VOIDOPPTR);

    /* void * must be used in conjunction with the [context_handle] attribute */

    if (!AST_LOCAL_SET(int_p)
#if 0   /** Obsolete **/
        &&  type_p->xmit_as_type == NULL
#endif
        &&  type_p->kind == AST_pointer_k
        &&  type_p->type_structure.pointer->pointee_type->kind == AST_void_k
        &&  !AST_CONTEXT_RD_SET(type_p)
        &&  !AST_CONTEXT_SET(arm_p)
		  )
        CHECKER_error(arm_p, NIDL_PTRVOIDCTX);

    /*
     * V1 attributes are incompatible with this type
     * Note: Issue error only if the same error doesn't fire for the type.
     */
    if (FE_TEST(arm_p->fe_info->flags, FE_HAS_V1_ATTR)
        &&  FE_TEST(arm_p->fe_info->flags, FE_HAS_V2_ATTR)
        &&  ! ( FE_TEST(type_p->fe_info->flags, FE_HAS_V1_ATTR)
                &&  FE_TEST(type_p->fe_info->flags, FE_HAS_V2_ATTR) ))
        CHECKER_warning(arm_p, NIDL_INCOMPATV1);
}


/*
**  a r m _ s t r i n g
**
**  Checks an arm node's [string] and [v1_string] attributes.
*/

static void arm_string
(
    AST_arm_n_t         *arm_p          /* [in] Ptr to AST arm node */
)

{
    AST_type_n_t        *type_p;        /* Data type of the arm */

    type_p = arm_p->type;               /* Possibly NULL */

    /* Remaining checks do not apply to an empty arm. */

    if (type_p == NULL)
        return;

    /* A [v1_string] must be an array of char with fixed bounds */

    if (AST_STRING0_SET(arm_p)
        &&  !type_is_v1_string(type_p))
        CHECKER_error(arm_p, NIDL_STRV1FIXED);

    /* The [v1_string] attribute can only be applied to a [v1_array] */

    if ((AST_STRING0_SET(arm_p) || AST_STRING0_SET(type_p))
        &&  !(AST_SMALL_SET(arm_p) || AST_SMALL_SET(type_p)))
        CHECKER_error(arm_p, NIDL_STRV1ARRAY);

    /* The [string] attribute cannot be applied to a [v1_array] */

    if ((AST_STRING_SET(arm_p) && AST_SMALL_SET(arm_p))
        ||  (AST_STRING_SET(arm_p) && AST_SMALL_SET(type_p))
        ||  (AST_STRING_SET(type_p) && AST_SMALL_SET(arm_p)))
        CHECKER_error(arm_p, NIDL_STRARRAYV1);

    /* [string] attribute valid only for one-dim arrays of char or byte */

    if (AST_STRING_SET(arm_p)
        &&  !type_is_string(type_p))
        CHECKER_error(arm_p, NIDL_STRCHARBYTE);

    /* Cannot have both [string] and [v1_string] attributes */

    if (    (AST_STRING_SET(arm_p) || AST_STRING_SET(type_p))
        &&  (AST_STRING0_SET(arm_p) || AST_STRING0_SET(type_p))   )
    {
        ASTP_attr_flag_t attr1 = ASTP_STRING;
        ASTP_attr_flag_t attr2 = ASTP_STRING0;

        CHECKER_error(arm_p, NIDL_CONFLICTATTR,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)),
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr2)));
    }
}


/*
**  a r m _ p o i n t e r
**
**  Checks an arm node's attributes related to pointers.
*/

static void arm_pointer
(
    AST_arm_n_t         *arm_p,         /* [in] Ptr to AST arm node */
    AST_interface_n_t   *int_p ATTRIBUTE_UNUSED         /* [in] Ptr to interface node */
)

{
    AST_type_n_t        *type_p;        /* Data type of the arm */
    boolean pointer_attr_valid = FALSE;

    type_p = arm_p->type;
    if (type_p == NULL)
        return;

    type_p = type_xmit_type(type_p);    /* Pick up transmissible type */

    /* An arm of a union can't be or contain a [ref] pointer */
	/* WEZ:FIXME we need this for ORPC */
#ifdef nodef
    if (FE_TEST(type_p->fe_info->flags, FE_HAS_REF_PTR) ||
        AST_REF_SET(arm_p))
        CHECKER_error(arm_p, NIDL_ARMREFPTR);
#endif
    if (type_p->kind == AST_pointer_k
        &&  type_p->type_structure.pointer->pointee_type->kind != AST_void_k)
        pointer_attr_valid = TRUE;

    /* [ref] attribute valid only for pointer or array types */

    if (AST_REF_SET(arm_p) && !pointer_attr_valid)
        CHECKER_error(arm_p, NIDL_REFATTRPTR);

#if 0
	/* ignore REF, UNIQUE or PTR for pointers to interfaces */
	 if (type_p->kind == AST_pointer_k && type_p->type_structure.pointer->pointee_type->kind == AST_interface_k
			 && (AST_UNIQUE_SET(arm_p) || AST_REF_SET(arm_p) || AST_PTR_SET(arm_p)))
		 CHECKER_warning(arm_p, NIDL_PTRATTBIGN);
#endif
    /* An arm of a union can't be or contain a [unique] pointer */

#ifdef notdef
    if (FE_TEST(type_p->fe_info->flags, FE_HAS_UNIQUE_PTR) ||
        AST_UNIQUE_SET(arm_p))
        CHECKER_error(arm_p, NIDL_ARMUNIQUEPTR);
#endif

    /* [unique] attribute valid only for pointer or array types */

    if (AST_UNIQUE_SET(arm_p) && !pointer_attr_valid)
        CHECKER_error(arm_p, NIDL_UNIQATTRPTR);

    /* [ptr] attribute valid only for pointer or array types */

    if (AST_PTR_SET(arm_p) && !pointer_attr_valid)
        CHECKER_error(arm_p, NIDL_PTRATTRPTR);

    /* [unique] attribute requires -standard extended */

    if (AST_UNIQUE_SET(arm_p)
        &&  (*(int *)cmd_val[opt_standard] <= opt_standard_dce_1_0))
        CHECKER_warning(arm_p, NIDL_NOPORTUNIQUE, OPT_STD_EXTENDED);

    /* An array with a pointer attribute is valid only as a parameter. */

    /*
     * Have to check type attrs as well as arm attributes for the obscure case
     * of transmit_as type whose transmissible type is an array with a pointer
     * attr, since propagation from type to arm doesn't happen in this case.
     */
    if (type_p->kind == AST_array_k &&
        (AST_REF_SET(arm_p) || AST_PTR_SET(arm_p) || AST_UNIQUE_SET(arm_p) ||
         AST_REF_SET(type_p) || AST_PTR_SET(type_p) || AST_UNIQUE_SET(type_p)))
        CHECKER_error(arm_p, NIDL_ARRPTRPRM);

    /*
     * **NOTE**: It could be valid for an arm to be a varying array, however
     * the AST can not represent this since there is no arm_p->field_attrs
     * field.  If there were, an arm that is a pointer and has any of the
     * array attributes would undergo the checks for arrays here.
     */
}


/*
**  a r m _ s m a l l
**
**  Checks an arm node's [v1_array] attribute.
*/

static void arm_small
(
    AST_arm_n_t         *arm_p          /* [in] Ptr to AST arm node */
)

{
    AST_type_n_t        *type_p;        /* Data type of the arm */
    AST_array_n_t       *array_p;       /* Ptr to array node */

    type_p = arm_p->type;               /* Possibly NULL */

    /* Check only applies to array types. */

    if (type_p == NULL
        ||  !type_is_array_np(type_p))
        return;

    array_p = type_p->type_structure.array;

    /* [v1_array] attribute invalid for array with more than 65535 elements */

    if (AST_SMALL_SET(arm_p)
        &&  array_is_large(array_p))
        CHECKER_error(arm_p, NIDL_SMALLINV);
}


/*
**  a r m _ c o n t e x t
**
**  Checks an arm node's [context_handle] attribute.
*/

static void arm_context
(
    AST_arm_n_t         *arm_p          /* [in] Ptr to AST arm node */
)

{
    AST_type_n_t        *type_p;        /* Data type of the arm */

    type_p = arm_p->type;
    if (type_p == NULL)
        return;

    /* [context_handle] attribute only applies to void * types */

    if (AST_CONTEXT_SET(arm_p)
        &&  (type_p->kind != AST_pointer_k
            ||  (type_p->kind == AST_pointer_k
                &&  type_p->type_structure.pointer->pointee_type->kind
                    != AST_void_k)))
        CHECKER_error(arm_p, NIDL_CTXPTRVOID);

    /* [unique] pointers to context handles are not allowed */

    if (AST_UNIQUE_SET(arm_p)
        &&  type_p->kind == AST_pointer_k
        &&  AST_CONTEXT_RD_SET(type_p->type_structure.pointer->pointee_type))
        CHECKER_error(arm_p, NIDL_UNIQCTXHAN);

    /* Full pointers to context handles are not allowed */

    if (AST_PTR_SET(arm_p)
        &&  type_p->kind == AST_pointer_k
        &&  AST_CONTEXT_RD_SET(type_p->type_structure.pointer->pointee_type))
        CHECKER_error(arm_p, NIDL_PTRCTXHAN);
}


/*
**  a r m _ c h e c k
**
**  Checks an AST arm node.
*/

static void arm_check
(
    AST_arm_n_t         *arm_p,         /* [in] Ptr to AST arm node */
    AST_type_n_t        *type_p,        /* [in] Discriminator data type */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_case_label_n_t  *clabel_p;      /* List of case labels for this arm */

    arm_type(arm_p, int_p);

    arm_string(arm_p);
    arm_pointer(arm_p, int_p);
    arm_small(arm_p);
    arm_context(arm_p);

    /* Check each case label for this arm. */

    if (type_p == NULL) return;
    clabel_p = arm_p->labels;
    while (clabel_p != NULL)
    {
        clabel_check(clabel_p, type_p);
        clabel_p = clabel_p->next;
    }
}

/*
**  u n i o n _ d i s c r i m _ t y p e
**
**  Checks a discriminated union node's discriminator data type.
*/

static void union_discrim_type
(
    AST_disc_union_n_t  *union_p        /* [in] Ptr to AST discr. union node */
)

{
    AST_type_n_t        *type_p;        /* Discriminator data type */

    type_p = union_p->discrim_type;
    if (type_p == NULL) return;

    /* Union discriminator type must be int, char, boolean, or enum */

    if (!type_is_index(type_p)
        &&  !type_is_enum(type_p)
        &&  type_p->kind != AST_boolean_k
        &&  type_p->kind != AST_character_k)
        CHECKER_error(union_p, NIDL_UNIDISCTYP);
}


/*
**  u n i o n _ c a s e _ l a b e l s
**
**  Checks a discriminated union's case label values for uniqueness.
*/

static void union_case_labels
(
    AST_disc_union_n_t  *union_p        /* [in] Ptr to AST discr. union node */
)

{
    AST_arm_n_t         *arm_p;         /* Ptr to one arm node of the union */
    AST_case_label_n_t  *clabel_p;      /* Ptr to one case label of the arm */
    AST_arm_n_t         *s_arm_p;       /* Ptr to a subsequent arm */
    AST_case_label_n_t  *s_clabel_p;    /* Ptr to a subsequent case label */
    boolean             dup;            /* True if duplicate case label */

    arm_p = union_p->arms;
    while (arm_p != NULL)
    {
        clabel_p = arm_p->labels;
        while (clabel_p != NULL)
        {
            /*
             * Check all subsequent arms for a repeated case label value.
             * A repeated case label value within an arm is OK.
             */
            s_arm_p = arm_p->next;
            while (s_arm_p != NULL)
            {
                s_clabel_p = s_arm_p->labels;

                /* Go through all labels of this subsequent arm. */

                dup = FALSE;

                while (s_clabel_p != NULL)
                {
                    /* Check for duplicate "case default:" */

                    if (clabel_p->default_label && s_clabel_p->default_label)
                        dup = TRUE;
                    else if (clabel_p->value != NULL
                            && s_clabel_p->value != NULL
                            && clabel_p->value->kind == s_clabel_p->value->kind)
                    {
                        /* Check for duplicate case label values. */

                        switch (clabel_p->value->kind)
                        {
                        case AST_nil_const_k:
                            break;
                        case AST_int_const_k:
                            dup = (clabel_p->value->value.int_val
                                   == s_clabel_p->value->value.int_val);
                            break;
                        case AST_hyper_int_const_k:
                            dup = (clabel_p->value->value.hyper_int_val.high
                                   == s_clabel_p->value->value.hyper_int_val.high
                                  &&
                                   clabel_p->value->value.hyper_int_val.low
                                   == s_clabel_p->value->value.hyper_int_val.low);
                            break;
                        case AST_char_const_k:
                            dup = (clabel_p->value->value.char_val
                                   == s_clabel_p->value->value.char_val);
                            break;
                        case AST_string_const_k:
                            break;
                        case AST_boolean_const_k:
                            dup = (clabel_p->value->value.boolean_val
                                   == s_clabel_p->value->value.boolean_val);
                            break;
                        default:
                            error(NIDL_INTERNAL_ERROR, __FILE__, __LINE__);
                        }   /* switch (clabel_p->value->kind) */
                    }   /* else if (same constant kinds) */

                    /* Duplicate case label value invalid */

                    if (dup)
                    {
                        CHECKER_error(s_clabel_p, NIDL_DUPCASEVAL);
                        break;
                    }

                    s_clabel_p = s_clabel_p->next;
                }   /* while (s_clabel_p != NULL) */

                if (dup)
                    break;
                else
                    s_arm_p = s_arm_p->next;
            }   /* while (s_arm_p != NULL) */

            clabel_p = clabel_p->next;
        }   /* while (clabel_p != NULL) */

        arm_p = arm_p->next;
    }   /* while (arm_p != NULL) */
}


/*
**  u n i o n _ c h e c k
**
**  Checks an AST discriminated union node.
*/

static void union_check
(
    AST_disc_union_n_t  *union_p,       /* [in] Ptr to AST discr. union node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_arm_n_t         *arm_p;         /* Ptr to one arm node of the union */

    union_discrim_type(union_p);
    union_case_labels(union_p);

    /* Use of a nonencapsulated union requires -standard extended */

    if (union_p->discrim_name == NAMETABLE_NIL_ID
        &&  (*(int *)cmd_val[opt_standard] <= opt_standard_dce_1_0))
        CHECKER_warning(union_p, NIDL_NOPORTNEU, OPT_STD_EXTENDED);

    /* Check each arm of the union. */

    arm_p = union_p->arms;
    while (arm_p != NULL)
    {
        arm_check(arm_p, union_p->discrim_type, int_p);
        arm_p = arm_p->next;
    }
}

/*
**  t y p e _ n a m e _ l e n
**
**  Checks a type node's name length.
*/

static void type_name_len
(
    AST_type_n_t        *top_type_p,    /* [in] Top-level presented type */
    AST_type_n_t        *type_p,        /* [in] Ptr to AST type node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)
{
    char const          *type_name;     /* Type name */
    int                 type_len;       /* Length of type name */
    int                 max_len;        /* Maximum identifier length */

    if (!AST_LOCAL_SET(int_p)
        &&  top_type_p->name != NAMETABLE_NIL_ID
        &&  top_type_p->xmit_as_type != NULL)   /* Pres type != transmit type */
    {
        NAMETABLE_id_to_string(top_type_p->name, &type_name);
        if (type_name == NULL)
            return;     /* Shouldn't happen */

        type_len = strlen(type_name);

        /* Maximum id length for type with [transmit_as] is <n> characters */

        max_len = MAX_ID - strlen("_from_xmit");
        if (type_len > max_len)
            CHECKER_error(top_type_p, NIDL_MAXIDTYPTA, max_len);
    }

    if (type_p->name == NAMETABLE_NIL_ID)
        return;

    NAMETABLE_id_to_string(type_p->name, &type_name);
    if (type_name == NULL)
        return;     /* Shouldn't happen */

    type_len = strlen(type_name);

    /* Maximum identifier length for [handle] type is <n> characters */

    max_len = MAX_ID - strlen("_unbind");
    if (!AST_LOCAL_SET(int_p)
        &&  type_len > max_len
        &&  AST_HANDLE_SET(type_p))
        CHECKER_error(type_p, NIDL_MAXIDTYPHAN, max_len);

    /* Maximum identifier length for [context_handle] type is <n> characters */

    max_len = MAX_ID - strlen("_rundown");
    if (!AST_LOCAL_SET(int_p)
        &&  type_len > max_len
        &&  AST_CONTEXT_RD_SET(type_p))
        CHECKER_error(type_p, NIDL_MAXIDTYPCH, max_len);

    /* Maximum identifier length for pointed-to type is <n> characters */

    max_len = MAX_ID - strlen("_mr");
    if (!AST_LOCAL_SET(int_p)
        &&  !AST_IN_VARYING_SET(type_p) && !AST_OUT_VARYING_SET(type_p)
        &&  type_len > max_len
        &&  FE_TEST(type_p->fe_info->flags,FE_POINTED_AT))
    {
        char const *file_name;
        AST_type_n_t *ptr_type_p = type_p->fe_info->type_specific.pointer_type;

        CHECKER_error(type_p, NIDL_MAXIDTYPPT, max_len);
        /* Give location of pointer declaration that points at this type. */
        STRTAB_str_to_string(ptr_type_p->fe_info->file, &file_name);
        CHECKER_warning(type_p, NIDL_NAMEREFAT, file_name,
            ptr_type_p->fe_info->source_line);
    }

    max_len = MAX_ID - strlen("_mrV");
    if (!AST_LOCAL_SET(int_p)
        &&  (AST_IN_VARYING_SET(type_p) || AST_OUT_VARYING_SET(type_p))
        &&  type_len > max_len
        &&  FE_TEST(type_p->fe_info->flags,FE_POINTED_AT))
    {
        char const *file_name;
        AST_type_n_t *ptr_type_p = type_p->fe_info->type_specific.pointer_type;

        CHECKER_error(type_p, NIDL_MAXIDTYPPT, max_len);
        /* Give location of pointer declaration that points at this type. */
        STRTAB_str_to_string(ptr_type_p->fe_info->file, &file_name);
        CHECKER_warning(type_p, NIDL_NAMEREFAT, file_name,
            ptr_type_p->fe_info->source_line);
    }

    /* Maximum identifier length for pipe type is <n> characters */

    max_len = MAX_ID - strlen("_h");
    if (!AST_LOCAL_SET(int_p)
        &&  type_len > max_len
        &&  type_p->kind == AST_pipe_k)
        CHECKER_error(type_p, NIDL_MAXIDTYPPIPE, max_len);

    /* Maximum identifier length for [represent_as] type is <n> characters */

    max_len = MAX_ID - strlen("_from_local");
    if (!AST_LOCAL_SET(int_p)
        &&  type_len > max_len
        &&  type_p->rep_as_type != NULL)
        CHECKER_error(type_p, NIDL_MAXIDTYPRA, max_len);

    /* Maximum identifier length for [out_of_line] type is <n> characters */

    max_len = MAX_ID - strlen("Omr");
    if (!AST_LOCAL_SET(int_p)
        &&  !AST_IN_VARYING_SET(type_p) && !AST_OUT_VARYING_SET(type_p)
        &&  type_len > max_len
        &&  AST_OUT_OF_LINE_SET(type_p))
        CHECKER_error(type_p, NIDL_MAXIDTYPOOL, max_len);

    max_len = MAX_ID - strlen("OmrV");
    if (!AST_LOCAL_SET(int_p)
        &&  (AST_IN_VARYING_SET(type_p) || AST_OUT_VARYING_SET(type_p))
        &&  type_len > max_len
        &&  AST_OUT_OF_LINE_SET(type_p))
        CHECKER_error(type_p, NIDL_MAXIDTYPOOL, max_len);
}


/*
**  t y p e _ i n _ l i n e
**
**  Checks a type node's [in_line] and [out_of_line] attributes.
*/

static void type_in_line
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    /* Can't have both [in_line] and [out_of_line] type attributes */

    if (AST_IN_LINE_SET(type_p)
        &&  AST_OUT_OF_LINE_SET(type_p))
        CHECKER_acf_error(type_p, NIDL_TYPLINEATTR);

    /* [in_line] and [out_of_line] attributes apply only to non-scalar types */

    if ((AST_IN_LINE_SET(type_p) || AST_OUT_OF_LINE_SET(type_p))
        &&  type_is_scalar(type_p))
    {
        char const *file_name;     /* Related file name */
        char const *type_name;     /* Data type name */

        STRTAB_str_to_string(type_p->fe_info->file, &file_name);
        NAMETABLE_id_to_string(type_p->name, &type_name);

        CHECKER_acf_warning(type_p, NIDL_LINENONSCAL);
        CHECKER_acf_warning(type_p, NIDL_NAMEDECLAT, type_name, file_name,
                          type_p->fe_info->source_line);
    }
}


/*
**  t y p e _ s t r i n g
**
**  Checks a type node's [string] and [v1_string] attributes.
*/

static void type_string
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    /* A [v1_string] must be an array of char with fixed bounds */

    if (AST_STRING0_SET(type_p)
        &&  !type_is_v1_string(type_p))
        CHECKER_error(type_p, NIDL_STRV1FIXED);

    /* The [string] attribute cannot be applied to a [v1_array] */

    if (AST_STRING_SET(type_p) && AST_SMALL_SET(type_p))
        CHECKER_error(type_p, NIDL_STRARRAYV1);

    /* [string] attribute valid only for one-dim arrays of char or byte */

    if (AST_STRING_SET(type_p)
        &&  !type_is_string(type_p))
        CHECKER_error(type_p, NIDL_STRCHARBYTE);

    /* Cannot have both [string] and [v1_string] attributes */

    if (AST_STRING_SET(type_p)
        &&  AST_STRING0_SET(type_p))
    {
        ASTP_attr_flag_t attr1 = ASTP_STRING;
        ASTP_attr_flag_t attr2 = ASTP_STRING0;

        CHECKER_error(type_p, NIDL_CONFLICTATTR,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)),
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr2)));
    }
}


/*
**  t y p e _ p o i n t e r
**
**  Checks a type node's attributes related to pointers.
*/

static void type_pointer
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    boolean pointer_attr_valid = FALSE;

    if (type_p->kind == AST_array_k
        ||  (type_p->kind == AST_pointer_k
            &&  type_p->type_structure.pointer->pointee_type->kind
                != AST_void_k))
        pointer_attr_valid = TRUE;

    /* [ref] attribute valid only for pointer or array types */

    if (AST_REF_SET(type_p) && !pointer_attr_valid)
        CHECKER_error(type_p, NIDL_REFATTRPTR);

    /* [unique] attribute valid only for pointer or array types */

    if (AST_UNIQUE_SET(type_p) && !pointer_attr_valid)
        CHECKER_error(type_p, NIDL_UNIQATTRPTR);
#if 0
	/* ignore REF, UNIQUE or PTR for pointers to interfaces */
	 if (type_p->kind == AST_pointer_k && type_p->type_structure.pointer->pointee_type->kind == AST_interface_k
			 && (AST_UNIQUE_SET(type_p) || AST_REF_SET(type_p) || AST_PTR_SET(type_p)))
		 CHECKER_warning(type_p, NIDL_PTRATTBIGN);
#endif
    /* [ptr] attribute valid only for pointer or array types */

    if (AST_PTR_SET(type_p) && !pointer_attr_valid)
        CHECKER_error(type_p, NIDL_PTRATTRPTR);

    /* [unique] attribute requires -standard extended */

    if (AST_UNIQUE_SET(type_p)
        &&  (*(int *)cmd_val[opt_standard] <= opt_standard_dce_1_0))
        CHECKER_warning(type_p, NIDL_NOPORTUNIQUE, OPT_STD_EXTENDED);

    /* An array with a pointer attribute is valid only as a parameter. */

    if (FE_TEST(type_p->fe_info->flags, FE_HAS_PTR_ARRAY))
        CHECKER_error(type_p, NIDL_ARRPTRPRM);

    /* Cannot have more than one level of indirection to a ne union */

#if 0
    {
    AST_type_n_t *top_type_p = type_p;
    type_p = ASTP_chase_ptr_to_kind(type_p, AST_disc_union_k);
    if (type_p != NULL
        &&  type_p->kind == AST_disc_union_k
        &&  type_p->type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID
        &&  type_p->fe_info->pointer_count > 1)
        CHECKER_error(top_type_p, NIDL_PTRNEUNION);
    }
#endif
}


/*
**  t y p e _ s m a l l
**
**  Checks a type node's [v1_array] attribute.
*/

static void type_small
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    AST_array_n_t       *array_p;       /* Ptr to array node */

    /* A [v1_array] must be in array, not pointer, syntax */

    if (AST_SMALL_SET(type_p)
        &&  type_p->kind == AST_pointer_k)
        CHECKER_error(type_p, NIDL_SMALLARRSYN);

    /*
     * Remaining checks only apply to array types in array syntax.
     * V1 did not allow arrays in pointer syntax.
     */
    if (!type_is_array_np(type_p))
        return;

    array_p = type_p->type_structure.array;

    /* A [v1_array] can be conformant or varying in the first dimension only */

    if (AST_SMALL_SET(type_p)
        &&  AST_CONFORMANT_SET(type_p)
        &&  array_is_conformant_upper(array_p))
        CHECKER_error(type_p, NIDL_SMALLMULTID);

    /* [v1_array] attribute invalid for array with more than 65535 elements */

    if (AST_SMALL_SET(type_p)
        &&  array_is_large(array_p))
        CHECKER_error(type_p, NIDL_SMALLINV);

    /* A [v1_array] cannot have a conformant lower bound */

    if (AST_SMALL_SET(type_p)
        &&  AST_CONFORMANT_SET(type_p)
        &&  array_has_open_lb(array_p))
        CHECKER_error(type_p, NIDL_SMALLOPENLB);
}


/*
**  t y p e _ c o n t e x t
**
**  Checks a type node's [context_handle] and [handle] attributes.
*/

static void type_context
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    /* [context_handle] attribute only applies to void * types */

    if (AST_CONTEXT_RD_SET(type_p)
        &&  (type_p->kind != AST_pointer_k
            ||  (type_p->kind == AST_pointer_k
                &&  type_p->type_structure.pointer->pointee_type->kind
                    != AST_void_k
                    /* OK in opaque context handle typedef */
                &&  !(type_p->type_structure.pointer->pointee_type->kind
                      == AST_structure_k
                      && AST_DEF_AS_TAG_SET(type_p->type_structure.pointer->
                                            pointee_type)))))
        CHECKER_error(type_p, NIDL_CTXPTRVOID);

    /* [unique] pointers to context handles are not allowed */

    if (AST_UNIQUE_SET(type_p)
        &&  type_p->kind == AST_pointer_k
        &&  AST_CONTEXT_RD_SET(type_p->type_structure.pointer->pointee_type))
        CHECKER_error(type_p, NIDL_UNIQCTXHAN);

    /* Full pointers to context handles are not allowed */

    if (AST_PTR_SET(type_p)
        &&  type_p->kind == AST_pointer_k
        &&  AST_CONTEXT_RD_SET(type_p->type_structure.pointer->pointee_type))
        CHECKER_error(type_p, NIDL_PTRCTXHAN);

    /* Attribute [handle] cannot be applied to a void * type */

    if (AST_HANDLE_SET(type_p)
        && type_p->kind == AST_pointer_k
        && type_p->type_structure.pointer->pointee_type->kind == AST_void_k)
    {
        ASTP_attr_flag_t attr1 = ASTP_HANDLE;
        CHECKER_error(type_p, NIDL_ATTRPTRVOID,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)));
    }

    /* The [handle] attribute is valid only on transmittable types */

    if (AST_HANDLE_SET(type_p)
        && type_p->kind == AST_handle_k)
        CHECKER_error(type_p, NIDL_HANATTRTRAN);

    /* The attributes [handle] and [represent_as] cannot occur together */

    if (AST_HANDLE_SET(type_p)
        && type_p->rep_as_type != NULL)
    {
        ASTP_attr_flag_t attr1 = ASTP_HANDLE;
        CHECKER_error(type_p, NIDL_CONFLICTATTR,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)),
            "represent_as");
    }
}


/*
**  t y p e _ c o n f o r m a n t
**
**  Checks a type node's "conformant" (synthesized) attribute.
*/

static void type_conformant
(
    AST_type_n_t        *type_p,        /* [in] Ptr to AST type node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_array_n_t       *array_p;       /* Ptr to array node */

    /* Remaining check only applies to array types in array syntax. */

    if (!type_is_array_np(type_p))
        return;

    array_p = type_p->type_structure.array;

    /* Arrays conformant in other than first dim require -standard extended */

    if (!AST_LOCAL_SET(int_p)
        &&  AST_CONFORMANT_SET(type_p)
        &&  array_is_conformant_upper(array_p)
        &&  (*(int *)cmd_val[opt_standard] <= opt_standard_dce_1_0))
        CHECKER_warning(type_p, NIDL_NOPORTCFMT, OPT_STD_EXTENDED);
}


/*
**  t y p e _ i g n o r e
**
**  Checks a type node's [ignore] attribute.
*/

static void type_ignore
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    /* The [ignore] attribute is valid only for pointers */

    if (AST_IGNORE_SET(type_p)
        &&  type_p->kind != AST_pointer_k)
        CHECKER_error(type_p, NIDL_IGNATTRPTR);
}


/*
**  t y p e _ s w i t c h _ t y p e
**
**  Checks a type node's [switch_type] attribute.
*/

static void type_switch_type
(
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    /* A non-encapsulated union type must have a [switch_type] attribute */

    if (type_p->kind == AST_disc_union_k
        &&  type_p->type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID
        &&  type_p->type_structure.disc_union->discrim_type == NULL)
        CHECKER_error(type_p, NIDL_NEUSWTYPE);
}


/*
**  t y p e _ t r a n s m i t _ a s
**
**  Checks a type node's [transmit_as] attribute.
*/

static void type_transmit_as
(
    AST_type_n_t        *top_type_p,    /* [in] Top-level presented type */
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)

{
    if (top_type_p->xmit_as_type == NULL)   /* Presented type = transmit type */
        return;

    /*
     *  types with [transmit_as] can't be conformant not locally because
     *  of conversion routines and not on the net because of the net type is
     *  freed immediately after marshalling so there is no place to unmarshall
     *  the net type in to on the client side after the call.  This restriction
     *  could be lifted with enough programming.
     */

    /* Local [transmit_as] type can not be a conformant array */

    if (AST_CONFORMANT_SET(type_p)
        &&  !AST_STRING_SET(type_p)
        &&  type_is_array(type_p))
        CHECKER_error(top_type_p, NIDL_XMITCFMTARR);

    /* Net [transmit_as] type can not be a conformant array */

    if (AST_CONFORMANT_SET(top_type_p->xmit_as_type)
        &&  !AST_STRING_SET(top_type_p->xmit_as_type)
        &&  type_is_array(top_type_p->xmit_as_type))
        CHECKER_error(top_type_p, NIDL_XMITCFMTARR);


    /* Presented type can't be conformant */

    if (AST_CONFORMANT_SET(top_type_p)
        &&  top_type_p->kind == AST_structure_k)
        CHECKER_error(top_type_p, NIDL_STRUCTXMITCFMT);

    /* A translated transmissible type cannot contain pointers. */

    if (FE_TEST(type_p->fe_info->flags, FE_HAS_REF_PTR))
    {
        ASTP_attr_flag_t attr = ASTP_REF;

        CHECKER_error(type_p, NIDL_XMITPTR,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr)));
    }
    if (FE_TEST(type_p->fe_info->flags, FE_HAS_FULL_PTR))
    {
        ASTP_attr_flag_t attr = ASTP_PTR;

        CHECKER_error(type_p, NIDL_XMITPTR,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr)));
    }
    if (FE_TEST(type_p->fe_info->flags, FE_HAS_UNIQUE_PTR))
    {
        ASTP_attr_flag_t attr = ASTP_UNIQUE;

        CHECKER_error(type_p, NIDL_XMITPTR,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr)));
    }

    /* A type used in a transmit_as clause cannot have a represent_as type */

    if (type_p->rep_as_type != NULL)
        CHECKER_error(type_p, NIDL_XMITASREP);

    /* A type with [transmit_as] may not have other type attributes */

    if (AST_STRING_SET(top_type_p)
        ||  AST_STRING0_SET(top_type_p)
        ||  AST_UNIQUE_SET(top_type_p)
        ||  AST_REF_SET(top_type_p)
        ||  AST_IGNORE_SET(top_type_p)
        ||  AST_SMALL_SET(top_type_p)
        ||  AST_CONTEXT_RD_SET(top_type_p)
        ||  AST_PTR_SET(top_type_p))
        CHECKER_error(top_type_p, NIDL_XMITTYPEATTRS);

    /* A [transmit_as] type cannot be a non-encapsulated union */

    if (type_p->kind == AST_disc_union_k
        &&  type_p->type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID)
        CHECKER_error(top_type_p, NIDL_NEUXMITAS);

    /* A non-encapsulated union cannot have a [transmit_as] type */

    if (top_type_p->kind == AST_disc_union_k
     && top_type_p->type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID)
        CHECKER_error(top_type_p, NIDL_NEUXMITYPE);
}

/*
**  t y p e _ r e p r e s e n t _ a s
**
**  Checks a type node's [represent_as] attribute.
*/

static void type_represent_as
(
    AST_type_n_t        *type_p,        /* [in] Top-level presented type */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    if (type_p->rep_as_type == NULL)   /* Presented type = Net type */
        return;

    /*
     *  types with [represent_as] can't be conformant on the net because the
     *  net type is freed immediately after marshalling so there is no place to
     *  unmarshall the net type in to on the client side after the call.  This
     *  restriction could be lifted with enough programming.
     */

    /* Net type can not be conformant */

    if (AST_CONFORMANT_SET(type_p))
        CHECKER_error(type_p, NIDL_TYPEREPCFMT);

    /* A translated transmissible type cannot contain pointers. */

    if (FE_TEST(type_p->fe_info->flags, FE_HAS_REF_PTR))
    {
        ASTP_attr_flag_t attr = ASTP_REF;

        CHECKER_error(type_p, NIDL_XMITPTR,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr)));
    }
    if (FE_TEST(type_p->fe_info->flags, FE_HAS_FULL_PTR))
    {
        ASTP_attr_flag_t attr = ASTP_PTR;

        CHECKER_error(type_p, NIDL_XMITPTR,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr)));
    }
    if (FE_TEST(type_p->fe_info->flags, FE_HAS_UNIQUE_PTR))
    {
        ASTP_attr_flag_t attr = ASTP_UNIQUE;

        CHECKER_error(type_p, NIDL_XMITPTR,
            KEYWORDS_lookup_text(AST_attribute_to_token(&attr)));
    }

    /* ACF 'include' statement advised for definition of type 'name' */

    if (int_p->includes == NULL
        &&  ASTP_lookup_binding(type_p->rep_as_type->type_name,
                                fe_type_n_k, FALSE) == NULL)
    {
        char const*id_name;

        NAMETABLE_id_to_string(type_p->rep_as_type->type_name, &id_name);
        CHECKER_acf_warning(type_p, NIDL_INCLTYPE, id_name);
    }

    {
    AST_type_n_t *rep_type_p = (AST_type_n_t *) /* IDL rep_as type, if any */
        ASTP_lookup_binding(type_p->rep_as_type->type_name, fe_type_n_k, FALSE);

    /* A [represent_as] type cannot be a non-encapsulated union */

    if (rep_type_p != NULL
        &&  rep_type_p->kind == AST_disc_union_k
        &&  rep_type_p->type_structure.disc_union->discrim_name
            == NAMETABLE_NIL_ID)
        CHECKER_acf_error(type_p, NIDL_NEUREPAS);
    }

    /* A non-encapsulated union cannot have a [represent_as] type */

    if (type_p->kind == AST_disc_union_k
        &&  type_p->type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID)
        CHECKER_acf_error(type_p, NIDL_NEUREPTYPE);
}


/*
**  t y p e _ c h e c k
**
**  Checks an AST type node.
*/

static void type_check
(
    AST_type_n_t        *type_p,        /* [in] Ptr to AST type node */
    ASTP_node_t         *node_p,        /* [in] Parent node of type node */
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    AST_type_n_t        *xmit_type_p;   /* Transmissible type */
	char const * id_name;
    /*
     * Pick up the transmissible type (the top-level, presented, type is
     * different from the the transmissible type if it has a [transmit_as]).
     * Most checks are done on the transmissible, not presented, type.
     */
    xmit_type_p = type_xmit_type(type_p);

    if (!AST_LOCAL_SET(int_p))
        type_name_len(type_p, xmit_type_p, int_p);

    type_in_line(xmit_type_p);
    type_string(xmit_type_p);
    type_pointer(xmit_type_p);
    type_small(xmit_type_p);
    type_context(xmit_type_p);
    type_conformant(xmit_type_p, int_p);
    type_ignore(xmit_type_p);
    type_switch_type(xmit_type_p);
    type_transmit_as(type_p, xmit_type_p);
    type_represent_as(xmit_type_p, int_p);
    CHK_type_cs(type_p, xmit_type_p, int_p);

    /* V1 attributes are incompatible with this type */

    if (FE_TEST(type_p->fe_info->flags, FE_HAS_V1_ATTR)
        &&  FE_TEST(type_p->fe_info->flags, FE_HAS_V2_ATTR))
        CHECKER_warning(type_p, NIDL_INCOMPATV1);

    switch (type_p->kind)
    {
    case AST_handle_k:
    case AST_boolean_k:
    case AST_byte_k:
    case AST_character_k:
    case AST_small_integer_k:
    case AST_short_integer_k:
    case AST_long_integer_k:
    case AST_hyper_integer_k:
    case AST_small_unsigned_k:
    case AST_short_unsigned_k:
    case AST_long_unsigned_k:
    case AST_hyper_unsigned_k:
    case AST_short_float_k:
    case AST_long_float_k:
    case AST_void_k:
        break;      /* No checks for these types */

    case AST_enum_k:
        enum_check(type_p->type_structure.enumeration, type_p);
        break;

    case AST_array_k:
        array_check((ASTP_node_t *)type_p->type_structure.array,
                    type_p,
                    node_p,
                    type_p->type_structure.array->element_type,
                    int_p,
                    false);
        break;

    case AST_structure_k:
        struct_check(type_p->type_structure.structure, int_p);
        break;

    case AST_pipe_k:
        pipe_check(type_p->type_structure.pipe, type_p, int_p);
        break;

    case AST_pointer_k:
        ptr_check(type_p->type_structure.pointer, type_p, node_p, int_p);
        break;

    case AST_function_k:
        /* Function type declaration is not allowed */
        if (!AST_LOCAL_SET(int_p))
            CHECKER_error(type_p, NIDL_FUNTYPDCL);
        break;

    case AST_disc_union_k:
        union_check(type_p->type_structure.disc_union, int_p);
        break;

	case AST_interface_k:
		  /* Interface type declaration is not allowed */
		  NAMETABLE_id_to_string(type_p->name, &id_name);
		  CHECKER_error(type_p, NIDL_INTREFNOTALO, id_name);
		  break;
		  
    default:
        error(NIDL_INTERNAL_ERROR, __FILE__, __LINE__);
    }
}

/*
**  c o n s t a n t _ c h e c k
**
**  Checks an AST constant node.
*/

static void constant_check
(
    AST_constant_n_t    *const_p ATTRIBUTE_UNUSED       /* [in] Ptr to AST constant node */
)

{
}

/*
**  e x p o r t _ c h e c k
**
**  Checks an AST export node.
*/

static void export_check
(
    AST_export_n_t      *export_p,      /* [in] Ptr to AST export node */
    AST_interface_n_t   *int_p,         /* [in] Ptr to interface node */
    AST_interface_n_t   *parent_int_p   /* [in] Parent interface node */
)

{

    switch (export_p->kind)
    {
    case AST_cpp_quote_k:
        break;

    case AST_constant_k:
        constant_check(export_p->thing_p.exported_constant);
        break;

    case AST_operation_k:
        if (parent_int_p == NULL)   /* main interface */
            operation_check(export_p->thing_p.exported_operation, int_p);
        break;

    case AST_type_k:
        type_check(export_p->thing_p.exported_type, (ASTP_node_t *)export_p,
                   int_p);
        break;

    default:
        error(NIDL_INTERNAL_ERROR, __FILE__, __LINE__);
    }
}

/*
**  i n t _ n a m e _ l e n
**
**  Checks an interface node's name length.
*/

static void int_name_len
(
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)
{
    char const   *int_name;      /* Interface name */
    unsigned int max_len;        /* Maximum identifier length */

    NAMETABLE_id_to_string(int_p->name, &int_name);
    if (int_name == NULL)
        return;     /* Shouldn't happen */

    /* Maximum identifier length for interface name is <n> characters */

    max_len = MAX_ID - strlen("_v#_#_c_ifspec");    /* Could do better here */
    if (strlen(int_name) > max_len)
        CHECKER_error(int_p, NIDL_MAXIDINTF, max_len);
}


/*
**  i n t _ i n _ l i n e
**
**  Checks an interface node's [in_line] and [out_of_line] attributes.
*/

static void int_in_line
(
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    /* Can't have both [in_line] and [out_of_line] interface attributes */

    if (AST_IN_LINE_SET(int_p)
        &&  AST_OUT_OF_LINE_SET(int_p))
        CHECKER_acf_error(int_p, NIDL_INTLINEATTR);
}


/*
**  i n t _ c o d e
**
**  Checks an interface node's [code] and [nocode] attributes.
*/

static void int_code
(
    AST_interface_n_t   *int_p,         /* [in] Ptr to interface node */
    AST_interface_n_t   *parent_int_p   /* [in] Parent interface node */
)

{
    /* Can't have both [code] and [nocode] interface attributes */

    if (AST_CODE_SET(int_p)
        &&  AST_NO_CODE_SET(int_p))
        CHECKER_acf_error(int_p, NIDL_INTCODEATTR);

    /* [nocode] attribute does not apply to server stub */

    if (parent_int_p == NULL    /* main interface */
        &&  AST_NO_CODE_SET(int_p)
        &&  cmd_opt[opt_emit_sstub]
        &&  !cmd_opt[opt_emit_cstub])
        CHECKER_acf_warning(int_p, NIDL_SRVNOCODE);

    /* At least one operation should have the [code] attribute */

    if (parent_int_p == NULL    /* main interface */
        &&  int_p->op_count > 0)
    {
        AST_export_n_t *export_p;
        boolean code_op = FALSE;

        for (export_p = int_p->exports; export_p; export_p = export_p->next)
        {
            if (export_p->kind != AST_operation_k) continue;
            if (AST_CODE_SET(export_p->thing_p.exported_operation))
            {
                code_op = TRUE;
                break;
            }
        }
        if (!code_op)
            CHECKER_acf_warning(int_p, NIDL_NOCODEOPS);
    }
}


/*
**  i n t _ h a n d l e
**
**  Checks an interface node's attributes relating to handles/binding.
*/

static void int_handle
(
    AST_interface_n_t   *int_p          /* [in] Ptr to interface node */
)

{
    /* Can't have both [auto_handle] and [implicit_handle] interface attrs */

    if (int_p->implicit_handle_name != NAMETABLE_NIL_ID
        &&  AST_AUTO_HANDLE_SET(int_p))
        CHECKER_acf_error(int_p, NIDL_CONFHANATTR);

    /*
     * [implicit_handle] variable must either be of type handle_t
     * or have the [handle] type attribute
     */
    if (int_p->implicit_handle_name != NAMETABLE_NIL_ID
        &&  int_p->implicit_handle_type != NULL /* is an IDL-defined type */
        &&  !type_is_handle(int_p->implicit_handle_type))
        CHECKER_acf_error(int_p, NIDL_IMPHANVAR);
}


/*
**  i n t _ l o c a l
**
**  Checks an interface node's [local] and [uuid] attributes.
*/

static void int_local
(
    AST_interface_n_t   *int_p,         /* [in] Ptr to interface node */
    AST_interface_n_t   *parent_int_p   /* [in] Parent interface node */
)

{
    boolean             uuid_null;

    uuid_null = uuid_is_null(&int_p->uuid);

    /* [uuid] attribute invalid when [local] attribute is specified */

    if (AST_LOCAL_SET(int_p)
        &&  !uuid_null && !AST_OBJECT_SET(int_p))
        CHECKER_error(int_p, NIDL_UUIDINV);

    /*
     * If this is a non-local interface being imported by a local interface,
     * then pretend that it is a local interface as well.
     */
    if (!AST_LOCAL_SET(int_p)
        &&  parent_int_p != NULL
        &&  AST_LOCAL_SET(parent_int_p))
        AST_SET_LOCAL(int_p);

    /* Interface UUID must be specified */

    if (!AST_LOCAL_SET(int_p)
        &&  uuid_null
        &&  parent_int_p == NULL    /* main interface */
        &&  int_p->op_count != 0)
        CHECKER_error(int_p, NIDL_INTUUIDREQ);

    /* A non-local interface cannot import a local interface */

    if (parent_int_p != NULL
        &&  !AST_LOCAL_SET(parent_int_p)
        &&  AST_LOCAL_SET(int_p))
        CHECKER_error(int_p, NIDL_IMPORTLOCAL);
}


/* Checks an interface node's inheritance attributes */
static void int_inherit(AST_interface_n_t * int_p)
{
	char const * id_name;	/* name for [switch_is] */
	/* check to see if there is a binding for the inherited interface;
	 * if not, complain about it */
	if (int_p->inherited_interface_name != NAMETABLE_NIL_ID)	{
		/* WEZ:TODO only valid for ORPC */
		if (!NAMETABLE_lookup_local(int_p->inherited_interface_name))	{
			NAMETABLE_id_to_string(int_p->inherited_interface_name, &id_name);
			//CHECKER_error(int_p, NIDL_INHERITNOTDEF, id_name);
		}
	}
}

/*
**  i n t e r f a c e _ c h e c k
**
**  Checks the AST interface node.
*/

static void interface_check
(
    AST_interface_n_t   *int_p,         /* [in] Ptr to interface node */
    AST_interface_n_t   *parent_int_p   /* [in] Parent interface node */
)

{
    AST_export_n_t      *export_p;      /* Ptr to export node */
    AST_import_n_t      *import_p;      /* Ptr to import node */ 


    int_name_len(int_p);

    int_in_line(int_p);
    int_code(int_p, parent_int_p);
    int_handle(int_p);
    int_local(int_p, parent_int_p);
    int_inherit(int_p);
   	

    /* Check any interfaces that this interface imports. */

    import_p = int_p->imports;
    while (import_p != NULL)
    {
        if (import_p->interface != NULL)
            interface_check(import_p->interface, int_p);
        import_p = import_p->next;
    }

    /* Check everything that is exported by the interface. */

    export_p = int_p->exports;
    while (export_p != NULL)
    {
        export_check(export_p, int_p, parent_int_p);
        export_p = export_p->next;
    }

    /* For the main interface only, process the pointed-at types list. */

    if (parent_int_p == NULL)
    {
        AST_type_p_n_t  *typep_p;
        AST_type_n_t    *type_p;

        for (typep_p = int_p->pa_types
             ;  typep_p != NULL
             ;  typep_p = typep_p->next)
        {
            type_p = typep_p->type;

            /* Pointers to [v1_enum] types are not allowed */

            if (type_p->kind == AST_enum_k
                &&  AST_V1_ENUM_SET(type_p))
                CHECKER_error(type_p, NIDL_PTRV1ENUM);
        }

        /* Use of [exceptions] attribute requires -standard extended */
        if (int_p->exceptions != NULL
            && (*(int *)cmd_val[opt_standard] < opt_standard_dce_1_1))
            CHECKER_warning(int_p, NIDL_NOPORTATTR,
                            "exceptions", OPT_STD_EXTENDED);
    }
	if (AST_OBJECT_SET(int_p) && ASTP_IF_AF_SET(int_p, ASTP_IF_VERSION))	{
		CHECKER_warning(int_p, NIDL_CONFLICTATTR, "version", "object");
	}
	if (!AST_OBJECT_SET(int_p) && int_p->inherited_interface_name != NAMETABLE_NIL_ID)	{
		CHECKER_warning(int_p, NIDL_ANCREQSOBJ);
	}

}

/*
**  C H E C K E R _ m a i n
**
**  Main routine of semantic checking component of IDL compiler.
**  Does semantic checking of the interface definition (and any imported
**  items that are referred to in the interface).
*/

boolean CHECKER_main            /* Returns TRUE on success */
(
    boolean     *cmd_opt_arr,   /* [in] Array of command option flags */
    void        **cmd_val_arr,  /* [in] Array of command option values */
    AST_interface_n_t *int_p    /* [in] Ptr to AST interface node */
)

{
    /* Save passed command array addresses in static storage. */
    cmd_opt = cmd_opt_arr;
    cmd_val = cmd_val_arr;

    /*
     * Check the interface.  The interface node is the root of the AST.
     * Each function above checks an AST node that hangs off the main
     * interface node.
     */
    interface_check(int_p, NULL);

    /*
     * If we defaulted to auto_handle for any operations,
     * set [auto_handle] on the interface.
     */
    if (def_auto_handle > 0)
        AST_SET_AUTO_HANDLE(int_p);

    /* Return success if no errors, failure otherwise. */
    return (error_count == 0);
}

/*
**  C H E C K E R _ e r r o r
**
**  Handles any semantic errors detected.  Determines the source file and
**  line number of the error from the AST node; passes this information and
**  its remaining parameters onto the error logging routine.
**
**  Note:   This function is intentionally NOT prototyped.  This allows us a
**          poor man's way of passing a variable number of arguments to the
**          function.  This technique is only viable since the msgid argument
**          completely determines how many additional arguments are accessed,
**          a la printf.
*/

void CHECKER_error(
    void *node_p,        /* [in] Ptr to an AST node */
    long msgid,          /* [in] Message ID */
    ...
    )
{
    va_list ap;

    va_start(ap, msgid);

    log_source_va(
        &error_count,
        ((ASTP_node_t*) (node_p))->fe_info->file,
        ((ASTP_node_t*) (node_p))->fe_info->source_line,
        msgid,
        ap);

    va_end(ap);
}


/*
**  C H E C K E R _ w a r n i n g
**
**  Handles any semantic warnings detected.  Determines the source file and
**  line number of the warning from the AST node; passes this information and
**  its remaining parameters onto the warning logging routine.
**
**  Note:   This function is intentionally NOT prototyped.  This allows us a
**          poor man's way of passing a variable number of arguments to the
**          function.  This technique is only viable since the msgid argument
**          completely determines how many additional arguments are accessed,
**          a la printf.
*/

void CHECKER_warning(
    void *node_p,        /* [in] Ptr to an AST node */
    long msgid,          /* [in] Message ID */
    ...
    )
{
    va_list ap;

    va_start(ap, msgid);

    log_source_va(
        &warnings,
        ((ASTP_node_t*) (node_p))->fe_info->file,
        ((ASTP_node_t*) (node_p))->fe_info->source_line,
        msgid,
        ap);

    va_end(ap);
}

/*
**  C H E C K E R _ a c f _ e r r o r
**
**  Handles any semantic errors detected on ACF-related attributes.  Determines
**  the source file and line number of the error from the AST node; passes this
**  information and its remaining parameters onto the error logging routine.
**
**  Note:   This function is intentionally NOT prototyped.  This allows us a
**          poor man's way of passing a variable number of arguments to the
**          function.  This technique is only viable since the msgid argument
**          completely determines how many additional arguments are accessed,
**          a la printf.
*/

void CHECKER_acf_error(
    void *node_p,        /* [in] Ptr to an AST node */
    long msgid,          /* [in] Message ID */
    ...
    )
{
    va_list ap;

    va_start(ap, msgid);

    log_source_va(
        &error_count,
        ((ASTP_node_t*) (node_p))->fe_info->file,
        ((ASTP_node_t*) (node_p))->fe_info->source_line,
        msgid,
        ap);

    va_end(ap);
}


/*
**  C H E C K E R _ a c f _ w a r n i n g
**
**  Handles any semantic warnings detected on ACF-related attrs.  Determines
**  the source file and line number of warning from the AST node; passes this
**  information and its remaining parameters onto the warning logging routine.
**
**  Note:   This function is intentionally NOT prototyped.  This allows us a
**          poor man's way of passing a variable number of arguments to the
**          function.  This technique is only viable since the msgid argument
**          completely determines how many additional arguments are accessed,
**          a la printf.
*/

void CHECKER_acf_warning(
    void *node_p,        /* [in] Ptr to an AST node */
    long msgid,          /* [in] Message ID */
    ...
    )
{
    va_list ap;

    va_start(ap, msgid);

    log_source_va(
        &warnings,
        ((ASTP_node_t*) (node_p))->fe_info->file,
        ((ASTP_node_t*) (node_p))->fe_info->source_line,
        msgid,
        ap);

    va_end(ap);
}


/*
**  t y p e _ i s _ b a s e
**
**  Returns true if the specified type node is for a base type, false otherwise.
*/

boolean type_is_base
(
    AST_type_n_t *type_p    /* [in] Ptr to AST type node */
)
{
    return      ((type_p) == ASTP_char_ptr
        ||  (type_p) == ASTP_boolean_ptr
        ||  (type_p) == ASTP_byte_ptr
        ||  (type_p) == ASTP_void_ptr
        ||  (type_p) == ASTP_handle_ptr
        ||  (type_p) == ASTP_short_float_ptr
        ||  (type_p) == ASTP_long_float_ptr
        ||  (type_p) == ASTP_small_int_ptr
        ||  (type_p) == ASTP_short_int_ptr
        ||  (type_p) == ASTP_long_int_ptr
        ||  (type_p) == ASTP_hyper_int_ptr
        ||  (type_p) == ASTP_small_unsigned_ptr
        ||  (type_p) == ASTP_short_unsigned_ptr
        ||  (type_p) == ASTP_long_unsigned_ptr
        ||  (type_p) == ASTP_hyper_unsigned_ptr);
}
