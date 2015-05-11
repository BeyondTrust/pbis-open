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
**      ndrmi2.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      NDR marshalling interpreter modules for - pointers
**
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


#include <dce/idlddefs.h>
#include <ndrmi.h>
#include <lsysdep.h>

static void rpc_ss_ndr_m_f_or_c_arr_ptees
(
    rpc_void_p_t array_addr,
    idl_ulong_int dimensionality,
    IDL_bound_pair_t *bounds_list,
    idl_byte *defn_vec_ptr,
    IDL_msp_t IDL_msp
);

static void rpc_ss_ndr_m_v_or_o_arr_ptees
(
    rpc_void_p_t array_addr,
    idl_ulong_int *Z_values,
    idl_ulong_int dimensionality,
    IDL_bound_pair_t *range_list,
    idl_byte *defn_vec_ptr,
    IDL_msp_t IDL_msp
);

/******************************************************************************/
/*                                                                            */
/*  Marshall the target of a pointer                                          */
/*  All params apart from IDL_msp are [in]                                    */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_pointee
(
    idl_byte *defn_vec_ptr,         /* On entry points at type of pointee */
    rpc_void_p_t pointee_addr,      /* address of pointee */
    idl_boolean register_node,      /* TRUE => this is the pointee of a
                                        deferred [ptr] pointer */
    IDL_pointee_desc_t *p_pointee_desc, /* Data describing pointee 
                               - not used unless pointee is non-fixed array
                                                or non-encapsulated union */
    IDL_msp_t IDL_msp
)
{
    long already_marshalled;
    idl_byte pointee_type;
    idl_ulong_int pointee_defn_index;   /* Offset of the pointee type
                                         description in the definition vector */
    idl_boolean type_has_pointers;
    idl_ulong_int node_number;
    idl_byte *array_defn_ptr;
    idl_ulong_int switch_index;
    IDL_pointee_desc_t pointee_desc;    /* Description of pointee */
    IDL_bound_pair_t range_bounds;

    if (register_node)
    {
        rpc_ss_register_node( IDL_msp->IDL_node_table, (byte_p_t)pointee_addr,
                                idl_true, &already_marshalled );
        if (already_marshalled)
            return;
    }

    pointee_type = *defn_vec_ptr;
    pointee_desc.dimensionality = 0;

    if (pointee_type == IDL_DT_STRING)
    {
        /* Varying/open array code will do the right thing */
        defn_vec_ptr++;
        pointee_type = *defn_vec_ptr;
    }

    IDL_INIT_RANGE(range_bounds);
    if (pointee_type == IDL_DT_RANGE)
    {
        IDL_GET_RANGE_FROM_VECTOR(range_bounds, defn_vec_ptr);
        pointee_type = *defn_vec_ptr;
    }

    switch(pointee_type)
    {
        case IDL_DT_BOOLEAN:
            IDL_CHECK_RANGE_BOOLEAN( range_bounds, pointee_addr );
            IDL_MARSH_BOOLEAN( pointee_addr );
            break;
        case IDL_DT_BYTE:
            IDL_CHECK_RANGE_BYTE( range_bounds, pointee_addr );
            IDL_MARSH_BYTE( pointee_addr );
            break;
        case IDL_DT_CHAR:
            IDL_CHECK_RANGE_CHAR( range_bounds, pointee_addr );
            IDL_MARSH_CHAR( pointee_addr );
            break;
        case IDL_DT_SMALL:
            IDL_CHECK_RANGE_SMALL( range_bounds, pointee_addr );
            IDL_MARSH_SMALL( pointee_addr );
            break;
        case IDL_DT_USMALL:
            IDL_CHECK_RANGE_USMALL( range_bounds, pointee_addr );
            IDL_MARSH_USMALL( pointee_addr );
            break;
        case IDL_DT_ENUM:
            IDL_MARSH_ENUM( pointee_addr );
            break;
        case IDL_DT_SHORT:
            IDL_CHECK_RANGE_SHORT( range_bounds, pointee_addr );
            IDL_MARSH_SHORT( pointee_addr );
            break;
        case IDL_DT_USHORT:
            IDL_CHECK_RANGE_USHORT( range_bounds, pointee_addr );
            IDL_MARSH_USHORT( pointee_addr );
            break;
        case IDL_DT_FLOAT:
            IDL_CHECK_RANGE_FLOAT( range_bounds, pointee_addr );
            IDL_MARSH_FLOAT( pointee_addr );
            break;
        case IDL_DT_LONG:
            IDL_CHECK_RANGE_LONG( range_bounds, pointee_addr );
            IDL_MARSH_LONG( pointee_addr );
            break;
        case IDL_DT_ULONG:
            IDL_CHECK_RANGE_ULONG( range_bounds, pointee_addr );
            IDL_MARSH_ULONG( pointee_addr );
            break;
        case IDL_DT_DOUBLE:
            IDL_CHECK_RANGE_DOUBLE( range_bounds, pointee_addr );
            IDL_MARSH_DOUBLE( pointee_addr );
            break;
        case IDL_DT_HYPER:
            IDL_MARSH_HYPER( pointee_addr );
            break;
        case IDL_DT_UHYPER:
            IDL_MARSH_UHYPER( pointee_addr );
            break;
        case IDL_DT_ERROR_STATUS:
            IDL_MARSH_ERROR_STATUS( pointee_addr );
            break;
        case IDL_DT_CONF_STRUCT:
        case IDL_DT_FIXED_STRUCT:
            /* Properties byte */
            defn_vec_ptr++;
            type_has_pointers = IDL_PROP_TEST(*defn_vec_ptr, IDL_PROP_HAS_PTRS);
            defn_vec_ptr++;
            IDL_GET_LONG_FROM_VECTOR(pointee_defn_index,defn_vec_ptr);
            rpc_ss_ndr_marsh_struct(pointee_type, pointee_defn_index,
                                    pointee_addr, IDL_msp);
            if (type_has_pointers)
            {
                rpc_ss_ndr_m_struct_pointees(pointee_type, pointee_defn_index,
                                                pointee_addr, IDL_msp);
            }
            break;
        case IDL_DT_FIXED_ARRAY:
            /* Properties byte */
            defn_vec_ptr++;
            type_has_pointers = IDL_PROP_TEST(*defn_vec_ptr, IDL_PROP_HAS_PTRS);
            defn_vec_ptr++;
            IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                             /* Discard full array definition */
            IDL_GET_LONG_FROM_VECTOR(pointee_defn_index,defn_vec_ptr);
            rpc_ss_ndr_marsh_fixed_arr(pointee_defn_index,
                                       pointee_addr, 0, IDL_msp);
            if (type_has_pointers)
            {
                rpc_ss_ndr_m_dfc_arr_ptees(pointee_defn_index,
                                          pointee_addr, NULL, NULL, 0, IDL_msp);
            }
            break;
        case IDL_DT_VARYING_ARRAY:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                             /* Discard full array definition */
            IDL_GET_LONG_FROM_VECTOR(pointee_defn_index,defn_vec_ptr);
            array_defn_ptr = IDL_msp->IDL_type_vec + pointee_defn_index;
            /* Advance over dimensionality, bounds and data limit info */
            array_defn_ptr += (1 + (p_pointee_desc->dimensionality
                                  * (IDL_FIXED_BOUND_PAIR_WIDTH
                                        + IDL_DATA_LIMIT_PAIR_WIDTH)));
            rpc_ss_ndr_m_var_or_open_arr( pointee_addr,
                                          p_pointee_desc->Z_values,
                                          p_pointee_desc->dimensionality,
                                          p_pointee_desc->range_list,
                                          array_defn_ptr, 0, IDL_msp );
            if (p_pointee_desc->base_type_has_pointers)
            {
               rpc_ss_ndr_m_v_or_o_arr_ptees( pointee_addr,
                                             p_pointee_desc->Z_values,
                                             p_pointee_desc->dimensionality,
                                             p_pointee_desc->range_list,
                                             array_defn_ptr, IDL_msp );
            }
            break;
        case IDL_DT_CONF_ARRAY:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                             /* Discard full array definition */
            IDL_GET_LONG_FROM_VECTOR(pointee_defn_index,defn_vec_ptr);
            array_defn_ptr = IDL_msp->IDL_type_vec + pointee_defn_index;
            array_defn_ptr++;       /* Dimensionality byte */
            IDL_ADV_DEFN_PTR_OVER_BOUNDS(array_defn_ptr,
                                             p_pointee_desc->dimensionality);
            rpc_ss_ndr_marsh_Z_values( p_pointee_desc->dimensionality,
                                       p_pointee_desc->Z_values, IDL_msp );
            rpc_ss_ndr_m_fix_or_conf_arr( pointee_addr,
                                          p_pointee_desc->dimensionality,
                                          p_pointee_desc->bounds_list,
                                          array_defn_ptr, IDL_M_CONF_ARRAY,
                                          IDL_msp );
            if (p_pointee_desc->base_type_has_pointers)
            {
               rpc_ss_ndr_m_f_or_c_arr_ptees( pointee_addr,
                                             p_pointee_desc->dimensionality,
                                             p_pointee_desc->bounds_list,
                                             array_defn_ptr, IDL_msp );
            }
            break;
        case IDL_DT_OPEN_ARRAY:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                             /* Discard full array definition */
            IDL_GET_LONG_FROM_VECTOR(pointee_defn_index,defn_vec_ptr);
            array_defn_ptr = IDL_msp->IDL_type_vec + pointee_defn_index;
            array_defn_ptr++;       /* Dimensionality byte */
            IDL_ADV_DEFN_PTR_OVER_BOUNDS(array_defn_ptr,
                                             p_pointee_desc->dimensionality);
            rpc_ss_ndr_marsh_Z_values( p_pointee_desc->dimensionality,
                                       p_pointee_desc->Z_values, IDL_msp );
            array_defn_ptr += p_pointee_desc->dimensionality
                                                    * IDL_DATA_LIMIT_PAIR_WIDTH;
            rpc_ss_ndr_m_var_or_open_arr( pointee_addr,
                                          p_pointee_desc->Z_values,
                                          p_pointee_desc->dimensionality,
                                          p_pointee_desc->range_list,
                                          array_defn_ptr, 0, IDL_msp );
            if (p_pointee_desc->base_type_has_pointers)
            {
               rpc_ss_ndr_m_v_or_o_arr_ptees( pointee_addr,
                                             p_pointee_desc->Z_values,
                                             p_pointee_desc->dimensionality,
                                             p_pointee_desc->range_list,
                                             array_defn_ptr, IDL_msp );
            }
            break;
        case IDL_DT_ENC_UNION:
            /* Properties byte */
            defn_vec_ptr++;
            type_has_pointers = IDL_PROP_TEST(*defn_vec_ptr, IDL_PROP_HAS_PTRS);
            defn_vec_ptr++;
            IDL_GET_LONG_FROM_VECTOR(pointee_defn_index,defn_vec_ptr);
            rpc_ss_ndr_m_enc_union_or_ptees(pointee_addr, pointee_defn_index,
                                            idl_false, IDL_msp);
            if (type_has_pointers)
            {
                rpc_ss_ndr_m_enc_union_or_ptees(pointee_addr,
                                        pointee_defn_index, idl_true, IDL_msp);
            }
            break;
        case IDL_DT_N_E_UNION:
            /* Properties byte */
            defn_vec_ptr++;
            type_has_pointers = IDL_PROP_TEST(*defn_vec_ptr, IDL_PROP_HAS_PTRS);
            defn_vec_ptr++;
            IDL_GET_LONG_FROM_VECTOR(switch_index,defn_vec_ptr);
            IDL_GET_LONG_FROM_VECTOR(pointee_defn_index,defn_vec_ptr);
            rpc_ss_ndr_m_n_e_union_or_ptees(pointee_addr, switch_index,
                                          pointee_defn_index,
                                          p_pointee_desc->struct_addr,
                                          p_pointee_desc->struct_offset_vec_ptr,
                                          idl_false, IDL_msp);
            if (type_has_pointers)
            {
                rpc_ss_ndr_m_n_e_union_or_ptees(pointee_addr, switch_index,
                                          pointee_defn_index,
                                          p_pointee_desc->struct_addr,
                                          p_pointee_desc->struct_offset_vec_ptr,
                                          idl_true, IDL_msp);
            }
            break;
        case IDL_DT_FULL_PTR:
            node_number = rpc_ss_register_node( IDL_msp->IDL_node_table,
                          (byte_p_t)*(rpc_void_p_t *)pointee_addr, ndr_true,
                          &already_marshalled );
            IDL_MARSH_ULONG( &node_number );
            if ((*(rpc_void_p_t *)pointee_addr != NULL) && !already_marshalled)
            {
                defn_vec_ptr++;
                rpc_ss_pointee_desc_from_data( defn_vec_ptr,
                                          *(rpc_void_p_t *)pointee_addr,
                                          NULL, NULL, &pointee_desc, IDL_msp );
                rpc_ss_ndr_marsh_pointee( defn_vec_ptr,
                                          *(rpc_void_p_t *)pointee_addr,
                                          idl_false, &pointee_desc, IDL_msp );
            }
            break;
        case IDL_DT_UNIQUE_PTR:
            /* Get a value of 0 if pointer is null, 1 otherwise */
            node_number = ((byte_p_t)*(rpc_void_p_t *)pointee_addr != NULL);
            IDL_MARSH_ULONG( &node_number );
            if (*(rpc_void_p_t *)pointee_addr != NULL)
            {
                defn_vec_ptr++;
                rpc_ss_pointee_desc_from_data( defn_vec_ptr,
                                          *(rpc_void_p_t *)pointee_addr,
                                          NULL, NULL, &pointee_desc, IDL_msp );
                rpc_ss_ndr_marsh_pointee( defn_vec_ptr,
                                          *(rpc_void_p_t *)pointee_addr,
                                          idl_false, &pointee_desc, IDL_msp );
            }
            break;
        case IDL_DT_REF_PTR:
            /* For naked ref pointer, just marshall the pointee */
            defn_vec_ptr++;
            rpc_ss_pointee_desc_from_data( defn_vec_ptr,
                                          *(rpc_void_p_t *)pointee_addr,
                                          NULL, NULL, &pointee_desc, IDL_msp );
            rpc_ss_ndr_marsh_pointee( defn_vec_ptr,
                                          *(rpc_void_p_t *)pointee_addr,
                                          idl_false, &pointee_desc, IDL_msp );
            break;
        case IDL_DT_TRANSMIT_AS:
        case IDL_DT_REPRESENT_AS:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_GET_LONG_FROM_VECTOR(pointee_defn_index,defn_vec_ptr);
            rpc_ss_ndr_marsh_xmit_as(pointee_defn_index, pointee_addr, IDL_msp);
            break;
#if 0
        case IDL_DT_INTERFACE:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_GET_LONG_FROM_VECTOR(pointee_defn_index,defn_vec_ptr);
            rpc_ss_ndr_marsh_interface(pointee_defn_index,pointee_addr,IDL_msp);
            break;
#endif
        case IDL_DT_CS_TYPE:
            defn_vec_ptr += 2;      /* Byte after properties byte */
            IDL_GET_LONG_FROM_VECTOR(pointee_defn_index,defn_vec_ptr);
            rpc_ss_ndr_marsh_cs_char(pointee_addr, pointee_defn_index, IDL_msp);
            break;
        case IDL_DT_CS_ARRAY:
            rpc_ss_ndr_m_fixed_cs_array(pointee_addr, &defn_vec_ptr, IDL_msp);
            break;
        default:
#ifdef DEBUG_INTERP
            printf("rpc_ss_ndr_marsh_pointee: unrecognized type %d\n",
                        pointee_type);
            exit(0);
#endif
            DCETHREAD_RAISE(rpc_x_coding_error);

    }
    rpc_ss_rlse_data_pointee_desc( &pointee_desc, IDL_msp );
}

