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
**      irep.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Defines data structures and API's for Intermediate Representation (IR)
**  of an IDL interface.
**
**  %a%private_begin
**
**
**  %a%private_end  
*/

#ifndef IREPH_INCL
#define IREPH_INCL

typedef unsigned char byte;

/*
 * Define maximum arguments to an intermediate rep. tuple and the standard
 * argument numbers of particular types of arguments.
 */
#define IR_MAX_ARG      4           /* Maximum args to tuple                  */
#define IR_ARG_BOUND    0           /* standard arg# for bound kind           */
#define IR_ARG_EXPR     0           /* standard arg# for expression           */
#define IR_ARG_INTFC    0           /* standard arg# for interface node ptr   */
#define IR_ARG_LIMIT    0           /* standard arg# for data limit kind      */
#define IR_ARG_NAME     0           /* standard arg# for nametable ID         */
#define IR_ARG_TUP      0           /* standard arg# for ptr to another tuple */
#define IR_ARG_PFNUM    1           /* standard arg# for param or field num   */
#define IR_ARG_TYPE     1           /* standard arg# for AST type node ptr    */
#define IR_ARG_ARM      2           /* standard arg# for arm node ptr         */
#define IR_ARG_BYT      2           /* standard arg# for byte value           */
#define IR_ARG_FIELD    2           /* standard arg# for field node ptr       */
#define IR_ARG_INST     2           /* standard arg# for instance node ptr    */
#define IR_ARG_INT      2           /* standard arg# for integer value        */
#define IR_ARG_LABEL    2           /* standard arg# for case label node ptr  */
#define IR_ARG_PARAM    2           /* standard arg# for parameter node ptr   */
#define IR_ARG_REP_AS   2           /* standard arg# for rep_as node ptr      */
#define IR_ARG_TYPE2    2           /* standard arg# for second type node ptr */
#define IR_ARG_CS_CHAR  2           /* standard arg# for cs_char node ptr     */
#define IR_ARG_BOUND_XTRA 3         /* standard arg# for xtra bound info      */

/*
 * Each tuple has an opcode in addition to its arguments.
 * Define opcode data type and the valid opcodes.
 * For each opcode the comment indicates the valid arguments and their types.
 */
typedef unsigned short IR_opcode_k_t;
                                    /* arg0         arg1        arg2          */
#define IR_op_align_k            1  /* --           --          int_val       */
#define IR_op_allocate_k         2  /* expr         type        expr          */
#define IR_op_array_bounds_k     3  /* tup          type        int_val       */
#define IR_op_array_end_k        4  /* --           type        --            */
#define IR_op_bound_k            5  /* bound_k      int_val     int_val |     */
                                    /*                          param|field   */
#define IR_op_call_k             6  /* expr         expr        int_val       */
#define IR_op_call_param_k       7  /* expr         type        --            */
#define IR_op_case_k             8  /* expr         --          case_label    */
#define IR_op_conformant_array_k 9  /* expr         type        instance      */
#define IR_op_conformant_info_k 10  /* tup          --          --            */
#define IR_op_context_handle_k  11  /* expr         type        instance      */
#define IR_op_declare_k         12  /* name         type        [param]       */
#define IR_op_default_k         13  /* --           type        --            */
#define IR_op_disc_union_begin_k 14 /*              type        int_val       */
#define IR_op_disc_union_end_k  15  /*              type        int_val       */
#define IR_op_fixed_array_k     16  /* expr         type        instance      */
#define IR_op_flat_array_k      17  /* tup          --          --            */
#define IR_op_free_k            18  /* expr         type        --            */
#define IR_op_full_array_end_k  19  /* --           --          --            */
#define IR_op_full_array_k      20  /* tup          --          --            */
#define IR_op_full_ptr_k        21  /* expr         type        instance      */
#define IR_op_limit_k           22  /* limit_k      int_val     int_val |     */
                                    /*                          param|field   */
