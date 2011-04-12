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
**      ndrmi.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      NDR marshalling interpreter main module
**
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdio.h>
#include <dce/idlddefs.h>
#include <ndrmi.h>
#include <lsysdep.h>

/*
 *  Forward function references
 */

/* None */

/******************************************************************************/
/*                                                                            */
/*  Attach the current buffer to the iovector                                 */
/*                                                                            */
/******************************************************************************/
void rpc_ss_attach_buff_to_iovec
(
    IDL_msp_t IDL_msp
)
{
    rpc_iovector_elt_t *p_elt;

    if (IDL_msp->IDL_pickling_handle != NULL)
    {
        idl_es_encode_attach_buff(IDL_msp);
        return;
    }

    p_elt = &(IDL_msp->IDL_iovec.elt[IDL_msp->IDL_elts_in_use]);
    if (IDL_msp->IDL_stack_packet_status == IDL_stack_packet_in_use_k)
    {
        IDL_msp->IDL_stack_packet_status = IDL_stack_packet_used_k;
        p_elt->buff_dealloc = NULL_FREE_RTN;
        p_elt->flags = rpc_c_iovector_elt_reused;
    }
    else if (IDL_msp->IDL_stack_packet_status == IDL_stack_packet_part_used_k)
    {
        p_elt->buff_dealloc = NULL_FREE_RTN;
        p_elt->flags = rpc_c_iovector_elt_reused;
    }
    else
    {
        p_elt->buff_dealloc = (rpc_buff_dealloc_fn_t)free;
        p_elt->flags = 0;
    }
    p_elt->buff_addr = (byte_p_t)IDL_msp->IDL_buff_addr;
    p_elt->buff_len = IDL_BUFF_SIZE;
    p_elt->data_addr = (byte_p_t)IDL_msp->IDL_data_addr;
    p_elt->data_len = IDL_msp->IDL_mp - IDL_msp->IDL_data_addr;
    (IDL_msp->IDL_elts_in_use)++;
    IDL_msp->IDL_buff_addr = NULL;
}

/******************************************************************************/
/*                                                                            */
/*  If all the iovector slots are full or we are marshalling pipe data        */
/*  or [transmit_as] data by pointing, despatch the iovector and reset the    */
/*  slot count to zero                                                        */
/*                                                                            */
/******************************************************************************/
void rpc_ss_xmit_iovec_if_necess
(
    /* [in] */ idl_boolean attached_pointed_at,  /* TRUE => last element added 
                                     to iovector was a pointer to user data */
    IDL_msp_t IDL_msp
)
{
    if (IDL_msp->IDL_pickling_handle != NULL)
    {
        return;     /* Data already captured by rpc_ss_attach_buff_to_iovec */
    }

    if ( (IDL_msp->IDL_elts_in_use == IDL_IOVECTOR_SIZE)
        || (attached_pointed_at
              && (IDL_msp->IDL_marsh_pipe || IDL_msp->IDL_m_xmit_level > 0)) )
    {
        /* Despatch the iovector */
        IDL_msp->IDL_iovec.num_elt = IDL_msp->IDL_elts_in_use;
        rpc_call_transmit( (rpc_call_handle_t)IDL_msp->IDL_call_h,
                           (rpc_iovector_p_t)&IDL_msp->IDL_iovec, 
                           (unsigned32 *)&IDL_msp->IDL_status );
        /* rpc_call_transmit() will always assume ownership of the
         * iovector elements, even on error, so empty it now */
        IDL_msp->IDL_elts_in_use = 0;
        if (IDL_msp->IDL_status != error_status_ok)
            DCETHREAD_RAISE(rpc_x_ss_pipe_comm_error);

        /* If there is a stack packet, mark it as reusable */
        if (IDL_msp->IDL_stack_packet_addr != NULL)
            IDL_msp->IDL_stack_packet_status = IDL_stack_packet_unused_k;
    }
}

/******************************************************************************/
/*                                                                            */
/*  Open a new buffer                                                         */
/*  The usable length of the buffer is set up so that the last address        */
/*  in the data area is (7 mod 8)                                             */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_init_buffer
(
    IDL_msp_t IDL_msp
)
{
    idl_byte *beyond_usable_buffer; /* (0 mod 8) address of byte beyond usable
                                        buffer area */

    if (IDL_msp->IDL_stack_packet_status == IDL_stack_packet_unused_k)
    {
        IDL_msp->IDL_buff_addr = IDL_msp->IDL_stack_packet_addr;
        IDL_msp->IDL_stack_packet_status = IDL_stack_packet_in_use_k;
        beyond_usable_buffer = (idl_byte *)
            (((IDL_msp->IDL_buff_addr + IDL_STACK_PACKET_SIZE)
                                                     - (idl_byte *)0) & (~7));
    }
    else if (IDL_msp->IDL_stack_packet_status == IDL_stack_packet_part_used_k)
    {
        /* IDL_mp was pointing at the first free location in the stack packet
            and has not been changed by the "marshall by pointing" */
        IDL_msp->IDL_buff_addr = IDL_msp->IDL_mp;
        IDL_msp->IDL_stack_packet_status = IDL_stack_packet_in_use_k;
        beyond_usable_buffer = (idl_byte *)
            (((IDL_msp->IDL_stack_packet_addr + IDL_STACK_PACKET_SIZE)
                                                     - (idl_byte *)0) & (~7));
    }
    else
    {
        if (IDL_msp->IDL_pickling_handle != NULL)
        {
            /* This is the only case where we may be doing an encoding */
            idl_ulong_int buff_size;

            idl_es_encode_init_buffer(&buff_size, IDL_msp);
            beyond_usable_buffer = (idl_byte *)
                                     (((IDL_msp->IDL_buff_addr + buff_size)
                                                     - (idl_byte *)0) & (~7));
        }
        else
        {
#ifdef DEBUG
            /* Zero marshalling buffers. */
            IDL_msp->IDL_buff_addr = (idl_byte *)calloc(1, IDL_BUFF_SIZE);
#else
            IDL_msp->IDL_buff_addr = (idl_byte *)malloc(IDL_BUFF_SIZE);
#endif /* DEBUG */
            if (IDL_msp->IDL_buff_addr == NULL)
                DCETHREAD_RAISE(rpc_x_no_memory);
            beyond_usable_buffer = (idl_byte *)
                                     (((IDL_msp->IDL_buff_addr + IDL_BUFF_SIZE)
                                                     - (idl_byte *)0) & (~7));
        }
    }
    IDL_msp->IDL_data_addr = (idl_byte *)
                ((((IDL_msp->IDL_buff_addr - (idl_byte *)0) + 7) & (~7))
                                             + IDL_msp->IDL_mp_start_offset);
    IDL_msp->IDL_mp = IDL_msp->IDL_data_addr;
    IDL_msp->IDL_left_in_buff = beyond_usable_buffer - IDL_msp->IDL_data_addr;
}

/******************************************************************************/
/*                                                                            */
/*  Marshall an array by making an iovector element point at it               */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_by_pointing
(
    /* [in] */  idl_ulong_int element_count,
    /* [in] */  idl_ulong_int element_size,
    /* [in] */  rpc_void_p_t array_addr,
    IDL_msp_t IDL_msp
)
{
    rpc_iovector_elt_t *p_elt;
    idl_ulong_int array_size_in_bytes;

    /* Close the current buffer if one is open */
    if (IDL_msp->IDL_buff_addr != NULL)
    {
        if ((IDL_msp->IDL_stack_packet_status == IDL_stack_packet_in_use_k)
            && (IDL_msp->IDL_left_in_buff >= 8))
        {
            /* Can use the rest of the stack packet later */
            IDL_msp->IDL_stack_packet_status = IDL_stack_packet_part_used_k;
        }
        rpc_ss_attach_buff_to_iovec( IDL_msp );
        rpc_ss_xmit_iovec_if_necess( idl_false, IDL_msp );
        IDL_msp->IDL_left_in_buff = 0;
        IDL_msp->IDL_mp_start_offset = (IDL_msp->IDL_mp - (idl_byte *)0) % 8;
    }

    p_elt = &(IDL_msp->IDL_iovec.elt[IDL_msp->IDL_elts_in_use]);
    array_size_in_bytes = element_count * element_size;
    p_elt->buff_dealloc = NULL_FREE_RTN;
    if ( (IDL_msp->IDL_side == IDL_server_side_k)
         || IDL_msp->IDL_marsh_pipe
         || (IDL_msp->IDL_m_xmit_level > 0) )
    {
        /* Run time must copy the data before server stub stack is unwound
            or pipe or [transmit_as] buffer is reused */
        p_elt->flags = rpc_c_iovector_elt_reused;
    }
    else
        p_elt->flags = 0;
    p_elt->buff_addr = (byte_p_t)array_addr;
    p_elt->buff_len = array_size_in_bytes;
    p_elt->data_addr = (byte_p_t)array_addr;
    p_elt->data_len = array_size_in_bytes;
    (IDL_msp->IDL_elts_in_use)++;
    rpc_ss_xmit_iovec_if_necess( idl_true, IDL_msp );
    IDL_msp->IDL_mp_start_offset =
                     (IDL_msp->IDL_mp_start_offset + array_size_in_bytes) % 8;
}

