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
 **      irepgen.c
 **
 **  FACILITY:
 **
 **      Interface Definition Language (IDL) Compiler
 **
 **  ABSTRACT:
 **
 **  Generates the Intermediate Representation of the IDL interface.
 **
 **  %a%private_begin
 **
 **
 **  %a%private_end  
 */

#include <nidl.h>       /* Standard IDL defs */
#include <ast.h>        /* Abstract Syntax Tree defs */
#include <astp.h>       /* AST processing package */
#include <irep.h>       /* Intermediate Rep defs */


/* Necessary forward function declarations */

static void IR_gen_type_rep(
		IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
		AST_type_n_t        *type_p,    /* [in] Ptr to AST type node */
		AST_instance_n_t    *inst_p,    /* [in] Ptr to AST instance node */
		IR_flags_t          flags       /* [in] IREP flags */
		);


/*
*  I R _ I N I T _ I N F O
 *
*  Allocates and initializes an ir_info node for the AST node.
*/
#define IR_INIT_INFO(node_p)\
{\
	/* Create an IREP information node */\
		node_p->ir_info = NEW (IR_info_n_t);\
}


/*
*  I R _ I N I T _ N O D E
 *
*  Initializes an AST node for IREP processing.  Allocates and initializes an
*  ir_info node for the AST node.  Allocates a sentinel tuple to start the
*  node's data_tups list.  Points the current tuple pointer at the sentinel.
*/
#define IR_INIT_NODE(node_p)\
{\
	/* Create an IREP information node */\
		node_p->ir_info = NEW (IR_info_n_t);\
		\
		/* Create a sentinel tuple to simplify list manipulation */\
		node_p->data_tups = NEW (IR_tup_n_t);\
		node_p->data_tups->opcode = IR_op_noop_k;\
		node_p->data_tups->flags  = 0;\
		node_p->data_tups->next   = NULL;\
		\
		/* Save pointer to tuple in IREP info node */\
		node_p->ir_info->cur_tup_p = node_p->data_tups;\
}


/*
*  I R _ c u r _ t u p _ p
 *
*  Returns a pointer to the most recently created tuple in the current scope.
*/
static IR_tup_n_t *IR_cur_tup_p
(
 IR_scope_ctx_t      *ctx_p      /* [in] Scope context */
)
{
	IR_type_scope_t *type_s_p;

	if (ctx_p->type_scope == 0)
		return ctx_p->param_p->ir_info->cur_tup_p;

	type_s_p = &(ctx_p->type_s_a[ctx_p->type_scope]);
	if (type_s_p->flags & IR_REP_AS)
		return type_s_p->type_p->rep_as_type->ir_info->cur_tup_p;
	if (type_s_p->flags & IR_CS_CHAR)
		return type_s_p->type_p->cs_char_type->ir_info->cur_tup_p;
	return type_s_p->type_p->ir_info->cur_tup_p;
}


/*
*  I R _ i n s e r t _ i r e p _ t u p
 *
*  Inserts a tuple after the insertion pointer and updates the insertion
*  pointer to point at the newly inserted tuple.
*/
static void IR_insert_irep_tup
(
 IR_tup_n_t      *tup_p,         /* [in] Ptr to irep tuple to insert */
 IR_tup_n_t      **p_insert_p    /* [io] Ptr to tuple to insert after */
)
{
	tup_p->next = (*p_insert_p)->next;
	(*p_insert_p)->next = tup_p;
	*p_insert_p = tup_p;
}


/*
*  I R _ g e n _ i r e p _ t u p
 *
*  Allocates and initializes a tuple.  Inserts the tuple into a list of tuples.
*  Which list to insert into is determined from the scope context data passed.
*  It can either a list hanging off the current parameter, or a list hanging
*  off a type node which is referenced by the parameter.
*/
static IR_tup_n_t *IR_gen_irep_tup  /* Returns ptr to generated tuple */
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 IR_opcode_k_t       opcode      /* [in] irep opcode */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */

	tup_p = NEW (IR_tup_n_t);
	tup_p->opcode = opcode;
	tup_p->flags  = 0;
	tup_p->next   = NULL;

	/* Insert in parameter or type data_tups */
	if (ctx_p->type_scope == 0)
		IR_insert_irep_tup(tup_p, &ctx_p->param_p->ir_info->cur_tup_p);
	else if (ctx_p->type_s_a[ctx_p->type_scope].flags & IR_REP_AS)
		IR_insert_irep_tup(tup_p, &ctx_p->type_s_a[ctx_p->type_scope].type_p->
				rep_as_type->ir_info->cur_tup_p);
	else if (ctx_p->type_s_a[ctx_p->type_scope].flags & IR_CS_CHAR)
		IR_insert_irep_tup(tup_p, &ctx_p->type_s_a[ctx_p->type_scope].type_p->
				cs_char_type->ir_info->cur_tup_p);
	else
		IR_insert_irep_tup(tup_p, &ctx_p->type_s_a[ctx_p->type_scope].type_p->
				ir_info->cur_tup_p);
	return tup_p;
}


/*
*  I R _ f r e e _ i r e p _ t u p
 *
*  Frees a tuple.  Optionally unlinks the tuple from a singly linked list.
*  If the tuple being freed was the current tuple pointer in the scope
*  context block, the block is optionally updated to the predecessor tuple.
*  Use with care - i.e. no checks are done to make sure that there are no
*  additional references to the freed tuple hanging around.
*/
static void IR_free_irep_tup
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 IR_tup_n_t          *tup_p,     /* [in] Ptr to irep tuple to free */
 IR_tup_n_t          *pred_tup_p /* [io] Non-NULL => Ptr to predecessor */
 /* irep tuple; used to unlink from list  */
)
{
	if (pred_tup_p != NULL)
	{
		if (pred_tup_p->next != tup_p)
			INTERNAL_ERROR("Inconsistency in linked list");
		pred_tup_p->next = tup_p->next;

		/*
		 * If the tuple being freed was the current tuple pointer, update
		 * the current tuple pointer to point to the predecessor.
		 */
		if (ctx_p->type_scope == 0)
		{
			if (ctx_p->param_p->ir_info->cur_tup_p == tup_p)
				ctx_p->param_p->ir_info->cur_tup_p = pred_tup_p;
		}
		else if (ctx_p->type_s_a[ctx_p->type_scope].flags & IR_REP_AS)
		{
			if (ctx_p->type_s_a[ctx_p->type_scope].type_p->
					rep_as_type->ir_info->cur_tup_p == tup_p)
				ctx_p->type_s_a[ctx_p->type_scope].type_p->
					rep_as_type->ir_info->cur_tup_p = pred_tup_p;
		}
		else if (ctx_p->type_s_a[ctx_p->type_scope].flags & IR_CS_CHAR)
		{
			if (ctx_p->type_s_a[ctx_p->type_scope].type_p->
					cs_char_type->ir_info->cur_tup_p == tup_p)
				ctx_p->type_s_a[ctx_p->type_scope].type_p->
					cs_char_type->ir_info->cur_tup_p = pred_tup_p;
		}
		else
		{
			if (ctx_p->type_s_a[ctx_p->type_scope].type_p->
					ir_info->cur_tup_p == tup_p)
				ctx_p->type_s_a[ctx_p->type_scope].type_p->
					ir_info->cur_tup_p = pred_tup_p;
		}
	}
	FREE(tup_p);
}


/*
*  I R _ g e n _ a l i g n m e n t
*  Generates an alignment tuple if the passed alignment value is more than 1.
*  Returns tuple address if tuple was generated, NULL otherwise.
*/
static IR_tup_n_t *IR_gen_alignment /* Returns tuple ptr or NULL */
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 int                 alignment   /* [in] Required alignment */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */

	if (alignment <= 1)
		tup_p = NULL;
	else
	{
		tup_p = IR_gen_irep_tup(ctx_p, IR_op_align_k);
		tup_p->arg[IR_ARG_INT].int_val = alignment;
	}
	return tup_p;
}


/*
*  I R _ p a r a m _ n u m
 *
*  Returns the parameter number of a parameter.  Operation parameters are
*  numbered starting with 1; the function result is parameter 0.
*/
static unsigned long IR_param_num   /* Returns parameter number */
(
 AST_parameter_n_t   *lookup_p   /* [in] Ptr to AST parameter node */
)
{
	AST_operation_n_t   *oper_p;    /* Ptr to AST operation node */
	AST_parameter_n_t   *param_p;   /* Ptr to a parameter node to test */
	unsigned long       param_num;  /* Parameter number */

	oper_p = lookup_p->uplink;
	param_num = 0;

	for (param_p = oper_p->parameters; param_p != NULL; param_p = param_p->next)
	{
		param_num++;
		if (param_p == lookup_p)
			return param_num;
	}

	if (param_p == oper_p->result)
		return 0;

	INTERNAL_ERROR("Parameter not found in operation parameter list");
	return 0;
}


/*
*  I R _ f i e l d _ n u m
 *
*  Returns the field number of a field.  Structure fields in a non-nested
*  structure are numbered starting with 1.  Nested structure fields are
*  numbered consecutively starting with the number of the last field before
*  the nested structure plus one, or 1 if the first field.
*/
static unsigned long IR_field_num   /* Returns field number */
(
 IR_scope_ctx_t  *ctx_p ATTRIBUTE_UNUSED,         /* [in] Scope context */
 AST_field_n_t   *attr_field_p,  /* [in] Field with a field attribute */
 AST_field_n_t   *lookup_p       /* [in] Field referenced by attribute */
)
{
	AST_field_n_t       *field_p;   /* Ptr to a field node to test */
	unsigned long       field_num;  /* Field number */

	/*
	 * First scan forward from the current field for the reference.  This
	 * assures that if there are multiple instances of the same substruct,
	 * we assign the field number that goes with the current instance.
	 */
	field_num = attr_field_p->ir_info->id_num;
	for (field_p = attr_field_p->next; field_p != NULL; field_p = field_p->next)
	{
		if (field_p->ir_info == NULL)
			IR_INIT_INFO(field_p);

		field_num++;
		field_p->ir_info->id_num = field_num;
		if (field_p == lookup_p)
			return field_num;
	}

	/*
	 * Not found forward in the struct so must be a backward reference.  It
	 * therefore follows that a field number has already been assigned to the
	 * referenced field so we can just pick up the field number.
	 */
	return lookup_p->ir_info->id_num;
}


