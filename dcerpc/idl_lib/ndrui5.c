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
**      ndrui5.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      NDR unmarshalling routines for - International characters
**
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


#include <dce/idlddefs.h>
#include <ndrui.h>
#include <lsysdep.h>
#include <ndrmi.h>

/******************************************************************************/
/*                                                                            */
/* Unmarshall the conformance information for a conformant structure when     */
/* the conformant array field has [cs_char] base type                         */
/* Such an array must be 1-dimensional, fixed lower bound and [size_is]       */
/* Get the local size of the array. If necessary, allocate the structure      */
/* Return conversion type and local size                                      */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_u_conf_cs_struct_hdr
(
    idl_byte *struct_defn_ptr,  /* [in] Start of structure definition */
    idl_byte *array_defn_ptr,   /* [in] Points at start of bounds info */
    idl_ulong_int *Z_values,    /* [in] Unmarshalled Z value */
    idl_ulong_int fixed_part_size,  /* [in] size of structure excluding
                                                            conformant array */
    idl_boolean type_has_pointers,  /* [in] */
    idl_ulong_int conf_arr_shadow_index,    /* [in] index in shadow of
                                                            conformant array */
    idl_boolean allocate,           /* [in] TRUE=>structure must be allocated */
    IDL_cs_shadow_elt_t *cs_shadow, 
                           /* [out] convert type and local value of [size_is] */
    rpc_void_p_t *p_param_addr, /* [out] NULL or where to put address of
                                                        allocated structure */
    IDL_msp_t IDL_msp
)
{
    idl_ulong_int sz_index;     /* Index in shadow of [size_is] item */
    idl_ulong_int cs_type_defn_index;
    idl_byte *cs_type_defn_ptr;
    idl_ulong_int routine_index;
    void (**routine_ptr)();
    idl_ulong_int l_storage_len;
    idl_byte *base_type_defn_ptr;       /* Pointer to base type of array */

    /* Skip over lower bound entirely and upper bound kind and type */
    array_defn_ptr += IDL_CONF_BOUND_PAIR_WIDTH/2 + 2;
    IDL_GET_LONG_FROM_VECTOR(sz_index, array_defn_ptr);
    sz_index--;
    /* array_defn_ptr is now pointing to the base type, which has [cs_char] */
    base_type_defn_ptr = array_defn_ptr;
    array_defn_ptr += 2;       /* IDL_DT_CS_TYPE and properties byte */
    IDL_GET_LONG_FROM_VECTOR(cs_type_defn_index, array_defn_ptr);
    cs_type_defn_ptr = IDL_msp->IDL_type_vec + cs_type_defn_index;
    IDL_DISCARD_LONG_FROM_VECTOR(cs_type_defn_ptr); /* Size of local type */
    IDL_GET_LONG_FROM_VECTOR(routine_index, cs_type_defn_ptr);
    routine_ptr = IDL_msp->IDL_rtn_vec + routine_index;

    /* Call ..._local_size */
    (*(routine_ptr + IDL_RTN_LOCAL_SIZE_INDEX))(IDL_msp->IDL_h,
            *(IDL_msp->IDL_cs_tags_p->p_unmar_tag),
            *Z_values,
            &cs_shadow[conf_arr_shadow_index].IDL_convert_type,
            &l_storage_len,
            &(IDL_msp->IDL_status));
    if (IDL_msp->IDL_status != error_status_ok)
        DCETHREAD_RAISE(rpc_x_ss_pipe_comm_error);

    cs_shadow[sz_index].IDL_data.IDL_value = l_storage_len;

    if (allocate)
    {
        rpc_ss_ndr_alloc_storage(fixed_part_size, 1, &l_storage_len,
                                   base_type_defn_ptr, p_param_addr, IDL_msp);
        if (type_has_pointers)
        {
            rpc_ss_init_new_struct_ptrs(IDL_DT_CONF_STRUCT, struct_defn_ptr,
                                        *p_param_addr, &l_storage_len, IDL_msp);
        }
    }
}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall an array of [cs_char]s                                         */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_unmar_cs_array
(
    rpc_void_p_t array_addr,        /* [in] */
    IDL_cs_shadow_elt_t *cs_shadow,  /* [in] ignored for fixed array */
    idl_ulong_int *Z_values,        /* [in] ignored if array not conformant */
    idl_ulong_int array_shadow_index, /* [in] ignored if array not conformant 
                                        Position of array in cs-shadow */
    idl_byte **p_defn_vec_ptr,     /* [in] Points at DT_..._ARRAY
                                      [out] Points after array definition */
    IDL_msp_t IDL_msp
)
{
    idl_byte *defn_vec_ptr;
    idl_byte array_type;
    idl_boolean conformant;     /* TRUE => array is conformant or open */
    idl_ulong_int array_defn_index;
    idl_byte *array_defn_ptr;
    IDL_bound_pair_t *bounds_list;
    IDL_bound_pair_t range_data;
    idl_ulong_int cs_type_defn_index;
    idl_byte *cs_type_defn_ptr;
    idl_ulong_int routine_index;
    idl_ulong_int ln_index;     /* Index in shadow of [length_is] item */
    idl_ulong_int sz_index;     /* Index in shadow of [size_is] item */
    void (**routine_ptr)();
    /* Parameters for ..._net_size */
    idl_ulong_int l_storage_len;
    idl_ulong_int w_storage_len;
    idl_cs_convert_t convert_type;
    /* Parameters for ..._to_netcs */
    idl_ulong_int l_data_len;
    rpc_void_p_t wdata;
    idl_ulong_int w_data_len;

    defn_vec_ptr = *p_defn_vec_ptr;
    array_type = *defn_vec_ptr;
    conformant = ! ((array_type == IDL_DT_FIXED_ARRAY)
                                    || (array_type == IDL_DT_VARYING_ARRAY));
    defn_vec_ptr += 2;      /* Array type and properties byte */
    IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);    /* Full array definition */
    IDL_GET_LONG_FROM_VECTOR(array_defn_index, defn_vec_ptr);
    array_defn_ptr = IDL_msp->IDL_type_vec + array_defn_index;
    array_defn_ptr++;       /* dimensionality */

    if ( ! conformant )
    {
      if (IDL_msp->IDL_type_vec[TVEC_INT_REP_OFFSET] != NDR_LOCAL_INT_REP)
        rpc_ss_fixed_bounds_from_vector(1, array_defn_ptr, &bounds_list,
                                        IDL_msp);
      else
        bounds_list = (IDL_bound_pair_t *)array_defn_ptr;
        array_defn_ptr += IDL_FIXED_BOUND_PAIR_WIDTH;
        w_storage_len = bounds_list[0].upper - bounds_list[0].lower + 1;
        l_storage_len = w_storage_len;
    }
    else    /* Conformant or open */
    {
        /* Skip over lower bound entirely and upper bound kind and type */
        array_defn_ptr += IDL_CONF_BOUND_PAIR_WIDTH/2 + 2;
        IDL_GET_LONG_FROM_VECTOR(sz_index, array_defn_ptr);
        sz_index--;
        l_storage_len = cs_shadow[sz_index].IDL_data.IDL_value;
        w_storage_len = *Z_values;
    }

    /* Data limit information */
    if ((array_type == IDL_DT_VARYING_ARRAY)
        || (array_type == IDL_DT_OPEN_ARRAY))
    {
        idl_ulong_int lower;
        /* Skip over lower data limit entirely and upper data limit kind */
        array_defn_ptr += IDL_DATA_LIMIT_PAIR_WIDTH/2 + 1;
        IDL_GET_LONG_FROM_VECTOR(ln_index, array_defn_ptr);
        ln_index--;
        IDL_UNMAR_ULONG(&lower);     /* A-value */
        range_data.lower = lower;
        /* The B-value is the wire form of [length_is] */
        IDL_UNMAR_ULONG(&w_data_len);
        range_data.upper = range_data.lower + w_data_len;
    }
    else
        w_data_len = w_storage_len;

    /* array_defn_ptr is now pointing to the base type, which has [cs_char] */
    array_defn_ptr += 2;       /* IDL_DT_CS_TYPE and properties byte */
    IDL_GET_LONG_FROM_VECTOR(cs_type_defn_index, array_defn_ptr);
    cs_type_defn_ptr = IDL_msp->IDL_type_vec + cs_type_defn_index;
    IDL_DISCARD_LONG_FROM_VECTOR(cs_type_defn_ptr); /* Size of local type */
    IDL_GET_LONG_FROM_VECTOR(routine_index, cs_type_defn_ptr);
    routine_ptr = IDL_msp->IDL_rtn_vec + routine_index;
    /* cs_type_defn_ptr now pointing at network type definition */

    if ( conformant )
        convert_type = cs_shadow[array_shadow_index].IDL_convert_type;
    else
    {
        /* Call ..._local_size */
        (*(routine_ptr + IDL_RTN_LOCAL_SIZE_INDEX))(IDL_msp->IDL_h,
                *(IDL_msp->IDL_cs_tags_p->p_unmar_tag),
                w_storage_len,
                &convert_type,
                ( ! conformant ) ? NULL : &l_storage_len,
                &IDL_msp->IDL_status);
        if (IDL_msp->IDL_status != error_status_ok)
            DCETHREAD_RAISE(rpc_x_ss_pipe_comm_error);
    }

    if (convert_type == idl_cs_new_buffer_convert)
    {
        wdata = (rpc_void_p_t)rpc_ss_mem_alloc(&IDL_msp->IDL_mem_handle,
                          w_storage_len * rpc_ss_type_size(cs_type_defn_ptr,
                                                                      IDL_msp));
    }
    else
        wdata = array_addr;

    /* Unmarshall the wire form of the data */
    if ((array_type == IDL_DT_FIXED_ARRAY)
        || (array_type == IDL_DT_CONF_ARRAY))
    {
        rpc_ss_ndr_u_fix_or_conf_arr(1, &w_storage_len, cs_type_defn_ptr, 
                                     wdata, 0, IDL_msp);
        w_data_len = w_storage_len;
    }
    else    /* Varying or open */
    {
        rpc_ss_ndr_u_var_or_open_arr(1, &w_storage_len, cs_type_defn_ptr,
                                     wdata, &range_data, 0, IDL_msp);
    }

    if (convert_type != idl_cs_no_convert)
    {
        /* Call ..._from_netcs */
        (*(routine_ptr + IDL_RTN_FROM_NETCS_INDEX))(IDL_msp->IDL_h,
                *(IDL_msp->IDL_cs_tags_p->p_unmar_tag),
                wdata,
                w_data_len,
                l_storage_len,
                array_addr,
                ((array_type == IDL_DT_FIXED_ARRAY)
                    || (array_type == IDL_DT_CONF_ARRAY)) ? NULL : &l_data_len,
                &IDL_msp->IDL_status);
        if (IDL_msp->IDL_status != error_status_ok)
            DCETHREAD_RAISE(rpc_x_ss_codeset_conv_error);
    }
    else
    {
	l_data_len = w_data_len;
    }

    if ((array_type == IDL_DT_VARYING_ARRAY)
        || (array_type == IDL_DT_OPEN_ARRAY))
    {
        cs_shadow[ln_index].IDL_data.IDL_value = l_data_len;
    }

    if (convert_type == idl_cs_new_buffer_convert)
    {
        rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, wdata);
    }

    *p_defn_vec_ptr = defn_vec_ptr;
    if (IDL_msp->IDL_type_vec[TVEC_INT_REP_OFFSET] != NDR_LOCAL_INT_REP)
      if ( ! conformant )
        rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)bounds_list);
}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall a [cs_char] which is not an array element                      */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_unmar_cs_char
(
    rpc_void_p_t data_addr,         /* [in] */
    idl_ulong_int cs_type_defn_index,   /* [in] */
    IDL_msp_t IDL_msp
)
{
    idl_byte *cs_type_defn_ptr;
    idl_ulong_int routine_index;
    void (**routine_ptr)();
    /* Parameters for ..._net_size */
    idl_ulong_int w_storage_len = 1;
    idl_cs_convert_t convert_type;
    /* Parameters for ..._to_netcs */
    rpc_void_p_t wdata;

    cs_type_defn_ptr = IDL_msp->IDL_type_vec + cs_type_defn_index;
    IDL_DISCARD_LONG_FROM_VECTOR(cs_type_defn_ptr); /* Size of local type */
    IDL_GET_LONG_FROM_VECTOR(routine_index, cs_type_defn_ptr);
    routine_ptr = IDL_msp->IDL_rtn_vec + routine_index;

    /* Call ..._local_size */
    (*(routine_ptr + IDL_RTN_LOCAL_SIZE_INDEX))(IDL_msp->IDL_h,
            *(IDL_msp->IDL_cs_tags_p->p_unmar_tag),
            w_storage_len,
            &convert_type,
            NULL,
            &IDL_msp->IDL_status);
    if (IDL_msp->IDL_status != error_status_ok)
        DCETHREAD_RAISE(rpc_x_ss_pipe_comm_error);

    if (convert_type == idl_cs_new_buffer_convert)
    {
        /* Allocate a conversion buffer
                 - cs_type_defn_ptr now points at network type */
        wdata = (idl_void_p_t)rpc_ss_mem_alloc(&IDL_msp->IDL_mem_handle,
                                rpc_ss_type_size(cs_type_defn_ptr, IDL_msp));
    }
    else
        wdata = data_addr;

    rpc_ss_ndr_u_fix_or_conf_arr(1, &w_storage_len, cs_type_defn_ptr,
                                        wdata, 0, IDL_msp);
    if (convert_type != idl_cs_no_convert)
    {
        /* Call ..._from_netcs */
        (*(routine_ptr + IDL_RTN_FROM_NETCS_INDEX))(IDL_msp->IDL_h,
                *(IDL_msp->IDL_cs_tags_p->p_unmar_tag),
                wdata,
                w_storage_len,
                w_storage_len,
                data_addr,
                NULL,
                &IDL_msp->IDL_status);
        if (IDL_msp->IDL_status != error_status_ok)
            DCETHREAD_RAISE(rpc_x_ss_codeset_conv_error);
    }

    if (convert_type == idl_cs_new_buffer_convert)
    {
        /* Release conversion buffer */
        rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)wdata);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Update [length_is] and [size_is] fields associated with arrays of         */
