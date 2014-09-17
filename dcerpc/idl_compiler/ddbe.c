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
**      ddbe.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Data Driven Backend - Generates data structures that hold the vector
**  information needed by a Marshalling Interpreter.
**
*/

#include <ddbe.h>
#include <astp.h>
#include <command.h>
#include <message.h>
#include <mtsbacke.h>
#include <nidlmsg.h>

#define DDBE_SUFFIX_TO_XMIT     "_to_xmit"
#define DDBE_SUFFIX_FROM_XMIT   "_from_xmit"
#define DDBE_SUFFIX_FREE_INST   "_free_inst"
#define DDBE_SUFFIX_FREE_XMIT   "_free_xmit"

#define DDBE_SUFFIX_FROM_LOCAL  "_from_local"
#define DDBE_SUFFIX_TO_LOCAL    "_to_local"
#define DDBE_SUFFIX_FREE_LOCAL  "_free_local"
#define DDBE_SUFFIX_FREE_INST   "_free_inst"

#define DDBE_SUFFIX_NET_SIZE    "_net_size"
#define DDBE_SUFFIX_TO_NETCS    "_to_netcs"
#define DDBE_SUFFIX_LOCAL_SIZE  "_local_size"
#define DDBE_SUFFIX_FROM_NETCS  "_from_netcs"

/* Necessary forward function declarations */

#ifdef DUMPERS
static void DDBE_test_marsh_spellers(
    AST_interface_n_t   *int_p,     /* [in] ptr to interface node */
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val   /* [in] array of cmd option values */
);
#endif

/*
 * Global that determines whether integers are spelled in hexadecimal.
 */
boolean DDBE_stub_hex = FALSE;

/*
 * Global that determines whether integers are spelled byte-by-byte in
 * either big-endian or little-endian order.
 */
boolean DDBE_little_endian = TRUE;

/*
 * This cell is used to assign unique names to spelled instances of types.
 */
long DDBE_inst_num = 0;

/*
 * Symbolic names used in stub files for Interpreter tags.
 */
static NAMETABLE_id_t
    DDBE_dt_allocate,
    DDBE_dt_allocate_ref,
    DDBE_dt_begin_nested_struct,
    DDBE_dt_boolean,
    DDBE_dt_byte,
    DDBE_dt_char,
    DDBE_dt_conf_array,
    DDBE_dt_conf_struct,
    DDBE_dt_cs_array,
    DDBE_dt_cs_attribute,
    DDBE_dt_cs_rlse_shadow,
    DDBE_dt_cs_shadow,
    DDBE_dt_cs_type,
    DDBE_dt_deleted_nodes,
    DDBE_dt_does_not_exist,
    DDBE_dt_double,
    DDBE_dt_enc_union,
    DDBE_dt_end_nested_struct,
    DDBE_dt_enum,
    DDBE_dt_eol,
    DDBE_dt_error_status,
    DDBE_dt_first_is_limit,
    DDBE_dt_first_is_early_limit,
    DDBE_dt_fixed_array,
    DDBE_dt_fixed_bound,
    DDBE_dt_fixed_limit,
    DDBE_dt_fixed_struct,
    DDBE_dt_float,
    DDBE_dt_free_rep,
    DDBE_dt_full_ptr,
    DDBE_dt_hyper,
    DDBE_dt_ignore,
    DDBE_dt_interface,
    DDBE_dt_in_context,
    DDBE_dt_in_out_context,
    DDBE_dt_last_is_limit,
    DDBE_dt_last_is_early_limit,
    DDBE_dt_length_is_limit,
    DDBE_dt_length_is_early_limit,
    DDBE_dt_long,
    DDBE_dt_max_is_bound,
    DDBE_dt_max_is_early_bound,
    DDBE_dt_min_is_bound,
    DDBE_dt_min_is_early_bound,
    DDBE_dt_modified,
    DDBE_dt_ndr_align_2,
    DDBE_dt_ndr_align_4,
    DDBE_dt_ndr_align_8,
    DDBE_dt_n_e_union,
    DDBE_dt_open_array,
    DDBE_dt_out_context,
    DDBE_dt_passed_by_ref,
    DDBE_dt_pipe,
    DDBE_dt_range,
    DDBE_dt_ref_ptr,
    DDBE_dt_represent_as,
    DDBE_dt_size_is_bound,
    DDBE_dt_size_is_early_bound,
    DDBE_dt_small,
    DDBE_dt_short,
    DDBE_dt_string,
    DDBE_dt_string_bound,
    DDBE_dt_string_limit,
    DDBE_dt_transmit_as,
    DDBE_dt_unique_ptr,
    DDBE_dt_uhyper,
    DDBE_dt_ulong,
    DDBE_dt_upper_conf_limit,
    DDBE_dt_ushort,
    DDBE_dt_usmall,
    DDBE_dt_v1_array,
    DDBE_dt_v1_conf_struct,
    DDBE_dt_v1_enum,
    DDBE_dt_v1_string,
    DDBE_dt_varying_array,
    DDBE_dt_void;

static void DDBE_func_code_vec_entry(
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
	byte func_code /* function code */
	);


/*
 * Macro to add a string to the stringtable and store the resulting strtab ID.
 * If DUMPERS not defined comments are not processed, so store null strtab ID.
 */
#ifdef DUMPERS
#define DDBE_STRTAB_ENTRY(dest, comment)\
    if (comment == NULL)\
        dest = STRTAB_NULL_STR;\
    else\
        dest = STRTAB_add_string(comment)
#else
#define DDBE_STRTAB_ENTRY(dest, comment)\
    dest = STRTAB_NULL_STR
#endif

/*
 * Macro to allocate and initialize a vector list entry and insert into list
 * after current element.  The list pointer is left pointing at the new entry.
 */
#define DDBE_NEW_ENTRY(new_p, p_defn_p, _comment)\
{\
    new_p = NEW (DDBE_vec_rep_t);\
    DDBE_STRTAB_ENTRY(new_p->comment, _comment);\
    /* Insert new entry after current element (p_defn_p) and make it current */\
    new_p->next    = (*(p_defn_p))->next;\
    (*(p_defn_p))->next = new_p;\
    *(p_defn_p) = new_p;\
}


/*
 * Allocate and initialize a sentinel entry in a vector list.
 * Using sentinels simplifies the list manipulation.
 */
#define DDBE_VEC_SENTINEL(vec_rep_p) \
{ \
    (vec_rep_p) = NEW (DDBE_vec_rep_t); \
    (vec_rep_p)->kind = DDBE_vec_noop_k; \
    (vec_rep_p)->next = NULL; \
}


/*
 * Initialize the vector information for a new parameter.  Note that not all
 * fields are initialized since some have scope across parameters, such as
 * the vector lists.
 */
#define DDBE_VEC_INIT(vip) \
    vip->p_cur_p        = &vip->type_p; \
    vip->conformant     = FALSE;        \
    vip->saved_defn_p   = NULL;         \
    vip->in_cfmt_struct_hdr = FALSE;    \
    vip->cfmt_info_tup_p    = NULL;     \
    vip->in_flatarr     = FALSE;        \
    vip->update_tup     = TRUE;

/*****************************/
/*  Public utility routines  */
/*****************************/

/*
 *  D D B E _ p a r a m _ i s _ a r r a y _ o f _ k i n d
 *
 *  Returns TRUE if a parameter is any form of an array whose element type
 *  is the specified kind.  If the a_of_a argument is TRUE, array of array
 *  of specified kind returns TRUE, otherwise this case returns FALSE.
 */
boolean DDBE_param_is_array_of_kind
(
    AST_parameter_n_t   *param_p,   /* [in] Ptr to AST parameter node */
    AST_type_k_t        elem_kind,  /* [in] Array element kind */
    boolean             a_of_a      /* [in] TRUE=>a of a of kind returns TRUE */
)
{
    AST_type_n_t        *type_p;

    if (param_p->type->kind == AST_array_k)
        type_p = param_p->type->type_structure.array->element_type;
    else if (DDBE_param_is_p_array(param_p))
        type_p = param_p->type->type_structure.pointer->pointee_type->
                 type_structure.array->element_type;
    else if (DDBE_ARRAYIFIED(param_p))
        type_p = param_p->type->type_structure.pointer->pointee_type;
    else
        return FALSE;

    if (type_p->kind == AST_array_k && a_of_a)
        do {
            type_p = type_p->type_structure.array->element_type;
        } while (type_p->kind == AST_array_k);

    if (type_p->kind == elem_kind)
        return TRUE;

    return FALSE;
}


/*
 *  D D B E _ s p e l l _ t y p e _ k i n d
 *
 *  Spells the Interpreter tag that corresponds to a scalar type.
 */
void DDBE_spell_type_kind
(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vec_rep_t      *vec_p      /* [in] ptr to vector entry */
)
{
    NAMETABLE_id_t      nameid = 0;     /* Interpreter tag name ID */
    char const          *name;      /* Interpreter tag name string */
    AST_type_n_t        *type_p;
    char const          *type_name;
#ifdef DUMPERS
    char const                *comment;   /* Comment for entry */
#endif

    if (vec_p->kind != DDBE_vec_type_kind_k)
    {
        INTERNAL_ERROR("Unexpected vector entry kind");
        return;
    }

    switch (vec_p->val.type_p->kind)
    {
    case AST_boolean_k:
        nameid = DDBE_dt_boolean;
        break;
    case AST_byte_k:
        nameid = DDBE_dt_byte;
        break;
    case AST_character_k:
        nameid = DDBE_dt_char;
        break;
    case AST_small_integer_k:
        nameid = DDBE_dt_small;
        break;
    case AST_short_integer_k:
        nameid = DDBE_dt_short;
        break;
    case AST_long_integer_k:
        nameid = DDBE_dt_long;
        break;
    case AST_hyper_integer_k:
        nameid = DDBE_dt_hyper;
        break;
    case AST_small_unsigned_k:
        nameid = DDBE_dt_usmall;
        break;
    case AST_short_unsigned_k:
        nameid = DDBE_dt_ushort;
        break;
    case AST_long_unsigned_k:
        if (vec_p->val.type_p->name != NAMETABLE_NIL_ID)
        {
            type_p = vec_p->val.type_p;
            while (type_p->defined_as != NULL)
                type_p = type_p->defined_as;
            NAMETABLE_id_to_string(type_p->name, &type_name);
            if (strcmp(type_name, "error_status_t") == 0)
            {
                nameid = DDBE_dt_error_status;
                break;
            }
        }
        nameid = DDBE_dt_ulong;
        break;
    case AST_hyper_unsigned_k:
        nameid = DDBE_dt_uhyper;
        break;
    case AST_short_float_k:
        nameid = DDBE_dt_float;
        break;
    case AST_long_float_k:
        nameid = DDBE_dt_double;
        break;
    case AST_enum_k:
        if (AST_V1_ENUM_SET(vec_p->val.type_p))
            nameid = DDBE_dt_v1_enum;
        else
            nameid = DDBE_dt_enum;
        break;
    default:
        INTERNAL_ERROR("Unexpected type kind");
    }

    NAMETABLE_id_to_string(nameid, &name);

#ifdef DUMPERS
    if (vec_p->comment == STRTAB_NULL_STR)
#endif
        fprintf(fid, "%s,\n", name);
#ifdef DUMPERS
    else
    {
        STRTAB_str_to_string(vec_p->comment, &comment);
        fprintf(fid, "%s,\t/* %s */\n", name, comment);
    }
#endif
}

/*************************************/
/*  Vector entry insertion routines  */
/*************************************/

/*
 *  D D B E _ t a g _ v e c _ e n t r y
 *
 *  Insert a tag vector entry into a vector list.
 */
static void DDBE_tag_vec_entry
(
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
    NAMETABLE_id_t  tag_name        /* [in] ID of symbolic name of tag */
)
{
    DDBE_vec_rep_t  *new_p;

    DDBE_NEW_ENTRY(new_p, p_defn_p, NULL);
    new_p->kind     = DDBE_vec_tag_k;
    new_p->val.name = tag_name;
}

/*
 *  D D B E _ b y t e _ v e c _ e n t r y
 *
 *  Insert a byte vector entry into a vector list.
 */
static void DDBE_byte_vec_entry
(
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
    DDBE_vec_kind_t entry_kind,     /* [in] Kind of byte value entry */
    byte            value,          /* [in] Byte value */
    char            *comment ATTRIBUTE_UNUSED        /* [in] Comment string for entry */
)
{
    DDBE_vec_rep_t  *new_p;

    DDBE_NEW_ENTRY(new_p, p_defn_p, comment);
    new_p->kind         = entry_kind;
    new_p->val.byte_val = value;
}
/*
 *  D D B E _ s h o r t _ v e c _ e n t r y
 *
 *  Insert a short vector entry into a vector list.
 */
static void DDBE_short_vec_entry
(
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
    DDBE_vec_kind_t entry_kind,     /* [in] Kind of short value entry */
    short           value,          /* [in] Short value */
    char            *comment  __attribute__((unused))      /* [in] Comment string for entry */
)
{
    DDBE_vec_rep_t  *new_p;

    DDBE_NEW_ENTRY(new_p, p_defn_p, comment);
    new_p->kind         = entry_kind;
    new_p->val.short_val= value;
}

/*
 *  D D B E _ l o n g _ v e c _ e n t r y
 *
 *  Insert a long vector entry into a vector list.
 */
static void DDBE_long_vec_entry
(
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
    DDBE_vec_kind_t entry_kind,     /* [in] Kind of long value entry */
    long            value,          /* [in] Long value */
    char            *comment ATTRIBUTE_UNUSED        /* [in] Comment string for entry */
)
{
    DDBE_vec_rep_t  *new_p;

    DDBE_NEW_ENTRY(new_p, p_defn_p, comment);
    new_p->kind         = entry_kind;
    new_p->val.long_val = value;
}

/*
 *  D D B E _ c o m m e n t _ v e c _ e n t r y
 *
 *  Insert a 'block comment' vector entry into a vector list.  This entry
 *  contains only a comment and can be used to delimit major vector sections.
 */
static void DDBE_comment_vec_entry
(
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
    char            *comment ATTRIBUTE_UNUSED       /* [in] Comment string for entry */
)
{
    DDBE_vec_rep_t  *new_p;

    DDBE_NEW_ENTRY(new_p, p_defn_p, comment);
    new_p->kind     = DDBE_vec_comment_k;
}

/*
 *  D D B E _ e x p r _ v e c _ e n t r y
 *
 *  Insert an expression vector entry into a vector list.
 */
static void DDBE_expr_vec_entry
(
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
    DDBE_vec_kind_t expr_kind,      /* [in] Kind of expression */
    STRTAB_str_t    expr,           /* [in] Expression */
    char            *comment ATTRIBUTE_UNUSED       /* [in] Comment string for entry */
)
{
    DDBE_vec_rep_t  *new_p;

    DDBE_NEW_ENTRY(new_p, p_defn_p, comment);
    new_p->kind     = expr_kind;
    new_p->val.expr = expr;
}

/*
 *  D D B E _ t y p e _ i n f o _ v e c _ e n t r y
 *
 *  Insert a type information vector entry into a vector list.
 */
static void DDBE_type_info_vec_entry
(
    DDBE_vec_kind_t kind,           /* [in] Type info entry kind */
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
    AST_type_n_t    *type_p,        /* [in] Ptr to AST type node */
    char            *comment ATTRIBUTE_UNUSED       /* [in] Comment string for entry */
)
{
    DDBE_vec_rep_t  *new_p;

    DDBE_NEW_ENTRY(new_p, p_defn_p, comment);
    new_p->kind       = kind;
    new_p->val.type_p = type_p;
}

/*
 *  D D B E _ n a m e _ v e c _ e n t r y
 *
 *  Insert an name vector entry into a vector list.
 */
static void DDBE_name_vec_entry
(
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
    NAMETABLE_id_t  name,           /* [in] Name ID */
    char            *comment ATTRIBUTE_UNUSED       /* [in] Comment string for entry */
)
{
    DDBE_vec_rep_t  *new_p;

    DDBE_NEW_ENTRY(new_p, p_defn_p, comment);
    new_p->kind     = DDBE_vec_name_k;
    new_p->val.name = name;
}
/*
 *  D D B E _ o p t _ n a m e _ v e c _ e n t r y
 *
 *  Insert an name vector entry into a vector list.  Same as above routine
 *  expect has a flag which makes it only used on the client xor server side.
 */
#if 0
static void DDBE_opt_name_vec_entry
(
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
    NAMETABLE_id_t  name,           /* [in] Name ID */
    char            *comment  __attribute__((unused)),       /* [in] Comment string for entry */
    boolean         client_side     /* [in] T=>client only, F=>server only */
)
{
    DDBE_vec_rep_t  *new_p;

    DDBE_NEW_ENTRY(new_p, p_defn_p, comment);
    new_p->kind = (client_side ? DDBE_vec_name_client_k : DDBE_vec_name_server_k);
    new_p->val.name = name;
}
#endif
/*
 *  D D B E _ p a d _ v e c _ e n t r y
 *
 *  Insert a pad vector entry into a vector list.  A pad vector entry marks the
 *  need to add pad bytes within the vector definition itself - it does NOT
 *  mark the need for the Interpreter to insert filler bytes to accomplish
 *  alignment at runtime.
 */
static void DDBE_pad_vec_entry
(
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
    byte            pad             /* [in] Number of pad bytes */
)
{
    DDBE_vec_rep_t  *new_p;

    DDBE_NEW_ENTRY(new_p, p_defn_p, NULL);
    new_p->kind         = DDBE_vec_pad_k;
    new_p->val.byte_val = pad;
}

/*
 *  D D B E _ n o o p _ v e c _ e n t r y
 *
 *  Insert a no-operation entry into a vector list.
 *  Noop's are useful as sentinels or placeholders.
 */
static void DDBE_noop_vec_entry
(
    DDBE_vec_rep_t  **p_defn_p      /* [io] Ptr to vec entry to insert after */
)
{
    DDBE_vec_rep_t  *new_p;

    DDBE_NEW_ENTRY(new_p, p_defn_p, NULL);
    new_p->kind = DDBE_vec_noop_k;
}

/*
 *  D D B E _ s c a l a r _ v e c _ e n t r y
 *
 *  Insert a tag vector entry for a scalar type into a vector list.
 *  The comment for the entry is formed by concatenating the type name
 *  (if any) and the passed instance expression.
 */
static void DDBE_scalar_vec_entry
(
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
    AST_type_n_t    *type_p,        /* [in] Ptr to AST scalar type node */
    char const      *inst_expr,     /* [in] Instance expression */
    DDBE_vectors_t  *vip ATTRIBUTE_UNUSED           /* [in] vector information ptr */
)
{
    DDBE_vec_rep_t  *new_p;         /* Ptr to new vector entry */
    char const      *type_name;     /* Scalar type name, if any */
    char const      *comment ATTRIBUTE_UNUSED;       /* Comment string */
    char            comment_buf[DDBE_MAX_COMMENT];

    comment_buf[0] = '\0';

    if (type_p->name != NAMETABLE_NIL_ID)
    {
        NAMETABLE_id_to_string(type_p->name, &type_name);
        strcat(comment_buf, type_name);
        if (inst_expr != NULL)
            strcat(comment_buf, " ");
    }

    if (inst_expr != NULL)
        strcat(comment_buf, inst_expr);
    else
        inst_expr = "byte or char data"; /* set generic expr for msg below */

    comment = (comment_buf[0] == '\0') ? NULL : comment_buf;

    /* Generate scalar type kind tag */
    DDBE_NEW_ENTRY(new_p, p_defn_p, comment);
    new_p->kind       = DDBE_vec_type_kind_k;
    new_p->val.type_p = type_p; 
}

/*
 *  D D B E _ r e f e r e n c e _ v e c _ e n t r y
 *
 *  Insert an indirect reference vector entry into a vector list.
 *  The entry simply points at another vector entry.  A subsequent pass will
 *  process the indirect entries and compute actual vector indices.
 *
 *  Contrast with the DDBE_indirect_vec_entry routine below.
 */
static void DDBE_reference_vec_entry
(
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
    DDBE_vec_rep_t  *ref_p,         /* [in] Referenced vector entry */
    char            *comment ATTRIBUTE_UNUSED       /* [in] Comment string for entry */
)
{
    DDBE_vec_rep_t  *new_p;

    DDBE_NEW_ENTRY(new_p, p_defn_p, comment);
    new_p->kind         = DDBE_vec_reference_k;
    new_p->val.ref_p    = ref_p;
}

/*
 *  D D B E _ i n d i r e c t _ v e c _ e n t r y
 *
 *  Insert an indirect reference vector entry into a vector list.
 *  The entry simply points at another vector entry.  A subsequent pass will
 *  process the indirect entries and compute actual vector indices.
 *
 *  This routine is like DDBE_reference_vec_entry but provides an extra level
 *  of indirection.  This allows one to store the _address_ of a cell that will
 *  eventually hold the indirect reference but does not yet.
 */