/*
*  I R _ g e n _ s t r u c t _ t y p e _ r e p
 *
*  Generates tuples for a non-nested or nested structure definition.
*  A structure definition is only done once, at the first reference, and
*  subsequent references are to the already completed definition.
*/
static void IR_gen_struct_type_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [io] AST struct type node */
 AST_instance_n_t    *inst_p,    /* [in] Ptr to AST instance node */
 unsigned long   *field_num_p,   /* [io] Starting field number */
 boolean         *has_nf_cs_array/* [io] TRUE => struct or a nested struct */
 /* has non-fixed array of [cs_char] field */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to generated IREP tuple */
	IR_tup_n_t          *cs_tup_p = NULL;  /* Ptr to codeset shadow tuple */
	IR_tup_n_t          *pred_tup_p = NULL;/* Saved tuple pointer */
	IR_tup_n_t          *info_tup_p = NULL;/* Ptr to conformance info tuple */
	IR_tup_n_t          *last_tup_p = NULL;/* Ptr to tup before last field tuples */
	AST_structure_n_t   *struct_p;  /* Ptr to AST structure node */
	AST_field_n_t       *field_p;   /* Ptr to AST field node */
	AST_field_n_t   *last_field_p = NULL;  /* Ptr to last field in struct */

	/*
	 * Generate 'struct begin' tuple and maintain scope context.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_struct_begin_k);
	tup_p->arg[IR_ARG_EXPR].expr = NULL;
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	tup_p->arg[IR_ARG_INST].inst = inst_p;
	IR_process_tup(ctx_p, tup_p);

	/*
	 * Generate 'conformance info' tuple if outermost conformant struct
	 * and set conformant flag on 'struct begin' tuple.
	 */
	if (AST_CONFORMANT_SET(type_p) && IR_in_struct(ctx_p) == 1)
	{
		tup_p->flags |= IR_CONFORMANT;
		tup_p = info_tup_p = IR_gen_irep_tup(ctx_p, IR_op_conformant_info_k);
	}

	/*
	 * If non-nested struct, generate a placeholder tuple in case the struct
	 * or any nested struct has a non-fixed array of [cs_char].
	 */
	if (IR_in_struct(ctx_p) == 1)
	{
		pred_tup_p = tup_p;
		cs_tup_p = IR_gen_irep_tup(ctx_p, IR_op_noop_k);
	}
	if (FE_TEST(type_p->fe_info->flags, FE_HAS_NF_CS_ARRAY))
		*has_nf_cs_array = TRUE;

	/*
	 * If non-nested struct, generate alignment tuple if necessary since
	 * the structure definition can be referenced at any alignment.
	 */
	if (!AST_UNALIGN_SET(type_p) && IR_in_struct(ctx_p) == 1)
		tup_p = IR_gen_alignment(ctx_p, type_p->alignment_size);

	/*
	 * Process each structure field.
	 */
	struct_p = type_p->type_structure.structure;

	for (field_p = struct_p->fields; field_p != NULL; field_p = field_p->next)
	{
		/*
		 * If not done yet, set up irep private info for field.  A field number
		 * is stored here for fields that are not themselves structures.
		 */
		if (field_p->ir_info == NULL)
			IR_INIT_INFO(field_p);
		/*
		 * Save field node address and predecessor of field's first tuple so
		 * that if the struct is conformant we can later locate information
		 * to patch the 'conformance info' tuple created above.
		 */
		last_field_p = field_p;
		last_tup_p   = IR_cur_tup_p(ctx_p);

		/*
		 * Recurse to generate tuples for field data type.
		 */
		if (field_p->type->kind != AST_structure_k
				||  field_p->type->xmit_as_type != NULL
				||  field_p->type->rep_as_type != NULL
				||  field_p->type->cs_char_type != NULL)
		{
			(*field_num_p)++;
			field_p->ir_info->id_num = *field_num_p;
			IR_gen_type_rep(ctx_p, field_p->type, (AST_instance_n_t *)field_p,
					0);
		}
		else
		{
			/* First generate alignment for nested struct if needed */
			if (!AST_UNALIGN_SET(field_p->type))
				tup_p = IR_gen_alignment(ctx_p, field_p->type->alignment_size);

			/* Recurse to generate nested struct tuples */
			IR_gen_struct_type_rep(ctx_p, field_p->type,
					(AST_instance_n_t *)field_p, field_num_p, has_nf_cs_array);
		}
	}

	/*
	 * Patch 'conformance info' tuple to point at 'conformant array' tuple.
	 */
	if (AST_CONFORMANT_SET(type_p) && IR_in_struct(ctx_p) == 1)
	{
		while (last_tup_p->opcode != IR_op_conformant_array_k
				&& last_tup_p->opcode != IR_op_open_array_k)
			last_tup_p = last_tup_p->next;
		/*
		 * If array of array skip to next conformant or open array tuple so
		 * that 'conformance info' tuple points at the flat, not full, rep.
		 */
		if (last_field_p->type->type_structure.array->element_type->kind
				== AST_array_k)
		{
			last_tup_p = last_tup_p->next;
			while (last_tup_p->opcode != IR_op_conformant_array_k
					&& last_tup_p->opcode != IR_op_open_array_k)
				last_tup_p = last_tup_p->next;
		}
		info_tup_p->arg[IR_ARG_TUP].tup = last_tup_p;
	}

	/*
	 * Process the placeholder tuple created above.  If the struct or any nested
	 * struct has a non-fixed array of [cs_char], turn the tuple into a
	 * 'codeset shadow' tuple, otherwise unlink and delete the tuple.  In the
	 * former case, also create a 'release shadow' tuple before 'struct end'.
	 */
	if (IR_in_struct(ctx_p) == 1)
	{
		if (*has_nf_cs_array)
		{
			cs_tup_p->opcode = IR_op_codeset_shadow_k;
			cs_tup_p->arg[IR_ARG_INT].int_val = *field_num_p;
			tup_p = IR_gen_irep_tup(ctx_p, IR_op_release_shadow_k);
		}
		else
			IR_free_irep_tup(ctx_p, cs_tup_p, pred_tup_p);
	}

	/*
	 * Generate 'struct end' tuple and maintain scope context.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_struct_end_k);
	tup_p->arg[IR_ARG_EXPR].expr = NULL;
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	tup_p->arg[IR_ARG_INST].inst = inst_p;
	IR_process_tup(ctx_p, tup_p);
}


/*
*  I R _ g e n _ s t r u c t _ r e p
 *
*  Generates tuples to reference a non-nested structure and if the structure
*  type has not yet been processed, generates tuples for the type.
*/
static void IR_gen_struct_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [io] AST type node */
 AST_instance_n_t    *inst_p     /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to generated IREP tuple */
	unsigned long       start_num;  /* Starting field number */
	boolean         has_nf_cs_array;/* TRUE => struct or a nested struct      */
	/* has non-fixed array of [cs_char] field */

	/*
	 * If a tagged reference to a self-pointing type, use the original type node
	 * if one exists.
	 */
	if (AST_SELF_POINTER_SET(type_p) && AST_DEF_AS_TAG_SET(type_p)
			&& type_p->fe_info->original != NULL)
		type_p = type_p->fe_info->original;

	/*
	 * Generate indirect reference tuple.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_type_indirect_k);
	tup_p->arg[IR_ARG_EXPR].expr = NULL;
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	tup_p->arg[IR_ARG_INST].inst = inst_p;

	/*
	 * Generate tuples for struct type if not yet done.
	 */
	if (type_p->ir_info == NULL)
	{
		/* Initialize type IREP info. */
		IR_INIT_NODE(type_p);

		/*
		 * Maintain scope context.  This will cause subsequent tuple insertions
		 * to be into the type node's tuple list until another indirect type
		 * reference is generated or the end of this type reference.
		 */
		IR_process_tup(ctx_p, tup_p);

		/*
		 * Generate tuples for non-nested struct type.  When control returns
		 * from this routine, the indirect type scope will have been popped.
		 */
		start_num = 0;
		has_nf_cs_array = FALSE;
		IR_gen_struct_type_rep(ctx_p, type_p, inst_p, &start_num,
				&has_nf_cs_array);
	}

	/*
	 * Propagate irep type flags to parameter where appropriate.
	 */
	if (!AST_IN_SET(ctx_p->param_p)
			&& type_p->ir_info->allocate_ref
			&& !IR_under_pointer(ctx_p))
		ctx_p->param_p->ir_info->allocate_ref = TRUE;
}


/*
*  I R _ c a s e _ i n f o _ c o m p a r e
 *
*  Routine called by sort routine to compare the contants represented by two
*  case information nodes.  Returns an integer less than or greater than 0
*  according as the first argument is to be considered less than or greater
*  than the second.
 *
*  Assumption: first argument is never equal to second argument.
*/
static int IR_case_info_compare
(
 IR_case_info_n_t    *p1,        /* [io] Ptr to first case info node */
 IR_case_info_n_t    *p2         /* [io] Ptr to second case info node */
)
{
	if (p1->value < p2->value)
		return -1;
	else
		return 1;
}


