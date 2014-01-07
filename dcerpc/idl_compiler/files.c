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
**      files.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  IDL file manipulation routines.
**
**  VERSION: DCE 1.0
**
*/

#ifdef vms
#  include <types.h>
#  include <stat.h>
#  include <descrip.h>
#else
#  include <sys/types.h>
#  include <sys/stat.h>
#endif

#include <nidl.h>
#include <files.h>
#include <unistd.h>
#include "message.h"

/*
**  Default filespec; only good for one call to FILE_parse.
*/
char const *FILE_def_filespec = NULL;

/*
**  F I L E _ o p e n
**
**  Opens an existing file for read access.
*/

boolean FILE_open               /* Returns TRUE on success */
(
    char        *filespec,      /* [in] Filespec */
    FILE        **fid           /*[out] File handle; ==NULL on FALSE status */
)

{
    if ((*fid = fopen(filespec, "r")) == NULL)
    {
        idl_error_list_t errvec[2];
        errvec[0].msg_id = NIDL_OPENREAD;
        errvec[0].arg[0] = filespec;
        errvec[1].msg_id = NIDL_SYSERRMSG;
        errvec[1].arg[0] = strerror(errno);
        error_list(2, errvec, TRUE);
    }

    return TRUE;
}

/*
**  F I L E _ c r e a t e
**
**  Creates and opens a new file for write access.
*/

boolean FILE_create             /* Returns TRUE on success */
(
    char        *filespec,      /* [in] Filespec */
    FILE        **fid           /*[out] File handle; ==NULL on FALSE status */
)

{
#ifndef VMS
#define MODE_WRITE "w"
#else
#define MODE_WRITE "w","mbc = 16","rop = WBH","mbf = 3"
#endif

    if ((*fid = fopen(filespec, MODE_WRITE)) == NULL)
    {
        idl_error_list_t errvec[2];
        errvec[0].msg_id = NIDL_OPENWRITE;
        errvec[0].arg[0] = filespec;
        errvec[1].msg_id = NIDL_SYSERRMSG;
        errvec[1].arg[0] = strerror(errno);
        error_list(2, errvec, TRUE);
    }

    return TRUE;
}

/*
**  F I L E _ l o o k u p
**
**  Looks for the specified file first in the working directory,
**  and then in the list of specified directories.
**
**  Returns:    TRUE if file was found, FALSE otherwise
*/

boolean FILE_lookup             /* Returns TRUE on success */
(
    char const  *filespec,      /* [in] Filespec */
    char const  * const *idir_list,    /* [in] Array of directories to search */
                                /*      NULL => just use filespec */
    struct stat *stat_buf,      /*[out] Stat buffer - see stat.h */
    char        *lookup_spec    /*[out] Filespec of found file (on success) */
)
{
#ifdef HASDIRTREE
    int     i;

    /*
     * First try the filespec by itself.
     */
    if (stat(filespec, stat_buf) != -1)
    {
#ifndef VMS
        strcpy(lookup_spec, filespec);
#else
        /*
         * This code is for the special case where foo.idl is the source file
         * but foo is also a logical name; form a full filespec so that
         * subsequent logic won't remove .idl and translate logical foo.
         */
        char *cp, *cwd = getcwd((char *)NULL, PATH_MAX);
        if (cwd != NULL)
            for (cp = cwd; *cp != '\0'; cp++) *cp = tolower(*cp);
        if (!FILE_form_filespec(filespec, cwd, (char *)NULL, (char *)NULL,
                                lookup_spec))
            strcpy(lookup_spec, filespec);
        if (cwd != NULL) free(cwd);
#endif
        return TRUE;
    }

    /*
     * Fail if idir_list is null.
     */
    if (idir_list == NULL)
        return FALSE;

    /*
     * Lookup other pathnames using the directories in the idir_list.
     */
    for (i = 0; idir_list[i]; i++)
    {
        if (FILE_form_filespec(filespec, idir_list[i], (char *)NULL,
                               (char *)NULL, lookup_spec)
            &&  stat(lookup_spec, stat_buf) != -1)
            return TRUE;
    }

#if defined(UNIX) || defined(_MSDOS)
    /*
     * On Unix-like filesystems, make another pass over the idir_list if the
     * search filespec has a directory name, prepending each idir to the search
     * filespec.  For example, importing "y/z.idl" will match "/x/y/z.idl" if
     * -I/x is on the command line.
     */
    if (*filespec != BRANCHCHAR && FILE_has_dir_info(filespec))
        for (i = 0; idir_list[i]; i++)
        {
            sprintf(lookup_spec, "%s%c%s", idir_list[i], BRANCHCHAR, filespec);
            if (stat(lookup_spec, stat_buf) != -1)
                return TRUE;
        }
#endif

    return FALSE;

#else
    error(NIDL_FNUNIXONLY, __FILE__, __LINE__);
#endif
}

