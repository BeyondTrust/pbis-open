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
**      rpclog.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  Definitions of global variables.
**
**
*/

#if HAVE_CONFIG_H
#include <config.h>
#endif



#ifdef LOGPTS
#include <rpclog.h>

#ifdef ultrix
#include <nlist.h>
#include <unistd.h>
#endif /* ultrix */

static rpc_logpt_t      logpt_invisible;
rpc_logpt_ptr_t         rpc_g_log_ptr = &logpt_invisible;

/*
**++
**
**  ROUTINE NAME:       rpc__log_ptr_init
**
**  SCOPE:              PRIVATE - declared in rpclog.h
**
**  DESCRIPTION:
**
**  This routine will initialize the RPC logging service.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     
**
**      log pointer     pointer value to location to which codes to
**                      be timestamped are written.
**
**  SIDE EFFECTS:       none
**
**--
**/

rpc_logpt_ptr_t rpc__log_ptr_init (void)
#ifdef ultrix
{
    rpc_logpt_ptr_t     ptr;
    unsigned long       logpt_addr_in_virt_mem;

#define QMEM_X 0
    struct nlist symtab[QMEM_X + 2];

    symtab[QMEM_X].n_name = "_qmem";
    symtab[QMEM_X + 1].n_name = NULL;

    nlist ("/vmunix", symtab);
    logpt_addr_in_virt_mem = (symtab[QMEM_X].n_value + LOGPT_ADDR_IN_QMEM);
    ptr = (rpc_logpt_ptr_t) (logpt_addr_in_virt_mem);

    return (ptr);
}

#endif /* ultrix */


#else
#ifndef __GNUC__
/*
 *  ANSI c does not allow a file to be compiled without declarations.  
 *  If LOGPTS is not defined, we need to declare a dummy variable to
 *  compile under strict ansi c standards.
 */
static  char    _rpclog_dummy_ = 0, *_rpclog_dummy_p = &_rpclog_dummy_;
#endif
#endif /* LOGOPTS */