/******************************************************************************/
/*                                                                            */
/*  Marshall the pointees of a structure                                      */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_m_struct_pointees
(
    /* [in] */  idl_byte struct_type,   /* DT_FIXED_STRUCT or DT_CONF_STRUCT */
    /* [in] */  idl_ulong_int defn_index,  /* Index of structure dedecription */
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
    idl_ulong_int conf_dims;    /* Number of dimensions of conformance info */
    IDL_bound_pair_t *conf_bounds;   /* Bounds list from conformance info */
    idl_ulong_int *Z_values;
    IDL_bound_pair_t *range_list;
    idl_boolean type_has_pointers;
    IDL_pointee_desc_t pointee_desc;    /* Description of pointee */
    idl_ulong_int switch_index; /* Index of switch field for non-encapsulated
                                                                        union */
    idl_boolean add_null;       /* Dummy argument in procedure call */

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index;
    IDL_GET_LONG_FROM_VECTOR(offset_index,defn_vec_ptr);
    struct_offset_vec_ptr = IDL_msp->IDL_offset_vec + offset_index;
    offset_vec_ptr = struct_offset_vec_ptr + 1;
                                        /* Skip over size at start of offsets */

    if (struct_type == IDL_DT_CONF_STRUCT)
    {
        /* Next integer in the definition vector is a fully flattened array rep
            for the conformant array field */
        IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
    }

    pointee_desc.dimensionality = 0;
    do {
        type_byte = *defn_vec_ptr;
        defn_vec_ptr++;
        switch(type_byte)
        {
            /* For fields that do not include pointers, simply advance
                the definition and offset pointers in step */
            case IDL_DT_CS_SHADOW:
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
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
                break;
            case IDL_DT_FIXED_ARRAY:
                /* Properties byte */
                type_has_pointers =
                             IDL_PROP_TEST(*defn_vec_ptr, IDL_PROP_HAS_PTRS);
                defn_vec_ptr++;
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                    /* Full array definition */
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                if (type_has_pointers)
                {
                    offset = *offset_vec_ptr;
                    rpc_ss_ndr_m_dfc_arr_ptees(field_defn_index,
                                        (idl_byte *)struct_addr+offset, 
                                        NULL, NULL, 0, IDL_msp);
                }
                offset_vec_ptr++;
                break;
            case IDL_DT_VARYING_ARRAY:
                /* Properties byte */
                type_has_pointers =
                             IDL_PROP_TEST(*defn_vec_ptr, IDL_PROP_HAS_PTRS);
                defn_vec_ptr++;
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                    /* Full array definition */
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                offset = *offset_vec_ptr;
                if (type_has_pointers)
                {
                    rpc_ss_ndr_m_dvo_arr_ptees(field_defn_index,
                            (rpc_void_p_t)((idl_byte *)struct_addr+offset),
                            struct_addr, struct_offset_vec_ptr, 0,
                            IDL_msp);
                }
                offset_vec_ptr++;
                break;
            case IDL_DT_CONF_ARRAY:
                /* Properties byte */
                type_has_pointers =
                             IDL_PROP_TEST(*defn_vec_ptr, IDL_PROP_HAS_PTRS);
                defn_vec_ptr++;
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                    /* Full array definition */
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                /* Note - must be last field of struct */
                if (type_has_pointers)
                {
                    offset = *offset_vec_ptr;
                    field_defn_ptr = IDL_msp->IDL_type_vec + field_defn_index;
                    conf_dims = (idl_ulong_int)*field_defn_ptr;
                    field_defn_ptr++;
                    conf_bounds = NULL;
                    rpc_ss_build_bounds_list( &field_defn_ptr,
                            (rpc_void_p_t)((idl_byte *)struct_addr+offset),
                            struct_addr, struct_offset_vec_ptr, conf_dims,
                                              &conf_bounds, IDL_msp );
                    rpc_ss_ndr_m_f_or_c_arr_ptees(
                        (rpc_void_p_t)((idl_byte *)struct_addr+offset),
                        conf_dims, conf_bounds, field_defn_ptr, IDL_msp );
                    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                            (byte_p_t)conf_bounds);
                }
                break;
            case IDL_DT_OPEN_ARRAY:
                /* Properties byte */
                type_has_pointers =
                             IDL_PROP_TEST(*defn_vec_ptr, IDL_PROP_HAS_PTRS);
                defn_vec_ptr++;
                IDL_DISCARD_LONG_FROM_VECTOR(defn_vec_ptr);
                                                    /* Full array definition */
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                /* Note - must be last field of struct */
                if (type_has_pointers)
                {
                    offset = *offset_vec_ptr;
                    field_defn_ptr = IDL_msp->IDL_type_vec + field_defn_index;
                    conf_dims = (idl_ulong_int)*field_defn_ptr;
                    field_defn_ptr++;
                    conf_bounds = NULL;
                    rpc_ss_build_bounds_list( &field_defn_ptr,
                            (rpc_void_p_t)((idl_byte *)struct_addr+offset),
                            struct_addr, struct_offset_vec_ptr, conf_dims,
                                              &conf_bounds, IDL_msp );
                    Z_values = NULL;
                    rpc_ss_Z_values_from_bounds( conf_bounds, conf_dims,
                                                           &Z_values, IDL_msp );
                    range_list = NULL;
                    rpc_ss_build_range_list( &field_defn_ptr,
                                (rpc_void_p_t)((idl_byte *)struct_addr+offset),
                                 struct_addr, struct_offset_vec_ptr, conf_dims,
                                 conf_bounds, &range_list, &add_null, IDL_msp );
                    rpc_ss_ndr_m_v_or_o_arr_ptees(
                        (rpc_void_p_t)((idl_byte *)struct_addr+offset),
                        Z_values, conf_dims, range_list, field_defn_ptr,
                        IDL_msp );
                    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                            (byte_p_t)range_list);
                    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                            (byte_p_t)Z_values);
                    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                            (byte_p_t)conf_bounds);
                }
                break;
            case IDL_DT_ENC_UNION:
                /* Properties byte */
                type_has_pointers =
                             IDL_PROP_TEST(*defn_vec_ptr, IDL_PROP_HAS_PTRS);
                defn_vec_ptr++;
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                if (type_has_pointers)
                {
                    offset = *offset_vec_ptr;
                    rpc_ss_ndr_m_enc_union_or_ptees(
                                              (idl_byte *)struct_addr+offset,
                                           field_defn_index, idl_true, IDL_msp);
                }
                offset_vec_ptr++;
                break;
            case IDL_DT_N_E_UNION:
                /* Properties byte */
                type_has_pointers =
                             IDL_PROP_TEST(*defn_vec_ptr, IDL_PROP_HAS_PTRS);
                defn_vec_ptr++;
                IDL_GET_LONG_FROM_VECTOR(switch_index, defn_vec_ptr);
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                if (type_has_pointers)
                {
                    offset = *offset_vec_ptr;
                    rpc_ss_ndr_m_n_e_union_or_ptees(
                                   (idl_byte *)struct_addr+offset, switch_index,
                                   field_defn_index, struct_addr,
                                   struct_offset_vec_ptr, idl_true, IDL_msp);
                }
                offset_vec_ptr++;
                break;
            case IDL_DT_FULL_PTR:
                defn_vec_ptr++;     /* Properties byte */
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                field_defn_ptr = IDL_msp->IDL_type_vec + field_defn_index;
                if (*(rpc_void_p_t *)((idl_byte *)struct_addr+offset) != NULL)
                {
                    rpc_ss_pointee_desc_from_data( field_defn_ptr,
                            *(rpc_void_p_t *)((idl_byte *)struct_addr+offset),
                            struct_addr,
                            struct_offset_vec_ptr, &pointee_desc, IDL_msp );
                    rpc_ss_ndr_marsh_pointee( field_defn_ptr,
                            *(rpc_void_p_t *)((idl_byte *)struct_addr+offset),
                            idl_true, &pointee_desc, IDL_msp );
                }
                break;
            case IDL_DT_UNIQUE_PTR:
                defn_vec_ptr++;     /* Properties byte */
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                field_defn_ptr = IDL_msp->IDL_type_vec + field_defn_index;
                if (*(rpc_void_p_t *)((idl_byte *)struct_addr+offset) != NULL)
                {
                    rpc_ss_pointee_desc_from_data( field_defn_ptr, 
                            *(rpc_void_p_t *)((idl_byte *)struct_addr+offset),
                            struct_addr,
                            struct_offset_vec_ptr, &pointee_desc, IDL_msp );
                    rpc_ss_ndr_marsh_pointee( field_defn_ptr,
                            *(rpc_void_p_t *)((idl_byte *)struct_addr+offset),
                            idl_false, &pointee_desc, IDL_msp );
                }
                break;
            case IDL_DT_REF_PTR:
                defn_vec_ptr++;     /* Properties byte */
                IDL_GET_LONG_FROM_VECTOR(field_defn_index, defn_vec_ptr);
                offset = *offset_vec_ptr;
                offset_vec_ptr++;
                field_defn_ptr = IDL_msp->IDL_type_vec + field_defn_index;
                rpc_ss_pointee_desc_from_data( field_defn_ptr, 
                            *(rpc_void_p_t *)((idl_byte *)struct_addr+offset),
                            struct_addr,
                            struct_offset_vec_ptr, &pointee_desc, IDL_msp );
                rpc_ss_ndr_marsh_pointee( field_defn_ptr,
                            *(rpc_void_p_t *)((idl_byte *)struct_addr+offset),
                            idl_false, &pointee_desc, IDL_msp );
                break;
            case IDL_DT_TRANSMIT_AS:
            case IDL_DT_REPRESENT_AS:
            case IDL_DT_CS_TYPE:
                defn_vec_ptr++;     /* Properties byte */
                IDL_DISCARD_LONG_FROM_VECTOR( defn_vec_ptr );
                offset_vec_ptr++;
                break;
            case IDL_DT_RANGE:
                IDL_DISCARD_LONG_FROM_VECTOR( defn_vec_ptr );
                IDL_DISCARD_LONG_FROM_VECTOR( defn_vec_ptr );
                break;
            case IDL_DT_STRING:
            case IDL_DT_NDR_ALIGN_2:
            case IDL_DT_NDR_ALIGN_4:
            case IDL_DT_NDR_ALIGN_8:
            case IDL_DT_BEGIN_NESTED_STRUCT:
            case IDL_DT_END_NESTED_STRUCT:
            case IDL_DT_V1_ARRAY:
            case IDL_DT_V1_STRING:
            case IDL_DT_CS_ATTRIBUTE:
            case IDL_DT_CS_ARRAY:
            case IDL_DT_CS_RLSE_SHADOW:
            case IDL_DT_EOL:
                break;
            default:
#ifdef DEBUG_INTERP
                printf("rpc_ss_ndr_m_struct_pointees:unrecognized type %d\n",
                        type_byte);
                exit(0);
#endif
                DCETHREAD_RAISE(rpc_x_coding_error);
        }
    } while (type_byte != IDL_DT_EOL);
    rpc_ss_rlse_data_pointee_desc( &pointee_desc, IDL_msp );

}

/******************************************************************************/
/*                                                                            */
/* Marshall the pointees of a fixed or conformant array                       */
/*                                                                            */
/******************************************************************************/
static void rpc_ss_ndr_m_f_or_c_arr_ptees
(
    /* [in] */  rpc_void_p_t array_addr,
    /* [in] */  idl_ulong_int dimensionality,
    /* [in] */  IDL_bound_pair_t *bounds_list,
    /* [in] */  idl_byte *defn_vec_ptr, /* On entry points to array base info */
    IDL_msp_t IDL_msp
)
{
    idl_byte base_type;
    idl_ulong_int element_count;
    unsigned32 i;
    idl_ulong_int element_defn_index;
    idl_ulong_int element_offset_index;
    idl_ulong_int element_size;
    idl_byte *element_defn_ptr;
    rpc_void_p_t *array_elt_addr;
    IDL_pointee_desc_t pointee_desc;

    element_count = 1;
    for (i=0; i<dimensionality; i++)
    {
        element_count *= (bounds_list[i].upper - bounds_list[i].lower + 1);
    }

    base_type = *defn_vec_ptr;

    if (base_type == IDL_DT_FIXED_STRUCT)
    {
        defn_vec_ptr += 2;  /* DT_FIXED_STRUCT and properties byte */
        /* Array of structures containing pointers */
        IDL_GET_LONG_FROM_VECTOR(element_defn_index, defn_vec_ptr);
        element_defn_ptr = IDL_msp->IDL_type_vec + element_defn_index;
        IDL_GET_LONG_FROM_VECTOR(element_offset_index, element_defn_ptr);
        element_size = *(IDL_msp->IDL_offset_vec + element_offset_index);
        for (i=0; i<element_count; i++)
        {
            rpc_ss_ndr_m_struct_pointees( base_type, element_defn_index,
                                            array_addr, IDL_msp );
            array_addr = (rpc_void_p_t)((idl_byte *)array_addr + element_size);
        }
    }
    else if (base_type == IDL_DT_ENC_UNION)
    {
        defn_vec_ptr += 2;  /* DT_ENC_UNION and properties byte */
        /* Array of unions containing pointers */
        IDL_GET_LONG_FROM_VECTOR(element_defn_index, defn_vec_ptr);
        element_defn_ptr = IDL_msp->IDL_type_vec + element_defn_index;
        IDL_GET_LONG_FROM_VECTOR(element_offset_index, element_defn_ptr);
        element_size = *(IDL_msp->IDL_offset_vec + element_offset_index);
        for (i=0; i<element_count; i++)
        {
            rpc_ss_ndr_m_enc_union_or_ptees( array_addr, element_defn_index,
                                             idl_true, IDL_msp );
            array_addr = (rpc_void_p_t)((idl_byte *)array_addr + element_size);
        }
    }
    else
    {
        defn_vec_ptr++;     /* Move to pointee type */
        /* Array of pointers */
        array_elt_addr = (rpc_void_p_t *)(array_addr);
        pointee_desc.dimensionality = 0;
        for (i=0; i<element_count; i++)
        {
            if (*(rpc_void_p_t *)array_elt_addr != NULL)
            {
                rpc_ss_pointee_desc_from_data( defn_vec_ptr, 
                                           *(rpc_void_p_t *)array_elt_addr,
                                           NULL, NULL, &pointee_desc, IDL_msp );
                rpc_ss_ndr_marsh_pointee( defn_vec_ptr, *array_elt_addr,
                                        (base_type == IDL_DT_FULL_PTR),
                                        &pointee_desc, IDL_msp );
            }
            array_elt_addr++;
        }
        rpc_ss_rlse_data_pointee_desc( &pointee_desc, IDL_msp );
    }
}