/*
*  I R _ g e n _ u n i o n _ t y p e _ r e p
 *
*  Generates tuples for an encapsulated or non-encapsulated union definition.
*  A union definition is only done once, at the first reference, and
*  subsequent references are to the already completed definition.
*/
static void IR_gen_union_type_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [io] AST struct type node */
 AST_instance_n_t    *inst_p ATTRIBUTE_UNUSED     /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to generated IREP tuple */
	IR_tup_n_t          *beg_tup_p; /* Ptr to 'union begin' tuple */
	AST_disc_union_n_t  *union_p;   /* Ptr to AST union node */
	AST_arm_n_t         *arm_p;     /* Ptr to AST arm node */
	AST_case_label_n_t  *label_p;   /* Ptr to AST case label node */
	AST_constant_n_t    *value_p;   /* Ptr to AST const node for case label */
	AST_arm_n_t         *default_p; /* Ptr to default arm for union */
	IR_case_info_n_t    *array_p;   /* Ptr to array of union case info */
	IR_case_info_n_t  *beg_array_p; /* Ptr to start of case info array */
	IR_case_info_n_t  *end_array_p; /* Ptr to end of case info array */
	long                num_arms;   /* Number of non-default arms in union */

	union_p = type_p->type_structure.disc_union;

	/*
	 * Generate 'union begin' tuple and maintain scope context.
	 */
	beg_tup_p = IR_gen_irep_tup(ctx_p, IR_op_disc_union_begin_k);
	beg_tup_p->arg[IR_ARG_TYPE].type = type_p;
	/* Set 'encapsulated' flag for encapsulated union */
	if (union_p->discrim_name != NAMETABLE_NIL_ID)
		beg_tup_p->flags |= IR_ENCAPSULATED;
	/* Maintain scope */
	IR_process_tup(ctx_p, beg_tup_p);

	/*
	 * For encapsulated union, generate 'encapsulated switch' tuple.
	 */
	if (beg_tup_p->flags & IR_ENCAPSULATED)
	{
		tup_p = IR_gen_irep_tup(ctx_p, IR_op_switch_enc_k);
		tup_p->arg[IR_ARG_NAME].name = union_p->discrim_name;
		tup_p->arg[IR_ARG_TYPE].type = union_p->discrim_type;
	}

	/*
	 * Make a first pass through the union arms to count the number of
	 * non-default arms and place the result in the 'union begin' tuple.
	 */
	num_arms = 0;
	default_p = NULL;
	for (arm_p = union_p->arms; arm_p != NULL; arm_p = arm_p->next)
	{
		/* Count a separate arm for each case label constant. */
		for (label_p = arm_p->labels; label_p != NULL; label_p = label_p->next)
		{
			if (label_p->default_label)
				default_p = arm_p;
			else
				num_arms++;
		}
	}
	beg_tup_p->arg[IR_ARG_INT].int_val = num_arms;

	/*
	 * Allocate an array to hold arm information.  Make a second pass through
	 * union arms to load this array.
	 */
	if (num_arms != 0)
	{
		array_p = NEW_VEC (IR_case_info_n_t, num_arms);
		beg_array_p = array_p;
		end_array_p = array_p;
		for (arm_p = union_p->arms; arm_p != NULL; arm_p = arm_p->next)
		{
			for (label_p = arm_p->labels; label_p != NULL; label_p = label_p->next)
			{
				if (label_p->default_label)
					continue;

				value_p = label_p->value;
				switch (value_p->kind)
				{
					case AST_int_const_k:
						end_array_p->value = (unsigned long)value_p->value.int_val;
						break;
					case AST_char_const_k:
						end_array_p->value = (unsigned long)value_p->value.char_val;
						break;
					case AST_boolean_const_k:
						end_array_p->value = (unsigned long)value_p->value.boolean_val;
						break;
					default:
						INTERNAL_ERROR("Unsupported case label constant kind");
				}
				end_array_p->arm_p   = arm_p;
				end_array_p->label_p = label_p;
				end_array_p++;
			}
		}
		/* Sort the array in ascending order of case value. */
		qsort(array_p, num_arms, sizeof(IR_case_info_n_t),
#if defined(PROTO) && !(defined(mips) && defined(ultrix))
				(int (*)(const void *, const void *))
#endif
				IR_case_info_compare);

		/*
		 * Process the sorted array of case information.
		 */
		while (array_p < end_array_p)
		{
			/* Generate 'case' tuple */
			tup_p = IR_gen_irep_tup(ctx_p, IR_op_case_k);
			if (union_p->discrim_type->kind == AST_boolean_k)
				tup_p->flags |= IR_BOOLEAN;
			tup_p->arg[IR_ARG_INT].int_val = array_p->value;

			/* If not empty arm, recurse to generate tuples for arm data type */
			if (array_p->arm_p->type == NULL)
				tup_p->flags |= IR_VOID;
			else
			{
				IR_gen_type_rep(ctx_p, array_p->arm_p->type,
						(AST_instance_n_t *)array_p->arm_p, 0);
			}
			array_p++;
		}
		FREE(beg_array_p);
	}

	/*
	 * Generate 'default case' tuple and recurse to gen tuples for default type.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_default_k);
	if (default_p == NULL)
		tup_p->arg[IR_ARG_TYPE].type = NULL;
	else
	{
		tup_p->arg[IR_ARG_TYPE].type = default_p->type;
		if (default_p->type == NULL)
			tup_p->flags |= IR_VOID;
		else
		{
			IR_gen_type_rep(ctx_p, default_p->type,
					(AST_instance_n_t *)default_p, 0);
		}
	}

	/*
	 * Generate 'union end' tuple and maintain scope context.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_disc_union_end_k);
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	tup_p->arg[IR_ARG_INT].int_val = num_arms;
	tup_p->flags = beg_tup_p->flags;
	IR_process_tup(ctx_p, tup_p);
}


/*
*  I R _ g e n _ d i s c _ u n i o n _ r e p
 *
*  Generates tuples to reference a discriminated union and if the union
*  type has not yet been processed, generates tuples for the type.
*/
static void IR_gen_disc_union_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [io] AST type node */
 AST_instance_n_t    *inst_p     /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to generated IREP tuple */
	AST_field_attr_n_t  *fattr_p;   /* Ptr to AST field attributes node */

	/*
	 * If a tagged reference to a self-pointing type, use the original type node
	 * if one exists.
	 */
	if (AST_SELF_POINTER_SET(type_p) && AST_DEF_AS_TAG_SET(type_p)
			&& type_p->fe_info->original != NULL)
		type_p = type_p->fe_info->original;

	/*
	 * If union is non-encapsulated, generate a tuple for switch information.
	 */
	if (type_p->type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID)
	{
		fattr_p = inst_p->field_attrs;
		tup_p = IR_gen_irep_tup(ctx_p, IR_op_switch_n_e_k);
		if (IR_under_struct(ctx_p))
		{
			tup_p->arg[IR_ARG_FIELD].field = fattr_p->switch_is->ref.f_ref;
			tup_p->arg[IR_ARG_PFNUM].int_val = IR_field_num(ctx_p,
					(AST_field_n_t *)inst_p, tup_p->arg[IR_ARG_FIELD].field);
		}
		else
		{
			tup_p->arg[IR_ARG_PARAM].param = fattr_p->switch_is->ref.p_ref;
			tup_p->arg[IR_ARG_PFNUM].int_val =
				IR_param_num(tup_p->arg[IR_ARG_PARAM].param);
		}
	}

	/*
	 * Generate indirect reference tuple.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_type_indirect_k);
	tup_p->arg[IR_ARG_EXPR].expr = NULL;
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	tup_p->arg[IR_ARG_INST].inst = inst_p;

	/*
	 * Generate tuples for union type if not yet done.
	 */
	if (type_p->ir_info == NULL)
	{
		/* Initialize type IREP info. */
		IR_INIT_NODE(type_p);

		/*
		 * Maintain scope context.  This will cause subsequent tuple insertions
		 * to be into the type node's tuple list until another indirect type
		 * reference is generated or the end of this type reference.
		 */
		IR_process_tup(ctx_p, tup_p);

		/*
		 * Generate tuples for union type.  When control returns
		 * from this routine, the indirect type scope will have been popped.
		 */
		IR_gen_union_type_rep(ctx_p, type_p, inst_p);
	}
}

/*
*  I R _ g e n _ i n t e r f a c e _ r e p
 *
*  Generates an IR_op_interface_k tuple.
*/
static void IR_gen_interface_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [io] AST type node */
 AST_instance_n_t    *inst_p     /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to generated IREP tuple */

	tup_p = IR_gen_irep_tup(ctx_p, IR_op_interface_k);
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	tup_p->arg[IR_ARG_INST].inst = inst_p;
	tup_p->arg[IR_ARG_INTFC].intfc = type_p->type_structure.interface;
}

/*
*  I R _ g e n _ p t r _ t u p
 *
*  Generates an IR_op_*_ptr_k or IR_op_passed_by_ref_k tuple.
*/
static IR_tup_n_t *IR_gen_ptr_tup   /* Returns ptr to generated tuple */
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [in] Ptr to AST type node */
 AST_instance_n_t    *inst_p     /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */

	if (   (inst_p != NULL && AST_UNIQUE_SET(inst_p))
			|| (inst_p == NULL && AST_UNIQUE_SET(type_p)) )
	{
		tup_p = IR_gen_irep_tup(ctx_p, IR_op_unique_ptr_k);
	}
	else if (   (inst_p != NULL && AST_PTR_SET(inst_p))
			|| (inst_p == NULL && AST_PTR_SET(type_p)) )
	{
		tup_p = IR_gen_irep_tup(ctx_p, IR_op_full_ptr_k);
	}
	else if (IR_cur_scope(ctx_p) != IR_SCP_TOPLEVEL)
	{
		tup_p = IR_gen_irep_tup(ctx_p, IR_op_ref_ptr_k);
		/*
		 * [ref] pointers that are not under a full or unique pointer in an
		 * [out]-only parameter require preallocation on server side - set flag.
		 */
		if (ctx_p->type_scope == 0)
		{
			/*
			 * [ref] pointer in non-indirect scope - set 'allocate ref' flag
			 * on parameter if it is [out]-only unless we are under any
			 * non-passed-by-ref pointer.
			 */
			if (!AST_IN_SET(ctx_p->param_p) && !IR_under_pointer(ctx_p))
				ctx_p->param_p->ir_info->allocate_ref = TRUE;
		}
		else
		{
			/*
			 * [ref] pointer in an indirect type scope (must be struct type) -
			 * set 'allocate ref' flag on type.  This must be done irrespective
			 * of whether the current parameter is [out]-only or whether we are
			 * under any non-passed-by-ref pointer, since, for example, a
			 * subsequent [out]-only parameter can reference this same type
			 * which is only flattened once.  When this type is referenced the
			 * flag will be propagated to the parameter if the appropriate
			 * conditions are met.
			 */
			ctx_p->type_s_a[ctx_p->type_scope].type_p->ir_info->allocate_ref
				= TRUE;
		}
	}
	else
		tup_p = IR_gen_irep_tup(ctx_p, IR_op_passed_by_ref_k);

	/*
	 * Conditionally set IR_STRING and/or IR_ARRAYIFIED_PTR flags.
	 */
	if (IR_STRING_ARRAY(type_p, inst_p) || IR_STRINGIFIED(type_p, inst_p))
		tup_p->flags |= IR_STRING;
	if (IR_ARRAYIFIED(type_p, inst_p))
		tup_p->flags |= IR_ARRAYIFIED_PTR;

	tup_p->arg[IR_ARG_EXPR].expr = NULL;
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	tup_p->arg[IR_ARG_INST].inst = inst_p;
	return tup_p;
}


/*
*  I R _ g e n _ a r r a y _ t u p
 *
*  Generates an IR_op_*_array_k tuple.
*/
static IR_tup_n_t *IR_gen_array_tup /* Returns ptr to generated tuple */
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [in] Ptr to AST type node */
 AST_instance_n_t    *inst_p,    /* [in] Ptr to AST instance node */
 IR_flags_t          flags       /* [in] IREP flags: IR_STRING set iff     */
 /*     object is a [string] array         */
 /*     IR_CS_CHAR iff base type [cs_char] */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */
	boolean conformant, varying, string;

	conformant  = (AST_CONFORMANT_SET(type_p) != 0);
	string      = ((flags & IR_STRING) != 0);
	varying     = (string || (inst_p != NULL && AST_VARYING_SET(inst_p)));

	if (conformant)
	{
		if (varying)
			tup_p = IR_gen_irep_tup(ctx_p, IR_op_open_array_k);
		else
			tup_p = IR_gen_irep_tup(ctx_p, IR_op_conformant_array_k);
	}
	else
	{
		if (varying)
			tup_p = IR_gen_irep_tup(ctx_p, IR_op_varying_array_k);
		else
			tup_p = IR_gen_irep_tup(ctx_p, IR_op_fixed_array_k);
	}

	tup_p->flags |= (flags & (IR_CS_CHAR|IR_STRING)); /* OR in certain flags */
	tup_p->arg[IR_ARG_EXPR].expr = NULL;
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	tup_p->arg[IR_ARG_INST].inst = inst_p;
	return tup_p;
}

/*
 *  I R _ b o u n d _ e a r l y
 *
 *  Determine whether attribute field occurs before instance, so that
 *  we can enable in-line correlation checking
 */
static boolean IR_bound_early
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_instance_n_t    *inst_p,    /* [in] Ptr to AST instance node */
 unsigned long       attribute_index /* [in] index of field attribute */
)
{
	unsigned long pf_index;

	if (IR_under_struct(ctx_p))
	{
		pf_index = IR_field_num(ctx_p, (AST_field_n_t *)inst_p,
				(AST_field_n_t *)inst_p);
	}
	else
	{
		pf_index = IR_param_num((AST_parameter_n_t *)inst_p);
	}

	return (attribute_index < pf_index);	
}

/*
 *  I R _ g e n _ b o u n d _ t u p s
 *
 *  Generates a sequence of IR_op_bound_k tuples to describe the bounds
 *  of an array.
 */
