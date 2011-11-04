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
**      ERRORS.C
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      IDL Error logging and reporting routines.
**
**  VERSION: DCE 1.0
**
*/


#include <nidl.h>
#include <errors.h>
#include <nametbl.h>
#include <frontend.h>
#include <message.h>
#include <nidlmsg.h>
#include <driver.h>
#include <stdarg.h>

#define MAX_LINE_LEN    256
#define WHERE_TEXT_LEN   20
#define MAX_WARNINGS    5
#define MAX_ERROR_FILES 10


/*
 *  Error log record format.
 */
typedef struct error_log_rec_t
{
    STRTAB_str_t     filename;  /* Error file name */
    int              lineno;    /* Source line number */
    idl_error_list_t msgs;      /* msgid + args */
    union                       /* Tree node or List node */
    {
        struct
        {                                   /* As tree node: */
            struct  error_log_rec_t *left;  /* Left pointer */
            struct  error_log_rec_t *right; /* Right pointer */
        } asBinTree;
        struct
        {                                   /* As list node: */
            struct  error_log_rec_t *next;  /* Ptr to next for this lineno */
        } asList;
    } links;
    struct  error_log_rec_t *first_this_line;   /* 1st addtl msg this line */
    struct  error_log_rec_t *last_this_line;    /* last addtl msg this line */
} error_log_rec_t;



/* Pointers to IDL parser or ACF parser global variables. */

FILE    **yyin_p;           /* Points to yyin or acf_yyin */
int     *yylineno_p;        /* Points to yylineno or acf_yylineno */
int     *yynerrs_p;         /* Points to yynerrs or acf_yynerrs */
char ** yytext_p;

boolean ERR_no_warnings;    /* Global copy of -no_warn command line option */

int              warnings = 0;      /* Warning count */
static  STRTAB_str_t    error_files[MAX_ERROR_FILES]; /* List of all files with errors */
static  int     error_file_count = 0;       /* Number of files with errors */
static  int     last_error_line = 0;        /* Line of last error */

#if !YYDEBUG
static
#endif
char const *current_file   = NULL;     /* Current source file name */


error_log_rec_t  *errors = NULL;    /* Tree root for error nodes */
int     error_count     = 0;        /* Error count */
STRTAB_str_t    error_file_name_id; /* Id of current source file */

extern void sysdep_cleanup_temp ( void );

/*
 *  y y w h e r e
 *
 *  Function:   Returns current input position for yyparse.
 *
 *  Globals:    yy*_p
 *
 */

void yywhere
(
    void
)
{
	boolean    have_text = false;  /* True if have source text to output */
	int        text_len = 0;       /* Length of source text to output */
	int        lineno;             /* Source line number of relevant text */
	long       msg_id;             /* ID of message to output */
	char const *near_text;         /* Text of object near error */
	STRTAB_str_t string_id;     /* Entry in string table of near text */
	char wherebuf[WHERE_TEXT_LEN+1];
	char * text_p = *yytext_p;
	lineno = *yylineno_p;

	if (*text_p)
	{
		for (text_len = 0; text_len < WHERE_TEXT_LEN; ++text_len)
		{
			if (text_p[text_len] == '\0' || text_p[text_len] == '\n')
				break;
			wherebuf[text_len] = text_p[text_len];
		}
		wherebuf[text_len] = '\0';

		if (text_len > 0)
		{
			have_text = true;
			lineno = lineno - (*text_p == '\n' || ! *text_p);
		}
	/*	printf("wherebuf is %s, yytext has %s\n", wherebuf, *yytext_p); */
	}


	/*
	 * If there is some text to show, put it in the string table
	 */
	if (have_text)
	{
		string_id = STRTAB_add_string(wherebuf);
		STRTAB_str_to_string(string_id, &near_text);
	}

	if (have_text)
	{
		if (feof(*yyin_p))
			msg_id = NIDL_EOFNEAR;
		else
			msg_id = NIDL_SYNTAXNEAR;
	}
	else
	{
		if (feof(*yyin_p))
			msg_id = NIDL_EOF;
		else
			msg_id = NIDL_SYNTAXERR;
	}
	log_error(lineno, msg_id, text_len, near_text, NULL);
}