/*
**  F I L E _ f o r m _ f i l e s p e c
**
**  Forms a file specification from the specified components.
*/
#ifdef VMS
# include <ctype.h>
#endif

boolean FILE_form_filespec      /* Returns TRUE on success */
(                               /* For all [in] args, NULL => none */
    char const  *in_filespec,   /* [in] Filespec (full or partial) */
    char const  *dirspec,       /* [in] Directory; used if in_filespec */
                                /*      doesn't have directory field */
    char const  *type,          /* [in] Filetype; used if in_filespec */
                                /*      doesn't have filetype field */
    char const  *rel_filespec,  /* [in] Related filespec; fields are used to */
                                /*      fill in missing components after */
                                /*      applying in_filespec, dir, type */
    char        *out_filespec   /*[out] Full filespec formed */
)
{
    char const *dir = NULL;        /* Directory specified */
    char       in_dir[PATH_MAX];   /* Directory part of in_filespec */
    char       in_name[PATH_MAX];  /* Filename part of in_filespec */
    char       in_type[PATH_MAX];  /* Filetype part of in_filespec */
    char       rel_dir[PATH_MAX];  /* Directory part of rel_filespec */
    char       rel_name[PATH_MAX]; /* Filename part of rel_filespec */
    char       rel_type[PATH_MAX]; /* Filetype part of rel_filespec */
    char const *res_dir;           /* Resultant directory */
    char const *res_name;          /* Resultant filename */
    char const *res_type;          /* Resultant filetype */

    in_dir[0]   = '\0';
    in_name[0]  = '\0';
    in_type[0]  = '\0';
    rel_dir[0]  = '\0';
    rel_name[0] = '\0';
    rel_type[0] = '\0';
    res_dir     = "";
    res_name    = "";
    res_type    = "";

    /* Parse in_filespec into its components. */
    if (in_filespec != NULL && in_filespec[0] != '\0')
    {
        /*
         * Setup the related or file type global FILE_def_filespec such that
         * any file lookup is handled appropriately in FILE_parse.
         */
        if (rel_filespec)
            FILE_def_filespec = rel_filespec;
        else if (type)
            FILE_def_filespec = type;

        if (!FILE_parse(in_filespec, in_dir, in_name, in_type))
#ifndef VMS
            return FALSE;
#else
        {
            char tmp_filespec[PATH_MAX];
            /*
             * On VMS, the parse could fail because the input filespec is a
             * partial filespec in U*ix format.  Attempt to translate the dir-
             * ectory spec into and compose the output filespec in U*ix format.
             */
            if (dirspec == NULL || strcmp(dirspec, CD_IDIR) == 0)
                /* Don't do relative path; Top-level name could be logical. */
                dir = "";
            else if (dirspec != NULL)
            {
                extern char *getenv(), *shell$translate_vms();
                char *logical_val;

                /*
                ** Determine if the dirspec is a logical (need to try twice
                ** because getenv is case sensitive).
                */
                logical_val = getenv(dirspec);
                if (logical_val == NULL)
                {
                    char upcase_log[PATH_MAX];
                    strncpy(upcase_log, dirspec, PATH_MAX);
                    for (logical_val = upcase_log; *logical_val; logical_val++)
                        if (isalpha(*logical_val))
                            *logical_val = toupper(*logical_val);
                    logical_val = getenv(upcase_log);
                }

                /*
                ** If the dirspec is a logical, translate the equivalence
                ** name, otherwise translate the dirspec itself.
                */
                if (logical_val != NULL)
                {
                    dir = shell$translate_vms(logical_val);
                }
                else
                    dir = shell$translate_vms(dirspec);

                if (dir == (char *)0 || dir == (char *)-1)
                    return FALSE;
            }

            /*
            ** Concatenate U*ix dir spec with input filespec.
            */
            strcpy(tmp_filespec, dir);
            strcat(tmp_filespec, "/");
            strcat(tmp_filespec, in_filespec);
            if (!FILE_parse(tmp_filespec, in_dir, in_name, in_type))
                return FALSE;
        }
#endif  /* VMS */
    }

    if (dir == NULL)
	dir = dirspec;

    /* Parse rel_filespec into its components. */
    if (rel_filespec != NULL && rel_filespec[0] != '\0')
#ifndef VMS
        if (!FILE_parse(rel_filespec, rel_dir, rel_name, rel_type))
            return FALSE;
#else
        if (    (in_filespec != NULL && in_filespec[0] != '\0')
            ||  (dir != NULL && dir[0] != '\0') )
        {
            /*
             * Setup the related or file type global FILE_def_filespec such
             * that any file lookup is handled appropriately in FILE_parse.
             */
            if (type)
                FILE_def_filespec = type;
            if (!FILE_parse(rel_filespec, rel_dir, rel_name, rel_type))
                return FALSE;
        }
        else
        {
            /*
             * Special case VMS logic to finesse the case where the related
             * filespec can be a partial filespec in U*ix format - break it into
             * a filetype and a "filename" containing all but the filetype.
             */
            int  len;
            char *cp = strrchr(rel_filespec, '.');
            if (cp == NULL)
                len = strlen(rel_filespec);
            else
                len = cp - rel_filespec;
            strncpy(rel_name, rel_filespec, len);
            rel_name[len] = '\0';
            strcpy(rel_type, &rel_filespec[len]);
        }
#endif  /* VMS */

    /* Apply first valid of in_dir, dir, or rel_dir. */
    if (in_dir[0] != '\0')
        res_dir = in_dir;
    else if (dir != NULL && dir[0] != '\0')
        res_dir = dir;
    else if (rel_dir[0] != '\0')
        res_dir = rel_dir;

    /* Apply first valid of in_name, rel_name. */
    if (in_name[0] != '\0')
        res_name = in_name;
    else if (rel_name[0] != '\0')
        res_name = rel_name;

    /* Apply first valid of in_type, type, rel_type.  Note that rel_type is
     * only applied if in_filespec is null.
     */
    if (in_type[0] != '\0')
        res_type = in_type;
    else if (type != NULL && type[0] != '\0')
        res_type = type;
    else if (rel_type[0] != '\0')
        res_type = rel_type;

#ifdef HASDIRTREE

    /* Concatenate the result. */

    out_filespec[0] = '\0';

    if (res_dir[0] != '\0')
    {
        strcat(out_filespec, res_dir);
#ifndef VMS
        strcat(out_filespec, BRANCHSTRING);
#else
        {
        /* If the directory spec is a logical name, append a colon. */
        char upcase_dir[PATH_MAX];
        char *cp;

        strcpy(upcase_dir, res_dir);
        for (cp = upcase_dir; *cp; cp++)
            if (isalpha(*cp))
                *cp = toupper(*cp);
        if (getenv(upcase_dir) != NULL) strcat(out_filespec, ":");
        }
#endif
    }

    if (res_name[0] != '\0')
        strcat(out_filespec, res_name);

    if (res_type[0] != '\0')
        strcat(out_filespec, res_type); /* The '.' is part of the filetype */

    return TRUE;

#else
    error(NIDL_FNUNIXONLY, __FILE__, __LINE__);
#endif
}

