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
**
**  NAME:
**
**      dsm.c
**
**  FACILITY:
**
**      Data Storage Manager (DSM)
**
**  ABSTRACT:
**
**  The DSM implements a heap storage interface wherein records are strongly
**  associated with records in a file, such that they can be stably stored
**  upon modification.  The basic paradigm is that the client ALLOCATEs a
**  block of some requested size, modifies it in memory, and WRITEs it;
**  successful completion of the WRITE implies that the record has been stably
**  stored in the file.  DSM makes this assumption about the underlying file
**  system and operations thereon: there is a basic page size which is well
**  known (compile time constant), and file operations operate on pages as
**  units.  In particular, a write operation which is entirely within a page
**  (i.e. a write of a page or a subset of a page) will succeed or fail
**  atomically, with no intermediate state.  We use this to stably write
**  new blocks, and free existing ones.  There is no atomic update operation;
**  the caller must establish protocol for doing updates of existing records.
**  
**  The memory model is simple.  The heap file begins with a header page
**  containing version information etc. (including the size of the stably
**  initialized portion of the file).  The rest of the file is some integral
**  number of pages long, with records laid out in sequence.  Each record
**  begins with a preheader (to us it's a header, to the client application
**  an invisible preheader) which contains the offset of the record in the
**  file (so we can easily update that record), the size of the record not
**  including the preheader, a 'link' field used to string the free list
**  in memory, a 'free' indication, and a 'magic cookie' used to flag validly
**  allocated records in memory.  The portion of the record after the preheader
**  is what we give the client; note that there may be slop at the end of
**  the record beyond the amount of space requested, for alignment.  The
**  preheader and the data portion of the record are both aligned to an 8-byte
**  boundary.  The first 64 bytes of each record, including the preheader,
**  fit within a page, so the preheader (16 bytes currently) and the first
**  48 bytes of the user data can be atomically updated.
**  
**  When initially created, no data pages are allocated to the file (just
**  the header is filled in).  When the file needs to grow (because no suitable
**  free block is available) it is extended by an integral number of pages,
**  at least large enough to accomodate the client request, and at least
**  a minimum size.  A chunk of memory of the growth size is allocated at
**  the same time to act as the virtual mirror of this new section of the
**  file.  As the file grows over time a list of such chunks is formed.  When
**  a preexisting file is opened a single chunk representing the actual length
**  of the file is created, and the entire file read into it.  Within memory
**  a free list is constructed linking all free blocks; this list is
**  reconstructed at open time by traversing the entire (virtual) file.
**  
**  A given data store can be specified to be volatile at open/create time
**  by giving a null or empty filename, in which case there will be no backing
**  file.  This allows PL/I-style storage areas, where an arbitrary number
**  of records can be allocated in memory in a data store and freed with
**  a single operation of closing the data store handle, whereas each record
**  would have to be kept track of and freed individually with conventional
**  malloc.
**
**
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


#include <dce/dce.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#define _I_AM_DSM_C_
#include "dsm_p.h"  /* private include file */

/** verify_dsh

    Make sure this thing looks like a good data store handle.  It shouldn't
    be null, and should have the proper magic cookie.
*/

#define verify_dsh(dsh) { \
    if ((dsh) == NULL) SIGNAL(dsm_err_invalid_handle);  \
    if ((unsigned32)(dsh)->cookie != DSM_COOKIE) SIGNAL(dsm_err_invalid_handle);  \
  }

/** dsm_create

    Create a new data store.  This involves creating a new file based on
    the user-provided name, initializing its header, and allocating a data
    store record to represent it.  As created the file is empty other than
    the header, and thus will have to grow on the first dsm_allocate()
    call.  If the filename is null/empty no backing file will exist for this
    datastore.
*/

public void dsm_create(fname,new_dsh,st)
unsigned char  *fname;      /* filename (null-terminated) */
dsm_handle_t   *new_dsh;    /* (output) resulting data store handle */
error_status_t *st;         /* (output) status */
{
    int                 fd = -1;            /* file descriptor of created file */
    dsm_handle          dsh;                /* allocated data store handle */
    file_hdr_t          fhdr;               /* file header (first page) */

    CLEANUP {      /* upon errors */
        if (fd != -1) close(fd);            /* if file is open close it */
        return;                             /* return with error status */
    }

    if (fname != NULL && ustrlen(fname) != 0) {
        fd = create_file(fname);                /* try to create the file */
        if (fd == -1) SIGNAL(dsm_err_create_failed);  /* error if fails */
        dsm__lock_file(fd, st);         /* lock the file for unique access */
        PROP_BAD_ST;

        fhdr.version = dsm_version;            /* file has current version */
        fhdr.pages = 0;                         /* no currently allocated data pages (only hdr) */
        if (dcethread_write(fd,&fhdr,PAGE_SIZE) != PAGE_SIZE) SIGNAL(dsm_err_file_io_error); /* write hdr */
    }

    dsh = NEW(dsm_db_t);                    /* allocate data store handle */
    if (dsh == NULL) SIGNAL(dsm_err_no_memory);    /* make sure we got it */

    dsh->fd = fd;                           /* remember fd */
    if (fname == NULL) {
        dsh->fname = (char *) malloc(1);
        dsh->fname[0] = 0;
    }
    else {
        dsh->fname = (char *) malloc(ustrlen(fname)+1);  /* and filename */
        ustrcpy(dsh->fname,fname);
    }
    dsh->freelist = NULL;                   /* no free memory */
    dsh->map = NULL;                        /* therefore no file mapping */
    dsh->pages = 0;                         /* and no data pages in the file */
    dsh->cookie = DSM_COOKIE;               /* magic cookie */
    dsh->coalesced = false;                 /* no coalesce yet */
    dsh->pending = 0;                       /* no allocated/unwritten blocks */
    cache_clear(dsh);                       /* nothing in cache */                    

    *new_dsh = (dsm_handle_t)dsh;           /* fill in user's dsh */

    CLEAR_ST;

} /* dsm_create */

/** dsm_open

    Open a preexisting data store.  This involves opening the user-named
    file, reading its header to determine its version stamp and data size,
    then allocating a data store record for the file.  We then allocate
    an initial file map (one entry) mapping the entire file (pages
    1..pagecount) to a single buffer, which we allocate and read the file
    into.  Finally we use the build_freelist() operation to traverse the file
    constructing the free list.  If the file name is null/empty there is no  
    backing store.
*/