/******************************************************************************/
/*                                                                            */
/*  Starting from the index in the definition vector for a fixed or           */
/*  conformant array, marshall the array's pointees                           */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_m_dfc_arr_ptees
(
    /* [in] */  idl_ulong_int defn_index,   /* Index of array description */
    /* [in] */  rpc_void_p_t array_addr,
    /* [in] */  rpc_void_p_t struct_addr,   /* Address of structure array is
                      part of. NULL if array is a parameter or not conformant */
    /* [in] */  idl_ulong_int *struct_offset_vec_ptr,
                            /*  NULL if array is a parameter or not conformant*/
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
    /* By design defn_vec_ptr is longword aligned */
    if (IDL_M_FLAGS_TEST(flags,IDL_M_CONF_ARRAY))
    {
        bounds_list = NULL;
        rpc_ss_build_bounds_list( &defn_vec_ptr, array_addr, struct_addr,
                                    struct_offset_vec_ptr, dimensionality,
                                    &bounds_list, IDL_msp );
    }
    else
    {
	if (IDL_msp->IDL_type_vec[TVEC_INT_REP_OFFSET] != NDR_LOCAL_INT_REP)
	    rpc_ss_fixed_bounds_from_vector(dimensionality, defn_vec_ptr,
					    &bounds_list, IDL_msp);
	else
	    bounds_list = (IDL_bound_pair_t *)defn_vec_ptr;
        defn_vec_ptr += dimensionality * sizeof(IDL_bound_pair_t);
    }
    rpc_ss_ndr_m_f_or_c_arr_ptees( array_addr, dimensionality, bounds_list,
                                  defn_vec_ptr, IDL_msp );
    if (IDL_msp->IDL_type_vec[TVEC_INT_REP_OFFSET] == NDR_LOCAL_INT_REP)
    {
	if (IDL_M_FLAGS_TEST(flags,IDL_M_CONF_ARRAY))
	  rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)bounds_list);
    }
    else 
	rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)bounds_list);
}

