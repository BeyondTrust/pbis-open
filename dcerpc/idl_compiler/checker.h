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
**      checker.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Declaration of functions exported by checker.c.
**
**  VERSION: DCE 1.0
**
*/

#ifndef CHECKERH_INCL
#define CHECKERH_INCL

#include <ast.h>                /* Abstract Syntax Tree defs */


/*
**  m i s c   m a c r o s
**
**  Miscellaneous macros.
*/

/* Evaluates to the text for the 'standard extended' command option */
#ifdef VMS
#define OPT_STD_EXTENDED \
(CMD_DCL_interface) ? "/STANDARD=EXTENDED" : "-standard extended"
#else
#define OPT_STD_EXTENDED "-standard extended"
#endif

/*
 * Get the last field in a structure.  The first field in a structure has a
 * pointer to the last field, unless it is the only field in the structure.
 */
#define struct_last_field(struct_p) \
    (((struct_p)->fields->last == NULL) ? \
        (struct_p)->fields : (struct_p)->fields->last)


/*
**  p r o p e r t y   m a c r o s
**
**  Property macros - determine whether a data structure has some property.
*/

#define uuid_is_null(uuid_p) \
    ((uuid_p)->time_low                     == 0 \
    &&  (uuid_p)->time_mid                  == 0 \
    &&  (uuid_p)->time_hi_and_version       == 0 \
    &&  (uuid_p)->clock_seq_hi_and_reserved == 0 \
    &&  (uuid_p)->clock_seq_low             == 0 \
    &&  (uuid_p)->node[0]                   == 0 \
    &&  (uuid_p)->node[1]                   == 0 \
    &&  (uuid_p)->node[2]                   == 0 \
    &&  (uuid_p)->node[3]                   == 0 \
    &&  (uuid_p)->node[4]                   == 0 \
    &&  (uuid_p)->node[5]                   == 0)


/*
**  t y p e   m a c r o s
**
**  Type "properties" used by other checks.
*/

#define type_is_handle(type_p) \
    ((type_p)->kind == AST_handle_k \
    ||  AST_HANDLE_SET(type_p))

#define param_is_handle(param_p) \
    ( ((param_p)->type->kind == AST_handle_k \
       || AST_HANDLE_SET((param_p)->type)) \
     || (AST_REF_SET(param_p) \
         && (param_p)->type->kind == AST_pointer_k \
         && ( (param_p)->type->type_structure.pointer->pointee_type->kind \
                  == AST_handle_k \
              || AST_HANDLE_SET( \
                  (param_p)->type->type_structure.pointer->pointee_type) ) ) )

#define type_is_integer(type_p) \
    ((type_p)->kind == AST_small_integer_k \
    ||  (type_p)->kind == AST_short_integer_k \
    ||  (type_p)->kind == AST_long_integer_k \
    ||  (type_p)->kind == AST_hyper_integer_k \
    ||  (type_p)->kind == AST_small_unsigned_k \
    ||  (type_p)->kind == AST_short_unsigned_k \
    ||  (type_p)->kind == AST_long_unsigned_k \
    ||  (type_p)->kind == AST_hyper_unsigned_k)

#define type_is_multibyte_integer(type_p) \
    ((type_p)->kind == AST_short_integer_k \
    ||  (type_p)->kind == AST_long_integer_k \
    ||  (type_p)->kind == AST_hyper_integer_k \
    ||  (type_p)->kind == AST_short_unsigned_k \
    ||  (type_p)->kind == AST_long_unsigned_k \
    ||  (type_p)->kind == AST_hyper_unsigned_k)

#define type_is_enum(type_p) \
    ((type_p)->kind == AST_enum_k)

#define type_is_float(type_p) \
    ((type_p)->kind == AST_short_float_k \
    ||  (type_p)->kind == AST_long_float_k)

#define type_is_scalar(type_p) \
    ((type_p)->kind == AST_boolean_k \
    ||  (type_p)->kind == AST_byte_k \
    ||  (type_p)->kind == AST_character_k \
    ||  type_is_integer(type_p) \
    ||  type_is_enum(type_p) \
    ||  type_is_float(type_p))

#define type_is_index(type_p) \
    (type_is_integer(type_p) \
      && ((type_p)->kind != AST_hyper_unsigned_k) \
      && ((type_p)->kind != AST_hyper_integer_k))

#define type_is_rangeable(type_p) \
    (type_is_scalar(type_p) \
      && !type_is_enum(type_p) \
      && ((type_p)->kind != AST_hyper_unsigned_k) \
      && ((type_p)->kind != AST_hyper_integer_k))

/*
 *  Notes on arrays: An array can be represented in array or pointer syntax.
 *  Array syntax is straightforward.  In general, whether a pointer type
 *  represents an array is not known at the type definition, only at the
 *  type instance.  The one exception to this is the [string] attribute,
 *  which "arrayifies" a pointer type ([v1_array] and [v1_string] are only
 *  allowed in the array syntax form, so they can't arrayify a pointer).
 *  An instance of a pointer type represents an array if it has any of the
 *  array size attributes, or if it has the [string] attribute.
 */
#define type_is_array(type_p) \
    ((type_p)->kind == AST_array_k \
    ||  ((type_p)->kind == AST_pointer_k && AST_STRING_SET(type_p)))

#define type_is_array_np(type_p) \
    ((type_p)->kind == AST_array_k)

#define type_is_array_1dim(type_p) \
    (   ((type_p)->kind == AST_array_k \
        &&  (type_p)->type_structure.array->index_count == 1) \
    ||  ((type_p)->kind == AST_pointer_k && AST_STRING_SET(type_p)))


#define type_is_anonymous(type_p) \
    ((type_p)->name == NAMETABLE_NIL_ID \
    &&  !type_is_base(type_p))