public void dsm_open(fname,new_dsh,st)
unsigned char  *fname;      /* filename (null-terminated) */
dsm_handle_t   *new_dsh;    /* (output) resulting data store handle */
error_status_t *st;         /* (output) status */
{
    int                 fd = -1;        /* file descriptor of opened file */
    dsm_handle          dsh = NULL;     /* allocated data store handle */
    file_hdr_t          fhdr;           /* file header (first page) */

    CLEANUP {  /* upon errors */
        if (fd != -1) close(fd);        /* if file is open close it */
        if (dsh) {                      /* free allocated memory */
            if (dsh->map) {             /* which is nested */
                if (dsh->map->ptr) free(dsh->map->ptr);
                free(dsh->map);
            }
            free(dsh->fname);
            free(dsh);
        }
        return;                         /* return with error status */
    }

    if (fname != NULL && ustrlen(fname) != 0) {
        fd = open((char *)fname,O_RDWR);        /* try to open the file */
        if (fd == -1) SIGNAL(dsm_err_open_failed);  /* signal error if fails */
        dsm__lock_file(fd, st);         /* lock the file for exclusive access */
        PROP_BAD_ST;

        if (dcethread_read(fd,&fhdr,PAGE_SIZE) != PAGE_SIZE) SIGNAL(dsm_err_file_io_error); /* read hdr */
     
        if (fhdr.version != dsm_version) SIGNAL(dsm_err_version); /* check version */
    }

    dsh = NEW(dsm_db_t);            /* allocate data store handle */
    if (dsh == NULL) SIGNAL(dsm_err_no_memory);    /* make sure we got it */

    dsh->fd = fd;                   /* remember fd */
    dsh->fname = (char *) malloc(ustrlen(fname)+1);  
    ustrcpy(dsh->fname,fname);      /* and filename */
    dsh->freelist = NULL;           /* no free list yet */
    dsh->map = NULL;                /* no memory mapping yet */
    dsh->pages = (fd < 0) ? 0 : fhdr.pages; /* number of valid pages */
    dsh->cookie = DSM_COOKIE;       /* magic cookie */
    dsh->coalesced = false;         /* haven't coalesced yet */
    dsh->pending = 0;               /* no allocated/unwritten blocks */
    cache_clear(dsh);               /* nothing in cache */

    if (dsh->pages > 0) {           /* if any valid pages */
        dsh->map = NEW(file_map_t);                                 /* get initial file map */
        dsh->map->link = NULL;                                      /* it's the last one */
        dsh->map->loc = PAGE_SIZE;                                  /* at starts at the second page */
        dsh->map->size = dsh->pages*PAGE_SIZE;                      /* it's this many bytes long */
        dsh->map->ptr = (block_t *)malloc(dsh->map->size);          /* allocate space for it */
        if (dsh->map->ptr == NULL) SIGNAL(dsm_err_no_memory);  /* did we get it? */
    
        /* observe that the file pointer is right after the header, from header read.
        */
        if ((unsigned32)dcethread_read(dsh->fd,dsh->map->ptr,dsh->map->size) != dsh->map->size) { /* read in the file */
            SIGNAL(dsm_err_file_io_error); /* gripe if that fails */
        }
    
        build_freelist(dsh);        /* reconstruct free list */
    }

    *new_dsh = (dsm_handle_t)dsh;   /* fill in user's dsh */

    CLEAR_ST;
} /* dsm_open */

/** dsm_close

    Close an open data store, freeing allocated resources.  We attempt
    to avoid potential dangling pointers by taking the dsh by reference
    and clearing it, as well as clearing its cookie before freeing it.
*/

public void dsm_close(dsx,st)
dsm_handle_t  *dsx;    /* (update) data store handle */
error_status_t *st;     /* (output) status */
{
    dsm_handle *dsh = (dsm_handle *)dsx;

    CLEANUP {  /* upon errors */
        return;                         /* and just return with fault status */
    }

    verify_dsh(*dsh);                   /* reality check */

    if ((*dsh)->fd >= 0) {              /* if there's a file */
        close((*dsh)->fd);              /* close it */
    }
    free_map((*dsh)->map);              /* free the memory map and all mapped memory */
    (*dsh)->cookie = 0;                 /* clear cookie (in case of alias pointers) */
    free((*dsh)->fname);
    free(*dsh);                         /* free the data store record */

    *dsh = NULL;                        /* invalidate the handle */
    CLEAR_ST;

} /* dsm_close */

/** dsm_allocate

    Allocate a block of at least the requested size from the storage pool
    associated with a dsm file.  First we try to locate such a block from
    the free pool; if that fails we try a coalesce pass (merge adjacent
    free blocks into single larger ones); if that fails we grow the file
    (if that fails it signals an error).  If the block we find is
    significantly larger than the one requested, we split it up by making
    a new free block from everything but the reqested amount (rounded up
    to an 8-byte boundary).  The header of the allocated block is marked
    with a "magic cookie" to let us do at least minimal validity checking
    on incoming pointers.

    Per datastore we maintain a count of "pending" records -- records that
    are free in the datastore, but the application holds pointers to them,
    as a result of dsm_allocate, or dsm_detach.  We don't want to
    coalesce the database while these are held because presumably the
    intent is to dsm_write[_hdr] (or at least dsm_free) them later;
    coalescing could move the block, rendering it invalid.
*/

public void dsm_allocate(dsx,usr_size,ptr,st)
dsm_handle_t   dsx;        /* data store handle */
unsigned32     usr_size;   /* requested minimum size of user data area */
void *         *ptr;        /* (output) allocated storage (user data) */
error_status_t *st;         /* (output) status */
{              
    dsm_handle dsh = (dsm_handle)dsx;
    block_t            *p;      /* new block */
    block_t            *fp;     /* constructed block made from tail of large free block */

    CLEANUP {  /* upon errors */
        return;                         /* and just return with fault status */
    }

    verify_dsh(dsh);                    /* reality check */

    usr_size = ROUND_UP(usr_size,8);            /* round size up to 8-byte alignment */
    
    p = get_free_block(dsh,usr_size);           /* get suitably-sized block from free list */

    if (p == NULL) {                            /* didn't find one? */
        coalesce(dsh, st);                      /* coalesce adjacent free blocks */
        PROP_BAD_ST;
        p = get_free_block(dsh,usr_size);       /* try again */
        if (p == NULL) {                        /* still out of luck? grow the file */ 
            p = grow_file(dsh, usr_size, st);   /* grow the file by at least usr_size */
            PROP_BAD_ST;
        }
    }

    if ((p->size - usr_size) >= MINBLOCK) {     /* is there enough left to split the block? */
        fp = (block_t *)(((char *)&p->data)+usr_size); /* point past user data portion */
        fp->loc = p->loc+PREHEADER+usr_size;    /* file location */
        if ((ROUND_UP(fp->loc, PAGE_SIZE) - fp->loc) >= UNIT) {  /* if at least unit left in page */
            fp->size = p->size-PREHEADER-usr_size;  /* compute its size */
            make_free(dsh,fp,st);                   /* make free in memory and on disk */
            p->size = usr_size;                     /* this block gets smaller */
            write_header(dsh, p, st);               /* done */
            PROP_BAD_ST;
        }
    }

    p->cookie = HDR_COOKIE;                     /* set magic cookie value */
    if (dsh->fd < 0) p->isfree = false;         /* in volatile DB it's nonfree */
    else dsh->pending += 1;                     /* otherwise remember pending block */
    *ptr = (void * )&p->data;                   /* fill in data pointer */
    CLEAR_ST;

} /* dsm_allocate */

/** dsm_free

    Frees a block previously allocated by dsm_allocate.
*/

