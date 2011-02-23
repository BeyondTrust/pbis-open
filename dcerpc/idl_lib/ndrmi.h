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
/*
**
**      ndrmi.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Header file for macros and procedures shared between ndrmi*.c modules
**
*/

/*  Marshalling strategy
 *
 *  All marshalling is done into buffers, except when an array is encountered
 *  that is sufficiently large to justify the "point at array optimization"
 *  A standard size iovector is used. When all its elements point at buffers
 *  or, by the optimization, arrays, it is despatched by an rpc_call_transmit.
 *  At the end of marshalling there is usually a further transmit, to send the
 *  data not already sent. On the server side, if there is no unsent data,
 *  there is no rpc_call_transmit. On the client side, if there is no unsent
 *  data there is still a transceive, to turn the line round.
 *
 *  The following state block fields affect marshalling
 *  IDL_buff_addr       address of the current buffer
 *  IDL_data_addr       address of first byte in buffer to be transmitted
 *  IDL_mp_start_offset determines the position of the start of data in the
 *                      current buffer relative to the first (0 mod 8) address
 *                      in the buffer. This will be 0 unless marshalling the
 *                      buffer follows a "marshall array by pointing"
 *  IDL_mp              Address in buffer at which next data item can be placed
 *  IDL_left_in_buff    Number of bytes in current buffer still available for
 *                      marshalling.
 */

/******************************************************************************/
/*                                                                            */
/*  Check whether there is enough space in the current buffer, if one exists  */
/*  If no buffer, create one                                                  */
/*  If buffer is full, perform closure/despatch and start a new buffer        */
/*                                                                            */
/******************************************************************************/
#define rpc_ss_ndr_marsh_check_buffer( datum_size, IDL_msp ) \
{ \
    if (datum_size > IDL_msp->IDL_left_in_buff) \
    { \
        if (IDL_msp->IDL_buff_addr != NULL) \
        { \
            rpc_ss_attach_buff_to_iovec( IDL_msp ); \
            rpc_ss_xmit_iovec_if_necess( idl_false, IDL_msp ); \
            IDL_msp->IDL_mp_start_offset = 0; \
        } \
        rpc_ss_ndr_marsh_init_buffer( IDL_msp ); \
    } \
}


/******************************************************************************/
/*                                                                            */
/* Marshall a scalar                                                          */
/*                                                                            */
/******************************************************************************/

#define IDL_MARSH_1_BYTE_SCALAR( marshalling_macro, type, param_addr ) \
{ \
    rpc_ss_ndr_marsh_check_buffer( 1, IDL_msp ); \
    marshalling_macro(IDL_msp->IDL_mp, *(type *)(param_addr)); \
    IDL_msp->IDL_mp += 1; \
    IDL_msp->IDL_left_in_buff -= 1; \
}

#define IDL_MARSH_BOOLEAN( param_addr ) \
{ \
        IDL_MARSH_1_BYTE_SCALAR( rpc_marshall_boolean, idl_boolean, param_addr ); \
}

#define IDL_MARSH_BYTE( param_addr ) \
    IDL_MARSH_1_BYTE_SCALAR( rpc_marshall_byte, idl_byte, param_addr )

#define IDL_MARSH_CHAR( param_addr ) \
    IDL_MARSH_1_BYTE_SCALAR( rpc_marshall_char, idl_char, param_addr )

#define IDL_MARSH_ALIGNED_SCALAR( marshalling_macro, size, type, param_addr ) \
{ \
    IDL_MARSH_ALIGN_MP( IDL_msp, size ); \
    rpc_ss_ndr_marsh_check_buffer( size, IDL_msp ); \
    marshalling_macro(IDL_msp->IDL_mp, *(type *)(param_addr)); \
    IDL_msp->IDL_mp += size; \
    IDL_msp->IDL_left_in_buff -= size; \
}

#define IDL_MARSH_DOUBLE( param_addr ) \
    IDL_MARSH_ALIGNED_SCALAR( rpc_marshall_long_float, 8, idl_long_float, param_addr )

#define IDL_MARSH_ENUM( param_addr ) \
    IDL_MARSH_ALIGNED_SCALAR( rpc_marshall_enum, 2, int, param_addr )

#define IDL_MARSH_FLOAT( param_addr ) \
    IDL_MARSH_ALIGNED_SCALAR( rpc_marshall_short_float, 4, idl_short_float, param_addr )

#define IDL_MARSH_SMALL( param_addr ) \
{ \
        IDL_MARSH_1_BYTE_SCALAR( rpc_marshall_small_int, idl_small_int, param_addr ); \
}

#define IDL_MARSH_SHORT( param_addr ) \
    IDL_MARSH_ALIGNED_SCALAR( rpc_marshall_short_int, 2, idl_short_int, param_addr )

