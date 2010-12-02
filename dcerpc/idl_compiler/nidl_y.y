%{
/*
 *
 *  (c) Copyright 1989 OPEN SOFTWARE FOUNDATION, INC.
 *  (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 *  (c) Copyright 1989 DIGITAL EQUIPMENT CORPORATION
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *                  permission to use, copy, modify, and distribute this
 *  file for any purpose is hereby granted without fee, provided that
 *  the above copyright notices and this notice appears in all source
 *  code copies, and that none of the names of Open Software
 *  Foundation, Inc., Hewlett-Packard Company, or Digital Equipment
 *  Corporation be used in advertising or publicity pertaining to
 *  distribution of the software without specific, written prior
 *  permission.  Neither Open Software Foundation, Inc., Hewlett-
 *  Packard Company, nor Digital Equipment Corporation makes any
 *  representations about the suitability of this software for any
 *  purpose.
 *
 */
/*
**
**  NAME:
**
**      IDL.Y
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      This module defines the main IDL grammar accepted
**      by the IDL compiler.
**
**  VERSION: DCE 1.0
**
*/

#ifdef vms
#  include <types.h>
#else
#  include <sys/types.h>
#endif

#include <nidl.h>
#include <nametbl.h>
#include <errors.h>
#include <ast.h>
#include <astp.h>
#include <frontend.h>
#include <flex_bison_support.h>

extern int nidl_yylineno;
extern boolean search_attributes_table ;

int yyparse(void);
int yylex(void);

/*
**  Local cells used for inter-production communication
*/
static ASTP_attr_k_t       ASTP_bound_type;    /* Array bound attribute */

%}


        /*   Declaration of yylval, yyval                   */
%union
{
	 NAMETABLE_id_t         y_id ;          /* Identifier           */
	 long                   y_ptrlevels;	/* levels of * for pointers */
	 long					y_ptrclass;		/* class of pointer */
	 STRTAB_str_t           y_string ;      /* String               */
	 STRTAB_str_t           y_float ;       /* Float constant       */
	 AST_export_n_t*        y_export ;      /* an export node       */
	 AST_import_n_t*        y_import ;      /* Import node          */
	 AST_exception_n_t*     y_exception ;   /* Exception node       */
	 AST_constant_n_t*      y_constant;     /* Constant node        */
	 AST_parameter_n_t*     y_parameter ;   /* Parameter node       */
	 AST_type_n_t*          y_type ;        /* Type node            */
	 AST_type_p_n_t*        y_type_ptr ;    /* Type pointer node    */
	 AST_field_n_t*         y_field ;       /* Field node           */
	 AST_arm_n_t*           y_arm ;         /* Union variant arm    */
	 AST_operation_n_t*     y_operation ;   /* Routine node         */
	 AST_interface_n_t*     y_interface ;   /* Interface node       */
	 AST_case_label_n_t*    y_label ;       /* Union tags           */
	 ASTP_declarator_n_t*   y_declarator ;  /* Declarator info      */
	 ASTP_array_index_n_t*  y_index ;       /* Array index info     */
	 nidl_uuid_t            y_uuid ;        /* Universal UID        */
	 char                   y_char;         /* character constant   */
	 ASTP_attributes_t      y_attributes;   /* attributes flags     */

     	 AST_cpp_quote_n_t*     y_cpp_quote;    /* Quoted C within interface treated as one 'kind' of export node + quote outside interfaces */
	 

	 struct {
		  long            int_val ;        /* Integer constant     */
		  AST_type_k_t    int_size;
		  int             int_signed;
	 }                  y_int_info;     /* int size and signedness */
	 AST_exp_n_t           * y_exp;          /* constant expression info */
}

%{
#if YYDEBUG
extern char const *current_file;
static void yyprint(FILE * stream, int token, YYSTYPE lval)	{
	fprintf(stream, " %s:%d", current_file, *yylineno_p);
}
#define YYPRINT yyprint
#endif


%}

/********************************************************************/
/*                                                                  */
/*          Tokens used by the IDL parser.                          */
/*                                                                  */
/********************************************************************/


/* Keywords                 */
%token ALIGN_KW
%token BYTE_KW
%token CHAR_KW
%token CONST_KW
%token DEFAULT_KW
%token ENUM_KW
%token EXCEPTIONS_KW
%token FLOAT_KW
%token HYPER_KW
%token INT_KW
%token INTERFACE_KW
%token IMPORT_KW
%token LIBRARY_KW
%token LONG_KW
%token PIPE_KW
%token REF_KW
%token SMALL_KW
%token STRUCT_KW
%token TYPEDEF_KW
%token UNION_KW
%token UNSIGNED_KW
%token SHORT_KW
%token VOID_KW
%token DOUBLE_KW
%token BOOLEAN_KW
%token CASE_KW
%token SWITCH_KW
%token HANDLE_T_KW
%token TRUE_KW
%token FALSE_KW
%token NULL_KW
%token BROADCAST_KW
%token COMM_STATUS_KW
%token CONTEXT_HANDLE_KW
%token FIRST_IS_KW
%token HANDLE_KW
%token IDEMPOTENT_KW
%token IGNORE_KW
%token CALL_AS_KW
%token IID_IS_KW
%token IMPLICIT_HANDLE_KW
%token IN_KW
%token LAST_IS_KW
%token LENGTH_IS_KW
%token LOCAL_KW
%token MAX_IS_KW
%token MAYBE_KW
%token MIN_IS_KW
%token MUTABLE_KW
%token OUT_KW
%token OBJECT_KW
%token POINTER_DEFAULT_KW
%token ENDPOINT_KW
%token PTR_KW
%token RANGE_KW
%token REFLECT_DELETIONS_KW
%token REMOTE_KW
%token SECURE_KW
%token SHAPE_KW
%token SIZE_IS_KW
%token STRING_KW
%token SWITCH_IS_KW
%token SWITCH_TYPE_KW
%token TRANSMIT_AS_KW
%token UNIQUE_KW
%token UUID_KW
%token VERSION_KW
%token V1_ARRAY_KW
%token V1_STRING_KW
%token V1_ENUM_KW
%token V1_STRUCT_KW

/* Added by Centeris */

%token CPP_QUOTE_KW

/*  Non-keyword tokens      */

%token UUID_REP


/*  Punctuation             */

%token COLON
%token COMMA
%token DOTDOT
%token EQUAL
%token LBRACE
%token LBRACKET
%token LPAREN
%token RBRACE
%token RBRACKET
%token RPAREN
%token SEMI
%token STAR
%token QUESTION
%token BAR
%token BARBAR
%token LANGLE
%token LANGLEANGLE
%token RANGLE
%token RANGLEANGLE
%token AMP
%token AMPAMP
%token LESSEQUAL
%token GREATEREQUAL
%token EQUALEQUAL
%token CARET
%token PLUS
%token MINUS
%token NOT
%token NOTEQUAL
%token SLASH
%token PERCENT
%token TILDE
%token POUND
%token UNKNOWN  /* Something that doesn't fit in any other token class */

/*  Tokens setting yylval   */

%token <y_id>      		IDENTIFIER
%token <y_string>  		STRING
%token <y_int_info>		INTEGER_NUMERIC
%token <y_char>			CHAR
%token <y_float>		FLOAT_NUMERIC
%start grammar_start

%%

/********************************************************************/
/*                                                                  */
/*          Syntax description and actions for IDL                  */
/*                                                                  */
/********************************************************************/

grammar_start:     	interfaces cpp_quotes
			{
				global_cppquotes_post = (AST_cpp_quote_n_t*)AST_concat_element(
					(ASTP_node_t*)global_cppquotes_post, (ASTP_node_t*)$<y_cpp_quote>2);
			}
			|
			optional_imports_cppquotes 
			|  
        		{
            			$<y_import>$ = (AST_import_n_t *)NULL;
		        }

			/*{
#if 0
	 			global_imports = (AST_import_n_t*)AST_concat_element(
	 				 (ASTP_node_t*)global_imports, (ASTP_node_t*)$<y_import>1);
#endif
				global_cppquotes_post = (AST_cpp_quote_n_t*)AST_concat_element(
					(ASTP_node_t*)global_cppquotes_post, (ASTP_node_t*)$<y_cpp_quote>2);
			}*/
		;


