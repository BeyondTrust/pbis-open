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
**      driver.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Main "driver" module - Invokes each major compiler component.
**
**  VERSION: DCE 1.0
**
*/

#include <nidl.h>       /* IDL common defs */
#include <signal.h>
#include <setjmp.h>
#include <driver.h>

#include <ast.h>        /* Abstract Syntax Tree defs */
#include <command.h>    /* Command Line defs */
#include <errors.h>
#include <files.h>      /* File handling defs */
#include <backend.h>    /* Decl. of BE_main() */
#include <message.h>    /* reporting functions */
#include <frontend.h>   /* Declaration of FE_main() */
#if defined(MIA) && (defined(VMS) || defined(__osf__) || defined(DUMPERS))
static char *saved_header_file; /* Saved header filespec */
#endif

/* Macro to call a function and return if boolean return status is FALSE */
#define CALL(rtn) \
    { \
    status = rtn; \
    if (!status) \
        {nidl_terminate();} \
    }

/*
 * setjmp/longjmp buffer to use for termination during fatal errors
 */
static jmp_buf nidl_termination_jmp_buf;

/*
** n i d l _ t e r m i n a t e
**
** This routine utilizes setjmp/longjmp to perform an orderly termination
** of the idl compiler.
*/
void nidl_terminate (
void
)
{
    extern void *errors;
    if (errors == NULL)
    {
        /*
         * No errors logged, yet we're terminating.  Better issue a generic
         * "compilation aborted" message since this is probably an unhandled
         * internal compiler error.
         */
        message_print(NIDL_COMPABORT);
    }
    longjmp(nidl_termination_jmp_buf,1);
}



/*
** a t t e m p t _ t o _ p r i n t _ e r r o r s
**
** This routine is established as an exception handler with the base system
** such that we always attempt to print out error messages even in the case of
** an error in the compiler.  The print_errors routine, which we call is coded
** such that we cannot get into an infinite loop.
*/
static long attempt_to_print_errors()
{
#if !defined(vms)
#ifndef _MSDOS
    signal(SIGBUS,SIG_DFL);
#endif
    signal(SIGSEGV,SIG_DFL);
    signal(SIGFPE,SIG_DFL);
    signal(SIGILL,SIG_DFL);
#endif
    /*
     * Try printing out the errors, if there are none, or it was
     * attempted before, this call will just return.
     */
    print_errors();
    return 0;
}


/*
**  o p e n _ f e _ f i l e s
**
**  Opens all output files that are processed by the frontend, and returns the
**  file handles.  Note that if an optional file was not selected by the user,
**  the corresponding file handle will be NULL.
**
**  Returns:    FALSE if an error occurs opening any of the files.
*/

static boolean open_fe_files
(
    boolean     *cmd_opt ATTRIBUTE_UNUSED,       /* [in] Array of command option flags */
    void        **cmd_val ATTRIBUTE_UNUSED,      /* [in] Array of command option values */
    FILE        **lis_fid       /*[out] Listing file handle */
)

{
    /* Set up default return values. */
    *lis_fid = NULL;

    /* Create listing file if specified. */
    /* (add code when listing file supported) */

    return TRUE;
}

/*
**  o p e n _ b e _ f i l e s
**
**  Opens all output files that are processed by the backend, and returns the
**  file handles.  Note that if an optional file was not selected by the user,
**  the corresponding file handle will be NULL.
**
**  Implicit Inputs:    cmd_opt[opt_confirm] : If FALSE, normal processing.
**                      If TRUE, no files are actually created, but
**                      remaining processing remains intact.
**
**  Implicit Outputs:   cmd_opt flags for files are changed to reflect
**                      whether the files have been created or not.
**
**  Returns:    FALSE if an error occurs opening any of the files.
**              FALSE if no files were opened (none selected by user).
**              TRUE otherwise.
*/

static boolean open_be_files
(
    boolean     *cmd_opt,       /* [in] Array of command option flags */
    void        **cmd_val,      /* [in] Array of command option values */
    FILE        **h_fid,        /*[out] Header file handle */
    FILE        **caux_fid,     /*[out] Client auxiliary file handle */
    FILE        **saux_fid,     /*[out] Server auxiliary file handle */
    FILE        **cstub_fid,    /*[out] Client stub file handle */
    FILE        **sstub_fid,    /*[out] Server stub file handle */
    AST_interface_n_t *int_p    /* [in] Ptr to interface node */
)