static IR_tup_n_t *IR_gen_bound_tups    /* Returns ptr to last tuple gen'd */
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [in] Ptr to AST array type node */
 AST_instance_n_t    *inst_p,    /* [in] Ptr to AST instance node */
 IR_flags_t          flags       /* [in] IREP flags: IR_STRING set iff     */
 /*     object is a [string] array         */
 /*     IR_CS_CHAR iff base type [cs_char] */
)
{
	IR_tup_n_t          *tup_p = NULL;     /* Ptr to irep tuple and args */
	IR_tup_n_t          *lower_tup_p = NULL;
	AST_array_n_t       *array_p = NULL;   /* Ptr to AST array node */
	AST_array_index_n_t *index_p;   /* Ptr to AST array index node */
	AST_constant_n_t    *const_p;   /* Ptr to AST constant node */
	AST_field_attr_n_t  *fattr_p;   /* Ptr to AST field attributes node */
	int i;
	boolean string;

	string = ((flags & IR_STRING) != 0);
	array_p = type_p->type_structure.array;
	fattr_p = (inst_p == NULL) ? NULL : inst_p->field_attrs;

	/*
	 * For each dimension in array:
	 */
	for (i = 0; i < array_p->index_count; i++)
	{
		index_p = &array_p->index_vec[i];

		/*
		 * Generate IR_op_bound_k for lower bound.  Note: AST array_rep_type for
		 * arrayified ptr can have constant lower bound even if an instance of
		 * it has a min_is attribute, so must test for min_is attribute first.
		 */
		const_p = index_p->lower_bound;
		lower_tup_p = tup_p = IR_gen_irep_tup(ctx_p, IR_op_bound_k);

		if (fattr_p != NULL &&
				fattr_p->min_is_vec != NULL && fattr_p->min_is_vec[i].valid)
		{
			if (fattr_p->min_is_vec[i].constant)
			{
				tup_p->arg[IR_ARG_BOUND].bound_k = IR_bnd_fixed_k;
				tup_p->arg[IR_ARG_INT].int_val   = fattr_p->min_is_vec[i].ref.integer;
			}
			else
			{
				/* Pick up the referenced [min_is] field/parameter */
				tup_p->arg[IR_ARG_BOUND].bound_k = IR_bnd_min_is_k;
				if (IR_under_struct(ctx_p))
				{
					tup_p->arg[IR_ARG_FIELD].field = fattr_p->min_is_vec[i].ref.f_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val = IR_field_num(ctx_p,
							(AST_field_n_t *)inst_p, tup_p->arg[IR_ARG_FIELD].field);
				}
				else
				{
					tup_p->arg[IR_ARG_PARAM].param = fattr_p->min_is_vec[i].ref.p_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val =
						IR_param_num(tup_p->arg[IR_ARG_PARAM].param);
				}
				/* Set the early correlation flag if referenced field occurs first */
				if (IR_bound_early(ctx_p, inst_p, tup_p->arg[IR_ARG_PFNUM].int_val))
					tup_p->flags |= IR_CF_EARLY;
			}
		}
		else
		{
			/* Constant lower bound */
			tup_p->arg[IR_ARG_BOUND].bound_k = IR_bnd_fixed_k;
			tup_p->arg[IR_ARG_INT].int_val   = const_p->value.int_val;
		}

		/*
		 * Generate IR_op_bound_k for upper bound.  Note: AST array_rep_type for
		 * arrayified ptr always has a dynamic upper bound by definition of
		 * arrayifying, so no need to test for dynamic bound before fixed bound
		 * as with the lower bound above.
		 */
		const_p = index_p->upper_bound;
		tup_p = IR_gen_irep_tup(ctx_p, IR_op_bound_k);

		if (const_p != NULL)
		{
			/* Constant upper bound */
			tup_p->arg[IR_ARG_BOUND].bound_k = IR_bnd_fixed_k;
			tup_p->arg[IR_ARG_INT].int_val   = const_p->value.int_val;
		}
		else if (fattr_p != NULL &&
				fattr_p->max_is_vec != NULL && fattr_p->max_is_vec[i].valid)
		{
			if (fattr_p->max_is_vec[i].constant)
			{
				tup_p->arg[IR_ARG_BOUND].bound_k = IR_bnd_fixed_k;
				tup_p->arg[IR_ARG_INT].int_val   = fattr_p->max_is_vec[i].ref.integer;
			}
			else
			{
				/* Pick up the referenced [max_is] field/parameter */
				tup_p->arg[IR_ARG_BOUND].bound_k = IR_bnd_max_is_k;
				if (IR_under_struct(ctx_p))
				{
					tup_p->arg[IR_ARG_FIELD].field = fattr_p->max_is_vec[i].ref.f_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val = IR_field_num(ctx_p,
							(AST_field_n_t *)inst_p, tup_p->arg[IR_ARG_FIELD].field);
				}
				else
				{
					tup_p->arg[IR_ARG_PARAM].param = fattr_p->max_is_vec[i].ref.p_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val =
						IR_param_num(tup_p->arg[IR_ARG_PARAM].param);
				}
				/* Set the early correlation flag if referenced field occurs first */
				if (IR_bound_early(ctx_p, inst_p, tup_p->arg[IR_ARG_PFNUM].int_val))
					tup_p->flags |= IR_CF_EARLY;
			}
		}
		else if (fattr_p != NULL &&
				fattr_p->size_is_vec != NULL && fattr_p->size_is_vec[i].valid)
		{
			if (fattr_p->size_is_vec[i].constant)
			{
				tup_p->arg[IR_ARG_BOUND].bound_k = IR_bnd_fixed_k;
				tup_p->arg[IR_ARG_INT].int_val   = 0;
				if (lower_tup_p->arg[IR_ARG_BOUND].bound_k == IR_bnd_fixed_k)
					tup_p->arg[IR_ARG_INT].int_val += lower_tup_p->arg[IR_ARG_INT].int_val;
				tup_p->arg[IR_ARG_INT].int_val   += fattr_p->size_is_vec[i].ref.integer - 1;
			}
			else
			{
				/* Pick up the referenced [size_is] field/parameter */
				tup_p->arg[IR_ARG_BOUND].bound_k = IR_bnd_size_is_k;
				tup_p->flags |= (flags & IR_CS_CHAR); /* OR in certain flags */
				if (IR_under_struct(ctx_p))
				{
					tup_p->arg[IR_ARG_FIELD].field = fattr_p->size_is_vec[i].ref.f_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val = IR_field_num(ctx_p,
							(AST_field_n_t *)inst_p, tup_p->arg[IR_ARG_FIELD].field);
					
				}
				else
				{
					tup_p->arg[IR_ARG_PARAM].param = fattr_p->size_is_vec[i].ref.p_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val =
						IR_param_num(tup_p->arg[IR_ARG_PARAM].param);
				}
				tup_p->arg[IR_ARG_BOUND_XTRA].byt_val = fattr_p->size_is_vec[i].xtra_opcode;
				/* Set the early correlation flag if referenced field occurs first */
				if (IR_bound_early(ctx_p, inst_p, tup_p->arg[IR_ARG_PFNUM].int_val))
					tup_p->flags |= IR_CF_EARLY;
			}
		}
		else if (string)
		{
			/*
			 * Get here only for conformant string without an explicit max_is or
			 * size_is attribute - upper bound must contain string octet size.
			 */
			tup_p->arg[IR_ARG_BOUND].bound_k = IR_bnd_string_k;
			/*
			 * If base type of string has rep_as we want size of local type;
			 * since not known at compile time, stash type node address instead.
			 */
			if (type_p->type_structure.array->element_type->rep_as_type != NULL)
			{
				tup_p->flags |= IR_REP_AS;
				tup_p->arg[IR_ARG_TYPE2].type =
					type_p->type_structure.array->element_type;
			}
			else
			{
				/* Store octet size */
				tup_p->arg[IR_ARG_INT].int_val =
					type_p->type_structure.array->element_type->ndr_size;
			}
			/* Conformant string in conformant struct requires field number */
			if (AST_CONFORMANT_SET(type_p)
					&& IR_parent_scope(ctx_p) == IR_SCP_STRUCT)
				tup_p->arg[IR_ARG_PFNUM].int_val = IR_field_num(ctx_p,
						(AST_field_n_t *)inst_p, (AST_field_n_t *)inst_p);
		}
		else
		{
			INTERNAL_ERROR("Invalid array bounds");
		}
	}
	return tup_p;
}


