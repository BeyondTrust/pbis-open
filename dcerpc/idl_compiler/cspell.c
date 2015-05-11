/*
 * 
 * (c) Copyright 1993 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1993 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1993 DIGITAL EQUIPMENT CORPORATION
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
 **      cspell.c
 **
 **  FACILITY:
 **
 **      Interface Definition Language (IDL) Compiler
 **
 **  ABSTRACT:
 **
 **      Routines needed by more than one module to output C source
 **
 **  VERSION: DCE 1.1
 **
 */

#include <nidl.h>
#include <ast.h>
#include <cspell.h>
#include <cspeldcl.h>
#include <genpipes.h>
#include <bedeck.h>


/*
 * The following define, typedef, and four static functions are the
 * rep for the type_tail_t data type which represents the residual suffix
 * which must be emitted in a declarator after the name being typed.
 */

#define MAX_TAIL_LEN 15

typedef enum {p_k, a_k, f_k} type_kind_t;
typedef struct {
	int len;
	struct {
		type_kind_t kind;
		union {
			struct {
				AST_array_n_t *array;
				boolean in_typedef_or_struct;
			} array_info;
			struct {
				AST_parameter_n_t *param_list;
				boolean function_def;
			} function_info;
		} content;
	}  vec[MAX_TAIL_LEN];
} type_tail_t;

static void CSPELL_add_paren_to_tail
(
 type_tail_t *tail
)
{
	int i;

	i = (tail->len) ++;
	if (tail->len > MAX_TAIL_LEN) INTERNAL_ERROR("Data structure too compilicated; Tail array overflow");
	(tail->vec)[i].kind = p_k;
}

static void CSPELL_add_array_to_tail
(
 type_tail_t *tail,
 AST_array_n_t *array,
 boolean in_typedef_or_struct
)
{
	int i;

	i = (tail->len) ++;
	if (tail->len > MAX_TAIL_LEN) INTERNAL_ERROR("Data structure too compilicated; Tail array overflow");
	(tail->vec)[i].kind = a_k;
	(tail->vec)[i].content.array_info.array = array;
	(tail->vec)[i].content.array_info.in_typedef_or_struct =
		in_typedef_or_struct;
}

static void CSPELL_add_func_to_tail
(
 type_tail_t *tail,
 AST_parameter_n_t *pl,
 boolean function_def
)
{
	int i;

	i = (tail->len) ++;
	if (tail->len > MAX_TAIL_LEN) INTERNAL_ERROR("Data structure too compilicated; Tail array overflow");
	(tail->vec)[i].kind = f_k;
	(tail->vec)[i].content.function_info.param_list = pl;
	(tail->vec)[i].content.function_info.function_def = function_def;
}

static void CSPELL_array_bounds (
		FILE *fid,
		AST_array_n_t *array,
		boolean in_typedef_or_struct
		);

static void CSPELL_function_sig (
		FILE *fid,
		AST_parameter_n_t *pp,
		boolean func_def,
		boolean encoding_services
		);

static void CSPELL_type_tail
(
 FILE *fid,
 type_tail_t *tail,
 boolean encoding_services   /* TRUE => [encode] or [decode] on operation */
)
{
	int i;

	for (i = 0; i < tail->len; i++)
		switch (tail->vec[i].kind) {
			case p_k:
				fprintf (fid, ")");
				break;
			case a_k:
				CSPELL_array_bounds (
						fid,
						tail->vec[i].content.array_info.array,
						tail->vec[i].content.array_info.in_typedef_or_struct);
				break;
			case f_k:
				CSPELL_function_sig (
						fid,
						tail->vec[i].content.function_info.param_list,
						tail->vec[i].content.function_info.function_def,
						encoding_services);
				break;
			default:
				INTERNAL_ERROR("Invalid tail kind");
		}
}


/*
 * spell_name
 *
 * Output an identifier to the file specified
 */
void spell_name
(
 FILE *fid,
 NAMETABLE_id_t name
)
{
	char const *str;

	NAMETABLE_id_to_string (name, &str);
	fprintf (fid, "%s", str);
}


