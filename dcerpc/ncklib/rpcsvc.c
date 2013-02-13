/*
 * 
 * (c) Copyright 1991 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1991 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1991 DIGITAL EQUIPMENT CORPORATION
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

#include <ctype.h>
#include <commonp.h>
#include <string.h>
#include <rpcsvc.h>
#include <stdarg.h>

/*
dce_svc_handle_t rpc_g_svc_handle;
*/
DCE_SVC_DEFINE_HANDLE(rpc_g_svc_handle, rpc_g_svc_table, "rpc")
      

/*
 * R P C _ _ S V C _ E P R I N T F
 *
 * Format and print arguments as a serviceability
 * debug message.
 */

PRIVATE int rpc__svc_eprintf ( char *fmt, ... )
{
    char	buf[RPC__SVC_DBG_MSG_SZ];
    va_list	arg_ptr;

    va_start (arg_ptr, fmt);
    vsprintf (buf, fmt, arg_ptr);
    va_end (arg_ptr);
    DCE_SVC_DEBUG((RPC__SVC_HANDLE, rpc_svc_general, RPC__SVC_DBG_LEVEL(0), buf));
    return(0);
}


/*
 * R P C _ _ S V C _ I N I T
 *
 * Do initialization required for serviceability
 */

PRIVATE void rpc__svc_init ( void )
{
    error_status_t status;

    /*
     * Currently, all we have to do is return, since
     * everything is statically registered.
     * 
     * But someday we might do something like turn
     * on debug levels corresponding to things set
     * in rpc_g_dbg_switches[], or ...
     */

    /*
     * This silliness is a placeholder, so that we
     * remember to do things differently in the kernel
     * if we ever decide to do more than just return
     * out of this routine.
     */
    return;
}

#ifdef DEBUG
/*
 * R P C _ _ S V C _ F M T _ D B G _ M S G
 *
 * This routine takes the printf "pargs" passed to
 * the RPC_DBG_PRINTF() macro and formats them
 * into a string that can be handed to DCE_SVC_DEBUG.
 *
 * This is necessary because the pargs are passed
 * in as a single, parenthesized argument -- which
 * also requires that the resulting string be passed
 * back as a pointer return value.
 *
 * The returned pointer must be free()'ed by the
 * caller (see comments at malloc() below).  This
 * should be fairly safe, since this routine should
 * only ever be called by RPC_DBG_PRINTF.
 */

PRIVATE char * rpc__svc_fmt_dbg_msg (char *format, ...)
{
    char            *bptr;
    va_list         arg_ptr;

    /*
     * Using malloc here is ugly but necessary.  The formatted
     * string must be passed back as a pointer return value.  The
     * possibility of recursive calls due to evaluation of pargs
     * (where, e.g., one of the pargs is a call to a routine that
     * calls RPC_DBG_PRINTF) preclude an implementation using a
     * mutex to protect a static buffer.  The potential for infinite
     * recursion precludes allocating memory using internal RPC
     * interfaces, since those interfaces call RPC_DBG_PRINTF.
     */

    if( (bptr = malloc(RPC__SVC_DBG_MSG_SZ*sizeof(char))) == NULL )
    {
	/* die horribly */
	abort();
    }

    va_start (arg_ptr, format);
    vsprintf (bptr, format, arg_ptr);
    va_end (arg_ptr);

    return( bptr );
}
#endif	/* DEBUG */