{
    AST_export_n_t  *export_p;          /* Ptr to export node */
    boolean         stubs_required;     /* TRUE if stub generation required */
    boolean         status;             /* Modified by CALL macro */
    boolean         all_encode_decode;  /* TRUE if all opers encode or decode */

    /* Set up default return values. */
    *caux_fid   = NULL;
    *saux_fid   = NULL;
    *h_fid      = NULL;
    *cstub_fid  = NULL;
    *sstub_fid  = NULL;

    /* Create header file if specified. */
    if (cmd_opt[opt_header])
    {
        if (cmd_opt[opt_verbose])
            message_print(NIDL_INCLCREATE, (char *)cmd_val[opt_header]);
        if (!cmd_opt[opt_confirm])
        {
            CALL(FILE_create((char *)cmd_val[opt_header], h_fid));
            fprintf(*h_fid, IDL_VERSION_TEMPLATE, IDL_VERSION_TEXT);
        }
    }

    /*
     * If there are no operations exported by the interface, or if the [local]
     * interface attribute is set, stub generation is not necessary.
     */
    stubs_required = FALSE;
    all_encode_decode = TRUE;

    if (!AST_LOCAL_SET(int_p))
    {
        for (export_p = int_p->exports;
             export_p != NULL;
             export_p = export_p->next)
            if (export_p->kind == AST_operation_k)
            {
                stubs_required = TRUE;
                if (!AST_ENCODE_SET(export_p->thing_p.exported_operation)
                    && !AST_DECODE_SET(export_p->thing_p.exported_operation))
                {
                    all_encode_decode = FALSE;
                    break;
                }
            }
    }

    /* Create client stub file if necessary. */
    if (stubs_required
        &&  cmd_opt[opt_cstub]
        &&  cmd_opt[opt_emit_cstub])
    {
        if (cmd_opt[opt_verbose])
            message_print(NIDL_STUBCREATE, (char *)cmd_val[opt_cstub]);
        if (!cmd_opt[opt_confirm])
        {
            CALL(FILE_create((char *)cmd_val[opt_cstub], cstub_fid))
            fprintf(*cstub_fid, IDL_VERSION_TEMPLATE, IDL_VERSION_TEXT);
        }
    }
    else
        cmd_opt[opt_cstub] = FALSE;

    /* Create server stub file if necessary. */
    if (stubs_required
        &&  !all_encode_decode
        &&  cmd_opt[opt_sstub]
        &&  cmd_opt[opt_emit_sstub])
    {
        if (cmd_opt[opt_verbose])
            message_print(NIDL_STUBCREATE, (char *)cmd_val[opt_sstub]);
        if (!cmd_opt[opt_confirm])
        {
            CALL(FILE_create((char *)cmd_val[opt_sstub], sstub_fid))
            fprintf(*sstub_fid, IDL_VERSION_TEMPLATE, IDL_VERSION_TEXT);
        }
    }
    else
        cmd_opt[opt_sstub] = FALSE;

    /* Create Client auxiliary file if it is necessary. */
#ifdef MIA
  if (getenv("IDL_GEN_AUX_FILES") != NULL)
#endif
  {
    if (cmd_opt[opt_caux]
        &&  (int_p->sp_types != NULL
            ||  int_p->ool_types != NULL))
    {
        if (cmd_opt[opt_verbose])
            message_print(NIDL_STUBCREATE, (char *)cmd_val[opt_caux]);
        if (!cmd_opt[opt_confirm])
        {
            CALL(FILE_create((char *)cmd_val[opt_caux], caux_fid))
            fprintf(*caux_fid, IDL_VERSION_TEMPLATE, IDL_VERSION_TEXT);
#ifdef MIA
            fprintf(*caux_fid,"static int idl_aux_stub = 0;\n");
#endif
        }
    }
    else
        cmd_opt[opt_caux] = FALSE;

    /* Create Server auxiliary file if it is necessary. */
    if (cmd_opt[opt_saux]
        &&  (int_p->sp_types != NULL
            ||  int_p->ool_types != NULL
            ||  int_p->pipe_types != NULL))
    {
        if (cmd_opt[opt_verbose])
            message_print(NIDL_STUBCREATE, (char *)cmd_val[opt_saux]);
        if (!cmd_opt[opt_confirm])
        {
            CALL(FILE_create((char *)cmd_val[opt_saux], saux_fid))
            fprintf(*saux_fid, IDL_VERSION_TEMPLATE, IDL_VERSION_TEXT);
#ifdef MIA
            fprintf(*saux_fid,"static int idl_aux_stub = 0;\n");
#endif
        }
    }
    else
        cmd_opt[opt_saux] = FALSE;
  } 
#ifdef MIA
  else
  {
        cmd_opt[opt_caux] = FALSE;
        cmd_opt[opt_saux] = FALSE;
  }
#endif

    return (*caux_fid   != NULL
            ||  *saux_fid   != NULL
            ||  *h_fid      != NULL
            ||  *cstub_fid  != NULL
            ||  *sstub_fid  != NULL);
}

