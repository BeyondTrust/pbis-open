/*
 * Copyright (c) 2010 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Portions of this software have been released under the following terms:
 *
 * (c) Copyright 1989-1993 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1989-1993 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1989-1993 DIGITAL EQUIPMENT CORPORATION
 *
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 * permission to use, copy, modify, and distribute this file for any
 * purpose is hereby granted without fee, provided that the above
 * copyright notices and this notice appears in all source code copies,
 * and that none of the names of Open Software Foundation, Inc., Hewlett-
 * Packard Company or Digital Equipment Corporation be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Neither Open Software
 * Foundation, Inc., Hewlett-Packard Company nor Digital
 * Equipment Corporation makes any representations about the suitability
 * of this software for any purpose.
 *
 * Copyright (c) 2007, Novell, Inc. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Novell Inc. nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @APPLE_LICENSE_HEADER_END@
 */
/*
**
**  NAME
**
**      cnpkt.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  Connection-based protocol packet packing/unpacking routines.
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comprot.h>    /* Common protocol services */
#include <ndrp.h>       /* System (machine architecture) dependent definitions */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnpkt.h>	/* NCA Connection packet encoding */

/*
 * R P C _ G _ C N _ C O M M O N _ H D R 
 *
 * This is a template of the common part of all NCA connection
 * packet headers. It is statically initialized where possible. This
 * is used by the rpc__cn_pkt_format_common routine.
 */
GLOBAL rpc_cn_common_hdr_t rpc_g_cn_common_hdr =
{

    RPC_C_CN_PROTO_VERS,        /* rpc_vers */
    RPC_C_CN_PROTO_VERS_MINOR,  /* rpc_vers_minor */
    RPC_C_CN_PKT_INVALID,       /* ptype */
    0,                          /* flags */
    {                           /* drep[4] */
        (NDR_LOCAL_INT_REP << 4) | NDR_LOCAL_CHAR_REP,
        NDR_LOCAL_FLOAT_REP,
        0,
        0
    },
    0,                          /* frag_len */
    0,                          /* auth_len */
    0                           /* call_id  */
};


INTERNAL rpc_cn_pres_result_list_p_t unpack_port_any (
        rpc_cn_port_any_t       *port_any_p,
        unsigned8               *drepp,
        unsigned8               *end_of_pkt,
        unsigned32              *st
        );

INTERNAL rpc_cn_auth_tlr_p_t unpack_pres_context_list (
        rpc_cn_pres_cont_list_p_t   pcontp,
        boolean32                   swap,
        unsigned8                   *end_of_pkt,
        unsigned32                  *st
        );

INTERNAL rpc_cn_auth_tlr_p_t unpack_pres_result_list (
        rpc_cn_pres_result_list_p_t presp,
        boolean32                   swap,
        unsigned8                   *end_of_pkt,
        unsigned32                  *st
        );

INTERNAL rpc_cn_auth_tlr_p_t unpack_versions_supported (
        rpc_cn_versions_supported_p_t /*versp*/
        );

void SWAP_INPLACE_UUID
(
    dce_uuid_t   *uuid_p,
    unsigned8    *end_of_pkt,
    unsigned32   *st
)
{
    SWAP_INPLACE_32(&uuid_p->time_low, end_of_pkt, st);
    if (*st == rpc_s_ok)
    {
        SWAP_INPLACE_16(&uuid_p->time_mid, end_of_pkt, st);
    }
    if (*st == rpc_s_ok)
    {
        SWAP_INPLACE_16(&uuid_p->time_hi_and_version, end_of_pkt, st);
    }
}

void SWAP_INPLACE_SYNTAX
(
    rpc_cn_pres_syntax_id_p_t   syntax_p,
    unsigned8                   *end_of_pkt,
    unsigned32                  *st
)
{
    SWAP_INPLACE_UUID(&syntax_p->id, end_of_pkt, st);
    if (*st == rpc_s_ok)
    {
        SWAP_INPLACE_32(&syntax_p->version, end_of_pkt, st);
    }
}

/*
**++
**
**  ROUTINE NAME:       unpack_port_any
**
**  SCOPE:              INTERNAL - declared in cnpkt.c
**
**  DESCRIPTION:
**      
**  This routine unpacks the 'port_any' secondary address field of an
**  RPC connection packet and returns a pointer to the next octet following
**  the port_any field.
**
**  INPUTS:
**
**      port_any_p	pointer to the 'port_any' field within the packet
**	drepp		pointer to the data representation
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
**      return          pointer to the next octet after the 'port_any' field
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL rpc_cn_pres_result_list_p_t unpack_port_any 
(
  rpc_cn_port_any_t       *port_any_p,
    unsigned8               *drepp,
    unsigned8               *end_of_pkt,
    unsigned32              *st
)
{
    unsigned8 * string_end;
    union
    {                                  /* a "proper" union to shut up lint */
        unsigned8 *string;             /* a string pointer */
        rpc_cn_pres_result_list_p_t rtn;        /* a return value */
    }   ptr;
    
    *st = rpc_s_ok;

    ptr.string = port_any_p->s;        /* init our string pointer */

    /*
     * byte-swap the length field if the endian's are different
     */
    if (NDR_LOCAL_INT_REP != NDR_DREP_INT_REP (drepp))
    {
        SWAP_INPLACE_16 (&port_any_p->length, end_of_pkt, st);
        if (*st != rpc_s_ok)
        {
            return (NULL);
        }
    }

    /*
     * only do the inplace string conversion if the two drep's are different
     */
    if (NDR_LOCAL_CHAR_REP != NDR_DREP_CHAR_REP (drepp))
    {
        string_end = ptr.string + port_any_p->length;
        if ( (string_end < ptr.string) || (end_of_pkt < string_end) )
        {
            *st = rpc_s_bad_pkt;
            return (NULL);
        }

	rpc_util_strcvt ( NDR_LOCAL_CHAR_REP == ndr_c_char_ascii,
			  port_any_p->length,
			  ptr.string,
			  ptr.string );
    }

    /*
     * point to just beyond the end of the string and return that value
     * as our return value
     */
    ptr.string += port_any_p->length;
    return (ptr.rtn);
}