/*
 *  y y e r r o r
 *
 *  Function:   Called by yypaser when a parse error is encountered.
 *
 *  Inputs:     message -  error message to display
 *              token    - expected token
 *
 *  Globals:    yynerrs_p
 *
 *  Notes:      This was adapted from the book
 *                  "Compiler Construction under Unix"
 *              by A. T. Schreiner & H. G. Friedman
 */

void yyerror
(
    char const *message
)
{
    static int list = 0;        /* Note: Currently always 0 */

    if (message || !list)
    {
        (*yynerrs_p)++;
        yywhere();

        /*
         * Only print the yacc version of errors if it isn't for a syntax error.
         */
        if (strcmp(message,"syntax error") != 0)
        {
            fputs(message, stderr);
            putc('\n', stderr);
            return;
        }

        return;
    }

    putc('\n', stderr);
    list = 0;
}

/*
 *  a c f _ y y e r r o r
 *  n i d l _ y y e r r o r
 *
 *
 *  Function:   Stub functions for the ACF and NIDL parsers
 *
 */

void nidl_yyerror(m)
 char * m;
{
  yyerror(m);
}

void acf_yyerror(m)
 char * m;
{
  yyerror(m);
}


/*
 *  a l l o c _ l o g _ r e c
 *
 *  Function:   Allocates and initializes error log record.
 *
 *  Inputs:     lineno - source line number
 *              message - message text
 *
 *  Outputs:    Address of error log record.
 */

static error_log_rec_t *alloc_log_rec
(
    STRTAB_str_t filename,
    int  lineno,
    idl_error_list_t *msgs
)
{
    error_log_rec_t *log_rec_p;
    int             i;

    log_rec_p = NEW (error_log_rec_t);

    log_rec_p->lineno  = lineno;
    log_rec_p->filename = filename;
    log_rec_p->msgs = *msgs;
    log_rec_p->first_this_line = 0;
    log_rec_p->last_this_line  = 0;

    log_rec_p->links.asBinTree.left  = 0;
    log_rec_p->links.asBinTree.right = 0;

    log_rec_p->links.asList.next     = 0;

    /*
     * If this file is not listed in the error file list
     */
    for (i = 0; i < error_file_count; i++)
        if (error_files[i] == filename)
	    break;

    /*
     * Add the file with errors to the error_file list
     */
    if (i == error_file_count)
        error_files[error_file_count++] = filename;

    /*
     * If we have too many files with errors, just ignore some
     */
    if (error_file_count >= MAX_ERROR_FILES)
	error_file_count--;

    return  log_rec_p;
}

/*
 *  f r e e _ l o g _ r e c
 *
 *  Function:   Frees an error log record and anything below it
 *
 *  Inputs:     record - error log record to free
 *  Outputs:    none
 */

static void free_log_rec
(
    error_log_rec_t *log_rec_p
)
{
    if (log_rec_p->links.asBinTree.left  != NULL)
    {
        free_log_rec(log_rec_p->links.asBinTree.left);
        log_rec_p->links.asBinTree.left = NULL;
    }

    if (log_rec_p->links.asBinTree.right  != NULL)
    {
        free_log_rec(log_rec_p->links.asBinTree.right);
        log_rec_p->links.asBinTree.right = NULL;
    }


    /*
     * Free all errors logged for this line number
     */
    while (log_rec_p->first_this_line != NULL)
    {
        error_log_rec_t *ep;
        ep = log_rec_p->first_this_line;
        log_rec_p->first_this_line = log_rec_p->links.asList.next;
        FREE(ep);
    }

    FREE(log_rec_p);
}