public void dsm_free(dsx,ptr,st)
dsm_handle_t   dsx;    /* data store handle */
void *          ptr;    /* user data pointer (to user portion of a block) */
error_status_t *st;     /* (output) status */
{
    dsm_handle dsh = (dsm_handle)dsx;
    block_t            *p;      /* pointer to actual block preheader */

    CLEANUP {  /* upon errors */
        return;                         /* and just return with fault status */
    }

    verify_dsh(dsh);                    /* reality check */

    p = block_from_ptr(ptr, st);            /* get block pointer */
    PROP_BAD_ST;

    /* if this block is already free (assume it's been allocated but
       not written), decrement the pending count.
    */
    if (p->isfree) {                    /* if it was already free */
        assert(dsh->pending > 0);       /* it should have been pending */
        dsh->pending -= 1;              /* but no longer */
    }

    make_free(dsh,p,st);                 /* free the block stably, add to free list */


} /* dsm_free */

/** dsm_detach

    We don't support an atomic update operation (replacing an existing record
    with a new copy).  The safest approach would be to create a new copy
    (presumably flagged so as to distinguish it in case of a crash) and then
    free the old one (finally unflagging the new one).  An update can also be
    considered a delete and an add, and the application must either not care
    if only the delete succeeds, or have some way of recovering (replaying a
    log etc.).  For such applications we provide a 'detach' operation.  Detach
    is similar to free; the effect is that if a crash occurs between the detach
    and write phases of an update, the block in question is effectively freed,
    i.e. upon restarting the record being updated will have been deleted. 
    dsm_detach of a free/detached record is a no-op.
*/

public void dsm_detach(dsx,ptr,st)
dsm_handle_t   dsx;    /* data store handle */
void *          ptr;    /* user data pointer (to user portion of a block) */
error_status_t *st;     /* output) status */
{
    dsm_handle dsh = (dsm_handle)dsx;
    block_t            *p;      /* pointer to actual block preheader */

    CLEANUP {  /* upon errors */
        return;                         /* and just return with fault status */
    }

    verify_dsh(dsh);                    /* reality check */

    p = block_from_ptr(ptr, st);            /* get block pointer */
    PROP_BAD_ST;

    if (!p->isfree) {                   /* unless already free */
        p->isfree = true;               /* mark as free */
        write_header(dsh, p, st);       /* make it stable */
        PROP_BAD_ST;
        dsh->pending += 1;              /* presumably detaching to write */
    }
    CLEAR_ST;

} /* dsm_detach */

/** dsm_write

    Writes the contents of the given block to the data store file.  The
    block should be 'free' in the sense of having not yet been written
    (via dsm_write) since being allocated by dsm_allocate or 'detached'
    by dsm_detach.  This restriction strictly need only apply to blocks
    that span page boundaries, due to the two-phased nature of the write
    operation: first the body of the record is written and committed, then
    the header is modified to clear the 'free' setting.  Up until this
    last atomic write the block is still free in the file, so that a crash
    at any time won't yield an inconsistent or partially-updated record.
*/

public void dsm_write(dsx,ptr,st)
dsm_handle_t    dsx;    /* data store handle */
void *          ptr;    /* user record pointer */
error_status_t *st;     /* (output) status */
{
    dsm_handle dsh = (dsm_handle)dsx;
    block_t            *p;          /* the actual record */
    unsigned long       sp,ep;      /* base of start page, end page */

    CLEANUP {  /* upon errors */
        return;                         /* and just return with fault status */
    }

    verify_dsh(dsh);                    /* reality check */

    p = block_from_ptr(ptr, st);            /* get actual record pointer */
    PROP_BAD_ST;

    /* as allocated records are not "free" in a volatile datastore,
       this check will go off in the case of write in such a case.
    */
    if (!p->isfree) SIGNAL(dsm_err_duplicate_write); /* must be free */

    if (dsh->fd >= 0) { /* only if there's a file */
        sp = ROUND_DOWN(p->loc,PAGE_SIZE);          /* file location of start of containing page */
        ep = ROUND_DOWN(p->loc+p->size+PREHEADER-1,PAGE_SIZE);    /* and of start of end page */

        if (ep == sp) {                             /* if all on one page */
            p->isfree = false;                        /* we'll write the whole thing at once */
            write_block(dsh,p,p->size+PREHEADER, st);   /* write the whole block */
            PROP_BAD_ST;
        }
        else {
            write_block(dsh,p,p->size+PREHEADER, st);   /* write the block with header still free */
            PROP_BAD_ST;
            p->isfree = false;                          /* clear free flag */
            write_header(dsh, p, st);                   /* update header */
            PROP_BAD_ST;
        }
    }

    assert(dsh->pending > 0);   /* this should have been pending */
    dsh->pending -= 1;  /* one less pending record */
    CLEAR_ST;

} /* dsm_write */

/** dsm_write_hdr

    Many applications want to support records too long to avoid spanning
    page boundaries, but have shorter header information that they may
    want to update atomically, such as "deleted" or "new" flags.  DSM
    guarantees that the first part of each record up to dsm_hdr_size [48]
    bytes can be written atomically, and so updated in place without having
    to worry about detaching.  dsm_write_hdr is like dsm_write except
    that an additional parameter specifies the number of bytes to write
    -- a typical call probably being:

        dsm_write_hdr(my_dsh,my_rec,sizeof(myrec.hdr),&status);

    Note - we'll count this as a pending record if it's free, but the
    caller is in charge, in such cases, of ensuring that this write is
    sufficient to stably reflect the record.
*/

public void dsm_write_hdr(dsx,ptr,size,st)
dsm_handle_t    dsx;    /* data store handle */
void *          ptr;    /* user pointer */
unsigned32      size;   /* length of user header */
error_status_t  *st;     /* status */
{
    dsm_handle dsh = (dsm_handle)dsx;
    block_t            *p;              /* the actual record */

    CLEANUP {  /* upon errors */
        return;                         /* and just return with fault status */
    }

    verify_dsh(dsh);                    /* reality check */

    p = block_from_ptr(ptr, st);            /* get actual record pointer */
    PROP_BAD_ST;

    if (size > dsm_hdr_size) SIGNAL(dsm_err_header_too_long); /* check size */

    if (p->isfree) {                    /* if it was free */
        assert(dsh->pending > 0);       /* it should have been pending */
        dsh->pending -= 1;              /* but no longer */
    }                                   /* now it's not free: */
    p->isfree = false;                  /* we'll write the whole thing at once */
    write_block(dsh, p, size+PREHEADER, st);  /* write the preheader and user header */
    PROP_BAD_ST;

    CLEAR_ST;
} /* dsm_write_hdr */

/** dsm_get_info
    dsm_set_info

    A small part of the file header (256 bytes) is reserved for client-defined
    file-wide header information (version information and the like).  These
    routines provide access to that storage.  Note that we don't keep a copy
    of the file header around, and so do I/O for every access, it being
    assumed that these operations are infrequent.  Note also that set_info
    does a read/alter/rewrite to preserve the DSM header information.

    Just give file_io_error on volatile data stores.
*/

