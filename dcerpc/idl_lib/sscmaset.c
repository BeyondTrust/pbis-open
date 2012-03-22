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
**
**  NAME:
**
**      sscmaset.c
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**      CMA machinery used by IDL stubs, conversion between
**       exceptions and status codes, maps between statuses,
**       ASCII/EBCDIC support, optimization support
**
**  VERSION: DCE 1.0
**
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


/* The ordering of the following 3 includes should NOT be changed! */
#include <dce/rpc.h>
#include <dce/stubbase.h>
#include <lsysdep.h>

#ifdef MIA
#include <dce/idlddefs.h>
#endif

#   include <stdio.h>
#       include <stdlib.h>

/*
 * declare the default character translation tables.  Not in stubbase, since
 * the stubs never reference them directly.
 */
globalref rpc_trans_tab_t ndr_g_def_ascii_to_ebcdic;
globalref rpc_trans_tab_t ndr_g_def_ebcdic_to_ascii;

/*
 * Forward referenced routines.
 */
static error_status_t rpc_ss_map_fault_code
(
    ndr_ulong_int fault_code
);

/* rpc_ss_call_free is needed because we can't use 'free' as a procedure argument
    or in an assignment, because CMA has redefined free to be
    a macro with an argument */
void rpc_ss_call_free
(
    rpc_void_p_t address
)
{
    free( address );
}


#if defined(CMA_INCLUDE) && !defined(VMS)  /* Need CMA_INCLUDE which provides cma_global_lock */

/******************************************************************************/
/*                                                                            */
/*    Set up jacket for getenv() to make it thread safe.                      */
/*    If the base implementation provides a safe getenv, this can be discarded*/
/*                                                                            */
/******************************************************************************/
char * safe_getenv
(
    char *variable
)
{
    char *result;
    cma_lock_global ();
    result = (char*)getenv (variable);
    cma_unlock_global ();
    return result;
}
#define getenv safe_getenv
#endif

#if defined(CMA_INCLUDE) && defined(VMS) 
/******************************************************************************/
/* Note: CMA now provides these jackets on all platforms other than VAX/VMS   */
/*       so we don't need them anywhere else                                  */
/*                                                                            */
/*    Set up jackets for fopen, fread, and fclose to make them thread safe.   */
/*    If the base implementation provides safe ones, these can be discarded.  */
/*                                                                            */
/******************************************************************************/
FILE * safe_fopen
(
    char *filename,
    char *type
)
{
    FILE *result;
    cma_lock_global();
    result = fopen(filename, type);
    cma_unlock_global();
    return result;
}
#define fopen safe_fopen

int safe_fread
(
    char        *ptr,
    unsigned    sizeof_ptr,
    unsigned    nitems,
    FILE        *stream
)
{
    int result;
    cma_lock_global();
    result = fread(ptr, sizeof_ptr, nitems, stream);
    cma_unlock_global();
    return result;
}
#define fread safe_fread

int safe_fclose
(
    FILE *stream
)
{
    int result;
    cma_lock_global();
    result = fclose(stream);
    cma_unlock_global();
    return result;
}
#define fclose safe_fclose

#endif

/***************************************************************************
 * Initialize the character translation tables.
 * This routine is designed to be called from rpc_ss_init_client.
 * That guarantees that it will only be invoked once per address space.
 **************************************************************************/
void rpc_ss_trans_table_init(
    void
)
{
char *filename;
FILE *fid;

    /*
     * Attempt to translate the environment variable which points us
     * to a file containing replacement translation tables.
     */
    if (!(filename = (char*)getenv("DCERPCCHARTRANS"))) {
        ndr_g_ascii_to_ebcdic = (rpc_trans_tab_p_t) &ndr_g_def_ascii_to_ebcdic[0];
        ndr_g_ebcdic_to_ascii = (rpc_trans_tab_p_t) &ndr_g_def_ebcdic_to_ascii[0];
        return;
    }

    /*
     * We have a translation, therefore we attempt to open the file.
     * A failure to open the file must be reported.  We can't use
     * the defalt translation table if the user told us to use some
     * alternate ones. Raise a file open failure exception.
     */
    ndr_g_ascii_to_ebcdic = (rpc_trans_tab_p_t) malloc (512);

    if (ndr_g_ascii_to_ebcdic==NULL)
        DCETHREAD_RAISE(rpc_x_no_memory);


    if (!(fid = fopen (filename, "r")))
        DCETHREAD_RAISE(rpc_x_ss_char_trans_open_fail);

    /*
     * Successfully opened the file.  There must be at least 512 bytes
     * in the file.  The first 256 are the ASCII->EBCDIC table--the second
     * 256 are the EBCDIC->ASCII table.  Bytes after 512 are ignored.
     */
    ndr_g_ebcdic_to_ascii = (rpc_trans_tab_p_t)(
                     (char*) ndr_g_ascii_to_ebcdic + 256);

    /*
     * If fewer than 512 bytes are read, raise a shortfile exception.
     */
    if (fread ((char *)ndr_g_ascii_to_ebcdic, 1, 512, fid) < (size_t)512)
    {
        fclose (fid);
        DCETHREAD_RAISE(rpc_x_ss_char_trans_short_file);
    }

    /*
     * We are done, with the table pointers initialized to either the default
     * tables, or to the custom tables read from the file.
     */
    fclose (fid);
}

/******************************************************************************/
/*                                                                            */
/*    Set up CMA machinery required by client                                 */
/*                                                                            */
/******************************************************************************/

#ifndef VMS
ndr_boolean rpc_ss_client_is_set_up = ndr_false;
#endif
static RPC_SS_THREADS_ONCE_T client_once = RPC_SS_THREADS_ONCE_INIT;

globaldef dcethread_exc rpc_x_assoc_grp_not_found;
globaldef dcethread_exc rpc_x_call_timeout;
globaldef dcethread_exc rpc_x_cancel_timeout;
globaldef dcethread_exc rpc_x_coding_error;
globaldef dcethread_exc rpc_x_context_id_not_found;
globaldef dcethread_exc rpc_x_comm_failure;
globaldef dcethread_exc rpc_x_endpoint_not_found;
globaldef dcethread_exc rpc_x_in_args_too_big;
globaldef dcethread_exc rpc_x_invalid_binding;
globaldef dcethread_exc rpc_x_invalid_bound;
globaldef dcethread_exc rpc_x_invalid_call_opt;
globaldef dcethread_exc rpc_x_invalid_naf_id;
globaldef dcethread_exc rpc_x_invalid_rpc_protseq;
globaldef dcethread_exc rpc_x_invalid_tag;
globaldef dcethread_exc rpc_x_invalid_timeout;
globaldef dcethread_exc rpc_x_manager_not_entered;
globaldef dcethread_exc rpc_x_max_descs_exceeded;
globaldef dcethread_exc rpc_x_no_fault;
globaldef dcethread_exc rpc_x_no_memory;
globaldef dcethread_exc rpc_x_not_rpc_tower;
globaldef dcethread_exc rpc_x_op_rng_error;
globaldef dcethread_exc rpc_x_object_not_found;
globaldef dcethread_exc rpc_x_protocol_error;
globaldef dcethread_exc rpc_x_protseq_not_supported;
globaldef dcethread_exc rpc_x_rpcd_comm_failure;
globaldef dcethread_exc rpc_x_server_too_busy;
globaldef dcethread_exc rpc_x_unknown_error;
globaldef dcethread_exc rpc_x_unknown_if;
globaldef dcethread_exc rpc_x_unknown_mgr_type;
globaldef dcethread_exc rpc_x_unknown_reject;
globaldef dcethread_exc rpc_x_unknown_remote_fault;
globaldef dcethread_exc rpc_x_unsupported_type;
globaldef dcethread_exc rpc_x_who_are_you_failed;
globaldef dcethread_exc rpc_x_wrong_boot_time;
globaldef dcethread_exc rpc_x_wrong_kind_of_binding;
globaldef dcethread_exc uuid_x_getconf_failure;
globaldef dcethread_exc uuid_x_no_address;
globaldef dcethread_exc uuid_x_internal_error;
globaldef dcethread_exc uuid_x_socket_failure;

globaldef dcethread_exc rpc_x_access_control_info_inv;
globaldef dcethread_exc rpc_x_assoc_grp_max_exceeded;
globaldef dcethread_exc rpc_x_assoc_shutdown;
globaldef dcethread_exc rpc_x_cannot_accept;
globaldef dcethread_exc rpc_x_cannot_connect;
globaldef dcethread_exc rpc_x_cannot_set_nodelay;
globaldef dcethread_exc rpc_x_cant_inq_socket;
globaldef dcethread_exc rpc_x_connect_closed_by_rem;
globaldef dcethread_exc rpc_x_connect_no_resources;
globaldef dcethread_exc rpc_x_connect_rejected;
globaldef dcethread_exc rpc_x_connect_timed_out;
globaldef dcethread_exc rpc_x_connection_aborted;
globaldef dcethread_exc rpc_x_connection_closed;
globaldef dcethread_exc rpc_x_host_unreachable;
globaldef dcethread_exc rpc_x_invalid_endpoint_format;
globaldef dcethread_exc rpc_x_loc_connect_aborted;
globaldef dcethread_exc rpc_x_network_unreachable;
globaldef dcethread_exc rpc_x_no_rem_endpoint;
globaldef dcethread_exc rpc_x_rem_host_crashed;
globaldef dcethread_exc rpc_x_rem_host_down;
globaldef dcethread_exc rpc_x_rem_network_shutdown;
globaldef dcethread_exc rpc_x_rpc_prot_version_mismatch;
globaldef dcethread_exc rpc_x_string_too_long;
globaldef dcethread_exc rpc_x_too_many_rem_connects;
globaldef dcethread_exc rpc_x_tsyntaxes_unsupported;

