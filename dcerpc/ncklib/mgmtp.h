/*
 * 
 * (c) Copyright 1991 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1991 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1991 DIGITAL EQUIPMENT CORPORATION
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
#ifndef _MGMTP_H
#define _MGMTP_H	1

/*
**
**  NAME
**
**      mgmtp.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Private management services.
**
**
*/

#include <dce/dce.h>


#ifdef __cplusplus
extern "C" {
#endif


PRIVATE unsigned32 rpc__mgmt_init ( void );

PRIVATE boolean32 rpc__mgmt_authorization_check (
        rpc_binding_handle_t     /*binding_h*/,
        unsigned32               /*op*/,
        boolean32                /*deflt*/,
        unsigned32              * /*status*/
    );

PRIVATE void rpc__mgmt_stop_server_lsn_mgr (            
        rpc_binding_handle_t     /*binding_h*/,
        unsigned32              * /*status*/
    );


#ifdef __cplusplus
}
#endif

#endif /* _MGMTP_H */