/*
 *  q u e u e _ e r r o r
 *
 *  Function:   Adds an error message to the end of a list of
 *              errors queued for a particular line.
 *
 *  Inputs:     log_rec_p - a pointer to the header error log rec.
 *              message   - the error message.
 */

static void queue_error
(
    error_log_rec_t  *log_rec_p,
    STRTAB_str_t     filename,
    idl_error_list_t *msgs
)
{
    error_log_rec_t *new_log_rec_p;

    new_log_rec_p = alloc_log_rec(filename, log_rec_p->lineno, msgs);

    if (log_rec_p->first_this_line == NULL)
    {
        log_rec_p->first_this_line = new_log_rec_p;
        log_rec_p->last_this_line  = new_log_rec_p;
        return;
    }

    log_rec_p->last_this_line->links.asList.next = new_log_rec_p;
    log_rec_p->last_this_line                    = new_log_rec_p;
}

/*
 *  a d d _ e r r o r _ l o g _ r e c
 *
 *  Function:   Adds an error log to the sorted binary tree of
 *              error messages.
 *
 *  Inputs:     log_rec_p - pointer to current root of tree.
 *              lineno    - line number on which error occurred.
 *              message   - the error message.
 */

static void add_error_log_rec
(
    error_log_rec_t *log_rec_p,
    STRTAB_str_t filename,
    int lineno,
    idl_error_list_t *msgs
)
{
    if (log_rec_p->lineno < lineno)
    {
        if (log_rec_p->links.asBinTree.right != NULL)
            add_error_log_rec(log_rec_p->links.asBinTree.right, filename, lineno,
                    msgs);
        else
            log_rec_p->links.asBinTree.right = alloc_log_rec(filename, lineno,
                    msgs);

        return;
    }

    if (log_rec_p->lineno > lineno)
    {
        if (log_rec_p->links.asBinTree.left != NULL)
            add_error_log_rec(log_rec_p->links.asBinTree.left, filename, lineno,
                    msgs);

        else
            log_rec_p->links.asBinTree.left = alloc_log_rec(filename, lineno,
                    msgs);
        return;
    }

    if (log_rec_p->lineno == lineno)
	queue_error(log_rec_p, filename, msgs);
}

/*
 *  l o g _ s o u r c e _ v a
 *
 *  Function:   Accumulates a warning or error message for later printout.
 *              All accumulated errors are printed by print_errors.
 *              Errors are kept sorted by line number and source
 *              file name.
 *
 *  Inputs:
 *              counter - which counter should be increased (error/warning)
 *              filename - STRTAB_str_t of full source file name
 *              lineno  - the line number of the error.
 *              msg_id - the error message ID.
 *              ap - va_list for the parameter last must be NULL if # < IDL_ERROR_LIST_SIZE
 *
 *  Outputs:    An error log record is inserted in the error tree.
 *
 */

void log_source_va
(
 int*         counter,
 STRTAB_str_t filename,
 int          lineno,
 long         msg_id,
 va_list      ap
)
{
    idl_error_list_t msgs = {0, {NULL}};

    size_t  idx;

    ++*counter;

    msgs.msg_id = msg_id;
    for (idx = 0; idx < IDL_ERROR_LIST_SIZE; idx++) {
	msgs.arg[idx] = va_arg(ap, void*);
	if (!msgs.arg[idx])
	    break;
    }

    if (errors == NULL)
	errors = alloc_log_rec(filename, lineno, &msgs);
    else
	add_error_log_rec(errors, filename, lineno, &msgs);
}


/*
 *  l o g _ s o u r c e _ e r r o r
 *
 *  Function:   Accumulates an error message for later printout.
 *              All accumulated errors are printed by print_errors.
 *              Errors are kept sorted by line number and source
 *              file name.
 *
 *  Inputs:
 *              filename - STRTAB_str_t of full source file name
 *              lineno  - the line number of the error.
 *              msg_id - the error message ID.
 *              [arg1,...,arg5] - 0 to 5 arguments for error message formatting
 *
 *  Outputs:    An error log record is inserted in the error tree.
 *
 */

