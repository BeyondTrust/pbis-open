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
**  NAME
**
**      ASTP.H
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Header file for the ASTP.C
**
**  VERSION: DCE 1.0
**
*/

#ifndef ASTP_H
#define ASTP_H

#include <nidl.h>
#include <ast.h>

/* NDR Data Sizes for the Transmissable Types */
#define NDR_C_BOOLEAN_SIZE      1
#define NDR_C_BYTE_SIZE         1
#define NDR_C_CHARACTER_SIZE    1
#define NDR_C_SMALL_INT_SIZE    1
#define NDR_C_SHORT_INT_SIZE    2
#define NDR_C_LONG_INT_SIZE     4
#define NDR_C_HYPER_INT_SIZE    8
#define NDR_C_SHORT_FLOAT_SIZE  4
#define NDR_C_LONG_FLOAT_SIZE   8
#define NDR_C_POINTER_SIZE      4

/* MIN/MAX values for various integer sizes */
#define ASTP_C_SMALL_MAX        127
#define ASTP_C_SMALL_MIN        -128
#define ASTP_C_USMALL_MAX       255
#define ASTP_C_USMALL_MIN       0

#define ASTP_C_SHORT_MAX        32767
#define ASTP_C_SHORT_MIN        -32768
#define ASTP_C_USHORT_MAX       65535
#define ASTP_C_USHORT_MIN       0

#define ASTP_C_LONG_MAX         2147483647
#define ASTP_C_LONG_MIN         (-2147483647 - 1)
#define ASTP_C_ULONG_MAX        4294967295.0
#define ASTP_C_ULONG_MIN        0


/*
 *  Boolean Attributes Flags for Operations, Parameters, Types and Fields
 *
 *  broadcast,          - operation
 *  maybe,              - operation
 *  idempotent,         - operation
 *  reflect_deletions,  - operation
 *  local               - operation
 *  in,                 - parameter
 *  out,                - parameter
 *  mutable,            - parameter
 *  context,            - parameter, type
 *  handle,             - type
 *  ignore,             - field, parameter, type
 *  ref,                - field, parameter, type
 *  small,              - field, parameter, type
 *  string,             - field, parameter, type
 *  string0,            - field, parameter, type
 *
 *  Boolean Attributes Flags for an Interface.
 *  Separated from the above attributes since
 *  their life time is the life of the interface.
 *
 *  npb         (non-parametric handle)
 *  local
 */

/*
 * ASTP (Private) definitions for the boolean attributes for operations,
 * parameters, fields and types.  Theses differ from the common definitions
 * defined in AST.H, since we don't need to create a separate operation
 * flags word.
 */

/* Operation Attributes */
#define ASTP_BROADCAST      0x00000001
#define ASTP_MAYBE          0x00000002
#define ASTP_IDEMPOTENT     0x00000004
#define ASTP_LOCAL			 0x08000000
#define ASTP_REFLECT_DELETIONS 0x04000000

/* Parameter-only Attributes */
#define ASTP_IN             0x00000008
#define ASTP_OUT            0x00000010
#define ASTP_IN_SHAPE       0x00000020
#define ASTP_OUT_SHAPE      0x00000040
#define ASTP_PTR            0x00000080

/* Type, Field, Parameter Attributes */
#define ASTP_STRING0        0x00000100
#define ASTP_STRING         0x00000200
#define ASTP_IGNORE         0x00000400
#define ASTP_SMALL          0x00000800
#define ASTP_CONTEXT        0x00001000

/* Type-only Attribute(s) */
#define ASTP_REF            0x00002000
#define ASTP_UNIQUE         0x00004000
#define ASTP_HANDLE         0x00008000
#define ASTP_UNALIGN        0x00010000
#define ASTP_TRANSMIT_AS    0x00020000
#define ASTP_ALIGN_SMALL    0x00040000
#define ASTP_ALIGN_SHORT    0x00080000
#define ASTP_ALIGN_LONG     0x00100000
#define ASTP_ALIGN_HYPER    0x00200000
#define ASTP_V1_ENUM        0x00400000
#define ASTP_SWITCH_TYPE    0x00800000