#define IDL_MARSH_LONG( param_addr ) \
    IDL_MARSH_ALIGNED_SCALAR( rpc_marshall_long_int, 4, idl_long_int, param_addr )

#define IDL_MARSH_HYPER( param_addr ) \
    IDL_MARSH_ALIGNED_SCALAR( rpc_marshall_hyper_int, 8, idl_hyper_int, param_addr )

#define IDL_MARSH_USMALL( param_addr ) \
{ \
        IDL_MARSH_1_BYTE_SCALAR( rpc_marshall_usmall_int, idl_usmall_int, param_addr ); \
}

#define IDL_MARSH_USHORT( param_addr ) \
{ \
        IDL_MARSH_ALIGNED_SCALAR( rpc_marshall_ushort_int, 2, idl_ushort_int, param_addr ); \
}

#define IDL_MARSH_ULONG( param_addr ) \
    IDL_MARSH_ALIGNED_SCALAR( rpc_marshall_ulong_int, 4, idl_ulong_int, param_addr )

#define IDL_MARSH_UHYPER( param_addr ) \
    IDL_MARSH_ALIGNED_SCALAR( rpc_marshall_uhyper_int, 8, idl_uhyper_int, param_addr )

#define IDL_MARSH_V1_ENUM( param_addr ) \
    IDL_MARSH_ALIGNED_SCALAR( rpc_marshall_v1_enum, 4, int, param_addr )

#ifdef IDL_ENABLE_STATUS_MAPPING
#define IDL_MARSH_ERROR_STATUS( param_addr ) \
{ \
    IDL_MARSH_ALIGN_MP( IDL_msp, 4 ); \
    rpc_ss_ndr_marsh_check_buffer( 4, IDL_msp ); \
    rpc_marshall_ulong_int(IDL_msp->IDL_mp, *(idl_ulong_int *)(param_addr)); \
    rpc_ss_map_local_to_dce_status((error_status_t *)(IDL_msp->IDL_mp)); \
    IDL_msp->IDL_mp += 4; \
    IDL_msp->IDL_left_in_buff -= 4; \
}
#else
#define IDL_MARSH_ERROR_STATUS( param_addr ) \
{ \
    IDL_MARSH_ALIGN_MP( IDL_msp, 4 ); \
    rpc_ss_ndr_marsh_check_buffer( 4, IDL_msp ); \
    rpc_marshall_ulong_int(IDL_msp->IDL_mp, *(idl_ulong_int *)(param_addr)); \
    IDL_msp->IDL_mp += 4; \
    IDL_msp->IDL_left_in_buff -= 4; \
}
#endif

/* For marshalling interpreter internal variables, which are always C format */
#define IDL_MARSH_CUSHORT( param_addr ) \
    IDL_MARSH_ALIGNED_SCALAR( rpc_marshall_ushort_int, 2, idl_ushort_int, param_addr )

#define IDL_MARSH_CUSMALL( param_addr ) \
    IDL_MARSH_1_BYTE_SCALAR( rpc_marshall_usmall_int, idl_usmall_int, param_addr )


/* Function prototypes */

void rpc_ss_attach_buff_to_iovec
(
    IDL_msp_t IDL_msp
);

void rpc_ss_conf_struct_cs_bounds
(
    idl_byte *defn_vec_ptr,
    IDL_cs_shadow_elt_t *cs_shadow,
    IDL_bound_pair_t *bounds_list,
    IDL_msp_t IDL_msp
);

void rpc_ss_discard_allocate_ref
(
    idl_byte **p_type_vec_ptr
);

