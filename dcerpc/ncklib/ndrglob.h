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
#ifndef _NDRGLOB_H
#define _NDRGLOB_H 1
/*
**
**  NAME:
**
**      ndrglob.h
**
**  FACILITY:
**
**      Network Data Representation (NDR)
**
**  ABSTRACT:
**
**  Runtime global variable (external) declarations.
**
**
*/

#include <sys/types.h>

    /*
     * Local data representation.
     *
     * "ndr_g_local_drep" is what stubs use when they're interested in
     * the local data rep.  "ndr_local_drep_packed" is the actual correct
     * 4-byte wire format of the local drep, suitable for copying into
     * packet headers.
     */

EXTERNAL u_char ndr_g_local_drep_packed[4];

EXTERNAL ndr_format_t ndr_g_local_drep;

    /*
     * A constant transfer syntax descriptor that says "NDR".
     */

EXTERNAL rpc_transfer_syntax_t ndr_g_transfer_syntax;

#endif /* _NDRGLOB_H */