static void CSPELL_type_exp (
		FILE *fid,
		AST_type_n_t *tp,
		type_tail_t *tail,
		AST_type_n_t *in_typedef,
		boolean in_struct,
		boolean func_def,
		boolean spell_tag
		);

/*
 * C S P E L L _ s c a l a r _ t y p e _ s u f f i x
 */
boolean CSPELL_scalar_type_suffix
(
 FILE *fid,
 AST_type_n_t *tp
)
{
	boolean result = true;


	/* prepend a 'u' for unsigned types */
	switch (tp->kind) {
		case AST_small_unsigned_k:
		case AST_short_unsigned_k:
		case AST_long_unsigned_k:
		case AST_hyper_unsigned_k:
			fprintf (fid, "u");
		default:
			/* nothing */
			break;
	}

	switch (tp->kind) {
		case AST_boolean_k:
			fprintf (fid, "boolean");
			break;

		case AST_byte_k:
			fprintf (fid, "byte");
			break;

		case AST_character_k:
			fprintf (fid, "char");
			break;

		case AST_small_integer_k:
		case AST_small_unsigned_k:
			fprintf (fid, "small_int");
			break;

		case AST_short_integer_k:
		case AST_short_unsigned_k:
			fprintf (fid, "short_int");
			break;

		case AST_long_integer_k:
		case AST_long_unsigned_k:
			fprintf (fid, "long_int");
			break;

		case AST_hyper_integer_k:
		case AST_hyper_unsigned_k:
			fprintf (fid, "hyper_int");
			break;

		case AST_short_float_k:
			fprintf (fid, "short_float");
			break;

		case AST_long_float_k:
			fprintf (fid, "long_float");
			break;

		case AST_enum_k:
			fprintf (fid, "enum");
			break;

		default:
			result = false;
	}
	return result;
}

/*
 * s p e l l _ i d l _ s c a l a r _ t y p e _ n a m e
 */
static void spell_idl_scalar_type_name
(
 FILE *fid,
 AST_type_n_t *tp
)
{

	fprintf(fid, "idl_");

	if (CSPELL_scalar_type_suffix(fid, tp))
		fprintf(fid, " ");
	else
		INTERNAL_ERROR("Invalid type kind");
}

/*
 * CSPELL_function_sig
 *
 * Spell a function's parameter list
 */
static void CSPELL_function_sig
(
 FILE *fid,
 AST_parameter_n_t *pp,
 boolean func_def,
 boolean encoding_services   /* TRUE => [encode] or [decode] on operation */
)
{
	if (!func_def)
	{
		fprintf (fid, "(\n");
		CSPELL_parameter_list (fid, pp, encoding_services);
		fprintf (fid, "\n)");
	}
	else
	{
		fprintf (fid, "\n(\n");
		CSPELL_parameter_list (fid, pp, encoding_services);
		fprintf (fid, "\n)\n");
	}
}

/*
 * DDBE_array_is_C_like
 *
 * Returns TRUE if there is a natural C mapping for an array
 */
static boolean DDBE_array_is_C_like
(
 AST_array_n_t *array
)
{
	int i;
	AST_array_index_n_t *index_array_ptr;

	index_array_ptr = array->index_vec;
	/* Test for dynamic bounds in dimensions other than the first */
	for (i = 1; i < array->index_count; i++)
	{
		index_array_ptr ++;
		if ( !(AST_FIXED_LOWER_SET(index_array_ptr))
				|| !(AST_FIXED_UPPER_SET(index_array_ptr)) )
		{
			return( false );
		}
	}
	return( true );
}

/*
 * CSPELL_array_bounds
 *
 * Spell an array's bounds
 *
 */
