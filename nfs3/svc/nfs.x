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
 *        nfs.x
 *
 * Abstract:
 *
 *        NFS
 *
 *        NFSv3, v4 and MOUNTv3 RPC-level structs.
 *        Using "default" in some well-defined unions to be able to take
 *        control over errors in the code.
 *        Only request path is handled here. Reply encodings should use
 *        rpc2.x reply definitions.
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

struct empty_proc_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
};

struct getattr3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    GETATTR3args      agrs;
};

struct setattr3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    SETATTR3args      agrs;
};

struct lookup3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    LOOKUP3args       args;
};

struct access3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    ACCESS3args       args;
};

struct readlink3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    READLINK3args     args;
};

struct read3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    READ3args         args;
};

struct write3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    WRITE3args        args;
};

struct create3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    CREATE3args       args;
};

struct mkdir3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    MKDIR3args        args;
};

struct symlink3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    SYMLINK3args      args;
};

struct mknod3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    MKNOD3args        args;
};

struct remove3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    REMOVE3args       args;
};

struct rmdir3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    RMDIR3args        args;
};

struct rename3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    RENAME3args       args;
};

struct link3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    LINK3args         args;
};

struct readdir3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    READDIR3args      args;
};

struct readdirplus3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    READDIRPLUS3args  args;
};

struct fsstat3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    FSSTAT3args       args;
};

struct fsinfo3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    FSINFO3args       args;
};

struct pathconf3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    PATHCONF3args     args;
};

struct commit3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    COMMIT3args       args;
};

union nfsv3_call_body switch (unsigned int proc) {
    case NFSPROC3_NULL:
        empty_proc_body             null_body;
    case NFSPROC3_GETATTR:
        getattr3_proc_call_body     getattr_body;
    case NFSPROC3_SETATTR:
        setattr3_proc_call_body     setattr_body;
    case NFSPROC3_LOOKUP:
        lookup3_proc_call_body      lookup_body;
    case NFSPROC3_ACCESS:
        access3_proc_call_body      access_body;
    case NFSPROC3_READLINK:
        readlink3_proc_call_body    readlink_body;
    case NFSPROC3_READ:
        read3_proc_call_body        read_body;
    case NFSPROC3_WRITE:
        write3_proc_call_body       write_body;
    case NFSPROC3_CREATE:
        create3_proc_call_body      create_body;
    case NFSPROC3_MKDIR:
        mkdir3_proc_call_body       mkdir_body;
    case NFSPROC3_SYMLINK:
        symlink3_proc_call_body     symlink_body;
    case NFSPROC3_MKNOD:
        mknod3_proc_call_body       mknod_body;
    case NFSPROC3_REMOVE:
        remove3_proc_call_body      remove_body;
    case NFSPROC3_RMDIR:
        rmdir3_proc_call_body       rmdir_body;
    case NFSPROC3_RENAME:
        rename3_proc_call_body      rename_body;
    case NFSPROC3_LINK:
        link3_proc_call_body        link_body;
    case NFSPROC3_READDIR:
        readdir3_proc_call_body     readdir_body;
    case NFSPROC3_READDIRPLUS:
        readdirplus3_proc_call_body readdirplus_body;
    case NFSPROC3_FSSTAT:
        fsstat3_proc_call_body      fsstat_body;
    case NFSPROC3_FSINFO:
        fsinfo3_proc_call_body      fsinfo_body;
    case NFSPROC3_PATHCONF:
        pathconf3_proc_call_body    pathconf_body;
    case NFSPROC3_COMMIT:
        commit3_proc_call_body      commit_body;
    default:
        void;
};

union nfsv4_call_body switch (unsigned int proc) {
    case 0:
        empty_proc_body null_body;
    default:
        void;
};

union nfs_call_body switch (unsigned int vers) {
    case 3:
        nfsv3_call_body  body3;
    case 4:
        nfsv4_call_body  body4;
    default:
        void;
};

struct mnt3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    string            args<MNTPATHLEN>;
};

struct umnt3_proc_call_body {
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    string            args<MNTPATHLEN>;
};

union mountv3_call_body switch (unsigned int proc) {
    case MOUNTPROC3_NULL:
        empty_proc_body         null_body;
    case MOUNTPROC3_MNT:
        mnt3_proc_call_body     mnt_body;
    case MOUNTPROC3_DUMP:
        empty_proc_body         dump_body;
    case MOUNTPROC3_UMNT:
        umnt3_proc_call_body    umnt_body;
    case MOUNTPROC3_UMNTALL:
        empty_proc_body         umntall_body;
    case MOUNTPROC3_EXPORT:
        empty_proc_body         export_body;
    default:
        void;
};

union mount_call_body switch (unsigned int vers) {
    case 3:
        mountv3_call_body    body;
    default:
        void;
};

union program_call_body switch (unsigned int prog) {
    case NFS_PROGRAM:
        nfs_call_body    nbody;
    case MOUNT_PROGRAM:
        mount_call_body  mbody;
    default:
        void;
};

union call_body switch (unsigned int rpcvers) {
    case 2:
        program_call_body body;
    default:
        void;
};