/* Arm-only Attribute(s) */
#define ASTP_CASE           0x01000000
#define ASTP_DEFAULT        0x02000000

/* MIDL-only Attribute(s) */
#define ASTP_RANGE          0x10000000

/*
 * NOTE: This bit must correspond to the Highest Attribute bit used
 * above.  It is used to check which attributes are applicable.
 */
#define ASTP_MAX_ATTRIBUTE  0x10000000

/*
 * Sets of valid flags for each node type.  Note, these are the flags that can
 * potentially be set in a node.  These do not always correspond as to what is
 * allowed in the source code get reflected in the AST node built and expected
 * by the backend.  For those attributes such as [unique] [ref] [ptr] which are
 * allowed in the source in multiple locations, but are only set on types, they
 * are not specified below, but handled explicity in the builder routines.
 */
#define ASTP_OPERATION_FLAGS ASTP_BROADCAST | ASTP_MAYBE | ASTP_IDEMPOTENT | \
            ASTP_REFLECT_DELETIONS | ASTP_LOCAL

#define ASTP_PARAMETER_FLAGS ASTP_STRING | ASTP_STRING0 |   \
            ASTP_SMALL | ASTP_CONTEXT |       \
            ASTP_IN | ASTP_OUT | ASTP_IN_SHAPE | \
            ASTP_OUT_SHAPE |  ASTP_UNIQUE | \
            ASTP_REF | ASTP_PTR | ASTP_RANGE

#define ASTP_FIELD_FLAGS ASTP_STRING | ASTP_STRING0 | \
            ASTP_IGNORE | ASTP_SMALL | ASTP_CONTEXT |  ASTP_UNIQUE | \
            ASTP_REF | ASTP_PTR | ASTP_RANGE

#define ASTP_TYPE_FLAGS ASTP_STRING | ASTP_STRING0 |  ASTP_UNIQUE | \
            ASTP_SWITCH_TYPE | \
            ASTP_REF | ASTP_SMALL | ASTP_CONTEXT |     \
            ASTP_HANDLE | ASTP_TRANSMIT_AS | ASTP_UNALIGN | \
            ASTP_ALIGN_SMALL | ASTP_ALIGN_SHORT | ASTP_ALIGN_LONG | \
            ASTP_ALIGN_HYPER | ASTP_UNIQUE | ASTP_PTR | ASTP_V1_ENUM

#define ASTP_ARM_FLAGS ASTP_STRING | ASTP_STRING0 |  \
            ASTP_CASE | ASTP_DEFAULT | \
            ASTP_SMALL | ASTP_CONTEXT |  ASTP_UNIQUE | \
            ASTP_REF | ASTP_PTR


/*
 * ASTP (Private) definitions for the boolean attributes for an interface.
 * These are identical to the common definitions defined in AST.H.
 *
 * Note, specifying the implicit_handle attribute in the IDL means that
 * a specific handle will be defined in the ACF, so we set the NPB flag
 * (non-parametric binding) in the interface.
 */

#define ASTP_IF_PORT            0x00000040
#define ASTP_IF_UUID            0x00000080
#define ASTP_IF_VERSION         0x00000100
#define ASTP_IF_EXCEPTIONS      0x00000200


/*
 * Macro for manipulating interface boolean attributes.
 */
#define ASTP_CLR_IF_AF(if)        \
    (if)->fe_info->type_specific.if_flags = 0

#define ASTP_SET_IF_AF(if,attribute)       \
    (if)->fe_info->type_specific.if_flags |= (attribute)

#define ASTP_IF_AF_SET(if,attribute)       \
    (((if)->fe_info->type_specific.if_flags & (attribute)) != 0)


/*
 * Macros for checking and clearing attributes in an ASTP_attr_flag_t structure.
 */
#define ASTP_TEST_ATTR(__attr_ptr,__flags)   \
    ((((__attr_ptr)->attr_flags) & (__flags)) != 0)

