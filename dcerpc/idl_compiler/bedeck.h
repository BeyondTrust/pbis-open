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
**      bedeck.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Decoration structures and definitions
**
**  VERSION: DCE 1.0
**
*/

#ifndef BEDECK_H
#define BEDECK_H

#include <nametbl.h>

typedef unsigned long BE_flags_t;

#define BE_Is_Open_Record(type)\
    ((type)->kind == AST_structure_k && AST_CONFORMANT_SET(type))

#define BE_Is_SP_Pointer(param)\
    ((param)->type->kind == AST_pointer_k &&\
     AST_SELF_POINTER_SET((param)->type->type_structure.pointer->pointee_type))

/*
 * Macro to determine whether parameter is an array of [ref] pointers
 */
#define BE_Is_Arr_of_Refs(param)\
    (param->type->kind == AST_array_k &&\
     param->type->type_structure.array->element_type->kind == AST_pointer_k &&\
     AST_REF_SET(param->type->type_structure.array->element_type))

/*
 * BE_Is_Arrayified macro returns true if all of the following:
 *    1) pt type is pointer
 *    2) pt base type has array_rep_type set
 *    3) param has an arrayification attribute (field_attrs or [string])
 */
#define BE_Is_Arrayified(p,pt) \
      (((pt)->kind == AST_pointer_k) && \
       ((pt)->type_structure.pointer->pointee_type->array_rep_type != NULL) &&    \
       ( ((p)->field_attrs != NULL) || AST_STRING_SET(p) ))

/*
 * BE_array_i_t flags
 */
#define BE_SIMPLE_ARRAY     0x00001 /* make certain marshalling optimizations */

typedef struct BE_array_i_t
{
    BE_flags_t flags;

    boolean decorated;
    boolean element_ptr_var_declared;

    NAMETABLE_id_t index_var;
    NAMETABLE_id_t frag_var;

    NAMETABLE_id_t element_ptr_var;
    struct AST_type_n_t *element_ptr_type;

    NAMETABLE_id_t alloc_var;
    struct AST_type_n_t *alloc_type;
    NAMETABLE_id_t block_var;

    char *first_element_exp;

    NAMETABLE_id_t count_var;
    NAMETABLE_id_t pcount_var;
    NAMETABLE_id_t size_var;
    char *count_exp;
    char *size_exp;

    /*
     * These fields are for 2.0 NDR only.  They are vectors, with a
     * distinct value for each dimension of the array.
     */
    NAMETABLE_id_t *Z;  /* conformance variables */
    NAMETABLE_id_t *A;  /* variance variables */
    NAMETABLE_id_t *B;
    char **Z_exps;
    char **A_exps;
    char **B_exps;

    int num_elts;           /* for non-conformant arrays only */
    int ndr_elt_size;       /* the ndr size of an element of the array */
    int loop_nest_count;    /* the number of nested loops to be spelt when
                                (un)marshalling the array */

    struct AST_parameter_n_t *flat_elt;  /* flattened array element */
    struct AST_parameter_n_t *flat_base_elt;  /* points to same structure as
                        flat_elt unless flat_elt is an [out_of_line] array type,
                        in which case it points to a flattened element of
                        the ultimate array base type */
    struct AST_parameter_n_t *original;  /* link from REF_PASS to first pass */
} BE_array_i_t;

/*
 * Conformant structure info
 */
typedef struct BE_orecord_i_t
{
    NAMETABLE_id_t alloc_var;             /* name of thing to malloc */
    struct AST_type_n_t *alloc_type;      /* type of thing to malloc */
    struct AST_type_n_t *alloc_typep;     /* type pointer to above type */
    struct AST_parameter_n_t *open_array; /* pointer to open array field */
} BE_orecord_i_t;

/*
 * BE_arm_t flags
 */
#define BE_POINTERS     0x00001  /* are there any pointer fields in this arm */

/*
 * Flattened union arm
 */