interfaces:
	interfaces interface_plus
	|
	interface_plus	
	;


/*Centeris wfu*/
cpp_quotes:
	cpp_quote cpp_quotes   
	{        		
		$<y_cpp_quote>$ = (AST_cpp_quote_n_t *) AST_concat_element(
                                                (ASTP_node_t *) $<y_cpp_quote>1,
                                                (ASTP_node_t *) $<y_cpp_quote>2);				
        }
	| /*Nothing*/
	{	$<y_cpp_quote>$ = (AST_cpp_quote_n_t *)NULL;		
	}	
	;
    

interface_plus:
	cpp_quotes interface
	{
		global_cppquotes = (AST_cpp_quote_n_t*)AST_concat_element(
			(ASTP_node_t*)global_cppquotes, (ASTP_node_t*)$<y_cpp_quote>1);		
	}	
	;


interface:	
        interface_init interface_start interface_ancestor interface_tail
        {
            AST_finish_interface_node(the_interface);
        }
	;



interface_start:
        interface_attributes INTERFACE_KW IDENTIFIER
        {
	    AST_type_n_t * interface_type = AST_type_node(AST_interface_k);
	    interface_type->type_structure.interface = the_interface;
	    interface_type->name = $<y_id>3;
            the_interface->name = $<y_id>3;
            ASTP_add_name_binding (the_interface->name, interface_type);
        }
    ;

interface_ancestor:
	/* Nothing */
	{
		 the_interface->inherited_interface_name = NAMETABLE_NIL_ID;
	}
	 | COLON IDENTIFIER
	{
		 the_interface->inherited_interface_name = $<y_id>2;
	}
	;

interface_init:
        /* Always create the interface node and auto-import the system idl */		
        {
            STRTAB_str_t nidl_idl_str;
            nidl_idl_str = STRTAB_add_string (AUTO_IMPORT_FILE);
            AST_interface_n_t* old = the_interface;

	    the_interface = AST_interface_node();			  		
            the_interface->prev = old;
	    the_interface->exports = NULL;
            the_interface->imports = AST_import_node(nidl_idl_str);
            the_interface->imports->interface = FE_parse_import (nidl_idl_str);
            if (the_interface->imports->interface != NULL)
            {
                AST_CLR_OUT_OF_LINE(the_interface->imports->interface);
                AST_SET_IN_LINE(the_interface->imports->interface);
            }
        }
    ;



interface_tail:
        LBRACE interface_body RBRACE
        { $<y_interface>$ = $<y_interface>2; }
    |   error
        {
            $<y_interface>$ = NULL;
            log_error(nidl_yylineno,NIDL_MISSONINTER, NULL);
        }
    |   error RBRACE
        {
            $<y_interface>$ = NULL;
        }
    ;


interface_body:
        optional_imports exports extraneous_semi
        {
            /* May already be an import of nbase, so concat */
            the_interface->imports = (AST_import_n_t *) AST_concat_element(
                                        (ASTP_node_t *) the_interface->imports,
                                        (ASTP_node_t *) $<y_import>1);
            the_interface->exports = (AST_export_n_t*)AST_concat_element(
			(ASTP_node_t*)the_interface->exports,
			(ASTP_node_t*)$<y_export>2);
        }
  ;

optional_imports:
        imports 
    |   /* Nothing */
        {
            $<y_import>$ = (AST_import_n_t *)NULL;
        }
    ;

optional_imports_cppquotes:
        imports cpp_quotes
	{
#if 0
	 			global_imports = (AST_import_n_t*)AST_concat_element(
	 				 (ASTP_node_t*)global_imports, (ASTP_node_t*)$<y_import>1);
#endif

				global_cppquotes_post = (AST_cpp_quote_n_t*)AST_concat_element(
					(ASTP_node_t*)global_cppquotes_post, (ASTP_node_t*)$<y_cpp_quote>2);
	}    	
	
    ;

imports:
        import
    |   imports import
        {
                $<y_import>$ = (AST_import_n_t *) AST_concat_element(
                                                (ASTP_node_t *) $<y_import>1,
                                                (ASTP_node_t *) $<y_import>2);
        }
    ;

import:
        IMPORT_KW error
        {
            $<y_import>$ = (AST_import_n_t *)NULL;
        }
    |   IMPORT_KW error SEMI
        {
            $<y_import>$ = (AST_import_n_t *)NULL;
        }
    |   IMPORT_KW import_files SEMI
        {
            $<y_import>$ = $<y_import>2;
        }
    ;

import_files:
        import_file
    |   import_files COMMA import_file
        {
                $<y_import>$ = (AST_import_n_t *) AST_concat_element(
                                                (ASTP_node_t *) $<y_import>1,
                                                (ASTP_node_t *) $<y_import>3);
        }
    ;



import_file:
        STRING
        {
            AST_interface_n_t  *int_p;
            int_p = FE_parse_import ($<y_string>1);
            if (int_p != (AST_interface_n_t *)NULL)
            {
                $<y_import>$ = AST_import_node($<y_string>1);
                $<y_import>$->interface = int_p;
            }
            else
                $<y_import>$ = (AST_import_n_t *)NULL;
        }
    ;

exports:
        export
    |   exports extraneous_semi export
        {
                $<y_export>$ = (AST_export_n_t *) AST_concat_element(
                                            (ASTP_node_t *) $<y_export>1,
                                            (ASTP_node_t *) $<y_export>3) ;
        }
    ;


export:
        type_dcl      SEMI
        {
                $<y_export>$ = AST_types_to_exports ($<y_type_ptr>1);
        }
    |   const_dcl     SEMI
        {
                $<y_export>$ = AST_export_node (
                        (ASTP_node_t *) $<y_constant>1, AST_constant_k);
        }
    |   operation_dcl SEMI
        {
            if (ASTP_parsing_main_idl)
                $<y_export>$ = AST_export_node (
                        (ASTP_node_t *) $<y_operation>1, AST_operation_k);
        }
    |   cpp_quote
        {
            $<y_export>$ = AST_export_node (
                (ASTP_node_t *) $<y_cpp_quote>1, AST_cpp_quote_k);
        }
    |   error SEMI
        {
            $<y_export>$ = (AST_export_n_t *)NULL;
        }
    ;

cpp_quote:
        CPP_QUOTE_KW LPAREN STRING RPAREN
        {
        	$<y_cpp_quote>$ = AST_cpp_quote_node($<y_string>3);	
		
        }    
	;

const_dcl:
        CONST_KW type_spec declarator EQUAL const_exp

        {
           $<y_constant>$ = AST_finish_constant_node ($<y_constant>5,
                                        $<y_declarator>3, $<y_type>2);
        }
    ;


const_exp:  expression
        {
				$<y_constant>$ = AST_constant_from_exp($<y_exp>1);
				if ($<y_constant>$ == NULL)	{
					 log_error(nidl_yylineno, NIDL_EXPNOTCONST, NULL);
				}
        }
    ;


type_dcl:
        TYPEDEF_KW type_declarator
        {
            $<y_type_ptr>$ = $<y_type_ptr>2;
        }
    ;

type_declarator:
        attributes type_spec declarators extraneous_comma
        {
            $<y_type_ptr>$  = AST_declarators_to_types(the_interface, $<y_type>2,
                        $<y_declarator>3, &$<y_attributes>1) ;
            ASTP_free_simple_list((ASTP_node_t *)$<y_attributes>1.bounds);
        }
    ;


type_spec:
        simple_type_spec
    |   constructed_type_spec
    ;

simple_type_spec:
        floating_point_type_spec
    |   integer_type_spec
    |   char_type_spec
    |   boolean_type_spec
    |   byte_type_spec
    |   void_type_spec
    |   named_type_spec
    |   handle_type_spec
    ;