static void DDBE_indirect_vec_entry
(
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
    DDBE_vec_rep_t  **p_ref_p,      /* [in] Addr of referenced vector entry */
    char            *comment ATTRIBUTE_UNUSED        /* [in] Comment string for entry */
)
{
    DDBE_vec_rep_t  *new_p;

    DDBE_NEW_ENTRY(new_p, p_defn_p, comment);
    new_p->kind         = DDBE_vec_indirect_k;
    new_p->val.p_ref_p  = p_ref_p;
}

/******************************/
/*  Private support routines  */
/******************************/

/*
 *  D D B E _ c o m p u t e _ v e c _ o f f s e t s
 *
 *  Computes the vector offset for each vector entry in a list.
 *  Returns the maximum (0-based) offset used, i.e. the size of the vector.
 */
static unsigned long DDBE_compute_vec_offsets
(
    DDBE_vec_rep_t      *vec_p,     /* [in] ptr to vector entry list */
    unsigned long       align_req,  /* [in] size of a multi-byte entry */
    unsigned long       index_incr, /* [in] index incr for multi-byte entry */
    unsigned long       start_index,/* [in] starting index for vector data */
    DDBE_vec_rep_t      **end_vec_p /* [io] != NULL => return last vec addr */
)
{
    unsigned long   offset;         /* Offset to (index of) current entry */
    unsigned long   save_offset;    /* Saved offset value */
    unsigned long   align_val;      /* Max bytes required for alignment */
    DDBE_vec_rep_t  *prev_vec_p;    /* Ptr to previous vector entry needed */
                                    /*  to insert pad entry for alignment  */
    /*
     * Start offset at passed starting index.
     */
    offset = start_index;
    align_val = align_req - 1;
    prev_vec_p = NULL;  /* OK since first entry always sentinel */

    while (vec_p != NULL)
    {
        switch (vec_p->kind)
        {
        case DDBE_vec_byte_k:
        case DDBE_vec_expr_byte_k:
        case DDBE_vec_tag_k:
        case DDBE_vec_type_kind_k:
            /*
             * These entries always occupy a single byte.
             */
            vec_p->index = offset;
            offset++;
            break;

        case DDBE_vec_byte_3m4_k:
            /*
             * This entry occupies a single byte but it must be made to
             * start on a 3 mod 4 boundary.
             */
            save_offset = offset;
            offset = offset | 3;
            if (offset != save_offset)
            {
                DDBE_pad_vec_entry(&prev_vec_p, (byte)offset-save_offset);
                /* prev_vec_p now points at the new pad entry */
                prev_vec_p->index = save_offset;
            }
            vec_p->index = offset;
            offset++;
            break;
        case DDBE_vec_short_k:
            /* Assumed to only be used in byte-based arrays */
            save_offset = offset;
            offset = (offset + 1) & ~1;
            if (offset != save_offset)
            {
                DDBE_pad_vec_entry(&prev_vec_p, (byte)(offset-save_offset));
                /* prev_vec_p now points at the new pad entry */
                prev_vec_p->index = save_offset;
            }
            vec_p->index = offset;
            offset += 2;
            break;


        case DDBE_vec_expr_arr_k:
        case DDBE_vec_expr_k:
        case DDBE_vec_expr_long_k:
        case DDBE_vec_indirect_k:
        case DDBE_vec_long_k:
        case DDBE_vec_long_bool_k:
        case DDBE_vec_name_k:
        case DDBE_vec_name_client_k:
        case DDBE_vec_name_server_k:
        case DDBE_vec_reference_k:
        case DDBE_vec_sizeof_k:
            /*
             * Integer entries.  First, generate pad entry if necessary as a
             * result of alignment requirements.  Then assign index to entry
             * and bump offset by index base.
             */
            if (index_incr != 1)
            {
                save_offset = offset;
                offset = (offset + align_val) & ~align_val;
                if (offset != save_offset)
                {
                    DDBE_pad_vec_entry(&prev_vec_p, (byte)(offset-save_offset));
                    /* prev_vec_p now points at the new pad entry */
                    prev_vec_p->index = save_offset;
                }
            }
            vec_p->index = offset;
            offset += index_incr;
            break;

        case DDBE_vec_noop_k:
        case DDBE_vec_comment_k:
        case DDBE_vec_offset_begin_k:
        case DDBE_vec_offset_end_k:
            /*
             * These entries are placeholders and do not result in vector entry.
             * However, noop entries can be referenced indirectly and must have
             * a valid index.
             */
            vec_p->index = offset;
            break;

        case DDBE_vec_pad_k:
            /*
             * A pad entry says to pad the vector with a number of filler bytes.
             * Update the offset by the number of filler bytes.
             */
            vec_p->index = offset;
            offset += vec_p->val.byte_val;
            break;

        default:
            INTERNAL_ERROR("Invalid vector entry kind");
        }

        prev_vec_p = vec_p;
        vec_p = vec_p->next;
    }

    /*
     * Return ending offset without accounting for a terminating sentinel entry.
     */
    if (end_vec_p != NULL)
        *end_vec_p = prev_vec_p;
    return offset;
}


/*
 *  D D B E _ p a t c h _ v e c _ r e f s
 *
 *  Patches each vector entry that indirectly refers to another vector entry.
 *  Assumes that vector entry indices have already been calculated by the
 *  routine above.  Turns each DDBE_vec_indirect_k or DDBE_vec_reference_k
 *  entry into a DDBE_vec_long_k entry that contains the index of the referenced
 *  vector entry.
 */
static void DDBE_patch_vec_refs
(
    DDBE_vec_rep_t      *vec_p      /* [in] ptr to vector entry list */
)
{
    DDBE_vec_rep_t      *ref_vec_p = NULL; /* Referenced vector entry */

    while (vec_p != NULL)
    {
        if (vec_p->kind == DDBE_vec_indirect_k)
            ref_vec_p = *(vec_p->val.p_ref_p);
        else if (vec_p->kind == DDBE_vec_reference_k)
            ref_vec_p = vec_p->val.ref_p;

        if (vec_p->kind == DDBE_vec_indirect_k
            || vec_p->kind == DDBE_vec_reference_k)
        {
            /*
             * If referenced entry is a pad, actual reference is the entry that
             * follows the pad.  Turn entry into an integer index value.
             */
            while (ref_vec_p->kind == DDBE_vec_pad_k
                   || ref_vec_p->kind == DDBE_vec_noop_k
                   || ref_vec_p->kind == DDBE_vec_comment_k)
                ref_vec_p = ref_vec_p->next;
            vec_p->kind = DDBE_vec_long_k;
            vec_p->val.long_val = ref_vec_p->index;
        }

        vec_p = vec_p->next;
    }
}


/*
 *  D D B E _ p a t c h _ o p e r _ i n f o
 *
 *  Patches each operation info node to contain the starting type vector
 *  indices for the [in] parameters and [out] parameters.  For each parameter
 *  in the operation, patches the parameter info node to contain the starting
 *  type vector index for the parameter.
 */
static void DDBE_patch_oper_info
(
    AST_interface_n_t   *int_p      /* [in] ptr to AST interface node */
)
{
    AST_export_n_t      *export_p;  /* Ptr to AST export node */
    AST_operation_n_t   *oper_p;    /* Ptr to AST operation node */
    DDBE_oper_i_t       *oper_i_p;  /* Ptr to operation info node */
    AST_parameter_n_t   *param_p;   /* Ptr to AST parameter node */

    /*
     * Process each operation in the interface.
     */
    for (export_p = int_p->exports; export_p != NULL; export_p = export_p->next)
    {
        if (export_p->kind == AST_operation_k)
        {
            oper_p = export_p->thing_p.exported_operation;
            oper_i_p = oper_p->be_info.dd_oper;

            if (oper_i_p->ins_type_vec_p != NULL)
                oper_i_p->ins_type_index = oper_i_p->ins_type_vec_p->index;

            if (oper_i_p->outs_type_vec_p != NULL)
                oper_i_p->outs_type_index = oper_i_p->outs_type_vec_p->index;

            for (param_p = oper_p->parameters;
                 param_p != NULL;
                 param_p = param_p->next)
            {
                if (param_p->be_info.dd_param->type_vec_p != NULL)
                    param_p->be_info.dd_param->type_index =
                        param_p->be_info.dd_param->type_vec_p->index;
            }
            if (oper_p->result->be_info.dd_param->type_vec_p != NULL)
                oper_p->result->be_info.dd_param->type_index =
                    oper_p->result->be_info.dd_param->type_vec_p->index;
        }
    }
}

/******************************/
/*  DDBE generation routines  */
/******************************/

/*
 *  D D B E _ i n i t _ v e c t o r s
 *
 *  One-time allocation and initialization of the vector information data
 *  structure.  This contains all the context needed by the Data Driven Backend.
 */
static void DDBE_init_vectors
(
    AST_interface_n_t   *int_p,     /* [in] ptr to interface node */
    DDBE_vectors_t      **p_vip     /*[out] Vector information pointer */
)
{
    DDBE_vectors_t      *vip;       /* Vector information pointer */

    *p_vip = vip = NEW (DDBE_vectors_t);

    DDBE_VEC_SENTINEL(vip->defn_p);
    DDBE_VEC_SENTINEL(vip->type_p);
    DDBE_VEC_SENTINEL(vip->offset_p);
    DDBE_VEC_SENTINEL(vip->rtn_p);
    DDBE_VEC_SENTINEL(vip->type_info_p);

    vip->allocate     = FALSE;
    vip->allocate_ref = FALSE;
    vip->free_rep     = FALSE;

    /* Initialize the stacks and save interface node addr */
    vip->ind_sp    = NULL;
    vip->tup_sp    = NULL;
    vip->ast_int_p = int_p;

    /* Re-initializable data */
    DDBE_VEC_INIT(vip);
}


/*
 *  D D B E _ i n i t _ t a g s
 *
 *  One-time initialization of the symbolic names for Interpreter tags.
 */
static void DDBE_init_tags
(void)
{
    DDBE_dt_allocate            = NAMETABLE_add_id("IDL_DT_ALLOCATE");
    DDBE_dt_allocate_ref        = NAMETABLE_add_id("IDL_DT_ALLOCATE_REF");
    DDBE_dt_begin_nested_struct = NAMETABLE_add_id("IDL_DT_BEGIN_NESTED_STRUCT");
    DDBE_dt_boolean             = NAMETABLE_add_id("IDL_DT_BOOLEAN");
    DDBE_dt_byte                = NAMETABLE_add_id("IDL_DT_BYTE");
    DDBE_dt_char                = NAMETABLE_add_id("IDL_DT_CHAR");
    DDBE_dt_conf_array          = NAMETABLE_add_id("IDL_DT_CONF_ARRAY");
    DDBE_dt_conf_struct         = NAMETABLE_add_id("IDL_DT_CONF_STRUCT");
    DDBE_dt_cs_array            = NAMETABLE_add_id("IDL_DT_CS_ARRAY");
    DDBE_dt_cs_attribute        = NAMETABLE_add_id("IDL_DT_CS_ATTRIBUTE");
    DDBE_dt_cs_rlse_shadow      = NAMETABLE_add_id("IDL_DT_CS_RLSE_SHADOW");
    DDBE_dt_cs_shadow           = NAMETABLE_add_id("IDL_DT_CS_SHADOW");
    DDBE_dt_cs_type             = NAMETABLE_add_id("IDL_DT_CS_TYPE");
    DDBE_dt_deleted_nodes       = NAMETABLE_add_id("IDL_DT_DELETED_NODES");
    DDBE_dt_does_not_exist      = NAMETABLE_add_id("IDL_DT_DOES_NOT_EXIST");
    DDBE_dt_double              = NAMETABLE_add_id("IDL_DT_DOUBLE");
    DDBE_dt_enc_union           = NAMETABLE_add_id("IDL_DT_ENC_UNION");
    DDBE_dt_end_nested_struct   = NAMETABLE_add_id("IDL_DT_END_NESTED_STRUCT");
    DDBE_dt_enum                = NAMETABLE_add_id("IDL_DT_ENUM");
    DDBE_dt_eol                 = NAMETABLE_add_id("IDL_DT_EOL");
    DDBE_dt_error_status        = NAMETABLE_add_id("IDL_DT_ERROR_STATUS");
    DDBE_dt_first_is_limit      = NAMETABLE_add_id("IDL_LIMIT_FIRST_IS");
    DDBE_dt_first_is_early_limit = NAMETABLE_add_id("IDL_LIMIT_FIRST_IS | IDL_CF_EARLY");
    DDBE_dt_fixed_array         = NAMETABLE_add_id("IDL_DT_FIXED_ARRAY");
    DDBE_dt_fixed_bound         = NAMETABLE_add_id("IDL_BOUND_FIXED");
    DDBE_dt_fixed_limit         = NAMETABLE_add_id("IDL_LIMIT_FIXED");
    DDBE_dt_fixed_struct        = NAMETABLE_add_id("IDL_DT_FIXED_STRUCT");
    DDBE_dt_float               = NAMETABLE_add_id("IDL_DT_FLOAT");
    DDBE_dt_free_rep            = NAMETABLE_add_id("IDL_DT_FREE_REP");
    DDBE_dt_full_ptr            = NAMETABLE_add_id("IDL_DT_FULL_PTR");
    DDBE_dt_hyper               = NAMETABLE_add_id("IDL_DT_HYPER");
    DDBE_dt_ignore              = NAMETABLE_add_id("IDL_DT_IGNORE");
    DDBE_dt_interface           = NAMETABLE_add_id("IDL_DT_ORPC_INTERFACE");
    DDBE_dt_in_context          = NAMETABLE_add_id("IDL_DT_IN_CONTEXT");
    DDBE_dt_in_out_context      = NAMETABLE_add_id("IDL_DT_IN_OUT_CONTEXT");
    DDBE_dt_last_is_limit       = NAMETABLE_add_id("IDL_LIMIT_LAST_IS");
    DDBE_dt_last_is_early_limit = NAMETABLE_add_id("IDL_LIMIT_LAST_IS | IDL_CF_EARLY");
    DDBE_dt_length_is_limit     = NAMETABLE_add_id("IDL_LIMIT_LENGTH_IS");
    DDBE_dt_length_is_early_limit = NAMETABLE_add_id("IDL_LIMIT_LENGTH_IS | IDL_CF_EARLY");
    DDBE_dt_long                = NAMETABLE_add_id("IDL_DT_LONG");
    DDBE_dt_max_is_bound        = NAMETABLE_add_id("IDL_BOUND_MAX_IS");
    DDBE_dt_max_is_early_bound  = NAMETABLE_add_id("IDL_BOUND_MAX_IS | IDL_CF_EARLY");
    DDBE_dt_min_is_bound        = NAMETABLE_add_id("IDL_BOUND_MIN_IS");
    DDBE_dt_min_is_early_bound  = NAMETABLE_add_id("IDL_BOUND_MIN_IS | IDL_CF_EARLY");
    DDBE_dt_modified            = NAMETABLE_add_id("IDL_DT_MODIFIED");
    DDBE_dt_ndr_align_2         = NAMETABLE_add_id("IDL_DT_NDR_ALIGN_2");
    DDBE_dt_ndr_align_4         = NAMETABLE_add_id("IDL_DT_NDR_ALIGN_4");
    DDBE_dt_ndr_align_8         = NAMETABLE_add_id("IDL_DT_NDR_ALIGN_8");
    DDBE_dt_n_e_union           = NAMETABLE_add_id("IDL_DT_N_E_UNION");
    DDBE_dt_open_array          = NAMETABLE_add_id("IDL_DT_OPEN_ARRAY");
    DDBE_dt_out_context         = NAMETABLE_add_id("IDL_DT_OUT_CONTEXT");
    DDBE_dt_passed_by_ref       = NAMETABLE_add_id("IDL_DT_PASSED_BY_REF");
    DDBE_dt_pipe                = NAMETABLE_add_id("IDL_DT_PIPE");
    DDBE_dt_range               = NAMETABLE_add_id("IDL_DT_RANGE");
    DDBE_dt_ref_ptr             = NAMETABLE_add_id("IDL_DT_REF_PTR");
    DDBE_dt_represent_as        = NAMETABLE_add_id("IDL_DT_REPRESENT_AS");
    DDBE_dt_size_is_bound       = NAMETABLE_add_id("IDL_BOUND_SIZE_IS");
    DDBE_dt_size_is_early_bound = NAMETABLE_add_id("IDL_BOUND_SIZE_IS | IDL_CF_EARLY");
    DDBE_dt_short               = NAMETABLE_add_id("IDL_DT_SHORT");
    DDBE_dt_small               = NAMETABLE_add_id("IDL_DT_SMALL");
    DDBE_dt_string              = NAMETABLE_add_id("IDL_DT_STRING");
    DDBE_dt_string_bound        = NAMETABLE_add_id("IDL_BOUND_STRING");
    DDBE_dt_string_limit        = NAMETABLE_add_id("IDL_LIMIT_STRING");
    DDBE_dt_transmit_as         = NAMETABLE_add_id("IDL_DT_TRANSMIT_AS");
    DDBE_dt_unique_ptr          = NAMETABLE_add_id("IDL_DT_UNIQUE_PTR");
    DDBE_dt_uhyper              = NAMETABLE_add_id("IDL_DT_UHYPER");
    DDBE_dt_ulong               = NAMETABLE_add_id("IDL_DT_ULONG");
    DDBE_dt_upper_conf_limit    = NAMETABLE_add_id("IDL_LIMIT_UPPER_CONF");
    DDBE_dt_ushort              = NAMETABLE_add_id("IDL_DT_USHORT");
    DDBE_dt_usmall              = NAMETABLE_add_id("IDL_DT_USMALL");
    DDBE_dt_v1_array            = NAMETABLE_add_id("IDL_DT_V1_ARRAY");
    DDBE_dt_v1_conf_struct      = NAMETABLE_add_id("IDL_DT_V1_CONF_STRUCT");
    DDBE_dt_v1_enum             = NAMETABLE_add_id("IDL_DT_V1_ENUM");
    DDBE_dt_v1_string           = NAMETABLE_add_id("IDL_DT_V1_STRING");
    DDBE_dt_varying_array       = NAMETABLE_add_id("IDL_DT_VARYING_ARRAY");
    DDBE_dt_void                = NAMETABLE_add_id("IDL_DT_VOID");
}


/*
 *  D D B E _ s k i p _ t o _ t u p
 *
 *  Skip to specified tuple at same scoping level as current tuple.
 */
static void DDBE_skip_to_tup
(
    IR_tup_n_t      **p_tup_p,      /* [io] ptr to intermediate rep tuple */
    IR_opcode_k_t   end_opcode      /* [in] matching opcode to skip to */
)
{
    IR_tup_n_t      *tup_p;         /* ptr to intermediate rep tuple */
    IR_opcode_k_t   begin_opcode;   /* starting opcode */
    int             scope;          /* relative scoping level */

    tup_p = *p_tup_p;
    scope = 0;
    begin_opcode = tup_p->opcode;

    while (TRUE)
    {
        if (tup_p->opcode == begin_opcode)
            scope++;
        else if (tup_p->opcode == end_opcode)
        {
            scope--;
            if (scope == 0)
                break;
        }

        tup_p = tup_p->next;
    }

    *p_tup_p = tup_p;
}


/*
 *  D D B E _ p u s h _ i n d i r e c t i o n _ s c o p e
 *  D D B E _ p o p _ i n d i r e c t i o n _ s c o p e
 *
 * During the building of type/definition vector data it is necessary to build
 * separate definition vector data for a contained constructed type and
 * reference that data indirectly.  This is handled by the indirection scoping
 * routines below.
 */
static void DDBE_push_indirection_scope
(
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    DDBE_ind_stack_t *new_p;        /* Ptr to new indirection stack entry */

    /* Sanity check */
    if (vip->p_cur_p == &vip->type_p && vip->ind_sp != NULL)
        INTERNAL_ERROR("Current vector is type vector but *ind_sp != NULL");

    /*
     * Save current vector pointer on indirection stack.  Current vector
     * becomes definition vector for the life of the indirection.
     */
    new_p = NEW (DDBE_ind_stack_t);
    new_p->ind_p = *(vip->p_cur_p);
    new_p->off_p = vip->offset_p;
    new_p->rtn_p = vip->rtn_p;
    new_p->info_p= vip->type_info_p;
    new_p->saved_defn_p       = vip->saved_defn_p;
    new_p->cfmt_info_tup_p    = vip->cfmt_info_tup_p;
    new_p->in_cfmt_struct_hdr = vip->in_cfmt_struct_hdr;
    vip->in_cfmt_struct_hdr   = FALSE;  /* Do NOT propagate downward */
    new_p->in_flatarr         = vip->in_flatarr;
    vip->in_flatarr           = FALSE;  /* Do NOT propagate downward */
    new_p->next  = vip->ind_sp;
    vip->ind_sp  = new_p;

    vip->p_cur_p = &vip->defn_p;
}

