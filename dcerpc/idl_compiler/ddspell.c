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
**      ddspell.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Data Driven Backend - Support routines to spell C source vector definitions
**  and code to interface with a Marshalling Interpreter.
**
*/

#include <ddbe.h>
#include <astp.h>
#include <command.h>
#include <message.h>
#include <mtsbacke.h>
#include <nidlmsg.h>
#include <cspell.h>

/*
 * Conditional macro to spell a comment containing the current vector index.
 */
#ifdef DUMPERS
#define DDBE_SPELL_INDEX(_fid, _index) \
    fprintf(_fid, "/* %s */ ", DDBE_spell_long(_index))
#else
#define DDBE_SPELL_INDEX(_fid, _index) 
#endif

/*
 * Conditional macro to spell a comment if the comment stringtable ID non-NULL.
 */
#ifdef DUMPERS
#define DDBE_SPELL_COMMENT(_fid, _comment_id, _comment_fmt, _comment_buf) \
    if (_comment_id != STRTAB_NULL_STR) \
    { \
        STRTAB_str_to_string(_comment_id, &_comment_buf); \
        fprintf(_fid, _comment_fmt, _comment_buf); \
    }
#else
#define DDBE_SPELL_COMMENT(_fid, _comment_id, _comment_fmt, _comment_buf) 
#endif

/*
 * Conditional macros to spell a string with zero, one, or two printf arguments.
 */
#ifdef DUMPERS
#define DDBE_SPELL_TEXT(_fid, _string) \
    fprintf(_fid, _string)

#define DDBE_SPELL_TEXT_1ARG(_fid, _fmt, _arg) \
    fprintf(_fid, _fmt, _arg)

#define DDBE_SPELL_TEXT_2ARG(_fid, _fmt, _arg1, _arg2) \
    fprintf(_fid, _fmt, _arg1, _arg2)
#else
#define DDBE_SPELL_TEXT(_fid, _string) 
#define DDBE_SPELL_TEXT_1ARG(_fid, _fmt, _arg) 
#define DDBE_SPELL_TEXT_2ARG(_fid, _fmt, _arg1, _arg2) 
#endif

/**************************************/
/*  Private speller support routines  */
/**************************************/

/*
 *  D D B E _ s p e l l _ l o n g
 *  D D B E _ s p e l l _ l o n g _ n f
 *
 *  Returns a string that represents a long integer in either hex or decimal.
 *  DDBE_spell_long spells the long with filler chars for right justification.
 *  DDBE_spell_long_nf spells the long with no filler chars.
 *
 *  Implicit Input: DDBE_stub_hex - TRUE => return hex format, FALSE => decimal
 *  Restriction:
 *      For simplicity of implementation, the routine uses a global buffer.
 *      Thus routine is not reentrant and call will overwrite previous buffer
 *      contents.  Do not code two calls to this routine in one printf call.
 */
static char DDBE_long_buf[DDBE_MAX_EXPR];

static char *DDBE_spell_long
(
    long            val             /* [in] long value */
)
{
    if (DDBE_stub_hex)
        sprintf(DDBE_long_buf, "0x%04lx", val);
    else
        sprintf(DDBE_long_buf, "%6ld", val);

    return DDBE_long_buf;
}

static char *DDBE_spell_long_nf
(
    long            val             /* [in] long value */
)
{
    if (DDBE_stub_hex)
        sprintf(DDBE_long_buf, "0x%lx", val);
    else
        sprintf(DDBE_long_buf, "%ld", val);

    return DDBE_long_buf;
}


/*
 *  D D B E _ s p e l l _ l o n g _ v a l
 *
 *  Spells a 'long' value as a series of bytes so that the resulting data
 *  encoding is not dependent on the platform's integer endianism.
 */
static void DDBE_spell_long_val
(
    FILE            *fid,           /* [in] output file handle */
    DDBE_vec_rep_t  *vec_p,         /* [in] ptr to vector entry list */
    boolean         little_endian ATTRIBUTE_UNUSED   /* [in] T/F => spell as little/big endian */
)
{
    byte        *bp;
    int         i;
#ifdef DUMPERS
    char const       *comment;
#endif

    bp = (byte *)&vec_p->val.long_val;

    for (i = 0; i < 4 /*sizeof(idl_long_int)*/; i++)
	fprintf(fid, "0x%02x,", bp[i]);

#ifdef DUMPERS
    STRTAB_str_to_string(vec_p->comment, &comment);
    fprintf(fid, "\t/* long %s %s */",
            DDBE_spell_long_nf(vec_p->val.long_val), comment);
#endif
    fprintf(fid, "\n");
}


/*
 *  D D B E _ s p e l l _ s h o r t _ b y t e s
 *
 *  Spells a 'short' value as a series of bytes so that the resulting data
 *  encoding is not dependent on the platform's integer endianism.
 */
static void DDBE_spell_short_bytes
(
    FILE            *fid,           /* [in] output file handle */
    unsigned short  *val,           /* [in] value to print */
    boolean         little_endian ATTRIBUTE_UNUSED  /* [in] T/F => spell as little/big endian */
)
{
    byte        *bp;
    int         i;

    bp = (byte *)val;

    for (i = 0; i < 2 /*sizeof(idl_short_int)*/; i++)
	fprintf(fid, "0x%02x,", bp[i]);
}


/*
 *  D D B E _ s p e l l _ l o n g _ b y t e s
 *
 *  Spells a 'long' value as a series of bytes so that the resulting data
 *  encoding is not dependent on the platform's integer endianism.
 */
static void DDBE_spell_long_bytes
(
    FILE            *fid,           /* [in] output file handle */
    unsigned long   *val,           /* [in] value to print */
    boolean         little_endian ATTRIBUTE_UNUSED  /* [in] T/F => spell as little/big endian */
)
{
    byte        *bp;
    int         i;

    bp = (byte *)val;

    for (i = 0; i < 4 /*sizeof(idl_long_int)*/; i++)
	fprintf(fid, "0x%02x,", bp[i]);
}


/*
 *  D D B E _ s p e l l _ l o n g _ b o o l _ v a l
 *
 *  Spells a 'long' representation of a boolean value as a series of bytes so
 *  that the resulting data encoding is not dependent on the platform's integer
 *  endianism.
 */
static void DDBE_spell_long_bool_val
(
    FILE            *fid,           /* [in] output file handle */
    DDBE_vec_rep_t  *vec_p,         /* [in] ptr to vector entry list */
    boolean         little_endian   /* [in] T/F => spell as little/big endian */
)
{
    char        *sym;
#ifdef DUMPERS
    char const       *comment;
#endif

    if (vec_p->val.long_val == 0)
        sym = "idl_false";
    else
        sym = "idl_true";

    /*
     * Regardless of whether we're running on little/big endian or spelling
     * for little/big endian, the first byte spelt is the low-order byte.
     */
    if (little_endian)
	fprintf(fid, "%s,0x00,0x00,0x00,", sym);
    else
	fprintf(fid, "0x00,0x00,0x00,%s,", sym);
#ifdef DUMPERS
    STRTAB_str_to_string(vec_p->comment, &comment);
    fprintf(fid, "\t/* long %s %s */",
            DDBE_spell_long_nf(vec_p->val.long_val), comment);
#endif
    fprintf(fid, "\n");
}