static void CSPELL_array_bounds
(
 FILE *fid,
 AST_array_n_t *array,
 boolean in_typedef_or_struct
)
{
	int i;
	AST_array_index_n_t *index_array_ptr;
	long array_dim_size;    /* Size of array in current dimension */

	if ( ! DDBE_array_is_C_like(array) )
	{
		if( ! in_typedef_or_struct ) fprintf (fid, "[]");
		else fprintf (fid, "[1]");
		return;
	}

	index_array_ptr = array->index_vec;
	for (i = 0; i < array->index_count; i++) {
		if ( (AST_FIXED_LOWER_SET(index_array_ptr))
				&& (AST_FIXED_UPPER_SET(index_array_ptr)) ) {
			/* Fixed bounds, convert to C syntax */
			array_dim_size = index_array_ptr->upper_bound->value.int_val
				- index_array_ptr->lower_bound->value.int_val + 1;
			fprintf (fid, "[%ld]", array_dim_size);
		}
		else {
			/* Varying bounds */
			if ((i == 0) && ( ! in_typedef_or_struct )) fprintf (fid, "[]");
			else fprintf (fid, "[1]");
			/* Tell lies as C does not understand conformant non-first bound
				or conformant bounds within structures or typedefs  */
		}
		index_array_ptr ++;
	}
}


/*
 * CSPELL_pipe_struct_routine_decl
 *
 * Spell one function pointer type of a pipe's rep type
 */
void CSPELL_pipe_struct_routine_decl
(
 FILE *fid,
 AST_type_n_t *p_pipe_type,
 BE_pipe_routine_k_t routine_kind,
 boolean cast
)
{
	type_tail_t tail;
	char *name = "";

	/* choose a name for the pipe field, unless a cast expression */
	if (!cast)
	{
		name = (routine_kind == BE_pipe_push_k) ? "push" :
			((routine_kind == BE_pipe_pull_k) ? "pull" : "alloc");
	}

	fprintf( fid, "void (* %s)(\n",name);
	fprintf (fid, "rpc_ss_pipe_state_t state,\n" );
	if ( routine_kind == BE_pipe_alloc_k )
	{
		fprintf( fid, "idl_ulong_int bsize,\n" );
	}

	tail.len = 0;
	CSPELL_type_exp (fid, p_pipe_type->type_structure.pipe->base_type,
			&tail, NULL, false, false, true);
	fprintf( fid, "%s*buf,\n",
			(routine_kind == BE_pipe_alloc_k) ? "*" : "");
	CSPELL_type_tail (fid, &tail, false);

	if ( routine_kind == BE_pipe_pull_k )
	{
		fprintf( fid, "idl_ulong_int esize,\n" );
	}
	fprintf( fid, "idl_ulong_int %c%ccount\n",
			((routine_kind == BE_pipe_push_k) ? ' ' : '*'),
			((routine_kind == BE_pipe_alloc_k) ? 'b' : 'e') );
	if (!cast) fprintf (fid, ";\n");
}

/*
 * CSPELL_pipe_def
 *
 * Spell a pipe's concrete rep type
 */
static void CSPELL_pipe_def
(
 FILE *fid,
 AST_type_n_t *p_pipe_type
)
{
	/* Declare the structure that represents the pipe */
	fprintf( fid, "struct " );
	spell_name (fid, p_pipe_type->name);
	fprintf( fid, " {\n" );
	CSPELL_pipe_struct_routine_decl( fid, p_pipe_type, BE_pipe_pull_k, FALSE );
	CSPELL_pipe_struct_routine_decl( fid, p_pipe_type, BE_pipe_push_k, FALSE );
	CSPELL_pipe_struct_routine_decl( fid, p_pipe_type, BE_pipe_alloc_k, FALSE );
	fprintf( fid, "rpc_ss_pipe_state_t state;\n" );
	fprintf( fid, "} " );
}


/*
 * CSPELL_type_exp
 *
 * Spell a type exp by writing its prefix (i.e. the portion before the
 * name in a declarator) to file fid and by building a rep of its suffix
 * in the tail data structure.
 */
