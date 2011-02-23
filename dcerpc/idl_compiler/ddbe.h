/*
 * 
 * (c) Copyright 1992 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1992 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1992 DIGITAL EQUIPMENT CORPORATION
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
**      ddbe.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Data Structures and Interfaces for Data Driven Backend.
**
*/

#ifndef DDBEH_INCL
#define DDBEH_INCL

#include <ast.h>
#include <be_pvt.h>
#include <checker.h>
#include <irep.h>
#include <nametbl.h>

#define DDBE_ADDENDA_ALIGN    4     /* Required alignment of type vec addenda */
#define DDBE_ARM_SIZE        16     /* Constant data size for union arm */
#define DDBE_MAX_COMMENT    256     /* Maximum size of comment string */
#define DDBE_MAX_EXPR       256     /* Maximum size of expression string */
#define DDBE_OPER_ENTRIES     6     /* # type vector entries per operation */
#define DDBE_PARAM_START     64     /* Type vector index of start of params */
#define DDBE_PREFIX_IDL     "IDL_"  /* Prefix for data declarations */
#define DDBE_VER_MAJOR        3     /* Interpreter encoding major version */
#define DDBE_VER_MINOR        2     /* Interpreter encoding minor version */

/*
 * DDBE_ARRAYIFIED macro evaluates TRUE if all of the following:
 *  1) instance type is a pointer
 *  2) instance pointee type has an array_rep_type
 *  3) instance has an arrayification attribute (field_attrs or [string])
 * inst_p = instance node address (param, field, or arm)
 */
#define DDBE_ARRAYIFIED(inst_p) \
    ((inst_p)->type->kind == AST_pointer_k \
     && (inst_p)->type->type_structure.pointer->pointee_type->array_rep_type \
        != NULL \
     && ( (inst_p)->field_attrs != NULL || AST_STRING_SET(inst_p) ))

/*
 * DDBE_PIPE macro evaluates TRUE if parameter is pipe, by reference or value.
 */
#define DDBE_PIPE(param_p) \
    ((param_p)->type->kind == AST_pipe_k \
     || ((param_p)->type->kind == AST_pointer_k \
         && (param_p)->type->type_structure.pointer->pointee_type->kind \
            == AST_pipe_k))

/*
 * DDBE_XMIT_REP macro evaluates TRUE is parameter has [transmit_as] or
 * [transmit_as] type, by reference or value.
 */
#define DDBE_XMIT_REP(param_p) \
    (   ((param_p)->type->xmit_as_type != NULL \
         || (param_p)->type->rep_as_type != NULL) \
     || (AST_REF_SET(param_p) \
         && (param_p)->type->kind == AST_pointer_k \
         && ((param_p)->type->type_structure.pointer->pointee_type-> \
                xmit_as_type != NULL \
             || (param_p)->type->type_structure.pointer->pointee_type-> \
                rep_as_type != NULL) \
         && !DDBE_ARRAYIFIED(param_p)) )

/*
 * DDBE_ALLOCATE macro evaluates TRUE if the parameter requires a
 * representation modified by DT_ALLOCATE in the [in]s parameter list.
 */
#define DDBE_ALLOCATE(param_p) \
    (AST_OUT_SET(param_p) && !AST_IN_SET(param_p) && \
        (   (AST_CONFORMANT_SET(param_p->type) \
             && param_p->type->kind == AST_array_k) \
         || (DDBE_ARRAYIFIED(param_p) \
             && AST_CONFORMANT_SET(param_p->type->type_structure. \
                                   pointer->pointee_type->array_rep_type)) ))


#define DDBE_param_is_p_array(param_p) \
    (param_p->type->kind == AST_pointer_k \
    && param_p->type->type_structure.pointer->pointee_type->kind == AST_array_k)

#define DDBE_param_is_array(param_p) \
    (   param_p->type->kind == AST_array_k \
     || DDBE_param_is_p_array(param_p) \
     || DDBE_ARRAYIFIED(param_p) )

#define DDBE_param_is_string(param_p) \
(AST_STRING_SET(param_p) \
 || (DDBE_param_is_p_array(param_p) \
     && AST_STRING_SET(param_p->type->type_structure.pointer->pointee_type)))

