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
**      cnxfer.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Entrypoints to support buffered data transfer within the 
**  Connection-oriented protocol services component of the RPC
**  runtime.
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comprot.h>    /* Common protocol services */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnfbuf.h>     /* NCA Connection fragment buffer declarations */
#include <cnpkt.h>      /* NCA Connection protocol header */
#include <cncall.h>     /* NCA Connection call service */
#include <cnassoc.h>    /* NCA Connection association service */
#include <cnxfer.h>

/*
 * Prototype for internal entrypoints.
 */

INTERNAL void rpc__cn_prep_next_iovector_elmt (
        rpc_cn_call_rep_p_t /*call_rep*/, 
        unsigned32     * /*status*/
    );


/*
**++
**
**  ROUTINE NAME:       rpc__cn_copy_buffer
**
**  SCOPE:              PRIVATE
**
**  DESCRIPTION:
**      
**  Copies an iovector element to the iovector array in the
**  call rep.  This routine will buffer data until the total
**  byte count reaches the max segment size for the transport
**  or when we have exhausted the size of the iovector in the
**  call rep.  If either of those conditions hold, the data
**  would be transferred.
**
**  INPUTS:
**
**      call_rep        The call rep.
**
**      iov_elt_p       The iovector element we are copying.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The completion status
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

PRIVATE void rpc__cn_copy_buffer 
(
  rpc_cn_call_rep_p_t     call_rep,
  rpc_iovector_elt_p_t    iov_elt_p,
  unsigned32              *status
)
{
    unsigned32              xfer_size;
    unsigned32              bytes_to_segment_size;
    unsigned32              bytes_left_to_xfer;
    unsigned32              cur_iov_index;
    byte_p_t                src;

    src = iov_elt_p->data_addr;
    bytes_left_to_xfer = iov_elt_p->data_len;
    *status = rpc_s_ok;

    bytes_to_segment_size = call_rep->max_seg_size - 
                            RPC_CN_CREP_ACC_BYTCNT (call_rep);

    cur_iov_index = RPC_CN_CREP_CUR_IOV_INDX (call_rep);
    while (bytes_left_to_xfer > 0)
    {
        /*
         * See if we've reached our transmit segment size.
         * If so, send what we have accumulated so far.
         */
        if (bytes_to_segment_size == 0)
        {
            /* 
             * Transmit all the data buffered thus far.
             */
            rpc__cn_transmit_buffers (call_rep, status);
            rpc__cn_dealloc_buffered_data (call_rep);
            
            /*
             * Fix up the iovector in the call rep so
             * that we again have only the cached protocol
             * header (and no stub data).
             */
            RPC_CN_FREE_ALL_EXCEPT_PROT_HDR (call_rep);
            cur_iov_index = RPC_CN_CREP_CUR_IOV_INDX (call_rep);
            if (*status != rpc_s_ok)
            {
                return;
            }
        }
        /* 
         * Check to see if the current iovector element is full.
         */
        else if (RPC_CN_CREP_FREE_BYTES (call_rep) == 0)
        {
            /*
             * If the current iovector element is full and we've
             * reached the end of our iovector, send what we
             * have accumulated so far.
             */
            if (RPC_CN_CREP_IOVLEN (call_rep) >= RPC_C_MAX_IOVEC_LEN)
            {
                /* 
                 * Transmit all the data buffered thus far.
                 */
                rpc__cn_transmit_buffers (call_rep, status);
                rpc__cn_dealloc_buffered_data (call_rep);

                /*
                 * Fix up the iovector in the call rep so
                 * that we again have only the cached protocol
                 * header (and no stub data).
                 */
                RPC_CN_FREE_ALL_EXCEPT_PROT_HDR (call_rep);
                cur_iov_index = RPC_CN_CREP_CUR_IOV_INDX (call_rep);
                if (*status != rpc_s_ok)
                {
                    return;
                }

            }
            else
            {
                /*
                 * We have not reached the end of our iovector.
                 * In this case, we can use a new iovector element.
                 */
                rpc__cn_prep_next_iovector_elmt (call_rep, status);
                cur_iov_index ++;
            }
        }

        /*
         * Copy the minimum of:
         *   1) what will fit into current fragment,
         *   2) number of bytes left to transfer,
         *   3) remaining bytes left before we reach max_seg_size.
         */
        xfer_size = RPC_CN_CREP_FREE_BYTES (call_rep);
        bytes_to_segment_size = call_rep->max_seg_size -
                                RPC_CN_CREP_ACC_BYTCNT (call_rep);
        if (xfer_size > bytes_to_segment_size)
        {
            xfer_size = bytes_to_segment_size;
        }
        if (xfer_size > bytes_left_to_xfer)
        {
            xfer_size = bytes_left_to_xfer;
        }

        memcpy (RPC_CN_CREP_FREE_BYTE_PTR (call_rep), src, xfer_size);
        bytes_left_to_xfer -= xfer_size;
        RPC_CN_CREP_ACC_BYTCNT (call_rep) += xfer_size;
        src += xfer_size;
        RPC_CN_CREP_FREE_BYTE_PTR (call_rep) += xfer_size;
        RPC_CN_CREP_FREE_BYTES (call_rep) -= xfer_size;
        RPC_CN_CREP_IOV (call_rep) [cur_iov_index].data_len += 
            xfer_size;
    }
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_add_new_iovector_elmt
**
**  SCOPE:              PRIVATE
**
**  DESCRIPTION:
**
**  Append the specified buffer as a new iovector element to
**  the iovector in the call rep.  This routine will transfer
**  data as necessary over the association until the total
**  accumulated data in the call rep is less than the 
**  negotiated segment size.
**
**  INPUTS:
**
**      call_rep        The call rep.
**
**      iovector_elmt   The iovector element describing the
**                      data to add.
**
**  INPUTS/OUTPUTS:     none
**          
**  OUTPUTS:
**
**      status          The completion status
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