/*
*  I R _ g e n _ l i m i t _ t u p s
 *
*  Generates a sequence of IR_op_limit_k tuples to describe the data limits
*  of an array.
*/
static IR_tup_n_t *IR_gen_limit_tups    /* Returns ptr to last tuple gen'd */
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [in] Ptr to AST array type node */
 AST_instance_n_t    *inst_p,    /* [in] Ptr to AST instance node */
 IR_flags_t          flags       /* [in] IREP flags: IR_STRING set iff     */
 /*     object is a [string] array         */
 /*     IR_CS_CHAR iff base type [cs_char] */
)
{
	IR_tup_n_t          *tup_p = NULL;     /* Ptr to irep tuple and args */
	IR_tup_n_t          *lower_tup_p = NULL;
	AST_array_n_t       *array_p;   /* Ptr to AST array node */
	AST_array_index_n_t *index_p;   /* Ptr to AST array index node */
	AST_constant_n_t    *const_p;   /* Ptr to AST constant node */
	AST_field_attr_n_t  *fattr_p;   /* Ptr to AST field attributes node */
	int i;
	boolean string;

	string = ((flags & IR_STRING) != 0);
	array_p = type_p->type_structure.array;
	fattr_p = (inst_p == NULL) ? NULL : inst_p->field_attrs;

#if 0
	printf("IR_gen_limit_tups: called with flags %08x string %d\n", flags, string);
#endif

	/*
	 * For each dimension in array:
	 */
	for (i = 0; i < array_p->index_count; i++)
	{
		index_p = &array_p->index_vec[i];

		/*
		 * Generate IR_op_limit_k for lower data limit.
		 */
		const_p = index_p->lower_bound;
		lower_tup_p = tup_p = IR_gen_irep_tup(ctx_p, IR_op_limit_k);

		if (fattr_p != NULL && fattr_p->first_is_vec != NULL
				&& fattr_p->first_is_vec[i].valid)
		{
			if (fattr_p->first_is_vec[i].constant)
			{
				tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_fixed_k;
				tup_p->arg[IR_ARG_INT].int_val    = fattr_p->first_is_vec[i].ref.integer;
			}
			else
			{
				/* Pick up the referenced [first_is] field/parameter */
				tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_first_is_k;
				if (IR_under_struct(ctx_p))
				{
					tup_p->arg[IR_ARG_FIELD].field = fattr_p->first_is_vec[i].ref.f_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val = IR_field_num(ctx_p,
							(AST_field_n_t *)inst_p, tup_p->arg[IR_ARG_FIELD].field);
				}
				else
				{
					tup_p->arg[IR_ARG_PARAM].param = fattr_p->first_is_vec[i].ref.p_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val =
						IR_param_num(tup_p->arg[IR_ARG_PARAM].param);
				}
				/* Set the early correlation flag if referenced field occurs first */
				if (IR_bound_early(ctx_p, inst_p, tup_p->arg[IR_ARG_PFNUM].int_val))
					tup_p->flags |= IR_CF_EARLY;
			}
		}
		/*
		 * Note: AST array_rep_type for arrayified ptr can have constant lower
		 * bound even if an instance of it has a min_is attribute, so must test
		 * for min_is attribute first.
		 */
		else if (fattr_p != NULL && fattr_p->min_is_vec != NULL
				&& fattr_p->min_is_vec[i].valid)
		{
			if (fattr_p->min_is_vec[i].constant)
			{
				tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_fixed_k;
				tup_p->arg[IR_ARG_INT].int_val   = fattr_p->first_is_vec[i].ref.integer;
			}
			else
			{
				/*
				 * No lower data limit but also not a fixed bound so dummy a
				 * first_is reference to point to the min_is variable.
				 */
				tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_first_is_k;
				if (IR_under_struct(ctx_p))
				{
					tup_p->arg[IR_ARG_FIELD].field = fattr_p->min_is_vec[i].ref.f_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val = IR_field_num(ctx_p,
							(AST_field_n_t *)inst_p, tup_p->arg[IR_ARG_FIELD].field);
				}
				else
				{
					tup_p->arg[IR_ARG_PARAM].param = fattr_p->min_is_vec[i].ref.p_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val =
						IR_param_num(tup_p->arg[IR_ARG_PARAM].param);
				}
				/* Set the early correlation flag if referenced field occurs first */
				if (IR_bound_early(ctx_p, inst_p, tup_p->arg[IR_ARG_PFNUM].int_val))
					tup_p->flags |= IR_CF_EARLY;
			}
		}
		else
		{
			/* Fixed lower data limit */
			tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_fixed_k;
			tup_p->arg[IR_ARG_INT].int_val   = const_p->value.int_val;
		}

		/*
		 * Generate IR_op_limit_k for upper data limit.
		 */
		const_p = index_p->upper_bound;
		tup_p = IR_gen_irep_tup(ctx_p, IR_op_limit_k);

		if (string)
		{
			/*
			 * Normally, the upper data limit is computed by the string length
			 * at runtime.  The upper data limit must contain string octet size.
			 * If in flat array rep of array of string, limit should have dummy
			 * value of 0 to simplify Interpreter logic for A,B pairs.
			 */
			if (ctx_p->in_flat_rep)
			{
				tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_fixed_k;
				tup_p->arg[IR_ARG_INT].int_val = 0;
			}
			else
			{
				tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_string_k;
				/*
				 * If base type of string has rep_as we want size of local type;
				 * since not known at compile time, stash type node address instead.
				 */
				if (type_p->type_structure.array->element_type->rep_as_type != NULL)
				{
					tup_p->flags |= IR_REP_AS;
					tup_p->arg[IR_ARG_TYPE2].type =
						type_p->type_structure.array->element_type;
				}
				else
				{
					/* Store octet size */
					tup_p->arg[IR_ARG_INT].int_val =
						type_p->type_structure.array->element_type->ndr_size;
				}
			}
		}
		else if (fattr_p != NULL && fattr_p->last_is_vec != NULL
				&& fattr_p->last_is_vec[i].valid)
		{
			if (fattr_p->last_is_vec[i].constant)
			{
				tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_fixed_k;
				tup_p->arg[IR_ARG_INT].int_val   = fattr_p->last_is_vec[i].ref.integer;
			}
			else
			{
				/* Pick up the referenced [last_is] field/parameter */
				tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_last_is_k;
				if (IR_under_struct(ctx_p))
				{
					tup_p->arg[IR_ARG_FIELD].field = fattr_p->last_is_vec[i].ref.f_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val = IR_field_num(ctx_p,
							(AST_field_n_t *)inst_p, tup_p->arg[IR_ARG_FIELD].field);
				}
				else
				{
					tup_p->arg[IR_ARG_PARAM].param = fattr_p->last_is_vec[i].ref.p_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val =
						IR_param_num(tup_p->arg[IR_ARG_PARAM].param);
				}
				/* Set the early correlation flag if referenced field occurs first */
				if (IR_bound_early(ctx_p, inst_p, tup_p->arg[IR_ARG_PFNUM].int_val))
					tup_p->flags |= IR_CF_EARLY;
			}
		}
		else if (fattr_p != NULL && fattr_p->length_is_vec != NULL
				&& fattr_p->length_is_vec[i].valid)
		{
			if (fattr_p->length_is_vec[i].constant)
			{
				tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_fixed_k;
				tup_p->arg[IR_ARG_INT].int_val   = 0;
				if (lower_tup_p->arg[IR_ARG_LIMIT].limit_k == IR_lim_fixed_k)
					tup_p->arg[IR_ARG_INT].int_val += lower_tup_p->arg[IR_ARG_INT].int_val;
				tup_p->arg[IR_ARG_INT].int_val  += fattr_p->length_is_vec[i].ref.integer - 1;
			}
			else
			{
				/* Pick up the referenced [length_is] field/parameter */
				tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_length_is_k;
				tup_p->flags |= (flags & IR_CS_CHAR); /* OR in certain flags */
				if (IR_under_struct(ctx_p))
				{
					tup_p->arg[IR_ARG_FIELD].field = fattr_p->length_is_vec[i].ref.f_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val = IR_field_num(ctx_p,
							(AST_field_n_t *)inst_p, tup_p->arg[IR_ARG_FIELD].field);
				}
				else
				{
					tup_p->arg[IR_ARG_PARAM].param = fattr_p->length_is_vec[i].ref.p_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val =
						IR_param_num(tup_p->arg[IR_ARG_PARAM].param);
				}
				tup_p->arg[IR_ARG_BOUND_XTRA].byt_val = fattr_p->length_is_vec[i].xtra_opcode;
				/* Set the early correlation flag if referenced field occurs first */
				if (IR_bound_early(ctx_p, inst_p, tup_p->arg[IR_ARG_PFNUM].int_val))
					tup_p->flags |= IR_CF_EARLY;
			}

		}
		else if (const_p == NULL && fattr_p->max_is_vec != NULL
				&& fattr_p->max_is_vec[i].valid)
		{
			if (fattr_p->max_is_vec[i].constant)
			{
				tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_fixed_k;
				tup_p->arg[IR_ARG_INT].int_val   = fattr_p->max_is_vec[i].ref.integer;
			}
			else
			{
				/*
				 * No upper data limit but also not a fixed bound so dummy a
				 * last_is reference to point to the max_is variable.
				 */
				tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_last_is_k;
				if (IR_under_struct(ctx_p))
				{
					tup_p->arg[IR_ARG_FIELD].field = fattr_p->max_is_vec[i].ref.f_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val = IR_field_num(ctx_p,
							(AST_field_n_t *)inst_p, tup_p->arg[IR_ARG_FIELD].field);
				}
				else
				{
					tup_p->arg[IR_ARG_PARAM].param = fattr_p->max_is_vec[i].ref.p_ref;
					tup_p->arg[IR_ARG_PFNUM].int_val =
						IR_param_num(tup_p->arg[IR_ARG_PARAM].param);
				}
				/* Set the early correlation flag if referenced field occurs first */
				if (IR_bound_early(ctx_p, inst_p, tup_p->arg[IR_ARG_PFNUM].int_val))
					tup_p->flags |= IR_CF_EARLY;
			}
		}
		else if (const_p == NULL && fattr_p->size_is_vec != NULL
				&& fattr_p->size_is_vec[i].valid)
		{
			/*
			 * No upper data limit and upper bound is a size_is reference.
			 * Generate tuple that says upper data limit must be computed.
			 */
			tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_upper_conf_k;
		}
		else
		{
			/* Fixed upper data limit */
			tup_p->arg[IR_ARG_LIMIT].limit_k = IR_lim_fixed_k;
			tup_p->arg[IR_ARG_INT].int_val   = const_p->value.int_val;
		}
	}
	return tup_p;
}