constructed_type_spec:
        struct_type_spec
    |   union_type_spec
    |   enum_type_spec
    |   pipe_type_spec
    ;


named_type_spec:
        IDENTIFIER
        {
            $<y_type>$ = AST_lookup_named_type($<y_id>1);
        }
    ;

floating_point_type_spec:
        FLOAT_KW
        {
            $<y_type>$ = AST_lookup_type_node(AST_short_float_k);
        }
    |   DOUBLE_KW
        {
            $<y_type>$ = AST_lookup_type_node(AST_long_float_k);
        }
    ;

extraneous_comma:
        /* Nothing */
    |   COMMA
        { log_warning(nidl_yylineno, NIDL_EXTRAPUNCT, ",", NULL);}
    ;

extraneous_semi:
        /* Nothing */
    |   SEMI
        { log_warning(nidl_yylineno, NIDL_EXTRAPUNCT, ";", NULL);}
    ;

optional_unsigned_kw:
        UNSIGNED_KW     { $<y_int_info>$.int_signed = false; }
    |   /* Nothing */   { $<y_int_info>$.int_signed = true; }
    ;

integer_size_spec:
        SMALL_KW
        {
            $<y_int_info>$.int_size = AST_small_integer_k;
            $<y_int_info>$.int_signed = true;
        }
    |   SHORT_KW
        {
            $<y_int_info>$.int_size = AST_short_integer_k;
            $<y_int_info>$.int_signed = true;
        }
    |   LONG_KW
        {
            $<y_int_info>$.int_size = AST_long_integer_k;
            $<y_int_info>$.int_signed = true;
        }
    |   HYPER_KW
        {
            $<y_int_info>$.int_size = AST_hyper_integer_k;
            $<y_int_info>$.int_signed = true;
        }
    ;

integer_modifiers:
        integer_size_spec
        { $<y_int_info>$ = $<y_int_info>1; }
    |   UNSIGNED_KW integer_size_spec
        {
            $<y_int_info>$.int_size = $<y_int_info>2.int_size;
            $<y_int_info>$.int_signed = false;
        }
    |   integer_size_spec UNSIGNED_KW
        {
            $<y_int_info>$.int_size = $<y_int_info>1.int_size;
            $<y_int_info>$.int_signed = false;
        }
    ;

integer_type_spec:
        integer_modifiers
        { $<y_type>$ = AST_lookup_integer_type_node($<y_int_info>1.int_size,$<y_int_info>1.int_signed); }
    |   integer_modifiers INT_KW
        { $<y_type>$ = AST_lookup_integer_type_node($<y_int_info>1.int_size,$<y_int_info>1.int_signed); }
    |   optional_unsigned_kw INT_KW
        {
            log_warning(nidl_yylineno,NIDL_INTSIZEREQ, NULL);
            $<y_type>$ = AST_lookup_integer_type_node(AST_long_integer_k,$<y_int_info>1.int_signed);
        }
    ;

char_type_spec:
        optional_unsigned_kw CHAR_KW
        { $<y_type>$ = AST_lookup_type_node(AST_character_k); }
    ;

boolean_type_spec:
        BOOLEAN_KW
        { $<y_type>$ = AST_lookup_type_node(AST_boolean_k); }
    ;

byte_type_spec:
        BYTE_KW
        { $<y_type>$ = AST_lookup_type_node(AST_byte_k); }
    ;

void_type_spec:
        VOID_KW
        { $<y_type>$ = AST_lookup_type_node(AST_void_k); }
    ;

handle_type_spec:
       HANDLE_T_KW
        { $<y_type>$ = AST_lookup_type_node(AST_handle_k); }
    ;

push_name_space:
        LBRACE
        {
            NAMETABLE_push_level ();
        }
    ;

pop_name_space:
        RBRACE
        {
            ASTP_patch_field_reference ();
            NAMETABLE_pop_level ();
        }
    ;

union_type_spec:
        UNION_KW ne_union_body
        {
        $<y_type>$ = AST_disc_union_node(
                         NAMETABLE_NIL_ID,      /* tag name          */
                         NAMETABLE_NIL_ID,      /* union name        */
                         NAMETABLE_NIL_ID,      /* discriminant name */
                         NULL,                  /* discriminant type */
                         $<y_arm>2 );           /* the arm list      */
        }
    |
        UNION_KW SWITCH_KW LPAREN simple_type_spec IDENTIFIER RPAREN union_body
        {
        $<y_type>$ = AST_disc_union_node(
                         NAMETABLE_NIL_ID,      /* tag name          */
                         ASTP_tagged_union_id,  /* union name        */
                         $<y_id>5,              /* discriminant name */
                         $<y_type>4,            /* discriminant type */
                         $<y_arm>7 );           /* the arm list      */
        }
    |   UNION_KW IDENTIFIER ne_union_body
        {
        $<y_type>$ = AST_disc_union_node(
                         $<y_id>2,              /* tag name          */
                         NAMETABLE_NIL_ID,      /* union name        */
                         NAMETABLE_NIL_ID,      /* discriminant name */
                         NULL,                  /* discriminant type */
                         $<y_arm>3 );           /* the arm list      */
        }
    |   UNION_KW SWITCH_KW LPAREN simple_type_spec IDENTIFIER RPAREN IDENTIFIER union_body
        {
        $<y_type>$ = AST_disc_union_node(
                         NAMETABLE_NIL_ID,      /* tag name          */
                         $<y_id>7,              /* union name        */
                         $<y_id>5,              /* discriminant name */
                         $<y_type>4,            /* discriminant type */
                         $<y_arm>8 );           /* the arm list      */
        }
    |   UNION_KW IDENTIFIER SWITCH_KW LPAREN simple_type_spec IDENTIFIER RPAREN union_body
        {
        $<y_type>$ = AST_disc_union_node(
                         $<y_id>2,              /* tag name          */
                         ASTP_tagged_union_id,  /* union name        */
                         $<y_id>6,              /* discriminant name */
                         $<y_type>5,            /* discriminant type */
                         $<y_arm>8 );           /* the arm list      */
        }
    |   UNION_KW IDENTIFIER SWITCH_KW LPAREN simple_type_spec IDENTIFIER RPAREN IDENTIFIER union_body
        {
        $<y_type>$ = AST_disc_union_node(
                         $<y_id>2,              /* tag name          */
                         $<y_id>8,              /* union name        */
                         $<y_id>6,              /* discriminant name */
                         $<y_type>5,            /* discriminant type */
                         $<y_arm>9 );           /* the arm list      */
        }
    |   UNION_KW IDENTIFIER
        {
            $<y_type>$ = AST_type_from_tag (AST_disc_union_k, $<y_id>2);
        }
    ;

ne_union_body:
        push_name_space ne_union_cases pop_name_space
        {
                $<y_arm>$ = $<y_arm>2;
        }
    ;
union_body:
        push_name_space union_cases pop_name_space
        {
                $<y_arm>$ = $<y_arm>2;
        }
    ;

ne_union_cases:
        ne_union_case
    |   ne_union_cases extraneous_semi ne_union_case
        {
            $<y_arm>$ = (AST_arm_n_t *) AST_concat_element(
                                        (ASTP_node_t *) $<y_arm>1,
                                        (ASTP_node_t *) $<y_arm>3);
        }
    ;
union_cases:
        union_case
    |   union_cases extraneous_semi union_case
        {
            $<y_arm>$ = (AST_arm_n_t *) AST_concat_element(
                                        (ASTP_node_t *) $<y_arm>1,
                                        (ASTP_node_t *) $<y_arm>3);
        }
    ;

ne_union_case:
        ne_union_member
        {
            $<y_arm>$ = $<y_arm>1;
        }
    ;
union_case:
        union_case_list union_member
        {
            $<y_arm>$ = AST_label_arm($<y_arm>2, $<y_label>1) ;
        }
    ;