/*
 *  D D B E _ l a s t _ f i e l d
 *
 *  Returns pointer to last field node in structure type.  If last field is
 *  a nested struct returns the last field in the innermost struct.  Also
 *  returns an field expression, e.g. "last_toplevel_field.last_nested_field".
 *
 *  Assumption: Input type is a structure.
 */
static void DDBE_last_field
(
    AST_type_n_t        *type_p,    /* [in] ptr to AST type node */
    AST_field_n_t       **p_field_p,/*[out] ptr to AST field node */
    STRTAB_str_t        *field_expr /*[out] field expression */
)
{
    AST_structure_n_t   *struct_p;
    AST_field_n_t       *field_p;
    char const          *field_name;
    char                expr[DDBE_MAX_EXPR];
    boolean             nested;

    expr[0] = '\0';
    nested = FALSE;
    struct_p = type_p->type_structure.structure;

    field_p = struct_p->fields;
    while (field_p != NULL)
    {
        if (field_p->next == NULL)
        {
            *p_field_p = field_p;
            NAMETABLE_id_to_string(field_p->name, &field_name);
            if (nested)
                strcat(expr, ".");
            strcat(expr, field_name);

            if (field_p->type->kind == AST_structure_k)
            {
                nested = TRUE;
                field_p = field_p->type->type_structure.structure->fields;
                continue;
            }
        }
        field_p = field_p->next;
    }

    *field_expr = STRTAB_add_string(expr);
}


/*
 *  D D B E _ s i z e o f _ e x p r
 *
 *  Spells an expression for the size of a data type.
 */
static void DDBE_sizeof_expr
(
    FILE                *fid,       /* [in] output file handle */
    AST_type_n_t        *type_p,    /* [in] ptr to AST type node */
    STRTAB_str_t        comment_id ATTRIBUTE_UNUSED /* [in] ID of comment string */
)
{
#ifdef DUMPERS
    char const          *comment;   /* Comment string */
#endif

    if (AST_CONFORMANT_SET(type_p))
    {
        AST_field_n_t   *field_p;   /* Ptr to conformant array field node */
        STRTAB_str_t    field_expr;
        char const      *field_text;

        if (type_p->kind != AST_structure_k)
        {
            INTERNAL_ERROR("sizeof conformant array not supported");
            return;
        }
        DDBE_last_field(type_p, &field_p, &field_expr);
        STRTAB_str_to_string(field_expr, &field_text);
#if defined(ultrix)
        fprintf(fid, "IDL_offsetofarr(");
#else
        fprintf(fid, "offsetof(");
#endif
        CSPELL_typed_name(fid, type_p, NAMETABLE_NIL_ID /*instance name*/,
            (AST_type_n_t *)NULL /*in_typedef*/,
            TRUE /*in_struct*/, TRUE /*spell_tag*/, FALSE /*encoding_services*/
            );
        fprintf(fid, ", %s)", field_text);
    }
    else
    {
        fprintf(fid, "sizeof");
        CSPELL_cast_exp(fid, type_p);
    }

    DDBE_SPELL_COMMENT(fid, comment_id, "\t/* %s */", comment);
}


/*
 *  D D B E _ s i z e o f _ e x p r _ u s e _ i n s t
 *
 *  Spells an expression for the size of a data type.  Requires the instance
 *  declarations that are spelt by DDBE_spell_offset_instances.
 */
static void DDBE_sizeof_expr_use_inst
(
    FILE                *fid,       /* [in] output file handle */
    AST_type_n_t        *type_p,    /* [in] ptr to AST type node */
    STRTAB_str_t        comment_id ATTRIBUTE_UNUSED /* [in] ID of comment string */
)
{
    char const          *inst_name; /* Name of generated instance of type */
#ifdef DUMPERS
    char const                *comment;   /* Comment string */
#endif

    NAMETABLE_id_to_string(type_p->be_info.dd_type->inst_name, &inst_name);

    if (AST_CONFORMANT_SET(type_p))
    {
        AST_field_n_t   *field_p;   /* Ptr to conformant array field node */
        STRTAB_str_t    field_expr;
        char const      *field_text;

        if (type_p->kind != AST_structure_k)
        {
            INTERNAL_ERROR("sizeof conformant array not supported");
            return;
        }
        DDBE_last_field(type_p, &field_p, &field_expr);
        STRTAB_str_to_string(field_expr, &field_text);
        fprintf(fid, "(idl_byte *)%s.%s - (idl_byte *)&%s",
                inst_name, field_text, inst_name);
    }
    else
        fprintf(fid, "sizeof(%s)", inst_name);

    DDBE_SPELL_COMMENT(fid, comment_id, "\t/* %s */", comment);
}

/*****************************/
/*  Public utility routines  */
/*****************************/

/*
 *  D D B E _ c f m t _ a r r _ l o c a l _ r e p
 *
 *  Returns TRUE if a parameter's local representation is any form of
 *  a conformant array.
 */
boolean DDBE_cfmt_arr_local_rep
(
    AST_parameter_n_t   *param_p    /* [in] Ptr to AST parameter node */
)
{
    AST_type_n_t        *type_p;

    type_p = param_p->type;

    /* A conformant array without represent_as */
    if (AST_CONFORMANT_SET(type_p)
        && type_p->rep_as_type == NULL)
        return TRUE;

    /* A [ref] pointer to a conformant object without represent_as */
    if (AST_REF_SET(param_p) && type_p->kind == AST_pointer_k
        && AST_CONFORMANT_SET(type_p->type_structure.pointer->pointee_type)
        && type_p->type_structure.pointer->pointee_type->rep_as_type == NULL)
        return TRUE;

    /* An arrayified pointer whose array representation is conformant */
    if (DDBE_ARRAYIFIED(param_p)
        && AST_CONFORMANT_SET(
                type_p->type_structure.pointer->pointee_type->array_rep_type))
        return TRUE;

    return FALSE;
}

/*************************************/
/*  Public speller support routines  */
/*************************************/

/*
 *  D D B E _ s p e l l _ o f f s e t _ i n s t a n c e s
 *
 *  Spells an instance declaration of each type that is represented in the
 *  offset vector.  This is necessary so that the data is portable.
 */
void DDBE_spell_offset_instances
(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vectors_t      *vip,       /* [in] vector information pointer */
    boolean             *cmd_opt ATTRIBUTE_UNUSED,   /* [in] array of cmd option flags */
    void                **cmd_val ATTRIBUTE_UNUSED  /* [in] array of cmd option values */
)
{
    DDBE_vec_rep_t      *vec_p;     /* Ptr to offset vector entry list */
    AST_type_n_t        *type_p;    /* Ptr to AST structure type node */
    AST_rep_as_n_t      *rep_p;     /* Ptr to AST represent_as node */
    AST_cs_char_n_t     *ichar_p;   /* Ptr to AST cs_char node */
    boolean             in_struct;  /* T => in struct (for speller) */

    for (vec_p = vip->offset_p; vec_p != NULL; vec_p = vec_p->next)
    {
        if (vec_p->kind != DDBE_vec_sizeof_k)
            continue;

        type_p = vec_p->val.type_p;

        if (type_p->kind == AST_disc_union_k && type_p->
            type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID)
            in_struct = FALSE;
        else
            in_struct = TRUE;
        fprintf(fid, "static ");
        /*
         * Null out and later restore represent_as node address, if any,
         * to prevent spelling of local type - we want network type.
         * Same for cs_char.
         */
        rep_p = type_p->rep_as_type;
        type_p->rep_as_type = NULL;
        ichar_p = type_p->cs_char_type;
        type_p->cs_char_type = NULL;
        CSPELL_typed_name(fid, type_p, type_p->be_info.dd_type->inst_name,
            (AST_type_n_t *)NULL, in_struct, TRUE /*spell_tag*/,
            FALSE /*encoding_services*/);
        type_p->rep_as_type = rep_p;
        type_p->cs_char_type = ichar_p;
        fprintf(fid, ";\n");
    }

    fflush(fid);
}


