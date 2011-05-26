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
/*
**
**  NAME:
**
**      rpcdepdb.c
**
**  FACILITY:
**
**      RPC Daemon
**
**  ABSTRACT:
**
**  Generic Endpoint Database Management, Forwarding,
**  and Lookup Routines
**
**
*/

#include <commonp.h>
#include <com.h>
#include <comp.h>

#include <dce/ep.h>     /* derived from ep.idl */
#include <dce/mgmt.h>   /* derived from mgmt.idl */
#include <dsm.h>        /* derived from dsm.idl */

#include <rpcdp.h>
#include <rpcddb.h>
#include <rpcdepdb.h>
#include <rpcdepdbp.h>
#include <rpcdutil.h>

#include <comtwr.h>

/*
 * The server supports a single endpoint location database.
 */
INTERNAL epdb_handle_t    epdb_handle = NULL;

/*
 * The current version of the persistent database file (the value in a file_hdr).
 */
#define epdb_c_file_version 8


INTERNAL void epdb_recreate_lists
    (
        struct db       *h,
        error_status_t  *status
    );
    
INTERNAL void epdb_chk_entry
    (
        ept_entry_p_t   xentry,
        twr_fields_p_t  tfp,
        rpc_addr_p_t    addr,
        error_status_t  *status
    );

INTERNAL void epdb_chk_map_entry
    (
        twr_fields_p_t  tfp,
        error_status_t  *status
    );

INTERNAL void epdb_to_ept
    (
        db_entry_p_t    entp,
        ept_entry_t     *xentry,
        error_status_t  *status
    );
        
INTERNAL void epdb_insert_entry
    (
        struct db       *h,
        ept_entry_p_t   xentry,
        twr_fields_p_t  tfp,
        rpc_addr_p_t    addr,
        db_entry_p_t    *entp,
        error_status_t  *status
    );
        
INTERNAL void epdb_replace_entry
    (
        struct db       *h,
        ept_entry_p_t   xentry,
        db_entry_p_t    entp,
        error_status_t  *status
    );
        
INTERNAL boolean32 epdb_is_replace_candidate
    (
        db_entry_t      *entp,
        dce_uuid_p_t        object,
        twr_fields_p_t  tfp,
        rpc_addr_p_t    addr
    );

INTERNAL void epdb_delete_replaceable_entries
    (
        struct db       *h,
        dce_uuid_p_t        object,
        twr_fields_p_t  tfp,
        rpc_addr_p_t    addr,
        error_status_t  *status
    );

INTERNAL void epdb_delete_entries_by_obj_if_addr
    (
        struct db           *h,
        boolean32           object_speced,
        dce_uuid_p_t            object,
        rpc_if_id_p_t       interface,
        rpc_addr_p_t        addr,
        error_status_t      *status
    );

INTERNAL db_entry_t *epdb_lookup_entry
    (
        struct db       *h,
        ept_entry_p_t   xentry
    );
        