ne_union_case_list:
        ne_union_case_label
    |   ne_union_case_list COMMA ne_union_case_label
        {
            $<y_label>$ = (AST_case_label_n_t *) AST_concat_element(
                                        (ASTP_node_t *) $<y_label>1,
                                        (ASTP_node_t *) $<y_label>3);
        }
    ;
union_case_list:
        union_case_label
    |   union_case_list union_case_label
        {
            $<y_label>$ = (AST_case_label_n_t *) AST_concat_element(
                                        (ASTP_node_t *) $<y_label>1,
                                        (ASTP_node_t *) $<y_label>2);
        }
    ;

ne_union_case_label:
        const_exp
        {
            $<y_label>$ = AST_case_label_node($<y_constant>1);
        }
    ;
union_case_label:
        CASE_KW const_exp COLON
        {
            $<y_label>$ = AST_case_label_node($<y_constant>2);
        }
    |   DEFAULT_KW COLON
        {
            $<y_label>$ = AST_default_case_label_node();
        }
    ;

ne_union_member:
        attribute_opener rest_of_attribute_list SEMI
        {
            $<y_arm>$ = AST_declarator_to_arm(NULL, NULL, &$<y_attributes>2);
            ASTP_free_simple_list((ASTP_node_t *)$<y_attributes>2.bounds);
        }
    |   attribute_opener rest_of_attribute_list type_spec declarator SEMI
        {
            $<y_arm>$ = AST_declarator_to_arm($<y_type>3,
                                $<y_declarator>4, &$<y_attributes>2);
            ASTP_free_simple_list((ASTP_node_t *)$<y_attributes>2.bounds);
        }
    ;
union_member:
        /* nothing */ SEMI
        {
            $<y_arm>$ = AST_arm_node(NAMETABLE_NIL_ID,NULL,NULL);
        }
    |   attributes type_spec declarator SEMI
        {
            if (ASTP_TEST_ATTR(&$<y_attributes>1, ASTP_CASE))
            {
                ASTP_attr_flag_t attr1 = ASTP_CASE;
                log_error(nidl_yylineno, NIDL_EUMEMATTR,
                      KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)),
		      NULL);
            }
            if (ASTP_TEST_ATTR(&$<y_attributes>1, ASTP_DEFAULT))
            {
                ASTP_attr_flag_t attr1 = ASTP_DEFAULT;
                log_error(nidl_yylineno, NIDL_EUMEMATTR,
                      KEYWORDS_lookup_text(AST_attribute_to_token(&attr1)),
		      NULL);
            }
            $<y_arm>$ = AST_declarator_to_arm($<y_type>2,
                                $<y_declarator>3, &$<y_attributes>1);
            ASTP_free_simple_list((ASTP_node_t *)$<y_attributes>1.bounds);
        }
    ;

struct_type_spec:
        STRUCT_KW push_name_space member_list pop_name_space
        {
            $<y_type>$ = AST_structure_node($<y_field>3, NAMETABLE_NIL_ID) ;
        }
    |   STRUCT_KW IDENTIFIER push_name_space member_list pop_name_space
        {
            $<y_type>$ = AST_structure_node($<y_field>4, $<y_id>2) ;
        }
    |   STRUCT_KW IDENTIFIER
        {
            $<y_type>$ = AST_type_from_tag (AST_structure_k, $<y_id>2);
        }
    ;

member_list:
        member
    |   member_list extraneous_semi member
        {
            $<y_field>$ = (AST_field_n_t *)AST_concat_element(
                                    (ASTP_node_t *) $<y_field>1,
                                    (ASTP_node_t *) $<y_field>3) ;
        }
    ;

member:
        attributes type_spec old_attribute_syntax declarators SEMI
        {
            $<y_field>$ = AST_declarators_to_fields($<y_declarator>4,
                                                    $<y_type>2,
                                                    &$<y_attributes>1);
            ASTP_free_simple_list((ASTP_node_t *)$<y_attributes>1.bounds);
        }
    ;

enum_type_spec:
        ENUM_KW optional_tag enum_body
        {
             $<y_type>$ = AST_enumerator_node($<y_constant>3, AST_short_integer_k);
        }
    ;

optional_tag:
	IDENTIFIER
		{
		}
	| /* Nothing */
	;

enum_body:
        LBRACE enum_ids extraneous_comma RBRACE
        {
            $<y_constant>$ = $<y_constant>2 ;
        }
    ;

enum_ids:
        enum_id
    |   enum_ids COMMA enum_id
        {
            $<y_constant>$ = (AST_constant_n_t *) AST_concat_element(
                                    (ASTP_node_t *) $<y_constant>1,
                                    (ASTP_node_t *) $<y_constant>3) ;
        }
    ;

enum_id:
        IDENTIFIER optional_value
        {
            $<y_constant>$ = AST_enum_constant($<y_id>1, $<y_exp>2) ;
        }
    ;

pipe_type_spec:
        PIPE_KW type_spec
        {
            $<y_type>$ = AST_pipe_node ($<y_type>2);
        }
    ;

optional_value:
	/* Nothing */
		{
			 $<y_exp>$ = AST_exp_integer_constant(0, true);
		}
	| EQUAL expression
		{
	 		 ASTP_validate_integer($<y_exp>2);
			 $<y_exp>$ = $<y_exp>2;
		}
	;

declarators:
        declarator
		  {
				$<y_declarator>$ =  $<y_declarator>1;
		  }
    |   declarators COMMA declarator
        {
            $<y_declarator>$ = (ASTP_declarator_n_t *) AST_concat_element(
                                            (ASTP_node_t *) $<y_declarator>1,
                                            (ASTP_node_t *) $<y_declarator>3) ;
        }
    ;



declarator:
	declarator1
		{ $<y_declarator>$ = $<y_declarator>1; }
		;

declarator1:
        direct_declarator
            { $<y_declarator>$ = $<y_declarator>1; }
       |    pointer direct_declarator
            {
                $<y_declarator>$ = $<y_declarator>2;
                AST_declarator_operation($<y_declarator>$, AST_pointer_k,
                        (ASTP_node_t *)NULL, $<y_ptrlevels>1 );
            };


pointer :
            STAR
            { $<y_ptrlevels>$ = 1;}
       |    STAR pointer
            { $<y_ptrlevels>$ = $<y_ptrlevels>2 + 1; };


direct_declarator:
            IDENTIFIER
            { $<y_declarator>$ = AST_declarator_node ( $<y_id>1 ); }
       |        direct_declarator array_bounds
            {
                $<y_declarator>$ = $<y_declarator>$;
                AST_declarator_operation($<y_declarator>$, AST_array_k,
                        (ASTP_node_t *) $<y_index>2, 0 );
            }
       |   LPAREN declarator RPAREN
            {
            $<y_declarator>$ = $<y_declarator>2;
            }
       |        direct_declarator parameter_dcls
            {
                $<y_declarator>$ = $<y_declarator>$;
                AST_declarator_operation($<y_declarator>$, AST_function_k,
                        (ASTP_node_t *) $<y_parameter>2, 0 );
            }
       ;


    /*
     * The following productions use an AST routine with the signature:
     *
     *   ASTP_array_index_node( AST_constant_n_t * lower_bound,
     *                          ASTP_bound_t lower_bound_type,
     *                          AST_constant_n_t * upper_bound,
     *                          ASTP_bound_t upper_bound_type);
     *
     * The type ASTP_bound_t is defined as:
     *
     *   typedef enum {ASTP_constant_bound,
     *                 ASTP_default_bound,
     *                 ASTP_open_bound} ASTP_bound_t;
     *
     * The bound value passed is only used if the associated bound type is
     * ASTP_constant_bound.
     */

