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
**      rpcddb.c
**
**  FACILITY:
**
**      RPC Daemon Basic Database Routines
**
**  ABSTRACT:
**
**      Hash table management, list management, context handle management,
**      database concurrency control (lock/unlock) routines. Basic database 
**      open and update routine.
**
**
*/

#include <dce/ep.h>
#include <dsm.h>

#include <commonp.h>
#include <com.h>

#include <rpcdp.h>
#include <rpcddb.h>


typedef struct {
    struct db           *db_handle;
    db_list_type_t      list_type;
    db_lists_t          *lp;
    unsigned32          pass;
}   db_contexth_t, *db_contexth_p_t;


INTERNAL void list_add
    (
        db_list_t   *list,
        db_list_t   *elp
    );

INTERNAL void list_remove
    (
        db_list_t   *list,
        db_list_t   *elp
    );

INTERNAL boolean32 db_bad_context
    (
        struct db           *h,
        ept_lookup_handle_t *entry_handle
    ); 

/*
 * Setup the persistent database image.
 * If no database exists on disk, create it.
 */
PRIVATE void db_open(h, database_file, version, status)
struct db       *h;
unsigned char   *database_file;
unsigned32      version;
error_status_t  *status;
{
    struct db_file_hdr  hdr;
    error_status_t      tmp_st;

    SET_STATUS_OK(status);

    dsm_open(database_file, (dsm_handle_t *) &h->dsh, status);
    if (STATUS_OK(status))
    {
        /* read and check the file header info */
        dsm_get_info(h->dsh,&hdr,sizeof(hdr),status); 
        if (! STATUS_OK(status)) 
        {
            if (dflag) show_st("Error reading endpoint database", status);
            db_to_ept_ecode(status);
            return;
        }

        if (hdr.version != version) 
        {
            /*  Bad database version
             *  Close database
             *  If earlier version, just delete database,
             *  we'll try to create a new one later.
             *  If later version, return an error
             */
            dsm_close((dsm_handle_t *) &h->dsh, &tmp_st);
            h->dsh = NULL;

            if (hdr.version < version) 
            {
                remove((char *)database_file);
            }
            else
            {
                SET_STATUS(status, ept_s_database_invalid);
                if (dflag) show_st("Newer persistent database version", status);
                return;
            }
        }
        else
            h->object = hdr.object;
    }
    else
    if (*status != dsm_err_open_failed)
    {
        /* file exists but couldn't open it */
        if (dflag) 
            show_st("Error opening endpoint database", status);
        db_to_ept_ecode(status);
        return;
    }

    if (h->dsh == NULL)
    {
        /*  Create and initialize file 
         */
        dsm_create(database_file, (dsm_handle_t *) &h->dsh, status);
        if (! STATUS_OK(status))
        {
            if (dflag) 
                show_st("Error creating endpoint database", status);
            db_to_ept_ecode(status);
            return;
        }

        dce_uuid_create(&h->object, &tmp_st);

        hdr.version = version;
        hdr.object = h->object;
        dsm_set_info(h->dsh, &hdr, sizeof(hdr), status);
        if (! STATUS_OK(status)) 
        {
            if (dflag) 
                show_st("Error writing to endpoint database", status);
            db_to_ept_ecode(status);
            return;
        }
    }

    return;
} 

/*  db_update_entry
 *  Update an existing record in the disk copy 
 *  of a database.
 *  DSM update is really a delete and add so
 *  if the process crashes between dsm_detach
 *  and dsm_write the record is lost.
 *
 *  dsm_detach marks an entry as free on disk, used
 *  in volatile memory (so it won't be given out to anyone
 *  else during this process's lifetime but it will be 
 *  lost in a crash)
 */

PRIVATE void db_update_entry(h, entp, status)
struct db       *h;
db_entry_p_t    entp;
error_status_t  *status;
{
    dsm_detach(h->dsh, (void *)entp, status);
    if (! STATUS_OK(status)) return;

    dsm_write(h->dsh, (void *)entp, status);
    if (! STATUS_OK(status)) return;
}



/*
 *  Each entry is on 3 lists: the entry list (of all entries), a list that is
 *  keyed by the entry's object uuid, and a list that is keyed by the entry's
 *  interface uuid.  An entry's object list and interface list are accessed
 *  via uuid hash into a hash table which marks the beginning of a list for
 *  all uuids that hash to the same value.  Separate hash tables are used for 
 *  the object lists and the interface lists.
 *
 *  The entry lists are not stably stored.  They are recreated at startup
 *  time.
 *
 *  A database's lists are accessed via its struct db.lists_mgmt field.
 *  The lists contain forward and back pointers with the last entry on
 *  a list having its fwd ptr = NULL.  
 *
 *  The procedures that traverse lists (db_list_first, db_list_next) return a pointer 
 *  to the beginning of an entry (assuming that db_lists_t is at the beginning of 
 *  db_entry_t).  To get a pointer to the beginning of the entry, these procedures 
 *  subtract a pre-inited offset from the entry's object or interface list ptr.
 */