/******************************************************************************/
/*                                                                            */
/*  If we have an array whose base type is such that it could be marshalled   */
/*  by pointing, but its size is below the threshold level for having its own */
/*  iovector_elt, marshall it by block copying into one or more buffers       */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_by_copying
(
    /* [in] */  idl_ulong_int element_count,
    /* [in] */  idl_ulong_int element_size,
    /* [in] */  rpc_void_p_t array_addr,
    IDL_msp_t IDL_msp
)
{
    idl_ulong_int bytes_required;   /* Number of bytes left to copy */
    idl_ulong_int bytes_to_copy;  /* Number of bytes to copy into this buffer */

    bytes_required = element_count * element_size;
    while (bytes_required != 0)
    {
        rpc_ss_ndr_marsh_check_buffer( 1, IDL_msp);
        if (bytes_required > IDL_msp->IDL_left_in_buff)
            bytes_to_copy = IDL_msp->IDL_left_in_buff;
        else
            bytes_to_copy = bytes_required;
        memcpy(IDL_msp->IDL_mp, array_addr, bytes_to_copy);
        IDL_msp->IDL_mp += bytes_to_copy;
        IDL_msp->IDL_left_in_buff -= bytes_to_copy;
        bytes_required -= bytes_to_copy;
        array_addr = (rpc_void_p_t)((idl_byte *)array_addr + bytes_to_copy);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Marshall a structure                                                      */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_struct
(
    /* [in] */  idl_byte struct_type,   /* DT_FIXED_STRUCT, DT_CONF_STRUCT 
                                            or DT_V1_CONF_STRUCT */
    /* [in] */  idl_ulong_int defn_index,
    /* [in] */  rpc_void_p_t struct_addr,
    IDL_msp_t IDL_msp
)
{
    idl_ulong_int offset_index;
    idl_byte *defn_vec_ptr;
    idl_ulong_int *struct_offset_vec_ptr; /* Start of offsets for this struct */
    idl_ulong_int *offset_vec_ptr;
    idl_byte type_byte;
    idl_ulong_int offset;
    idl_ulong_int field_defn_index;
    idl_byte *field_defn_ptr;
    idl_ulong_int conf_dims = 0;    /* Number of dimensions of conformance info */
    IDL_bound_pair_t *conf_bounds;   /* Bounds list from conformance info */
    IDL_bound_pair_t normal_conf_bounds[IDL_NORMAL_DIMS];
    IDL_bound_pair_t range_bounds; /* Bounds list from range info */
    idl_ulong_int *Z_values;
    idl_ulong_int normal_Z_values[IDL_NORMAL_DIMS];
    IDL_bound_pair_t *range_list;
    IDL_bound_pair_t normal_range_list[IDL_NORMAL_DIMS];
    idl_boolean v1_array = idl_false;
    idl_ulong_int node_number;
    long already_marshalled;
    idl_ulong_int switch_index; /* Index of switch field for non-encapsulated
                                                                        union */
    idl_ushort_int v1_size;     /* Number of elements in open [v1_array] */
    idl_boolean add_null;
    idl_ulong_int shadow_length;    /* Number of elements in a cs_shadow */
    IDL_cs_shadow_elt_t *cs_shadow = NULL;
    unsigned32 i;

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index;
    IDL_GET_LONG_FROM_VECTOR(offset_index,defn_vec_ptr);
    struct_offset_vec_ptr = IDL_msp->IDL_offset_vec + offset_index;
    offset_vec_ptr = struct_offset_vec_ptr + 1;
                                        /* Skip over size at start of offsets */

    IDL_INIT_RANGE(range_bounds);

    if ( (struct_type == IDL_DT_CONF_STRUCT)
        || (struct_type == IDL_DT_V1_CONF_STRUCT) )
    {
        /* Next integer in the definition vector is index to a fully flattened 
            array rep for the conformant array field */
        IDL_GET_LONG_FROM_VECTOR(field_defn_index,defn_vec_ptr);
        field_defn_ptr = IDL_msp->IDL_type_vec + field_defn_index;
        conf_dims = (idl_ulong_int)*field_defn_ptr;
        if (conf_dims > IDL_NORMAL_DIMS)
        {
            conf_bounds = NULL;
            Z_values = NULL;
        }
        else
        {
            conf_bounds = normal_conf_bounds;
            Z_values = normal_Z_values;
        }
        field_defn_ptr++;
        if (*defn_vec_ptr == IDL_DT_CS_SHADOW)
        {
            /* Need to set up I-char machinery */
            defn_vec_ptr++;
            IDL_GET_LONG_FROM_VECTOR(shadow_length, defn_vec_ptr);
            rpc_ss_ndr_m_struct_cs_shadow(struct_addr, struct_type,
                                          shadow_length, offset_index, 
                                          defn_vec_ptr, &cs_shadow, IDL_msp);
            if (*(field_defn_ptr
                 + conf_dims * IDL_CONF_BOUND_PAIR_WIDTH) == IDL_DT_CS_TYPE)
            {
                rpc_ss_conf_struct_cs_bounds(field_defn_ptr, cs_shadow,
                                         conf_bounds, IDL_msp);
            }
            else
                rpc_ss_build_bounds_list( &field_defn_ptr, NULL, struct_addr,
                    struct_offset_vec_ptr, conf_dims, &conf_bounds, IDL_msp );
        }
        else
            rpc_ss_build_bounds_list( &field_defn_ptr, NULL, struct_addr,
                    struct_offset_vec_ptr, conf_dims, &conf_bounds, IDL_msp );
        rpc_ss_Z_values_from_bounds(conf_bounds, conf_dims, &Z_values,IDL_msp);
        if (struct_type == IDL_DT_V1_CONF_STRUCT)
        {
            v1_size = 1;
            for (i=0; i<conf_dims; i++)
                v1_size *= Z_values[i];
            IDL_MARSH_CUSHORT( &v1_size );
        }
        else
            rpc_ss_ndr_marsh_Z_values( conf_dims, Z_values, IDL_msp );
    }

    do {
        type_byte = *defn_vec_ptr;
        defn_vec_ptr++;
        switch(type_byte)
        {
            case IDL_DT_CS_SHADOW:
                /* I-char machinery  - struct has varying array(s) of [cs_char]
                    but no conformant array of [cs_char] */
                IDL_GET_LONG_FROM_VECTOR(shadow_length, defn_vec_ptr);
                rpc_ss_ndr_m_struct_cs_shadow(struct_addr, struct_type,
                                          shadow_length, offset_index, 
                                          defn_vec_ptr, &cs_shadow, IDL_msp);
                break;
            case IDL_DT_BYTE:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_CHECK_RANGE_BYTE( range_bounds, (idl_byte *)struct_addr + offset );
                IDL_MARSH_BYTE( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_CHAR:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_CHECK_RANGE_CHAR( range_bounds, (idl_byte *)struct_addr + offset );
                IDL_MARSH_CHAR( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_BOOLEAN:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_CHECK_RANGE_BOOLEAN( range_bounds, (idl_byte *)struct_addr + offset );
                IDL_MARSH_BOOLEAN( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_DOUBLE:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_CHECK_RANGE_DOUBLE( range_bounds, (idl_byte *)struct_addr + offset );
                IDL_MARSH_DOUBLE( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_ENUM:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_MARSH_ENUM( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_FLOAT:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_CHECK_RANGE_FLOAT( range_bounds, (idl_byte *)struct_addr + offset );
                IDL_MARSH_FLOAT( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_SMALL:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_CHECK_RANGE_SMALL( range_bounds, (idl_byte *)struct_addr + offset );
                IDL_MARSH_SMALL( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_SHORT:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_CHECK_RANGE_SHORT( range_bounds, (idl_byte *)struct_addr + offset );
                IDL_MARSH_SHORT( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_LONG:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_CHECK_RANGE_LONG( range_bounds, (idl_byte *)struct_addr + offset );
                IDL_MARSH_LONG( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_HYPER:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_MARSH_HYPER( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_USMALL:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_CHECK_RANGE_USMALL( range_bounds, (idl_byte *)struct_addr + offset );
                IDL_MARSH_USMALL( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_USHORT:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_CHECK_RANGE_USHORT( range_bounds, (idl_byte *)struct_addr + offset );
                IDL_MARSH_USHORT( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_ULONG:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_CHECK_RANGE_ULONG( range_bounds, (idl_byte *)struct_addr + offset );
                IDL_MARSH_ULONG( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_UHYPER:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_MARSH_UHYPER( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_ERROR_STATUS:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_MARSH_ERROR_STATUS( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_V1_ENUM:
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                IDL_MARSH_V1_ENUM( (idl_byte *)struct_addr + offset );
                break;
            case IDL_DT_FIXED_ARRAY:
                defn_vec_ptr++;     /* Skip over properties byte */
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                    /* Full array definition */
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                rpc_ss_ndr_marsh_fixed_arr( field_defn_index,
                            (idl_byte *)struct_addr+offset, 0, IDL_msp);
                break;
            case IDL_DT_VARYING_ARRAY:
                defn_vec_ptr++;     /* Skip over properties byte */
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                    /* Full array definition */
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                rpc_ss_ndr_marsh_varying_arr(field_defn_index,
                        (rpc_void_p_t)((idl_byte *)struct_addr+offset),
                        struct_addr, struct_offset_vec_ptr,
                        (v1_array ? IDL_M_V1_ARRAY : 0),
                        IDL_msp);
                v1_array = idl_false;
                break;
            case IDL_DT_CONF_ARRAY:
                defn_vec_ptr++;     /* Skip over properties byte */
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                    /* Full array definition */
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                offset = *offset_vec_ptr;   /* Must be last field of struct */
                field_defn_ptr = IDL_msp->IDL_type_vec + field_defn_index
                                   + 1; /* We already know the dimensionality */
                IDL_ADV_DEFN_PTR_OVER_BOUNDS( field_defn_ptr, conf_dims );
                rpc_ss_ndr_m_fix_or_conf_arr(
                        (rpc_void_p_t)((idl_byte *)struct_addr+offset),
                        conf_dims, conf_bounds, field_defn_ptr,
                        IDL_M_CONF_ARRAY, IDL_msp);
                if (conf_dims > IDL_NORMAL_DIMS)
                {
                    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                         (byte_p_t)Z_values);
                    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                         (byte_p_t)conf_bounds);
                }
                break;
            case IDL_DT_OPEN_ARRAY:
                defn_vec_ptr++;     /* Skip over properties byte */
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                    /* Full array definition */
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                offset = *offset_vec_ptr;   /* Must be last field of struct */
                field_defn_ptr = IDL_msp->IDL_type_vec + field_defn_index
                                   + 1; /* We already know the dimensionality */
                IDL_ADV_DEFN_PTR_OVER_BOUNDS( field_defn_ptr, conf_dims );
                if (conf_dims > IDL_NORMAL_DIMS)
                    range_list = NULL;
                else
                    range_list = normal_range_list;
                rpc_ss_build_range_list( &field_defn_ptr,
                            (rpc_void_p_t)((idl_byte *)struct_addr+offset),
                             struct_addr, struct_offset_vec_ptr, conf_dims,
                             conf_bounds, &range_list, &add_null, IDL_msp );
                rpc_ss_ndr_m_var_or_open_arr(
                        (rpc_void_p_t)((idl_byte *)struct_addr+offset),
                        Z_values, conf_dims, range_list, field_defn_ptr,
                        ((v1_array ? IDL_M_V1_ARRAY : 0) | IDL_M_CONF_ARRAY
                            | (add_null ? IDL_M_ADD_NULL : 0)),
                        IDL_msp);
                if (conf_dims > IDL_NORMAL_DIMS)
                {
                    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                            (byte_p_t)range_list);
                    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                            (byte_p_t)Z_values);
                    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                            (byte_p_t)conf_bounds);
                }
                v1_array = false;
                break;
            case IDL_DT_ENC_UNION:
                defn_vec_ptr++;     /* Skip over properties byte */
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                rpc_ss_ndr_m_enc_union_or_ptees( (idl_byte *)struct_addr+offset,
                                          field_defn_index, idl_false, IDL_msp);
                break;
            case IDL_DT_N_E_UNION:
                defn_vec_ptr++;     /* Skip over properties byte */
                IDL_GET_LONG_FROM_VECTOR(switch_index, defn_vec_ptr);
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                rpc_ss_ndr_m_n_e_union_or_ptees( (idl_byte *)struct_addr+offset,
                                             switch_index, field_defn_index,
                                             struct_addr, struct_offset_vec_ptr,
                                             idl_false, IDL_msp);
                break;
            case IDL_DT_FULL_PTR:
                defn_vec_ptr++;     /* Properties byte */
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                /* Pointee definition */
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                node_number = rpc_ss_register_node( IDL_msp->IDL_node_table,
                        *(byte_p_t *)((idl_byte *)struct_addr+offset),
                        ndr_false, &already_marshalled );
                IDL_MARSH_ULONG( &node_number );
                break;
            case IDL_DT_UNIQUE_PTR:
                defn_vec_ptr++;     /* Properties byte */
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                /* Pointee definition */
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                /* Get a value of 0 if pointer is null, 1 otherwise */
                node_number = 
                        (*(byte_p_t *)((idl_byte *)struct_addr+offset) != NULL);
                IDL_MARSH_ULONG( &node_number );
                break;
            case IDL_DT_REF_PTR:
                defn_vec_ptr++;     /* Properties byte */
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                /* Pointee definition */
                offset_vec_ptr++;
                /* Aligned 4-byte place holder */
                IDL_MARSH_ALIGN_MP( IDL_msp, 4 );
                rpc_ss_ndr_marsh_check_buffer( 4, IDL_msp );
                IDL_msp->IDL_mp += 4;
                IDL_msp->IDL_left_in_buff -= 4;
                break;
            case IDL_DT_IGNORE:
                offset_vec_ptr++;
                /* Aligned 4-byte place holder */
                IDL_MARSH_ALIGN_MP( IDL_msp, 4 );
                rpc_ss_ndr_marsh_check_buffer( 4, IDL_msp );
                IDL_msp->IDL_mp += 4;
                IDL_msp->IDL_left_in_buff -= 4;
                break;
            case IDL_DT_STRING:
                /* Varying/open array code will do the right thing */
                break;
            case IDL_DT_TRANSMIT_AS:
            case IDL_DT_REPRESENT_AS:
                defn_vec_ptr++;     /* Skip over properties byte */
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                rpc_ss_ndr_marsh_xmit_as(field_defn_index,
                                       (idl_byte *)struct_addr+offset, IDL_msp);
                break;
#if 0
				case IDL_DT_INTERFACE:
					 defn_vec_ptr++;	/* properties byte */
					 IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
					 offset = *offset_vec_ptr;
					 offset_vec_ptr++;
					 rpc_ss_ndr_marsh_interface(field_defn_index, (idl_byte*)struct_addr+offset, IDL_msp);
					 break;
#endif
            case IDL_DT_V1_ARRAY:
                v1_array = idl_true;
                break;
            case IDL_DT_V1_STRING:
                defn_vec_ptr += 2;  /* DT_VARYING_ARRAY and properties */
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                        /* Full array defn */
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                      /* Flattened array defn */
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                rpc_ss_ndr_marsh_v1_string((idl_byte *)struct_addr+offset,
                                                       0, IDL_msp);
                break;
            case IDL_DT_CS_TYPE:
                defn_vec_ptr++;     /* Skip over properties byte */
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                rpc_ss_ndr_marsh_cs_char((idl_byte *)struct_addr+offset,
                                         field_defn_index, IDL_msp);
                break;
            case IDL_DT_CS_ATTRIBUTE:
                /* Is followed by an array attribute, which is an integer */
                rpc_ss_ndr_marsh_scalar(*defn_vec_ptr,
                        (rpc_void_p_t)&cs_shadow[offset_vec_ptr
                                        -(struct_offset_vec_ptr+1)].IDL_data,
                        IDL_msp);
                defn_vec_ptr++;     /* Attribute type */
                offset_vec_ptr++;
                break;
            case IDL_DT_CS_ARRAY:
                /* Is followed by an array description */
                offset = *offset_vec_ptr;
                rpc_ss_ndr_marsh_cs_array(
                        (rpc_void_p_t)((idl_byte *)struct_addr+offset),
                        cs_shadow,
                        offset_vec_ptr-(struct_offset_vec_ptr+1),
                        idl_true, &defn_vec_ptr, IDL_msp);
                offset_vec_ptr++;
                break;
            case IDL_DT_CS_RLSE_SHADOW:
                rpc_ss_ndr_m_rlse_cs_shadow(cs_shadow, shadow_length, IDL_msp);
                break;
            case IDL_DT_NDR_ALIGN_2:
                IDL_MARSH_ALIGN_MP( IDL_msp, 2 );
                break;
            case IDL_DT_NDR_ALIGN_4:
                IDL_MARSH_ALIGN_MP( IDL_msp, 4 );
                break;
            case IDL_DT_NDR_ALIGN_8:
                IDL_MARSH_ALIGN_MP( IDL_msp, 8 );
                break;
            case IDL_DT_BEGIN_NESTED_STRUCT:
            case IDL_DT_END_NESTED_STRUCT:
            case IDL_DT_EOL:
                break;
            case IDL_DT_RANGE:
                IDL_GET_RANGE_FROM_VECTOR( range_bounds, defn_vec_ptr );
                break;
            default:
#ifdef DEBUG_INTERP
                printf("rpc_ss_ndr_marsh_struct:unrecognized type %d\n",
                        type_byte);
                exit(0);
#endif
                DCETHREAD_RAISE(rpc_x_coding_error);
        }
    } while (type_byte != IDL_DT_EOL);
}

/******************************************************************************/
/*                                                                            */
/*  Marshall a contiguous set of elements one by one                          */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_by_looping
(
    /* [in] */  idl_ulong_int element_count,
    /* [in] */  idl_byte base_type,
    /* [in] */  rpc_void_p_t array_addr,
    /* [in] */  idl_ulong_int element_size,
                                /* Used if array of constructed type */
    /* [in] */  idl_ulong_int element_defn_index,
                                /* Used if array of constructed type */
    IDL_msp_t IDL_msp
)
{
    unsigned32 i;
    idl_ulong_int node_number;
    long already_marshalled;

    for (i=0; i<element_count; i++)
    {
        switch (base_type)
        {
            case IDL_DT_BOOLEAN:
                IDL_MARSH_BOOLEAN( array_addr );
                array_addr = (rpc_void_p_t)((idl_boolean *)(array_addr) + 1);
                break;
            case IDL_DT_BYTE:
                IDL_MARSH_BYTE( array_addr );
                array_addr = (rpc_void_p_t)((idl_byte *)(array_addr) + 1);
                break;
            case IDL_DT_CHAR:
                IDL_MARSH_CHAR( array_addr );
                array_addr = (rpc_void_p_t)((idl_char *)(array_addr) + 1);
                break;
            case IDL_DT_DOUBLE:
                IDL_MARSH_DOUBLE( array_addr );
                array_addr = (rpc_void_p_t)((idl_long_float *)(array_addr) + 1);
                break;
            case IDL_DT_ENUM:
                IDL_MARSH_ENUM( array_addr );
                array_addr = (rpc_void_p_t)((int *)(array_addr) + 1);
                break;
            case IDL_DT_V1_ENUM:
                IDL_MARSH_V1_ENUM( array_addr );
                array_addr = (rpc_void_p_t)((int *)(array_addr) + 1);
                break;
            case IDL_DT_FLOAT:
                IDL_MARSH_FLOAT( array_addr );
                array_addr = (rpc_void_p_t)
                                         ((idl_short_float *)(array_addr) + 1);
                break;
            case IDL_DT_SMALL:
                IDL_MARSH_SMALL( array_addr );
                array_addr = (rpc_void_p_t)((idl_small_int *)(array_addr) + 1);
                break;
            case IDL_DT_SHORT:
                IDL_MARSH_SHORT( array_addr );
                array_addr = (rpc_void_p_t)((idl_short_int *)(array_addr) + 1);
                break;
            case IDL_DT_LONG:
                IDL_MARSH_LONG( array_addr );
                array_addr = (rpc_void_p_t)((idl_long_int *)(array_addr) + 1);
                break;
            case IDL_DT_HYPER:
                IDL_MARSH_HYPER( array_addr );
                array_addr = (rpc_void_p_t)((idl_hyper_int *)(array_addr) + 1);
                break;
            case IDL_DT_USMALL:
                IDL_MARSH_USMALL( array_addr );
                array_addr = (rpc_void_p_t)((idl_usmall_int *)(array_addr) + 1);
                break;
            case IDL_DT_USHORT:
                IDL_MARSH_USHORT( array_addr );
                array_addr = (rpc_void_p_t)((idl_ushort_int *)(array_addr) + 1);
                break;
            case IDL_DT_ULONG:
                IDL_MARSH_ULONG( array_addr );
                array_addr = (rpc_void_p_t)((idl_ulong_int *)(array_addr) + 1);
                break;
            case IDL_DT_ERROR_STATUS:
                IDL_MARSH_ERROR_STATUS( array_addr );
                array_addr = (rpc_void_p_t)((idl_ulong_int *)(array_addr) + 1);
                break;
            case IDL_DT_UHYPER:
                IDL_MARSH_UHYPER( array_addr );
                array_addr = (rpc_void_p_t)((idl_uhyper_int *)(array_addr) + 1);
                break;
            case IDL_DT_FIXED_STRUCT:
                rpc_ss_ndr_marsh_struct( base_type, element_defn_index,
                                                        array_addr, IDL_msp);
                array_addr = (rpc_void_p_t)
                                    ((idl_byte *)(array_addr) + element_size);
                break;
            case IDL_DT_FIXED_ARRAY:
                /* Base type of pipe is array */
                rpc_ss_ndr_marsh_fixed_arr( element_defn_index, array_addr,
                                            0, IDL_msp );
                array_addr = (rpc_void_p_t)
                                    ((idl_byte *)(array_addr) + element_size);
                break;
            case IDL_DT_ENC_UNION:
                rpc_ss_ndr_m_enc_union_or_ptees( array_addr, element_defn_index,
                                                    idl_false, IDL_msp );
                array_addr = (rpc_void_p_t)
                                    ((idl_byte *)(array_addr) + element_size);
                break;
            case IDL_DT_FULL_PTR:
                node_number = rpc_ss_register_node( IDL_msp->IDL_node_table,
                                            *(byte_p_t *)array_addr,
                                            ndr_false, &already_marshalled );
                IDL_MARSH_ULONG( &node_number );
                array_addr = (rpc_void_p_t)((rpc_void_p_t *)(array_addr) + 1);
                break;
            case IDL_DT_UNIQUE_PTR:
                /* Get a value of 0 if pointer is null, 1 otherwise */
                node_number = (*(byte_p_t *)array_addr != NULL);
                IDL_MARSH_ULONG( &node_number );
                array_addr = (rpc_void_p_t)((rpc_void_p_t *)(array_addr) + 1);
                break;
            case IDL_DT_STRING:
#if defined(PACKED_BYTE_ARRAYS) && defined(PACKED_CHAR_ARRAYS) \
 && defined(PACKED_SCALAR_ARRAYS)
                if (IDL_msp->IDL_language == IDL_lang_c_k)
                {
                    idl_byte *element_defn_ptr;
                    idl_ulong_int A=0, B;
                    idl_ulong_int base_type_size;

                    element_defn_ptr = IDL_msp->IDL_type_vec
                                        + element_defn_index
                                        + 1     /* Dimensionality */
                                        + IDL_FIXED_BOUND_PAIR_WIDTH
                                        + IDL_DATA_LIMIT_PAIR_WIDTH;
                    /* Now pointing at base type of string */
                    base_type_size = rpc_ss_type_size(element_defn_ptr,
                                                                     IDL_msp);
                    if (base_type_size == 1)
                        B = strlen(array_addr) + 1;
                    else
                        B = rpc_ss_strsiz((idl_char *)array_addr,
                                                               base_type_size);
                    IDL_MARSH_ALIGN_MP(IDL_msp, 4);
                    rpc_ss_ndr_marsh_check_buffer(8, IDL_msp);
                    rpc_marshall_ulong_int(IDL_msp->IDL_mp, A);
                    IDL_msp->IDL_mp += 4;
                    rpc_marshall_ulong_int(IDL_msp->IDL_mp, B);
                    IDL_msp->IDL_mp += 4;
                    IDL_msp->IDL_left_in_buff -= 8;
                    rpc_ss_ndr_marsh_by_copying(B, base_type_size, array_addr,
                                                IDL_msp);
                }
                else
#endif
                {
                    rpc_ss_ndr_marsh_varying_arr(element_defn_index, array_addr,
                                                 NULL, NULL, 0, IDL_msp);
                }
                array_addr = (rpc_void_p_t)
                                    ((idl_byte *)(array_addr) + element_size);
                break;
            case IDL_DT_V1_STRING:
                rpc_ss_ndr_marsh_v1_string(array_addr, 0, IDL_msp);
                array_addr = (rpc_void_p_t)
                                    ((idl_byte *)(array_addr) + element_size);
                break;
            case IDL_DT_TRANSMIT_AS:
            case IDL_DT_REPRESENT_AS:
                rpc_ss_ndr_marsh_xmit_as( element_defn_index, array_addr,
                                                                    IDL_msp );
                array_addr = (rpc_void_p_t)
                                    ((idl_byte *)(array_addr) + element_size);
                break;
#if 0
				case IDL_DT_INTERFACE:
					 rpc_ss_ndr_marsh_interface(element_defn_index, array_addr, IDL_msp);
					 array_addr = (rpc_void_p_t)((idl_byte*)(array_addr) + element_size);
					 break;
#endif
            default:
#ifdef DEBUG_INTERP
                printf(
                      "rpc_ss_ndr_marsh_by_looping:unrecognized type %d\n",
                        base_type);
                exit(0);
#endif
                DCETHREAD_RAISE(rpc_x_coding_error);
        }
    }
}

/******************************************************************************/
/*                                                                            */
/* Marshall a fixed or conformant array                                       */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_m_fix_or_conf_arr
(
    /* [in] */  rpc_void_p_t array_addr,
    /* [in] */  idl_ulong_int dimensionality,
    /* [in] */  IDL_bound_pair_t *bounds_list,
    /* [in] */  idl_byte *defn_vec_ptr, /* Points at array base info */
    /* [in] */  idl_ulong_int flags,
    IDL_msp_t IDL_msp
)
{
    idl_ulong_int element_count;
    int i;
    idl_ulong_int element_defn_index;
    idl_ulong_int element_offset_index;
    idl_ulong_int element_size;
    idl_byte *element_defn_ptr;
    idl_byte base_type;
    idl_boolean marshall_by_pointing;
    IDL_bound_pair_t *bound_p;	    /* Steps through bounds */

    if ( (*defn_vec_ptr == IDL_DT_STRING)
        || (*defn_vec_ptr == IDL_DT_V1_STRING) )
    {
        /* Arrays of strings have a special representation */
        dimensionality--;
    }

    /* Calculate the total number of elements to be transmited */
    element_count = 1;
    bound_p = bounds_list;
    for (i=dimensionality; i>0; i--)
    {
        element_count *= (bound_p->upper - bound_p->lower + 1);
	bound_p++;
    }
    if (element_count == 0)
        return;

    rpc_ss_ndr_arr_align_and_opt( IDL_marshalling_k, dimensionality, 
                                  &base_type, defn_vec_ptr,
                                  &marshall_by_pointing, IDL_msp );

    if (base_type == IDL_DT_REF_PTR)
    {
        return;
    }


    /* Marshall by pointing if possible and enough elements or chunk of pipe of
         array (only case where array base type is DT_FIXED_ARRAY) */
    if (marshall_by_pointing)
    {
        element_size = rpc_ss_type_size( defn_vec_ptr, IDL_msp );
        if (base_type != IDL_DT_FIXED_STRUCT)
        {
	    /*
	     * 	Marshall by pointing only if the object is at least
	     * 	IDL_POINT_THRESHOLD items in size, or it is the base type of a
	     * 	pipe (array of fixed array).  Since there is only a single pipe
	     * 	chunk per transmit, may as well point at it no matter its size.
             *  Can never marshall by pointing when pickling
	     */
            if (((element_count >= IDL_POINT_THRESHOLD)
                                    || (base_type == IDL_DT_FIXED_ARRAY))
                    && (! IDL_M_FLAGS_TEST(flags, IDL_M_DO_NOT_POINT))
                    && (IDL_msp->IDL_pickling_handle == NULL)
                )
                rpc_ss_ndr_marsh_by_pointing(element_count, element_size,
                                         array_addr, IDL_msp);
            else
                rpc_ss_ndr_marsh_by_copying(element_count, element_size,
                                         array_addr, IDL_msp);
	    return;
	}
	else  /* For NDR-aligned fixed structures */
	{
            /*
	     * 	The last element of the structure cannot, in general, be
	     * 	marshalled via the point-at optimization because of trailing
	     * 	pad is returned from the sizeof() function that is not included
	     * 	in NDR.
	     */
            element_count--;        /* So do the last one separately */

	    /* If there are more that 1, do them by pointing */
	    if (element_count != 0)
	    {
		if (((element_count*element_size) >= IDL_POINT_THRESHOLD)
                        && (! IDL_M_FLAGS_TEST(flags, IDL_M_DO_NOT_POINT))
                        && (IDL_msp->IDL_pickling_handle == NULL)
                    )
		    rpc_ss_ndr_marsh_by_pointing(element_count, element_size,
					     array_addr, IDL_msp);
		else
		    rpc_ss_ndr_marsh_by_copying(element_count, element_size,
					     array_addr, IDL_msp);
	    }

            /* Marshall the last element separately */
            defn_vec_ptr += 2;  /* See comment below */
            IDL_GET_LONG_FROM_VECTOR( element_defn_index, defn_vec_ptr );
            rpc_ss_ndr_marsh_struct(base_type, element_defn_index,
                                    (rpc_void_p_t)((idl_byte *)array_addr
                                         + element_count * element_size),
                                    IDL_msp);
        }
        return;
    }

    if ( (base_type == IDL_DT_FIXED_STRUCT)
         || (base_type == IDL_DT_ENC_UNION)
         || (base_type == IDL_DT_TRANSMIT_AS)
         || (base_type == IDL_DT_REPRESENT_AS) )
    {
        defn_vec_ptr += 2;  /* Discard type and properties */
        /* If we are marshalling an array, not a pipe, defn_vec_ptr was
            4-byte aligned and DT_MODIFIED and modifier are discarded
            by the +=2 followed by GET_LONG */
        IDL_GET_LONG_FROM_VECTOR( element_defn_index, defn_vec_ptr );
        element_defn_ptr =  IDL_msp->IDL_type_vec + element_defn_index;
        IDL_GET_LONG_FROM_VECTOR(element_offset_index, element_defn_ptr);
        element_size = *(IDL_msp->IDL_offset_vec + element_offset_index);
    }
    else if ( (base_type == IDL_DT_STRING)
            || (base_type == IDL_DT_V1_STRING) )
    {
        rpc_ss_get_string_base_desc( defn_vec_ptr, &element_size,
                                               &element_defn_index, IDL_msp );
    }
    else if (base_type == IDL_DT_FIXED_ARRAY)
    {
        /* Base type of pipe is array */
        element_size = rpc_ss_type_size( defn_vec_ptr, IDL_msp );
        defn_vec_ptr += 2;  /* Base type and properties byte */
        IDL_DISCARD_LONG_FROM_VECTOR( defn_vec_ptr );   /* Full array defn */
        IDL_GET_LONG_FROM_VECTOR( element_defn_index, defn_vec_ptr );
    }
#if 0
	 else if (base_type == IDL_DT_INTERFACE)	{
		 element_size = sizeof(void*);
		 defn_vec_ptr += 2; /* skip base and properties bytes */
		 IDL_GET_LONG_FROM_VECTOR(element_defn_index, defn_vec_ptr);
	 }
#endif

    /* element_index, element_defn_ptr may not be set, but in these cases
        the procedure calls following do not use them */
        rpc_ss_ndr_marsh_by_looping(element_count,
              base_type, array_addr, element_size, element_defn_index, IDL_msp);
}

/******************************************************************************/
/*                                                                            */
/*  Marshall a fixed array                                                    */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_fixed_arr
(
    /* [in] */  idl_ulong_int defn_index,
    /* [in] */  rpc_void_p_t array_addr,
    /* [in] */  idl_ulong_int flags,
    IDL_msp_t IDL_msp
)
{
    idl_byte *defn_vec_ptr;
    idl_ulong_int dimensionality;
    IDL_bound_pair_t *bounds_list;

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index; 
    dimensionality = (idl_ulong_int)*defn_vec_ptr;
    defn_vec_ptr++;
    /* By design defn_vec_ptr is now aligned (0 mod 4) */
    if (IDL_msp->IDL_type_vec[TVEC_INT_REP_OFFSET] != NDR_LOCAL_INT_REP)
      rpc_ss_fixed_bounds_from_vector(dimensionality, defn_vec_ptr, 
				      &bounds_list, IDL_msp);
    else
      bounds_list = (IDL_bound_pair_t *)defn_vec_ptr;
    rpc_ss_ndr_m_fix_or_conf_arr( array_addr, dimensionality, bounds_list,
                    defn_vec_ptr + dimensionality * IDL_FIXED_BOUND_PAIR_WIDTH,
                    flags, IDL_msp );
    if (IDL_msp->IDL_type_vec[TVEC_INT_REP_OFFSET] != NDR_LOCAL_INT_REP)
      rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)bounds_list);
}

/******************************************************************************/
/*                                                                            */
/* Marshall A,B pairs                                                         */
/*                                                                            */
/******************************************************************************/
static void rpc_ss_ndr_marsh_A_B_pairs
(
    /* [in] */  idl_ulong_int dimensionality,
    /* [in] */  IDL_bound_pair_t *range_list,
    IDL_msp_t IDL_msp
)
{
    unsigned32 i;
    idl_ulong_int A,B;

    for (i=0; i<dimensionality; i++)
    {
        A = range_list[i].lower;
        IDL_MARSH_ULONG( &A );
        B = range_list[i].upper - A;
        IDL_MARSH_ULONG( &B );
    }
}

/******************************************************************************/
/*                                                                            */
/* Marshall a varying or open array                                           */
/*                                                                            */
/******************************************************************************/
static idl_byte null_byte = 0;

void rpc_ss_ndr_m_var_or_open_arr
(
    /* [in] */  rpc_void_p_t array_addr,
    /* [in] */  idl_ulong_int *Z_values,
    /* [in] */  idl_ulong_int dimensionality,
    /* [in] */  IDL_bound_pair_t *range_list,
    /* [in] */  idl_byte *defn_vec_ptr, /* On entry points at array base info */
    /* [in] */ idl_ulong_int flags,
    IDL_msp_t IDL_msp
)
{
    idl_ulong_int element_defn_index;
    idl_ulong_int element_size;
    idl_byte base_type;
    IDL_varying_control_t *control_data;    /* List of data used to control
                                                marshalling loops */
    IDL_varying_control_t normal_control_data[IDL_NORMAL_DIMS];
    int i;
    idl_byte *inner_slice_address;  /* Address of 1-dim subset of array */
    int dim;    /* Index through array dimensions */
    idl_boolean contiguous;
    idl_ulong_int element_count;
    idl_boolean marshall_by_pointing;
    rpc_void_p_t point_addr;    /* Address to be used in marsh by pointing */
    idl_ushort_int v1_count;

    if ( (*defn_vec_ptr == IDL_DT_STRING)
        || (*defn_vec_ptr == IDL_DT_V1_STRING) )
    {
        /* Arrays of strings have a special representation */
        dimensionality--;
    }


    if (IDL_M_FLAGS_TEST(flags, IDL_M_V1_ARRAY))
    {
        /* Marshall one short word of varyingness info */
        v1_count =  1;
        for ( i=0; (unsigned32)i<dimensionality; i++)
        {
            v1_count *= (range_list[i].upper - range_list[i].lower);
        }
        IDL_MARSH_CUSHORT( &v1_count );
        if (v1_count == 0)
        {
            if ( rpc_ss_bug_1_thru_31(IDL_BUG_1|IDL_BUG_2, IDL_msp) )
            {
                rpc_ss_ndr_arr_align_and_opt( IDL_marshalling_k, dimensionality,
                                              &base_type, defn_vec_ptr,
                                              &marshall_by_pointing, IDL_msp );
                if ( rpc_ss_bug_1_thru_31(IDL_BUG_1, IDL_msp) 
                        && ( (base_type == IDL_DT_FIXED_STRUCT)
                                || (base_type == IDL_DT_ENC_UNION)
                                || (base_type == IDL_DT_TRANSMIT_AS) ) )
                {
                    /* -bug 1 and non-scalar base type for [v1_array] */
                    idl_ulong_int bug_1_align;
                    bug_1_align = rpc_ss_ndr_bug_1_align(defn_vec_ptr, IDL_msp);
                    IDL_MARSH_ALIGN_MP( IDL_msp, bug_1_align );
                }
            }
            return;
        }
    }
    else
    {
        if (IDL_M_FLAGS_TEST(flags, IDL_M_ADD_NULL))
        {
            /* This is a one-dimensional array and the count of elements
                to be marshalled does not include the trailing null */
            (range_list[0].upper)++;
        }
        rpc_ss_ndr_marsh_A_B_pairs(dimensionality, range_list, IDL_msp);
        if (IDL_M_FLAGS_TEST(flags, IDL_M_ADD_NULL))
        {
            (range_list[0].upper)--;
        }
        for ( i=0; (unsigned32)i<dimensionality; i++)
        {
            if (range_list[i].upper ==  range_list[i].lower)
            {
                /* No elements to marshall */
                return;
            }
        }
    }

    rpc_ss_ndr_arr_align_and_opt( IDL_marshalling_k, dimensionality, &base_type,
                                defn_vec_ptr, &marshall_by_pointing, IDL_msp );
    if ((IDL_M_FLAGS_TEST(flags, IDL_M_ADD_NULL))
                    || (IDL_M_FLAGS_TEST(flags, IDL_M_DO_NOT_POINT)))
        marshall_by_pointing = idl_false;

    if (base_type == IDL_DT_REF_PTR)
    {
        return;
    }
    else if ( (base_type == IDL_DT_STRING)
            || (base_type == IDL_DT_V1_STRING) )
        rpc_ss_get_string_base_desc(defn_vec_ptr, &element_size,
                                    &element_defn_index, IDL_msp);
    else
        element_size = rpc_ss_type_size(defn_vec_ptr, IDL_msp); 

    if (marshall_by_pointing)
    {
        point_addr = array_addr;
        rpc_ss_ndr_contiguous_elt( dimensionality, Z_values, range_list,
                                   element_size, &contiguous, &element_count,
                                   &point_addr );
        if (contiguous)
        {
            if (base_type != IDL_DT_FIXED_STRUCT)
            {
                if ((element_count >= IDL_POINT_THRESHOLD)
                            && (IDL_msp->IDL_pickling_handle == NULL)
                    )
                    rpc_ss_ndr_marsh_by_pointing(element_count, element_size,
                                             point_addr, IDL_msp);
                else
                    rpc_ss_ndr_marsh_by_copying(element_count, element_size,
                                             point_addr, IDL_msp);
		return;
	    }
	    else /* For NDR-aligned fixed structures */
	    {
		/*
		 * 	The last element of the structure cannot, in general, be
		 * 	marshalled via the point-at optimization because of trailing
		 * 	pad is returned from the sizeof() function that is not included
		 * 	in NDR.
		 */
                element_count--;        /* So do the last one separately */

		/* If there are more that 1, do them by pointing */
		if (element_count != 0)
		{
		    if (((element_count*element_size) >= IDL_POINT_THRESHOLD)
                            && (IDL_msp->IDL_pickling_handle == NULL)
                        )
			rpc_ss_ndr_marsh_by_pointing(element_count, element_size,
						 point_addr, IDL_msp);
		    else
			rpc_ss_ndr_marsh_by_copying(element_count, element_size,
						 point_addr, IDL_msp);
		}

                /* Marshall the last element separately */
                defn_vec_ptr += 4;  /* See comment below */
                IDL_GET_LONG_FROM_VECTOR( element_defn_index, defn_vec_ptr );
                rpc_ss_ndr_marsh_struct(base_type, element_defn_index,
                                    (rpc_void_p_t)((idl_byte *)point_addr
                                             + element_count * element_size),
                                    IDL_msp);
            }
            return;
        }
    }


    if ( (base_type == IDL_DT_FIXED_STRUCT)
         || (base_type == IDL_DT_ENC_UNION)
         || (base_type == IDL_DT_TRANSMIT_AS)
         || (base_type == IDL_DT_REPRESENT_AS) )
    {
        /* Discard any of DT_MODIFIED, type, modifier, properties
            which may be present, and any filler bytes */
        defn_vec_ptr += 4;
        IDL_GET_LONG_FROM_VECTOR( element_defn_index, defn_vec_ptr );
    }


    if (dimensionality > IDL_NORMAL_DIMS)
    {
        control_data = (IDL_varying_control_t *)rpc_ss_mem_alloc(
                                &IDL_msp->IDL_mem_handle,
                                dimensionality * sizeof(IDL_varying_control_t));
    }
    else
        control_data = normal_control_data;
    control_data[dimensionality-1].subslice_size = element_size;
    control_data[dimensionality-1].index_value = 
                                            range_list[dimensionality-1].lower;
    for (i=dimensionality-2; i>=0; i--)
    {
        control_data[i].index_value = range_list[i].lower;
        control_data[i].subslice_size = control_data[i+1].subslice_size
                                                            * Z_values[i+1];   
    }

    do {
        inner_slice_address = (idl_byte *)array_addr;
        for (i=0; (unsigned32)i<dimensionality; i++)
        {
            inner_slice_address += control_data[i].index_value
                                        * control_data[i].subslice_size;
        }
        rpc_ss_ndr_marsh_by_looping(
                 range_list[dimensionality-1].upper
                                         - range_list[dimensionality-1].lower,
                 base_type, (rpc_void_p_t)inner_slice_address, 
                 element_size, element_defn_index, IDL_msp);
        dim = dimensionality - 2;
        while (dim >= 0)
        {
            control_data[dim].index_value++;
            if (control_data[dim].index_value < (unsigned32)range_list[dim].upper)
                break;
            control_data[dim].index_value = range_list[dim].lower;
            dim--;
        }
    } while (dim >= 0);

    if (dimensionality > IDL_NORMAL_DIMS)
        rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)control_data);

    if (IDL_M_FLAGS_TEST(flags, IDL_M_ADD_NULL))
    {
        /* Number of null bytes required is character width */
        for (i=0; (unsigned32)i<element_size; i++)
        {
            IDL_MARSH_BYTE( &null_byte );
        }
    }
}

/******************************************************************************/
/*                                                                            */
/*  Marshall a varying array                                                  */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_varying_arr
(
    /* [in] */  idl_ulong_int defn_index,
                                    /* Index of start of array description */
    /* [in] */  rpc_void_p_t array_addr,
    /* [in] */  rpc_void_p_t struct_addr,   /* Address of structure array is
                                part of. NULL if array is not structure field */
    /* [in] */  idl_ulong_int *struct_offset_vec_ptr,
                                      /* NULL if array is not structure field */
    /* [in] */ idl_ulong_int flags,
    IDL_msp_t IDL_msp
)
{
    idl_byte *defn_vec_ptr;
    idl_ulong_int dimensionality;
    IDL_bound_pair_t *bounds_list;
    idl_ulong_int *Z_values;
    idl_ulong_int normal_Z_values[IDL_NORMAL_DIMS];
    IDL_bound_pair_t *range_list;
    IDL_bound_pair_t normal_range_list[IDL_NORMAL_DIMS];
    idl_boolean add_null;
    rpc_void_p_t array_data_addr; /* May need to decode descriptor to get
                                        the address of the array data */

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index; 
    dimensionality = (idl_ulong_int)*defn_vec_ptr;
    defn_vec_ptr++;
    /* By design we are now longword aligned */
    if (IDL_msp->IDL_type_vec[TVEC_INT_REP_OFFSET] != NDR_LOCAL_INT_REP)
      rpc_ss_fixed_bounds_from_vector(dimensionality, defn_vec_ptr, &bounds_list,
                                    IDL_msp);
    else
      bounds_list = (IDL_bound_pair_t *)defn_vec_ptr;
    if (dimensionality > IDL_NORMAL_DIMS)
    {
        Z_values = NULL;
        range_list = NULL;
    }
    else
    {
        Z_values = normal_Z_values;
        range_list = normal_range_list;
    }
    rpc_ss_Z_values_from_bounds( bounds_list, dimensionality, &Z_values,
                                                                     IDL_msp );
    defn_vec_ptr += dimensionality * IDL_FIXED_BOUND_PAIR_WIDTH;
    /* The address of the array data is only needed when a range list is
        being built for [string] data */
        array_data_addr = array_addr;

    rpc_ss_build_range_list( &defn_vec_ptr, array_data_addr, struct_addr,
                        struct_offset_vec_ptr, dimensionality, bounds_list, 
                        &range_list, &add_null, IDL_msp );
    rpc_ss_ndr_m_var_or_open_arr( array_addr, Z_values, dimensionality,
                                  range_list, defn_vec_ptr,
                                  flags | (add_null ? IDL_M_ADD_NULL : 0),
                                  IDL_msp );
    if (dimensionality > IDL_NORMAL_DIMS)
    {
        rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)range_list);
        rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)Z_values);
    }
    if (IDL_msp->IDL_type_vec[TVEC_INT_REP_OFFSET] != NDR_LOCAL_INT_REP)
      rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)bounds_list);
}

/******************************************************************************/
/*                                                                            */
/*  Marshall the Z values for a conformant or open array                      */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_Z_values
(
    /* [in] */  idl_ulong_int dimensionality,
    /* [in] */  idl_ulong_int *Z_values,
    IDL_msp_t IDL_msp
)
{
    unsigned32 i;

    for (i=0; i<dimensionality; i++)
    {
        IDL_MARSH_ULONG( &Z_values[i] );
    }
}

/******************************************************************************/
/*                                                                            */
/*  Marshall a conformant array parameter                                     */
/*                                                                            */
/******************************************************************************/
static void rpc_ss_ndr_marsh_conf_arr
(
    /* [in] */  idl_ulong_int defn_index,
    /* [in] */  rpc_void_p_t array_addr,
    IDL_msp_t IDL_msp
)
{
    idl_byte *defn_vec_ptr;
    idl_ulong_int dimensionality;
    IDL_bound_pair_t *bounds_list;
    IDL_bound_pair_t normal_bounds_list[IDL_NORMAL_DIMS];
    idl_ulong_int *Z_values;
    idl_ulong_int normal_Z_values[IDL_NORMAL_DIMS];

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index; 
    dimensionality = (idl_ulong_int)*defn_vec_ptr;
    defn_vec_ptr++;
    if (dimensionality > IDL_NORMAL_DIMS)
    {
        bounds_list = NULL;
        Z_values = NULL;
    }
    else
    {
        bounds_list = normal_bounds_list;
        Z_values = normal_Z_values;
    }
    rpc_ss_build_bounds_list( &defn_vec_ptr, array_addr, NULL, NULL,
                              dimensionality, &bounds_list, IDL_msp );
    rpc_ss_Z_values_from_bounds(bounds_list, dimensionality, &Z_values,IDL_msp);
    rpc_ss_ndr_marsh_Z_values( dimensionality, Z_values, IDL_msp );
    rpc_ss_ndr_m_fix_or_conf_arr( array_addr, dimensionality, bounds_list,
                                defn_vec_ptr, IDL_M_IS_PARAM | IDL_M_CONF_ARRAY,
                                 IDL_msp );
    if (dimensionality > IDL_NORMAL_DIMS)
    {
        rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)Z_values);
        rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)bounds_list);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Marshall an open array                                                    */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_open_arr
