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
#ifndef RPCDP_H
#define RPCDP_H

/*
**
**  NAME:
**
**      rpcdp.h
**
**  FACILITY:
**
**      RPC Daemon
**
**  ABSTRACT:
**
**  RPC Daemon "private" types, defines, ...
**
**
*/

/*
** Useful macros
*/

#define STATUS_OK(s) ((s)==NULL || *(s) == rpc_s_ok)
#define SET_STATUS(s,val) *(s) = val
#define SET_STATUS_OK(s) SET_STATUS(s, error_status_ok)
#define STATUS(s) *(s)

EXTERNAL dce_uuid_t nil_uuid;
EXTERNAL boolean32 dflag;

typedef enum {warning, fatal, fatal_usage} check_mode_t;

PRIVATE boolean32 check_st_bad
    (
        char            *str,
        error_status_t  *st
    );

PRIVATE void show_st
    (
        char            *str,
        error_status_t  *st
    );

#endif

