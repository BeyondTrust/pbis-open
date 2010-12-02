#include <nidl.h>
#include <nametbl.h>
#include <errors.h>
#include <astp.h>
#include <nidlmsg.h>
#include <nidl_y.h>

extern int nidl_yylineno;

AST_exp_n_t * AST_exp_new(unsigned long exp_type)	{
	AST_exp_n_t * exp = NEW(AST_exp_n_t);
	exp->exp_type = exp_type;
	return exp;
}

AST_exp_n_t * AST_exp_integer_constant(long value, int int_signed)
{
	AST_exp_n_t * exp = AST_exp_new(AST_EXP_CONSTANT);
	exp->exp.constant.type = AST_int_const_k;
	exp->exp.constant.val.integer = value;
	exp->exp.constant.int_signed = int_signed;
	return exp;
}

AST_exp_n_t * AST_exp_char_constant(char value)	{
	AST_exp_n_t * exp = AST_exp_new(AST_EXP_CONSTANT);
	exp->exp.constant.type = AST_nil_const_k;
	exp->exp.constant.val.other = AST_char_constant(value);
	return exp;
}

AST_exp_n_t * AST_exp_identifier(NAMETABLE_id_t name)	{
	AST_exp_n_t * exp = AST_exp_new(AST_EXP_CONSTANT);
	exp->exp.constant.type = AST_nil_const_k;
	exp->exp.constant.val.other = NULL;
	exp->exp.constant.name = name;
	return exp;
}

AST_exp_n_t * AST_exp_string_constant(STRTAB_str_t string)	{
	AST_exp_n_t * exp = AST_exp_new(AST_EXP_CONSTANT);
	exp->exp.constant.type = AST_nil_const_k;
	exp->exp.constant.val.other = AST_string_constant(string);
	return exp;
}

AST_exp_n_t * AST_exp_null_constant(void)	{
	AST_exp_n_t * exp = AST_exp_new(AST_EXP_CONSTANT);
	exp->exp.constant.type = AST_nil_const_k;
	exp->exp.constant.val.other = AST_null_constant();
	return exp;
}

AST_exp_n_t * AST_exp_boolean_constant(boolean value)	{
	AST_exp_n_t * exp = AST_exp_new(AST_EXP_CONSTANT);
	exp->exp.constant.type = AST_nil_const_k;
	exp->exp.constant.val.other = AST_boolean_constant(value);
	return exp;
}

AST_exp_n_t * AST_expression(unsigned long exp_type, AST_exp_n_t * oper1, AST_exp_n_t * oper2, AST_exp_n_t * oper3)
{
	AST_exp_n_t * exp = AST_exp_new(exp_type);
	exp->exp.expression.oper1 = oper1;
	exp->exp.expression.oper2 = oper2;
	exp->exp.expression.oper3 = oper3;
	
	return exp;
}

AST_constant_n_t * AST_constant_from_exp(AST_exp_n_t * exp)	{
	AST_constant_n_t * const_return;
	ASTP_evaluate_expr(exp, true);
	if (exp->exp_type == AST_EXP_CONSTANT)	{
		if (exp->exp.constant.type == AST_int_const_k)	{
			const_return = AST_integer_constant(exp->exp.constant.val.integer);
			const_return->int_signed = exp->exp.constant.int_signed;
		}
		else	{
			const_return = exp->exp.constant.val.other;
		}
	}
	else
		const_return = NULL;
	return const_return;
}

/* returns true if the expression is regarded as "simple".
 * Simple expressions match this criteria:
 * identifier  (parameter, field)
 * integer constant
 * <dereference+>identifier
 * identifier / [248]
 * identifier * [248]
 * identifier - 1
 * identifier + 1
 * 
 * */
