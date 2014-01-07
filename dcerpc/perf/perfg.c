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
**      rpcp.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Server manager routines for performance and system execiser
**  auxiliary interface. This interface is generic.
**
**
*/

#include <perf_c.h>
#include <perf_p.h>

/***************************************************************************/

void foo_perfg_op1 (h, n, x)

handle_t                h __attribute__((unused));
unsigned long           n;
unsigned long           *x;

{
    *x = 2 * n;
}


/***************************************************************************/

void foo_perfg_op2 (h, n, x)

handle_t                h __attribute__((unused));
unsigned long           n;
unsigned long           *x;

{
    *x = 3 * n;
}


/***************************************************************************/

perfg_v1_0_epv_t foo_perfg_epv =
{
    foo_perfg_op1,
    foo_perfg_op2
};

/***************************************************************************/

void bar_perfg_op1 (h, n, x)

handle_t                h __attribute__((unused));
unsigned long           n;
unsigned long           *x;

{
    *x = 4 * n;
}


/***************************************************************************/

void bar_perfg_op2 (h, n, x)

handle_t                h __attribute__((unused));
unsigned long           n;
unsigned long           *x;

{
    *x = 5 * n;
}


/***************************************************************************/

perfg_v1_0_epv_t bar_perfg_epv =
{
    bar_perfg_op1,
    bar_perfg_op2
};
