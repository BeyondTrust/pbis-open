/*
 * 
 * (c) Copyright 1993 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1993 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1993 DIGITAL EQUIPMENT CORPORATION
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
**  NAME:
**
**      rpcexc.h
**
**  FACILITY:
**
**      Remote Procedure Call
**
**  ABSTRACT:
**
**      Include file to make NIDL exceptions visible to user code
**
**  %a%private_begin
**
**
**  %a%private_end
**
*/

#ifndef RPCEXC_H
#define RPCEXC_H 	1

#if defined(VMS) || defined(__VMS)
#pragma nostandard
#if defined(__DECC) || defined(__cplusplus)
#pragma extern_model __save
#pragma extern_model __strict_refdef
#endif /* DEC C or C++ */
#endif /* VMS  */

#include <dce/dcethread.h>

#ifndef HAS_GLOBALDEFS 
#define RPC_EXTERN_EXCEPTION extern dcethread_exc
#else
#define RPC_EXTERN_EXCEPTION globalref dcethread_exc
#endif /* HAS_GLOBALDEFS */

/* DG and common errors */

RPC_EXTERN_EXCEPTION rpc_x_assoc_grp_not_found;
RPC_EXTERN_EXCEPTION rpc_x_call_timeout;
RPC_EXTERN_EXCEPTION rpc_x_cancel_timeout;
RPC_EXTERN_EXCEPTION rpc_x_coding_error;
RPC_EXTERN_EXCEPTION rpc_x_comm_failure;
RPC_EXTERN_EXCEPTION rpc_x_context_id_not_found;
RPC_EXTERN_EXCEPTION rpc_x_endpoint_not_found;
RPC_EXTERN_EXCEPTION rpc_x_in_args_too_big;
RPC_EXTERN_EXCEPTION rpc_x_invalid_binding;
RPC_EXTERN_EXCEPTION rpc_x_invalid_bound;
RPC_EXTERN_EXCEPTION rpc_x_invalid_call_opt;
RPC_EXTERN_EXCEPTION rpc_x_invalid_naf_id;
RPC_EXTERN_EXCEPTION rpc_x_invalid_rpc_protseq;
RPC_EXTERN_EXCEPTION rpc_x_invalid_tag;
RPC_EXTERN_EXCEPTION rpc_x_invalid_timeout;
RPC_EXTERN_EXCEPTION rpc_x_manager_not_entered;
RPC_EXTERN_EXCEPTION rpc_x_max_descs_exceeded;
RPC_EXTERN_EXCEPTION rpc_x_no_fault;
RPC_EXTERN_EXCEPTION rpc_x_no_memory;
RPC_EXTERN_EXCEPTION rpc_x_not_rpc_tower;
RPC_EXTERN_EXCEPTION rpc_x_object_not_found;
RPC_EXTERN_EXCEPTION rpc_x_op_rng_error;
RPC_EXTERN_EXCEPTION rpc_x_protocol_error;
RPC_EXTERN_EXCEPTION rpc_x_protseq_not_supported;
RPC_EXTERN_EXCEPTION rpc_x_rpcd_comm_failure;
RPC_EXTERN_EXCEPTION rpc_x_server_too_busy;
RPC_EXTERN_EXCEPTION rpc_x_unknown_if;
RPC_EXTERN_EXCEPTION rpc_x_unknown_error;
RPC_EXTERN_EXCEPTION rpc_x_unknown_mgr_type;
RPC_EXTERN_EXCEPTION rpc_x_unknown_reject;
RPC_EXTERN_EXCEPTION rpc_x_unknown_remote_fault;
RPC_EXTERN_EXCEPTION rpc_x_unsupported_type;
RPC_EXTERN_EXCEPTION rpc_x_who_are_you_failed;
RPC_EXTERN_EXCEPTION rpc_x_wrong_boot_time;
RPC_EXTERN_EXCEPTION rpc_x_wrong_kind_of_binding;
RPC_EXTERN_EXCEPTION uuid_x_getconf_failure;
RPC_EXTERN_EXCEPTION uuid_x_internal_error;
RPC_EXTERN_EXCEPTION uuid_x_no_address;
RPC_EXTERN_EXCEPTION uuid_x_socket_failure;

