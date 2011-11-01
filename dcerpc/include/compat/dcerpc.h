#if !defined(_WIN32) && !defined(DCERPC_H)
#define DCERPC_H
#include <dce/rpc.h>
#define DCETHREAD_CHECKED
#define DCETHREAD_USE_THROW
#include <dce/dcethread.h>
#include <dce/dce_error.h>

// Make sure the parent directory is in the include path
#include <compat/rpcfields.h>
#elif !defined(DCERPC_H)
#define DCERPC_H
#include <rpc.h>

// Make sure the parent directory is in the include path
#include <compat/rpcfields.h>

#define error_status_ok 0

// DCE-to-MS RPC

#define rpc_binding_free(binding, status) \
    (*(status) = RpcBindingFree(binding))

#define rpc_binding_from_string_binding(string_binding, binding, status) \
    (*(status) = RpcBindingFromStringBinding(string_binding, binding))

#define rpc_binding_set_auth_info(binding, server_name, authn_level, authn_protocol, auth_identity, authz_protocol, status) \
    (*(status) = RpcBindingSetAuthInfo(binding, server_name, authn_level, authn_protocol, auth_identity, authz_protocol))

#define rpc_binding_to_string_binding(binding, string_binding, status) \
    (*(status) = RpcBindingToStringBinding(binding, string_binding))

#define rpc_ep_register(if_handle, binding_vec, object_uuid_vec, annotation, status) \
    (*(status) = RpcEpRegister(if_handle, binding_vec, object_uuid_vec, annotation))

#define rpc_ep_resolve_binding(binding, if_handle, status) \
    (*(status) = RpcEpResolveBinding(binding, if_handle))

#define rpc_ep_unregister(if_handle, binding_vec, object_uuid_vec, status) \
    (*(status) = RpcEpUnregister(if_handle, binding_vec, object_uuid_vec))

#define rpc_server_inq_bindings(binding_vector, status) \
    (*(status) = RpcServerInqBindings(binding_vector))

// NOTE: Difference between DCE and MS
#define rpc_server_listen(max_calls_exec, status) \
    (*(status) = RpcServerListen(1, max_calls_exec, FALSE))

#define rpc_server_register_auth_info(server_name, auth_svc, get_key_func, arg, status) \
    (*(status) = RpcServerRegisterAuthInfo(server_name, auth_svc, get_key_func, arg))

#define rpc_server_register_if(if_handle, mgr_type_uuid, mgr_epv, status) \
    (*(status) = RpcServerRegisterIf(if_handle, mgr_type_uuid, mgr_epv))

// NOTE: Difference between DCE and MS
// TODO: Determine what to use for WaitForCallsToComplete argument
#define rpc_server_unregister_if(if_handle, mgr_type_uuid, status) \
    (*(status) = RpcServerUnregisterIf(if_handle, mgr_type_uuid, TRUE))

// NOTE: Difference between DCE and MS
#define rpc_server_use_all_protseqs(max_call_requests, status) \
    (*(status) = RpcServerUseAllProtseqs(max_call_requests, NULL))

// NOTE: Difference between DCE and MS
#define rpc_server_use_all_protseqs_if(max_call_requests, if_handle, status) \
    (*(status) = RpcServerUseAllProtseqsIf(max_call_requests, if_handle, NULL))

// NOTE: Difference between DCE and MS
#define rpc_server_use_protseq(protseq, max_call_requests, status) \
    (*(status) = RpcServerUseProtseq(protseq, max_call_requests, NULL))

// NOTE: Difference between DCE and MS
#define rpc_server_use_protseq_ep(protseq, max_call_requests, endpoint, status) \
    (*(status) = RpcServerUseProtseqEp(protseq, max_call_requests, endpoint, NULL))

#define rpc_string_binding_compose(obj_uuid, protseq, network_addr, endpoint, options, string_binding, status) \
    (*(status) = RpcStringBindingCompose(obj_uuid, protseq, network_addr, endpoint, options, string_binding))

#define rpc_mgmt_inq_server_princ_name(binding_h, authn_svc, server_princ_name, status) \
    (*(status) = RpcMgmtInqServerPrincName(binding_h, authn_svc, server_princ_name))

#define rpc_string_binding_compose(obj_uuid, protseq, network_addr, endpoint, options, string_binding, status) \
    (*(status) = RpcStringBindingCompose(obj_uuid, protseq, network_addr, endpoint, options, string_binding))

#define rpc_string_free(string, status) \
    (*(status) = RpcStringFree(string))

#define rpc_ss_allocate RpcSsAllocate


typedef RPC_BINDING_VECTOR* rpc_binding_vector_p_t;
typedef RPC_BINDING_HANDLE rpc_binding_handle_t;
typedef RPC_IF_HANDLE rpc_if_handle_t;
typedef RPC_IF_ID rpc_if_id_t;

#define ATTRIBUTE_UNUSED

#define DCETHREAD_TRY RpcTryExcept {
#define DCETHREAD_CATCH_ALL(_exc) } RpcExcept(1) { DWORD _exc = RpcExceptionCode();
#define DCETHREAD_ENDTRY } RpcEndExcept

#define rpc_c_protseq_max_calls_default RPC_C_PROTSEQ_MAX_REQS_DEFAULT
#define rpc_c_listen_max_calls_default RPC_C_LISTEN_MAX_CALLS_DEFAULT

typedef boolean idl_boolean;

#define rpc_s_ok RPC_S_OK

typedef TCHAR dce_error_string_t[1024];
#define dce_error_inq_text(error_code, error_string, status) (*(status) = DceErrorInqText(error_code, error_string))

#define rpc_c_protect_level_default RPC_C_AUTHN_LEVEL_DEFAULT
#define rpc_c_protect_level_none RPC_C_AUTHN_LEVEL_NONE
#define rpc_c_protect_level_connect RPC_C_AUTHN_LEVEL_CONNECT
#define rpc_c_protect_level_call RPC_C_AUTHN_LEVEL_CALL
#define rpc_c_protect_level_pkt RPC_C_AUTHN_LEVEL_PKT
#define rpc_c_protect_level_pkt_integ RPC_C_AUTHN_LEVEL_PKT_INTEGRITY
#define rpc_c_protect_level_pkt_privacy RPC_C_AUTHN_LEVEL_PKT_PRIVACY

#define rpc_c_authn_none           RPC_C_AUTHN_NONE
#define rpc_c_authn_dce_secret     RPC_C_AUTHN_DCE_PRIVATE
#define rpc_c_authn_dce_public     RPC_C_AUTHN_DCE_PUBLIC
#define rpc_c_authn_dssa_public    RPC_C_AUTHN_DEC_PUBLIC
#define rpc_c_authn_gss_negotiate  RPC_C_AUTHN_GSS_NEGOTIATE
#define rpc_c_authn_winnt          RPC_C_AUTHN_WINNT
#define rpc_c_authn_gss_tls        RPC_C_AUTHN_GSS_SCHANNEL
#define rpc_c_authn_gss_mskrb      RPC_C_AUTHN_GSS_KERBEROS
#define rpc_c_authn_msn            RPC_C_AUTHN_DPA
#define rpc_c_authn_dpa            RPC_C_AUTHN_MSN
#define rpc_c_authn_default        RPC_C_AUTHN_DEFAULT

#define rpc_c_authz_none     RPC_C_AUTHZ_NONE
#define rpc_c_authz_name     RPC_C_AUTHZ_NAME
#define rpc_c_authz_dce      RPC_C_AUTHZ_DCE
#define rpc_c_authz_gss_name RPC_C_AUTHZ_DEFAULT

#endif