static void CSPELL_type_exp
(
 FILE *fid,
 AST_type_n_t *tp,
 type_tail_t *tail,
 AST_type_n_t *in_typedef,
 boolean in_struct,
 boolean func_def,
 boolean spell_tag
)
{
	AST_field_n_t           *fp;
	AST_arm_n_t             *cp;
	AST_disc_union_n_t      *vp;
	AST_constant_n_t        *ecp;
	int                     pointer_count;
	AST_type_n_t            *pointee_tp;
	boolean                 parenthesized;
	boolean                 first = true;
	boolean                 n_e_union;

	/*
	 * If we are in the process of defining a type which was defined as
	 * another type, emit the definiend.
	 */
	if (in_typedef == tp && tp->defined_as)
		CSPELL_type_exp (
				fid,
				tp->defined_as,
				tail,
				in_typedef,
				in_struct,
				func_def,
				spell_tag);

	/*
	 * If the type has the [represent_as] attribute, emit its local rep name.
	 */
	else if (in_typedef != tp && tp->rep_as_type
			&& !(in_typedef != NULL && in_typedef->name == tp->name))
	{
		fprintf( fid, "/* Type must appear in user header or IDL */ " );
		spell_name (fid, tp->rep_as_type->type_name);
		fprintf (fid, " ");
	}

	/*
	 * If the type has the [cs_char] attribute, emit its local name.
	 */
	else if (in_typedef != tp && tp->cs_char_type
			&& !(in_typedef != NULL && in_typedef->name == tp->name))
	{
		fprintf( fid, "/* Type must appear in user header or IDL */ " );
		spell_name (fid, tp->cs_char_type->type_name);
		fprintf (fid, " ");
	}

	/*
	 * If we are spelling a type which was expressed simply as "struct foo"
	 * or "union foo" then just echo its original expression
	 */
	else if (AST_DEF_AS_TAG_SET(tp))
	{
		if (tp->kind == AST_disc_union_k
				&& tp->type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID)
			fprintf(fid, "union ");
		else
			fprintf (fid, "struct ");
		if (tp->kind == AST_structure_k)
			spell_name (fid, tp->type_structure.structure->tag_name);
		else if (tp->kind == AST_disc_union_k)
			spell_name (fid, tp->type_structure.disc_union->tag_name);
		fprintf (fid, " ");
	}

	/*
	 * If we are not in the process of defining the type and it has a name,
	 * emit it.
	 */
	else if (in_typedef != tp && tp->name != NAMETABLE_NIL_ID)
	{
		spell_name (fid, tp->name);
		fprintf (fid, " ");
	}

	/*
	 * The DEF_AS_TAG case above took care of in_typedef cases where only a
	 * tag name should be emitted.  If we have gotten this far and we are still
	 * in_typedef, then the complete structure or union body must be emitted.
	 * However, if we are not in_typedef, then we should just use a tagname
	 * here if one exists.
	 */
	else if (in_typedef == NULL && tp->kind == AST_structure_k &&
			tp->type_structure.structure->tag_name != NAMETABLE_NIL_ID)
	{
		fprintf (fid, "struct ");
		spell_name (fid, tp->type_structure.structure->tag_name);
		fprintf (fid, " ");
	}

	else if (in_typedef == NULL && tp->kind == AST_disc_union_k &&
			tp->type_structure.disc_union->tag_name != NAMETABLE_NIL_ID)
	{
		if (tp->type_structure.disc_union->discrim_name == NAMETABLE_NIL_ID)
			fprintf (fid, "union ");
		else
			fprintf (fid, "struct ");
		spell_name (fid, tp->type_structure.disc_union->tag_name);
		fprintf (fid, " ");
	}

	else switch (tp->kind) {

		case AST_boolean_k:
		case AST_byte_k:
		case AST_small_integer_k:
		case AST_small_unsigned_k:
		case AST_short_integer_k:
		case AST_short_unsigned_k:
		case AST_long_integer_k:
		case AST_long_unsigned_k:
		case AST_hyper_integer_k:
		case AST_hyper_unsigned_k:
		case AST_character_k:
		case AST_short_float_k:
		case AST_long_float_k:
			spell_idl_scalar_type_name (fid, tp);
			break;

		case AST_handle_k:
			fprintf (fid, "handle_t ");
			break;

		case AST_enum_k:
			fprintf (fid, "enum {");
			for (ecp = tp->type_structure.enumeration->enum_constants;
					ecp != NULL; ecp = ecp->next) {
				if (first)
					first = false;
				else
					fprintf (fid, ",\n");
				spell_name (fid, ecp->name);
				fprintf(fid, " = %ld", ecp->value.int_val);
			}
			fprintf (fid, "} ");
			break;

		case AST_structure_k:
			fprintf (fid, "struct ");
			if ( spell_tag )
				spell_name (fid, tp->type_structure.structure->tag_name);
			fprintf (fid, " {\n");
			for (fp = tp->type_structure.structure->fields; fp != NULL;
					fp = fp->next) {
				CSPELL_typed_name (
						fid,
						fp->type,
						fp->name,
						in_typedef,
						true,
						true,
						false);
				fprintf (fid, ";\n");
			}
			fprintf (fid, "} ");
			break;

		case AST_disc_union_k:
			vp = tp->type_structure.disc_union;
			n_e_union = (vp->discrim_name == NAMETABLE_NIL_ID);
			if ( ! n_e_union )
			{
				fprintf (fid, "struct ");
				if ( spell_tag )
					spell_name (fid, tp->type_structure.disc_union->tag_name);
				fprintf (fid, " {\n");
				/*
				 * Use the parameters the procedure was called with
				 * We know we are declaring a scalar and they will be set null
				 */
				CSPELL_typed_name (
						fid,
						vp->discrim_type,
						vp->discrim_name,
						in_typedef,
						true,
						true,
						false);
				fprintf (fid, ";\n");
			}
			fprintf (fid, "union ");
			if ( n_e_union && spell_tag )
				spell_name (fid, tp->type_structure.disc_union->tag_name);
			fprintf (fid, " {\n");
			for (cp = vp->arms; cp != NULL; cp = cp->next)
			{
				CSPELL_labels (fid, cp->labels);
				if (cp->type == NULL)
					fprintf (fid, "/* Empty arm */\n");
				else
				{
					CSPELL_typed_name (
							fid,
							cp->type,
							cp->name,
							in_typedef,
							true,
							true,
							false);
					fprintf (fid, ";\n");
				}
			}
			fprintf (fid, "} ");
			if ( ! n_e_union )
			{
				spell_name (fid, vp->union_name);
				fprintf (fid, ";\n");
				fprintf (fid, "} ");
			}
			break;

		case AST_pipe_k:
			CSPELL_pipe_def(fid, tp);
			break;

		case AST_void_k:
			fprintf (fid, "void ");
			break;

		case AST_array_k:
			CSPELL_add_array_to_tail (
					tail,
					tp->type_structure.array,
					(tp == in_typedef) || in_struct);
			CSPELL_type_exp (
					fid,
					tp->type_structure.array->element_type,
					tail,
					in_typedef,
					in_struct,
					false,
					spell_tag);
			break;

		case AST_pointer_k:
			pointer_count = 0;
			pointee_tp = tp;
			while (pointee_tp->kind == AST_pointer_k
					&& (in_typedef == pointee_tp ||
						pointee_tp->name == NAMETABLE_NIL_ID))
			{
				pointee_tp = pointee_tp->type_structure.pointer->pointee_type;
				pointer_count++;
			}
			if (( parenthesized = (pointee_tp->kind == AST_array_k
							|| pointee_tp->kind == AST_function_k)))
				CSPELL_add_paren_to_tail (tail);

			if (pointee_tp->kind == AST_void_k) {
				fprintf (fid, "idl_void_p_t ");
				pointer_count--;
			}
			else
				CSPELL_type_exp (
						fid,
						pointee_tp,
						tail,
						in_typedef,
						in_struct,
						false,
						spell_tag);

			if (parenthesized)
				fprintf (fid, "(");
			for (; pointer_count; pointer_count--)
				fprintf (fid, "*");
			break;

		case AST_function_k:
			CSPELL_add_func_to_tail(
					tail,
					tp->type_structure.function->parameters,
					func_def);
			CSPELL_type_exp(
					fid,
					tp->type_structure.function->result->type,
					tail,
					in_typedef,
					in_struct,
					false,
					true);
			break;

		default:
			INTERNAL_ERROR("Unknown type kind in CSPELL_type_exp");
	}

}