/*
**  s t u b _ c o m p i l e
**
**  Conditionally invokes the C compiler to compile a generated stub file,
**  directing the output file to the same directory as the source file.
**  Conditionally outputs message that stub is being compiled.
*/

static int stub_compile
(
    boolean     *cmd_opt,       /* [in] Array of command option flags */
    void        **cmd_val,      /* [in] Array of command option values */
    int         opt_file,       /* [in] Index of stub file to process */
    FILE        *fid,           /* [in] File handle of stub file */
    char        *compile_cmd    /* [in] Base command to compile stub */
)

{
    char    compile_opt[max_string_len];
    char    filespec[PATH_MAX];

    /* If file was disabled, just return. */
    if (!cmd_opt[opt_file])
        return pgm_ok;

    /* Conditionally output "Compiling stub file" message. */
    if (cmd_opt[opt_verbose] && error_count == 0 && cmd_opt[opt_confirm])
        message_print(NIDL_STUBCOMPILE, (char *)cmd_val[opt_file]);

    /* If this is just a -confirm pass then return now. */
    if (cmd_opt[opt_confirm] || fid == NULL)
        return pgm_ok;

    compile_opt[0] = '\0';

    FILE_parse((char *)cmd_val[opt_file], filespec, (char *)NULL, (char *)NULL);
    if (!FILE_is_cwd(filespec))
#ifdef CC_OPT_OBJECT
        /*
         * Make sure the object file ends up in the same directory as source
         * file by forming the object file name from the source file name and
         * constructing a command option to the C compiler.  (This can't be
         * done, however, if the user has specified a -cc_cmd other than the
         * default, since we can't be sure of valid C compiler options.)
         */
        if (strcmp((char *)cmd_val[opt_cc_cmd], CC_DEF_CMD) == 0)
        {
            FILE_form_filespec((char *)NULL, (char *)NULL, OBJ_FILETYPE,
                               (char *)cmd_val[opt_file], filespec);

            sprintf(compile_opt, "%s%s", CC_OPT_OBJECT, filespec);
        }
        else
#endif
            message_print(NIDL_OUTDIRIGN, (char *)cmd_val[opt_file]);

    return FILE_execute_cmd(compile_cmd, compile_opt, (char *)cmd_val[opt_file],
        ((cmd_opt[opt_verbose]) ? NIDL_STUBCOMPILE: 0)
        );
}

/*
**  c l o s e _ f i l e s
**
**  Closes any output files that were opened during the compilation.
*/

static void close_files
(
    FILE        *lis_fid,       /* [in] Listing file handle */
    FILE        *h_fid,         /* [in] Header file handle */
    FILE        *caux_fid,      /* [in] Client auxiliary file handle */
    FILE        *saux_fid,      /* [in] Server auxiliary file handle */
    FILE        *cstub_fid,     /* [in] Client stub file handle */
    FILE        *sstub_fid      /* [in] Server stub file handle */
)

{
    if (lis_fid     != NULL) fclose(lis_fid);
    if (h_fid       != NULL) fclose(h_fid);
    if (caux_fid    != NULL) fclose(caux_fid);
    if (saux_fid    != NULL) fclose(saux_fid);
    if (cstub_fid   != NULL) fclose(cstub_fid);
    if (sstub_fid   != NULL) fclose(sstub_fid);
}

/*
**  i n i t
**
**  Does any initializations that are global to the entire compiler.
**  Component-specific initialization should be done by a separate
**  initialization function for that component.
*/

static boolean init(char *image_name)       /* Returns TRUE on success */

{
    /* Open error message database. */
    message_open(image_name);

    return TRUE;
}


/*
**  c l e a n u p
**
**  Does any cleanup that is global to the entire compiler.
**  Component-specific cleanup should be done by a separate
**  cleanup function for that component.
*/

static boolean cleanup(void)    /* Returns TRUE on success */

{
    /* Close error message database. */
    message_close();


    return TRUE;
}

/*
**  D R I V E R _ m a i n
**
**  Invokes each major compiler component.
*/

boolean DRIVER_main
(
    int         argc,           /* Command line argument count */
    char        **argv          /* Array of command line arguments */
)