/*
*  I R _ g e n _ f l a t _ a r r a y _ r e p
 *
*  Generates tuples for the flattened (multidimensional array) representation
*  of an array of array [of array]... reference.
*/
static void IR_gen_flat_array_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [in] Ptr to AST type node */
 AST_instance_n_t    *inst_p     /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */
	AST_type_n_t        *atype_p;   /* Ptr to an array type */
	AST_type_n_t        *btype_p;   /* Ptr to non-array base type */
	AST_array_n_t       *array_p;   /* Ptr to AST array node */
	unsigned short      dim;        /* Total number of dimensions */

	dim = 0;

	/*
	 * Compute the total number of dimensions and the non-array base type.
	 * Array of string array: The string array is considered the base type but
	 * the string dimension is still included in the bound (or limit) tuples.
	 */
	for (atype_p = type_p;
			atype_p->kind == AST_array_k
			&& !AST_STRING_SET(atype_p) && !AST_STRING0_SET(atype_p);
			atype_p = atype_p->type_structure.array->element_type)
	{
		array_p = atype_p->type_structure.array;
		dim += array_p->index_count;
	}
	btype_p = atype_p;
	if (btype_p->kind == AST_array_k)
		dim++;

	/*
	 * Generate IR_op_*_array_k tuple and maintain scope context.
	 */
	ctx_p->in_flat_rep = TRUE;
	tup_p = IR_gen_array_tup(ctx_p, type_p, inst_p, 0);
	IR_process_tup(ctx_p, tup_p);

	/*
	 * Generate IR_op_array_bounds_k.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_array_bounds_k);
	/*** TBS: IR_ARG_TUP argument points to helper variable tuples ***/
	tup_p->arg[IR_ARG_TUP].tup     = NULL;
	tup_p->arg[IR_ARG_TYPE].type   = type_p;
	tup_p->arg[IR_ARG_INT].int_val = dim;

	/*
	 * Generate IR_op_bound_k pair for each dimension.
	 * If array is varying, generate IR_op_limit_k pair for each dimension.
	 */
	for (atype_p = type_p;
			atype_p->kind == AST_array_k;
			atype_p = atype_p->type_structure.array->element_type)
	{
		/*
		 * After toplevel array, null instance ptr since its attributes
		 * can only apply to the toplevel array.
		 */
		IR_gen_bound_tups(ctx_p, atype_p, (atype_p == type_p) ? inst_p : NULL,
				(AST_STRING_SET(atype_p) || AST_STRING0_SET(atype_p)) ?
				IR_STRING : 0);
	}

	if (inst_p != NULL && AST_VARYING_SET(inst_p))
	{
		for (atype_p = type_p;
				atype_p->kind == AST_array_k;
				atype_p = atype_p->type_structure.array->element_type)
		{
			/*
			 * After toplevel array, null instance ptr since its attributes
			 * can only apply to the toplevel array.
			 */
			IR_flags_t flags;

			if (AST_STRING_SET(atype_p) || AST_STRING0_SET(atype_p))
				flags = IR_STRING;
			else
				flags = 0;
			IR_gen_limit_tups(ctx_p, atype_p,
					(atype_p == type_p) ? inst_p : NULL,
					flags);
		}
	}

	/*
	 * Recurse to generate tuples for the non-array base element type.
	 */
	ctx_p->in_flat_rep = FALSE;
	IR_gen_type_rep(ctx_p, btype_p, NULL, 0);

	/*
	 * Generate IR_op_array_end_k tuple and maintain scope context.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_array_end_k);
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	IR_process_tup(ctx_p, tup_p);
}


/*
*  I R _ g e n _ a r r a y _ r e p
 *
*  Generates tuples for an array reference.
*/
static void IR_gen_array_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [in] Ptr to AST type node */
 AST_instance_n_t    *inst_p,    /* [in] Ptr to AST instance node */
 IR_flags_t          flags       /* [in] IREP flags: IR_STRING set iff */
 /*      object is a [string] array    */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */
	AST_array_n_t       *array_p;   /* Ptr to AST array node */
	AST_type_n_t        *btype_p;   /* Ptr to AST array base type node */
	IR_tup_n_t *full_tup_p = NULL, *flat_tup_p; /* Saved tuple ptrs */
	boolean     array_of_array;     /* TRUE => toplevel of array of array... */
	boolean     toplevel_array;     /* TRUE => array parameter */

	array_p = type_p->type_structure.array;
	btype_p = array_p->element_type;

	/*
	 * If base type is [cs_char], set flag used in array tuples.
	 */
	if (btype_p->cs_char_type != NULL)
		flags |= IR_CS_CHAR;

	/*
	 * If toplevel array, generate IR_op_passed_by_ref_k or IR_op_*_ptr_k tuple
	 * since arrays are implicitly passed by reference.
	 */
	if (IR_cur_scope(ctx_p) == IR_SCP_TOPLEVEL)
	{
		tup_p = IR_gen_ptr_tup(ctx_p, type_p, inst_p);
		IR_process_tup(ctx_p, tup_p);
		toplevel_array = TRUE;
	}
	else
		toplevel_array = FALSE;

	/*
	 * If at toplevel of an array of array, generate IR_op_full_array_k tuple.
	 */
	array_of_array = (IR_cur_scope(ctx_p) != IR_SCP_ARRAY
			&& btype_p->kind == AST_array_k);
	if (array_of_array)
		full_tup_p = IR_gen_irep_tup(ctx_p, IR_op_full_array_k);

	/*
	 * Generate IR_op_*_array_k tuple and maintain scope context.
	 */
	tup_p = IR_gen_array_tup(ctx_p, type_p, inst_p, flags);
	IR_process_tup(ctx_p, tup_p);

	/*
	 * Generate block of IR_op_declare_k helper variables into operation tups.
	 */

	/*
	 * Generate IR_op_array_bounds_k.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_array_bounds_k);
	/*** TBS: IR_ARG_TUP argument points to helper variable tuples ***/
	tup_p->arg[IR_ARG_TUP].tup     = NULL;
	tup_p->arg[IR_ARG_TYPE].type   = type_p;
	tup_p->arg[IR_ARG_INT].int_val = array_p->index_count;

	/*
	 * Generate IR_op_bound_k pair for each dimension.
	 * If array is varying, generate IR_op_limit_k pair for each dimension.
	 */
	IR_gen_bound_tups(ctx_p, type_p, inst_p, flags);

	if ((flags & IR_STRING) || (inst_p != NULL && AST_VARYING_SET(inst_p)))
		IR_gen_limit_tups(ctx_p, type_p, inst_p, flags);

	/*
	 * Recurse to generate tuples for the array element type.
	 */
	IR_gen_type_rep(ctx_p, btype_p, NULL, 0);

	/*
	 * Generate IR_op_array_end_k tuple and maintain scope context.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_array_end_k);
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	IR_process_tup(ctx_p, tup_p);

	/*
	 * If the toplevel of an array of array construct,
	 * generate the fully flattened representation.
	 */
	if (array_of_array)
	{
		/*
		 * Generate IR_op_flat_array_k tuple.
		 * Generate flattened representation of the array of array.
		 */
		flat_tup_p = IR_gen_irep_tup(ctx_p, IR_op_flat_array_k);
		IR_gen_flat_array_rep(ctx_p, type_p, inst_p);

		/*
		 * Generate IR_op_full_array_end_k.
		 * Patch IR_op_full_array_k to point at IR_op_flat_array_k.
		 * Patch IR_op_flat_array_k to point at IR_op_full_array_end_k.
		 */
		tup_p = IR_gen_irep_tup(ctx_p, IR_op_full_array_end_k);
		full_tup_p->arg[IR_ARG_TUP].tup = flat_tup_p;
		flat_tup_p->arg[IR_ARG_TUP].tup = tup_p;
	}

	/*
	 * If toplevel array, generate IR_op_pointee_end_k tuple and maintain ctx.
	 */
	if (toplevel_array)
	{
		tup_p = IR_gen_irep_tup(ctx_p, IR_op_pointee_end_k);
		tup_p->arg[IR_ARG_TYPE].type = type_p;
		IR_process_tup(ctx_p, tup_p);
	}
}


/*
*  I R _ g e n _ m u l t i d _ a o s
 *
*  Generates tuples for the special case of a multidimensional array which
*  represents an array of strings.  This is only possible using [v1_string].
*  To handle this anomolous case, create a standin array of string array
*  representation of the multidimensional array.
*/
static void IR_gen_multid_aos
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [in] Ptr to AST type node */
 AST_instance_n_t    *inst_p     /* [in] Ptr to AST instance node */
)
{
	AST_array_n_t       *array_p;       /* Ptr to AST array node */
	AST_array_index_n_t *index_p;       /* Ptr to AST array index node */
	AST_type_n_t        *new_type_p;    /* Ptr to standin array type node */
	AST_array_n_t       *new_array_p;   /* Ptr to standin array node */
	AST_array_index_n_t *new_index_p;   /* Ptr to standin array index node */
	AST_type_n_t        *base_type_p;   /* Ptr to standin base type node */
	AST_array_n_t       *base_array_p;  /* Ptr to standin base array node */
	AST_array_index_n_t *base_index_p;  /* Ptr to standin base array idx node */
	int i;

	array_p = type_p->type_structure.array;
	index_p = array_p->index_vec;

	/*
	 * From the N-dim array type with [v1_string], create an (N-1)-dim
	 * array type, without [v1_string], whose base type is array.
	 */
	new_type_p = AST_type_node(AST_array_k);
	base_type_p = AST_type_node(AST_array_k);
	new_array_p = AST_array_node(base_type_p);
	new_index_p = AST_array_index_node(array_p->index_count-1);

	new_type_p->name = type_p->name;
	new_type_p->type_structure.array = new_array_p;
	new_type_p->flags = type_p->flags;
	new_type_p->fe_info->flags = type_p->fe_info->flags;
	AST_CLR_STRING0(new_type_p);
	if (inst_p != NULL) AST_CLR_STRING0(inst_p);
	new_array_p->index_count = array_p->index_count-1;
	new_array_p->index_vec   = new_index_p;

	for (i = 1; i < array_p->index_count; i++)
	{
		new_index_p->flags       = index_p->flags;
		new_index_p->lower_bound = index_p->lower_bound;
		new_index_p->upper_bound = index_p->upper_bound;
		new_index_p++;
		index_p++;
	}
	/* index_p left pointing at index node for minor (string) dimension */

	/*
	 * Set up the array base type with the [v1_string] attribute.
	 */
	base_array_p = AST_array_node(array_p->element_type);
	base_index_p = AST_array_index_node(1);

	base_type_p->type_structure.array = base_array_p;
	AST_SET_STRING0(base_type_p);
	base_type_p->fe_info->flags = type_p->fe_info->flags;
	base_array_p->index_count = 1;
	base_array_p->index_vec   = base_index_p;
	base_index_p->flags       = index_p->flags;
	base_index_p->lower_bound = index_p->lower_bound;
	base_index_p->upper_bound = index_p->upper_bound;

	/*
	 * Now create tuples for the newly constructed array of string type.
	 */
	IR_gen_type_rep(ctx_p, new_type_p, inst_p, 0);
}


/*
*  I R _ g e n _ p o i n t e r _ r e p
 *
*  Generates tuples for a pointer/pointee reference.
*/
static void IR_gen_pointer_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [in] Ptr to AST type node */
 AST_instance_n_t    *inst_p     /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */
	AST_type_n_t    *arr_rep_type_p;/* Array rep type for arrayified pointer */
	AST_type_n_t     *ptee_type_p;  /* Pointee type */
	AST_instance_n_t *ptee_inst_p;  /* Instance ptr for pointee rep */

	/*
	 * If pointee type is handle_t without transmit_as, no wire rep so return.
	 */
	ptee_type_p = type_p->type_structure.pointer->pointee_type;
	if (ptee_type_p->kind == AST_handle_k && ptee_type_p->xmit_as_type == NULL)
		return;

	/*
	 * Generate IR_op_*_ptr_k and maintain scope context.
	 */
    
    /*
     * Centeris change:
     *   We need to pass inst_p recursively down the
     *   call chain so that we can look up the discriminator
     *   field for non-encapsulated unions; this could occur
     *   after several layers of pointer indirection.  Normally
     *   doing this would not make sense because it violates the
     *   invariant that inst_p->type == type_p; we should pass
     *   NULL for inst_p instead, but this would break unions.
     *   Because IR_gen_ptr_tup prefers using inst_p to check
     *   for attributes when possible, passing an inst_p that
     *   violates the invariant can cause it to generate bogus
     *   pointer attributes in the intermediate representation.
     *   In order to work around this, we recursively pass ourselves
     *   inst_p rather than NULL, but pass NULL to IR_gen_ptr_tup
     *   when the invariant is not met.  Hopefully this does not
     *   break anything else.
     *
     *   -- Brian
     */
     

    if (inst_p && inst_p->type == type_p)
        tup_p = IR_gen_ptr_tup(ctx_p, type_p, inst_p);
    else
        tup_p = IR_gen_ptr_tup(ctx_p, type_p, NULL);
	IR_process_tup(ctx_p, tup_p);

	/*
	 * If pointer is arrayified, generate array rep.
	 * Otherwise, generate rep of pointee type.
	 */
	if (tup_p->flags & IR_ARRAYIFIED_PTR)
	{
		/* Propagate attributes to array rep type if necessary */
		arr_rep_type_p = ptee_type_p->array_rep_type;
		if (FE_TEST(ptee_type_p->fe_info->flags, FE_HAS_PTR))
			FE_SET(arr_rep_type_p->fe_info->flags, FE_HAS_PTR);

		/*
		 * Parameter or field instance skips across pointer to array rep.
		 * NOTE: The need for the 'flags' argument to IR_gen_type_rep stems
		 * from here: The [string] attribute cannot be put on an array_rep_type
		 * since other arrayified types without [string] can have the same
		 * array_rep_type node.  If this reference to the array_rep_type is
		 * stringified, it is captured in and passed thru the tuple flags.
		 */
		IR_gen_type_rep(ctx_p, arr_rep_type_p, inst_p, tup_p->flags);
	}
	else
	{
		/*
		 * Generate rep of pointee type.  Note that if pointer is a toplevel
		 * pointer whose pointee type is not a pointer or array, the instance
		 * node address "skips across" the pointer to the pointee rep.  This
		 * is also true for ANY poiner to a non-encapsulated union, where the
		 * switch information which hangs off the instance node is needed when
		 * processing the union type.
		 */
		if ( (ptee_type_p->kind == AST_disc_union_k && ptee_type_p->
					type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID)
#if 1
/* Centeris change:
     Handle the case of two levels of indirection to a union.  This should
     be fixed to handle any number of levels.  A better solution would be to
     always pass inst_p, and add checks for the inst_p->type == type_p
     invariant everywhere else.
*/
             ||
             (ptee_type_p->kind == AST_pointer_k && 
              ptee_type_p->type_structure.pointer->pointee_type->kind ==
              AST_disc_union_k)
#endif
				||
				((ptee_type_p->kind != AST_pointer_k || ptee_type_p->
				  type_structure.pointer->pointee_type->kind == AST_void_k
				  || (ptee_type_p->type_structure.pointer->pointee_type->kind
					  == AST_structure_k && AST_CONTEXT_RD_SET(ptee_type_p)))
				 && ptee_type_p->kind != AST_array_k
				 && IR_parent_scope(ctx_p) == IR_SCP_TOPLEVEL) )
			ptee_inst_p = inst_p;
		else
			ptee_inst_p = NULL;
		IR_gen_type_rep(ctx_p, ptee_type_p, ptee_inst_p, 0);
	}

	/*
	 * Generate IR_op_pointee_end_k tuple and maintain scope context.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_pointee_end_k);
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	IR_process_tup(ctx_p, tup_p);
}


/*
*  I R _ g e n _ x m i t _ a s _ t y p e _ r e p
 *
*  Generates tuples for a [transmit_as] type definition.
*/
static void IR_gen_xmit_as_type_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [io] AST type node */
 AST_instance_n_t    *inst_p ATTRIBUTE_UNUSED    /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */

	/*
	 * Generate IR_op_transmit_as_k tuple and maintain scope context.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_transmit_as_k);
	/*** TBS: IR_ARG_TUP argument points to helper variable tuples ***/
	tup_p->arg[IR_ARG_TUP].tup    = NULL;
	tup_p->arg[IR_ARG_TYPE].type  = type_p->xmit_as_type;   /* transmissible */
	tup_p->arg[IR_ARG_TYPE2].type = type_p;                 /* presented     */

	IR_process_tup(ctx_p, tup_p);

	/*
	 * Generate part of IR_op_call_k IR_op_call_param_k... tuples.
	 */

	/*
	 * Recurse to generate tuples for the transmissible type.
	 */
	IR_gen_type_rep(ctx_p, type_p->xmit_as_type, NULL, 0);

	/*
	 * Generate rest of IR_op_call_k IR_op_call_param_k... tuples.
	 */

	/*
	 * Generate IR_op_transmit_end_k tuple and maintain scope context.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_transmit_end_k);
	/*** TBS: IR_ARG_TUP argument points to helper variable tuples ***/
	tup_p->arg[IR_ARG_TUP].tup    = NULL;
	tup_p->arg[IR_ARG_TYPE].type  = type_p->xmit_as_type;   /* transmissible */
	tup_p->arg[IR_ARG_TYPE2].type = type_p;                 /* presented     */

	IR_process_tup(ctx_p, tup_p);
}


