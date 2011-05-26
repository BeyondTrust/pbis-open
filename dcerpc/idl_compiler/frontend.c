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
**      FRONTEND.C
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Mainline for IDL compilers.
**
**  VERSION: DCE 1.0
**
*/

#include <signal.h>

#ifdef vms
#  include <types.h>
#  include <stat.h>
#  include <stsdef.h>
#  include <descrip.h>
#else
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#endif

#include <frontend.h>
#include <astp.h>
#include <astp_dmp.h>
#include <checker.h>
#include <command.h>
#include <errors.h>
#include <files.h>
#include <getflags.h>
#include <propagat.h>
#include <nidl_y.h>
#include <message.h>
#include <flex_bison_support.h>

#define CONFIG_SUFFIX ".acf"

/*
 * Macro to "close" a pipe - don't actually call pclose since it can cause hangs
 * in certain instances where a parent process opens and closes several pipes,
 * apparently due to the underlying wait() call from pclose and the unpredict-
 * ability of the order of child exits.
 */
#ifndef VMS
#include <errno.h>
#define PCLOSE(stream) \
{ \
    char buf[256]; \
    /* Silently read to eof - extra lines should cause COMPABORT msg anyway */ \
    while (fgets(buf, sizeof(buf), stream) != NULL) ; \
}
#endif

extern int yyparse(void);

extern void acf_cleanup(void);

extern void acf_init(
    boolean     *cmd_opt_arr,   /* [in] Array of command option flags */
    void        **cmd_val_arr,  /* [in] Array of command option values */
    char        *acf_file       /* [in] ACF file name */
);


/* Globals */
extern int acf_yylineno;
extern int nidl_yylineno;

/* Local data definitions. */

static boolean      *saved_cmd_opt;     /* Array of command option flags */
static void         **saved_cmd_val;    /* Array of command option values */

typedef struct FE_import_file_n_t {
        struct FE_import_file_n_t * next;
    STRTAB_str_t   imported_fn_id;
    STRTAB_str_t   imported_full_fn_id;
} FE_import_file_n_t;

static FE_import_file_n_t * imported_file_list = NULL;

extern boolean ASTP_parsing_main_idl;

/*
**  i n i t
**
**  Frontend-specific initialization.
*/

static void FE_init(void)
{
    saved_cmd_opt = NULL;
    saved_cmd_val = NULL;

    KEYWORDS_init();
    NAMETABLE_init();
    AST_init();
}

/*
**  c p p
**
**  Sends the source file through CPP before giving it to lex.
**  The cpp_output argument is a file ID for the output from cpp.
**  cpp_output is connected to piped output from cpp.
*/

#if defined(CPP)
static void cpp
(
    char        *cpp_cmd,       /* [in] Base command to invoke cpp */
    char        *cpp_opt,       /* [in] Addtl command options for cpp */
    char        *file_name,     /* [in] Source full filespec; "" => stdin */
    char        **def_strings,  /* [in] List of #define's for preprocessor */
    char        **undef_strings,/* [in] List of #undefine's for preprocessor */
    char        **idir_list,    /* [in] List of -I directories */
    FILE        **cpp_output    /*[out] File ID of cpp output */
)

{
    extern FILE *popen();
    char* cmd = NULL;  /* Command to spawn cpp */
    size_t length = 0;
    int i;

    /* Calculate length of command */

    length += strlen(cpp_cmd) + 1;
    length += strlen(cpp_opt) + 1;
    length += strlen(file_name);

    i = 0;
    while (def_strings[i])
    {
        length += strlen(" -D");
        length += strlen(def_strings[i++]);
    }

    i = 0;
    while (undef_strings[i])
    {
        length += strlen(" -U");
        length += strlen(undef_strings[i++]);
    }

    if (strcmp(cpp_cmd, CMD_def_cpp_cmd) == 0)
    {
        i = 0;
        while (idir_list[i])
        {
            length += strlen(" -I");
            length += strlen(idir_list[i++]);
        }
    }

    cmd = calloc(length + 1, 1);
    if (!cmd)
    {
        error(NIDL_INVOKECPP);
    }

    /* Put together beginning of command. */

    strcpy(cmd, cpp_cmd);
    strcat(cmd, " ");
    strcat(cmd, cpp_opt);
    strcat(cmd, " ");
    strcat(cmd, file_name);

    /* Append the -D strings. */

    while (*def_strings)
    {
        strcat(cmd, " -D");
        strcat(cmd, *def_strings++);
    }

    /* Append the -U strings. */

    while (*undef_strings)
    {
        strcat(cmd, " -U");
        strcat(cmd, *undef_strings++);
    }

    /* If cpp_cmd is the default, append the -I directories. */

    if (strcmp(cpp_cmd, CMD_def_cpp_cmd) == 0)
    {
        while (*idir_list)
        {
            strcat(cmd, " -I");
            strcat(cmd, *idir_list++);
        }
    }

    assert(strlen(cmd) == length);

    /* Now execute the cpp command and open output file or pipe. */

    if (saved_cmd_opt[opt_verbose])
        message_print(NIDL_RUNCPP,cmd);

    if ((*cpp_output = popen(cmd, "r")) == 0)
        error(NIDL_INVOKECPP);

    free(cmd);
}
#endif