boolean ASTP_expr_is_simple(AST_exp_n_t * exp)
{
	if (ASTP_evaluate_expr(exp, true))	{
		return true;
	}
	switch(exp->exp_type)	{
		case AST_EXP_CONSTANT:
			return true;
			break;
		case AST_EXP_BINARY_SLASH:
		case AST_EXP_BINARY_STAR:
			if (exp->exp.expression.oper1->exp_type == AST_EXP_CONSTANT &&
                            ((ASTP_expr_integer_value(exp->exp.expression.oper2) == 2) ||
                             (ASTP_expr_integer_value(exp->exp.expression.oper2) == 4) ||
                             (ASTP_expr_integer_value(exp->exp.expression.oper2) == 8)))
			{
				return true;
			}
			break;
		case AST_EXP_BINARY_PLUS:
		case AST_EXP_BINARY_MINUS:
			if (exp->exp.expression.oper1->exp_type == AST_EXP_CONSTANT &&
					ASTP_expr_integer_value(exp->exp.expression.oper2) == 1)
			{
				return true;
			}
			break;
		case AST_EXP_UNARY_STAR:
			while(exp->exp_type == AST_EXP_UNARY_STAR)
				exp = exp->exp.expression.oper1;
			if (exp->exp_type == AST_EXP_CONSTANT)	{

				return true;
			}
			break;
		case AST_EXP_BINARY_AND:
			/* could be alignment */
			if (exp->exp.expression.oper1->exp_type == AST_EXP_BINARY_PLUS
					&&
				exp->exp.expression.oper2->exp_type == AST_EXP_UNARY_TILDE)
			{
				AST_exp_n_t * op1 = exp->exp.expression.oper1;
				AST_exp_n_t * op2 = exp->exp.expression.oper2;
				AST_exp_n_t * ident_exp = op1->exp.expression.oper1;
				long nval = ASTP_expr_integer_value(op1->exp.expression.oper2);

				/* allow one level of indirection for the identifier part */
				if ((ident_exp->exp_type == AST_EXP_CONSTANT || (
								ident_exp->exp_type == AST_EXP_UNARY_STAR &&
								ident_exp->exp.expression.oper1->exp_type == AST_EXP_CONSTANT))
						&& (nval == 1 || nval == 3 || nval == 7) &&
						ASTP_expr_integer_value(op2->exp.expression.oper1)
							== nval)
					return true;
					
			}
			break;
	}
	return false;
}

long ASTP_expr_integer_value(AST_exp_n_t * exp)	
{
	long value;
	ASTP_evaluate_expr(exp, true);
	if (exp->exp.constant.type == AST_int_const_k)
		value = exp->exp.constant.val.integer;
	else	{
		switch(exp->exp.constant.val.other->kind)	{
			case AST_int_const_k:
				value = exp->exp.constant.val.other->value.int_val;
				break;
			case AST_char_const_k:
				value = exp->exp.constant.val.other->value.char_val;
				break;
			case AST_boolean_const_k:
				value = exp->exp.constant.val.other->value.boolean_val;
				break;
			default:
				value = 0;
		}
	}
	return value;
}

/* Free an expression node and any child operands */
void ASTP_free_exp(AST_exp_n_t * exp)	{
	if (exp == NULL)
		return;
	if (exp->exp_type != AST_EXP_CONSTANT)	{
		ASTP_free_exp(exp->exp.expression.oper1);
		ASTP_free_exp(exp->exp.expression.oper2);
		ASTP_free_exp(exp->exp.expression.oper3);
	}
	FREE(exp);
}

/* evaluate the expression, reducing it down to a single constant expression
 * node if possible.
 * If constant_only is true, this routine will return false when it hits
 * the name of a parameter to indicate that the expression is not a constant
 * expression.
 * */