/*
 * CSPELL_typed_name
 */
void CSPELL_typed_name
(
 FILE *fid,
 AST_type_n_t *type,
 NAMETABLE_id_t name,
 AST_type_n_t *in_typedef,
 boolean in_struct,
 boolean spell_tag,
 boolean encoding_services   /* TRUE => [encode] or [decode] on operation */
)
{
	type_tail_t tail;

	tail.len = 0;
	CSPELL_type_exp (fid, type, &tail, in_typedef, in_struct, false, spell_tag);
	spell_name (fid, name);
	CSPELL_type_tail (fid, &tail, encoding_services);
}


/*
 * CSPELL_function_def_header
 */
void CSPELL_function_def_header
(
 FILE *fid,
 AST_operation_n_t *oper,
 NAMETABLE_id_t name
)
{
	type_tail_t tail;
	AST_type_n_t func_type_node;

	func_type_node = *BE_function_p;
	func_type_node.type_structure.function = oper;

	tail.len = 0;
	CSPELL_type_exp (fid, &func_type_node, &tail, NULL, false, true, true);
	spell_name (fid, name);
	CSPELL_type_tail ( fid, &tail,
			(AST_ENCODE_SET(oper) || AST_DECODE_SET(oper)) );
}


/*
 * CSPELL_type_exp_simple
 */
