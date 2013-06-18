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
**      command.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Definitions for IDL command line parsing.
**
**  VERSION: DCE 1.0
**
*/

#include <nidl.h>               /* IDL common defs */
#include <nametbl.h>            /* Nametable defs */

/*
 *  IDL compiler command line options are kept in two parallel arrays,
 *  cmd_opt and cmd_val.  These arrays are passed as arguments to any
 *  routines that need to access command line information.  The partial
 *  signature of such routines is:
 *
 *  rtn
 *  (
 *      boolean     *cmd_opt,        * [in] Array of command option flags *
 *      void        **cmd_val,       * [in] Array of command option values *
 *      ...
 *  )
 *
 *  cmd_opt is an array of booleans.  Each entry is set to "true" if the
 *  corresponding option is selected and set to "false" if it is not.
 *  Some options, when set to "true", contain additional information in
 *  the cmd_val array.
 *
 *  cmd_val is an array of (void *) elements.  If an element in the cmd_opt
 *  array is set to "false", the corresponding element in the cmd_val array
 *  will be equal to NULL.  If an element in the cmd_opt array is set to
 *  "true", the corresponding element in the cmd_val array will in general
 *  contain additional information for that option.  The obvious exceptions
 *  to this rule are any "True/False" options, for which no additional
 *  information is necessary.
 *
 *  Valid elements of the cmd_val array point to additional data for
 *  the corresponding option.  The additional data can be of a simple type
 *  or a constructed type, depending on the information needed to describe
 *  the option.  For most is it simply (char *), a pointer to a string.
 *
 *  The list of #define's below define a set of indices into the two arrays.
 *  The meaning of each option should be obvious from its index name, which
 *  closely resembles the corresponding command option.
 *
 *  Comments appear next to those options for which there is additional
 *  information in the cmd_val array.  The comment indicates the actual
 *  data type of the cmd_val array element for that option, and what it
 *  represents.
 *
 *  *NOTE*: When adding new options to the list below, be sure to also modify
 *          the opt_info array for the dump_cmd_data function (command.c).
 */

#define opt_caux             0  /* (char *)     Client auxiliary file name */
#define opt_cc_cmd           1  /* (char *)     C command line */
#define opt_cc_opt           2  /* (char *)     Addtl C command line options */
#define opt_cepv             3
#define opt_confirm          4
#define opt_cpp              5  /* (char *)     Filespec of CPP to invoke */
#define opt_cpp_def          6  /* (char **)    Array of define strs for CPP */
#define opt_cpp_opt          7  /* (char *)     Addtl CPP cmd line options */
#define opt_cpp_undef        8  /* (char **)    Array of undef strs for CPP */
#define opt_cstub            9  /* (char *)     Client stub file name */
#define opt_def_idir        10
#define opt_do_bug          11  /* (boolean *)  Array of "bug" flags */
#define opt_emit_cstub      12
#define opt_emit_sstub      13
#define opt_header          14  /* (char *)     Header file name */
#define opt_idir            15  /* (char **)    Array of include dirs */
#define opt_keep_c          16
#define opt_keep_obj        17
#define opt_mepv            18
#define opt_out             19  /* (char *)     Output directory */
#define opt_saux            20  /* (char *)     Server auxiliary file name */
#define opt_source          21  /* (char *)     Source IDL file name */
#define opt_space_opt       22
#define opt_sstub           23  /* (char *)     Server stub file name */
#define opt_stdin           24
#define opt_syntax_check    25
#define opt_verbose         26
#define opt_version         27
#define opt_warn            28
#define opt_dia             29
#define opt_standard        30	/* (int)     Standard level */
#define opt_midl            40  /* (bool)    MIDL compatibility mode */
#define opt_cstub_pref      41  /* (char*)   Client stub function prefix */
#define opt_sstub_pref      42  /* (char*)   Server stub function prefix */
#define opt_lang            43

/*
 * Remaining options are valid only when code built with DUMPERS.
 */
#ifndef DUMPERS
#define NUM_OPTS            opt_lang+1
#else
#define opt_dump_acf        opt_lang+1
#define opt_dump_ast        opt_dump_acf+1
#define opt_dump_ast_after  opt_dump_ast+1
#define opt_dump_cmd        opt_dump_ast_after+1
#define opt_dump_debug      opt_dump_cmd+1
#define opt_dump_flat       opt_dump_debug+1
#define opt_dump_mnode      opt_dump_flat+1
#define opt_dump_mool       opt_dump_mnode+1
#define opt_dump_nametbl    opt_dump_mool+1
#define opt_dump_recs       opt_dump_nametbl+1
#define opt_dump_sends      opt_dump_recs+1
#define opt_dump_unode      opt_dump_sends+1
#define opt_dump_uool       opt_dump_unode+1
#define opt_dump_yy         opt_dump_uool+1
#define opt_ool             opt_dump_yy+1
#define NUM_OPTS            opt_ool+1
#endif

/*
 * Indices into the array of booleans pointed to by cmd_val[opt_do_bug].
 * Note that valid indices start at 1, not 0!
 */
#define bug_array_align  1
#define bug_array_align2 2
#define bug_boolean_def  3
#define bug_array_no_ref_hole 4 /* Leave no hole for array of ref pointers */
#define NUM_BUGS         4

/* Flag values for check and standard */
#define opt_standard_dce_1_0 100
#define opt_standard_dec_1_0 105
#define opt_standard_dce_1_1 110


/* Data exported by command.c */

extern char *CMD_def_cpp_cmd;   /* Default cpp command */

/* Functions exported by command.c */

extern boolean CMD_parse_args(
    int             argc,
    char            **argv,
    boolean         **p_cmd_opt,
    void            ***p_cmd_val,
    STRTAB_str_t    *idl_sid
);

extern void CMD_explain_args(
    void
);

extern boolean add_def_string(
    char *def_string
);

extern boolean CMD_DCL_interface;
extern boolean   *CMD_opts; /* True/False values for command options */
extern void     **CMD_vals; /* Values associated w/ options (if any) */