array_bounds:
        LBRACKET RBRACKET
        {
            $<y_index>$ = ASTP_array_index_node ( NULL, ASTP_default_bound,
                                                 NULL, ASTP_open_bound);
        }
    |   LBRACKET STAR RBRACKET
        {
            $<y_index>$ = ASTP_array_index_node  ( NULL, ASTP_default_bound,
                                                 NULL, ASTP_open_bound);
        }
    |   LBRACKET const_exp RBRACKET
        {
            $<y_index>$ = ASTP_array_index_node  ( NULL, ASTP_default_bound,
                                                 $<y_constant>2, ASTP_constant_bound);
        }
    |   LBRACKET STAR DOTDOT STAR RBRACKET
        {
            $<y_index>$ = ASTP_array_index_node  ( NULL, ASTP_open_bound,
                                                 NULL, ASTP_open_bound);
        }
    |   LBRACKET STAR DOTDOT const_exp RBRACKET
        {
            $<y_index>$ = ASTP_array_index_node  ( NULL, ASTP_open_bound,
                                                 $<y_constant>4, ASTP_constant_bound);
        }
    |   LBRACKET const_exp DOTDOT STAR RBRACKET
        {
            $<y_index>$ = ASTP_array_index_node  ( $<y_constant>2, ASTP_constant_bound,
                                                 NULL, ASTP_open_bound);
        }
    |   LBRACKET const_exp DOTDOT const_exp RBRACKET
        {
            $<y_index>$ = ASTP_array_index_node  ( $<y_constant>2, ASTP_constant_bound,
                                                 $<y_constant>4, ASTP_constant_bound);
        }
    ;


operation_dcl:
        attributes type_spec declarators extraneous_comma
        {
            if (ASTP_parsing_main_idl)
                $<y_operation>$ = AST_operation_node (
                                    $<y_type>2,         /*The type node*/
                                    $<y_declarator>3,   /* Declarator list */
                                   &$<y_attributes>1);  /* attributes */
            ASTP_free_simple_list((ASTP_node_t *)$<y_attributes>1.bounds);
        }
    | error declarators
        {
        log_error(nidl_yylineno,NIDL_MISSONOP, NULL);
        $<y_operation>$ = NULL;
        }
    ;

parameter_dcls:
        param_names param_list extraneous_comma end_param_names
        {
            $<y_parameter>$ = $<y_parameter>2;
        }
    ;

param_names:
        LPAREN
        {
        NAMETABLE_push_level ();
        }
    ;

end_param_names:
        RPAREN
        {
        ASTP_patch_field_reference ();
        NAMETABLE_pop_level ();
        }
    ;

param_list:
        param_dcl
    |   param_list COMMA param_dcl
        {
            if (ASTP_parsing_main_idl)
                $<y_parameter>$ = (AST_parameter_n_t *) AST_concat_element(
                                    (ASTP_node_t *) $<y_parameter>1,
                                    (ASTP_node_t *) $<y_parameter>3);
        }
    |   /* nothing */
        {
            $<y_parameter>$ = (AST_parameter_n_t *)NULL;
        }
    ;

param_dcl:
        attributes type_spec old_attribute_syntax declarator_or_null
        {
            /*
             * We have to use special code here to allow (void) as a parameter
             * specification.  If there are no declarators, then we need to
             * make sure that the type is void and that there are no attributes .
             */
            if ($<y_declarator>4 == NULL)
            {
                /*
                 * If the type is not void or some attribute is specified,
                 * there is a syntax error.  Force a yacc error, so that
                 * we can safely recover from the lack of a declarator.
                 */
                if (($<y_type>2->kind != AST_void_k) ||
                   ($<y_attributes>1.bounds != NULL) ||
                   ($<y_attributes>1.attr_flags != 0))
                {
                    yywhere();  /* Issue a syntax error for this line */
                    YYERROR;    /* Allow natural error recovery */
                }

                $<y_parameter>$ = (AST_parameter_n_t *)NULL;
            }
            else
            {
                if (ASTP_parsing_main_idl)
                    $<y_parameter>$ = AST_declarator_to_param(
                                            &$<y_attributes>1,
                                            $<y_type>2,
                                            $<y_declarator>4);
            }
            ASTP_free_simple_list((ASTP_node_t *)$<y_attributes>1.bounds);
        }
    |    error old_attribute_syntax declarator_or_null
        {
            log_error(nidl_yylineno, NIDL_MISSONPARAM, NULL);
            $<y_parameter>$ = (AST_parameter_n_t *)NULL;
        }
    ;

declarator_or_null:
        declarator
        { $<y_declarator>$ = $<y_declarator>1; }
    |   /* nothing */
        { $<y_declarator>$ = NULL; }
    ;

/*
 * Attribute definitions
 *
 * Attributes may appear on types, fields, parameters, operations and
 * interfaces.  Thes productions must be used around attributes in order
 * for LEX to recognize attribute names as keywords instead of identifiers.
 * The bounds productions are used in attribute options (such
 * as size_is) because variable names the may look like attribute names
 * should be allowed.
 */

attribute_opener:
        LBRACKET
        {
            search_attributes_table = true;
        }
    ;

attribute_closer:
        RBRACKET
        {
            search_attributes_table = false;
        }
    ;

bounds_opener:
        LPAREN
        {
            search_attributes_table = false;
        }
    ;

bounds_closer:
        RPAREN
        {
            search_attributes_table = true;
        }
    ;


/*
 * Production to accept attributes in the old location, and issue a clear error that
 * the translator should be used.
 */
old_attribute_syntax:
        attributes
        {
            /* Give an error on notranslated sources */
            if (($<y_attributes>1.bounds != NULL) ||
               ($<y_attributes>1.attr_flags != 0))
            {
                log_error(nidl_yylineno,NIDL_ATTRTRANS, NULL);
                ASTP_free_simple_list((ASTP_node_t *)$<y_attributes>1.bounds);
            }
        }
    ;


/*
 * Interface Attributes
 *
 * Interface attributes are special--there is no cross between interface
 * attributes and other attributes (for instance on fields or types.
 */
interface_attributes:
        attribute_opener interface_attr_list extraneous_comma attribute_closer
    |   attribute_opener error attribute_closer
        {
            log_error(nidl_yylineno,NIDL_ERRINATTR, NULL);
        }

    |   /* Nothing */
    ;

interface_attr_list:
        interface_attr
    |   interface_attr_list COMMA interface_attr
    |   /* nothing */
    ;

interface_attr:
        UUID_KW error
        {
            log_error(nidl_yylineno,NIDL_SYNTAXUUID, NULL);
        }
    |   UUID_KW UUID_REP
        {
            {
                if (ASTP_IF_AF_SET(the_interface,ASTP_IF_UUID))
                        log_error(nidl_yylineno, NIDL_ATTRUSEMULT, NULL);
                ASTP_SET_IF_AF(the_interface,ASTP_IF_UUID);
                the_interface->uuid = $<y_uuid>2;
            }
        }
    |   ENDPOINT_KW LPAREN port_list extraneous_comma RPAREN
        {
            if (ASTP_IF_AF_SET(the_interface,ASTP_IF_PORT))
                    log_error(nidl_yylineno, NIDL_ATTRUSEMULT, NULL);
            ASTP_SET_IF_AF(the_interface,ASTP_IF_PORT);
        }
    |   EXCEPTIONS_KW LPAREN excep_list extraneous_comma RPAREN
        {
            if (ASTP_IF_AF_SET(the_interface, ASTP_IF_EXCEPTIONS))
                log_error(nidl_yylineno, NIDL_ATTRUSEMULT, NULL);
            ASTP_SET_IF_AF(the_interface, ASTP_IF_EXCEPTIONS);
        }
    |   VERSION_KW LPAREN version_number RPAREN
        {
            {
                if (ASTP_IF_AF_SET(the_interface,ASTP_IF_VERSION))
                        log_error(nidl_yylineno, NIDL_ATTRUSEMULT, NULL);
                ASTP_SET_IF_AF(the_interface,ASTP_IF_VERSION);
            }

        }
    |   LOCAL_KW
        {
            {
                if (AST_LOCAL_SET(the_interface))
                        log_warning(nidl_yylineno, NIDL_MULATTRDEF, NULL);
                AST_SET_LOCAL(the_interface);
            }
        }
    |   POINTER_DEFAULT_KW LPAREN pointer_class RPAREN
        {
            if (the_interface->pointer_default != 0)
                    log_error(nidl_yylineno, NIDL_ATTRUSEMULT, NULL);
            the_interface->pointer_default = $<y_ptrclass>3;
        }
	 /* extensions to osf */
	 |	  OBJECT_KW
	 		{
				if (AST_OBJECT_SET(the_interface))
					 log_warning(nidl_yylineno, NIDL_MULATTRDEF, NULL);
				AST_SET_OBJECT(the_interface);
			}
	 |		acf_interface_attr
	 		{
				/* complain about compat here */
			}
    ;