INTERNAL unsigned32  db_c_object_list_offset;
INTERNAL unsigned32  db_c_interface_list_offset;

PRIVATE void db_init_lists(h)
struct db           *h;
{   
    int             i;
    db_lists_mgmt_t *lists_mgmt;
    db_lists_t      lists; 

    lists_mgmt = &h->lists_mgmt;

    lists_mgmt->entry_list.fwd = NULL;
    lists_mgmt->entry_list.back = &lists_mgmt->entry_list;

    for (i = 0; i < db_c_nbucket; i++)
    {
        lists_mgmt->object_table[i].fwd = NULL;
        lists_mgmt->object_table[i].back = &lists_mgmt->object_table[i];
        lists_mgmt->interface_table[i].fwd = NULL;
        lists_mgmt->interface_table[i].back = &lists_mgmt->interface_table[i];
    }

    db_c_object_list_offset = ((char *) &lists.object_list) - ((char *) &lists.entry_list);
    db_c_interface_list_offset = ((char *) &lists.interface_list) - ((char *) &lists.entry_list);
}

PRIVATE void db_lists_add(h, entp)
struct db       *h;
db_entry_t      *entp;
{
    db_list_add(&h->lists_mgmt.entry_list, db_c_entry_list, (db_lists_t *) entp);

    db_htable_add(h->lists_mgmt.object_table, db_c_object_list, &entp->object, 
        (db_lists_t *) entp);

    db_htable_add(h->lists_mgmt.interface_table, db_c_interface_list, &entp->interface.uuid, 
        (db_lists_t *) entp);
}

PRIVATE void db_lists_remove(h, entp)
struct db       *h;
db_entry_t      *entp;
{
    db_list_remove(&h->lists_mgmt.entry_list, db_c_entry_list, (db_lists_t *) entp);

    db_htable_remove(h->lists_mgmt.object_table, db_c_object_list, &entp->object, 
        (db_lists_t *) entp);

    db_htable_remove(h->lists_mgmt.interface_table, db_c_interface_list, &entp->interface.uuid, 
        (db_lists_t *) entp);
}

PRIVATE void db_htable_add(htable, table_type, id, entp)
db_hash_table_t     htable;
db_list_type_t      table_type;
dce_uuid_p_t            id;
db_lists_t          *entp;
{
    unsigned16      bucket;
    error_status_t  tmp_st;

    bucket = dce_uuid_hash(id, &tmp_st);

    bucket = bucket % db_c_nbucket;

    db_list_add(&htable[bucket], table_type, entp);
}

PRIVATE void db_htable_remove(htable, table_type, id, entp)
db_hash_table_t     htable;
db_list_type_t      table_type;
dce_uuid_p_t            id;
db_lists_t          *entp;
{
    unsigned16      bucket;
    error_status_t  tmp_st;

    bucket = dce_uuid_hash(id, &tmp_st);

    bucket = bucket % db_c_nbucket;

    db_list_remove(&htable[bucket], table_type, entp);
}

/*
 *  Add an entry (entp) to the beginning of list.
 *  The entry is being added to one of 3 lists
 *  which is defined by list_type
 */
PRIVATE void db_list_add(list, list_type, entp)
db_list_t           *list;
db_list_type_t      list_type;
db_lists_t          *entp;
{
    db_list_t       *elp;
    error_status_t  tmp_st;

    switch(list_type)
    {
        case db_c_entry_list:
            elp = &entp->entry_list;
            break;
        case db_c_object_list:
            elp = &entp->object_list;
            break;
        case db_c_interface_list:
            elp = &entp->interface_list;
            break;
        default:
            tmp_st = ept_s_database_invalid;
            show_st("db_list_add -  bad list type", &tmp_st);
            return;
    }

    list_add(list, elp);
}

PRIVATE void db_list_remove(list, list_type, entp)
db_list_t           *list;
db_list_type_t      list_type;
db_lists_t          *entp;
{
    db_list_t       *elp;
    error_status_t  tmp_st;

    switch(list_type)
    {
        case db_c_entry_list:
            elp = &entp->entry_list;
            break;
        case db_c_object_list:
            elp = &entp->object_list;
            break;
        case db_c_interface_list:
            elp = &entp->interface_list;
            break;
        default:
            tmp_st = ept_s_database_invalid;
            show_st("db_list_remove -  bad list type", &tmp_st);
            return;
    }

    list_remove(list, elp);
}

/*
 *  Add entry elp to the end of list
 */