globaldef dcethread_exc rpc_x_binding_vector_full;
globaldef dcethread_exc rpc_x_entry_not_found;
globaldef dcethread_exc rpc_x_group_not_found;
globaldef dcethread_exc rpc_x_incomplete_name;
globaldef dcethread_exc rpc_x_invalid_arg;
globaldef dcethread_exc rpc_x_invalid_import_context;
globaldef dcethread_exc rpc_x_invalid_inquiry_context;
globaldef dcethread_exc rpc_x_invalid_inquiry_type;
globaldef dcethread_exc rpc_x_invalid_lookup_context;
globaldef dcethread_exc rpc_x_invalid_name_syntax;
globaldef dcethread_exc rpc_x_invalid_object;
globaldef dcethread_exc rpc_x_invalid_vers_option;
globaldef dcethread_exc rpc_x_name_service_unavailable;
globaldef dcethread_exc rpc_x_no_env_setup;
globaldef dcethread_exc rpc_x_no_more_bindings;
globaldef dcethread_exc rpc_x_no_more_elements;
globaldef dcethread_exc rpc_x_no_ns_permission;
globaldef dcethread_exc rpc_x_not_found;
globaldef dcethread_exc rpc_x_not_rpc_entry;
globaldef dcethread_exc rpc_x_obj_uuid_not_found;
globaldef dcethread_exc rpc_x_profile_not_found;
globaldef dcethread_exc rpc_x_unsupported_name_syntax;

globaldef dcethread_exc rpc_x_auth_bad_integrity;
globaldef dcethread_exc rpc_x_auth_badaddr;
globaldef dcethread_exc rpc_x_auth_baddirection;
globaldef dcethread_exc rpc_x_auth_badkeyver;
globaldef dcethread_exc rpc_x_auth_badmatch;
globaldef dcethread_exc rpc_x_auth_badorder;
globaldef dcethread_exc rpc_x_auth_badseq;
globaldef dcethread_exc rpc_x_auth_badversion;
globaldef dcethread_exc rpc_x_auth_field_toolong;
globaldef dcethread_exc rpc_x_auth_inapp_cksum;
globaldef dcethread_exc rpc_x_auth_method;
globaldef dcethread_exc rpc_x_auth_msg_type;
globaldef dcethread_exc rpc_x_auth_modified;
globaldef dcethread_exc rpc_x_auth_mut_fail;
globaldef dcethread_exc rpc_x_auth_nokey;
globaldef dcethread_exc rpc_x_auth_not_us;
globaldef dcethread_exc rpc_x_auth_repeat;
globaldef dcethread_exc rpc_x_auth_skew;
globaldef dcethread_exc rpc_x_auth_tkt_expired;
globaldef dcethread_exc rpc_x_auth_tkt_nyv;
globaldef dcethread_exc rpc_x_call_id_not_found;
globaldef dcethread_exc rpc_x_credentials_too_large;
globaldef dcethread_exc rpc_x_invalid_checksum;
globaldef dcethread_exc rpc_x_invalid_crc;
globaldef dcethread_exc rpc_x_invalid_credentials;
globaldef dcethread_exc rpc_x_key_id_not_found;

globaldef dcethread_exc rpc_x_ss_pipe_empty;
globaldef dcethread_exc rpc_x_ss_pipe_closed;
globaldef dcethread_exc rpc_x_ss_pipe_order;
globaldef dcethread_exc rpc_x_ss_pipe_discipline_error;
globaldef dcethread_exc rpc_x_ss_pipe_comm_error;
globaldef dcethread_exc rpc_x_ss_pipe_memory;
globaldef dcethread_exc rpc_x_ss_context_mismatch;
globaldef dcethread_exc rpc_x_ss_context_damaged;
globaldef dcethread_exc rpc_x_ss_in_null_context;
globaldef dcethread_exc rpc_x_ss_char_trans_short_file;
globaldef dcethread_exc rpc_x_ss_char_trans_open_fail;
globaldef dcethread_exc rpc_x_ss_remote_comm_failure;
globaldef dcethread_exc rpc_x_ss_remote_no_memory;
globaldef dcethread_exc rpc_x_ss_bad_buffer;
globaldef dcethread_exc rpc_x_ss_bad_es_action;
globaldef dcethread_exc rpc_x_ss_wrong_es_version;
globaldef dcethread_exc rpc_x_ss_incompatible_codesets;
globaldef dcethread_exc rpc_x_stub_protocol_error;
globaldef dcethread_exc rpc_x_unknown_stub_rtl_if_vers;
globaldef dcethread_exc rpc_x_ss_codeset_conv_error;