/*
**  p a r s e _ a c f
**
**  Invokes the ACF (Attribute Configuration File) parser on specified file.
**
**  Note:   If cmd_opt[opt_confirm], then no real work is done except to
**          issue messages for the verbose option.
**
**  Returns:    == 0    => success
**              != 0    => failure
*/

static boolean parse_acf        /* Returns true on success */
(
    boolean     *cmd_opt,       /* [in] Array of command option flags */
    void        **cmd_val,      /* [in] Array of command option values */
    char        *acf_file       /* [in] ACF full file name */
)

{
    extern int acf_yyparse( void);


    extern FILE *acf_yyin;
    extern int  acf_yynerrs;
    extern char * acf_yytext;

    FILE        **yyin_sp;              /* Used to save yy pointer variables */
    int         *yylineno_sp;
    int         *yynerrs_sp;
    char        **yytext_sp;

    if (cmd_opt[opt_verbose])
        message_print(NIDL_PROCESSACF, acf_file);
    if (cmd_opt[opt_confirm])
        return true;

    /*
     * lex & yacc intializations
     */
    acf_yylineno = 1;                       /* Not sure why/if these 3 are needed */

    /*
     * Hack the yy pointer variables to point at the ACF-specific yy variables.
     * Then parse the ACF, and restore the yy pointer variable to their
     * original state.  This hack allows us to share error reporting routines
     * with the main parser.  See errors.h for details.
     */
    yyin_sp     = yyin_p;
    yylineno_sp = yylineno_p;
    yynerrs_sp  = yynerrs_p;
    yytext_sp   = yytext_p;

    yyin_p      = &acf_yyin;
    yylineno_p  = &acf_yylineno;
    yynerrs_p   = &acf_yynerrs;
    yytext_p    = &acf_yytext;

    acf_init(cmd_opt, cmd_val, acf_file);

#if defined(CPP)
    if (cmd_opt[opt_cpp])
    {
        cpp((char *)cmd_val[opt_cpp],
            (char *)cmd_val[opt_cpp_opt],
            acf_file,
            (char **)cmd_val[opt_cpp_def],
            (char **)cmd_val[opt_cpp_undef],
            (char **)cmd_val[opt_idir],
            &acf_yyin);
    }
    else
#endif
        /* No cpp, just open source file */
        FILE_open(acf_file, &acf_yyin);

    if (acf_yyparse() != 0 && acf_yynerrs == 0)
        log_error(acf_yylineno, NIDL_COMPABORT, NULL);

    acf_cleanup();

#ifndef VMS
    if (cmd_opt[opt_cpp])
        PCLOSE(acf_yyin)
    else
        fclose(acf_yyin);
#endif
#ifdef VMS
    fclose(acf_yyin);
    if (cmd_opt[opt_cpp])
        delete(temp_path_name);
#endif

    yyin_p      = yyin_sp;
    yylineno_p  = yylineno_sp;
    yynerrs_p   = yynerrs_sp;
    yytext_p    = yytext_sp;

    if (acf_yynerrs != 0)
        return false;

    return true;
}

/*
** a l r e a d y _ i m p o r t e d
**
** Checks whether a file is already included in the parse or not.
**
** Returns TRUE if the import file has already been parsed.
** Returns false otherwise  (Including can't find the file.)
**
*/

static boolean already_imported
(
    STRTAB_str_t import_path_id      /* The name to check */
)