static void DDBE_pop_indirection_scope
(
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    DDBE_ind_stack_t *top_p;        /* Ptr to top indirection stack entry */

    top_p = vip->ind_sp;

    /*
     * Either the type vector or definition vector pointer, as determined from
     * the indirection scoping level, is restored to the saved value on the
     * indirection stack.  This vector again becomes current.
     */
    if (top_p == NULL)
    {
        INTERNAL_ERROR("Can't return from indirection scope");
        return;
    }

    if (top_p->next == NULL)
    {
        vip->p_cur_p = &vip->type_p;
        vip->type_p  = top_p->ind_p;
    }
    else
    {
        /* Current vector pointer should already be defn vector */
        if (vip->p_cur_p != &vip->defn_p)
            INTERNAL_ERROR("Scope > 1 but current vector not definition vector");
        vip->defn_p = top_p->ind_p;
    }

    vip->offset_p = top_p->off_p;
    vip->rtn_p    = top_p->rtn_p;
    vip->type_info_p = top_p->info_p;
    vip->saved_defn_p       = top_p->saved_defn_p;
    vip->cfmt_info_tup_p    = top_p->cfmt_info_tup_p;
    vip->in_cfmt_struct_hdr = top_p->in_cfmt_struct_hdr;
    vip->in_flatarr         = top_p->in_flatarr;

    /* Pop stack entry */
    vip->ind_sp = top_p->next;
    FREE(top_p);
}


/*
 *  D D B E _ o p e r a t i o n _ i n f o
 *
 *  Creates and initializes a DDBE operation info node and hangs off AST node.
 *  For each parameter and the operation result, creates and initializes a
 *  DDBE parameter info node and hangs off parameter AST node.
 */
static void DDBE_operation_info
(
    AST_operation_n_t   *oper_p     /* [in] ptr to AST operation node */
)
{
    AST_parameter_n_t   *param_p;

    oper_p->be_info.dd_oper = NEW (DDBE_oper_i_t);

    for (param_p = oper_p->parameters; param_p != NULL; param_p = param_p->next)
        param_p->be_info.dd_param = NEW (DDBE_param_i_t);
    oper_p->result->be_info.dd_param = NEW (DDBE_param_i_t);
}


/*
 *  D D B E _ t y p e _ i n f o
 *
 *  Creates and initializes a DDBE type info node and hangs off the AST node.
 */
static void DDBE_type_info
(
    AST_type_n_t    *type_p,        /* [in] ptr to AST type node */
    IR_flags_t      flags           /* [in] IREP type indirection flags */
)
{
    DDBE_type_i_t   *type_i_p;      /* Ptr to backend type info node */
    char    inst_name[MAX_ID+1];    /* Instance name */

    type_i_p = NEW (DDBE_type_i_t);

    /*
     * Construct a unique name so that an instance of this type can be spelled
     * if necessary.
     */
    sprintf(inst_name, "%si_%ld", DDBE_PREFIX_IDL, DDBE_inst_num++);
    type_i_p->inst_name = NAMETABLE_add_id(inst_name);

    if (flags & IR_REP_AS)
        type_p->rep_as_type->be_info.dd_type = type_i_p;
    else if (flags & IR_CS_CHAR)
        type_p->cs_char_type->be_info.dd_type = type_i_p;
    else
        type_p->be_info.dd_type = type_i_p;
}


/*
 *  D D B E _ o p _ t y p e _ i n d i r e c t
 *
 *  Process an indirect type reference.  Union, non-nested structure, pipe,
 *  transmit_as, represent_as, and cs_char types are always referenced
 *  indirectly and have exactly one set of IREP tuples off the type node.
 *
 *  NOTE: If the indirect tuple has the REP_AS flag, the IREP tuples and
 *  backend info hang off the AST rep_as node rather than the AST type node.
 *  Similar situation for CS_CHAR flag.
 */
static void DDBE_op_type_indirect
(
    IR_tup_n_t      **p_tup_p,      /* [io] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    AST_type_n_t    *type_p;        /* Ptr to AST struct type node */
    DDBE_type_i_t   *type_i_p;      /* Ptr to backend type info node */
    DDBE_tup_stack_t *new_p;        /* Ptr to new tuple stack entry */
    IR_tup_n_t      *tup_p;         /* Ptr to type_indirect tuple */

    tup_p  = *p_tup_p;
    type_p = tup_p->arg[IR_ARG_TYPE].type;

    /*
     * Create a DDBE info node for the type if not yet done.  If type has a
     * transmit_as type create a DDBE info node for it if not yet done.
     */
    if (tup_p->flags & IR_REP_AS)
        type_i_p = type_p->rep_as_type->be_info.dd_type;
    else if (tup_p->flags & IR_CS_CHAR)
        type_i_p = type_p->cs_char_type->be_info.dd_type;
    else
        type_i_p = type_p->be_info.dd_type;

    if (type_i_p == NULL)
        DDBE_type_info(type_p, tup_p->flags);
    if (type_p->xmit_as_type != NULL
        &&  type_p->xmit_as_type->be_info.dd_type == NULL)
        DDBE_type_info(type_p->xmit_as_type, 0);

    /*
     * Push indirection scope and maintain intermediate rep scope context.
     */
    DDBE_push_indirection_scope(vip);
    IR_process_tup(vip->ir_ctx_p, tup_p);

    /*
     * Save current (next) tuple pointer on tuple stack.
     * Switch tuple pointer over to type's data_tups.
     */
    new_p = NEW (DDBE_tup_stack_t);
    new_p->tup_p    = tup_p->next;
    new_p->next     = vip->tup_sp;
    vip->tup_sp     = new_p;

    if (tup_p->flags & IR_REP_AS)
        *p_tup_p = type_p->rep_as_type->data_tups;
    else if (tup_p->flags & IR_CS_CHAR)
        *p_tup_p = type_p->cs_char_type->data_tups;
    else
        *p_tup_p = type_p->data_tups;
    vip->update_tup = FALSE;
}


/*
 *  D D B E _ t y p e _ i n d i r e c t _ e n d
 *
 *  Finish processing an indirect type reference.  Union, non-nested structure,
 *  pipe, transmit_as, represent_as, and cs_char types are always referenced
 *  indirectly and have exactly one set of IREP tuples off the type node.
 */
static void DDBE_type_indirect_end
(
    IR_tup_n_t      **p_tup_p,      /* [io] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    DDBE_tup_stack_t *top_p;        /* Ptr to top tuple stack entry */

    top_p = vip->tup_sp;

    /*
     * Switch tuple pointer back over to parameter data_tups.
     */
    if (top_p == NULL)
    {
        INTERNAL_ERROR("Can't return from indirect tuples");
        return;
    }

    *p_tup_p    = top_p->tup_p;
    vip->tup_sp = top_p->next;
    FREE(top_p);

    /*
     * Pop indirection scope.
     */
    DDBE_pop_indirection_scope(vip);
}


/*
 *  D D B E _ p r o p e r t i e s _ b y t e
 *
 *  Generates a type vector entry to describe data type properties.
 */
static void DDBE_properties_byte
(
    AST_type_n_t    *type_p,        /* [in] Type to generate properties for */
    DDBE_vec_rep_t  **p_vec_p       /* [io] Insertion point for vector entry */
)
{
    char properties[DDBE_MAX_EXPR]; /* Symbolic expression for properties */
    STRTAB_str_t    prop_expr;

    strcpy(properties, "0");

    /* Set flags for any data rep dependencies */
    if (FE_TEST(type_p->fe_info->flags, FE_HAS_CHAR))
        strcat(properties, "|IDL_PROP_DEP_CHAR");
    if (FE_TEST(type_p->fe_info->flags, FE_HAS_FLOAT))
        strcat(properties, "|IDL_PROP_DEP_FLOAT");
    if (FE_TEST(type_p->fe_info->flags, FE_HAS_INT))
        strcat(properties, "|IDL_PROP_DEP_INT");

    /* Set flag if type is possibly NDR aligned */
    if (FE_TEST(type_p->fe_info->flags, FE_MAYBE_WIRE_ALIGNED))
        strcat(properties, "|IDL_PROP_MAYBE_WIRE_ALIGNED");

    /* Set flag if transmissible type is a pointer or contains pointers */
    if (type_p->xmit_as_type != NULL)
        type_p = type_p->xmit_as_type;
    if (FE_TEST(type_p->fe_info->flags, FE_HAS_PTR))
        strcat(properties, "|IDL_PROP_HAS_PTRS");

    prop_expr = STRTAB_add_string(properties);
    DDBE_expr_vec_entry(p_vec_p, DDBE_vec_expr_byte_k, prop_expr, "properties");
}


/*
 *  D D B E _ o p _ m a r s h a l l
 *
 *  Process IREP tuple to [un]marshall a scalar type.
 */
static void DDBE_op_marshall
(
    IR_tup_n_t      *tup_p,         /* [in] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    char const      *inst_name;     /* Instance name */

    /*
     * If in a structure, offset vector entry:
     *  long: expression for offset to this field
     */
    if (IR_cur_scope(vip->ir_ctx_p) == IR_SCP_STRUCT)
    {
        AST_field_n_t   *field_p;       /* Ptr to AST field node */
        char const      *field_name;    /* Field name */
        char comment[DDBE_MAX_COMMENT]; /* Comment buffer */

        field_p = tup_p->arg[IR_ARG_FIELD].field;
        NAMETABLE_id_to_string(field_p->name, &field_name);
        sprintf(comment, "field %s offset", field_name);

        DDBE_expr_vec_entry(&vip->offset_p, DDBE_vec_expr_k,
            IR_field_expr(vip->ir_ctx_p, field_p), comment);
    }

    /*
     * type/definition vector entry:
     *  byte: I-char attribute tag (iff IR_CS_CHAR)
     *  byte: scalar type tag
     */
    if (tup_p->flags & IR_CS_CHAR)
        DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_cs_attribute);

    if (tup_p->arg[IR_ARG_INST].inst == NULL)
        inst_name = NULL;
    else
        NAMETABLE_id_to_string(tup_p->arg[IR_ARG_INST].inst->name, &inst_name);

    DDBE_scalar_vec_entry(vip->p_cur_p, tup_p->arg[IR_ARG_TYPE].type,
        inst_name, vip);

    /*
     * If scalar is non-default union arm, add pad bytes to vector
     * since all arms must occupy the same number of bytes.
     */
    if (IR_cur_scope(vip->ir_ctx_p) == IR_SCP_UNION && !vip->in_default_arm)
    {
        vip->arm_byte_cnt++;    /* Account for scalar tag */
        DDBE_pad_vec_entry(vip->p_cur_p, DDBE_ARM_SIZE - vip->arm_byte_cnt);
    }
}


/*
 *  D D B E _ o p _ s t r u c t _ b e g i n
 *
 *  Process IREP tuple for beginning of a structure definition.
 */
static void DDBE_op_struct_begin
(
    IR_tup_n_t      **p_tup_p,      /* [io] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    IR_tup_n_t      *tup_p;         /* ptr to intermediate rep tuple */
    AST_type_n_t    *type_p;        /* Ptr to AST struct type node */
    char const      *type_name;     /* Type name */
    boolean     modified = FALSE;   /* TRUE => struct needs modifier tag */
    boolean         conformant;     /* TRUE => conformant struct */
    char comment[DDBE_MAX_COMMENT]; /* Comment buffer */
    boolean     defn_done = TRUE;   /* Definition vec entries done for type */

    tup_p = *p_tup_p;

    /*
     * On entry, tup_p points at the tuple IR_op_struct_begin_k and the
     * struct data scope has NOT yet been pushed on the data scope stack.
     * A separate indirection scope HAS been pushed since non-nested structures
     * are always referenced via IR_op_type_indirect_k.
     */
    type_p = tup_p->arg[IR_ARG_TYPE].type;  /* -> struct type node */

    if (type_p->name == NAMETABLE_NIL_ID)
        type_name = "anon_struct";
    else
        NAMETABLE_id_to_string(type_p->name, &type_name);

    /*
     * Structures within structures must be handled differently due to the
     * requirement that structures are fully flattened in the definition vec.
     * Process vector entries if NON-nested struct.
     */
    if (IR_cur_scope(vip->ir_ctx_p) != IR_SCP_STRUCT)
    {
        /*
         * Do definition and offset vector entries if not done yet
         * and not in a self-pointing data structure (the type node
         * address appears exactly once on the type stack).
         */
        if (!type_p->be_info.dd_type->defn_done
            && IR_under_type(vip->ir_ctx_p, type_p) == 1)
        {
            defn_done = FALSE;
            /*
             * first offset vector entry for non-nested struct:
             *  long: sizeof struct
             */
            sprintf(comment, "struct %s size and offsets", type_name);
            DDBE_comment_vec_entry(&vip->offset_p, comment);

            sprintf(comment, "sizeof %s", type_name);
            DDBE_type_info_vec_entry(DDBE_vec_sizeof_k, &vip->offset_p, type_p,
                comment);

            /*
             * Store offset vector pointer for this type.
             */
            type_p->be_info.dd_type->offset_vec_p = vip->offset_p;

            /*
             * offset vector meta-entry to mark start of structure offsets.
             */
            DDBE_type_info_vec_entry(DDBE_vec_offset_begin_k, &vip->offset_p,
                type_p, NULL);

            /*
             * first definition vector entry for non-nested struct:
             *  long: offset vector location for this struct
             */
            sprintf(comment, "struct %s definition", type_name);
            DDBE_comment_vec_entry(&vip->defn_p, comment);
            DDBE_indirect_vec_entry(&vip->defn_p,
                &type_p->be_info.dd_type->offset_vec_p, "offset vector index");

            /*
             * Store definition vector pointer for this type.
             */
            type_p->be_info.dd_type->type_vec_p = vip->defn_p;
        }

        /*
         * type vector entries for non-nested struct:
         * [Note non-nested structs are always referenced indirectly.]
         *  byte: dt_fixed_struct  or  dt_conformant_struct
         *  byte: properties byte
         *  long: definition vector location of full structure
         */
        conformant = ((tup_p->flags & IR_CONFORMANT) != 0);

        DDBE_tag_vec_entry(&vip->ind_sp->ind_p,
            (conformant) ? 
                ((AST_UNALIGN_SET(type_p)) ? DDBE_dt_v1_conf_struct
                                           : DDBE_dt_conf_struct)
                              : DDBE_dt_fixed_struct);
        if (!modified && !defn_done)
          DDBE_reference_vec_entry(&vip->type_info_p, vip->ind_sp->ind_p, NULL);

        DDBE_properties_byte(type_p, &vip->ind_sp->ind_p);

        sprintf(comment, "type %s index", type_name);
        DDBE_indirect_vec_entry(&vip->ind_sp->ind_p,
            &type_p->be_info.dd_type->type_vec_p, comment);

        /*
         * If struct is non-default union arm, add pad bytes to vector
         * since all arms must occupy the same number of bytes.
         */
        if (IR_cur_scope(vip->ir_ctx_p) == IR_SCP_UNION && !vip->in_default_arm)
        {
            vip->arm_byte_cnt += 8; /* Account for tag, props, filler, index */
            DDBE_pad_vec_entry(&vip->ind_sp->ind_p,
                DDBE_ARM_SIZE - vip->arm_byte_cnt);
        }

        /*
         * If definition vector entries have already been done for this type,
         * skip the rest of its tuples since the reference to the already
         * existing entries has been done above and no more need be done.
         */
        if (defn_done)
        {
            DDBE_skip_to_tup(p_tup_p, IR_op_struct_end_k);
            /* Set flag so struct_end_k tuple gets processed */
            vip->update_tup = FALSE;
        }
    }
    else
    {
        /*
         * definition vector entry for nested struct:
         *  byte: nested struct begin tag
         *
         * Note: we only get here the first time a nested struct is processed
         * since thereafter the nested struct is skipped (see above).
         */

        DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_begin_nested_struct);
    }

    /*
     * Push data scoping level to handle contained types within structure.
     */
    IR_process_tup(vip->ir_ctx_p, tup_p);
}


/*
 *  D D B E _ o p _ s t r u c t _ e n d
 *
 *  Process IREP tuple for end of a structure definition.
 */
static void DDBE_op_struct_end
(
    IR_tup_n_t      **p_tup_p,      /* [io] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    AST_type_n_t    *type_p;        /* Ptr to AST struct type node */

    type_p = (*p_tup_p)->arg[IR_ARG_TYPE].type;

    /*
     * Pop structure data scope; top scope will then be parent scope.
     * We are still in a separate indirection scope since non-nested structures
     * are always referenced via IR_op_type_indirect_k; the indirection scope
     * is popped by DDBE_type_indirect_end.
     */
    IR_process_tup(vip->ir_ctx_p, *p_tup_p);

    if (IR_cur_scope(vip->ir_ctx_p) != IR_SCP_STRUCT)
    {
        /*
         * If definition vector entries already done for this type, return.
         */
        if (type_p->be_info.dd_type->defn_done
            || IR_under_type(vip->ir_ctx_p, type_p) > 0)
            return;

        type_p->be_info.dd_type->defn_done = TRUE;

        /*
         * definition vector entry for non-nested struct:
         *  byte: dt_eol
         */
        DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_eol);

        /*
         * offset vector meta-entry to mark end of structure offsets.
         */
        DDBE_type_info_vec_entry(DDBE_vec_offset_end_k, &vip->offset_p, type_p,
            NULL);
    }
    else
    {
        /*
         * definition vector entry for nested struct:
         *  byte: nested struct end tag
         *
         * Note: we only get here the first time a nested struct is processed
         */
        DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_end_nested_struct);
    }
}


/*
 *  D D B E _ o p _ d i s c _ u n i o n _ b e g i n
 *
 *  Process IREP tuple for beginning of a discriminated union definition.
 */