static void rpc_ss_init_client(
    void
)
{
    /* Initialize exceptions */
    DCETHREAD_EXC_INIT(rpc_x_assoc_grp_not_found);
    dcethread_exc_setstatus(&rpc_x_assoc_grp_not_found,rpc_s_assoc_grp_not_found);
    DCETHREAD_EXC_INIT(rpc_x_call_timeout);
    dcethread_exc_setstatus(&rpc_x_call_timeout,rpc_s_call_timeout);
    DCETHREAD_EXC_INIT(rpc_x_cancel_timeout);
    dcethread_exc_setstatus(&rpc_x_cancel_timeout,rpc_s_cancel_timeout);
    DCETHREAD_EXC_INIT(rpc_x_coding_error);
    dcethread_exc_setstatus(&rpc_x_coding_error,rpc_s_coding_error);
    DCETHREAD_EXC_INIT(rpc_x_comm_failure);
    dcethread_exc_setstatus(&rpc_x_comm_failure,rpc_s_comm_failure);
    DCETHREAD_EXC_INIT(rpc_x_context_id_not_found);
    dcethread_exc_setstatus(&rpc_x_context_id_not_found,rpc_s_context_id_not_found);
    DCETHREAD_EXC_INIT(rpc_x_endpoint_not_found);
    dcethread_exc_setstatus(&rpc_x_endpoint_not_found,rpc_s_endpoint_not_found);
    DCETHREAD_EXC_INIT(rpc_x_in_args_too_big);
    dcethread_exc_setstatus(&rpc_x_in_args_too_big,rpc_s_in_args_too_big);
    DCETHREAD_EXC_INIT(rpc_x_invalid_binding);
    dcethread_exc_setstatus(&rpc_x_invalid_binding,rpc_s_invalid_binding);
    DCETHREAD_EXC_INIT(rpc_x_invalid_bound);
    dcethread_exc_setstatus(&rpc_x_invalid_bound,rpc_s_fault_invalid_bound);
    DCETHREAD_EXC_INIT(rpc_x_invalid_call_opt);
    dcethread_exc_setstatus(&rpc_x_invalid_call_opt,rpc_s_invalid_call_opt);
    DCETHREAD_EXC_INIT(rpc_x_invalid_naf_id);
    dcethread_exc_setstatus(&rpc_x_invalid_naf_id,rpc_s_invalid_naf_id);
    DCETHREAD_EXC_INIT(rpc_x_invalid_rpc_protseq);
    dcethread_exc_setstatus(&rpc_x_invalid_rpc_protseq,rpc_s_invalid_rpc_protseq);
    DCETHREAD_EXC_INIT(rpc_x_invalid_tag);
    dcethread_exc_setstatus(&rpc_x_invalid_tag,rpc_s_fault_invalid_tag);
    DCETHREAD_EXC_INIT(rpc_x_invalid_timeout);
    dcethread_exc_setstatus(&rpc_x_invalid_timeout,rpc_s_invalid_timeout);
    DCETHREAD_EXC_INIT(rpc_x_manager_not_entered);
    dcethread_exc_setstatus(&rpc_x_manager_not_entered,rpc_s_manager_not_entered);
    DCETHREAD_EXC_INIT(rpc_x_max_descs_exceeded);
    dcethread_exc_setstatus(&rpc_x_max_descs_exceeded,rpc_s_max_descs_exceeded);
    DCETHREAD_EXC_INIT(rpc_x_no_fault);
    dcethread_exc_setstatus(&rpc_x_no_fault,rpc_s_no_fault);
    DCETHREAD_EXC_INIT(rpc_x_no_memory);
    dcethread_exc_setstatus(&rpc_x_no_memory,rpc_s_no_memory);
    DCETHREAD_EXC_INIT(rpc_x_not_rpc_tower);
    dcethread_exc_setstatus(&rpc_x_not_rpc_tower,rpc_s_not_rpc_tower);
    DCETHREAD_EXC_INIT(rpc_x_object_not_found);
    dcethread_exc_setstatus(&rpc_x_object_not_found,rpc_s_object_not_found);
    DCETHREAD_EXC_INIT(rpc_x_op_rng_error);
    dcethread_exc_setstatus(&rpc_x_op_rng_error,rpc_s_op_rng_error);
    DCETHREAD_EXC_INIT(rpc_x_protocol_error);
    dcethread_exc_setstatus(&rpc_x_protocol_error,rpc_s_protocol_error);
    DCETHREAD_EXC_INIT(rpc_x_protseq_not_supported);
    dcethread_exc_setstatus(&rpc_x_protseq_not_supported,rpc_s_protseq_not_supported);
    DCETHREAD_EXC_INIT(rpc_x_rpcd_comm_failure);
    dcethread_exc_setstatus(&rpc_x_rpcd_comm_failure,rpc_s_rpcd_comm_failure);
    DCETHREAD_EXC_INIT(rpc_x_server_too_busy);
    dcethread_exc_setstatus(&rpc_x_server_too_busy,rpc_s_server_too_busy);
    DCETHREAD_EXC_INIT(rpc_x_unknown_error);
    dcethread_exc_setstatus(&rpc_x_unknown_error,rpc_s_unknown_error);
    DCETHREAD_EXC_INIT(rpc_x_unknown_if);
    dcethread_exc_setstatus(&rpc_x_unknown_if,rpc_s_unknown_if);
    DCETHREAD_EXC_INIT(rpc_x_unknown_mgr_type);
    dcethread_exc_setstatus(&rpc_x_unknown_mgr_type,rpc_s_unknown_mgr_type);
    DCETHREAD_EXC_INIT(rpc_x_unknown_reject);
    dcethread_exc_setstatus(&rpc_x_unknown_reject,rpc_s_unknown_reject);
    DCETHREAD_EXC_INIT(rpc_x_unknown_remote_fault);
    dcethread_exc_setstatus(&rpc_x_unknown_remote_fault,rpc_s_fault_unspec);
    DCETHREAD_EXC_INIT(rpc_x_unsupported_type);
    dcethread_exc_setstatus(&rpc_x_unsupported_type,rpc_s_unsupported_type);
    DCETHREAD_EXC_INIT(rpc_x_who_are_you_failed);
    dcethread_exc_setstatus(&rpc_x_who_are_you_failed,rpc_s_who_are_you_failed);
    DCETHREAD_EXC_INIT(rpc_x_wrong_boot_time);
    dcethread_exc_setstatus(&rpc_x_wrong_boot_time,rpc_s_wrong_boot_time);
    DCETHREAD_EXC_INIT(rpc_x_wrong_kind_of_binding);
    dcethread_exc_setstatus(&rpc_x_wrong_kind_of_binding,rpc_s_wrong_kind_of_binding);
    DCETHREAD_EXC_INIT(uuid_x_getconf_failure);
    dcethread_exc_setstatus(&uuid_x_getconf_failure,uuid_s_getconf_failure);
    DCETHREAD_EXC_INIT(uuid_x_internal_error);
    dcethread_exc_setstatus(&uuid_x_internal_error,uuid_s_internal_error);
    DCETHREAD_EXC_INIT(uuid_x_no_address);
    dcethread_exc_setstatus(&uuid_x_no_address,uuid_s_no_address);
    DCETHREAD_EXC_INIT(uuid_x_socket_failure);
    dcethread_exc_setstatus(&uuid_x_socket_failure,uuid_s_socket_failure);

    DCETHREAD_EXC_INIT(rpc_x_access_control_info_inv);
    dcethread_exc_setstatus(&rpc_x_access_control_info_inv,rpc_s_access_control_info_inv);
    DCETHREAD_EXC_INIT(rpc_x_assoc_grp_max_exceeded);
    dcethread_exc_setstatus(&rpc_x_assoc_grp_max_exceeded,rpc_s_assoc_grp_max_exceeded);
    DCETHREAD_EXC_INIT(rpc_x_assoc_shutdown);
    dcethread_exc_setstatus(&rpc_x_assoc_shutdown,rpc_s_assoc_shutdown);
    DCETHREAD_EXC_INIT(rpc_x_cannot_accept);
    dcethread_exc_setstatus(&rpc_x_cannot_accept,rpc_s_cannot_accept);
    DCETHREAD_EXC_INIT(rpc_x_cannot_connect);
    dcethread_exc_setstatus(&rpc_x_cannot_connect,rpc_s_cannot_connect);
    DCETHREAD_EXC_INIT(rpc_x_cannot_set_nodelay);
    dcethread_exc_setstatus(&rpc_x_cannot_set_nodelay,rpc_s_cannot_set_nodelay);
    DCETHREAD_EXC_INIT(rpc_x_cant_inq_socket);
    dcethread_exc_setstatus(&rpc_x_cant_inq_socket,rpc_s_cant_inq_socket);
    DCETHREAD_EXC_INIT(rpc_x_connect_closed_by_rem);
    dcethread_exc_setstatus(&rpc_x_connect_closed_by_rem,rpc_s_connect_closed_by_rem);
    DCETHREAD_EXC_INIT(rpc_x_connect_no_resources);
    dcethread_exc_setstatus(&rpc_x_connect_no_resources,rpc_s_connect_no_resources);
    DCETHREAD_EXC_INIT(rpc_x_connect_rejected);
    dcethread_exc_setstatus(&rpc_x_connect_rejected,rpc_s_connect_rejected);
    DCETHREAD_EXC_INIT(rpc_x_connect_timed_out);
    dcethread_exc_setstatus(&rpc_x_connect_timed_out,rpc_s_connect_timed_out);
    DCETHREAD_EXC_INIT(rpc_x_connection_aborted);
    dcethread_exc_setstatus(&rpc_x_connection_aborted,rpc_s_connection_aborted);
    DCETHREAD_EXC_INIT(rpc_x_connection_closed);
    dcethread_exc_setstatus(&rpc_x_connection_closed,rpc_s_connection_closed);
    DCETHREAD_EXC_INIT(rpc_x_host_unreachable);
    dcethread_exc_setstatus(&rpc_x_host_unreachable,rpc_s_host_unreachable);
    DCETHREAD_EXC_INIT(rpc_x_invalid_endpoint_format);
    dcethread_exc_setstatus(&rpc_x_invalid_endpoint_format,rpc_s_invalid_endpoint_format);
    DCETHREAD_EXC_INIT(rpc_x_loc_connect_aborted);
    dcethread_exc_setstatus(&rpc_x_loc_connect_aborted,rpc_s_loc_connect_aborted);
    DCETHREAD_EXC_INIT(rpc_x_network_unreachable);
    dcethread_exc_setstatus(&rpc_x_network_unreachable,rpc_s_network_unreachable);
    DCETHREAD_EXC_INIT(rpc_x_no_rem_endpoint);
    dcethread_exc_setstatus(&rpc_x_no_rem_endpoint,rpc_s_no_rem_endpoint);
    DCETHREAD_EXC_INIT(rpc_x_rem_host_crashed);
    dcethread_exc_setstatus(&rpc_x_rem_host_crashed,rpc_s_rem_host_crashed);
    DCETHREAD_EXC_INIT(rpc_x_rem_host_down);
    dcethread_exc_setstatus(&rpc_x_rem_host_down,rpc_s_rem_host_down);
    DCETHREAD_EXC_INIT(rpc_x_rem_network_shutdown);
    dcethread_exc_setstatus(&rpc_x_rem_network_shutdown,rpc_s_rem_network_shutdown);
    DCETHREAD_EXC_INIT(rpc_x_rpc_prot_version_mismatch);
    dcethread_exc_setstatus(&rpc_x_rpc_prot_version_mismatch,rpc_s_rpc_prot_version_mismatch);
    DCETHREAD_EXC_INIT(rpc_x_string_too_long);
    dcethread_exc_setstatus(&rpc_x_string_too_long,rpc_s_string_too_long);
    DCETHREAD_EXC_INIT(rpc_x_too_many_rem_connects);
    dcethread_exc_setstatus(&rpc_x_too_many_rem_connects,rpc_s_too_many_rem_connects);
    DCETHREAD_EXC_INIT(rpc_x_tsyntaxes_unsupported);
    dcethread_exc_setstatus(&rpc_x_tsyntaxes_unsupported,rpc_s_tsyntaxes_unsupported);

    DCETHREAD_EXC_INIT(rpc_x_binding_vector_full);
    dcethread_exc_setstatus(&rpc_x_binding_vector_full,rpc_s_binding_vector_full);
    DCETHREAD_EXC_INIT(rpc_x_entry_not_found);
    dcethread_exc_setstatus(&rpc_x_entry_not_found,rpc_s_entry_not_found);
    DCETHREAD_EXC_INIT(rpc_x_group_not_found);
    dcethread_exc_setstatus(&rpc_x_group_not_found,rpc_s_group_not_found);
    DCETHREAD_EXC_INIT(rpc_x_incomplete_name);
    dcethread_exc_setstatus(&rpc_x_incomplete_name,rpc_s_incomplete_name);
    DCETHREAD_EXC_INIT(rpc_x_invalid_arg);
    dcethread_exc_setstatus(&rpc_x_invalid_arg,rpc_s_invalid_arg);
    DCETHREAD_EXC_INIT(rpc_x_invalid_import_context);
    dcethread_exc_setstatus(&rpc_x_invalid_import_context,rpc_s_invalid_import_context);
    DCETHREAD_EXC_INIT(rpc_x_invalid_inquiry_context);
    dcethread_exc_setstatus(&rpc_x_invalid_inquiry_context,rpc_s_invalid_inquiry_context);
    DCETHREAD_EXC_INIT(rpc_x_invalid_inquiry_type);
    dcethread_exc_setstatus(&rpc_x_invalid_inquiry_type,rpc_s_invalid_inquiry_type);
    DCETHREAD_EXC_INIT(rpc_x_invalid_lookup_context);
    dcethread_exc_setstatus(&rpc_x_invalid_lookup_context,rpc_s_invalid_lookup_context);
    DCETHREAD_EXC_INIT(rpc_x_invalid_name_syntax);
    dcethread_exc_setstatus(&rpc_x_invalid_name_syntax,rpc_s_invalid_name_syntax);
    DCETHREAD_EXC_INIT(rpc_x_invalid_object);
    dcethread_exc_setstatus(&rpc_x_invalid_object,rpc_s_invalid_object);
    DCETHREAD_EXC_INIT(rpc_x_invalid_vers_option);
    dcethread_exc_setstatus(&rpc_x_invalid_vers_option,rpc_s_invalid_vers_option);
    DCETHREAD_EXC_INIT(rpc_x_name_service_unavailable);
    dcethread_exc_setstatus(&rpc_x_name_service_unavailable,rpc_s_name_service_unavailable);
    DCETHREAD_EXC_INIT(rpc_x_no_env_setup);
    dcethread_exc_setstatus(&rpc_x_no_env_setup,rpc_s_no_env_setup);
    DCETHREAD_EXC_INIT(rpc_x_no_more_bindings);
    dcethread_exc_setstatus(&rpc_x_no_more_bindings,rpc_s_no_more_bindings);
    DCETHREAD_EXC_INIT(rpc_x_no_more_elements);
    dcethread_exc_setstatus(&rpc_x_no_more_elements,rpc_s_no_more_elements);
    DCETHREAD_EXC_INIT(rpc_x_no_ns_permission);
    dcethread_exc_setstatus(&rpc_x_no_ns_permission,rpc_s_no_ns_permission);
    DCETHREAD_EXC_INIT(rpc_x_not_found);
    dcethread_exc_setstatus(&rpc_x_not_found,rpc_s_not_found);
    DCETHREAD_EXC_INIT(rpc_x_not_rpc_entry);
    dcethread_exc_setstatus(&rpc_x_not_rpc_entry,rpc_s_not_rpc_entry);
    DCETHREAD_EXC_INIT(rpc_x_obj_uuid_not_found);
    dcethread_exc_setstatus(&rpc_x_obj_uuid_not_found,rpc_s_obj_uuid_not_found);
    DCETHREAD_EXC_INIT(rpc_x_profile_not_found);
    dcethread_exc_setstatus(&rpc_x_profile_not_found,rpc_s_profile_not_found);
    DCETHREAD_EXC_INIT(rpc_x_unsupported_name_syntax);
    dcethread_exc_setstatus(&rpc_x_unsupported_name_syntax,rpc_s_unsupported_name_syntax);

    DCETHREAD_EXC_INIT(rpc_x_auth_bad_integrity);
    dcethread_exc_setstatus(&rpc_x_auth_bad_integrity,rpc_s_auth_bad_integrity);
    DCETHREAD_EXC_INIT(rpc_x_auth_badaddr);
    dcethread_exc_setstatus(&rpc_x_auth_badaddr,rpc_s_auth_badaddr);
    DCETHREAD_EXC_INIT(rpc_x_auth_baddirection);
    dcethread_exc_setstatus(&rpc_x_auth_baddirection,rpc_s_auth_baddirection);
    DCETHREAD_EXC_INIT(rpc_x_auth_badkeyver);
    dcethread_exc_setstatus(&rpc_x_auth_badkeyver,rpc_s_auth_badkeyver);
    DCETHREAD_EXC_INIT(rpc_x_auth_badmatch);
    dcethread_exc_setstatus(&rpc_x_auth_badmatch,rpc_s_auth_badmatch);
    DCETHREAD_EXC_INIT(rpc_x_auth_badorder);
    dcethread_exc_setstatus(&rpc_x_auth_badorder,rpc_s_auth_badorder);
    DCETHREAD_EXC_INIT(rpc_x_auth_badseq);
    dcethread_exc_setstatus(&rpc_x_auth_badseq,rpc_s_auth_badseq);
    DCETHREAD_EXC_INIT(rpc_x_auth_badversion);
    dcethread_exc_setstatus(&rpc_x_auth_badversion,rpc_s_auth_badversion);
    DCETHREAD_EXC_INIT(rpc_x_auth_field_toolong);
    dcethread_exc_setstatus(&rpc_x_auth_field_toolong,rpc_s_auth_field_toolong);
    DCETHREAD_EXC_INIT(rpc_x_auth_inapp_cksum);
    dcethread_exc_setstatus(&rpc_x_auth_inapp_cksum,rpc_s_auth_inapp_cksum);
    DCETHREAD_EXC_INIT(rpc_x_auth_method);
    dcethread_exc_setstatus(&rpc_x_auth_method,rpc_s_auth_method);
    DCETHREAD_EXC_INIT(rpc_x_auth_msg_type);
    dcethread_exc_setstatus(&rpc_x_auth_msg_type,rpc_s_auth_msg_type);
    DCETHREAD_EXC_INIT(rpc_x_auth_modified);
    dcethread_exc_setstatus(&rpc_x_auth_modified,rpc_s_auth_modified);
    DCETHREAD_EXC_INIT(rpc_x_auth_mut_fail);
    dcethread_exc_setstatus(&rpc_x_auth_mut_fail,rpc_s_auth_mut_fail);
    DCETHREAD_EXC_INIT(rpc_x_auth_nokey);
    dcethread_exc_setstatus(&rpc_x_auth_nokey,rpc_s_auth_nokey);
    DCETHREAD_EXC_INIT(rpc_x_auth_not_us);
    dcethread_exc_setstatus(&rpc_x_auth_not_us,rpc_s_auth_not_us);
    DCETHREAD_EXC_INIT(rpc_x_auth_repeat);
    dcethread_exc_setstatus(&rpc_x_auth_repeat,rpc_s_auth_repeat);
    DCETHREAD_EXC_INIT(rpc_x_auth_skew);
    dcethread_exc_setstatus(&rpc_x_auth_skew,rpc_s_auth_skew);
    DCETHREAD_EXC_INIT(rpc_x_auth_tkt_expired);
    dcethread_exc_setstatus(&rpc_x_auth_tkt_expired,rpc_s_auth_tkt_expired);
    DCETHREAD_EXC_INIT(rpc_x_auth_tkt_nyv);
    dcethread_exc_setstatus(&rpc_x_auth_tkt_nyv,rpc_s_auth_tkt_nyv);
    DCETHREAD_EXC_INIT(rpc_x_call_id_not_found);
    dcethread_exc_setstatus(&rpc_x_call_id_not_found,rpc_s_call_id_not_found);
    DCETHREAD_EXC_INIT(rpc_x_credentials_too_large);
    dcethread_exc_setstatus(&rpc_x_credentials_too_large,rpc_s_credentials_too_large);
    DCETHREAD_EXC_INIT(rpc_x_invalid_checksum);
    dcethread_exc_setstatus(&rpc_x_invalid_checksum,rpc_s_invalid_checksum);
    DCETHREAD_EXC_INIT(rpc_x_invalid_crc);
    dcethread_exc_setstatus(&rpc_x_invalid_crc,rpc_s_invalid_crc);
    DCETHREAD_EXC_INIT(rpc_x_invalid_credentials);
    dcethread_exc_setstatus(&rpc_x_invalid_credentials,rpc_s_invalid_credentials);
    DCETHREAD_EXC_INIT(rpc_x_key_id_not_found);
    dcethread_exc_setstatus(&rpc_x_key_id_not_found,rpc_s_key_id_not_found);

    DCETHREAD_EXC_INIT(rpc_x_ss_char_trans_open_fail);
    dcethread_exc_setstatus(&rpc_x_ss_char_trans_open_fail,rpc_s_ss_char_trans_open_fail);
    DCETHREAD_EXC_INIT(rpc_x_ss_char_trans_short_file);
    dcethread_exc_setstatus(&rpc_x_ss_char_trans_short_file,rpc_s_ss_char_trans_short_file);
    DCETHREAD_EXC_INIT(rpc_x_ss_pipe_empty);
    dcethread_exc_setstatus(&rpc_x_ss_pipe_empty,rpc_s_fault_pipe_empty);
    DCETHREAD_EXC_INIT(rpc_x_ss_pipe_closed);
    dcethread_exc_setstatus(&rpc_x_ss_pipe_closed,rpc_s_fault_pipe_closed);
    DCETHREAD_EXC_INIT(rpc_x_ss_pipe_order);
    dcethread_exc_setstatus(&rpc_x_ss_pipe_order,rpc_s_fault_pipe_order);
    DCETHREAD_EXC_INIT(rpc_x_ss_pipe_discipline_error);
    dcethread_exc_setstatus(&rpc_x_ss_pipe_discipline_error,rpc_s_fault_pipe_discipline);
    DCETHREAD_EXC_INIT(rpc_x_ss_pipe_comm_error);
    dcethread_exc_setstatus(&rpc_x_ss_pipe_comm_error,rpc_s_fault_pipe_comm_error);
    DCETHREAD_EXC_INIT(rpc_x_ss_pipe_memory);
    dcethread_exc_setstatus(&rpc_x_ss_pipe_memory,rpc_s_fault_pipe_memory);
    DCETHREAD_EXC_INIT(rpc_x_ss_context_mismatch);
    dcethread_exc_setstatus(&rpc_x_ss_context_mismatch,rpc_s_fault_context_mismatch);
    DCETHREAD_EXC_INIT(rpc_x_ss_context_damaged);
    dcethread_exc_setstatus(&rpc_x_ss_context_damaged,rpc_s_ss_context_damaged);
    DCETHREAD_EXC_INIT(rpc_x_ss_in_null_context);
    dcethread_exc_setstatus(&rpc_x_ss_in_null_context,rpc_s_ss_in_null_context);
    DCETHREAD_EXC_INIT(rpc_x_ss_remote_comm_failure);
    dcethread_exc_setstatus(&rpc_x_ss_remote_comm_failure,rpc_s_fault_remote_comm_failure);
    DCETHREAD_EXC_INIT(rpc_x_ss_remote_no_memory);
    dcethread_exc_setstatus(&rpc_x_ss_remote_no_memory,rpc_s_fault_remote_no_memory);
    DCETHREAD_EXC_INIT(rpc_x_ss_bad_buffer);
    dcethread_exc_setstatus(&rpc_x_ss_bad_buffer,rpc_s_ss_bad_buffer);
    DCETHREAD_EXC_INIT(rpc_x_ss_bad_es_action);
    dcethread_exc_setstatus(&rpc_x_ss_bad_es_action,rpc_s_ss_bad_es_action);
    DCETHREAD_EXC_INIT(rpc_x_ss_wrong_es_version);
    dcethread_exc_setstatus(&rpc_x_ss_wrong_es_version,rpc_s_ss_wrong_es_version);
    DCETHREAD_EXC_INIT(rpc_x_ss_incompatible_codesets);
    dcethread_exc_setstatus(&rpc_x_ss_incompatible_codesets,rpc_s_ss_incompatible_codesets);
    DCETHREAD_EXC_INIT(rpc_x_stub_protocol_error);
    dcethread_exc_setstatus(&rpc_x_stub_protocol_error,rpc_s_stub_protocol_error);
    DCETHREAD_EXC_INIT(rpc_x_unknown_stub_rtl_if_vers);
    dcethread_exc_setstatus(&rpc_x_unknown_stub_rtl_if_vers,rpc_s_unknown_stub_rtl_if_vers);

    rpc_ss_trans_table_init ();
}