public void dsm_get_info(dsx,info,size,st)
dsm_handle_t   dsx;    /* data store handle */
void *         info;   /* pointer to user info buffer */
unsigned32     size;   /* size of user info buffer */
error_status_t *st;    /* status */
{
    dsm_handle dsh = (dsm_handle)dsx;
    file_hdr_t  hdr;    /* data store file header */

    CLEANUP {  /* upon errors */
        return;                         /* and just return with fault status */
    }

    verify_dsh(dsh);                    /* reality check */

    if (size > INFOSZ) SIGNAL(dsm_err_info_too_long);  /* check size */

    /* reset to beginning of file and read in first page (file header).
    */
    if (dsh->fd < 0
    ||  lseek(dsh->fd,0,L_SET) == -1
    ||  dcethread_read(dsh->fd,&hdr,PAGE_SIZE) != PAGE_SIZE) SIGNAL(dsm_err_file_io_error);

    memcpy(info,hdr.info,size);          /* copy user info to caller's buffer */

    CLEAR_ST;
} /* dsm_get_info */

public void dsm_set_info(dsx,info,size,st)
dsm_handle_t   dsx;    /* data store handle */
void *         info;   /* pointer to user info buffer */
unsigned32     size;   /* size of user info buffer */
error_status_t *st;     /* status */
{
    dsm_handle dsh = (dsm_handle)dsx;
    file_hdr_t  hdr;    /* data store file header */

    CLEANUP {  /* upon errors */
        return;                         /* and just return with fault status */
    }

    verify_dsh(dsh);                    /* reality check */

    if (size > INFOSZ) SIGNAL(dsm_err_info_too_long);  /* check size */

    /* reset to beginning of file and read in first page (file header).
    */
    if (lseek(dsh->fd,0,L_SET) == -1
    ||  dcethread_read(dsh->fd,&hdr,PAGE_SIZE) != PAGE_SIZE) SIGNAL(dsm_err_file_io_error);

    memcpy(hdr.info,info,size);          /* copy caller's info into header */

    /* 
     * seek back to the beginning, write out file header, synch the file.
     */
    if (lseek(dsh->fd,0,L_SET) == -1
    ||  dcethread_write(dsh->fd,&hdr,PAGE_SIZE) != PAGE_SIZE
    ||  dsm__flush_file(dsh->fd) != status_ok ) SIGNAL(dsm_err_file_io_error);
 
    CLEAR_ST;
} /* dsm_set_info */

/** dsm_read

    Successively returns each non-free block in the data store.  We are
    passed (by reference) a marker which we interpret as a file offset
    in the data store.  The marker points to the last block examined; a
    special invalid offset as set by dsm_marker_reset will see the first
    non-free block, otherwise the next non-free block after that identified
    by the marker will be returned.  The marker is updated, and a pointer
    to the record filled in.  A status of dsm_err_no_more_entries will
    be returned when no more valid entries exist (i.e. on the call *after*
    the last valid entry is seen).  Thus the complete set of allocated
    records can be traversed by calling dsm_marker_reset and then 
    repeatedly calling dsm_read with that marker as long as the status
    is status_ok.

    A single-tasked process making consecutive calls to dsm_read without
    other dsm calls will see all allocated records.  However in general it
    is possible for a marker to become stale, as could happen during a
    coalesce operation.  We maintain a small cache of valid marker values,
    cleared upon coalescing; if a given marker value is not cached, then 
    we scan the data store from some earlier known valid block loooking 
    for the first allocated block at a higher offset than the nominal
    marker value (at worst we might have to scan the entire data store;
    the problem is that with variable-length records it's hard to find an
    anchor point).
*** heuristics can be applied: scanning the free list
    can generate valid record starts, as can scanning the memory map (chunk
    list), as can scanning the valid marker cache.
*/

public void dsm_read(dsx,marker,ptr,st)
dsm_handle_t   dsx;    /* data store handle */
dsm_marker_t  *marker; /* marker (file offset of previous record) */
void *         *ptr;    /* (output) next record */
error_status_t *st;     /* (output) status */
{
    dsm_handle dsh = (dsm_handle)dsx;
    block_t            *p = NULL;       /* a block */

    CLEANUP {  /* upon errors */
        return;                         /* and just return with fault status */
    }

    verify_dsh(dsh);                    /* reality check */

    if (*marker == MAGIC_MARKER) {      /* special request for first block */
        if (dsh->map) {                 /* must be mapped memory */
            p = dsh->map->ptr;          /* use first record */
            if (p->isfree) p = get_next_block(dsh,p); /* get the next one if that's free */
        }
    }
    else {
        /* If this marker is in the cache, we can use the block cached with it.
           Otherwise we have to back up to some known block at an earlier
           location and step forward until we reach the marker location (or
           a later block).
        */
        p = cache_lookup(dsh,*marker);  /* see if marker loc is in cache */

        if (!p) {                       /* if not in the cache */
            p = get_earlier_block(dsh,*marker); /* find some known earlier block */
        }
        while (p && (p->loc <= *marker)) p = get_next_block(dsh,p); /* get block after mkr */
    }

    if (!p) {                           /* if at end of file (no next record) */
        *ptr = NULL;                    /* flag EOF */
        SIGNAL(dsm_err_no_more_entries);   /* special status */
    }
    else {
        *ptr = (void * )&p->data;       /* return user pointer */
        *marker = p->loc;               /* update marker */
        cache_add(dsh,p,p->loc);        /* add to the cache */
    }

    CLEAR_ST;
} /* dsm_read */

/** dsm_locate

    Returns a pointer to a dsm block, given a marker identifying that block, as
    returned by dsm_read (the marker returned by a dsm_read call corresponds 
    to the record returned by that call), or by dsm_inq_marker.  The marker is
    simply the record's offset in the file.  We locate the record by methods
    similar to dsm_read, but the record must be located exactly.
*/

public void dsm_locate(dsx,marker,ptr,st)
dsm_handle_t   dsx;    /* data store handle */
dsm_marker_t   marker; /* marker (file offset of record) */
void *         *ptr;    /* (output) record */
error_status_t *st;     /* (output) status */
{
    dsm_handle dsh = (dsm_handle)dsx;
    block_t            *p;              /* a block */

    CLEANUP {  /* upon errors */
        return;                         /* and just return with fault status */
    }

    verify_dsh(dsh);                    /* reality check */

    /* reset marker is not valid 
    */
    if (marker == MAGIC_MARKER) SIGNAL(dsm_err_invalid_marker); 

    /* If this marker is in the cache, we can use the block cached with it.
       Otherwise we have to back up to some known block at an earlier
       location and step forward until we reach the marker location (or
       a later block).
    */
    p = cache_lookup(dsh,marker);  /* see if marker loc is in cache */

    if (!p) {                       /* if not in the cache */
        p = get_earlier_block(dsh,marker);  /* find some known earlier block */
        while (p && p->loc < marker) p = get_next_block(dsh,p); /* find mkr */
    }

    if (!p || p->loc != marker)  {      /* didn't find it? (EOF or wrong loc) */
        *ptr = NULL;                    /* don't leave anything dangling */
        SIGNAL(dsm_err_invalid_marker); /* gripe */
    }
    else {
        *ptr = (void * )&p->data;       /* return user pointer */
        cache_add(dsh,p,p->loc);        /* add to the cache */
    }

    CLEAR_ST;
} /* dsm_locate */

