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
**      command.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  IDL command line parsing.
**
**  VERSION: DCE 1.0
**
*/

#include <nidl.h>       /* IDL common defs */
#include <command.h>

#include <files.h>      /* File handling defs */
#include <getflags.h>   /* Command line parsing defs */
#include <message.h>    /* reporting functions */

#ifndef MAX_DEF_STRINGS
# define MAX_DEF_STRINGS 10
#endif

#ifndef MAX_DUMP_STRINGS
# define MAX_DUMP_STRINGS 10
#endif

#ifndef MAX_IMPORT_DIRECTORIES
# define MAX_IMPORT_DIRECTORIES 50
#endif

extern boolean  ERR_no_warnings;    /* Global copy of -no_warn cmd option */
extern char     *last_string;       /* Last string parsed from cmd line */

#ifdef MSDOS
static int max_suffix_len;
#endif

static boolean  cmd_opt[NUM_OPTS];  /* True/False values for command options */
static void     *cmd_val[NUM_OPTS]; /* Values associated w/ options (if any) */

/* Global versions of the command options */
       boolean *CMD_opts = (boolean*)cmd_opt;
       void    **CMD_vals = (void**)cmd_val;

static char     *UNSPECIFIED = "";
static char     *nidl_library = NULL;

static int      do_bug[NUM_BUGS];
static int      do_not_do_bug[NUM_BUGS];
static boolean  support_bug[NUM_BUGS + 1];

static char     *caux_suffix    = CAUX_SUFFIX,
                *caux_file;

static char     *cc_cmd;
static char     *cc_opt;

static char     *client;
#define client_none 0
#define client_stub 1
#define client_aux  2
#define client_all  3
static char     *client_vals[]    = { "none", "stub", "aux", "all", NULL };

#if defined(VMS) || defined(UNIX)
char            *CMD_def_cpp_cmd;
static char     *cpp_cmd;
static char     *cpp_opt;
#endif

static char     *cstub_suffix   = CSTUB_SUFFIX,
                *cstub_file;

static char     *(def_strings[MAX_DEF_STRINGS + 1]);
static char     *(undef_strings[MAX_DEF_STRINGS + 1]);

#ifdef DUMPERS
static char     *dump_strings[MAX_DUMP_STRINGS+1];
#define dump_acf        0
#define dump_ast        1
#define dump_ast_after  2
#define dump_cmd        3
#define dump_debug      4
#define dump_flat       5
#define dump_mnode      6
#define dump_mool       7
#define dump_nametbl    8
#define dump_recs       9
#define dump_sends     10
#define dump_unode     11
#define dump_uool      12
#define dump_yy        13
static char     *dump_vals[]    = { "acf", "ast", "ast_after", "cmd", "debug",
                                    "flat", "mnode", "mool", "nametbl", "recs",
                                    "sends", "unode", "uool", "yy", NULL };
#endif

static char     *header_suffix  = HEADER_SUFFIX,
                *header_file;

/*  List of import directories - allow two extra slots for the implicit
 *  -I CD_DIR -I DEFAULT_IDIR and one for the sentinel.
 */
static char     *(import_directories[MAX_IMPORT_DIRECTORIES + 2 + 1]);

static char     *keep;
#define keep_none       0
#define keep_c_source   1
#define keep_object     2
#define keep_both       3
#define keep_all        4
static char     *keep_vals[]    = { "none", "c_source", "object", "both",
                                    "all", NULL  };

static char     *out_dir;

static char     *saux_suffix    = SAUX_SUFFIX,
                *saux_file;

static char     *server;
#define server_none 0
#define server_stub 1
#define server_aux  2
#define server_all  3
static char     *server_vals[]    = { "none", "stub", "aux", "all", NULL };

static char     *sstub_suffix   = SSTUB_SUFFIX,
   
             *sstub_file;
boolean CMD_DCL_interface = FALSE;
static char     *standard = "extended";
static int      standard_opt;
static char     *standard_vals[] = {
                "portable", "dce_v10", "dce_v11", "extended", NULL };
static int      standard_ivals[] = {
                opt_standard_dce_1_0, opt_standard_dce_1_0,
                opt_standard_dce_1_1, opt_standard_dce_1_1 };

#define FD(x)   (FLAGDEST)&x
#define FDV(x)  (FLAGDEST)x