#ifdef VMS
/*
 * VMS-specific code used in parsing file specifications.
 */
#include <fab.h>
#include <nam.h>
#include <rmsdef.h>

static char vms_filespec[NAM$C_MAXRSS];

/*
**  p r o c e s s _ v m s _ f i l e s p e c
**
**  Action routine called from shell$to_vms to process a VMS file specification.
*/

static int process_vms_filespec
(
    char        *filespec,      /* [in] VMS filespec */
    int         flags           /* [in] Translation flags */
)

{
    strcpy(vms_filespec, filespec);

    /* Return zero to prevent further translation of wildcards. */
    return 0;
}
#endif

/*
**  F I L E _ p a r s e
**
**  Parses a specified pathanme into individual components.
*/

#ifdef VMS
/*
**  Macro to copy a string and translate it to lower case.
**  Requires local variable declaration of int i.
*/
#define STRNLCPY(dst, src, len) \
    for (i = 0; i < (len); i++) (dst)[i] = tolower((src)[i]);
#endif

boolean FILE_parse              /* Returns TRUE on success */
(
    char const  *filespec,      /* [in] Filespec */
    char        *dir,           /*[i,o] Directory portion; NULL =>don't want */
    char        *name,          /*[i,o] Filename portion;  NULL =>don't want */
    char        *type           /*[i,o] File type (ext);   NULL =>don't want */
)
#ifndef VMS     /* This code works partially on VMS; better version below */
{
#if defined(HASDIRTREE)
    FILE_k_t    filekind;       /* File kind */
    char const  *pn;
    int         pn_len,
                leaf_len;
    int         i,
                j;
    int         leaf_start,
                ext_start;
    int         dir_end,
                leaf_end;
    boolean     slash_seen,
                dot_seen;

    /* Init return values. */
    if (dir)
        dir[0] = '\0';
    if (name)
        name[0] = '\0';
    if (type)
        type[0] = '\0';

    /*
     * If the filespec has BRANCHCHAR do special case check to see if pathname
     * is a directory to prevent directory /foo/bar from being interpreted as
     * directory /foo file bar.
     */
    if (strchr(filespec, BRANCHCHAR)
        &&  FILE_kind(filespec, &filekind)
        &&  filekind == file_dir)
    {
        strcpy(dir, filespec);
        return TRUE;
    }

    /*
     *  Scan backwards looking for a BRANCHCHAR -
     *  If not found, then no directory was specified.
     */
    pn = filespec;
    pn_len = strlen(pn);
    slash_seen = FALSE;
    dir_end = -1;
    leaf_start = 0;
    dot_seen = FALSE;

    /*
     * For temporary VMS support, until full file capabilities are in place,
     * look for the defined BRANCHCHAR or a colon, which is indicative of a
     * device name or logical name.  Device and directory information is
     * collectively returned as the dir argument.
     */
    for (i = pn_len - 1; i >= 0; i--)
        if (pn[i] == BRANCHCHAR
#ifdef VMS
            || pn[i] == ':'
#endif
#if BRANCHAR == '\\'
            || pn[i] == '/'
#endif
           )
        {
            /*
             * On VMS, the BRANCHCHAR is considered part of the directory.
             */
            leaf_start = i + 1;
#ifdef VMS
            dir_end = i + 1;
#else
            dir_end = i > 0 ? i : 1;
#endif
            slash_seen = TRUE;
            break;
        }

    if (dir)
    {
        if (slash_seen)
        {
            strncpy(dir, pn, dir_end);
            dir[dir_end] = '\0';
        }
        else
            dir[0] = '\0';
    }

    /*
     *  Start scanning from the BRANCHCHAR for a '.' to find the leafname.
     */
    ext_start = pn_len;
    leaf_end = pn_len;

    for (j = pn_len; j > leaf_start; --j)
        if (pn[j] == '.')
        {
            leaf_end = j - 1;
            ext_start = j;      /* Extension includes the '.' */
            dot_seen = TRUE;
            break;
        }

    if (leaf_end >= dir_end + 1)
    {
        leaf_len = dot_seen ? leaf_end - leaf_start + 1 : leaf_end - leaf_start;
        if (name)
        {
            strncpy(name, &pn[leaf_start], leaf_len);
            name[leaf_len] = '\0';
        }

        if (!dot_seen)
        {
            if (type)
                type[0] = '\0';
            return TRUE;
        }
        else
        {
        if (type)
            strcpy(type, &pn[ext_start]);
        }
    }

    return TRUE;

#else
    error(NIDL_FNUNIXONLY, __FILE__, __LINE__);
    return FALSE;
#endif
}