void rpc_ss_init_client_once(
    void
)
{
    RPC_SS_THREADS_INIT;
    RPC_SS_THREADS_ONCE( &client_once, rpc_ss_init_client );

#ifndef VMS
    rpc_ss_client_is_set_up = ndr_true;
#endif

}


#ifdef IDL_ENABLE_STATUS_MAPPING
/******************************************************************************/
/*                                                                            */
/*    Routines to map a DCE status code to/from the local status code format  */
/*                                                                            */
/******************************************************************************/

/* DCE status format definitions */
#define FACILITY_CODE_MASK          0xF0000000
#define FACILITY_CODE_SHIFT         28
#define COMPONENT_CODE_MASK         0x0FFFF000
#define COMPONENT_CODE_SHIFT        12
#define STATUS_CODE_MASK            0x00000FFF
#define STATUS_CODE_SHIFT           0

/* DCE status definitions */
#define dce_rpc_s_mod 0x16c9a000
#define dce_thd_s_mod 0x177db000
#define dce_sec_s_mod 0x17122000

/* Architecture-Specific Status definitions */
#ifdef VMS
#include <stsdef.h>
#include <ssdef.h>
#ifndef sec_s_mod
#define sec_s_mod 249790466
#endif
#endif

/******************************************************************************/
/*                                                                            */
/*    Map a DCE status code to the local status code format                   */
/*                                                                            */
/******************************************************************************/
void rpc_ss_map_dce_to_local_status
(
    error_status_t *status_code_p   /* [in,out] pointer to DCE status -> local status */
)
{
    unsigned long facility_and_comp_code;
    unsigned short  status_code;

    /*
     * extract the DCE component, facility and status codes
     */
    facility_and_comp_code = (*status_code_p & 
        (FACILITY_CODE_MASK|COMPONENT_CODE_MASK));

    status_code = (*status_code_p & STATUS_CODE_MASK)
        >> STATUS_CODE_SHIFT;

#ifdef VMS
    /* Mapping for DCE/RPC status codes */
    if (facility_and_comp_code == dce_rpc_s_mod)
        *status_code_p =  (rpc_s_mod & ~STS$M_CODE) | (status_code << 3);

    /* Mapping for DCE/Security status codes */
    if (facility_and_comp_code == dce_sec_s_mod)
        *status_code_p = (sec_s_mod & ~STS$M_CODE) | (status_code << 3);

    /* Mapping for DCE/THD status codes */
    if (facility_and_comp_code == dce_thd_s_mod)
    {
        /* Have to case this because some are system-specific errors */
        switch (status_code) 
        {
            case 5:     *status_code_p = SS$_ACCVIO; break;
            case 6:     *status_code_p = SS$_EXQUOTA; break;
            case 7:     *status_code_p = SS$_INSFMEM; break;
            case 8:     *status_code_p = SS$_NOPRIV; break;        
            case 9:     *status_code_p = SS$_NORMAL; break;
            case 10:    *status_code_p = SS$_OPCDEC; break;
            case 11:    *status_code_p = SS$_RADRMOD; break;
            case 12:    *status_code_p = SS$_OPCDEC; break;
            case 13:    *status_code_p = SS$_ROPRAND; break;
            case 14:    *status_code_p = SS$_BREAK; break;
            case 15:    *status_code_p = SS$_ABORT; break;
            case 16:    *status_code_p = SS$_COMPAT; break;
            case 17:    *status_code_p = SS$_FLTOVF; break;
            case 18:    *status_code_p = SS$_BADPARAM; break;
            case 19:    *status_code_p = SS$_NOMBX; break;
            case 20:    *status_code_p = SS$_EXCPUTIM; break;
            case 21:    *status_code_p = SS$_EXDISKQUOTA; break;
            case 22:    *status_code_p = SS$_INTOVF; break;
            case 23:    *status_code_p = SS$_INTDIV; break;
            case 24:    *status_code_p = SS$_FLTOVF; break;
            case 25:    *status_code_p = SS$_FLTDIV; break;
            case 26:    *status_code_p = SS$_FLTUND; break;
            case 27:    *status_code_p = SS$_DECOVF; break;
            case 28:    *status_code_p = SS$_SUBRNG; break;
            default:  
                /* Actual DCE/THD status */
                *status_code_p =  (exc_s_exception & ~STS$M_CODE) | (status_code << 3);
                break;
        }
    }

    /* If mapping isn't available, leave it alone */
#endif
}