OPTIONS option_table[]={
	{"bug",          VINTARG(NUM_BUGS),              FDV(do_bug)},
	{"caux",         STRARG,                         FD(caux_file)},
	{"cc_cmd",       STRARG,                         FD(cc_cmd)},
	{"cc_opt",       STRARG,                         FD(cc_opt)},
	{"cepv",         ASSERTARG,                      FD(cmd_opt[opt_cepv])},
	{"client",       STRARG,                         FD(client)},
	{"confirm",      ASSERTARG|HIDARG,               FD(cmd_opt[opt_confirm])},
#if defined(VMS) || defined(UNIX)
	{"cpp_cmd",      OSTRARG,                        FD(cpp_cmd)},
	{"cpp_opt",      STRARG,                         FD(cpp_opt)},
#endif
	{"cstub",        STRARG,                         FD(cstub_file)},
#ifdef VMS
	{"d",            VSTRARG(MAX_DEF_STRINGS),       FDV(def_strings)},
#else
	{"D",            VSTRARG(MAX_DEF_STRINGS),       FDV(def_strings)},
#endif
#ifdef DUMPERS
	{"dump",         VSTRARG(MAX_DUMP_STRINGS)|HIDARG,FDV(dump_strings)},
#endif
	{"header",       OSTRARG,                        FD(header_file)},
#ifdef VMS
	{"i",            VSTRARG(MAX_IMPORT_DIRECTORIES),FDV(import_directories)},
#else
	{"I",            VSTRARG(MAX_IMPORT_DIRECTORIES),FDV(import_directories)},
#endif
	{"keep",         STRARG,                         FD(keep)},
	{"midl",         ASSERTARG|HIDARG,               FD(cmd_opt[opt_midl])},
	{"no_bug",       VINTARG(NUM_BUGS),              FDV(do_not_do_bug)},
	{"no_cpp",       DENYARG,                        FD(cmd_opt[opt_cpp])},
	{"no_def_idir",  DENYARG,                        FD(cmd_opt[opt_def_idir])},
	{"no_header",    DENYARG|HIDARG,                 FD(cmd_opt[opt_header])},
	{"no_mepv",      DENYARG,                        FD(cmd_opt[opt_mepv])},
	{"no_warn",      DENYARG,                        FD(cmd_opt[opt_warn])},
#ifdef DUMPERS
	{"ool",          ASSERTARG|HIDARG,               FD(cmd_opt[opt_ool])},
#endif
	{"out",          STRARG,                         FD(out_dir)},
	{"saux",         STRARG,                         FD(saux_file)},
	{"server",       STRARG,                         FD(server)},
	{"space_opt",    ASSERTARG,                      FD(cmd_opt[opt_space_opt])},
	{"sstub",        STRARG,                         FD(sstub_file)},
	{"cstub_pref",   STRARG,                         FD(cmd_val[opt_cstub_pref])},
	{"sstub_pref",   STRARG,                         FD(cmd_val[opt_sstub_pref])},
	{"standard",     STRARG,                         FD(standard)},
#ifdef UNIX
	{"stdin",        ASSERTARG,                      FD(cmd_opt[opt_stdin])},
#endif
	{"syntax_only",  ASSERTARG,                      FD(cmd_opt[opt_syntax_check])},
#ifdef VMS
	{"u",            VSTRARG(MAX_DEF_STRINGS),       FDV(undef_strings)},
#else
	{"U",            VSTRARG(MAX_DEF_STRINGS),       FDV(undef_strings)},
#endif
	{"v",            ASSERTARG|HIDARG,               FD(cmd_opt[opt_verbose])},
	{"version",      ASSERTARG|HIDARG,               FD(cmd_opt[opt_version])},
	{0,              0,                              0}
};

/*
**  C M D _ e x p l a i n _ a r g s
**
**  Explains command line arguments.
*/

void CMD_explain_args(void)

{
    message_print(NIDL_USAGE);

    /*
     * Don't print full list of command options here unless -confirm
     * specified, but let user know there is a way to do it.
     */
    /** When VMS DCL syntax is added, will need to conditionalize arg below **/
    if (cmd_opt[opt_confirm])
        printflags(option_table);
    else
        message_print(NIDL_CMDERR, "-confirm");
}

/*
**  c h e c k _ s t r _ l i s t
**
**  Checks a string argument against a list of legal
**  values.  Issues error and exits for illegal value.
**
**  Returns:    Index of match.  Returns -1 for null string.
*/

static int check_str_list
(
    char        *opt,           /* [in] Name of command option */
    char        *val,           /* [in] Value assigned to command option */
    char        **list          /* [in] List of legal values for cmd option */
)

{
    int i;      /* List index */

    if (val[0] == '\0')
        return -1;

    for (i = 0 ; list[i] != NULL ; i++ )
        if (strcmp(val, list[i]) == 0)
            return i;

    message_print(NIDL_INVOPTION, opt, val);
    message_print(NIDL_LEGALVALS);

    for ( ; *list != NULL ; list++ )
        fprintf(stderr, " %s", *list);

    fprintf(stderr, "\n");
    exit(pgm_error);
}

/*
**  c h e c k _ s t r _ i n t _ l i s t
**
**  Checks a string argument against a list of legal
**  values.  Issues error and exits for illegal value.
**
**  Returns:    Integer corresponding to matched string.  Integer is
**              obtained from array using index of matched string.
**              Returns -1 for null string.
*/

