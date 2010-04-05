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
 * 
 */
/*
**
**  NAME:
**
**      codesets_stub.h
**
**	This file cannot be IDL file, since 
**		idl_cs_convert_t 		-> (idlbase.h)
**		wchar_t 			-> (stdlib.h)
**	are defined in header file.  IDL file cannot import *.h files.
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  This file defines stub support routines, which support marshalling and
**  unmarshalling of i18n data.  Code set conversion is done automatically
**  when code set interoperability functionality is enabled.
**  
**
*/

#ifndef _CODESETS_STUB_H
#define _CODESETS_STUB_H

#include <dce/idlbase.h>
#include <stdlib.h>

/*
 * cs_byte is used as I18N byte type from I18N applications.
 */
typedef ndr_byte cs_byte;

/*
 * rpc_ns_import_ctx_add_eval routine requires the identification for
 * the evaluation routines.  If more evaluation routines are supported
 * in the future, the new identifications need to be added here.
 */
#define rpc_c_eval_type_codesets		0x0001
#define rpc_c_custom_eval_type_codesets		0x0002


extern void cs_byte_net_size (
	/* [in] */	rpc_binding_handle_t	h,
	/* [in] */	unsigned32		tag,
	/* [in] */	unsigned32		l_storage_len,
	/* [out] */	idl_cs_convert_t	*p_convert_type,
	/* [out] */	unsigned32		*p_w_storage_len,
	/* [out] */	error_status_t		*status
);

extern void wchar_t_net_size (
	/* [in] */	rpc_binding_handle_t	h,
	/* [in] */	unsigned32		tag,
	/* [in] */	unsigned32		l_storage_len,
	/* [out] */	idl_cs_convert_t	*p_convert_type,
	/* [out] */	unsigned32		*p_w_storage_len,
	/* [out] */	error_status_t		*status
);

extern void cs_byte_local_size (
	/* [in] */	rpc_binding_handle_t	h,
	/* [in] */	unsigned32		tag,
	/* [in] */	unsigned32		w_storage_len,
	/* [out] */	idl_cs_convert_t	*p_convert_type,
	/* [out] */	unsigned32		*p_l_storage_len,
	/* [out] */	error_status_t		*status
);

extern void wchar_t_local_size (
	/* [in] */	rpc_binding_handle_t	h,
	/* [in] */	unsigned32		tag,
	/* [in] */	unsigned32		w_storage_len,
	/* [out] */	idl_cs_convert_t	*p_convert_type,
	/* [out] */	unsigned32		*p_l_storage_len,
	/* [out] */	error_status_t		*status
);

extern void cs_byte_to_netcs (
	/* [in] */	rpc_binding_handle_t	h,
	/* [in] */	unsigned32		tag,
	/* [in] */	idl_byte		*ldata,
	/* [in] */	unsigned32		l_data_len,
	/* [out] */	idl_byte		*wdata,
	/* [out] */	unsigned32		*p_w_data_len,
	/* [out] */	error_status_t		*status
);

extern void wchar_t_to_netcs (
	/* [in] */	rpc_binding_handle_t	h,
	/* [in] */	unsigned32		tag,
	/* [in] */	wchar_t			*ldata,
	/* [in] */	unsigned32		l_data_len,
	/* [out] */	idl_byte		*wdata,
	/* [out] */	unsigned32		*p_w_data_len,
	/* [out] */	error_status_t		*status
);

extern void cs_byte_from_netcs (
	/* [in] */	rpc_binding_handle_t	h,
	/* [in] */	unsigned32		tag,
	/* [in] */	idl_byte		*wdata,
	/* [in] */	unsigned32		w_data_len,
	/* [in] */	unsigned32		l_storage_len,
	/* [out] */	idl_byte		*ldata,
	/* [out] */	unsigned32		*p_l_data_len,
	/* [out] */	error_status_t		*status
);

extern void wchar_t_from_netcs (
	/* [in] */	rpc_binding_handle_t	h,
	/* [in] */	unsigned32		tag,
	/* [in] */	idl_byte		*wdata,
	/* [in] */	unsigned32		w_data_len,
	/* [in] */	unsigned32		l_storage_len,
	/* [out] */	wchar_t			*ldata,
	/* [out] */	unsigned32		*p_l_data_len,
	/* [out] */	error_status_t		*status
);

/*
 * R P C _ C S _ G E T _ T A G S
 *
 * Take a binding handle, and figure out the necessary tags value.
 * This routine is called from stubs.
 */
extern void rpc_cs_get_tags (
	/* [in] */	rpc_binding_handle_t	h,
	/* [in] */	idl_boolean		server_side,
	/* [out] */	unsigned32		*p_stag,
	/* [in, out] */	unsigned32		*p_drtag,
	/* [out] */	unsigned32		*p_rtag,
	/* [out] */	error_status_t		*status
);

#endif