{
    char                 new_import_full_fn[max_string_len];
    STRTAB_str_t         new_import_full_fn_id;
    STRTAB_str_t         new_import_fn_id;
    char                 base_file_name[max_string_len];
    char                 base_file_ext[max_string_len];
    struct               stat stat_buf;
    FE_import_file_n_t * imported_file;
    char const * *       idir_list;
    boolean              alr_imp;
    char const        * file_name;

    /*
     * Get a string to lookup.
     */
    STRTAB_str_to_string (import_path_id, &file_name);
    idir_list = (char const * *)saved_cmd_val[opt_idir];

    /*
     * Note that a lookup failure will not report a failure here;
     * That will be reported when we actually try to import it.
     */
    if (!FILE_lookup(file_name, idir_list, &stat_buf, new_import_full_fn))
        return false;
    new_import_full_fn_id = STRTAB_add_string(new_import_full_fn);

    /*
     * Make sure there is no partial path information.
     */
    if (!FILE_parse(new_import_full_fn, NULL, base_file_name, base_file_ext))
        return false;

    strncat(base_file_name, base_file_ext, max_string_len-1);
    base_file_name[max_string_len-1] = '\0';
    new_import_fn_id = STRTAB_add_string(base_file_name);

    /*
     * Initialize the default return status.
     */
    alr_imp = false;

    imported_file = imported_file_list;

    while ((imported_file != NULL) && (!alr_imp))
    {
        if (new_import_fn_id == imported_file->imported_fn_id)
        {
            /*
             * A match is found in the list. We are done.
             */
            alr_imp = true;
        }
        imported_file = imported_file -> next;
    }

    /*
     * Record the import we are about to do if not imported already.
     */
    if (!alr_imp)
    {
        imported_file = NEW (FE_import_file_n_t);
        imported_file -> imported_fn_id = new_import_fn_id;
        imported_file -> imported_full_fn_id = new_import_full_fn_id;
        imported_file -> next = imported_file_list;
        imported_file_list = imported_file;
    }

    return alr_imp;
}

/*
**  p a r s e
**
**  Invokes the parser on the specified IDL file.  If the IDL file is
**  successfully parsed, and there is a related ACF file, then the ACF
**  file is parsed also.
**
**  Returns:    a pointer to the interface binding if the IDL file was parsed
**              successfully and the ACF file (if any) was parsed successfully;
**              NULL otherwise.
*/