static int check_str_int_list
(
    char        *opt,           /* [in] Name of command option */
    char        *val,           /* [in] Value assigned to command option */
    char        **list,         /* [in] List of legal values for cmd option */
    int         *ilist          /* [in] List of corresponding integer values */
)

{
    int i;      /* List index */

    if (val[0] == '\0')
        return -1;

    for (i = 0 ; list[i] != NULL ; i++ )
        if (strcmp(val, list[i]) == 0)
            return ilist[i];

    message_print(NIDL_INVOPTION, opt, val);
    message_print(NIDL_LEGALVALS);

    for ( ; *list != NULL ; list++ )
        fprintf(stderr, " %s", *list);

    fprintf(stderr, "\n");
    exit(pgm_error);
}

/*
**  d u m p _ c m d _ d a t a
**
**  Dump state of internal command vectors.
*/

#ifdef DUMPERS
typedef enum {bit, string, string_list, int_list, number} opt_kind_t;
typedef struct
{
    char *opt_name;
    opt_kind_t opt_kind;
} opt_struct;

/*
 * Entries in this table must be consistent with definitions in command.h.
 */
static opt_struct opt_info[NUM_OPTS] =
{
    { "caux",               string },
    { "cc_cmd",             string },
    { "cc_opt",             string },
    { "cepv",               bit },
    { "confirm",            bit },
    { "cpp_cmd",            string },
    { "cpp_def",            string_list },
    { "cpp_opt",            string },
    { "cpp_undef",          string_list },
    { "cstub",              string },
    { "def_idir",           bit },
    { "do_bug",             int_list },
    { "emit_cstub",         bit },
    { "emit_sstub",         bit },
    { "header",             string },
    { "idir",               string_list },
    { "keep_c",             bit },
    { "keep_obj",           bit },
    { "mepv",               bit },
    { "out",                string },
    { "saux",               string },
    { "source",             string },
    { "space_opt",          bit },
    { "sstub",              string },
    { "stdin",              bit },
    { "syntax_only",        bit },
    { "v",                  bit },
    { "version",            bit },
    { "warn",               bit },
    { "standard",           number },
    { "cstub_pref",         string },
    { "sstub_pref",         string },
    { "midl",               bit}
#ifdef DUMPERS
    ,
    { "dump_acf",           bit },
    { "dump_ast",           bit },
    { "dump_ast_after",     bit },
    { "dump_cmd",           bit },
    { "dump_debug",         bit },
    { "dump_flat",          bit },
    { "dump_mnode",         bit },
    { "dump_mool",          bit },
    { "dump_nametbl",       bit },
    { "dump_recs",          bit },
    { "dump_sends",         bit },
    { "dump_unode",         bit },
    { "dump_uool",          bit },
    { "dump_yy",            bit },
    { "ool",                bit }
#endif
};

static void dump_cmd_data(void)

{
    int     i;          /* Option index */
    int     j;          /* Table index */
    int     tbl_size;   /* Table size */
    char    **pstr;     /* Ptr to string table entry */
    int     *pint;      /* Ptr to integer table entry */

    printf("\n");

    for (i = 0 ; i < NUM_OPTS ; i++)
    {
        printf("%-20s", opt_info[i].opt_name);

        if (cmd_opt[i])
            printf("true    ");
        else
            printf("false   ");

        switch (opt_info[i].opt_kind)
        {
        case bit:
            printf("\n");
            break;

        case string:
            if (cmd_val[i] != NULL)
                printf(" %s\n", (char*)cmd_val[i]);
            else
                printf(" \n");
            break;

        case string_list:
            pstr = (char **)cmd_val[i];
            if (pstr != NULL)
                while (*pstr != NULL)
                    printf(" %s", *pstr++);
            printf("\n");
            break;

        case int_list:
            pint = (int *)cmd_val[i];
            tbl_size = flags_option_count(option_table, opt_info[i].opt_name);
            for (j = 0 ; j < tbl_size ; j++)
                printf(" %d", *pint++);
            printf("\n");
            break;

        case number:
            pint = (int *)cmd_val[i];
	    printf(" %d\n", *pint++);
            break;

        default:
            printf("**Error**: Unsupported opt_kind in dump_cmd_data.\n");
        }
    }
}
#endif

/*
**  a l l o c _ a n d _ c o p y
**
**  Allocates memory for and copies a string to a return string.
*/

static char *alloc_and_copy     /* Returns address of new string */
(
    char    *orig_str           /* String to copy */
)

{
    char    *new_str;           /* Local ptr to new string */

    if (orig_str == NULL || orig_str[0] == '\0')
        return UNSPECIFIED;     /* Empty string */

    new_str = NEW_VEC (char, strlen(orig_str) + 1);

    strcpy(new_str, orig_str);

    return new_str;
}

/*
** a d d _ d e f _ s t r i n g
**
** Adds specified string to the array of symbols defined for
** preprocessor input.
*/