/** dsm_marker_reset

    Reset the given dsm_marker_t such that it will cause the first 
    block to be returned from the next dsm_read.
*/

public void dsm_marker_reset(marker)
dsm_marker_t  *marker;
{
    *marker = MAGIC_MARKER;     /* there; that was easy */
} /* dsm_marker_reset */

/** dsm_inq_marker

    Returns a marker identifying a given dsm block.  This marker can later
    be used to redevelop a pointer to the block, with dsm_locate, even if 
    the data store is closed in between.  Naturally such a marker becomes
    invalid if the block in question is freed.
*/

public void dsm_inq_marker(dsx,ptr,mkr,st)
dsm_handle_t   dsx;    /* data store handle */
void *          ptr;    /* user record pointer */
dsm_marker_t  *mkr;    /* (output) marker thereto */
error_status_t *st;     /* (output) status */
{
    dsm_handle dsh = (dsm_handle)dsx;
    block_t            *p;          /* the actual record */

    CLEANUP {  /* upon errors */
        return;                         /* and just return with fault status */
    }

    verify_dsh(dsh);                    /* reality check */

    p = block_from_ptr(ptr, st);            /* get actual record pointer */
    PROP_BAD_ST;

    *mkr = p->loc;                      /* fill in marker */

    CLEAR_ST;
} /* dsm_inq_marker */

/** dsm_inq_size

    Returns (in the "size" parameter) the size in bytes allocated to the 
    given dsm block.  This may be larger than the amount requested when
    the block was allocated.
*/

public void dsm_inq_size(dsx,ptr,len,st)
dsm_handle_t   dsx;    /* data store handle */
void *         ptr;    /* user record pointer */
unsigned32     *len;    /* (output) its length */
error_status_t *st;     /* (output) status */
{
    dsm_handle dsh = (dsm_handle)dsx;
    block_t            *p;          /* the actual record */

    CLEANUP {  /* upon errors */
        return;                         /* and just return with fault status */
    }

    verify_dsh(dsh);                    /* reality check */

    p = block_from_ptr(ptr, st);            /* get actual record pointer */
    PROP_BAD_ST;

    *len = p->size;                     /* fill in length */

    CLEAR_ST;
} /* dsm_inq_size */

/** dsm_get_stats

    Returns information about a data store.
*/

public void dsm_get_stats(dsx,stats,st)
dsm_handle_t   dsx;    /* data store handle */
dsm_stats_t   *stats;  /* (output) statistics record */
error_status_t *st;     /* (output) status */
{
    dsm_handle dsh = (dsm_handle)dsx;
    block_t            *p;              /* freelist traverser */

    CLEANUP {  /* upon errors */
        return;                         /* and just return with fault status */
    }

    verify_dsh(dsh);                    /* reality check */

    stats->size = dsh->pages*PAGE_SIZE;  /* size in bytes */

    /* determine total size of free list, size of largest block
    */
    stats->free_space = stats-> largest_free = 0;
    for (p = dsh->freelist; p != NULL; p = p->link) {   /* traverse free list */
        stats->free_space += p->size + PREHEADER;       /* accumulate size */
        stats->largest_free = p->size;  /* this is the largest as they're sorted */
    }

    CLEAR_ST;
} /* dsm_get_stats */

/** dsm_reclaim

    Reclaim free storage in a named datastore by making a new copy of it.
*/

public boolean dsm_reclaim(dsx,name,tmpname,bakname,pct,st)
dsm_handle_t   *dsx;        /* open data store handle */
unsigned char  *name;       /* data store name */
unsigned char  *tmpname;    /* build new copy in this temp */
unsigned char  *bakname;    /* name old (backup) version this */
unsigned32     pct;        /* percent freespace acceptable */
error_status_t *st;         /* (output) status */
{
    dsm_handle_t        curh = *dsx;    /* current data store handle */
    dsm_handle_t        newh=NULL;      /* new data store handle */
    dsm_stats_t         stats;          /* usage statistics */
    unsigned char       info[INFOSZ];   /* hdr info */
    dsm_marker_t        mkr;            /* traversing marker */
    void               *curp,*newp;     /* rec from curh, rec in newh */
    unsigned32          size;           /* size of record */
    error_status_t      st2;


    if (curh) {
        /*  does handle point to same data store that is being
            reclaimed?
        */
        if (ustrcmp(name, ((dsm_handle)curh)->fname) != 0) {
            *st = dsm_err_invalid_handle; 
            return false;
        }
    }
    else {
        /*  data store needs to be opened
        */
        dsm_open(name, &curh, st);
        if (BAD_ST) return false;
    }

    dsm_get_stats(curh,&stats,st);      /* get statistics */
    if (BAD_ST) goto EXIT;

    if ((stats.size == 0) || 
        (stats.free_space <= (GROW_PAGES*PAGE_SIZE)) ||
        ((100*stats.free_space)/stats.size <= pct)) { /* ignore if within pct */
        goto EXIT;                      /* return (with that status) */
    }

    dsm_create(tmpname,&newh,st);       /* create the temp/new file */
    if (BAD_ST) goto EXIT;

    dsm_get_info(curh,info,INFOSZ,st);
    if (BAD_ST) goto EXIT;
    dsm_set_info(newh,info,INFOSZ,st);
    if (BAD_ST) goto EXIT;

    dsm_marker_reset(&mkr);             /* start at the beginning */

    for (;;) {                          /* traverse the datastore */
        dsm_read(curh,&mkr,&curp,st);   /* get next record */
        if ((*st) == dsm_err_no_more_entries) break;  /* break at end */
        else if (BAD_ST) goto EXIT;

        dsm_inq_size(curh,curp,&size,st);   /* get record size */
        if (BAD_ST) goto EXIT;

        dsm_allocate(newh,size,&newp,st);   /* allocate same size in new file */
        if (BAD_ST) goto EXIT;

        memcpy(newp,curp,size);          /* copy the record */

        dsm_write(newh,newp,st);        /* and write it */
        if (BAD_ST) goto EXIT;
    }

    /* close datastores 
    */
    dsm_close(&newh,st); 
    if (BAD_ST) goto EXIT; 

    dsm_close(&curh,st);              
    if (BAD_ST) goto EXIT; 

    if (rename((char *)name,(char *)bakname) != 0) { /* rename foo, foo.bak */
        (*st) = dsm_err_file_io_error;
        goto EXIT;
    }
    
    if (rename((char *)tmpname,(char *)name) != 0) { /* rename foo.new foo */
        (*st) = dsm_err_file_io_error;
        goto EXIT;
    }

    if ((*dsx) != NULL) {
        dsm_open(name,dsx,st);          /* open reclaimed datastore */      
        if (BAD_ST) goto EXIT;
    }

    return true;

EXIT:   /* problem 
           clean up stuff dsm_reclaim has opened/created
           exit (with status st) 
        */
    if ((curh) && ((*dsx) == NULL )) {
        dsm_close(&curh,&st2);
    }
    else {
        *dsx = curh;
    }

    if (newh) {
        dsm_close(&newh,&st2);
    } 

    remove((char *) tmpname);  
    return false;
} /* dsm_reclaim */