#define ASTP_CLR_ATTR(__attr_ptr,__flags)   \
    (__attr_ptr)->attr_flags &= (~(__flags))


/* AST Private structure types */

/*
 * Structure for attribute flags
 */
typedef unsigned long   ASTP_attr_flag_t;

/*
 * Structure for handling both array_bounds and attribute flags in the
 * grammar as a unit.
 */
typedef struct ASTP_attributes_t
{
    struct ASTP_type_attr_n_t       *bounds;
    ASTP_attr_flag_t                attr_flags; /* values are the ASTP_xxx  */
                                                /* flags above              */
    NAMETABLE_id_t  	iid_is_name;	/* name for iid_is */

} ASTP_attributes_t;


/*
 * Structure for the singly-linked list used to build the AST.
 *
 * Note, since next and last are not guarenteed for all nodes,
 * but fe_info, and be_info are, fe_info and be_info are always
 * the first two fields, followed by the next and last fields
 * when present.
 */

typedef struct ASTP_node_t
{
    fe_info_t           *fe_info;
    be_info_t           *be_info;
    struct ASTP_node_t  *next;
    struct ASTP_node_t  *last;
} ASTP_node_t, *ASTP_node_t_p;


typedef enum
{
    ASTP_constant_bound,
    ASTP_default_bound,
    ASTP_open_bound
} ASTP_bound_t;

/*
 * Array Index private node.
 *
 * Keeps the lower and upper bounds
 * to a dimension of an array as well
 * the type of each bound.
 */
typedef struct ASTP_array_index_n_t
{
    fe_info_t           *fe_info;       /* Must be here, but unused */
    be_info_t           *be_info;       /* Must be here, but unused */
    struct ASTP_array_index_n_t *next;
    struct ASTP_array_index_n_t *last;
    ASTP_bound_t     lower_bound_type;
    ASTP_bound_t     upper_bound_type;
    AST_constant_n_t *lower_bound;
    AST_constant_n_t *upper_bound;
} ASTP_array_index_n_t;

/*
 *  Type attribute node.
 *
 *  A Type attribute node is used to hold the
 *  attributes for a type during the parse.
 */

typedef enum
{
    last_is_k,
    first_is_k,
    max_is_k,
    min_is_k,
    length_is_k,
    size_is_k,
    switch_is_k,
    iid_is_k,
    range_k
} ASTP_attr_k_t;

typedef struct ASTP_type_attr_n_t
{
    fe_info_t           *fe_info;       /* Must be here, but unused */
    be_info_t           *be_info;       /* Must be here, but unused */
    struct ASTP_type_attr_n_t *next;
    struct ASTP_type_attr_n_t *last;
    int             source_line;
    ASTP_attr_k_t   kind;
    boolean is_expr;
    union	{
	struct {
	    NAMETABLE_id_t  name;
	    boolean         pointer;
	} simple;
	struct AST_exp_n_t * expr;
    } b;
} ASTP_type_attr_n_t;

/*
 * Context block to save the field attribute
 * reference information.
 * This is used when processing a parameter
 * field attribute and the referent parameter
 * is not yet defined.
 */

typedef struct ASTP_field_ref_ctx_t
{
    fe_info_t           *fe_info;       /* Must be here, but unused */
    be_info_t           *be_info;       /* Must be here, but unused */
    struct ASTP_field_ref_ctx_t *next;
    struct ASTP_field_ref_ctx_t *last;
    NAMETABLE_id_t    name;             /* Saved parameter reference */
    AST_field_ref_n_t *field_ref_ptr;   /* Address to field ref needing patch */
} ASTP_field_ref_ctx_t;

/*
 *  A declarator node is used hold the information about a IDL/C declarator
 *  which will eventually be merged with the type node and turned into an
 *  export.  A declarator node is private to the AST builder which is why it is
 *  not declared in AST.H.  If the next_op field is NULL then this is a simple
 *  declarator otherwise the various operations must be applied to the base type to
 *  yeild the result type described via this declarator.
 */