(
    /* [in] */  idl_ulong_int defn_index,
    /* [in] */  rpc_void_p_t array_addr,
    /* [in] */ idl_ulong_int flags,
    IDL_msp_t IDL_msp
)
{
    idl_byte *defn_vec_ptr;
    idl_ulong_int dimensionality;
    IDL_bound_pair_t *bounds_list;
    IDL_bound_pair_t normal_bounds_list[IDL_NORMAL_DIMS];
    idl_ulong_int *Z_values;
    idl_ulong_int normal_Z_values[IDL_NORMAL_DIMS];
    IDL_bound_pair_t *range_list;
    IDL_bound_pair_t normal_range_list[IDL_NORMAL_DIMS];
    idl_ushort_int v1_size;
    idl_boolean add_null;
    rpc_void_p_t array_data_addr; /* May need to decode descriptor to get
                                        the address of the array_data */
    unsigned32 i;

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index; 
    dimensionality = (idl_ulong_int)*defn_vec_ptr;
    defn_vec_ptr++;
    if (dimensionality > IDL_NORMAL_DIMS)
    {
        bounds_list = NULL;
        Z_values = NULL;
        range_list = NULL;
    }
    else
    {
        bounds_list = normal_bounds_list;
        Z_values = normal_Z_values;
        range_list = normal_range_list;
    }
    {
        array_data_addr = array_addr;
    }
    rpc_ss_build_bounds_list( &defn_vec_ptr, array_data_addr, NULL, NULL,
                                    dimensionality, &bounds_list, IDL_msp );
    rpc_ss_Z_values_from_bounds( bounds_list, dimensionality, &Z_values,
                                                                     IDL_msp );
    rpc_ss_build_range_list( &defn_vec_ptr, array_data_addr, NULL,
                        NULL, dimensionality, bounds_list, 
                        &range_list, &add_null, IDL_msp );
    if (IDL_M_FLAGS_TEST(flags, IDL_M_V1_ARRAY))
    {
        v1_size = 1;
        for ( i=0; i<dimensionality; i++ )
            v1_size *= Z_values[i];
        IDL_MARSH_CUSHORT( &v1_size );
    }
    else
        rpc_ss_ndr_marsh_Z_values( dimensionality, Z_values, IDL_msp );
    rpc_ss_ndr_m_var_or_open_arr( array_addr, Z_values, dimensionality,
                                  range_list, defn_vec_ptr,
                                  flags | (add_null ? IDL_M_ADD_NULL : 0),
                                  IDL_msp );
    if (dimensionality > IDL_NORMAL_DIMS)
    {
        rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)range_list);
        rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)Z_values);
        rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)bounds_list);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Discard the description following DT_ALLOCATE_REF                         */