/** dsm_print_map

    For debugging purposes, this traverses the mapped file printing a
    summary of each block (and doing some error checks on alignment etc.).
*/

public void dsm_print_map(dsx,st)
dsm_handle_t   dsx ATTRIBUTE_UNUSED;
error_status_t *st;
{
#ifndef DSM_DEBUG
    printf("dsm_print_map unavailable.\n");
    CLEAR_ST;
#else
    dsm_handle dsh = (dsm_handle)dsx;
    file_map_t         *map;        /* for traversing chunk list */
    block_t            *this,*next; /* current block, next block */

    CLEANUP {  /* upon errors */
        return;                         /* and just return with fault status */
    }

    verify_dsh(dsh);                    /* reality check */

    printf("DSM map; %d initialized pages of %d bytes; %d allocations pending\n",
           dsh->pages, PAGE_SIZE, dsh->pending);

    for (map = dsh->map; map != NULL; map = map->link) {    /* for each file chunk */
        printf("---- %d byte chunk at %d\n",map->size,map->loc);

        this = map->ptr;    /* first block in chunk */

        for (;;) {  /* until explicit break at end of chunk */
            printf("    %08x %08x: (page %d offset %d) %d bytes ",
                   this,this->loc,this->loc/PAGE_SIZE,MOD(this->loc,PAGE_SIZE),this->size);

            if (MOD(this->size,8) != 0) printf(" *** NOT 8-BYTE ALIGNED *** ");
            if (this->isfree) printf("(free)");
            else if (this->cookie != HDR_COOKIE) printf(" *** INVALID COOKIE *** ");

            printf("\n");

            next = (block_t *) (((char *)this)+this->size+PREHEADER);   /* find next block */

            if ((char *)next-(char *)map->ptr >= map->size) break;      /* is this the last block? */
            else this = next;   /* if more, iterate with next block */
        }
    }
    CLEAR_ST;

#endif
} /* dsm_print_map */

/** get_free_block

    Scan the free list seeking a block of at least some required amount,
    returning a pointer to it, after removing it from the free list.  We
    return the first match that's at least the requested size; thus the
    order of the free list as maintained by the free routine (q.v.) is
    significant.  Returns NULL if no large enough block exists.
*/

private block_t *get_free_block(dsh,min_size)
dsm_handle     dsh;        /* data store handle */
unsigned long   min_size;   /* requested size */
{
    block_t    *p,*pp;  /* list traversers */

    pp = NULL;
    for (p = dsh->freelist; p != NULL; p = p->link) {   /* traverse free list */
        if (p->size >= min_size) {          /* is this one big enough? take it */
            if (pp) pp->link = p->link;     /* if we've seen a previous node link there */
            else dsh->freelist = p->link;   /* else it's the first */
            p->link = NULL;                 /* cover our tracks to be safe */
            break;                          /* done */
        }
        pp = p;     /* backpointer */
    }

    return p;       /* back with our catch (NULL if no match found) */

} /* get_free_block */

/** grow_file

    Grow the given data store file by at least some required amount,
    allocating equivalent memory space for it, returning a pointer to a
    block of at least the required amount.  Currently the entire new growth
    is treated as a single free block and a pointer to it is returned.
    Note that the block returned is nominally 'free' but is not on the
    free list.  Note also that if the data store is volatile (fd == -1)
    there's not actually any file backing the new storage.
*/

private block_t *grow_file(dsh, min_size, st)
dsm_handle     dsh;        /* handle to file to grow */
unsigned long   min_size;   /* size of requested block (minimum grow amount) in bytes */
error_status_t *st;
{
    unsigned long       grow_pages,grow_bytes;  /* number of pages, bytes to grow */
    block_t            *p = NULL;               /* new header */
    file_map_t         *map = NULL;             /* file map entry */
    long                flen;                   /* file length/offset of new chunk */

    CLEANUP {
        if (p != NULL) free(p);                 /* free allocated memory if any */
        if (map != NULL) free(map);
        return NULL;                                 /* return w/ bad status */
    }

    grow_pages = ROUND_UP(min_size,PAGE_SIZE)/PAGE_SIZE;  /* what's minimum to grow in pages? */
    grow_pages = MAX(grow_pages,GROW_PAGES);    /* use larger of that and default */

    grow_bytes = grow_pages*PAGE_SIZE;      /* what's that in dollars? */

    p = (block_t *)malloc(grow_bytes);      /* allocate the memory chunk */
    if (p == NULL) SIGNAL(dsm_err_no_memory);

    map = NEW(file_map_t);                  /* get new file map entry */
    if (map == NULL) SIGNAL(dsm_err_no_memory);

    flen = (dsh->pages+1)*PAGE_SIZE;        /* compute file length/offset of new chunk */

    if (dsh->fd >= 0) {                     /* if a file, set to that position */
        flen = lseek(dsh->fd, flen, L_SET); /* get length of file/offset of new chunk */
        if (flen < 0) SIGNAL(dsm_err_file_io_error);
    }

    map->link = NULL;                       /* this'll be the last link */
    map->loc = p->loc = flen;               /* location of chunk and first block */
    map->size = grow_bytes;                 /* size in bytes */
    map->ptr = p;                           /* where the memory copy is */
    p->size = grow_bytes-PREHEADER;         /* first/only block is everything but one preheader */
    p->isfree = true;                       /* this is a free block */

    if (dsh->map == NULL) dsh->map = map;   /* if this is first map entry fill in */
    else {                                  /* else append to existing list */
        file_map_t  *m;                     /* declare list traverser */
        for (m = dsh->map; m->link != NULL; m = m->link) {};    /* find last link */
        m->link = map;                      /* append new one */
    }

    /*  Now update the file.  We need to write out the newly allocated chunk (so there will be
        something in the file), then update the "pages" count in the file header.  Only that
        many pages "really" exist upon re-opening.
    */

    if (dsh->fd >= 0) { /* only if there's a file, */
        if ((unsigned32)dcethread_write(dsh->fd,p,grow_bytes) != grow_bytes) SIGNAL(dsm_err_file_io_error);

        dsh->pages += grow_pages;           /* update our page count */
        update_file_header(dsh, st);        /* update the file header to include new page count */
    }
    else dsh->pages += grow_pages;          /* maintain page count even if no fd */
                                            
    CLEAR_ST;
    return p;                               /* return the newly allocated block */
} /* grow_file */

/** write_header

    Writes the given block header to the disk file, an atomic operation
    as far as the file is concerned.
*/