#define DDBE_param_is_ref_string(param_p) \
(AST_STRING_SET(param_p) \
 || (AST_REF_SET(param_p) && DDBE_param_is_p_array(param_p) \
     && AST_STRING_SET(param_p->type->type_structure.pointer->pointee_type)))


/*
 * The data needed by a data driven stub and marshalling interpreter is:
 *  1) A 'parameter vector' which contains the parameter addresses for an
 *     interface operation.
 *  2) A 'type vector' which describes the parameter data types and any
 *     contained types in a format that is input to the interpreter.
 *  3) An 'offset vector' which describes sizes of certain data types and
 *     offsets into the fields of structure types.
 *  4) A 'routine vector' which contains the addresses of routines that are
 *     associated with certain data types.
 *
 * The Data Driven Backend (DDBE) describes the latter 3 vectors in an intermed-
 * iate format which is simply a linked list of vector entries.  The linked list
 * provides flexibility for manipulation of the vector entries; once complete,
 * the linked list can be processed and the actual vectors spelled into code.
 *
 * These are the node kinds used to describe the various vector entries.
 */
typedef enum DDBE_vec_kind_t {
    DDBE_vec_byte_k,        /* Byte value                                   */
    DDBE_vec_byte_3m4_k,    /* Byte value on 3 mod 4 boundary               */
    DDBE_vec_comment_k,     /* Block comment                                */
    DDBE_vec_expr_arr_k,    /* Array expr to be spelled in vector element   */
    DDBE_vec_expr_byte_k,   /* Symbolic byte to be spelled in vector element*/
    DDBE_vec_expr_k,        /* Expression to be spelled in vector element   */
    DDBE_vec_expr_long_k,   /* Symbolic long to be spelled in vector element*/
    DDBE_vec_indirect_k,    /* Indirect reference to another vector entry   */
    DDBE_vec_long_k,        /* Long value for previous entry                */
    DDBE_vec_long_bool_k,   /* Long value to be spelt using symbolic name   */
    DDBE_vec_name_k,        /* Name to be spelled as vector element         */
    DDBE_vec_name_client_k, /* Client-only DDBE_vec_name_k                  */
    DDBE_vec_name_server_k, /* Server-only DDBE_vec_name_k                  */
    DDBE_vec_noop_k,        /* No-operation - useful as placeholder         */
    DDBE_vec_offset_begin_k,/* Beginning of offset expressions              */
    DDBE_vec_offset_end_k,  /* End of offset expressions                    */
    DDBE_vec_pad_k,         /* Pad metadata with N filler bytes             */
    DDBE_vec_reference_k,   /* Reference to another vector entry            */
    DDBE_vec_short_k,       /* Short value                                  */
    DDBE_vec_sizeof_k,      /* Entry for size of data type                  */
    DDBE_vec_tag_k,         /* An interpreter 'tag' entry                   */
    DDBE_vec_type_kind_k    /* Interpreter tag for scalar data type         */
} DDBE_vec_kind_t;


/*
 * A linked list of DDBE_vec_rep_t nodes is used to describe the data needed to
 * spell the vector definitions.
 */
typedef struct DDBE_vec_rep_t {
    union {                         /* Arm valid for kind ==            */
        byte            byte_val;   /* byte*_k, pad_k                   */
        short           short_val;  /* short*_k                         */
        long            long_val;   /* long*_k                          */
        NAMETABLE_id_t  name;       /* name_k, tag_k                    */
        STRTAB_str_t    expr;       /* expr*_k                          */
        AST_type_n_t    *type_p;    /* offset_*_k, sizeof_k, type_kind_k*/
        struct DDBE_vec_rep_t *ref_p;     /* reference_k                */
        struct DDBE_vec_rep_t **p_ref_p;  /* indirect_k                 */
    } val;
    long                index;      /* Vector index of this entry */
    struct DDBE_vec_rep_t *next;    /* Pointer to next entry */
    STRTAB_str_t        comment;    /* Comment for this entry */
    DDBE_vec_kind_t     kind;       /* Kind of this entry */
} DDBE_vec_rep_t;


