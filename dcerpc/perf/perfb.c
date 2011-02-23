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
**  NAME
**
**      perfb.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Server manager routines for performance and system execiser auxiliary
**  interface.  This interface is dynamically registered by the server when
**  request by the client through a call to an operation in the "perf"
**  interface.
**
**
*/

#include <perf_c.h>
#include <perf_p.h>
#include <unistd.h>

void print_binding_info(char *text, handle_t h);

perfb_v1_0_epv_t perfb_mgr_epv =
{
    perfb_init,
    perfb_in,
    perfb_brd,
    perfb_null,
    perfb_null_idem
};


/***************************************************************************/

void perfb_init 
(
    handle_t                h,
    idl_char                *name
)
{
    print_binding_info ("perfb_init", h);
    gethostname(name, 256);
}

/***************************************************************************/

void perfb_in 
(
    handle_t                h,
    perf_data_t             d,
    unsigned32              l,
    idl_boolean             verify,
    unsigned32           *sum
)
{
    print_binding_info ("perfb_in", h);
    perf_in(h, d, l, verify, sum);
}

/***************************************************************************/

void perfb_brd 
(
    handle_t                h,
    idl_char                *name
)
{
    print_binding_info ("perfb_brd", h);
    gethostname(name, 256);
}

/***************************************************************************/

void perfb_null
(
    handle_t                h __attribute__((unused))
)
{
}

/***************************************************************************/

void perfb_null_idem
(
    handle_t                h __attribute__((unused))
)
{
}