#else
/*
**  VMS-specific version of FILE_parse.
*/
{
    unsigned int rms_status;    /* RMS status */
    struct FAB  fab;            /* RMS File Access Block */
    struct NAM  nam;            /* RMS Name Block */
    char defspec[NAM$C_MAXRSS]; /* Default file specification */
    char outspec[NAM$C_MAXRSS]; /* Expanded file specification */
    int         i;

    fab = cc$rms_fab;       /* Set to prototype FAB for proper initialization */
    if (FILE_def_filespec == NULL)
        defspec[0] = '\0';
    else
        strcpy(defspec, FILE_def_filespec);
    FILE_def_filespec = NULL;
    fab.fab$l_dna   = defspec;
    fab.fab$b_dns   = strlen(defspec);
    fab.fab$l_fna   = filespec;
    fab.fab$b_fns   = strlen(filespec);
    fab.fab$l_fop   = 0;
    fab.fab$w_ifi   = 0;
    fab.fab$l_nam   = &nam;

    nam = cc$rms_nam;       /* Set to prototype NAM for proper initialization */
    nam.nam$l_esa   = outspec;
    nam.nam$b_ess   = NAM$C_MAXRSS;
    nam.nam$b_nop   = NAM$M_SYNCHK;
    nam.nam$l_rlf   = 0;

    rms_status = SYS$PARSE(&fab, 0, 0);
    if (rms_status != RMS$_NORMAL)
    {
        /*
         * Don't give up yet - file specification could be in U*ix format.
         */
        if (shell$to_vms(filespec, process_vms_filespec, 1) != 1)
        {
            /*
             * Still don't give up - if filespec starts with logical name,
             * it will need a leading slash.
             */
            char inspec[NAM$C_MAXRSS];
            inspec[0] = '/';
            strcpy(&inspec[1], filespec);
            if (shell$to_vms(inspec, process_vms_filespec, 1) != 1)
                return FALSE;
        }

        fab.fab$l_fna = vms_filespec;
        fab.fab$b_fns = strlen(vms_filespec);

        rms_status = SYS$PARSE(&fab, 0, 0);
        if (rms_status != RMS$_NORMAL)
            return FALSE;
    }


    /*
     * Copy results to output parameters.  Only copy explicitly specified
     * components of the file specification.
     */
    if (dir != NULL)
    {
        if (nam.nam$l_fnb & (NAM$M_NODE | NAM$M_EXP_DEV | NAM$M_EXP_DIR))
        {
            /*
             *  If the directory was explictly in the filespec, do a search to
             *  get the directory that actually contains the file.  If it fails
             *  redo the parse, because we know that the parse succeeded above
             *  and we may be trying to create a file instead of opening one.
             */
            nam.nam$b_nop   = 0;
            rms_status = SYS$PARSE(&fab, 0, 0);
            rms_status = SYS$SEARCH(&fab, 0, 0);
            if (rms_status != RMS$_NORMAL)
            {
                /* Search failed, so just do a syntax_only parse */
                nam.nam$b_nop   = NAM$M_SYNCHK;
                SYS$PARSE(&fab, 0, 0);
            }

            /* Append the results of the parse into the dir output argument */
            STRNLCPY(dir, nam.nam$l_node, nam.nam$b_node);
            STRNLCPY(&dir[nam.nam$b_node], nam.nam$l_dev, nam.nam$b_dev);
            STRNLCPY(&dir[nam.nam$b_node + nam.nam$b_dev],
                    nam.nam$l_dir, nam.nam$b_dir);
            dir[nam.nam$b_node + nam.nam$b_dev + nam.nam$b_dir] = '\0';
        }
        else
            dir[0] = '\0';
    }

    if (name != NULL)
    {
        if (nam.nam$l_fnb & NAM$M_EXP_NAME)
        {
            STRNLCPY(name, nam.nam$l_name, nam.nam$b_name);
            name[nam.nam$b_name] = '\0';
        }
        else
            name[0] = '\0';
    }

    if (type != NULL)
    {
        if (nam.nam$l_fnb & (NAM$M_EXP_TYPE | NAM$M_EXP_VER))
        {
            STRNLCPY(type, nam.nam$l_type, nam.nam$b_type);
            if (nam.nam$l_fnb & NAM$M_EXP_VER)
            {
                STRNLCPY(&type[nam.nam$b_type], nam.nam$l_ver, nam.nam$b_ver);
                type[nam.nam$b_type + nam.nam$b_ver] = '\0';
            }
            else
                type[nam.nam$b_type] = '\0';
        }
        else
            type[0] = '\0';
    }

    return TRUE;
}
#endif