/*
 *  D D B E _ s p e l l _ o f f s e t _ v e c
 *
 *  Spells the offset vector definition and initialization.  Does not require
 *  the instance declarations that are spelt by DDBE_spell_offset_instances,
 *  and thus is the most straightforward way of spelling the offset vector.
 */
void DDBE_spell_offset_vec
(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vectors_t      *vip,       /* [in] vector information pointer */
    boolean             *cmd_opt ATTRIBUTE_UNUSED,   /* [in] array of cmd option flags */
    void                **cmd_val ATTRIBUTE_UNUSED  /* [in] array of cmd option values */
)
{
    DDBE_vec_rep_t      *vec_p;     /* Ptr to offset vector entry list */
    AST_type_n_t        *type_p = NULL;    /* Ptr to AST type node */
    AST_rep_as_n_t      *rep_p = NULL;     /* Ptr to AST represent_as node */
    AST_cs_char_n_t     *ichar_p = NULL;   /* Ptr to AST cs_char node */
#ifdef DUMPERS
    char const                *comment;   /* Comment text */
#endif
    boolean             in_struct = false;  /* T => in struct (for speller) */

    vec_p = vip->offset_p;

    fprintf(fid, "static idl_ulong_int %soffset_vec[] = {\n", DDBE_PREFIX_IDL);
    fprintf(fid, "0,");
    DDBE_SPELL_TEXT(fid, "\t/* sentinel */");
    fprintf(fid, "\n");

    while (vec_p != NULL)
    {
        switch (vec_p->kind)
        {
        case DDBE_vec_comment_k:
            DDBE_SPELL_COMMENT(fid, vec_p->comment, "\t\t/* %s */\n", comment);
            break;

        case DDBE_vec_expr_long_k:
        {
            /* Simple expression for long value */
            char const *long_expr;

            STRTAB_str_to_string(vec_p->val.expr, &long_expr);
            DDBE_SPELL_INDEX(fid, vec_p->index);
            fprintf(fid, "(idl_ulong_int)%s,", long_expr);

            DDBE_SPELL_COMMENT(fid, vec_p->comment, "\t/* %s */", comment);
            fprintf(fid, "\n");
            break;
        }

        case DDBE_vec_sizeof_k:
            type_p = vec_p->val.type_p;
            /*
             * Null out and later restore represent_as node address, if any,
             * to prevent spelling of local type - we want network type.
             * Same for cs_char.
             */
            rep_p = type_p->rep_as_type;
            type_p->rep_as_type = NULL;
            ichar_p = type_p->cs_char_type;
            type_p->cs_char_type = NULL;
            DDBE_SPELL_INDEX(fid, vec_p->index);
            DDBE_sizeof_expr(fid, type_p, vec_p->comment);
            fprintf(fid, ",\n");
            type_p->rep_as_type = rep_p;
            type_p->cs_char_type = ichar_p;
            break;

        case DDBE_vec_noop_k:
            break;

        case DDBE_vec_offset_begin_k:
            type_p = vec_p->val.type_p;
            if (type_p->kind == AST_disc_union_k && type_p->
                type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID)
                in_struct = FALSE;
            else
                in_struct = TRUE;
            /*
             * Null out and later restore represent_as node address, if any,
             * to prevent spelling of local type - we want network type.
             * Same for cs_char.
             */
            rep_p = type_p->rep_as_type;
            type_p->rep_as_type = NULL;
            ichar_p = type_p->cs_char_type;
            type_p->cs_char_type = NULL;
            break;

        case DDBE_vec_offset_end_k:
            /* Restore node addresses that were saved above */
            type_p->rep_as_type = rep_p;
            type_p->cs_char_type = ichar_p;
            break;

        case DDBE_vec_expr_k:
        case DDBE_vec_expr_arr_k:
        {
            /*
             * field offset: &((type *)NULL)->field-expr - NULL
             */
            char const *field_expr;

            STRTAB_str_to_string(vec_p->val.expr, &field_expr);
            DDBE_SPELL_INDEX(fid, vec_p->index);

#if defined(ultrix)
          if (vec_p->kind == DDBE_vec_expr_arr_k)
            fprintf(fid, "IDL_offsetofarr(");
          else
#endif
            fprintf(fid, "offsetof(");
            CSPELL_typed_name(fid, type_p, NAMETABLE_NIL_ID /*instance name*/,
                (AST_type_n_t *)NULL /*in_typedef*/,
                in_struct, TRUE /*spell_tag*/, FALSE /*encoding_services*/);
            fprintf(fid, ", %s),", field_expr);

            DDBE_SPELL_COMMENT(fid, vec_p->comment, "\t/* %s */", comment);
            fprintf(fid, "\n");
            break;
        }

        default:
            INTERNAL_ERROR("Invalid offset vector entry kind");
        }

        vec_p = vec_p->next;
    }

    fprintf(fid, "0");
    DDBE_SPELL_TEXT(fid, "\t/* sentinel */");
    fprintf(fid, "\n};\n\n");
}


/*
 *  D D B E _ s p e l l _ o f f s e t _ v e c _ u s e _ i n s t
 *
 *  Spells the offset vector definition.  Requires the instance declarations
 *  that are spelt by DDBE_spell_offset_instances.
 *
 *  NOTE:
 *  The resulting declaration will not compile successfully under some C
 *  compilers.  On those platforms it will be necessary to call DDBE_init_
 *  offset_vec to spell a routine which must be called once per interface to
 *  initialize the offset vector.
 */