boolean ASTP_evaluate_expr(AST_exp_n_t * exp, boolean constant_only)
{
	boolean result;
	long val, val1, val2;
	AST_exp_n_t * op1=NULL, *op2=NULL, *op3=NULL;

	if (exp == NULL)	{
		log_warning(nidl_yylineno, NIDL_EXP_IS_NULL, NULL);
		return true;
	}
	
	if (exp->exp_type == AST_EXP_CONSTANT)	{
		/* resolve binding for identifiers */
		if (exp->exp.constant.type == AST_nil_const_k &&
				exp->exp.constant.val.other == NULL)	{

			exp->exp.constant.val.other = (AST_constant_n_t*)ASTP_lookup_binding(exp->exp.constant.name,
					fe_constant_n_k, FALSE);
			
			if (exp->exp.constant.val.other == NULL)
				return false;
					
		}
		/* already reduced to the simplest form */
		return true;
	} else if (exp->exp_type == AST_EXP_TUPLE) {
		op1 = exp->exp.expression.oper1;

		if (op1->exp.constant.type == AST_nil_const_k &&
		    op1->exp.constant.val.other == NULL) {
		    op1->exp.constant.val.other = (AST_constant_n_t *)ASTP_lookup_binding(op1->exp.constant.name,
			fe_constant_n_k, FALSE);
		    if (op1->exp.constant.val.other == NULL)
			return false;
		}

		op2 = exp->exp.expression.oper2;

		if (op2->exp.constant.type == AST_nil_const_k &&
		    op2->exp.constant.val.other == NULL) {
		    op2->exp.constant.val.other = (AST_constant_n_t *)ASTP_lookup_binding(op2->exp.constant.name,
			fe_constant_n_k, FALSE);
		    if (op2->exp.constant.val.other == NULL)
			return false;
		}
		return true;
        }

	/* time to evaluate some expressions.
	 * First, reduce the operands to their simplest forms too */

	result = ASTP_evaluate_expr(exp->exp.expression.oper1, constant_only);
	if (result == false)
		return false;
	op1 = exp->exp.expression.oper1;

	if ((exp->exp_type & AST_EXP_2_OP_MASK) != 0)	{
		result = ASTP_evaluate_expr(exp->exp.expression.oper2, constant_only);
		if (result == false)
			return false;
		op2 = exp->exp.expression.oper2;
	}
	if ((exp->exp_type & AST_EXP_3_OP_MASK) != 0)	{
		result = ASTP_evaluate_expr(exp->exp.expression.oper3, constant_only);
		if (result == false)
			return false;
		op3 = exp->exp.expression.oper3;
	}
	/* only reached if we are dealing with a constant expression,
	 * and that means that we can play around with the operands */
	switch(exp->exp_type)	{
		case AST_EXP_UNARY_NOT:
			exp->exp.constant.val.integer = !ASTP_expr_integer_value(op1);
			break;
		case AST_EXP_UNARY_TILDE:
			exp->exp.constant.val.integer = ~ASTP_expr_integer_value(op1);
			break;
		case AST_EXP_UNARY_PLUS: /* why this? I can't remember what I was thinking */
			exp->exp.constant.val.integer = +ASTP_expr_integer_value(op1);
			break;
		case AST_EXP_UNARY_MINUS:
			exp->exp.constant.val.integer = -ASTP_expr_integer_value(op1);
			break;
		case AST_EXP_UNARY_STAR:
			return false;
		case AST_EXP_BINARY_PERCENT:
			val = ASTP_expr_integer_value(op2);
			if (val == 0)
				log_error(nidl_yylineno, NIDL_INTDIVBY0, NULL);
			else
				val = ASTP_expr_integer_value(op1) % val;
			exp->exp.constant.val.integer = val;
			break;
		case AST_EXP_BINARY_SLASH:
			val = ASTP_expr_integer_value(op2);
			if (val == 0)
				log_error(nidl_yylineno, NIDL_INTDIVBY0, NULL);
			else
				val = ASTP_expr_integer_value(op1) / val;
			exp->exp.constant.val.integer = val;
			break;
		case AST_EXP_BINARY_STAR:
			val1 = ASTP_expr_integer_value(op1);
			val2 = ASTP_expr_integer_value(op2);
			val = val1 * val2;
			if (val < val1 && val > val2)
				log_error(nidl_yylineno, NIDL_INTOVERFLOW, KEYWORDS_lookup_text(LONG_KW), NULL);
			exp->exp.constant.val.integer = val;
			break;
		case AST_EXP_BINARY_MINUS:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) - ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_BINARY_PLUS:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) + ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_BINARY_RSHIFT:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) >> ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_BINARY_LSHIFT:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) << ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_BINARY_GE:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) >= ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_BINARY_LE:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) <= ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_BINARY_GT:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) > ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_BINARY_LT:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) < ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_BINARY_NE:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) != ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_BINARY_EQUAL:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) == ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_BINARY_AND:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) & ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_BINARY_OR:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) | ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_BINARY_XOR:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) ^ ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_BINARY_LOG_AND:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) && ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_BINARY_LOG_OR:
			exp->exp.constant.val.integer = ASTP_expr_integer_value(op1) || ASTP_expr_integer_value(op2);
			break;
		case AST_EXP_TERNARY_OP:
			/* we want to preserve the type for this, so we short-circuit the return */
			*exp = ASTP_expr_integer_value(op1) ? *op2 : *op3;
			return true;
		default:
			/* NOTREACHED (hopefully!) */
			break;
	}
	ASTP_free_exp(op1);
	ASTP_free_exp(op2);
	ASTP_free_exp(op3);
	exp->exp_type = AST_EXP_CONSTANT;
	exp->exp.constant.type = AST_int_const_k;
	return true;
}


