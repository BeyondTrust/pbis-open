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
#ifndef _CNMGMT_H
#define _CNMGMT_H	1
/*
**
**  NAME
**
**      cnmgmt.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Interface to the NCA Connection Protocol Service's Management Service.
**
**
*/


#include <dce/dce.h>

/*
 * R P C _ _ C N _ M G M T _ I N I T
 */

PRIVATE void rpc__cn_mgmt_init (void);

/*
 * R P C _ _ C N _ M G M T _ I N Q _ C A L L S _ S E N T
 */

PRIVATE unsigned32 rpc__cn_mgmt_inq_calls_sent (void);

/*
 * R P C _ _ C N _ M G M T _ I N Q _ C A L L S _ R C V D 
 */

PRIVATE unsigned32 rpc__cn_mgmt_inq_calls_rcvd (void);

/*
 * R P C _ _ C N _ M G M T _ I N Q _ P K T S _ S E N T
 */

PRIVATE unsigned32 rpc__cn_mgmt_inq_pkts_sent (void);

/*
 * R P C _ _ C N _ M G M T _ I N Q _ P K T S _ R C V D
 */

PRIVATE unsigned32 rpc__cn_mgmt_inq_pkts_rcvd (void);

#endif /* _CNMGMT_H */