/*
**  F I L E _ h a s _ d i r _ i n f o
**
**  Returns:    TRUE if filespec includes directory information.
*/

boolean FILE_has_dir_info
(
    char const  *filespec       /* [in] Filespec */
)
{
    char    dir[PATH_MAX];      /* Directory part of filespec */

    if (!FILE_parse(filespec, dir, (char *)NULL, (char *)NULL))
        return FALSE;

    return (dir[0] != '\0');
}

/*
**  F I L E _ i s _ c w d
**
**  Returns:    TRUE if filespec is equivalent to the current working directory.
*/

boolean FILE_is_cwd
(
    char        *filespec       /* [in] Filespec */
)

{
    char    *cwd;       /* Current working directory */
    char    *twd;       /* Temp working directory = filespec argument */
    boolean result;     /* Function result */

    /* Null filespec => current working directory. */
    if (filespec[0] == '\0')
        return TRUE;

    /* Get current working directory. */
    cwd = getcwd((char *)NULL, PATH_MAX);
    if (cwd == NULL)
        return FALSE;

    /* chdir to the passed directory filespec. */
    if (chdir(filespec) != 0)
    {
        /* Can chdir; probably a bogus directory. */
        free(cwd);
        return FALSE;
    }

    /*
     * Again get current working directory - this gets us the passed
     * directory filespec in a "normallized form".
     */
    twd = getcwd((char *)NULL, PATH_MAX);
    if (twd == NULL)
    {
        free(cwd);
        return FALSE;
    }

    if (strcmp(cwd, twd) == 0)
        result = TRUE;
    else
    {
        /* Not current working directory; be sure to chdir back to original! */
        result = FALSE;
        if (chdir(cwd) != 0)
        {
            abort();
        }
    }

    /* Free storage malloc'ed by getcwd(). */
    free(cwd);
    free(twd);

    return result;
}