typedef struct ASTP_declarator_n_t
{
    fe_info_t           *fe_info;       /* Points to fe_info of parent node */
                                        /* which determines the type of     */
                                        /* the expected referent            */
    be_info_t           *be_info;       /* Must be here, but unused */
    struct ASTP_declarator_n_t *next;
    struct ASTP_declarator_n_t *last;
    NAMETABLE_id_t      name;
    struct ASTP_declarator_op_n_t   *next_op;
    struct ASTP_declarator_op_n_t   *last_op;
} ASTP_declarator_n_t;


/*
 * Representation of operations that can be performed on declarators to produce
 * various types (pointer, array, function_pointer).
 */
typedef struct ASTP_declarator_op_n_t
{
    struct ASTP_declarator_op_n_t *next_op;
    AST_type_k_t        op_kind;    /* Valid operators are: AST_array_k,    */
                                    /* AST_pointer_k, or AST_function_k */
    union
    {
        ASTP_node_t             *node;
        ASTP_array_index_n_t    *indices;       /* Set if op_kind == AST_array_k */
        AST_type_n_t            *type_node_ptr;
        AST_parameter_n_t       *routine_params;/* Set if op_kind == AST_function_k */
        int                     pointer_count;  /* Set if op_kind == AST_pointer_k */
    } op_info;
} ASTP_declarator_op_n_t;


/*
 * A Tag ref node is created for each forward reference to a
 * struct/union via it's tag name (e.g.  struct foo).  It
 * provides enough information to patch references to the
 * type after the parse has completed.
 */
typedef struct ASTP_tag_ref_n_t
{
    fe_info_t           *fe_info;       /* Must be here, but unused */
    be_info_t           *be_info;       /* Must be here, but unused */
    struct ASTP_tag_ref_n_t *next;
    struct ASTP_tag_ref_n_t *last;
    NAMETABLE_id_t      name;           /* Tag name referenced */
    AST_type_k_t        ref_kind;       /* AST_struct_k or AST_disc_union_k */
    AST_type_n_t        *type_node_ptr; /* Type node that referenced tag */
} ASTP_tag_ref_n_t;


/*
 * Structure to hold count of input/output parameters needing marshalling
 * used to formulate operation synthesized attributes AST_HAS_INS/AST_HAS_OUTS.
 */
typedef struct ASTP_parameter_count_t
{
    int  in_params;
    int  out_params;
} ASTP_parameter_count_t;


extern boolean ASTP_expr_is_simple(AST_exp_n_t * exp);
extern AST_exp_n_t * AST_expression(unsigned long exp_type, AST_exp_n_t * oper1, AST_exp_n_t * oper2, AST_exp_n_t * oper3);
extern AST_exp_n_t * AST_exp_boolean_constant(boolean value);
extern AST_exp_n_t * AST_exp_null_constant(void);
extern AST_exp_n_t * AST_exp_string_constant(STRTAB_str_t string);
extern AST_exp_n_t * AST_exp_identifier(NAMETABLE_id_t name);
extern AST_exp_n_t * AST_exp_char_constant(char value);
extern AST_exp_n_t * AST_exp_integer_constant(long value, int int_signed);
extern AST_constant_n_t * AST_constant_from_exp(AST_exp_n_t * exp);
extern boolean ASTP_evaluate_expr(AST_exp_n_t * exp_node, boolean constant_only);
extern void ASTP_free_exp(AST_exp_n_t * exp);
extern long ASTP_expr_integer_value(AST_exp_n_t * exp);


/*
 *  Interface Attributes
 */
extern int              interface_pointer_class;


/*
 *  Operation, Parameter, Type Attributes
 */
extern AST_type_n_t         *ASTP_transmit_as_type;
extern AST_type_n_t         *ASTP_switch_type;
extern AST_case_label_n_t   *ASTP_case;


/*
 *  Interface just parsed
 */
extern AST_interface_n_t * the_interface;


//centeris wfu
extern AST_cpp_quote_n_t * global_cppquotes; //store cppquote nodes that appear in front of interfaces

extern AST_cpp_quote_n_t * global_cppquotes_post; //store cppquote nodes that appear behind the last parsed interface