INTERNAL void lookup
    (
        struct db           *h,
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
        
INTERNAL void lookup_match
    (
        unsigned32          inquiry_type,
        dce_uuid_p_t            object,
        rpc_if_id_p_t       interface,
        unsigned32          vers_option,
        unsigned32          max_ents,
        unsigned32          *num_ents,
        ept_entry_t         entries[],
        unsigned32          list_type,
        db_lists_t          **lpp,
        error_status_t      *status
    );
        
INTERNAL void map
    (
        struct db           *h,
        dce_uuid_p_t            object,
        rpc_if_id_p_t       interface,
        rpc_syntax_id_p_t   data_rep,
        rpc_protocol_id_t   rpc_protocol,
        unsigned32          rpc_protocol_vers_major,
        unsigned32          rpc_protocol_vers_minor,
        rpc_protseq_id_t    protseq,
        ept_lookup_handle_t *map_handle,
        unsigned32          max_ents,
        unsigned32          *n_ents,
        db_entry_t          *db_entries[],
        unsigned32          *status
    );
        
INTERNAL void map_match
    (
        dce_uuid_p_t            object,
        rpc_if_id_p_t       interface,
        rpc_syntax_id_p_t   data_rep,
        rpc_protocol_id_t   rpc_protocol,
        unsigned32          rpc_protocol_vers_major,
        unsigned32          rpc_protocol_vers_minor,
        rpc_protseq_id_t    protseq,
        unsigned32          max_ents,
        unsigned32          *n_ents,
        db_entry_t          *entries[],
        ept_lookup_handle_t *map_handle,
        unsigned32          pass,
        db_list_type_t      list,
        db_lists_t          **lpp,
        unsigned32          *status
    );

INTERNAL void map_mgmt
    (
        struct db           *h,
        dce_uuid_p_t            object,
        rpc_syntax_id_p_t   data_rep,
        rpc_protocol_id_t   rpc_protocol,
        unsigned32          rpc_protocol_vers_major,
        unsigned32          rpc_protocol_vers_minor,
        rpc_protseq_id_t    protseq,
        ept_lookup_handle_t *map_handle,
        unsigned32          max_ents,
        unsigned32          *n_ents,
        db_entry_t          *db_entries[],
        unsigned32          *status
    );
        
INTERNAL void map_mgmt_match
    (
        dce_uuid_p_t            object,
        rpc_syntax_id_p_t   data_rep,
        rpc_protocol_id_t   rpc_protocol,
        unsigned32          rpc_protocol_vers_major,
        unsigned32          rpc_protocol_vers_minor,
        rpc_protseq_id_t    protseq,
        unsigned32          max_ents,
        unsigned32          *n_ents,
        db_entry_t          *entries[],
        ept_lookup_handle_t *map_handle,
        unsigned32          pass,
        db_list_type_t      list,
        db_lists_t          **lpp,
        unsigned32          *status
    );

INTERNAL boolean32 map_mgmt_endpt_unique
    (
        rpc_addr_p_t    addr,
        unsigned32      n_ents,
        db_entry_t      *entries[]
    );


/*
 * Get an endpoint database handle from a handle_t.
 *
 * If the handle is not bound to an object (i.e. the object
 * is nil) treat this as a request to just use "the" endpoint
 * database.  If a specific object is specified, make sure that
 * the object is the same as the endpoint database object.
 * (accept a null handle to allow for "local" callers).
 */
PRIVATE void epdb_handle_from_ohandle(h, epdb_h, status)
handle_t        h;
epdb_handle_t   *epdb_h;
error_status_t  *status;
{
    dce_uuid_t  obj;
    dce_uuid_t  epdb_obj;

    SET_STATUS(status, rpc_s_ok);

    if (h == NULL)
    {
        *epdb_h = epdb_handle;
        return;
    }

    rpc_binding_inq_object(h, &obj, status);
    if (dce_uuid_is_nil(&obj, status))
        *epdb_h = epdb_handle;
    else
    {
        epdb_inq_object(epdb_handle, &epdb_obj, status);
        if (dce_uuid_equal(&obj, &epdb_obj, status))
            *epdb_h = epdb_handle;
        else
            SET_STATUS(status, ept_s_cant_perform_op); /* !!! ep_wrong_object? */
    }
    return;
}

/*  Return the handle to the ep database
 *  Since there's only one database just return what's in
 *  epdb_handle
 */
PRIVATE epdb_handle_t epdb_inq_handle()
{
    return(epdb_handle);
}


/*
 *  Open/create a database and return a handle to it.  
 *  Init the database lists and lock.  Start the task
 *  to check server liveness and to purge entries that
 *  are marked as deleted.
 */
PRIVATE epdb_handle_t epdb_init(pathname, status)
unsigned char   *pathname;
error_status_t  *status;
{
    struct db *h;

    SET_STATUS_OK(status);

    h = (struct db *) malloc(sizeof(struct db));
    if (h == NULL) {
        SET_STATUS(status, ept_s_cant_perform_op);
        return(NULL);
    }

    h->dsh = NULL;

    db_init_lists(h);

    db_open(h, pathname, epdb_c_file_version, status);
    if (! STATUS_OK(status))
    {
        /* Try deleting and recreating the database */
        unlink((char*) pathname);
        db_open(h, pathname, epdb_c_file_version, status);
        if (! STATUS_OK(status))
        {
            return NULL;
        }
    }

    /*  Database now exists, recreate its lists
     */
    epdb_recreate_lists(h, status);
    if (! STATUS_OK(status)) return(NULL);

    db_init_lock(h);

    sliv_init(h, status);
    if (! STATUS_OK(status)) return(NULL);

    epdb_handle = (epdb_handle_t) h;

    return((epdb_handle_t) h);
}

INTERNAL void epdb_recreate_lists(h, status)
struct db       *h;
error_status_t  *status;
{
    db_entry_t      *entp = NULL;
    dsm_marker_t    marker;      /* DSM datastore traversal marker */

    dsm_marker_reset(&marker);

    while(true) 
    {
        void* _entp = NULL;
        dsm_read(h->dsh,&marker, &_entp,status);    /* get next record */
        entp = (db_entry_t*) _entp;
        if (! STATUS_OK(status))
        {
            if (*status == dsm_err_no_more_entries) 
            {
                SET_STATUS(status, error_status_ok);
            }
            else
            { 
                if (dflag)
                    show_st("Error reading ept database", status);
                SET_STATUS(status, ept_s_cant_access);
            }
            return;
        }

        tower_to_addr(&entp->tower, &entp->addr, status);
        if (! STATUS_OK(status)) 
        {
            if (dflag)
                show_st("tower_to_addr error for ept entry", status);

            SET_STATUS(status, ept_s_invalid_entry);
            return;
        }

        entp->read_nrefs = 0;
        entp->ncomm_fails = 0;

        db_lists_add(h, entp);
    }
}


/*
 *  epdb_chk_entry - check an entry that is about to be inserted
 *  into the ep database
 *  All tower floors in an entry must be valid and non-nil
 *  (eg. nil interface uuid is prohibited).  Also, check tower
 *  via rpc_tower_to_binding - sliv tasks will need to get binding
 *  handle
 */
INTERNAL void epdb_chk_entry(xentry, tfp, addr, status)
ept_entry_p_t   xentry;
twr_fields_p_t  tfp;
rpc_addr_p_t    addr;
error_status_t  *status;
{ 
    rpc_binding_handle_t    binding_h;
    error_status_t          tmp_st;

    if (dce_uuid_is_nil(&tfp->interface.uuid, &tmp_st) ||
        dce_uuid_is_nil(&tfp->data_rep.id, &tmp_st) ||
        (addr == NULL) ||
        (addr->len == 0)) 
    {
        SET_STATUS(status, ept_s_invalid_entry);
        return;
    }

    rpc_tower_to_binding(xentry->tower->tower_octet_string, &binding_h, status);
    if (! STATUS_OK(status)) return;

    rpc_binding_free(&binding_h, &tmp_st);
}

INTERNAL void epdb_chk_map_entry(tfp, status)
twr_fields_p_t  tfp;
error_status_t  *status;
{
    error_status_t  tmp_st;

    if (dce_uuid_is_nil(&tfp->interface.uuid, &tmp_st) ||
        dce_uuid_is_nil(&tfp->data_rep.id, &tmp_st) ||
        (! RPC_PROTSEQ_INQ_SUPPORTED(tfp->protseq)) )
        SET_STATUS(status, ept_s_invalid_entry);
    else
        SET_STATUS_OK(status);
}

INTERNAL void epdb_to_ept(entp, xentry, status)
db_entry_p_t    entp;
ept_entry_t     *xentry;
error_status_t  *status;
{
    xentry->object = entp->object;
    memcpy((char *)xentry->annotation, (char *)entp->annotation, 
        sizeof(xentry->annotation));

    tower_ss_copy(&entp->tower, &xentry->tower, status);
}

/*  epdb_insert_entry
 *  Allocate dsm storage for an entry in the endpoint database
 *  Fill the entry from xentry, tfp, and addr
 *  Write it to disk
 *  Add entry to various lists
 *  Return db pointer to entry in entp
 */
INTERNAL void epdb_insert_entry(h, xentry, tfp, addr, entp, status)
struct db       *h;
ept_entry_p_t   xentry;
twr_fields_p_t  tfp;
rpc_addr_p_t    addr;
db_entry_p_t    *entp;
error_status_t  *status;
{ 
    db_entry_t      *db_entp;
    error_status_t  tmp_st;

    dsm_allocate(h->dsh, sizeof(db_entry_t) + xentry->tower->tower_length, 
        (void **) entp, status);
    if (! STATUS_OK(status)) return;

    db_entp = *entp;

    db_entp->read_nrefs = 0;
    db_entp->ncomm_fails = 0;
    db_entp->delete_flag = false;

    db_entp->object = xentry->object;
    db_entp->interface = tfp->interface;
    db_entp->object_nil = dce_uuid_is_nil(&db_entp->object, &tmp_st);
    db_entp->if_nil = dce_uuid_is_nil(&db_entp->interface.uuid, &tmp_st);
    db_entp->data_rep_id = tfp->data_rep.id;
    db_entp->data_rep_vers_major = RPC_IF_VERS_MAJOR(tfp->data_rep.version);
    db_entp->data_rep_vers_minor = RPC_IF_VERS_MINOR(tfp->data_rep.version);
    db_entp->rpc_protocol = tfp->rpc_protocol;
    db_entp->rpc_protocol_vers_major = tfp->rpc_protocol_vers_major;
    db_entp->rpc_protocol_vers_minor = tfp->rpc_protocol_vers_minor;
    db_entp->addr = addr;
    memcpy((char *)db_entp->annotation, (char *)xentry->annotation, 
        sizeof(db_entp->annotation));
    db_entp->tower.tower_length = xentry->tower->tower_length;
    memcpy((char *) db_entp->tower.tower_octet_string, (char *) xentry->tower->tower_octet_string, 
        xentry->tower->tower_length);

    dsm_write(h->dsh, (void *) db_entp, status);
    if (! STATUS_OK(status)) return;

    db_lists_add(h, db_entp);
}

/*  epdb_replace_entry
 *  Replace an existing entry - just change the annotation
 *  because all other fields are the same
 *  Also do some mgmt stuff - clear delete flag and ncomm_fails field
 */
INTERNAL void epdb_replace_entry(h, xentry, entp, status)
struct db       *h;
ept_entry_p_t   xentry;
db_entry_p_t    entp;
error_status_t  *status;
{
    entp->ncomm_fails = 0;
    entp->delete_flag = false;
    memcpy((char *)entp->annotation, (char *)xentry->annotation, 
        sizeof(entp->annotation));

    db_update_entry(h, entp, status);
}        

/*  epdb_delete_entry
 *  Delete an entry from the database.
 *
 *  If any readers have unlocked the db but 
 *  are pointing to this entry, mark the entry 
 *  as deleted and write it to disk.
 *  Otherwise, remove the entry from all 
 *  lists and delete it from the database
 */
PRIVATE void epdb_delete_entry(h, entp, status)
struct db       *h;
db_entry_p_t    entp;
error_status_t  *status;
{
    if (entp->read_nrefs == 0)
    {
        db_lists_remove(h, entp);
        rpc__naf_addr_free(&entp->addr, status);
        dsm_free(h->dsh, (void *) entp, status);
    } 
    else
    {
        entp->delete_flag = true;
        db_update_entry(h, entp, status);
    }
}

/*
 *  epdb_is_replace_candidate
 *
 *  Return true iff the specified entry looks like a candidate for
 *  replacement.  Requirements for candidacy are: matching object, matching
 *  i/f UUID and major version, matching RPC protseq, matching data rep,
 *  matching RPC protocol ID and RPC protocol major and minor version.
 *  Candidacy is not sufficient to determine actual replaceability; i/f
 *  minor version and network address must be considered as well.  See
 *  below.
 */
INTERNAL boolean32 epdb_is_replace_candidate(entp, object, tfp, addr)
twr_fields_p_t  tfp;
dce_uuid_p_t        object;
db_entry_t      *entp;
rpc_addr_p_t    addr;
{
    error_status_t      tmp_st;
    unsigned32          data_rep_vers_major,
                        data_rep_vers_minor;

    data_rep_vers_major = RPC_IF_VERS_MAJOR(tfp->data_rep.version);
    data_rep_vers_minor = RPC_IF_VERS_MINOR(tfp->data_rep.version);

    return (dce_uuid_equal(object, &entp->object, &tmp_st) &&
            dce_uuid_equal(&tfp->interface.uuid, &entp->interface.uuid, &tmp_st) &&
            (tfp->interface.vers_major == entp->interface.vers_major) && 
            (addr->rpc_protseq_id == entp->addr->rpc_protseq_id) &&
            dce_uuid_equal(&tfp->data_rep.id, &entp->data_rep_id, &tmp_st) && 
            (data_rep_vers_major == entp->data_rep_vers_major) &&
            (data_rep_vers_minor == entp->data_rep_vers_minor) &&
            (tfp->rpc_protocol == entp->rpc_protocol) &&
            (tfp->rpc_protocol_vers_major == entp->rpc_protocol_vers_major) &&
            (tfp->rpc_protocol_vers_minor == entp->rpc_protocol_vers_minor));
}

/*
 *  epdb_delete_replaceable_entries
 *
 *  Delete entries which are "replaceable" by the new entry implied by
 *  the object, tfp, and addr parameters.
 */
INTERNAL void epdb_delete_replaceable_entries(h, object, tfp, addr, status)
struct db       *h;
dce_uuid_p_t        object;
twr_fields_p_t  tfp;
rpc_addr_p_t    addr;
error_status_t  *status;
{
    unsigned_char_p_t   netaddr,
                        netaddr2;
    db_lists_t          *lp,
                        *lp_first,
                        *lp_next;
    db_list_type_t      list_type;
    error_status_t      tmp_st;

    rpc__naf_addr_inq_netaddr(addr, &netaddr, status);
    if (! STATUS_OK(status)) 
    {
        SET_STATUS(status, ept_s_invalid_entry);
        return;
    }

    if (! dce_uuid_is_nil(object, &tmp_st))
    { 
        list_type = db_c_object_list;
        lp_first = db_list_first(&h->lists_mgmt, db_c_object_list, object);
    }
    else
    {
        list_type = db_c_interface_list;
        lp_first = db_list_first(&h->lists_mgmt, db_c_interface_list, &tfp->interface.uuid);
    } 

    /*
     *  Scan the list and see if there are any replace candidates with an
     *  i/f minor version that's greater than the proposed new entry.  If
     *  there are, the replace must fail.
     */
    for (lp = lp_first; lp != NULL; lp = db_list_next(list_type, lp))
    {
        db_entry_t *entp = (db_entry_t *) lp;

        if (entp->delete_flag) continue;

        if (epdb_is_replace_candidate(entp, object, tfp, addr) &&
            entp->interface.vers_minor > tfp->interface.vers_minor)
        {
            SET_STATUS(status, ept_s_invalid_entry);
            goto DONE;
        }
    }

    /*
     *  Scan the list again and delete all replace candidates that have
     *  network address that matches the network entry of the new entry.
     */
    for (lp = lp_first; lp != NULL; lp = lp_next)
    {
        db_entry_t *entp = (db_entry_t *) lp;

        /*
         *  Point to next entry - may remove this one from the list
         *  and delete it
         */
        lp_next = db_list_next(list_type, lp);


        if (entp->delete_flag) continue;

        if (epdb_is_replace_candidate(entp, object, tfp, addr))
        {
            rpc__naf_addr_inq_netaddr(entp->addr, &netaddr2, &tmp_st);
            if (tmp_st != rpc_s_ok) continue;

            if (strcmp((char *) netaddr, (char *) netaddr2) == 0)
            {
                /*  Entry matches target in all fields except 
                 *  endpoint so delete it
                 */
                epdb_delete_entry(h, entp, &tmp_st);
            }

            rpc_string_free(&netaddr2, &tmp_st);
        }
    }

    SET_STATUS_OK(status);

DONE:

    rpc_string_free(&netaddr, &tmp_st);

}

/*
 *  epdb_delete_entries_by_obj_if_addr
 *
 *  Delete entries which match target object (if speced), interface, and addr
 */
INTERNAL void epdb_delete_entries_by_obj_if_addr(h, object_speced, object, interface, addr, status)
struct db           *h;
boolean32           object_speced;
dce_uuid_p_t            object;
rpc_if_id_p_t       interface;
rpc_addr_p_t        addr;
error_status_t      *status;
{
    unsigned32      ndelete;
    db_lists_t      *lp,
                    *lp_next;
    db_entry_t      *entp;
    error_status_t  tmp_st;

    ndelete = 0;

    for (lp = db_list_first(&h->lists_mgmt, db_c_interface_list, &interface->uuid);
            lp != NULL; lp = lp_next)
    {
        /*
         *  Point to next entry - may remove this one from the list
         *  and delete it
         */
        lp_next = db_list_next(db_c_interface_list, lp);

        entp = (db_entry_t *) lp;

        if (entp->delete_flag) continue;

        if (((! object_speced) || dce_uuid_equal(object, &entp->object, &tmp_st)) &&
            (dce_uuid_equal(&interface->uuid, &entp->interface.uuid, &tmp_st)) &&
            (interface->vers_major == entp->interface.vers_major) &&
            (interface->vers_minor == entp->interface.vers_minor) &&
            (rpc__naf_addr_compare(addr, entp->addr, &tmp_st)))
        {
            /*  Entry matches target object (if speced),
             *  interface, and addr so delete it
             */
            epdb_delete_entry(h, entp, status);
            if (! STATUS_OK(status)) return;

            ndelete++;
        }
    }

    if (ndelete > 0)
        SET_STATUS_OK(status);
    else
        SET_STATUS(status, ept_s_not_registered);
}

/*  epdb_lookup_entry
 *  Return pointer to entry which exactly matches
 *  xentry
 *  Return all entries, even ones marked as deleted
 */
INTERNAL db_entry_t *epdb_lookup_entry(h, xentry)
struct db       *h;
ept_entry_p_t   xentry;
{ 
    db_entry_t      *entp;
    db_lists_t      *lp;
    db_list_type_t  list_type;
    rpc_if_id_t     interface;
    error_status_t  tmp_st;

    if (! dce_uuid_is_nil(&xentry->object, &tmp_st))
    { 
        list_type = db_c_object_list;
        lp = db_list_first(&h->lists_mgmt, db_c_object_list, &xentry->object);
    }
    else
    {
        tower_to_if_id(xentry->tower, &interface, &tmp_st);
        list_type = db_c_interface_list;
        lp = db_list_first(&h->lists_mgmt, db_c_interface_list, &interface.uuid);
    } 

    for ( ; lp != NULL; lp = db_list_next(list_type, lp))
    {
        entp = (db_entry_t *) lp;

        if (dce_uuid_equal(&xentry->object, &entp->object, &tmp_st) &&
            (xentry->tower->tower_length == entp->tower.tower_length) &&
            (memcmp((char *) xentry->tower->tower_octet_string, 
                    (char *) entp->tower.tower_octet_string, 
                    xentry->tower->tower_length) == 0))
        {
            return(entp);
        }
    }

    return(NULL);
}




/*
 * Add a new entry to the database.  If the entry already exists simply
 * modify its annotation field (it's the only field that might be
 * different), clear some of its mgmt fields and update the entry
 * on disk.
 *
 * Note:  All tower floors in an entry must be valid and non-nil
 * (eg. nil interface uuid is prohibited).
 */
PRIVATE void epdb_insert(h_, xentry, replace, status)
epdb_handle_t   h_;
ept_entry_p_t   xentry;
boolean32       replace;
error_status_t  *status;
{
    struct db               *h = (struct db *) h_;
    db_entry_t              *entp;
    twr_fields_t            twr_fields;
    rpc_addr_p_t            addr;
    error_status_t          tmp_st;

    addr = NULL;

    /*  Parse the new entry's tower and check 
     *  whether the new entry is ok.
     *  Prefill status to bad entry for quick return.
     */
    SET_STATUS(status, ept_s_invalid_entry);

    /*  
     * Parse xentry's tower into twr_fields
     */
    tower_to_fields(xentry->tower, &twr_fields, &tmp_st);
    if (tmp_st != rpc_s_ok) return;

    tower_to_addr(xentry->tower, &addr, &tmp_st);
    if (tmp_st != rpc_s_ok) return;

    epdb_chk_entry(xentry, &twr_fields, addr, &tmp_st);
    if (tmp_st != rpc_s_ok)
    {
        if (addr != NULL) rpc__naf_addr_free(&addr, &tmp_st);
        return;
    }

    db_lock(h);

    if (replace)
    {
        /*  Delete entries that match
         *  in all fields except endpoint
         */
        epdb_delete_replaceable_entries(h, &xentry->object, &twr_fields, addr, status);
        if (! STATUS_OK(status)) 
        {
            rpc__naf_addr_free(&addr, &tmp_st);
            db_unlock(h);
            return;
        }
    }

    entp = epdb_lookup_entry(h, xentry);

    if (entp == NULL)
    { 
        /*  New entry
         *  insert it in dbase and add it to lists
         */
        epdb_insert_entry(h, xentry, &twr_fields, addr, &entp, status);
    }
    else
    {
        /*  Existing entry - just replace annotation and clear some mgmt fields
         *  Free unused addr (already have it for this entry)
         */
        rpc__naf_addr_free(&addr, &tmp_st);

        epdb_replace_entry(h, xentry, entp, status);
    }

    db_unlock(h);

    if (! STATUS_OK(status))
    {
        if (addr != NULL) rpc__naf_addr_free(&addr, &tmp_st);
        db_to_ept_ecode(status);
    }
}

/*
 * Remove an entry from the database.
 * Only a database entry which exactly matches
 * xentry is deleted.
 */
PRIVATE void epdb_delete(h_, xentry, status)
epdb_handle_t   h_;
ept_entry_p_t   xentry;
error_status_t  *status;
{
    struct db       *h = (struct db *) h_;
    db_entry_t      *entp;

    SET_STATUS_OK(status);

    db_lock(h);

    entp = epdb_lookup_entry(h, xentry);

    if (entp != NULL)
    {
        if (entp->delete_flag)
            SET_STATUS(status, ept_s_not_registered);
        else
        {
        /*  
         *  Matching entry found so delete it
         */
        epdb_delete_entry(h, entp, status);
        if (! STATUS_OK(status)) db_to_ept_ecode(status);
    }
    }
    else
        SET_STATUS(status, ept_s_not_registered);

    db_unlock(h);
}

PRIVATE void epdb_mgmt_delete(h_, object_speced, object, tower, status)
epdb_handle_t       h_;
boolean32           object_speced;
dce_uuid_p_t            object;
twr_p_t             tower;
error_status_t      *status;
{
    struct db       *h = (struct db *) h_; 
    rpc_if_id_t     interface;
    rpc_addr_p_t    addr;
    error_status_t  tmp_st;


    SET_STATUS(status, ept_s_invalid_entry);

    tower_to_if_id(tower, &interface, status);
    if (! STATUS_OK(status)) return;
    tower_to_addr(tower, &addr, status);
    if (! STATUS_OK(status)) return;

    db_lock(h);

    epdb_delete_entries_by_obj_if_addr(h, object_speced, object, &interface, addr, status);
    if (! STATUS_OK(status)) db_to_ept_ecode(status);

    db_unlock(h);

    rpc__naf_addr_free(&addr, &tmp_st);
}



/*
 *  epdb_lookup
 *  Return entries that match filter speced by inquiry_type, vers_option,
 *  object, and interface.
 * 
 */
PRIVATE void epdb_lookup(h_, inquiry_type, object, interface, vers_option, entry_handle, max_ents, 
    num_ents, entries, status)
epdb_handle_t       h_;
unsigned32          inquiry_type;
dce_uuid_p_t            object;
rpc_if_id_p_t       interface;
unsigned32          vers_option;
ept_lookup_handle_t *entry_handle;
unsigned32          max_ents;
unsigned32          *num_ents;
ept_entry_t         entries[];
error_status_t      *status;
{
    struct db       *h = (struct db *) h_;

    *num_ents = 0;

    /* lock database before delete_context or lookup
     */
    db_lock(h);

    if (entries == NULL)
    {
        db_delete_context(h, entry_handle);
        SET_STATUS(status, ept_s_invalid_entry);
        db_unlock(h);
        return;
    }

    lookup(h, inquiry_type, object, interface, vers_option, entry_handle, max_ents, num_ents, entries, status);

    db_unlock(h);
}

INTERNAL void lookup(h, inquiry_type, object, interface, vers_option, entry_handle, max_ents, 
    num_ents, entries, status)
struct db           *h;
unsigned32          inquiry_type;
dce_uuid_p_t            object;
rpc_if_id_p_t       interface;
unsigned32          vers_option;
ept_lookup_handle_t *entry_handle;
unsigned32          max_ents;
unsigned32          *num_ents;
ept_entry_t         entries[];
error_status_t      *status;
{
    unsigned32      pass;
    db_list_type_t  list_type;
    db_lists_t      *lp;
    unsigned32      i;

    SET_STATUS_OK(status);

    if ((entry_handle == NULL) || (*entry_handle == NULL))
    {
        /*  no context saved so init context
         */
        switch ((int)inquiry_type)
        {
            case rpc_c_ep_all_elts:
                pass = 1;
                list_type = db_c_entry_list;
                lp = db_list_first(&h->lists_mgmt, list_type, NULL);
                break;

            case rpc_c_ep_match_by_if:
            case rpc_c_ep_match_by_both:
                pass = 1;
                list_type = db_c_interface_list;
                lp = db_list_first(&h->lists_mgmt, list_type, &interface->uuid);
                break;

            case rpc_c_ep_match_by_obj:
                pass = 1;
                list_type = db_c_object_list;
                lp = db_list_first(&h->lists_mgmt, list_type, object);
                break;

            default:
                *status = rpc_s_invalid_inquiry_type;
                return;
        }
    }
    else
    {
        /*  context has been saved.
         *  restore context
         */
        db_get_context(h, entry_handle, &list_type, &lp, &pass, status);
        if (! STATUS_OK(status)) 
        {
            *entry_handle = NULL;
            return;
        }

        /*  do some sanity checking on inquiry_type/context info
         */
        switch ((int)inquiry_type)
        {
            case rpc_c_ep_all_elts:
                if (list_type != db_c_entry_list)
                    *status = ept_s_invalid_context;
                break;

            case rpc_c_ep_match_by_if:
            case rpc_c_ep_match_by_both:
                if (list_type != db_c_interface_list)
                    *status = ept_s_invalid_context;
                break;

            case rpc_c_ep_match_by_obj:
                if (list_type != db_c_object_list)
                    *status = ept_s_invalid_context;
                break;

            default:
                *status = rpc_s_invalid_inquiry_type;
                break;
        }

        if (! STATUS_OK(status))
        {
            db_delete_context(h, entry_handle);
            return;
        }
    }

    lookup_match(inquiry_type, object, interface, vers_option, max_ents, num_ents, entries, list_type, 
        &lp, status);

    if (! STATUS_OK(status))
    {
        /*  Lookup failed so delete
         *  the context handle and free
         *  the towers that have been allocated
         */
        db_delete_context(h, entry_handle);

        for (i = 0; i < *num_ents; i++)
            rpc_ss_free(entries[i].tower);

        *num_ents = 0;
        return;
    }

    db_save_context(h, entry_handle, list_type, lp, pass);

    if (*num_ents == 0)
    {
        SET_STATUS(status, ept_s_not_registered);
    }
}

INTERNAL void lookup_match(inquiry_type, object, interface, vers_option, max_ents, num_ents, entries, list_type, 
    lpp, status)
unsigned32          inquiry_type;
dce_uuid_p_t            object;
rpc_if_id_p_t       interface;
unsigned32          vers_option;
unsigned32          max_ents;
unsigned32          *num_ents;
ept_entry_t         entries[];
unsigned32          list_type;
db_lists_t          **lpp;
error_status_t      *status;
{
    boolean32       match;
    db_lists_t      *lp;
    db_entry_t      *entp;
    error_status_t  tmp_st;

    for (lp = *lpp; lp != NULL; lp = db_list_next(list_type, lp))
    {
        entp = (db_entry_t *) lp;

        if (entp->delete_flag) continue;
                                        
        match = false;
        switch ((int)inquiry_type)
        {
            case rpc_c_ep_all_elts:
                match = true;
                break;

            case rpc_c_ep_match_by_if:
                if (dce_uuid_equal(&interface->uuid, &entp->interface.uuid, &tmp_st))
                    match = true;
                break;

            case rpc_c_ep_match_by_obj:
                if (dce_uuid_equal(object, &entp->object, &tmp_st))
                    match = true;
                break;

            case rpc_c_ep_match_by_both:
                if (dce_uuid_equal(&interface->uuid, &entp->interface.uuid, &tmp_st) &&
                    dce_uuid_equal(object, &entp->object, &tmp_st))
                    match = true;
                break;

            default:
                *status = rpc_s_invalid_inquiry_type;
                return;

        }

        if (match)
        {
            if ((inquiry_type == rpc_c_ep_match_by_if) || (inquiry_type == rpc_c_ep_match_by_both))
            {
                /* check interface version 
                 */

                match = false;
                switch ((int)vers_option)
                {
                    case rpc_c_vers_all:
                        match = true;
                        break;

                    case rpc_c_vers_compatible:
                        if ((interface->vers_major == entp->interface.vers_major) && 
                            (interface->vers_minor <= entp->interface.vers_minor))
                            match = true;
                        break;

                    case rpc_c_vers_exact:
                        if ((interface->vers_major == entp->interface.vers_major) && 
                            (interface->vers_minor == entp->interface.vers_minor))
                            match = true;
                        break;

                    case rpc_c_vers_major_only:
                        if (interface->vers_major == entp->interface.vers_major)
                            match = true;
                        break;

                    case rpc_c_vers_upto:
                        if (interface->vers_major > entp->interface.vers_major)
                            match = true;
                        else
                        if ((interface->vers_major == entp->interface.vers_major) && 
                            (interface->vers_minor >= entp->interface.vers_minor))
                            match = true;
                        break;

                    default:
                        *status = rpc_s_invalid_vers_option;
                        return;
                }
            }
        }

        if (match)
        {   
            if (*num_ents >= max_ents) 
            {
                break;
            }

            epdb_to_ept(entp, &entries[*num_ents], status);
            if (! STATUS_OK(status)) return;

            (*num_ents)++;
        }
    }

    *lpp = lp;

    SET_STATUS_OK(status);
}

/* 
 *  epdb_fwd
 *
 *  Invoke map or map_mgmt to do a sequence of searches through the 
 *  endpoint map for appropriate entries to forward to and return pointers 
 *  to these entries.  Copy the entries' addr info into fwd_addrs.
 *
 *  map_mgmt is used for the search if the interface is the mgmt interface,
 *  otherwise map is used.
 *
 *  If bad status is returned delete the search context, otherwise map
 *  will save it.
 *
 * NB: caller should set *num_ents to 0 or appropriate offset into fwd_addrs.
 *
 * Note: we do not forward to an entry with a nil interface id
 * AND a nil object id.  If this ever becomes desireable, it will
 * likely require visiting (fixing) the protocol service implementations
 * that use DG style forwarding.  The implementations may try to forward
 * first through the map and only if the forward map fails, then attempt
 * to perform normal pkt handling.  The problem is that allowing a tuple
 * with a nil interface AND object id to be registered will result in
 * an inexact lookup match (due to the nil interface id) for operations
 * that are really for the server implementing the endpoint map operations.
 *
 * Also note that rpcd.c/fwd_map invokes epdb_fwd with max_ents = 1 and 
 * map_handle = NULL.  We could remove the context handle mgmt code from
 * epdb_fwd and gain some in performance - but we'd still need to leave 
 * the context handle code in map and map_match for epdb_map.
 */

PRIVATE void epdb_fwd(h_, object, interface, data_rep, 
                        rpc_protocol, rpc_protocol_vers_major, rpc_protocol_vers_minor,
                        addr, map_handle, max_ents, num_ents, fwd_addrs, status)
epdb_handle_t           h_;
dce_uuid_p_t                object;
rpc_if_id_p_t           interface;
rpc_syntax_id_p_t       data_rep;
rpc_protocol_id_t       rpc_protocol;
unsigned32              rpc_protocol_vers_major;
unsigned32              rpc_protocol_vers_minor;
rpc_addr_p_t            addr;
ept_lookup_handle_t     *map_handle;
unsigned32              max_ents;
unsigned32              *num_ents;
rpc_addr_p_t            fwd_addrs[];
unsigned32              *status;
{
    struct db       *h = (struct db *) h_;
    rpc_if_rep_p_t  mgmt_if_rep;
    db_entry_t      **db_entries;
    unsigned32      start_ent;
    unsigned32      i;
    error_status_t  tmp_st;

    mgmt_if_rep = (rpc_if_rep_p_t) mgmt_v1_0_s_ifspec;

    if (db_different_context(h, map_handle, status))
        return;

    db_entries = (db_entry_t **) malloc(max_ents * sizeof(db_entry_p_t));

    /* lock database before delete_context or lookup
     */
    db_lock(h);

    if ((fwd_addrs == NULL) || (db_entries == NULL) || (max_ents == 0) || (*num_ents > max_ents))
    {
        if (db_entries != NULL) free(db_entries);
        db_delete_context(h, map_handle);
        SET_STATUS(status, ept_s_cant_perform_op);
        db_unlock(h);
        return;
    }

    start_ent = *num_ents;

    if (dce_uuid_equal(&interface->uuid, &mgmt_if_rep->id, &tmp_st))
        map_mgmt(h, object, data_rep, 
        rpc_protocol, rpc_protocol_vers_major, rpc_protocol_vers_minor, addr->rpc_protseq_id,
        map_handle, max_ents, num_ents, db_entries, status); 
    else
        map(h, object, interface, data_rep, 
        rpc_protocol, rpc_protocol_vers_major, rpc_protocol_vers_minor, addr->rpc_protseq_id,
        map_handle, max_ents, num_ents, db_entries, status);
    if (! STATUS_OK(status)) 
    {
        free(db_entries);
        db_unlock(h);
        return;
    }

    for (i = start_ent; i < *num_ents; i++) 
    {
        rpc__naf_addr_copy(db_entries[i]->addr, &fwd_addrs[i], status);
        if (! STATUS_OK(status))
        {
            /*  don't remember context but return
             *  addrs that already copied to oput array.
             */
            db_delete_context(h, map_handle);
            *num_ents = i;
            if (*num_ents > 0) 
            {
                SET_STATUS_OK(status);
            }
            else
            {
                SET_STATUS(status, ept_s_cant_perform_op);
            }
            break;
        }
    }

    free(db_entries);

    db_unlock(h);
}

/*  epdb_map
 *  Invoke map or map_mgmt to do a sequence of searches through the 
 *  endpoint map for appropriate entries to forward to and return pointers 
 *  to these entries.  Copy the entries' tower info into fwd_towers.
 *
 *  map_mgmt is used for the search if the interface is the mgmt interface,
 *  otherwise map is used.
 *
 *  If bad status is returned delete the search context, otherwise map
 *  will save it.
 *
 *  NB: caller should set *num_ents to 0 or appropriate offset into fwd_towers.
 */
PRIVATE void epdb_map(h_, object, map_tower, map_handle, max_ents, num_ents, fwd_towers, status)
epdb_handle_t       h_;
dce_uuid_p_t            object;
twr_p_t             map_tower;
ept_lookup_handle_t *map_handle;
unsigned32          max_ents;
unsigned32          *num_ents;
twr_t               *fwd_towers[];
unsigned32          *status;
{
    struct db       *h = (struct db *) h_;
    twr_fields_t    twr_fields, *tfp;
    rpc_if_rep_p_t  mgmt_if_rep;
    db_entry_t      **db_entries;
    unsigned32      start_ent;
    unsigned32      i;
    error_status_t  tmp_st;

    tfp = &twr_fields;
    mgmt_if_rep = (rpc_if_rep_p_t) mgmt_v1_0_s_ifspec;

    if (db_different_context(h, map_handle, status))
        return;

    /*  lock database before delete_context or map
     */
    db_lock(h);

    tower_to_fields(map_tower, &twr_fields, status);
    if (! STATUS_OK(status)) 
    {
        db_delete_context(h, map_handle);
        SET_STATUS(status, ept_s_invalid_entry);
        db_unlock(h);
        return;
    } 

    epdb_chk_map_entry(&twr_fields, status);
    if (! STATUS_OK(status)) 
    {
        db_delete_context(h, map_handle);
        SET_STATUS(status, ept_s_invalid_entry);
        db_unlock(h);
        return;
    } 

    db_entries = (db_entry_t **) malloc(max_ents * sizeof(db_entry_p_t));

    if ((db_entries == NULL) || (fwd_towers == NULL) || (max_ents == 0) || (*num_ents > max_ents))
    {
        if (db_entries != NULL) free(db_entries);
        db_delete_context(h, map_handle);
        SET_STATUS(status, ept_s_cant_perform_op);
        db_unlock(h);
        return;
    }

    start_ent = *num_ents;

    if (dce_uuid_equal(&tfp->interface.uuid, &mgmt_if_rep->id, &tmp_st))
        map_mgmt(h, object, &tfp->data_rep, 
        tfp->rpc_protocol, tfp->rpc_protocol_vers_major, tfp->rpc_protocol_vers_minor, 
        tfp->protseq, map_handle, max_ents, num_ents, db_entries, status);
    else
        map(h, object, &tfp->interface, &tfp->data_rep, 
        tfp->rpc_protocol, tfp->rpc_protocol_vers_major, tfp->rpc_protocol_vers_minor, 
        tfp->protseq, map_handle, max_ents, num_ents, db_entries, status);
    if (! STATUS_OK(status)) 
    {
        free(db_entries);
        db_unlock(h);
        return;
    }

    for (i = start_ent; i < *num_ents; i++) 
    {
        tower_ss_copy(&(db_entries[i]->tower), &fwd_towers[i], status);
        if (! STATUS_OK(status))
        {
            /*  don't remember context but return
             *  towers that already copied to oput array.
             */
            db_delete_context(h, map_handle);
            *num_ents = i;
            if (*num_ents > 0) 
            {
                SET_STATUS_OK(status);
            }
            else
            {
                SET_STATUS(status, ept_s_cant_perform_op);
            }
            break;
        }
    }

    free(db_entries);

    db_unlock(h);
}

/*
 *  Search the endpoint database looking for entries which match the target object, 
 *  interface, etc.
 *
 *  If the target object is non-nil, do a 2 pass search
 *  First, search the objectq looking for exact object/interface matches.
 *  Second, search the interfaceq for interface match/object nil
 *
 *  If the target object is nil
 *  Search the interfaceq for interface match/object nil (same as pass 2 above)
 *
 *  The following table gives some examples of applying the above algorithm
 *  to various map entrys (the table assumes that the entrys already exactly
 *  match the data rep, protseq, versions, etc. specified in the key):
 *
 *  Keys and entrys of the form:    <object,interface>
 *  Matches (in order of specificity):
 *          Moi     - Match on Object and Interface
 *          Mi      - Match on Interface
 *  No Match:  ' - '
 *
 *
 *   \   Map   |                                  
 *     \ entry |                                  
 *  key  \     | <nil,I1> <nil,I2> <O1,I1> <O1,I2> <O2,I1> 
 *  -----------|-------------------------------------------
 *  <nil,I1>   |   Moi       -        -       -      -     
 *  <O1,I1>    |   Mi        -       Moi      -      -     
 *
 */

INTERNAL void map(h, object, interface, data_rep, 
        rpc_protocol, rpc_protocol_vers_major, rpc_protocol_vers_minor, protseq,
        map_handle, max_ents, n_ents, db_entries, status)
struct db               *h;
dce_uuid_p_t                object;
rpc_if_id_p_t           interface;
rpc_syntax_id_p_t       data_rep;
rpc_protocol_id_t       rpc_protocol;
unsigned32              rpc_protocol_vers_major;
unsigned32              rpc_protocol_vers_minor;
rpc_protseq_id_t        protseq;
ept_lookup_handle_t     *map_handle;
unsigned32              max_ents;
unsigned32              *n_ents;
db_entry_t              *db_entries[];
unsigned32              *status;
{
    unsigned32      pass;
    db_list_type_t  list_type;
    db_lists_t      *lp;
    error_status_t  tmp_st;

    SET_STATUS_OK(status);

    if ((map_handle == NULL) || (*map_handle == NULL))
    {
        if (dce_uuid_is_nil(object, &tmp_st))
        {
            pass = 2;
            list_type = db_c_interface_list;
            lp = db_list_first(&h->lists_mgmt, list_type, &interface->uuid);
        }
        else
        {
            pass = 1;
            list_type = db_c_object_list;
            lp = db_list_first(&h->lists_mgmt, list_type, object);
        }
    }
    else 
    {
        db_get_context(h, map_handle, &list_type, &lp, &pass, status);
        if (! STATUS_OK(status)) return;
    }

    /*  search objectq for object/interface match */
    if (pass == 1)
    {
        map_match(
            object, interface, data_rep, 
            rpc_protocol, rpc_protocol_vers_major, rpc_protocol_vers_minor, protseq,
            max_ents, n_ents, db_entries, map_handle, pass, list_type, &lp, status);
        if (! STATUS_OK(status))
        {
            db_delete_context(h, map_handle);
            return;
        }
    
        if ((*n_ents >= max_ents) && ((lp != NULL) || (map_handle == NULL)))
        {
            /*  If entry buffer is full and
             *  have found next match or are not saving context
             *  save context and return 
             *  (save_context just returns if map_handle == NULL)
             */
            db_save_context(h, map_handle, list_type, lp, pass);
            return;
        }

        /*  get organized for next pass
         */ 
        pass = 2;
        list_type = db_c_interface_list;
        lp = db_list_first(&h->lists_mgmt, list_type, &interface->uuid);

    }   /* end of pass 1 */

    /*  search interfaceq for interface match/object nil */
    if (pass == 2)
    {
        map_match(
            &nil_uuid, interface, data_rep, 
            rpc_protocol, rpc_protocol_vers_major, rpc_protocol_vers_minor, protseq,
            max_ents, n_ents, db_entries, map_handle, pass, list_type, &lp, status);
        if (! STATUS_OK(status))
        {
            db_delete_context(h, map_handle);
            return;
        }
    }

    /*  If there's context to be saved, save it.
     *  If there's no context to save, free entry_handle's
     *  context and set entry_handle to NULL
     */
    db_save_context(h, map_handle, list_type, lp, pass);

    if (*n_ents == 0) 
    {
        SET_STATUS(status, ept_s_not_registered);
    }
}

INTERNAL void map_match(
        object, interface, data_rep, 
        rpc_protocol, rpc_protocol_vers_major, rpc_protocol_vers_minor, protseq,
        max_ents, n_ents, entries, map_handle, pass, list_type, lpp, status)
dce_uuid_p_t                object;
rpc_if_id_p_t           interface;
rpc_syntax_id_p_t       data_rep;
rpc_protocol_id_t       rpc_protocol;
unsigned32              rpc_protocol_vers_major;
unsigned32              rpc_protocol_vers_minor ATTRIBUTE_UNUSED;
rpc_protseq_id_t        protseq;
unsigned32              max_ents;
unsigned32              *n_ents;
db_entry_t              *entries[];
ept_lookup_handle_t     *map_handle;
unsigned32              pass ATTRIBUTE_UNUSED;
db_list_type_t          list_type;
db_lists_t              **lpp;
unsigned32              *status;
{
    boolean32       object_nil;
    unsigned32      data_rep_vers_major,
                    data_rep_vers_minor;
    unsigned32      tmp_st;
    db_lists_t      *lp;
    db_entry_t      *entp;

    SET_STATUS_OK(status);

    object_nil = dce_uuid_is_nil(object, &tmp_st);
    data_rep_vers_major = RPC_IF_VERS_MAJOR(data_rep->version);
    data_rep_vers_minor = RPC_IF_VERS_MINOR(data_rep->version);

    for (lp = *lpp; lp != NULL; lp = db_list_next(list_type, lp))
    {
        entp = (db_entry_t *) lp;

        if (entp->delete_flag) continue;

        if  (((object_nil && entp->object_nil) || 
                dce_uuid_equal(object, &entp->object, &tmp_st)) && 
            dce_uuid_equal(&interface->uuid, &entp->interface.uuid, &tmp_st) &&
            (interface->vers_major == entp->interface.vers_major) && 
            (interface->vers_minor <= entp->interface.vers_minor) &&
            (protseq == entp->addr->rpc_protseq_id) &&
            dce_uuid_equal(&data_rep->id, &entp->data_rep_id, &tmp_st) && 
            (data_rep_vers_major == entp->data_rep_vers_major) &&
            (data_rep_vers_minor <= entp->data_rep_vers_minor) &&
            (rpc_protocol == entp->rpc_protocol) &&
            (rpc_protocol_vers_major == entp->rpc_protocol_vers_major))
            /*
             * We dont do this so we can rev the minor protocol number
             * && (rpc_protocol_vers_minor <= entp->rpc_protocol_vers_minor) )
             */
        {   
            /*
             *  found the next match and have filled the
             *  entries vector so quit search
             */
            if (*n_ents >= max_ents) 
            {
                *lpp = lp;
                return;
            }

            entries[*n_ents] = entp;

            (*n_ents)++;

            /* if not saving search context don't look for next matching
             * entry 
             */
            if ((map_handle == NULL) && (*n_ents >= max_ents)) 
            {
                *lpp = lp;
                return;
            }
        } /* entry == target */

    } /* for lp */

    *lpp = lp;
}

/*  map_mgmt
 *  Packet is for the mgmt interface which all processes export. 
 *  Look for entries with matching object, protseq, data_rep,
 *  and rpc_protocol.  Only return one entry per endpoint.
 */
INTERNAL void map_mgmt(h, object, data_rep, 
        rpc_protocol, rpc_protocol_vers_major, rpc_protocol_vers_minor, protseq,
        map_handle, max_ents, n_ents, db_entries, status)
struct db               *h;
dce_uuid_p_t                object;
rpc_syntax_id_p_t       data_rep;
rpc_protocol_id_t       rpc_protocol;
unsigned32              rpc_protocol_vers_major;
unsigned32              rpc_protocol_vers_minor;
rpc_protseq_id_t        protseq;
ept_lookup_handle_t     *map_handle;
unsigned32              max_ents;
unsigned32              *n_ents;
db_entry_t              *db_entries[];
unsigned32              *status;
{
    unsigned32      pass;
    db_list_type_t  list_type;
    db_lists_t      *lp;
    error_status_t  tmp_st;

    if (dce_uuid_is_nil(object, &tmp_st))
    {
        db_delete_context(h, map_handle);
        SET_STATUS(status, ept_s_invalid_entry);
        return;
    }

    if ((map_handle == NULL) || (*map_handle == NULL))
    {
        pass = 1;
        list_type = db_c_object_list;
        lp = db_list_first(&h->lists_mgmt, list_type, object);
    }
    else 
    {
        db_get_context(h, map_handle, &list_type, &lp, &pass, status);
        if (! STATUS_OK(status)) return;
    }

    map_mgmt_match(
        object, data_rep, 
        rpc_protocol, rpc_protocol_vers_major, rpc_protocol_vers_minor, protseq,
        max_ents, n_ents, db_entries, map_handle, pass, list_type, &lp, status);
    if (! STATUS_OK(status))
    {
        db_delete_context(h, map_handle);
        return;
    }

    /*  If there's context to be saved, save it.
     *  If there's no context to save, free map_handle's
     *  context and set map_handle to NULL
     */
    db_save_context(h, map_handle, list_type, lp, pass);

    if (*n_ents == 0) 
    {
        SET_STATUS(status, ept_s_not_registered);
    }
}        

INTERNAL void map_mgmt_match(
        object, data_rep, 
        rpc_protocol, rpc_protocol_vers_major, rpc_protocol_vers_minor, protseq,
        max_ents, n_ents, entries, map_handle, pass, list_type, lpp, status)
dce_uuid_p_t                object;
rpc_syntax_id_p_t       data_rep;
rpc_protocol_id_t       rpc_protocol;
unsigned32              rpc_protocol_vers_major;
unsigned32              rpc_protocol_vers_minor ATTRIBUTE_UNUSED;
rpc_protseq_id_t        protseq;
unsigned32              max_ents;
unsigned32              *n_ents;
db_entry_t              *entries[];
ept_lookup_handle_t     *map_handle;
unsigned32              pass ATTRIBUTE_UNUSED;
db_list_type_t          list_type;
db_lists_t              **lpp;
unsigned32              *status;
{
    unsigned32      data_rep_vers_major,
                    data_rep_vers_minor;
    unsigned32      tmp_st;
    db_lists_t      *lp;
    db_entry_t      *entp;

    SET_STATUS_OK(status);

    data_rep_vers_major = RPC_IF_VERS_MAJOR(data_rep->version);
    data_rep_vers_minor = RPC_IF_VERS_MINOR(data_rep->version);

    for (lp = *lpp; lp != NULL; lp = db_list_next(list_type, lp))
    {
        entp = (db_entry_t *) lp;

        if (entp->delete_flag) continue;

        if (dce_uuid_equal(object, &entp->object, &tmp_st) && 
            (protseq == entp->addr->rpc_protseq_id) &&
            dce_uuid_equal(&data_rep->id, &entp->data_rep_id, &tmp_st) && 
            (data_rep_vers_major == entp->data_rep_vers_major) &&
            (data_rep_vers_minor <= entp->data_rep_vers_minor) &&
            (rpc_protocol == entp->rpc_protocol) &&
            (rpc_protocol_vers_major == entp->rpc_protocol_vers_major) &&
            /*
             * We dont do this so we can rev the minor protocol number
             * (rpc_protocol_vers_minor <= entp->rpc_protocol_vers_minor) &&
             */
            /*  match - see whether already returning
             *  this endpoint in entries
             */
            map_mgmt_endpt_unique(entp->addr, *n_ents, entries) )
        {   
            /*
             *  found the next match with unique endpoint 
             */

            /*
             *  have filled the entries vector so quit search
             */
            if (*n_ents >= max_ents) 
            {
                *lpp = lp;
                return;
            }

            entries[*n_ents] = entp;

            (*n_ents)++;

            /* if not saving search context don't look for next matching
             * entry 
             */
            if ((map_handle == NULL) && (*n_ents >= max_ents)) 
            {
                *lpp = lp;
                return;
            }
        } /* entry == target */

    } /* for lp */

    *lpp = lp;
}

/*  map_mgmt_endpt_unique
 *  return true if addr does not match addr of any entry in entries 
 *  otherwise return false
 */
INTERNAL boolean32 map_mgmt_endpt_unique(addr, n_ents, entries)
rpc_addr_p_t    addr;
unsigned32      n_ents;
db_entry_t      *entries[];
{
    unsigned32      i;
    error_status_t  tmp_st;

    for (i = 0; i < n_ents; i++)
    { 
        /*  matching endpoint
         */
        if (rpc__naf_addr_compare(addr, entries[i]->addr, &tmp_st))
            return(false);
    }

    /*  no matching endpoint found
     */
    return(true);
}



/*
 * Return the database object's id.
 */
PRIVATE void epdb_inq_object(h_, object, status)
epdb_handle_t h_;
dce_uuid_t *object;
error_status_t *status;
{
    struct db *h = (struct db *) h_;

    SET_STATUS_OK(status);

    *object = h->object;
}

PRIVATE void epdb_delete_lookup_handle(h_, entry_handle)
epdb_handle_t       h_;
ept_lookup_handle_t *entry_handle;
{
    struct db *h = (struct db *) h_;

    db_lock(h);

    db_delete_context(h, entry_handle);

    db_unlock(h);
}