/*  [cs_char] from the cs-shadow for a structure. Then release the cs-shadow  */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_u_struct_cs_shadow
(
    rpc_void_p_t struct_addr,           /* [in] Address of struct */
    idl_byte struct_type ATTRIBUTE_UNUSED,               /* [in] FIXED_STRUCT or CONF_STRUCT */
    idl_ulong_int offset_index,         /* [in] Start of struct's offset vec */
    idl_byte *defn_vec_ptr,             /* [in] Posn following shadow length */
    IDL_cs_shadow_elt_t *cs_shadow,     /* [in] Address of cs-shadow */
    IDL_msp_t IDL_msp
)
{
    idl_ulong_int *offset_vec_ptr;
    idl_byte type_byte;
    idl_ulong_int shadow_index;     /* Index into cs-shadow */

    offset_vec_ptr = IDL_msp->IDL_offset_vec + offset_index + 1;
                                        /* Skip over size at start of offsets */

    shadow_index = 0;
    do {
        type_byte = *defn_vec_ptr;
        defn_vec_ptr++;
        switch(type_byte)
        {
            case IDL_DT_CS_ATTRIBUTE:
                /* The local value of the attribute variable is taken from the
                    shadow */
                rpc_ss_put_typed_integer(
                    cs_shadow[shadow_index].IDL_data.IDL_value,
                    *defn_vec_ptr,
                    (rpc_void_p_t)((idl_byte *)struct_addr + *offset_vec_ptr));
                defn_vec_ptr++;       /* attribute type */
                shadow_index++;
                offset_vec_ptr++;
                break;
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
            case IDL_DT_IGNORE:
            case IDL_DT_V1_ENUM:
            case IDL_DT_ERROR_STATUS:
                offset_vec_ptr++;
                shadow_index++;
                break;
            case IDL_DT_FIXED_ARRAY:
            case IDL_DT_VARYING_ARRAY:
            case IDL_DT_CONF_ARRAY:
            case IDL_DT_OPEN_ARRAY:
                /* Properties byte */
                defn_vec_ptr++;
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                    /* Full array definition */
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                /* Flattened array definition */
                offset_vec_ptr++;
                cs_shadow[shadow_index].IDL_release = idl_false;
                shadow_index++;
                break;
            case IDL_DT_ENC_UNION:
            case IDL_DT_N_E_UNION:
            case IDL_DT_FULL_PTR:
            case IDL_DT_UNIQUE_PTR:
            case IDL_DT_REF_PTR:
            case IDL_DT_TRANSMIT_AS:
            case IDL_DT_REPRESENT_AS:
            case IDL_DT_CS_TYPE:
                /* Properties byte */
                defn_vec_ptr++;
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                offset_vec_ptr++;
                shadow_index++;
                break;
            case IDL_DT_RANGE:
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                break;
            case IDL_DT_STRING:
            case IDL_DT_NDR_ALIGN_2:
            case IDL_DT_NDR_ALIGN_4:
            case IDL_DT_NDR_ALIGN_8:
            case IDL_DT_BEGIN_NESTED_STRUCT:
            case IDL_DT_END_NESTED_STRUCT:
            case IDL_DT_V1_ARRAY:
            case IDL_DT_V1_STRING:
            case IDL_DT_CS_ARRAY:
            case IDL_DT_CS_RLSE_SHADOW:
            case IDL_DT_EOL:
                break;
            default:
#ifdef DEBUG_INTERP
                printf("rpc_ss_ndr_u_struct_cs_shadow:unrecognized type %d\n",
                        type_byte);
                exit(0);
#endif
                DCETHREAD_RAISE(rpc_x_coding_error);
        }
    } while (type_byte != IDL_DT_EOL);

    /* Release the cs-shadow */
    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)cs_shadow);
}