static void DDBE_op_disc_union_begin
(
    IR_tup_n_t      **p_tup_p,      /* [io] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    IR_tup_n_t      *tup_p;         /* ptr to intermediate rep tuple */
    IR_tup_n_t      *sw_tup_p;      /* ptr to switch_enc_k tuple, encap union */
    AST_type_n_t    *type_p;        /* Ptr to AST union type node */
    AST_disc_union_n_t *union_p;    /* Ptr to AST union node */
    char const      *type_name;     /* Type name */
    char const      *switch_name;   /* Name of union switch */
    char const      *union_name;    /* Name of encapsulated union field */
    STRTAB_str_t    union_expr;     /* Expr for encapsulated union field */
    char comment[DDBE_MAX_COMMENT]; /* Comment buffer */
    boolean     defn_done = TRUE;   /* Definition vec entries done for type */

    tup_p = *p_tup_p;

    /*
     * On entry, tup_p points at the tuple IR_op_disc_union_begin_k and the
     * union data scope has NOT yet been pushed on the data scope stack.
     * A separate indirection scope HAS been pushed since unions
     * are always referenced via IR_op_type_indirect_k.
     */
    type_p = tup_p->arg[IR_ARG_TYPE].type;  /* -> union type node */
    union_p = type_p->type_structure.disc_union;

    if (type_p->name == NAMETABLE_NIL_ID)
        type_name = "anon_union";
    else
        NAMETABLE_id_to_string(type_p->name, &type_name);

    /*
     * Do definition and offset vector entries if not done yet
     * and not in a self-pointing data structure (the type node
     * address appears exactly once on the type stack).
     */
    if (!type_p->be_info.dd_type->defn_done
        && IR_under_type(vip->ir_ctx_p, type_p) == 1)
    {
        defn_done = FALSE;
        if (tup_p->flags & IR_ENCAPSULATED)
        {
            /*
             * offset vector entries for encapsulated union:
             *  long: sizeof encapsulated union type (including switch)
             *  long: offset to union body
             */
            *p_tup_p = (*p_tup_p)->next;
            sw_tup_p = *p_tup_p;
            /* An 'encapsulated switch' tuple must follow 'union begin' tuple */
            if (sw_tup_p->opcode != IR_op_switch_enc_k)
                INTERNAL_ERROR("Expected encapsulated switch tuple");
            vip->switch_name = sw_tup_p->arg[IR_ARG_NAME].name;
            vip->switch_type = sw_tup_p->arg[IR_ARG_TYPE].type;

            sprintf(comment, "union %s size and offset", type_name);
            DDBE_comment_vec_entry(&vip->offset_p, comment);
            sprintf(comment, "sizeof %s", type_name);
            DDBE_type_info_vec_entry(DDBE_vec_sizeof_k, &vip->offset_p, type_p,
                comment);

            /* Store offset vector pointer for this type */
            type_p->be_info.dd_type->offset_vec_p = vip->offset_p;

            /* Now do entries needed for offset to union body */
            DDBE_type_info_vec_entry(DDBE_vec_offset_begin_k, &vip->offset_p,
                type_p, NULL);
            NAMETABLE_id_to_string(union_p->union_name, &union_name);
            union_expr = STRTAB_add_string(union_name);
            DDBE_expr_vec_entry(&vip->offset_p, DDBE_vec_expr_k, union_expr,
                NULL);
            DDBE_type_info_vec_entry(DDBE_vec_offset_end_k, &vip->offset_p,
                type_p, NULL);
        }
        else
        {
            /*
             * offset vector entry for non-encapsulated union:
             *  long: sizeof non-encapsulated union type
             */
            sprintf(comment, "n_e union %s size", type_name);
            DDBE_comment_vec_entry(&vip->offset_p, comment);
            sprintf(comment, "sizeof %s", type_name);
            DDBE_type_info_vec_entry(DDBE_vec_sizeof_k, &vip->offset_p, type_p,
                comment);
            /* Store offset vector pointer for this type */
            type_p->be_info.dd_type->offset_vec_p = vip->offset_p;
        }

        /*
         * first definition vector entries for discriminated union:
         *  long: offset vector location for this union
         *  byte: switch type
         *  long: number of non-default arms
         */
        sprintf(comment, "union %s definition", type_name);
        DDBE_comment_vec_entry(&vip->defn_p, comment);
        DDBE_indirect_vec_entry(&vip->defn_p,
            &type_p->be_info.dd_type->offset_vec_p, "offset vector index");

        /* Store definition vector pointer for this type */
        type_p->be_info.dd_type->type_vec_p = vip->defn_p;

        DDBE_scalar_vec_entry(&vip->defn_p,
            ASTP_chase_ptr_to_type(vip->switch_type), "switch type", vip);
        DDBE_long_vec_entry(&vip->defn_p, DDBE_vec_long_k,
            tup_p->arg[IR_ARG_INT].int_val, "number of arms");
    }

    /*
     * type vector entries for discriminated union:
     * [Note unions are always referenced indirectly.]
     *  byte: dt_enc_union  or  dt_n_e_union
     *  byte: properties byte
     *  long: param/field number of switch variable (iff dt_n_e_union)
     *  long: definition vector location of union
     */
    DDBE_tag_vec_entry(&vip->ind_sp->ind_p, (tup_p->flags & IR_ENCAPSULATED) ?
        DDBE_dt_enc_union : DDBE_dt_n_e_union);
    if (!defn_done)
        DDBE_reference_vec_entry(&vip->type_info_p, vip->ind_sp->ind_p, NULL);

    DDBE_properties_byte(type_p, &vip->ind_sp->ind_p);
    if (!(tup_p->flags & IR_ENCAPSULATED))
    {
        NAMETABLE_id_to_string(vip->switch_name, &switch_name);
        sprintf(comment, "switch %s index", switch_name);
        DDBE_long_vec_entry(&vip->ind_sp->ind_p, DDBE_vec_long_k,
            vip->switch_index, comment);
    }
    sprintf(comment, "type %s index", type_name);
    DDBE_indirect_vec_entry(&vip->ind_sp->ind_p,
        &type_p->be_info.dd_type->type_vec_p, comment);

    /*
     * If union is non-default arm of another union, add pad bytes to vector
     * since all arms must occupy the same number of bytes.
     */
    if (IR_cur_scope(vip->ir_ctx_p) == IR_SCP_UNION && !vip->in_default_arm)
    {
        vip->arm_byte_cnt += 8; /* Account for tag, props, filler, index */
        DDBE_pad_vec_entry(&vip->ind_sp->ind_p,
            DDBE_ARM_SIZE - vip->arm_byte_cnt);
    }

    /*
     * If definition vector entries have already been done for this type,
     * skip the rest of its tuples since the reference to the already
     * existing entries has been done above and no more need be done.
     */
    if (defn_done)
    {
        DDBE_skip_to_tup(p_tup_p, IR_op_disc_union_end_k);
        /* Set flag so disc_union_end_k tuple gets processed */
        vip->update_tup = FALSE;
    }

    /*
     * Push data scoping level to handle contained types within union.
     */
    IR_process_tup(vip->ir_ctx_p, tup_p);

    /*
     * If union is in a structure, offset vector entry (in parent):
     *  long: expression for offset to this union field
     */
    if (IR_parent_scope(vip->ir_ctx_p) == IR_SCP_STRUCT)
    {
        AST_field_n_t   *field_p;       /* Ptr to AST field node */
        char const      *field_name;    /* Field name */

        field_p = (AST_field_n_t *)IR_cur_inst(vip->ir_ctx_p);
        NAMETABLE_id_to_string(field_p->name, &field_name);
        sprintf(comment, "field %s offset", field_name);

        DDBE_expr_vec_entry(&vip->ind_sp->off_p, DDBE_vec_expr_k,
            IR_field_expr(vip->ir_ctx_p, field_p), comment);
    }
}


/*
 *  D D B E _ o p _ d i s c _ u n i o n _ e n d
 *
 *  Process IREP tuple for end of a discriminated union definition.
 */
static void DDBE_op_disc_union_end
(
    IR_tup_n_t      **p_tup_p,      /* [io] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    AST_type_n_t    *type_p;        /* Ptr to AST union type node */

    type_p = (*p_tup_p)->arg[IR_ARG_TYPE].type;

    /*
     * Pop union data scope; top scope will then be parent scope.
     * We are still in a separate indirection scope since unions
     * are always referenced via IR_op_type_indirect_k; the
     * indirection scope is popped by DDBE_type_indirect_end.
     */
    IR_process_tup(vip->ir_ctx_p, *p_tup_p);

    /* Mark union definition as having been done */
    type_p->be_info.dd_type->defn_done = TRUE;
    vip->in_default_arm = FALSE;
}


/*
 *  D D B E _ o p _ p i p e _ b e g i n
 *
 *  Process IREP tuple for beginning of a pipe definition.
 */
static void DDBE_op_pipe_begin
(
    IR_tup_n_t      **p_tup_p,      /* [io] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    IR_tup_n_t      *tup_p;         /* ptr to intermediate rep tuple */
    AST_type_n_t    *type_p;        /* Ptr to AST pipe type node */
    char const      *type_name;     /* Type name */
    char comment[DDBE_MAX_COMMENT]; /* Comment buffer */

    tup_p = *p_tup_p;

    /*
     * On entry, tup_p points at the tuple IR_op_pipe_begin_k and the
     * pipe data scope has NOT yet been pushed on the data scope stack.
     * A separate indirection scope HAS been pushed since pipes
     * are always referenced via IR_op_type_indirect_k.
     */
    type_p = tup_p->arg[IR_ARG_TYPE].type;  /* -> pipe type node */
    NAMETABLE_id_to_string(type_p->name, &type_name);

    /*
     * Push data scope (now in pipe scope).  If definition vector entries for
     * pipe not done yet, start with comment entry.  This enhances output and
     * allows us to safely make entries in the parent reference.
     */
    IR_process_tup(vip->ir_ctx_p, tup_p);
    if (!type_p->be_info.dd_type->defn_done)
    {
        sprintf(comment, "pipe %s definition", type_name);
        DDBE_comment_vec_entry(&vip->defn_p, comment);
        /* Store definition vector pointer for this type */
        type_p->be_info.dd_type->type_vec_p = vip->defn_p;
    }

    /*
     * type vector entries for pipe:
     * [Note pipes are always referenced indirectly.]
     *  byte: dt_pipe
     *  byte: properties byte
     *  long: definition vector location of pipe base type
     */
    DDBE_tag_vec_entry(&vip->ind_sp->ind_p, DDBE_dt_pipe);
    DDBE_properties_byte(type_p, &vip->ind_sp->ind_p);
    sprintf(comment, "type %s index", type_name);
    DDBE_indirect_vec_entry(&vip->ind_sp->ind_p,
        &type_p->be_info.dd_type->type_vec_p, comment);

    /*
     * If definition vector entries have already been done for this type,
     * skip the rest of its tuples since the reference to the already
     * existing entries has been done above and no more need be done.
     * Otherwise, the tuples which follow, up to pipe_end_k, will be
     * processed, causing entries into the definition vector.
     */
    if (type_p->be_info.dd_type->defn_done)
    {
        DDBE_skip_to_tup(p_tup_p, IR_op_pipe_end_k);
        /* Set flag so pipe_end_k tuple gets processed */
        vip->update_tup = FALSE;
    }
}


/*
 *  D D B E _ o p _ p i p e _ e n d
 *
 *  Process IREP tuple for end of a pipe definition.
 */
static void DDBE_op_pipe_end
(
    IR_tup_n_t      *tup_p,         /* [in] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    AST_type_n_t    *type_p;        /* Ptr to AST union type node */

    type_p = tup_p->arg[IR_ARG_TYPE].type;

    /*
     * Pop pipe data scope; top scope will then be parent scope.
     * We are still in a separate indirection scope since pipes
     * are always referenced via IR_op_type_indirect_k; the
     * indirection scope is popped by DDBE_type_indirect_end.
     */
    IR_process_tup(vip->ir_ctx_p, tup_p);

    /* Mark pipe definition as having been done */
    type_p->be_info.dd_type->defn_done = TRUE;
}


/*
 *  D D B E _ o p _ a r r a y
 *
 *  Process IREP tuple for beginning of an array definition.
 */
static void DDBE_op_array
(
    IR_tup_n_t      *tup_p,         /* [in] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    AST_type_n_t    *type_p;        /* Ptr to AST type node */
    AST_type_n_t    *base_type_p;   /* Array base type */
    char comment[DDBE_MAX_COMMENT]; /* Comment buffer */
    boolean         flat_is_full;   /* TRUE => flat rep = full rep */
    boolean         in_flatarr;     /* Local copy of flag before */
                                    /*  array scope pushed       */
    /*
     * On entry, tup_p points at one of the 4 array introducer tuples
     *      IR_op_{conformant|fixed|open|varying}_array_k
     *
     * Array data scope has NOT yet been pushed on the data scope stack.
     * We are NOT yet in an indirection scope for the array.
     */
    type_p = tup_p->arg[IR_ARG_TYPE].type;
    base_type_p = type_p->type_structure.array->element_type;
    in_flatarr = vip->in_flatarr;

    /*
     * offset vector entry (if array in struct):
     *  long: offset to array field
     */
    if (IR_cur_scope(vip->ir_ctx_p) == IR_SCP_STRUCT && !in_flatarr)
    {
        AST_field_n_t   *field_p;       /* Ptr to AST field node */
        char const      *field_name;    /* Field name */

        field_p = tup_p->arg[IR_ARG_FIELD].field;
        NAMETABLE_id_to_string(field_p->name, &field_name);
        sprintf(comment, "field %s offset", field_name);

        DDBE_expr_vec_entry(&vip->offset_p, DDBE_vec_expr_arr_k,
            IR_field_expr(vip->ir_ctx_p, field_p), comment);
    }

    /*
     * Push scope (now in array scope) and start child definition with dummy
     * entry so we can safely make entries in the parent.
     */
    IR_process_tup(vip->ir_ctx_p, tup_p);
    DDBE_push_indirection_scope(vip);
    DDBE_noop_vec_entry(&vip->defn_p);

    /*
     * type/definition vector entries:
     *  byte: properties byte
     *  long: definition vector location of full or flattened array
     * Note: Assumption that array entries occupy DDBE_ARM_SIZE bytes,
     * so no logic for pad bytes if array is arm of union.
     */
    if (!in_flatarr)
        DDBE_properties_byte(type_p, &vip->ind_sp->ind_p);

    sprintf(comment, "%s array index", (in_flatarr) ? "flat" : "full");
    DDBE_reference_vec_entry(&vip->ind_sp->ind_p, vip->defn_p, comment);

    /*
     * If we're in a full (not flat) array definition and it is not the toplevel
     * of an array [of array]... construct, then we won't get back here for this
     * array, so emit flat array index which is the same as full array index.
     */
    if (!in_flatarr &&
        (base_type_p->kind != AST_array_k || IR_in_array(vip->ir_ctx_p) > 1))
    {
        DDBE_reference_vec_entry(&vip->ind_sp->ind_p, vip->defn_p,
            "flat array index");
        flat_is_full = TRUE;
    }
    else
        flat_is_full = FALSE;

    /*
     * If a conformant array within a struct, insert the flat array index at
     * the definition vector location saved in DDBE_op_conformant_info.
     */
    if ((in_flatarr || flat_is_full)
        && tup_p->opcode == IR_op_conformant_array_k
        && IR_parent_scope(vip->ir_ctx_p) == IR_SCP_STRUCT)
        DDBE_reference_vec_entry(&vip->ind_sp->saved_defn_p, vip->defn_p,
            "flat conformant array index");
}


/*
 *  D D B E _ o p _ a r r a y _ b o u n d s
 *
 *  Process IREP tuple that describes the start of array bounds information.
 */
static void DDBE_op_array_bounds
(
    IR_tup_n_t      *tup_p,         /* [in] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    AST_type_n_t    *type_p;        /* Ptr to AST struct type node */
    char const      *type_name;     /* Type name */
    char comment[DDBE_MAX_COMMENT]; /* Comment buffer */

    /*
     * This tuple follows all 4 types of arrays; it is preceded in the
     * intermediate rep by one of the four array_k tuples which when processed
     * sets the conformant flag appropriately.  It is always followed by
     * IR_op_bound_k tuples and, if the array is varying, IR_op_limit_k tuples.
     * On entry, we are in a separate data and indirection scope for the array.
     */
    type_p = tup_p->arg[IR_ARG_TYPE].type;  /* -> array type node */

    if (type_p->name == NAMETABLE_NIL_ID)
        type_name = "anon_array";
    else
        NAMETABLE_id_to_string(type_p->name, &type_name);

    /*
     * first definition vector entry for array:
     *  byte: dimensions
     */
    sprintf(comment, "array %s %s instance", type_name,
            (vip->ind_sp->in_flatarr) ? "flat" : "full");
    DDBE_comment_vec_entry(&vip->defn_p, comment);

    DDBE_byte_vec_entry(&vip->defn_p, DDBE_vec_byte_3m4_k,
        tup_p->arg[IR_ARG_INT].int_val, "num dimensions");
}

static void DDBE_func_code_vec_entry(
    DDBE_vec_rep_t  **p_defn_p,     /* [io] Ptr to vec entry to insert after */
	byte func_code /* function code */
	)
{
    char * name = NULL;
    STRTAB_str_t    name_id;

    switch(func_code)
    {
	case IR_EXP_FC_NONE:	name = "IDL_FC_NONE"; break;
	case IR_EXP_FC_DIV_2:	name = "IDL_FC_DIV_2"; break;
	case IR_EXP_FC_MUL_2:	name = "IDL_FC_MUL_2"; break;
	case IR_EXP_FC_SUB_1:	name = "IDL_FC_SUB_1"; break;
	case IR_EXP_FC_ADD_1:	name = "IDL_FC_ADD_1"; break;
	case IR_EXP_FC_ALIGN_2:	name = "IDL_FC_ALIGN_2"; break;
	case IR_EXP_FC_ALIGN_4:	name = "IDL_FC_ALIGN_4"; break;
	case IR_EXP_FC_ALIGN_8:	name = "IDL_FC_ALIGN_8"; break;
	case IR_EXP_FC_CALLBACK:name = "IDL_FC_CALLBACK"; break;
	case IR_EXP_FC_DIV_4:	name = "IDL_FC_DIV_4"; break;
	case IR_EXP_FC_MUL_4:	name = "IDL_FC_MUL_4"; break;
	case IR_EXP_FC_DIV_8:	name = "IDL_FC_DIV_8"; break;
	case IR_EXP_FC_MUL_8:	name = "IDL_FC_MUL_8"; break;
	case IR_EXP_FC_FIXED:	name = "IDL_FC_FIXED"; break;
	default:		return;
    }
    name_id = STRTAB_add_string(name);
    DDBE_expr_vec_entry(p_defn_p, DDBE_vec_expr_byte_k, name_id, "bound info");
}


/*
 *  D D B E _ o p _ b o u n d
 *
 *  Process IREP tuple that describes a single bound of an array.
 */
static void DDBE_op_bound
(
    IR_tup_n_t      *tup_p,         /* [in] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    IR_bound_k_t        bound_kind;         /* Bound kind */
    char                *bound_text = NULL;        /* Bound attr text for comment */
    AST_type_n_t        *ref_type_p;        /* Referenced type  in bound attr */
    AST_parameter_n_t   *ref_param_p;       /* Referenced param in bound attr */
    AST_field_n_t       *ref_field_p;       /* Referenced field in bound attr */
    long                ref_index;          /* Referenced index in bound attr */
    NAMETABLE_id_t      ref_name_id;        /* Ref'd param/field name ID */
    char const          *ref_name;          /* Ref'd param/field name */
    char const          *ref_text;          /* "field" or "param" */
    STRTAB_str_t        octetsize_expr;     /* Expression for octetsize */
    char        ref_expr[DDBE_MAX_COMMENT]; /* Comment for type vector entry */

    bound_kind = tup_p->arg[IR_ARG_BOUND].bound_k;

    /*
     * definition vector entries for fixed bound:
     *  byte: bound kind (iff array is conformant)
     *  long: bound value
     */
    if (bound_kind == IR_bnd_fixed_k)
    {
        if (vip->conformant)
            DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_fixed_bound);

        DDBE_long_vec_entry(&vip->defn_p, DDBE_vec_long_k,
            tup_p->arg[IR_ARG_INT].int_val, "fixed bound");

        return;
    }

    /*
     * definition vector entries for conformant bound:
     *  byte: bound kind
     *  byte: bound type (octetsize for strings)
     *  long: parameter number or field index (or dummy value for strings)
     */
    switch (bound_kind)
    {
    case IR_bnd_min_is_k:
        bound_text = "min_is";
        if (tup_p->flags & IR_CF_EARLY)
            DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_min_is_early_bound);
        else
            DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_min_is_bound);
        break;

    case IR_bnd_max_is_k:
        bound_text = "max_is";
        if (tup_p->flags & IR_CF_EARLY)
            DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_max_is_early_bound);
        else
            DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_max_is_bound);
        break;

    case IR_bnd_size_is_k:
        bound_text = "size_is";
        if (tup_p->flags & IR_CF_EARLY)
            DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_size_is_early_bound);
        else
            DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_size_is_bound);
        DDBE_func_code_vec_entry(&vip->defn_p, tup_p->arg[IR_ARG_BOUND_XTRA].byt_val);
        break;

    case IR_bnd_string_k:
        DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_string_bound);
        /* If base type of string is rep_as, octetsize is of local type */
        if (tup_p->flags & IR_REP_AS)
        {
            NAMETABLE_id_to_string(
                tup_p->arg[IR_ARG_TYPE2].type->rep_as_type->type_name,
                &ref_name);
            sprintf(ref_expr, "sizeof(%s)", ref_name);
            octetsize_expr = STRTAB_add_string(ref_expr);
            DDBE_expr_vec_entry(&vip->defn_p, DDBE_vec_expr_byte_k,
                octetsize_expr, "octetsize");
        }
        else
            DDBE_byte_vec_entry(&vip->defn_p, DDBE_vec_byte_k,
                tup_p->arg[IR_ARG_INT].int_val, "octetsize");
        /*
         * A conformant string within a struct requires the string field number
         * so an Interpreter can compute the string, and therefore struct, size.
         * The IREP stores a nonzero field number in the tuple for this case.
         */
        if (tup_p->arg[IR_ARG_PFNUM].int_val == 0)
            DDBE_long_vec_entry(&vip->defn_p, DDBE_vec_long_k, -1,
                "dummy value");
        else
            DDBE_long_vec_entry(&vip->defn_p, DDBE_vec_long_k,
                tup_p->arg[IR_ARG_PFNUM].int_val, "string field number");
        return;     /* Return to caller */

    default:
        INTERNAL_ERROR("Invalid bound kind");
    }

    /* Now pick up the parameter or field reference and get type */
    if (IR_under_struct(vip->ir_ctx_p))
    {
        ref_field_p = tup_p->arg[IR_ARG_FIELD].field;
        ref_index   = tup_p->arg[IR_ARG_PFNUM].int_val;
        ref_type_p  = ASTP_chase_ptr_to_type(ref_field_p->type);
        ref_name_id = ref_field_p->name;
        ref_text    = "field";
    }
    else
    {
        ref_param_p = tup_p->arg[IR_ARG_PARAM].param;
        ref_index   = tup_p->arg[IR_ARG_PFNUM].int_val;
        ref_type_p  = ASTP_chase_ptr_to_type(ref_param_p->type);
        ref_name_id = ref_param_p->name;
        ref_text    = "param";
    }

    DDBE_scalar_vec_entry(&vip->defn_p, ref_type_p, "bound type", vip);

    NAMETABLE_id_to_string(ref_name_id, &ref_name);
    sprintf(ref_expr, "%s number of %s reference", ref_text, bound_text);
    DDBE_long_vec_entry(&vip->defn_p, DDBE_vec_long_k, ref_index, ref_expr);
}


/*
 *  D D B E _ o p _ l i m i t
 *
 *  Process IREP tuple that describes a single data limit of an array.
 */