/******************************************************************************/
/*                                                                            */
/*    Map to a DCE status code from the local status code format              */
/*                                                                            */
/******************************************************************************/
void rpc_ss_map_local_to_dce_status
(
    error_status_t *status_code_p   /* [in,out] pointer to local status -> DCE status */
)
{
#ifdef VMS
    int facility = $VMS_STATUS_FAC_NO(*status_code_p);
    int message_number = $VMS_STATUS_CODE(*status_code_p);

    /* If success, return error_status_ok */
    if ((*status_code_p & 1) != 0) 
    {
        *status_code_p = error_status_ok;
        return;
    }

    /* Otherwise, map for each facility */
    switch (facility)
    {
        case $VMS_STATUS_FAC_NO(rpc_s_mod): /* DCE RPC facility */
            *status_code_p = (dce_rpc_s_mod | message_number);
            break;
        case $VMS_STATUS_FAC_NO(exc_s_exception): /* CMA facility */
            *status_code_p = (dce_thd_s_mod | message_number);
            break;
        case $VMS_STATUS_FAC_NO(sec_s_mod): /* DCE Security facility */
            *status_code_p = (dce_sec_s_mod | message_number);
            break;
        case 0: /* VMS facility */
            /* Map system errors, onto DCE threads status values */
            switch (*status_code_p) 
            {
                case SS$_ACCVIO:    *status_code_p = dce_thd_s_mod | 5; break;
                case SS$_EXQUOTA:   *status_code_p = dce_thd_s_mod | 6; break;
                case SS$_INSFMEM:   *status_code_p = dce_thd_s_mod | 7; break;
                case SS$_NOPRIV:    *status_code_p = dce_thd_s_mod | 8; break;        
                case SS$_NORMAL:    *status_code_p = dce_thd_s_mod | 9; break;
                case SS$_OPCDEC:    *status_code_p = dce_thd_s_mod |10; break;
                case SS$_RADRMOD:   *status_code_p = dce_thd_s_mod |11; break;
                case SS$_ROPRAND:   *status_code_p = dce_thd_s_mod |13; break;
                case SS$_BREAK:     *status_code_p = dce_thd_s_mod |14; break;
                case SS$_ABORT:     *status_code_p = dce_thd_s_mod |15; break;
                case SS$_COMPAT:    *status_code_p = dce_thd_s_mod |16; break;
                case SS$_FLTOVF:    *status_code_p = dce_thd_s_mod |17; break;
                case SS$_BADPARAM:  *status_code_p = dce_thd_s_mod |18; break;
                case SS$_NOMBX:     *status_code_p = dce_thd_s_mod |19; break;
                case SS$_EXCPUTIM:  *status_code_p = dce_thd_s_mod |20; break;
                case SS$_EXDISKQUOTA:*status_code_p = dce_thd_s_mod |21; break;
                case SS$_INTOVF:    *status_code_p = dce_thd_s_mod |22; break;
                case SS$_INTDIV:    *status_code_p = dce_thd_s_mod |23; break;
                case SS$_FLTDIV:    *status_code_p = dce_thd_s_mod |25; break;
                case SS$_FLTUND:    *status_code_p = dce_thd_s_mod |26; break;
                case SS$_DECOVF:    *status_code_p = dce_thd_s_mod |27; break;
                case SS$_SUBRNG:    *status_code_p = dce_thd_s_mod |28; break;
            }
            break;
        default:
            /* If mapping isn't available, leave value alone */
            break;
    }
#endif
}
#endif /* IDL_ENABLE_STATUS_MAPPING */