void CSPELL_type_exp_simple
(
 FILE *fid,
 AST_type_n_t *tp
)
{
	CSPELL_typed_name(fid, tp, NAMETABLE_NIL_ID, NULL, false, true, false);
}

/*
 * CSPELL_var_decl
 *
 * Spell a variable declaration
 */
void CSPELL_var_decl
(
 FILE *fid,
 AST_type_n_t *type,
 NAMETABLE_id_t name
)
{
	CSPELL_typed_name (fid, type, name, NULL, false, true, false);
	fprintf (fid, ";\n");
}


/*
 * CSPELL_cast_exp
 */
void CSPELL_cast_exp
(
 FILE *fid,
 AST_type_n_t *tp
)
{
	fprintf (fid, "(");
	CSPELL_typed_name (fid, tp, NAMETABLE_NIL_ID, NULL, false, true, false);
	fprintf (fid, ")");
}

/*
 * CSPELL_ptr_cast_exp
 */
void CSPELL_ptr_cast_exp
(
 FILE *fid,
 AST_type_n_t *tp
)
{
	AST_type_n_t    pointer_type;
	AST_pointer_n_t pointer;

	/*
	 * Build temporary nodes to construct a pointer to the passed type,
	 * then spell the cast expression using normal mechanism.
	 */
	memset(&pointer_type, 0, sizeof(pointer_type));
	pointer_type.kind = AST_pointer_k;
	pointer_type.type_structure.pointer = &pointer;

	memset(&pointer, 0, sizeof(pointer));
	pointer.pointee_type = tp;

	CSPELL_cast_exp(fid, &pointer_type);
}



/*
 * DDBE_spell_manager_param_cast
 *
 * If the parameter being passed to the manager is an array, cast it as a
*  pointer to the array base type
*/
void DDBE_spell_manager_param_cast
(
 FILE *fid,
 AST_type_n_t *tp
)
{
	type_tail_t tail;
	AST_type_n_t *tp_for_type_exp;
	AST_type_n_t array_slice_type;
	AST_array_n_t array_slice_desc;

	fprintf (fid, "(");
	tail.len = 0;
	if (tp->kind == AST_array_k)
	{
		if ((tp->type_structure.array->index_count == 1)
				|| !DDBE_array_is_C_like(tp->type_structure.array))
			tp_for_type_exp = tp->type_structure.array->element_type;
		else
		{
			array_slice_desc.element_type
				= tp->type_structure.array->element_type;
			array_slice_desc.index_count
				= tp->type_structure.array->index_count - 1;
			array_slice_desc.index_vec
				= &(tp->type_structure.array->index_vec[1]);
			array_slice_type = *tp;
			array_slice_type.name = NAMETABLE_NIL_ID;
			array_slice_type.type_structure.array = &array_slice_desc;
			tp_for_type_exp = &array_slice_type;
		}
	} 
	else
		tp_for_type_exp = tp;
	CSPELL_type_exp (fid, tp_for_type_exp, &tail, NULL, false, false, true);

	if (tp->kind == AST_array_k)
		fprintf (fid, "(*)");

	CSPELL_type_tail (fid, &tail, false);

	fprintf (fid, ")");
}