PRIVATE void rpc__cn_add_new_iovector_elmt 
(
  rpc_cn_call_rep_p_t     call_rep,
  rpc_iovector_elt_p_t    iov_elt_p,
  unsigned32              *status
)
{
    unsigned32              bytes_to_segment_size;
    unsigned32              cur_iov_index;
    rpc_iovector_elt_p_t    iov_p;

    *status = rpc_s_ok;

    /*
     * If the current iovector element is full and we've
     * reached the end of our iovector, send what we
     * have accumulated so far.
     */
    if (RPC_CN_CREP_IOVLEN (call_rep) >= RPC_C_MAX_IOVEC_LEN)
    {
        /* 
         * Transmit all the data buffered thus far.
         */
        rpc__cn_transmit_buffers (call_rep, status);
        rpc__cn_dealloc_buffered_data (call_rep);

        /*
         * Fix up the iovector in the call rep so
         * that we again have only the cached protocol
         * header (and no stub data).
         */
        RPC_CN_FREE_ALL_EXCEPT_PROT_HDR (call_rep);
        if (*status != rpc_s_ok)
        {
            return;
        }
    }

    /*
     * At this point, we know that there is at least one
     * iovector element available for us to use in the
     * call rep.  There may be other elements between
     * this and the first element (containing the protocol
     * header).
     */

    /*
     * Fill in a new iovector element.
     */
    RPC_CN_CREP_IOVLEN (call_rep)++;
    cur_iov_index = ++RPC_CN_CREP_CUR_IOV_INDX (call_rep);
    RPC_CN_CREP_FREE_BYTES (call_rep) = 0;

    iov_p = &(RPC_CN_CREP_IOV (call_rep)[cur_iov_index]);
    *iov_p = *iov_elt_p;

    /*
     * If the new iovector element causes the total amount
     * of buffered data to exceed our max segment size,
     * transmit chunks of data from the current iovector
     * element until the total remaining size (including
     * protocol header) is less than our segment size.
     */
    bytes_to_segment_size = call_rep->max_seg_size -
                            RPC_CN_CREP_ACC_BYTCNT (call_rep);

    /* 
     * Only invoke rpc__cn_transmit_buffers() if iov_elt_p->data_len is 
     * greater than bytes_to_segment_size.
     */
    while (iov_elt_p->data_len > bytes_to_segment_size)
    {
        /*
         * Adjust the new iovector element to reflect only
         * enough data that can fit into current segment;
         * send it.
         */
        iov_p->data_len = bytes_to_segment_size;
        RPC_CN_CREP_ACC_BYTCNT (call_rep) += bytes_to_segment_size;
        rpc__cn_transmit_buffers (call_rep, status);
        if (*status != rpc_s_ok)
        {
            /*
             * Fix up the iovector in the call rep so
             * that we again have only the cached protocol
             * header (and no stub data).
             */
            rpc__cn_dealloc_buffered_data (call_rep);
            RPC_CN_FREE_ALL_EXCEPT_PROT_HDR (call_rep);
            return;
        }
        iov_elt_p->data_len -= bytes_to_segment_size;
        iov_elt_p->data_addr += bytes_to_segment_size;

        /*
         * Deallocate all the buffers except the 1st and last.
         * Then adjust iovector so that we have only 2 elements:
         * the header plus the current stub data.
         */
        if (RPC_CN_CREP_IOVLEN (call_rep) > (call_rep->sec != NULL) ? 3 : 2)
        {
            /*
             * rpc__cn_dealloc_buffered_data will always skip the
             * first iovector element (protocol header).  Decrementing
             * iovlen will cause it to skip the last element also.
             */
            RPC_CN_CREP_IOVLEN (call_rep) --;
            rpc__cn_dealloc_buffered_data (call_rep);

            /*
             * Now we rebuild the iovector.  It will have only
             * 2 elements: the header, plus the iovector element
             * which we are processing.
             */
            RPC_CN_CREP_IOVLEN (call_rep) = (call_rep->sec != NULL) ? 3 : 2;
            RPC_CN_CREP_CUR_IOV_INDX (call_rep) = 1;
            iov_p = &(RPC_CN_CREP_IOV (call_rep)[1]);
        }

        /*
         * Now logically, the only data is the header.
         * We are going to chain on the next iovector element
         * during the next iteration of this while loop.
         */
        RPC_CN_CREP_ACC_BYTCNT (call_rep) = RPC_CN_CREP_SIZEOF_HDR (call_rep);
        RPC_CN_CREP_IOV(call_rep)[0].data_len = 
                RPC_CN_CREP_SIZEOF_HDR (call_rep);
        
        *iov_p = *iov_elt_p;

        bytes_to_segment_size = call_rep->max_seg_size -
                            RPC_CN_CREP_ACC_BYTCNT (call_rep);

    }

    /*
     * At this point, the iovector element added (plus the
     * header) cannot exceed the max segment size.
     */

    /* If we started out with an iovector element whose
     * size (combined with the header) is a multiple of
     * our segment size, then the newly added iovector
     * element would have length = 0 (since all the data
     * would have been transmitted in the while loop).
     * Free the element in this case.
     * 
     */
    if (iov_p->data_len == 0)
    {
        if (iov_p->buff_dealloc != (rpc_buff_dealloc_fn_t) NULL)
	{
	    (iov_p->buff_dealloc) (iov_p->buff_addr);
	}
        RPC_CN_CREP_IOVLEN (call_rep) --;
        RPC_CN_CREP_CUR_IOV_INDX (call_rep) --;
    }
    else
    {
        /*
         * Update the total bytecount to account for the new
         * iovector element.
         */
        RPC_CN_CREP_ACC_BYTCNT (call_rep) += iov_elt_p->data_len;

        /*
         * Set free bytes to 0 so that we would allocate
         * a new iovector element next time instead of
         * copying data past the end of the current iovector
         * element.
         */
        RPC_CN_CREP_FREE_BYTES (call_rep) = 0;
    }
}

