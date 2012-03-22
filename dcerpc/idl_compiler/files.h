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
**      files.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Header file for file manipulation routines.
**
**  VERSION: DCE 1.0
**
*/

#ifndef files_incl
#define files_incl

#ifndef S_IFREG
#ifdef vms
#  include <types.h>
#  include <stat.h>
#else
#  include <sys/types.h>
#  include <sys/stat.h>
#endif
#endif

#include <nidl.h>               /* IDL common defs */
#include <nametbl.h>

typedef enum                    /* Filespec kinds: */
{
    file_dir,                   /* Directory */
    file_file,                  /* Regular ol' file */
    file_special                /* Something else */
} FILE_k_t;

extern boolean FILE_open(
    char *filespec,
    FILE **fid
);

extern boolean FILE_create(
    char *filespec,
    FILE **fid
);

extern boolean FILE_lookup(
    char const  *filespec,
    char const  * const *idir_list,
    struct stat *stat_buf,
    char        *lookup_spec
);

extern boolean FILE_form_filespec(
    char const *in_filespec,
    char const *dir,
    char const *type,
    char const *rel_filespec,
    char       *out_filespec
);

#ifdef VMS
/*
**  Default filespec; only good for one call to FILE_parse.
*/
extern char *FILE_def_filespec;
#endif

extern boolean FILE_parse(
    char const *filespec,
    char       *dir,
    char       *name,
    char       *type
);

extern boolean FILE_has_dir_info(
    char const *filespec
);

extern boolean FILE_is_cwd(
    char *filespec
);

extern boolean FILE_kind(
    char const  *filespec,
    FILE_k_t    *filekind
);

extern int FILE_execute_cmd(
    char        *cmd_string,
    char        *p1,
    char        *p2,
    long        msg_id
);

extern void FILE_delete(
    char        *filename
);

#endif /* files_incl */