/*
*  I R _ g e n _ x m i t _ a s _ r e p
 *
*  Generates tuples to reference a [transmit_as] type and if the
*  type has not yet been processed, generates tuples for the type.
*/
static void IR_gen_xmit_as_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [io] AST type node */
 AST_instance_n_t    *inst_p     /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */

	/*
	 * Generate block of IR_op_declare_k variables into operation init_tups.
	 */

	/*
	 * Generate indirect reference tuple.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_type_indirect_k);
	tup_p->arg[IR_ARG_EXPR].expr = NULL;
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	tup_p->arg[IR_ARG_INST].inst = inst_p;

	/*
	 * Generate tuples for [transmit_as] type if not yet done.
	 */
	if (type_p->ir_info == NULL)
	{
		/* Initialize type IREP info. */
		IR_INIT_NODE(type_p);

		/*
		 * Maintain scope context.  This will cause subsequent tuple insertions
		 * to be into the type node's tuple list until another indirect type
		 * reference is generated or the end of this type reference.
		 */
		IR_process_tup(ctx_p, tup_p);

		/*
		 * Generate tuples for [transmit_as] type.  When control returns
		 * from this routine, the indirect type scope will have been popped.
		 */
		IR_gen_xmit_as_type_rep(ctx_p, type_p, inst_p);
	}
}


/*
*  I R _ g e n _ r e p r _ a s _ t y p e _ r e p
 *
*  Generates tuples for a [represent_as] type definition.
*/
static void IR_gen_repr_as_type_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [io] AST type node */
 AST_instance_n_t    *inst_p ATTRIBUTE_UNUSED    /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */

	/*
	 * Generate IR_op_represent_as_k tuple and maintain scope context.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_represent_as_k);
	/*** TBS: IR_ARG_TUP argument points to helper variable tuples ***/
	tup_p->arg[IR_ARG_TUP].tup    = NULL;
	tup_p->arg[IR_ARG_TYPE].type  = type_p;     /* network type */
	tup_p->arg[IR_ARG_REP_AS].rep_as = type_p->rep_as_type;

	IR_process_tup(ctx_p, tup_p);

	/*
	 * Generate part of IR_op_call_k IR_op_call_param_k... tuples.
	 */

	/*
	 * Recurse to generate tuples for the network type.
	 */
	IR_gen_type_rep(ctx_p, type_p, NULL, 0);

	/*
	 * Generate rest of IR_op_call_k IR_op_call_param_k... tuples.
	 */

	/*
	 * Generate IR_op_represent_end_k tuple and maintain scope context.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_represent_end_k);
	/*** TBS: IR_ARG_TUP argument points to helper variable tuples ***/
	tup_p->arg[IR_ARG_TUP].tup    = NULL;
	tup_p->arg[IR_ARG_TYPE].type  = type_p;     /* network type */
	tup_p->arg[IR_ARG_REP_AS].rep_as = type_p->rep_as_type;

	IR_process_tup(ctx_p, tup_p);
}


/*
*  I R _ g e n _ r e p r _ a s _ r e p
 *
*  Generates tuples for a [represent_as] type.
*/
static void IR_gen_repr_as_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [io] AST type node */
 AST_instance_n_t    *inst_p     /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */
	AST_rep_as_n_t      *rep_p;     /* Ptr to AST represent_as node */

	rep_p = type_p->rep_as_type;

	/*
	 * Generate block of IR_op_declare_k variables into operation init_tups.
	 */

	/*
	 * Generate indirect reference tuple.  The flag indicates that the tuples
	 * will hang off the rep_as node instead of the type node itself.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_type_indirect_k);
	tup_p->flags |= IR_REP_AS;
	tup_p->arg[IR_ARG_EXPR].expr = NULL;
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	tup_p->arg[IR_ARG_INST].inst = inst_p;

	/*
	 * Generate tuples for [represent_as] type if not yet done.
	 */
	if (rep_p->ir_info == NULL)
	{
		/* Initialize type IREP info. */
		IR_INIT_NODE(rep_p);

		/*
		 * Maintain scope context.  This will cause subsequent tuple insertions
		 * to be into the rep_as node's tuple list until another indirect type
		 * reference is generated or the end of this reference.
		 */
		IR_process_tup(ctx_p, tup_p);

		/*
		 * Generate tuples for [represent_as] type.  When control returns
		 * from this routine, the indirect type scope will have been popped.
		 */
		IR_gen_repr_as_type_rep(ctx_p, type_p, inst_p);
	}
}


/*
*  I R _ g e n _ c s _ c h a r _ t y p e _ r e p
 *
*  Generates tuples for a [cs_char] type definition.
*/
static void IR_gen_cs_char_type_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [io] AST type node */
 AST_instance_n_t    *inst_p ATTRIBUTE_UNUSED    /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */

	/*
	 * Generate IR_op_cs_char_k tuple and maintain scope context.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_cs_char_k);
	tup_p->arg[IR_ARG_TUP].tup    = NULL;       /* currently not used */
	tup_p->arg[IR_ARG_TYPE].type  = type_p;     /* network type */
	tup_p->arg[IR_ARG_CS_CHAR].cs_char = type_p->cs_char_type;

	IR_process_tup(ctx_p, tup_p);

	/*
	 * Recurse to generate tuples for the network type.
	 */
	IR_gen_type_rep(ctx_p, type_p, NULL, 0);

	/*
	 * Generate IR_op_cs_char_end_k tuple and maintain scope context.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_cs_char_end_k);
	tup_p->arg[IR_ARG_TUP].tup    = NULL;       /* currently not used */
	tup_p->arg[IR_ARG_TYPE].type  = type_p;     /* network type */
	tup_p->arg[IR_ARG_CS_CHAR].cs_char = type_p->cs_char_type;

	IR_process_tup(ctx_p, tup_p);
}


/*
*  I R _ g e n _ c s _ c h a r _ r e p
 *
*  Generates tuples for a [cs_char] type.
*/
static void IR_gen_cs_char_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [io] AST type node */
 AST_instance_n_t    *inst_p     /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */
	AST_cs_char_n_t     *ichar_p;   /* Ptr to AST cs_char node */

	ichar_p = type_p->cs_char_type;

	/*
	 * Generate indirect reference tuple.  The flag indicates that the tuples
	 * will hang off the cs_char node instead of the type node itself.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_type_indirect_k);
	tup_p->flags |= IR_CS_CHAR;
	tup_p->arg[IR_ARG_EXPR].expr = NULL;
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	tup_p->arg[IR_ARG_INST].inst = inst_p;

	/*
	 * Generate tuples for [cs_char] type if not yet done.
	 */
	if (ichar_p->ir_info == NULL)
	{
		/* Initialize type IREP info. */
		IR_INIT_NODE(ichar_p);

		/*
		 * Maintain scope context.  This will cause subsequent tuple insertions
		 * to be into the cs_char node's tuple list until another indirect type
		 * reference is generated or the end of this reference.
		 */
		IR_process_tup(ctx_p, tup_p);

		/*
		 * Generate tuples for [cs_char] type.  When control returns
		 * from this routine, the indirect type scope will have been popped.
		 */
		IR_gen_cs_char_type_rep(ctx_p, type_p, inst_p);
	}
}


/*
*  I R _ g e n _ p i p e _ t y p e _ r e p
 *
*  Generates tuples for a pipe type definition.
*/
static void IR_gen_pipe_type_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [io] AST type node */
 AST_instance_n_t    *inst_p ATTRIBUTE_UNUSED    /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */
	AST_pipe_n_t        *pipe_p;    /* Ptr to AST pipe node */

	pipe_p = type_p->type_structure.pipe;

	/*
	 * Generate IR_op_pipe_begin_k tuple and maintain scope context.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_pipe_begin_k);
	tup_p->arg[IR_ARG_TYPE].type  = type_p;             /* pipe type */
	tup_p->arg[IR_ARG_TYPE2].type = pipe_p->base_type;  /* pipe base type */
	IR_process_tup(ctx_p, tup_p);

	/*
	 * Recurse to generate tuples for the pipe base type.
	 */
	IR_gen_type_rep(ctx_p, pipe_p->base_type, NULL, 0);

	/*
	 * Generate IR_op_pipe_end_k tuple and maintain scope context.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_pipe_end_k);
	tup_p->arg[IR_ARG_TYPE].type  = type_p;             /* pipe type */
	tup_p->arg[IR_ARG_TYPE2].type = pipe_p->base_type;  /* pipe base type */
	IR_process_tup(ctx_p, tup_p);
}


