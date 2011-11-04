#include <rpc.h>

// Make sure the parent directory is in the include path
#include <compat/rpcfields.h>

#define error_status_ok 0

// DCE-to-MS RPC

#define rpc_binding_free(binding, status) \
    (*(status) = RpcBindingFree(binding))

#define rpc_binding_from_string_binding(string_binding, binding, status) \
    (*(status) = RpcBindingFromStringBinding(string_binding, binding))

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

#define rpc_string_free(string, status) \
    (*(status) = RpcStringFree(string))

#define rpc_ss_allocate RpcSsAllocate


typedef RPC_BINDING_VECTOR* rpc_binding_vector_p_t;
typedef RPC_BINDING_HANDLE rpc_binding_handle_t;
typedef RPC_IF_HANDLE rpc_if_handle_t;
typedef RPC_IF_ID rpc_if_id_t;

#define ATTRIBUTE_UNUSED

#define TRY RpcTryExcept
#define CATCH_ALL RpcExcept(1)
#define ENDTRY RpcEndExcept

#define rpc_c_protseq_max_calls_default RPC_C_PROTSEQ_MAX_REQS_DEFAULT
#define rpc_c_listen_max_calls_default RPC_C_LISTEN_MAX_CALLS_DEFAULT

typedef boolean idl_boolean;

#define rpc_s_ok RPC_S_OK

typedef TCHAR dce_error_string_t[1024];
#define dce_error_inq_text(error_code, error_string, status) (*(status) = DceErrorInqText(error_code, error_string))