#if 0

/*
**++
**
**  ROUTINE NAME:       rpc__cn_flush_buffers
**
**  SCOPE:              PRIVATE - declared in cnxfer.h
**
**  DESCRIPTION:
**
**  Transmit a final fragment if any or all of the iovector element
**  buffers would have to be copied as indicated by the make "reusable"
**  bit. A final fragment can only be sent if the total number of bytes
**  is greater than the RT->stub guaranteed minumum. The data sent must
**  be a mutiple of 8 bytes.
**
**  INPUTS:
**
**      call_rep        The call rep.
**
**  INPUTS/OUTPUTS:     none
**          
**  OUTPUTS:
**
**      status          The completion status
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

PRIVATE void rpc__cn_flush_buffers 
(
  rpc_cn_call_rep_p_t     call_rep,
  unsigned32              *status
)
{
    unsigned32          i;
    rpc_iovector_elt_t  *iov_p;

    *status = rpc_s_ok;

    if (RPC_CN_CREP_ACC_BYTCNT (call_rep) >= rpc_c_assoc_must_recv_frag_size)
    {
        /*
         * There's enough data to do another send.
         */
        rpc__cn_transmit_buffers (call_rep, status);
        rpc__cn_dealloc_buffered_data (call_rep);
        RPC_CN_FREE_ALL_EXCEPT_PROT_HDR (call_rep);
    }
    else
    {
        /*
         * There's not enough data to send. Copy all that's
         * buffered.
         */
        for (i = 1;
             i < RPC_CN_CREP_IOVLEN (call_rep);
             i++)
        {
            rpc__cn_copy_buffer (call_rep, 
                                 &RPC_CN_CREP_IOV (call_rep)[i],
                                 status);
        }
    }
}
#endif /* 0 */