void log_source_error
(
    STRTAB_str_t filename,
    int lineno,
    long msg_id,
    ...
)
{
    va_list ap;

    va_start(ap, msg_id);

    log_source_va(&error_count, filename, lineno, msg_id, ap);

    va_end(ap);
}

/*
 *  l o g _ s o u r c e _ w a r n i n g
 *
 *  Function:   Accumulates a warning message for later printout.
 *              All accumulated errors are printed by print_errors.
 *              Errors are kept sorted by line number and source
 *              file name.
 *
 *  Inputs:
 *              filename - STRTAB_str_t of full source file name
 *              lineno  - the line number of the error.
 *              msg_id - the error message ID.
 *              [arg1,...,arg5] - 0 to 5 arguments for error message formatting
 *
 *  Outputs:    An error log record is inserted in the error tree.
 *
 */

void log_source_warning
(
    STRTAB_str_t filename,
    int lineno,
    long msg_id,
    ...
)
{
    va_list ap;

    /* Return if warnings are suppressed. */
    if (ERR_no_warnings)
	return;

    va_start(ap, msg_id);

    log_source_va(&warnings, filename, lineno, msg_id, ap);

    va_end(ap);
}

/*
 *  l o g _ e r r o r
 *
 *  Function:   Accumulates an error message for later printout.
 *              All accumulated errors are printed by print_errors.
 *              Errors are kept sorted by line number.  The error
 *              is logged on the file for which set_name_for_errors
 *              was most recently called.
 *
 *  Inputs:     lineno  - the line number of the error.
 *              msg_id - the error message ID.
 *              [arg1,...,arg5] - 0 to 5 arguments for error message formatting
 *
 *  Outputs:    An error log record is inserted in the error tree.
 *
 */

void log_error
(
    int lineno,
    long msg_id,
    ...
)
{
    va_list ap;

    va_start(ap, msg_id);

    log_source_va(&error_count, error_file_name_id, lineno, msg_id, ap);

    va_end(ap);
}


/*
 *  l o g _ w a r n i n g
 *
 *  Function:   Accumulates a warning message for later printout.
 *              All accumulated errors are printed by print_errors.
 *              Errors are kept sorted by line number.  The warning
 *              is logged on the file for which set_name_for_errors
 *              was most recently called.
 *
 *  Inputs:     lineno  - the line number of the warning.
 *              msg_id - the error message ID.
 *              [arg1,...,arg5] - 0 to 5 arguments for error message formatting
 *
 *  Outputs:    An error log record is inserted in the error tree.
 *
 */

void log_warning
(
    int lineno,
    long msg_id,
    ...
)
{
    va_list ap;

    if (ERR_no_warnings)
	return;
    va_start(ap, msg_id);

    log_source_va(&warnings, error_file_name_id, lineno, msg_id, ap);

    va_end(ap);
}


/*
 *  s e e k _ f o r _ l i n e
 *
 *  Function:   Reads the line specified by lineno.
 *
 *  Inputs:     source - the file descriptor for the source file.
 *              lineno - the number of the line in error.
 *
 *  Outputs:    source_line - the source line is returned through here
 *
 *  Globals:    last_error_line
 */

void seek_for_line
(
    FILE *source_file,
    int lineno,
    char *source_line
)
{
    int lines_to_skip;
    int i;

    /*
     * If the FILE is NULL then can't get the source
     */
    if (source_file == NULL)
    {
        source_line[0] = 0;
        return;
    }

    lines_to_skip = lineno - last_error_line;

    for (i=0; i<lines_to_skip; i++)
    {
        if (fgets(source_line, MAX_LINE_LEN, source_file) == NULL)
        {
            abort();
        }
    }

    /* Strip off newline. */
    i = strlen(source_line) - 1;
    if (source_line[i] == '\n')
        source_line[i] = '\0';

    last_error_line = lineno;
}

