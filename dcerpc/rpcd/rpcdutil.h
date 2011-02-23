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
#ifndef RPCDUTIL_H
#define RPCDUTIL_H

/*
**
**  NAME:
**
**      rpcdutil.h
**
**  FACILITY:
**
**      RPC Daemon Utility Routines - header file
**
**  ABSTRACT:
**
**  RPC Daemon Utility Routines - protocol tower manipulation, sleep primitives
**
**
*/


typedef struct 
{
    rpc_if_id_t             interface;
    rpc_syntax_id_t         data_rep;
    rpc_protocol_id_t       rpc_protocol;
    unsigned32              rpc_protocol_vers_major;
    unsigned32              rpc_protocol_vers_minor;
    rpc_protseq_id_t        protseq;
} twr_fields_t, *twr_fields_p_t;


PRIVATE void tower_to_fields
    (
        twr_p_t         tower,
        twr_fields_t    *tfp,
        error_status_t  *status
    );
        
PRIVATE void tower_to_addr
    (
        twr_p_t         tower,
        rpc_addr_p_t    *addr,
        error_status_t  *status
    );
        
PRIVATE void tower_to_if_id
    (
        twr_p_t         tower,
        rpc_if_id_t     *if_id,
        error_status_t  *status
    );

PRIVATE void tower_ss_copy
    (
        twr_p_t         src_tower,
        twr_p_t         *dest_tower,
        error_status_t  *status
    );


PRIVATE void ru_sleep_until
    (
        struct timeval  *starttime, 
        unsigned32      nsecs
    );

PRIVATE void ru_sleep
    (
        unsigned32      nsecs
    );

#endif