static void DDBE_op_limit
(
    IR_tup_n_t      *tup_p,         /* [in] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    IR_limit_k_t        limit_kind;         /* Data limit kind */
    char                *limit_text = NULL;        /* Limit attr text for comment */
    AST_type_n_t        *ref_type_p;        /* Referenced type  in limit attr */
    AST_parameter_n_t   *ref_param_p;       /* Referenced param in limit attr */
    AST_field_n_t       *ref_field_p;       /* Referenced field in limit attr */
    long                ref_index;          /* Referenced index in limit attr */
    NAMETABLE_id_t      ref_name_id;        /* Ref'd param/field name ID */
    char const          *ref_name;          /* Ref'd param/field name */
    char const          *ref_text;          /* "field" or "param" */
    STRTAB_str_t        octetsize_expr;     /* Expression for octetsize */
    char        ref_expr[DDBE_MAX_COMMENT]; /* Comment for type vector entry */

    /*
     * Return if we're processing header information for a conformant structure
     * since only the bounds information and base type are needed.
     */
    if (vip->in_cfmt_struct_hdr)
        return;

    limit_kind = tup_p->arg[IR_ARG_LIMIT].limit_k;

    /*
     * definition vector entries for fixed limit or string:
     *  byte: fixed limit kind
     *  long: limit value
     *
     * Note that for [string]s the interpreter must dynamically compute the
     * data length; in the intermediate rep data limits for strings are marked
     * by IR_lim_string_k with an integer value that is the octetsize in bytes.
     */
    if (limit_kind == IR_lim_fixed_k)
    {
        DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_fixed_limit);
        DDBE_long_vec_entry(&vip->defn_p, DDBE_vec_long_k,
            tup_p->arg[IR_ARG_INT].int_val, "fixed limit");
        return;
    }
    if (limit_kind == IR_lim_string_k)
    {
        DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_string_limit);

        /* If base type of string is rep_as, octetsize is of local type */
        if (tup_p->flags & IR_REP_AS)
        {
            NAMETABLE_id_to_string(
                tup_p->arg[IR_ARG_TYPE2].type->rep_as_type->type_name,
                &ref_name);
            sprintf(ref_expr, "sizeof(%s)", ref_name);
            octetsize_expr = STRTAB_add_string(ref_expr);
            DDBE_expr_vec_entry(&vip->defn_p, DDBE_vec_expr_byte_k,
                octetsize_expr, "octetsize");
        }
        else
            DDBE_byte_vec_entry(&vip->defn_p, DDBE_vec_byte_k,
                tup_p->arg[IR_ARG_INT].int_val, "octetsize");
        DDBE_long_vec_entry(&vip->defn_p, DDBE_vec_long_k, -1, "dummy value");
        return;
    }

    /*
     * definition vector entries for upper data limit that must be computed:
     *  byte: upper conf limit kind
     *  long: dummy value
     */
    if (limit_kind == IR_lim_upper_conf_k)
    {
        DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_upper_conf_limit);
        DDBE_long_vec_entry(&vip->defn_p, DDBE_vec_long_k, -1, "dummy value");
        return;
    }

    /*
     * definition vector entries for varying limit:
     *  byte: limit kind
     *  byte: limit type
     *  long: parameter number  or  field index
     */
    switch (limit_kind)
    {
    case IR_lim_first_is_k:
        limit_text = "first_is";
        if (tup_p->flags & IR_CF_EARLY)
            DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_first_is_early_limit);
        else
            DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_first_is_limit);
        break;

    case IR_lim_last_is_k:
        limit_text = "last_is";
        if (tup_p->flags & IR_CF_EARLY)
            DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_last_is_early_limit);
        else
            DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_last_is_limit);
        break;

    case IR_lim_length_is_k:
        limit_text = "length_is";
        if (tup_p->flags & IR_CF_EARLY)
            DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_length_is_early_limit);
        else
            DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_length_is_limit);
        DDBE_func_code_vec_entry(&vip->defn_p, tup_p->arg[IR_ARG_BOUND_XTRA].byt_val);
        break;

    default:
        INTERNAL_ERROR("Invalid data limit kind");
    }

    /* Now pick up the parameter or field reference and get type */
    if (IR_under_struct(vip->ir_ctx_p))
    {
        ref_field_p = tup_p->arg[IR_ARG_FIELD].field;
        ref_index   = tup_p->arg[IR_ARG_PFNUM].int_val;
        ref_type_p  = ASTP_chase_ptr_to_type(ref_field_p->type);
        ref_name_id = ref_field_p->name;
        ref_text    = "field";
    }
    else
    {
        ref_param_p = tup_p->arg[IR_ARG_PARAM].param;
        ref_index   = tup_p->arg[IR_ARG_PFNUM].int_val;
        ref_type_p  = ASTP_chase_ptr_to_type(ref_param_p->type);
        ref_name_id = ref_param_p->name;
        ref_text    = "param";
    }

    DDBE_scalar_vec_entry(&vip->defn_p, ref_type_p, "limit type", vip);

    NAMETABLE_id_to_string(ref_name_id, &ref_name);
    sprintf(ref_expr, "%s number of %s reference", ref_text, limit_text);
    DDBE_long_vec_entry(&vip->defn_p, DDBE_vec_long_k, ref_index, ref_expr);
}


/*
 *  D D B E _ o p _ a r r a y _ e n d
 *
 *  Process IREP tuple for end of an array definition.
 */
static void DDBE_op_array_end
(
    IR_tup_n_t      **p_tup_p,      /* [io] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    /*
     * If processing conformant struct header and type address on array_end
     * tuple matches saved type address from conformant_info tuple:
     */
    if (vip->in_cfmt_struct_hdr && (*p_tup_p)->arg[IR_ARG_TYPE].type ==
        vip->cfmt_info_tup_p->arg[IR_ARG_TUP].tup->arg[IR_ARG_TYPE].type)
    {
        /*
         * Done processing header information for a conformant structure.
         * Revert the tuple pointer back to the 'conformant info' tuple that
         * immediately follows the 'struct begin' tuple.
         */
        IR_process_tup(vip->ir_ctx_p, *p_tup_p);
        *p_tup_p = vip->cfmt_info_tup_p;
        DDBE_pop_indirection_scope(vip);  /* matches op_conformant_info push */
    }
    else
    {
        /*
         * Pop array data and indirection scope.
         */
        IR_process_tup(vip->ir_ctx_p, *p_tup_p);
        DDBE_pop_indirection_scope(vip);  /* matches op_array push */
    }
}


/*
 *  D D B E _ o p _ c o n f o r m a n t _ i n f o
 *
 *  Process IREP tuple that describes the conformance information for a
 *  conformant array that occurs as the field of a structure.
 */
static void DDBE_op_conformant_info
(
    IR_tup_n_t      **p_tup_p,      /* [io] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    IR_tup_n_t      *tup_p;

    tup_p = (*p_tup_p)->arg[IR_ARG_TUP].tup;    /* -> IR_op_*_array tuple */

    /*
     * This tuple immediately follows a 'struct begin' tuple for a conformant
     * struct and points to the conformant or open array tuples ahead.
     * 1) If array is conformant only, we can simply reference the same
     *    location that is referenced in the array field entry.  Save the
     *    current definition vector pointer so we can insert a reference to
     *    the flattened rep of the conformant array when we process it.
     * 2) If array is open, we must create a separate conformant-only array
     *    definition and reference it.  Follow the pointer to the open array
     *    tuples ahead, set a flag that says we're in a conformant array header,
     *    save this tuple addr so we can come back to it when done with header.
     */
    if (tup_p->opcode == IR_op_conformant_array_k)
        vip->saved_defn_p = vip->defn_p;
    else    /* opcode == IR_op_open_array_k */
    {
        /*
         * Push scope (now in array scope) and start child definition with
         * dummy entry so we can safely make entries in the parent.
         */
        IR_process_tup(vip->ir_ctx_p, tup_p);
        DDBE_push_indirection_scope(vip);
        DDBE_noop_vec_entry(&vip->defn_p);
        /*
         * struct (parent) definition vector entry:
         *  long: definition vector location of flattened conformant rep
         *        of the open array
         */
        DDBE_reference_vec_entry(&vip->ind_sp->ind_p, vip->defn_p,
            "flat conformant array index");
        /*
         * Within the array scope, flag that we're in conformant struct header,
         * and save address of 'conformant info' tuple to return to.
         */
        vip->in_cfmt_struct_hdr = TRUE;
        vip->cfmt_info_tup_p = *p_tup_p;
        vip->conformant = TRUE;
        /*
         * Update to the 'array bounds' tuple where processing will resume
         * normally until the end of the array then revert back to structure.
         */
        tup_p = tup_p->next;
        if (tup_p->opcode != IR_op_array_bounds_k)
            INTERNAL_ERROR("Logic error for conformant struct");
        *p_tup_p = tup_p;
        vip->update_tup = FALSE;
    }
}


/*
 *  D D B E _ o p _ p o i n t e r
 *
 *  Process IREP tuple for a pointer.
 */
static void DDBE_op_pointer
(
    IR_tup_n_t      **p_tup_p,      /* [io] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip,           /* [io] vector information */
    NAMETABLE_id_t  pointer_tag     /* [in] pointer tag name */
)
{
    IR_tup_n_t      *tup_p;         /* Ptr to intermediate rep tuple */

    tup_p = *p_tup_p;

    /*
     * offset vector entry (if pointer in struct):
     *  long: offset to pointer field
     */
    if (IR_cur_scope(vip->ir_ctx_p) == IR_SCP_STRUCT)
    {
        AST_field_n_t   *field_p;       /* Ptr to AST field node */
        char const      *field_name;    /* Field name */
        char comment[DDBE_MAX_COMMENT]; /* Comment buffer */

        field_p = tup_p->arg[IR_ARG_FIELD].field;
        NAMETABLE_id_to_string(field_p->name, &field_name);
        sprintf(comment, "field %s offset", field_name);

        DDBE_expr_vec_entry(&vip->offset_p, DDBE_vec_expr_k,
            IR_field_expr(vip->ir_ctx_p, field_p), comment);

        /*
         * If the field has the [ignore] attribute, simply emit an 'ignore' tag,
         * skip the pointee tuples, and return.
         */
        if (AST_IGNORE_SET(field_p))
        {
            DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_ignore);
            DDBE_skip_to_tup(p_tup_p, IR_op_pointee_end_k);
            return;
        }
    }

    /*
     * type/definition vector entry:
     *  byte: {ref | unique | full} pointer type
     */
    DDBE_tag_vec_entry(vip->p_cur_p, pointer_tag);

    /*
     * Push data scope to indicate we are now processing a pointee.
     */
    IR_process_tup(vip->ir_ctx_p, tup_p);

    /*
     * For parameters, structs, and unions, the pointee information is indirect-
     * ly referenced.  Push indirection scope and start the child pointee
     * definition with dummy entry so we can safely make entries in parent.
     */
    if (IR_parent_scope(vip->ir_ctx_p) != IR_SCP_TOPLEVEL
        && IR_parent_scope(vip->ir_ctx_p) != IR_SCP_STRUCT
        && IR_parent_scope(vip->ir_ctx_p) != IR_SCP_UNION)
        return;
    DDBE_push_indirection_scope(vip);
    DDBE_noop_vec_entry(&vip->defn_p);

    /*
     * type/definition vector entries:
     *  byte: properties byte
     *  long: definition vector location of pointee
     */
    DDBE_properties_byte(
        tup_p->arg[IR_ARG_TYPE].type->type_structure.pointer->pointee_type,
        &vip->ind_sp->ind_p);
    DDBE_reference_vec_entry(&vip->ind_sp->ind_p, vip->defn_p, "pointee index");

    /*
     * If pointer is non-default union arm, add pad bytes to vector
     * since all arms must occupy the same number of bytes.
     */
    if (IR_parent_scope(vip->ir_ctx_p) == IR_SCP_UNION && !vip->in_default_arm)
    {
        vip->arm_byte_cnt += 8; /* Account for tag, props, filler, index */
        DDBE_pad_vec_entry(&vip->ind_sp->ind_p,
            DDBE_ARM_SIZE - vip->arm_byte_cnt);
    }
}


/*
 *  D D B E _ o p _ p o i n t e e _ e n d
 *
 *  Process IREP tuple for end of a pointee definition.
 */
static void DDBE_op_pointee_end
(
    IR_tup_n_t      *tup_p,         /* [in] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    /* Pop pointee data scope */
    IR_process_tup(vip->ir_ctx_p, tup_p);

    /*
     * For parameters, structs, and unions, the pointee information is indirect-
     * ly referenced.  Pop pointee indirection scope.
     */
    if (IR_cur_scope(vip->ir_ctx_p) == IR_SCP_TOPLEVEL
        || IR_cur_scope(vip->ir_ctx_p) == IR_SCP_STRUCT
        || IR_cur_scope(vip->ir_ctx_p) == IR_SCP_UNION)
        DDBE_pop_indirection_scope(vip);
}


/*
 *  D D B E _ o p _ t r a n s m i t _ a s
 *
 *  Process IREP tuple for beginning of a [transmit_as] definition.
 */
static void DDBE_op_transmit_as
(
    IR_tup_n_t      **p_tup_p,      /* [io] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    IR_tup_n_t      *tup_p;         /* Ptr to intermediate rep tuple */
    AST_type_n_t    *type_p;        /* Ptr to transmissible type AST node */
    AST_type_n_t    *pres_p;        /* Ptr to presented type AST node */
    char const      *pres_name;     /* Presented type name */
    char            rtn_name[MAX_ID]; /* Routine name */
    char comment[DDBE_MAX_COMMENT]; /* Comment buffer */

    tup_p = *p_tup_p;

    /*
     * On entry, tup_p points at the tuple IR_op_transmit_as_k and the
     * transmit_as data scope has NOT yet been pushed on the data scope stack.
     * A separate indirection scope HAS been pushed since transmit_as types
     * are always referenced via IR_op_type_indirect_k.
     */
    type_p = tup_p->arg[IR_ARG_TYPE].type;  /* -> transmissible type */
    pres_p = tup_p->arg[IR_ARG_TYPE2].type; /* -> presented type */

    if (pres_p->name == NAMETABLE_NIL_ID)
        NAMETABLE_id_to_string(pres_p->type_structure.pointer->pointee_type->name, &pres_name);
		else
    NAMETABLE_id_to_string(pres_p->name, &pres_name);

    /*
     * If we are processing the [out] representation for an [in]-only
     * [transmit_as] parameter, do only the following type vector entries:
     *  byte: dt_free_rep
     *  long: routine vector location of _free_inst routine
     */
    if (vip->free_rep)
    {
        DDBE_tag_vec_entry(&vip->ind_sp->ind_p, DDBE_dt_free_rep);
        DDBE_reference_vec_entry(&vip->ind_sp->ind_p,
            pres_p->be_info.dd_type->rtn_vec_p->next->next, "_free_inst index");
        DDBE_skip_to_tup(p_tup_p, IR_op_transmit_end_k);
        return;
    }

    /*
     * Push data scope to work on transmissible type definition vector entries.
     * This also makes available to us the instance of the [transmit_as] type.
     */
    IR_process_tup(vip->ir_ctx_p, tup_p);

    /*
     * Do offset vector entries for presented type if not done yet.
     */
    if (pres_p->be_info.dd_type->offset_vec_p == NULL)
    {
        /*
         * offset vector entry:
         *  long: presented type size
         */
        sprintf(comment, "presented type %s size", pres_name);
        DDBE_comment_vec_entry(&vip->offset_p, comment);

        sprintf(comment, "sizeof %s", pres_name);
        DDBE_type_info_vec_entry(DDBE_vec_sizeof_k, &vip->offset_p, pres_p,
            comment);

        /* Store offset vector pointer for presented type */
        pres_p->be_info.dd_type->offset_vec_p = vip->offset_p;
    }

    /*
     * If in a structure, offset vector entry (in parent):
     *  long: expression for offset to this field
     */
    if (IR_parent_scope(vip->ir_ctx_p) == IR_SCP_STRUCT)
    {
        AST_field_n_t   *field_p;       /* Ptr to AST field node */
        char const      *field_name;    /* Field name */

        field_p = (AST_field_n_t *)IR_cur_inst(vip->ir_ctx_p);
        NAMETABLE_id_to_string(field_p->name, &field_name);
        sprintf(comment, "field %s offset", field_name);

        DDBE_expr_vec_entry(&vip->ind_sp->off_p, DDBE_vec_expr_k,
            IR_field_expr(vip->ir_ctx_p, field_p), comment);
    }

    /*
     * Now in [transmit_as] indirection scope; if its definition not done yet,
     * start with dummy entry so we can safely make entries in the parent.
     */
    if (!pres_p->be_info.dd_type->defn_done)
        DDBE_noop_vec_entry(&vip->defn_p);

    /*
     * type/definition vector entries (site of reference):
     *  byte: transmit_as tag
     *  byte: properties byte
     *  long: definition vector location of transmit_as info
     */
    DDBE_tag_vec_entry(&vip->ind_sp->ind_p, DDBE_dt_transmit_as);
    if (!pres_p->be_info.dd_type->defn_done)
        DDBE_reference_vec_entry(&vip->type_info_p, vip->ind_sp->ind_p, NULL);

    DDBE_properties_byte(type_p, &vip->ind_sp->ind_p);

    sprintf(comment, "type %s index", pres_name);
    DDBE_indirect_vec_entry(&vip->ind_sp->ind_p,
        &pres_p->be_info.dd_type->type_vec_p, comment);

    /*
     * If transmit_as is non-default union arm, add pad bytes to vector
     * since all arms must occupy the same number of bytes.
     */
    if (IR_parent_scope(vip->ir_ctx_p) == IR_SCP_UNION && !vip->in_default_arm)
    {
        vip->arm_byte_cnt += 8; /* Account for tag, props, filler, index */
        DDBE_pad_vec_entry(&vip->ind_sp->ind_p,
            DDBE_ARM_SIZE - vip->arm_byte_cnt);
    }

    /*
     * Do routine and definition vector entries if not done yet.
     * Note that these hang off the presented type.  The transmissible type is
     * handled by processing tuples that follow this one.  If definition entries
     * are already done, skip the transmissible type tuples.
     */
    if (pres_p->be_info.dd_type->defn_done)
    {
        DDBE_skip_to_tup(p_tup_p, IR_op_transmit_end_k);
        /* Must process tuple since matching transmit_as_k tuple done above */
        IR_process_tup(vip->ir_ctx_p, *p_tup_p);
        return;
    }

    pres_p->be_info.dd_type->defn_done = TRUE;

    /*
     * routine vector entries:
     *  addr: _to_xmit
     *  addr: _from_xmit
     *  addr: _free_inst
     *  addr: _free_xmit
     */
    sprintf(rtn_name, "%s%s", pres_name, DDBE_SUFFIX_TO_XMIT);
    DDBE_name_vec_entry(&vip->rtn_p, NAMETABLE_add_id(rtn_name), "");

    /* Store routine vector pointer for this type */
    pres_p->be_info.dd_type->rtn_vec_p = vip->rtn_p;

    sprintf(rtn_name, "%s%s", pres_name, DDBE_SUFFIX_FROM_XMIT);
    DDBE_name_vec_entry(&vip->rtn_p, NAMETABLE_add_id(rtn_name), "");

    sprintf(rtn_name, "%s%s", pres_name, DDBE_SUFFIX_FREE_INST);
    DDBE_name_vec_entry(&vip->rtn_p, NAMETABLE_add_id(rtn_name), "");

    sprintf(rtn_name, "%s%s", pres_name, DDBE_SUFFIX_FREE_XMIT);
    DDBE_name_vec_entry(&vip->rtn_p, NAMETABLE_add_id(rtn_name), "");

    /*
     * definition vector entries:
     *  long: offset vector location of presented type size
     *  long: routine vector location of presented type routines
     */
    sprintf(comment, "presented type %s definition", pres_name);
    DDBE_comment_vec_entry(&vip->defn_p, comment);

    sprintf(comment, "%s offset vector index", pres_name);
    DDBE_indirect_vec_entry(&vip->defn_p,
        &pres_p->be_info.dd_type->offset_vec_p, comment);

    /* Store definition vector pointer for [transmit_as] type */
    pres_p->be_info.dd_type->type_vec_p = vip->defn_p;

    sprintf(comment, "%s routine vector index", pres_name);
    DDBE_indirect_vec_entry(&vip->defn_p,
        &pres_p->be_info.dd_type->rtn_vec_p, comment);

    /*
     * The tuples that follow this one describe the transmissible type.  The
     * data scope is IR_SCP_XMIT_AS and we are in a separate indirection scope.
     */
}


/*
 *  D D B E _ o p _ r e p r e s e n t _ a s
 *
 *  Process IREP tuple for beginning of a [represent_as] definition.
 *  NOTE: For represent_as objects, the IREP tuples and backend info
 *  hang off the AST rep_as node rather than the AST type node.
 */
