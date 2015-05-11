/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "lwrpcrt/lwrpcrt.h"
#include <lw/winerror.h>
#include <stdlib.h>
#include <wc16str.h>
#include <errno.h>

RPC_STATUS WideChar16ToMultiByte(PWSTR input, idl_char **output)
{
    if (input == NULL)
    {
        *output = NULL;
    }
    else
    {
        *output = (idl_char *)awc16stombs(input);
        if(*output == NULL)
        {
            if(errno == ENOMEM)
                return rpc_s_no_memory;
            return rpc_s_invalid_arg;
        }
    }
    return rpc_s_ok;
}

RPC_STATUS MultiByteToWideChar16(idl_char *input, PWSTR *output)
{
    *output = ambstowc16s((char *)input);
    if(*output == NULL)
    {
        if(errno == ENOMEM)
            return rpc_s_no_memory;
        return rpc_s_invalid_arg;
    }
    return rpc_s_ok;
}

#define CONVERT_INPUTSTR(var) \
    idl_char *converted_##var = NULL; \
    if(status == rpc_s_ok) \
        status = WideChar16ToMultiByte((var), &converted_##var);

#define DECLARE_OUTPUTSTR(var) \
    idl_char *temp_##var = NULL; \
    *(var) = NULL;

#define CONVERT_OUTPUTSTR(var) \
    if(temp_##var != NULL) \
    { \
        RPC_STATUS unused_status; \
        if(status == rpc_s_ok) \
            status = MultiByteToWideChar16(temp_##var, (var)); \
        rpc_string_free(&temp_##var, &unused_status); \
    }

#define OUTPUTSTR(var)  &temp_##var
#define INPUTSTR(var)  converted_##var

#define FREE_INPUTSTR(var) \
    if(converted_##var != NULL) \
    { \
        free(converted_##var); \
        converted_##var = NULL; \
    }

RPC_STATUS RpcCompatExceptionToCode(dcethread_exc *exc)
{
    RPC_STATUS status;
    if((status = dcethread_exc_getstatus(exc)) == -1)
        return RPC_S_INTERNAL_ERROR;
    return LwMapDCEStatusToWinerror(status);
}

RPC_STATUS g_lastcode = 0;
RPC_STATUS RpcCompatReturnLastCode()
{
    return g_lastcode;
}

RpcCompatReturnCodeFuncPtr RpcCompatReturnLater(RPC_STATUS value)
{
    g_lastcode = value;
    return RpcCompatReturnLastCode;
}

RPC_STATUS RpcStringBindingComposeA(
    /* [in] */ UCHAR *string_object_uuid,
    /* [in] */ UCHAR *string_protseq,
    /* [in] */ UCHAR *string_netaddr,
    /* [in] */ UCHAR *string_endpoint,
    /* [in] */ UCHAR *string_options,
    /* [out] */ UCHAR **string_binding
)
{
    RPC_STATUS status;
    rpc_string_binding_compose(
            (idl_char *)string_object_uuid,
            (idl_char *)string_protseq,
            (idl_char *)string_netaddr,
            (idl_char *)string_endpoint,
            (idl_char *)string_options,
            (idl_char **)string_binding,
            &status);
    return LwMapDCEStatusToWinerror(status);
}

RPC_STATUS RpcStringBindingComposeW(
    /* [in] */ PWSTR string_object_uuid,
    /* [in] */ PWSTR string_protseq,
    /* [in] */ PWSTR string_netaddr,
    /* [in] */ PWSTR string_endpoint,
    /* [in] */ PWSTR string_options,
    /* [out] */ PWSTR *string_binding
)
{
    RPC_STATUS status = rpc_s_ok;
    CONVERT_INPUTSTR(string_object_uuid);
    CONVERT_INPUTSTR(string_protseq);
    CONVERT_INPUTSTR(string_netaddr);
    CONVERT_INPUTSTR(string_endpoint);
    CONVERT_INPUTSTR(string_options);
    DECLARE_OUTPUTSTR(string_binding);

    if(status == rpc_s_ok)
    {
        rpc_string_binding_compose(INPUTSTR(string_object_uuid),
                INPUTSTR(string_protseq),
                INPUTSTR(string_netaddr),
                INPUTSTR(string_endpoint),
                INPUTSTR(string_options),
                OUTPUTSTR(string_binding),
		&status);
    }

    FREE_INPUTSTR(string_object_uuid);
    FREE_INPUTSTR(string_protseq);
    FREE_INPUTSTR(string_netaddr);
    FREE_INPUTSTR(string_endpoint);
    FREE_INPUTSTR(string_options);
    CONVERT_OUTPUTSTR(string_binding);

    return LwMapDCEStatusToWinerror(status);
}

RPC_STATUS RpcBindingFromStringBindingA(
    /* [in] */ UCHAR *string_binding,
    /* [out] */ RPC_BINDING_HANDLE *binding_handle
)
{
    RPC_STATUS status;
    rpc_binding_from_string_binding((idl_char *)string_binding, binding_handle, &status);
    return LwMapDCEStatusToWinerror(status);
}

RPC_STATUS RpcBindingFromStringBindingW(
    /* [in] */ PWSTR string_binding,
    /* [out] */ RPC_BINDING_HANDLE *binding_handle
)
{
    RPC_STATUS status = rpc_s_ok;
    CONVERT_INPUTSTR(string_binding);

    if(status == rpc_s_ok)
    {
        rpc_binding_from_string_binding(
                INPUTSTR(string_binding),
                binding_handle,
                &status);
    }

    FREE_INPUTSTR(string_binding);

    return LwMapDCEStatusToWinerror(status);
}

RPC_STATUS RpcBindingSetAuthInfoA(
    /* [in] */ RPC_BINDING_HANDLE binding_h,
    /* [in] */ UCHAR* server_princ_name,
    /* [in] */ DWORD authn_level,
    /* [in] */ DWORD authn_protocol,
    /* [in] */ RPC_AUTH_IDENTITY_HANDLE auth_identity,
    /* [in] */ DWORD authz_protocol
)
{
    RPC_STATUS status;
    rpc_binding_set_auth_info(
            binding_h,
            server_princ_name,
            authn_level,
            authn_protocol,
            auth_identity,
            authz_protocol,
            &status);
    return LwMapDCEStatusToWinerror(status);
}

RPC_STATUS RpcBindingSetAuthInfoW(
    /* [in] */ RPC_BINDING_HANDLE binding_h,
    /* [in] */ PWSTR server_princ_name,
    /* [in] */ DWORD authn_level,
    /* [in] */ DWORD authn_protocol,
    /* [in] */ RPC_AUTH_IDENTITY_HANDLE auth_identity,
    /* [in] */ DWORD authz_protocol
)
{
    RPC_STATUS status = rpc_s_ok;
    CONVERT_INPUTSTR(server_princ_name);

    if (status == rpc_s_ok)
    {
        rpc_binding_set_auth_info(
                binding_h,
                INPUTSTR(server_princ_name),
                authn_level,
                authn_protocol,
                auth_identity,
                authz_protocol,
                &status);
    }

    FREE_INPUTSTR(server_princ_name);

    return LwMapDCEStatusToWinerror(status);
}

RPC_STATUS RpcStringFreeA(
    /* [in, out] */ PUCHAR *string
)
{
    RPC_STATUS status = rpc_s_ok;
    rpc_string_free((idl_char **)string, &status);
    return LwMapDCEStatusToWinerror(status);
}

RPC_STATUS RpcStringFreeW(
    /* [in, out] */ PWSTR *string
)
{
    //We allocated this string, not dce rpc
    if(*string != NULL)
    {
        free(*string);
        *string = NULL;
    }
    return ERROR_SUCCESS;
}

RPC_STATUS RpcBindingFree(
    /* [in, out] */ RPC_BINDING_HANDLE *binding_handle
)
{
    RPC_STATUS status = rpc_s_ok;
    rpc_binding_free(binding_handle, &status);
    return LwMapDCEStatusToWinerror(status);
}

RPC_STATUS RpcServerUseProtseqEpA(
    /* [in] */ PUCHAR protseq,
    /* [in] */ unsigned int max_call_requests,
    /* [in] */ PUCHAR endpoint,
    void *security /*not used*/
)
{
    RPC_STATUS status = rpc_s_ok;
    rpc_server_use_protseq_ep((idl_char *)protseq, max_call_requests, (idl_char *)endpoint, &status);
    return LwMapDCEStatusToWinerror(status);
}

RPC_STATUS RpcServerUseProtseqEpW(
    /* [in] */ PWSTR protseq,
    /* [in] */ unsigned int max_call_requests,
    /* [in] */ PWSTR endpoint,
    void *security /*not used*/
)
{
    RPC_STATUS status = rpc_s_ok;
    CONVERT_INPUTSTR(protseq);
    CONVERT_INPUTSTR(endpoint);

    if(status == rpc_s_ok)
    {
        rpc_server_use_protseq_ep(INPUTSTR(protseq),
                max_call_requests,
                INPUTSTR(endpoint),
                &status);
    }

    FREE_INPUTSTR(protseq);
    FREE_INPUTSTR(endpoint);

    return LwMapDCEStatusToWinerror(status);
}

RPC_STATUS RpcServerRegisterIf(
    /* [in] */ RPC_IF_HANDLE if_spec,
    /* [in] */ UUID *mgr_type_uuid,
    /* [in] */ RPC_MGR_EPV *mgr_epv
)
{
    RPC_STATUS status = rpc_s_ok;
    rpc_server_register_if(if_spec, mgr_type_uuid, mgr_epv, &status);
    return LwMapDCEStatusToWinerror(status);
}

RPC_STATUS RpcServerListen(
    unsigned32 minimum_call_threads, /*not used*/
    /* [in] */ unsigned32 max_calls_exec,
    unsigned32 dont_wait /*not used*/
)
{
    RPC_STATUS status = rpc_s_ok;
    rpc_server_listen(max_calls_exec, &status);
    return LwMapDCEStatusToWinerror(status);
}

RPC_STATUS LwMapDCEStatusToWinerror(
    RPC_STATUS dceStatus
)
{
    switch(dceStatus)
    {
        case rpc_s_ok:
            return RPC_S_OK;
        case rpc_s_op_rng_error:
            return RPC_S_PROCNUM_OUT_OF_RANGE;
        case rpc_s_not_in_call:
            return RPC_S_NO_CALL_ACTIVE;
        case rpc_s_no_port:
            return RPC_S_OUT_OF_RESOURCES;
        case rpc_s_invalid_string_binding:
            return RPC_S_INVALID_STRING_BINDING;
        case rpc_s_wrong_kind_of_binding:
            return RPC_S_WRONG_KIND_OF_BINDING;
        case rpc_s_invalid_binding:
            return RPC_S_INVALID_BINDING;
        case rpc_s_protseq_not_supported:
            return RPC_S_PROTSEQ_NOT_SUPPORTED;
        case rpc_s_invalid_rpc_protseq:
            return RPC_S_INVALID_RPC_PROTSEQ;
        case uuid_s_invalid_string_uuid:
            return RPC_S_INVALID_STRING_UUID;
        case rpc_s_invalid_endpoint_format:
            return RPC_S_INVALID_ENDPOINT_FORMAT;
        case rpc_s_inval_net_addr:
            return RPC_S_INVALID_NET_ADDR;
        case rpc_s_endpoint_not_found:
            return RPC_S_NO_ENDPOINT_FOUND;
        case rpc_s_invalid_timeout:
            return RPC_S_INVALID_TIMEOUT;
        case rpc_s_object_not_found:
            return RPC_S_OBJECT_NOT_FOUND;
        case rpc_s_already_registered:
            return RPC_S_ALREADY_REGISTERED;
        case rpc_s_type_already_registered:
            return RPC_S_TYPE_ALREADY_REGISTERED;
        case rpc_s_already_listening:
            return RPC_S_ALREADY_LISTENING;
        case rpc_s_no_protseqs_registered:
            return RPC_S_NO_PROTSEQS_REGISTERED;
        case rpc_s_not_listening:
            return RPC_S_NOT_LISTENING;
        case rpc_s_unknown_mgr_type:
            return RPC_S_UNKNOWN_MGR_TYPE;
        case rpc_s_unknown_if:
            return RPC_S_UNKNOWN_IF;
        case rpc_s_no_bindings:
            return RPC_S_NO_BINDINGS;
        case rpc_s_no_protseqs:
            return RPC_S_NO_PROTSEQS;
        case rpc_s_no_rem_endpoint:
            return RPC_S_CANT_CREATE_ENDPOINT;
        case rpc_s_connect_no_resources:
            return RPC_S_OUT_OF_RESOURCES;
        case ept_s_server_unavailable:
            return RPC_S_SERVER_UNAVAILABLE;
        case rpc_s_server_too_busy:
            return RPC_S_SERVER_TOO_BUSY;
        case rpc_s_invalid_call_opt:
            return RPC_S_INVALID_NETWORK_OPTIONS;
        case rpc_s_call_failed:
            return RPC_S_CALL_FAILED;
        case rpc_s_call_faulted:
            return RPC_S_CALL_FAILED_DNE;
        case rpc_s_protocol_error:
            return RPC_S_PROTOCOL_ERROR;
        case rpc_s_tsyntaxes_unsupported:
            return RPC_S_UNSUPPORTED_TRANS_SYN;
        case rpc_s_unsupported_type:
            return RPC_S_UNSUPPORTED_TYPE;
        case rpc_s_fault_invalid_tag:
            return RPC_S_INVALID_TAG;
        case rpc_s_fault_invalid_bound:
            return RPC_S_INVALID_BOUND;
        case rpc_s_no_entry_name:
            return RPC_S_NO_ENTRY_NAME;
        case rpc_s_invalid_name_syntax:
            return RPC_S_INVALID_NAME_SYNTAX;
        case rpc_s_unsupported_name_syntax:
            return RPC_S_UNSUPPORTED_NAME_SYNTAX;
        case uuid_s_no_address:
            return RPC_S_UUID_NO_ADDRESS;
        case ept_s_cant_access:
            return RPC_S_DUPLICATE_ENDPOINT;
        case rpc_s_unsupported_auth_subtype:
            return RPC_S_UNKNOWN_AUTHN_TYPE;
        case rpc_s_max_calls_too_small:
            return RPC_S_MAX_CALLS_TOO_SMALL;
        case rpc_s_string_too_long:
            return RPC_S_STRING_TOO_LONG;
        case rpc_s_binding_has_no_auth:
            return RPC_S_BINDING_HAS_NO_AUTH;
        case rpc_s_unknown_authn_service:
            return RPC_S_UNKNOWN_AUTHN_SERVICE;
        case rpc_s_authn_authz_mismatch:
            return RPC_S_UNKNOWN_AUTHZ_SERVICE;
        case rpc_s_nothing_to_export:
            return RPC_S_NOTHING_TO_EXPORT;
        case rpc_s_incomplete_name:
            return RPC_S_INCOMPLETE_NAME;
        case rpc_s_invalid_vers_option:
            return RPC_S_INVALID_VERS_OPTION;
        case rpc_s_no_more_members:
            return RPC_S_NO_MORE_MEMBERS;
        case rpc_s_not_all_objs_unexported:
            return RPC_S_NOT_ALL_OBJS_UNEXPORTED;
        case rpc_s_interface_not_found:
            return RPC_S_INTERFACE_NOT_FOUND;
        case rpc_s_entry_already_exists:
            return RPC_S_ENTRY_ALREADY_EXISTS;
        case rpc_s_entry_not_found:
            return RPC_S_ENTRY_NOT_FOUND;
        case rpc_s_name_service_unavailable:
            return RPC_S_NAME_SERVICE_UNAVAILABLE;
        case rpc_s_invalid_naf_id:
            return RPC_S_INVALID_NAF_ID;
        case rpc_s_not_supported:
            return RPC_S_CANNOT_SUPPORT;
        case rpc_s_context_id_not_found:
            return RPC_S_NO_CONTEXT_AVAILABLE;
        case uuid_s_internal_error:
            return RPC_S_INTERNAL_ERROR;
        case rpc_s_fault_int_div_by_zero:
            return RPC_S_ZERO_DIVIDE;
        case rpc_s_cant_bind_socket:
            return RPC_S_ADDRESS_ERROR;
        case rpc_s_fault_fp_div_by_zero:
            return RPC_S_FP_DIV_ZERO;
        case rpc_s_fault_fp_underflow:
            return RPC_S_FP_UNDERFLOW;
        case rpc_s_fault_fp_overflow:
            return RPC_S_FP_OVERFLOW;
        case rpc_s_call_queued:
            return RPC_S_CALL_IN_PROGRESS;
        case rpc_s_no_more_bindings:
            return RPC_S_NO_MORE_BINDINGS;
        case rpc_s_no_interfaces:
            return RPC_S_NO_INTERFACES;
        case rpc_s_call_cancelled:
            return RPC_S_CALL_CANCELLED;
        case rpc_s_binding_incomplete:
            return RPC_S_BINDING_INCOMPLETE;
        case rpc_s_comm_failure:
            return RPC_S_COMM_FAILURE;
        case rpc_s_auth_method:
            return RPC_S_INVALID_AUTH_IDENTITY;
        case rpc_s_unsupported_authn_level:
            return RPC_S_UNSUPPORTED_AUTHN_LEVEL;
        case rpc_s_invalid_credentials:
            return RPC_S_NO_PRINC_NAME;
        case rpc_s_unknown_error:
            return RPC_S_NOT_RPC_ERROR;
        case rpc_m_cant_create_uuid:
            return RPC_S_UUID_LOCAL_ONLY;
        case rpc_s_cancel_timeout:
            return RPC_S_NOT_CANCELLED;
        case rpc_s_group_member_not_found:
            return RPC_S_GROUP_MEMBER_NOT_FOUND;
        case rpc_s_invalid_object:
            return RPC_S_INVALID_OBJECT;
        case rpc_s_wrong_pickle_type:
            return RPC_S_ENTRY_TYPE_MISMATCH;
        case rpc_s_no_interfaces_exported:
            return RPC_S_INTERFACE_NOT_EXPORTED;
        case rpc_s_profile_not_found:
            return RPC_S_PROFILE_NOT_ADDED;

        case ept_s_invalid_entry:
            return EPT_S_INVALID_ENTRY;
        case ept_s_cant_perform_op:
            return EPT_S_CANT_PERFORM_OP;
        case ept_s_not_registered:
            return EPT_S_NOT_REGISTERED;
        case ept_s_cant_create:
            return EPT_S_CANT_CREATE;

        case uuid_s_no_memory:
        case rpc_s_no_memory:
        case ept_s_no_memory:
        case dce_cs_c_cannot_allocate_memory:
            return ERROR_OUTOFMEMORY;

        case rpc_s_rpcd_comm_failure:
        case rpc_s_cannot_connect:
            return ERROR_CONNECTION_REFUSED;
        case rpc_s_connection_aborted:
            return ERROR_CONNECTION_ABORTED;
        case rpc_s_connection_closed:
            return ERROR_GRACEFUL_DISCONNECT;
        case rpc_s_connect_timed_out:
            return WSAETIMEDOUT;
        case rpc_s_connect_rejected:
            return ERROR_CONNECTION_REFUSED;
        case rpc_s_connect_closed_by_rem:
            return WSAEDISCON;
        case rpc_s_cant_create_socket:
            return ERROR_CANNOT_MAKE;
        case rpc_s_no_name_mapping:
            return ERROR_CANNOT_MAKE;

        case rpc_s_invalid_arg:
            return RPC_S_INVALID_ARG;

        case rpc_s_auth_tkt_expired:
            return ERROR_CONTEXT_EXPIRED;

        case rpc_s_ss_in_null_context:
            return RPC_X_SS_IN_NULL_CONTEXT;

        default:
            return RPC_S_INTERNAL_ERROR;
    }
}

RPC_STATUS DceErrorInqTextA(
    unsigned32 convert_status,
    /* [out] */ UCHAR *error_text
)
{
    int dce_status = 0;

    dce_error_inq_text(convert_status, (idl_char *)error_text, &dce_status);

    return LwMapDCEStatusToWinerror(dce_status);
}

RPC_STATUS DceErrorInqTextW(
    unsigned32 convert_status,
    /* [out] */ PWSTR error_text
)
{
    int dce_status = 0;
    dce_error_string_t errstr;

    dce_error_inq_text(convert_status, (idl_char *)errstr, &dce_status);

    if (dce_status == rpc_s_ok)
    {
        mbstowc16s(error_text, errstr, DCE_C_ERROR_STRING_LEN);
    }

    return LwMapDCEStatusToWinerror(dce_status);
}
