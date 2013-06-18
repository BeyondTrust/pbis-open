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
#ifndef _COMSOC_BSD_H
#define _COMSOC_BSD_H	1
/*
**
**  NAME:
**
**      comsoc_bsd.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  The platform-specific portion of the internal network "socket" object
**  interface.  See abstract in "comsoc.h" for details.
**
**  For BSD 4.3 & 4.4 systems.  
**
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dce/dce.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <comnaf.h>
#include <commonp.h>
#include <ipnaf.h>

PRIVATE rpc_socket_error_t
rpc__bsd_socket_open_basic(
    rpc_naf_id_t        naf,
    rpc_network_if_id_t net_if,
    rpc_network_protocol_id_t net_prot ATTRIBUTE_UNUSED,
    rpc_socket_basic_t        *sock
    );

PRIVATE rpc_socket_error_t
rpc__bsd_socket_close_basic(
    rpc_socket_basic_t        sock
    );

EXTERNAL PRIVATE rpc_socket_vtbl_t rpc_g_bsd_socket_vtbl;

#endif /* _COMSOC_BSD_H */