/*
 *  p r i n t _ e r r o r s _ f o r _ l i n e
 *
 *  Function:   Prints out a source line and accumulated errors
 *              for that line.
 *
 *  Inputs:     fd - file descriptor for source file
 *              log_rec_ptr - a pointer to the header log rec for the line
 *              source - source file name
 */

void print_errors_for_line
(
    FILE *fd,
    char const *source,
    STRTAB_str_t    source_id,
    error_log_rec_t *log_rec_ptr
)
{
    char            source_line[MAX_LINE_LEN];
    boolean         source_printed = false;
    error_log_rec_t *erp;


    /* Print the error only if it in this file */
    if (source_id == log_rec_ptr->filename)
    {
        source_printed = true;
        seek_for_line(fd, log_rec_ptr->lineno, source_line);
        message_print(
            NIDL_FILESOURCE, source, log_rec_ptr->lineno, source_line
        );
        message_print(
            log_rec_ptr->msgs.msg_id,
	    log_rec_ptr->msgs.arg[0],
	    log_rec_ptr->msgs.arg[1],
	    log_rec_ptr->msgs.arg[2],
	    log_rec_ptr->msgs.arg[3],
	    log_rec_ptr->msgs.arg[4]
        );
    }


    for (erp = log_rec_ptr->first_this_line; erp; erp=erp->links.asList.next)
    {
        /* Print the error only if it in this file */
        if (source_id == erp->filename)
        {
            /* If we haven't output the source line text yet, the do so */
            if (!source_printed)
            {
                source_printed = true;
                seek_for_line(fd, log_rec_ptr->lineno, source_line);
                message_print(
                    NIDL_FILESOURCE, source, log_rec_ptr->lineno, source_line
                );
            }

            /* Now print out the actual error message */
	    message_print(
			  erp->msgs.msg_id,
			  erp->msgs.arg[0],
			  erp->msgs.arg[1],
			  erp->msgs.arg[2],
			  erp->msgs.arg[3],
			  erp->msgs.arg[4]
			 );
        }
    }
}

/*
 *  p r i n t _ e r r o r _ m e s s a g e s
 *
 *  Function:   Recursively prints all accumulated error messages.
 *
 *  Inputs:     fd          - file descriptor for source file
 *              source      - name of source file
 *              log_rec_ptr - root of error log tree
 */

void print_error_messages
(
    FILE *fd,
    char const *source,
    STRTAB_str_t    source_id,
    error_log_rec_t *log_rec_ptr
)
{
    if (log_rec_ptr->links.asBinTree.left != NULL)
        print_error_messages(fd, source, source_id, log_rec_ptr->links.asBinTree.left);

    print_errors_for_line(fd, source, source_id, log_rec_ptr);

    if (log_rec_ptr->links.asBinTree.right != NULL)
        print_error_messages(fd, source, source_id, log_rec_ptr->links.asBinTree.right);
}

/*
 *  p r i n t _ e r r o r s
 *
 *  Function:   Prints all accumulated error messages.
 *
 *  Inputs:     source - String table ID of source file name
 *
 *  Functional value:   true - if any errors were printed
 *                      false otherwise
 *
 */

boolean print_errors
(
    void
)

{
    FILE       *fd;
    char const *fn;
    int        i;
    STRTAB_str_t   source_id;
    error_log_rec_t *error_root;

    /*
     * If there are no errors, just return
     */
     if (errors == NULL) return 0;


    /*
     * Copy the root of the error tree and null it out so that
     * it looks like all the errors have been printed if we get
     * an error while printing them out.
     */
     error_root = errors;
     errors = NULL;

    /*
     * Loop through all files with errors
     */
    for (i = 0; i < error_file_count; i++)
    {
        source_id = error_files[i];
        STRTAB_str_to_string(source_id, &fn);
        fd = fopen(fn, "r");
        print_error_messages(fd, fn, source_id, error_root);
        last_error_line = 0;
    }

    /*
     * Free the tree of errors,
     */
    free_log_rec(error_root);
    error_file_count = 0;


    /*
     * Return true if we found any errors.
     */
    return (error_file_count != 0);
}