void DDBE_spell_offset_vec_use_inst
(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vectors_t      *vip,       /* [in] vector information pointer */
    boolean             *cmd_opt ATTRIBUTE_UNUSED,   /* [in] array of cmd option flags */
    void                **cmd_val ATTRIBUTE_UNUSED  /* [in] array of cmd option values */
)
{
    DDBE_vec_rep_t      *vec_p;     /* Ptr to offset vector entry list */
    AST_type_n_t        *type_p;    /* Ptr to AST type node */
    char const          *inst_name; /* Name of generated instance of type */
#ifdef DUMPERS
    char const               *comment;   /* Comment text */
#endif

    vec_p = vip->offset_p;
    inst_name = NULL;

    fprintf(fid, "static idl_ulong_int %soffset_vec[] = {\n", DDBE_PREFIX_IDL);
    fprintf(fid, "0,");
    DDBE_SPELL_TEXT(fid, "\t/* sentinel */");
    fprintf(fid, "\n");

    while (vec_p != NULL)
    {
        type_p = vec_p->val.type_p;

        switch (vec_p->kind)
        {
        case DDBE_vec_comment_k:
            DDBE_SPELL_COMMENT(fid, vec_p->comment, "\t\t/* %s */\n", comment);
            break;

        case DDBE_vec_expr_long_k:
        {
            /* Simple expression for long value */
            char const *long_expr;

            STRTAB_str_to_string(vec_p->val.expr, &long_expr);
            DDBE_SPELL_INDEX(fid, vec_p->index);
            fprintf(fid, "%s,", long_expr);

            DDBE_SPELL_COMMENT(fid, vec_p->comment, "\t/* %s */", comment);
            fprintf(fid, "\n");
            break;
        }

        case DDBE_vec_sizeof_k:
            DDBE_SPELL_INDEX(fid, vec_p->index);
            DDBE_sizeof_expr_use_inst(fid, type_p, vec_p->comment);
            fprintf(fid, ",\n");
            break;

        case DDBE_vec_noop_k:
            break;

        case DDBE_vec_offset_end_k:
            inst_name = NULL;
            break;

        case DDBE_vec_offset_begin_k:
            /* Form instance name for struct */
            NAMETABLE_id_to_string(type_p->be_info.dd_type->inst_name,
                &inst_name);
            break;

        case DDBE_vec_expr_k:
        case DDBE_vec_expr_arr_k:
        {
            /*
             * field offset: &inst.field-expr - &inst
             */
            char const *field_expr;

            STRTAB_str_to_string(vec_p->val.expr, &field_expr);
            DDBE_SPELL_INDEX(fid, vec_p->index);
            fprintf(fid, "(idl_byte *)%s%s.%s - (idl_byte *)&%s,",
                    (vec_p->kind == DDBE_vec_expr_arr_k) ? "" : "&",
                    inst_name, field_expr, inst_name);

            DDBE_SPELL_COMMENT(fid, vec_p->comment, "\t/* %s */", comment);
            fprintf(fid, "\n");
            break;
        }

        default:
            INTERNAL_ERROR("Invalid offset vector entry kind");
        }

        vec_p = vec_p->next;
    }

    fprintf(fid, "0");
    DDBE_SPELL_TEXT(fid, "\t/* sentinel */");
    fprintf(fid, "\n};\n\n");
}


/*
 *  D D B E _ i n i t _ o f f s e t _ v e c
 *
 *  Spells an uninitialized definition of the offset vector and a routine
 *  IDL_init_offset_vec which must be called once per interface to initialize
 *  the offset vector.  Alternative to using DDBE_spell_offset_vec_use_inst.
 */
void DDBE_init_offset_vec
(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vectors_t      *vip,       /* [in] vector information pointer */
    boolean             *cmd_opt ATTRIBUTE_UNUSED,   /* [in] array of cmd option flags */
    void                **cmd_val ATTRIBUTE_UNUSED  /* [in] array of cmd option values */
)
{
    DDBE_vec_rep_t      *vec_p;     /* Ptr to offset vector entry list */
    AST_type_n_t        *type_p;    /* Ptr to AST type node */
    unsigned long       last_index; /* Last index spelled */
    char const          *inst_name; /* Name of generated instance of type */
#ifdef DUMPERS
    char const               *comment;   /* Comment text */
#endif

    vec_p = vip->offset_p;
    inst_name = NULL;

    fprintf(fid, "static idl_ulong_int %soffset_vec[%s];\n\n",
            DDBE_PREFIX_IDL, DDBE_spell_long_nf(vip->offset_vec_size));

    fprintf(fid, "static void %sinit_offset_vec\n", DDBE_PREFIX_IDL);
    fprintf(fid, "(void)\n");

    fprintf(fid, "%soffset_vec[%s] = 0;", DDBE_PREFIX_IDL, DDBE_spell_long(0));
    DDBE_SPELL_TEXT(fid, "\t/* sentinel */");
    fprintf(fid, "\n");
    last_index = 0;

    while (vec_p != NULL)
    {
        type_p = vec_p->val.type_p;

        switch (vec_p->kind)
        {
        case DDBE_vec_comment_k:
            DDBE_SPELL_COMMENT(fid, vec_p->comment, "/*\n * %s\n */\n", comment);
            break;

        case DDBE_vec_expr_long_k:
        {
            /* Simple expression for long value */
            char const *long_expr;

            STRTAB_str_to_string(vec_p->val.expr, &long_expr);
            fprintf(fid, "%soffset_vec[%s] = ", DDBE_PREFIX_IDL,
                    DDBE_spell_long(vec_p->index));
            fprintf(fid, "%s;", long_expr);
            last_index = vec_p->index;

            DDBE_SPELL_COMMENT(fid, vec_p->comment, "\t/* %s */", comment);
            fprintf(fid, "\n");
            break;
        }

        case DDBE_vec_sizeof_k:
            fprintf(fid, "%soffset_vec[%s] = ", DDBE_PREFIX_IDL,
                    DDBE_spell_long(vec_p->index));
            DDBE_sizeof_expr_use_inst(fid, type_p, vec_p->comment);
            fprintf(fid, ";\n");
            last_index = vec_p->index;
            break;

        case DDBE_vec_noop_k:
            break;

        case DDBE_vec_offset_end_k:
            inst_name = NULL;
            break;

        case DDBE_vec_offset_begin_k:
            /* Form instance name for struct */
            NAMETABLE_id_to_string(type_p->be_info.dd_type->inst_name,
                &inst_name);
            break;

        case DDBE_vec_expr_k:
        case DDBE_vec_expr_arr_k:
        {
            /*
             * field offset: &inst.field-expr - &inst
             */
            char const *field_expr;

            STRTAB_str_to_string(vec_p->val.expr, &field_expr);
            fprintf(fid, "%soffset_vec[%s] = ", DDBE_PREFIX_IDL,
                    DDBE_spell_long(vec_p->index));
            fprintf(fid, "(idl_byte *)%s%s.%s - (idl_byte *)&%s;",
                    (vec_p->kind == DDBE_vec_expr_arr_k) ? "" : "&",
                    inst_name, field_expr, inst_name);
            last_index = vec_p->index;

            DDBE_SPELL_COMMENT(fid, vec_p->comment, "\t/* %s */", comment);
            fprintf(fid, "\n");
            break;
        }

        default:
            INTERNAL_ERROR("Invalid offset vector entry kind");
        }

        vec_p = vec_p->next;
    }

    fprintf(fid, "%soffset_vec[%s] = 0;", DDBE_PREFIX_IDL,
            DDBE_spell_long(last_index+1));
    DDBE_SPELL_TEXT(fid, "\t/* sentinel */");
    fprintf(fid, "\n}\n\n");
}


/*
 *  D D B E _ s p e l l _ r t n _ v e c
 *
 *  Spells the routine vector definition.
 */
void DDBE_spell_rtn_vec
(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vectors_t      *vip,       /* [in] vector information pointer */
    boolean             *cmd_opt ATTRIBUTE_UNUSED,   /* [in] array of cmd option flags */
    void                **cmd_val ATTRIBUTE_UNUSED,  /* [in] array of cmd option values */
    boolean         client_side    /* [in] T=>client only, F=>server only */
)
{
    DDBE_vec_rep_t      *vec_p;     /* Ptr to routine vector entry list */
    char const *rtn_name;
#ifdef DUMPERS
    char const    *comment;
#endif

    vec_p = vip->rtn_p;

    fprintf(fid, "static IDL_rtn_func_t %srtn_vec[] = {\n", DDBE_PREFIX_IDL);
    fprintf(fid, "(IDL_rtn_func_t)NULL,");
    DDBE_SPELL_TEXT(fid, "\t/* sentinel */");
    fprintf(fid, "\n");

    for ( ; vec_p != NULL; vec_p = vec_p->next)
	 {
		 if (vec_p->kind == DDBE_vec_noop_k)
			 continue;
		 if (vec_p->kind == DDBE_vec_comment_k)
		 {
			 DDBE_SPELL_COMMENT(fid, vec_p->comment, "/* %s */\n", comment);
			 continue;
		 }

		 NAMETABLE_id_to_string(vec_p->val.name, &rtn_name);
		 DDBE_SPELL_INDEX(fid, vec_p->index);
		 if (   (vec_p->kind == DDBE_vec_name_client_k && !client_side)
				 || (vec_p->kind == DDBE_vec_name_server_k && client_side) )
			 fprintf(fid, "(IDL_rtn_func_t)NULL,\n");
		 else
			 fprintf(fid, "(IDL_rtn_func_t)%s,\n", rtn_name);
	 }

    fprintf(fid, "(IDL_rtn_func_t)NULL");
    DDBE_SPELL_TEXT(fid, "\t/* sentinel */");
    fprintf(fid, "\n};\n\n");
}