/*
**++
**
**  ROUTINE NAME:       unpack_pres_context_list
**
**  SCOPE:              INTERNAL - declared in cnpkt.c
**
**  DESCRIPTION:
**      
**  This routine unpacks a presentation context list in an RPC connection
**  packet and returns a pointer to the next octet following the list.
**
**  INPUTS:
**
**      pcontp		pointer to the presentation context list
**	swap		boolean indicating we need to perform byte swapping
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**      return          pointer to the next octet following the presentation
**			context list
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL rpc_cn_auth_tlr_p_t unpack_pres_context_list 
(
  rpc_cn_pres_cont_list_p_t       pcontp,
    boolean32                     swap,
    unsigned8                     *end_of_pkt,
    unsigned32                    *st
)
{
    unsigned8 n;                       /* presentation context list element
                                        * loop count */
    unsigned8 id;                      /* transfer syntax list loop count */
    unsigned8 tsn;                     /* number of transfer syntaxes */
    union
    {                                  /* a "proper" union to shutup lint */
        rpc_cn_pres_cont_elem_p_t ep;  /* ptr to presentation context list
                                        * element */
        rpc_cn_pres_syntax_id_p_t sp;  /* ptr to a transfer syntax id */
        rpc_cn_auth_tlr_p_t rtn;       /* return value */
    } ptrs;

    *st = rpc_s_ok;

    /*
     * Get a pointer to the first element.
     * 
     * NB: The elements and lists that we are parsing are all variable length.
     */
    ptrs.ep = &(pcontp->pres_cont_elem[0]);

    /*
     * Process each element of the presentation context list.
     */
    for (n = 0; n < pcontp->n_context_elem; n++)
    {
    	/*
    	 * Get the number of transfer syntaxes in this element.
    	 */
        tsn = (ptrs.ep)->n_transfer_syn;
        
        /*
         * Unpack the abstract and transfer syntaxes.
         */
        if (swap)
        {
	    /*
	     * Convert the presentation context id.
	     */
            SWAP_INPLACE_16 (&(ptrs.ep)->pres_context_id, end_of_pkt, st);
            if (*st != rpc_s_ok)
            {
                return (NULL);
            }
	    
            /*
             * Convert the abstract syntax.
             */
            SWAP_INPLACE_SYNTAX (&(ptrs.ep)->abstract_syntax, end_of_pkt, st);
            if (*st != rpc_s_ok)
            {
                return (NULL);
            }

            /*
             * Process each transfer syntax id of this element.
             */
            for (id = 0; id < tsn; id++)
            {
                SWAP_INPLACE_SYNTAX (&(ptrs.ep)->transfer_syntaxes[id], end_of_pkt, st);
                if (*st != rpc_s_ok)
                {
                    return (NULL);
                }
            }
        }

        /*
         * Point to the next context element.
         */
        (ptrs.sp) = &((ptrs.ep)->transfer_syntaxes[tsn]);
    }

    /*
     * We return a pointer to the next byte.
     */
    return (ptrs.rtn);
}


/*
**++
**
**  ROUTINE NAME:       unpack_pres_result_list
**
**  SCOPE:              INTERNAL - declared in cnpkt.c
**
**  DESCRIPTION:
**      
**  This routine unpacks a presentation result list in an RPC connection
**  packet and returns a pointer to the next octet following the list.
**
**  INPUTS:
**
**      presp		pointer to the presentation result list
**      swap		boolean indicating we need to perform byte swapping
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
**      return          pointer to the next octet following the presentation
**			result list
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL rpc_cn_auth_tlr_p_t unpack_pres_result_list 
(
  rpc_cn_pres_result_list_p_t     presp,
    boolean32                   swap,
    unsigned8                   *end_of_pkt,
    unsigned32                  *st
)
{
    unsigned8 n;		/* presentation result list element loop
                                        * count */
    unsigned8 prn;		/* number of presentation results */

    *st = rpc_s_ok;

    /*
     * Process each element of the presentation result list.
     */
    prn = presp->n_results;
    for (n = 0; (n < prn) && swap; n++)
    {
        SWAP_INPLACE_16 (&presp->pres_results[n].result, end_of_pkt, st);
        if (*st != rpc_s_ok)
        {
            return (NULL);
        }

        SWAP_INPLACE_16 (&presp->pres_results[n].reason, end_of_pkt, st);
        if (*st != rpc_s_ok)
        {
            return (NULL);
        }

        SWAP_INPLACE_SYNTAX (&presp->pres_results[n].transfer_syntax, end_of_pkt, st);
        if (*st != rpc_s_ok)
        {
            return (NULL);
        }
    }

    /*
     * We return a pointer to the next byte.
     */
    return ((rpc_cn_auth_tlr_p_t) &presp->pres_results[prn]);
}


