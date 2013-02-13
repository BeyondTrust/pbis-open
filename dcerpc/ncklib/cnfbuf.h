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
#ifndef _CNFBUF_H
#define _CNFBUF_H 1

/*
**
**  NAME
**
**      cnfbuf.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Definitions of types and interfaces to the fragment buffer
**  routines for connection based protocol services.
**
**
*/

/*
 * NOTE: rpc_c_cn_large_frag_size must always be at least
 * rpc_c_assoc_must_recv_frag_size as defined in cnassm.h. This is
 * an architectural requirement.
 */
#include <cnassm.h>

#if (RPC_C_CN_LARGE_FRAG_SIZE < RPC_C_ASSOC_MUST_RECV_FRAG_SIZE)
#error "large frag size < architecural minimum"
#endif

#define	RPC_C_CN_LG_FRAGBUF_ALLOC_SIZE (sizeof(rpc_cn_fragbuf_t)\
        + RPC_C_CN_LARGE_FRAG_SIZE - 1)

#define RPC_C_CN_SM_FRAGBUF_ALLOC_SIZE (sizeof(rpc_cn_fragbuf_t)\
        + RPC_C_CN_SMALL_FRAG_SIZE - 1 )

EXTERNAL unsigned32 rpc_g_cn_large_frag_size;


/***********************************************************************/
/*
 * R P C _ C N _ F R A G B U F _ P K T _ H D R
 *
 * The unpacked header for a received fragment starts at the used 
 * portion of the header overhead area.
 */

#define RPC_CN_FRAGBUF_PKT_HDR(fbuf) \
    (rpc_cn_packet_p_t) ((rpc_cn_fragbuf_p_t)(fbuf))->data_p

/***********************************************************************/
/*
 * R P C _ C N _ F R A G B U F _ A L L O C
 *
 */

#define RPC_CN_FRAGBUF_ALLOC(fragbuf, size, st)\
    if ((size) <= RPC_C_CN_SMALL_FRAG_SIZE)\
    {\
        (fragbuf) = rpc__cn_fragbuf_alloc (false);\
    }\
    else\
    {\
        (fragbuf) = rpc__cn_fragbuf_alloc (true);\
    }\
    (fragbuf)->data_size = (size);\
    *(st) = rpc_s_ok;

/***********************************************************************/
/*
 * R P C _ C N _ F R A G B U F _ S E T _ D A T A _ P
 *
 */

#define RPC_CN_FRAGBUF_SET_DATA_P(fbp)\
    (fbp)->data_p = (pointer_t) RPC_CN_ALIGN_PTR((fbp)->data_area, 8);

/***********************************************************************/
/*
 * R P C _ _ C N _ F R A G B U F _ F R E E
 *
 */

void rpc__cn_fragbuf_free (rpc_cn_fragbuf_p_t	/*fragbuf_p*/);


/***********************************************************************/
/*
 * R P C _ _ C N _ S M F R A G B U F _ F R E E
 *
 */

void rpc__cn_smfragbuf_free (rpc_cn_fragbuf_p_t /*fragbuf_p*/);


/***********************************************************************/
/*
 * R P C _ _ C N _ F R A G B U F _ A L L O C
 *
 */
#define RPC_C_CN_LARGE_FRAGBUF true
#define RPC_C_CN_SMALL_FRAGBUF false

rpc_cn_fragbuf_p_t rpc__cn_fragbuf_alloc (
    boolean32               /* alloc_large_buf */);


/***********************************************************************/
/*
 * R P C _ _ C N _ F R A G B U F _ A L L O C _ D Y N
 *
 */
rpc_cn_fragbuf_p_t rpc__cn_fragbuf_alloc_dyn (
    unsigned32               /* alloc_size */);

#endif /* _CNFBUF_H */