/******************************************************************************/
/*                                                                            */
/* Marshall the pointees of a varying or open array                           */
/*                                                                            */
/******************************************************************************/
static void rpc_ss_ndr_m_v_or_o_arr_ptees
(
    /* [in] */  rpc_void_p_t array_addr,
    /* [in] */  idl_ulong_int *Z_values,
    /* [in] */  idl_ulong_int dimensionality,
    /* [in] */  IDL_bound_pair_t *range_list,
    /* [in] */  idl_byte *defn_vec_ptr, /* On entry points at base type */
    IDL_msp_t IDL_msp
)
{
    idl_ulong_int element_defn_index;
    idl_ulong_int element_size;
    idl_byte base_type;
    idl_byte *element_defn_ptr;
    idl_ulong_int element_offset_index;
    IDL_varying_control_t *control_data;    /* List of data used to control
                                                marshalling loops */
    int i;
    idl_byte *inner_slice_address;  /* Address of 1-dim subset of array */
    int dim;    /* Index through array dimensions */
    IDL_pointee_desc_t pointee_desc;

    base_type = *defn_vec_ptr;

    element_size = rpc_ss_type_size(defn_vec_ptr, IDL_msp); 
    defn_vec_ptr++;

    if ( (base_type == IDL_DT_FIXED_STRUCT) || (base_type == IDL_DT_ENC_UNION) )
    {
        IDL_GET_LONG_FROM_VECTOR( element_defn_index, defn_vec_ptr );
        element_defn_ptr = IDL_msp->IDL_type_vec + element_defn_index;
        IDL_GET_LONG_FROM_VECTOR(element_offset_index, element_defn_ptr);
        element_size = *(IDL_msp->IDL_offset_vec + element_offset_index);
    }


    control_data = (IDL_varying_control_t *)rpc_ss_mem_alloc(
                                &IDL_msp->IDL_mem_handle,
                                dimensionality * sizeof(IDL_varying_control_t));
    control_data[dimensionality-1].subslice_size = element_size;
    control_data[dimensionality-1].index_value = 
                                            range_list[dimensionality-1].lower;
    for (i=dimensionality-2; i>=0; i--)
    {
        control_data[i].index_value = range_list[i].lower;
        control_data[i].subslice_size = control_data[i+1].subslice_size
                                                            * Z_values[i+1];   
    }

    pointee_desc.dimensionality = 0;
    do {
        inner_slice_address = (idl_byte *)array_addr;
        for (i=0; (unsigned32)i<dimensionality; i++)
        {
            inner_slice_address += control_data[i].index_value
                                        * control_data[i].subslice_size;
        }
        for (i = range_list[dimensionality-1].lower;
             i < range_list[dimensionality-1].upper;
             i++)
        {
            if (base_type == IDL_DT_FIXED_STRUCT)
            {
                rpc_ss_ndr_m_struct_pointees( base_type, element_defn_index,
                                                inner_slice_address, IDL_msp );
                inner_slice_address += element_size;
            }
            else if (base_type == IDL_DT_ENC_UNION)
            {
                rpc_ss_ndr_m_enc_union_or_ptees(inner_slice_address,
                                                element_defn_index,
                                                idl_true, IDL_msp );
                inner_slice_address += element_size;
            }
            else
            {
                /* Array of pointers */
                if (*(rpc_void_p_t *)inner_slice_address != NULL)
                {
                    rpc_ss_pointee_desc_from_data( defn_vec_ptr, 
                                           *(rpc_void_p_t *)inner_slice_address,
                                           NULL, NULL, &pointee_desc, IDL_msp );
                    rpc_ss_ndr_marsh_pointee( defn_vec_ptr,
                                          *(rpc_void_p_t *)inner_slice_address,
                                          (base_type == IDL_DT_FULL_PTR),
                                          &pointee_desc, IDL_msp );
                }
                inner_slice_address += sizeof(rpc_void_p_t *);
            }
        }
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
    rpc_ss_rlse_data_pointee_desc( &pointee_desc, IDL_msp );

    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)control_data);
}