INTERNAL void list_add(list, elp)
db_list_t   *list;
db_list_t   *elp;
{
    elp->fwd = NULL;
    elp->back = list->back;

    elp->back->fwd = elp;
    list->back = elp;
}

/*  Remove entry elp from a list
 */
INTERNAL void list_remove(list, elp)
db_list_t   *list;
db_list_t   *elp;
{
    elp->back->fwd = elp->fwd;
    if (elp->fwd != NULL)
        elp->fwd->back = elp->back;
    else
        list->back = elp->back;         /* remove from end of list */

    elp->fwd = NULL;
    elp->back = NULL;
}

/*  Return a pointer to the first entry on list list_type
 */
PRIVATE db_lists_t *db_list_first(lists_mgmt, list_type, id)
db_lists_mgmt_t     *lists_mgmt;
db_list_type_t      list_type;
dce_uuid_p_t            id;
{
    unsigned16      bucket;
    error_status_t  tmp_st;
    db_list_t       *elp;
    db_lists_t      *entp;

    entp = NULL;

    switch(list_type)
    {
        case db_c_entry_list:
            entp = (db_lists_t *) lists_mgmt->entry_list.fwd;
            break;
        case db_c_object_list:
            bucket = dce_uuid_hash(id, &tmp_st);
            bucket = bucket % db_c_nbucket;
            elp = lists_mgmt->object_table[bucket].fwd;
            if (elp != NULL)
                entp = (db_lists_t *) (((char *) elp) - db_c_object_list_offset);
            break;
        case db_c_interface_list:
            bucket = dce_uuid_hash(id, &tmp_st);
            bucket = bucket % db_c_nbucket;
            elp = lists_mgmt->interface_table[bucket].fwd;
            if (elp != NULL)
                entp = (db_lists_t *) (((char *) elp) - db_c_interface_list_offset);
            break;
        default:
            tmp_st = ept_s_database_invalid;
            show_st("db_list_first -  bad list type", &tmp_st);
            break;
    }

    return(entp);
}

/*  Return a pointer to the next entry (after xentp) on list list_type
 */
PRIVATE db_lists_t *db_list_next(list_type, xentp)
db_list_type_t      list_type;
db_lists_t          *xentp;
{
    db_list_t       *elp;
    db_lists_t      *entp;
    error_status_t  tmp_st;

    entp = NULL;

    switch(list_type)
    {
        case db_c_entry_list:
            entp = (db_lists_t *) xentp->entry_list.fwd;
            break;
        case db_c_object_list:
            elp = xentp->object_list.fwd;
            if (elp != NULL)
                entp = (db_lists_t *) (((char *) elp) - db_c_object_list_offset);
            break;
        case db_c_interface_list:
            elp = xentp->interface_list.fwd;
            if (elp != NULL)
                entp = (db_lists_t *) (((char *) elp) - db_c_interface_list_offset);
            break;
        default:
            tmp_st = ept_s_database_invalid;
            show_st("db_list_next -  bad list type", &tmp_st);
            break;
    }

    return(entp);
}

/*  Save context
 *  epdb_lookup, fwd, map take an ept_entry_handle_t context handle 
 *  argument which supports iterative search of the database.  The list 
 *  type, list entry pointer, and pass number are saved so the search
 *  can resume where it left off.
 */
PRIVATE void db_save_context(h, entry_handle, list_type, lp, pass)
struct db           *h;
ept_lookup_handle_t *entry_handle;
db_list_type_t      list_type;
db_lists_t          *lp;
unsigned32          pass;
{
    db_contexth_t   *chp;
    db_entry_t      *entp;

    /*
     *  If the entry list pointer is NULL assume that the
     *  job is done - just delete the context (if one has been
     *  established) and return
     */
    if (lp == NULL) 
    { 
        db_delete_context(h, entry_handle);
        return;
    }

    /*  Check that context handle is ok
     */
    if (db_bad_context(h, entry_handle))
        return;

    /*  If the context already exists, decrement the number of
     *  read references in the entry to which the old context
     *  points; this context won't point to the entry any more.  
     *  If the context does not exist, malloc space for it.
     *
     *  Set chp and *entry_handle to the context's address.
     */
    if (*entry_handle != NULL)
    {
        chp = (db_contexth_t *) (*entry_handle);
        entp = (db_entry_t *) chp->lp;
        entp->read_nrefs--;
    }
    else
    {
        *entry_handle = (ept_lookup_handle_t *) malloc(sizeof(db_contexth_t));
        if (*entry_handle == NULL)
            return;
        chp = (db_contexth_t *) (*entry_handle);
    } 

    /*  Increment read ref count of the entry to which context will point.
     *  Check for read refs overflow 
     */
    entp = (db_entry_t *) lp;
    entp->read_nrefs++;
    if (entp->read_nrefs >= db_c_max_read_nrefs) 
    {
        db_delete_context(h, entry_handle);
        return;
    }

    /*  Save new context info
     */
    chp->db_handle = h;
    chp->list_type = list_type;
    chp->lp = lp;
    chp->pass = pass;
}

