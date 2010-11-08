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
 *        nfs3_types.x
 *
 * Abstract:
 *
 *        NFS3
 *
 *        NFS3 XDR types.
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

const NFS_PROGRAM               = 100003;
const NFS3_FHSIZE               = 64;
const NFS3_COOKIEVERFSIZE       = 8;
const NFS3_CREATEVERFSIZE       = 8;
const NFS3_WRITEVERFSIZE        = 8;

const NFS3_MODE_SET_UID         = 0x0800;
const NFS3_MODE_SET_GID         = 0x0400;
const NFS3_MODE_OWNER_READ      = 0x0100;
const NFS3_MODE_OWNER_WRITE     = 0x0080;
const NFS3_MODE_OWNER_EXECUTE   = 0x0040;
const NFS3_MODE_GROUP_READ      = 0x0020;
const NFS3_MODE_GROUP_WRITE     = 0x0010;
const NFS3_MODE_GROUP_EXECUTE   = 0x0008;
const NFS3_MODE_OTHERS_READ     = 0x0004;
const NFS3_MODE_OTHERS_WRITE    = 0x0002;
const NFS3_MODE_OTHERS_EXECUTE  = 0x0001;

typedef unsigned hyper uint64;
typedef hyper int64;
typedef unsigned long uint32;
typedef long int32;
typedef string filename3<>;
typedef string nfspath3<>;
typedef uint64 fileid3;
typedef uint64 cookie3;
typedef opaque cookieverf3[NFS3_COOKIEVERFSIZE];
typedef opaque createverf3[NFS3_CREATEVERFSIZE];
typedef opaque writeverf3[NFS3_WRITEVERFSIZE];
typedef uint32 uid3;
typedef uint32 gid3;
typedef uint64 size3;
typedef uint64 offset3;
typedef uint32 mode3;
typedef uint32 count3;

enum nfsstat3 {
    NFS3_OK             = 0,    /* Success */
    NFS3ERR_PERM        = 1,    /* Not owner */
    NFS3ERR_NOENT       = 2,    /* No such file or directory */
    NFS3ERR_IO          = 5,    /* IO error */
    NFS3ERR_NXIO        = 6,    /* IO error. No such device or address. */
    NFS3ERR_ACCES       = 13,   /* Permission denied */
    NFS3ERR_EXIST       = 17,   /* File exists */
    NFS3ERR_XDEV        = 18,   /* Attempt to do a cross-device hard link */
    NFS3ERR_NODEV       = 19,   /* No such device */
    NFS3ERR_NOTDIR      = 20,   /* Not a directory */
    NFS3ERR_ISDIR       = 21,   /* Is a directory */
    NFS3ERR_INVAL       = 22,   /* Invalid argument for an operation */
    NFS3ERR_FBIG        = 27,   /* File too large */
    NFS3ERR_NOSPC       = 28,   /* No space left on device */
    NFS3ERR_ROFS        = 30,   /* Read-only file system */
    NFS3ERR_MLINK       = 31,   /* Too many hard links */
    NFS3ERR_NAMETOOLONG = 63,   /* The filename in an operation was too long */
    NFS3ERR_NOTEMPTY    = 66,   /* At attempt was made to remove a directory
                                   that was not empty */
    NFS3ERR_DQUOT       = 69,   /* Quta hard limit exceeded */
    NFS3ERR_STALE       = 70,   /* Invalid file handle */
    NFS3ERR_REMOTE      = 71,   /* Too many levels of remote in path */
    NFS3ERR_BADHANDLE   = 10001,/* Illegal NFS file handle */
    NFS3ERR_NOT_SYNC    = 10002,/* Update synchronization mismatch was detected
                                   during a SETATTR operation */
    NFS3ERR_BAD_COOKIE  = 10003,/* READDIR or READDIRPLUS cookie is stale */
    NFS3ERR_NOTSUPP     = 10004,/* Operation is not supported */
    NFS3ERR_TOOSMALL    = 10005,/* Buffer or request is too small */
    NFS3ERR_SERVERFAULT = 10006,/* An error occured on the server which does
                                   not map to any of the legal NFSv3 errors */
    NFS3ERR_BADTYPE     = 10007,/* An attempt was made to create an object of
                                   a type not supported by the server */
    NFS3ERR_JUKEBOX     = 10008 /* The server initiated the request, but was
                                   not able to complete it in a timely fashion */
};

enum ftype3 {
    NF3REG    = 1,
    NF3DIR    = 2,
    NF3BLK    = 3,
    NF3CHR    = 4,
    NF3LNK    = 5,
    NF3SOCK   = 6,
    NF3FIFO   = 7
};

struct specdata3 {
    uint32     specdata1;
    uint32     specdata2;
};

struct nfs_fh3 {
    opaque       data<NFS3_FHSIZE>;
};

struct nfstime3 {
    uint32   seconds;
    uint32   nseconds;
};

struct fattr3 {
    ftype3     type;
    mode3      mode;
    uint32     nlink;
    uid3       uid;
    gid3       gid;
    size3      size;
    size3      used;
    specdata3  rdev;
    uint64     fsid;
    fileid3    fileid;
    nfstime3   atime;
    nfstime3   mtime;
    nfstime3   ctime;
};

union post_op_attr switch (bool attributes_follow) {
    case TRUE:
        fattr3   attributes;
    case FALSE:
        void;
};

struct wcc_attr {
    size3       size;
    nfstime3    mtime;
    nfstime3    ctime;
};

union pre_op_attr switch (bool attributes_follow) {
    case TRUE:
        wcc_attr  attributes;
    case FALSE:
        void;
};

struct wcc_data {
    pre_op_attr    before;
    post_op_attr   after;
};

union post_op_fh3 switch (bool handle_follows) {
    case TRUE:
        nfs_fh3  handle;
    case FALSE:
        void;
};

/* sattr3 */
enum time_how {
    DONT_CHANGE        = 0,
    SET_TO_SERVER_TIME = 1,
    SET_TO_CLIENT_TIME = 2
};

union set_mode3 switch (bool set_it) {
    case TRUE:
        mode3    mode;
    default:
        void;
};

union set_uid3 switch (bool set_it) {
    case TRUE:
        uid3     uid;
    default:
        void;
};

union set_gid3 switch (bool set_it) {
    case TRUE:
        gid3     gid;
    default:
        void;
};

union set_size3 switch (bool set_it) {
    case TRUE:
        size3    size;
    default:
        void;
};

union set_atime switch (time_how set_it) {
    case SET_TO_CLIENT_TIME:
        nfstime3  atime;
    default:
        void;
};

union set_mtime switch (time_how set_it) {
    case SET_TO_CLIENT_TIME:
        nfstime3  mtime;
    default:
        void;
};

struct sattr3 {
    set_mode3   mode;
    set_uid3    uid;
    set_gid3    gid;
    set_size3   size;
    set_atime   atime;
    set_mtime   mtime;
};
/* sattr3 end */

struct diropargs3 {
    nfs_fh3     dir;
    filename3   name;
};