void rpc_ss_ndr_m_dfc_arr_ptees
(
    idl_ulong_int defn_index,
    rpc_void_p_t array_addr,
    rpc_void_p_t struct_addr,
    idl_ulong_int *struct_offset_vec_ptr,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_m_dvo_arr_ptees
(
    idl_ulong_int defn_index,
    rpc_void_p_t array_addr,
    rpc_void_p_t struct_addr,
    idl_ulong_int *struct_offset_vec_ptr,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_m_enc_union_or_ptees
(
    rpc_void_p_t param_addr,
    idl_ulong_int defn_index,
    idl_boolean pointees,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_m_fix_or_conf_arr
(
    rpc_void_p_t array_addr,
    idl_ulong_int dimensionality,
    IDL_bound_pair_t *bounds_list,
    idl_byte *defn_vec_ptr,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_m_fixed_cs_array
(
    rpc_void_p_t array_addr,
    idl_byte **p_defn_vec_ptr,
    IDL_msp_t IDL_msp
);


void rpc_ss_ndr_m_n_e_union_or_ptees
(
    rpc_void_p_t param_addr,
    idl_ulong_int switch_index,
    idl_ulong_int defn_index,
    rpc_void_p_t struct_addr,
    idl_ulong_int *struct_offset_vec_ptr,
    idl_boolean pointees,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_m_param_cs_shadow
(
    idl_byte *type_vec_ptr,
    idl_ulong_int param_index,
    idl_ulong_int shadow_length,
    IDL_cs_shadow_elt_t **p_cs_shadow,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_m_rlse_cs_shadow
(
    IDL_cs_shadow_elt_t *cs_shadow,
    idl_ulong_int shadow_length,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_m_struct_cs_shadow
(
    rpc_void_p_t struct_addr,
    idl_byte struct_type,
    idl_ulong_int shadow_length,
    idl_ulong_int offset_index,
    idl_byte *defn_vec_ptr,
    IDL_cs_shadow_elt_t **p_cs_shadow,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_m_struct_pointees
(
    idl_byte struct_type,
    idl_ulong_int defn_index,
    rpc_void_p_t struct_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_m_var_or_open_arr
(
    rpc_void_p_t array_addr,
    idl_ulong_int *Z_values,
    idl_ulong_int dimensionality,
    IDL_bound_pair_t *range_list,
    idl_byte *defn_vec_ptr,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_by_copying
(
    idl_ulong_int element_count,
    idl_ulong_int element_size,
    rpc_void_p_t array_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_by_looping
(
    idl_ulong_int element_count,
    idl_byte base_type,
    rpc_void_p_t array_addr,
    idl_ulong_int element_size,
    idl_ulong_int element_defn_index,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_by_pointing
(
    idl_ulong_int element_count,
    idl_ulong_int element_size,
    rpc_void_p_t array_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_context
(
    idl_byte context_type,
    rpc_void_p_t param_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_cs_array
(
    rpc_void_p_t array_addr,
    IDL_cs_shadow_elt_t *cs_shadow,
    idl_ulong_int shadow_index,
    idl_boolean in_struct,
    idl_byte **p_defn_vec_ptr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_cs_char
(
    rpc_void_p_t char_addr,
    idl_ulong_int cs_type_defn_index,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_deletes
(
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_fixed_arr
(
    idl_ulong_int defn_index,
    rpc_void_p_t array_addr,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
);


void rpc_ss_ndr_marsh_open_arr
(
    idl_ulong_int defn_index,
    rpc_void_p_t array_addr,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_pipe
(
    idl_ulong_int defn_index,
    rpc_void_p_t param_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_pointee
(
    idl_byte *defn_vec_ptr,
    rpc_void_p_t pointee_addr,
    idl_boolean register_node,
    IDL_pointee_desc_t *p_pointee_desc,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_scalar
(
    idl_byte type_byte,
    rpc_void_p_t param_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_bounded_scalar
(
    IDL_bound_pair_t *range_bounds,
    idl_byte type_byte,
    rpc_void_p_t param_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_interface
(
    idl_ulong_int defn_index,
    void 	  *param_addr,
    IDL_msp_t     IDL_msp
);

void rpc_ss_ndr_marsh_dyn_interface
(
    idl_ulong_int rtn_index,
    void 	  *param_addr,
    dce_uuid_t	  *piid,
    IDL_msp_t     IDL_msp
);

void rpc_ss_ndr_marsh_struct
(
    idl_byte struct_type,
    idl_ulong_int defn_index,
    rpc_void_p_t struct_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_v1_string
(
    rpc_void_p_t param_addr,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_varying_arr
(
    idl_ulong_int defn_index,
    rpc_void_p_t array_addr,
    rpc_void_p_t struct_addr,
    idl_ulong_int *struct_offset_vec_ptr,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_xmit_as
(
    idl_ulong_int defn_index,
    rpc_void_p_t param_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_marsh_Z_values
(
    idl_ulong_int dimensionality,
    idl_ulong_int *Z_values,
    IDL_msp_t IDL_msp
);

void rpc_ss_pointee_desc_from_data
(
    idl_byte *defn_vec_ptr,
    rpc_void_p_t array_addr,
    rpc_void_p_t struct_addr,
    idl_ulong_int *struct_offset_vec_ptr,
    IDL_pointee_desc_t *p_pointee_desc,
    IDL_msp_t IDL_msp
);

#define rpc_ss_rlse_data_pointee_desc( p_pointee_desc, IDL_msp ) \
    if ((p_pointee_desc)->dimensionality > 0) \
    { \
        rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, \
                                      (byte_p_t)((p_pointee_desc)->Z_values)); \
    }

void rpc_ss_xmit_iovec_if_necess
(
    idl_boolean attached_pointed_at,
    IDL_msp_t IDL_msp
);

#if defined(VMS) && defined(VAX)
void rpc_ss_ndr_m_for_conf_string
(
    idl_byte **p_type_vec_ptr,
    rpc_void_p_t param_addr,
    idl_boolean *p_is_string,
    IDL_msp_t IDL_msp
);
#endif