/*
 * The 'indirection stack' is a linked list of the following data structure.
 */
typedef struct DDBE_ind_stack_t {
    struct DDBE_ind_stack_t *next;  /* Pointer to next entry on stack */
    DDBE_vec_rep_t          *ind_p; /* Pointer to a defn/type vector entry */
    DDBE_vec_rep_t          *off_p; /* Pointer to an offset vector entry */
    DDBE_vec_rep_t          *rtn_p; /* Pointer to a routine vector entry */
    DDBE_vec_rep_t          *info_p;/* Pointer to a type info vector entry */
    DDBE_vec_rep_t  *saved_defn_p;  /* Saved definition vector pointer */
    IR_tup_n_t  *cfmt_info_tup_p;   /* Ptr to latest 'conformant info' tuple */
    boolean     in_cfmt_struct_hdr; /* TRUE => in conformant struct header */
    boolean     in_flatarr;         /* TRUE => in array flattened definition */
} DDBE_ind_stack_t;


/*
 * The 'tuple stack' is a linked list of the following data structure.
 */
typedef struct DDBE_tup_stack_t {
    struct DDBE_tup_stack_t *next;  /* Pointer to next entry on stack */
    IR_tup_n_t              *tup_p; /* Pointer to IREP tuple */
} DDBE_tup_stack_t;


/*
 * The DDBE keeps a global state that is used to maintain information while
 * the vector representations are being built.  The address of the state block
 * is passed as a parameter to DDBE routines.  The state block is defined
 * here and described in detail below.
 */
typedef struct DDBE_vectors_t {
    DDBE_vec_rep_t  **p_cur_p;      /* Pointer to defn_p or type_p field */
    DDBE_vec_rep_t  *defn_p;        /* Pointer into definition vector list */
    DDBE_vec_rep_t  *type_p;        /* Pointer into type vector list */
    DDBE_vec_rep_t  *offset_p;      /* Pointer into offset vector list */
    DDBE_vec_rep_t  *rtn_p;         /* Pointer into routine vector list */
    DDBE_vec_rep_t  *saved_defn_p;  /* Saved definition vector pointer */
    DDBE_vec_rep_t  *type_info_p;   /* Pointer into type information list */
    IR_scope_ctx_t  *ir_ctx_p;      /* Ptr to irep scope context */
    AST_interface_n_t *ast_int_p;   /* Ptr to AST interface node */
    DDBE_ind_stack_t *ind_sp;       /* Stack of saved DDBE vector entry addrs */
    DDBE_tup_stack_t *tup_sp;       /* Stack of saved IREP tuple addrs */
    IR_tup_n_t  *cfmt_info_tup_p;   /* Ptr to latest 'conformant info' tuple */
    AST_type_n_t    *switch_type;   /* Union switch type */
    NAMETABLE_id_t  switch_name;    /* Union switch name */
    unsigned long   type_vec_size;  /* Type vec size after offsets resolved */
    unsigned long   offset_vec_size;/* Offset vec size after offsets resolved */
    unsigned long   rtn_vec_size;   /* Rtn vec size after offsets resolved */
    long            switch_index;   /* Index of union switch param or field */
    long            param_num;      /* Parameter number */
    int             arm_byte_cnt;   /* Number of bytes in union arm */
    boolean         allocate;       /* TRUE => allocate pass for out-only prm */
    boolean         allocate_ref;   /* TRUE => allocref pass for out-only prm */
    boolean         conformant;     /* TRUE => next array conformant */
    boolean         free_rep;       /* TRUE => free_rep pass for in-only prm */
    boolean     in_cfmt_struct_hdr; /* TRUE => in conformant struct header */
    boolean         in_default_arm; /* TRUE => in default arm of union */
    boolean         in_flatarr;     /* TRUE => in array flattened definition */
    boolean         update_tup;     /* TRUE => update pointer to next tuple */
    boolean         processing_ins; /* TRUE => processing [in]-only list */
    boolean         processing_outs;/* TRUE => processing [out]-only list */
} DDBE_vectors_t;