/******************************************************************************/
/*                                                                            */
/*  Update [length_is] and [size_is] fields associated with arrays of         */
/*  [cs_char] from the cs-shadow for a parameter list. Then release the       */
/*  cs-shadow                                                                 */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_u_param_cs_shadow
(
    idl_ulong_int type_index,  /* [in] Index of start of definitions of
                                                             parameter list */
    IDL_cs_shadow_elt_t *cs_shadow,     /* [in] Address of cs-shadow */
    IDL_msp_t IDL_msp
)
{
    idl_byte *type_vec_ptr;
    idl_byte type_byte;
    idl_ulong_int param_index;

    /* Loop over parameters. Exit when DT_RLSE_SHADOW found */
    type_vec_ptr = (IDL_msp->IDL_type_vec) + type_index;
    for ( ; ; )
    {
        IDL_GET_LONG_FROM_VECTOR(param_index,type_vec_ptr);
        do {
            type_byte = *type_vec_ptr;
            type_vec_ptr++;
            switch(type_byte)
            {
                case IDL_DT_CS_ATTRIBUTE:
                    /* The local value of the attribute variable is taken from
                         the shadow */
                    rpc_ss_put_typed_integer(
                        cs_shadow[param_index-1].IDL_data.IDL_value,
                        *type_vec_ptr,
                        IDL_msp->IDL_param_vec[param_index]);
                    type_vec_ptr++;       /* attribute type */
                    break;
                /* For any other parameters we just need to move over them */
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
                case IDL_DT_IGNORE:
                case IDL_DT_V1_ENUM:
                case IDL_DT_ERROR_STATUS:
                case IDL_DT_IN_CONTEXT:
                case IDL_DT_IN_OUT_CONTEXT:
                case IDL_DT_OUT_CONTEXT:
                    break;
                case IDL_DT_FIXED_ARRAY:
                case IDL_DT_VARYING_ARRAY:
                case IDL_DT_CONF_ARRAY:
                case IDL_DT_OPEN_ARRAY:
                    /* Properties byte */
                    type_vec_ptr++;
                    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                                                    /* Full array definition */
                    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                                                /* Flattened array definition */
                    break;
                case IDL_DT_FIXED_STRUCT:
                case IDL_DT_CONF_STRUCT:
                case IDL_DT_V1_CONF_STRUCT:
                case IDL_DT_ENC_UNION:
                case IDL_DT_N_E_UNION:
                case IDL_DT_FULL_PTR:
                case IDL_DT_UNIQUE_PTR:
                case IDL_DT_REF_PTR:
                case IDL_DT_TRANSMIT_AS:
                case IDL_DT_REPRESENT_AS:
                case IDL_DT_PIPE:
                case IDL_DT_CS_TYPE:
                    /* Properties byte */
                    type_vec_ptr++;
                    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                    break;
                case IDL_DT_ALLOCATE_REF:
                    rpc_ss_discard_allocate_ref(&type_vec_ptr);
                    break;
                case IDL_DT_CS_SHADOW:
                case IDL_DT_FREE_REP:
                    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                    break;
                case IDL_DT_PASSED_BY_REF:
                case IDL_DT_STRING:
                case IDL_DT_ALLOCATE:
                case IDL_DT_V1_ARRAY:
                case IDL_DT_V1_STRING:
                case IDL_DT_DELETED_NODES:
                case IDL_DT_CS_ARRAY:
                    break;
                case IDL_DT_CS_RLSE_SHADOW:
                    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                                         (byte_p_t)cs_shadow);
                    return;
                case IDL_DT_RANGE:
                    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                    break;
                case IDL_DT_EOL:
                    break;
                default:
#ifdef DEBUG_INTERP
                    printf("rpc_ss_ndr_u_param_cs_shadow:unrecognized type %d\n",
                                type_byte);
                    exit(0);
#endif
                    DCETHREAD_RAISE(rpc_x_coding_error);
            }
        } while (type_byte != IDL_DT_EOL);
    }
}