/*
 *  D D B E _ s p e l l _ t y p e _ v e c _ p r e a m b l e
 *
 *  Spells the preamble portion of the type vector definition.
 *  Assumes: vip->type_vec_size is offset to addenda portion of type vector.
 */
void DDBE_spell_type_vec_preamble
(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vectors_t      *vip        /* [in] vector information pointer */
)
{
    AST_interface_n_t   *int_p;     /* Ptr to AST interface node */
    AST_export_n_t      *export_p;  /* Ptr to AST export node */
    AST_operation_n_t   *oper_p;    /* Ptr to AST operation node */
    char const          *int_name;  /* Interface name */
    char const          *oper_name; /* Operation name */
    byte                *bp;        /* Pointer to byte stream */
    char                *getenvres; /* Environment variable translation */
    unsigned long       index;      /* Type vector index */
    unsigned long       longint;    /* 4-byte integer data */
    unsigned short      shortint;   /* 2-byte integer data */
    boolean         spell_stg_info; /* TRUE => spell storage information list */

    int_p = vip->ast_int_p;
    NAMETABLE_id_to_string(int_p->name, &int_name);

    getenvres = getenv("IDL_GEN_INTF_DATA");
    spell_stg_info = (getenvres != NULL);

    /*
     * NOTE: Currently assume ASCII encodings.
     */
    DDBE_SPELL_INDEX(fid, 0);
    fprintf(fid, "0xff,0xff,0xff,0xff,\n");

    DDBE_SPELL_INDEX(fid, 4);
    if (DDBE_little_endian){
	fprintf(fid, "1,");
	DDBE_SPELL_TEXT(fid, "\t\t/* little endian */");
    }
    else{
	fprintf(fid, "0,");
	DDBE_SPELL_TEXT(fid, "\t\t/* big endian */");
    }
    fprintf(fid, "\n");

    DDBE_SPELL_INDEX(fid, 5);
    fprintf(fid, "0,");
    DDBE_SPELL_TEXT(fid, "\t\t/* ASCII */");
    fprintf(fid, "\n");

    DDBE_SPELL_INDEX(fid, 6);
    fprintf(fid, "0xff,0xff,\n");

    shortint = DDBE_VER_MAJOR;
    DDBE_SPELL_INDEX(fid, 8);
    DDBE_spell_short_bytes(fid, &shortint, DDBE_little_endian);
    DDBE_SPELL_TEXT_1ARG(fid, "\t/* interpreter encoding major version %d */",
                         shortint);
    fprintf(fid, "\n");

    shortint = DDBE_VER_MINOR;
    DDBE_SPELL_INDEX(fid, 10);
    DDBE_spell_short_bytes(fid, &shortint, DDBE_little_endian);
    DDBE_SPELL_TEXT_1ARG(fid, "\t/* interpreter encoding minor version %d */",
                         shortint);
    fprintf(fid, "\n");

    shortint = int_p->version % 65536;
    DDBE_SPELL_INDEX(fid, 12);
    DDBE_spell_short_bytes(fid, &shortint, DDBE_little_endian);
    DDBE_SPELL_TEXT_2ARG(fid, "\t/* interface %s major version %d */",
                         int_name, shortint);
    fprintf(fid, "\n");

    shortint = int_p->version / 65536;
    DDBE_SPELL_INDEX(fid, 14);
    DDBE_spell_short_bytes(fid, &shortint, DDBE_little_endian);
    DDBE_SPELL_TEXT_2ARG(fid, "\t/* interface %s minor version %d */",
                         int_name, shortint);
    fprintf(fid, "\n");

    longint = int_p->uuid.time_low;
    DDBE_SPELL_INDEX(fid, 16);
    DDBE_spell_long_bytes(fid, &longint, DDBE_little_endian);
    DDBE_SPELL_TEXT(fid, "\t/* uuid time_low */");
    fprintf(fid, "\n");

    shortint = int_p->uuid.time_mid;
    DDBE_SPELL_INDEX(fid, 20);
    DDBE_spell_short_bytes(fid, &shortint, DDBE_little_endian);
    DDBE_SPELL_TEXT(fid, "\t/* uuid time_mid */");
    fprintf(fid, "\n");

    shortint = int_p->uuid.time_hi_and_version;
    DDBE_SPELL_INDEX(fid, 22);
    DDBE_spell_short_bytes(fid, &shortint, DDBE_little_endian);
    DDBE_SPELL_TEXT(fid, "\t/* uuid time_hi_and_version */");
    fprintf(fid, "\n");

    DDBE_SPELL_INDEX(fid, 24);
    fprintf(fid, "0x%02x,", int_p->uuid.clock_seq_hi_and_reserved);
    DDBE_SPELL_TEXT(fid, "\t\t/* uuid clock_seq_hi_and_reserved */");
    fprintf(fid, "\n");

    DDBE_SPELL_INDEX(fid, 25);
    fprintf(fid, "0x%02x,", int_p->uuid.clock_seq_low);
    DDBE_SPELL_TEXT(fid, "\t\t/* uuid clock_seq_low */");
    fprintf(fid, "\n");

    bp = int_p->uuid.node;
    DDBE_SPELL_INDEX(fid, 26);
    fprintf(fid, "0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,",
            bp[0], bp[1], bp[2], bp[3], bp[4], bp[5]);
    DDBE_SPELL_TEXT(fid, "\t/* uuid node */");
    fprintf(fid, "\n");

    DDBE_SPELL_INDEX(fid, 32);
    if (spell_stg_info)
    {
        /* Assume storage info immediately follows bugs list */
        longint = vip->type_vec_size + (NUM_BUGS/32 + 1) * 4;
        DDBE_spell_long_bytes(fid, &longint, DDBE_little_endian);
        DDBE_SPELL_TEXT_1ARG(fid, "\t/* index of storage information = %s */",
                             DDBE_spell_long_nf(longint));
    }
    else
    {
        fprintf(fid, "0x00,0x00,0x00,0x00,");
        DDBE_SPELL_TEXT(fid, "\t/* no storage information */");
    }
    fprintf(fid, "\n");

    longint = NUM_BUGS;
    DDBE_SPELL_INDEX(fid, 36);
    DDBE_spell_long_bytes(fid, &longint, DDBE_little_endian);
    DDBE_SPELL_TEXT_1ARG(fid, "\t/* number of bug flags = %ld */", longint);
    fprintf(fid, "\n");

    longint = vip->type_vec_size;
    DDBE_SPELL_INDEX(fid, 40);
    DDBE_spell_long_bytes(fid, &longint, DDBE_little_endian);
    DDBE_SPELL_TEXT_1ARG(fid, "\t/* index of bug flags = %s */",
                         DDBE_spell_long_nf(longint));
    fprintf(fid, "\n");

    DDBE_SPELL_INDEX(fid, 44);
    fprintf(fid, "0xff,0xff,0xff,0xff,\n");

    DDBE_SPELL_INDEX(fid, 48);
    fprintf(fid, "0xff,0xff,0xff,0xff,\n");

    DDBE_SPELL_INDEX(fid, 52);
    fprintf(fid, "0xff,0xff,0xff,0xff,\n");

    DDBE_SPELL_INDEX(fid, 56);
    fprintf(fid, "0xff,0xff,0xff,0xff,\n");

    longint = int_p->op_count;
    DDBE_SPELL_INDEX(fid, 60);
    DDBE_spell_long_bytes(fid, &longint, DDBE_little_endian);
    DDBE_SPELL_TEXT_1ARG(fid, "\t/* number of operations = %ld */", longint);
    fprintf(fid, "\n");

    index = DDBE_PARAM_START;
    ASSERTION(DDBE_PARAM_START == 64);

    /*
     * Process each operation in the interface.
     */
    for (export_p = int_p->exports; export_p != NULL; export_p = export_p->next)
    {
        if (export_p->kind == AST_operation_k)
        {
            oper_p = export_p->thing_p.exported_operation;
            NAMETABLE_id_to_string(oper_p->name, &oper_name);

            DDBE_SPELL_INDEX(fid, index);
            longint = 0;
            /*
             * The following code assumes these definitions in rpcbase.idl:
             *  const long  rpc_c_call_brdcst               = 0x00000001;
             *  const long  rpc_c_call_idempotent           = 0x00000002;
             *  const long  rpc_c_call_maybe                = 0x00000004;
             */
            if (AST_BROADCAST_SET(oper_p))  longint |= 1;
            if (AST_IDEMPOTENT_SET(oper_p)) longint |= 2;
            if (AST_MAYBE_SET(oper_p))      longint |= 4;
            DDBE_spell_long_bytes(fid, &longint, DDBE_little_endian);
            DDBE_SPELL_TEXT_1ARG(fid, "\t/* operation %s flags */", oper_name);
            fprintf(fid, "\n");
            index += 4; /* 4 = sizeof(idl_long) */

            longint = oper_p->be_info.dd_oper->num_params;
            DDBE_SPELL_INDEX(fid, index);
            DDBE_spell_long_bytes(fid, &longint, DDBE_little_endian);
            DDBE_SPELL_TEXT_2ARG(fid, "\t/* number of %s params = %ld */",
                                 oper_name, longint);
            fprintf(fid, "\n");
            index += 4; /* 4 = sizeof(idl_long) */

            longint = oper_p->be_info.dd_oper->num_srv_ins;
            DDBE_SPELL_INDEX(fid, index);
            DDBE_spell_long_bytes(fid, &longint, DDBE_little_endian);
            DDBE_SPELL_TEXT_2ARG(fid, "\t/* number of %s [in]s  = %ld */",
                                 oper_name, longint);
            fprintf(fid, "\n");
            index += 4; /* 4 = sizeof(idl_long) */
                
            if (oper_p->be_info.dd_oper->ins_type_vec_p == NULL)
            {
                DDBE_SPELL_INDEX(fid, index);
                fprintf(fid, "0xff,0xff,0xff,0xff,\n");
            }
            else
            {
                longint = oper_p->be_info.dd_oper->ins_type_vec_p->index;
                DDBE_SPELL_INDEX(fid, index);
                DDBE_spell_long_bytes(fid, &longint, DDBE_little_endian);
                DDBE_SPELL_TEXT_2ARG(fid, "\t/* index  of %s [in]s  = %s */",
                                     oper_name, DDBE_spell_long_nf(longint));
                fprintf(fid, "\n");
            }
            index += 4; /* 4 = sizeof(idl_long) */

            longint = oper_p->be_info.dd_oper->num_outs;
            DDBE_SPELL_INDEX(fid, index);
            DDBE_spell_long_bytes(fid, &longint, DDBE_little_endian);
            DDBE_SPELL_TEXT_2ARG(fid, "\t/* number of %s [out]s = %ld */",
                                 oper_name, longint);
            fprintf(fid, "\n");
            index += 4; /* 4 = sizeof(idl_long) */
                
            if (oper_p->be_info.dd_oper->outs_type_vec_p == NULL)
            {
                DDBE_SPELL_INDEX(fid, index);
                fprintf(fid, "0xff,0xff,0xff,0xff,\n");
            }
            else
            {
                longint = oper_p->be_info.dd_oper->outs_type_vec_p->index;
                DDBE_SPELL_INDEX(fid, index);
                DDBE_spell_long_bytes(fid, &longint, DDBE_little_endian);
                DDBE_SPELL_TEXT_2ARG(fid, "\t/* index  of %s [out]s = %s */",
                                     oper_name, DDBE_spell_long_nf(longint));
                fprintf(fid, "\n");
            }
            index += 4; /* 4 = sizeof(idl_long) */
        }
    }
}