/*
    The relationship between DDBE_vectors_t data and the routines in ddbe.c that
    manipulate them is intricate; here is an attempt at a complete definition.

    1) The definition vector holds descriptions for all types that are
    referenced indirectly by the interpreter.  These are NOT limited to irep
    IR_op_type_indirect_k tuples; for example in some cases the interpreter
    needs to references arrays or pointers indirectly, but the irep flattens
    each instance rather than producing an IR_op_type_indirect_k tuple.

    2) The type vector holds descriptions for parameter types.  These descrip-
    tions can have indirect references into the definition vector.  Note that
    indirect references are always into the definition vector.

    3) At a higher level the distinction between type and definition vectors is
    not necessary; at this level it is useful (if not necessary) in order to
    keep track of indirections.  The terminology can get confusing - an attempt
    has been made to use the terms "type", "definition", or "type/definition"
    in the proper context, but there might be ambiguities about.  For example,
    the DDBE_type_i_t "type vector" fields really are "definition vector"
    fields in this context; the confusing naming is because at a higher level,
    the type and definition vectors are concatenated and collectively called
    the type vector.

    4) The type_p field always points into the type vector list.  The type
    vector can be created sequentially since indirect references are put into
    the definition vector.  Thus, the type_p always points to the last entry in
    the type vector list.

    5) The defn_p field always points into the definition vector list.  The
    definition vector can NOT be created sequentially since an indirect
    reference requires adding entries to the list and then reverting the
    insertion cursor back to the indirection reference.  Thus, the defn_p does
    NOT always point to the last entry in the definition vector list.

    6) The p_cur_p field always points at either the type_p or defn_p fields.
    When processing, some entries always go into the definition vector, some
    always go into the type vector, and some always go into either one -
    whichever is current.  Indirecting through p_cur_p handles the latter case
    easily.  Entries that do not fall into one of these 3 categories go into
    the parent vector - see 9).

    7) Insertions into the vector lists are always AFTER the indicated element
    and leave the vector list pointer pointing at the new element.

    8) Whenever the beginning of a constructed type is encountered, the type
    kind is pushed onto a data scoping stack (provided transparently by the
    IREP scoping services).  One common use of the stack is that many actions
    differ if the parent type is of a certain kind.

    9) When the need for an indirect type definition in the definition vector
    occurs, the current (type or definition) vector pointer is pushed onto an
    indirection scoping stack (ind_sp field).  This stack is independent of the
    data scoping stack.  The current vector becomes the definition vector.

     a) During processing of the indirect type, entries can be made into the
        parent vector by referencing the top value on the indirection stack.
        Note that if both parent and child are in the definition vector, there
        are two cursors into the definition vector concurrently.  Special care
        must be taken when both cursors start at the same entry.  In this case,
        make sure that the first entry is to the child, not the parent.  This
        assures that child entries do not get interspersed with the parent
        in the definition vector.

     b) When processing of the indirect type completes, indirection scope is
        popped.  The type vector or definition vector pointer, as determined
        from the indirection stack level, is restored to the saved value on
        the indirection stack.  This vector again becomes current.  If it is
        the definition vector, note that subsequent entries will occur BEFORE
        any child entries that were made but AFTER any parent entries that
        were added during child processing.

    10) Indirect type definitions that are also indirectly represented in the
    IREP (via an IR_op_type_indirect_k tuple) form a subset of the situation
    described in 9) above.  For these cases, another stack is necessary.  The
    tuple stack is used to save the address of the next tuple after the
    IR_op_type_indirect_k tuple.  Then the tuple pointer is changed to point at
    the indirectly referenced type's tuples.  When processing of the type is
    complete, the tuple pointer is restored from the saved value on the stack.

    11) Indirect type definitions described in 9) above also cause the current
    offset and routine vector pointers to be pushed on the indirection scoping
    stack. This is necessary to properly handle processing of nested definitions
    for which offset and/or routine vector entries are necessary.  When
    processing of the indirect type completes, the offset and routine vector
    pointers are restored from the stack.

    12) Indirect vector entries - where a vector entry refers to another
    location in the same or different vector - are accomplished in two ways.
    The straightforward way is to simply insert a vector entry that contains
    a pointer to the referenced vector entry.  A slightly more complex way is
    needed when the referenced vector entry has not yet been created.  In this
    case, a vector entry is created that points to a cell that will eventually
    contain a pointer to the referenced vector entry.  In either case, a
    subsequent processing pass can sequentially walk the vector and resolve
    the indirections with actual indices.
    
    13) The remainder of DDBE_vectors_t fields are flags whose purpose should be
    fairly obvious from the code.
*/


