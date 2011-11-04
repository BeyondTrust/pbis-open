/*
 * 
 * (c) Copyright 1990 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1990 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1990 DIGITAL EQUIPMENT CORPORATION
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
**      cnmgmt.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  The NCA Connection Protocol Service's Management Service.
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comprot.h>    /* Common protocol services */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnmgmt.h>


/*
**++
**
**  ROUTINE NAME:       rpc__cn_mgmt_init
**
**  SCOPE:              PRIVATE - declared in cnmgmt.h,
**                                called from cninit.
**
**  DESCRIPTION:
**
**  Initialize the Connection management data collection
**  registers.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   Management counters cleared. 
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_mgmt_init (void)
{
    memset (&rpc_g_cn_mgmt, 0, sizeof (rpc_g_cn_mgmt));
}

/*
**++
**
**  Routine NAME:       rpc__cn_mgmt_inq_calls_sent
**
**  SCOPE:              PRIVATE - declared in cnmgmt.h
**
**  DESCRIPTION:
**
**  Report the total number of RPC that have been sent by
**  the NCA Connection Protocol.
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
**      return          The number of RPCs sent through the NCA
**                      Connection Protocol Service.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32 rpc__cn_mgmt_inq_calls_sent (void)

{
    return (rpc_g_cn_mgmt.calls_sent);
}



/*
**++
**
**  Routine NAME:       rpc__cn_mgmt_inq_calls_rcvd
**
**  SCOPE:              PRIVATE - declared in cnmgmt.h
**
**  DESCRIPTION:
**
**  Report the total number of RPCs that have been received by
**  the NCA Connection Protocol.
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
**      return          The number of RPCs received through the NCA
**                      Connection Protocol Service.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32 rpc__cn_mgmt_inq_calls_rcvd (void)

{
    return (rpc_g_cn_mgmt.calls_rcvd);
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_mgmt_inq_pkts_sent
**
**  SCOPE:              PRIVATE - declared in cnmgmt.h
**
**  DESCRIPTION:
**
**  Report the total number of packets that have been sent by
**  the NCA Connection Protocol.
**
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
**      return          The number of RPC packets sent by the NCA
**                      Connection Protocol Service.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32 rpc__cn_mgmt_inq_pkts_sent (void)

{

    return (rpc_g_cn_mgmt.pkts_sent);
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_mgmt_inq_pkts_rcvd
**
**  SCOPE:              PRIVATE - declared in cnmgmt.h
**
**  DESCRIPTION:
**
**  Report the total number of packets that have been received by
**  the NCA Connection Protocol.
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
**      return          The number of RPC packets received by
**                      the NCA Connection Protocol Service.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32 rpc__cn_mgmt_inq_pkts_rcvd (void)
{
    return (rpc_g_cn_mgmt.pkts_rcvd);
}