PRIVATE void db_delete_context(h, entry_handle)
struct db           *h;
ept_lookup_handle_t *entry_handle;
{
    db_contexth_t   *chp;
    db_entry_t      *entp;

    if (db_bad_context(h, entry_handle) || (*entry_handle == NULL))
        return;

    chp = (db_contexth_t *) (*entry_handle);
    entp = (db_entry_t *) chp->lp;
    entp->read_nrefs--;

    free(chp);

    *entry_handle = NULL;
}

PRIVATE void db_get_context(h, entry_handle, list_type, lp, pass, status)
struct db           *h;
ept_lookup_handle_t *entry_handle;
db_list_type_t      *list_type;
db_lists_t          **lp;
unsigned32          *pass;
error_status_t      *status;
{
    db_contexth_t   *chp;

    if (db_bad_context(h, entry_handle) || (*entry_handle == NULL))
    {
        SET_STATUS(status, ept_s_invalid_context);
        return;
    }

    chp = (db_contexth_t *) *entry_handle;
    *list_type = chp->list_type;
    *lp = chp->lp;
    *pass = chp->pass;

    SET_STATUS_OK(status);
}

/*  Check whether this entry_handle's context 
 *  points to the dbase denoted by h.
 *  Return true and status = ept_s_invalid_context
 *  if entry_handle's db_handle field != h arg.
 *  Otherwise return false.
 */
PRIVATE boolean32 db_different_context(h, entry_handle, status)
struct db           *h;
ept_lookup_handle_t *entry_handle;
error_status_t      *status;
{
    db_contexth_t   *chp;

    SET_STATUS_OK(status);

    if (entry_handle == NULL) 
        return(false);

    if (*entry_handle != NULL) 
    {
        chp = (db_contexth_t *) *entry_handle;
        if (chp->db_handle != h)
        {
            SET_STATUS(status, ept_s_invalid_context);
            return(true);
        }
    }

    return(false);
}

/*  return true if entry_handle is a "bad" context
 *  handle - ie.
 *      there's no place to pass the handle back 
 *          to the caller (ie. he really didn't want to 
 *          save context)
 *      the context handle doesn't point to this database
 *  return false if entry_handle appears to be ok
 */
INTERNAL boolean32 db_bad_context(h, entry_handle)
struct db           *h;
ept_lookup_handle_t *entry_handle;
{
    db_contexth_t   *chp;

    if (entry_handle == NULL) 
        return(true);

    if (*entry_handle != NULL) 
    {
        chp = (db_contexth_t *) *entry_handle;
        if (chp->db_handle != h)
            return(true);
    }

    return(false);
}


/*
 * Note that the rpcddb package must protect a database from
 * concurrent access since there can be a call executor thread,
 * a dg forward map callback thread, and a check server liveness
 * thread.
 */

PRIVATE void db_lock(h)
struct db *h;
{
    dcethread_mutex_lock_throw(&h->lock);
}

PRIVATE void db_unlock(h)
struct db *h;
{
    dcethread_mutex_unlock_throw(&h->lock);
}

PRIVATE void db_init_lock(h)
struct db *h;
{
    dcethread_mutex_init_throw(&h->lock, NULL);
}


/*  Map dsm error codes to ept error codes
 */
PRIVATE void db_to_ept_ecode(status)
error_status_t  *status;
{
    switch ((int)*status) 
    {
        case dsm_err_create_failed:
            *status = ept_s_cant_create;
            break;
        case dsm_err_file_io_error:
            *status = ept_s_update_failed;
            break;
        case dsm_err_open_failed:
            *status = ept_s_cant_access;
            break;
        case dsm_err_version:
            *status = ept_s_database_invalid;
            break;
        case dsm_err_no_memory:
            *status = ept_s_no_memory;
            break;
        case dsm_err_duplicate_write:
            *status = ept_s_update_failed;
            break;
        case dsm_err_header_too_long:
            *status = ept_s_database_invalid;
            break;
        case dsm_err_no_more_entries:
            *status = ept_s_not_registered;
            break;
        case dsm_err_invalid_handle:
            *status = ept_s_cant_perform_op;
            break;
        case dsm_err_invalid_pointer:
            *status = ept_s_cant_perform_op;
            break;
        case dsm_err_info_too_long:
            *status = ept_s_cant_perform_op;
            break;
        case dsm_err_file_busy:
            *status = ept_s_database_already_open;
            break;
        case dsm_err_invalid_marker:
            *status = ept_s_cant_perform_op;
            break;
        default:
            break;
    }
}

