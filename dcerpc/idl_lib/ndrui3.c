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
**      ndrui3.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      NDR unmarshalling interpreter routines for - unions, pipes,
**           [transmit_as], context handles, [v1_array]s, [v1_string]s
**
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


#include <dce/idlddefs.h>
#include <ndrui.h>
#include <lsysdep.h>

/******************************************************************************/
/*                                                                            */
/* Unmarshall a scalar                                                        */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_unmar_scalar
(
    /* [in] */  idl_byte type_byte,
    /* [in] */  rpc_void_p_t param_addr,  /* Address item is to be marshalled 
                                                into */
    IDL_msp_t IDL_msp
)
{
    switch(type_byte)
    {
        case IDL_DT_BOOLEAN:
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_boolean(IDL_msp->IDL_drep, ndr_g_local_drep,
                                 IDL_msp->IDL_mp, *(idl_boolean *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_BYTE:
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_byte(IDL_msp->IDL_drep, ndr_g_local_drep,
                                 IDL_msp->IDL_mp, *(idl_byte *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_CHAR:
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_char(IDL_msp->IDL_drep, ndr_g_local_drep,
                                 IDL_msp->IDL_mp, *(idl_byte *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_DOUBLE:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 8 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_long_float(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(idl_long_float *)param_addr);
            IDL_msp->IDL_mp += 8;
            IDL_msp->IDL_left_in_buff -= 8;
            return;
        case IDL_DT_ENUM:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 2 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_enum(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(int *)param_addr);
            IDL_msp->IDL_mp += 2;
            IDL_msp->IDL_left_in_buff -= 2;
            return;
        case IDL_DT_FLOAT:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_short_float(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(ndr_short_float *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        case IDL_DT_SMALL:
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_small_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(idl_small_int *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_SHORT:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 2 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_short_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(idl_short_int *)param_addr);
            IDL_msp->IDL_mp += 2;
            IDL_msp->IDL_left_in_buff -= 2;
            return;
        case IDL_DT_LONG:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_long_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                                 IDL_msp->IDL_mp, *(idl_long_int *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        case IDL_DT_HYPER:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 8 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_hyper_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(idl_hyper_int *)param_addr);
            IDL_msp->IDL_mp += 8;
            IDL_msp->IDL_left_in_buff -= 8;
            return;
        case IDL_DT_USMALL:
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_usmall_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(idl_usmall_int *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            return;
        case IDL_DT_USHORT:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 2 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_ushort_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(idl_ushort_int *)param_addr);
            IDL_msp->IDL_mp += 2;
            IDL_msp->IDL_left_in_buff -= 2;
            return;
        case IDL_DT_ULONG:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_ulong_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                                 IDL_msp->IDL_mp, *(idl_ulong_int *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        case IDL_DT_UHYPER:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 8 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_uhyper_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(idl_uhyper_int *)param_addr);
            IDL_msp->IDL_mp += 8;
            IDL_msp->IDL_left_in_buff -= 8;
            return;
        case IDL_DT_V1_ENUM:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_v1_enum(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(int *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        case IDL_DT_ERROR_STATUS:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_ulong_int( IDL_msp->IDL_drep, ndr_g_local_drep,
                        IDL_msp->IDL_mp, *(idl_ulong_int *)(param_addr));
#ifdef IDL_ENABLE_STATUS_MAPPING
            rpc_ss_map_dce_to_local_status((error_status_t *)(param_addr));
#endif
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        default:
#ifdef DEBUG_INTERP
            printf("rpc_ss_ndr_unmar_scalar: unrecognized type %d\n",
                        type_byte);
            exit(0);
#endif
            DCETHREAD_RAISE(rpc_x_coding_error);
    }
}

/******************************************************************************/
/*                                                                            */
/* Unmarshall a bounded scalar                                                */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_unmar_bounded_scalar
(
    /* [in] */  IDL_bound_pair_t *range_bounds,
    /* [in] */  idl_byte type_byte,
    /* [in] */  rpc_void_p_t param_addr,  /* Address item is to be marshalled 
                                                into */
    IDL_msp_t IDL_msp
)
{
    switch(type_byte)
    {
        case IDL_DT_BOOLEAN:
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_boolean(IDL_msp->IDL_drep, ndr_g_local_drep,
                                 IDL_msp->IDL_mp, *(idl_boolean *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            IDL_CHECK_RANGE_BOOLEAN( *range_bounds, param_addr );
            return;
        case IDL_DT_BYTE:
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_byte(IDL_msp->IDL_drep, ndr_g_local_drep,
                                 IDL_msp->IDL_mp, *(idl_byte *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            IDL_CHECK_RANGE_BYTE( *range_bounds, param_addr );
            return;
        case IDL_DT_CHAR:
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_char(IDL_msp->IDL_drep, ndr_g_local_drep,
                                 IDL_msp->IDL_mp, *(idl_byte *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            IDL_CHECK_RANGE_CHAR( *range_bounds, param_addr );
            return;
        case IDL_DT_DOUBLE:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 8 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_long_float(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(idl_long_float *)param_addr);
            IDL_msp->IDL_mp += 8;
            IDL_msp->IDL_left_in_buff -= 8;
            IDL_CHECK_RANGE_DOUBLE( *range_bounds, param_addr );
            return;
        case IDL_DT_ENUM:
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_enum(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(int *)param_addr);
            IDL_msp->IDL_mp += 2;
            IDL_msp->IDL_left_in_buff -= 2;
            IDL_UNMAR_ALIGN_MP( IDL_msp, 2 );
            return;
        case IDL_DT_FLOAT:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_short_float(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(ndr_short_float *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            IDL_CHECK_RANGE_FLOAT( *range_bounds, param_addr );
            return;
        case IDL_DT_SMALL:
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_small_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(idl_small_int *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            IDL_CHECK_RANGE_SMALL( *range_bounds, param_addr );
            return;
        case IDL_DT_SHORT:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 2 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_short_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(idl_short_int *)param_addr);
            IDL_msp->IDL_mp += 2;
            IDL_msp->IDL_left_in_buff -= 2;
            IDL_CHECK_RANGE_SHORT( *range_bounds, param_addr );
            return;
        case IDL_DT_LONG:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_long_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                                 IDL_msp->IDL_mp, *(idl_long_int *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            IDL_CHECK_RANGE_LONG( *range_bounds, param_addr );
            return;
        case IDL_DT_HYPER:
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_hyper_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(idl_hyper_int *)param_addr);
            IDL_msp->IDL_mp += 8;
            IDL_msp->IDL_left_in_buff -= 8;
            IDL_UNMAR_ALIGN_MP( IDL_msp, 8 );
            return;
        case IDL_DT_USMALL:
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_usmall_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(idl_usmall_int *)param_addr);
            IDL_msp->IDL_mp += 1;
            IDL_msp->IDL_left_in_buff -= 1;
            IDL_CHECK_RANGE_USMALL( *range_bounds, param_addr );
            return;
        case IDL_DT_USHORT:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 2 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_ushort_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(idl_ushort_int *)param_addr);
            IDL_msp->IDL_mp += 2;
            IDL_msp->IDL_left_in_buff -= 2;
            IDL_CHECK_RANGE_USHORT( *range_bounds, param_addr );
            return;
        case IDL_DT_ULONG:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_ulong_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                                 IDL_msp->IDL_mp, *(idl_ulong_int *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            IDL_CHECK_RANGE_ULONG( *range_bounds, param_addr );
            return;
        case IDL_DT_UHYPER:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 8 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_uhyper_int(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(idl_uhyper_int *)param_addr);
            IDL_msp->IDL_mp += 8;
            IDL_msp->IDL_left_in_buff -= 8;
            return;
        case IDL_DT_V1_ENUM:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_v1_enum(IDL_msp->IDL_drep, ndr_g_local_drep,
                               IDL_msp->IDL_mp, *(int *)param_addr);
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        case IDL_DT_ERROR_STATUS:
            IDL_UNMAR_ALIGN_MP( IDL_msp, 4 );
            rpc_ss_ndr_unmar_check_buffer( IDL_msp );
            rpc_convert_ulong_int( IDL_msp->IDL_drep, ndr_g_local_drep,
                        IDL_msp->IDL_mp, *(idl_ulong_int *)(param_addr));
#ifdef IDL_ENABLE_STATUS_MAPPING
            rpc_ss_map_dce_to_local_status((error_status_t *)(param_addr));
#endif
            IDL_msp->IDL_mp += 4;
            IDL_msp->IDL_left_in_buff -= 4;
            return;
        default:
#ifdef DEBUG_INTERP
            printf("rpc_ss_ndr_unmar_scalar: unrecognized type %d\n",
                        type_byte);
            exit(0);
#endif
            DCETHREAD_RAISE(rpc_x_coding_error);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall a union body                                                   */
/*                                                                            */
/******************************************************************************/
static void rpc_ss_ndr_unmar_union_body
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
    intptr_t node_number = 0;
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
            rpc_ss_ndr_unmar_scalar(type_byte, body_addr, IDL_msp);
            break;
        case IDL_DT_FIXED_STRUCT:
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
                                                /* Will skip properties byte */
            rpc_ss_ndr_unmar_struct(type_byte, IDL_msp->IDL_type_vec+defn_index,
                                     body_addr, NULL, NULL, IDL_msp);
            break;
        case IDL_DT_FIXED_ARRAY:
            IDL_DISCARD_LONG_FROM_VECTOR(arm_type_ptr);
                                       /* Properties byte and full array defn */
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
            rpc_ss_ndr_unmar_fixed_arr(defn_index, body_addr, 
                                       0, IDL_msp);
            break;
        case IDL_DT_ENC_UNION:
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
                                                /* Will skip properties byte */
            rpc_ss_ndr_u_enc_union_or_ptees(body_addr, defn_index, idl_false,
                                                                      IDL_msp);
            break;
        case IDL_DT_FULL_PTR:
            /* Unmarshall the node number into the space for the pointer */
            rpc_ss_ndr_unmar_scalar(IDL_DT_ULONG, &node_number, IDL_msp);
            *(rpc_void_p_t *)body_addr = (rpc_void_p_t)node_number;
            break;
        case IDL_DT_UNIQUE_PTR:
        {
            idl_ulong_int unique_flag;

            rpc_ss_ndr_unmar_scalar(IDL_DT_ULONG, &unique_flag, IDL_msp);
            if (unique_flag == 0)
                *(rpc_void_p_t *)body_addr = NULL;
            else
                *(rpc_void_p_t *)body_addr = IDL_NEW_NODE;
            break;
        }
        case IDL_DT_STRING:
            IDL_DISCARD_LONG_FROM_VECTOR(arm_type_ptr);
                           /* DT_VARYING, properties byte and full array defn */
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
            rpc_ss_ndr_unmar_varying_arr(IDL_msp->IDL_type_vec + defn_index,
                                         idl_false, body_addr, 
                                         0, IDL_msp);
            break;
        case IDL_DT_VOID:
            break;
        case IDL_DT_TRANSMIT_AS:
        case IDL_DT_REPRESENT_AS:
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
            rpc_ss_ndr_unmar_xmit_as(defn_index, body_addr, NULL, IDL_msp);
            break;
#if 0
        case IDL_DT_INTERFACE:
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
            rpc_ss_ndr_unmar_interface(defn_index, body_addr, NULL, IDL_msp);
            break;
#endif

        case IDL_DT_V1_STRING:
            rpc_ss_ndr_unmar_v1_string(body_addr, 0, IDL_msp);
            break;
        case IDL_DT_CS_ARRAY:
            rpc_ss_ndr_unmar_cs_array(body_addr, NULL, NULL, 0, &arm_type_ptr,
                                                                    IDL_msp);
            break;
        case IDL_DT_CS_TYPE:
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
            rpc_ss_ndr_unmar_cs_char(body_addr, defn_index, IDL_msp);
            break;
        case IDL_DT_RANGE:
            IDL_GET_RANGE_FROM_VECTOR(range_bounds, arm_type_ptr);
            rpc_ss_ndr_unmar_bounded_scalar(&range_bounds, *arm_type_ptr,
                                            body_addr, IDL_msp);
            break;
        default:
#ifdef DEBUG_INTERP
            printf("rpc_ss_ndr_unmar_union_body: unrecognized type %d\n",
                        type_byte);
            exit(0);
#endif
            DCETHREAD_RAISE(rpc_x_coding_error);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall the pointees of a union                                        */
/*                                                                            */
/******************************************************************************/
static void rpc_ss_ndr_unmar_union_ptees
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
    intptr_t node_number = 0;

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
            rpc_ss_ndr_u_struct_pointees(type_byte, defn_index, body_addr, NULL,
                                                                    IDL_msp);
            break;
        case IDL_DT_FIXED_ARRAY:
            if ( ! IDL_PROP_TEST(*arm_type_ptr, IDL_PROP_HAS_PTRS) )
                break;
            IDL_DISCARD_LONG_FROM_VECTOR(arm_type_ptr);
                                       /* Properties byte and full array defn */
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
            rpc_ss_ndr_u_fixed_arr_ptees(defn_index, body_addr, IDL_msp);
            break;
        case IDL_DT_ENC_UNION:
            if ( ! IDL_PROP_TEST(*arm_type_ptr, IDL_PROP_HAS_PTRS) )
                break;
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
                                                /* Will skip properties byte */
            rpc_ss_ndr_u_enc_union_or_ptees(body_addr, defn_index, idl_true,
                                                                      IDL_msp);
            break;
        case IDL_DT_FULL_PTR:
            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
                                                 /* Will skip properties byte */
            node_number = (intptr_t)(*(rpc_void_p_t *)body_addr);
            if (node_number != 0)
            {
                pointee_defn_ptr = IDL_msp->IDL_type_vec + defn_index;
                pointee_desc.dimensionality = 0;
                pointee_desc.struct_addr = NULL;
                pointee_desc.struct_offset_vec_ptr = NULL;
                rpc_ss_ndr_unmar_pointee_desc( type_byte,
                                    pointee_defn_ptr, &pointee_desc,
                                    (rpc_void_p_t *)body_addr, IDL_msp );
                rpc_ss_ndr_unmar_pointee( type_byte,
                                    pointee_defn_ptr, &pointee_desc,
                                    (rpc_void_p_t *)body_addr, IDL_msp );
                rpc_ss_ndr_u_rlse_pointee_desc( &pointee_desc, IDL_msp );
            }
            break;
	case IDL_DT_UNIQUE_PTR:
        {
            intptr_t unique_flag;

            IDL_GET_LONG_FROM_VECTOR(defn_index, arm_type_ptr);
                                                 /* Will skip properties byte */
            unique_flag = (intptr_t)(*(rpc_void_p_t *)body_addr);
            if (unique_flag == 0)
            {
                *(rpc_void_p_t *)body_addr = NULL;
            }
            else
            {
                pointee_defn_ptr = IDL_msp->IDL_type_vec + defn_index;
                pointee_desc.dimensionality = 0;
                pointee_desc.struct_addr = NULL;
                pointee_desc.struct_offset_vec_ptr = NULL;
                rpc_ss_ndr_unmar_pointee_desc( type_byte,
                                    pointee_defn_ptr, &pointee_desc,
                                    (rpc_void_p_t *)body_addr, IDL_msp );
                if ((IDL_msp->IDL_side == IDL_server_side_k)
                   || (*(rpc_void_p_t *)body_addr == NULL))
                {
                    *(rpc_void_p_t *)body_addr = IDL_NEW_NODE;
                }
                rpc_ss_ndr_unmar_pointee( type_byte,
                                    pointee_defn_ptr, &pointee_desc,
                                    (rpc_void_p_t *)body_addr, IDL_msp );
                rpc_ss_ndr_u_rlse_pointee_desc( &pointee_desc, IDL_msp );
            }
            break;
        }
        default:
            /* Selected arm did not contain pointers */
            break;
    }

}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall an encapsulated union or pointees                              */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_u_enc_union_or_ptees
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

    if ( ! pointees )
    {
        /* Unmarshall discriminant */
        rpc_ss_ndr_unmar_scalar(switch_type, union_addr, IDL_msp);
    }
    switch_value = (idl_ulong_int)rpc_ss_get_typed_integer(switch_type,
                                                           union_addr, IDL_msp);
    offset_vec_ptr = IDL_msp->IDL_offset_vec + offset_index + 1;
                                            /* + 1 to skip over union size */
    body_addr = (rpc_void_p_t)((idl_byte *)union_addr + *offset_vec_ptr);
    
    if (pointees)
    {
        rpc_ss_ndr_unmar_union_ptees(defn_vec_ptr, switch_value, body_addr,
                                                                      IDL_msp);
    }
    else
    {
        rpc_ss_ndr_unmar_union_body(defn_vec_ptr, switch_value, body_addr,
                                                                      IDL_msp);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall a non-encapsulated union                                       */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_unmar_n_e_union
(
    /* [in] */ rpc_void_p_t union_addr,
    /* [in] */ idl_ulong_int defn_index,    /* Points at dummy offset index
                                                at start of union definition */
    /* [out] */ idl_ulong_int *p_switch_value,
    IDL_msp_t IDL_msp
)
{
    idl_byte *defn_vec_ptr;
    idl_byte switch_type;       /* Type of discriminant */
    idl_ulong_int switch_work_area;  /* Enough storage to accomodate the
                                biggest discriminant that can be unmarshalled */

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index;
    IDL_DISCARD_LONG_FROM_VECTOR( defn_vec_ptr ); /* Offset vec index */
    switch_type = *defn_vec_ptr;
    defn_vec_ptr++;

    /* Unmarshall discriminant */
    rpc_ss_ndr_unmar_scalar(switch_type, (rpc_void_p_t)&switch_work_area,
                                                                     IDL_msp);
    *p_switch_value = rpc_ss_get_typed_integer(switch_type,
                                               (rpc_void_p_t)&switch_work_area,
                                               IDL_msp);
    /* Unmarshall union */
    rpc_ss_ndr_unmar_union_body(defn_vec_ptr, *p_switch_value, union_addr,
                                                                      IDL_msp);
}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall the pointees of a non-encapsulated union                       */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_u_n_e_union_ptees
(
    /* [in] */ rpc_void_p_t union_addr,
    /* [in] */ idl_ulong_int switch_value,
                               /* Only valid on entry if union is parameter */
    /* [in] */ idl_ulong_int switch_index,
                /* If union is parameter, not used */
                /* If union is field, index in offset list for discriminant */
    /* [in] */ idl_ulong_int defn_index,    /* Points at dummy offset index
                                                at start of union definition */
    /* [in] */ rpc_void_p_t struct_addr,     /* Address of structure union is
                                        field of. NULL if union is parameter */
    /* [in] */ idl_ulong_int *struct_offset_vec_ptr,
                                                /* NULL if union is parameter */
    IDL_msp_t IDL_msp
)
{
    idl_byte *defn_vec_ptr;
    idl_byte switch_type;       /* Type of discriminant */

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index;
    IDL_DISCARD_LONG_FROM_VECTOR( defn_vec_ptr ); /* Offset vec index */
    switch_type = *defn_vec_ptr;
    defn_vec_ptr++;

    if (struct_addr != NULL)
        rpc_ss_get_switch_from_data(switch_index, switch_type, struct_addr,
                                 struct_offset_vec_ptr, &switch_value, IDL_msp);
    
    rpc_ss_ndr_unmar_union_ptees(defn_vec_ptr, switch_value, union_addr,
                                                                      IDL_msp);
}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall a pipe chunk on the callee side                                */
/*  As written assumes NDR transfer syntax. If we support others, this        */
/*      routine needs to be split into a switch on transfer syntaxes and      */
/*      an NDR specific unmarshalling routine                                 */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_ee_unmar_pipe_chunk
(
    rpc_ss_pipe_state_t IDL_pipe_state,
    rpc_void_p_t IDL_chunk_array,
    idl_ulong_int IDL_esize,
    idl_ulong_int *IDL_ecount_p
)
{
    rpc_ss_mts_ee_pipe_state_t *p_pipe_state
                             = (rpc_ss_mts_ee_pipe_state_t *)IDL_pipe_state;

    if (p_pipe_state->pipe_drained)
    {
        rpc_ss_ndr_clean_up(p_pipe_state->IDL_msp);
        DCETHREAD_RAISE(rpc_x_ss_pipe_empty);
    }
    if (p_pipe_state->pipe_number != *p_pipe_state->p_current_pipe)
    {
        rpc_ss_ndr_clean_up(p_pipe_state->IDL_msp);
        DCETHREAD_RAISE(rpc_x_ss_pipe_order);
    }

    if (p_pipe_state->left_in_wire_array == 0)
    {
        /* Finished unmarshalling previous chunk sent by caller */
        rpc_ss_ndr_unmar_scalar(IDL_DT_ULONG, &p_pipe_state->left_in_wire_array,
                                p_pipe_state->IDL_msp);
        if (p_pipe_state->left_in_wire_array == 0)
        {
            /* End of pipe */
            p_pipe_state->pipe_drained = idl_true;
            *p_pipe_state->p_current_pipe = p_pipe_state->next_in_pipe;
            if (p_pipe_state->next_in_pipe < 0)
            {
                /* Last in pipe */
                if (p_pipe_state->IDL_msp->IDL_elt_p->buff_dealloc
                            && p_pipe_state->IDL_msp->IDL_elt_p->data_len != 0)
                    (*(p_pipe_state->IDL_msp->IDL_elt_p->buff_dealloc))
                                  (p_pipe_state->IDL_msp->IDL_elt_p->buff_addr);
                p_pipe_state->IDL_msp->IDL_elt_p = NULL;
            }
            *IDL_ecount_p = 0;
            return;
        }
    }

    if (IDL_esize == 0)
    {
        /* Manager has supplied no memory for unmarshalling */
        rpc_ss_ndr_clean_up(p_pipe_state->IDL_msp);
        DCETHREAD_RAISE(rpc_x_ss_pipe_memory);
    }

    /* Give manager as much data as possible */
    if (IDL_esize > p_pipe_state->left_in_wire_array)
        *IDL_ecount_p = p_pipe_state->left_in_wire_array;
    else
        *IDL_ecount_p = IDL_esize;

    /* Unmarshall requested data */
    rpc_ss_ndr_u_fix_or_conf_arr( 1, IDL_ecount_p,
                           p_pipe_state->IDL_msp->IDL_type_vec
                                         + p_pipe_state->IDL_base_type_offset,
                           IDL_chunk_array, 0, p_pipe_state->IDL_msp );

    /* How much of the chunk supplied by the caller was not passed to the
        manager? */
    p_pipe_state->left_in_wire_array -= *IDL_ecount_p;
}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall a pipe on the caller side                                      */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_unmar_pipe
(
    /* [in] */ idl_ulong_int defn_index,  /* Points at pipe base type */
    /* [in] */ rpc_void_p_t param_addr,
    IDL_msp_t IDL_msp
)
{
    idl_byte *defn_vec_ptr;
    idl_ulong_int left_in_wire_array;
    rpc_void_p_t chunk_ptr;
    idl_ulong_int supplied_size;
    idl_ulong_int element_size; /* Size of pipe base type */
    IDL_pipe *p_pipe = (IDL_pipe *)param_addr;

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index;
    element_size = rpc_ss_type_size(defn_vec_ptr, IDL_msp);

    left_in_wire_array = 0;
    while (idl_true)
    {
        if (left_in_wire_array == 0)
        {
            rpc_ss_ndr_unmar_scalar(IDL_DT_ULONG, &left_in_wire_array, IDL_msp);
            if (left_in_wire_array == 0)
            {
                (*p_pipe->push)(p_pipe->state, NULL, 0);
                break;
            }
        }
        (*p_pipe->alloc)(p_pipe->state, left_in_wire_array * element_size,
                         &chunk_ptr, &supplied_size);
        supplied_size /= element_size;
        if (supplied_size == 0)
            DCETHREAD_RAISE(rpc_x_ss_pipe_memory);
        if (supplied_size > left_in_wire_array)
            supplied_size = left_in_wire_array;
        rpc_ss_ndr_u_fix_or_conf_arr( 1, &supplied_size, defn_vec_ptr,
                                      chunk_ptr, 0, IDL_msp );
        left_in_wire_array -= supplied_size;
        (*p_pipe->push)(p_pipe->state, chunk_ptr, supplied_size);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall a [transmit_as] type                                           */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_unmar_xmit_as
(
    /* [in] */ idl_ulong_int defn_index,
                          /* Points at offset index of presented type size */
    /* [in] */ rpc_void_p_t param_addr,
    /* [in] */ rpc_void_p_t xmit_data_buff,     /* Either NULL or the address
                     of storage the transmitted type can be unmarshalled into */
    IDL_msp_t IDL_msp
)
{
    idl_ulong_int routine_index;    /* Index in routine vector of routine group
                                                            for this type */
    void (**routine_ptr)();         /* Pointer to routine group */
    rpc_void_p_t transmitted_data;
    idl_ulong_int transmitted_data_size; /* Storage size for transmitted data */
    idl_byte *defn_vec_ptr;
    idl_byte transmitted_type;      /* Type of transmitted data */
    idl_ulong_int xmit_defn_index;  /* Index of definition of constructed
                                                            transmitted type */
    idl_ulong_int offset_index;
    idl_byte *struct_defn_ptr;
    idl_ulong_int *struct_offset_vec_ptr; /* Start of offsets for this struct */
    idl_ulong_int array_defn_index;
    idl_byte *array_defn_ptr = NULL;
    idl_ulong_int array_dims = 0;    /* Number of dimensions of array info */
    idl_ulong_int *Z_values = NULL;
    idl_ulong_int normal_Z_values[IDL_NORMAL_DIMS];
    IDL_bound_pair_t *range_list;
    IDL_bound_pair_t normal_range_list[IDL_NORMAL_DIMS];
    idl_ushort_int v1_size;
    idl_ulong_int array_flags = 0;  /* Becomes non-zero only if transmitted
                                       type is [v1_array] and not [v1_string] */
    IDL_bound_pair_t range_bounds;
    IDL_bound_pair_t *correl_bounds_list;
    IDL_bound_pair_t normal_correl_bounds_list[IDL_NORMAL_DIMS];
    IDL_bound_pair_t *correl_range_list;
    IDL_bound_pair_t normal_correl_range_list[IDL_NORMAL_DIMS];

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index;
    IDL_DISCARD_LONG_FROM_VECTOR( defn_vec_ptr );  /* Presented type size ptr */

    /* Get a pointer to the [transmit_as] routines for this type */
    IDL_GET_LONG_FROM_VECTOR( routine_index, defn_vec_ptr );
    routine_ptr = IDL_msp->IDL_rtn_vec + routine_index;

    /* Get space to unmarshall the transmitted type into */
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
    if (xmit_data_buff != NULL)
        transmitted_data = xmit_data_buff;
    else
    {
        if ( (transmitted_type == IDL_DT_CONF_STRUCT)
            || (transmitted_type == IDL_DT_V1_CONF_STRUCT) )
        {
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_GET_LONG_FROM_VECTOR(xmit_defn_index, defn_vec_ptr);
            struct_defn_ptr = IDL_msp->IDL_type_vec + xmit_defn_index;
            IDL_GET_LONG_FROM_VECTOR(offset_index, struct_defn_ptr);
            struct_offset_vec_ptr = IDL_msp->IDL_offset_vec + offset_index;
            IDL_GET_LONG_FROM_VECTOR(array_defn_index,struct_defn_ptr);
            array_defn_ptr = IDL_msp->IDL_type_vec + array_defn_index;
            array_dims = (idl_ulong_int)*array_defn_ptr;
            array_defn_ptr++;
            /* Get Z values for correlation checking */
            if (array_dims > IDL_NORMAL_DIMS)
                Z_values = NULL;
            else
                Z_values = normal_Z_values;
            if (transmitted_type == IDL_DT_V1_CONF_STRUCT)
            {
                IDL_UNMAR_CUSHORT( &v1_size );
                *Z_values = (idl_ulong_int)v1_size;
            }
            else
            {
                rpc_ss_ndr_unmar_Z_values( array_dims, &Z_values, IDL_msp );
            }
            /* Skip over the bounds in the array defn to get to the base type */
            rpc_ss_ndr_unmar_check_bounds_correlation( &array_defn_ptr,
                                     NULL,
                                     param_addr,
                                     struct_offset_vec_ptr,
                                     array_dims,
                                     Z_values,
                                     FALSE,
                                     NULL,
                                     IDL_msp );

            if (transmitted_type == IDL_DT_V1_CONF_STRUCT)
            {
                transmitted_data_size = rpc_ss_ndr_allocation_size(
                                            *struct_offset_vec_ptr, 1,
                                            Z_values, array_defn_ptr, IDL_msp );
            }
            else
            {
                transmitted_data_size = rpc_ss_ndr_allocation_size(
                                            *struct_offset_vec_ptr, array_dims,
                                            Z_values, array_defn_ptr, IDL_msp );
            }
        }
        else if (transmitted_type == IDL_DT_OPEN_ARRAY)
        {
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                  /* Full array definition */
            IDL_GET_LONG_FROM_VECTOR(array_defn_index, defn_vec_ptr);
            array_defn_ptr = IDL_msp->IDL_type_vec + array_defn_index;
            array_dims = (idl_ulong_int)*array_defn_ptr;
            array_defn_ptr++;
            if (array_dims > IDL_NORMAL_DIMS)
                Z_values = NULL;
            else
                Z_values = normal_Z_values;
            rpc_ss_ndr_unmar_Z_values(array_dims, &Z_values, IDL_msp);
            if (array_dims > IDL_NORMAL_DIMS)
            {
                correl_bounds_list = NULL;
                correl_range_list = NULL;
            }
            else
            {
                correl_bounds_list = normal_correl_bounds_list;
                correl_range_list = normal_correl_range_list;
            }
#if 0
            rpc_ss_ndr_unmar_check_bounds_correlation( &array_defn_ptr,
                                     NULL,
                                     param_addr,
                                     struct_offset_vec_ptr,
                                     array_dims,
                                     Z_values,
                                     &correl_bounds_list,
                                     IDL_msp );
            rpc_ss_ndr_unmar_range_list( array_dims,
                                     array_defn_ptr[array_dims * IDL_DATA_LIMIT_PAIR_WIDTH],
                                     &correl_range_list, IDL_msp );
            rpc_ss_ndr_unmar_check_range_correlation( &array_defn_ptr,
                                     NULL,
                                     param_addr,
                                     struct_offset_vec_ptr,
                                     array_dims,
                                     correl_bounds_list,
                                     correl_range_list,
                                     IDL_msp );    
#else
            IDL_ADV_DEFN_PTR_OVER_BOUNDS(array_defn_ptr, array_dims);
            array_defn_ptr += array_dims * IDL_DATA_LIMIT_PAIR_WIDTH;
            rpc_ss_ndr_unmar_range_list( array_dims,
                                     array_defn_ptr[array_dims * IDL_DATA_LIMIT_PAIR_WIDTH],
                                     &correl_range_list, IDL_msp );
#endif
            if (array_dims > IDL_NORMAL_DIMS)
            {
                rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)correl_bounds_list);
                rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)correl_range_list);
            }
            transmitted_data_size = rpc_ss_ndr_allocation_size( 0, array_dims,
                                            Z_values, array_defn_ptr, IDL_msp );
        }
        else
        {
            transmitted_data_size = rpc_ss_type_size(defn_vec_ptr, IDL_msp);
        }
        transmitted_data = (rpc_void_p_t)rpc_ss_mem_alloc(
                               &IDL_msp->IDL_mem_handle, transmitted_data_size);
    }

    /* Unmarshall transmitted type */
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
            rpc_ss_ndr_unmar_scalar( transmitted_type, transmitted_data,
                                                                     IDL_msp );
            break;
        case IDL_DT_FIXED_STRUCT:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_GET_LONG_FROM_VECTOR(xmit_defn_index, defn_vec_ptr);
            rpc_ss_ndr_unmar_struct(transmitted_type,
                                    IDL_msp->IDL_type_vec+xmit_defn_index,
                                    transmitted_data, NULL, NULL, IDL_msp);
            break;
        case IDL_DT_CONF_STRUCT:
        case IDL_DT_V1_CONF_STRUCT:
            rpc_ss_ndr_unmar_struct(transmitted_type,
                                    IDL_msp->IDL_type_vec+xmit_defn_index,
                                    transmitted_data, Z_values, NULL, IDL_msp);
            if (array_dims > IDL_NORMAL_DIMS)
                rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                        (byte_p_t)Z_values);
            break;
        case IDL_DT_FIXED_ARRAY:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                             /* Discard full array definition */
            IDL_GET_LONG_FROM_VECTOR(xmit_defn_index, defn_vec_ptr);
            rpc_ss_ndr_unmar_fixed_arr(xmit_defn_index, transmitted_data,
                                       0, IDL_msp);
            break;
        case IDL_DT_VARYING_ARRAY:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                  /* Full array definition */
            IDL_GET_LONG_FROM_VECTOR(array_defn_index, defn_vec_ptr);
            array_defn_ptr = IDL_msp->IDL_type_vec + array_defn_index;
            array_dims = (idl_ulong_int)*array_defn_ptr;
            array_defn_ptr++;
            array_defn_ptr += array_dims * (IDL_FIXED_BOUND_PAIR_WIDTH 
                                                + IDL_DATA_LIMIT_PAIR_WIDTH);
            /* Now array_defn_ptr points at base type, drop through to
                    open array case */
        case IDL_DT_OPEN_ARRAY:
            if (array_dims > IDL_NORMAL_DIMS)
                range_list = NULL;
            else
                range_list = normal_range_list;
            rpc_ss_ndr_unmar_range_list(array_dims, *array_defn_ptr,
                                        &range_list, IDL_msp);
            rpc_ss_ndr_u_var_or_open_arr(array_dims, Z_values, array_defn_ptr,
                                         transmitted_data, range_list,
                                         array_flags, IDL_msp);
            if (array_dims > IDL_NORMAL_DIMS)
            {
                rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                        (byte_p_t)range_list);
                if (transmitted_type == IDL_DT_OPEN_ARRAY)
                    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                            (byte_p_t)Z_values);
            }
            break;
        case IDL_DT_ENC_UNION:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_GET_LONG_FROM_VECTOR(xmit_defn_index, defn_vec_ptr);
            rpc_ss_ndr_u_enc_union_or_ptees(transmitted_data, xmit_defn_index,
                                            idl_false, IDL_msp);
            break;
        case IDL_DT_TRANSMIT_AS:
        case IDL_DT_REPRESENT_AS:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_GET_LONG_FROM_VECTOR(xmit_defn_index, defn_vec_ptr);
            rpc_ss_ndr_unmar_xmit_as(xmit_defn_index, transmitted_data, NULL,
                                                                     IDL_msp);
            break;
#if 0
        case IDL_DT_INTERFACE:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_GET_LONG_FROM_VECTOR(xmit_defn_index, defn_vec_ptr);
            rpc_ss_ndr_unmar_interface(xmit_defn_index, transmitted_data, NULL, IDL_msp);
            break;
#endif

        case IDL_DT_V1_STRING:
            rpc_ss_ndr_unmar_v1_string(transmitted_data, 0, IDL_msp);
            break;
        case IDL_DT_RANGE:
            IDL_GET_RANGE_FROM_VECTOR( range_bounds, defn_vec_ptr );
            rpc_ss_ndr_unmar_bounded_scalar( &range_bounds, *defn_vec_ptr,
                                             transmitted_data, IDL_msp );
            break;
        default:
#ifdef DEBUG_INTERP
            printf("rpc_ss_ndr_unmar_xmit_as: unrecognized type %d\n",
                        transmitted_type);
            exit(0);
#endif
            DCETHREAD_RAISE(rpc_x_coding_error);

    }

    /* Convert to presented type */
    (*(routine_ptr+IDL_RTN_FROM_XMIT_INDEX))(transmitted_data, param_addr);

    /* Release storage for transmitted type */
    if (xmit_data_buff == NULL)
        rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                (byte_p_t)transmitted_data);
}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall a context handle                                               */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_unmar_context
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
        p_wire_format = &wire_format;
    }
    else
        p_wire_format = &((IDL_ee_context_t *)param_addr)->wire;

    rpc_ss_ndr_unmar_scalar(IDL_DT_ULONG,
                         &p_wire_format->context_handle_attributes, IDL_msp);
    rpc_ss_ndr_unmar_scalar(IDL_DT_ULONG,
                         &p_wire_format->context_handle_uuid.time_low, IDL_msp);
    IDL_UNMAR_CUSHORT(&p_wire_format->context_handle_uuid.time_mid);
    IDL_UNMAR_CUSHORT(&p_wire_format->context_handle_uuid.time_hi_and_version);
    IDL_UNMAR_CUSMALL(&p_wire_format->context_handle_uuid.clock_seq_hi_and_reserved);
    IDL_UNMAR_CUSMALL(&p_wire_format->context_handle_uuid.clock_seq_low);
    for (i=0; i<6; i++)
    {
        rpc_ss_ndr_unmar_scalar(IDL_DT_BYTE,
                    &p_wire_format->context_handle_uuid.node[i], IDL_msp);
    }

    /* Convert context to local form */
    if (IDL_msp->IDL_side == IDL_client_side_k)
    {
        rpc_ss_er_ctx_from_wire( &wire_format,
                                 (rpc_ss_context_t *)param_addr,
                                 IDL_msp->IDL_h,
                                 (context_type == IDL_DT_IN_OUT_CONTEXT),
                                 &IDL_msp->IDL_status );
    }
    else
    {
        rpc_ss_ee_ctx_from_wire( &((IDL_ee_context_t *)param_addr)->wire,
                                 &((IDL_ee_context_t *)param_addr)->local,
                                 &IDL_msp->IDL_status );
    }
}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall a varying [v1_array]                                           */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_u_v1_varying_arr
(
    /* [in] */ rpc_void_p_t array_addr,
    /* [in] */ idl_byte *array_defn_ptr,    /* Points at base type info */
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
)
{
    idl_ushort_int v1_count;
    idl_ulong_int pseudo_Z_value;
    idl_byte base_type;
    idl_boolean unmarshall_by_copying;

    IDL_UNMAR_CUSHORT(&v1_count);
    if (v1_count == 0)
    {
        if ( rpc_ss_bug_1_thru_31(IDL_BUG_1|IDL_BUG_2, IDL_msp) )
        {
            rpc_ss_ndr_arr_align_and_opt( IDL_unmarshalling_k, 1, &base_type,
                             array_defn_ptr, &unmarshall_by_copying, IDL_msp );
            if ( rpc_ss_bug_1_thru_31(IDL_BUG_1, IDL_msp) 
                        && ( (base_type == IDL_DT_FIXED_STRUCT)
                                || (base_type == IDL_DT_ENC_UNION)
                                || (base_type == IDL_DT_TRANSMIT_AS) ) )
            {
                /* -bug 1 and non-scalar base type for [v1_array] */
                idl_ulong_int bug_1_align;
                bug_1_align = rpc_ss_ndr_bug_1_align(array_defn_ptr, IDL_msp);
                IDL_UNMAR_ALIGN_MP( IDL_msp, bug_1_align );
            }
        }
        return;
    }
    pseudo_Z_value = (idl_ulong_int)v1_count;
    /* Now make fixed array unmarshalling do the right thing */
    rpc_ss_ndr_u_fix_or_conf_arr( (*array_defn_ptr == IDL_DT_V1_STRING) ? 2 : 1,
                                    &pseudo_Z_value, array_defn_ptr,
                                    array_addr, flags, IDL_msp );
}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall a [v1_string]                                                  */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_unmar_v1_string
(
    /* [in] */ rpc_void_p_t param_addr,
    idl_ulong_int flags,
    IDL_msp_t IDL_msp
)
{
    idl_ushort_int actual_count;    /* See NDR spec */
    idl_byte dummy_defn_vec = IDL_DT_CHAR;  /* [v1_string] always of char */
    idl_ulong_int pseudo_Z_value;

    IDL_UNMAR_CUSHORT(&actual_count);
    pseudo_Z_value = actual_count + 1;  /* Null terminator */
    rpc_ss_ndr_u_fix_or_conf_arr( 1, &pseudo_Z_value, &dummy_defn_vec,
                                    param_addr, flags, IDL_msp );
}