/*
**++
**
**  ROUTINE NAME:       unpack_versions_supported
**
**  SCOPE:              INTERNAL - declared in cnpkt.c
**
**  DESCRIPTION:
**      
**  This routine unpacks a 'rpc_cn_versions_supported_t' structure
**  in an RPC connection packet and returns a pointer to the next
**  octet following the versions structure.
**
**  INPUTS:
**
**      versions_p	pointer to versions supported field
**
**  INPUTS/OUTPUTS:	none
**
**  OUTPUTS:		none
**
**  IMPLICIT INPUTS:	none
**
**  IMPLICIT OUTPUTS:	none
**
**  FUNCTION VALUE:
**
**      return          pointer to the next octet after the
**                      'versions_supported'
**
**  SIDE EFFECTS:
**
**--
**/

INTERNAL rpc_cn_auth_tlr_p_t unpack_versions_supported 
(
  rpc_cn_versions_supported_p_t versions_p
)
{
    union
    {
    	rpc_cn_version_p_t versp;
    	rpc_cn_auth_tlr_p_t rtn;
    } ptrs;
    int i;

    for (ptrs.versp = &versions_p->protocols[0], i = 0;
	 i<versions_p->n_protocols;
	 i++, ptrs.versp++)
        ;
        
    return (ptrs.rtn);
}


/*
**++
**
**  ROUTINE NAME:       force_alignment
**
**  SCOPE:              INTERNAL - declared in cnpkt.c
**
**  DESCRIPTION:
**      
**  This routine takes as in put a pointer which is to be aligned on a
**  requested byte boundary.  Note well that this routine is a potential
**  source of portability problems since we are performing math operations
**  on pointers.
**
**  INPUTS:
**
**      boundary	number of octets of forced alignment
**
**  INPUTS/OUTPUTS:
**
**      ptr		pointer to be aligned
**
**  OUTPUTS:		none
**
**  IMPLICIT INPUTS:	none
**
**  IMPLICIT OUTPUTS:	none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void force_alignment 
(
  unsigned32 boundary,
  unsigned8 **ptr
)
{
    union
    {
    	unsigned8 **as_ptr;
    	unsigned32 *as_int;
    } anyptr;

    anyptr.as_ptr = ptr;

    if (*anyptr.as_int & (boundary-1))
    {
    	*anyptr.as_int += boundary;
    	*anyptr.as_int &= ~(boundary-1);
    }
}


/*
**++
**
**  ROUTINE NAME:       end_of_stub_data
**
**  SCOPE:              INTERNAL - declared in cnpkt.c
**
**  DESCRIPTION:
**      
**  This routine returns a pointer to the octet just following the stub data.
**
**  INPUTS:
**
**      pkt_p		a pointer to the packet being unpacked
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL rpc_cn_auth_tlr_p_t end_of_stub_data 
(
  rpc_cn_packet_p_t pkt_p
)
{
    union
    {
	unsigned8 *any;
	rpc_cn_packet_p_t pkt;
    	rpc_cn_auth_tlr_p_t rtn;
    } ptrs;
    unsigned32 stub_data_length;
    unsigned32 packet_header_overhead = 0;

    /*
     * Figure out our packet overhead.
     */
    switch (RPC_CN_PKT_PTYPE (pkt_p))
    {
    	case RPC_C_CN_PKT_REQUEST:

            if (RPC_CN_PKT_OBJ_UUID_PRESENT (pkt_p))
	    {
    	        packet_header_overhead = RPC_CN_PKT_SIZEOF_RQST_HDR_W_OBJ;
	    }
	    else
	    {
    	    	packet_header_overhead = RPC_CN_PKT_SIZEOF_RQST_HDR_NO_OBJ;
    	    }
    	    break;
    	    
    	case RPC_C_CN_PKT_RESPONSE:

    	    packet_header_overhead = RPC_CN_PKT_SIZEOF_RESP_HDR;
	    break;
	    
    	case RPC_C_CN_PKT_FAULT:

    	    packet_header_overhead = RPC_CN_PKT_SIZEOF_FAULT_HDR;
    	    break;
		default:
			 /* FIXME */
			 fprintf(stderr, "%s, unhandled case in switch: aborting\n", __PRETTY_FUNCTION__);
			 raise(SIGILL);
    }

#ifdef AUTH_QUESTIONS
    This is going to need some work based on what steve says about
    padding of stub data for auth trailers.