static boolean parse
(
    boolean     *cmd_opt,       /* [in] Array of command option flags */
    void        **cmd_val,      /* [in] Array of command option values */
    STRTAB_str_t idl_sid,       /* [in] IDL filespec stringtable ID */
                                /*      STRTAB_NULL_STR => stdin */
    boolean     idir_valid,     /* [in] true => use import directory list */
    AST_interface_n_t **int_p   /*[out] Ptr to interface node */
)
{
    extern FILE *nidl_yyin;
    extern int nidl_yynerrs;
    extern char *nidl_yytext;
    extern int nidl_yyparse(void);

    char const  *sf;                            /* Source filespec */
    char        full_path_name[max_string_len]; /* Full source pathname */
    STRTAB_str_t full_pn_id;                    /* Full src path string id */
    char const * *idir_list;                    /* List of search directories */
    char        file_dir[max_string_len];       /* Directory part of src file */
    boolean     file_dir_is_cwd;                /* T => file_dir current dir */
    char        file_name[max_string_len];      /* File name part of src file */
    char        acf_file[max_string_len];       /* ACF file name w/o dir */
    char        full_acf_name[max_string_len];  /* Full ACF pathname */
    boolean     acf_exists;                     /* T => ACF file exists */
    struct stat stat_buf;                       /* File lookup stats */
    int         i=0;


    /* One-time saving of command array addresses to static storage. */
    if (saved_cmd_opt == NULL)
        saved_cmd_opt = cmd_opt;
    if (saved_cmd_val == NULL)
        saved_cmd_val = cmd_val;

    /*
     * If the idir_valid flag is set, look in the -I directories for the
     * source file.  Otherwise, just use the filespec as is.
     */
    if (idir_valid)
        idir_list = (char const * *)cmd_val[opt_idir];
    else
        idir_list = NULL;

    if (idl_sid == STRTAB_NULL_STR)     /* stdin */
        full_path_name[0] = '\0';
    else
    {
        STRTAB_str_to_string(idl_sid, &sf);
        if  (!FILE_lookup(sf, idir_list, &stat_buf, full_path_name))
        {
            error(NIDL_FILENOTFND, sf);
            return false;
        }
    }

    if (cmd_opt[opt_verbose] && !ASTP_parsing_main_idl)
        message_print(NIDL_IMPORTIDL, full_path_name);

#if defined(CPP)
    if (cmd_opt[opt_cpp])
    {
	/* define the macro describing dceidl compiler (for conditional
	   constructions) */
        if (!add_def_string(DCEIDL_DEF))
        {
            message_print(NIDL_IMPORTIDL, "Warning: Couldn't define macro %s!\n", DCEIDL_DEF);
        }

        cpp((char *)cmd_val[opt_cpp],
            (char *)cmd_val[opt_cpp_opt],
            full_path_name,
            (char **)cmd_val[opt_cpp_def],
            (char **)cmd_val[opt_cpp_undef],
            (char **)cmd_val[opt_idir],
            &nidl_yyin);
    }
    else
#endif
        if (full_path_name[0] == '\0')  /* stdin */
            nidl_yyin = stdin;
        else
            FILE_open(full_path_name, &nidl_yyin);

    /*
     * Setup file name for errors to the full file name
     */
    set_name_for_errors((full_path_name[0] == '\0') ? "stdin" : full_path_name);

    /*
     * lex & yacc intializations
     */
    nidl_yylineno = 1;

    /*
     * Hack the yy pointer variables to point at the IDL-specific yy variables
     * before starting the parse.  This hack allows us to share error reporting
     * routines with the ACF parser.  See errors.h for details.
     */
    yyin_p      = &nidl_yyin;
    yylineno_p  = &nidl_yylineno;
    yynerrs_p   = &nidl_yynerrs;
    yytext_p    = &nidl_yytext;

    if (nidl_yyparse() != 0 && error_count == 0)
        log_error(nidl_yylineno, NIDL_COMPABORT, NULL);
    *int_p = the_interface;

#ifndef VMS
    if (cmd_opt[opt_cpp])
        PCLOSE(nidl_yyin)
    else
        fclose(nidl_yyin);
#endif
#ifdef VMS
    fclose(nidl_yyin);
    if (cmd_opt[opt_cpp])
        delete(temp_path_name);
#endif

    if (error_count != 0)
        return false;        /* Error parsing IDL */

    /* Successful parse: save IDL filespec in interface node. */
    if (the_interface != NULL)
    {
        full_pn_id = STRTAB_add_string(full_path_name);
        the_interface->fe_info->file = full_pn_id;
    }
    else
    {
        if (ASTP_parsing_main_idl) return false;    /* Shouldn't happen */
    }

    /*
     * Now see if there is an associated Attribute Configuration File (ACF).
     * The ACF name is constructed from the IDL file name.  The ACF file can
     * be in any of the -I directories.
     */
    if (!FILE_parse(full_path_name, file_dir, file_name, (char *)NULL))
        return false;

    if (!FILE_form_filespec(file_name, (char *)NULL, CONFIG_SUFFIX,
                            (char *)NULL, acf_file))
        return false;
#ifdef UNIX
    /*
     * If the created ACF filespec matches the file_name portion of the IDL
     * filespec, it implies that the IDL filespec contains multiple '.'s - in
     * this special case, append the ACF suffix to the filespec.
     */
    if (strcmp(file_name, acf_file) == 0)
        strcat(acf_file, CONFIG_SUFFIX);
#endif

    /*
     * If the directory part of the source filespec is not the current dir,
     * tack it on at the end of the include directory list.  Note: we assume
     * that there is enough room for a temporary extra entry in the idir list.
     */
    idir_list = (char const **)cmd_val[opt_idir];
    file_dir_is_cwd = FILE_is_cwd(file_dir);

    if (!file_dir_is_cwd)
    {
        for (i = 0 ; idir_list[i] != NULL ; i++)
            ;
        idir_list[i] = file_dir;
        idir_list[i+1] = NULL;
    }

    acf_exists = FILE_lookup(acf_file, idir_list, &stat_buf, full_acf_name);
#ifdef VMS
    if (!acf_exists)
    {
        /*
         * This code is for the special case where foo.idl is the source file
         * but foo is also a logical name; use the full filespec so that foo
         * isn't translated.
         */
        if (!FILE_form_filespec((char *)NULL, (char *)NULL, CONFIG_SUFFIX,
                                full_path_name, acf_file))
            return false;
        acf_exists = FILE_lookup(acf_file, (char **)NULL, &stat_buf,
                                 full_acf_name);
    }
#endif

    if (!file_dir_is_cwd)
        idir_list[i] = NULL;

    if (!acf_exists)
        return true;   /* No ACF; return success */

    /* Parse the ACF. */
    if (!parse_acf(cmd_opt, cmd_val, full_acf_name))
        return false;            /* Error parsing ACF */

    return true;       /* Both IDL and ACF parsed without errors */
}