/******************************************************************************/
/*                                                                            */
/*    Convert contents of fault packet to exception                           */
/*                                                                            */
/******************************************************************************/
static void rpc_ss_raise_arch_exception
(
    ndr_ulong_int fault_code,
    RPC_SS_THREADS_CANCEL_STATE_T async_cancel_state
)
{
    dcethread_exc *p_exception;

    switch (fault_code) {
        case nca_s_fault_context_mismatch:
            p_exception = &rpc_x_ss_context_mismatch;
            break;
        case nca_s_fault_cancel:
            p_exception = &RPC_SS_THREADS_X_CANCELLED;
            break;
        case nca_s_fault_addr_error:
            p_exception = &dcethread_illaddr_e;
            break;
        case nca_s_fault_fp_div_zero:
            p_exception = &dcethread_fltdiv_e;
            break;
        case nca_s_fault_fp_error:
            p_exception = &dcethread_aritherr_e;
            break;
        case nca_s_fault_fp_overflow:
            p_exception = &dcethread_fltovf_e;
            break;
        case nca_s_fault_fp_underflow:
            p_exception = &dcethread_fltund_e;
            break;
        case nca_s_fault_ill_inst:
            p_exception = &dcethread_illinstr_e;
            break;
        case nca_s_fault_int_div_by_zero:
            p_exception = &dcethread_intdiv_e;
            break;
        case nca_s_fault_int_overflow:
            p_exception = &dcethread_intovf_e;
            break;
        case nca_s_fault_invalid_bound:
            p_exception = &rpc_x_invalid_bound;
            break;
        case nca_s_fault_invalid_tag:
            p_exception = &rpc_x_invalid_tag;
            break;
        case nca_s_fault_pipe_closed:
            p_exception = &rpc_x_ss_pipe_closed;
            break;
        case nca_s_fault_pipe_comm_error:
            p_exception = &rpc_x_ss_pipe_comm_error;
            break;
        case nca_s_fault_pipe_discipline:
            p_exception = &rpc_x_ss_pipe_discipline_error;
            break;
        case nca_s_fault_pipe_empty:
            p_exception = &rpc_x_ss_pipe_empty;
            break;
        case nca_s_fault_pipe_memory:
            p_exception = &rpc_x_ss_pipe_memory;
            break;
        case nca_s_fault_pipe_order:
            p_exception = &rpc_x_ss_pipe_order;
            break;
        case nca_s_fault_remote_comm_failure:
            p_exception = &rpc_x_ss_remote_comm_failure;
            break;
        case nca_s_fault_remote_no_memory:
            p_exception = &rpc_x_ss_remote_no_memory;
            break;
        default:
            p_exception = &rpc_x_unknown_remote_fault;
            break;
    }
    RPC_SS_THREADS_RESTORE_ASYNC( async_cancel_state );
    DCETHREAD_RAISE( *p_exception );
}

/******************************************************************************/
/*                                                                            */
/*    Convert contents of fault packet to local error code                    */
/*                                                                            */
/******************************************************************************/
static error_status_t rpc_ss_map_fault_code
(
    ndr_ulong_int fault_code
)
{
    switch (fault_code) {
        case nca_s_fault_addr_error:
            return( rpc_s_fault_addr_error );
        case nca_s_fault_context_mismatch:
            return( rpc_s_fault_context_mismatch );
        case nca_s_fault_cancel:
            return( rpc_s_call_cancelled );
        case nca_s_fault_fp_div_zero:
            return( rpc_s_fault_fp_div_by_zero );
        case nca_s_fault_fp_error:
            return( rpc_s_fault_fp_error );
        case nca_s_fault_fp_overflow:
            return( rpc_s_fault_fp_overflow );
        case nca_s_fault_fp_underflow:
            return( rpc_s_fault_fp_underflow );
        case nca_s_fault_ill_inst:
            return( rpc_s_fault_ill_inst );
        case nca_s_fault_int_div_by_zero:
            return( rpc_s_fault_int_div_by_zero );
        case nca_s_fault_int_overflow:
            return( rpc_s_fault_int_overflow );
        case nca_s_fault_invalid_bound:
            return( rpc_s_fault_invalid_bound );
        case nca_s_fault_invalid_tag:
            return( rpc_s_fault_invalid_tag );
        case nca_s_fault_pipe_closed:
            return( rpc_s_fault_pipe_closed );
        case nca_s_fault_pipe_comm_error:
            return( rpc_s_fault_pipe_comm_error );
        case nca_s_fault_pipe_discipline:
            return( rpc_s_fault_pipe_discipline );
        case nca_s_fault_pipe_empty:
            return( rpc_s_fault_pipe_empty );
        case nca_s_fault_pipe_memory:
            return( rpc_s_fault_pipe_memory );
        case nca_s_fault_pipe_order:
            return( rpc_s_fault_pipe_order );
        case nca_s_fault_remote_comm_failure:
            return( rpc_s_fault_remote_comm_failure );
        case nca_s_fault_remote_no_memory:
            return( rpc_s_fault_remote_no_memory );
        case nca_s_fault_user_defined:
            return(rpc_s_fault_user_defined);
        default:
            return( rpc_s_fault_unspec );
    }
}

