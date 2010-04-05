#ifndef _NFSSVC_NFS_H_
#define _NFSSVC_NFS_H_


DWORD
NfsSvcRegisterForRPC(
    PSTR pszServiceName,
    rpc_binding_vector_p_t* ppServerBinding
    );

PVOID
NfsSvcListenForRPC(
    PVOID pArg
    );

DWORD
NfsSvcUnregisterForRPC(
    rpc_binding_vector_p_t pServerBinding
    );

BOOLEAN
NfsSvcRpcIsListening(
    VOID
    );

DWORD
NfsSvcRpcStopListening(
    VOID
    );

#endif /* _NFSSVC_NFS_H_ */