/*
 *  F E _ p a r s e _ i m p o r t
 *
 *  Parse an import file.
 *  This involves pushing the state of the current parse, initing a few
 *  cells, calling parse, and restoring the state of the current parse.
 *
 *  Parametric inputs: The string table id of the filename we wish to parse.
 *  Global inputs: All the parse and lex static storage that maintains parse
 *                 state.
 *  Parametric outputs: None
 *  Global outputs: The parse state is restored to its value on entry.
 *
 *  Routine value:  None.
 */

AST_interface_n_t *FE_parse_import
(
    STRTAB_str_t    new_input   /* [in] string table id of file to parse */
)

{

	 /*
	  * BISON/FLEX nester parser support
	  *
	  * FE_parse_import() is called by the import_file rule in
	  * nidl.y. We can be called recursively, for each file requested
	  * to be included. To support this with non-reentrant Flex
	  * and Bison parsers, we save the current Flex/Bison state,
	  * create a new, initialized state for the ACF & NIDL x Flex & BISON
	  * state machines, and parse the import. When we are done,
	  * we delete the temporary parsers states used to do the import,
	  * and restore the parser state variables back to where they were.
	  *
	  * The support logic to do context switching of parser state machines
	  * can be found at the end of nidl.l, nidl.y, acf.l, acf.y
	  *
	  * For any given import, we have to save and reset FOUR different
	  * state machines:
	  *
	 *    ACF parser, ACF flexxer, NIDL parser, NIDL lexxer
	  *
	  */

	 void * saved_nidl_flexxer_state;
	 void * saved_acf_flexxer_state;
	 void * saved_nidl_bisonparser_state;
	 void * saved_acf_bisonparser_state;

	 void * new_nidl_flexxer_state;
	 void * new_acf_flexxer_state;
	 void * new_nidl_bisonparser_state;
	 void * new_acf_bisonparser_state;

	 boolean saved_ASTP_parsing_main_idl;
	 char saved_current_file[ PATH_MAX ];
	 STRTAB_str_t saved_error_file_name_id;
	 AST_interface_n_t *int_p;

	 /* Saved interface attributes */
	 AST_interface_n_t *saved_interface;

	 /*
	  * Return now, if the file is already imported.
	  */
	 if (already_imported (new_input))
		  return (AST_interface_n_t *)NULL;


	 /*
	  * SAVE THE CURRENT FLEXXER STATE
	  */

	 saved_nidl_flexxer_state = get_current_nidl_flexxer_activation();
	 saved_acf_flexxer_state = get_current_acf_flexxer_activation();


	 /*
	  * SAVE THE CURRENT BISON STATE
	  */

	 saved_nidl_bisonparser_state = get_current_nidl_bisonparser_activation();
	 saved_acf_bisonparser_state = get_current_acf_bisonparser_activation();

	 /* save the AST state */

	 saved_ASTP_parsing_main_idl = ASTP_parsing_main_idl;
	 inq_name_for_errors(saved_current_file);
	 saved_error_file_name_id = error_file_name_id;

	 /*
	  * Save interface information
	  */

	 saved_interface = the_interface;

	 /*
	  * Initialize interface attributes
	  */

	 the_interface = NULL;


	 /*
	  * We have now saved away all the state of the current parse.
	  * Initialize a few cells, open the imported file and recursively invoke the
	  * parse.
	  */

	 if (saved_interface)
		  ASTP_parsing_main_idl = false;

	 /*
	  * Create new, empty and initialized parser context to
	  * do the import.
	  */

	 /* Make a new NIDL Flex token analyzer state-machine */

	 new_nidl_flexxer_state = new_nidl_flexxer_activation_record(); 
	 set_current_nidl_flexxer_activation(new_nidl_flexxer_state); 
	 init_new_nidl_flexxer_activation();

	 /* Make a new NIDL Bison parser state-machine */

	 new_nidl_bisonparser_state = new_nidl_bisonparser_activation_record(); 
	 set_current_nidl_bisonparser_activation(new_nidl_bisonparser_state); 
	 init_new_nidl_bisonparser_activation();

	 /* Make a new ACF Flex token analyzer state-machine */

	 new_acf_flexxer_state = new_acf_flexxer_activation_record(); 
	 set_current_acf_flexxer_activation(new_acf_flexxer_state); 
	 init_new_acf_flexxer_activation();

	 /* Make a new ACF Bison parser state-machine */

	 new_acf_bisonparser_state = new_acf_bisonparser_activation_record(); 
	 set_current_acf_bisonparser_activation(new_acf_bisonparser_state); 
	 init_new_acf_bisonparser_activation();


	 /* Now we can parse the import file....
	  * Parse the file.  Routine parse normally returns a AST_interface_n_t,
	  * but since we are not parsing the main IDL, we don't care about it.
	  * The "true" argument says to search -I directories for the file.
	  */

	 parse(saved_cmd_opt, saved_cmd_val, new_input, true, &int_p);

#if 0
	 if (saved_interface && saved_interface->inherited_interface_name == the_interface->name)	{
		  AST_export_n_t * ep = the_interface->exports;
		  AST_export_n_t * op;

		  if (AST_OBJECT_SET(the_interface))	{
				/* ORPC inheritance by pulling in the base class interface's
				 * operations */
				saved_interface->op_count = the_interface->op_count;

				for (; ep != NULL; ep = ep->next)	{
					 if (ep->kind != AST_operation_k)
						  continue;
					 op = AST_export_node((ASTP_node_t*)ep->thing_p.exported_operation,
								AST_operation_k);
					 /* Assign the operation number to this operation relative to any
					  * inherited operations */
					 op->thing_p.exported_operation->op_number = saved_op_count++;
					 saved_interface->exports = (AST_export_n_t*)AST_concat_element(
								(ASTP_node_t*)saved_interface->exports,
								(ASTP_node_t*)op
								);
				}
				/* update the operation count to include base class operations */
				saved_interface->op_count = saved_op_count;
		  }
	 }
#endif

	 /*
	  * Restore interface information
	  */
	 the_interface = saved_interface;

	 /*
	  * The recursive parse is done (at this level).
	  * Restore the state machines to the previous values before the import...
	  */

	 /*
	  * Blow away the Bison and Flexer state-machines that we setup in here
	  * to parse the imported file.
	  */

	 delete_nidl_flexxer_activation_record(new_nidl_flexxer_state); 
	 delete_acf_flexxer_activation_record(new_acf_flexxer_state); 
	 delete_nidl_bisonparser_activation_record(new_nidl_bisonparser_state); 
	 delete_acf_bisonparser_activation_record(new_acf_bisonparser_state); 

	 /*
	  * Restore the previous Bison and Flexer state-machines so
	  * that we can continue where we left off.
	  */

	 set_current_nidl_flexxer_activation(saved_nidl_flexxer_state); 
	 set_current_acf_flexxer_activation(saved_acf_flexxer_state); 
	 set_current_nidl_bisonparser_activation(saved_nidl_bisonparser_state); 
	 set_current_acf_bisonparser_activation(saved_acf_bisonparser_state); 


	 /*
	  * Restore information used by error reporting routines.
	  */
	 ASTP_parsing_main_idl = saved_ASTP_parsing_main_idl;
	 set_name_for_errors(saved_current_file);
	 error_file_name_id = saved_error_file_name_id;

	 return int_p;
}
/*
**  p a r s e _ i d l
**
**  Parse the source IDL file.
**
**  Side Effects:   Returns the interface in the int_p parameters and
**      a boolean status if any errors were encountered.
*/