/******************************************************************************/
/*                                                                            */
/*  Starting from the index in the definition vector for a varying or         */
/*  open array, marshall the array's pointees                                */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_m_dvo_arr_ptees
(
    /* [in] */  idl_ulong_int defn_index,   /* Index of array description */
    /* [in] */  rpc_void_p_t array_addr,
    /* [in] */  rpc_void_p_t struct_addr,   /* Address of structure array is
                                     part of. NULL if array is a parameter */
    /* [in] */  idl_ulong_int *struct_offset_vec_ptr,
                                           /*  NULL if array is a parameter */
    /* [in] */  idl_ulong_int flags,
    IDL_msp_t IDL_msp
)
{
    idl_byte *defn_vec_ptr;
    idl_ulong_int dimensionality;
    IDL_bound_pair_t *bounds_list;
    idl_ulong_int *Z_values;
    IDL_bound_pair_t *range_list;
    idl_boolean add_null;       /* Dummy argument in procedure call */

    defn_vec_ptr = IDL_msp->IDL_type_vec + defn_index; 
    dimensionality = (idl_ulong_int)*defn_vec_ptr;
    defn_vec_ptr++;
    /* By design defn_vec_ptr is longword aligned */
    if (IDL_M_FLAGS_TEST(flags,IDL_M_CONF_ARRAY))
    {
        bounds_list = NULL;
        rpc_ss_build_bounds_list( &defn_vec_ptr, array_addr, struct_addr,
                                    struct_offset_vec_ptr, dimensionality,
                                    &bounds_list, IDL_msp );
    }
    else
    {
	if (IDL_msp->IDL_type_vec[TVEC_INT_REP_OFFSET] != NDR_LOCAL_INT_REP)
	    rpc_ss_fixed_bounds_from_vector(dimensionality, defn_vec_ptr,
					    &bounds_list, IDL_msp);
	else
	    bounds_list = (IDL_bound_pair_t *)defn_vec_ptr;
        defn_vec_ptr += dimensionality * sizeof(IDL_bound_pair_t);
    }
    Z_values = NULL;
    rpc_ss_Z_values_from_bounds( bounds_list, dimensionality, &Z_values,
                                                                   IDL_msp );
    range_list = NULL;
    rpc_ss_build_range_list( &defn_vec_ptr, array_addr, struct_addr,
                             struct_offset_vec_ptr, dimensionality, bounds_list,
                             &range_list, &add_null, IDL_msp );
    rpc_ss_ndr_m_v_or_o_arr_ptees( array_addr, Z_values, dimensionality,
                                    range_list, defn_vec_ptr, IDL_msp );
    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)range_list);
    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)Z_values);
    if (IDL_msp->IDL_type_vec[TVEC_INT_REP_OFFSET] == NDR_LOCAL_INT_REP)
    {
	if (IDL_M_FLAGS_TEST(flags,IDL_M_CONF_ARRAY))
	    rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, 
				 (byte_p_t)bounds_list);
    }
    else 
	rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle, (byte_p_t)bounds_list);

}

