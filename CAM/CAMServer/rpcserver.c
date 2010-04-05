#include "stdafx.h"


DWORD 
CamStartRpcServer(
    RPC_BINDING_VECTOR ** server_binding
	)
{
    RPC_STATUS status = 0;

	status = RpcServerUseProtseqEp(CAM_PROTOCOL_SEQUENCE, 
		                           RPC_C_PROTSEQ_MAX_REQS_DEFAULT, 
								   CAM_END_PORT, 
								   NULL);
	BAIL_ON_ERROR(status);
 
    status = RpcServerRegisterIf(CamSvc_v0_0_s_ifspec,
                                 NULL,   
                                 NULL); 
	BAIL_ON_ERROR(status);

	status = RpcServerInqBindings(server_binding);
	BAIL_ON_ERROR(status);
 
    status = RpcServerListen(1,
                             RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                             FALSE); 
    BAIL_ON_ERROR(status);

error:
	return status;
}




DWORD
CamStopRpcServer(
    RPC_BINDING_VECTOR * pServerBinding
	)
{
    DWORD status = 0;
    
    status = RpcEpUnregister(CamSvc_v0_0_s_ifspec,
                             pServerBinding,
                             NULL
                             );
    BAIL_ON_ERROR(status);
    
    status = RpcServerUnregisterIf (CamSvc_v0_0_s_ifspec,
                                    NULL,
                                    0
                                    );
    BAIL_ON_ERROR(status);

error:

	return status;
}