/*
 *  e r r o r
 *
 *  Function:   Prints the specifed error message and terminates the program
 *
 *  Inputs:     msg_id - the error message ID.
 *              [arg1,...,arg5] - 0 to 5 arguments for error message formatting
 *
 *  Notes:      This call terminates the calling program with a failure status
 *
 */

void error
(
    long msg_id,
    ...
)
{
    va_list arglist;

    va_start(arglist, msg_id);

    if (current_file)
        message_print(NIDL_LINEFILE, current_file, *yylineno_p);
    message_printv(msg_id, arglist);

    va_end(arglist);

#ifndef HASPOPEN
    sysdep_cleanup_temp();
#endif


    nidl_terminate();
}

/*
 *  e r r o r _ l i s t
 *
 *  Function:   Prints the specifed error message(s) and terminates the program
 *
 *  Inputs:     vecsize - number of messages
 *              errvec - pointer to one or more message info elements
 *              exitflag - TRUE => exit program
 *
 *  Notes:      This call terminates the calling program with a failure status
 *
 */

void error_list
(
    int vecsize,
    idl_error_list_p errvec,
    boolean exitflag
)
{
    int i;

    if (current_file)
        message_print(NIDL_LINEFILE, current_file, *yylineno_p);

    for (i = 0; i < vecsize; i++)
	message_print(errvec[i].msg_id,
		      errvec[i].arg[0],
		      errvec[i].arg[1],
		      errvec[i].arg[2],
		      errvec[i].arg[3],
		      errvec[i].arg[4]
		     );

    if (!exitflag) return;

#ifndef HASPOPEN
    sysdep_cleanup_temp();
#endif

    nidl_terminate();
}

/*
 *  w a r n i n g
 *
 *  Function:   Prints the specifed error message. Terminates if the
 *              error count excees the threshold.
 *
 *  Inputs:     msg_id - the error message ID.
 *              [arg1,...,arg5] - 0 to 5 arguments for error message formatting
 *
 *  Globals:    yylineno_p
 *
 *  Notes:      This call terminates the calling program with a status of -2.
 *
 */

void warning
(
    long msg_id,
    ...
)
{
    va_list arglist;

    /* Return if warnings are suppressed. */
    if (ERR_no_warnings)
        return;

    va_start(arglist, msg_id);

    if (current_file)
        message_print(NIDL_LINEFILE, current_file, *yylineno_p);
    message_printv(msg_id, arglist);

    va_end(arglist);

    if (++warnings > MAX_WARNINGS)
    {
        message_print(NIDL_MAXWARN, MAX_WARNINGS);
        nidl_terminate();
    }
}

/*
 *  s e t _ n a m e _ f o r _ e r r o r s
 *
 *  Function:   Records the name of the file being processed.  This name
 *              will be prepended onto error messages.
 *
 *  Inputs:     name - a pointer to the file name.
 *
 */

void set_name_for_errors
(
    char const *filename
)
{
    if (filename != NULL)
    {
        error_file_name_id = STRTAB_add_string(filename);
        STRTAB_str_to_string(error_file_name_id, &current_file);
    }
    else current_file = NULL;
}


/*
 *  i n q _ n a m e _ f o r _ e r r o r s
 *
 *  Function:   Returns the name of the file being processed.
 *
 *  Outputs:   name - the file name is copied through this.
 *
 */

void inq_name_for_errors
(
    char *name
)
{
    if (current_file)
        strcpy(name, current_file);
    else
        *name = '\0';
}

/* preserve coding style vim: set tw=78 sw=4 ts=4: */