/******************************************************************************/
/*                                                                            */
/*    Convert error code to exception                                         */
/*                                                                            */
/******************************************************************************/
static void rpc_ss_raise_impl_exception
(
    ndr_ulong_int result_code,
    RPC_SS_THREADS_CANCEL_STATE_T async_cancel_state
)
{
    dcethread_exc *p_exception;

    switch (result_code) {
        case rpc_s_assoc_grp_not_found:
            p_exception = &rpc_x_assoc_grp_not_found;
            break;
        case rpc_s_call_timeout:
            p_exception = &rpc_x_call_timeout;
            break;
        case rpc_s_cancel_timeout:
            p_exception = &rpc_x_cancel_timeout;
            break;
        case rpc_s_coding_error:
            p_exception = &rpc_x_coding_error;
            break;
        case rpc_s_comm_failure:
            p_exception = &rpc_x_comm_failure;
            break;
        case rpc_s_context_id_not_found:
            p_exception = &rpc_x_context_id_not_found;
            break;
        case rpc_s_endpoint_not_found:
            p_exception = &rpc_x_endpoint_not_found;
            break;
        case rpc_s_in_args_too_big:
            p_exception = &rpc_x_in_args_too_big;
            break;
        case rpc_s_invalid_binding:
            p_exception = &rpc_x_invalid_binding;
            break;
        case rpc_s_invalid_call_opt:
            p_exception = &rpc_x_invalid_call_opt;
            break;
        case rpc_s_invalid_naf_id:
            p_exception = &rpc_x_invalid_naf_id;
            break;
        case rpc_s_invalid_rpc_protseq:
            p_exception = &rpc_x_invalid_rpc_protseq;
            break;
        case rpc_s_invalid_timeout:
            p_exception = &rpc_x_invalid_timeout;
            break;
        case rpc_s_manager_not_entered:
            p_exception = &rpc_x_manager_not_entered;
            break;
        case rpc_s_max_descs_exceeded:
            p_exception = &rpc_x_max_descs_exceeded;
            break;
        case rpc_s_no_fault:
            p_exception = &rpc_x_no_fault;
            break;
        case rpc_s_no_memory:
            p_exception = &rpc_x_no_memory;
            break;
        case rpc_s_not_rpc_tower:
            p_exception = &rpc_x_not_rpc_tower;
            break;
        case rpc_s_object_not_found:
            p_exception = &rpc_x_object_not_found;
            break;
        case rpc_s_op_rng_error:
            p_exception = &rpc_x_op_rng_error;
            break;
        case rpc_s_protocol_error:
            p_exception = &rpc_x_protocol_error;
            break;
        case rpc_s_protseq_not_supported:
            p_exception = &rpc_x_protseq_not_supported;
            break;
        case rpc_s_rpcd_comm_failure:
            p_exception = &rpc_x_rpcd_comm_failure;
            break;
        case rpc_s_unknown_if:
            p_exception = &rpc_x_unknown_if;
            break;
        case rpc_s_unknown_mgr_type:
            p_exception = &rpc_x_unknown_mgr_type;
            break;
        case rpc_s_unknown_reject:
            p_exception = &rpc_x_unknown_reject;
            break;
        case rpc_s_unsupported_type:
            p_exception = &rpc_x_unsupported_type;
            break;
        case rpc_s_who_are_you_failed:
            p_exception = &rpc_x_who_are_you_failed;
            break;
        case rpc_s_wrong_boot_time:
            p_exception = &rpc_x_wrong_boot_time;
            break;
        case rpc_s_wrong_kind_of_binding:
            p_exception = &rpc_x_wrong_kind_of_binding;
            break;
        case uuid_s_getconf_failure:
            p_exception = &uuid_x_getconf_failure;
            break;
        case uuid_s_internal_error:
            p_exception = &uuid_x_internal_error;
            break;
         case uuid_s_no_address:
            p_exception = &uuid_x_no_address;
            break;
        case uuid_s_socket_failure:
            p_exception = &uuid_x_socket_failure;
            break;
       /* CN errors */
        case rpc_s_access_control_info_inv:
            p_exception = &rpc_x_access_control_info_inv;
            break;
        case rpc_s_assoc_grp_max_exceeded:
            p_exception = &rpc_x_assoc_grp_max_exceeded;
            break;
        case rpc_s_assoc_shutdown:
            p_exception = &rpc_x_assoc_shutdown;
            break;
        case rpc_s_cannot_accept:
            p_exception = &rpc_x_cannot_accept;
            break;
        case rpc_s_cannot_connect:
            p_exception = &rpc_x_cannot_connect;
            break;
        case rpc_s_cannot_set_nodelay:
            p_exception = &rpc_x_cannot_set_nodelay;
            break;
        case rpc_s_cant_inq_socket:
            p_exception = &rpc_x_cant_inq_socket;
            break;
        case rpc_s_connect_closed_by_rem:
            p_exception = &rpc_x_connect_closed_by_rem;
            break;
        case rpc_s_connect_no_resources:
            p_exception = &rpc_x_connect_no_resources;
            break;
        case rpc_s_connect_rejected:
            p_exception = &rpc_x_connect_rejected;
            break;
        case rpc_s_connect_timed_out:
            p_exception = &rpc_x_connect_timed_out;
            break;
        case rpc_s_connection_aborted:
            p_exception = &rpc_x_connection_aborted;
            break;
        case rpc_s_connection_closed:
            p_exception = &rpc_x_connection_closed;
            break;
        case rpc_s_host_unreachable:
            p_exception = &rpc_x_host_unreachable;
            break;
        case rpc_s_invalid_endpoint_format:
            p_exception = &rpc_x_invalid_endpoint_format;
            break;
        case rpc_s_loc_connect_aborted:
            p_exception = &rpc_x_loc_connect_aborted;
            break;
        case rpc_s_network_unreachable:
            p_exception = &rpc_x_network_unreachable;
            break;
        case rpc_s_no_rem_endpoint:
            p_exception = &rpc_x_no_rem_endpoint;
            break;
        case rpc_s_rem_host_crashed:
            p_exception = &rpc_x_rem_host_crashed;
            break;
        case rpc_s_rem_host_down:
            p_exception = &rpc_x_rem_host_down;
            break;
        case rpc_s_rem_network_shutdown:
            p_exception = &rpc_x_rem_network_shutdown;
            break;
        case rpc_s_rpc_prot_version_mismatch:
            p_exception = &rpc_x_rpc_prot_version_mismatch;
            break;
        case rpc_s_string_too_long:
            p_exception = &rpc_x_string_too_long;
            break;
        case rpc_s_too_many_rem_connects:
            p_exception = &rpc_x_too_many_rem_connects;
            break;
        case rpc_s_tsyntaxes_unsupported:
            p_exception = &rpc_x_tsyntaxes_unsupported;
            break;
        /* NS import routine errors */
        case rpc_s_binding_vector_full:
            p_exception = &rpc_x_binding_vector_full;
            break;
        case rpc_s_entry_not_found:
            p_exception = &rpc_x_entry_not_found;
            break;
        case rpc_s_group_not_found:
            p_exception = &rpc_x_group_not_found;
            break;
        case rpc_s_incomplete_name:
            p_exception = &rpc_x_incomplete_name;
            break;
        case rpc_s_invalid_arg:
            p_exception = &rpc_x_invalid_arg;
            break;
        case rpc_s_invalid_import_context:
            p_exception = &rpc_x_invalid_import_context;
            break;
        case rpc_s_invalid_inquiry_context:
            p_exception = &rpc_x_invalid_inquiry_context;
            break;
        case rpc_s_invalid_inquiry_type:
            p_exception = &rpc_x_invalid_inquiry_type;
            break;
        case rpc_s_invalid_lookup_context:
            p_exception = &rpc_x_invalid_lookup_context;
            break;
        case rpc_s_invalid_name_syntax:
            p_exception = &rpc_x_invalid_name_syntax;
            break;
        case rpc_s_invalid_object:
            p_exception = &rpc_x_invalid_object;
            break;
        case rpc_s_invalid_vers_option:
            p_exception = &rpc_x_invalid_vers_option;
            break;
        case rpc_s_name_service_unavailable:
            p_exception = &rpc_x_name_service_unavailable;
            break;
        case rpc_s_no_env_setup:
            p_exception = &rpc_x_no_env_setup;
            break;
        case rpc_s_no_more_bindings:
            p_exception = &rpc_x_no_more_bindings;
            break;
        case rpc_s_no_more_elements:
            p_exception = &rpc_x_no_more_elements;
            break;
        case rpc_s_no_ns_permission:
            p_exception = &rpc_x_no_ns_permission;
            break;
        case rpc_s_not_found:
            p_exception = &rpc_x_not_found;
            break;
        case rpc_s_not_rpc_entry:
            p_exception = &rpc_x_not_rpc_entry;
            break;
        case rpc_s_obj_uuid_not_found:
            p_exception = &rpc_x_obj_uuid_not_found;
            break;
        case rpc_s_profile_not_found:
            p_exception = &rpc_x_profile_not_found;
            break;
        case rpc_s_unsupported_name_syntax:
            p_exception = &rpc_x_unsupported_name_syntax;
            break;
        /* Authentication errors */
        case rpc_s_auth_bad_integrity:
            p_exception = &rpc_x_auth_bad_integrity;
            break;
        case rpc_s_auth_badaddr:
            p_exception = &rpc_x_auth_badaddr;
            break;
        case rpc_s_auth_baddirection:
            p_exception = &rpc_x_auth_baddirection;
            break;
        case rpc_s_auth_badkeyver:
            p_exception = &rpc_x_auth_badkeyver;
            break;
        case rpc_s_auth_badmatch:
            p_exception = &rpc_x_auth_badmatch;
            break;
        case rpc_s_auth_badorder:
            p_exception = &rpc_x_auth_badorder;
            break;
        case rpc_s_auth_badseq:
            p_exception = &rpc_x_auth_badseq;
            break;
        case rpc_s_auth_badversion:
            p_exception = &rpc_x_auth_badversion;
            break;
        case rpc_s_auth_field_toolong:
            p_exception = &rpc_x_auth_field_toolong;
            break;
        case rpc_s_auth_inapp_cksum:
            p_exception = &rpc_x_auth_inapp_cksum;
            break;
        case rpc_s_auth_method:
            p_exception = &rpc_x_auth_method;
            break;
        case rpc_s_auth_msg_type:
            p_exception = &rpc_x_auth_msg_type;
            break;
        case rpc_s_auth_modified:
            p_exception = &rpc_x_auth_modified;
            break;
        case rpc_s_auth_mut_fail:
            p_exception = &rpc_x_auth_mut_fail;
            break;
        case rpc_s_auth_nokey:
            p_exception = &rpc_x_auth_nokey;
            break;
        case rpc_s_auth_not_us:
            p_exception = &rpc_x_auth_not_us;
            break;
        case rpc_s_auth_repeat:
            p_exception = &rpc_x_auth_repeat;
            break;
        case rpc_s_auth_skew:
            p_exception = &rpc_x_auth_skew;
            break;
        case rpc_s_auth_tkt_expired:
            p_exception = &rpc_x_auth_tkt_expired;
            break;
        case rpc_s_auth_tkt_nyv:
            p_exception = &rpc_x_auth_tkt_nyv;
            break;
        case rpc_s_call_id_not_found:
            p_exception = &rpc_x_call_id_not_found;
            break;
        case rpc_s_credentials_too_large:
            p_exception = &rpc_x_credentials_too_large;
            break;
        case rpc_s_invalid_checksum:
            p_exception = &rpc_x_invalid_checksum;
            break;
        case rpc_s_invalid_crc:
            p_exception = &rpc_x_invalid_crc;
            break;
        case rpc_s_invalid_credentials:
            p_exception = &rpc_x_invalid_credentials;
            break;
        case rpc_s_key_id_not_found:
            p_exception = &rpc_x_key_id_not_found;
            break;
        /* Other pickling errors */
        case rpc_s_ss_bad_buffer:
            p_exception = &rpc_x_ss_bad_buffer;
            break;
        case rpc_s_ss_bad_es_action:
            p_exception = &rpc_x_ss_bad_es_action;
            break;
        case rpc_s_ss_wrong_es_version:
            p_exception = &rpc_x_ss_wrong_es_version;
            break;
        case rpc_s_ss_incompatible_codesets:
            p_exception = &rpc_x_ss_incompatible_codesets;
            break;
        case rpc_s_stub_protocol_error:
            p_exception = &rpc_x_stub_protocol_error;
            break;
        case rpc_s_unknown_stub_rtl_if_vers:
            p_exception = &rpc_x_unknown_stub_rtl_if_vers;
            break;
        default:
            {
            dcethread_exc unknown_status_exception;
            DCETHREAD_EXC_INIT(unknown_status_exception);
            dcethread_exc_setstatus(&unknown_status_exception, result_code);
            RPC_SS_THREADS_RESTORE_ASYNC( async_cancel_state );
            DCETHREAD_RAISE( unknown_status_exception );
            break;
            }
    }
    RPC_SS_THREADS_RESTORE_ASYNC( async_cancel_state );
    DCETHREAD_RAISE( *p_exception );
}