/*
 *  D D B E _ s p e l l _ t y p e _ v e c _ a d d e n d a
 *
 *  Spells the addenda portion of the type vector definition.
 *  Assumes: vip->type_vec_size does not include the addenda on entry,
 *           but does include alignment pad bytes before the addenda.
 *           vip->type_vec_size does include the addenda on exit.
 */
void DDBE_spell_type_vec_addenda
(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vectors_t      *vip,       /* [io] vector information pointer */
    boolean             *cmd_opt ATTRIBUTE_UNUSED,   /* [in] array of cmd option flags */
    void                **cmd_val  /* [in] array of cmd option values */
)
{
    unsigned long size,index,longint; /* 4-byte integer data */
    byte                *p4;        /* Pointer to 4-byte integer */
    boolean             *do_bug;    /* Pointer to array of bug flags */
    char                *getenvres; /* Environment variable translation */
    DDBE_vec_rep_t      *vec_p;     /* Ptr to vector entry in list */
    int                 bugnum;     /* Current bug number being processed */
    int                 i, j;
    boolean         spell_stg_info; /* TRUE => spell storage information list */

    p4 = (byte *)&longint;

    /*
     * Process bugs flags.  Bug numbers start at 1, bug 0 value is a don't-care.
     */
    do_bug = (boolean *)cmd_val[opt_do_bug];
    index  = vip->type_vec_size;
    bugnum = 0;
    for (i = 0; i <= NUM_BUGS/32; i++)
    {
        longint = 0;
        for (j = 0; j < 32 && bugnum <= NUM_BUGS; j++)
        {
            if (do_bug[bugnum])
                longint |= (1 << j);
            bugnum++;
        }
        DDBE_SPELL_INDEX(fid, index);
        fprintf(fid, "0x%02x,0x%02x,0x%02x,0x%02x,",p4[0], p4[1], p4[2], p4[3]);
        DDBE_SPELL_TEXT(fid, "\t/* bug flags */");
        fprintf(fid, "\n");
        index += 4;
    }

    /*
     * Process storage information.
     */
    getenvres = getenv("IDL_GEN_INTF_DATA");
    spell_stg_info = (getenvres != NULL);
    if (!spell_stg_info)
        return;

    /* Count type information entries; note guaranteed sentinel entry at head */
    for (i = 0, vec_p = vip->type_info_p->next;
         vec_p != NULL;
         i++, vec_p = vec_p->next)
        ;

    /*
     * Compute size of type vector and do initial information entries:
     *  total size (in bytes) of type vector
     *  total size (in idl_ulong_ints) of offset vector
     *  total size (in pointer-sized words) of routine vector
     *  number of 'offset types'
     *  index of 'offset types'
     */
    size = index + 20 /*5 entries above*/ + i*4 /*offset types list*/
                 +  1 /* single byte sentinel entry */;

    longint = size;
    DDBE_SPELL_INDEX(fid, index);
    fprintf(fid, "0x%02x,0x%02x,0x%02x,0x%02x,",p4[0], p4[1], p4[2], p4[3]);
    DDBE_SPELL_TEXT_1ARG(fid, "\t/* type vector size = %s */",
                         DDBE_spell_long_nf(longint));
    fprintf(fid, "\n");
    index += 4;

    longint = vip->offset_vec_size;
    DDBE_SPELL_INDEX(fid, index);
    fprintf(fid, "0x%02x,0x%02x,0x%02x,0x%02x,",p4[0], p4[1], p4[2], p4[3]);
    DDBE_SPELL_TEXT_1ARG(fid, "\t/* offset vector size = %s */",
                         DDBE_spell_long_nf(longint));
    fprintf(fid, "\n");
    index += 4;

    longint = vip->rtn_vec_size;
    DDBE_SPELL_INDEX(fid, index);
    fprintf(fid, "0x%02x,0x%02x,0x%02x,0x%02x,",p4[0], p4[1], p4[2], p4[3]);
    DDBE_SPELL_TEXT_1ARG(fid, "\t/* routine vector size = %s */",
                         DDBE_spell_long_nf(longint));
    fprintf(fid, "\n");
    index += 4;

    longint = i;
    DDBE_SPELL_INDEX(fid, index);
    fprintf(fid, "0x%02x,0x%02x,0x%02x,0x%02x,",p4[0], p4[1], p4[2], p4[3]);
    DDBE_SPELL_TEXT_1ARG(fid, "\t/* number of offset types = %ld */", longint);
    fprintf(fid, "\n");
    index += 4;

    longint = index + 4;  /* offset types list immediately follows this word */
    DDBE_SPELL_INDEX(fid, index);
    fprintf(fid, "0x%02x,0x%02x,0x%02x,0x%02x,",p4[0], p4[1], p4[2], p4[3]);
    DDBE_SPELL_TEXT_1ARG(fid, "\t/* index of offset types = %s */",
                         DDBE_spell_long_nf(longint));
    fprintf(fid, "\n");
    index += 4;

    /* Walk type information list; note guaranteed sentinel entry at head */
    for (vec_p = vip->type_info_p->next; vec_p != NULL; vec_p = vec_p->next)
    {
        DDBE_SPELL_INDEX(fid, index);
        longint = vec_p->val.ref_p->index;
        DDBE_spell_long_bytes(fid, &longint, DDBE_little_endian);
        DDBE_SPELL_TEXT_1ARG(fid, "\t/* %s */", DDBE_spell_long(longint));
        fprintf(fid, "\n");
        index += 4;
    }

    vip->type_vec_size = size;
}


