/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        mount3.x
 *
 * Abstract:
 *
 *        NFS
 *
 *        MOUNT version 3 types and procedures in XDR format.
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

const MOUNT_PROGRAM     = 100005;
const MNTPATHLEN        = 1024;  /* Maximum bytes in a path name */
const MNTNAMLEN         = 255;   /* Maximum bytes in a name */
const FHSIZE3           = 64;    /* Maximum bytes in a V3 file handle */

const MOUNTPROC3_NULL   = 0;
const MOUNTPROC3_MNT    = 1;
const MOUNTPROC3_DUMP   = 2;
const MOUNTPROC3_UMNT   = 3;
const MOUNTPROC3_UMNTALL= 4;
const MOUNTPROC3_EXPORT = 5;

typedef opaque fhandle3<FHSIZE3>;
typedef string dirpath<MNTPATHLEN>;
typedef string name<MNTNAMLEN>;

enum mountstat3 {
    MNT3_OK = 0,                 /* no error */
    MNT3ERR_PERM = 1,            /* Not owner */
    MNT3ERR_NOENT = 2,           /* No such file or directory */
    MNT3ERR_IO = 5,              /* I/O error */
    MNT3ERR_ACCES = 13,          /* Permission denied */
    MNT3ERR_NOTDIR = 20,         /* Not a directory */
    MNT3ERR_INVAL = 22,          /* Invalid argument */
    MNT3ERR_NAMETOOLONG = 63,    /* Filename too long */
    MNT3ERR_NOTSUPP = 10004,     /* Operation not supported */
    MNT3ERR_SERVERFAULT = 10006  /* A failure on the server */
};

struct mountres3_ok {
    fhandle3   fhandle;
    int        auth_flavors<>;
};

union mountres3 switch (mountstat3 fhs_status) {
    case MNT3_OK:
        mountres3_ok  mountinfo;
    default:
        void;
};

typedef struct mountbody *mountlist;

struct mountbody {
    name       ml_hostname;
    dirpath    ml_directory;
    mountlist  ml_next;
};

typedef struct groupnode *groups;

struct groupnode {
    name     gr_name;
    groups   gr_next;
};

typedef struct exportnode *exports;

struct exportnode {
    dirpath  ex_dir;
    groups   ex_groups;
    exports  ex_next;
};

