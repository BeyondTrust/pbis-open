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
#ifndef RPCDEPDB_H
#define RPCDEPDB_H

/*
**
**  NAME:
**
**      rpcdepdb.h
**
**  FACILITY:
**
**      RPC Daemon
**
**  ABSTRACT:
**
**  Generic Endpoint Database Manager.
**
**
*/


typedef void *epdb_handle_t;

/*  Get the handle for the ep database from
 *  a handle to the endpoint object
 */
PRIVATE void epdb_handle_from_ohandle
    (
        handle_t            h,
        epdb_handle_t       *epdb_h,
        error_status_t      *status
    );

/*  Return the handle to the ep database
 */
PRIVATE epdb_handle_t epdb_inq_handle (void);


PRIVATE epdb_handle_t epdb_init
    (
        unsigned char       *pathname, 
        error_status_t      *status
    );

PRIVATE void epdb_insert
    (
        epdb_handle_t       h,
        ept_entry_p_t       xentry,
        boolean32           replace,
        error_status_t      *status
    );

PRIVATE void epdb_delete
    (
        epdb_handle_t       h,
        ept_entry_p_t       xentry,
        error_status_t      *status
    );

PRIVATE void epdb_mgmt_delete
    (
        epdb_handle_t       h,
        boolean32           object_speced,
        dce_uuid_p_t            object,
        twr_p_t             tower,
        error_status_t      *status
    );

PRIVATE void epdb_lookup
    (
        epdb_handle_t       h,
        unsigned32          inquiry_type,
        dce_uuid_p_t            object,
        rpc_if_id_p_t       interface,
        unsigned32          vers_option,
        ept_lookup_handle_t *entry_handle,
        unsigned32          max_ents,
        unsigned32          *num_ents,
        ept_entry_t         entries[],
        error_status_t      *status
    );

PRIVATE void epdb_map
    (
        epdb_handle_t       h,
        dce_uuid_p_t            object,
        twr_p_t             map_tower,
        ept_lookup_handle_t *entry_handle,
        unsigned32          max_towers,
        unsigned32          *num_towers,
        twr_t               *fwd_towers[],
        unsigned32          *status
    );

PRIVATE void epdb_fwd
    (
        epdb_handle_t       h,
        dce_uuid_p_t            object,
        rpc_if_id_p_t       interface,
        rpc_syntax_id_p_t   data_rep,
        rpc_protocol_id_t   rpc_protocol,
        unsigned32          rpc_protocol_vers_major,
        unsigned32          rpc_protocol_vers_minor,
        rpc_addr_p_t        addr,
        ept_lookup_handle_t *map_handle,
        unsigned32          max_addrs,
        unsigned32          *num_addrs,
        rpc_addr_p_t        fwd_addrs[],
        unsigned32          *status
    );

PRIVATE void epdb_inq_object
    (
        epdb_handle_t h,
        dce_uuid_t *object,
        error_status_t *status
    );

PRIVATE void epdb_delete_lookup_handle
    (
        epdb_handle_t       h,
        ept_lookup_handle_t *entry_handle
    );



#endif