typedef struct BE_arm_t
{
    BE_flags_t flags;

    struct AST_case_label_n_t *labels;
    struct AST_parameter_n_t *flat;    /* flattened parameter */
    struct BE_arm_t *referred_to_by;   /* Used in the arms of a BE_REF_PASS
            union object to find the arm which gave rise to these pointers */

    struct BE_arm_t *next;
    struct BE_arm_t *last;
} BE_arm_t;

/*
 * Pipe info
 */
typedef struct BE_pipe_i_t
{
    boolean decorated;
    struct AST_parameter_n_t *flat;    /* conformant array of base type */
    struct AST_field_attr_n_t *attrs;  /* generated field attributes */
} BE_pipe_i_t;

/*
 * Discriminated union info
 */
typedef struct BE_du_i_t
{
    struct AST_parameter_n_t *discrim; /* flattened disciminator */
    struct BE_arm_t *arms;             /* list of flattened arms */

    int vec_size;                      /* slots required for this param */
} BE_du_i_t;

/*
 * Pointer info
 */
typedef struct BE_ptr_i_t
{
#if 0
    struct AST_parameter_n_t *pointee; /* flattened pointee */
    int pointee_slots;                 /* slots occupied by the pointee */
#endif
    struct AST_parameter_n_t *flat_array; /* flattened original type of [ptr]
                                             arrays */
} BE_ptr_i_t;

/*
 * OOL info
 */
typedef struct BE_ool_i_t
{
    struct AST_type_n_t *type;  /* original unflattened type of param */
    NAMETABLE_id_t name;        /* name to use for ool param */
    struct AST_type_n_t *call_type;  /* Type to be used when generating call
                                        to ool routine */
    NAMETABLE_id_t call_name;        /* Parameter name to be used when
                                        generating call to ool routine */
    boolean any_pointers;       /* param's type contains pointers */
    boolean use_P_rtn;          /* invoke ool pointer routine */
    boolean has_calls_before;   /* xmit_as or rep_as type */
    boolean top_level;          /* object being OOLed is top-level param */
} BE_ool_i_t;

/*
 * Type info
 */
typedef struct BE_type_i_t
{
    struct AST_type_n_t *clone;        /* named clone for anonymous pa types */
} BE_type_i_t;

/*
 * Call record types
 */
typedef enum
{
    BE_call_xmit_as,      /* calls associated with [transmit_as] */
    BE_call_rep_as,       /* calls associated with [represent_as] */
    BE_call_ctx_handle,   /* calls associated with [context_handle] */
    BE_init_ptr_to_array, /* action associated with [ptr] array parameters */
    BE_init_ptr_to_conf   /* action assoiciated with ptr to conformant
                             array parameters (such as strings) */
} BE_call_type_t;

/*
 * Ordered list of calls to invoke before or after parameter marshalling
 */
typedef struct BE_call_rec_t
{
    struct BE_call_rec_t *next;
    struct BE_call_rec_t *last;

    BE_call_type_t type;                      /* union discriminator */

    union
    {
        /*
         * type == BE_call_xmit_as
         */
        struct
        {
            NAMETABLE_id_t native_name;       /* presented parameter name */
            struct AST_type_n_t *native_type; /* presented parameter type */

            NAMETABLE_id_t xmit_name;         /* unmarshalling xmissible name */
            NAMETABLE_id_t pxmit_name;        /* marshalling xmissible name */

            struct AST_type_n_t *xmit_type;   /* unmarshalling xmissible type */

        } xmit_as;

        /*
         * type == BE_call_rep_as
         */
        struct
        {
            NAMETABLE_id_t local_name;        /* local parameter name */

            NAMETABLE_id_t net_name;          /* unmarshalling network name */
            NAMETABLE_id_t pnet_name;         /* marshalling network name */

            struct AST_type_n_t *net_type;    /* unmarshalling network type */
        } rep_as;

        /*
         * type == BE_call_ctx_handle
         */
        struct
        {
            NAMETABLE_id_t native_name;       /* void * parameter name */
            struct AST_type_n_t *native_type; /* original parameter type */

            NAMETABLE_id_t wire_name;         /* ndr_context_handle param */

            boolean rundown;
        } ctx_handle;

        /*
         * type == BE_init_ptr_to_array
         */
        struct
        {
            NAMETABLE_id_t param_name;        /* array parameter's name */

            NAMETABLE_id_t ptr_name;          /* name of ptr to array */
            struct AST_type_n_t *ptr_type;    /* (ptr to array) type */
        } ptr_to_array;

        /*
         * type == BE_init_ptr_to_conf
         */
        struct
        {
            NAMETABLE_id_t param_name;         /* pointer parameter's name */

            NAMETABLE_id_t pointee_name;       /* helper var name */
            struct AST_type_n_t *param_type;   /* param's type */
        } ptr_to_conf;
    } call;

} BE_call_rec_t;

