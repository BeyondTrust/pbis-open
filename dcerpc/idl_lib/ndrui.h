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
**      ndrui.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Macros and prototypes for routines shared between ndrui*.c modules
**
*/

/******************************************************************************/
/*                                                                            */
/*  Check whether there is an item to unmarshall in the existing buffer       */
/*  If we are correctly aligned, there is unless we are at buffer end         */
/*  If not, release the current buffer and get a new buffer                   */
/*                                                                            */
/*  Note. IDL_msp->IDL_elt_p->buff_dealloc = 0; before rpc_call_receive       */
/*  because DG runtime does not do this if receive fails                      */
/*                                                                            */
/******************************************************************************/
#define rpc_ss_ndr_unmar_check_buffer( IDL_msp ) \
{ \
    if (IDL_msp->IDL_left_in_buff == 0) \
    { \
        if (IDL_msp->IDL_pickling_handle != NULL) \
            idl_es_decode_check_buffer(IDL_msp);\
        else \
        { \
            if (IDL_msp->IDL_elt_p->buff_dealloc \
                    && IDL_msp->IDL_elt_p->data_len != 0) \
               (*(IDL_msp->IDL_elt_p->buff_dealloc))(IDL_msp->IDL_elt_p->buff_addr); \
            rpc_call_receive( (rpc_call_handle_t)IDL_msp->IDL_call_h, IDL_msp->IDL_elt_p, \
                            (unsigned32 *)&IDL_msp->IDL_status ); \
            if (IDL_msp->IDL_status != error_status_ok) \
                DCETHREAD_RAISE( rpc_x_ss_pipe_comm_error ); \
            IDL_msp->IDL_mp = (idl_byte *)IDL_msp->IDL_elt_p->data_addr; \
            if (IDL_msp->IDL_mp == NULL) \
            { \
                IDL_msp->IDL_status = rpc_s_stub_protocol_error; \
                DCETHREAD_RAISE( rpc_x_ss_pipe_comm_error ); \
            } \
            IDL_msp->IDL_left_in_buff = IDL_msp->IDL_elt_p->data_len; \
        } \
    } \
}

/******************************************************************************/
/*                                                                            */
/* Unmarshall a scalar                                                        */
/*                                                                            */
/******************************************************************************/

#define IDL_UNMAR_1_BYTE_SCALAR( marshalling_macro, type, param_addr ) \
{ \
    rpc_ss_ndr_unmar_check_buffer( IDL_msp ); \
    marshalling_macro( IDL_msp->IDL_drep, ndr_g_local_drep, \
                        IDL_msp->IDL_mp, *(type *)(param_addr)); \
    IDL_msp->IDL_mp += 1; \
    IDL_msp->IDL_left_in_buff -= 1; \
}

#define IDL_UNMAR_BOOLEAN( param_addr ) \
{ \
        IDL_UNMAR_1_BYTE_SCALAR( rpc_convert_boolean, idl_boolean, param_addr ); \
}

#define IDL_UNMAR_BYTE( param_addr ) \
    IDL_UNMAR_1_BYTE_SCALAR( rpc_convert_byte, idl_byte, param_addr )

#define IDL_UNMAR_CHAR( param_addr ) \
    IDL_UNMAR_1_BYTE_SCALAR( rpc_convert_char, idl_char, param_addr )

#define IDL_UNMAR_ALIGNED_SCALAR( marshalling_macro, size, type, param_addr ) \
{ \
    IDL_UNMAR_ALIGN_MP( IDL_msp, size ); \
    rpc_ss_ndr_unmar_check_buffer( IDL_msp ); \
    marshalling_macro( IDL_msp->IDL_drep, ndr_g_local_drep, \
                        IDL_msp->IDL_mp, *(type *)(param_addr)); \
    IDL_msp->IDL_mp += size; \
    IDL_msp->IDL_left_in_buff -= size; \
}

#define IDL_UNMAR_DOUBLE( param_addr ) \
    IDL_UNMAR_ALIGNED_SCALAR( rpc_convert_long_float, 8, idl_long_float, param_addr )

#define IDL_UNMAR_ENUM( param_addr ) \
    IDL_UNMAR_ALIGNED_SCALAR( rpc_convert_enum, 2, int, param_addr )

#define IDL_UNMAR_FLOAT( param_addr ) \
    IDL_UNMAR_ALIGNED_SCALAR( rpc_convert_short_float, 4, idl_short_float, param_addr )

#define IDL_UNMAR_SMALL( param_addr ) \
{ \
        IDL_UNMAR_1_BYTE_SCALAR( rpc_convert_small_int, idl_small_int, param_addr ); \
}

