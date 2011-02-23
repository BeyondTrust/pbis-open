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
#ifndef _CNID_H
#define _CNID_H	1
/*
**
**  NAME
**
**      cnid.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Interface to the Local Identifier Service.
**
**
*/

/*
 * R P C _ C N _ L O C A L _ I D _ E Q U A L
 */
#define RPC_CN_LOCAL_ID_EQUAL(id1, id2)\
    ((id1.parts.id_seqnum == id2.parts.id_seqnum) &&\
     (id1.parts.id_index == id2.parts.id_index)) 

/*
 * R P C _ C N _ L O C A L _ I D _ V A L I D
 */

#define RPC_CN_LOCAL_ID_VALID(id) (id.parts.id_seqnum != 0)

/*
 * R P C _ C N _ L O C A L _ I D _ C L E A R
 */
#define RPC_CN_LOCAL_ID_CLEAR(id)\
{\
    id.parts.id_seqnum = 0;\
    id.parts.id_index = 0;\
}

/*
 * R P C _ _ C N _ I N I T _ S E Q N U M
 *
 * This routine initializes the global sequence number cell and
 * corresponding mutex.
 */

void rpc__cn_init_seqnum (void);

/*
 * R P C _ _ C N _ G E N _ L O C A L _ I D
 *
 * This routine creates a new local identifier.
 */

void rpc__cn_gen_local_id (
    unsigned32          /* index */,
    rpc_cn_local_id_t   * /* lcl_id */);

#endif /* _CNID_H */