/* CN errors */

RPC_EXTERN_EXCEPTION rpc_x_access_control_info_inv;
RPC_EXTERN_EXCEPTION rpc_x_assoc_grp_max_exceeded;
RPC_EXTERN_EXCEPTION rpc_x_assoc_shutdown;
RPC_EXTERN_EXCEPTION rpc_x_cannot_accept;
RPC_EXTERN_EXCEPTION rpc_x_cannot_connect;
RPC_EXTERN_EXCEPTION rpc_x_cant_inq_socket;
RPC_EXTERN_EXCEPTION rpc_x_connect_closed_by_rem;
RPC_EXTERN_EXCEPTION rpc_x_connect_no_resources;
RPC_EXTERN_EXCEPTION rpc_x_connect_rejected;
RPC_EXTERN_EXCEPTION rpc_x_connect_timed_out;
RPC_EXTERN_EXCEPTION rpc_x_connection_aborted;
RPC_EXTERN_EXCEPTION rpc_x_connection_closed;
RPC_EXTERN_EXCEPTION rpc_x_host_unreachable;
RPC_EXTERN_EXCEPTION rpc_x_invalid_endpoint_format;
RPC_EXTERN_EXCEPTION rpc_x_loc_connect_aborted;
RPC_EXTERN_EXCEPTION rpc_x_network_unreachable;
RPC_EXTERN_EXCEPTION rpc_x_no_rem_endpoint;
RPC_EXTERN_EXCEPTION rpc_x_rem_host_crashed;
RPC_EXTERN_EXCEPTION rpc_x_rem_host_down;
RPC_EXTERN_EXCEPTION rpc_x_rem_network_shutdown;
RPC_EXTERN_EXCEPTION rpc_x_rpc_prot_version_mismatch;
RPC_EXTERN_EXCEPTION rpc_x_string_too_long;
RPC_EXTERN_EXCEPTION rpc_x_too_many_rem_connects;
RPC_EXTERN_EXCEPTION rpc_x_tsyntaxes_unsupported;

/* NS import routine errors */

RPC_EXTERN_EXCEPTION rpc_x_binding_vector_full;
RPC_EXTERN_EXCEPTION rpc_x_entry_not_found;
RPC_EXTERN_EXCEPTION rpc_x_group_not_found;
RPC_EXTERN_EXCEPTION rpc_x_incomplete_name;
RPC_EXTERN_EXCEPTION rpc_x_invalid_arg;
RPC_EXTERN_EXCEPTION rpc_x_invalid_import_context;
RPC_EXTERN_EXCEPTION rpc_x_invalid_inquiry_context;
RPC_EXTERN_EXCEPTION rpc_x_invalid_inquiry_type;
RPC_EXTERN_EXCEPTION rpc_x_invalid_lookup_context;
RPC_EXTERN_EXCEPTION rpc_x_invalid_name_syntax;
RPC_EXTERN_EXCEPTION rpc_x_invalid_object;
RPC_EXTERN_EXCEPTION rpc_x_invalid_vers_option;
RPC_EXTERN_EXCEPTION rpc_x_name_service_unavailable;
RPC_EXTERN_EXCEPTION rpc_x_no_env_setup;
RPC_EXTERN_EXCEPTION rpc_x_no_more_bindings;
RPC_EXTERN_EXCEPTION rpc_x_no_more_elements;
RPC_EXTERN_EXCEPTION rpc_x_not_found;
RPC_EXTERN_EXCEPTION rpc_x_not_rpc_entry;
RPC_EXTERN_EXCEPTION rpc_x_obj_uuid_not_found;
RPC_EXTERN_EXCEPTION rpc_x_profile_not_found;
RPC_EXTERN_EXCEPTION rpc_x_unsupported_name_syntax;

/* Authentication errors */