/*
**++
**
**  ROUTINE NAME:       rpc__cn_transmit_buffers
**
**  SCOPE:              PRIVATE
**
**  DESCRIPTION:
**
**  Transmits the data buffered in the call rep's iovector
**  over the association.
**
**  INPUTS:
**
**      call_rep        The call rep.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The completion status
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

PRIVATE void rpc__cn_transmit_buffers 
(
 rpc_cn_call_rep_p_t     call_rep,
 unsigned32              *status
)
{
    rpc_cn_packet_p_t   header_p;
    
    /*
     * Write the bytecount accumulated thus far into the fragment
     * length field of the cached protocol header.
     */
    *status = rpc_s_ok;
    header_p = (rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (call_rep);
    RPC_CN_PKT_FRAG_LEN (header_p) = RPC_CN_CREP_ACC_BYTCNT (call_rep);

    /*
     * Set the alloc hint; appears that NetApp's RPC implementation
     * depends on this.
     */
    RPC_CN_PKT_ALLOC_HINT (header_p) = RPC_CN_CREP_ACC_BYTCNT (call_rep) -
                                       RPC_CN_CREP_SIZEOF_HDR (call_rep);

    if (RPC_CALL_IS_CLIENT (((rpc_call_rep_t *) call_rep)))
    {
        /*
         * Check for pending cancels if sending a request. Set the flag
         * in the request header to forward the cancel if there is one
         * pending and this is the first fragment of the request.
         */
        if (RPC_CN_PKT_FLAGS (header_p) & RPC_C_CN_FLAGS_FIRST_FRAG)
        {
            if (call_rep->u.client.cancel.local_count)
            {
                RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                               ("(rpc__cn_transmit_buffers) setting alert pending bit in request header for queued cancel\n"));
                RPC_CN_PKT_FLAGS (header_p) |= RPC_C_CN_FLAGS_ALERT_PENDING;
                call_rep->u.client.cancel.local_count--;
            }
            else
            {
                DCETHREAD_TRY 
                {
                    dcethread_checkinterrupt ();
                }
                DCETHREAD_CATCH (dcethread_interrupt_e)
                {
                    RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                                   ("(rpc__cn_transmit_buffers) setting alert pending bit in request header for cancel just detected\n"));
                    RPC_CN_PKT_FLAGS (header_p) |= RPC_C_CN_FLAGS_ALERT_PENDING;
                    rpc__cn_call_start_cancel_timer (call_rep, status);
                }
                DCETHREAD_ENDTRY
            }
            if (*status != rpc_s_ok)
            {
                return;
            }
        }
        RPC_DBG_PRINTF (rpc_e_dbg_cancel, RPC_C_CN_DBG_CANCEL,
                       ("(rpc__cn_transmit_buffers) setting flag indicating first frag has been sent\n"));
        call_rep->u.client.cancel.server_is_accepting = true;
        call_rep->num_pkts = 0;
    }

    /*
     * If security was requested attach the authentication trailer
     * to the last iovector element. Make sure to add padding, if
     * required to the stub data to ensure the trailer starts on a
     * 4-byte boundary.
     */
    if (call_rep->sec != NULL)
    {
        rpc_iovector_elt_p_t    iov_p;
        rpc_cn_auth_tlr_t       *auth_tlr;

        /*
         * Remove the authentication trailer size from the header
         * iovector element. This was added by
         * RPC_CN_CREP_ADJ_IOV_FOR_TLR.
         */
        (RPC_CN_CREP_IOV(call_rep)[0]).data_len -= call_rep->prot_tlr->data_size;

        /*
         * Now adjust some fields in the auth trailer. The auth
         * trailer must start on a 4-byte boundary. Pad the user, or
         * stub, data to make it so. The amount of padding is
         * contained in the auth trailer so that the receiver can
         * determine the real user data size. 
         */
        auth_tlr = (rpc_cn_auth_tlr_t *)call_rep->prot_tlr->data_p;
        auth_tlr->stub_pad_length = 
            (4 - ((RPC_CN_CREP_ACC_BYTCNT (call_rep) -
                   call_rep->prot_tlr->data_size) & 0x03)) & 0x03; 
        (RPC_CN_CREP_IOV(call_rep)[RPC_CN_CREP_IOVLEN(call_rep) - 2]).data_len += 
                                   auth_tlr->stub_pad_length;
        RPC_CN_PKT_FRAG_LEN (header_p) += 
            auth_tlr->stub_pad_length -
            RPC_CN_CREP_SIZEOF_TLR_PAD (call_rep);

        /*
         * Hook the auth trailer iovector element after the last
         * iovector element.
         */
        iov_p = &(RPC_CN_CREP_IOV(call_rep)[RPC_CN_CREP_IOVLEN(call_rep) - 1]);
        iov_p->buff_dealloc = NULL;
        iov_p->data_len = 
            call_rep->prot_tlr->data_size -
            RPC_CN_CREP_SIZEOF_TLR_PAD (call_rep) ;
        iov_p->data_addr = (byte_p_t) call_rep->prot_tlr->data_p;
    }

    /*
     * Send the buffers in the iovector out over the association.
     */
    rpc__cn_assoc_send_frag (call_rep->assoc, 
                             &(call_rep->buffered_output.iov), 
                             call_rep->sec,
                             status);


    /*
     * Clear the first frag flag bit in the cached protocol header
     * so that subsequent packets will not have the bit set.
     */
    RPC_CN_PKT_FLAGS (header_p) &= ~RPC_C_CN_FLAGS_FIRST_FRAG;

    /*
     * Update the count of packets sent and received for this call.
     */
    call_rep->num_pkts++;
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_prep_iovector_elmt
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Prepare a new iovector element from the call rep for use
**  in buffering data.
**
**  INPUTS:
**
**      call_rep        The call rep.
**
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          The completion status
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