/*
 * BE_param_i_t flags
 */

/*
 * Special backend parameter types
 */
#define BE_OPEN_RECORD  0x00000001  /* magic open record header*/
#define BE_ARRAY_HEADER 0x00000002  /* magic array header*/

#define BE_ALIGN_MP     0x00000004  /* align mp before this parameter */
#define BE_SYNC_MP      0x00000008  /* synchronize mp with offset pointer */
#define BE_NEW_SLOT     0x00000010  /* does this parameter require a new slot */
#define BE_FIELD        0x00000020  /* was this parameter originally a field */
#define BE_CHECK_BUFFER 0x00000040  /* unmarshalling: check for new comm_receive */
#define BE_ALLOCATE     0x00000080  /* allocate this parameter when unmrshlling */
#define BE_IN_ORECORD   0x00000100  /* this open array is in an open record */
#define BE_ADVANCE_MP   0x00000200  /* advance mp after this parameter */
#define BE_NEW_BLOCK    0x00000400  /* this parameter is the first in a block */
#define BE_DEFER        0x00000800  /* for non-[ref] pointers within structs */
#define BE_STRING0      0x00001000  /* for 1-d [string0] arrays of char */
#define BE_FATTRS_FLAT  0x00002000  /* param's field attrs. have been flattened */
#define BE_REF_PASS     0x00004000  /* marshall referents of pointer array elts. */
#define BE_PARRAY       0x00008000  /* pointed-at array decoration/marshalling */
#define BE_LAST_FIELD   0x00010000  /* last field of a conformant structure */
#define BE_PTR_ARRAY    0x00020000  /* this pointer used to be a [ptr] array */
#define BE_ARRAYIFIED   0x00040000  /* this pointer to an array used to be a scalar pointer */
#define BE_PTR2STR      0x00080000  /* this is a pointer to a [string] array */
#define BE_OOL          0x00100000  /* a param to be marshalled out-of-line */
#define BE_SKIP         0x00200000  /* don't marshall this param at all */
#define BE_OOL_HEADER   0x00400000  /* a header for an OOL parameter */
#define BE_XMITCFMT     0x00800000  /* conformant xmit/repas net type */
#define BE_HDRLESS      0x01000000  /* array has had it header pruned away */
#define BE_OOL_YANK_ME  0x02000000  /* an OOL param for which a "use_p" clone is needed */
#define BE_ARR_OF_STR   0x04000000  /* an array whose elements are strings */
#define BE_NEU_DISCRIM  0x80000000  /* discriminant of a non-encapsulated union */