static void DDBE_op_represent_as
(
    IR_tup_n_t      **p_tup_p,      /* [io] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    IR_tup_n_t      *tup_p;         /* Ptr to intermediate rep tuple */
    AST_type_n_t    *type_p;        /* Ptr to network type AST node */
    AST_rep_as_n_t  *rep_p;         /* Ptr to represent_as AST node */
    char const      *type_name;     /* Network type name */
    char const      *local_name;    /* Local type name */
    char            rtn_name[MAX_ID]; /* Routine name */
    char comment[DDBE_MAX_COMMENT]; /* Comment buffer */
    char local_size[DDBE_MAX_EXPR]; /* Symbolic expr for sizeof local type */
    STRTAB_str_t    size_expr;

    tup_p = *p_tup_p;

    /*
     * On entry, tup_p points at the tuple IR_op_represent_as_k and the
     * represent_as data scope has NOT yet been pushed on the data scope stack.
     * A separate indirection scope HAS been pushed since represent_as types
     * are always referenced via IR_op_type_indirect_k.
     */
    type_p = tup_p->arg[IR_ARG_TYPE].type;      /* -> network type */
    rep_p = tup_p->arg[IR_ARG_REP_AS].rep_as;   /* -> represent_as node */

    NAMETABLE_id_to_string(type_p->name, &type_name);
    NAMETABLE_id_to_string(rep_p->type_name, &local_name);

    /*
     * If we are processing the [out] representation for an [in]-only
     * [represent_as] parameter, do only the following type vector entries:
     *  byte: dt_free_rep
     *  long: routine vector location of _free_local routine
     */
    if (vip->free_rep)
    {
        DDBE_tag_vec_entry(&vip->ind_sp->ind_p, DDBE_dt_free_rep);
        DDBE_reference_vec_entry(&vip->ind_sp->ind_p,
            rep_p->be_info.dd_type->rtn_vec_p->next->next, "_free_local index");
        DDBE_skip_to_tup(p_tup_p, IR_op_represent_end_k);
        return;
    }

    /*
     * Push data scope to work on network type definition vector entries.
     * This also makes available to us the instance of the [represent_as] type.
     */
    IR_process_tup(vip->ir_ctx_p, tup_p);

    /*
     * Do offset vector entries for represent_as type if not done yet.
     */
    if (rep_p->be_info.dd_type->offset_vec_p == NULL)
    {
        /*
         * offset vector entry:
         *  long: local type size
         */
        sprintf(comment, "local type %s size", local_name);
        DDBE_comment_vec_entry(&vip->offset_p, comment);

        sprintf(local_size, "sizeof(%s)", local_name);
        size_expr = STRTAB_add_string(local_size);
        sprintf(comment, "local type size");
        DDBE_expr_vec_entry(&vip->offset_p, DDBE_vec_expr_long_k, size_expr,
            comment);

        /* Store offset vector pointer for network type */
        rep_p->be_info.dd_type->offset_vec_p = vip->offset_p;
    }

    /*
     * If in a structure, offset vector entry (in parent):
     *  long: expression for offset to this field
     */
    if (IR_parent_scope(vip->ir_ctx_p) == IR_SCP_STRUCT)
    {
        AST_field_n_t   *field_p;       /* Ptr to AST field node */
        char const      *field_name;    /* Field name */

        field_p = (AST_field_n_t *)IR_cur_inst(vip->ir_ctx_p);
        NAMETABLE_id_to_string(field_p->name, &field_name);
        sprintf(comment, "field %s offset", field_name);

        DDBE_expr_vec_entry(&vip->ind_sp->off_p, DDBE_vec_expr_k,
            IR_field_expr(vip->ir_ctx_p, field_p), comment);
    }

    /*
     * Now in [represent_as] indirection scope; if its definition not done yet,
     * start with dummy entry so we can safely make entries in the parent.
     */
    if (!rep_p->be_info.dd_type->defn_done)
        DDBE_noop_vec_entry(&vip->defn_p);

    /*
     * type/definition vector entries (site of reference):
     *  byte: represent_as tag
     *  byte: properties byte
     *  long: definition vector location of represent_as info
     */
    DDBE_tag_vec_entry(&vip->ind_sp->ind_p, DDBE_dt_represent_as);
    if (!rep_p->be_info.dd_type->defn_done)
        DDBE_reference_vec_entry(&vip->type_info_p, vip->ind_sp->ind_p, NULL);

    DDBE_properties_byte(type_p, &vip->ind_sp->ind_p);

    sprintf(comment, "type %s index", type_name);
    DDBE_indirect_vec_entry(&vip->ind_sp->ind_p,
        &rep_p->be_info.dd_type->type_vec_p, comment);

    /*
     * If represent_as is non-default union arm, add pad bytes to vector
     * since all arms must occupy the same number of bytes.
     */
    if (IR_parent_scope(vip->ir_ctx_p) == IR_SCP_UNION && !vip->in_default_arm)
    {
        vip->arm_byte_cnt += 8; /* Account for tag, props, filler, index */
        DDBE_pad_vec_entry(&vip->ind_sp->ind_p,
            DDBE_ARM_SIZE - vip->arm_byte_cnt);
    }

    /*
     * Do routine and definition vector entries if not done yet.
     * Note that these hang off the rep_as node.  The network type is
     * handled by processing tuples that follow this one.  If definition
     * entries are already done, skip the represent_as tuples.
     */
    if (rep_p->be_info.dd_type->defn_done)
    {
        DDBE_skip_to_tup(p_tup_p, IR_op_represent_end_k);
        /* Must process tuple since matching represent_as_k tuple done above */
        IR_process_tup(vip->ir_ctx_p, *p_tup_p);
        return;
    }

    rep_p->be_info.dd_type->defn_done = TRUE;

    /*
     * routine vector entries:
     *  addr: _from_local
     *  addr: _to_local
     *  addr: _free_local
     *  addr: _free_inst
     */
    sprintf(rtn_name, "%s%s", type_name, DDBE_SUFFIX_FROM_LOCAL);
    DDBE_name_vec_entry(&vip->rtn_p, NAMETABLE_add_id(rtn_name), "");

    /* Store routine vector pointer for this type */
    rep_p->be_info.dd_type->rtn_vec_p = vip->rtn_p;

    sprintf(rtn_name, "%s%s", type_name, DDBE_SUFFIX_TO_LOCAL);
    DDBE_name_vec_entry(&vip->rtn_p, NAMETABLE_add_id(rtn_name), "");

    sprintf(rtn_name, "%s%s", type_name, DDBE_SUFFIX_FREE_LOCAL);
    DDBE_name_vec_entry(&vip->rtn_p, NAMETABLE_add_id(rtn_name), "");

    sprintf(rtn_name, "%s%s", type_name, DDBE_SUFFIX_FREE_INST);
    DDBE_name_vec_entry(&vip->rtn_p, NAMETABLE_add_id(rtn_name), "");

    /*
     * definition vector entries:
     *  long: offset vector location of local type size
     *  long: routine vector location of network type routines
     */
    sprintf(comment, "network type %s definition", type_name);
    DDBE_comment_vec_entry(&vip->defn_p, comment);

    sprintf(comment, "%s offset vector index", local_name);
    DDBE_indirect_vec_entry(&vip->defn_p,
        &rep_p->be_info.dd_type->offset_vec_p, comment);

    /* Store definition vector pointer for [represent_as] type */
    rep_p->be_info.dd_type->type_vec_p = vip->defn_p;

    sprintf(comment, "%s routine vector index", type_name);
    DDBE_indirect_vec_entry(&vip->defn_p,
        &rep_p->be_info.dd_type->rtn_vec_p, comment);

    /*
     * The tuples that follow this one describe the network type.  The
     * data scope is IR_SCP_REP_AS and we are in a separate indirection scope.
     */
}


/*
 *  D D B E _ o p _ c s _ c h a r
 *
 *  Process IREP tuple for beginning of a [cs_char] definition.
 *  NOTE: For cs_char objects, the IREP tuples and backend info
 *  hang off the AST cs_char node rather than the AST type node.
 */
static void DDBE_op_cs_char
(
    IR_tup_n_t      **p_tup_p,      /* [io] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    IR_tup_n_t      *tup_p;         /* Ptr to intermediate rep tuple */
    AST_type_n_t    *type_p;        /* Ptr to network type AST node */
    AST_cs_char_n_t *ichar_p;       /* Ptr to cs_char AST node */
    char const      *type_name;     /* Network type name */
    char const      *local_name;    /* Local type name */
    char            rtn_name[MAX_ID]; /* Routine name */
    char comment[DDBE_MAX_COMMENT]; /* Comment buffer */
    char local_size[DDBE_MAX_EXPR]; /* Symbolic expr for sizeof local type */
    STRTAB_str_t    size_expr;

    tup_p = *p_tup_p;

    /*
     * On entry, tup_p points at the tuple IR_op_cs_char_k and the
     * cs_char data scope has NOT yet been pushed on the data scope stack.
     * A separate indirection scope HAS been pushed since cs_char types
     * are always referenced via IR_op_type_indirect_k.
     */
    type_p = tup_p->arg[IR_ARG_TYPE].type;          /* -> network type */
    ichar_p = tup_p->arg[IR_ARG_CS_CHAR].cs_char;   /* -> cs_char node */

    NAMETABLE_id_to_string(type_p->name, &type_name);
    NAMETABLE_id_to_string(ichar_p->type_name, &local_name);

    /*
     * Push data scope to work on network type definition vector entries.
     * This also makes available to us the instance of the [cs_char] type.
     */
    IR_process_tup(vip->ir_ctx_p, tup_p);

    /*
     * Do offset vector entries for cs_char type if not done yet.
     */
    if (ichar_p->be_info.dd_type->offset_vec_p == NULL)
    {
        /*
         * offset vector entry:
         *  long: local type size
         */
        sprintf(comment, "local type %s size", local_name);
        DDBE_comment_vec_entry(&vip->offset_p, comment);

        sprintf(local_size, "sizeof(%s)", local_name);
        size_expr = STRTAB_add_string(local_size);
        sprintf(comment, "local type size");
        DDBE_expr_vec_entry(&vip->offset_p, DDBE_vec_expr_long_k, size_expr,
            comment);

        /* Store offset vector pointer for network type */
        ichar_p->be_info.dd_type->offset_vec_p = vip->offset_p;
    }

    /*
     * If in a structure, offset vector entry (in parent):
     *  long: expression for offset to this field
     */
    if (IR_parent_scope(vip->ir_ctx_p) == IR_SCP_STRUCT)
    {
        AST_field_n_t   *field_p;       /* Ptr to AST field node */
        char const      *field_name;    /* Field name */

        field_p = (AST_field_n_t *)IR_cur_inst(vip->ir_ctx_p);
        NAMETABLE_id_to_string(field_p->name, &field_name);
        sprintf(comment, "field %s offset", field_name);

        DDBE_expr_vec_entry(&vip->ind_sp->off_p, DDBE_vec_expr_k,
            IR_field_expr(vip->ir_ctx_p, field_p), comment);
    }

    /*
     * Now in [cs_char] indirection scope; if its definition not done yet,
     * start with dummy entry so we can safely make entries in the parent.
     */
    if (!ichar_p->be_info.dd_type->defn_done)
        DDBE_noop_vec_entry(&vip->defn_p);

    /*
     * type/definition vector entries (site of reference):
     *  byte: cs_type tag
     *  byte: properties byte
     *  long: definition vector location of cs_type info
     */
    DDBE_tag_vec_entry(&vip->ind_sp->ind_p, DDBE_dt_cs_type);
    if (!ichar_p->be_info.dd_type->defn_done)
        DDBE_reference_vec_entry(&vip->type_info_p, vip->ind_sp->ind_p, NULL);

    DDBE_properties_byte(type_p, &vip->ind_sp->ind_p);

    sprintf(comment, "type %s index", type_name);
    DDBE_indirect_vec_entry(&vip->ind_sp->ind_p,
        &ichar_p->be_info.dd_type->type_vec_p, comment);

    /*
     * If cs_char is non-default union arm, add pad bytes to vector
     * since all arms must occupy the same number of bytes.
     */
    if (IR_parent_scope(vip->ir_ctx_p) == IR_SCP_UNION && !vip->in_default_arm)
    {
        vip->arm_byte_cnt += 8; /* Account for tag, props, filler, index */
        DDBE_pad_vec_entry(&vip->ind_sp->ind_p,
            DDBE_ARM_SIZE - vip->arm_byte_cnt);
    }

    /*
     * Do routine and definition vector entries if not done yet.
     * Note that these hang off the cs_char node.  The network type is
     * handled by processing tuples that follow this one.  If definition
     * entries are already done, skip the cs_char tuples.
     */
    if (ichar_p->be_info.dd_type->defn_done)
    {
        DDBE_skip_to_tup(p_tup_p, IR_op_cs_char_end_k);
        /* Must process tuple since matching cs_char_k tuple done above */
        IR_process_tup(vip->ir_ctx_p, *p_tup_p);
        return;
    }

    ichar_p->be_info.dd_type->defn_done = TRUE;

    /*
     * routine vector entries:
     *  addr: _net_size
     *  addr: _to_netcs
     *  addr: _local_size
     *  addr: _from_netcs
     */
    sprintf(rtn_name, "%s%s", local_name, DDBE_SUFFIX_NET_SIZE);
    DDBE_name_vec_entry(&vip->rtn_p, NAMETABLE_add_id(rtn_name), "");

    /* Store routine vector pointer for this type */
    ichar_p->be_info.dd_type->rtn_vec_p = vip->rtn_p;

    sprintf(rtn_name, "%s%s", local_name, DDBE_SUFFIX_TO_NETCS);
    DDBE_name_vec_entry(&vip->rtn_p, NAMETABLE_add_id(rtn_name), "");

    sprintf(rtn_name, "%s%s", local_name, DDBE_SUFFIX_LOCAL_SIZE);
    DDBE_name_vec_entry(&vip->rtn_p, NAMETABLE_add_id(rtn_name), "");

    sprintf(rtn_name, "%s%s", local_name, DDBE_SUFFIX_FROM_NETCS);
    DDBE_name_vec_entry(&vip->rtn_p, NAMETABLE_add_id(rtn_name), "");

    /*
     * definition vector entries:
     *  long: offset vector location of local type size
     *  long: routine vector location of network type routines
     */
    sprintf(comment, "network type %s definition", type_name);
    DDBE_comment_vec_entry(&vip->defn_p, comment);

    sprintf(comment, "%s offset vector index", local_name);
    DDBE_indirect_vec_entry(&vip->defn_p,
        &ichar_p->be_info.dd_type->offset_vec_p, comment);

    /* Store definition vector pointer for [cs_char] type */
    ichar_p->be_info.dd_type->type_vec_p = vip->defn_p;

    sprintf(comment, "%s routine vector index", type_name);
    DDBE_indirect_vec_entry(&vip->defn_p,
        &ichar_p->be_info.dd_type->rtn_vec_p, comment);

    /*
     * The tuples that follow this one describe the network type.  The
     * data scope is IR_SCP_CS_CHAR and we are in a separate indirection scope.
     */
}

/*
 *  D D B E _ o p _ i n t e r f a c e
 *
 *  Process IREP tuple for an interface reference.
 */
static void DDBE_op_interface
(
    IR_tup_n_t      *tup_p,         /* [in] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
	AST_interface_n_t *int_p;       /* Ptr to interface node */
	AST_type_n_t    *type_p;        /* Ptr to interface type node */
	DDBE_type_i_t   *type_i_p;      /* Ptr to backend type info node */
	char const           *type_name;     /* Type name */
	int             i;
	char comment[DDBE_MAX_COMMENT]; /* Comment buffer */

	int_p = tup_p->arg[IR_ARG_INTFC].intfc;
	type_p = tup_p->arg[IR_ARG_TYPE].type;
	NAMETABLE_id_to_string(type_p->name, &type_name);

	/*
	 * Create a DDBE info node for the type if not yet done.
	 */
	if (type_p->be_info.dd_type == NULL)
		DDBE_type_info(type_p, tup_p->flags);

	type_i_p = type_p->be_info.dd_type;
	if (!type_i_p->defn_done)
	{
		/* Push indirection scope to add definition/routine vector entries */
		DDBE_push_indirection_scope(vip);

		/*
		 * definition vector entries:
		*  a number of fields for the interface UUID
		*/
		sprintf(comment, "interface %s type definition", type_name);
		DDBE_comment_vec_entry(&vip->defn_p, comment);
		/* Store definition vector pointer for this type */
		type_i_p->type_vec_p = vip->defn_p;
		DDBE_long_vec_entry(&vip->defn_p, DDBE_vec_long_k,
				int_p->uuid.time_low, NULL);
		DDBE_short_vec_entry(&vip->defn_p, DDBE_vec_short_k,
				int_p->uuid.time_mid, NULL);
		DDBE_short_vec_entry(&vip->defn_p, DDBE_vec_short_k,
				int_p->uuid.time_hi_and_version, NULL);
		DDBE_byte_vec_entry(&vip->defn_p, DDBE_vec_byte_k,
				int_p->uuid.clock_seq_hi_and_reserved, NULL);
		DDBE_byte_vec_entry(&vip->defn_p, DDBE_vec_byte_k,
				int_p->uuid.clock_seq_low, NULL);
		for (i = 0; i < 6; i++)
		{
			DDBE_byte_vec_entry(&vip->defn_p, DDBE_vec_byte_k,
					int_p->uuid.node[i], NULL);
		}

		/*
		 * routine vector entries:
		*  addr: object ref to wire rep routine
		*  addr: wire rep to object ref routine
		*/
		DDBE_name_vec_entry(&vip->rtn_p,
				NAMETABLE_add_id("rpc_ss_ndr_orpc_oref_to_wire_rep"), "");
		/* Store routine vector pointer for this type */
		type_i_p->rtn_vec_p = vip->rtn_p;

		DDBE_name_vec_entry(&vip->rtn_p,
				NAMETABLE_add_id("rpc_ss_ndr_orpc_wire_rep_to_oref"), "");

		sprintf(comment, "interface %s routines", type_name);
		DDBE_reference_vec_entry(&vip->defn_p, type_i_p->rtn_vec_p, comment);

		DDBE_pop_indirection_scope(vip);
		type_i_p->defn_done = TRUE;
	}

	/*
	 * offset vector entry (if interface reference in struct):
	*  long: offset to interface reference field
	*/
	if (IR_cur_scope(vip->ir_ctx_p) == IR_SCP_STRUCT)
	{
		AST_field_n_t   *field_p;       /* Ptr to AST field node */
		char  const          *field_name;    /* Field name */

		field_p = tup_p->arg[IR_ARG_FIELD].field;
		NAMETABLE_id_to_string(field_p->name, &field_name);
		sprintf(comment, "field %s offset", field_name);

		DDBE_expr_vec_entry(&vip->offset_p, DDBE_vec_expr_k,
				IR_field_expr(vip->ir_ctx_p, field_p), comment);
	}

	/*
	 * type/definition vector entries:
	*  byte: interface tag
	*  byte: properties byte
	*  long: definition vector location of interface description
	*/
	DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_interface);
	DDBE_properties_byte(type_p, vip->p_cur_p);

	sprintf(comment, "interface %s index", type_name);
	DDBE_reference_vec_entry(vip->p_cur_p, type_i_p->type_vec_p, comment);
}

/*
 *  D D B E _ o p _ r a n g e
 *
 *  Process IREP tuple that describes the start of scalar type
 *  bounds information ([range] attribute)
 */
static void DDBE_op_range
(
    IR_tup_n_t      *tup_p,         /* [in] ptr to intermediate rep tuple */
    DDBE_vectors_t  *vip            /* [io] vector information */
)
{
    char comment[DDBE_MAX_COMMENT]; /* Comment buffer */

    sprintf(comment, "range(%lu,%lu)",
            tup_p->arg[IR_ARG_INT].int_val,
            tup_p->arg[IR_ARG_BOUND_XTRA].int_val);
    DDBE_comment_vec_entry(&vip->defn_p, comment);

    DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_range);

    DDBE_long_vec_entry(vip->p_cur_p, DDBE_vec_long_k,
        tup_p->arg[IR_ARG_INT].int_val,        "[range] low-val");
    DDBE_long_vec_entry(vip->p_cur_p, DDBE_vec_long_k,
        tup_p->arg[IR_ARG_BOUND_XTRA].int_val, "[range] high-val");
}

/*
 *  D D B E _ g e n _ p a r a m _ r e p s
 *
 *  Generates information for a parameter and its contained types into the
 *  vector information data structure.
 *
 *  Returns the address of the first type vector entry for the parameter.
 *  This address is also stored in the DDBE private parameter info node.
 */
