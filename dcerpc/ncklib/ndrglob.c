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
**      ndrglob.c
**
**  FACILITY:
**
**      Network Data Representation (NDR)
**
**  ABSTRACT:
**
**  Runtime global variable definitions.
**
**
*/

#include <commonp.h>
#include <ndrp.h>
#include <ndrglob.h>

GLOBAL u_char ndr_g_local_drep_packed[4] = {
    (NDR_LOCAL_INT_REP << 4) | NDR_LOCAL_CHAR_REP,
    NDR_LOCAL_FLOAT_REP,
    0,
    0,
};

GLOBAL 
#ifdef VMS
/* Provide psect name if VMS */
{"ndr_g_local_drep"} 
#endif
ndr_format_t ndr_g_local_drep = {
    NDR_LOCAL_INT_REP,
    NDR_LOCAL_CHAR_REP,
    NDR_LOCAL_FLOAT_REP,
    0
};

GLOBAL rpc_transfer_syntax_t ndr_g_transfer_syntax = {
    {
        {
            0x8a885d04U, 0x1ceb, 0x11c9, 0x9f, 0xe8, 
            {0x8, 0x0, 0x2b, 0x10, 0x48, 0x60}
        },
        2
    },
    0,
    NULL
};