acf_interface_attr:
	IMPLICIT_HANDLE_KW LPAREN HANDLE_T_KW IDENTIFIER RPAREN
	{
		if (the_interface->implicit_handle_name != NAMETABLE_NIL_ID)
			 log_error(nidl_yylineno, NIDL_ATTRUSEMULT, NULL);

		ASTP_set_implicit_handle(the_interface, NAMETABLE_NIL_ID, $<y_id>4);
	}
	|
	IMPLICIT_HANDLE_KW LPAREN IDENTIFIER IDENTIFIER RPAREN
	{
		if (the_interface->implicit_handle_name != NAMETABLE_NIL_ID)
			log_error(nidl_yylineno, NIDL_ATTRUSEMULT, NULL);
	
		ASTP_set_implicit_handle(the_interface, $<y_id>3, $<y_id>4);
	}
	;

pointer_class:
        REF_KW { $<y_ptrclass>$ = ASTP_REF; }
    |   PTR_KW { $<y_ptrclass>$ = ASTP_PTR; }
    |   UNIQUE_KW { $<y_ptrclass>$ = ASTP_UNIQUE; }
    ;

version_number:
        INTEGER_NUMERIC
        {
            the_interface->version = $<y_int_info>1.int_val;
            if (the_interface->version > /*(unsigned int)*/ASTP_C_USHORT_MAX)
                log_error(nidl_yylineno, NIDL_MAJORTOOLARGE,
			  ASTP_C_USHORT_MAX, NULL);
        }
   |    FLOAT_NUMERIC
        {
            char const *float_text;
            unsigned int            major_version,minor_version;
            STRTAB_str_to_string($<y_string>1, &float_text);
            sscanf(float_text,"%d.%d",&major_version,&minor_version);
            if (major_version > (unsigned int)ASTP_C_USHORT_MAX)
                log_error(nidl_yylineno, NIDL_MAJORTOOLARGE,
			  ASTP_C_USHORT_MAX, NULL);
            if (minor_version > (unsigned int)ASTP_C_USHORT_MAX)
                log_error(nidl_yylineno, NIDL_MINORTOOLARGE,
			  ASTP_C_USHORT_MAX, NULL);
            the_interface->version = (minor_version * 65536) + major_version;
        }
    ;

port_list:
        port_spec
    |   port_list COMMA port_spec
    ;

excep_list:
        excep_spec
        {
            the_interface->exceptions = $<y_exception>1;
        }
    |   excep_list COMMA excep_spec
        {
            $<y_exception>$ = (AST_exception_n_t *) AST_concat_element(
                                (ASTP_node_t *) the_interface->exceptions,
                                (ASTP_node_t *) $<y_exception>3 );
        }
    ;

port_spec:
        STRING
        {
            ASTP_parse_port(the_interface,$<y_string>1);
        }
    ;

excep_spec:
        IDENTIFIER
        {
            if (ASTP_parsing_main_idl)
                $<y_exception>$ = AST_exception_node($<y_id>1);
            else
                $<y_exception>$ = NULL;
        }
    ;




/*
 * Attributes that can appear on fields or parameters. These are the array
 * bounds attributes, i.e., last_is and friends. They are handled differently
 * from any other attributes.
 */
fp_attribute:
        array_bound_type bounds_opener array_bound_id_list bounds_closer
        {
            $<y_attributes>$.bounds = $<y_attributes>3.bounds;
            $<y_attributes>$.attr_flags = 0;
        }
    |   neu_switch_type bounds_opener neu_switch_id bounds_closer
        {
            $<y_attributes>$.bounds = $<y_attributes>3.bounds;
            $<y_attributes>$.attr_flags = 0;
        }
    ;

array_bound_type:
        FIRST_IS_KW
        {
            ASTP_bound_type = first_is_k;
        }
    |   LAST_IS_KW
        {
            ASTP_bound_type = last_is_k;
        }
    |   LENGTH_IS_KW
        {
            ASTP_bound_type = length_is_k;
        }
    |   MAX_IS_KW
        {
            ASTP_bound_type = max_is_k;
        }
    |   MIN_IS_KW
        {
            ASTP_bound_type = min_is_k;
        }
    |   SIZE_IS_KW
        {
            ASTP_bound_type = size_is_k;
        }
    ;


array_bound_id_list:
        array_bound_id
    |   array_bound_id_list COMMA array_bound_id
        {
        $<y_attributes>$.bounds = (ASTP_type_attr_n_t *) AST_concat_element (
                                (ASTP_node_t*) $<y_attributes>1.bounds,
                                (ASTP_node_t*) $<y_attributes>3.bounds);
        }
    ;

/* expression conflicts with identifier here */
array_bound_id:
	  expression
			{
				 $<y_attributes>$.bounds = AST_array_bound_from_expr($<y_exp>1, ASTP_bound_type);
			}
    |   /* nothing */
        {
        $<y_attributes>$.bounds = AST_array_bound_info (NAMETABLE_NIL_ID, ASTP_bound_type, FALSE);
        }
    ;

neu_switch_type:
        SWITCH_IS_KW
        {
            ASTP_bound_type = switch_is_k;
        }
    ;

neu_switch_id:
        IDENTIFIER
        {
        $<y_attributes>$.bounds = AST_array_bound_info($<y_id>1, ASTP_bound_type, FALSE);
        }
    |   STAR IDENTIFIER
        {
        $<y_attributes>$.bounds = AST_array_bound_info($<y_id>2, ASTP_bound_type, TRUE);
        }
    ;

/*
 * Generalized Attribute processing
 */
attributes:
        attribute_opener rest_of_attribute_list
        { $<y_attributes>$ = $<y_attributes>2; }
     |
        /* nothing */
        {
        $<y_attributes>$.bounds = NULL;
        $<y_attributes>$.attr_flags = 0;
        }
     ;


rest_of_attribute_list:
        attribute_list extraneous_comma attribute_closer
     |  error attribute_closer
        {
        /*
         * Can't tell if we had any valid attributes in the list, so return
         * none.
         */
        $<y_attributes>$.bounds = NULL;
        $<y_attributes>$.attr_flags = 0;
        log_error(nidl_yylineno, NIDL_ERRINATTR, NULL);
        }
     |  error SEMI
        {
        /*
         * No closer to the attribute, so give a different message.
         */
        $<y_attributes>$.bounds = NULL;
        $<y_attributes>$.attr_flags = 0;
        log_error(nidl_yylineno, NIDL_MISSONATTR, NULL);
        search_attributes_table = false;
        }
     ;


attribute_list:
        attribute
        { $<y_attributes>$ = $<y_attributes>1; }
     |
        attribute_list COMMA attribute
        {
          /*
           * If the same bit has been specified more than once, then issue
           * a message.
           */
          if (($<y_attributes>1.attr_flags & $<y_attributes>3.attr_flags) != 0)
                log_warning(nidl_yylineno, NIDL_MULATTRDEF, NULL);
          $<y_attributes>$.attr_flags = $<y_attributes>1.attr_flags |
                                        $<y_attributes>3.attr_flags;
          $<y_attributes>$.bounds = (ASTP_type_attr_n_t *) AST_concat_element (
                                (ASTP_node_t*) $<y_attributes>1.bounds,
                                (ASTP_node_t*) $<y_attributes>3.bounds);
        }
     ;