static DDBE_vec_rep_t *DDBE_gen_param_reps
(
    AST_interface_n_t   *int_p ATTRIBUTE_UNUSED,     /* [in] ptr to interface node */
    AST_operation_n_t   *oper_p,    /* [in] ptr to operation node */
    AST_parameter_n_t   *param_p,   /* [in] ptr to parameter node */
    DDBE_vectors_t      *vip,       /* [in] vector information pointer */
    boolean             *cmd_opt ATTRIBUTE_UNUSED,   /* [in] array of cmd option flags */
    void                **cmd_val ATTRIBUTE_UNUSED  /* [in] array of cmd option values */
)
{
    AST_type_n_t    *type_p;        /* Type node pointer */
    char const      *name;          /* Variable name */
    char const      *oper_name;     /* Operation name */
    IR_tup_n_t      *tup_p;         /* Intermediate rep tuple pointer */
    DDBE_vec_rep_t  *first_entry;   /* Ptr to first type vec entry for param */
    char comment[DDBE_MAX_COMMENT]; /* Comment buffer */

    /*
     * Start off by pointing at first parameter tuple and current vector
     * being the type vector.
     */
    tup_p       = param_p->data_tups;

    /*
     * No vector representation for these cases:
     *   1) No intermediate representation for parameter
     *   2) ACF-added [comm_status] or [fault_status] parameter
     *   3) binding handle_t parameter with [represent_as] (for this case,
     *      the necessary work is done in stubs rather than in Interpreter)
     */
    if (IR_NO_IREP(tup_p))
        return NULL;
    if (AST_ADD_COMM_STATUS_SET(param_p) || AST_ADD_FAULT_STATUS_SET(param_p))
        return NULL;
    if (vip->param_num == 1 && param_p->type->kind == AST_handle_k
        && param_p->type->rep_as_type != NULL)
        return NULL;

    /* Re-initialize vector information fields */
    DDBE_VEC_INIT(vip);

    /*
     * type vector entry:
     *  long: parameter number
     */
    NAMETABLE_id_to_string(param_p->name, &name);
    NAMETABLE_id_to_string(oper_p->name, &oper_name);
    sprintf(comment, "Operation %s parameter %s", oper_name, name);
    DDBE_comment_vec_entry(&vip->type_p, comment);

    sprintf(comment, "index of parameter %s", name);
    DDBE_long_vec_entry(&vip->type_p, DDBE_vec_long_k, vip->param_num, comment);
    first_entry = vip->type_p;

    while (TRUE)
    {
        /* By default, update to next tuple at end of loop */
        vip->update_tup = TRUE;

        /*
         * If we reach the end of an tuple list hanging off a type node,
         * return to the next parameter tuple where we left off.
         * If we reach the end of the parameter tuple list, we're done.
         */
        if (tup_p == NULL && vip->tup_sp != NULL)
            DDBE_type_indirect_end(&tup_p, vip);
        if (tup_p == NULL)
            break;

        /* Pick up type pointer, which is valid for most tuples. */
        type_p = tup_p->arg[IR_ARG_TYPE].type;

        switch (tup_p->opcode)
        {
        case IR_op_align_k:
            /*
             * type/definition vector entries:
             *  byte: dt_ndr_align_N
             */
            switch (tup_p->arg[IR_ARG_INT].int_val)
            {
            case 2:
                DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_ndr_align_2);
                break;
            case 4:
                DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_ndr_align_4);
                break;
            case 8:
                DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_ndr_align_8);
                break;
            default:
                INTERNAL_ERROR("Invalid alignment value");
            }
            break;

        case IR_op_allocate_k:
            /*
             * This tuple is not meaningful for the interpreter; skip it.
             */
            break;

        case IR_op_array_bounds_k:
            DDBE_op_array_bounds(tup_p, vip);
            break;

        case IR_op_array_end_k:
            DDBE_op_array_end(&tup_p, vip);
            break;

        case IR_op_bound_k:
            DDBE_op_bound(tup_p, vip);
            break;

        case IR_op_call_k:
        case IR_op_call_param_k:
            /*
             * These tuples are not meaningful for the interpreter; skip them.
             */
            break;

        case IR_op_case_k:
            /*
             * definition vector entry:
             *  long: switch value
             *  byte: dt_void (iff empty arm)
             */
            DDBE_long_vec_entry(&vip->defn_p, (tup_p->flags & IR_BOOLEAN) ?
                DDBE_vec_long_bool_k : DDBE_vec_long_k,
                tup_p->arg[IR_ARG_INT].int_val, "switch value");
            vip->in_default_arm = FALSE;
            vip->arm_byte_cnt = 4;
            if (tup_p->flags & IR_VOID)
            {
                DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_void);
                vip->arm_byte_cnt++;    /* Account for void tag */
                /* Add pad bytes since all arms must occupy same number bytes */
                DDBE_pad_vec_entry(&vip->defn_p,
                    DDBE_ARM_SIZE - vip->arm_byte_cnt);
            }
            break;

        case IR_op_codeset_shadow_k:
            /*
             * type/definition vector entries (conditional for parameter):
             *  byte: codeset shadow tag
             *  long: number of fields or parameters
             */
            if (IR_cur_scope(vip->ir_ctx_p) == IR_SCP_TOPLEVEL)
            {
                if ((tup_p->flags & IR_IN_ONLY) && vip->processing_outs)
                    break;
                if ((tup_p->flags & IR_OUT_ONLY) && vip->processing_ins)
                    break;
            }
            DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_cs_shadow);
            if (IR_cur_scope(vip->ir_ctx_p) == IR_SCP_TOPLEVEL)
            {
                AST_parameter_n_t *pp;
                unsigned long num_params = 1;   /* always count fn result */
                for (pp = oper_p->parameters; pp != NULL; pp = pp->next)
                    num_params++;
                DDBE_long_vec_entry(vip->p_cur_p, DDBE_vec_long_k,
                    num_params, "number of parameters");
            }
            else    /* IR_in_struct(vip->ir_ctx_p) */
            {
                DDBE_long_vec_entry(vip->p_cur_p, DDBE_vec_long_k,
                    tup_p->arg[IR_ARG_INT].int_val, "number of fields");
            }
            break;

        case IR_op_conformant_array_k:
            /*
             * type/definition vector entries (if not in flat array rep):
             *  byte: i-char tag   (iff base type has [cs_char])
             *  byte: allocate tag (iff toplevel and 'allocate' flag set)
             *  byte: conformant array type
             */
            if (!vip->in_flatarr)
            {
                if (tup_p->flags & IR_CS_CHAR)
                    DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_cs_array);
                if (vip->allocate
                    && IR_cur_scope(vip->ir_ctx_p) == IR_SCP_TOPLEVEL)
                    DDBE_tag_vec_entry(&vip->type_p, DDBE_dt_allocate);
                DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_conf_array);
            }

            /* common type/definition vector entries for arrays */
            DDBE_op_array(tup_p, vip);

            vip->conformant = TRUE;
            break;

        case IR_op_conformant_info_k:
            DDBE_op_conformant_info(&tup_p, vip);
            break;

        case IR_op_context_handle_k:
            /*
             * type vector entry:
             *  byte: context handle tag
             */
            /* Context handle only supported on parameter */
            if (param_p != tup_p->arg[IR_ARG_PARAM].param)
                INTERNAL_ERROR("Unsupported context handle instance");
            if (!AST_OUT_SET(param_p))
                DDBE_tag_vec_entry(&vip->type_p, DDBE_dt_in_context);
            else if (!AST_IN_SET(param_p))
                DDBE_tag_vec_entry(&vip->type_p, DDBE_dt_out_context);
            else
                DDBE_tag_vec_entry(&vip->type_p, DDBE_dt_in_out_context);
            break;

        case IR_op_cs_char_k:
            DDBE_op_cs_char(&tup_p, vip);
            break;

        case IR_op_cs_char_end_k:
            /* Pop network type data scope */
            IR_process_tup(vip->ir_ctx_p, tup_p);
            break;

        case IR_op_declare_k:
            /*
             * This tuple is not meaningful for the interpreter; skip it.
             */
            break;

        case IR_op_default_k:
            /*
             * definition vector entry for default case of discriminated union:
             *  byte: dt_void           (iff empty arm)
             *        dt_does_not_exist (iff no default)
             *
             * If there is a default type then tuples for it follow this tuple -
             * only action needed here is to flag that in default arm.
             */
            if (tup_p->flags & IR_VOID)
                DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_void);
            else if (type_p == NULL)
                DDBE_tag_vec_entry(&vip->defn_p, DDBE_dt_does_not_exist);
            else
                vip->in_default_arm = TRUE;
            break;

        case IR_op_disc_union_begin_k:
            DDBE_op_disc_union_begin(&tup_p, vip);
            break;

        case IR_op_disc_union_end_k:
            DDBE_op_disc_union_end(&tup_p, vip);
            break;

        case IR_op_fixed_array_k:
            /*
             * type/definition vector entries (if not in flat array rep):
             *  byte: i-char tag (iff base type has [cs_char])
             *  byte: fixed array type
             */
            if (!vip->in_flatarr)
            {
                if (tup_p->flags & IR_CS_CHAR)
                    DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_cs_array);
                DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_fixed_array);
            }

            /* common type/definition vector entries for arrays */
            DDBE_op_array(tup_p, vip);

            vip->conformant = FALSE;
            break;

        case IR_op_flat_array_k:
            /* Mark that we're in flattened array tuples */
            vip->in_flatarr = TRUE;
            break;

        case IR_op_free_k:
            /*
             * This tuple is not meaningful for the interpreter; skip it.
             */
            break;

        case IR_op_full_array_end_k:
            /* No longer in flattened array tuples */
            vip->in_flatarr = FALSE;
            break;

        case IR_op_full_array_k:
            /* Starting full definition of array of array - no action needed */
            break;

        case IR_op_full_ptr_k:
            DDBE_op_pointer(&tup_p, vip, DDBE_dt_full_ptr);
            break;

		  case IR_op_interface_k:
				DDBE_op_interface(tup_p, vip);
				break;
				
        case IR_op_limit_k:
            DDBE_op_limit(tup_p, vip);
            break;

        case IR_op_marshall_k:
            DDBE_op_marshall(tup_p, vip);
            break;

        case IR_op_noop_k:
            break;

        case IR_op_open_array_k:
            /*
             * If a string, forget we're in a flat array rep if we are, since
             * in arrays of strings the minor dimension is not flattened out.
             */
            if (tup_p->flags & IR_STRING)
                vip->in_flatarr = FALSE;

            /*
             * type/definition vector entries (if not in flat array rep):
             *  byte: i-char tag   (iff base type has [cs_char])
             *  byte: allocate tag (iff toplevel and 'allocate' flag set)
             *  byte: string tag (iff string) or v1 array tag (iff v1 array)
             *  byte: open array type
             */
            if (!vip->in_flatarr)
            {
                if (tup_p->flags & IR_CS_CHAR)
                    DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_cs_array);

                if (vip->allocate
                    && IR_cur_scope(vip->ir_ctx_p) == IR_SCP_TOPLEVEL)
                    DDBE_tag_vec_entry(&vip->type_p, DDBE_dt_allocate);

                if (tup_p->flags & IR_STRING)
                    DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_string);

                if (AST_SMALL_SET(type_p)
                    || (tup_p->arg[IR_ARG_INST].inst != NULL
                        && AST_SMALL_SET(tup_p->arg[IR_ARG_INST].inst)))
                    DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_v1_array);

                DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_open_array);
            }

            /* common type/definition vector entries for arrays */
            DDBE_op_array(tup_p, vip);

            vip->conformant = TRUE;
            break;

        case IR_op_passed_by_ref_k:
            /*
             * type vector entry:
             *  byte: passed by reference
             *  byte: allocate_ref tag (if necessary)
             */
            DDBE_tag_vec_entry(&vip->type_p, DDBE_dt_passed_by_ref);
            /*
             * Generate tag entry if processing the [in] representation of an
             * [out]-only parameter for which [ref] pointees must be allocated.
             */
            if (vip->allocate_ref)
                DDBE_tag_vec_entry(&vip->type_p, DDBE_dt_allocate_ref);
            break;

        case IR_op_pipe_begin_k:
            DDBE_op_pipe_begin(&tup_p, vip);
            break;

        case IR_op_pipe_end_k:
            DDBE_op_pipe_end(tup_p, vip);
            break;

        case IR_op_pointee_end_k:
            /*
             * Process tuple unless it matches passed_by_ref tuple, for which we
             * simply generate tag but don't process tuple or push data scope.
             */
            if (IR_cur_scope(vip->ir_ctx_p) != IR_SCP_TOPLEVEL)
                DDBE_op_pointee_end(tup_p, vip);
            break;

        case IR_op_ref_ptr_k:
            DDBE_op_pointer(&tup_p, vip, DDBE_dt_ref_ptr);
            break;

        case IR_op_release_shadow_k:
            /*
             * type/definition vector entry (conditional for parameter):
             *  byte: release shadow tag
             */
            if (IR_cur_scope(vip->ir_ctx_p) == IR_SCP_TOPLEVEL)
            {
                if ((tup_p->flags & IR_IN_ONLY) && vip->processing_outs)
                    break;
                if ((tup_p->flags & IR_OUT_ONLY) && vip->processing_ins)
                    break;
            }
            DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_cs_rlse_shadow);
            break;

        case IR_op_represent_as_k:
            DDBE_op_represent_as(&tup_p, vip);
            break;

        case IR_op_represent_end_k:
            /* Pop network type data scope */
            IR_process_tup(vip->ir_ctx_p, tup_p);
            break;

        case IR_op_struct_begin_k:
            DDBE_op_struct_begin(&tup_p, vip);
            break;

        case IR_op_struct_end_k:
            DDBE_op_struct_end(&tup_p, vip);
            break;

        case IR_op_switch_enc_k:
            /*
             * Should not get here - this tuple eaten by union processing.
             */
            INTERNAL_ERROR("switch_enc_k tuple outside of union processing");
            break;

        case IR_op_switch_n_e_k:
            /*
             * This tuple precedes a non-encapsulated discriminated union and
             * provides the discriminator parameter or field.  Flag that the
             * union to follow is non-encapsulated and save param|field index.
             * The saved information must only be referenced at the beginning
             * of union processing before we process other types, which might
             * also be unions.
             */
            vip->switch_name  = tup_p->arg[IR_ARG_INST].inst->name;
            vip->switch_type  = tup_p->arg[IR_ARG_INST].inst->type;
            vip->switch_index = tup_p->arg[IR_ARG_PFNUM].int_val;
            break;

        case IR_op_transmit_as_k:
            DDBE_op_transmit_as(&tup_p, vip);
            break;

        case IR_op_transmit_end_k:
            /* Pop transmissible type data scope */
            IR_process_tup(vip->ir_ctx_p, tup_p);
            break;

        case IR_op_type_indirect_k:
            DDBE_op_type_indirect(&tup_p, vip);
            break;

        case IR_op_unique_ptr_k:
            DDBE_op_pointer(&tup_p, vip, DDBE_dt_unique_ptr);
            break;

        case IR_op_varying_array_k:
            /*
             * If a string, forget we're in a flat array rep if we are, since
             * in arrays of strings the minor dimension is not flattened out.
             */
            if (tup_p->flags & IR_STRING)
                vip->in_flatarr = FALSE;

            /*
             * type/definition vector entry (if not in flat array rep):
             *  byte: i-char tag (iff base type has [cs_char])
             *  byte: string tag (iff string)
             *  byte: varying array type
             */
            if (!vip->in_flatarr)
            {
                if (tup_p->flags & IR_CS_CHAR)
                    DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_cs_array);
                if ((tup_p->flags & IR_STRING) != 0)
                {
                    if (AST_STRING0_SET(type_p)
                        || (tup_p->arg[IR_ARG_INST].inst != NULL
                            && AST_STRING0_SET(tup_p->arg[IR_ARG_INST].inst)))
                        DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_v1_string);
                    else
                        DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_string);
                }
                else
                {
                    if (AST_SMALL_SET(type_p)
                        || (tup_p->arg[IR_ARG_INST].inst != NULL
                            && AST_SMALL_SET(tup_p->arg[IR_ARG_INST].inst)))
                        DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_v1_array);
                }
                DDBE_tag_vec_entry(vip->p_cur_p, DDBE_dt_varying_array);
            }

            /* common type/definition vector entries for arrays */
            DDBE_op_array(tup_p, vip);

            vip->conformant = FALSE;
            break;

        case IR_op_range_k:
            DDBE_op_range(tup_p, vip);
            break;

        default:
            INTERNAL_ERROR("Unknown intermediate rep tuple skipped");
            break;
        }

        /* On to next intermediate rep tuple */
        if (vip->update_tup)
        {
            tup_p = tup_p->next;
        }
    }

    /*
     * type vector entry:
     *  byte: end of list
     */
    DDBE_tag_vec_entry(&vip->type_p, DDBE_dt_eol);

    param_p->be_info.dd_param->type_vec_p = first_entry;
    return first_entry;
}


/*
 *  D D B E _ o p e r _ i n _ o u t _ o p t
 *
 *  Returns TRUE if the '[in,out] optimization' is possible, which is when the
 *  [in] parameters are contiguous and the [out] parameters are contiguous.
 *  Specifically, if the parameter ordering is either:
 *   1) all [in]s followed by all [in,out]s followed by
 *      a) all [out]s, or b) all [in]s and no function result, OR
 *   2) all [out]s followed by all [in,out]s followed by
 *      a) all [in]s and no function result, or b) all [out]s
 *  Non-marshallable parameters are skipped, e.g. handle_t parameters.
 */
static boolean DDBE_oper_in_out_opt
(
    AST_operation_n_t   *oper_p     /* [in] ptr to AST operation node */
)
{
    AST_parameter_n_t   *param_p;   /* Ptr to AST parameter node */

    /*
     * If the operation has any pipes the optimization is never done.
     * TBD: Some optimization could still be done - note [in] pipes must
     * follow all other [in]s and [out] pipes must precede all other [out]s.
     */
    if (AST_HAS_IN_PIPES_SET(oper_p) || AST_HAS_OUT_PIPES_SET(oper_p))
        return FALSE;

    /*
     * If any parameter is an [out]-only conformant array, the optimization is
     * not possible since the parameter needs a separate [in] definition which
     * tells the server stub to allocate storage before calling the manager.
     * Same if parameter is flagged as needing [ref] pointees allocated.
     * If any parameter is [in]-only with [transmit_as] and/or [represent_as],
     * the optimization is not possible since the parameter needs a separate
     * [out] definition which tells the server stub to call a 'free' routine.
     */
    for (param_p = oper_p->parameters; param_p != NULL; param_p = param_p->next)
    {
        if (param_p->ir_info->allocate_ref || DDBE_ALLOCATE(param_p))
            return FALSE;
        if (AST_IN_SET(param_p) && !AST_OUT_SET(param_p)
            && DDBE_XMIT_REP(param_p))
            return FALSE;
    }

    /*
     * If there are [in,out] parameters with only one of the two flags
     * FE_FIRST_IN_NF_CS_ARR and FE_FIRST_OUT_NF_CS_ARR set, don't optimize.
     * Only the first marshallable [in] and/or [out] has the corr. flag set.
     * If there are [in,out] parameters with only one of the two flags
     * FE_LAST_IN_NF_CS_ARR and FE_LAST_OUT_NF_CS_ARR set, don't optimize.
     * Only the last marshallable [in] and/or [out] has the corr. flag set.
     */
    for (param_p = oper_p->parameters; param_p != NULL; param_p = param_p->next)
    {
        if (AST_IN_SET(param_p) && AST_OUT_SET(param_p))
        {
            if (FE_TEST(param_p->fe_info->flags, FE_FIRST_IN_NF_CS_ARR))
            {
                if (!FE_TEST(param_p->fe_info->flags, FE_FIRST_OUT_NF_CS_ARR))
                    return FALSE;
            }
            else
            {
                if (FE_TEST(param_p->fe_info->flags, FE_FIRST_OUT_NF_CS_ARR))
                    return FALSE;
            }
            if (FE_TEST(param_p->fe_info->flags, FE_LAST_IN_NF_CS_ARR))
            {
                if (!FE_TEST(param_p->fe_info->flags, FE_LAST_OUT_NF_CS_ARR))
                    return FALSE;
            }
            else
            {
                if (FE_TEST(param_p->fe_info->flags, FE_LAST_OUT_NF_CS_ARR))
                    return FALSE;
            }
        }
    }

    /*
     * 1) all [in]s followed by all [in,out]s followed by
     *    a) all [out]s, or b) all [in]s and no function result, OR
     */
    param_p = oper_p->parameters;

    /* all [in]s */
    while (param_p != NULL
            && ( (AST_IN_SET(param_p) && !AST_OUT_SET(param_p))
                || IR_NO_IREP(param_p->data_tups) ))
        param_p = param_p->next;

    /* followed by all [in,out]s */
    while (param_p != NULL
            && ( (AST_IN_SET(param_p) && AST_OUT_SET(param_p))
                || IR_NO_IREP(param_p->data_tups) ))
        param_p = param_p->next;

    if (param_p == NULL)
        return TRUE;    /* no more params, fn result (if any) is [out] => OK */

    if (AST_OUT_SET(param_p))
    {
        /* followed by all [out]s */
        while (param_p != NULL
                && ( (!AST_IN_SET(param_p) && AST_OUT_SET(param_p))
                    || IR_NO_IREP(param_p->data_tups) ))
            param_p = param_p->next;
    }
    else
    {
        /* followed by all [in]s and no function result */
        if (oper_p->result == NULL || IR_NO_IREP(oper_p->result->data_tups))
        {
            while (param_p != NULL
                    && ( (AST_IN_SET(param_p) && !AST_OUT_SET(param_p))
                        || IR_NO_IREP(param_p->data_tups) ))
                param_p = param_p->next;
        }
    }

    if (param_p == NULL)
        return TRUE;

    /*
     * 2) all [out]s followed by all [in,out]s followed by
     *    a) all [in]s and no function result, or b) all [out]s
     */
    param_p = oper_p->parameters;

    /* all [out]s */
    while (param_p != NULL
            && ( (!AST_IN_SET(param_p) && AST_OUT_SET(param_p))
                || IR_NO_IREP(param_p->data_tups) ))
        param_p = param_p->next;

    /* followed by all [in,out]s */
    while (param_p != NULL
            && ( (AST_IN_SET(param_p) && AST_OUT_SET(param_p))
                || IR_NO_IREP(param_p->data_tups) ))
        param_p = param_p->next;

    if (param_p == NULL)
        return TRUE;

    if (AST_IN_SET(param_p))
    {
        /* followed by all [in]s and no function result */
        if (oper_p->result != NULL && !IR_NO_IREP(oper_p->result->data_tups))
            return FALSE;
        while (param_p != NULL
                && ( (AST_IN_SET(param_p) && !AST_OUT_SET(param_p))
                    || IR_NO_IREP(param_p->data_tups) ))
            param_p = param_p->next;
    }
    else
    {
        /* followed by all [out]s */
        while (param_p != NULL
                && ( (!AST_IN_SET(param_p) && AST_OUT_SET(param_p))
                    || IR_NO_IREP(param_p->data_tups) ))
            param_p = param_p->next;
    }

    if (param_p == NULL)
        return TRUE;

    return FALSE;
}