#ifdef MIA
/******************************************************************************/
/*                                                                            */
/*    Report error status to the caller                                       */
/*    New version - user exceptions                                           */
/*                                                                            */
/******************************************************************************/
void rpc_ss_report_error_2
(
    ndr_ulong_int fault_code,
    ndr_ulong_int user_fault_id,
    ndr_ulong_int result_code,
    RPC_SS_THREADS_CANCEL_STATE_T *p_async_cancel_state,
    error_status_t *p_comm_status,
    error_status_t *p_fault_status,
    dcethread_exc *user_exception_pointers[],
    IDL_msp_t IDL_msp ATTRIBUTE_UNUSED
)
{
    if (p_comm_status != NULL) *p_comm_status = error_status_ok;
    if (p_fault_status != NULL) *p_fault_status = error_status_ok;

    if (fault_code != error_status_ok)
    {
        if ( p_fault_status == NULL )
        {
            if (fault_code == nca_s_fault_user_defined)
            {
                RPC_SS_THREADS_RESTORE_ASYNC( *p_async_cancel_state );
                DCETHREAD_RAISE( *(user_exception_pointers[user_fault_id]) );
            }
            rpc_ss_raise_arch_exception( fault_code, *p_async_cancel_state );
        }
        else
        {
            *p_fault_status = rpc_ss_map_fault_code( fault_code );
            return;
        }
    }
    else if (result_code != error_status_ok)
    {
        if ( p_comm_status == NULL )
            rpc_ss_raise_impl_exception( result_code, *p_async_cancel_state );
        else
        {
            *p_comm_status = result_code;
            return;
        }
    }
}
#endif

/******************************************************************************/
/*                                                                            */
/*    Report error status to the caller                                       */
/*    Old version - no user exceptions                                        */
/*                                                                            */
/******************************************************************************/
void rpc_ss_report_error
(
    ndr_ulong_int fault_code,
    ndr_ulong_int result_code,
    RPC_SS_THREADS_CANCEL_STATE_T async_cancel_state,
    error_status_t *p_comm_status,
    error_status_t *p_fault_status
)
{
    rpc_ss_report_error_2(fault_code, 0, result_code, &async_cancel_state,
                            p_comm_status, p_fault_status, NULL, NULL);
}

/******************************************************************************/
/*                                                                            */
/*  If there is a fault, get the fault packet. Then end the call              */
/*  New interface - user exceptions                                           */
/*                                                                            */
/******************************************************************************/
void rpc_ss_call_end_2
(
    volatile rpc_call_handle_t *p_call_h,
    volatile ndr_ulong_int *p_fault_code,
    volatile ndr_ulong_int *p_user_fault_id,
    volatile error_status_t *p_st
)
{
    rpc_iovector_elt_t iovec_elt;
    ndr_format_t drep;
    error_status_t status;
    rpc_mp_t mp;

/*    *p_fault_code = error_status_ok; Initialization done by stub */
    if ( *p_st == rpc_s_call_faulted )
    {
        rpc_call_receive_fault((rpc_call_handle_t)*p_call_h, &iovec_elt, &drep,
                                 &status );
        if (status == error_status_ok)
        {
            rpc_init_mp(mp, iovec_elt.data_addr);
            rpc_convert_ulong_int(drep, ndr_g_local_drep, mp, (*p_fault_code));
            if (*p_fault_code == nca_s_fault_user_defined)
            {
                rpc_advance_mp(mp, 4);  /* Next longword represents user
                                                                    exception */
                rpc_convert_ulong_int(drep, ndr_g_local_drep, mp, 
                                                    (*p_user_fault_id));
            }
            if (iovec_elt.buff_dealloc != NULL)
            {
                (*iovec_elt.buff_dealloc)(iovec_elt.buff_addr);
                iovec_elt.buff_dealloc = NULL;
            }
            
            /*
             * Remote comm failures are reported by a fault packet.  However,
             * we want to treat them in the same way as a local comm failure,
             * not as a fault.  We do the translation here.
             */
            if (*p_fault_code == nca_s_fault_remote_comm_failure)
            {
                *p_st = rpc_s_fault_remote_comm_failure;
                *p_fault_code = error_status_ok;
            }
        }
        else *p_st = status;
    }
    if ( *p_call_h != NULL )
    {
        rpc_call_end( (rpc_call_handle_t*) p_call_h, &status);
        /* Don't destroy any existing error code with the value from call end */
        if ( *p_st == error_status_ok ) *p_st = status;
    }
}

/******************************************************************************/
/*                                                                            */
/*  If there is a fault, get the fault packet. Then end the call              */
/*  Old interface - no user exceptions                                        */
/*                                                                            */
/******************************************************************************/
void rpc_ss_call_end
(
    volatile rpc_call_handle_t *p_call_h,
    volatile ndr_ulong_int *p_fault_code,
    volatile error_status_t *p_st
)
{
    ndr_ulong_int user_fault_id;    /* Discarded argument */

    rpc_ss_call_end_2(p_call_h, p_fault_code, &user_fault_id, p_st);
}

/******************************************************************************/
/*                                                                            */
/*  Optimization support routine - change receive buffer                      */
/*                                                                            */
/******************************************************************************/
void rpc_ss_new_recv_buff
(
    rpc_iovector_elt_t *elt,
    rpc_call_handle_t call_h,
    rpc_mp_t *p_mp,
    volatile error_status_t *st
)
{
    if (elt->buff_dealloc && (elt->data_len != 0))
    {
        (*elt->buff_dealloc)(elt->buff_addr);
        elt->buff_dealloc = NULL;
    }

    rpc_call_receive(call_h, elt, (unsigned32*)st);
    if (*st == error_status_ok)
    {
        if (elt->data_addr != NULL)
        {
            rpc_init_mp((*p_mp), elt->data_addr);
            return;
        }
        else
            *st = rpc_s_stub_protocol_error;
    }
    {
        /* If cancelled, raise the cancelled exception */
        if (*st==rpc_s_call_cancelled) DCETHREAD_RAISE(RPC_SS_THREADS_X_CANCELLED);

        /*
         *  Otherwise, raise the pipe comm error which causes the stub to
         *  report the value of the status variable.
         */
        DCETHREAD_RAISE(rpc_x_ss_pipe_comm_error);
    }
}