private void write_header(dsh, p, st)
dsm_handle     dsh;    /* data store handle */
block_t        *p;      /* block in question */
error_status_t *st;
{
    CLEAR_ST;

    if (dsh->fd < 0) return;    /* noop if no file */

    if (lseek(dsh->fd,p->loc,L_SET) < 0             /* point at the record on disk */
    ||  dcethread_write(dsh->fd,p,PREHEADER) != PREHEADER     /* write the header */
    ||  fsync(dsh->fd) < 0                          /* commit that */
    ) (*st) = dsm_err_file_io_error;              /* error if any fail */
} /* write_header */

/** write_block

    Writes the given block contents including preheader to the disk file,
    an operation which may span multiple pages and is not necessarily
    atomic.  The caller should check this and not attempt a page-spanning
    write unless the block is free.
*/

private void write_block(dsh, p, size, st)
dsm_handle     dsh;    /* data store handle */
block_t        *p;      /* block in question */
unsigned long   size;   /* how much to write */
error_status_t *st;
{
    CLEAR_ST;

    if (dsh->fd < 0) return;    /* noop if no file */

    if (lseek(dsh->fd,p->loc,L_SET) < 0     /* point at the record on disk */
    ||  (unsigned32)dcethread_write(dsh->fd,p,size) != size       /* write the header */
    ||  fsync(dsh->fd) < 0                  /* commit that */
    ) (*st) = dsm_err_file_io_error;      /* error if any fail */
} /* write_block */

/** update_file_header

    Updates (Read/Alter/Rewrite) the header page of the data store file
    to reflect volatile reality.  In particular the size of the data store
    is determined by the page count field in the header, not the length
    of the file.  We could keep a copy of the header around and avoid the
    first read, but currently this doesn't happen much (only when the file
    grows).
*/

private void update_file_header(dsh, st)
dsm_handle   dsh;
error_status_t *st;
{
    file_hdr_t  hdr;    /* data store file header */

    CLEANUP {
        return;
    }

    if (dsh->fd < 0) return;    /* noop if no file */

    /* reset to beginning of file and read in first page (file header).
    */
    if (lseek(dsh->fd,0,L_SET) == -1
    ||  dcethread_read(dsh->fd,&hdr,PAGE_SIZE) != PAGE_SIZE) SIGNAL(dsm_err_file_io_error);

    hdr.pages = dsh->pages; /* update page count */

    /* seek back to the beginning, write out file header, synch the file.
    */
    if (lseek(dsh->fd,0,L_SET) == -1
    ||  dcethread_write(dsh->fd,&hdr,PAGE_SIZE) != PAGE_SIZE
    ||  fsync(dsh->fd) == -1) SIGNAL(dsm_err_file_io_error);

    CLEAR_ST;
 
} /* update_file_header */

/** create_file

    Create a new file of a given name.  Essentially x = create_file(foo) 
    has the semantics of x = open(foo,O_CREAT|O_RDWR,0666), but on older
    Apollo systems (pre-SR10) we need to force the created object to
    'unstruct' not 'uasc' -- the latter includes an invisible stream header
    that throws off our page-alignment assumptions thereby invalidating
    stable storage.
*/

private int create_file(fname)
unsigned char  *fname;
{
#ifdef pre_sr10
#   include <apollo/type_uids.h>
#   include <apollo/ios.h>

    short               namelen;        /* length of filename */
    uid_t              unstruct_uid;   /* uid of object type to create: unstruct */
    ios_create_mode_t  cmode;          /* create mode (treatment of preexisting object) */
    ios_open_options_t oopts;          /* options pertaining to implicit open of created obj */
    ios_id_t           id;             /* returned stream id (== unix fd) */
    error_status_t      status;         /* status of create operation */

    namelen = ustrlen(fname);        /* compute name length for pascal interface */
    unstruct_uid = unstruct_uid;   /* create as unstruct (raw bytes) */
    cmode = ios_recreate_mode;     /* delete any existing object */
    oopts = 0;                      /* no particular open options */

    ios_create(fname,namelen,unstruct_uid,cmode,oopts,&id,&status);    /* do it */
    if (status.all != status_ok) return -1;    /* check status */
    else return (int)id;                        /* else stream id (== fd) of created obj */
#else
    return open((char *) fname, O_CREAT | O_RDWR, 0666);
#endif
} /* create_file */

/** make_free

    Mark a block free and insert it into the free list.
*/

private void make_free(dsh, p, st)
dsm_handle     dsh;    /* data store handle */
block_t        *p;      /* block to free */
error_status_t *st;
{
    p->isfree = true;           /* flag the block free in the header */
    p->cookie = 0;              /* clear cookie -- not a valid user record */
    write_header(dsh, p, st);   /* write that */
    free_block(dsh,p);          /* insert in free list */
} /* make_free */

/** free_block

    Insert a free block into the free list.  The free list is currently
    a single linked list sorted in ascending order of size.  Since the
    allocator returns the first match it sees this means that it will
    always allocate the smallest suitable block.  Experimentation is in
    order, but the rationale here is that allocating a too-large block
    requires i/o to stably fragment the block and again to coalesce
    adjacent free blocks later, while allocation of a suitably small block
    is free of i/o.  The cost of stably changing the disk is such that
    overhead of a) maintaining the list sorted and b) potentially having
    to link through many small blocks to find a big one is deemed
    acceptable.  *** experiment.
*/

private void free_block(dsh,ptr)
dsm_handle     dsh;    /* data store handle */
block_t        *ptr;    /* block to free */
{
    block_t *p,*pp;     /* list traversers */

    pp = NULL;                          /* haven't seen any blocks yet */
    for (p = dsh->freelist; p != NULL; p = p->link) {   /* look at each block on free list */
        if (ptr->size <= p->size) break; /* if new block is smaller than this one, insert here */
        pp = p;
    }

    if (pp) pp->link = ptr;             /* in the list link previous node around */
    else dsh->freelist = ptr;           /* inserting at beginning of list, replace freelist */

    ptr->link = p;                      /* followed by larger block or NULL at EOL */
} /* free_block */

/** free_map

    Given a pointer to a memory map list, frees all associated memory (both
    mapped memory and the map records themselves) in that list.
*/

private void free_map(m)
file_map_t *m;
{
    if (m == NULL) return;          /* all done for an empty list */
    if (m->link) free_map(m->link); /* first free the tail of the list if any */

    free(m->ptr);                   /* free mapped memory */
    free(m);                        /* free the map record */
} /* free_map */

/** coalesce

    Traverses the file in order merging adjacent free blocks into single larger
    free blocks.  Rebuilds the free list in the process.  Two adjacent blocks
    are merged by simply incrementing the size of the first one to subsume the
    second.  Note that coalescing does not occur across allocation chunks; each
    chunk is separately coalesced.
*/

