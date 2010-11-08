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
 *        nfs3_procs.x
 *
 * Abstract:
 *
 *        NFS3
 *
 *        NFS3 procedures and arguments definitions
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

const NFSPROC3_NULL         = 0;
const NFSPROC3_GETATTR      = 1;
const NFSPROC3_SETATTR      = 2;
const NFSPROC3_LOOKUP       = 3;
const NFSPROC3_ACCESS       = 4;
const NFSPROC3_READLINK     = 5;
const NFSPROC3_READ         = 6;
const NFSPROC3_WRITE        = 7;
const NFSPROC3_CREATE       = 8;
const NFSPROC3_MKDIR        = 9;
const NFSPROC3_SYMLINK      = 10;
const NFSPROC3_MKNOD        = 11;
const NFSPROC3_REMOVE       = 12;
const NFSPROC3_RMDIR        = 13;
const NFSPROC3_RENAME       = 14;
const NFSPROC3_LINK         = 15;
const NFSPROC3_READDIR      = 16;
const NFSPROC3_READDIRPLUS  = 17;
const NFSPROC3_FSSTAT       = 18;
const NFSPROC3_FSINFO       = 19;
const NFSPROC3_PATHCONF     = 20;
const NFSPROC3_COMMIT       = 21;

/* NFSPROC3_GETATTR3 */
struct GETATTR3args {
    nfs_fh3  object;
};

struct GETATTR3resok {
    fattr3   obj_attributes;
};

union GETATTR3res switch (nfsstat3 status) {
    case NFS3_OK:
        GETATTR3resok  resok;
    default:
        void;
};

/* NFSPROC3_SETATTR */
union sattrguard3 switch (bool check) {
    case TRUE:
        nfstime3  obj_ctime;
    case FALSE:
        void;
};

struct SETATTR3args {
    nfs_fh3      object;
    sattr3       new_attributes;
    sattrguard3  guard;
};

struct SETATTR3resok {
    wcc_data  obj_wcc;
};

struct SETATTR3resfail {
    wcc_data  obj_wcc;
};

union SETATTR3res switch (nfsstat3 status) {
    case NFS3_OK:
        SETATTR3resok   resok;
    default:
        SETATTR3resfail resfail;
};

/* NFSPROC3_LOOKUP */
struct LOOKUP3args {
    diropargs3  what;
};

struct LOOKUP3resok {
    nfs_fh3      object;
    post_op_attr obj_attributes;
    post_op_attr dir_attributes;
};

struct LOOKUP3resfail {
    post_op_attr dir_attributes;
};

union LOOKUP3res switch (nfsstat3 status) {
    case NFS3_OK:
        LOOKUP3resok    resok;
    default:
        LOOKUP3resfail  resfail;
};

/* NFSPROC3_ACCESS */
const ACCESS3_READ    = 0x0001;
const ACCESS3_LOOKUP  = 0x0002;
const ACCESS3_MODIFY  = 0x0004;
const ACCESS3_EXTEND  = 0x0008;
const ACCESS3_DELETE  = 0x0010;
const ACCESS3_EXECUTE = 0x0020;

struct ACCESS3args {
    nfs_fh3  object;
    uint32   access;
};

struct ACCESS3resok {
    post_op_attr   obj_attributes;
    uint32         access;
};

struct ACCESS3resfail {
    post_op_attr   obj_attributes;
};

union ACCESS3res switch (nfsstat3 status) {
    case NFS3_OK:
        ACCESS3resok   resok;
    default:
        ACCESS3resfail resfail;
};

/* NFSPROC3_READLINK */
struct READLINK3args {
    nfs_fh3  symlink;
};

struct READLINK3resok {
    post_op_attr   symlink_attributes;
    nfspath3       data;
};

struct READLINK3resfail {
    post_op_attr   symlink_attributes;
};

union READLINK3res switch (nfsstat3 status) {
    case NFS3_OK:
        READLINK3resok   resok;
    default:
        READLINK3resfail resfail;
};

/* NFSPROC3_READ */
struct READ3args {
    nfs_fh3  file;
    offset3  offset;
    count3   count;
};

struct READ3resok {
    post_op_attr   file_attributes;
    count3         count;
    bool           eof;
    opaque         data<>;
};

struct READ3resfail {
    post_op_attr   file_attributes;
};

union READ3res switch (nfsstat3 status) {
    case NFS3_OK:
        READ3resok   resok;
    default:
        READ3resfail resfail;
};

/* NFSPROC3_WRITE */
enum stable_how {
    UNSTABLE  = 0,
    DATA_SYNC = 1,
    FILE_SYNC = 2
};

struct WRITE3args {
    nfs_fh3     file;
    offset3     offset;
    count3      count;
    stable_how  stable;
    opaque      data<>;
};