static boolean parse_idl        /* Returns true on success */
(
    boolean     *cmd_opt,       /* [in] Array of command option flags */
    void        **cmd_val,      /* [in] Array of command option values */
    STRTAB_str_t idl_sid,       /* [in] IDL filespec stringtable ID */
                                /*      STRTAB_NULL_STR => stdin */
    AST_interface_n_t **int_p   /*[out] Ptr to interface node */
)

{
    boolean status;                     /* Status to return */
    FE_import_file_n_t *imported_file;  /* Main IDL file info */
    char const *file_name;                 /* Main IDL file name */
    char       imported_fn[max_string_len];/* Main IDL full file name */
    struct stat stat_buf;               /* File lookup info */
    char       file_name_part[max_string_len];/* Main IDL file name part */
    char       file_type_part[max_string_len];/* Main IDL file type part */
    boolean name_warning = false;       /* Warn on name used */

    if (idl_sid != STRTAB_NULL_STR)
    {
        /*
         * Record the main IDL as if it is an imported file.
         * This preserves idempotency in case the main IDL is also imported.
         */
        STRTAB_str_to_string(idl_sid, &file_name);
        FILE_parse(file_name, NULL, file_name_part, file_type_part);
        strcat(file_name_part, file_type_part);

#ifndef DISABLE_SYSIDL_WARNING
        /*
         * Issue a warning on any system IDL files a user might
         * accidentally chose one of those name and get strange behavior.
         */
        if (!strcmp(file_name_part,"iovector.idl")) name_warning = true;
        else if (!strcmp(file_name_part,"lbase.idl")) name_warning = true;
        else if (!strcmp(file_name_part,"nbase.idl")) name_warning = true;
        else if (!strcmp(file_name_part,"ncastat.idl")) name_warning = true;
        else if (!strcmp(file_name_part,"ndrold.idl")) name_warning = true;
        else if (!strcmp(file_name_part,"rpc.idl")) name_warning = true;
        else if (!strcmp(file_name_part,"rpcsts.idl")) name_warning = true;
        else if (!strcmp(file_name_part,"rpctypes.idl")) name_warning = true;
        else if (!strcmp(file_name_part,"twr.idl")) name_warning = true;
        else if (!strcmp(file_name_part,"uuid.idl")) name_warning = true;
#endif

        if (name_warning)
            message_print(NIDL_SYSIDLNAME,file_name);

        /*
         * Note that a lookup failure will not report a failure here;
         * That will be reported when we actually try to parse it.
         */
        if (FILE_lookup(file_name, NULL, &stat_buf, imported_fn))
        {
            imported_file = NEW (FE_import_file_n_t);
            imported_file->imported_fn_id = STRTAB_add_string(file_name_part);
            imported_file->imported_full_fn_id = STRTAB_add_string(imported_fn);
            imported_file->next = imported_file_list;
            imported_file_list = imported_file;
        }
    }

    /*
     * Parse the top-level IDL file.  The "false" argument tells parse not
     * to scan the import directories for the source IDL file.
     */
    *int_p = NULL;
    status = parse(cmd_opt, cmd_val, idl_sid, false, int_p);

    /* Terminate if there were any syntax errors in the top level IDL file. */
    if (*int_p == NULL)
        status = false;

    return status;
}