/*                                                                            */
/******************************************************************************/
void rpc_ss_discard_allocate_ref
(
    /* [in,out] */ idl_byte **p_type_vec_ptr
                            /* On entry type_vec_ptr points to type of parameter
                                         It is advanced over type definition */
)
{
    idl_byte type_byte;
    idl_byte *type_vec_ptr = *p_type_vec_ptr;

    type_byte = *type_vec_ptr;
    type_vec_ptr++;
    switch (type_byte)
    {
        case IDL_DT_FIXED_STRUCT:
            type_vec_ptr++;     /* Ignore properties byte */
            IDL_DISCARD_LONG_FROM_VECTOR( type_vec_ptr );   /* Struct defn */
            break;
        case IDL_DT_FIXED_ARRAY:
        case IDL_DT_VARYING_ARRAY:
            type_vec_ptr++;     /* Ignore properties byte */
            IDL_DISCARD_LONG_FROM_VECTOR( type_vec_ptr );  /* Full array defn */
            IDL_DISCARD_LONG_FROM_VECTOR( type_vec_ptr );  /* Flat array defn */
            break;
        case IDL_DT_REF_PTR:
            type_vec_ptr++;     /* Ignore properties byte */
            IDL_DISCARD_LONG_FROM_VECTOR( type_vec_ptr );   /* Pointee defn */
            break;
        default:
#ifdef DEBUG_INTERP
            printf("rpc_ss_discard_allocate_ref:unrecognized type %d\n",
                        type_byte);
            exit(0);
#endif
            DCETHREAD_RAISE(rpc_x_coding_error);
    }
    *p_type_vec_ptr = type_vec_ptr;
}


