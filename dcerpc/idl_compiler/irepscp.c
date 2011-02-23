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
**      irepscp.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Scoping routines that use the Intermediate Representation to keep
**  information about data type scoping.  Used by irep generation and
**  potentially by backends to keep track of nested constructs with
**  special processing requirements.
**
**
*/

#include <nidl.h>       /* Standard IDL defs */
#include <ast.h>        /* Abstract Syntax Tree defs */
#include <irep.h>       /* Intermediate Rep defs */

/*
**  p r i v a t e   s c o p i n g   s e r v i c e s
**
**  Scoping services and data private to this module.
*/
#define IR_SCP_STACK_INIT   64
#define IR_TYP_STACK_INIT   16


/*
**  I R _ p u s h _ s c o p e
**
**  Pushes data on the data scoping stack.  Grows the stack if necessary.
**
**  Assumption: Passed tuple has a valid type node ptr argument.
*/
static void IR_push_scope
(
    IR_scope_ctx_t      *ctx_p,         /* [io] Scope context */
    IR_tup_n_t          *tup_p,         /* [in] Irep tuple */
    AST_instance_n_t    *inst_p,        /* [in] Ptr to AST instance node */
    IR_scope_k_t        scope           /* [in] Scope data kind */
)
{
    /* Grow the stack if necessary */
    if (ctx_p->scope >= ctx_p->scope_stack_size-1)
    {
        IR_scope_t      *new_scope_a;
        int             i;

	new_scope_a = NEW_VEC (IR_scope_t, ctx_p->scope_stack_size * 2);
        for (i = 0; i < ctx_p->scope_stack_size; i++)
        {
            new_scope_a[i].scope_k = ctx_p->scope_a[i].scope_k;
            new_scope_a[i].type_p  = ctx_p->scope_a[i].type_p;
            new_scope_a[i].inst_p  = ctx_p->scope_a[i].inst_p;
        }

        FREE(ctx_p->scope_a);
        ctx_p->scope_a = new_scope_a;
        ctx_p->scope_stack_size *= 2;
    }

    /* Push the scope */
    (ctx_p->scope)++;
    ctx_p->scope_a[ctx_p->scope].scope_k = scope;
    ctx_p->scope_a[ctx_p->scope].type_p  = tup_p->arg[IR_ARG_TYPE].type;
    ctx_p->scope_a[ctx_p->scope].inst_p  = inst_p;
}


/*
**  I R _ p o p _ s c o p e
**
**  Pops data off the data scoping stack.
*/
static void IR_pop_scope
(
    IR_scope_ctx_t      *ctx_p          /* [io] Scope context */
)
{
    ctx_p->scope--;
}


/*
**  I R _ p u s h _ t y p e
**
**  Pushes data on the type scoping stack.  Grows the stack if necessary.
*/
static void IR_push_type
(
    IR_scope_ctx_t      *ctx_p,         /* [io] Scope context */
    AST_type_n_t        *type_p,        /* [in] Ptr to AST type node */
    IR_flags_t          flags           /* [in] IREP flags */
)
{
    /* Grow the stack if necessary */
    if (ctx_p->type_scope >= ctx_p->type_stack_size-1)
    {
        IR_type_scope_t *new_type_s_a;
        int             i;

	new_type_s_a = NEW_VEC (IR_type_scope_t, ctx_p->type_stack_size * 2);
        for (i = 0; i < ctx_p->type_stack_size; i++)
            new_type_s_a[i] = ctx_p->type_s_a[i];

        FREE(ctx_p->type_s_a);
        ctx_p->type_s_a = new_type_s_a;
        ctx_p->type_stack_size *= 2;
    }

    /* Push the type */
    (ctx_p->type_scope)++;
    ctx_p->type_s_a[ctx_p->type_scope].type_p = type_p;
    ctx_p->type_s_a[ctx_p->type_scope].flags  = flags;
}


/*
**  I R _ p o p _ t y p e
**
**  Pops data off the type scoping stack.
*/
static void IR_pop_type
(
    IR_scope_ctx_t      *ctx_p          /* [io] Scope context */
)
{
    ctx_p->type_scope--;
}