/*
**  F E _ m a i n
**
**  Main frontend routine.  Invokes each major frontend component.
**
**  Note:   If cmd_opt[opt_confirm], then no real work is done except to call
**          the components that can issue messages for the verbose option.
*/

boolean FE_main                 /* Returns true on success */
(
    boolean     *cmd_opt,       /* [in] Array of command option flags */
    void        **cmd_val,      /* [in] Array of command option values */
    STRTAB_str_t idl_sid,       /* [in] IDL filespec stringtable ID */
                                /*      STRTAB_NULL_STR => stdin */
    AST_interface_n_t **int_p   /*[out] Ptr to interface node */
)

{
    boolean status;

    /* Frontend-specific initialization. */
    FE_init();

    /* Parse the source IDL file (and related ACF if applicable). */
    status = parse_idl(cmd_opt, cmd_val, idl_sid, int_p);

#ifdef DUMPERS
    /* Dump the nametable if requested. */
    if (cmd_opt[opt_dump_nametbl])
        NAMETABLE_dump_tab();
#endif

#ifdef DUMPERS
    /* Dump the AST (before checking) if requested. */
    if (cmd_opt[opt_dump_ast])
        AST_dump_interface(*int_p);
#endif

    /* Propagate attributes throughout the AST. */
    if (status && !cmd_opt[opt_confirm])
        status = PROP_main(cmd_opt, cmd_val, *int_p);

    /* Do semantic checking of the interface. */
    if (status)
    {
        if (!cmd_opt[opt_confirm])
            status = CHECKER_main(cmd_opt, cmd_val, *int_p);
    }
    else
        message_print(NIDL_NOSEMCHECK);

#ifdef DUMPERS
    /* Dump the AST (after other frontend components) if requested. */
    if (cmd_opt[opt_dump_ast_after])
        AST_dump_interface(*int_p);
#endif

    /* Cancel filename for error processing because we are done with source */
    set_name_for_errors(NULL);

    return status;
}

/* preserve coding style vim: set tw=78 sw=4 : */
