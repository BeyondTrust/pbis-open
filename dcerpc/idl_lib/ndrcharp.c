/*
 * 
 * (c) Copyright 1993 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1993 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1993 DIGITAL EQUIPMENT CORPORATION
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
**      ndrcharp.c
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**  This module contains the declarations of two character table pointers
**  used by the stub support library for translating to/from ascii/ebcdic.
**
**  VERSION: DCE 1.0
**
*/

#if HAVE_CONFIG_H
#include <config.h>
#endif


/* The ordering of the following 3 includes should NOT be changed! */
#include <dce/rpc.h>
#include <dce/stubbase.h>
#include <lsysdep.h>

/*
 * Pointer cells for the two tables.  Note that for a VMS shareable image
 * version of the runtime, these two cells must be moved to a separate
 * module at the head of the image.
 */
#if defined(VMS) && defined(VAX)
globaldef {"NDR_G_ASCII_TO_EBCDIC"} rpc_trans_tab_p_t ndr_g_ascii_to_ebcdic;
globaldef {"NDR_G_EBCDIC_TO_ASCII"} rpc_trans_tab_p_t ndr_g_ebcdic_to_ascii;
#else
globaldef rpc_trans_tab_p_t ndr_g_ascii_to_ebcdic;
globaldef rpc_trans_tab_p_t ndr_g_ebcdic_to_ascii;
#endif

/* 
** For alpha/VMS we need to export the globals to match the externs in
** stubbase.h because the default common model is ANSI-style
*/
#if defined(VMS) && defined(__alpha)
ndr_boolean rpc_ss_allocate_is_set_up = ndr_false;
ndr_boolean rpc_ss_context_is_set_up = ndr_false;
ndr_boolean rpc_ss_client_is_set_up = ndr_false;
ndr_boolean rpc_ss_server_is_set_up = ndr_false;
#endif