/*
**  p u b l i c   s c o p i n g   s e r v i c e s
**
**  The irep provides a simple set of scoping services that it uses and that a
**  backend can use to keep track of the heirarchy of data types.  The first 3
**  routines maintain the scope and the others query the scope.
*/

/*
**  I R _ i n i t _ s c o p e
**
**  Allocates and initializes a scope context and returns its address.
*/
IR_scope_ctx_t *IR_init_scope           /* Returns ptr to new scope context */
(
    struct AST_parameter_n_t *param_p   /* [in] Ptr to AST parameter node */
)
{
    IR_scope_ctx_t      *ctx_p;

    ctx_p = NEW (IR_scope_ctx_t);
    ctx_p->param_p      = param_p;
    ctx_p->saved_inst_p = NULL;
    ctx_p->type_s_a     = NEW_VEC (IR_type_scope_t, IR_TYP_STACK_INIT);
    ctx_p->type_s_a[0].type_p = NULL;
    ctx_p->type_s_a[0].flags  = 0;
    ctx_p->scope_a      = NEW_VEC (IR_scope_t, IR_SCP_STACK_INIT);
    ctx_p->scope_a[0].scope_k = IR_SCP_TOPLEVEL;
    ctx_p->scope_a[0].type_p  = NULL;
    ctx_p->scope_a[0].inst_p  = NULL;
    ctx_p->type_stack_size  = IR_TYP_STACK_INIT;
    ctx_p->scope_stack_size = IR_SCP_STACK_INIT;
    ctx_p->type_scope       = 0;
    ctx_p->scope            = 0;

    return ctx_p;
}


/*
**  I R _ f i n i s h _ s c o p e
**
**  Cleans up a scope context.
*/
void IR_finish_scope
(
    IR_scope_ctx_t      *ctx_p          /* [io] Scope context */
)
{
    FREE(ctx_p->scope_a);
    FREE(ctx_p->type_s_a);
    FREE(ctx_p);
}


/*
**  I R _ p r o c e s s _ t u p
**
**  Processes tuple and maintains scoping data.
*/
void IR_process_tup
(
    IR_scope_ctx_t      *ctx_p,         /* [io] Scope context */
    IR_tup_n_t          *tup_p          /* [in] Irep tuple */
)
{
    switch (tup_p->opcode)
    {
    case IR_op_fixed_array_k:
    case IR_op_conformant_array_k:
    case IR_op_varying_array_k:
    case IR_op_open_array_k:
        IR_push_scope(ctx_p, tup_p, tup_p->arg[IR_ARG_INST].inst, IR_SCP_ARRAY);
        break;

    case IR_op_passed_by_ref_k:
        IR_push_scope(ctx_p, tup_p, tup_p->arg[IR_ARG_INST].inst, IR_SCP_PASSED_BY_REF);
        break;
    case IR_op_ref_ptr_k:
    case IR_op_unique_ptr_k:
    case IR_op_full_ptr_k:
        IR_push_scope(ctx_p, tup_p, tup_p->arg[IR_ARG_INST].inst, IR_SCP_POINTER);
        break;

    case IR_op_array_end_k:
    case IR_op_pointee_end_k:
        IR_pop_scope(ctx_p);
        break;

    case IR_op_type_indirect_k:
        /*
         * Save the instance node address in the context block.  Push type
         * scope; the type scope must be popped by the tuple that marks the
         * end of the indirect type reference.
         */
        ctx_p->saved_inst_p = tup_p->arg[IR_ARG_INST].inst;
        IR_push_type(ctx_p, tup_p->arg[IR_ARG_TYPE].type, tup_p->flags);
        break;

    case IR_op_struct_begin_k:
        /*
         * If a nested struct then field instance is part of struct_begin tuple,
         * otherwise the instance is saved in the context block from the
         * type_indirect reference.
         */
        if (IR_in_struct(ctx_p))
            IR_push_scope(ctx_p, tup_p, tup_p->arg[IR_ARG_INST].inst,
                          IR_SCP_STRUCT);
        else
            IR_push_scope(ctx_p, tup_p, ctx_p->saved_inst_p, IR_SCP_STRUCT);
        break;

    case IR_op_struct_end_k:
        IR_pop_scope(ctx_p);
        if (IR_in_struct(ctx_p) == 0)
            IR_pop_type(ctx_p);
        break;

    case IR_op_disc_union_begin_k:
        IR_push_scope(ctx_p, tup_p, ctx_p->saved_inst_p, IR_SCP_UNION);
        break;

    case IR_op_transmit_as_k:
        IR_push_scope(ctx_p, tup_p, ctx_p->saved_inst_p, IR_SCP_XMIT_AS);
        break;

    case IR_op_represent_as_k:
        IR_push_scope(ctx_p, tup_p, ctx_p->saved_inst_p, IR_SCP_REP_AS);
        break;

    case IR_op_cs_char_k:
        IR_push_scope(ctx_p, tup_p, ctx_p->saved_inst_p, IR_SCP_CS_CHAR);
        break;

    case IR_op_pipe_begin_k:
        IR_push_scope(ctx_p, tup_p, ctx_p->saved_inst_p, IR_SCP_PIPE);
        break;

    case IR_op_disc_union_end_k:
    case IR_op_transmit_end_k:
    case IR_op_represent_end_k:
    case IR_op_cs_char_end_k:
    case IR_op_pipe_end_k:
        IR_pop_scope(ctx_p);
        IR_pop_type(ctx_p);
        break;

    default:
        break;
    }
}