/************************************/
/*  Private DDBE data hung off AST  */
/************************************/

/*
 * Data Driven Backend private information for an AST type or rep_as node.
 * This is information for a type that is valid for all instances of that type.
 * Currently, struct, union, pipe, [transmit_as], and [represent_as] types
 * contain this information.  Other types have similar information captured
 * per-instance in the DDBE.  Each of these fields must initially be NULL.
 * NOTE: Information for a represent_as type hangs off the rep_as AST node.
 * Such a type can have information in both the AST type and rep_as nodes.
 */
typedef struct DDBE_type_i_t {
    /*
     * Type vector pointer: This is the address of the DDBE vector entry that
     * is the first type vector entry for this type.
     */
    struct DDBE_vec_rep_t   *type_vec_p;
    /*
     * Offset vector pointer: This is the address of the DDBE vector entry that
     * is the first offset vector entry for this type.  The offset vector is
     * used for struct types to describe the size of the structure and offsets
     * to its fields.  It is also used to describe the size of presented types.
     */
    struct DDBE_vec_rep_t   *offset_vec_p;
    /*
     * Routine vector pointer: This is the address of the DDBE vector entry
     * that is the first routine vector entry for this type.  Any named type
     * which requires user-written routines whose name is derived from the
     * type name have routine vector entries.
     */
    struct DDBE_vec_rep_t   *rtn_vec_p;
    /*
     * Instance name: For types that need a spelled instance, this is the
     * instance name.  If the type is named, it is derived from the type name.
     * Otherwise, it is assigned a unique name.
     */
    NAMETABLE_id_t          inst_name;
    /*
     * Flag to indicate whether type vector entries for the definition of
     * this type have been completed.
     */
    boolean                 defn_done;
} DDBE_type_i_t;

/*
 * Data Driven Backend private information for an AST operation node.
 */
typedef struct DDBE_oper_i_t {
    unsigned long   num_params;         /* Total # params, incl fn result */
    unsigned long   num_ins;            /* # of marshallable [in] parameters */
    unsigned long   num_srv_ins;        /* num_ins + # server side allocates */
    unsigned long   num_outs;           /* # of marshallable [out] parameters */
    DDBE_vec_rep_t  *ins_type_vec_p;    /* Type vector ptr for [in] params */
    DDBE_vec_rep_t  *outs_type_vec_p;   /* Type vector ptr for [out] params */
    unsigned long   ins_type_index;     /* Type vector index of [in] params */
    unsigned long   outs_type_index;    /* Type vector index of [out] params */
} DDBE_oper_i_t;

/*
 * Data Driven Backend private information for an AST parameter node.
 */
typedef struct DDBE_param_i_t {
    DDBE_vec_rep_t  *type_vec_p;        /* Type vector ptr to start of param */
    unsigned long   type_index;         /* Type vector index for this param */
} DDBE_param_i_t;


/**************************/
/*  Public DDBE routines  */
/**************************/

/*
 * Main routine of Data Driven Backend.  Produces data structures for the
 * type, offset, and routine vectors needed by a Marshalling Interpreter.
 * The public speller support routines take these data structures as input
 * and spell the corresponding vector data.
 */
extern boolean DDBE_main(           /* Main rtn for Data Driven Backend */
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val,  /* [in] array of cmd option values */
    AST_interface_n_t   *int_p,     /* [in] ptr to interface node */
    DDBE_vectors_t      **p_vip     /*[out] vector information pointer */
);


/***********************/
/*  Property routines  */
/***********************/

/*
 *  Returns TRUE if a parameter is any form of an array whose element type
 *  is the specified kind.  If the a_of_a argument is TRUE, array of array
 *  of specified kind returns TRUE, otherwise this case returns FALSE.
 */