#define IR_op_marshall_k        23  /* expr         type        instance      */
#define IR_op_noop_k            24  /* --           --          --            */
#define IR_op_open_array_k      25  /* expr         type        instance      */
#define IR_op_passed_by_ref_k   26  /* expr         type        instance      */
#define IR_op_pipe_begin_k      27  /* --           type        type          */
#define IR_op_pipe_end_k        28  /* --           type        type          */
#define IR_op_pointee_end_k     29  /* --           type        --            */
#define IR_op_ref_ptr_k         30  /* expr         type        instance      */
#define IR_op_represent_as_k    31  /* tup          type        rep_as        */
#define IR_op_represent_end_k   32  /* tup          type        rep_as        */
#define IR_op_struct_begin_k    33  /* expr         type        instance      */
#define IR_op_struct_end_k      34  /* expr         type        instance      */
#define IR_op_switch_enc_k      35  /* name         type        --            */
#define IR_op_switch_n_e_k      36  /* expr         type        param|field   */
#define IR_op_transmit_as_k     37  /* tup          type        type          */
#define IR_op_transmit_end_k    38  /* tup          type        type          */
#define IR_op_type_indirect_k   39  /* expr         type        param         */
#define IR_op_unique_ptr_k      40  /* expr         type        instance      */
#define IR_op_varying_array_k   41  /* expr         type        instance      */
#define IR_op_cs_char_k         42  /* tup          type        cs_char       */
#define IR_op_cs_char_end_k     43  /* tup          type        cs_char       */
#define IR_op_codeset_shadow_k  44  /*                          [int_val]     */
#define IR_op_release_shadow_k  45  /*                                        */
#define IR_op_interface_k       46  /* interface    type        instance      */
#define IR_op_range_k           47  /* --           int_val     int_val       */

/*
 * Each tuple has flags in addition to its opcode and arguments.
 * Define the valid flags and values.
 */
typedef unsigned long IR_flags_t;
#define IR_CLIENT_SIDE      0x0001  /* Client side only tuple */
#define IR_SERVER_SIDE      0x0002  /* Server side only tuple */
#define IR_MARSHALL_CODE    0x0004  /* Marshalling only tuple */
#define IR_UNMARSHALL_CODE  0x0008  /* Unmarshalling only tuple */
#define IR_ARRAYIFIED_PTR   0x0010  /* Arrayified pointer */
#define IR_STRING           0x0020  /* [string] array */
#define IR_CONFORMANT       0x0040  /* Conformant struct */
#define IR_ENCAPSULATED     0x0080  /* Encapsulated union */
#define IR_VOID             0x0100  /* on 'case' tuple => empty arm */
#define IR_BOOLEAN          0x0200  /* on 'case' tuple => boolean case value */
#define IR_REP_AS           0x0400  /* on 'type indirect' tuple => indirect  */
                                    /*  tuples hang off rep_as node          */
                                    /* on 'string_k' tuples => octetsize is  */
                                    /*  of rep_as type, n.k. at compile time */
#define IR_CS_CHAR          0x0800  /* on 'type indirect' tuple => indirect  */
                                    /*  tuples hang off cs_char node         */
                                    /* on IR_op_*_array_k, IR_op_bound_k,    */
                                    /*  IR_op_limit_k tuples => base type of */
                                    /*  array has [cs_char] attribute        */
                                    /* on IR_op_marshall_k tuple => instance */
                                    /*  is used as a field attribute for a   */
                                    /*  non-fixed array of [cs_char] type    */
#define IR_IN_ONLY          0x1000  /* [in]-only tuple */
#define IR_OUT_ONLY         0x2000  /* [out]-only tuple */

#define IR_CF_EARLY         0x10000 /* early correlation flag */
#define IR_CF_SPLIT         0x20000 /* split correlation flag */
#define IR_CF_IS_IID_IS     0x40000 /* is iis_is correlation flag */
#define IR_CF_DONT_CHECK    0x80000 /* don't check correlation flag */

/*
 * IR_bound_k_t defines the kinds of array bounds.
 */
