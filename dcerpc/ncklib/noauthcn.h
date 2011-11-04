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
 */
#ifndef _NOAUTHCN_H
#define _NOAUTHCN_H 	1

/*
**  NAME
**
**      noauthcn.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  The noauth CN authentication module interface.
**
**
*/

#include <cn.h> 

typedef struct 
{
    rpc_cn_auth_info_t  cn_info;

    /*
     * Noauth specific fields here.
     */
} rpc_noauth_cn_info_t, *rpc_noauth_cn_info_p_t;

EXTERNAL rpc_cn_auth_epv_t rpc_g_noauth_cn_epv;

#endif /* _NOAUTHCN_H */