/******************************************************************************/
/*                                                                            */
/*  Do length conversion for conformant array parameter                       */
/*  If necessary allocate storage for the parameter                           */
/*                                                                            */
/******************************************************************************/
static void rpc_ss_ndr_conf_cs_array_param
(
    idl_boolean allocate,       /* [in] TRUE => allocate array */
    idl_byte array_type,        /* [in] Conformant or open */
    idl_byte *array_defn_ptr,   /* [in] Points after bounds info */
    idl_ulong_int Z_value,   /* [in] Z-value from wire */
    idl_ulong_int *p_l_storage_len,  /* [out] converted form of Z-value */
    idl_cs_convert_t *p_convert_type,   /* [out] */
    rpc_void_p_t *p_array_addr, /* [out] allocated array */
    IDL_msp_t IDL_msp
)
{
    idl_ulong_int cs_type_defn_index;
    idl_byte *cs_type_defn_ptr;
    idl_ulong_int routine_index;
    void (**routine_ptr)();

    if (array_type == IDL_DT_OPEN_ARRAY)
    {
        /* Ignore the "varying" information */
        array_defn_ptr += IDL_DATA_LIMIT_PAIR_WIDTH;
    }

    /* array_defn_ptr is now pointing to the base type, which has [cs_char] */
    array_defn_ptr += 2;       /* IDL_DT_CS_TYPE and properties byte */
    IDL_GET_LONG_FROM_VECTOR(cs_type_defn_index, array_defn_ptr);
    cs_type_defn_ptr = IDL_msp->IDL_type_vec + cs_type_defn_index;
    IDL_DISCARD_LONG_FROM_VECTOR(cs_type_defn_ptr); /* Size of local type */
    IDL_GET_LONG_FROM_VECTOR(routine_index, cs_type_defn_ptr);
    routine_ptr = IDL_msp->IDL_rtn_vec + routine_index;
    /* cs_type_defn_ptr now pointing at network type definition */

    /* Call ..._local_size */
    (*(routine_ptr + IDL_RTN_LOCAL_SIZE_INDEX))(IDL_msp->IDL_h,
                *(IDL_msp->IDL_cs_tags_p->p_unmar_tag),
                Z_value,
                p_convert_type,
                p_l_storage_len,
                &IDL_msp->IDL_status);
    if (IDL_msp->IDL_status != error_status_ok)
        DCETHREAD_RAISE(rpc_x_ss_pipe_comm_error);

    if (allocate)
    {
        /* Allocate the array */
        rpc_ss_ndr_alloc_storage( 0, 1, p_l_storage_len, cs_type_defn_ptr,
                              p_array_addr, IDL_msp );
    }
}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall a parameter which is an array of [cs_chars]                    */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_u_cs_array_param
(
    idl_byte **p_type_vec_ptr,     /* [in] Points at IDL_DT_..._ARRAY */
                                   /* [out] Points after array indices */
    IDL_cs_shadow_elt_t *param_cs_shadow,   /* [in] cs-shadow for param list */
    idl_ulong_int param_index,     /* [in] Index of parameter in param list */
    IDL_msp_t IDL_msp
)
{
    idl_byte *type_vec_ptr;
    idl_byte array_type;
    idl_byte *array_defn_ptr;
    idl_ulong_int array_defn_index;
    idl_ulong_int sz_index;     /* Index in shadow of [size_is] item */
    idl_ulong_int Z_value;
    idl_ulong_int *Z_values = &Z_value;

    type_vec_ptr = *p_type_vec_ptr;
    array_type = *type_vec_ptr;
    if ((array_type == IDL_DT_CONF_ARRAY)
        || (array_type == IDL_DT_OPEN_ARRAY))
    {
        type_vec_ptr++;       /* Properties byte */
        IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                                            /* Discard full array definition */
        IDL_GET_LONG_FROM_VECTOR(array_defn_index, type_vec_ptr);
        array_defn_ptr = IDL_msp->IDL_type_vec + array_defn_index; 
        array_defn_ptr++;       /* Dimensionality must be 1 */

        /* Skip over lower bound entirely and upper bound kind and type */
        array_defn_ptr += IDL_CONF_BOUND_PAIR_WIDTH/2 + 2;
        IDL_GET_LONG_FROM_VECTOR(sz_index, array_defn_ptr);
        sz_index--;

        rpc_ss_ndr_unmar_Z_values(1, &Z_values, IDL_msp);
        rpc_ss_ndr_conf_cs_array_param(IDL_msp->IDL_side == IDL_server_side_k,
                               array_type, array_defn_ptr, Z_value,
                               &param_cs_shadow[sz_index].IDL_data.IDL_value,
                               &param_cs_shadow[param_index-1].IDL_convert_type,
                               &(IDL_msp->IDL_param_vec[param_index]),
                               IDL_msp);
    }

    rpc_ss_ndr_unmar_cs_array(IDL_msp->IDL_param_vec[param_index], 
                              param_cs_shadow,
                              Z_values, param_index-1, p_type_vec_ptr, IDL_msp);
}