boolean add_def_string
(
    char *def_string           /* [in] Additional #define string for preprocessor input */
)

{
    char **defs = (char**) cmd_val[opt_cpp_def];
    char *def;
    int len, i = 0;

    len = 1;       /* just to start the loop */
    def = defs[i];

    /* find the last def string */
    while (i < MAX_DEF_STRINGS && (def != NULL && len > 0))
    {
	if (def != NULL)
	{
	    /* it makes no sens to define the same thing twice */
	    if (!strcmp(def, def_string)) return true;
	    len = strlen(def);
	}
	def = defs[++i];
    }

    /* add only if there's enough space */
    if (i < MAX_DEF_STRINGS)
    {
        defs[i] = alloc_and_copy(def_string);
	return true;
    }

    return false;
}

/*
**  g e t _ s r c _ f i l e s p e c
**
**  Gets the source filespec from the command line, if any.  When it is
**  ambiguous as to which parameter on the command line specifies the source
**  IDL file, the rightmost parameter that can sensibly specify the source
**  IDL file is chosen.
**
**  Returns:    TRUE if a source file was specified; FALSE otherwise
*/

static boolean get_src_filespec
(
    char    *src_filespec       /* [out] Source filespec */
)

{
    int     other_count;        /* Parameter cnt (excl. option cnt) */

    other_count = flags_other_count();

    /* Check for invalid extra arguments. */
    if (other_count > 1)
    {
        int i;
        CMD_explain_args();

        message_print(NIDL_INVPARAMS);
        for (i = 1; i < other_count ; i++)
            fprintf(stderr, "  %s", flags_other(i));
        fprintf(stderr, "\n");

        return FALSE;
    }

    /*
     * If one argument, it must be the source filespec if -stdin was not
     * specified.  It -stdin was specified, it is an invalid extra argument.
     */
    if (other_count == 1)
    {
        if (cmd_opt[opt_stdin])
        {
            CMD_explain_args();
            message_print(NIDL_INVPARAMS);
            fprintf(stderr, "  %s\n", flags_other(0));
            return FALSE;
        }
        else
        {
            strcpy(src_filespec, flags_other(0));
            return TRUE;
        }
    }

    /* No arguments.  Fine if -stdin selected. */
    if (cmd_opt[opt_stdin])
    {
        src_filespec[0] = '\0';
        return TRUE;
    }

    /*
     * We do not have an obvious source filespec on the command line as parsed.
     * However, the command line format is ambiguous.  Any command option that
     * takes an OPTIONAL string argument might have swallowed up what was
     * really intended to be the source filespec.  The getflags function saves
     * us the last such optional string that was parsed.  If we have one, that
     * option becomes argument-less and what was parsed as its argument becomes
     * the source filespec.
     */
    if (last_string == NULL || last_string[0] == '\0')
    {
        if (cmd_opt[opt_version])
            exit(pgm_ok);

        CMD_explain_args();

        message_print(NIDL_SRCFILEREQ);
#ifdef DUMPERS
        if (cmd_opt[opt_dump_cmd])
            dump_cmd_data();
#endif
        return FALSE;
    }

    strcpy(src_filespec, last_string);
    last_string[0] = '\0';
    return TRUE;
}

/*
**  C M D _ p a r s e _ a r g s
**
**  Parses command arguments.
*/

boolean CMD_parse_args          /* Returns TRUE on success */
(
    int         argc,           /* [in] Argument count */
    char        **argv,         /* [in] Argument vector */
    boolean     **p_cmd_opt,    /*[out] Ptr to array of cmd option arguments */
    void        ***p_cmd_val,   /*[out] Ptr to array of cmd option values */
    STRTAB_str_t *p_idl_sid     /*[out] Ptr to IDL filespec stringtable ID */
                                /*      STRTAB_NULL_STR => stdin */
)

