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
#ifndef RPCDEPDBP_H
#define RPCDEPDBP_H
/*
**
**  NAME:
**
**      rpcdepdbp.h
**
**  FACILITY:
**
**      RPC Daemon
**
**  ABSTRACT:
**
**      RPCD Endpoint Database Mgmt - routines, etc. shared by modules
**      which know more about epdb internals
**      
**
**
*/


/*  
 *  Delete disk copy of entry and free associated
 *  memory
 */        
PRIVATE void epdb_delete_entry
    (
        struct db       *h,
        db_entry_p_t    entp,
        error_status_t  *status
    );
        

PRIVATE void sliv_init
    (
        struct db       *h,
        error_status_t  *status
    );

#endif