typedef struct BE_param_i_t
{
    BE_flags_t flags;

    /*
     * The only significant fields in the BE_param_i_t structure of a
     * pristine (unflattened) parameter are the following two:
     */
    struct AST_parameter_n_t *flat; /* pointer to flattened counterpart */
    NAMETABLE_id_t name;            /* IDL-generated name for parameter */

    BE_array_i_t *array_info;       /* etc. for array parameters */
    BE_orecord_i_t *orecord_info;   /*  "    "  conformant struct parameters */
    BE_du_i_t *du_info;             /*  "    "  discriminated unions params*/
    BE_ptr_i_t *ptr_info;           /*  "    "  non-ref pointer parameters */
    BE_ool_i_t *ool_info;           /*  "    "  parameters of [ool] type */

    BE_call_rec_t *call_before;     /* functions to call pre-un/marshalling */
    BE_call_rec_t *call_after;      /*     "      "   "   post-un/marshalling */
} BE_param_i_t;

#define BE_PI_Flags(p)\
    ((p)->be_info.param->flags)

#define BE_Array_Info(p)\
    ((p)->be_info.param->array_info)

#define BE_Orecord_Info(p)\
    ((p)->be_info.param->orecord_info)

#define BE_Open_Array(p)\
    (BE_Orecord_Info(p)->open_array)

#define BE_Du_Info(p)\
    ((p)->be_info.param->du_info)

#define BE_Ptr_Info(p)\
    ((p)->be_info.param->ptr_info)

#define BE_Ool_Info(p)\
    ((p)->be_info.param->ool_info)

#define BE_Calls_Before(p)\
    ((p)->be_info.param->call_before)

#define BE_Calls_After(p)\
    ((p)->be_info.param->call_after)

typedef struct BE_field_i_t
{
    struct AST_parameter_n_t *flat; /* field's flat counterpart */
} BE_field_i_t;

/*
 * A pointer initialization node for server side [ref] pointer surrogates
 */
typedef struct BE_ptr_init_t
{
    boolean heap;                      /* allocate on heap */

    NAMETABLE_id_t pointer_name;
    struct AST_type_n_t *pointer_type; /* only used if (heap) */

    NAMETABLE_id_t pointee_name;       /* only used if (!heap) */
    struct AST_type_n_t *pointee_type; /* only used if (heap) */

    struct BE_ptr_init_t *next;
    struct BE_ptr_init_t *last;
} BE_ptr_init_t;

/*
 * BE_oper_i_t flags
 */
#define BE_MAINTAIN_OP   0x00001  /* maintain offset pointer */
#define BE_BUFF_BOOL     0x00002  /* currently unused--any arrays of structs */
#define BE_HELPERS_EXIST 0x00004  /* Helper variables already declared */

typedef struct BE_oper_i_t
{
    BE_flags_t flags;
    struct AST_parameter_n_t *flat_params;  /* flattened parameter list */
    struct BE_param_blk_t *sends;           /* parameter block list */
    struct AST_parameter_n_t *recs;         /* parameter block list */
    struct BE_local_var_t *local_vars;      /* local variables */
    struct BE_ptr_init_t *pointers;         /* pointer initialization list */

    int vec_size;                           /* size of marshalling iovector */
    int pkt_size;                           /* size of stack packet */
    int next_local_var;                     /* obsolete */
} BE_oper_i_t;

#define BE_Flat_Params(o) ((o)->be_info.oper->flat_params)
#define BE_Sends(o)       ((o)->be_info.oper->sends)
#define BE_Recs(o)        ((o)->be_info.oper->recs)
#define BE_Pointers(o)    ((o)->be_info.oper->pointers)

/*
 * BE_param_blk_t flags
 */
#define BE_SP_BLOCK     0x00001  /* block contains only a self-pointing param */
#define BE_PTR_BLOCK    0x00002  /* block contains only a pointer param */
#define BE_FIRST_BLOCK  0x00004  /* this is the first marshalling block */
#define BE_PRUNED_BLOCK 0x00008  /* block contains no marshallable params */

typedef struct BE_param_blk_t
{
    BE_flags_t flags;
    struct AST_parameter_n_t *params;

    struct BE_param_blk_t *next; /* next block in this direction */
    struct BE_param_blk_t *last; /* last block in this direction */

    int vec_size;                /* number of iovector slots required */
} BE_param_blk_t;

#endif