/*
**  I R _ i n _ s c o p e
**
**  Returns immediate nesting level of the specified scope kind.
**  Examples:
**      If processing pointer within struct, IR_in_scope(ctx,IR_SCP_STRUCT) = 0.
**      If processing  scalar within struct, IR_in_scope(ctx,IR_SCP_STRUCT) = 1.
**      If processing  struct within struct, IR_in_scope(ctx,IR_SCP_STRUCT) = 2.
*/
int IR_in_scope                         /* Returns data kind nesting level */
(
    IR_scope_ctx_t      *ctx_p,         /* [in] Scope context */
    IR_scope_k_t        scope           /* [in] Scope data kind */
)
{
    int level;

    level = ctx_p->scope;
    while (level > 0 && ctx_p->scope_a[level].scope_k == scope)
        level--;

    return ctx_p->scope - level;
}


/*
**  I R _ i n _ s c o p e _ p a r e n t
**
**  Works like IR_in_scope except uses the parent scope rather than current
**  scope as its starting point.
*/
int IR_in_scope_parent                  /* Returns data kind nesting level */
(
    IR_scope_ctx_t      *ctx_p,         /* [in] Scope context */
    IR_scope_k_t        scope           /* [in] Scope data kind */
)
{
    int level;

    if (ctx_p->scope == 0)
        return 0;

    level = ctx_p->scope - 1;
    while (level > 0 && ctx_p->scope_a[level].scope_k == scope)
        level--;

    return ctx_p->scope - 1 - level;
}


/*
**  I R _ u n d e r _ s c o p e
**
**  Returns TRUE if in or under a scope of the specified kind.
**  Example: If processing pointer within struct,
**      IR_under_scope(ctx,IR_SCP_POINTER)  = TRUE
**      IR_under_scope(ctx,IR_SCP_STRUCT)   = TRUE
**      IR_under_scope(ctx,IR_SCP_ARRAY)    = FALSE
*/
boolean IR_under_scope                  /* Returns TRUE if under scope kind */
(
    IR_scope_ctx_t      *ctx_p,         /* [in] Scope context */
    IR_scope_k_t        scope           /* [in] Scope data kind */
)
{
    int level;

    for (level = ctx_p->scope; level > 0; level--)
        if (ctx_p->scope_a[level].scope_k == scope)
            return TRUE;

    return FALSE;
}


