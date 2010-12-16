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
**  NAME
**
**      ERRORS.H
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**
**
**  VERSION: DCE 1.0
**
*/

#ifndef ERRORS_H
#define ERRORS_H

#include <errno.h>
#include <nidl.h>
#include <nametbl.h>
#include <stdarg.h>


#define IDL_ERROR_LIST_SIZE 5


/*
 *  The following error and warning routines are NOT function prototyped
 *  since they are designed, a la printf, to accept a variable number of
 *  arguments without using the varargs nonsense.
 */
void error(long msgid, ...);
void warning(long msgid, ...);

void log_source_va
(
 int*         counter,
 STRTAB_str_t filename,
 int          lineno,
 long         msg_id,
 va_list      ap
);

void log_source_error
(
	/* it is not a nonsense */
	STRTAB_str_t filename,
	int lineno,
	long msg_id,
	... /* 0..5 args terminated by NULL if less than five */
);

void log_source_warning
(
	/* it is not a nonsense */
	STRTAB_str_t filename,
	int lineno,
	long msg_id,
	... /* 0..5 args terminated by NULL if less than five */
);

void log_error
(
	/* it is not a nonsense */
	int lineno, /* Source line number */
	long msg_id, /* Message ID */
	... /* 0..5 args terminated by NULL if less than five */
);

void log_warning
(
	/* it is not a nonsense */
	int lineno, /* Source line number */
	long msg_id, /* Message ID */
	... /* 0..5 args terminated by NULL if less than five */
);


typedef struct {
    long msg_id;
    const void* arg[IDL_ERROR_LIST_SIZE];
} idl_error_list_t;

typedef idl_error_list_t *idl_error_list_p;

void error_list
(
    int vecsize,
    idl_error_list_p errvec,
    boolean exitflag
);

void inq_name_for_errors
(
    char *name
);

void set_name_for_errors
(
    char const *name
);

boolean print_errors
(
    void
);

void yyerror
(
    char const *message
);

void yywhere(void);


/*
 *  The following global variables are used by error reporting routines, most
 *  notably yyerror().  The signature of yyerror is defined by yacc to be:
 *
 *          void yyerror(char *message)
 *
 *  Because of the limitations of the routine signature, yyerror relies on
 *  global variables defined by the parser to output additional information
 *  about the error.  Since IDL uses multiple parsers, the global variables
 *  needed depend on which parse is active.  The workaround used here is
 *  that before each individual parse, the pointer variables below will be
 *  set to point to the relevant globals for that parse.  The error routines
 *  can then access the relevant data indirectly through the pointer variables.
 */
extern FILE     **yyin_p;
extern int      *yylineno_p;
extern int      *yynerrs_p;
extern char     **yytext_p;

/*
 * Error info to be fillin the fe_info nodes
 */
extern int          error_count;
extern int          warnings;
extern STRTAB_str_t error_file_name_id;

#ifdef DUMPERS
#define INTERNAL_ERROR(string) {printf("Internal Error Diagnostic: %s\n",string);warning(NIDL_INTERNAL_ERROR,__FILE__,__LINE__);}
#else
#define INTERNAL_ERROR(string) {error(NIDL_INTERNAL_ERROR,__FILE__,__LINE__); printf(string);}
#endif
#endif
/* preserve coding style vim: set tw=78 sw=4 : */