/******************************************************************************/
/*                                                                            */
/*  Find out if a pointee is a non-fixed array or non-encapsulated union.     */
/*  If it is, use data in memory to set up the data structures needed for     */
/*  marshalling it                                                            */
/*                                                                            */
/******************************************************************************/
void rpc_ss_pointee_desc_from_data
(
    /* [in] */  idl_byte *defn_vec_ptr, /* Points at definition of pointee */
    /* [in] */  rpc_void_p_t array_addr,    /* Need only be non-NULL if pointee
                                requires string bound/data limit calculations */
    /* [in] */  rpc_void_p_t struct_addr,   /* Address of structure pointer is
                                       part of NULL if pointer is a parameter */
    /* [in] */  idl_ulong_int *struct_offset_vec_ptr,
                                           /*  NULL if pointer is a parameter */
    /* [out] */ IDL_pointee_desc_t *p_pointee_desc, /* Pointer to data structure
                                     to be filled in with pointee description */
    IDL_msp_t IDL_msp
)
{
    idl_ulong_int array_defn_index;
    idl_byte *array_defn_ptr;
    idl_boolean add_null;       /* Dummy argument in procedure call */
    idl_ulong_int dimensionality;

    if (*defn_vec_ptr == IDL_DT_STRING)
        defn_vec_ptr++;
    p_pointee_desc->pointee_type = *defn_vec_ptr;
    if (p_pointee_desc->pointee_type == IDL_DT_N_E_UNION)
    {
        p_pointee_desc->struct_addr = struct_addr;
        p_pointee_desc->struct_offset_vec_ptr = struct_offset_vec_ptr;
        return;
    }
    else 
        if ( (p_pointee_desc->pointee_type != IDL_DT_VARYING_ARRAY)
        && (p_pointee_desc->pointee_type != IDL_DT_CONF_ARRAY)
        && (p_pointee_desc->pointee_type != IDL_DT_OPEN_ARRAY) )
    {
        return;
    }

    /* Remainder of this routine only executed for non-fixed array */
    defn_vec_ptr++;
    p_pointee_desc->base_type_has_pointers =
                            IDL_PROP_TEST( *defn_vec_ptr, IDL_PROP_HAS_PTRS );
    defn_vec_ptr++;
    IDL_DISCARD_LONG_FROM_VECTOR( defn_vec_ptr );   /* Full array definition */
    IDL_GET_LONG_FROM_VECTOR( array_defn_index, defn_vec_ptr );
    array_defn_ptr = IDL_msp->IDL_type_vec + array_defn_index;
    dimensionality = (idl_ulong_int)*array_defn_ptr;
    array_defn_ptr++;

    /* Minimize the number of malloc's used to build the data structure */
    /* In code that follows, range list and bounds list will be allocated
        out of this memory, if necessary */
    if (dimensionality > p_pointee_desc->dimensionality)
    {
        if (p_pointee_desc->dimensionality > 0)
        {
            /* Some array description storage already exists, release it */
            rpc_ss_mem_item_free(&IDL_msp->IDL_mem_handle,
                                            (byte_p_t)p_pointee_desc->Z_values);
        }    
        p_pointee_desc->Z_values = (idl_ulong_int *) rpc_ss_mem_alloc(
                                  &IDL_msp->IDL_mem_handle,
                                    dimensionality *
                                     ( sizeof(idl_ulong_int) /* Z_values */
                                      + 2 * sizeof(IDL_bound_pair_t)
                                        /* bounds list and range list */ ) );
    }
    p_pointee_desc->dimensionality = dimensionality;

    switch(p_pointee_desc->pointee_type)
    {
        case IDL_DT_VARYING_ARRAY:
	    if (IDL_msp->IDL_type_vec[TVEC_INT_REP_OFFSET] != NDR_LOCAL_INT_REP){
		/* Use storage after Z_values and range list for bounds list */
		p_pointee_desc->bounds_list =  (IDL_bound_pair_t *)
		    (p_pointee_desc->Z_values + p_pointee_desc->dimensionality)
		    + p_pointee_desc->dimensionality;
		rpc_ss_fixed_bounds_from_vector(p_pointee_desc->dimensionality,
						array_defn_ptr,
						&(p_pointee_desc->bounds_list),
						IDL_msp);
	    }
	    else
		p_pointee_desc->bounds_list = (IDL_bound_pair_t *)array_defn_ptr;
            /* Build the range list in store allocated for the pointee desc,
                following the Z-values */
            p_pointee_desc->range_list =  (IDL_bound_pair_t *)
                    (p_pointee_desc->Z_values + p_pointee_desc->dimensionality);
            rpc_ss_Z_values_from_bounds( p_pointee_desc->bounds_list,
                                         p_pointee_desc->dimensionality,
                                         &(p_pointee_desc->Z_values),
                                         IDL_msp );
            array_defn_ptr += p_pointee_desc->dimensionality
                                    * IDL_FIXED_BOUND_PAIR_WIDTH;
            rpc_ss_build_range_list( &array_defn_ptr, array_addr, struct_addr,
                                     struct_offset_vec_ptr,
                                     p_pointee_desc->dimensionality,
                                     p_pointee_desc->bounds_list,
                                     &(p_pointee_desc->range_list),
                                     &add_null, IDL_msp );
            break;
        case IDL_DT_CONF_ARRAY:
            /* Build the bounds list in store allocated for the pointee desc,
                following the Z-values */
            p_pointee_desc->bounds_list =  (IDL_bound_pair_t *)
                    (p_pointee_desc->Z_values + p_pointee_desc->dimensionality);
            rpc_ss_build_bounds_list( &array_defn_ptr, array_addr, struct_addr,
                                     struct_offset_vec_ptr,
                                     p_pointee_desc->dimensionality,
                                     &(p_pointee_desc->bounds_list),
                                     IDL_msp );
            rpc_ss_Z_values_from_bounds( p_pointee_desc->bounds_list,
                                         p_pointee_desc->dimensionality,
                                         &(p_pointee_desc->Z_values),
                                         IDL_msp );
            break;
        case IDL_DT_OPEN_ARRAY:
            /* Store allocated for the pointee desc is organized
                Z-values, bounds list, range list */
            p_pointee_desc->bounds_list =  (IDL_bound_pair_t *)
                    (p_pointee_desc->Z_values + p_pointee_desc->dimensionality);
            p_pointee_desc->range_list =  p_pointee_desc->bounds_list
                                               + p_pointee_desc->dimensionality;
            rpc_ss_build_bounds_list( &array_defn_ptr, array_addr, struct_addr,
                                     struct_offset_vec_ptr,
                                     p_pointee_desc->dimensionality,
                                     &(p_pointee_desc->bounds_list),
                                     IDL_msp );
            rpc_ss_Z_values_from_bounds( p_pointee_desc->bounds_list,
                                         p_pointee_desc->dimensionality,
                                         &(p_pointee_desc->Z_values),
                                         IDL_msp );
            rpc_ss_build_range_list( &array_defn_ptr, array_addr, struct_addr,
                                     struct_offset_vec_ptr,
                                     p_pointee_desc->dimensionality,
                                     p_pointee_desc->bounds_list,
                                     &(p_pointee_desc->range_list),
                                     &add_null, IDL_msp );
            break;
        default:
            DCETHREAD_RAISE( rpc_x_coding_error );
    }
    p_pointee_desc->array_base_defn_ptr = array_defn_ptr;
}