typedef enum {
    IR_bnd_fixed_k,
    IR_bnd_min_is_k,
    IR_bnd_max_is_k,
    IR_bnd_size_is_k,
    IR_bnd_string_k
} IR_bound_k_t;


/*
 * IR_limit_k_t defines the kinds of array data limits.
 */
typedef enum {
    IR_lim_fixed_k,
    IR_lim_first_is_k,
    IR_lim_last_is_k,
    IR_lim_length_is_k,
    IR_lim_string_k,
    IR_lim_upper_conf_k
} IR_limit_k_t;

/*
 * IR_expr_t defines a data type for 'expressions' that are associated with a
 * tuple.  Expressions in tuples are not essential to Data Driven Backends and
 * thus are not yet specified.  IR_expr_t is specified, however, in a way that
 * should provide flexibility for future implementation of expressions.
 * The current implementation must always store NULL for IR_expr_t data.
 */
typedef struct IR_expr_n_t {
    long            dummy;
} IR_expr_n_t;
#define IR_EXP_FC_NONE		0
#define IR_EXP_FC_DIV_2	1
#define IR_EXP_FC_MUL_2	2
#define IR_EXP_FC_ADD_1	3
#define IR_EXP_FC_SUB_1	4
#define IR_EXP_FC_ALIGN_2	5
#define IR_EXP_FC_ALIGN_4	6
#define IR_EXP_FC_ALIGN_8	7
#define IR_EXP_FC_CALLBACK	8
#define IR_EXP_FC_DIV_4	9
#define IR_EXP_FC_MUL_4	10
#define IR_EXP_FC_DIV_8	11
#define IR_EXP_FC_MUL_8	12
#define IR_EXP_FC_FIXED	13

typedef IR_expr_n_t *IR_expr_t;


/*
 * IR_arg_t defines the various possible argument types in tuples.
 */
typedef union IR_arg_t {
    struct AST_type_n_t         *type;
    struct AST_parameter_n_t    *param;
    struct AST_field_n_t        *field;
    struct AST_arm_n_t          *arm;
    struct AST_instance_n_t     *inst;
    struct AST_pointer_n_t      *ptr;
    struct AST_array_n_t        *array;
    struct AST_case_label_n_t   *label;
    struct AST_rep_as_n_t       *rep_as;
    struct AST_cs_char_n_t      *cs_char;
    struct AST_interface_n_t    *intfc;
    struct IR_tup_n_t           *tup;
    NAMETABLE_id_t              name;
    IR_expr_t                   expr;
    IR_bound_k_t                bound_k;
    IR_limit_k_t                limit_k;
    long                        int_val;
    byte                        byt_val;
} IR_arg_t;


/*
 * IR_tup_n_t is the basic data element of the intermediate representation.
 * Each IR_tup_n_t is a tuple consisting of an operation-code, flags, and 0 or
 * more arguments.  The name 'tuple' sometimes shortened to just 'tup'.
 * Tuples are organized into singly linked lists hung off AST type, parameter,
 * and operation nodes.
 */
typedef struct IR_tup_n_t {
    struct IR_tup_n_t   *next;
    IR_arg_t            arg[IR_MAX_ARG];
    IR_opcode_k_t       opcode;
    IR_flags_t          flags;
} IR_tup_n_t;


/*
 * Macro that evaluates to TRUE if a tuple list has no meaningful data entries.
 * Macro necessary because IREP generation can insert no-op sentinel entry.
 *
 * tup_p = ptr to beginning of list of IREP tuples.
 */
#define IR_NO_IREP(tup_p) \
    ( (tup_p) == NULL || \
      ((tup_p)->next == NULL && (tup_p)->opcode == IR_op_noop_k) )


/*
 * Macro to determine if an instance of a type is a string array.
 * Returns TRUE if type is array and either:
 *  1) instance address NULL and type has [(v1_)string] attribute
 *  2) instance address non-NULL and instance has [(v1_)string] attribute
 */