/* list of imports that occurred outside of interfaces */
extern AST_import_n_t * global_imports;

/*
 * List head for saved context for field
 * attributes forward referenced parameters.
 */
extern ASTP_field_ref_ctx_t *ASTP_field_ref_ctx_list;

/*
 * List head for referenced struct/union tags.
 */
extern ASTP_tag_ref_n_t *ASTP_tag_ref_list;

/*
 *  Control for parser
 */
extern boolean ASTP_parsing_main_idl;   /* True when parsing the main idl */
extern int  yylineno;                   /* Current line number */

/*
 *  Builtin in constants
 */

extern AST_constant_n_t *zero_constant_p;

/*
 * Builtin base types
 */
extern AST_type_n_t     *ASTP_char_ptr,
                        *ASTP_boolean_ptr,
                        *ASTP_byte_ptr,
                        *ASTP_void_ptr,
                        *ASTP_handle_ptr,
                        *ASTP_short_float_ptr,
                        *ASTP_long_float_ptr,
                        *ASTP_small_int_ptr,
                        *ASTP_short_int_ptr,
                        *ASTP_long_int_ptr,
                        *ASTP_hyper_int_ptr,
                        *ASTP_small_unsigned_ptr,
                        *ASTP_short_unsigned_ptr,
                        *ASTP_long_unsigned_ptr,
                        *ASTP_hyper_unsigned_ptr;

/* Default tag for union */
extern NAMETABLE_id_t   ASTP_tagged_union_id;

/*
 * Function prototypes exported by ASTP_{COM/GBL/DMP/CPX/SIM}.C
 */
ASTP_type_attr_n_t *AST_array_bound_info(
    NAMETABLE_id_t name,
    ASTP_attr_k_t kind,
    boolean is_pointer
);
ASTP_type_attr_n_t * AST_array_bound_from_expr(
		  AST_exp_n_t * exp,
		  ASTP_attr_k_t kind
);
ASTP_type_attr_n_t * AST_range_from_expr(
		  AST_exp_n_t * min_exp,
		  AST_exp_n_t * max_exp
);
void AST_capture_operation_attrs(
    void
);

AST_constant_n_t *AST_clone_constant(
    AST_constant_n_t *constant_node_p
);

ASTP_node_t *AST_concat_element(
    ASTP_node_t *list_head,
    ASTP_node_t *element
);

AST_constant_n_t *AST_constant_node(
    AST_constant_k_t kind
);

ASTP_declarator_n_t *AST_declarator_node(
    NAMETABLE_id_t name
);

void AST_declarator_operation(
    ASTP_declarator_n_t     *declarator,
    AST_type_k_t            op_kind,
    ASTP_node_t             *op_info,
    int                     pointer_count
);

AST_parameter_n_t *AST_declarator_to_param(
    ASTP_attributes_t   *attributes,
    AST_type_n_t *type,
    ASTP_declarator_n_t *declarator
);

AST_type_p_n_t *AST_declarators_to_types(
 AST_interface_n_t * ifp,
    AST_type_n_t        *type_ptr,
    ASTP_declarator_n_t *declarators_ptr,
    ASTP_attributes_t   *attributes
);

AST_export_n_t *AST_export_node(
    ASTP_node_t *export_ptr,
    AST_export_k_t kind
);

AST_cpp_quote_n_t *AST_cpp_quote_node(
    STRTAB_str_t text
);

AST_include_n_t *AST_include_node(
    STRTAB_str_t include_file,
    STRTAB_str_t include_file_name
);

AST_import_n_t *AST_import_node(
    STRTAB_str_t imported_file
);

AST_exception_n_t *AST_exception_node(
    NAMETABLE_id_t  excep_name
);

AST_name_n_t *AST_name_node(
    NAMETABLE_id_t  name
);

void AST_init(
        void
);

AST_constant_n_t *AST_integer_constant(
    long int value
);

AST_interface_n_t *AST_interface_node(
      void
);

void AST_finish_interface_node(
    AST_interface_n_t *interface_node
);