/*
*  I R _ g e n _ p i p e _ r e p
 *
*  Generates tuples to reference a pipe type and if the type has
*  not yet been processed, generates tuples for the type.
*/
static void IR_gen_pipe_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [io] AST type node */
 AST_instance_n_t    *inst_p     /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */

	/*
	 * Generate indirect reference tuple.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_type_indirect_k);
	tup_p->arg[IR_ARG_EXPR].expr = NULL;
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	tup_p->arg[IR_ARG_INST].inst = inst_p;

	/*
	 * Generate tuples for pipe type if not yet done.
	 */
	if (type_p->ir_info == NULL)
	{
		/* Initialize type IREP info. */
		IR_INIT_NODE(type_p);

		/*
		 * Maintain scope context.  This will cause subsequent tuple insertions
		 * to be into the type node's tuple list until another indirect type
		 * reference is generated or the end of this type reference.
		 */
		IR_process_tup(ctx_p, tup_p);

		/*
		 * Generate tuples for pipe type.  When control returns
		 * from this routine, the indirect type scope will have been popped.
		 */
		IR_gen_pipe_type_rep(ctx_p, type_p, inst_p);
	}
}


/*
*  I R _ g e n _ c o n t e x t _ r e p
 *
*  Generates tuples for a [context_handle] type.
*/
static void IR_gen_context_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [in] Ptr to AST type node */
 AST_parameter_n_t   *param_p    /* [in] Ptr to AST parameter node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */

	/*
	 * Generate IR_op_context_handle_k tuple.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_context_handle_k);
	tup_p->arg[IR_ARG_EXPR].expr = NULL;
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	tup_p->arg[IR_ARG_PARAM].param = param_p;
}


/*
*  I R _ g e n _ s c a l a r _ r e p
 *
*  Generates tuples for a scalar type.
*/
static void IR_gen_scalar_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [in] Ptr to AST type node */
 AST_instance_n_t    *inst_p     /* [in] Ptr to AST instance node */
)
{
	IR_tup_n_t          *tup_p;     /* Ptr to irep tuple and args */

	/*
	 * We support bounded scalars ([range] attribute) but prefixing
	 * the scalar opcode with one indicating the scalar boundaries.
 	 */
	if (inst_p != NULL &&
            inst_p->field_attrs != NULL &&
            inst_p->field_attrs->range != NULL)
	{
		tup_p = IR_gen_irep_tup(ctx_p, IR_op_range_k);

		tup_p->arg[IR_ARG_TUP].tup = NULL;
		tup_p->arg[IR_ARG_TYPE].type = type_p;
		tup_p->arg[IR_ARG_INT].int_val = inst_p->field_attrs->range->value[0];
		tup_p->arg[IR_ARG_BOUND_XTRA].int_val = inst_p->field_attrs->range->value[1];
	}

	/*
	 * Generate IR_op_marshall_k tuple.
	 */
	tup_p = IR_gen_irep_tup(ctx_p, IR_op_marshall_k);
	/*
	 * Set a flag if this scalar parameter or field is used as a field
	 * attribute for an array of [cs_char] type.
	 */
	if (inst_p != NULL
			&& FE_TEST(inst_p->fe_info->flags, FE_USED_AS_CS_FLD_ATTR))
		tup_p->flags |= IR_CS_CHAR;
	tup_p->arg[IR_ARG_EXPR].expr = NULL;
	tup_p->arg[IR_ARG_TYPE].type = type_p;
	tup_p->arg[IR_ARG_INST].inst = inst_p;
}


/*
*  I R _ g e n _ t y p e _ r e p
 *
*  Generates tuples for a type reference.
*/
static void IR_gen_type_rep
(
 IR_scope_ctx_t      *ctx_p,     /* [io] Scope context */
 AST_type_n_t        *type_p,    /* [in] Ptr to AST type node */
 AST_instance_n_t    *inst_p,    /* [in] Ptr to AST instance node */
 IR_flags_t          flags       /* [in] IREP flags */
)
{
	/*
	 * Must test for [represent_as] before [transmit_as].  Types with both
	 * attributes have the transmit_as tuples nested within the rep_as tuples.
	 * Both are accessed indirectly, but the represent_as tuples hang off the
	 * rep_as node instead of the type node.  If we're already in a rep_as
	 * scope, this is the second time through for the same type, so ignore the
	 * rep_as this time around.  We know this since rep_as cannot be nested.
	 */
	if (type_p->rep_as_type != NULL && !IR_in_rep_as(ctx_p)
			&& type_p->kind != AST_handle_k)
	{
		IR_gen_repr_as_rep(ctx_p, type_p, inst_p);
		return;
	}

	if (type_p->xmit_as_type != NULL)
	{
		IR_gen_xmit_as_rep(ctx_p, type_p, inst_p);
		return;
	}

	/*
	 * If we're already in a cs_char scope, this is the second time through for
	 * the same type (this time to process the network type), so ignore the
	 * cs_char this time around.  We know this since cs_char cannot be nested.
	 */
	if (type_p->cs_char_type != NULL && !IR_in_cs_char(ctx_p))
	{
		IR_gen_cs_char_rep(ctx_p, type_p, inst_p);
		return;
	}

	/* Note: no special action for [handle], the type is transmissible. */

	switch(type_p->kind)
	{
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
		case AST_enum_k:
			IR_gen_scalar_rep(ctx_p, type_p, inst_p);
			break;

		case AST_void_k:
			/* void valid on function result; no action */
			break;

		case AST_handle_k:
			/* handle_t not shipped; no action */
			break;

		case AST_array_k:
			if (type_p->type_structure.array->index_count > 1
					&& inst_p != NULL && AST_STRING0_SET(inst_p))
			{
				IR_gen_multid_aos(ctx_p, type_p, inst_p);
			}
			else
			{
				/*
				 * Caller will have set IR_STRING flag for stringified pointer case,
				 * we must set it here for stringified array case.
				 */
				if (IR_STRING_ARRAY(type_p, inst_p))
					flags |= IR_STRING;
				IR_gen_array_rep(ctx_p, type_p, inst_p, flags);
			}
			break;

		case AST_structure_k:
			IR_gen_struct_rep(ctx_p, type_p, inst_p);
			break;

		case AST_pipe_k:
			IR_gen_pipe_rep(ctx_p, type_p, inst_p);
			break;

		case AST_pointer_k:
			/*
			 * Test first for context handle, which is only valid use of void *.
			 * Context handles can only be parameters.
			 * Look for object references, which are not normal pointers
			 */
			if (type_p->type_structure.pointer->pointee_type->kind == AST_void_k)
			{
				if (inst_p == NULL || !AST_CONTEXT_SET(inst_p))
				{
					INTERNAL_ERROR("void * in invalid context");
				}
				else
					IR_gen_context_rep(ctx_p, type_p, (AST_parameter_n_t *)inst_p);
			}
			else if (AST_CONTEXT_RD_SET(type_p)
					&& type_p->type_structure.pointer->pointee_type->kind
					== AST_structure_k)
				IR_gen_context_rep(ctx_p, type_p, (AST_parameter_n_t *)inst_p);
			else if (type_p->type_structure.pointer->pointee_type->kind == AST_interface_k)
				IR_gen_interface_rep(ctx_p, type_p->type_structure.pointer->pointee_type, inst_p);
			else
				IR_gen_pointer_rep(ctx_p, type_p, inst_p);
			break;

		case AST_function_k:
			/*** NYI ***/
			break;

		case AST_disc_union_k:
			IR_gen_disc_union_rep(ctx_p, type_p, inst_p);
			break;

		default:
			INTERNAL_ERROR("Unexpected type kind");
	}
}


/*
*  I R _ g e n _ p a r a m _ r e p
 *
*  Generates tuples to describe an operation parameter.
*/
void IR_gen_param_rep
(
 AST_parameter_n_t   *param_p    /* [io] AST parameter node */
)
{
	IR_scope_ctx_t      *ctx_p;     /* Scope context */
	IR_tup_n_t          *tup_p;     /* Ptr to generated IREP tuple */

	/* Initialize scope context */
	ctx_p = IR_init_scope(param_p);

	/* Initialize parameter IREP info. */
	IR_INIT_NODE(param_p);

	/*
	 * If this is the first [in] and/or [out] parameter in its operation, and
	 * the operation has non-fixed array(s) of [cs_char] type, generate a
	 * 'codeset shadow' tuple.
	 */
	if (FE_TEST(param_p->fe_info->flags, FE_FIRST_IN_NF_CS_ARR)
			|| FE_TEST(param_p->fe_info->flags, FE_FIRST_OUT_NF_CS_ARR))
	{
		tup_p = IR_gen_irep_tup(ctx_p, IR_op_codeset_shadow_k);
		tup_p->arg[IR_ARG_INT].int_val = 0;
		if (!FE_TEST(param_p->fe_info->flags, FE_FIRST_IN_NF_CS_ARR))
			tup_p->flags |= IR_OUT_ONLY;
		if (!FE_TEST(param_p->fe_info->flags, FE_FIRST_OUT_NF_CS_ARR))
			tup_p->flags |= IR_IN_ONLY;
	}

	/* Generate irep for parameter and contained types */
	IR_gen_type_rep(ctx_p, param_p->type, (AST_instance_n_t *)param_p, 0);

	/*
	 * If this is the last [in] and/or [out] parameter in its operation, and
	 * the operation has non-fixed array(s) of [cs_char] type, generate a
	 * 'release shadow' tuple.
	 */
	if (FE_TEST(param_p->fe_info->flags, FE_LAST_IN_NF_CS_ARR)
			|| FE_TEST(param_p->fe_info->flags, FE_LAST_OUT_NF_CS_ARR))
	{
		tup_p = IR_gen_irep_tup(ctx_p, IR_op_release_shadow_k);
		if (!FE_TEST(param_p->fe_info->flags, FE_LAST_IN_NF_CS_ARR))
			tup_p->flags |= IR_OUT_ONLY;
		if (!FE_TEST(param_p->fe_info->flags, FE_LAST_OUT_NF_CS_ARR))
			tup_p->flags |= IR_IN_ONLY;
	}

	/* Cleanup scope context */
	IR_finish_scope(ctx_p);
}


/*
*  I R _ g e n _ i r e p
 *
*  Main IREP routine - generates the Intermediate Representation of an IDL
*  interface from the Abstract Syntax Tree representation of the interface.
*  Tuples that describe the irep are hung off AST parameter and type nodes.
*/
boolean IR_gen_irep                 /* Returns TRUE on success */
(
 boolean             *cmd_opt ATTRIBUTE_UNUSED,   /* [in] array of cmd option flags */
 void                **cmd_val ATTRIBUTE_UNUSED,  /* [in] array of cmd option values */
 struct AST_interface_n_t *int_p /* [io] interface abstract syntax tree */
)
{
	AST_export_n_t      *export_p;
	AST_operation_n_t   *oper_p;
	AST_parameter_n_t   *param_p;

	for (export_p = int_p->exports; export_p != NULL; export_p = export_p->next)
	{
		if (export_p->kind == AST_operation_k)
		{
			oper_p = export_p->thing_p.exported_operation;

			/* Process each parameter */
			for (param_p = oper_p->parameters;
					param_p != NULL;
					param_p = param_p->next)
			{
				IR_gen_param_rep(param_p);
			}

			/* Process the operation result */
			IR_gen_param_rep(oper_p->result);
		}
	}

	return TRUE;
}