#define IDL_UNMAR_SHORT( param_addr ) \
    IDL_UNMAR_ALIGNED_SCALAR( rpc_convert_short_int, 2, idl_short_int, param_addr )

#define IDL_UNMAR_LONG( param_addr ) \
    IDL_UNMAR_ALIGNED_SCALAR( rpc_convert_long_int, 4, idl_long_int, param_addr )

#define IDL_UNMAR_HYPER( param_addr ) \
    IDL_UNMAR_ALIGNED_SCALAR( rpc_convert_hyper_int, 8, idl_hyper_int, param_addr )

#define IDL_UNMAR_USMALL( param_addr ) \
{ \
        IDL_UNMAR_1_BYTE_SCALAR( rpc_convert_usmall_int, idl_usmall_int, param_addr ); \
}

#define IDL_UNMAR_USHORT( param_addr ) \
{ \
        IDL_UNMAR_ALIGNED_SCALAR( rpc_convert_ushort_int, 2, idl_ushort_int, param_addr ); \
}

#define IDL_UNMAR_ULONG( param_addr ) \
    IDL_UNMAR_ALIGNED_SCALAR( rpc_convert_ulong_int, 4, idl_ulong_int, param_addr )

#define IDL_UNMAR_UHYPER( param_addr ) \
    IDL_UNMAR_ALIGNED_SCALAR( rpc_convert_uhyper_int, 8, idl_uhyper_int, param_addr )

#define IDL_UNMAR_V1_ENUM( param_addr ) \
    IDL_UNMAR_ALIGNED_SCALAR( rpc_convert_v1_enum, 4, int, param_addr )

#ifdef IDL_ENABLE_STATUS_MAPPING
#define IDL_UNMAR_ERROR_STATUS( param_addr ) \
{ \
    IDL_UNMAR_ALIGN_MP( IDL_msp, 4 ); \
    rpc_ss_ndr_unmar_check_buffer( IDL_msp ); \
    rpc_convert_ulong_int( IDL_msp->IDL_drep, ndr_g_local_drep, \
                        IDL_msp->IDL_mp, *(idl_ulong_int *)(param_addr)); \
    rpc_ss_map_dce_to_local_status((error_status_t *)(param_addr)); \
    IDL_msp->IDL_mp += 4; \
    IDL_msp->IDL_left_in_buff -= 4; \
}
#else
#define IDL_UNMAR_ERROR_STATUS( param_addr ) \
{ \
    IDL_UNMAR_ALIGN_MP( IDL_msp, 4 ); \
    rpc_ss_ndr_unmar_check_buffer( IDL_msp ); \
    rpc_convert_ulong_int( IDL_msp->IDL_drep, ndr_g_local_drep, \
                        IDL_msp->IDL_mp, *(idl_ulong_int *)(param_addr)); \
    IDL_msp->IDL_mp += 4; \
    IDL_msp->IDL_left_in_buff -= 4; \
}
#endif

/* For unmarshalling interpreter internal values, which are always C format */
#define IDL_UNMAR_CUSMALL( param_addr ) \
    IDL_UNMAR_1_BYTE_SCALAR( rpc_convert_usmall_int, idl_usmall_int, param_addr )

#define IDL_UNMAR_CUSHORT( param_addr ) \
    IDL_UNMAR_ALIGNED_SCALAR( rpc_convert_short_int, 2, idl_ushort_int, param_addr )

/* Function prototypes */
void rpc_ss_ndr_u_struct_cs_shadow
(
    rpc_void_p_t struct_addr,           /* [in] Address of struct */
    idl_byte struct_type,               /* [in] FIXED_STRUCT or CONF_STRUCT */
    idl_ulong_int offset_index,         /* [in] Start of struct's offset vec */
    idl_byte *defn_vec_ptr,             /* [in] Posn following shadow length */
    IDL_cs_shadow_elt_t *cs_shadow,     /* [in] Address of cs-shadow */
    IDL_msp_t IDL_msp
);
byte_p_t rpc_ss_inquire_pointer_to_node
(
    rpc_ss_node_table_t tab,
    idl_ulong_int num,
    long *has_been_unmarshalled
);