#endif

    /*
     * calculate our stub data length
     */
    stub_data_length = RPC_CN_PKT_FRAG_LEN(pkt_p) -
		       RPC_CN_PKT_AUTH_LEN(pkt_p) -
		       packet_header_overhead;

    /*
     * 1 - init our union of pointers to the start of the packet
     * 2 - bump to point to start of stub data in packet
     * 3 - bump to point to end of stub data
     */
    ptrs.pkt  = pkt_p;
    ptrs.any += packet_header_overhead;
    ptrs.any += stub_data_length;

    return (ptrs.rtn);
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_unpack_hdr
**
**  SCOPE:              PRIVATE - declared in cnpkt.h
**
**  DESCRIPTION:
**      
**  This routine is called by the network receiver thread to unpack an
**  RPC connection packet.  Basically, this routine does all the byte
**  swapping required between different endian machines as well as any
**  ebcdic <-> ascii translation required.
**
**  INPUTS:
**
**      pkt_p		a pointer to the packet to be unpacked
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32 rpc__cn_unpack_hdr
(
    rpc_cn_packet_p_t pkt_p,
    unsigned32 data_size
)
{
    rpc_cn_auth_tlr_p_t authp;           /* ptr to pkt authentication data */
    rpc_cn_pres_cont_list_p_t pconp;     /* ptr to pkt presentation context lists */
    rpc_cn_pres_result_list_p_t presp;   /* ptr to pkt presentation result lists */
    rpc_cn_port_any_p_t secadrp;         /* ptr to pkt secondary address data */
    rpc_cn_versions_supported_p_t versp; /* ptr to pkt versions supported data */
    unsigned8 *drepp;                    /* ptr to pkt drep[] data */
    boolean swap;                        /* boolean says we swap bytes/words */
    boolean authenticate;                /* boolean says authentication data is valid */
    boolean has_uuid;		         /* boolean says an OBJECT uuid is present */
    unsigned32 st;                       /* status variable */
    unsigned8 *end_of_pkt;               /* ptr to 1 byte past end of packet */

    end_of_pkt = (unsigned8 *) pkt_p;
    end_of_pkt += data_size;

    /*
     * Get the DREP and see if we need to do byte/word swapping.
     */
    drepp = RPC_CN_PKT_DREP (pkt_p);
    swap = (NDR_DREP_INT_REP (drepp) != NDR_LOCAL_INT_REP);

    /*
     * RPC_CN_COMMON_HDR_T
     * 
     * Unpack the common part of the packet header.
     */
    if (swap)
    {
        SWAP_INPLACE_16 (&RPC_CN_PKT_FRAG_LEN (pkt_p), end_of_pkt, &st);
        if (st != rpc_s_ok)
        {
            return (st);
        }

        SWAP_INPLACE_16 (&RPC_CN_PKT_AUTH_LEN (pkt_p), end_of_pkt, &st);
        if (st != rpc_s_ok)
        {
            return (st);
        }

        SWAP_INPLACE_32 (&RPC_CN_PKT_CALL_ID (pkt_p), end_of_pkt, &st);
        if (st != rpc_s_ok)
        {
            return (st);
        }
    }

    /*
     * See if the authentication data is valid.
     */
    authenticate = RPC_CN_PKT_AUTH_LEN(pkt_p) != 0;
    authp = NULL;

    /*
     * Check for the presence of an OBJECT uuid.
     */
    has_uuid = (RPC_CN_PKT_FLAGS (pkt_p) & RPC_C_CN_FLAGS_OBJECT_UUID) != 0;

    /*
     * Unpack the packet-type specific part of the packet. We also find the
     * start of the authentication data if there is any.
     */
    switch (RPC_CN_PKT_PTYPE (pkt_p))
    {
            /*
             * RPC_CN_BIND_HDR_T
             */
        case RPC_C_CN_PKT_BIND:
        case RPC_C_CN_PKT_ALTER_CONTEXT:
            if (swap)
            {
                SWAP_INPLACE_16 (&RPC_CN_PKT_MAX_XMIT_FRAG (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }

                SWAP_INPLACE_16 (&RPC_CN_PKT_MAX_RECV_FRAG (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }

                SWAP_INPLACE_32 (&RPC_CN_PKT_ASSOC_GROUP_ID (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }
            }
            pconp = (rpc_cn_pres_cont_list_p_t)((unsigned8 *)(&RPC_CN_PKT_ASSOC_GROUP_ID (pkt_p)) + 4);
            authp = unpack_pres_context_list (pconp, swap, end_of_pkt, &st);
            if (st != rpc_s_ok)
            {
                return (st);
            }
            break;

            /*
             * RPC_CN_BIND_ACK_HDR_T
             */
        case RPC_C_CN_PKT_BIND_ACK:
        case RPC_C_CN_PKT_ALTER_CONTEXT_RESP:
            if (swap)
            {
                SWAP_INPLACE_16 (&RPC_CN_PKT_MAX_XMIT_FRAG (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }

                SWAP_INPLACE_16 (&RPC_CN_PKT_MAX_RECV_FRAG (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }

                SWAP_INPLACE_32 (&RPC_CN_PKT_ASSOC_GROUP_ID (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }
            }
            secadrp = (rpc_cn_port_any_t *)
                ((unsigned8 *)(pkt_p) + RPC_CN_PKT_SIZEOF_BIND_ACK_HDR);
            presp = unpack_port_any (secadrp, drepp, end_of_pkt, &st);
            if (st != rpc_s_ok)
            {
                return (st);
            }

            force_alignment (4, (unsigned8 **)&presp);
            authp = unpack_pres_result_list (presp, swap, end_of_pkt, &st);
            if (st != rpc_s_ok)
            {
                return (st);
            }
            break;

            /*
             * RPC_CN_BIND_NACK_HDR_T
             */
        case RPC_C_CN_PKT_BIND_NAK:
            if (swap)
            {
                SWAP_INPLACE_16 (&RPC_CN_PKT_PROV_REJ_REASON (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }
            }
	    versp = &RPC_CN_PKT_VERSIONS (pkt_p);
            authp = unpack_versions_supported (versp);
            break;

            /*
             * RPC_CN_REQUEST_HDR_T
             */
        case RPC_C_CN_PKT_REQUEST:
            if (swap)
            {
                SWAP_INPLACE_32 (&RPC_CN_PKT_ALLOC_HINT (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }

                SWAP_INPLACE_16 (&RPC_CN_PKT_PRES_CONT_ID (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }

                SWAP_INPLACE_16 (&RPC_CN_PKT_OPNUM (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }

		if (has_uuid)
		{
                    SWAP_INPLACE_UUID (&RPC_CN_PKT_OBJECT (pkt_p), end_of_pkt, &st);
                    if (st != rpc_s_ok)
                    {
                        return (st);
                    }
                }
            }
            authp = end_of_stub_data (pkt_p);
            break;

            /*
             * RPC_CN_RESPONSE_HDR_T
             */
        case RPC_C_CN_PKT_RESPONSE:
            if (swap)
            {
                SWAP_INPLACE_32 (&RPC_CN_PKT_ALLOC_HINT (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }

                SWAP_INPLACE_16 (&RPC_CN_PKT_PRES_CONT_ID (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }
            }
            authp = end_of_stub_data (pkt_p);
            break;

	    /*
	     * RPC_CN_FAULT_HDR_T
	     */
        case RPC_C_CN_PKT_FAULT:
            if (swap)
            {
                SWAP_INPLACE_32 (&RPC_CN_PKT_ALLOC_HINT (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }

                SWAP_INPLACE_16 (&RPC_CN_PKT_PRES_CONT_ID (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }

                SWAP_INPLACE_32 (&RPC_CN_PKT_STATUS (pkt_p), end_of_pkt, &st);
                if (st != rpc_s_ok)
                {
                    return (st);
                }
            }
            authp = end_of_stub_data (pkt_p);
            break;
            
            /*
             * RPC_CN_AUTH3_HDR_T
             */
        case RPC_C_CN_PKT_SHUTDOWN:
        case RPC_C_CN_PKT_AUTH3:
        case RPC_C_CN_PKT_REMOTE_ALERT:
        case RPC_C_CN_PKT_ORPHANED:
            break;

        default:
            /* "(%s) Illegal or unknown packet type: %x\n" */
	    RPC_DCE_SVC_PRINTF ((
	        DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
	        rpc_svc_cn_pkt,
	        svc_c_sev_warning,
	        rpc_m_bad_pkt_type,
	        "rpc__cn_unpack_hdr",
	        RPC_CN_PKT_PTYPE(pkt_p) ));
            return (rpc_s_bad_pkt);
    }

    /*
     * Unpack the authentication part of the packet if it exists.
     */
    if (authenticate && swap)
    {
        switch (RPC_CN_PKT_PTYPE (pkt_p))
        {
	    case RPC_C_CN_PKT_BIND:
	    case RPC_C_CN_PKT_ALTER_CONTEXT:
	    case RPC_C_CN_PKT_BIND_ACK:
	    case RPC_C_CN_PKT_ALTER_CONTEXT_RESP:
	    case RPC_C_CN_PKT_BIND_NAK:
	    case RPC_C_CN_PKT_AUTH3:
            {
#ifdef DEBUG
		char *p;
#endif
                rpc_authn_protocol_id_t authn_protocol;

		authp = RPC_CN_PKT_AUTH_TLR (pkt_p, RPC_CN_PKT_FRAG_LEN (pkt_p));
#ifdef DEBUG
		p = (char *)authp;
		force_alignment(4, (unsigned8 **)&authp);
                if (p != (char *)authp)
                {
		    /*
		     * rpc_m_unalign_authtrl
		     * "(%s) Unaligned RPC_CN_PKT_AUTH_TRL"
		     */
		    RPC_DCE_SVC_PRINTF ((
			DCE_SVC(RPC__SVC_HANDLE, "%s"),
			rpc_svc_cn_pkt,
			svc_c_sev_fatal | svc_c_action_abort,
			rpc_m_unalign_authtrl,
			"rpc__cn_unpack_hdr" ));
                }
#endif
                authn_protocol = RPC_CN_AUTH_CVT_ID_WIRE_TO_API (authp->auth_type, &st);
                if (st == rpc_s_ok)
                {
                    RPC_CN_AUTH_TLR_UNPACK (authn_protocol,
                              	            pkt_p, RPC_CN_PKT_AUTH_LEN (pkt_p), drepp);
                }
                break;
	    }

            default:
        	/*
	         * We do not need the auth trailer of the other packet
                 * types since they are thrown away after recv_check.
                 * So don't bother to unpack them in these cases.
	         */
            break;
        }
    }

    return (rpc_s_ok);
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_pkt_format_common
**
**  SCOPE:              PRIVATE - declared in cnpkt.h
**
**  DESCRIPTION:
**      
**      This routine will format the fields common to all packet
**      types in the NCA connection protocol.
**
**  INPUTS:
**
**      pkt_p		a pointer to the packet to be formatted
**      ptype           the packet type
**      flags           the packet flags
**      frag_len        the length of the packet (header and body)
**      auth_len        the length of the authentication trailer
**      call_id         call identifier
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_pkt_format_common 
(
  rpc_cn_packet_p_t       pkt_p,
  unsigned32              ptype,
  unsigned32              flags,
  unsigned32              frag_len,
  unsigned32              auth_len,
  unsigned32              call_id,
  unsigned8               minor_version
)
{
    /*
     * First copy the template common packet header.
     */
    memcpy (pkt_p, &rpc_g_cn_common_hdr, sizeof (rpc_g_cn_common_hdr));

    /*
     * Now fill in the fields based on the input args to this
     * routine.
     */
    RPC_CN_PKT_PTYPE (pkt_p) = (unsigned8)ptype;
    RPC_CN_PKT_FLAGS (pkt_p) = (unsigned8)flags;
    RPC_CN_PKT_FRAG_LEN (pkt_p) = (unsigned16)frag_len;
    RPC_CN_PKT_AUTH_LEN (pkt_p) = (unsigned16)auth_len;
    RPC_CN_PKT_CALL_ID (pkt_p) = call_id;
    RPC_CN_PKT_VERS_MINOR (pkt_p) = minor_version;
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_stats_print
**
**  SCOPE:              PRIVATE - declared in cnpkt.h
**
**  DESCRIPTION:
**      
**  This routine will dump all statistics kept by the CN protocol.
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
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_stats_print (void)
{
    unsigned16 i;

    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("RPC CN Protocol Statistics"));
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("--------------------------------------------------------"));
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Calls sent:            %9lu", rpc_g_cn_mgmt.calls_sent));
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Calls rcvd:            %9lu", rpc_g_cn_mgmt.calls_rcvd));
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Pkts sent:             %9lu", rpc_g_cn_mgmt.pkts_sent));
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Pkts rcvd:             %9lu", rpc_g_cn_mgmt.pkts_rcvd));
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Connects established:  %9lu", rpc_g_cn_mgmt.connections));
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Connects broken:       %9lu", rpc_g_cn_mgmt.closed_connections));
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Associations alloced:  %9lu", rpc_g_cn_mgmt.alloced_assocs));
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Associations dealloced:%9lu", rpc_g_cn_mgmt.dealloced_assocs));
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Associations aborted:  %9lu", rpc_g_cn_mgmt.aborted_assocs));
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Association groups:    %9lu", rpc_g_cn_mgmt.assoc_grps));

    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("Breakdown by packet type               sent                 rcvd"));
    RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	("-------------------------------------------------------------------"));

    for (i = 0; i <= RPC_C_CN_PKT_MAX_TYPE; i++)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_stats, 1,
	    ("(%02u) %-10s             %9lu             %9lu",
                i, rpc__cn_pkt_name(i),
                rpc_g_cn_mgmt.pstats[i].sent, 
                rpc_g_cn_mgmt.pstats[i].rcvd));
    }
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_pkt_name
**
**  SCOPE:              PRIVATE - declared in cnpkt.h
**
**  DESCRIPTION:
**      
**  Return the string name for a type of packet.  This can't simply be
**  a variable because of the vagaries of global libraries.
**
**  INPUTS:             
**
**      ptype           The packet type.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE const char *rpc__cn_pkt_name
(
  unsigned32      ptype ATTRIBUTE_UNUSED
)
{
#ifndef DEBUG

    return("");

#else

    static const char *names[RPC_C_CN_PKT_MAX_TYPE + 1] =
    {
        "REQUEST      ",
        "PING         ",
        "RESPONSE     ",
        "FAULT        ",
        "WORKING      ",
        "NOCALL       ",
        "REJECT       ",
        "ACK          ",
        "QUIT         ",
        "FACK         ",
        "QUACK        ",
        "BIND         ",
        "BIND_ACK     ",
        "BIND_NAK     ",
        "ALT_CTXT     ",
        "ALT_CTXT_RESP",
        "AUTH3        ",
        "SHUTDOWN     ",
        "REMOTE_ALERT ",
        "ORPHANED     "
    };

    return((int) ptype > RPC_C_CN_PKT_MAX_TYPE ? "BOGUS PACKET TYPE" : names[(int) ptype]);

#endif
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_pkt_crc_compute
**
**  SCOPE:              PRIVATE - declared in cnpkt.h
**
**  DESCRIPTION:
**      
**  Compute a CRC-32/AUTODIN-II.
**
**  First, the polynomial itself and its table of feedback terms.  The  
**  polynomial is                                                       
**  X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0 
**  Note that we take it "backwards" and put the highest-order term in  
**  the lowest-order bit.  The X^32 term is "implied"; the LSB is the   
**  X^31 term, etc.  The X^0 term (usually shown as "+1") results in    
**  the MSB being 1.                                                    

**  Note that the usual hardware shift register implementation, which   
**  is what we're using (we're merely optimizing it by doing eight-bit  
**  chunks at a time) shifts bits into the lowest-order term.  In our   
**  implementation, that means shifting towards the right.  Why do we   
**  do it this way?  Because the calculated CRC must be transmitted in  
**  order from highest-order term to lowest-order term.  UARTs transmit 
**  characters in order from LSB to MSB.  By storing the CRC this way,  
**  we hand it to the UART in the order low-byte to high-byte; the UART 
**  sends each low-bit to hight-bit; and the result is transmission bit 
**  by bit from highest- to lowest-order term without requiring any bit 
**  shuffling on our part.  Reception works similarly.                  

**  The feedback terms table consists of 256, 32-bit entries.  Notes:   
**                                                                      
**   1. The table can be generated at runtime if desired; code to do so 
**      is shown later.  It might not be obvious, but the feedback      
**      terms simply represent the results of eight shift/xor opera-    
**      tions for all combinations of data and CRC register values.     
**                                                                      
**   2. The CRC accumulation logic is the same for all CRC polynomials, 
**      be they sixteen or thirty-two bits wide.  You simply choose the 
**      appropriate table.  Alternatively, because the table can be     
**      generated at runtime, you can start by generating the table for 
**      the polynomial in question and use exactly the same "updcrc",   
**      if your application needn't simultaneously handle two CRC       
**      polynomials.  (Note, however, that XMODEM is strange.)          
**                                                                      
**   3. For 16-bit CRCs, the table entries need be only 16 bits wide;   
**      of course, 32-bit entries work OK if the high 16 bits are zero. 
**                                                                      
**   4. The values must be right-shifted by eight bits by the "updcrc"  
**      logic; the shift must be unsigned (bring in zeroes).  On some   
**      hardware you could probably optimize the shift in assembler by  
**      using byte-swap instructions.                                   
**
**  INPUTS:             
**
**      block           The block whose checksum is to be computed.
**      block_len       The length, in bytes, of the block.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     unsigned32
**
**      The 32-bit CRC of the block.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32      rpc__cn_pkt_crc_compute 
(
  unsigned8       *block,
  unsigned32      block_len
)
{
    register unsigned_char_t    *data;
    register unsigned32         c = 0;
    register unsigned32         idx;
    unsigned32                  i;
    static unsigned32 crc_table[256] = 
    {
        0x00000000U, 0x77073096U, 0xee0e612cU, 0x990951baU,
        0x076dc419U, 0x706af48fU, 0xe963a535U, 0x9e6495a3U,
        0x0edb8832U, 0x79dcb8a4U, 0xe0d5e91eU, 0x97d2d988U,
        0x09b64c2bU, 0x7eb17cbdU, 0xe7b82d07U, 0x90bf1d91U,
        0x1db71064U, 0x6ab020f2U, 0xf3b97148U, 0x84be41deU,
        0x1adad47dU, 0x6ddde4ebU, 0xf4d4b551U, 0x83d385c7U,
        0x136c9856U, 0x646ba8c0U, 0xfd62f97aU, 0x8a65c9ecU,
        0x14015c4fU, 0x63066cd9U, 0xfa0f3d63U, 0x8d080df5U,
        0x3b6e20c8U, 0x4c69105eU, 0xd56041e4U, 0xa2677172U,
        0x3c03e4d1U, 0x4b04d447U, 0xd20d85fdU, 0xa50ab56bU,
        0x35b5a8faU, 0x42b2986cU, 0xdbbbc9d6U, 0xacbcf940U,
        0x32d86ce3U, 0x45df5c75U, 0xdcd60dcfU, 0xabd13d59U,
        0x26d930acU, 0x51de003aU, 0xc8d75180U, 0xbfd06116U,
        0x21b4f4b5U, 0x56b3c423U, 0xcfba9599U, 0xb8bda50fU,
        0x2802b89eU, 0x5f058808U, 0xc60cd9b2U, 0xb10be924U,
        0x2f6f7c87U, 0x58684c11U, 0xc1611dabU, 0xb6662d3dU,
        0x76dc4190U, 0x01db7106U, 0x98d220bcU, 0xefd5102aU,
        0x71b18589U, 0x06b6b51fU, 0x9fbfe4a5U, 0xe8b8d433U,
        0x7807c9a2U, 0x0f00f934U, 0x9609a88eU, 0xe10e9818U,
        0x7f6a0dbbU, 0x086d3d2dU, 0x91646c97U, 0xe6635c01U,
        0x6b6b51f4U, 0x1c6c6162U, 0x856530d8U, 0xf262004eU,
        0x6c0695edU, 0x1b01a57bU, 0x8208f4c1U, 0xf50fc457U,
        0x65b0d9c6U, 0x12b7e950U, 0x8bbeb8eaU, 0xfcb9887cU,
        0x62dd1ddfU, 0x15da2d49U, 0x8cd37cf3U, 0xfbd44c65U,
        0x4db26158U, 0x3ab551ceU, 0xa3bc0074U, 0xd4bb30e2U,
        0x4adfa541U, 0x3dd895d7U, 0xa4d1c46dU, 0xd3d6f4fbU,
        0x4369e96aU, 0x346ed9fcU, 0xad678846U, 0xda60b8d0U,
        0x44042d73U, 0x33031de5U, 0xaa0a4c5fU, 0xdd0d7cc9U,
        0x5005713cU, 0x270241aaU, 0xbe0b1010U, 0xc90c2086U,
        0x5768b525U, 0x206f85b3U, 0xb966d409U, 0xce61e49fU,
        0x5edef90eU, 0x29d9c998U, 0xb0d09822U, 0xc7d7a8b4U,
        0x59b33d17U, 0x2eb40d81U, 0xb7bd5c3bU, 0xc0ba6cadU,
        0xedb88320U, 0x9abfb3b6U, 0x03b6e20cU, 0x74b1d29aU,
        0xead54739U, 0x9dd277afU, 0x04db2615U, 0x73dc1683U,
        0xe3630b12U, 0x94643b84U, 0x0d6d6a3eU, 0x7a6a5aa8U,
        0xe40ecf0bU, 0x9309ff9dU, 0x0a00ae27U, 0x7d079eb1U,
        0xf00f9344U, 0x8708a3d2U, 0x1e01f268U, 0x6906c2feU,
        0xf762575dU, 0x806567cbU, 0x196c3671U, 0x6e6b06e7U,
        0xfed41b76U, 0x89d32be0U, 0x10da7a5aU, 0x67dd4accU,
        0xf9b9df6fU, 0x8ebeeff9U, 0x17b7be43U, 0x60b08ed5U,
        0xd6d6a3e8U, 0xa1d1937eU, 0x38d8c2c4U, 0x4fdff252U,
        0xd1bb67f1U, 0xa6bc5767U, 0x3fb506ddU, 0x48b2364bU,
        0xd80d2bdaU, 0xaf0a1b4cU, 0x36034af6U, 0x41047a60U,
        0xdf60efc3U, 0xa867df55U, 0x316e8eefU, 0x4669be79U,
        0xcb61b38cU, 0xbc66831aU, 0x256fd2a0U, 0x5268e236U,
        0xcc0c7795U, 0xbb0b4703U, 0x220216b9U, 0x5505262fU,
        0xc5ba3bbeU, 0xb2bd0b28U, 0x2bb45a92U, 0x5cb36a04U,
        0xc2d7ffa7U, 0xb5d0cf31U, 0x2cd99e8bU, 0x5bdeae1dU,
        0x9b64c2b0U, 0xec63f226U, 0x756aa39cU, 0x026d930aU,
        0x9c0906a9U, 0xeb0e363fU, 0x72076785U, 0x05005713U,
        0x95bf4a82U, 0xe2b87a14U, 0x7bb12baeU, 0x0cb61b38U,
        0x92d28e9bU, 0xe5d5be0dU, 0x7cdcefb7U, 0x0bdbdf21U,
        0x86d3d2d4U, 0xf1d4e242U, 0x68ddb3f8U, 0x1fda836eU,
        0x81be16cdU, 0xf6b9265bU, 0x6fb077e1U, 0x18b74777U,
        0x88085ae6U, 0xff0f6a70U, 0x66063bcaU, 0x11010b5cU,
        0x8f659effU, 0xf862ae69U, 0x616bffd3U, 0x166ccf45U,
        0xa00ae278U, 0xd70dd2eeU, 0x4e048354U, 0x3903b3c2U,
        0xa7672661U, 0xd06016f7U, 0x4969474dU, 0x3e6e77dbU,
        0xaed16a4aU, 0xd9d65adcU, 0x40df0b66U, 0x37d83bf0U,
        0xa9bcae53U, 0xdebb9ec5U, 0x47b2cf7fU, 0x30b5ffe9U,
        0xbdbdf21cU, 0xcabac28aU, 0x53b39330U, 0x24b4a3a6U,
        0xbad03605U, 0xcdd70693U, 0x54de5729U, 0x23d967bfU,
        0xb3667a2eU, 0xc4614ab8U, 0x5d681b02U, 0x2a6f2b94U,
        0xb40bbe37U, 0xc30c8ea1U, 0x5a05df1bU, 0x2d02ef8dU
    };
    
    data = (unsigned_char_t *) block;
    for (i = 0; i < block_len; i++) 
    {
	idx = (data[i] ^ c);
	idx &= 0xff;
	c >>= 8;
	c ^= crc_table[idx];
    }

    return (c);
}