#define type_is_function(type_p) \
    ((type_p)->kind == AST_pointer_k \
    &&  (type_p)->type_structure.pointer->pointee_type->kind \
        == AST_function_k)


/*
**  i n s t a n c e   m a c r o s
**
**  Instance-of-type "properties" used by other checks.
*/

#if 0
#define instance_is_array(type_p, fattr_p, flags) \
    ((type_p)->kind == AST_array_k \
    ||  ((type_p)->kind == AST_pointer_k \
        &&  (AST_STRING_SET(type_p) \
            ||  fattr_p != NULL \
            ||  (AST_STRING & flags))))
#endif

#define instance_is_array(type_p, fattr_p, flags) \
    ((type_p)->kind == AST_array_k \
    ||  ((type_p)->kind == AST_pointer_k \
        &&  (AST_STRING_SET(type_p) \
            || (fattr_p != NULL && \
                 (fattr_p->max_is_vec != NULL || fattr_p->size_is_vec != NULL))\
            || (AST_STRING & flags))))

#define instance_has_array_attr(inst_p) \
    ((inst_p)->field_attrs != NULL \
     && ((inst_p)->field_attrs->min_is_vec != NULL \
         || (inst_p)->field_attrs->max_is_vec != NULL \
         || (inst_p)->field_attrs->size_is_vec != NULL \
         || (inst_p)->field_attrs->first_is_vec != NULL \
         || (inst_p)->field_attrs->last_is_vec != NULL \
         || (inst_p)->field_attrs->length_is_vec != NULL))


/*
**  c o n s t a n t   m a c r o s
**
**  Constant "properties" used by other checks.
*/

#define const_is_integer(const_p) \
        ((const_p)->kind == AST_int_const_k \
        &&  (const_p)->fe_info->type_specific.const_kind == fe_int_const_k)

#define const_is_enum(const_p) \
        ((const_p)->kind == AST_int_const_k \
        &&  (const_p)->fe_info->type_specific.const_kind == fe_enum_const_k)



/*
**  C H K _ s t r u c t _ i s _ a l l _ b y t e _ f i e l d s
**
**  Returns TRUE if a struct consists only of scalar byte fields.
*/

extern boolean CHK_struct_is_all_byte_fields(
    AST_structure_n_t   *struct_p       /* [in] Ptr to AST structure node */
);


/*
**  p a r a m _ f o l l o w _ r e f _ p t r
**
**  Dereferences a parameter's type if it is a pointer and it meets certain
**  conditions.  Some of the conditions apply always; others are dependent
**  on the 'mode' argument (see below).
**
**  The general intent of this function is to follow reference pointers.
**  However, there are ambiguities as to what that really means, and
**  different dereferencing behavior is required depending on the need.
**  Thus, the 'mode' argument allows one to tweak what this routine does.
**
**  Returns address of pointee type node if the type was dereferenced.
**  Returns address of parameter's type node if the type was NOT dereferenced.
*/

/*
 * Enum which dictates the behavior of param_follow_ref_ptr.
 *
 * In NO cases does it follow:
 *  - a pointer function result parameter, which by def can't be a ref ptr
 *  - void *, since "void passed-by-reference" makes no sense
 *  - function pointer
 *  - arrayified [string] pointers (but not pointer to string array)
 */
typedef enum
{
    CHK_follow_ref,     /* Follow [ref] but not [unique] or [ptr] pointers   */
                        /* Do not follow pointers arrayified via size attrs  */
                        /* Do not follow pointers arrayified via string attr */
                        /* DO follow named pointers                          */
CHK_follow_ref_arr_siz, /* Follow [ref] but not [unique] or [ptr] pointers   */
                        /* DO follow pointers arrayified via size attrs      */
                        /* Do not follow pointers arrayified via string attr */
                        /* DO follow named pointers                          */
    CHK_follow_any      /* Follow [ref], [unique], or [ptr] pointers         */
                        /* Do not follow pointers arrayified via size attrs  */
                        /* Do not follow pointers arrayified via string attr */
                        /* Do not follow named pointers                      */
} CHK_follow_t;

extern AST_type_n_t * param_follow_ref_ptr( /* Returns ptr to type node */
    AST_parameter_n_t   *param_p,       /* [in] Ptr to AST parameter node */
    CHK_follow_t        mode            /* [in] Follow mode (see above) */
);


/*
**  t y p e _ i s _ b a s e
**
**  Returns true if the specified type node is for a base type, false otherwise.
*/

extern boolean type_is_base(
    AST_type_n_t *type_p    /* [in] Ptr to AST type node */
);


/*
**  C H E C K E R _ m a i n
**
**  Main routine of semantic checking component of IDL compiler.
**  Does semantic checking of the interface definition (and any imported
**  items that are referred to in the interface).
*/

extern boolean CHECKER_main(    /* Returns true on success */
    boolean     *cmd_opt_arr,   /* [in] Array of command option flags */
    void        **cmd_val_arr,  /* [in] Array of command option values */
    AST_interface_n_t *int_p    /* [in] Ptr to AST interface node */
);

extern void CHECKER_error(
    void* node_p,        /* [in] Ptr to an AST node */
    long  msgid,          /* [in] Message ID */
    ...
    );
extern void CHECKER_warning(
    void* node_p,        /* [in] Ptr to an AST node */
    long  msgid,          /* [in] Message ID */
    ...
    );
extern void CHECKER_acf_error(
    void* node_p,        /* [in] Ptr to an AST node */
    long  msgid,          /* [in] Message ID */
    ...
    );
extern void CHECKER_acf_warning(
    void* node_p,        /* [in] Ptr to an AST node */
    long  msgid,          /* [in] Message ID */
    ...
    );

#endif