void rpc_ss_alloc_out_cs_conf_array
(
    IDL_cs_shadow_elt_t *cs_shadow,
    idl_byte **p_type_vec_ptr,
    rpc_void_p_t *p_array_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_alloc_pointer_target
(
    idl_byte *defn_vec_ptr,
    rpc_void_p_t *p_pointer,
    IDL_msp_t IDL_msp
);

void rpc_ss_init_new_array_ptrs
(
    idl_ulong_int dimensionality,
    idl_ulong_int *Z_values,
    idl_byte *defn_vec_ptr,
    rpc_void_p_t array_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_init_new_struct_ptrs
(
    idl_byte struct_type,
    idl_byte *defn_vec_ptr,
    rpc_void_p_t struct_addr,
    idl_ulong_int *conf_Z_values,
    IDL_msp_t IDL_msp
);

void rpc_ss_init_out_ref_ptrs
(
    idl_byte **p_type_vec_ptr,
    rpc_void_p_t param_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_alloc_storage
(
    idl_ulong_int fixed_part_size,
    idl_ulong_int dimensionality,
    idl_ulong_int *Z_values,
    idl_byte *array_defn_ptr,
    rpc_void_p_t *p_storage_addr,
    IDL_msp_t IDL_msp
);

idl_ulong_int rpc_ss_ndr_allocation_size
(
    idl_ulong_int fixed_part_size,
    idl_ulong_int dimensionality,
    idl_ulong_int *Z_values,
    idl_byte *array_defn_ptr,
    IDL_msp_t IDL_msp
);
void rpc_ss_ndr_u_conf_cs_struct_hdr
(
    idl_byte *struct_defn_ptr,
    idl_byte *array_defn_ptr,
    idl_ulong_int *Z_values,
    idl_ulong_int fixed_part_size,
    idl_boolean type_has_pointers,
    idl_ulong_int conf_arr_shadow_index,
    idl_boolean allocate,
    IDL_cs_shadow_elt_t *cs_shadow,
    rpc_void_p_t *p_param_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_u_cs_array_param
(
    idl_byte **p_type_vec_ptr,
    IDL_cs_shadow_elt_t *param_cs_shadow,
    idl_ulong_int param_index,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_u_enc_union_or_ptees
(
    rpc_void_p_t param_addr,
    idl_ulong_int defn_index,
    idl_boolean pointees,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_u_f_or_c_arr_ptees
(
    idl_ulong_int dimensionality,
    idl_ulong_int *Z_values,
    idl_byte *defn_vec_ptr,
    rpc_void_p_t array_addr,
    IDL_msp_t IDL_msp
);


void rpc_ss_ndr_u_fix_or_conf_arr
(
    idl_ulong_int dimensionality,
    idl_ulong_int *Z_values,
    idl_byte *defn_vec_ptr,
    rpc_void_p_t array_addr,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_u_fixed_arr_ptees
(
    idl_ulong_int defn_index,
    rpc_void_p_t array_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_u_for_foc_arr
(
    rpc_void_p_t array_addr,
    idl_ulong_int dimensionality,
    idl_ulong_int *Z_values,
    idl_byte base_type,
    idl_ulong_int element_size,
    idl_ulong_int element_defn_index,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_u_for_foc_arr_ptees
(
    rpc_void_p_t array_addr,
    idl_ulong_int dimensionality,
    idl_ulong_int *Z_values,
    idl_byte base_type,
    idl_ulong_int element_size,
    idl_ulong_int element_defn_index,
    idl_byte *defn_vec_ptr,
    IDL_msp_t IDL_msp
);


void rpc_ss_ndr_u_n_e_union_ptees
(
    rpc_void_p_t param_addr,
    idl_ulong_int switch_value,
    idl_ulong_int switch_index,
    idl_ulong_int defn_index,
    rpc_void_p_t struct_addr,
    idl_ulong_int *struct_offset_vec_ptr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_u_param_cs_shadow
(
    idl_ulong_int type_index,
    IDL_cs_shadow_elt_t *cs_shadow,
    IDL_msp_t IDL_msp
);

#define rpc_ss_ndr_u_rlse_pointee_desc( p_pointee_desc, IDL_msp ) \
    if ((p_pointee_desc)->dimensionality > 0) \
    { \
        rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, \
                                      (byte_p_t)((p_pointee_desc)->Z_values)); \
    }

void rpc_ss_ndr_u_struct_pointees
(
    idl_byte struct_type,
    idl_ulong_int defn_index,
    rpc_void_p_t struct_addr,
    idl_ulong_int *Z_values,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_u_v_or_o_arr_ptees
(
    idl_ulong_int dimensionality,
    idl_ulong_int *Z_values,
    idl_byte *defn_vec_ptr,
    rpc_void_p_t array_addr,
    IDL_bound_pair_t *range_list,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_u_v1_varying_arr
(
    rpc_void_p_t array_addr,
    idl_byte *array_defn_ptr,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_u_var_or_open_arr
(
    idl_ulong_int dimensionality,
    idl_ulong_int *Z_values,
    idl_byte *defn_vec_ptr,
    rpc_void_p_t array_addr,
    IDL_bound_pair_t *range_list,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_by_copying
(
    idl_ulong_int element_count,
    idl_ulong_int element_size,
    rpc_void_p_t array_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_by_looping
(
    idl_ulong_int element_count,
    idl_byte base_type,
    rpc_void_p_t array_addr,
    idl_ulong_int element_size,
    idl_ulong_int element_defn_index,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_context
(
    idl_byte context_type,
    rpc_void_p_t param_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_cs_array
(
    rpc_void_p_t array_addr,
    IDL_cs_shadow_elt_t *cs_shadow,
    idl_ulong_int *Z_values,
    idl_ulong_int array_shadow_index,
    idl_byte **p_defn_vec_ptr,
    IDL_msp_t IDL_msp
);
void rpc_ss_ndr_unmar_cs_char
(
    rpc_void_p_t data_addr,
    idl_ulong_int cs_type_defn_index,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_deletes
(
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_fixed_arr
(
    idl_ulong_int defn_index,
    rpc_void_p_t array_addr,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
);


void rpc_ss_ndr_unmar_n_e_union
(
    rpc_void_p_t param_addr,
    idl_ulong_int defn_index,
    idl_ulong_int *p_switch_value,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_pipe
(
    idl_ulong_int defn_index,
    rpc_void_p_t param_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_pointee
(
    idl_byte pointer_type,
    idl_byte *defn_vec_ptr,
    IDL_pointee_desc_t *p_pointee_desc,
    rpc_void_p_t *p_pointer,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_pointee_desc
(
    idl_byte pointer_type,
    idl_byte *defn_vec_ptr,
    IDL_pointee_desc_t *p_pointee_desc,
    rpc_void_p_t *p_pointer,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_range_list
(
    idl_ulong_int dimensionality,
    idl_byte base_type,
    IDL_bound_pair_t **p_range_list,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_scalar
(
    idl_byte type_byte,
    rpc_void_p_t param_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_bounded_scalar
(
    IDL_bound_pair_t *range_bounds,
    idl_byte type_byte,
    rpc_void_p_t param_addr,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_struct
(
    idl_byte struct_type,
    idl_byte *defn_vec_ptr,
    rpc_void_p_t struct_addr,
    idl_ulong_int *Z_values,
    IDL_cs_shadow_elt_t *cs_shadow,
    IDL_msp_t IDL_msp
);
void rpc_ss_ndr_unmar_interface(
    idl_ulong_int defn_index,
    void         *param_addr,
    void         *xmit_data_buff,
    IDL_msp_t     IDL_msp
);

void rpc_ss_ndr_unmar_dyn_interface(
    idl_ulong_int rtn_index,
    void         *param_addr,
    dce_uuid_t       *piid,
    void         *xmit_data_buff,
    IDL_msp_t     IDL_msp
);

void rpc_ss_ndr_unmar_v1_string
(
    rpc_void_p_t param_addr,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_varying_arr
(
    idl_byte *array_defn_ptr,
    idl_boolean type_has_pointers,
    rpc_void_p_t param_addr,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_xmit_as
(
    idl_ulong_int defn_index,
    rpc_void_p_t param_addr,
    rpc_void_p_t xmit_data_buff,
    IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_Z_values
(
    idl_ulong_int dimensionality,
    idl_ulong_int **p_Z_values,
    IDL_msp_t IDL_msp
);

#if defined(VMS) && defined(VAX)

void rpc_ss_complete_conf_arr_descs
(
    idl_ulong_int num_conf_char_arrays,
    idl_byte **conf_char_array_list,
    rpc_void_p_t IDL_param_vector[],
    IDL_msp_t IDL_msp
);

#endif

void rpc_ss_ndr_unmar_check_bounds_correlation
(
    /* [in] */ idl_byte **p_defn_vec_ptr,
    /* [in] */ rpc_void_p_t array_addr,
    /* [in] */ rpc_void_p_t struct_addr,
    /* [in] */ idl_ulong_int *struct_offset_vec_ptr,
    /* [in] */ idl_ulong_int dimensionality,
    /* [in] */ idl_ulong_int *Z_values,
    /* [in] */ idl_boolean pre_unmar,
    /* [out] */ IDL_bound_pair_t **p_correl_bounds_list,
    /* [in] */ IDL_msp_t IDL_msp
);

void rpc_ss_ndr_unmar_check_range_correlation
(
    /* [in] */ idl_byte **p_defn_vec_ptr,
    /* [in] */ rpc_void_p_t array_addr,
    /* [in] */ rpc_void_p_t struct_addr,
    /* [in] */ idl_ulong_int *struct_offset_vec_ptr,
    /* [in] */ idl_ulong_int dimensionality,
    /* [in] */ IDL_bound_pair_t *bounds_list,
    /* [in] */ IDL_bound_pair_t *range_list,
    /* [in] */ IDL_msp_t IDL_msp
);