{
    boolean     *cmd_opt;       /* Array of command option flags */
    void        **cmd_val;      /* Array of command option values */
    STRTAB_str_t idl_sid;       /* IDL filespec stringtable ID */
    FILE        *lis_fid;       /* Listing file handle */
    FILE        *h_fid;         /* Header file handle */
    FILE        *caux_fid;      /* Client auxiliary file handle */
    FILE        *saux_fid;      /* Server auxiliary file handle */
    FILE        *cstub_fid;     /* Client stub file handle */
    FILE        *sstub_fid;     /* Server stub file handle */
    AST_interface_n_t *int_p;   /* Ptr to interface node */
    boolean     status = TRUE;  /* Assume we are successful */
    int         tmpsts;
    /*
     * Null out all file pointers in case we get an error we know which
     * have been opened.
     */
    lis_fid = NULL;
    h_fid = NULL;
    caux_fid = NULL;
    saux_fid = NULL;
    cstub_fid = NULL;
    sstub_fid = NULL;

    cmd_opt = NULL;
    cmd_val = NULL;


    /*
     * Establish a handler such that we always try to output the
     * error messages, even when the compiler has an internal error.
     */
#ifdef vms
    VAXC$ESTABLISH(attempt_to_print_errors);
#else
#ifndef _MSDOS
    signal(SIGBUS, (void (*)())attempt_to_print_errors);
#endif
    signal(SIGSEGV, (void (*)())attempt_to_print_errors);
    signal(SIGFPE, (void (*)())attempt_to_print_errors);
    signal(SIGILL, (void (*)())attempt_to_print_errors);
#endif


    /*
     * This point is established for orderly termination of the compilation.
     * If a fatal error is detected, execution continues following this
     * if statement and the normal cleanup is performed.
     */
    if (setjmp(nidl_termination_jmp_buf) == 0)
    {
        /* Global initialization (not specific to any one component). */
        CALL(init(argv[0]));

        /*
         * Command line parsing.  **NOTE**: If the combination of options
         * -confirm -v is selected, control *will* return back here; thus
         * make sure not to call any component if cmd_opt[opt_confirm] is true.
         */
        CALL(CMD_parse_args(argc-1, &argv[1], &cmd_opt, &cmd_val, &idl_sid));

        /*
         * Open output files that are processed by the frontend.  Returned file
         * handles are null if not applicable.  The source IDL files, any imported
         * IDL files, and any ACF files are opened by the parse routines.
         */
        if (!cmd_opt[opt_confirm])
            CALL(open_fe_files(cmd_opt, cmd_val, &lis_fid));

        /* Frontend processing. */
        CALL(FE_main(cmd_opt, cmd_val, idl_sid, &int_p));

        /* Print accumulated errors and warnings generated by frontend. */
        if (cmd_opt == NULL || !cmd_opt[opt_confirm])
            print_errors();

        /* Backend processing. */
        if (open_be_files(cmd_opt, cmd_val, &h_fid, &caux_fid,
                &saux_fid, &cstub_fid, &sstub_fid, int_p))
        {
            char    filename[PATH_MAX];
            char    filetype[PATH_MAX];
            char    *saved_header;

            /* Prune the header filespec for BE so only the leafname remains. */
            if (cmd_val[opt_header] != NULL)
            {
                FILE_parse((char *)cmd_val[opt_header], NULL, filename, filetype);
                strcat(filename, filetype);
            }
            else
                filename[0] = '\0';
            saved_header = (char *)cmd_val[opt_header];
#if defined(MIA) && (defined(VMS) || defined(__osf__) || defined(DUMPERS))
            saved_header_file = saved_header; /* Static copy for BE callback */
#endif
            cmd_val[opt_header] = (void *)filename;

            CALL(BE_main(cmd_opt, cmd_val, h_fid, caux_fid, saux_fid,
                         cstub_fid, sstub_fid, int_p));

            cmd_val[opt_header] = (void *)saved_header;
        }
    }
    else status = FALSE;

    /* Generation of listing file. */
    /* (add call here) */

    /* Statistics reporting. */
    /* (add call here) */

    /* Print accumulated errors and warnings generated by the backend, if any. */
    if (cmd_opt == NULL || !cmd_opt[opt_confirm])
        print_errors();

    /* Close all output files. */
    close_files(lis_fid, h_fid, caux_fid, saux_fid,
                cstub_fid, sstub_fid);

    /* Compile any generated files if objects were requested */
    if (cmd_opt != NULL && cmd_opt[opt_keep_obj] && status)
    {
        char **idir_list;               /* Array of include directories */
        char idir_opt[max_string_len];  /* Cmd list of include directories */
        char cc_cmd[max_string_len];    /* Base command line and options */
#ifdef VMS
        boolean     paren_flag;         /* TRUE if trailing paren needed */
#endif

        /* Paste together command line option for include directories. */

#if defined(UNIX) || defined(VMS)
        idir_list = (char **)cmd_val[opt_idir];
        idir_opt[0] = '\0';

#ifdef VMS
        if (*idir_list || cmd_opt[opt_out])
        {
            paren_flag = TRUE;
            strcat(idir_opt, " /INCLUDE_DIRECTORY=(");
        }
        else
            paren_flag = FALSE;
#endif

        /* If an -out directory was specified, place it at front of idir list */
        if (cmd_opt[opt_out])
        {
#ifdef UNIX
            strcat(idir_opt, " -I");
#endif
            strcat(idir_opt, (char *)cmd_val[opt_out]);
#ifdef VMS
            strcat(idir_opt, ",");
#endif
        }

        while (*idir_list)
        {
#ifdef UNIX
            strcat(idir_opt, " -I");
#endif
            /*
             * If this include dir is the system IDL dir, then replace it
             * with the system H dir, which might be different.
             */
            if (strcmp(*idir_list, DEFAULT_IDIR) == 0)
                strcat(idir_opt, DEFAULT_H_IDIR);
            else
                strcat(idir_opt, *idir_list);
            idir_list++;
#ifdef VMS
            strcat(idir_opt, ",");
#endif
        }

#ifdef VMS
        if (paren_flag)
            /* Overwrite trailing comma with paren. */
            idir_opt[strlen(idir_opt)-1] = ')';
#endif
#endif

        /* Paste together base command line. */

#ifdef PASS_I_DIRS_TO_CC
        sprintf(cc_cmd, "%s %s %s", (char *)cmd_val[opt_cc_cmd],
                (char *)cmd_val[opt_cc_opt], idir_opt);
#else
        sprintf(cc_cmd, "%s %s", (char *)cmd_val[opt_cc_cmd],
                (char *)cmd_val[opt_cc_opt]);
#endif

        /* Now compile the stub modules. */

        tmpsts = stub_compile(cmd_opt, cmd_val, opt_cstub, cstub_fid, cc_cmd);
        if (ERROR_STATUS(tmpsts)) status = FALSE;
	else
	    tmpsts = stub_compile(cmd_opt, cmd_val, opt_sstub, sstub_fid, cc_cmd);
        if (ERROR_STATUS(tmpsts)) status = FALSE;
	else
	    tmpsts = stub_compile(cmd_opt, cmd_val, opt_caux, caux_fid, cc_cmd);
        if (ERROR_STATUS(tmpsts)) status = FALSE;
	else
	    tmpsts = stub_compile(cmd_opt, cmd_val, opt_saux, saux_fid, cc_cmd);
        if (ERROR_STATUS(tmpsts)) status = FALSE;
    }

    /* If no request to keep C files, delete each of them. */
    if (cmd_opt != NULL && !cmd_opt[opt_keep_c] && status)
    {
        if (cmd_opt[opt_cstub])
        {
            if (cmd_opt[opt_verbose] && error_count == 0)
                message_print(NIDL_STUBDELETE, (char *)cmd_val[opt_cstub]);
            if (!cmd_opt[opt_confirm] && cstub_fid)
                FILE_delete((char *)cmd_val[opt_cstub]);
        }

        if (cmd_opt[opt_sstub])
        {
            if (cmd_opt[opt_verbose] && error_count == 0)
                message_print(NIDL_STUBDELETE, (char *)cmd_val[opt_sstub]);
            if (!cmd_opt[opt_confirm] && sstub_fid)
                FILE_delete((char *)cmd_val[opt_sstub]);
        }

        if (cmd_opt[opt_caux])
        {
            if (cmd_opt[opt_verbose] && error_count == 0)
                message_print(NIDL_STUBDELETE, (char *)cmd_val[opt_caux]);
            if (!cmd_opt[opt_confirm] && caux_fid)
                FILE_delete((char *)cmd_val[opt_caux]);
        }

        if (cmd_opt[opt_saux])
        {
            if (cmd_opt[opt_verbose] && error_count == 0)
                message_print(NIDL_STUBDELETE, (char *)cmd_val[opt_saux]);
            if (!cmd_opt[opt_confirm] && saux_fid)
                FILE_delete((char *)cmd_val[opt_saux]);
        }
    }

    /* Global cleanup. */
    if (!cleanup()) status = FALSE;

    return status;
}