#define IR_STRING_ARRAY(type_p, inst_p) \
(type_p->kind == AST_array_k \
 && (((inst_p) == NULL && (AST_STRING_SET(type_p) || AST_STRING0_SET(type_p))) \
     || ((inst_p) != NULL \
         && (AST_STRING_SET(inst_p) || AST_STRING0_SET(inst_p)))))


/*
 * Macro to determine if an instance of a type is a stringified pointer.
 * Returns TRUE if type is pointer and either:
 *  1) instance address NULL and type has [string] attribute
 *  2) instance address non-NULL and instance has [string] attribute
 *     and an array_rep_type.  NOTE: Frontend puts [string] on parameter that
 *     is toplevel pointer to [string] array but to this code a pointer instance
 *     is considered stringified only if the pointer itself is stringified
 *     (not if it is a pointer to a string array).  This can be determined by
 *     the presence of an array_rep_type.
 */
#define IR_STRINGIFIED(type_p, inst_p) \
    (type_p->kind == AST_pointer_k \
     && (((inst_p) == NULL && AST_STRING_SET(type_p)) \
         || ((inst_p) != NULL && AST_STRING_SET(inst_p) \
             && type_p->type_structure.pointer->pointee_type->array_rep_type \
                != NULL)))

/*
 * Macro to determine if an instance of a type is an arrayified pointer.
 * Returns TRUE if type is pointer and either:
 *  1) instance of type is a stringified pointer
 *  2) instance address non-NULL and instance has an upper bound attribute.
 */
#define IR_ARRAYIFIED(type_p, inst_p) \
    (type_p->kind == AST_pointer_k \
     && (IR_STRINGIFIED(type_p, inst_p) \
         || ((inst_p) != NULL && (inst_p)->field_attrs != NULL \
             && ((inst_p)->field_attrs->max_is_vec != NULL \
                 || (inst_p)->field_attrs->size_is_vec != NULL))))


/********************************************************/
/*  Data structures and API for maintaining data scope  */
/********************************************************/

/*
 * Scope data maintained by this API includes:
 *  data structure kind (struct, array, pointer, union, etc.)
 *  type information
 *  instance information
 *
 * Before a parameter's tuples are processed, IR_init_scope should be called.
 * After completing parameter processing, IR_finish_scope should be called.
 * By calling IR_process_tup for each tuple processed, you cause a stack of
 * scope information to be maintained.  Each constructed data type kind causes
 * a new level of information on the stack, which is popped off the stack when
 * the end of the type's tuples is processed.  A number of routines and macros
 * are provided that allow you to query for scope information.
 * Note: the term 'scope kind' is simply shorthand for 'scope data kind'.
 */

#define IR_MAX_EXPR     256     /* Maximum size of expression string */

typedef byte IR_scope_k_t;

/*
 * Type and values for data kind associated with a scope.
 */
typedef struct {
    IR_scope_k_t            scope_k;    /* Scope kind */
    struct AST_type_n_t     *type_p;    /* Associated AST type node */
    struct AST_instance_n_t *inst_p;    /* Associated AST instance node */
} IR_scope_t;

#define IR_SCP_TOPLEVEL 1
#define IR_SCP_STRUCT   2
#define IR_SCP_UNION    3
#define IR_SCP_ARRAY    4
#define IR_SCP_POINTER  5
#define IR_SCP_XMIT_AS  6
#define IR_SCP_REP_AS   7
#define IR_SCP_PIPE     8
#define IR_SCP_PASSED_BY_REF 9
#define IR_SCP_CS_CHAR      10

/*
 * Type for information associated with a type indirection scope.
 */
typedef struct {
    IR_flags_t              flags;      /* IREP flags */
    struct AST_type_n_t     *type_p;    /* AST type node address */
} IR_type_scope_t;

