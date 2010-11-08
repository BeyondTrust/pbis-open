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
 *        rpc2.x
 *
 * Abstract:
 *
 *        ONC RPC
 *
 *        ONC RPC XDR routines.  We do not use glibc rpc - implementing it by
 *        ourselves on top of XDR.
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

enum rpc2_auth_flavor {
    AUTH_NONE       = 0,
    AUTH_SYS        = 1
};

struct rpc2_opaque_auth {
    auth_flavor flavor;
    opaque body<400>;
};

struct rpc2_authsys_parms {
    unsigned int stamp;
    string machinename<255>;
    unsigned int uid;
    unsigned int gid;
    unsigned int gids<16>;
};

enum rpc2_msg_type {
    CALL  = 0,
    REPLY = 1
};

enum rpc2_reply_stat {
    MSG_ACCEPTED = 0,
    MSG_DENIED   = 1
};

enum rpc2_accept_stat {
    SUCCESS       = 0, /* RPC executed successfully             */
    PROG_UNAVAIL  = 1, /* remote hasn't exported program        */
    PROG_MISMATCH = 2, /* remote can't support version #        */
    PROC_UNAVAIL  = 3, /* program can't support procedure       */
    GARBAGE_ARGS  = 4, /* procedure can't decode params         */
    SYSTEM_ERR    = 5  /* errors like memory allocation failure */
};

enum rpc2_reject_stat {
    RPC_MISMATCH = 0, /* RPC version number != 2          */
    AUTH_ERROR = 1    /* remote can't authenticate caller */
};

enum rpc2_auth_stat {
    AUTH_OK           = 0,  /* success                          */
    /*
    * failed at remote end
    */
    AUTH_BADCRED      = 1,  /* bad credential (seal broken)     */
    AUTH_REJECTEDCRED = 2,  /* client must begin new session    */
    AUTH_BADVERF      = 3,  /* bad verifier (seal broken)       */
    AUTH_REJECTEDVERF = 4,  /* verifier expired or replayed     */
    AUTH_TOOWEAK      = 5,  /* rejected for security reasons    */
    /*
    * failed locally
    */
    AUTH_INVALIDRESP  = 6,  /* bogus response verifier          */
    AUTH_FAILED       = 7   /* reason unknown                   */
};

struct rpc2_call_body {
    unsigned int rpcvers;       /* must be equal to two (2) */
    unsigned int prog;
    unsigned int vers;
    unsigned int proc;
    rpc2_opaque_auth  cred;
    rpc2_opaque_auth  verf;
    /* procedure specific parameters start here */
};

struct rpc2_mismatch_info {
    unsigned int low;
    unsigned int high;
};

union rpc2_accepted_reply_data switch (rpc2_accept_stat stat) {
    case SUCCESS:
        opaque results[0];
        /*
        * procedure-specific results start here
        */
    case PROG_MISMATCH:
        rpc2_mismatch_info  mismatch_info;
    default:
        /*
        * Void.  Cases include PROG_UNAVAIL, PROC_UNAVAIL,
        * GARBAGE_ARGS, and SYSTEM_ERR.
        */
        void;
};

struct rpc2_accepted_reply {
    rpc2_opaque_auth verf;
    rpc2_accepted_reply_data data;
};

union rpc2_rejected_reply switch (rpc2_reject_stat stat) {
    case RPC_MISMATCH:
        rpc2_mismatch_info  mismatch_info;
    case AUTH_ERROR:
        rpc2_auth_stat stat;
};

union rpc2_reply_body switch (rpc2_reply_stat stat) {
    case MSG_ACCEPTED:
        rpc2_accepted_reply areply;
    case MSG_DENIED:
        rpc2_rejected_reply rreply;
};

union rpc2_rpc_msg_body switch (rpc2_msg_type mtype) {
    case CALL:
        rpc2_call_body cbody;
    case REPLY:
        rpc2_reply_body rbody;
};

struct rpc2_rpc_msg {
    unsigned int xid;
    rpc2_rpc_msg_body body;
};