AST_operation_n_t *AST_operation_node(
    AST_type_n_t *result_type,
    ASTP_declarator_n_t *declarator,
    ASTP_attributes_t   *attributes
);

AST_parameter_n_t * AST_parameter_node(
    NAMETABLE_id_t identifier
);

AST_rep_as_n_t *AST_represent_as_node(
    NAMETABLE_id_t name
);

AST_cs_char_n_t *AST_cs_char_node(
    NAMETABLE_id_t name
);

AST_type_n_t *AST_set_type_attrs(
    AST_type_n_t        *type_node_ptr,
    ASTP_attributes_t   *attributes
);

void AST_set_ports(
    AST_interface_n_t *interface_node_p
);

AST_type_n_t *AST_type_node(
    AST_type_k_t kind
);

AST_type_p_n_t *AST_type_ptr_node(
    void
);

AST_export_n_t *AST_types_to_exports(
    AST_type_p_n_t *type_p_list
);


AST_array_n_t *AST_array_node(
    AST_type_n_t *element_type_ptr
);

AST_array_index_n_t *AST_array_index_node(
    unsigned short array_size
);


ASTP_array_index_n_t *ASTP_array_index_node(
    AST_constant_n_t *lower_bound,
    ASTP_bound_t lower_bound_type,
    AST_constant_n_t *upper_bound,
    ASTP_bound_t upper_bound_type
);

AST_constant_n_t *AST_boolean_constant(
    boolean value
);

AST_case_label_n_t *AST_case_label_node(
    AST_constant_n_t *case_label
);

AST_constant_n_t *AST_char_constant(
    char value
);


AST_arm_n_t *AST_declarator_to_arm(
    AST_type_n_t *type_ptr,
    ASTP_declarator_n_t *declarator,
    ASTP_attributes_t   *attributes
);

AST_field_n_t *AST_declarators_to_fields(
    ASTP_declarator_n_t *declarators_ptr,
    AST_type_n_t        *type_ptr,
    ASTP_attributes_t   *attributes
);

AST_case_label_n_t *AST_default_case_label_node(
    void
);

AST_type_n_t *AST_disc_union_node(
    NAMETABLE_id_t tag_name,
    NAMETABLE_id_t union_name,
    NAMETABLE_id_t disc_name,
    AST_type_n_t *disc_type,
    AST_arm_n_t *arms_list
);

AST_type_n_t *AST_enumerator_node(
    AST_constant_n_t *constant_list,
    AST_type_k_t size
);

AST_constant_n_t *AST_enum_constant(
    NAMETABLE_id_t identifier,
	 AST_exp_n_t * exp
);


AST_constant_n_t *AST_finish_constant_node(
    AST_constant_n_t *constant_ptr,
    ASTP_declarator_n_t *declarator,
    AST_type_n_t *type_ptr
);

AST_field_attr_n_t *AST_field_attr_node(
    void
);

AST_field_ref_n_t *AST_field_ref_node(
    unsigned short dimension
);

AST_operation_n_t *AST_function_node(
    AST_type_n_t          *result_type,
    NAMETABLE_id_t        op_name,
    AST_parameter_n_t *parameters
);

AST_arm_n_t *AST_label_arm(
    AST_arm_n_t *member,
    AST_case_label_n_t *case_labels
);

AST_type_n_t *AST_lookup_integer_type_node(
      AST_type_k_t    int_size,
      int             int_signed
);

AST_type_n_t *AST_lookup_type_node(
    AST_type_k_t kind
);

AST_type_n_t *AST_lookup_named_type(
    NAMETABLE_id_t type_name
);

AST_constant_n_t *AST_named_constant(
    NAMETABLE_id_t const_name
);

AST_constant_n_t *AST_null_constant(
    void
);

AST_type_n_t *AST_pipe_node(
    AST_type_n_t *pipe_type
);

AST_pointer_n_t *AST_pointer_node(
    AST_type_n_t * pointee
);

AST_type_n_t *AST_set_union_arms(
    AST_type_n_t *union_type_ptr,
    AST_arm_n_t *member_list
);