/*
 *  D D B E _ g e n _ v e c t o r _ r e p s
 *
 *  Generates information for all parameters of all operations and any contained
 *  types into the vector information data structure.
 */
static void DDBE_gen_vector_reps
(
    AST_interface_n_t   *int_p,     /* [in] ptr to interface node */
    DDBE_vectors_t      *vip,       /* [in] vector information pointer */
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val   /* [in] array of cmd option values */
)
{
    /*
     * At this level we have both a definition vector and a type vector which
     * will be concatenated into a single "type vector" passed back to caller
     * in the vector information.
     */
    DDBE_vec_rep_t      *defn_h;    /* Ptr to head of definition vector info */
    DDBE_vec_rep_t      *type_h;    /* Ptr to head of type vector info */
    DDBE_vec_rep_t      *type_l;    /* Ptr to last type vector entry */
    DDBE_vec_rep_t      *offset_h;  /* Ptr to head of offset vector info */
    DDBE_vec_rep_t      *rtn_h;     /* Ptr to head of routine vector info */
    DDBE_vec_rep_t      *info_h;    /* Ptr to head of more type vector info */
    DDBE_vec_rep_t      *param_h;   /* Ptr to first type vec entry for param */
    AST_export_n_t      *export_p;  /* Ptr to AST export node */
    AST_operation_n_t   *oper_p;    /* Ptr to AST operation node */
    AST_parameter_n_t   *param_p;   /* Ptr to AST parameter node */
    unsigned long start_type_vec_index; /* Starting type vector index */
    unsigned long addenda_offset;   /* Offset to type vector addenda */

    /*
     * Save pointer to start of each vector info.
     */
    defn_h   = vip->defn_p;
    type_h   = vip->type_p;
    offset_h = vip->offset_p;
    rtn_h    = vip->rtn_p;
    info_h   = vip->type_info_p;

    /*
     * Walk the IREP tuples for each parameter of each operation, adding
     * appropriate entries to the vector data structures.
     */
    for (export_p = int_p->exports; export_p != NULL; export_p = export_p->next)
    {
      if (export_p->kind == AST_operation_k)
      {
        oper_p = export_p->thing_p.exported_operation;
        /*
         * Create a DDBE info node for the operation.
         */
        DDBE_operation_info(oper_p);

        /*
         * If the optimization is possible, then just process all the parameters
         * sequentially.  Otherwise, process the [in]s then the [out]s.
         */
        if (DDBE_oper_in_out_opt(oper_p))
        {
            /*
             * Walk all the parameters in order.
             */
            vip->processing_ins = FALSE;
            vip->processing_outs = FALSE;
            vip->param_num = 0;

            for (param_p = oper_p->parameters;
                 param_p != NULL;
                 param_p = param_p->next)
            {
                vip->param_num++;

                vip->ir_ctx_p = IR_init_scope(
                    (struct AST_parameter_n_t *)param_p);

                param_h = DDBE_gen_param_reps(int_p, oper_p, param_p, vip,
                                              cmd_opt, cmd_val);

                /*
                 * If the param has type vector entries, bump [in] and/or [out]
                 * param count.  If the first [in] or first [out], save addr of
                 * its first type vec entry.
                 */
                if (param_h != NULL)
                {
                    if (AST_IN_SET(param_p))
                    {
                        oper_p->be_info.dd_oper->num_ins++;
                        if (oper_p->be_info.dd_oper->ins_type_vec_p == NULL)
                            oper_p->be_info.dd_oper->ins_type_vec_p = param_h;
                    }
                    if (AST_OUT_SET(param_p))
                    {
                        oper_p->be_info.dd_oper->num_outs++;
                        if (oper_p->be_info.dd_oper->outs_type_vec_p == NULL)
                            oper_p->be_info.dd_oper->outs_type_vec_p = param_h;
                    }
                }

                IR_finish_scope(vip->ir_ctx_p);
            }
            oper_p->be_info.dd_oper->num_srv_ins =
                oper_p->be_info.dd_oper->num_ins;
        }
        else
        {
            /*
             * Walk all the [in] non-pipe parameters.
             */
            vip->param_num = 0;
            vip->processing_ins = TRUE;
            vip->processing_outs = FALSE;

            for (param_p = oper_p->parameters;
                 param_p != NULL;
                 param_p = param_p->next)
            {
                vip->param_num++;

                if (!AST_IN_SET(param_p) || DDBE_PIPE(param_p))
                    continue;

                vip->ir_ctx_p = IR_init_scope(
                    (struct AST_parameter_n_t *)param_p);

                param_h = DDBE_gen_param_reps(int_p, oper_p, param_p, vip,
                                              cmd_opt, cmd_val);

                /*
                 * If the param has type vector entries, bump [in] param count.
                 * If the first [in], save addr of its first type vec entry.
                 */
                if (param_h != NULL)
                {
                    oper_p->be_info.dd_oper->num_ins++;
                    if (oper_p->be_info.dd_oper->ins_type_vec_p == NULL)
                        oper_p->be_info.dd_oper->ins_type_vec_p = param_h;
                }

                IR_finish_scope(vip->ir_ctx_p);
            }
            oper_p->be_info.dd_oper->num_srv_ins = 
                oper_p->be_info.dd_oper->num_ins;

            /*
             * Process all the [out]-only objects which have an [in] represen-
             * tation on the server side only (conformant arrays and objects
             * containing [ref] pointees that must be preallocated).
             */
            vip->param_num = 0;

            for (param_p = oper_p->parameters;
                 param_p != NULL;
                 param_p = param_p->next)
            {
                vip->param_num++;
                vip->allocate = DDBE_ALLOCATE(param_p);
                vip->allocate_ref = (!vip->allocate
                                     && param_p->ir_info->allocate_ref);
                if (!vip->allocate && !vip->allocate_ref)
                    continue;

                vip->ir_ctx_p = IR_init_scope(
                    (struct AST_parameter_n_t *)param_p);

                param_h = DDBE_gen_param_reps(int_p, oper_p, param_p, vip,
                                              cmd_opt, cmd_val);
                if (oper_p->be_info.dd_oper->ins_type_vec_p == NULL)
                    oper_p->be_info.dd_oper->ins_type_vec_p = param_h;

                oper_p->be_info.dd_oper->num_srv_ins++;
                if (AST_HAS_IN_PIPES_SET(oper_p))
                    oper_p->be_info.dd_oper->num_ins++;
                IR_finish_scope(vip->ir_ctx_p);
                vip->allocate     = FALSE;
                vip->allocate_ref = FALSE;
            }

            /*
             * Walk all the [in] pipe parameters.
             */
            if (AST_HAS_IN_PIPES_SET(oper_p))
            {
                vip->param_num = 0;

                for (param_p = oper_p->parameters;
                     param_p != NULL;
                     param_p = param_p->next)
                {
                    vip->param_num++;

                    if (!AST_IN_SET(param_p) || !DDBE_PIPE(param_p))
                        continue;

                    vip->ir_ctx_p = IR_init_scope(
                        (struct AST_parameter_n_t *)param_p);

                    param_h = DDBE_gen_param_reps(int_p, oper_p, param_p, vip,
                                                  cmd_opt, cmd_val);

                    /*
                     * Bump [in] param count.
                     * If the first [in], save addr of its first type vec entry.
                     */
                    oper_p->be_info.dd_oper->num_ins++;
                    oper_p->be_info.dd_oper->num_srv_ins++;
                    if (oper_p->be_info.dd_oper->ins_type_vec_p == NULL)
                        oper_p->be_info.dd_oper->ins_type_vec_p = param_h;

                    IR_finish_scope(vip->ir_ctx_p);
                }
            }

            /*
             * Walk all the [out] pipe parameters.
             */
            vip->processing_ins = FALSE;
            vip->processing_outs = TRUE;
            if (AST_HAS_OUT_PIPES_SET(oper_p))
            {
                vip->param_num = 0;

                for (param_p = oper_p->parameters;
                     param_p != NULL;
                     param_p = param_p->next)
                {
                    vip->param_num++;

                    if (!AST_OUT_SET(param_p) || !DDBE_PIPE(param_p))
                        continue;

                    vip->ir_ctx_p = IR_init_scope(
                        (struct AST_parameter_n_t *)param_p);

                    param_h = DDBE_gen_param_reps(int_p, oper_p, param_p, vip,
                                                  cmd_opt, cmd_val);

                    /*
                     * Bump [out] param count.
                     * If the first [out], save addr of its 1st type vec entry.
                     */
                    oper_p->be_info.dd_oper->num_outs++;
                    if (oper_p->be_info.dd_oper->outs_type_vec_p == NULL)
                        oper_p->be_info.dd_oper->outs_type_vec_p = param_h;

                    IR_finish_scope(vip->ir_ctx_p);
                }
            }

            /*
             * Walk all the [out] non-pipe parameters.
             */
            vip->param_num = 0;

            for (param_p = oper_p->parameters;
                 param_p != NULL;
                 param_p = param_p->next)
            {
                vip->param_num++;

                if (!AST_OUT_SET(param_p) || DDBE_PIPE(param_p))
                    continue;

                vip->ir_ctx_p = IR_init_scope(
                    (struct AST_parameter_n_t *)param_p);

                param_h = DDBE_gen_param_reps(int_p, oper_p, param_p, vip,
                                              cmd_opt, cmd_val);

                /*
                 * If the param has type vector entries, bump [out] param count.
                 * If the first [out], save addr of its first type vec entry.
                 */
                if (param_h != NULL)
                {
                    oper_p->be_info.dd_oper->num_outs++;
                    if (oper_p->be_info.dd_oper->outs_type_vec_p == NULL)
                        oper_p->be_info.dd_oper->outs_type_vec_p = param_h;
                }

                IR_finish_scope(vip->ir_ctx_p);
            }

            /*
             * Process all the [in]-only objects which have an [out] represen-
             * tation on the server side only ([transmit_as] and [represent_as]
             * objects for which _free_inst or _free_local routine is called).
             */
            vip->param_num = 0;

            for (param_p = oper_p->parameters;
                 param_p != NULL;
                 param_p = param_p->next)
            {
                vip->param_num++;
                vip->free_rep =
                    (AST_IN_SET(param_p) && !AST_OUT_SET(param_p)
                     && DDBE_XMIT_REP(param_p));
                if (!vip->free_rep)
                    continue;

                vip->ir_ctx_p = IR_init_scope(
                    (struct AST_parameter_n_t *)param_p);

                param_h = DDBE_gen_param_reps(int_p, oper_p, param_p, vip,
                                              cmd_opt, cmd_val);
                if (param_h != NULL)
                {
                    oper_p->be_info.dd_oper->num_outs++;
                    if (oper_p->be_info.dd_oper->outs_type_vec_p == NULL)
                        oper_p->be_info.dd_oper->outs_type_vec_p = param_h;
                }

                IR_finish_scope(vip->ir_ctx_p);
                vip->free_rep = FALSE;
            }
        }

        /*
         * Process the function result.
         */
        oper_p->be_info.dd_oper->num_params = vip->param_num + 1;
        vip->param_num = 0;     /* Function result always param number 0 */

        vip->ir_ctx_p = IR_init_scope(
            (struct AST_parameter_n_t *)oper_p->result);

        param_h = DDBE_gen_param_reps(int_p, oper_p, oper_p->result,
                                      vip, cmd_opt, cmd_val);

        /*
         * If the fn result has type vector entries, bump [out] param count.
         * If the first [out], save addr of its first type vec entry.
         */
        if (param_h != NULL)
        {
            oper_p->be_info.dd_oper->num_outs++;
            if (oper_p->be_info.dd_oper->outs_type_vec_p == NULL)
                oper_p->be_info.dd_oper->outs_type_vec_p = param_h;
        }

        /*
         * If the operation has [reflect_deletions], add another [out] param.
         * If the first [out], save addr of its first type vec entry.
         */
        if (AST_REFLECT_DELETIONS_SET(oper_p))
        {
            /*
             * type vector entries:
             *  long: parameter number
             *  byte: deleted nodes tag
             *  byte: end of list
             */
            oper_p->be_info.dd_oper->num_outs++;
            DDBE_long_vec_entry(&vip->type_p, DDBE_vec_long_k, 0,
                                "dummy parameter index");
            if (oper_p->be_info.dd_oper->outs_type_vec_p == NULL)
                oper_p->be_info.dd_oper->outs_type_vec_p = vip->type_p;
            DDBE_tag_vec_entry(&vip->type_p, DDBE_dt_deleted_nodes);
            DDBE_tag_vec_entry(&vip->type_p, DDBE_dt_eol);
        }

        IR_finish_scope(vip->ir_ctx_p);
      } /* export kind = operation */
    }   /* for all exports */

    /*
     * Paste together the type and definition vectors.
     * Reset each vector pointer back to head of list.
     */
    vip->type_p->next = defn_h;
    vip->defn_p   = NULL;
    vip->type_p   = type_h;
    vip->offset_p = offset_h;
    vip->rtn_p    = rtn_h;
    vip->type_info_p = info_h;

    /*
     * Compute starting type vector index - constant size of preamble plus
     * a number of longs for each operation in the interface.
     */
    start_type_vec_index = DDBE_PARAM_START +
                           DDBE_OPER_ENTRIES * 4 * int_p->op_count;

    /*
     * Resolve offsets in all the vectors.  For the offset and routine vectors,
     * add one to resulting vector size for terminating sentinel entry.
     */
    vip->type_vec_size =
        DDBE_compute_vec_offsets(vip->type_p, 4, 4, /* 4 = sizeof(idl_long) */
                                 start_type_vec_index, &type_l);
    vip->offset_vec_size =
        DDBE_compute_vec_offsets(vip->offset_p, sizeof(int), 1, 1, NULL) + 1;
    vip->rtn_vec_size =
        DDBE_compute_vec_offsets(vip->rtn_p, sizeof(void(*)()), 1, 1, NULL) + 1;

    /*
     * Do required alignment for type/definition vector addenda.
     */
    addenda_offset = vip->type_vec_size;
    addenda_offset = (addenda_offset + DDBE_ADDENDA_ALIGN-1)
                     & ~(DDBE_ADDENDA_ALIGN-1);
    if (addenda_offset != vip->type_vec_size)
    {
        DDBE_pad_vec_entry(&type_l, (byte)addenda_offset-vip->type_vec_size);
        /* type_l now points at the new pad entry */
        type_l->index = vip->type_vec_size;
        vip->type_vec_size = addenda_offset;
    }

    /*
     * Patch indirect references in the type/definition vector.
     */
    DDBE_patch_vec_refs(vip->type_p);
    DDBE_patch_oper_info(int_p);
}


/*
 *  D D B E _ m a i n
 *
 *  Main routine of Data Driven Backend.  Produces data structures for the
 *  type, offset, and routine vectors needed by a Marshalling Interpreter.
 *  The public speller support routines take these data structures as input
 *  and spell the corresponding vector data.
 */
boolean DDBE_main
(
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val,  /* [in] array of cmd option values */
    AST_interface_n_t   *int_p,     /* [in] ptr to interface node */
    DDBE_vectors_t      **p_vip     /*[out] vector information pointer */
)
{
    DDBE_vectors_t      *vip;       /* local copy of vector info pointer */
    long                endian = 1; /* integer to determine endianess */

    if (*(unsigned char *)&endian != 1) DDBE_little_endian = FALSE;

    /*
     * Initialize the vector information.
     */
    DDBE_init_vectors(int_p, p_vip);
    vip = *p_vip;

    /*
     * Initialize the symbolic names for interpreter tags.
     */
    DDBE_init_tags();

    /*
     * Generate the vector data structures.
     */
    DDBE_gen_vector_reps(int_p, vip, cmd_opt, cmd_val);

    /*
     * If requested, dump the vector data structures.  Currently this is done
     * simply by calling the speller routines, directing output to stdout.
     */
#ifdef DUMPERS
    DDBE_stub_hex = (getenv("IDL_STUB_HEX") != NULL);
    if (cmd_opt[opt_dump_recs])
    {
        printf("\nDump of DDBE instance definitions:\n");
        printf(  "----------------------------------\n");
        DDBE_spell_offset_instances(stdout, vip, cmd_opt, cmd_val);

        printf("\nDump of DDBE statically initialized offset vector:\n");
        printf(  "--------------------------------------------------\n");
        DDBE_spell_offset_vec_use_inst(stdout, vip, cmd_opt, cmd_val);

        printf("\nDump of DDBE uninitialized offset vector and init rtn:\n");
        printf(  "------------------------------------------------------\n");
        DDBE_init_offset_vec(stdout, vip, cmd_opt, cmd_val);

        printf("\nDump of DDBE routine vector data:\n");
        printf(  "---------------------------------\n");
        DDBE_spell_rtn_vec(stdout, vip, cmd_opt, cmd_val, TRUE);

        printf("\nDump of DDBE type/definition vector data:\n");
        printf(  "-----------------------------------------\n");
        DDBE_spell_type_vec(stdout, vip, cmd_opt, cmd_val);
    }

    /*
     * If requested, test the routines that spell marshalling code.
     */
    if (cmd_opt[opt_dump_mnode])
        DDBE_test_marsh_spellers(int_p, cmd_opt, cmd_val);
#endif

    return TRUE;
}

#ifdef DUMPERS
/*
 *  D D B E _ t e s t _ m a r s h _ s p e l l e r s
 *
 *  Test all the routines that spell marshalling code.
 */
static void DDBE_test_marsh_spellers
(
    AST_interface_n_t   *int_p,     /* [in] ptr to interface node */
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val   /* [in] array of cmd option values */
)
{
    AST_export_n_t      *export_p;  /* Ptr to AST export node */
    AST_operation_n_t   *oper_p;    /* Ptr to AST operation node */
    char const               *oper_name; /* Operation name */

    printf("\nDump of generated marshalling code:\n");
    printf(  "-----------------------------------\n");

    /*
     * Process each operation in the interface.
     */
    for (export_p = int_p->exports; export_p != NULL; export_p = export_p->next)
    {
        if (export_p->kind == AST_operation_k)
        {
            oper_p = export_p->thing_p.exported_operation;

            NAMETABLE_id_to_string(oper_p->name, &oper_name);
            printf("\nOperation %s:\n\n", oper_name);

            DDBE_spell_param_vec_def(stdout, oper_p, BE_client_side,
                cmd_opt, cmd_val);
            printf("\n");
            DDBE_spell_param_vec_init(stdout, oper_p, BE_client_side,
                cmd_opt, cmd_val);
            printf("\n");
            DDBE_spell_param_vec_init(stdout, oper_p, BE_server_side,
                cmd_opt, cmd_val);
            printf("\n");
            DDBE_spell_marsh_or_unmar(stdout, oper_p, "test_marsh_interp",
                "state_ptr", BE_client_side, BE_marshalling_k);
            printf("\n");
            DDBE_spell_marsh_or_unmar(stdout, oper_p, "test_unmar_interp",
                "state_ptr", BE_client_side, BE_unmarshalling_k);
        }
    }
}
#endif  /* DUMPERS */
