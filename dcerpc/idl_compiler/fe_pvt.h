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
**      fe_pvt.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Header file containing defining front-end private data structures
**  for data that is kept in the fe_info_t field of the AST nodes.
**
**  VERSION: DCE 1.0
**
*/

#ifndef fe_pvth_incl
#define fe_pvth_incl

#include <nametbl.h>

/*
 * The frontend structure describes information private to
 * the frontend portion of the compiler.
 */

typedef enum
{
    fe_bitset_info,
    fe_const_info,
    fe_source_only,
    fe_tag_fwd_ref,
    fe_clone_info,
    fe_if_info,
    fe_ptr_info
} fe_type_k_t;

typedef enum            /* Integer constant kinds */
{
    fe_int_const_k,
    fe_bitset_const_k,
    fe_enum_const_k
}   fe_const_k_t;

typedef enum            /* AST node kinds */
{
    fe_interface_n_k,
    fe_type_p_n_k,
    fe_import_n_k,
    fe_export_n_k,
    fe_cpp_quote_n_k,
    fe_constant_n_k,
    fe_type_n_k,
    fe_rep_as_n_k,
    fe_array_n_k,
    fe_array_index_n_k,
    fe_bitset_n_k,
    fe_disc_union_n_k,
    fe_arm_n_k,
    fe_case_label_n_k,
    fe_enumeration_n_k,
    fe_pipe_n_k,
    fe_pointer_n_k,
    fe_structure_n_k,
    fe_field_n_k,
    fe_field_attr_n_k,
    fe_field_ref_n_k,
    fe_parameter_n_k,
    fe_operation_n_k,
    fe_include_n_k,
    fe_exception_n_k,
    fe_cs_char_n_k,
    fe_expression_n_k
} fe_node_k_t;

/*
 * Bit names contained in fe_info flags word
 */

#define FE_SELF_POINTING        0x00000001      /* True if this type node   */
                                                /* is on the sp_types list. */
                                                /* Needed because non self- */
                                                /* pointing types can be on */
                                                /* the list also            */

#define FE_POINTED_AT           0x00000002      /* True if this type node   */
                                                /* is on the pa_types list  */

#define FE_HAS_PTR_ARRAY        0x00000004      /* True if type contains a  */
                                                /* [ptr] array used as other*/
                                                /* than a top-level param   */

#define FE_INCOMPLETE           0x00000008      /* True if this             */
                                                /* struct/union is not yet  */
                                                /* complete due to a        */
                                                /* forward reference        */

#define FE_HAS_UNIQUE_PTR       0x00000010      /* True if this type or any */
                                                /* types it pts to have any */
                                                /* [unique] ptrs            */

#define FE_HAS_PTR              0x00000020      /* True if this type or any */
                                                /* contained types are ptrs */

#define FE_HAS_CFMT_ARR         0x00000040      /* True if this param       */
                                                /* contains a non-top-level */
                                                /* conformant array that is */
                                                /* not under a full pointer */

#define FE_PROP_TYPE_DONE       0x00000080      /* True if this item has    */
                                                /* had type propagation     */
                                                /* completed                */

#define FE_HAS_REF_PTR          0x00000100      /* True if this type or any */
                                                /* types it pts to have any */
                                                /* [ref] ptrs               */

#define FE_PROP_IN_PARAM_DONE   0x00000200      /* True if this item has    */
                                                /* had [in] param           */
                                                /* propagation completed    */

#define FE_HAS_REP_AS           0x00000400      /* True if this item has    */
                                                /* or contains a type with  */
                                                /* [represent_as] on it     */

#define FE_OOL                  0x00000800      /* True if this item has    */
                                                /* been put on the ool list */

#define FE_HAS_VAL_FLOAT        0x00001000      /* True if operation has    */
                                                /* float passed by value    */

#define FE_HAS_V1_ATTR          0x00002000      /* True if type has or cts. */
                                                /* a V1-specific attribute  */

#define FE_HAS_V2_ATTR          0x00004000      /* True if type has or cts. */
                                                /* a V2-specific attribute  */

#define FE_PROP_OUT_PARAM_DONE  0x00008000      /* True if this item has    */
                                                /* had [out] param          */
                                                /* propagation completed    */

#define FE_HAS_FULL_PTR         0x00010000      /* True if this type or any */
                                                /* types it pts to have any */
                                                /* [ref] ptrs               */

#define FE_HAS_XMIT_AS          0x00020000      /* True if this item has    */
                                                /* or contains a type with  */
                                                /* [transmit_as] on it      */

#define FE_HAS_CHAR             0x00040000      /* True if this item is or  */
                                                /* contains a char type     */