RPC_EXTERN_EXCEPTION rpc_x_auth_bad_integrity;
RPC_EXTERN_EXCEPTION rpc_x_auth_badaddr;
RPC_EXTERN_EXCEPTION rpc_x_auth_baddirection;
RPC_EXTERN_EXCEPTION rpc_x_auth_badkeyver;
RPC_EXTERN_EXCEPTION rpc_x_auth_badmatch;
RPC_EXTERN_EXCEPTION rpc_x_auth_badorder;
RPC_EXTERN_EXCEPTION rpc_x_auth_badseq;
RPC_EXTERN_EXCEPTION rpc_x_auth_badversion;
RPC_EXTERN_EXCEPTION rpc_x_auth_field_toolong;
RPC_EXTERN_EXCEPTION rpc_x_auth_inapp_cksum;
RPC_EXTERN_EXCEPTION rpc_x_auth_method;
RPC_EXTERN_EXCEPTION rpc_x_auth_msg_type;
RPC_EXTERN_EXCEPTION rpc_x_auth_modified;
RPC_EXTERN_EXCEPTION rpc_x_auth_mut_fail;
RPC_EXTERN_EXCEPTION rpc_x_auth_nokey;
RPC_EXTERN_EXCEPTION rpc_x_auth_not_us;
RPC_EXTERN_EXCEPTION rpc_x_auth_repeat;
RPC_EXTERN_EXCEPTION rpc_x_auth_skew;
RPC_EXTERN_EXCEPTION rpc_x_auth_tkt_expired;
RPC_EXTERN_EXCEPTION rpc_x_auth_tkt_nyv;
RPC_EXTERN_EXCEPTION rpc_x_call_id_not_found;
RPC_EXTERN_EXCEPTION rpc_x_credentials_too_large;
RPC_EXTERN_EXCEPTION rpc_x_invalid_checksum;
RPC_EXTERN_EXCEPTION rpc_x_invalid_crc;
RPC_EXTERN_EXCEPTION rpc_x_invalid_credentials;
RPC_EXTERN_EXCEPTION rpc_x_key_id_not_found;

/* Stub support errors */

RPC_EXTERN_EXCEPTION rpc_x_ss_char_trans_open_fail;
RPC_EXTERN_EXCEPTION rpc_x_ss_char_trans_short_file;
RPC_EXTERN_EXCEPTION rpc_x_ss_context_damaged;
RPC_EXTERN_EXCEPTION rpc_x_ss_context_mismatch;
RPC_EXTERN_EXCEPTION rpc_x_ss_in_null_context;
RPC_EXTERN_EXCEPTION rpc_x_ss_pipe_closed;
RPC_EXTERN_EXCEPTION rpc_x_ss_pipe_comm_error;
RPC_EXTERN_EXCEPTION rpc_x_ss_pipe_discipline_error;
RPC_EXTERN_EXCEPTION rpc_x_ss_pipe_empty;
RPC_EXTERN_EXCEPTION rpc_x_ss_pipe_memory;
RPC_EXTERN_EXCEPTION rpc_x_ss_pipe_order;
RPC_EXTERN_EXCEPTION rpc_x_ss_remote_comm_failure;
RPC_EXTERN_EXCEPTION rpc_x_ss_remote_no_memory;
RPC_EXTERN_EXCEPTION rpc_x_ss_bad_buffer;
RPC_EXTERN_EXCEPTION rpc_x_ss_bad_es_action;
RPC_EXTERN_EXCEPTION rpc_x_ss_wrong_es_version;
RPC_EXTERN_EXCEPTION rpc_x_ss_incompatible_codesets;
RPC_EXTERN_EXCEPTION rpc_x_stub_protocol_error;
RPC_EXTERN_EXCEPTION rpc_x_unknown_stub_rtl_if_vers;
RPC_EXTERN_EXCEPTION rpc_x_ss_codeset_conv_error;

#if defined(VMS) || defined(__VMS)
#if defined(__DECC) || defined(__cplusplus)
#pragma extern_model __restore
#endif /* DEC C or C++ */

#pragma standard

#endif /* VMS */

#endif /* _RPCEXC_H */