struct WRITE3resok {
    wcc_data    file_wcc;
    count3      count;
    stable_how  committed;
    writeverf3  verf;
};

struct WRITE3resfail {
    wcc_data    file_wcc;
};

union WRITE3res switch (nfsstat3 status) {
    case NFS3_OK:
        WRITE3resok    resok;
    default:
        WRITE3resfail  resfail;
};

/* NFSPROC3_CREATE */
enum createmode3 {
    UNCHECKED = 0,
    GUARDED   = 1,
    EXCLUSIVE = 2
};

union createhow3 switch (createmode3 mode) {
    case UNCHECKED:
    case GUARDED:
        sattr3       obj_attributes;
    case EXCLUSIVE:
        createverf3  verf;
};

struct CREATE3args {
    diropargs3   where;
    createhow3   how;
};

struct CREATE3resok {
    post_op_fh3   obj;
    post_op_attr  obj_attributes;
    wcc_data      dir_wcc;
};

struct CREATE3resfail {
    wcc_data      dir_wcc;
};

union CREATE3res switch (nfsstat3 status) {
    case NFS3_OK:
        CREATE3resok    resok;
    default:
        CREATE3resfail  resfail;
};

/* NFSPROC3_MKDIR */
struct MKDIR3args {
    diropargs3   where;
    sattr3       attributes;
};

struct MKDIR3resok {
    post_op_fh3   obj;
    post_op_attr  obj_attributes;
    wcc_data      dir_wcc;
};

struct MKDIR3resfail {
    wcc_data      dir_wcc;
};

union MKDIR3res switch (nfsstat3 status) {
    case NFS3_OK:
        MKDIR3resok   resok;
    default:
        MKDIR3resfail resfail;
};

/* NFSPROC3_SYMLINK */
struct symlinkdata3 {
    sattr3    symlink_attributes;
    nfspath3  symlink_data;
};

struct SYMLINK3args {
    diropargs3    where;
    symlinkdata3  symlink;
};

struct SYMLINK3resok {
    post_op_fh3   obj;
    post_op_attr  obj_attributes;
    wcc_data      dir_wcc;
};

struct SYMLINK3resfail {
    wcc_data      dir_wcc;
};

union SYMLINK3res switch (nfsstat3 status) {
    case NFS3_OK:
        SYMLINK3resok   resok;
    default:
        SYMLINK3resfail resfail;
};

/* NFSPROC3_MKNOD */
struct devicedata3 {
    sattr3     dev_attributes;
    specdata3  spec;
};

union mknoddata3 switch (ftype3 type) {
    case NF3CHR:
    case NF3BLK:
        devicedata3  device;
    case NF3SOCK:
    case NF3FIFO:
        sattr3       pipe_attributes;
    default:
        void;
};

struct MKNOD3args {
    diropargs3   where;
    mknoddata3   what;
};

struct MKNOD3resok {
    post_op_fh3   obj;
    post_op_attr  obj_attributes;
    wcc_data      dir_wcc;
};

struct MKNOD3resfail {
    wcc_data      dir_wcc;
};

union MKNOD3res switch (nfsstat3 status) {
    case NFS3_OK:
        MKNOD3resok   resok;
    default:
        MKNOD3resfail resfail;
};

/* NFSPROC3_REMOVE */
struct REMOVE3args {
    diropargs3  object;
};

struct REMOVE3resok {
    wcc_data    dir_wcc;
};

struct REMOVE3resfail {
    wcc_data    dir_wcc;
};

union REMOVE3res switch (nfsstat3 status) {
    case NFS3_OK:
        REMOVE3resok   resok;
    default:
        REMOVE3resfail resfail;
};

/* NFSPROC3_RMDIR */
struct RMDIR3args {
    diropargs3  object;
};

struct RMDIR3resok {
    wcc_data    dir_wcc;
};

struct RMDIR3resfail {
    wcc_data    dir_wcc;
};

union RMDIR3res switch (nfsstat3 status) {
    case NFS3_OK:
        RMDIR3resok   resok;
    default:
        RMDIR3resfail resfail;
};

/* NFSPROC3_RENAME */
struct RENAME3args {
    diropargs3   from;
    diropargs3   to;
};

struct RENAME3resok {
    wcc_data     fromdir_wcc;
    wcc_data     todir_wcc;
};

struct RENAME3resfail {
    wcc_data     fromdir_wcc;
    wcc_data     todir_wcc;
};

union RENAME3res switch (nfsstat3 status) {
    case NFS3_OK:
        RENAME3resok   resok;
    default:
        RENAME3resfail resfail;
};

/* NFSPROC3_LINK */
struct LINK3args {
    nfs_fh3     file;
    diropargs3  link;
};

