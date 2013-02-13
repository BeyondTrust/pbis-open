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

#ifndef _LWRPCRT_H_
#define _LWRPCRT_H_

#include <lw/base.h>
#include <dce/idlbase.h>
#include <dce/rpc.h>
#define DCETHREAD_CHECKED
#define DCETHREAD_USE_THROW
#include <dce/dcethread.h>
#include <dce/dce_error.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned32 RPC_STATUS;
typedef handle_t RPC_BINDING_HANDLE;
typedef rpc_if_handle_t RPC_IF_HANDLE;
typedef dce_uuid_t UUID;
typedef rpc_mgr_proc_t RPC_MGR_EPV;
typedef idl_ushort_int *RPC_WSTR;
typedef rpc_auth_identity_handle_t RPC_AUTH_IDENTITY_HANDLE;

#define RPC_C_PROTSEQ_MAX_REQS_DEFAULT rpc_c_protseq_max_reqs_default
#define RPC_C_LISTEN_MAX_CALLS_DEFAULT rpc_c_listen_max_calls_default

#define RpcTryExcept	DCETHREAD_TRY
#define RpcExcept	DCETHREAD_CATCH_EXPR
#define RpcEndExcept	DCETHREAD_ENDTRY
#define RpcExceptionCode() RpcCompatExceptionToCode(DCETHREAD_EXC_CURRENT)

RPC_STATUS RpcCompatExceptionToCode(dcethread_exc *exc);
typedef RPC_STATUS (*RpcCompatReturnCodeFuncPtr)();
RpcCompatReturnCodeFuncPtr RpcCompatReturnLater(RPC_STATUS value);

//User programs put this inside of their type
#define __RPC_USER

RPC_STATUS RpcStringBindingComposeA(
    /* [in] */ UCHAR *string_object_uuid,
    /* [in] */ UCHAR *string_protseq,
    /* [in] */ UCHAR *string_netaddr,
    /* [in] */ UCHAR *string_endpoint,
    /* [in] */ UCHAR *string_options,
    /* [out] */ UCHAR **string_binding
);

RPC_STATUS RpcStringBindingComposeW(
    /* [in] */ PWSTR string_object_uuid,
    /* [in] */ PWSTR string_protseq,
    /* [in] */ PWSTR string_netaddr,
    /* [in] */ PWSTR string_endpoint,
    /* [in] */ PWSTR string_options,
    /* [out] */ PWSTR *string_binding
);

RPC_STATUS RpcBindingFromStringBindingA(
    /* [in] */ UCHAR *string_binding,
    /* [out] */ RPC_BINDING_HANDLE *binding_handle
);

RPC_STATUS RpcBindingFromStringBindingW(
    /* [in] */ PWSTR string_binding,
    /* [out] */ RPC_BINDING_HANDLE *binding_handle
);

RPC_STATUS RpcBindingSetAuthInfoA(
    /* [in] */ RPC_BINDING_HANDLE binding_h,
    /* [in] */ UCHAR* server_princ_name,
    /* [in] */ DWORD authn_level,
    /* [in] */ DWORD authn_protocol,
    /* [in] */ RPC_AUTH_IDENTITY_HANDLE auth_identity,
    /* [in] */ DWORD authz_protocol
);

RPC_STATUS RpcBindingSetAuthInfoW(
    /* [in] */ RPC_BINDING_HANDLE binding_h,
    /* [in] */ PWSTR server_princ_name,
    /* [in] */ DWORD authn_level,
    /* [in] */ DWORD authn_protocol,
    /* [in] */ RPC_AUTH_IDENTITY_HANDLE auth_identity,
    /* [in] */ DWORD authz_protocol
);

RPC_STATUS RpcStringFreeA(
    /* [in, out] */ PUCHAR *string
);

RPC_STATUS RpcStringFreeW(
    /* [in, out] */ PWSTR *string
);

RPC_STATUS RpcBindingFree(
    /* [in, out] */ RPC_BINDING_HANDLE *binding_handle
);

RPC_STATUS RpcServerUseProtseqEpA(
    /* [in] */ PUCHAR protseq,
    /* [in] */ unsigned int max_call_requests,
    /* [in] */ PUCHAR endpoint,
    void *security /*not used*/
);
RPC_STATUS RpcServerUseProtseqEpW(
    /* [in] */ PWSTR protseq,
    /* [in] */ unsigned int max_call_requests,
    /* [in] */ PWSTR endpoint,
    void *security /*not used*/
);

RPC_STATUS RpcServerRegisterIf(
    /* [in] */ RPC_IF_HANDLE if_spec,
    /* [in] */ UUID *mgr_type_uuid,
    /* [in] */ RPC_MGR_EPV *mgr_epv
);

RPC_STATUS RpcServerListen(
    unsigned32 minimum_call_threads, /*not used*/
    /* [in] */ unsigned32 max_calls_exec,
    unsigned32 dont_wait /*not used*/
);

RPC_STATUS LwMapDCEStatusToWinerror(
    RPC_STATUS dceStatus
);

RPC_STATUS DceErrorInqTextA(
    unsigned32 status,
    /* [out] */ UCHAR *error_text
);

RPC_STATUS DceErrorInqTextW(
    unsigned32 status,
    /* [out] */ PWSTR error_text
);

#define RpcStringBindingCompose RpcStringBindingComposeA
#define RpcServerUseProtseqEp RpcServerUseProtseqEpA
#define RpcBindingFromStringBinding RpcBindingFromStringBindingA
#define RpcStringFree RpcStringFreeA
#define RpcBindingSetAuthInfo RpcBindingSetAuthInfoA
#define RpcSsDestroyClientContext(x) rpc_ss_destroy_client_context((rpc_ss_context_t *)x)

#define RPC_C_AUTHN_LEVEL_DEFAULT rpc_c_protect_level_default
#define RPC_C_AUTHN_LEVEL_NONE rpc_c_protect_level_none
#define RPC_C_AUTHN_LEVEL_CONNECT rpc_c_protect_level_connect
#define RPC_C_AUTHN_LEVEL_CALL rpc_c_protect_level_call
#define RPC_C_AUTHN_LEVEL_PKT rpc_c_protect_level_pkt
#define RPC_C_AUTHN_LEVEL_PKT_INTEGRITY rpc_c_protect_level_pkt_integ
#define RPC_C_AUTHN_LEVEL_PKT_PRIVACY rpc_c_protect_level_pkt_privacy

#define RPC_C_AUTHN_GSS_NEGOTIATE   rpc_c_authn_gss_negotiate

#define RPC_C_AUTHZ_NAME    rpc_c_authz_name

#define DCE_C_ERROR_STRING_LEN  dce_c_error_string_len

#ifdef __cplusplus
} //extern C
#endif

#endif // _LWRPCRT_H_