{
    FILE_k_t out_dir_kind;              /* File kind of -out string */
    int     i, j;
    STRTAB_str_t src_file_str;          /* Source file stringtable ID */
    char    src_filespec[PATH_MAX];     /* Source file specification */
    char    src_filename[PATH_MAX];     /* Source file name portion */
    char    l_cstub_file[PATH_MAX];     /* Work buf for full cstub filespec */
    char    l_sstub_file[PATH_MAX];     /* Work buf for full sstub filespec */
    char    l_header_file[PATH_MAX];    /* Work buf for full header filespec */
    char    l_caux_file[PATH_MAX];      /* Work buf for full caux filespec */
    char    l_saux_file[PATH_MAX];      /* Work buf for full saux filespec */
    char    filespec[PATH_MAX];         /* Work buf for any filespec */
    
    /*
     * Set up default command line options.
     */
    cmd_opt[opt_do_bug]         = TRUE;
    for (i = 0; i <= NUM_BUGS; i++)     /* 1-based index, thus extra elem */
        support_bug[i] = FALSE;
    /*
     *  By default, -bug 4 is included in the code which causes
     *  arrays of [ref] pointers contained in structures to not 
     *  be represented as a hole in NDR.  
     */
    support_bug[bug_array_no_ref_hole] = TRUE;

    cmd_opt[opt_caux]           = TRUE;
    caux_file                   = "";

    cmd_opt[opt_cc_cmd]         = TRUE;
    cc_cmd                      = CC_DEF_CMD;

    cmd_opt[opt_cc_opt]         = TRUE;
#if defined(__alpha) && defined(__osf__)
    cc_opt                      = "-std1";
#else
    cc_opt                      = "";
#endif

    cmd_opt[opt_cepv]           = FALSE;

    cmd_opt[opt_emit_cstub]     = TRUE;
    client                      = client_vals[client_all];

    cmd_opt[opt_confirm]        = FALSE;

#ifdef VMS
    cpp_cmd                     = UNSPECIFIED;
    cmd_opt[opt_cpp]            = FALSE;
#endif
#ifdef UNIX
    cpp_cmd                     = CPP;
    cmd_opt[opt_cpp]            = TRUE;
#endif

#if defined(VMS) || defined(UNIX)
    CMD_def_cpp_cmd             = CPP;
    cmd_opt[opt_cpp_opt]        = TRUE;
    cpp_opt                     = "";
#endif

    cmd_opt[opt_cstub]          = TRUE;
    cstub_file                  = "";

    cmd_opt[opt_header]         = TRUE;
    header_file                 = "";

    cmd_opt[opt_keep_c]         = FALSE;
    cmd_opt[opt_keep_obj]       = TRUE;
    keep                        = keep_vals[keep_object];

    cmd_opt[opt_idir]           = TRUE;
    cmd_opt[opt_def_idir]       = TRUE;

    cmd_opt[opt_mepv]           = TRUE;
    cmd_opt[opt_warn]           = TRUE;

    cmd_opt[opt_out]            = FALSE;
    out_dir                     = "";

    cmd_opt[opt_saux]           = TRUE;
    saux_file                   = "";

    cmd_opt[opt_emit_sstub]     = TRUE;
    server                      = server_vals[server_all];

    cmd_opt[opt_source]         = TRUE;

    cmd_opt[opt_space_opt]      = FALSE;

    cmd_opt[opt_sstub]          = TRUE;
    sstub_file                  = "";

    cmd_opt[opt_stdin]          = FALSE;

    cmd_opt[opt_syntax_check]   = FALSE;

    cmd_opt[opt_verbose]        = FALSE;

    cmd_opt[opt_version]        = FALSE;

    standard_opt = opt_standard_dce_1_1;
    cmd_val[opt_standard] = (void*)&standard_opt;
    cmd_val[opt_cstub_pref] = "";
    cmd_val[opt_sstub_pref] = "";

    cmd_opt[opt_midl]           = FALSE;

#ifdef DUMPERS
    cmd_opt[opt_dump_acf]       = FALSE;
    cmd_opt[opt_dump_ast]       = FALSE;
    cmd_opt[opt_dump_ast_after] = FALSE;
    cmd_opt[opt_dump_cmd]       = FALSE;
    cmd_opt[opt_dump_debug]     = FALSE;
    cmd_opt[opt_dump_flat]      = FALSE;
    cmd_opt[opt_dump_mnode]     = FALSE;
    cmd_opt[opt_dump_mool]      = FALSE;
    cmd_opt[opt_dump_nametbl]   = FALSE;
    cmd_opt[opt_dump_recs]      = FALSE;
    cmd_opt[opt_dump_sends]     = FALSE;
    cmd_opt[opt_dump_unode]     = FALSE;
    cmd_opt[opt_dump_uool]      = FALSE;
    cmd_opt[opt_dump_yy]        = FALSE;
    cmd_opt[opt_ool]            = FALSE;
#endif

    /*
     * Set up pointers to static storage and return parameters.
     */
    cmd_val[opt_cpp_def]    = (void *)def_strings;
    cmd_val[opt_cpp_undef]  = (void *)undef_strings;
    cmd_val[opt_do_bug]     = (void *)support_bug;
    cmd_val[opt_idir]       = (void *)import_directories;
    *p_cmd_opt = cmd_opt;
    *p_cmd_val = cmd_val;

    /*
     * Check for no arguments.
     */
    if (argc == 0)
    {
        CMD_explain_args();
        exit(pgm_error);
    }

    /*
     * Parse command line options.
     */
#ifdef VMS
    {
    extern DCL_parse_args(    
        boolean     *cmd_opt,       /*[out] Array of cmd option arguments */
        void        **cmd_val       /*[out] Array of cmd option values */
        );

    if (DCL_parse_args(cmd_opt, cmd_val))
    {
        *p_idl_sid = STRTAB_add_string((char *)cmd_val[opt_source]);
        return TRUE;
    }
    }
#endif
    getflags(argc, argv, option_table);

    if (cmd_opt[opt_version])
    {
        message_print(NIDL_VERSION, IDL_VERSION_TEXT);
    }

    /*
     * Check -bug and -no_bug options.
     */
    for (i = flags_option_count(option_table, "bug") - 1; i >= 0; i--)
        if ((do_bug[i] < 1) || (do_bug[i] > NUM_BUGS) || 
            (do_bug[i] == bug_array_no_ref_hole))
        {
            message_print(NIDL_INVBUG, do_bug[i]);
            exit(pgm_error);
        }

    for (i = flags_option_count(option_table, "no_bug") - 1; i >= 0; i--)
        if ((do_not_do_bug[i] < 1) || (do_not_do_bug[i] > NUM_BUGS) ||
            (do_not_do_bug[i] == bug_array_no_ref_hole))
        {
            message_print(NIDL_INVNOBUG, do_not_do_bug[i]);
            exit(pgm_error);
        }

    for (i = flags_option_count(option_table, "bug") - 1; i >= 0; i--)
        for (j = flags_option_count(option_table, "no_bug") - 1; j >= 0; j--)
            if (do_bug[i] == do_not_do_bug[j])
            {
                message_print(NIDL_BUGNOBUG, do_bug[i], do_bug[i]);
                exit(pgm_error);
            }

    for (i = flags_option_count(option_table, "bug") - 1; i >= 0; i--)
    {
        cmd_opt[opt_do_bug] = TRUE;
        support_bug[do_bug[i]] = TRUE;
    }

    for (i = flags_option_count(option_table, "no_bug") - 1; i >= 0; i--)
        support_bug[do_not_do_bug[i]] = FALSE;

    /*
     * Check the -client option.
     */
    i = check_str_list("client", client, client_vals);
    switch (i)
    {
    case client_none:
        cmd_opt[opt_emit_cstub] = FALSE;
        cmd_opt[opt_caux]       = FALSE;
        break;

    case client_stub:
        cmd_opt[opt_emit_cstub] = TRUE;
        cmd_opt[opt_caux]       = FALSE;
        break;

    case client_aux:
        cmd_opt[opt_emit_cstub] = FALSE;
        cmd_opt[opt_caux]       = TRUE;
        break;

    case client_all:
    default:
        cmd_opt[opt_emit_cstub] = TRUE;
        cmd_opt[opt_caux]       = TRUE;
    }

    /*
     * Check the -dump options.
     */
#ifdef DUMPERS
    for (j = 0; dump_strings[j] != NULL; j++)
    {
        i = check_str_list("dump", dump_strings[j], dump_vals);
        switch (i)
        {
        case dump_acf:
            cmd_opt[opt_dump_acf] = TRUE;
            break;

        case dump_ast:
            cmd_opt[opt_dump_ast] = TRUE;
            break;

        case dump_ast_after:
            cmd_opt[opt_dump_ast_after] = TRUE;
            break;

        case dump_cmd:
            cmd_opt[opt_dump_cmd] = TRUE;
            break;

        case dump_debug:
            cmd_opt[opt_dump_debug] = TRUE;
            break;

        case dump_flat:
            cmd_opt[opt_dump_flat] = TRUE;
            break;

        case dump_mnode:
            cmd_opt[opt_dump_mnode] = TRUE;
            break;

        case dump_mool:
            cmd_opt[opt_dump_mool] = TRUE;
            break;

        case dump_nametbl:
            cmd_opt[opt_dump_nametbl] = TRUE;
            break;

        case dump_recs:
            cmd_opt[opt_dump_recs] = TRUE;
            break;

        case dump_sends:
            cmd_opt[opt_dump_sends] = TRUE;
            break;

        case dump_unode:
            cmd_opt[opt_dump_unode] = TRUE;
            break;

        case dump_uool:
            cmd_opt[opt_dump_uool] = TRUE;
            break;

        case dump_yy:
            {
            extern int nidl_yydebug;
            extern int acf_yydebug;
            cmd_opt[opt_dump_yy] = TRUE;
            nidl_yydebug = acf_yydebug = (int)TRUE;
            break;
            }
        }
    }
#endif
    standard_opt = check_str_int_list("standard", standard,
                                      standard_vals, standard_ivals);
    if (standard_opt == -1) standard_opt = opt_standard_dce_1_1;

    /*
     * Process the -I options for import directories.
     */
    if (cmd_opt[opt_def_idir])
    {
        for (i = 0; import_directories[i] != NULL
                    && import_directories[i][0] != '\0' ; i++)
            ;
        import_directories[i+2] = NULL;

#ifdef NIDL_LIBRARY_EV
        nidl_library = getenv(NIDL_LIBRARY_EV);
#endif
        if (nidl_library == NULL)
            nidl_library = DEFAULT_IDIR;
        import_directories[i+1] = nidl_library;

        for ( ; i > 0; i--)
            import_directories[i] = import_directories[i-1];
        import_directories[0] = CD_IDIR;
#ifdef VMS
        flags_incr_count(option_table, "i", 2);
#else
        flags_incr_count(option_table, "I", 2);
#endif
    }

    if (!cmd_opt[opt_def_idir] &&
        (import_directories[0] == NULL || import_directories[0][0] == '\0'))
    {
        import_directories[0] = CD_IDIR;
#ifdef VMS
        flags_incr_count(option_table, "i", 1);
#else
        flags_incr_count(option_table, "I", 1);
#endif
    }

    /*
     * Check the -keep option.
     */
    i = check_str_list("keep", keep, keep_vals);
    switch (i)
    {
    case keep_none:
        cmd_opt[opt_keep_c]     = FALSE;
        cmd_opt[opt_keep_obj]   = FALSE;
        break;

    case keep_c_source:
        cmd_opt[opt_keep_c]     = TRUE;
        cmd_opt[opt_keep_obj]   = FALSE;
        break;

    case keep_object:
    default:
        cmd_opt[opt_keep_c]     = FALSE;
        cmd_opt[opt_keep_obj]   = TRUE;
        break;

    case keep_both:
    case keep_all:
        cmd_opt[opt_keep_c]     = TRUE;
        cmd_opt[opt_keep_obj]   = TRUE;
        break;
    }

    /*
     * Check for -out specification of output directory.
     */
    if (out_dir[0] != '\0')
        cmd_opt[opt_out] = TRUE;

    /*
     * Make sure we have a source filespec before we use it to construct
     * other names.
     */
    if (!get_src_filespec(src_filespec))
        exit(pgm_error);

    if (src_filespec[0] == '\0')    /* -stdin */
    {
        src_file_str = STRTAB_NULL_STR;
        src_filename[0] = 'a';      /* output filenames a.h, a_cstub.c, etc. */
        src_filename[1] = '\0';
    }
    else                            /* file */
    {
        if (!FILE_parse(src_filespec, (char *)NULL, src_filename, (char *)NULL))
        {
            /* Not a valid filespec so probably a bogus option. */
            error(NIDL_UNKFLAG, src_filespec);
            return FALSE;
        }
#ifdef VMS
        else
            /* Default file type to .idl on VMS */
            FILE_form_filespec(src_filespec, (char *)NULL, ".idl", (char *)NULL,
                               src_filespec);
#endif
        src_file_str = STRTAB_add_string(src_filespec);
    }

    /*
     * Check the -server option.
     */
    i = check_str_list("server", server, server_vals);
    switch (i)
    {
    case server_none:
        cmd_opt[opt_emit_sstub] = FALSE;
        cmd_opt[opt_saux]       = FALSE;
        break;

    case server_stub:
        cmd_opt[opt_emit_sstub] = TRUE;
        cmd_opt[opt_saux]       = FALSE;
        break;

    case server_aux:
        cmd_opt[opt_emit_sstub] = FALSE;
        cmd_opt[opt_saux]       = TRUE;
        break;

    case server_all:
    default:
        cmd_opt[opt_emit_sstub] = TRUE;
        cmd_opt[opt_saux]       = TRUE;
    }

    /*
     * Now construct any names that are based on the source filespec.
     */

#ifdef MSDOS
    {
#define extlen(s) (strchr(s, '.') == NULL) ? strlen(s) : (strchr(s, '.') - s)
    int tmp;
    int     namelen;                    /* Length of source filename */

    namelen = strlen(src_filename);
    max_suffix_len = extlen(cstub_suffix);
    if ((tmp = extlen(sstub_suffix)) > max_suffix_len) max_suffix_len = tmp;

    if (namelen + max_suffix_len > 8)
    {
        message_print(NIDL_SRCFILELEN);
        exit(pgm_error);
    }
    }
#endif

    /* If -syntax_only specified, disable all output file processing. */
    if (cmd_opt[opt_syntax_check])
    {
        cmd_opt[opt_out]    = FALSE;
        cmd_opt[opt_cstub]  = FALSE;
        cmd_opt[opt_sstub]  = FALSE;
        cmd_opt[opt_header] = FALSE;
        cmd_opt[opt_caux]   = FALSE;
        cmd_opt[opt_saux]   = FALSE;
    }

    /*
     * If the -out option was selected, verify that the output-directory string
     * is a valid directory spec before prepending to any output filenames.
     */
    if (cmd_opt[opt_out])
    {
#ifndef vms
        if (!FILE_kind(out_dir, &out_dir_kind))
            error(NIDL_FILENOTFND, out_dir);

        if (out_dir_kind != file_dir)
            error(NIDL_FILENOTDIR, out_dir);
#else
        /* Parse out dir so any logical gets translated and for error check */
        char odir[PATH_MAX];
        char oname[PATH_MAX];
        char otype[PATH_MAX];
        if (FILE_parse(out_dir, odir, oname, otype))
            if (oname[0] != '\0' || otype[0] != '\0')
                error(NIDL_FILENOTDIR, out_dir);
            else
            {
		out_dir = NEW_VEC (char, strlen(odir) + 1);
                strcpy(out_dir, odir);
            }
        else
            error(NIDL_FILENOTFND, out_dir);
#endif
    }

    /*
     * Process -cstub option.
     */
    if (cmd_opt[opt_cstub])
    {
        sprintf(filespec, "%s%s", src_filename, cstub_suffix);
        if (!FILE_form_filespec(cstub_file, out_dir, (char *)NULL, filespec,
                                l_cstub_file))
        {
            message_print(NIDL_INVFILESPEC, cstub_file);
            return FALSE;
        }
        cstub_file = l_cstub_file;      /* Point at local buffer */
    }

    /*
     * Process -sstub option.
     */
    if (cmd_opt[opt_sstub])
    {
        sprintf(filespec, "%s%s", src_filename, sstub_suffix);
        if (!FILE_form_filespec(sstub_file, out_dir, (char *)NULL, filespec,
                                l_sstub_file))
        {
            message_print(NIDL_INVFILESPEC, sstub_file);
            return FALSE;
        }
        sstub_file = l_sstub_file;      /* Point at local buffer */
    }

    /*
     * Process -header option.
     */
    sprintf(filespec, "%s%s", src_filename, header_suffix);     /* =tbl */
    if (!FILE_form_filespec(header_file, out_dir, (char *)NULL, /* =tbl */
                            filespec, l_header_file))           /* =tbl */
    {
        message_print(NIDL_INVFILESPEC, header_file);
        return FALSE;
    }
    header_file = l_header_file;    /* Point at local buffer */

    /*
     * Process -caux option.
     */
    if (cmd_opt[opt_caux])
    {
        sprintf(filespec, "%s%s", src_filename, caux_suffix);
        if (!FILE_form_filespec(caux_file, out_dir, (char *)NULL, filespec,
                                l_caux_file))
        {
            message_print(NIDL_INVFILESPEC, caux_file);
            return FALSE;
        }
        caux_file = l_caux_file;        /* Point at local buffer */
    }

    /*
     * Process -saux option.
     */
    if (cmd_opt[opt_saux])
    {
        sprintf(filespec, "%s%s", src_filename, saux_suffix);
        if (!FILE_form_filespec(saux_file, out_dir, (char *)NULL, filespec,
                                l_saux_file))
        {
            message_print(NIDL_INVFILESPEC, saux_file);
            return FALSE;
        }
        saux_file = l_saux_file;        /* Point at local buffer */
    }

    /*
     * All options are now loaded with default values if necessary.
     * Setup the cmd_val table to point to the parsed values.
     * Memory management assumptions: It is assumed that pointers to
     * unmodified command options reference memory that is valid for the
     * entire program.  Any modified command options, however, reference
     * local storage.  We must allocate permanent memory for those, via
     * the alloc_and_copy function.
     */
    cmd_val[opt_caux]       = (void *)alloc_and_copy(caux_file);
    cmd_val[opt_cc_cmd]     = (void *)cc_cmd;
    cmd_val[opt_cc_opt]     = (void *)cc_opt;
#ifdef UNIX
    cmd_val[opt_cpp]        = (void *)cpp_cmd;
    cmd_val[opt_cpp_opt]    = (void *)cpp_opt;
#endif
#ifdef VMS
    /* On VMS the default is -no_cpp.  See if -cpp was specified. */
    if (cpp_cmd != UNSPECIFIED)
    {
        cmd_opt[opt_cpp] = TRUE;
        if (cpp_cmd[0] == '\0')
            cpp_cmd = (void *)CPP;  /* Set to default */
        cmd_val[opt_cpp] = (void *)cpp_cmd;
    }
    cmd_val[opt_cpp_opt]    = (void *)cpp_opt;
#endif
    cmd_val[opt_cstub]      = (void *)alloc_and_copy(cstub_file);
    cmd_val[opt_header]     = (void *)alloc_and_copy(header_file);
    cmd_val[opt_out]        = (void *)out_dir;
    cmd_val[opt_saux]       = (void *)alloc_and_copy(saux_file);
    cmd_val[opt_source]     = (void *)src_filespec;
    cmd_val[opt_sstub]      = (void *)alloc_and_copy(sstub_file);

#ifdef DUMPERS
    if (cmd_opt[opt_dump_cmd])
        dump_cmd_data();
#endif

    /*
     * Print list of options if requested.
     */
    if (cmd_opt[opt_confirm])
    {
        printflags(option_table);
        if (!cmd_opt[opt_verbose])
            exit(pgm_ok);
    }

    ERR_no_warnings = !cmd_opt[opt_warn];

    *p_idl_sid = src_file_str;
    return(TRUE);
}