/*
**  P r i v a t e   D a t a
**
**  Data structures private to Intermediate Rep.  May change in future so
**  external modules should use defined APIs and not access data directly.
*/
typedef struct {
    struct AST_parameter_n_t    *param_p;   /* AST parameter node ptr */
    struct AST_instance_n_t *saved_inst_p;  /* AST instance node ptr */
    IR_type_scope_t     *type_s_a;          /* Stack of type scope info for */
                                            /*  indirect type references */
    IR_scope_t          *scope_a;           /* Stack of data scope kinds */
    int                 type_stack_size;    /* Size of type_s_a stack */
    int                 scope_stack_size;   /* Size of scope_a stack */
    int                 type_scope;         /* Current type scoping level */
    int                 scope;              /* Current data scoping level */
    boolean             in_flat_rep;        /* T => processing flattened array*/
                                            /*  tuples prior to base type     */
} IR_scope_ctx_t;

/*
**  I R _ c u r _ s c o p e
**  I R _ p a r e n t _ s c o p e
**
**  Returns current/parent scope data kind.
*/
#define IR_cur_scope(ctx_p) (ctx_p)->scope_a[(ctx_p)->scope].scope_k
#define IR_parent_scope(ctx_p) (((ctx_p)->scope == 0) ? \
    IR_SCP_TOPLEVEL : (ctx_p)->scope_a[(ctx_p)->scope-1].scope_k)

/*
**  I R _ c u r _ t y p e
**  I R _ p a r e n t _ t y p e
**
**  Returns current/parent type on the scope stack.
**  Contrast with the indirection stack macros below.
*/
#define IR_cur_type(ctx_p) (ctx_p)->scope_a[(ctx_p)->scope].type_p
#define IR_parent_type(ctx_p) (((ctx_p)->scope == 0) ? \
    NULL : (ctx_p)->scope_a[(ctx_p)->scope-1].type_p)

/*
**  I R _ c u r _ i n s t
**
**  Returns current instance on the scope stack.
*/
#define IR_cur_inst(ctx_p) (ctx_p)->scope_a[(ctx_p)->scope].inst_p

/*
**  I R _ c u r _ i _ t y p e
**  I R _ p a r e n t _ i _ t y p e
**
**  Returns current/parent type on the type indirection stack.  This stack is
**  contains a subset of the types contained on the scope stack; it contains
**  only those types that are accessed via an IR_op_type_indirect_k tuple.
*/
#define IR_cur_i_type(ctx_p) (ctx_p)->type_s_a[(ctx_p)->type_scope].type_p
#define IR_parent_i_type(ctx_p) (((ctx_p)->type_scope == 0) ? \
    NULL : (ctx_p)->type_s_a[(ctx_p)->type_scope-1].type_p)

/*
**  I R _ i n _ *
**
**  Returns immediate {struct | union | array | pointer} nesting level.
**  Examples:
**      If processing pointer within struct, IR_in_struct(ctx) = 0.
**      If processing  scalar within struct, IR_in_struct(ctx) = 1.
**      If processing  struct within struct, IR_in_struct(ctx) = 2.
*/
#define IR_in_struct(ctx_p)     IR_in_scope(ctx_p, IR_SCP_STRUCT)
#define IR_in_union(ctx_p)      IR_in_scope(ctx_p, IR_SCP_UNION)
#define IR_in_array(ctx_p)      IR_in_scope(ctx_p, IR_SCP_ARRAY)
#define IR_in_pointer(ctx_p)    IR_in_scope(ctx_p, IR_SCP_POINTER)
#define IR_in_xmit_as(ctx_p)    IR_in_scope(ctx_p, IR_SCP_XMIT_AS)
#define IR_in_rep_as(ctx_p)     IR_in_scope(ctx_p, IR_SCP_REP_AS)
#define IR_in_cs_char(ctx_p)    IR_in_scope(ctx_p, IR_SCP_CS_CHAR)
#define IR_in_pipe(ctx_p)       IR_in_scope(ctx_p, IR_SCP_PIPE)