attribute:
        /* bound attributes */
        fp_attribute            { $<y_attributes>$ = $<y_attributes>1; }

        /* Operation Attributes */
    |   BROADCAST_KW            { $<y_attributes>$.attr_flags = ASTP_BROADCAST;
                                  $<y_attributes>$.bounds = NULL;       }
    |   MAYBE_KW                { $<y_attributes>$.attr_flags = ASTP_MAYBE;
                                  $<y_attributes>$.bounds = NULL;       }
    |   IDEMPOTENT_KW           { $<y_attributes>$.attr_flags = ASTP_IDEMPOTENT;
                                  $<y_attributes>$.bounds = NULL;       }
    |   REFLECT_DELETIONS_KW    { $<y_attributes>$.attr_flags = ASTP_REFLECT_DELETIONS;
                                  $<y_attributes>$.bounds = NULL;       }
	 |   LOCAL_KW                { $<y_attributes>$.attr_flags = ASTP_LOCAL;
	                               $<y_attributes>$.bounds = NULL;       }
	 |   CALL_AS_KW LPAREN IDENTIFIER RPAREN	{	}

        /* Parameter-only Attributes */
    |   PTR_KW                  { $<y_attributes>$.attr_flags = ASTP_PTR;
                                  $<y_attributes>$.bounds = NULL;       }
    |   IN_KW                   { $<y_attributes>$.attr_flags = ASTP_IN;
                                  $<y_attributes>$.bounds = NULL;       }
    |   IN_KW LPAREN SHAPE_KW RPAREN
                                { $<y_attributes>$.attr_flags =
                                        ASTP_IN | ASTP_IN_SHAPE;
                                  $<y_attributes>$.bounds = NULL;       }
    |   OUT_KW                  { $<y_attributes>$.attr_flags = ASTP_OUT;
                                  $<y_attributes>$.bounds = NULL;       }
    |   OUT_KW LPAREN SHAPE_KW RPAREN
                                { $<y_attributes>$.attr_flags =
                                        ASTP_OUT | ASTP_OUT_SHAPE;
                                  $<y_attributes>$.bounds = NULL;       }
	 |	  IID_IS_KW LPAREN IDENTIFIER RPAREN
											{ $<y_attributes>$.iid_is_name = $<y_id>3; 
                                   $<y_attributes>$.bounds = NULL;
                                   $<y_attributes>$.attr_flags = 0;
											}
	 |	  IID_IS_KW LPAREN STAR IDENTIFIER RPAREN /* MIDL extension */
											{ $<y_attributes>$.iid_is_name = $<y_id>4; 
                                   $<y_attributes>$.bounds = NULL;
                                   $<y_attributes>$.attr_flags = 0;
											}

        /* Type, Field, Parameter Attributes */
    |   V1_ARRAY_KW             { $<y_attributes>$.attr_flags = ASTP_SMALL;
                                  $<y_attributes>$.bounds = NULL;       }
    |   STRING_KW               { $<y_attributes>$.attr_flags = ASTP_STRING;
                                  $<y_attributes>$.bounds = NULL;       }
    |   V1_STRING_KW            { $<y_attributes>$.attr_flags = ASTP_STRING0;
                                  $<y_attributes>$.bounds = NULL;       }
    |   UNIQUE_KW               { $<y_attributes>$.attr_flags = ASTP_UNIQUE;
                                  $<y_attributes>$.bounds = NULL;       }
    |   REF_KW                  { $<y_attributes>$.attr_flags = ASTP_REF;
                                  $<y_attributes>$.bounds = NULL;       }
    |   IGNORE_KW               { $<y_attributes>$.attr_flags = ASTP_IGNORE;
                                  $<y_attributes>$.bounds = NULL;       }
    |   CONTEXT_HANDLE_KW       { $<y_attributes>$.attr_flags = ASTP_CONTEXT;
                                  $<y_attributes>$.bounds = NULL;       }
    |   RANGE_KW LPAREN expression COMMA expression RPAREN /* MIDL extension */
                                { $<y_attributes>$.attr_flags = ASTP_RANGE;
                                  $<y_attributes>$.bounds =
                                     AST_range_from_expr($<y_exp>3, $<y_exp>5);
                                }

        /* Type-only Attribute(s) */
    |   V1_STRUCT_KW            { $<y_attributes>$.attr_flags = ASTP_UNALIGN;
                                  $<y_attributes>$.bounds = NULL;       }
    |   V1_ENUM_KW              { $<y_attributes>$.attr_flags = ASTP_V1_ENUM;
                                  $<y_attributes>$.bounds = NULL;       }
    |   ALIGN_KW LPAREN SMALL_KW RPAREN
                                { $<y_attributes>$.attr_flags = ASTP_ALIGN_SMALL;
                                  $<y_attributes>$.bounds = NULL;       }
    |   ALIGN_KW LPAREN SHORT_KW RPAREN
                                { $<y_attributes>$.attr_flags = ASTP_ALIGN_SHORT;
                                  $<y_attributes>$.bounds = NULL;       }
    |   ALIGN_KW LPAREN LONG_KW RPAREN
                                { $<y_attributes>$.attr_flags = ASTP_ALIGN_LONG;
                                  $<y_attributes>$.bounds = NULL;       }
    |   ALIGN_KW LPAREN HYPER_KW RPAREN
                                { $<y_attributes>$.attr_flags = ASTP_ALIGN_HYPER;
                                  $<y_attributes>$.bounds = NULL;       }
    |   HANDLE_KW               { $<y_attributes>$.attr_flags = ASTP_HANDLE;
                                  $<y_attributes>$.bounds = NULL;       }
    |   TRANSMIT_AS_KW LPAREN simple_type_spec RPAREN
                                { $<y_attributes>$.attr_flags = ASTP_TRANSMIT_AS;
                                  $<y_attributes>$.bounds = NULL;
                                  ASTP_transmit_as_type = $<y_type>3;
                                }
    |   SWITCH_TYPE_KW LPAREN simple_type_spec RPAREN
                                { $<y_attributes>$.attr_flags = ASTP_SWITCH_TYPE;
                                  $<y_attributes>$.bounds = NULL;
                                  ASTP_switch_type = $<y_type>3;
                                }

        /* Arm-only Attribute(s) */
    |   CASE_KW LPAREN ne_union_case_list RPAREN
                                { $<y_attributes>$.attr_flags = ASTP_CASE;
                                  $<y_attributes>$.bounds = NULL;
                                  ASTP_case = $<y_label>3;
                                }
    |   DEFAULT_KW              { $<y_attributes>$.attr_flags = ASTP_DEFAULT;
                                  $<y_attributes>$.bounds = NULL;
                                }
    |   IDENTIFIER      /* Not an attribute, so give an error */
        {
                char const *identifier; /* place to receive the identifier text */
                NAMETABLE_id_to_string ($<y_id>1, &identifier);
                log_error (nidl_yylineno, NIDL_UNKNOWNATTR, identifier, NULL);
                $<y_attributes>$.attr_flags = 0;
                $<y_attributes>$.bounds = NULL;
        }
    ;


/********************************************************************/
/*                                                                  */
/*          Compiletime Integer expression evaluation               */
/*                                                                  */
/********************************************************************/
expression: conditional_expression
        {$<y_exp>$ = $<y_exp>1;}
   ;

conditional_expression:
        logical_OR_expression
        {$<y_exp>$ = $<y_exp>1;}
   |    logical_OR_expression QUESTION expression COLON conditional_expression
        {
	 			$<y_exp>$ = AST_expression(AST_EXP_TERNARY_OP, $<y_exp>1, $<y_exp>3, $<y_exp>5);
        }
   ;

logical_OR_expression:
        logical_AND_expression
        {$<y_exp>$ = $<y_exp>1;}
   |    logical_OR_expression BARBAR logical_AND_expression
        {
	 			$<y_exp>$ = AST_expression(AST_EXP_BINARY_LOG_OR, $<y_exp>1, $<y_exp>3, NULL);
        }
   ;