INTERNAL void rpc__cn_prep_next_iovector_elmt 
(
  rpc_cn_call_rep_p_t     call_rep,
  unsigned32              *status
)
{
    unsigned32              cur_iov_index;
    rpc_iovector_elt_p_t    iov_p;
    rpc_cn_fragbuf_p_t      buf_p;

    /*
     * Allocate a new [large] fragment buffer.
     */
    buf_p = rpc__cn_fragbuf_alloc (true);
    if (buf_p == NULL)
    {
        *status = rpc_s_no_memory;
        return;
    }

    /*
     * Make the next iovector element point to it.
     * Initialize pointers.
     */
    RPC_CN_CREP_IOVLEN (call_rep) ++;
    cur_iov_index = ++ (RPC_CN_CREP_CUR_IOV_INDX (call_rep));
    iov_p = &(RPC_CN_CREP_IOV (call_rep)[cur_iov_index]);
    iov_p->buff_dealloc = (rpc_buff_dealloc_fn_t)buf_p->fragbuf_dealloc;
    iov_p->buff_addr = (byte_p_t) buf_p;
    iov_p->buff_len = buf_p->max_data_size;
    iov_p->data_addr = (byte_p_t) buf_p->data_p;
    iov_p->data_len = 0;

    RPC_CN_CREP_FREE_BYTES (call_rep) = rpc_g_cn_large_frag_size;
    RPC_CN_CREP_FREE_BYTE_PTR (call_rep) = (byte_p_t) buf_p->data_p;
    *status = rpc_s_ok;

}

/*
**++
**
**  ROUTINE NAME:       rpc__cn_dealloc_buffered_data
**
**  SCOPE:              PRIVATE
**
**  DESCRIPTION:
**
**  Deallocates all the elements of an iovector (except for the 
**  first element).  The first element is assumed to contain the
**  protocol header which will be reused on subsequent transfers.
**
**  NOTE that this routine does not adjust any of the data
**  pointers (cur_iov_indx, iovlen, etc.) in the call rep.
**  This is done so that the caller can have better control.
**
**  INPUTS:
**
**      call_rep        The call rep.
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

PRIVATE void rpc__cn_dealloc_buffered_data
(
  rpc_cn_call_rep_p_t     call_rep
)
{
    unsigned32      cur_iov_index;
    unsigned32      iov_elmnts;

    iov_elmnts = RPC_CN_CREP_IOVLEN (call_rep);
    /*
     * If authenticated RPC is used, the last iovector
     * element is the auth trailer.
     * Don't free it; it is freed explicitly in call_end.
     */
    if (call_rep->sec != NULL)
    {
	iov_elmnts--;
    }

    for (cur_iov_index = 1; 
         cur_iov_index < iov_elmnts;
         cur_iov_index++)
    {
        if (RPC_CN_CREP_IOV (call_rep) [cur_iov_index].buff_dealloc != NULL)
        {
            (RPC_CN_CREP_IOV (call_rep) [cur_iov_index].buff_dealloc)
                (RPC_CN_CREP_IOV (call_rep) [cur_iov_index].buff_addr);
        }
        RPC_CN_CREP_IOV (call_rep) [cur_iov_index].buff_addr = NULL;
    }
}