private void coalesce(dsh, st)
dsm_handle      dsh;  /* data store handle */
error_status_t *st;
{
    file_map_t *map;        /* for traversing chunk list */
    block_t    *this,*next; /* current block, next block */

    CLEANUP {
        return;
    }
    CLEAR_ST;

    /* if there are any pending allocations (allocated records that have not
       been committed with a write (or free), then we should not coalesce,
       as we could destroy those pending allocations.
    */
    if (dsh->pending > 0) return;

    dsh->freelist = NULL;   /* clear existing freelist */
    cache_clear(dsh);       /* clear dsm_read cache */
    dsh->coalesced = true;  /* can't play cache tricks anymore */

    for (map = dsh->map; map != NULL; map = map->link) {    /* for each file chunk */

        /*  The idea is this: consider this block and the next one.  If
            they're both free, join them by increasing this block's size,
            and iterate with a new larger 'this', looking at the new
            'next'.  If they're not both free, iterate with 'next' as the
            current block, after adding 'this' to the free list (it's as
            big as it's going to get).
        */
        this = map->ptr;                                /* start with 1st block in chunk */
        for (;;) {                                      /* until explicit break at end of chunk */
            next = (block_t *) (((char *)this)+this->size+PREHEADER);   /* find next block */
            if ((unsigned)((char *)next-(char *)map->ptr) >= map->size) {   /* is this the last block? */
                if (this->isfree) free_block(dsh,this); /* add it to free list if free */
                break;                                  /* finished with this chunk */
            }
            if (this->isfree && next->isfree) {         /* if this and next are both free */
                this->size += next->size + PREHEADER;   /* join the two */
                write_header(dsh, this, st);            /* stably */
                PROP_BAD_ST;
                next->link = NULL;                      /* avoid dangling pointers */
            }
            else {                                      /* else move on */
                if (this->isfree) free_block(dsh,this); /* if this is free, add to free list */
                this = next;                            /* iterate looking at next */
            }
        }
    }
    CLEAR_ST;
} /* coalesce */

/** build_freelist

    Called after opening a data store file this traverses the file building
    the free list.
*/

private void build_freelist(dsh)
dsm_handle   dsh;  /* data store handle */
{
    file_map_t *map;        /* for traversing chunk list */
    block_t    *this,*next; /* current block, next block */

    dsh->freelist = NULL;   /* clear existing freelist */

    for (map = dsh->map; map != NULL; map = map->link) {    /* for each file chunk */

        this = map->ptr;                                /* start with 1st block in chunk */
        for (;;) {                                      /* until explicit break at end of chunk */
            if (this->isfree) free_block(dsh,this); /* if this is free, add to free list */

            next = (block_t *) (((char *)this)+this->size+PREHEADER);   /* find next block */
            if ((unsigned)((char *)next-(char *)map->ptr) >= map->size) break;   /* is this the last block? */
            
            this = next;                            /* iterate looking at next */
        }
    }
} /* build_freelist */

/** get_next_block

    Returns (a pointer to) the next non-free block after the one given, or NULL
    if there are none.
*/

private block_t *get_next_block(dsh,this)
dsm_handle     dsh;
block_t        *this;
{
    file_map_t     *map;        /* for traversing chunk list */
    block_t        *next;       /* next block */
    unsigned long   next_loc;   /* file location of next block */
    unsigned long   offset;     /* offset within a chunk */

    next_loc = this->loc+PREHEADER+this->size;              /* where next block is in the file */

    for (map = dsh->map; map != NULL; map = map->link) {    /* for each file chunk */
        while (map->loc+map->size > next_loc) {             /* is this the one? */
            offset = next_loc-map->loc;                     /* compute position within chunk */
            next = (block_t *) (((char *)map->ptr)+offset); /* develop pointer */
            if (!next->isfree) return next;                 /* if it's free, take it */
            next_loc += PREHEADER+next->size;               /* compute next block after this */
        }
    }

    return NULL;    /* ran off the end without finding it */

} /* get_next_block */

/** block_from_ptr

    Given a pointer from a user (i.e. what ought to be a pointer to the
    user data in a block), return a pointer to the block it's in, after
    basic validation.  Caller should have a cleanup handler in place to
    catch possible memory faults as we attempt the dereference.
*/

private block_t *block_from_ptr(ptr, st)
void *  ptr;    /* user pointer */
error_status_t *st;
{
    block_t *p; /* the block preheader */

    CLEANUP {
        return NULL;
    }

    if (ptr == NULL) SIGNAL(dsm_err_invalid_pointer);  /* basic sanity check */

    p = (block_t *)(((char *)ptr)-PREHEADER);  /* calculate block pointer */

    if (p->cookie != HDR_COOKIE) SIGNAL(dsm_err_invalid_pointer);  /* check magic cookie */

    CLEAR_ST;
    return p;           /* got it */

} /* block_from_ptr */

/** get_earlier_block

    Given a file location (marker), return a pointer to a valid block whose
    location is earlier (so we can traverse valid blocks looking for the one
    given which lost on a cache miss).  There are numerous heuristics we
    could use to find the best (i.e. closest to the marker) block.  As of
    now though we just return the first block.
*/

private block_t *get_earlier_block(dsh,marker)
dsm_handle     dsh;
dsm_marker_t   marker ATTRIBUTE_UNUSED;
{
    if (dsh->map) return dsh->map->ptr; /* if any allocated memory use 1st block */
    else return NULL;                   /* otherwise, no go: NULL */
} /* get_earlier_block */

/*  cache handling

    We maintain, per database, a cache of location/pointer values for use
    by dsm_read.  The cache is cleared initially and upon coalesce(),
    and the location/pointer pair returned by successful dsm_read is 
    entered in the cache.  The size of the cache depends on the expected
    pattern of dsm_read calls.

    Currently the cache is trivial: one deep.  This assumes that dsm_read
    will be called sequentially to read the database in a single thread,
    there won't be multiple tasks simultaneously reading the same database,
    and markers to older records won't be saved.  If any of these occur
    then the cost of dsm_read will go up, as many records must be scanned
    (depending on get_earlier_block()) for each cache miss.

    We have a special case for lookups in a recently opened data store - one
    that's still in a single memory chunk and which has not been coalesced;
    in this case the address can be computed from the chunk address and the
    file offset.  (This could be extended beyond a single chunk but since we
    always coalesce before allocating a new chunk it avails not).  Once you've
    coalesced the boundaries of blocks have changed, and stale pointers or
    markers could point other than to the beginning of a block.
*/

/** cache_clear

    Clear all entries in the cache to invalid.
*/

private void cache_clear(dsh)
dsm_handle     dsh;
{
    dsh->cache.loc = MAGIC_MARKER;
} /* cache_clear */

/** cache_add

    Add an entry to the cache.
*/

private void cache_add(dsh,p,mkr)
dsm_handle     dsh;
block_t        *p;
dsm_marker_t   mkr;
{
    dsh->cache.loc = mkr;
    dsh->cache.p = p;
} /* cache_add */    

/** cache_lookup

    Look up a given location (marker) in the cache; if found, return the
    corresponding block pointer, otherwise return NULL.
*/

private block_t *cache_lookup(dsh,mkr)
dsm_handle     dsh;
dsm_marker_t   mkr;
{
    if (dsh->cache.loc == mkr) return dsh->cache.p; /* in the cache? */
    else if (!dsh->coalesced && dsh->map != NULL && dsh->map->link == NULL) {
        long offset;

        offset = mkr - dsh->map->loc;   /* bytes into chunk */
        return (block_t *)((char *)dsh->map->ptr + offset);
    }
    else return NULL;
} /* cache_lookup */

