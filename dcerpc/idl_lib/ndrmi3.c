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
**      ndrmi3.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      NDR marshalling interpreter routines for - unions, pipes, [transmit_as],
**          context handles, [v1_string]
**
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


#include <dce/idlddefs.h>
#include <ndrmi.h>
#include <lsysdep.h>

/******************************************************************************/
/*                                                                            */
/* Marshall a scalar                                                          */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_scalar
(
    /* [in] */  idl_byte type_byte,
    /* [in] */  rpc_void_p_t param_addr,  /* Address of item to be marshalled */
    IDL_msp_t IDL_msp
)
{
    switch(type_byte)
    {
        case IDL_DT_BOOLEAN:
            rpc_ss_ndr_marsh_check_buffer( 1, IDL_msp );
            rpc_marshall_boolean(IDL_msp->IDL_mp, *(idl_boolean *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_BYTE:
            rpc_ss_ndr_marsh_check_buffer( 1, IDL_msp );
            rpc_marshall_byte(IDL_msp->IDL_mp, *(idl_byte *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_CHAR:
            rpc_ss_ndr_marsh_check_buffer( 1, IDL_msp );
            rpc_marshall_char(IDL_msp->IDL_mp, *(idl_char *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_DOUBLE:
            IDL_MARSH_ALIGN_MP( IDL_msp, 8 );
            rpc_ss_ndr_marsh_check_buffer( 8, IDL_msp );
            rpc_marshall_long_float(IDL_msp->IDL_mp, 
                                    *(idl_long_float *)param_addr);
            IDL_msp->IDL_mp += 8;
            IDL_msp->IDL_left_in_buff -= 8;
            return;
        case IDL_DT_ENUM:
            IDL_MARSH_ALIGN_MP( IDL_msp, 2 );
            rpc_ss_ndr_marsh_check_buffer( 2, IDL_msp );
            rpc_marshall_enum(IDL_msp->IDL_mp, *(int *)param_addr);
            IDL_msp->IDL_mp += 2;
            IDL_msp->IDL_left_in_buff -= 2;
            return;
        case IDL_DT_FLOAT:
            IDL_MARSH_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_marsh_check_buffer( 4, IDL_msp );
            rpc_marshall_short_float(IDL_msp->IDL_mp, 
                                    *(idl_short_float *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        case IDL_DT_SMALL:
            rpc_ss_ndr_marsh_check_buffer( 1, IDL_msp );
            rpc_marshall_small_int(IDL_msp->IDL_mp,
                                     *(idl_small_int *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_SHORT:
            IDL_MARSH_ALIGN_MP( IDL_msp, 2 );
            rpc_ss_ndr_marsh_check_buffer( 2, IDL_msp );
            rpc_marshall_short_int(IDL_msp->IDL_mp,
                                     *(idl_short_int *)param_addr);
            IDL_msp->IDL_mp += 2;
            IDL_msp->IDL_left_in_buff -= 2;
            return;
        case IDL_DT_LONG:
            IDL_MARSH_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_marsh_check_buffer( 4, IDL_msp );
            rpc_marshall_long_int(IDL_msp->IDL_mp, *(idl_long_int *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        case IDL_DT_HYPER:
            IDL_MARSH_ALIGN_MP( IDL_msp, 8 );
            rpc_ss_ndr_marsh_check_buffer( 8, IDL_msp );
            rpc_marshall_hyper_int(IDL_msp->IDL_mp,
                                     *(idl_hyper_int *)param_addr);
            IDL_msp->IDL_mp += 8;
            IDL_msp->IDL_left_in_buff -= 8;
            return;
        case IDL_DT_USMALL:
            rpc_ss_ndr_marsh_check_buffer( 1, IDL_msp );
            rpc_marshall_usmall_int(IDL_msp->IDL_mp,
                                     *(idl_usmall_int *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_USHORT:
            IDL_MARSH_ALIGN_MP( IDL_msp, 2 );
            rpc_ss_ndr_marsh_check_buffer( 2, IDL_msp );
            rpc_marshall_ushort_int(IDL_msp->IDL_mp,
                                     *(idl_ushort_int *)param_addr);
            IDL_msp->IDL_mp += 2;
            IDL_msp->IDL_left_in_buff -= 2;
            return;
        case IDL_DT_ULONG:
            IDL_MARSH_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_marsh_check_buffer( 4, IDL_msp );
            rpc_marshall_ulong_int(IDL_msp->IDL_mp,
                                                 *(idl_ulong_int *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        case IDL_DT_UHYPER:
            IDL_MARSH_ALIGN_MP( IDL_msp, 8 );
            rpc_ss_ndr_marsh_check_buffer( 8, IDL_msp );
            rpc_marshall_uhyper_int(IDL_msp->IDL_mp,
                                     *(idl_uhyper_int *)param_addr);
            IDL_msp->IDL_mp += 8;
            IDL_msp->IDL_left_in_buff -= 8;
            return;
        case IDL_DT_V1_ENUM:
            IDL_MARSH_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_marsh_check_buffer( 4, IDL_msp );
            rpc_marshall_v1_enum(IDL_msp->IDL_mp, *(int *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        case IDL_DT_ERROR_STATUS:
            IDL_MARSH_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_marsh_check_buffer( 4, IDL_msp );
            rpc_marshall_ulong_int(IDL_msp->IDL_mp,
                                             *(idl_ulong_int *)(param_addr));
#ifdef IDL_ENABLE_STATUS_MAPPING
            rpc_ss_map_local_to_dce_status((error_status_t *)(IDL_msp->IDL_mp));
#endif
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        default:
#ifdef DEBUG_INTERP
            printf("rpc_ss_ndr_marsh_scalar: unrecognized type %d\n",
                        type_byte);
            exit(0);
#endif
            DCETHREAD_RAISE(rpc_x_coding_error);
    }
}

/******************************************************************************/
/*                                                                            */
/* Marshall a bounded scalar                                                  */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_bounded_scalar
(
    /* [in] */  IDL_bound_pair_t *range_bounds,
    /* [in] */  idl_byte type_byte,
    /* [in] */  rpc_void_p_t param_addr,  /* Address of item to be marshalled */
    IDL_msp_t IDL_msp
)
{
    switch(type_byte)
    {
        case IDL_DT_BOOLEAN:
            IDL_CHECK_RANGE_BOOLEAN( *range_bounds, param_addr );
            rpc_ss_ndr_marsh_check_buffer( 1, IDL_msp );
            rpc_marshall_boolean(IDL_msp->IDL_mp, *(idl_boolean *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_BYTE:
            IDL_CHECK_RANGE_BYTE( *range_bounds, param_addr );
            rpc_ss_ndr_marsh_check_buffer( 1, IDL_msp );
            rpc_marshall_byte(IDL_msp->IDL_mp, *(idl_byte *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_CHAR:
            IDL_CHECK_RANGE_CHAR( *range_bounds, param_addr );
            rpc_ss_ndr_marsh_check_buffer( 1, IDL_msp );
            rpc_marshall_char(IDL_msp->IDL_mp, *(idl_char *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_DOUBLE:
            IDL_CHECK_RANGE_DOUBLE( *range_bounds, param_addr );
            IDL_MARSH_ALIGN_MP( IDL_msp, 8 );
            rpc_ss_ndr_marsh_check_buffer( 8, IDL_msp );
            rpc_marshall_long_float(IDL_msp->IDL_mp, 
                                    *(idl_long_float *)param_addr);
            IDL_msp->IDL_mp += 8;
            IDL_msp->IDL_left_in_buff -= 8;
            return;
        case IDL_DT_ENUM:
            IDL_MARSH_ALIGN_MP( IDL_msp, 2 );
            rpc_ss_ndr_marsh_check_buffer( 2, IDL_msp );
            rpc_marshall_enum(IDL_msp->IDL_mp, *(int *)param_addr);
            IDL_msp->IDL_mp += 2;
            IDL_msp->IDL_left_in_buff -= 2;
            return;
        case IDL_DT_FLOAT:
            IDL_CHECK_RANGE_FLOAT( *range_bounds, param_addr );
            IDL_MARSH_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_marsh_check_buffer( 4, IDL_msp );
            rpc_marshall_short_float(IDL_msp->IDL_mp, 
                                    *(idl_short_float *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        case IDL_DT_SMALL:
            IDL_CHECK_RANGE_SMALL( *range_bounds, param_addr );
            rpc_ss_ndr_marsh_check_buffer( 1, IDL_msp );
            rpc_marshall_small_int(IDL_msp->IDL_mp,
                                     *(idl_small_int *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_SHORT:
            IDL_CHECK_RANGE_SHORT( *range_bounds, param_addr );
            IDL_MARSH_ALIGN_MP( IDL_msp, 2 );
            rpc_ss_ndr_marsh_check_buffer( 2, IDL_msp );
            rpc_marshall_short_int(IDL_msp->IDL_mp,
                                     *(idl_short_int *)param_addr);
            IDL_msp->IDL_mp += 2;
            IDL_msp->IDL_left_in_buff -= 2;
            return;
        case IDL_DT_LONG:
            IDL_CHECK_RANGE_LONG( *range_bounds, param_addr );
            IDL_MARSH_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_marsh_check_buffer( 4, IDL_msp );
            rpc_marshall_long_int(IDL_msp->IDL_mp, *(idl_long_int *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        case IDL_DT_HYPER:
            IDL_MARSH_ALIGN_MP( IDL_msp, 8 );
            rpc_ss_ndr_marsh_check_buffer( 8, IDL_msp );
            rpc_marshall_hyper_int(IDL_msp->IDL_mp,
                                     *(idl_hyper_int *)param_addr);
            IDL_msp->IDL_mp += 8;
            IDL_msp->IDL_left_in_buff -= 8;
            return;
        case IDL_DT_USMALL:
            IDL_CHECK_RANGE_USMALL( *range_bounds, param_addr );
            rpc_ss_ndr_marsh_check_buffer( 1, IDL_msp );
            rpc_marshall_usmall_int(IDL_msp->IDL_mp,
                                     *(idl_usmall_int *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_USHORT:
            IDL_CHECK_RANGE_USHORT( *range_bounds, param_addr );
            IDL_MARSH_ALIGN_MP( IDL_msp, 2 );
            rpc_ss_ndr_marsh_check_buffer( 2, IDL_msp );
            rpc_marshall_ushort_int(IDL_msp->IDL_mp,
                                     *(idl_ushort_int *)param_addr);
            IDL_msp->IDL_mp += 2;
            IDL_msp->IDL_left_in_buff -= 2;
            return;
        case IDL_DT_ULONG:
            IDL_CHECK_RANGE_ULONG( *range_bounds, param_addr );
            IDL_MARSH_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_marsh_check_buffer( 4, IDL_msp );
            rpc_marshall_ulong_int(IDL_msp->IDL_mp,
                                                 *(idl_ulong_int *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        case IDL_DT_UHYPER:
            IDL_MARSH_ALIGN_MP( IDL_msp, 8 );
            rpc_ss_ndr_marsh_check_buffer( 8, IDL_msp );
            rpc_marshall_uhyper_int(IDL_msp->IDL_mp,
                                     *(idl_uhyper_int *)param_addr);
            IDL_msp->IDL_mp += 8;
            IDL_msp->IDL_left_in_buff -= 8;
            return;
        case IDL_DT_V1_ENUM:
            IDL_MARSH_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_marsh_check_buffer( 4, IDL_msp );
            rpc_marshall_v1_enum(IDL_msp->IDL_mp, *(int *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        case IDL_DT_ERROR_STATUS:
            IDL_MARSH_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_marsh_check_buffer( 4, IDL_msp );
            rpc_marshall_ulong_int(IDL_msp->IDL_mp,
                                             *(idl_ulong_int *)(param_addr));
#ifdef IDL_ENABLE_STATUS_MAPPING
            rpc_ss_map_local_to_dce_status((error_status_t *)(IDL_msp->IDL_mp));
#endif
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        default:
#ifdef DEBUG_INTERP
            printf("rpc_ss_ndr_marsh_bounded_scalar: unrecognized type %d\n",
                        type_byte);
            exit(0);
#endif
            DCETHREAD_RAISE(rpc_x_coding_error);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Marshall a union body                                                     */
/*                                                                            */
/******************************************************************************/
static void rpc_ss_ndr_marsh_union_body
(
    /* [in] */ idl_byte *defn_vec_ptr,
                     /* On entry GET_LONG will get number of non-default arms */
    /* [in] */ idl_ulong_int switch_value,  /* Value of discriminant */
    /* [in] */ rpc_void_p_t body_addr,    /* Address of the union body */
    IDL_msp_t IDL_msp
)
{
    idl_ulong_int arm_count;    /* Number of non-default arms */
    idl_byte *arm_type_ptr;
    idl_byte type_byte;
    idl_ulong_int defn_index;
    idl_ulong_int node_number;
    long already_marshalled;
    IDL_bound_pair_t range_bounds;

    IDL_GET_LONG_FROM_VECTOR(arm_count, defn_vec_ptr);
    if ( ! rpc_ss_find_union_arm_defn(defn_vec_ptr, arm_count, switch_value,
                                                                &arm_type_ptr,
				                                IDL_msp) )
    {
        /* Switch not matched. Is there a default clause? */
        defn_vec_ptr += arm_count * IDL_UNION_ARM_DESC_WIDTH;
        if (*defn_vec_ptr == IDL_DT_DOES_NOT_EXIST)
            DCETHREAD_RAISE( rpc_x_invalid_tag );
        else
            arm_type_ptr = defn_vec_ptr;
    }

    type_byte = *arm_type_ptr;
    arm_type_ptr++;

    switch(type_byte)
    {
        case IDL_DT_BYTE:
        case IDL_DT_CHAR:
        case IDL_DT_BOOLEAN:
        case IDL_DT_DOUBLE:
        case IDL_DT_ENUM:
        case IDL_DT_FLOAT:
        case IDL_DT_SMALL:
        case IDL_DT_SHORT:
        case IDL_DT_LONG:
        case IDL_DT_HYPER:
        case IDL_DT_USMALL:
        case IDL_DT_USHORT:
        case IDL_DT_ULONG:
        case IDL_DT_UHYPER:
        case IDL_DT_V1_ENUM:
        case IDL_DT_ERROR_STATUS:
            rpc_ss_ndr_marsh_scalar(type_byte, body_addr, IDL_msp);
            break;
        case IDL_DT_FIXED_STRUCT:
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
                                                /* Will skip properties byte */
            rpc_ss_ndr_marsh_struct(type_byte, defn_index, body_addr, IDL_msp);
            break;
        case IDL_DT_FIXED_ARRAY:
            IDL_DISCARD_LONG_FROM_VECTOR(arm_type_ptr);
                                       /* Properties byte and full array defn */
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
            rpc_ss_ndr_marsh_fixed_arr(defn_index, body_addr, 0, IDL_msp);
            break;
        case IDL_DT_ENC_UNION:
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
                                                /* Will skip properties byte */
            rpc_ss_ndr_m_enc_union_or_ptees(body_addr, defn_index, idl_false,
                                                                      IDL_msp);
            break;
        case IDL_DT_FULL_PTR:
            node_number = rpc_ss_register_node( IDL_msp->IDL_node_table,
                                                *(byte_p_t *)body_addr,
                                               ndr_false, &already_marshalled );
            IDL_MARSH_ULONG( &node_number );
            break;
        case IDL_DT_UNIQUE_PTR:
            node_number = ((byte_p_t)*(rpc_void_p_t *)body_addr != NULL);
            IDL_MARSH_ULONG( &node_number );
            break;
        case IDL_DT_STRING:
            IDL_DISCARD_LONG_FROM_VECTOR(arm_type_ptr);
                           /* DT_VARYING, properties byte and full array defn */
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
            rpc_ss_ndr_marsh_varying_arr(defn_index, body_addr, NULL, NULL, 0,
                                         IDL_msp);
            break;
        case IDL_DT_VOID:
            break;
        case IDL_DT_TRANSMIT_AS:
        case IDL_DT_REPRESENT_AS:
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
            rpc_ss_ndr_marsh_xmit_as(defn_index, body_addr, IDL_msp);
            break;
#if 0
        case IDL_DT_INTERFACE:
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
            rpc_ss_ndr_marsh_interface(defn_index, body_addr, IDL_msp);
            break;
#endif

        case IDL_DT_V1_STRING:
            IDL_DISCARD_LONG_FROM_VECTOR(arm_type_ptr);
                           /* DT_VARYING, properties byte and full array defn */
            IDL_DISCARD_LONG_FROM_VECTOR(arm_type_ptr);
                                                      /* Flattened array defn */
            rpc_ss_ndr_marsh_v1_string(body_addr, 0, IDL_msp);
            break;
        case IDL_DT_CS_TYPE:
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
            rpc_ss_ndr_marsh_cs_char(body_addr, defn_index, IDL_msp);
            break;
        case IDL_DT_CS_ARRAY:
            rpc_ss_ndr_m_fixed_cs_array(body_addr, &arm_type_ptr, IDL_msp);
            break;
        case IDL_DT_RANGE:
            IDL_GET_RANGE_FROM_VECTOR(range_bounds, arm_type_ptr);
            rpc_ss_ndr_marsh_bounded_scalar(&range_bounds, *arm_type_ptr,
                                            body_addr, IDL_msp);
            break;
        default:
#ifdef DEBUG_INTERP
            printf("rpc_ss_ndr_marsh_union_body: unrecognized type %d\n",
                        type_byte);
            exit(0);
#endif
            DCETHREAD_RAISE(rpc_x_coding_error);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Marshall the pointees of a union                                          */
/*                                                                            */
/******************************************************************************/
static void rpc_ss_ndr_marsh_union_ptees
(
    /* [in] */ idl_byte *defn_vec_ptr,
                     /* On entry GET_LONG will get number of non-default arms */
    /* [in] */ idl_ulong_int switch_value,  /* Value of discriminant */
    /* [in] */ rpc_void_p_t body_addr,    /* Address of the union body */
    IDL_msp_t IDL_msp
)
{
    idl_ulong_int arm_count;    /* Number of arms */
    idl_byte *arm_type_ptr;
    idl_byte type_byte;
    idl_ulong_int defn_index;
    idl_byte *pointee_defn_ptr;
    IDL_pointee_desc_t pointee_desc;    /* Description of pointee */

    IDL_GET_LONG_FROM_VECTOR(arm_count, defn_vec_ptr);
    if ( ! rpc_ss_find_union_arm_defn(defn_vec_ptr, arm_count, switch_value,
                                                                &arm_type_ptr,
				                                IDL_msp) )
    {
        /* Switch not matched. If there were no default clause, marshalling
            the union body would have raised an exception */
        defn_vec_ptr += arm_count * IDL_UNION_ARM_DESC_WIDTH;
        arm_type_ptr = defn_vec_ptr;
    }

    type_byte = *arm_type_ptr;
    arm_type_ptr++;

    switch(type_byte)
    {
        case IDL_DT_FIXED_STRUCT:
            if ( ! IDL_PROP_TEST(*arm_type_ptr, IDL_PROP_HAS_PTRS) )
                break;
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
                                                /* Will skip properties byte */
            rpc_ss_ndr_m_struct_pointees(type_byte, defn_index, body_addr,
                                                                    IDL_msp);
            break;
        case IDL_DT_FIXED_ARRAY:
            if ( ! IDL_PROP_TEST(*arm_type_ptr, IDL_PROP_HAS_PTRS) )
                break;
            IDL_DISCARD_LONG_FROM_VECTOR(arm_type_ptr);
                                       /* Properties byte and full array defn */
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
            rpc_ss_ndr_m_dfc_arr_ptees(defn_index, body_addr, NULL, NULL, 0,
                                                                     IDL_msp);
            break;
        case IDL_DT_ENC_UNION:
            if ( ! IDL_PROP_TEST(*arm_type_ptr, IDL_PROP_HAS_PTRS) )
                break;
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
                                                /* Will skip properties byte */
            rpc_ss_ndr_m_enc_union_or_ptees(body_addr, defn_index, idl_true,
                                                                      IDL_msp);
            break;
        case IDL_DT_FULL_PTR:
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
                                                 /* Will skip properties byte */
            pointee_defn_ptr = IDL_msp->IDL_type_vec + defn_index;
            if (*(rpc_void_p_t *)body_addr != NULL)
            {
                pointee_desc.dimensionality = 0;
                rpc_ss_pointee_desc_from_data( pointee_defn_ptr,
                                        *(rpc_void_p_t *)body_addr,
                                        NULL, NULL, &pointee_desc, IDL_msp );
                rpc_ss_ndr_marsh_pointee( pointee_defn_ptr,
                                        *(rpc_void_p_t *)body_addr,
                                        idl_true, &pointee_desc, IDL_msp );
                rpc_ss_rlse_data_pointee_desc( &pointee_desc, IDL_msp );
            }
            break;
        case IDL_DT_UNIQUE_PTR:
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
                                                 /* Will skip properties byte */
            pointee_defn_ptr = IDL_msp->IDL_type_vec + defn_index;
            if (*(rpc_void_p_t *)body_addr != NULL)
            {
                pointee_desc.dimensionality = 0;
                rpc_ss_pointee_desc_from_data( pointee_defn_ptr,
                                        *(rpc_void_p_t *)body_addr,
                                        NULL, NULL, &pointee_desc, IDL_msp );
                rpc_ss_ndr_marsh_pointee( pointee_defn_ptr,
                                        *(rpc_void_p_t *)body_addr,
                                        idl_false, &pointee_desc, IDL_msp );
                rpc_ss_rlse_data_pointee_desc( &pointee_desc, IDL_msp );
            }
            break;
        default:
            /* Selected arm did not contain pointers */
            break;
    }

}

/******************************************************************************/
/*                                                                            */
/*  Marshall an encapsulated union or pointees                                */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_m_enc_union_or_ptees
(
    /* [in] */ rpc_void_p_t union_addr,
    /* [in] */ idl_ulong_int defn_index,    /* Index to switch type */
    /* [in] */ idl_boolean pointees,        /* TRUE => marshall pointees */
    IDL_msp_t IDL_msp
)
{
    idl_byte *defn_vec_ptr;
    idl_ulong_int offset_index;
    idl_ulong_int *offset_vec_ptr;
    idl_byte switch_type;       /* Type of discriminant */
    idl_ulong_int switch_value;
    rpc_void_p_t body_addr;    /* Address of the union body */

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index;
    IDL_GET_LONG_FROM_VECTOR(offset_index, defn_vec_ptr);
    switch_type = *defn_vec_ptr;
    defn_vec_ptr++;

    switch_value = (idl_ulong_int)rpc_ss_get_typed_integer(switch_type,
                                                           union_addr, IDL_msp);
    offset_vec_ptr = IDL_msp->IDL_offset_vec + offset_index + 1;
                                            /* + 1 to skip over union size */
    body_addr = (rpc_void_p_t)((idl_byte *)union_addr + *offset_vec_ptr);
    
    if (pointees)
    {
        rpc_ss_ndr_marsh_union_ptees(defn_vec_ptr, switch_value, body_addr,
                                                                      IDL_msp);
    }
    else
    {
        /* Marshall discriminant */
        rpc_ss_ndr_marsh_scalar(switch_type, union_addr, IDL_msp);
        /* Marshall arm */
        rpc_ss_ndr_marsh_union_body(defn_vec_ptr, switch_value, body_addr,
                                                                      IDL_msp);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Marshall a non-encapsulated union or pointees                             */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_m_n_e_union_or_ptees
(
    /* [in] */ rpc_void_p_t union_addr,
    /* [in] */ idl_ulong_int switch_index,
                /* If union is parameter, index in param list of discriminant */
                /* If union is field, index in offset list for discriminant */
    /* [in] */ idl_ulong_int defn_index,    /* Points at dummy offset index
                                                at start of union definition */
    /* [in] */ rpc_void_p_t struct_addr,     /* Address of structure union is
                                        field of. NULL if union is parameter */
    /* [in] */ idl_ulong_int *struct_offset_vec_ptr,
                                                /* NULL if union is parameter */
    /* [in] */ idl_boolean pointees,        /* TRUE => marshall pointees */
    IDL_msp_t IDL_msp
)
{
    idl_byte *defn_vec_ptr;
    idl_byte switch_type;       /* Type of discriminant */
    idl_ulong_int switch_value;
    rpc_void_p_t switch_addr;    /* Address of the discriminant */

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index;
    IDL_DISCARD_LONG_FROM_VECTOR( defn_vec_ptr ); /* Offset vec index */
    switch_type = *defn_vec_ptr;
    defn_vec_ptr++;

    rpc_ss_get_switch_from_data(switch_index, switch_type, struct_addr,
                                 struct_offset_vec_ptr, &switch_value, IDL_msp);
    
    if (pointees)
    {
        rpc_ss_ndr_marsh_union_ptees(defn_vec_ptr, switch_value, union_addr,
                                                                      IDL_msp);
    }
    else
    {
        /* Marshall discriminant */
        if ( struct_addr == NULL )
            switch_addr = IDL_msp->IDL_param_vec[switch_index];
        else
            switch_addr = (rpc_void_p_t)((idl_byte *)struct_addr
                                       + struct_offset_vec_ptr[switch_index]);
        rpc_ss_ndr_marsh_scalar(switch_type, switch_addr, IDL_msp);
        /* Marshall arm */
        rpc_ss_ndr_marsh_union_body(defn_vec_ptr, switch_value, union_addr,
                                                                      IDL_msp);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Marshall a pipe chunk on the callee side                                  */
/*  As written assumes NDR transfer syntax. If we support others, this        */
/*      routine needs to be split into a switch on transfer syntaxes and      */
/*      an NDR specific marshalling routine                                   */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_ee_marsh_pipe_chunk
(
    rpc_ss_pipe_state_t IDL_pipe_state,
    rpc_void_p_t IDL_chunk_array,
    idl_ulong_int IDL_pipe_chunk_size
)
{
    IDL_bound_pair_t chunk_bounds;
    rpc_ss_mts_ee_pipe_state_t *p_pipe_state
                             = (rpc_ss_mts_ee_pipe_state_t *)IDL_pipe_state;

    p_pipe_state->IDL_msp->IDL_marsh_pipe = idl_true;

    if (p_pipe_state->pipe_filled)
    {
        rpc_ss_ndr_clean_up(p_pipe_state->IDL_msp);
        DCETHREAD_RAISE(rpc_x_ss_pipe_closed);
    }
    if (p_pipe_state->pipe_number != -*p_pipe_state->p_current_pipe)
    {
        rpc_ss_ndr_clean_up(p_pipe_state->IDL_msp);
        DCETHREAD_RAISE(rpc_x_ss_pipe_order);
    }

    /* Marshall the chunk */
    rpc_ss_ndr_marsh_scalar( IDL_DT_ULONG, &IDL_pipe_chunk_size,
                             p_pipe_state->IDL_msp );   /* Z-value */
    if (IDL_pipe_chunk_size == 0)
    {
        /* End of pipe */
        p_pipe_state->pipe_filled = idl_true;
        *p_pipe_state->p_current_pipe = p_pipe_state->next_out_pipe;
    }
    else
    {
        /* Marshall chunk data */
        chunk_bounds.lower = 1;
        chunk_bounds.upper = IDL_pipe_chunk_size;
        rpc_ss_ndr_m_fix_or_conf_arr( IDL_chunk_array, 1, &chunk_bounds,
                                      p_pipe_state->IDL_msp->IDL_type_vec 
                                          + p_pipe_state->IDL_base_type_offset,
                                      IDL_M_CONF_ARRAY, p_pipe_state->IDL_msp );
    }
    /* [out] pipes precede other [out]s. IDL_marsh_pipe will be reset when the
        interpreter is called to marshall the other [out]s */
}

/******************************************************************************/
/*                                                                            */
/*  Marshall a pipe on the caller side                                        */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_pipe
(
    /* [in] */ idl_ulong_int defn_index,  /* Points at pipe base type */
    /* [in] */ rpc_void_p_t param_addr,
    IDL_msp_t IDL_msp
)
{
    idl_byte *defn_vec_ptr;
    idl_ulong_int element_size; /* Size of pipe base type */
    rpc_void_p_t chunk_ptr;     /* Pointer to pipe chunk */
    idl_ulong_int buff_size;    /* Size of user buffer in bytes */
    idl_ulong_int chunk_size;   /* Number of elements in chunk */
    IDL_bound_pair_t bounds_list;
    IDL_pipe *p_pipe = (IDL_pipe *)param_addr;

    IDL_msp->IDL_marsh_pipe = idl_true;
    IDL_msp->IDL_restartable = idl_false;

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index;
    element_size = rpc_ss_type_size(defn_vec_ptr, IDL_msp);

    do {
        (*p_pipe->alloc)(p_pipe->state, 
                    (element_size > NIDL_PIPE_BUFF_SIZE/IDL_MIN_PIPE_CHUNK_SIZE)
                                ? IDL_MIN_PIPE_CHUNK_SIZE * element_size
                                : NIDL_PIPE_BUFF_SIZE,
                         &chunk_ptr,
                         &buff_size);
        if (element_size > buff_size)
            DCETHREAD_RAISE(rpc_x_ss_pipe_memory);
        (*p_pipe->pull)(p_pipe->state, chunk_ptr, buff_size/element_size,
                        &chunk_size);
        rpc_ss_ndr_marsh_scalar(IDL_DT_ULONG, &chunk_size, IDL_msp);
        if (chunk_size != 0)
        {
            bounds_list.lower = 1;
            bounds_list.upper = chunk_size;
            rpc_ss_ndr_m_fix_or_conf_arr( chunk_ptr, 1, &bounds_list,
                                      defn_vec_ptr, IDL_M_CONF_ARRAY, IDL_msp );
        }
    } while (chunk_size != 0);

    /* As pipes are the last [in] parameters, no need to reset IDL_marsh_pipe */
}

/******************************************************************************/
/*                                                                            */
/*  Marshall a [transmit_as] type                                             */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_xmit_as
(
    /* [in] */ idl_ulong_int defn_index,
                          /* Points at offset index of presented type size */
    /* [in] */ rpc_void_p_t param_addr,
    IDL_msp_t IDL_msp
)
{
    idl_ulong_int routine_index;    /* Index in routine vector of routine group
                                                            for this type */
    void (**routine_ptr)();         /* Pointer to routine group */
    rpc_void_p_t transmitted_data;
    idl_byte *defn_vec_ptr;
    idl_byte transmitted_type;      /* Type of transmitted data */
    idl_ulong_int xmit_defn_index;  /* Index of definition of constructed
                                                            transmitted type */
    idl_ulong_int array_flags = 0;  /* Becomes non-zero only if transmitted
                                       type is [v1_array] and not [v1_string] */
    IDL_bound_pair_t range_bounds;

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index;
    IDL_DISCARD_LONG_FROM_VECTOR( defn_vec_ptr );  /* Presented type size ptr */

    /* Get a pointer to the [transmit_as] routines for this type */
    IDL_GET_LONG_FROM_VECTOR( routine_index, defn_vec_ptr );
    routine_ptr = IDL_msp->IDL_rtn_vec + routine_index;

    /* Convert presented type to transmitted type */
    (*(routine_ptr+IDL_RTN_TO_XMIT_INDEX))(param_addr, &transmitted_data);
    (IDL_msp->IDL_m_xmit_level)++;

    /* Marshall transmitted type */
    transmitted_type = *defn_vec_ptr;
    if (transmitted_type == IDL_DT_STRING)
    {
        /* Sufficient to know whether string is varying or open array */
        /* Note this is the only case where the [transmit_as] type can
            be a varying or open array */
        defn_vec_ptr++;
        transmitted_type = *defn_vec_ptr;
    }
    else if (transmitted_type == IDL_DT_V1_ARRAY)
    {
        array_flags = IDL_M_V1_ARRAY;
        defn_vec_ptr++;
        transmitted_type = *defn_vec_ptr;
    }
    switch(transmitted_type)
    {
        case IDL_DT_BOOLEAN:
        case IDL_DT_BYTE:
        case IDL_DT_CHAR:
        case IDL_DT_SMALL:
        case IDL_DT_USMALL:
        case IDL_DT_ENUM:
        case IDL_DT_SHORT:
        case IDL_DT_USHORT:
        case IDL_DT_FLOAT:
        case IDL_DT_LONG:
        case IDL_DT_ULONG:
        case IDL_DT_DOUBLE:
        case IDL_DT_HYPER:
        case IDL_DT_UHYPER:
        case IDL_DT_V1_ENUM:
        case IDL_DT_ERROR_STATUS:
            rpc_ss_ndr_marsh_scalar( transmitted_type, transmitted_data,
                                                                     IDL_msp );
            break;
        case IDL_DT_CONF_STRUCT:
        case IDL_DT_V1_CONF_STRUCT:
        case IDL_DT_FIXED_STRUCT:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_GET_LONG_FROM_VECTOR(xmit_defn_index,defn_vec_ptr);
            rpc_ss_ndr_marsh_struct(transmitted_type, xmit_defn_index,
                                    transmitted_data, IDL_msp);
            break;
        case IDL_DT_FIXED_ARRAY:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                             /* Discard full array definition */
            IDL_GET_LONG_FROM_VECTOR(xmit_defn_index,defn_vec_ptr);
            rpc_ss_ndr_marsh_fixed_arr(xmit_defn_index,
                                       transmitted_data, 0, IDL_msp);
            break;
        case IDL_DT_VARYING_ARRAY:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                    /* Full array definition */
            IDL_GET_LONG_FROM_VECTOR(xmit_defn_index, defn_vec_ptr);
            rpc_ss_ndr_marsh_varying_arr(xmit_defn_index, transmitted_data,
                                     NULL, NULL, array_flags, IDL_msp);
            break;
        case IDL_DT_OPEN_ARRAY:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                    /* Full array definition */
            IDL_GET_LONG_FROM_VECTOR(xmit_defn_index, defn_vec_ptr);
            rpc_ss_ndr_marsh_open_arr(xmit_defn_index, transmitted_data,
                                      array_flags, IDL_msp);
            break;
        case IDL_DT_ENC_UNION:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_GET_LONG_FROM_VECTOR(xmit_defn_index, defn_vec_ptr);
            rpc_ss_ndr_m_enc_union_or_ptees(transmitted_data, xmit_defn_index,
                                            idl_false, IDL_msp);
            break;
        case IDL_DT_TRANSMIT_AS:
        case IDL_DT_REPRESENT_AS:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_GET_LONG_FROM_VECTOR(xmit_defn_index, defn_vec_ptr);
            rpc_ss_ndr_marsh_xmit_as(xmit_defn_index, transmitted_data,
                                                                     IDL_msp);
            break;
#if 0
        case IDL_DT_INTERFACE:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_GET_LONG_FROM_VECTOR(xmit_defn_index, defn_vec_ptr);
            rpc_ss_ndr_marsh_interface(xmit_defn_index, transmitted_data, IDL_msp);
            break;
#endif

        case IDL_DT_V1_STRING:
            rpc_ss_ndr_marsh_v1_string(transmitted_data, 0, IDL_msp);
            break;
        case IDL_DT_RANGE:
            IDL_GET_RANGE_FROM_VECTOR( range_bounds, defn_vec_ptr );
            rpc_ss_ndr_marsh_bounded_scalar( &range_bounds, *defn_vec_ptr,
                                             transmitted_data, IDL_msp );
            break;
        default:
#ifdef DEBUG_INTERP
            printf("rpc_ss_ndr_marsh_xmit_as: unrecognized type %d\n",
                        transmitted_type);
            exit(0);
#endif
            DCETHREAD_RAISE(rpc_x_coding_error);

    }

    /* Release storage for transmitted type */
    (IDL_msp->IDL_m_xmit_level)--;
    (*(routine_ptr+IDL_RTN_FREE_XMIT_INDEX))(transmitted_data);

    /* On server side, release targets of any pointers in presented type.
        If this is a recursive call, the "presented type" is freed by
        the "FREE_XMIT" above as the stack unwinds */
    if ((IDL_msp->IDL_side == IDL_server_side_k)
        && (IDL_msp->IDL_m_xmit_level == 0))
    {
        (*(routine_ptr+IDL_RTN_FREE_INST_INDEX))(param_addr);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Marshall a context handle                                                 */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_context
(
    /* [in] */ idl_byte context_type,
                                    /* Need to know directionality of context */
    /* [in] */ rpc_void_p_t param_addr,
    IDL_msp_t IDL_msp
)
{
    ndr_context_handle wire_format;
    ndr_context_handle *p_wire_format;
    int i;

    if (IDL_msp->IDL_side == IDL_client_side_k)
    {
        /* Convert context to wire form */
        rpc_ss_er_ctx_to_wire( *(rpc_ss_context_t *)param_addr, &wire_format,
                               IDL_msp->IDL_h,
                               (context_type == IDL_DT_IN_OUT_CONTEXT),
                               &IDL_msp->IDL_status );
        p_wire_format = &wire_format;
    }
    else
        p_wire_format = &((IDL_ee_context_t *)param_addr)->wire;

    rpc_ss_ndr_marsh_scalar(IDL_DT_ULONG,
                         &p_wire_format->context_handle_attributes, IDL_msp);
    rpc_ss_ndr_marsh_scalar(IDL_DT_ULONG,
                         &p_wire_format->context_handle_uuid.time_low, IDL_msp);
    IDL_MARSH_CUSHORT(&p_wire_format->context_handle_uuid.time_mid);
    IDL_MARSH_CUSHORT(&p_wire_format->context_handle_uuid.time_hi_and_version);
    IDL_MARSH_CUSMALL(&p_wire_format->context_handle_uuid.clock_seq_hi_and_reserved);
    IDL_MARSH_CUSMALL(&p_wire_format->context_handle_uuid.clock_seq_low);
    for (i=0; i<6; i++)
    {
        rpc_ss_ndr_marsh_scalar(IDL_DT_BYTE,
                    &p_wire_format->context_handle_uuid.node[i], IDL_msp);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Marshall a [v1_string]                                                    */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_v1_string
(
    /* [in] */ rpc_void_p_t param_addr,
    /* [in] */ idl_ulong_int flags,
    IDL_msp_t IDL_msp
)
{
    idl_ushort_int actual_count;    /* See NDR spec */
    idl_byte dummy_defn_vec = IDL_DT_CHAR;  /* [v1_string] always of char */
    IDL_bound_pair_t bounds_list;  /* Bounds of array data */

    actual_count = strlen((char *)param_addr);
    IDL_MARSH_CUSHORT(&actual_count);
    bounds_list.lower = 0;
    bounds_list.upper = actual_count;
    rpc_ss_ndr_m_fix_or_conf_arr(param_addr, 1, &bounds_list, &dummy_defn_vec,
                                 flags, IDL_msp);
}