/*
 *  D D B E _ s p e l l _ t y p e _ v e c
 *
 *  Spells the type vector definition.
 */
void DDBE_spell_type_vec
(
    FILE                *fid,       /* [in] output file handle */
    DDBE_vectors_t      *vip,       /* [in] vector information pointer */
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val   /* [in] array of cmd option values */
)
{
    DDBE_vec_rep_t      *vec_p;     /* Ptr to type vector entry list */
    char const *name;
    char const *expr;
#ifdef DUMPERS
    char const   *comment;
#endif
    int     i;

    vec_p = vip->type_p;

    fprintf(fid, "static idl_byte %stype_vec[] = {\n", DDBE_PREFIX_IDL);
    DDBE_spell_type_vec_preamble(fid, vip);

    while (vec_p != NULL)
    {
        switch (vec_p->kind)
        {
        case DDBE_vec_byte_k:
        case DDBE_vec_byte_3m4_k:
            DDBE_SPELL_INDEX(fid, vec_p->index);
            fprintf(fid, "%d,", vec_p->val.byte_val);
            DDBE_SPELL_COMMENT(fid, vec_p->comment, "\t/* %s */", comment);
            fprintf(fid, "\n");
            break;

        case DDBE_vec_comment_k:
            DDBE_SPELL_COMMENT(fid, vec_p->comment, "\t\t/* %s */\n", comment);
            break;

        case DDBE_vec_expr_byte_k:
            STRTAB_str_to_string(vec_p->val.expr, &expr);
            DDBE_SPELL_INDEX(fid, vec_p->index);
            fprintf(fid, "%s,", expr);
            DDBE_SPELL_COMMENT(fid, vec_p->comment, "\t/* %s */", comment);
            fprintf(fid, "\n");
            break;

        case DDBE_vec_long_k:
            DDBE_SPELL_INDEX(fid, vec_p->index);
            DDBE_spell_long_val(fid, vec_p, DDBE_little_endian);
            break;

        case DDBE_vec_long_bool_k:
            DDBE_SPELL_INDEX(fid, vec_p->index);
            DDBE_spell_long_bool_val(fid, vec_p, DDBE_little_endian);
            break;

        case DDBE_vec_noop_k:
            break;

        case DDBE_vec_pad_k:
            /*
             * A pad entry says to pad the vector with a number of filler bytes.
             * Spell out the proper number of filler bytes.
             */
            DDBE_SPELL_INDEX(fid, vec_p->index);
            for (i = 0; i < vec_p->val.byte_val; i++)
                fprintf(fid, "0xff,");
            DDBE_SPELL_TEXT(fid, "\t/* filler */");
            fprintf(fid, "\n");
            break;
        case DDBE_vec_short_k:
            DDBE_SPELL_INDEX(fid, vec_p->index);
            DDBE_spell_short_bytes(fid, (unsigned short *)&vec_p->val.short_val,
                DDBE_little_endian);
            fprintf(fid, "\n");
            break;

        case DDBE_vec_tag_k:
            DDBE_SPELL_INDEX(fid, vec_p->index);
            NAMETABLE_id_to_string(vec_p->val.name, &name);
            fprintf(fid, "%s,\n", name);
            break;

        case DDBE_vec_type_kind_k:
            DDBE_SPELL_INDEX(fid, vec_p->index);
            DDBE_spell_type_kind(fid, vec_p);
            break;

        case DDBE_vec_expr_arr_k:
        case DDBE_vec_expr_k:
        case DDBE_vec_name_k:
        case DDBE_vec_name_client_k:
        case DDBE_vec_name_server_k:
        case DDBE_vec_offset_begin_k:
        case DDBE_vec_offset_end_k:
        case DDBE_vec_sizeof_k:
            INTERNAL_ERROR("Vector entry not valid for type vector");

        case DDBE_vec_indirect_k:
        case DDBE_vec_reference_k:
            INTERNAL_ERROR("Unexpected indirect type vector entry");

        default:
            INTERNAL_ERROR("Invalid type vector entry");
        }

        vec_p = vec_p->next;
    }

    DDBE_spell_type_vec_addenda(fid, vip, cmd_opt, cmd_val);

    fprintf(fid, "0");
    DDBE_SPELL_TEXT(fid, "\t\t/* sentinel */");
    fprintf(fid, "\n};\n\n");
}