AST_constant_n_t *AST_string_constant(
    STRTAB_str_t value
);

AST_type_n_t *AST_structure_node(
    AST_field_n_t *field_list,
    NAMETABLE_id_t identifier
);

AST_field_n_t *AST_tag_declarators_to_fields(
    ASTP_declarator_n_t *declarators,
    NAMETABLE_id_t      identifier,
    ASTP_attributes_t   *attributes,
    AST_type_k_t        kind
);

AST_type_n_t *AST_type_from_tag(
    AST_type_k_t kind,
    NAMETABLE_id_t identifier
);

void AST_set_type_boolean_attrs(
    AST_type_n_t *type_node
);

void AST_clear_type_attrs(
    void
);

boolean AST_lookup_field_attr(
    ASTP_attributes_t   *attributes,    /* [in] Attributes - bounds field is */
                                        /*      linked list of field attrs   */
    ASTP_attr_k_t       field_attr      /* [in] Field attribute to look up */
);

AST_field_attr_n_t *AST_set_field_attrs(
    ASTP_attributes_t  *attributes,
    ASTP_node_t *parent_node,
    AST_type_n_t *type_node
);

void ASTP_free_declarators(
    ASTP_declarator_n_t *declarators_ptr
);

void ASTP_free_simple_list(
    ASTP_node_t *list_ptr
);

AST_type_n_t *AST_propagate_type(
    AST_type_n_t *type_node_ptr,
    ASTP_declarator_n_t *declarator_ptr,
    ASTP_attributes_t *attributes,
    ASTP_node_t *parent_node
);

void ASTP_add_name_binding(
    NAMETABLE_id_t name,
    void	   *AST_node
);

AST_type_n_t *ASTP_chase_ptr_to_kind(
    AST_type_n_t *type_node,
    AST_type_k_t kind
);

AST_type_n_t *ASTP_chase_ptr_to_type(
    AST_type_n_t *type_node
);

ASTP_node_t *ASTP_lookup_binding(
    NAMETABLE_id_t      name,
    fe_node_k_t         node_kind,
    boolean             noforward_ref
);

void ASTP_patch_tag_references(
    AST_interface_n_t *interface_node_ptr
);

void ASTP_patch_field_reference(
    void
);

void ASTP_set_fe_info(
    fe_info_t   **fe_info_ptr,
    fe_node_k_t   fe_node_kind
);

void ASTP_save_tag_ref(
    NAMETABLE_id_t      identifier,
    AST_type_k_t        kind,
    AST_type_n_t        *type_node_ptr
);

void ASTP_process_pa_type(
    AST_type_n_t *type_node_ptr
);

void AST_set_flags(
    AST_flags_t         *flags,
    ASTP_node_t         *node_ptr,
    ASTP_attributes_t   *attributes
);

long AST_attribute_to_token(
    ASTP_attr_flag_t    *attribute
);

AST_arm_n_t *AST_arm_node(
    NAMETABLE_id_t name,
    AST_case_label_n_t *label,
    AST_type_n_t *type
);

void ASTP_parse_port(
      AST_interface_n_t   *interface_p,
      STRTAB_str_t        port_string
);

void ASTP_validate_forward_ref(
    AST_type_n_t *type
);

NAMETABLE_id_t AST_generate_name(
    AST_interface_n_t *int_p,
    char              *suffix
);

void ASTP_validate_integer(
      AST_exp_n_t *expression
);

void KEYWORDS_init(
    void
);

char *KEYWORDS_lookup_text(
    long token
);

int KEYWORDS_screen(
    char * identifier,
    NAMETABLE_id_t * id
);

void ASTP_set_array_rep_type
(
    AST_type_n_t        *type_node_ptr,
    AST_type_n_t        *array_base_type,
    boolean             is_varying
);
void ASTP_set_implicit_handle(
	 AST_interface_n_t   *int_p,
	 NAMETABLE_id_t type_name,
	 NAMETABLE_id_t	handle_name
);

#endif