/*
**  I R _ u n d e r _ s c o p e _ u n d e r _ s c o p e
**
**  Returns TRUE if in or under a child scope of the specified kind and the
**  child scope is in or under a parent scope of the specified kind.
**  Example: If processing pointer within struct,
**      IR_under_scope_under_scope(ctx,IR_SCP_POINTER,IR_SCP_STRUCT) = TRUE
**      IR_under_scope_under_scope(ctx,IR_SCP_STRUCT,IR_SCP_POINTER) = FALSE
*/
boolean IR_under_scope_under_scope      /* Returns TRUE if under scope kinds */
(
    IR_scope_ctx_t      *ctx_p,         /* [in] Scope context */
    IR_scope_k_t        cscope,         /* [in] Child scope data kind */
    IR_scope_k_t        pscope          /* [in] Parent scope data kind */
)
{
    int level;

    for (level = ctx_p->scope; level > 0; level--)
        if (ctx_p->scope_a[level].scope_k == cscope)
            break;

    if (level != 0)
        for (level--; level > 0; level--)
            if (ctx_p->scope_a[level].scope_k == pscope)
                return TRUE;

    return FALSE;
}


/*
**  I R _ u n d e r _ t y p e
**
**  Returns nesting level of a type given by the specified address.
**  Useful for detecting self-pointing data structures.
**
**  NOTE: Does not count type with REP_AS or CS_CHAR flag, because:
**   a) such types cannot be nested
**   b) such types have represent_as tuples hanging off rep_as node, but can
**      have a separate set of tuples (e.g. structure) which can be nested,
**      and we want this routine to correctly determine nesting for that case.
*/
int IR_under_type                       /* Returns type nesting level */
(
    IR_scope_ctx_t      *ctx_p,         /* [in] Scope context */
    AST_type_n_t        *type_p         /* [in] Ptr to AST type node */
)
{
    int level;
    int nesting = 0;

    for (level = ctx_p->type_scope; level > 0; level--)
        if (ctx_p->type_s_a[level].type_p == type_p
            && !(ctx_p->type_s_a[level].flags & IR_REP_AS)
            && !(ctx_p->type_s_a[level].flags & IR_CS_CHAR))
            nesting++;

    return nesting;
}


/*
**  I R _ f i e l d _ e x p r
**
**  Constructs a field expression for a toplevel or nested field reference.
*/
STRTAB_str_t IR_field_expr              /* Returns field expression */
(
    IR_scope_ctx_t      *ctx_p,         /* [in] Scope context */
    AST_field_n_t       *field_p        /* [in] Ptr to AST field node */
)
{
    AST_instance_n_t    *inst_p;        /* Instance node with struct scope */
    char const          *inst_name;     /* Instance name */
    char const          *field_name;    /* Field name */
    int                 level;          /* Scope level */
    int                 base_level;     /* Most deeply nested struct scope */
    int                 depth;          /* Depth of struct nesting */
    char            expr[IR_MAX_EXPR];  /* Field expression */

    NAMETABLE_id_to_string(field_p->name, &field_name);

    /*
     * Get structure nesting depth.  If depth = 0, we are in a separate data
     * scope (e.g. transmit_as) which is a field instance.  This means we need
     * to look at the parent scope to get the correct structure nesting depth.
     */
    base_level = ctx_p->scope;
    depth = IR_in_scope(ctx_p, IR_SCP_STRUCT);
    if (depth == 0)
    {
        depth = IR_in_scope_parent(ctx_p, IR_SCP_STRUCT);
        base_level--;
    }
    if (depth == 0)
        INTERNAL_ERROR("Expected to be in structure scope");

    /*
     * If depth = 1, just return field name.
     */
    if (depth == 1)
        return STRTAB_add_string(field_name);

    expr[0] = '\0';

    /*
     * For each reference level except the toplevel one, cat the field instance
     * name that references the struct.  The toplevel instance name is
     * determined from the field node passed by the caller of this routine.
     */
    for (level = base_level - depth + 2; level <= base_level; level++)
    {
        inst_p = ctx_p->scope_a[level].inst_p;
        NAMETABLE_id_to_string(inst_p->name, &inst_name);
        strcat(expr, inst_name);
        strcat(expr, ".");
    }

    strcat(expr, field_name);
    return STRTAB_add_string(expr);
}