/*
 *  D D B E _ s p e l l _ p a r a m _ v e c _ d e f
 *
 *  Spell the definition of the parameter vector for an operation.
 */
void DDBE_spell_param_vec_def
(
    FILE            *fid,           /* [in] output file handle */
    AST_operation_n_t *oper_p,      /* [in] ptr to AST operation node */
    BE_side_t       side,           /* [in] client or server side code */
    boolean         *cmd_opt ATTRIBUTE_UNUSED,       /* [in] array of cmd option flags */
    void            **cmd_val ATTRIBUTE_UNUSED      /* [in] array of cmd option values */
)
{
    DDBE_oper_i_t   *oper_i_p;      /* Ptr to operation info node */
    AST_parameter_n_t *param_p;     /* Ptr to AST parameter node */
    unsigned long   param_num;      /* Parameter number */
    char const      *param_name;    /* Parameter name */

    oper_i_p = oper_p->be_info.dd_oper;

    fprintf(fid, "rpc_void_p_t %sparam_vec[%ld];\n", DDBE_PREFIX_IDL,
            oper_i_p->num_params);
    /*
     * Any idl_short_float passed by value needs an idl_short_float stacklocal
     * on the client side which is assigned the parameter value in case the
     * parameter was promoted to an idl_long_float.
     */
    if (side == BE_client_side)
    {
        for (param_num = 1, param_p = oper_p->parameters;
             param_p != NULL;
             param_num++,   param_p = param_p->next)
        {
            if (param_p->type->kind == AST_short_float_k)
            {
                NAMETABLE_id_to_string(param_p->name, &param_name);
                fprintf(fid, "idl_short_float IDL_short_float_%ld = %s;\n",
                        param_num, param_name);
            }
        }
    }
}


/*
 *  D D B E _ s p e l l _ p a r a m _ v e c _ i n i t
 *
 *  Spell the initialization of the parameter vector for an operation.
 */
void DDBE_spell_param_vec_init
(
    FILE            *fid,           /* [in] output file handle */
    AST_operation_n_t *oper_p,      /* [in] ptr to AST operation node */
    BE_side_t       side,           /* [in] client or server side code */
    boolean         *cmd_opt ATTRIBUTE_UNUSED,       /* [in] array of cmd option flags */
    void            **cmd_val ATTRIBUTE_UNUSED      /* [in] array of cmd option values */
)
{
    AST_parameter_n_t *param_p;     /* Ptr to AST parameter node */
    DDBE_oper_i_t   *oper_i_p;      /* Ptr to operation info node */
    unsigned long   param_num;      /* Parameter number */
    char const      *param_name;    /* Parameter name */
    boolean         spell_value;    /* TRUE => spell param value not address */

    oper_i_p = oper_p->be_info.dd_oper;

    /* Function result is stored in a local variable, unless we are using an
        additional parameter on the client side for a non-C language or the
        function result is 'char' on the client side and -lang fortran. */
    if (oper_p->result->type->kind != AST_void_k)
        fprintf(fid, "%sparam_vec[0] = (rpc_void_p_t)%cIDL_function_result;\n",
                DDBE_PREFIX_IDL,
                (
                  false
                ) ? ' ' : '&' );

    for (param_num = 1, param_p = oper_p->parameters;
         param_p != NULL;
         param_num++,   param_p = param_p->next)
    {
        /*
         * Server side conformant parameters that do not have a represent_as
         * type are allocated from heap elsewhere in the server stub routine
         * so no initialization is needed here.
         */
        if (side == BE_server_side && AST_REF_SET(param_p)
            && DDBE_cfmt_arr_local_rep(param_p) )
            continue;

        NAMETABLE_id_to_string(param_p->name, &param_name);
        fprintf(fid, "%sparam_vec[%ld] = (rpc_void_p_t)",
                DDBE_PREFIX_IDL, param_num);

        if (AST_HEAP_SET(param_p) && side == BE_server_side
            && !AST_PTR_SET(param_p) && !AST_UNIQUE_SET(param_p)
        )
        {
            fprintf(fid, "rpc_ss_mem_alloc(&IDL_ms.IDL_mem_handle, sizeof");
            if (param_p->type->kind == AST_pointer_k && AST_REF_SET(param_p))
                CSPELL_cast_exp(fid,
                    param_p->type->type_structure.pointer->pointee_type);
            else
                CSPELL_cast_exp(fid, param_p->type);
            fprintf(fid, ");\n");
        }
        else if (param_p->type->kind == AST_short_float_k
                 && side == BE_client_side)
        {
            fprintf(fid, "&IDL_short_float_%ld;\n", param_num);
        }
        else
        {
            spell_value =
                (   (side == BE_client_side
                     && !(param_p->type->kind == AST_pointer_k
                             && param_p->type->type_structure.pointer
                                ->pointee_type->kind == AST_interface_k)
                     && (AST_REF_SET(param_p)
                                                ))
                 || (side == BE_server_side && AST_REF_SET(param_p)
                     && (param_p->type->kind == AST_array_k
                         || (param_p->type->kind == AST_pointer_k
                             && param_p->type->type_structure.pointer
                                ->pointee_type->kind == AST_array_k))) );
            fprintf(fid, "%s%s;\n", (spell_value) ? "" : "&", param_name);
        }
    }

    if (param_num != oper_i_p->num_params)
    {
        INTERNAL_ERROR("Param count does not match param vec allocation");
    }
}


/*
 *  D D B E _ s p e l l _ m a r s h _ o r _ u n m a r
 *
 *  Spells the code to marshall or unmarshall the parameters in an operation.
 */
void DDBE_spell_marsh_or_unmar
(
    FILE            *fid,           /* [in] output file handle */
    AST_operation_n_t *oper_p,      /* [in] ptr to AST operation node */
    char            *interp_name,   /* [in] marshalling interpreter rtn name */
    char            *state_ptr_name,/* [in] name of state pointer variable */
    BE_side_t       side,           /* [in] client or server side code */
    BE_marshalling_k_t mar_or_unmar /* [in] spell marshall or unmarshall code */
)
{
    DDBE_oper_i_t   *oper_i_p;      /* Ptr to operation info node */
    boolean         in_params;      /* TRUE => processing [in] parameters */

    oper_i_p = oper_p->be_info.dd_oper;

    in_params = ((side == BE_client_side && mar_or_unmar == BE_marshalling_k)
              || (side == BE_server_side && mar_or_unmar == BE_unmarshalling_k));

    /* Marshalling interpreter routine name */
    fprintf(fid, "%s(", interp_name);

    /* Number of parameters */
    fprintf(fid, "\n    %ld,",
        (in_params) ? ((side == BE_server_side) ?
                        oper_i_p->num_srv_ins : oper_i_p->num_ins)
                    : oper_i_p->num_outs);
    DDBE_SPELL_TEXT_1ARG(fid, "\t/* number of [%s] parameters */",
                         ((in_params) ? "in" : "out"));

    /* Type vector index of first param */
    fprintf(fid, "\n    %s,",
        (in_params) ? DDBE_spell_long_nf(oper_i_p->ins_type_index)
                      : DDBE_spell_long_nf(oper_i_p->outs_type_index));
    DDBE_SPELL_TEXT_1ARG(fid,
                         "\t/* type vector index of first [%s] parameter */",
                         ((in_params) ? "in" : "out"));

    /* Param vector, marshalling state */
    fprintf(fid, "\n    IDL_param_vec, %s", state_ptr_name);

    /* End of routine call */
    fprintf(fid, ");\n");
}