struct LINK3resok {
    post_op_attr   file_attributes;
    wcc_data       linkdir_wcc;
};

struct LINK3resfail {
    post_op_attr   file_attributes;
    wcc_data       linkdir_wcc;
};

union LINK3res switch (nfsstat3 status) {
    case NFS3_OK:
        LINK3resok    resok;
    default:
        LINK3resfail  resfail;
};

/* NFSPROC3_READDIR */
struct READDIR3args {
    nfs_fh3      dir;
    cookie3      cookie;
    cookieverf3  cookieverf;
    count3       count;
};

struct entry3 {
    fileid3      fileid;
    filename3    name;
    cookie3      cookie;
    entry3       *nextentry;
};

struct dirlist3 {
    entry3       *entries;
    bool         eof;
};

struct READDIR3resok {
    post_op_attr dir_attributes;
    cookieverf3  cookieverf;
    dirlist3     reply;
};

struct READDIR3resfail {
    post_op_attr dir_attributes;
};

union READDIR3res switch (nfsstat3 status) {
    case NFS3_OK:
        READDIR3resok   resok;
    default:
        READDIR3resfail resfail;
};

/* NFSPROC3_READDIRPLUS */
struct READDIRPLUS3args {
    nfs_fh3      dir;
    cookie3      cookie;
    cookieverf3  cookieverf;
    count3       dircount;
    count3       maxcount;
};

struct entryplus3 {
    fileid3      fileid;
    filename3    name;
    cookie3      cookie;
    post_op_attr name_attributes;
    post_op_fh3  name_handle;
    entryplus3   *nextentry;
};

struct dirlistplus3 {
    entryplus3   *entries;
    bool         eof;
};

struct READDIRPLUS3resok {
    post_op_attr dir_attributes;
    cookieverf3  cookieverf;
    dirlistplus3 reply;
};

struct READDIRPLUS3resfail {
    post_op_attr dir_attributes;
};

union READDIRPLUS3res switch (nfsstat3 status) {
    case NFS3_OK:
        READDIRPLUS3resok   resok;
    default:
        READDIRPLUS3resfail resfail;
};

/* NFSPROC3_FSSTAT */
struct FSSTAT3args {
    nfs_fh3   fsroot;
};

struct FSSTAT3resok {
    post_op_attr obj_attributes;
    size3        tbytes;
    size3        fbytes;
    size3        abytes;
    size3        tfiles;
    size3        ffiles;
    size3        afiles;
    uint32       invarsec;
};

struct FSSTAT3resfail {
    post_op_attr obj_attributes;
};

union FSSTAT3res switch (nfsstat3 status) {
    case NFS3_OK:
        FSSTAT3resok   resok;
    default:
        FSSTAT3resfail resfail;
};

/* NFSPROC3_FSINFO */
const FSF3_LINK        = 0x0001;
const FSF3_SYMLINK     = 0x0002;
const FSF3_HOMOGENEOUS = 0x0008;
const FSF3_CANSETTIME  = 0x0010;

struct FSINFOargs {
    nfs_fh3   fsroot;
};

struct FSINFO3resok {
    post_op_attr obj_attributes;
    uint32       rtmax;
    uint32       rtpref;
    uint32       rtmult;
    uint32       wtmax;
    uint32       wtpref;
    uint32       wtmult;
    uint32       dtpref;
    size3        maxfilesize;
    nfstime3     time_delta;
    uint32       properties;
};

struct FSINFO3resfail {
    post_op_attr obj_attributes;
};

union FSINFO3res switch (nfsstat3 status) {
    case NFS3_OK:
        FSINFO3resok   resok;
    default:
        FSINFO3resfail resfail;
};

/* NFSPROC3_PATHCONF */
struct PATHCONF3args {
    nfs_fh3   object;
};

struct PATHCONF3resok {
    post_op_attr obj_attributes;
    uint32       linkmax;
    uint32       name_max;
    bool         no_trunc;
    bool         chown_restricted;
    bool         case_insensitive;
    bool         case_preserving;
};

struct PATHCONF3resfail {
    post_op_attr obj_attributes;
};

union PATHCONF3res switch (nfsstat3 status) {
    case NFS3_OK:
        PATHCONF3resok   resok;
    default:
        PATHCONF3resfail resfail;
};

/* NFSPROC3_COMMIT */
struct COMMIT3args {
    nfs_fh3    file;
    offset3    offset;
    count3     count;
};

struct COMMIT3resok {
    wcc_data   file_wcc;
    writeverf3 verf;
};

struct COMMIT3resfail {
    wcc_data   file_wcc;
};

union COMMIT3res switch (nfsstat3 status) {
    case NFS3_OK:
        COMMIT3resok   resok;
    default:
        COMMIT3resfail resfail;
};