extern boolean DDBE_param_is_array_of_kind(
    AST_parameter_n_t   *param_p,   /* [in] Ptr to AST parameter node */
    AST_type_k_t        elem_kind,  /* [in] Array element kind */
    boolean             a_of_a      /* [in] TRUE=>a of a of kind returns TRUE */
);

/*
 *  Returns TRUE if a parameter's local representation is any form of
 *  a conformant array.
 */
extern boolean DDBE_cfmt_arr_local_rep(
    AST_parameter_n_t   *param_p    /* [in] Ptr to AST parameter node */
);


/**********************************************/
/*  Public speller support routines and data  */
/**********************************************/

extern boolean  DDBE_stub_hex;
extern boolean  DDBE_little_endian;
extern long     DDBE_inst_num;

/*
 *  Spells the Interpreter tag that corresponds to a scalar type.
 */
extern void DDBE_spell_type_kind(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vec_rep_t      *vec_p      /* [in] ptr to vector entry */
);

/*
 * Spells an instance declaration of each type that is represented in the
 * offset vector.  This is necessary so that the data is portable.
 */
extern void DDBE_spell_offset_instances(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vectors_t      *vip,       /* [in] vector information pointer */
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val   /* [in] array of cmd option values */
);

/*
 * Spells the offset vector definition.
 */
extern void DDBE_spell_offset_vec(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vectors_t      *vip,       /* [in] vector information pointer */
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val   /* [in] array of cmd option values */
);

/*
 * Spells the offset vector definition.  Requires the instance declarations
 * that are spelt by DDBE_spell_offset_instances.
 */
extern void DDBE_spell_offset_vec_use_inst(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vectors_t      *vip,       /* [in] vector information pointer */
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val   /* [in] array of cmd option values */
);

/*
 *  Spells an uninitialized definition of the offset vector and a routine
 *  IDL_init_offset_vec which must be called once per interface to initialize
 *  the offset vector.  Alternative to using DDBE_spell_offset_vec.
 */
extern void DDBE_init_offset_vec(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vectors_t      *vip,       /* [in] vector information pointer */
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val   /* [in] array of cmd option values */
);

/*
 * Spells the routine vector definition.
 */
extern void DDBE_spell_rtn_vec(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vectors_t      *vip,       /* [in] vector information pointer */
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val,   /* [in] array of cmd option values */
    boolean         client_side     /* [in] T=>client only, F=>server only */
);

/*
 * Spells the type vector definition.
 */
extern void DDBE_spell_type_vec(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vectors_t      *vip,       /* [in] vector information pointer */
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val   /* [in] array of cmd option values */
);

/*
 *  Spell the definition of the parameter vector for an operation.
 */
extern void DDBE_spell_param_vec_def(
    FILE            *fid,           /* [in] output file handle */
    AST_operation_n_t *oper_p,      /* [in] ptr to AST operation node */
    BE_side_t       side,           /* [in] client or server side code */
    boolean         *cmd_opt,       /* [in] array of cmd option flags */
    void            **cmd_val       /* [in] array of cmd option values */
);

/*
 *  Spell the initialization of the parameter vector for an operation.
 */
extern void DDBE_spell_param_vec_init(
    FILE            *fid,           /* [in] output file handle */
    AST_operation_n_t *oper_p,      /* [in] ptr to AST operation node */
    BE_side_t       side,           /* [in] client or server side code */
    boolean         *cmd_opt,       /* [in] array of cmd option flags */
    void            **cmd_val       /* [in] array of cmd option values */
);

/*
 *  Spells the code to marshall or unmarshall the parameters in an operation.
 */
extern void DDBE_spell_marsh_or_unmar(
    FILE            *fid,           /* [in] output file handle */
    AST_operation_n_t *oper_p,      /* [in] ptr to AST operation node */
    char            *interp_name,   /* [in] marshalling interpreter rtn name */
    char            *state_ptr_name,/* [in] name of state pointer variable */
    BE_side_t       side,           /* [in] client or server side code */
    BE_marshalling_k_t mar_or_unmar /* [in] spell marshall or unmarshall code */
);

#endif