logical_AND_expression:
        inclusive_OR_expression
        {$<y_exp>$ = $<y_exp>1;}
   |    logical_AND_expression AMPAMP inclusive_OR_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_LOG_AND, $<y_exp>1, $<y_exp>3, NULL);
        }
   ;

inclusive_OR_expression:
        exclusive_OR_expression
        {$<y_exp>$ = $<y_exp>1;}
   |    inclusive_OR_expression BAR exclusive_OR_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_OR, $<y_exp>1, $<y_exp>3, NULL);
        }
   ;

exclusive_OR_expression:
        AND_expression
        {$<y_exp>$ = $<y_exp>1;}
   |    exclusive_OR_expression CARET AND_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_XOR, $<y_exp>1, $<y_exp>3, NULL);
        }
   ;

AND_expression:
        equality_expression
        {$<y_exp>$ = $<y_exp>1;}
   |    AND_expression AMP equality_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_AND, $<y_exp>1, $<y_exp>3, NULL);
        }
   ;

equality_expression:
        relational_expression
        {$<y_exp>$ = $<y_exp>1;}
   |    equality_expression EQUALEQUAL relational_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_EQUAL, $<y_exp>1, $<y_exp>3, NULL);
        }
   |    equality_expression NOTEQUAL relational_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_NE, $<y_exp>1, $<y_exp>3, NULL);

        }
   ;

relational_expression:
        shift_expression
        {$<y_exp>$ = $<y_exp>1;}
   |    relational_expression LANGLE shift_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_LT, $<y_exp>1, $<y_exp>3, NULL);
        }
   |    relational_expression RANGLE shift_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_GT, $<y_exp>1, $<y_exp>3, NULL);
        }
   |    relational_expression LESSEQUAL shift_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_LE, $<y_exp>1, $<y_exp>3, NULL);
        }
   |    relational_expression GREATEREQUAL shift_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_GE, $<y_exp>1, $<y_exp>3, NULL);

        }
   ;

shift_expression:
        additive_expression
        {$<y_exp>$ = $<y_exp>1;}
   |    shift_expression LANGLEANGLE additive_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_LSHIFT, $<y_exp>1, $<y_exp>3, NULL);
        }
   |    shift_expression RANGLEANGLE additive_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_RSHIFT, $<y_exp>1, $<y_exp>3, NULL);

        }
   ;

additive_expression:
        multiplicative_expression
        {$<y_exp>$ = $<y_exp>1;}
   |    additive_expression PLUS multiplicative_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_PLUS, $<y_exp>1, $<y_exp>3, NULL);

        }
   |    additive_expression MINUS multiplicative_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_MINUS, $<y_exp>1, $<y_exp>3, NULL);
        }
   ;

multiplicative_expression:
        cast_expression
        {$<y_exp>$ = $<y_exp>1;}
   |    multiplicative_expression STAR cast_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_STAR, $<y_exp>1, $<y_exp>3, NULL);
				/*
            if (($<y_exp>$.exp.constant.val.integer < $<y_exp>1.exp.constant.val.integer) &&
                ($<y_exp>$.exp.constant.val.integer < $<y_exp>3.exp.constant.val.integer))
                log_error (nidl_yylineno, NIDL_INTOVERFLOW,
			   KEYWORDS_lookup_text(LONG_KW), NULL);
					*/
        }
   |    multiplicative_expression SLASH cast_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_SLASH, $<y_exp>1, $<y_exp>3, NULL);
        }
   |    multiplicative_expression PERCENT cast_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_BINARY_PERCENT, $<y_exp>1, $<y_exp>3, NULL);
            /*    log_error (nidl_yylineno, NIDL_INTDIVBY0, NULL); */
        }
   ;

cast_expression: unary_expression
        {$<y_exp>$ = $<y_exp>1;}
    ;

unary_expression:
        primary_expression
        {$<y_exp>$ = $<y_exp>1;}
   |    PLUS primary_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_UNARY_PLUS, $<y_exp>2, NULL, NULL);
		  }
   |    MINUS primary_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_UNARY_MINUS, $<y_exp>2, NULL, NULL);
        }
   |    TILDE primary_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_UNARY_TILDE, $<y_exp>2, NULL, NULL);
        }
   |    NOT primary_expression
        {
				$<y_exp>$ = AST_expression(AST_EXP_UNARY_NOT, $<y_exp>2, NULL, NULL);
        }
	|	  STAR primary_expression
		  {
			  $<y_exp>$ = AST_expression(AST_EXP_UNARY_STAR, $<y_exp>2, NULL, NULL);
		  }
   ;

primary_expression:
        LPAREN expression RPAREN
        { $<y_exp>$ = $<y_exp>2; }
    |   INTEGER_NUMERIC
        {
				$<y_exp>$ = AST_exp_integer_constant(
					$<y_int_info>1.int_val,
					$<y_int_info>1.int_signed);
        }
    |   CHAR
        {
				$<y_exp>$ = AST_exp_char_constant($<y_char>1);
        }
    |   IDENTIFIER
        {
			  	$<y_exp>$ = AST_exp_identifier($<y_id>1);
        }
    |   STRING
        {
            $<y_exp>$ = AST_exp_string_constant($<y_string>1);
        }
    |   NULL_KW
        {
            $<y_exp>$ = AST_exp_null_constant();
        }

    |   TRUE_KW
        {
            $<y_exp>$ = AST_exp_boolean_constant(true);
        }

    |   FALSE_KW
        {
            $<y_exp>$ = AST_exp_boolean_constant(false);
        }
   |    FLOAT_NUMERIC
        {
				$<y_exp>$ = AST_exp_integer_constant(0,0);
            log_error(nidl_yylineno, NIDL_FLOATCONSTNOSUP, NULL);
        }
   ;
%%

/*****************************************************************
 *
 *  Helper functions for managing multiple BISON parser contexts
 *
 *  GNU Bison v1.25 support for DCE 1.2.2 idl_compiler
 *  added 07-11-97 Jim Doyle, Boston University, <jrd@bu.edu>
 *
 *  Maintainance note:
 *
 *    The set of bison-specific static and global variables
 *    managed by the following code may need to changed for versions
 *    GNU Bison earlier or newer than V1.25.
 *
 *
 *****************************************************************/

/*****************************************************************
 *
 * Data structure to store the state of a BISON lexxer context
 *
 *****************************************************************/

struct nidl_bisonparser_state
  {

    /*
     * BISON parser globals that need to preserved whenever
     * we switch into a new parser context (i.e. multiple,
     * nested parsers).
     */

    int yychar;
    int yynerrs;
    YYSTYPE yylval;

  };


typedef struct nidl_bisonparser_state nidl_bisonparser_activation_record;

/*****************************************************************
 *
 * Basic constructors/destructors for FLEX activation states
 *
 *****************************************************************/

void *
new_nidl_bisonparser_activation_record()
  {
    return (malloc(sizeof(nidl_bisonparser_activation_record)));
  }

void
delete_nidl_bisonparser_activation_record(void * p)
{
 if (p)
    free((void *)p);
}

/*****************************************************************
 *
 * Get/Set/Initialize methods
 *
 *****************************************************************/

void *
get_current_nidl_bisonparser_activation()
  {
    nidl_bisonparser_activation_record * p;

    p = (nidl_bisonparser_activation_record * )
                new_nidl_bisonparser_activation_record();

    /*
     * save the statics internal to the parser
     *
     */

     p->yychar = yychar;
     p->yynerrs = yynerrs;
     p->yylval = yylval;

     return (void *)p;
  }

void
set_current_nidl_bisonparser_activation(void * ptr)
  {

    nidl_bisonparser_activation_record * p =
      (nidl_bisonparser_activation_record *)ptr;

    // restore the statics


     yychar = p->yychar;
     yynerrs = p->yynerrs;
     yylval = p->yylval;


  }

void
init_new_nidl_bisonparser_activation()
  {
    // set some initial conditions for a new Bison parser state

    yynerrs = 0;

  }

/* preserve coding style vim: set tw=78 sw=3 ts=3 : */