/******************************************************************************/
/*                                                                            */
/*  Control for marshalling                                                   */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_interp
(
    idl_ulong_int IDL_parameter_count, /* [in] -- Number of parameters to   */
                                  /* marshall in this call to the           */
                                  /* interpreter                            */

    idl_ulong_int IDL_type_index,    /* [in] -- Offset into the type vector */
                               /* for the start of the parameter descriptions */

    rpc_void_p_t IDL_param_vector[], /* [in,out] -- The addresses of each of */
                                  /* the the parameters thus it's size is   */
                                  /* the number of parameters in the        */
                                  /* signature of the operation             */

    IDL_msp_t IDL_msp        /* [in,out] -- Pointer to marshalling state   */
)
{
    idl_byte *type_vec_ptr;
    idl_byte type_byte;
    idl_ulong_int param_index;
    rpc_void_p_t param_addr;
    idl_ulong_int defn_index;
#if 0
	 idl_ulong_int rtn_index, iid_index;
#endif
    idl_boolean type_has_pointers;
    idl_boolean v1_array = idl_false;
    idl_boolean is_string = idl_false;
    idl_ulong_int node_number;
    long already_marshalled;
    idl_byte *pointee_defn_ptr;
    IDL_pointee_desc_t pointee_desc;    /* Description of pointee */
    idl_ulong_int switch_index; /* Index of switch param for non-encapsulated
                                                                        union */
    idl_ulong_int routine_index; /* Index of rtn to be used to free referents
                       of [in]-only [transmit_as] or [represent_as] parameter */
    IDL_cs_shadow_elt_t *cs_shadow;     /* cs-shadow for parameter list */
    idl_ulong_int shadow_length;
    IDL_bound_pair_t range_bounds;

    IDL_msp->IDL_marsh_pipe = idl_false;
    IDL_msp->IDL_m_xmit_level = 0;
    type_vec_ptr = (IDL_msp->IDL_type_vec) + IDL_type_index;

    IDL_INIT_RANGE(range_bounds);

    /* Loop over parameters */
    for ( ; IDL_parameter_count > 0; IDL_parameter_count -- )
    {
        IDL_GET_LONG_FROM_VECTOR(param_index,type_vec_ptr);
        param_addr = IDL_param_vector[param_index];
        do {
            type_byte = *type_vec_ptr;
            type_vec_ptr++;
            switch(type_byte)
            {
                case IDL_DT_BYTE:
                    IDL_CHECK_RANGE_BYTE( range_bounds, param_addr );
                    IDL_MARSH_BYTE( param_addr );
                    break;
                case IDL_DT_CHAR:
                    IDL_CHECK_RANGE_CHAR( range_bounds, param_addr );
                    IDL_MARSH_CHAR( param_addr );
                    break;
                case IDL_DT_BOOLEAN:
                    IDL_CHECK_RANGE_BOOLEAN( range_bounds, param_addr );
                    IDL_MARSH_BOOLEAN( param_addr );
                    break;
                case IDL_DT_DOUBLE:
                    IDL_CHECK_RANGE_DOUBLE( range_bounds, param_addr );
                    IDL_MARSH_DOUBLE( param_addr );
                    break;
                case IDL_DT_ENUM:
                    IDL_MARSH_ENUM( param_addr );
                    break;
                case IDL_DT_FLOAT:
                    IDL_CHECK_RANGE_FLOAT( range_bounds, param_addr );
                    IDL_MARSH_FLOAT( param_addr );
                    break;
                case IDL_DT_SMALL:
                    IDL_CHECK_RANGE_SMALL( range_bounds, param_addr );
                    IDL_MARSH_SMALL( param_addr );
                    break;
                case IDL_DT_SHORT:
                    IDL_CHECK_RANGE_SHORT( range_bounds, param_addr );
                    IDL_MARSH_SHORT( param_addr );
                    break;
                case IDL_DT_LONG:
                    IDL_CHECK_RANGE_LONG( range_bounds, param_addr );
                    IDL_MARSH_LONG( param_addr );
                    break;
                case IDL_DT_HYPER:
                    IDL_MARSH_HYPER( param_addr );
                    break;
                case IDL_DT_USMALL:
                    IDL_CHECK_RANGE_USMALL( range_bounds, param_addr );
                    IDL_MARSH_USMALL( param_addr );
                    break;
                case IDL_DT_USHORT:
                    IDL_CHECK_RANGE_USHORT( range_bounds, param_addr );
                    IDL_MARSH_USHORT( param_addr );
                    break;
                case IDL_DT_ULONG:
                    IDL_CHECK_RANGE_ULONG( range_bounds, param_addr );
                    IDL_MARSH_ULONG( param_addr );
                    break;
                case IDL_DT_UHYPER:
                    IDL_MARSH_UHYPER( param_addr );
                    break;
                case IDL_DT_V1_ENUM:
                    IDL_MARSH_V1_ENUM( param_addr );
                    break;
                case IDL_DT_ERROR_STATUS:
                    IDL_MARSH_ERROR_STATUS( param_addr );
                    break;
                case IDL_DT_FIXED_STRUCT:
                case IDL_DT_CONF_STRUCT:
                case IDL_DT_V1_CONF_STRUCT:
                    /* Properties byte */
                    type_has_pointers =
                             IDL_PROP_TEST(*type_vec_ptr, IDL_PROP_HAS_PTRS);
                    type_vec_ptr++;
                    IDL_GET_LONG_FROM_VECTOR(defn_index,type_vec_ptr);
                    rpc_ss_ndr_marsh_struct(type_byte, defn_index, param_addr,
                                                                     IDL_msp);
                    if (type_has_pointers)
                    {
                        rpc_ss_ndr_m_struct_pointees(type_byte, defn_index,
                                                     param_addr, IDL_msp);
                    }
                    break;
                case IDL_DT_FIXED_ARRAY:
                    /* Properties byte */
                    type_has_pointers =
                             IDL_PROP_TEST(*type_vec_ptr, IDL_PROP_HAS_PTRS);
                    type_vec_ptr++;
                    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                                            /* Discard full array definition */
                    IDL_GET_LONG_FROM_VECTOR(defn_index,type_vec_ptr);
                    rpc_ss_ndr_marsh_fixed_arr(defn_index, param_addr, 
                                               IDL_M_IS_PARAM, IDL_msp);
                    if (type_has_pointers)
                    {
                        rpc_ss_ndr_m_dfc_arr_ptees(defn_index, param_addr,
                                                     NULL, NULL, 0, IDL_msp);
                    }
                    break;
                case IDL_DT_VARYING_ARRAY:
                    /* Properties byte */
                    type_has_pointers =
                             IDL_PROP_TEST(*type_vec_ptr, IDL_PROP_HAS_PTRS);
                    type_vec_ptr++;
                    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                                            /* Discard full array definition */
                    IDL_GET_LONG_FROM_VECTOR(defn_index,type_vec_ptr);
                    rpc_ss_ndr_marsh_varying_arr(defn_index, param_addr,
                                         NULL, NULL,
                                         (v1_array ? IDL_M_V1_ARRAY : 0)
                                             | (is_string ? IDL_M_IS_STRING : 0)
                                             | IDL_M_IS_PARAM,
                                         IDL_msp);
                    if (type_has_pointers)
                    {
                        rpc_ss_ndr_m_dvo_arr_ptees(defn_index, param_addr,
                                                        NULL, NULL, 0, IDL_msp);
                    }
                    v1_array = idl_false;
                    is_string = idl_false;
                    break;
                case IDL_DT_CONF_ARRAY:
                    /* Properties byte */
                    type_has_pointers =
                             IDL_PROP_TEST(*type_vec_ptr, IDL_PROP_HAS_PTRS);
                    type_vec_ptr++;
                    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                                            /* Discard full array definition */
                    IDL_GET_LONG_FROM_VECTOR(defn_index,type_vec_ptr);
                    rpc_ss_ndr_marsh_conf_arr(defn_index, param_addr,
                                                                     IDL_msp);
                    if (type_has_pointers)
                    {
                        rpc_ss_ndr_m_dfc_arr_ptees(defn_index, param_addr,
                                         NULL, NULL, IDL_M_CONF_ARRAY, IDL_msp);
                    }
                    break;
                case IDL_DT_OPEN_ARRAY:
                    /* Properties byte */
                    type_has_pointers =
                             IDL_PROP_TEST(*type_vec_ptr, IDL_PROP_HAS_PTRS);
                    type_vec_ptr++;
                    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                                            /* Discard full array definition */
                    IDL_GET_LONG_FROM_VECTOR(defn_index,type_vec_ptr);
                    rpc_ss_ndr_marsh_open_arr(defn_index, param_addr,
                                         (v1_array ? IDL_M_V1_ARRAY : 0)
                                            | (is_string ? IDL_M_IS_STRING : 0)
                                            | IDL_M_IS_PARAM | IDL_M_CONF_ARRAY,
                                         IDL_msp);
                    if (type_has_pointers)
                    {
                        rpc_ss_ndr_m_dvo_arr_ptees(defn_index, param_addr,
                                         NULL, NULL, IDL_M_CONF_ARRAY, IDL_msp);
                    }
                    v1_array = idl_false;
                    is_string = idl_false;
                    break;
                case IDL_DT_ENC_UNION:
                    /* Properties byte */
                    type_has_pointers =
                             IDL_PROP_TEST(*type_vec_ptr, IDL_PROP_HAS_PTRS);
                    type_vec_ptr++;
                    IDL_GET_LONG_FROM_VECTOR(defn_index,type_vec_ptr);
                    rpc_ss_ndr_m_enc_union_or_ptees( param_addr, defn_index,
                                                     idl_false, IDL_msp);
                    if (type_has_pointers)
                    {
                        rpc_ss_ndr_m_enc_union_or_ptees( param_addr, defn_index,
                                                         idl_true, IDL_msp);
                    }
                    break;
                case IDL_DT_N_E_UNION:
                    /* Properties byte */
                    type_has_pointers =
                             IDL_PROP_TEST(*type_vec_ptr, IDL_PROP_HAS_PTRS);
                    type_vec_ptr++;
                    IDL_GET_LONG_FROM_VECTOR(switch_index,type_vec_ptr);
                    IDL_GET_LONG_FROM_VECTOR(defn_index,type_vec_ptr);
                    rpc_ss_ndr_m_n_e_union_or_ptees( param_addr, switch_index,
                                                        defn_index, NULL, NULL,
                                                        idl_false, IDL_msp);
                    if (type_has_pointers)
                    {
                        rpc_ss_ndr_m_n_e_union_or_ptees( param_addr, 
                                                 switch_index, defn_index, NULL,
                                                 NULL, idl_true, IDL_msp);
                    }
                    break;
                case IDL_DT_PASSED_BY_REF:
                    break;
                case IDL_DT_FULL_PTR:
                    type_vec_ptr++;     /* Properties byte */
                    IDL_GET_LONG_FROM_VECTOR(defn_index,type_vec_ptr);
                    node_number = rpc_ss_register_node( IDL_msp->IDL_node_table,
                                          (byte_p_t)*(rpc_void_p_t *)param_addr,
                                             idl_true, &already_marshalled );
                    IDL_MARSH_ULONG( &node_number );
                    if ( (*(rpc_void_p_t *)param_addr != NULL)
                                                     && !already_marshalled )
                    {
                        pointee_defn_ptr = IDL_msp->IDL_type_vec + defn_index;
                        pointee_desc.dimensionality = 0;
                        rpc_ss_pointee_desc_from_data( pointee_defn_ptr,
                                          (byte_p_t)*(rpc_void_p_t *)param_addr,
                                           NULL, NULL, &pointee_desc, IDL_msp );
                        rpc_ss_ndr_marsh_pointee( pointee_defn_ptr,
                                                    *(rpc_void_p_t *)param_addr,
                                                    idl_false, &pointee_desc,
                                                    IDL_msp );
                        rpc_ss_rlse_data_pointee_desc( &pointee_desc, IDL_msp );
                    }
                    break;
                case IDL_DT_UNIQUE_PTR:
                    type_vec_ptr++;     /* Properties byte */
                    IDL_GET_LONG_FROM_VECTOR(defn_index,type_vec_ptr);
                    node_number = (*(rpc_void_p_t *)param_addr != NULL);
                                                          /* 1 if not null */
                    IDL_MARSH_ULONG( &node_number );
                    if (*(rpc_void_p_t *)param_addr != NULL)
                    {
                        pointee_defn_ptr = IDL_msp->IDL_type_vec + defn_index;
                        pointee_desc.dimensionality = 0;
                        rpc_ss_pointee_desc_from_data( pointee_defn_ptr,
                                          (byte_p_t)*(rpc_void_p_t *)param_addr,
                                           NULL, NULL, &pointee_desc, IDL_msp );
                        rpc_ss_ndr_marsh_pointee( pointee_defn_ptr,
                                                    *(rpc_void_p_t *)param_addr,
                                                    idl_false, &pointee_desc,
                                                    IDL_msp );
                        rpc_ss_rlse_data_pointee_desc( &pointee_desc, IDL_msp );
                    }
                    break;
                case IDL_DT_REF_PTR:
                    type_vec_ptr++;     /* Properties byte */
                    IDL_GET_LONG_FROM_VECTOR(defn_index,type_vec_ptr);
                    pointee_defn_ptr = IDL_msp->IDL_type_vec + defn_index;
                    pointee_desc.dimensionality = 0;
                    rpc_ss_pointee_desc_from_data( pointee_defn_ptr,
                                          (byte_p_t)*(rpc_void_p_t *)param_addr,
                                           NULL, NULL, &pointee_desc, IDL_msp );
                    rpc_ss_ndr_marsh_pointee( pointee_defn_ptr,
                                                    *(rpc_void_p_t *)param_addr,
                                                    idl_false, &pointee_desc,
                                                    IDL_msp );
                    rpc_ss_rlse_data_pointee_desc( &pointee_desc, IDL_msp );
                    break;
                case IDL_DT_STRING:
                    /* Varying/open array code will do the right thing */
                    is_string = idl_true;
                    break;
                case IDL_DT_TRANSMIT_AS:
                case IDL_DT_REPRESENT_AS:
                    type_vec_ptr++;     /* Properties byte */
                    IDL_GET_LONG_FROM_VECTOR(defn_index,type_vec_ptr);
                    rpc_ss_ndr_marsh_xmit_as(defn_index, param_addr, IDL_msp);
                    break;
#if 0
               case IDL_DT_INTERFACE:
                    type_vec_ptr++;	/* skip properties */
                    IDL_GET_LONG_FROM_VECTOR(defn_index, type_vec_ptr);
                    rpc_ss_ndr_marsh_interface(defn_index, param_addr, IDL_msp);
                    break;
					 case IDL_DT_DYN_INTERFACE:
                    type_vec_ptr++;     /* Properties byte */
                    IDL_GET_LONG_FROM_VECTOR(rtn_index,type_vec_ptr);
                    IDL_GET_LONG_FROM_VECTOR(iid_index,type_vec_ptr);
                    rpc_ss_ndr_marsh_dyn_interface(rtn_index, param_addr,
								  ((dce_uuid_t*) IDL_param_vector[iid_index]), IDL_msp);
                   break;
#endif

                case IDL_DT_ALLOCATE:
                    /* Do nothing except move to next parameter */
                    if ((*type_vec_ptr == IDL_DT_STRING)
                                          || (*type_vec_ptr == IDL_DT_V1_ARRAY))
                        type_vec_ptr++;
                    type_vec_ptr += 2;     /* Array type and properties byte */
                    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                                                        /* Full array defn */
                    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                                                      /* Flattened array defn */
                    break;
                case IDL_DT_ALLOCATE_REF:
                    /* Do nothing except move to next parameter */
                    rpc_ss_discard_allocate_ref(&type_vec_ptr);
                    break;
                case IDL_DT_PIPE:
                    type_vec_ptr++;     /* Properties byte */
                    IDL_GET_LONG_FROM_VECTOR(defn_index,type_vec_ptr);
                    if (IDL_msp->IDL_side == IDL_client_side_k)
                        rpc_ss_ndr_marsh_pipe(defn_index, param_addr, IDL_msp);
                    break;
                case IDL_DT_IN_CONTEXT:
                case IDL_DT_IN_OUT_CONTEXT:
                case IDL_DT_OUT_CONTEXT:
                    rpc_ss_ndr_marsh_context(type_byte, param_addr, IDL_msp);
                    break;
                case IDL_DT_V1_ARRAY:
                    v1_array = idl_true;
                    break;
                case IDL_DT_V1_STRING:
                    type_vec_ptr += 2;  /* DT_VARYING_ARRAY and properties */
                    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                                                        /* Full array defn */
                    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                                                      /* Flattened array defn */
                    rpc_ss_ndr_marsh_v1_string(param_addr, IDL_M_IS_PARAM,
                                                                     IDL_msp);
                    break;
                case IDL_DT_FREE_REP:
                    IDL_GET_LONG_FROM_VECTOR(routine_index, type_vec_ptr);
                    if (IDL_msp->IDL_side == IDL_server_side_k)
                    {
                        /* Only applies on server side */
                        (*(IDL_msp->IDL_rtn_vec + routine_index))(param_addr);
                    }
                    break;
                case IDL_DT_DELETED_NODES:
                    rpc_ss_ndr_marsh_deletes(IDL_msp);
                    break;
                case IDL_DT_CS_ATTRIBUTE:
                    /* Is followed by the (integer) type of the attribute */
                    rpc_ss_ndr_marsh_scalar(*type_vec_ptr,
                            (rpc_void_p_t)&cs_shadow[param_index-1].IDL_data,
                            IDL_msp);
                    type_vec_ptr++;     /* Attribute type */
                    break;
                case IDL_DT_CS_TYPE:
                    type_vec_ptr++;     /* Properties byte */
                    IDL_GET_LONG_FROM_VECTOR(defn_index,type_vec_ptr);
                    rpc_ss_ndr_marsh_cs_char(param_addr, defn_index, IDL_msp);
                    break;
                case IDL_DT_CS_ARRAY:
                    /* Is followed by an array description */
                    rpc_ss_ndr_marsh_cs_array(param_addr, cs_shadow,
                            param_index-1, idl_false, &type_vec_ptr, IDL_msp);
                    break;

                case IDL_DT_CS_SHADOW:
                    IDL_GET_LONG_FROM_VECTOR(shadow_length, type_vec_ptr);
                    rpc_ss_ndr_m_param_cs_shadow(
                                type_vec_ptr, param_index, shadow_length,
                                &cs_shadow, IDL_msp);
                    break;
                case IDL_DT_CS_RLSE_SHADOW:
                    rpc_ss_ndr_m_rlse_cs_shadow(cs_shadow, IDL_parameter_count,
                                                                     IDL_msp);
                    break;
                case IDL_DT_RANGE:
                    IDL_GET_RANGE_FROM_VECTOR(range_bounds, type_vec_ptr);
                    break;
                case IDL_DT_EOL:
                    break;
                default:
#ifdef DEBUG_INTERP
                    printf("rpc_ss_ndr_marsh_interp:unrecognized type %d\n",
                        type_byte);
                    exit(0);
#endif
                    DCETHREAD_RAISE(rpc_x_coding_error);
            }
        } while (type_byte != IDL_DT_EOL);
    }

    /* For pickling, round the buffer out to a multiple of 8 bytes */
    if (IDL_msp->IDL_pickling_handle != NULL)
    {
        IDL_MARSH_ALIGN_MP(IDL_msp, 8);
    }

    /* If there is a current buffer, attach it to the iovector */
    if (IDL_msp->IDL_buff_addr != NULL)
    {
        rpc_ss_attach_buff_to_iovec( IDL_msp );
    }
    /* Make the iovector ready for the rpc_call_transceive call */
    IDL_msp->IDL_iovec.num_elt = IDL_msp->IDL_elts_in_use;
    /* And nothing else needs to be cleaned up */
    IDL_msp->IDL_elts_in_use = 0;
}