extern int IR_in_scope(                 /* Returns data kind nesting level */
    IR_scope_ctx_t      *ctx_p,         /* [in] Scope context */
    IR_scope_k_t        scope           /* [in] Scope data kind */
);

/*
**  I R _ u n d e r _ *
**
**  Returns TRUE if in or under a scope of the specified kind.
**  Example: If processing pointer within struct,
**      IR_under_pointer(ctx)           = TRUE
**      IR_under_struct(ctx)            = TRUE
**      IR_under_ptr_under_struct(ctx)  = TRUE
**      IR_under_array(ctx)             = FALSE
*/
#define IR_under_struct(ctx_p)  IR_under_scope(ctx_p, IR_SCP_STRUCT)
#define IR_under_union(ctx_p)   IR_under_scope(ctx_p, IR_SCP_UNION)
#define IR_under_array(ctx_p)   IR_under_scope(ctx_p, IR_SCP_ARRAY)
#define IR_under_pointer(ctx_p) IR_under_scope(ctx_p, IR_SCP_POINTER)
#define IR_under_xmit_as(ctx_p) IR_under_scope(ctx_p, IR_SCP_XMIT_AS)
#define IR_under_rep_as(ctx_p)  IR_under_scope(ctx_p, IR_SCP_REP_AS)
#define IR_under_cs_char(ctx_p) IR_under_scope(ctx_p, IR_SCP_CS_CHAR)
#define IR_under_pipe(ctx_p)    IR_under_scope(ctx_p, IR_SCP_PIPE)

#define IR_under_ptr_under_struct(ctx_p) \
    IR_under_scope_under_scope(ctx_p, IR_SCP_POINTER, IR_SCP_STRUCT)

extern boolean IR_under_scope(          /* Returns TRUE if under scope kind */
    IR_scope_ctx_t      *ctx_p,         /* [in] Scope context */
    IR_scope_k_t        scope           /* [in] Scope data kind */
);

extern boolean IR_under_scope_under_scope(  /* Ret. TRUE if under scope kinds */
    IR_scope_ctx_t      *ctx_p,         /* [in] Scope context */
    IR_scope_k_t        cscope,         /* [in] Child scope data kind */
    IR_scope_k_t        pscope          /* [in] Parent scope data kind */
);

/*
**  I R _ u n d e r _ t y p e
**
**  Returns nesting level of a type given by the specified address.
**  Useful for detecting self-pointing data structures.
*/
extern int IR_under_type(               /* Returns type nesting level */
    IR_scope_ctx_t      *ctx_p,         /* [in] Scope context */
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
);

/*
**  I R _ i n i t _ s c o p e
**
**  Allocates and initializes a scope context and returns its address.
*/
extern IR_scope_ctx_t *IR_init_scope(   /* Returns ptr to new scope context */
    struct AST_parameter_n_t *param_p   /* [in] Ptr to AST parameter node */
);

/*
**  I R _ f i n i s h _ s c o p e
**
**  Cleans up a scope context.
*/
extern void IR_finish_scope(
    IR_scope_ctx_t      *ctx_p          /* [in] Scope context */
);

/*
**  I R _ p r o c e s s _ t u p
**
**  Processes tuple and maintains scoping data.
*/
extern void IR_process_tup(
    IR_scope_ctx_t      *ctx_p,         /* [in] Scope context */
    IR_tup_n_t          *tup_p          /* [in] Irep tuple */
);

/*
**  I R _ f i e l d _ e x p r
**
**  Constructs a field expression for a toplevel or nested field reference.
*/
extern STRTAB_str_t IR_field_expr(      /* Returns field expression */
    IR_scope_ctx_t      *ctx_p,         /* [in] Scope context */
    struct AST_field_n_t *field_p       /* [in] Ptr to AST field node */
);

/**************************/
/*  Public IREP routines  */
/**************************/

extern boolean IR_gen_irep(         /* Returns TRUE on success */
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val,  /* [in] array of cmd option values */
    struct AST_interface_n_t *int_p /* [in] ptr to interface node */
);

#endif