/*
**  F I L E _ k i n d
**
**  Returns whether a pathname is a directory, a file, or something else.
*/

boolean FILE_kind               /* Returns TRUE on success */
(
    char const  *filespec,      /* [in] Filespec */
    FILE_k_t    *filekind       /*[out] File kind (on success) */
)
{
    struct stat fileinfo;

    if (stat(filespec, &fileinfo) == -1)
        return FALSE;

    switch (fileinfo.st_mode & S_IFMT)
    {
    case S_IFDIR:
        *filekind = file_dir;
        break;

    case S_IFREG:
        *filekind = file_file;
        break;

    default:
        *filekind = file_special;
    }

    return TRUE;
}

/*
**  F I L E _ c o n t a i n s _ e v _ r e f
**
**  Scans a pathname to see if it contains an environment variable reference.
*/

boolean FILE_contains_ev_ref    /* Returns TRUE if filespec contains an */
                                /* environment variable reference */
(
    STRTAB_str_t    fs_id       /* [in] Filespec stringtable ID */
)

{
    char const  *pn;
    unsigned int         i;

    STRTAB_str_to_string(fs_id, &pn);

    for (i = 0; i < strlen(pn) - 1; i++)
        if (pn[i] == '$' && pn[i + 1] == '(')
            return TRUE;

    return FALSE;
}

/*
**  F I L E _ e x e c u t e _ c m d
**
**  This routine executes the specified command string with
**  the specified parameters.  All error output goes to the
**  default output/error device.
*/

int FILE_execute_cmd
(
    char    *cmd_string,        /* command to execute */
    char    *p1,                /* parameter1 */
    char    *p2,                /* parameter2 */
    long    msg_id              /* Optional msg_id to output */
)

{
    char    *cmd;       /* Command derived from inputs */
    int     status;

    /* Alloc space and create command string */
    cmd = NEW_VEC (char, strlen(cmd_string) + strlen(p1) + strlen(p2) + 3);
    cmd[0] = '\0';
    strcat(cmd,cmd_string);
    strcat(cmd," ");
    strcat(cmd,p1);
    strcat(cmd," ");
    strcat(cmd,p2);

    /* Output a message, if msg_id specified is non-zero */
    if (msg_id != 0)
        message_print(msg_id, (char*)cmd);

    /* Execute the command, errors to default output device */
#ifdef VMS
    {
        $DESCRIPTOR(vcmd, cmd);
        vcmd.dsc$w_length = strlen(cmd);
        status = LIB$SPAWN(&vcmd);
	/* If it failed, output a message as sometimes the subprocess won't even though it failed */
	if ((status & 1) != 1) {
	    /*
	    ** If we haven't yet output a message associated with the spawned command, then
	    ** include it in the message, because the SYSERMSG requires the context of
	    ** a previously reported message.
	    */
	    if (msg_id == 0) {
		idl_error_list_t errvec[2];
		errvec[0].msg_id = NIDL_STUBCOMPILE;
		errvec[0].arg1   = cmd;
		errvec[1].msg_id = NIDL_SYSERRMSG;
		errvec[1].arg1   = strerror(EVMSERR,status);
		error_list(2, errvec, TRUE);
		}
	    else
		message_print(NIDL_SYSERRMSG,strerror(EVMSERR,status));
	}
    }
#else
    status = system(cmd);
#endif

    /* Free the command string */
    FREE(cmd);

    return status;
}

/*
**  F I L E _ d e l e t e
**
**  This routine deletes the file specified by the filename
**  string specified.
*/

void FILE_delete
(
    char    *filename
)

{
#ifdef VMS
    remove (filename);
#else
    unlink (filename);
#endif
}
/* preserve coding style vim: set tw=78 sw=4 : */