#define FE_HAS_FLOAT            0x00080000      /* True if this item is or  */
                                                /* contains a floating type */

#define FE_HAS_INT              0x00100000      /* True if this item is or  */
                                                /* contains an integer type */

#define FE_MAYBE_WIRE_ALIGNED   0x00200000      /* True if this item's memory */
                                                /* alignment might match NDR  */

#define FE_HAS_IN_FULL_PTR      0x00400000      /* True if this operation has */
                                                /* any [ptr] pointers in [in] */
                                                /* or [in,out] parameters     */

#define FE_USED_AS_CS_FLD_ATTR  0x00800000      /* True if this instance is   */
                                                /* used as the target of a    */
                                                /* [size_is] or [length_is]   */
                                                /* for array of [cs_char] type*/

#define FE_HAS_NF_CS_ARRAY      0x01000000      /* On a type, True if the type*/
                                                /* contains a non-fixed array */
                                                /* of [cs_char] type          */
#define FE_FIRST_IN_NF_CS_ARR   0x01000000      /* On a param, True if it is  */
                                                /* the first [in] param in an */
                                                /* operation containing non-  */
                                                /* fixed array of [cs_char]   */

#define FE_CT_CS_CHAR           0x02000000      /* On a type, True if the type*/
                                                /* contains, not merely is, a */
                                                /* [cs_char] type             */
#define FE_LAST_IN_NF_CS_ARR    0x02000000      /* On a param, True if it is  */
                                                /* the last [in] param in an  */
                                                /* operation containing non-  */
                                                /* fixed array of [cs_char]   */

#define FE_USED_IN_TRANSMITTED  0x04000000      /* On a type, True if it is   */
                                                /* used within a transmitted  */
                                                /* type specified in a        */
                                                /* [transmit_as] attribute    */
#define FE_FIRST_OUT_NF_CS_ARR  0x04000000      /* On a param, True if it is  */
                                                /* the first [out] param in an*/
                                                /* operation containing non-  */
                                                /* fixed array of [cs_char]   */

#define FE_LAST_OUT_NF_CS_ARR   0x08000000      /* On a param, True if it is  */
                                                /* the last [out] param in an */
                                                /* operation containing non-  */
                                                /* fixed array of [cs_char]   */

#define FE_USED_AS_REG_FLD_ATTR 0x10000000      /* True if this instance is   */
                                                /* used as the target of a    */
                                                /* field attribute for array  */
                                                /* of non-[cs_char] type      */

/*
 * Macros to set, clear, and test fe_info flags word
 */
#define FE_SET(word,bit)    ((word) |= (bit))
#define FE_TEST(word,bit)   (((word) & (bit)) != 0)
#define FE_CLEAR(word,bit)  ((word) &= ~(bit))


/*
 * Info in the AST used only by the frontend.
 */
typedef struct fe_info_t
{
    struct fe_info_t *next;
    struct fe_info_t *last;
    fe_node_k_t      node_kind;         /* AST node kind */
    STRTAB_str_t     file;
    int              source_line;
    STRTAB_str_t     acf_file;          /* ACF file for this node or NIL */
    int              acf_source_line;   /* ACF line number or 0 if none */
    fe_type_k_t      fe_type_id;
    union
    {
        int             cardinal;       /* For bitsets and enumerations */
        fe_const_k_t    const_kind;     /* fe_type_id == fe_const_info */
        struct AST_type_p_n_t *clone;   /* fe_type_id == fe_clone_info */
        int             if_flags;       /* fe_type_id == fe_if_info */
        struct AST_type_n_t *pointer_type; /* fe_type_id == fe_ptr_info */
    } type_specific;

    struct AST_type_n_t *tag_ptr;       /* Type node for the tag from which */
                                        /* this type was derived.           */
    NAMETABLE_id_t      tag_name;       /* Tag name from which this type is */
                                        /* derived.                         */
    unsigned long int   gen_index;      /* Index used in gen'd names        */
    short int           pointer_count;  /* The number of pointers on a      */
                                        /* array bound reference.           */
    unsigned short int  ref_count;      /* Reference count. On a parameter, */
                                        /* used to count occurences of 'used*/
                                        /* as field attribute reference for */
                                        /* an array of [cs_char]            */
    unsigned short int  ref_count_a;    /* Reference count. On a parameter, */
                                        /* used to count occurences of 'used*/
                                        /* as [in] [size_is] reference for  */
                                        /* an [out]-only conformant array   */
                                        /* of [cs_char]'                    */
    unsigned long int  flags;           /* A bitvector of flags */

    struct AST_type_n_t *original;      /* Type node for a type with        */
                                        /* DEF_AS_TAG set                   */
} fe_info_t;

#endif