/******************************************************************************/
/*                                                                            */
/*  Allocate an [out]-only conformant/open array of [cs_char]s                */
/*                                                                            */
/******************************************************************************/
void rpc_ss_alloc_out_cs_conf_array
(
    IDL_cs_shadow_elt_t *cs_shadow, /* Converted value of array size written
                                        to appropriate element */
    idl_byte **p_type_vec_ptr,  /* [in] Points at IDL_DT_ALLOCATE
                                   [out] Points after array defn indices */
    /* [out] */ rpc_void_p_t *p_array_addr,
                   /* Where to return address of allocated array */
    IDL_msp_t IDL_msp
)
{
    idl_byte *type_vec_ptr = *p_type_vec_ptr;
    idl_byte array_type;        /* DT_CONF_ARRAY or DT_OPEN_ARRAY */
    idl_ulong_int array_defn_index;
    idl_byte *array_defn_ptr;
    idl_ulong_int Z_value;
    idl_byte sz_type;           /* Data type of [size_is] item */
    idl_ulong_int szp_index;    /* Index in parameter list of [size_is] item */
    idl_cs_convert_t convert_type;
    idl_ulong_int l_storage_len;

    type_vec_ptr++;      /* IDL_DT_ALLOCATE */
    array_type = *type_vec_ptr;
    type_vec_ptr += 2;      /* IDL_DT_..._ARRAY, properties */
    IDL_DISCARD_LONG_FROM_VECTOR(type_vec_ptr);
                                            /* Discard full array definition */
    IDL_GET_LONG_FROM_VECTOR(array_defn_index,type_vec_ptr);
    array_defn_ptr = IDL_msp->IDL_type_vec + array_defn_index;
    array_defn_ptr++; /* Skip dimensionality */

    /* Skip over lower bound entirely and upper bound kind */
    array_defn_ptr += IDL_CONF_BOUND_PAIR_WIDTH/2 + 1;
    sz_type = *array_defn_ptr;
    IDL_GET_LONG_FROM_VECTOR(szp_index, array_defn_ptr);
    /* Get the [size_is] value as it was unmarshalled from the wire */
    Z_value = rpc_ss_get_typed_integer(sz_type,
                                    IDL_msp->IDL_param_vec[szp_index], IDL_msp);

    rpc_ss_ndr_conf_cs_array_param(idl_true, array_type, array_defn_ptr,
                                   Z_value, &l_storage_len, &convert_type,
                                   p_array_addr, IDL_msp);

    /* Overrwrite the [size_is] value with its local value */
    rpc_ss_put_typed_integer(l_storage_len, sz_type,
                                (rpc_void_p_t)&cs_shadow[szp_index-1].IDL_data);

    *p_type_vec_ptr = type_vec_ptr;
}

